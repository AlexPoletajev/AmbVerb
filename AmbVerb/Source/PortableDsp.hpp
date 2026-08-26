#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cstddef>
#include <cstring>
#include <utility>

#include "BufferStorage.hpp"

// Stores the non-negative half of a real FFT as interleaved real/imaginary
// values, including independent DC and Nyquist bins.
class SpectrumBuffer
{
public:
    void allocate(std::size_t newFftSize)
    {
        fftSize = newFftSize;
        numBins = fftSize / 2 + 1;
        interleaved.allocate(numBins * 2);
        clear();
    }

    void clear() noexcept
    {
        interleaved.clear();
    }

    void swapWith(SpectrumBuffer& other) noexcept
    {
        interleaved.swapWith(other.interleaved);
        std::swap(fftSize, other.fftSize);
        std::swap(numBins, other.numBins);
    }

    [[nodiscard]] std::size_t getFftSize() const noexcept { return fftSize; }
    [[nodiscard]] std::size_t getNumBins() const noexcept { return numBins; }
    [[nodiscard]] std::size_t getNumFloats() const noexcept { return numBins * 2; }

    [[nodiscard]] float* data() noexcept { return interleaved.get(); }
    [[nodiscard]] const float* data() const noexcept { return interleaved.get(); }

    [[nodiscard]] float& real(std::size_t bin) noexcept { return interleaved[2 * bin]; }
    [[nodiscard]] const float& real(std::size_t bin) const noexcept
    {
        return interleaved[2 * bin];
    }

    [[nodiscard]] float& imag(std::size_t bin) noexcept { return interleaved[2 * bin + 1]; }
    [[nodiscard]] const float& imag(std::size_t bin) const noexcept
    {
        return interleaved[2 * bin + 1];
    }

private:
    FloatBuffer interleaved;
    std::size_t fftSize = 0;
    std::size_t numBins = 0;
};

// JUCE normalises its real FFT API across platform backends. The legacy AmbVerb
// code used the raw vDSP scaling instead: forward spectra were doubled and an
// inverse transform was N times larger. Keeping those conventions here allows
// the existing convolution gains to remain unchanged during the portability
// refactor.
class PortableRealFft
{
public:
    explicit PortableRealFft(int order)
        : fft(order), fftSize(std::size_t { 1 } << order), workspace(2 * fftSize)
    {
        workspace.clear();
    }

    void forwardVdspCompatible(const float* input, SpectrumBuffer& output) noexcept
    {
        jassert(output.getFftSize() == fftSize);
        workspace.clear();
        std::memcpy(workspace.get(), input, fftSize * sizeof(float));
        fft.performRealOnlyForwardTransform(workspace.get(), true);
        juce::FloatVectorOperations::copyWithMultiply(output.data(),
                                                      workspace.get(),
                                                      2.0f,
                                                      output.getNumFloats());
    }

    void inverseVdspCompatible(const SpectrumBuffer& input, float* output) noexcept
    {
        jassert(input.getFftSize() == fftSize);
        workspace.clear();
        std::memcpy(workspace.get(), input.data(), input.getNumFloats() * sizeof(float));
        fft.performRealOnlyInverseTransform(workspace.get());
        juce::FloatVectorOperations::copyWithMultiply(output,
                                                      workspace.get(),
                                                      static_cast<float>(fftSize),
                                                      fftSize);
    }

    static void multiply(const SpectrumBuffer& left,
                         const SpectrumBuffer& right,
                         SpectrumBuffer& destination) noexcept
    {
        jassert(left.getFftSize() == right.getFftSize()
                && left.getFftSize() == destination.getFftSize());

        for (std::size_t bin = 0; bin < left.getNumBins(); ++bin) {
            const float leftReal = left.real(bin);
            const float leftImag = left.imag(bin);
            const float rightReal = right.real(bin);
            const float rightImag = right.imag(bin);
            destination.real(bin) = leftReal * rightReal - leftImag * rightImag;
            destination.imag(bin) = leftReal * rightImag + leftImag * rightReal;
        }
    }

    static void add(const SpectrumBuffer& source, SpectrumBuffer& destination) noexcept
    {
        jassert(source.getFftSize() == destination.getFftSize());
        juce::FloatVectorOperations::add(destination.data(),
                                         source.data(),
                                         source.getNumFloats());
    }

    static void copy(const SpectrumBuffer& source, SpectrumBuffer& destination) noexcept
    {
        jassert(source.getFftSize() == destination.getFftSize());
        juce::FloatVectorOperations::copy(destination.data(),
                                          source.data(),
                                          source.getNumFloats());
    }

private:
    juce::dsp::FFT fft;
    std::size_t fftSize;
    FloatBuffer workspace;
};
