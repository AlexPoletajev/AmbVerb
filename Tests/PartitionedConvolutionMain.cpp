#include "PartitionedConvolution.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
constexpr int numInputs = 2;
constexpr int numOutputs = 3;
constexpr int maximumBlockSize = 257;
constexpr int signalLength = 2048;
constexpr int impulseLength = 777;
constexpr int renderLength = signalLength + impulseLength - 1;

struct Route
{
    int input = 0;
    int output = 0;
    std::vector<float> impulse;
};

std::vector<float> makeImpulse(int firstSample, int spacing, float amplitude)
{
    std::vector<float> impulse(impulseLength, 0.0f);

    for (int sample = firstSample; sample < impulseLength; sample += spacing) {
        impulse[static_cast<std::size_t>(sample)] = amplitude
            * std::exp(-0.004f * static_cast<float>(sample));
        amplitude = -0.91f * amplitude;
    }

    return impulse;
}

std::array<std::vector<float>, numInputs> makeInput()
{
    std::array<std::vector<float>, numInputs> input;

    for (int channel = 0; channel < numInputs; ++channel) {
        input[static_cast<std::size_t>(channel)].resize(renderLength, 0.0f);

        for (int sample = 0; sample < signalLength; ++sample) {
            input[static_cast<std::size_t>(channel)][static_cast<std::size_t>(sample)]
                = 0.08f * std::sin(0.013f * static_cast<float>(sample * (channel + 1)))
                + 0.03f * std::cos(0.031f * static_cast<float>(sample + 7 * channel));
        }
    }

    return input;
}

std::array<std::vector<float>, numOutputs> directConvolution(
    const std::array<std::vector<float>, numInputs>& input,
    const std::vector<Route>& routes)
{
    std::array<std::vector<float>, numOutputs> output;

    for (auto& channel : output)
        channel.resize(renderLength, 0.0f);

    for (const auto& route : routes) {
        for (int sample = 0; sample < renderLength; ++sample) {
            double value = 0.0;
            const int firstImpulseSample = std::max(0, sample - signalLength + 1);
            const int lastImpulseSample = std::min(sample, impulseLength - 1);

            for (int impulseSample = firstImpulseSample;
                 impulseSample <= lastImpulseSample;
                 ++impulseSample) {
                value += static_cast<double>(
                    input[static_cast<std::size_t>(route.input)]
                         [static_cast<std::size_t>(sample - impulseSample)])
                    * route.impulse[static_cast<std::size_t>(impulseSample)];
            }

            output[static_cast<std::size_t>(route.output)]
                  [static_cast<std::size_t>(sample)] += static_cast<float>(value);
        }
    }

    return output;
}

std::array<std::vector<float>, numOutputs> render(
    PartitionedConvolutionBank& bank,
    const std::array<std::vector<float>, numInputs>& input,
    const std::vector<int>& blockPattern)
{
    std::array<std::vector<float>, numOutputs> output;

    for (auto& channel : output)
        channel.resize(renderLength, 0.0f);

    int position = 0;
    std::size_t patternPosition = 0;

    while (position < renderLength) {
        const int blockSize = std::min(blockPattern[patternPosition % blockPattern.size()],
                                       renderLength - position);
        std::array<const float*, numInputs> inputPointers {};
        std::array<float*, numOutputs> outputPointers {};

        for (int channel = 0; channel < numInputs; ++channel) {
            inputPointers[static_cast<std::size_t>(channel)]
                = input[static_cast<std::size_t>(channel)].data() + position;
        }

        for (int channel = 0; channel < numOutputs; ++channel) {
            outputPointers[static_cast<std::size_t>(channel)]
                = output[static_cast<std::size_t>(channel)].data() + position;
        }

        bank.process(inputPointers.data(), outputPointers.data(), blockSize);
        position += blockSize;
        ++patternPosition;
    }

    return output;
}

bool compare(const std::array<std::vector<float>, numOutputs>& actual,
             const std::array<std::vector<float>, numOutputs>& expected,
             const char* description)
{
    double maximumError = 0.0;
    double errorEnergy = 0.0;
    double referenceEnergy = 0.0;

    for (int channel = 0; channel < numOutputs; ++channel) {
        for (int sample = 0; sample < renderLength; ++sample) {
            const double reference = expected[static_cast<std::size_t>(channel)]
                                             [static_cast<std::size_t>(sample)];
            const double error = static_cast<double>(
                actual[static_cast<std::size_t>(channel)][static_cast<std::size_t>(sample)])
                - reference;
            maximumError = std::max(maximumError, std::abs(error));
            errorEnergy += error * error;
            referenceEnergy += reference * reference;
        }
    }

    const double relativeRms = std::sqrt(errorEnergy
        / std::max(referenceEnergy, std::numeric_limits<double>::min()));
    const bool passed = maximumError <= 2.0e-5 && relativeRms <= 2.0e-5;
    std::cout << description << ": max abs " << maximumError
              << ", relative RMS " << relativeRms
              << (passed ? " (passed)\n" : " (FAILED)\n");
    return passed;
}
}

int main()
{
    const auto input = makeInput();
    const std::vector<Route> routes {
        { 0, 0, makeImpulse(0, 23, 0.31f) },
        { 1, 0, makeImpulse(11, 37, -0.19f) },
        { 0, 1, makeImpulse(3, 41, 0.23f) },
        { 1, 2, makeImpulse(29, 19, -0.17f) }
    };
    const auto expected = directConvolution(input, routes);
    PartitionedConvolutionBank bank(48000.0,
                                    maximumBlockSize,
                                    numInputs,
                                    numOutputs);

    for (const auto& route : routes) {
        bank.addRoute(route.input,
                      route.output,
                      route.impulse.data(),
                      route.impulse.size());
    }

    bank.prepare();
    const auto variable = render(bank,
                                 input,
                                 { 1, 17, 64, 3, 256, 31, 127, 257, 5, 89 });
    const bool variablePassed = compare(variable, expected, "variable callbacks");

    bank.reset();
    const auto fixed = render(bank, input, { maximumBlockSize });
    const bool fixedPassed = compare(fixed, expected, "fixed callbacks");
    return variablePassed && fixedPassed ? 0 : 1;
}
