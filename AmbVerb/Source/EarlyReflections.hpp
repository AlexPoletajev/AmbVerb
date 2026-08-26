#pragma once

#include <juce_core/juce_core.h>

#include <vecLib/vDSP.h>

#include <atomic>
#include <string_view>

#include "BufferStorage.hpp"
#include "CompilationFlags.h"

class EarlyRef
{
public:
    EarlyRef();
    ~EarlyRef();

    void processBlock(const float* const block[], int dspBlockSize, double sampleRate);
    void reset();

    void set_Q(float value);
    void set_RotAngle(float value);
    void set_EarlyrefVolume(float value);

    [[nodiscard]] int getQ() const noexcept { return Q; }

    bool readTransformationMatrices(std::string_view rxz,
                                    std::string_view rzx,
                                    std::string_view ryz,
                                    std::string_view rzy);

    // Activates a completely calculated matrix. Safe to call from the audio
    // thread; it never waits for an in-progress background calculation.
    void UnlockRotationMatrixForCalculaion();

    FloatBuffer OutBuffer[NumAmbisonicsChannels];
    FloatBuffer Output[NumAmbisonicsChannels];
    int EarlyrefDelayTime = 0;
    int EndOfIR = 0;
    int IRsymmetryPoint = 0;
    std::atomic<float> FilterCoeffA { 1.0f };
    std::atomic<float> FilterCoeffB { 0.0f };
    int OnsetLength = 0;

private:
    float h(float alpha, float beta, int lambda);
    void fillhBuffer(float phi, int offset, int q);
    void CalculateRotationMatrices(int qValue, float phiValue);
    void CalculateRx();
    void CalculateRy();
    void CalculateRz(int qValue, float phiValue);
    void CalculateRxyz();
    void CheckforNonZeroEntriesX();
    void CheckforNonZeroEntriesY();
    void CheckforNonZeroEntriesZ();
    void CheckforNonZeroEntriesXYZ();
    void MatrixConvolution();
    void FFTconvolution(float* signal,
                        DSPSplitComplex* impulseResponse,
                        float* convolutionBuffer);
    float LowPass(float* input,
                  double gain,
                  double pole,
                  float initialSample,
                  float* filterBuffer,
                  int blockSize);

    float earlyrefVolume = 0.7f;
    int Q = Qmax;
    int Q_TEMP = Qmax;
    int Phi = 80;

    FloatBuffer InBuffer[NumAmbisonicsChannels];
    FloatBuffer DummyMatrix1[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer Rx[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer Ry[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer Rz10[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer Rz13[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer Rz19[NumAmbisonicsChannels][NumAmbisonicsChannels];
    float Rxz[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    float Rzx[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    float Ryz[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    float Rzy[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    FloatBuffer Rxyz[NumAmbisonicsChannels][NumAmbisonicsChannels];
    FloatBuffer hBuffer[(AmbisonicsOrder * 2 + 1) * 2];
    float Filter_InitialSample[NumAmbisonicsChannels] {};
    FloatBuffer FilterBuffer;

    int NonZeroEntriesX[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    int NonZeroEntriesY[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    int NonZeroEntriesZ[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    int NonZeroEntriesXYZ[NumAmbisonicsChannels][NumAmbisonicsChannels] {};
    int NonZeroEntriesXYZ_TEMP[NumAmbisonicsChannels][NumAmbisonicsChannels] {};

    OwnedSplitComplex fft_Rxyz[NumAmbisonicsChannels][NumAmbisonicsChannels];
    OwnedSplitComplex fft_Rxyz_TEMP[NumAmbisonicsChannels][NumAmbisonicsChannels];
    OwnedSplitComplex fft_Rx[NumAmbisonicsChannels][NumAmbisonicsChannels];
    OwnedSplitComplex fft_Ry[NumAmbisonicsChannels][NumAmbisonicsChannels];
    OwnedSplitComplex fft_Rz[NumAmbisonicsChannels][NumAmbisonicsChannels];
    OwnedSplitComplex SplitComplexBuffer1;
    OwnedSplitComplex SplitComplexBuffer2;
    FloatBuffer FFTconvBuffer1;
    FFTSetup fftConvSetup = nullptr;
    float fftScale = 0.0f;

    juce::SpinLock matrixLock;
    std::atomic<bool> matrixReady { false };
};
