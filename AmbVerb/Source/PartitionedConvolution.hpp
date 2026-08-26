#pragma once

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#include "BufferStorage.hpp"

// Zero-latency, uniformly partitioned MIMO convolution. All routes share the
// input FFTs and each output is transformed back only once. Calls may contain
// any number of samples up to the maximum block size supplied at construction.
class PartitionedConvolutionBank
{
public:
    PartitionedConvolutionBank(double sampleRateIn,
                               int maximumBlockSizeIn,
                               int numberOfInputsIn,
                               int numberOfOutputsIn)
        : sampleRate(sampleRateIn),
          maximumBlockSize(maximumBlockSizeIn),
          numberOfInputs(numberOfInputsIn),
          numberOfOutputs(numberOfOutputsIn),
          partitionSize(static_cast<std::size_t>(
              juce::nextPowerOfTwo(maximumBlockSizeIn))),
          fftSize(2 * partitionSize),
          numberOfBins(fftSize / 2 + 1),
          fft(static_cast<int>(std::log2(fftSize)))
    {
        if (sampleRate <= 0.0
            || maximumBlockSize <= 0
            || numberOfInputs <= 0
            || numberOfOutputs <= 0)
            throw std::invalid_argument("Invalid partitioned convolution configuration");
    }

    void addRoute(int inputChannel,
                  int outputChannel,
                  const float* impulseResponse,
                  std::size_t impulseResponseLength,
                  float gain = 1.0f)
    {
        if (prepared
            || inputChannel < 0
            || inputChannel >= numberOfInputs
            || outputChannel < 0
            || outputChannel >= numberOfOutputs
            || impulseResponse == nullptr
            || impulseResponseLength == 0)
            throw std::invalid_argument("Invalid partitioned convolution route");

        Route route;
        route.inputChannel = inputChannel;
        route.outputChannel = outputChannel;
        const auto routePartitions = (impulseResponseLength + partitionSize - 1)
            / partitionSize;
        route.impulseSpectra.reserve(routePartitions);

        for (std::size_t partition = 0; partition < routePartitions; ++partition) {
            FloatBuffer spectrum(2 * fftSize);
            FloatBuffer transformWorkspace(2 * fftSize);
            spectrum.clear();
            transformWorkspace.clear();
            const auto inputOffset = partition * partitionSize;
            const auto samplesToCopy = std::min(partitionSize,
                                                impulseResponseLength - inputOffset);
            std::memcpy(transformWorkspace.get(),
                        impulseResponse + inputOffset,
                        samplesToCopy * sizeof(float));

            juce::FloatVectorOperations::multiply(transformWorkspace.get(),
                                                   gain,
                                                   static_cast<int>(samplesToCopy));

            fft.performRealOnlyForwardTransform(transformWorkspace.get(), true);
            convertInterleavedToSplit(transformWorkspace, spectrum);
            route.impulseSpectra.push_back(std::move(spectrum));
        }

        numberOfPartitions = std::max(numberOfPartitions, routePartitions);
        routes.push_back(std::move(route));
    }

    void prepare()
    {
        numberOfPartitions = std::max<std::size_t>(1, numberOfPartitions);
        inputTime.resize(static_cast<std::size_t>(numberOfInputs));
        inputTransformWorkspace.resize(static_cast<std::size_t>(numberOfInputs));
        inputHistory.resize(static_cast<std::size_t>(numberOfInputs));

        for (int input = 0; input < numberOfInputs; ++input) {
            inputTime[static_cast<std::size_t>(input)].allocate(2 * fftSize);
            inputTransformWorkspace[static_cast<std::size_t>(input)].allocate(2 * fftSize);
            auto& history = inputHistory[static_cast<std::size_t>(input)];
            history.reserve(numberOfPartitions);

            for (std::size_t partition = 0;
                 partition < numberOfPartitions;
                 ++partition)
                history.emplace_back(2 * fftSize);
        }

        outputSpectrum.resize(static_cast<std::size_t>(numberOfOutputs));
        tailSpectrum.resize(static_cast<std::size_t>(numberOfOutputs));
        outputTime.resize(static_cast<std::size_t>(numberOfOutputs));
        overlap.resize(static_cast<std::size_t>(numberOfOutputs));

        for (int output = 0; output < numberOfOutputs; ++output) {
            outputSpectrum[static_cast<std::size_t>(output)].allocate(2 * fftSize);
            tailSpectrum[static_cast<std::size_t>(output)].allocate(2 * fftSize);
            outputTime[static_cast<std::size_t>(output)].allocate(2 * fftSize);
            overlap[static_cast<std::size_t>(output)].allocate(partitionSize);
        }

        prepared = true;
        reset();
    }

    void reset() noexcept
    {
        for (auto& buffer : inputTime)
            buffer.clear();

        for (auto& buffer : inputTransformWorkspace)
            buffer.clear();

        for (auto& channelHistory : inputHistory)
            for (auto& buffer : channelHistory)
                buffer.clear();

        for (auto* buffers : { &outputSpectrum, &tailSpectrum, &outputTime, &overlap })
            for (auto& buffer : *buffers)
                buffer.clear();

        currentPartition = 0;
        inputPosition = 0;
    }

    void process(const float* const inputs[],
                 float* const outputs[],
                 int numSamples) noexcept
    {
        jassert(prepared
                && inputs != nullptr
                && outputs != nullptr
                && numSamples > 0
                && numSamples <= maximumBlockSize);

        if (!prepared
            || inputs == nullptr
            || outputs == nullptr
            || numSamples <= 0
            || numSamples > maximumBlockSize)
            return;

        for (int output = 0; output < numberOfOutputs; ++output)
            juce::FloatVectorOperations::clear(outputs[output], numSamples);

        std::size_t samplesProcessed = 0;
        const auto requestedSamples = static_cast<std::size_t>(numSamples);

        while (samplesProcessed < requestedSamples) {
            const bool startsNewPartition = inputPosition == 0;
            const auto samplesThisTime = std::min(requestedSamples - samplesProcessed,
                                                  partitionSize - inputPosition);

            transformInputs(inputs, samplesProcessed, samplesThisTime);

            if (startsNewPartition)
                calculateTailSpectra();

            calculateCurrentOutputSpectra();

            for (int output = 0; output < numberOfOutputs; ++output) {
                const auto outputIndex = static_cast<std::size_t>(output);
                auto& time = outputTime[outputIndex];
                convertSplitToInterleaved(outputSpectrum[outputIndex], time);
                fft.performRealOnlyInverseTransform(time.get());
                juce::FloatVectorOperations::add(outputs[output] + samplesProcessed,
                                                  time.get() + inputPosition,
                                                  overlap[outputIndex].get() + inputPosition,
                                                  static_cast<int>(samplesThisTime));
            }

            inputPosition += samplesThisTime;
            samplesProcessed += samplesThisTime;

            if (inputPosition == partitionSize)
                finishPartition();
        }
    }

    [[nodiscard]] std::size_t getNumRoutes() const noexcept { return routes.size(); }
    [[nodiscard]] std::size_t getPartitionSize() const noexcept { return partitionSize; }

private:
    struct Route
    {
        int inputChannel = 0;
        int outputChannel = 0;
        std::vector<FloatBuffer> impulseSpectra;
    };

    void transformInputs(const float* const inputs[],
                         std::size_t sourceOffset,
                         std::size_t numSamples) noexcept
    {
        for (int input = 0; input < numberOfInputs; ++input) {
            auto& time = inputTime[static_cast<std::size_t>(input)];
            std::memcpy(time.get() + inputPosition,
                        inputs[input] + sourceOffset,
                        numSamples * sizeof(float));
            auto& workspace = inputTransformWorkspace[static_cast<std::size_t>(input)];
            std::memcpy(workspace.get(), time.get(), 2 * fftSize * sizeof(float));
            fft.performRealOnlyForwardTransform(workspace.get(), true);
            auto& spectrum = inputHistory[static_cast<std::size_t>(input)]
                                              [currentPartition];
            convertInterleavedToSplit(workspace, spectrum);
        }
    }

    void calculateTailSpectra() noexcept
    {
        for (auto& spectrum : tailSpectrum)
            spectrum.clear();

        for (const auto& route : routes) {
            const auto routePartitions = route.impulseSpectra.size();

            for (std::size_t partition = 1;
                 partition < routePartitions;
                 ++partition) {
                const auto historyPartition = (currentPartition + partition)
                    % numberOfPartitions;
                multiplyAndAccumulate(
                    inputHistory[static_cast<std::size_t>(route.inputChannel)]
                                [historyPartition],
                    route.impulseSpectra[partition],
                    tailSpectrum[static_cast<std::size_t>(route.outputChannel)]);
            }
        }
    }

    void calculateCurrentOutputSpectra() noexcept
    {
        for (int output = 0; output < numberOfOutputs; ++output) {
            auto& destination = outputSpectrum[static_cast<std::size_t>(output)];
            std::memcpy(destination.get(),
                        tailSpectrum[static_cast<std::size_t>(output)].get(),
                        2 * numberOfBins * sizeof(float));
        }

        for (const auto& route : routes) {
            multiplyAndAccumulate(
                inputHistory[static_cast<std::size_t>(route.inputChannel)]
                            [currentPartition],
                route.impulseSpectra.front(),
                outputSpectrum[static_cast<std::size_t>(route.outputChannel)]);
        }
    }

    void finishPartition() noexcept
    {
        for (int output = 0; output < numberOfOutputs; ++output) {
            const auto outputIndex = static_cast<std::size_t>(output);
            std::memcpy(overlap[outputIndex].get(),
                        outputTime[outputIndex].get() + partitionSize,
                        partitionSize * sizeof(float));
        }

        for (auto& time : inputTime)
            time.clear();

        inputPosition = 0;
        currentPartition = currentPartition == 0
            ? numberOfPartitions - 1
            : currentPartition - 1;
    }

    void multiplyAndAccumulate(const FloatBuffer& input,
                               const FloatBuffer& impulse,
                               FloatBuffer& output) noexcept
    {
        const int vectorSize = static_cast<int>(numberOfBins);
        const auto imaginaryOffset = numberOfBins;
        juce::FloatVectorOperations::addWithMultiply(output.get(),
                                                      input.get(),
                                                      impulse.get(),
                                                      vectorSize);
        juce::FloatVectorOperations::subtractWithMultiply(output.get(),
                                                           input.get() + imaginaryOffset,
                                                           impulse.get() + imaginaryOffset,
                                                           vectorSize);
        juce::FloatVectorOperations::addWithMultiply(output.get() + imaginaryOffset,
                                                      input.get(),
                                                      impulse.get() + imaginaryOffset,
                                                      vectorSize);
        juce::FloatVectorOperations::addWithMultiply(output.get() + imaginaryOffset,
                                                      input.get() + imaginaryOffset,
                                                      impulse.get(),
                                                      vectorSize);
    }

    void convertInterleavedToSplit(const FloatBuffer& interleaved,
                                   FloatBuffer& split) noexcept
    {
        for (std::size_t bin = 0; bin < numberOfBins; ++bin) {
            split[bin] = interleaved[2 * bin];
            split[numberOfBins + bin] = interleaved[2 * bin + 1];
        }
    }

    void convertSplitToInterleaved(const FloatBuffer& split,
                                   FloatBuffer& interleaved) noexcept
    {
        interleaved.clear();

        for (std::size_t bin = 0; bin < numberOfBins; ++bin) {
            interleaved[2 * bin] = split[bin];
            interleaved[2 * bin + 1] = split[numberOfBins + bin];
        }
    }

    double sampleRate = 0.0;
    int maximumBlockSize = 0;
    int numberOfInputs = 0;
    int numberOfOutputs = 0;
    std::size_t partitionSize = 0;
    std::size_t fftSize = 0;
    std::size_t numberOfBins = 0;
    juce::dsp::FFT fft;
    std::vector<Route> routes;
    std::vector<FloatBuffer> inputTime;
    std::vector<FloatBuffer> inputTransformWorkspace;
    std::vector<std::vector<FloatBuffer>> inputHistory;
    std::vector<FloatBuffer> outputSpectrum;
    std::vector<FloatBuffer> tailSpectrum;
    std::vector<FloatBuffer> outputTime;
    std::vector<FloatBuffer> overlap;
    std::size_t numberOfPartitions = 0;
    std::size_t currentPartition = 0;
    std::size_t inputPosition = 0;
    bool prepared = false;
};
