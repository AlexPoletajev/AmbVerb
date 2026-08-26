#pragma once

#include <juce_core/juce_core.h>

#include <vecLib/vDSP.h>

#include <atomic>
#include <cmath>

#include "BufferStorage.hpp"
#include "CompilationFlags.h"

inline const float maxGain = 0.999999f / std::sqrt(static_cast<float>(NumDelaylines));

class FDN
{
public:
    FDN();
    ~FDN();

    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void processBlock(const float* block, int dspBlockSize, double sampleRate);

    void setParameters(float minDelay,
                       float maxDelay,
                       float t60,
                       float t60Ratio,
                       int windowStart,
                       int windowEnd);
    void setT60(float t60);
    void setT60Ratio(float t60Ratio);
    void setDelayTimes(float min, float max);
    void set_Volume(float value);
    void setWindowBoundries(int start, int end);
    void getPendingFilterCoefficients(float& gain, float& pole);

    [[nodiscard]] int getMinDelaytime() const noexcept { return minDelaytime; }
    [[nodiscard]] int getMaxDelaytime() const noexcept { return maxDelaytime; }

    FloatBuffer Output[NumAmbisonicsChannels];
    float T60 = 2.0f;
    float T60Ratio = 0.25f;
    double Samplerate = 44100.0;
    float fdnVol = 0.7f;
    double p[NumDelaylines + 1] {};
    double a0[NumDelaylines + 1] {};

private:
    void FeedbackMatrix_Multiplikation(float* input,
                                       float* rightEnd[],
                                       float* delayPoint[],
                                       int blockSize);
    float LowPass(float* input,
                  double gain,
                  double pole,
                  float initialSample,
                  float* filterBuffer,
                  int blockSize);
    void getIR();
    void getWindowedOutput(int blockSize);
    void windowIR();
    void setFilterCoefficients();
    void unlockParameters();
    void refreshWindow();
    void setDelayTimesUnchecked(float min, float max);

    FloatBuffer inBuffer;
    FloatBuffer Delayline_leftEnd[NumDelaylines];
    float* Delayline_rightEnd[NumDelaylines] {};
    float* Delayline_delayPoint[NumDelaylines] {};

    FloatBuffer IR_Delayline_leftEnd[NumDelaylines];
    float* IR_Delayline_rightEnd[NumDelaylines] {};
    float* IR_Delayline_delayPoint[NumDelaylines] {};

    FloatBuffer IR_inBuffer;
    FloatBuffer IR[NumDelaylines];
    FloatBuffer TempBuffer[NumDelaylines];
    FloatBuffer IR_TempBuffer1;
    FloatBuffer EarlyReflectionsBuffer[NumDelaylines];
    FloatBuffer Window;

    int FeedbackMarix[NumDelaylines][NumDelaylines] {};
    int DelayTimes[NumDelaylines] {};
    int DelayTimes_temp[NumDelaylines] {};

    double R0[NumDelaylines + 1] {};
    double Rpi[NumDelaylines + 1] {};
    double p_temp[NumDelaylines + 1] {};
    double a0_temp[NumDelaylines + 1] {};

    float Filter_initialSample[2 * NumDelaylines] {};
    float IR_Filter_initialSample[NumDelaylines] {};
    FloatBuffer FilterBuffer;
    FloatBuffer IR_FilterBuffer;

    int minDelaytime = MinRoomsize;
    int maxDelaytime = MaxRoomsize;
    int Primnumbers[794] {};
    int CustomBlocksize = 0;
    int tMixStart = 0;
    int tMixEnd = 0;

    std::atomic<bool> unlockParamtersOnOff { false };
    juce::SpinLock parameterLock;

    FFTSetup fftSetup = nullptr;
    OwnedSplitComplex fft_IR[NumDelaylines];
    OwnedSplitComplex fft_Delaylines[NumDelaylines];
    OwnedSplitComplex fft_IR_temp[NumDelaylines];
    OwnedSplitComplex fft_Input;
    float fftScale = 0.0f;
};
