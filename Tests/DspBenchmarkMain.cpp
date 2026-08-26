#include "PluginProcessor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

struct Options
{
    int warmupBlocks = 4;
    int measuredBlocks = 24;
    juce::File jsonOutput;
};

bool parsePositiveInteger(const char* text, int& value)
{
    const auto parsed = juce::String(text).getIntValue();

    if (parsed <= 0)
        return false;

    value = parsed;
    return true;
}

bool parseOptions(int argc, char* argv[], Options& options)
{
    for (int i = 1; i < argc; ++i) {
        const juce::String argument(argv[i]);

        if (argument == "--warmup" && i + 1 < argc) {
            if (! parsePositiveInteger(argv[++i], options.warmupBlocks))
                return false;
        } else if (argument == "--blocks" && i + 1 < argc) {
            if (! parsePositiveInteger(argv[++i], options.measuredBlocks))
                return false;
        } else if (argument == "--json" && i + 1 < argc) {
            options.jsonOutput = juce::File::getCurrentWorkingDirectory()
                                     .getChildFile(argv[++i]);
        } else {
            return false;
        }
    }

    return true;
}

bool setParameter(AmbVerbAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.getParameterState().getParameter(id);

    if (parameter == nullptr)
        return false;

    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    return true;
}

bool setBenchmarkParameters(AmbVerbAudioProcessor& processor)
{
    return setParameter(processor, ParameterIDs::earlyRefVolume, 0.82f)
        && setParameter(processor, ParameterIDs::fdnVolume, 0.76f)
        && setParameter(processor, ParameterIDs::reverbVolume, 0.9f)
        && setParameter(processor, ParameterIDs::distance, 0.32f)
        && setParameter(processor, ParameterIDs::roomSize, 1700.0f)
        && setParameter(processor, ParameterIDs::fdnT60, 3.2f)
        && setParameter(processor, ParameterIDs::fdnT60Ratio, 0.18f);
}

void fillInput(juce::AudioBuffer<float>& buffer, int blockIndex)
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* samples = buffer.getWritePointer(channel);
        const double frequency = 0.0007 * static_cast<double>(channel + 1);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            const auto absoluteSample = blockIndex * buffer.getNumSamples() + sample;
            samples[sample] = static_cast<float>(
                0.01 * std::sin(twoPi * frequency * static_cast<double>(absoluteSample)));
        }
    }
}

double percentile(std::vector<double> values, double fraction)
{
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(std::ceil(
        fraction * static_cast<double>(values.size() - 1)));
    return values[index];
}

juce::var benchmarkCase(AmbVerbAudioProcessor& processor,
                        double sampleRate,
                        int blockSize,
                        const Options& options)
{
    const auto prepareStart = Clock::now();
    processor.prepareToPlay(sampleRate, blockSize);
    const auto prepareEnd = Clock::now();

    juce::AudioBuffer<float> buffer(NumAmbisonicsChannels, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < options.warmupBlocks; ++block) {
        fillInput(buffer, block);
        processor.processBlock(buffer, midi);
    }

    std::vector<double> blockTimes;
    blockTimes.reserve(static_cast<std::size_t>(options.measuredBlocks));

    for (int block = 0; block < options.measuredBlocks; ++block) {
        fillInput(buffer, block + options.warmupBlocks);
        const auto start = Clock::now();
        processor.processBlock(buffer, midi);
        const auto end = Clock::now();
        blockTimes.push_back(
            std::chrono::duration<double, std::micro>(end - start).count());
    }

    processor.releaseResources();

    const double mean = std::accumulate(blockTimes.begin(), blockTimes.end(), 0.0)
        / static_cast<double>(blockTimes.size());
    const double blockBudget = 1.0e6 * static_cast<double>(blockSize) / sampleRate;
    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty("sampleRate", sampleRate);
    result->setProperty("blockSize", blockSize);
    result->setProperty("prepareUs",
                        std::chrono::duration<double, std::micro>(prepareEnd - prepareStart)
                            .count());
    result->setProperty("meanUs", mean);
    result->setProperty("p50Us", percentile(blockTimes, 0.50));
    result->setProperty("p95Us", percentile(blockTimes, 0.95));
    result->setProperty("p99Us", percentile(blockTimes, 0.99));
    result->setProperty("maxUs", *std::max_element(blockTimes.begin(), blockTimes.end()));
    result->setProperty("realtimeLoad", mean / blockBudget);
    return juce::var(result.release());
}

juce::String platformName()
{
   #if JUCE_MAC
    return "macOS";
   #elif JUCE_WINDOWS
    return "Windows";
   #elif JUCE_LINUX
    return "Linux";
   #else
    return "Unknown";
   #endif
}
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    Options options;

    if (! parseOptions(argc, argv, options)) {
        std::cerr << "Usage: AmbVerbDspBenchmark [--warmup N] [--blocks N] [--json FILE]\n";
        return 2;
    }

    const auto constructorStart = Clock::now();
    auto processor = std::make_unique<AmbVerbAudioProcessor>();
    const auto constructorEnd = Clock::now();

    if (! setBenchmarkParameters(*processor)) {
        std::cerr << "Could not configure benchmark parameters\n";
        return 1;
    }

    auto report = std::make_unique<juce::DynamicObject>();
    report->setProperty("schemaVersion", 1);
    report->setProperty("platform", platformName());
    report->setProperty("operatingSystem", juce::SystemStats::getOperatingSystemName());
    report->setProperty("cpu", juce::SystemStats::getCpuModel());
    report->setProperty("ambisonicsOrder", AmbisonicsOrder);
    report->setProperty("channels", NumAmbisonicsChannels);
    report->setProperty("fftSize", static_cast<int>(earlyref_Buffersize));
    report->setProperty("warmupBlocks", options.warmupBlocks);
    report->setProperty("measuredBlocks", options.measuredBlocks);
    report->setProperty("constructorUs",
                        std::chrono::duration<double, std::micro>(constructorEnd - constructorStart)
                            .count());

    juce::Array<juce::var> cases;

    for (const int blockSize : { 64, 128, 256, 512, 1024 })
        cases.add(benchmarkCase(*processor, 48000.0, blockSize, options));

    report->setProperty("cases", cases);
    const auto json = juce::JSON::toString(juce::var(report.release()), true);
    std::cout << json << '\n';

    if (options.jsonOutput != juce::File()) {
        if (! options.jsonOutput.getParentDirectory().createDirectory()) {
            std::cerr << "Could not create "
                      << options.jsonOutput.getParentDirectory().getFullPathName() << '\n';
            return 1;
        }

        if (! options.jsonOutput.replaceWithText(json)) {
            std::cerr << "Could not write " << options.jsonOutput.getFullPathName() << '\n';
            return 1;
        }
    }

    return 0;
}
