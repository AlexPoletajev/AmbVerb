#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    AmbVerbAudioProcessor processor;

    if (processor.getTotalNumInputChannels() != 2
        || processor.getTotalNumOutputChannels() != 2) {
        std::cerr << "Expected a stereo input and output bus\n";
        return 1;
    }

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;
    constexpr int blockCount = 32;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    double absoluteOutputSum = 0.0;

    for (int block = 0; block < blockCount; ++block) {
        buffer.clear();

        if (block == 0) {
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 0.5f);
        }

        processor.processBlock(buffer, midi);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const auto* samples = buffer.getReadPointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (!std::isfinite(samples[sample])) {
                    std::cerr << "Non-finite output sample\n";
                    return 2;
                }

                absoluteOutputSum += std::abs(samples[sample]);
            }
        }
    }

    processor.releaseResources();

    if (absoluteOutputSum <= 0.0) {
        std::cerr << "Stereo processor produced silence\n";
        return 3;
    }

    std::cout << "Stereo processor passed: output sum="
              << absoluteOutputSum << '\n';
    return 0;
}
