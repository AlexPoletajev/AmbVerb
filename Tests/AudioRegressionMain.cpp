#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

namespace
{
constexpr double absoluteTolerance = 5.0e-6;
constexpr double relativeTolerance = 5.0e-4;

enum class Stimulus
{
    wImpulse,
    directionalImpulse,
    multichannelImpulses
};

struct Parameters
{
    float earlyRefVolume = 0.7f;
    float fdnVolume = 0.7f;
    float reverbVolume = 1.0f;
    float distance = 0.1f;
    float roomSize = static_cast<float>(MinRoomsize);
    float t60 = 2.0f;
    float t60Ratio = 0.25f;
};

struct RegressionCase
{
    const char* name;
    double sampleRate;
    int blockSize;
    int numBlocks;
    Stimulus stimulus;
    Parameters parameters;
};

struct Comparison
{
    bool passed = false;
    double peakReference = 0.0;
    double rmsReference = 0.0;
    double maximumAbsoluteError = 0.0;
    double rmsError = 0.0;
    double relativeRmsError = 0.0;
    juce::String failure;
};

const std::vector<RegressionCase>& regressionCases()
{
    static const std::vector<RegressionCase> cases {
        { "direct_only_w_48k_b257",
          48000.0,
          257,
          40,
          Stimulus::wImpulse,
          { 0.0f, 0.0f, 0.0f, 0.15f, 1200.0f, 2.0f, 0.25f } },
        { "early_only_directional_48k_b512",
          48000.0,
          512,
          32,
          Stimulus::directionalImpulse,
          { 1.0f, 0.0f, 1.0f, 1.0f, 1500.0f, 1.5f, 0.6f } },
        { "late_only_w_44k_b1024",
          44100.0,
          1024,
          24,
          Stimulus::wImpulse,
          { 0.0f, 1.0f, 1.0f, 1.0f, 1000.0f, 0.8f, 0.8f } },
        { "combined_large_dark_96k_b1024",
          96000.0,
          1024,
          32,
          Stimulus::multichannelImpulses,
          { 0.82f, 0.76f, 0.9f, 0.32f, 2000.0f, 4.5f, 0.08f } },
    };

    return cases;
}

juce::String stimulusName(Stimulus stimulus)
{
    switch (stimulus) {
        case Stimulus::wImpulse: return "w-impulse";
        case Stimulus::directionalImpulse: return "channel-7-impulse";
        case Stimulus::multichannelImpulses: return "time-separated-multichannel-impulses";
    }

    return "unknown";
}

bool setParameter(AmbVerbAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.getParameterState().getParameter(id);

    if (parameter == nullptr)
        return false;

    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    return true;
}

bool applyParameters(AmbVerbAudioProcessor& processor, const Parameters& parameters)
{
    return setParameter(processor, ParameterIDs::earlyRefVolume, parameters.earlyRefVolume)
        && setParameter(processor, ParameterIDs::fdnVolume, parameters.fdnVolume)
        && setParameter(processor, ParameterIDs::reverbVolume, parameters.reverbVolume)
        && setParameter(processor, ParameterIDs::distance, parameters.distance)
        && setParameter(processor, ParameterIDs::roomSize, parameters.roomSize)
        && setParameter(processor, ParameterIDs::fdnT60, parameters.t60)
        && setParameter(processor, ParameterIDs::fdnT60Ratio, parameters.t60Ratio);
}

void addStimulus(juce::AudioBuffer<float>& block, Stimulus stimulus, int absoluteStartSample)
{
    const auto addImpulse = [&block, absoluteStartSample](int channel,
                                                          int absoluteSample,
                                                          float amplitude) {
        const int sampleInBlock = absoluteSample - absoluteStartSample;

        if (juce::isPositiveAndBelow(channel, block.getNumChannels())
            && juce::isPositiveAndBelow(sampleInBlock, block.getNumSamples()))
            block.addSample(channel, sampleInBlock, amplitude);
    };

    switch (stimulus) {
        case Stimulus::wImpulse:
            addImpulse(0, 0, 0.5f);
            break;
        case Stimulus::directionalImpulse:
            addImpulse(7, 0, 0.5f);
            break;
        case Stimulus::multichannelImpulses:
            addImpulse(0, 0, 0.5f);
            addImpulse(3, 37, -0.25f);
            addImpulse(7, 211, 0.125f);
            addImpulse(15, 997, -0.0625f);
            break;
    }
}

bool containsOnlyFiniteSamples(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite(buffer.getSample(channel, sample)))
                return false;

    return true;
}

double getPeakMagnitude(const juce::AudioBuffer<float>& buffer)
{
    double peak = 0.0;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = std::max(peak,
                        static_cast<double>(buffer.getMagnitude(channel,
                                                                0,
                                                                buffer.getNumSamples())));

    return peak;
}

bool render(AmbVerbAudioProcessor& processor,
            const RegressionCase& testCase,
            juce::AudioBuffer<float>& output,
            juce::String& error)
{
    processor.releaseResources();

    if (! applyParameters(processor, testCase.parameters)) {
        error = "could not set one or more plug-in parameters";
        return false;
    }

    processor.prepareToPlay(testCase.sampleRate, testCase.blockSize);

    if (processor.getTotalNumInputChannels() != NumAmbisonicsChannels
        || processor.getTotalNumOutputChannels() != NumAmbisonicsChannels) {
        error = "processor is not configured for 16-channel third-order Ambisonics";
        return false;
    }

    const int totalSamples = testCase.blockSize * testCase.numBlocks;
    output.setSize(NumAmbisonicsChannels, totalSamples, false, true, false);
    output.clear();

    juce::AudioBuffer<float> block(NumAmbisonicsChannels, testCase.blockSize);
    juce::MidiBuffer midi;

    for (int start = 0; start < totalSamples; start += testCase.blockSize) {
        block.clear();
        addStimulus(block, testCase.stimulus, start);
        processor.processBlock(block, midi);

        for (int channel = 0; channel < NumAmbisonicsChannels; ++channel)
            output.copyFrom(channel,
                            start,
                            block,
                            channel,
                            0,
                            testCase.blockSize);
    }

    if (! containsOnlyFiniteSamples(output)) {
        error = "render contains NaN or infinity";
        return false;
    }

    if (getPeakMagnitude(output) <= std::numeric_limits<float>::min()) {
        error = "render is silent";
        return false;
    }

    return true;
}

bool writeWaveFile(const juce::File& file,
                   const juce::AudioBuffer<float>& audio,
                   double sampleRate,
                   juce::String& error)
{
    if (! file.getParentDirectory().createDirectory()) {
        error = "could not create output directory: " + file.getParentDirectory().getFullPathName();
        return false;
    }

    if (file.existsAsFile() && ! file.deleteFile()) {
        error = "could not replace output file: " + file.getFullPathName();
        return false;
    }

    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();

    if (stream == nullptr) {
        error = "could not create output file: " + file.getFullPathName();
        return false;
    }

    juce::WavAudioFormat format;
    const auto options = juce::AudioFormatWriterOptions {}
                             .withSampleRate(sampleRate)
                             .withNumChannels(audio.getNumChannels())
                             .withBitsPerSample(32)
                             .withSampleFormat(
                                 juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    auto writer = format.createWriterFor(stream, options);

    if (writer == nullptr) {
        error = "could not create 32-bit float WAV writer for: " + file.getFullPathName();
        return false;
    }

    if (! writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples())) {
        error = "could not write audio data to: " + file.getFullPathName();
        return false;
    }

    return true;
}

bool readWaveFile(const juce::File& file,
                  juce::AudioBuffer<float>& audio,
                  double& sampleRate,
                  juce::String& error)
{
    if (! file.existsAsFile()) {
        error = "reference file is missing: " + file.getFullPathName();
        return false;
    }

    juce::WavAudioFormat format;
    auto stream = file.createInputStream();

    if (stream == nullptr) {
        error = "could not open reference file: " + file.getFullPathName();
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));

    if (reader == nullptr) {
        error = "reference is not a readable WAV file: " + file.getFullPathName();
        return false;
    }

    if (reader->numChannels != NumAmbisonicsChannels
        || reader->lengthInSamples <= 0
        || reader->lengthInSamples > std::numeric_limits<int>::max()) {
        error = "reference has an unexpected channel count or length: " + file.getFullPathName();
        return false;
    }

    sampleRate = reader->sampleRate;
    audio.setSize(static_cast<int>(reader->numChannels),
                  static_cast<int>(reader->lengthInSamples),
                  false,
                  true,
                  false);

    if (! reader->read(&audio, 0, audio.getNumSamples(), 0, true, true)) {
        error = "could not read samples from: " + file.getFullPathName();
        return false;
    }

    return true;
}

Comparison compare(const juce::AudioBuffer<float>& actual,
                   double actualSampleRate,
                   const juce::AudioBuffer<float>& reference,
                   double referenceSampleRate)
{
    Comparison result;

    if (std::abs(actualSampleRate - referenceSampleRate) >= 0.5) {
        result.failure = "sample rate differs";
        return result;
    }

    if (actual.getNumChannels() != reference.getNumChannels()
        || actual.getNumSamples() != reference.getNumSamples()) {
        result.failure = "channel count or sample count differs";
        return result;
    }

    double sumSquaredReference = 0.0;
    double sumSquaredError = 0.0;
    const auto valueCount = static_cast<double>(actual.getNumChannels())
        * static_cast<double>(actual.getNumSamples());

    for (int channel = 0; channel < actual.getNumChannels(); ++channel) {
        for (int sample = 0; sample < actual.getNumSamples(); ++sample) {
            const double expected = reference.getSample(channel, sample);
            const double received = actual.getSample(channel, sample);

            if (! std::isfinite(expected) || ! std::isfinite(received)) {
                result.failure = "reference or render contains NaN or infinity";
                return result;
            }

            const double difference = received - expected;
            result.peakReference = std::max(result.peakReference, std::abs(expected));
            result.maximumAbsoluteError = std::max(result.maximumAbsoluteError,
                                                   std::abs(difference));
            sumSquaredReference += expected * expected;
            sumSquaredError += difference * difference;
        }
    }

    result.rmsReference = std::sqrt(sumSquaredReference / valueCount);
    result.rmsError = std::sqrt(sumSquaredError / valueCount);
    result.relativeRmsError = result.rmsError / std::max(result.rmsReference, 1.0e-12);

    const double permittedPeakError = absoluteTolerance
        + relativeTolerance * result.peakReference;
    const double permittedRmsError = absoluteTolerance * 0.1
        + relativeTolerance * result.rmsReference;

    result.passed = result.maximumAbsoluteError <= permittedPeakError
        && result.rmsError <= permittedRmsError;

    if (! result.passed)
        result.failure = "audio difference exceeds the configured tolerance";

    return result;
}

juce::var parametersToVar(const Parameters& parameters)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty(ParameterIDs::earlyRefVolume, parameters.earlyRefVolume);
    object->setProperty(ParameterIDs::fdnVolume, parameters.fdnVolume);
    object->setProperty(ParameterIDs::reverbVolume, parameters.reverbVolume);
    object->setProperty(ParameterIDs::distance, parameters.distance);
    object->setProperty(ParameterIDs::roomSize, parameters.roomSize);
    object->setProperty(ParameterIDs::fdnT60, parameters.t60);
    object->setProperty(ParameterIDs::fdnT60Ratio, parameters.t60Ratio);
    return juce::var(object.release());
}

juce::var caseToVar(const RegressionCase& testCase)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("name", testCase.name);
    object->setProperty("file", juce::String(testCase.name) + ".wav");
    object->setProperty("sampleRate", testCase.sampleRate);
    object->setProperty("blockSize", testCase.blockSize);
    object->setProperty("samples", testCase.blockSize * testCase.numBlocks);
    object->setProperty("stimulus", stimulusName(testCase.stimulus));
    object->setProperty("parameters", parametersToVar(testCase.parameters));
    return juce::var(object.release());
}

bool writeJson(const juce::File& file, const juce::var& value, juce::String& error)
{
    if (! file.getParentDirectory().createDirectory()
        || ! file.replaceWithText(juce::JSON::toString(value, true))) {
        error = "could not write JSON file: " + file.getFullPathName();
        return false;
    }

    return true;
}

juce::var makeManifest()
{
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty("formatVersion", 1);
    root->setProperty("description", "AmbVerb pre-portability audio regression reference");
    root->setProperty("ambisonicsOrder", AmbisonicsOrder);
    root->setProperty("channels", NumAmbisonicsChannels);
    root->setProperty("absoluteTolerance", absoluteTolerance);
    root->setProperty("relativeTolerance", relativeTolerance);

    juce::Array<juce::var> cases;

    for (const auto& testCase : regressionCases())
        cases.add(caseToVar(testCase));

    root->setProperty("cases", cases);
    return juce::var(root.release());
}

void printUsage()
{
    std::cout
        << "Usage:\n"
        << "  AmbVerbAudioRegression --capture <reference-directory>\n"
        << "  AmbVerbAudioRegression --compare <reference-directory> "
           "[--artifacts <output-directory>]\n";
}
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    juce::File captureDirectory;
    juce::File referenceDirectory;
    juce::File artifactsDirectory;

    for (int index = 1; index < argc; ++index) {
        const juce::String argument = juce::String::fromUTF8(argv[index]);

        if ((argument == "--capture" || argument == "--compare" || argument == "--artifacts")
            && index + 1 < argc) {
            const juce::File directory(juce::String::fromUTF8(argv[++index]));

            if (argument == "--capture")
                captureDirectory = directory;
            else if (argument == "--compare")
                referenceDirectory = directory;
            else
                artifactsDirectory = directory;
        } else {
            printUsage();
            return 2;
        }
    }

    const bool capture = captureDirectory != juce::File {};
    const bool compareAgainstReference = referenceDirectory != juce::File {};

    if (capture == compareAgainstReference) {
        printUsage();
        return 2;
    }

    if (compareAgainstReference
        && ! referenceDirectory.getChildFile("manifest.json").existsAsFile()) {
        std::cerr << "Reference manifest is missing: "
                  << referenceDirectory.getChildFile("manifest.json").getFullPathName()
                  << '\n';
        return 1;
    }

    auto processor = std::make_unique<AmbVerbAudioProcessor>();
    auto report = std::make_unique<juce::DynamicObject>();
    juce::Array<juce::var> reportCases;
    bool allPassed = true;

    for (const auto& testCase : regressionCases()) {
        juce::AudioBuffer<float> actual;
        juce::String error;
        auto caseReport = std::make_unique<juce::DynamicObject>();
        caseReport->setProperty("name", testCase.name);

        std::cout << "Rendering " << testCase.name << "..." << std::flush;

        if (! render(*processor, testCase, actual, error)) {
            std::cout << " failed\n";
            std::cerr << error << '\n';
            caseReport->setProperty("passed", false);
            caseReport->setProperty("failure", error);
            reportCases.add(juce::var(caseReport.release()));
            allPassed = false;
            continue;
        }

        if (capture) {
            const auto outputFile = captureDirectory.getChildFile(
                juce::String(testCase.name) + ".wav");

            if (! writeWaveFile(outputFile, actual, testCase.sampleRate, error)) {
                std::cout << " failed\n";
                std::cerr << error << '\n';
                caseReport->setProperty("passed", false);
                caseReport->setProperty("failure", error);
                reportCases.add(juce::var(caseReport.release()));
                allPassed = false;
                continue;
            }

            caseReport->setProperty("passed", true);
            caseReport->setProperty("peak", getPeakMagnitude(actual));
            std::cout << " captured\n";
        } else {
            juce::AudioBuffer<float> reference;
            double referenceSampleRate = 0.0;
            const auto referenceFile = referenceDirectory.getChildFile(
                juce::String(testCase.name) + ".wav");

            if (! readWaveFile(referenceFile, reference, referenceSampleRate, error)) {
                std::cout << " failed\n";
                std::cerr << error << '\n';
                caseReport->setProperty("passed", false);
                caseReport->setProperty("failure", error);
                reportCases.add(juce::var(caseReport.release()));
                allPassed = false;
                continue;
            }

            const auto comparison = compare(actual,
                                            testCase.sampleRate,
                                            reference,
                                            referenceSampleRate);
            caseReport->setProperty("passed", comparison.passed);
            caseReport->setProperty("peakReference", comparison.peakReference);
            caseReport->setProperty("rmsReference", comparison.rmsReference);
            caseReport->setProperty("maximumAbsoluteError", comparison.maximumAbsoluteError);
            caseReport->setProperty("rmsError", comparison.rmsError);
            caseReport->setProperty("relativeRmsError", comparison.relativeRmsError);

            if (! comparison.failure.isEmpty())
                caseReport->setProperty("failure", comparison.failure);

            if (! artifactsDirectory.getFullPathName().isEmpty())
                allPassed = writeWaveFile(artifactsDirectory.getChildFile(
                                              juce::String(testCase.name) + ".wav"),
                                          actual,
                                          testCase.sampleRate,
                                          error)
                    && allPassed;

            std::cout << (comparison.passed ? " passed" : " failed")
                      << " (max abs " << comparison.maximumAbsoluteError
                      << ", relative RMS " << comparison.relativeRmsError << ")\n";
            allPassed = comparison.passed && allPassed;
        }

        reportCases.add(juce::var(caseReport.release()));
    }

    if (capture) {
        juce::String error;

        if (! writeJson(captureDirectory.getChildFile("manifest.json"), makeManifest(), error)) {
            std::cerr << error << '\n';
            allPassed = false;
        }
    } else if (! artifactsDirectory.getFullPathName().isEmpty()) {
        report->setProperty("passed", allPassed);
        report->setProperty("absoluteTolerance", absoluteTolerance);
        report->setProperty("relativeTolerance", relativeTolerance);
        report->setProperty("cases", reportCases);
        juce::String error;

        if (! writeJson(artifactsDirectory.getChildFile("report.json"),
                        juce::var(report.release()),
                        error)) {
            std::cerr << error << '\n';
            allPassed = false;
        }
    }

    processor->releaseResources();
    return allPassed ? 0 : 1;
}
