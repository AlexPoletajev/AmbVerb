//
//  EarlyReflections.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include "EarlyReflections.hpp"

namespace
{
double integerBesselJ(int order, double argument) noexcept
{
    jassert(order >= 0);

    const double halfArgument = 0.5 * argument;
    double term = 1.0;

    for (int i = 1; i <= order; ++i)
        term *= halfArgument / static_cast<double>(i);

    double result = term;

    for (int k = 1; k <= 64; ++k) {
        term *= -(halfArgument * halfArgument)
            / (static_cast<double>(k) * static_cast<double>(order + k));
        result += term;

        if (std::abs(term)
            <= std::numeric_limits<double>::epsilon()
                * std::max(1.0, std::abs(result)))
            break;
    }

    return result;
}
}

EarlyRef::EarlyRef() {
    earlyrefVolume = 0.7;
    EarlyrefDelayTime = 0;
    FilterCoeffA = 1;
    FilterCoeffB = 0;

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        InBuffer[i].allocate(earlyref_Buffersize);

        if (InBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        OutBuffer[i].allocate(earlyref_Buffersize);

        if (OutBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        Output[i].allocate(earlyref_Buffersize);

        if (Output[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        Filter_InitialSample[i] = 0;
    }

    //.....fft
    FFTconvBuffer1.allocate(earlyref_Buffersize);

    if (FFTconvBuffer1 == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    FilterBuffer.allocate(earlyref_Buffersize);

    if (FilterBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    audioInputSpectrum.allocate(earlyref_Buffersize);
    matrixProductSpectrum.allocate(earlyref_Buffersize);

    fftScale = 0.5f / (float)(16.0f * earlyref_Buffersize);

    //......

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        for (int u = 0; u < NumAmbisonicsChannels; u++) {
            Rx[i][u].allocate(earlyref_Buffersize);

            if (Rx[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Ry[i][u].allocate(earlyref_Buffersize);

            if (Ry[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz10[i][u].allocate(earlyref_Buffersize);

            if (Rz10[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz13[i][u].allocate(earlyref_Buffersize);

            if (Rz13[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz19[i][u].allocate(earlyref_Buffersize);

            if (Rz19[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            DummyMatrix1[i][u].allocate(earlyref_Buffersize);

            if (DummyMatrix1[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            // CalculateRxyz applies a non-wrapping time offset before taking
            // another full FFT, so this buffer deliberately has a zero-padded
            // second half.
            Rxyz[i][u].allocate(2 * earlyref_Buffersize);

            if (Rxyz[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rx[i][u].allocate(earlyref_Buffersize);
            fft_Ry[i][u].allocate(earlyref_Buffersize);
            fft_Rz[i][u].allocate(earlyref_Buffersize);
            fft_Rxyz[i][u].allocate(earlyref_Buffersize);
            fft_Rxyz_TEMP[i][u].allocate(earlyref_Buffersize);
        }
    }

    for (int i = 0; i < (AmbisonicsOrder * 2 + 1) * 2; i++) {
        hBuffer[i].allocate(earlyref_Buffersize);

        if (hBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }
    }

    Phi = 80.0;
    Q = 400.0;
}

void EarlyRef::reset() {
    for (int i = 0; i < NumAmbisonicsChannels; ++i) {
        InBuffer[i].clear();
        OutBuffer[i].clear();
        Output[i].clear();
        Filter_InitialSample[i] = 0.0f;
    }

    FFTconvBuffer1.clear();
    FilterBuffer.clear();
}

void EarlyRef::processBlock(const float *const Block[], int DspBlocksize, double pB_Samplerate) {
    juce::ignoreUnused(pB_Samplerate);

    if (DspBlocksize <= 0 || static_cast<std::size_t>(DspBlocksize) > earlyref_Buffersize) {
        jassertfalse;
        return;
    }

    UnlockRotationMatrixForCalculaion();

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        juce::FloatVectorOperations::clear(InBuffer[i], earlyref_Buffersize);
        memcpy(InBuffer[i], Block[i], DspBlocksize * sizeof(float));
    }

    MatrixConvolution();

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        // - - - - - Ausgabe - - - - - -
        memcpy(Output[i], OutBuffer[i], DspBlocksize * sizeof(float)); //Verzögerung der Erstreflexionen
        //memcpy(Output[i], OutBuffer[i]+ (int) (EarlyrefDelayTime * Q/100.0), DspBlocksize * sizeof(float)); //Verzögerung der Erstreflexionen

        Filter_InitialSample[i] = LowPass(Output[i],
                                          FilterCoeffA.load(std::memory_order_relaxed),
                                          FilterCoeffB.load(std::memory_order_relaxed),
                                          Filter_InitialSample[i],
                                          FilterBuffer,
                                          DspBlocksize); //Lowpass Filterung
        memcpy(Output[i], FilterBuffer, DspBlocksize * sizeof(float));
        juce::FloatVectorOperations::multiply(Output[i], earlyrefVolume, DspBlocksize);

        // - - - - - Vorbereitung für den nächsten Block - - - - -
        memmove(OutBuffer[i], OutBuffer[i] + DspBlocksize, (earlyref_Buffersize - DspBlocksize) * sizeof(float));
        juce::FloatVectorOperations::clear(
            OutBuffer[i] + (earlyref_Buffersize - DspBlocksize), DspBlocksize);
    }
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Matrix Convolution
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef:: MatrixConvolution() {
    for (int m = 0; m < NumAmbisonicsChannels; m++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            if (NonZeroEntriesXYZ[m][b] == 1) {
                FFTconvolution(InBuffer[b], fft_Rxyz[m][b], FFTconvBuffer1);
                juce::FloatVectorOperations::addWithMultiply(OutBuffer[m],
                                                              FFTconvBuffer1,
                                                              fftScale,
                                                              earlyref_Buffersize);
            }
        }
    }
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// FFT Convolution of IR and IR
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef::FFTconvolution(float* signal,
                             const SpectrumBuffer& impulseResponse,
                             float* convolutionBuffer) {
    audioFft.forwardVdspCompatible(signal, audioInputSpectrum);
    PortableRealFft::multiply(audioInputSpectrum,
                              impulseResponse,
                              audioInputSpectrum);
    audioFft.inverseVdspCompatible(audioInputSpectrum, convolutionBuffer);
}

float EarlyRef:: h(float alpha, float beta, int lambda) {
    return std::cos(juce::MathConstants<float>::halfPi * std::abs(lambda) + beta)
        * static_cast<float>(integerBesselJ(std::abs(lambda),
                                            static_cast<double>(std::abs(alpha))));
}

void EarlyRef:: fillhBuffer(float phi, int offset, int Q) {
    int m, i, lambda;

    for (i = 0; i < (AmbisonicsOrder * 2 + 1) * 2; i++) {
        hBuffer[i].clear();
    }

    i = 0;

    for (m = -AmbisonicsOrder; m <= AmbisonicsOrder; m++) {
        for (lambda = 0; lambda <= offset * 2; lambda++) { // lambda = offset-1 :: Abschneiden der linksseitigen Teils
            if (m < 0) {
                *(hBuffer[i] + Q * lambda) = h(m * phi,
                                               -juce::MathConstants<float>::halfPi,
                                               lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            } else if (m > 0) {
                *(hBuffer[i] + Q * lambda) = h(m * phi, 0, lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            } else {
                *(hBuffer[i] + Q * lambda) = h(m * phi, 0, lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            }
        }

        i++;

        for (lambda = 0; lambda <= offset * 2; lambda++) {
            if (m < 0) {
                *(hBuffer[i] + Q * lambda) = h(m * phi, 0, lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            } else if (m > 0) {
                *(hBuffer[i] + Q * lambda) = h(m * phi,
                                               juce::MathConstants<float>::halfPi,
                                               lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            } else {
                *(hBuffer[i] + Q * lambda) = h(m * phi, 0, lambda - offset);
                //printf("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(hBuffer[i]+Q*lambda));
            }
        }

        i++;
    }
}

void EarlyRef:: CalculateRz(int QValue, float PhiValue) {
    int l, m, i;

    for (l = 0; l < NumAmbisonicsChannels; l++) {            // set Rz to zero
        for (m = 0; m < NumAmbisonicsChannels; m++) {
            Rz10[l][m].clear();
            Rz13[l][m].clear();
            Rz19[l][m].clear();
        }
    }

    fillhBuffer(static_cast<float>(PhiValue / 360.0
                                   * juce::MathConstants<double>::twoPi),
                Trunc,
                QValue);

    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz10[l * (l + 1) + m][l * (l + 1) + std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz10[l * (l + 1) + m][l * (l + 1) - std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
        }
    }

    fillhBuffer(static_cast<float>(PhiValue / 360.0
                                   * juce::MathConstants<double>::twoPi),
                Trunc,
                static_cast<int>(QValue * Qy));
    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz13[l * (l + 1) + m][l * (l + 1) + std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz13[l * (l + 1) + m][l * (l + 1) - std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
        }
    }

    fillhBuffer(static_cast<float>(PhiValue / 360.0
                                   * juce::MathConstants<double>::twoPi),
                Trunc,
                static_cast<int>(QValue * Qx));
    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz19[l * (l + 1) + m][l * (l + 1) + std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz19[l * (l + 1) + m][l * (l + 1) - std::abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
        }
    }

    CheckforNonZeroEntriesZ();
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Calculate Rx
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef:: CalculateRx() {
    int a, b, i;

    i = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set DummyMatrix to zero
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            DummyMatrix1[a][b].clear();
        }
    }

    // ::::: Rxz*Rz ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                //if (NonZeroEntriesZ[i][b]==1) {

                juce::FloatVectorOperations::addWithMultiply(DummyMatrix1[a][b],
                                                              Rz19[i][b],
                                                              Rxz[a][i],
                                                              earlyref_Buffersize);
                //}
            }
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Copy results to Rx
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            memcpy(Rx[a][b], DummyMatrix1[a][b], earlyref_Buffersize * sizeof(float));
        }
    }

    // ::::: (Rxz*Rz)*Rzx ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set DummyMatrix to zero
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            DummyMatrix1[a][b].clear();
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                juce::FloatVectorOperations::addWithMultiply(DummyMatrix1[a][b],
                                                              Rx[a][i],
                                                              Rzx[i][b],
                                                              earlyref_Buffersize);
            }
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Copy results to Rx
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            memcpy(Rx[a][b], DummyMatrix1[a][b], earlyref_Buffersize * sizeof(float));
        }
    }

    CheckforNonZeroEntriesX();
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Calculate Ry
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef:: CalculateRy() {
    int a, b, i;

    i = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set DummyMatrix to zero
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            DummyMatrix1[a][b].clear();
        }
    }

    // ::::: Rxz*Rz ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                juce::FloatVectorOperations::addWithMultiply(DummyMatrix1[a][b],
                                                              Rz13[i][b],
                                                              Ryz[a][i],
                                                              earlyref_Buffersize);
            }
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Copy results to Rx
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            memcpy(Ry[a][b], DummyMatrix1[a][b], earlyref_Buffersize * sizeof(float));
        }
    }

    // ::::: (Rxz*Rz)*Rzx ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set DummyMatrix to zero
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            DummyMatrix1[a][b].clear();
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                juce::FloatVectorOperations::addWithMultiply(DummyMatrix1[a][b],
                                                              Ry[a][i],
                                                              Rzy[i][b],
                                                              earlyref_Buffersize);
            }
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Copy results to Rx
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            memcpy(Ry[a][b], DummyMatrix1[a][b], earlyref_Buffersize * sizeof(float));
        }
    }

    CheckforNonZeroEntriesY();
}

void EarlyRef:: CalculateRxyz() {
    for (int a = 0; a < NumAmbisonicsChannels; ++a) {
        for (int b = 0; b < NumAmbisonicsChannels; ++b) {
            fft_Rx[a][b].clear();
            fft_Ry[a][b].clear();
            fft_Rz[a][b].clear();
            fft_Rxyz_TEMP[a][b].clear();

            matrixFft.forwardVdspCompatible(Rx[a][b], fft_Rx[a][b]);
            matrixFft.forwardVdspCompatible(Ry[a][b], fft_Ry[a][b]);
            matrixFft.forwardVdspCompatible(Rz10[a][b], fft_Rz[a][b]);
        }
    }

    for (int a = 0; a < NumAmbisonicsChannels; ++a) {
        for (int b = 0; b < NumAmbisonicsChannels; ++b) {
            for (int c = 0; c < NumAmbisonicsChannels; ++c) {
                if (NonZeroEntriesZ[a][c] == 1 && NonZeroEntriesY[c][b] == 1) {
                    PortableRealFft::multiply(fft_Rz[a][c],
                                              fft_Ry[c][b],
                                              matrixProductSpectrum);
                    PortableRealFft::add(matrixProductSpectrum, fft_Rxyz_TEMP[a][b]);
                }
            }
        }
    }

    for (int a = 0; a < NumAmbisonicsChannels; ++a)
        for (int b = 0; b < NumAmbisonicsChannels; ++b)
            fft_Rz[a][b].clear();

    for (int a = 0; a < NumAmbisonicsChannels; ++a) {
        for (int b = 0; b < NumAmbisonicsChannels; ++b) {
            for (int c = 0; c < NumAmbisonicsChannels; ++c) {
                if (NonZeroEntriesX[c][b] == 1) {
                    PortableRealFft::multiply(fft_Rx[c][b],
                                              fft_Rxyz_TEMP[a][c],
                                              matrixProductSpectrum);
                    PortableRealFft::add(matrixProductSpectrum, fft_Rz[a][b]);
                }
            }
        }
    }

    OnsetLength = static_cast<int>(Q_TEMP * (1.0f + Qx + Qy) * Trunc);

    for (int a = 0; a < NumAmbisonicsChannels; ++a) {
        for (int b = 0; b < NumAmbisonicsChannels; ++b) {
            matrixFft.inverseVdspCompatible(fft_Rz[a][b], Rxyz[a][b]);
            juce::FloatVectorOperations::multiply(Rxyz[a][b],
                                                   fftScale * 8.0f,
                                                   earlyref_Buffersize);

            const auto nominalOnsetOffset = static_cast<std::size_t>(
                juce::jmax(0, OnsetLength - 10));

            // The old vDSP path read this data as DSPComplex pairs. Preserve
            // its observed odd-sample crop phase, retaining the preceding
            // sample whenever the nominal offset is even.
            const auto onsetOffset = nominalOnsetOffset
                - static_cast<std::size_t>((nominalOnsetOffset & 1U) == 0U);
            jassert(onsetOffset + earlyref_Buffersize <= Rxyz[a][b].size());
            matrixFft.forwardVdspCompatible(Rxyz[a][b] + onsetOffset, fft_Rz[a][b]);
        }
    }

    for (int a = 0; a < NumAmbisonicsChannels; ++a)
        for (int b = 0; b < NumAmbisonicsChannels; ++b)
            PortableRealFft::copy(fft_Rz[a][b], fft_Rxyz_TEMP[a][b]);

    CheckforNonZeroEntriesXYZ();
}

void EarlyRef::CheckforNonZeroEntriesX() {
    int a, b, i, o;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) { //reset Checkzeros
            NonZeroEntriesX[a][b] = 0;
        }
    }

    o = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i <= (Trunc * 2 + 1); i++) {
                if (fabsf(*(Rx[a][b] + i * (int)(Q_TEMP * Qx))) >= 0.000001) {
                    NonZeroEntriesX[a][b] = 1;
                    //printf("Rx%i%i : %f\n", a,b,*(Rx[a][b]+i*(int)(Q_TEMP*Qx)));
                    break;
                }
            }
        }
    }
}

void EarlyRef::CheckforNonZeroEntriesY() {
    int a, b, i, o;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) { //reset Checkzeros
            NonZeroEntriesY[a][b] = 0;
        }
    }

    o = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i <= (Trunc * 2 + 1); i++) {
                if (fabsf(*(Ry[a][b] + i * (int)(Q_TEMP * Qy))) >= 0.000001) {
                    NonZeroEntriesY[a][b] = 1;
                    //printf("Ry%i%i : %f\n", a,b,*(Ry[a][b]+i*(int)(Q_TEMP*Qy)));

                    break;
                }
            }
        }
    }
}

void EarlyRef::CheckforNonZeroEntriesZ() {
    int a, b, i, o;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) { //reset Checkzeros
            NonZeroEntriesZ[a][b] = 0;
        }
    }

    o = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i <= (Trunc * 2 + 1); i++) {
                if (fabsf(*(Rz10[a][b] + i * Q_TEMP)) >= 0.000001) {
                    NonZeroEntriesZ[a][b] = 1;
                    //printf("Rz%i%i : %f\n", a,b,*(Rz10[a][b]+i*(int)(Q_TEMP)));

                    break;
                }
            }
        }
    }
}

void EarlyRef::CheckforNonZeroEntriesXYZ() {
    int a, b, i, o;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) { //reset Checkzeros
            NonZeroEntriesXYZ_TEMP[a][b] = 0;
        }
    }

    o = 0;

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < static_cast<int>(fft_Rxyz_TEMP[a][b].getNumBins()); i++) {
                if ((std::abs(fft_Rxyz_TEMP[a][b].real(static_cast<std::size_t>(i)))
                     >= 0.000001f)
                    || (std::abs(fft_Rxyz_TEMP[a][b].imag(static_cast<std::size_t>(i)))
                        >= 0.000001f)) {
                    NonZeroEntriesXYZ_TEMP[a][b] = 1;
                    break;
                }
            }
        }
    }
}

void EarlyRef:: CalculateRotationMatrices(int QValue, float PhiValue) {
    Q_TEMP = QValue;
    CalculateRz(QValue, PhiValue);
    CalculateRx();
    CalculateRy();
    CalculateRxyz();
}

void EarlyRef::  UnlockRotationMatrixForCalculaion() {
    if (!matrixReady.load(std::memory_order_acquire))
        return;

    const juce::SpinLock::ScopedTryLockType lock(matrixLock);

    if (!lock.isLocked())
        return;

    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            fft_Rxyz_TEMP[a][b].swapWith(fft_Rxyz[a][b]);
            NonZeroEntriesXYZ[a][b] = NonZeroEntriesXYZ_TEMP[a][b];
        }
    }

    Q = Q_TEMP;
    matrixReady.store(false, std::memory_order_release);
}

bool EarlyRef::readTransformationMatrices(std::string_view rxz,
                                          std::string_view rzx,
                                          std::string_view ryz,
                                          std::string_view rzy) {
    auto parse = [](std::string_view text,
                    float (&target)[NumAmbisonicsChannels][NumAmbisonicsChannels]) {
        std::istringstream input { std::string { text } };

        for (auto& row : target) {
            for (auto& value : row) {
                if (!(input >> value))
                    return false;
            }
        }

        float unexpectedValue = 0.0f;
        return !(input >> unexpectedValue);
    };

    return parse(rxz, Rxz)
        && parse(rzx, Rzx)
        && parse(ryz, Ryz)
        && parse(rzy, Rzy);
}

float EarlyRef::LowPass(float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float *LP_FilterBuffer, const int LP_Blocksize) {
    *LP_FilterBuffer = (*LP_Input) * LP_g + LP_initialSample * LP_p;

    for (int n = 1; n < LP_Blocksize; n++) {
        *(LP_FilterBuffer + n) =  *(LP_Input + n) * LP_g  + *(LP_FilterBuffer + n - 1) * LP_p;
    }

    return *(LP_FilterBuffer + LP_Blocksize - 1);
}

void EarlyRef:: set_RotAngle(float f) {
    const juce::SpinLock::ScopedLockType lock(matrixLock);
    Phi = static_cast<int>(std::lround(f));
    CalculateRotationMatrices(Q, f);
    matrixReady.store(true, std::memory_order_release);
}

void EarlyRef:: set_Q(float f) {
    if (f < 1.0f || f > static_cast<float>(Qmax)) {
        return;
    }

    const juce::SpinLock::ScopedLockType lock(matrixLock);
    CalculateRotationMatrices(f, Phi);
    matrixReady.store(true, std::memory_order_release);
}

void EarlyRef:: set_EarlyrefVolume(float Value) {
    Value = juce::jlimit(0.0f, 1.0f, Value);

    if (Value == 0) {
        earlyrefVolume = 0;
    } else {
        earlyrefVolume = 0.01 * exp(4.605170 * Value);
    }
}
