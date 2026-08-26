//
//  EarlyReflections.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include "EarlyReflections.hpp"

#include <vecLib/cblas.h>

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

    SplitComplexBuffer1.allocate(earlyref_Buffersize / 2);

    if (SplitComplexBuffer1.realp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    if (SplitComplexBuffer1.imagp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    SplitComplexBuffer2.allocate(earlyref_Buffersize / 2);

    if (SplitComplexBuffer2.realp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    if (SplitComplexBuffer2.imagp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    fftScale = 0.5f / (float)(16.0f * earlyref_Buffersize);
    fftConvSetup = vDSP_create_fftsetup(earlyref_Log2N, FFT_RADIX2);

    if (fftConvSetup == nullptr)
        throw std::runtime_error("Unable to create the early-reflections FFT setup");

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

            fft_Rx[i][u].allocate(earlyref_Buffersize / 2);

            if (fft_Rx[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            if (fft_Rx[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Ry[i][u].allocate(earlyref_Buffersize / 2);

            if (fft_Ry[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            if (fft_Ry[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rz[i][u].allocate(earlyref_Buffersize / 2);

            if (fft_Rz[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            if (fft_Rz[i][u].imagp  == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz[i][u].allocate(earlyref_Buffersize / 2);

            if (fft_Rxyz[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            if (fft_Rxyz[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz_TEMP[i][u].allocate(earlyref_Buffersize / 2);

            if (fft_Rxyz_TEMP[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            if (fft_Rxyz_TEMP[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }
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

EarlyRef::~EarlyRef() {
    if (fftConvSetup != nullptr)
        vDSP_destroy_fftsetup(fftConvSetup);
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
        vDSP_vclr(InBuffer[i], 1, earlyref_Buffersize);
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
        vDSP_vsmul(Output[i], 1, &earlyrefVolume, Output[i], 1, DspBlocksize);

        // - - - - - Vorbereitung für den nächsten Block - - - - -
        memmove(OutBuffer[i], OutBuffer[i] + DspBlocksize, (earlyref_Buffersize - DspBlocksize) * sizeof(float));
        vDSP_vclr(OutBuffer[i] + (earlyref_Buffersize - DspBlocksize), 1, DspBlocksize);
    }
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Matrix Convolution
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef:: MatrixConvolution() {
    for (int m = 0; m < NumAmbisonicsChannels; m++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            if (NonZeroEntriesXYZ[m][b] == 1) {
                FFTconvolution(InBuffer[b], &fft_Rxyz[m][b], FFTconvBuffer1);
                vDSP_vsma(FFTconvBuffer1, 1, &fftScale, OutBuffer[m], 1, OutBuffer[m], 1, earlyref_Buffersize);
            }
        }
    }
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// FFT Convolution of IR and IR
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void EarlyRef:: FFTconvolution(float *Signal1, DSPSplitComplex *Signal2, float *FFTConvolutionBuffer) {
    float NyquistBit;
    const float impulseNyquist = Signal2->imagp[0];

    // - - - - - Reinterpret Input as SplitComplex - - - - - //
    vDSP_ctoz((DSPComplex *)Signal1, 2, &SplitComplexBuffer1, 1, earlyref_Buffersize / 2);

    // - - - - - fft transformation - - - - - //
    vDSP_fft_zrip(fftConvSetup, &SplitComplexBuffer1, 1, earlyref_Log2N, FFT_FORWARD);

    // - - - - - Nyquistbit Correction - - - - - //
    NyquistBit = SplitComplexBuffer1.imagp[0] * (*Signal2).imagp[0];
    SplitComplexBuffer1.imagp[0] = 0;
    (*Signal2).imagp[0] = 0;

    // - - - - - Multiplikation - - - - - //
    vDSP_zvmul(&SplitComplexBuffer1, 1, Signal2, 1, &SplitComplexBuffer1, 1, earlyref_Buffersize / 2, 1);
    SplitComplexBuffer1.imagp[0] = NyquistBit;
    Signal2->imagp[0] = impulseNyquist;

    // - - - - - inverse Transformation- - - - - //
    vDSP_fft_zrip(fftConvSetup, &SplitComplexBuffer1, 1, earlyref_Log2N, FFT_INVERSE);

    // - - - - - Reinterpret as floats Vector - - - - - //
    vDSP_ztoc(&SplitComplexBuffer1, 1, (DSPComplex *)FFTConvolutionBuffer, 2, earlyref_Buffersize / 2);
}

float EarlyRef:: h(float alpha, float beta, int lambda) {
    return cosf(M_PI / 2.0 * abs(lambda) + beta) * jn(abs(lambda), fabs(alpha));
}

void EarlyRef:: fillhBuffer(float phi, int offset, int Q) {
    int m, i, lambda;

    for (i = 0; i < (AmbisonicsOrder * 2 + 1) * 2; i++) {
        cblas_sscal(earlyref_Buffersize, 0.0, hBuffer[i], 1);
    }

    i = 0;

    for (m = -AmbisonicsOrder; m <= AmbisonicsOrder; m++) {
        for (lambda = 0; lambda <= offset * 2; lambda++) { // lambda = offset-1 :: Abschneiden der linksseitigen Teils
            if (m < 0) {
                *(hBuffer[i] + Q * lambda) = h(m * phi, -M_PI / 2, lambda - offset);
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
                *(hBuffer[i] + Q * lambda) = h(m * phi, M_PI / 2, lambda - offset);
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
            cblas_sscal(earlyref_Buffersize, 0.0, Rz10[l][m], 1);
            cblas_sscal(earlyref_Buffersize, 0.0, Rz13[l][m], 1);
            cblas_sscal(earlyref_Buffersize, 0.0, Rz19[l][m], 1);
        }
    }

    fillhBuffer(PhiValue / 360.0 * 2.0 * M_PI, Trunc, (int)(QValue));

    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz10[l * (l + 1) + m][l * (l + 1) + abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz10[l * (l + 1) + m][l * (l + 1) - abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
        }
    }

    fillhBuffer(PhiValue / 360.0 * 2.0 * M_PI, Trunc, (int)(QValue * Qy));
    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz13[l * (l + 1) + m][l * (l + 1) + abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz13[l * (l + 1) + m][l * (l + 1) - abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
        }
    }

    fillhBuffer(PhiValue / 360.0 * 2.0 * M_PI, Trunc, (int)(QValue * Qx));
    i = 0;

    for (l = 0; l <= AmbisonicsOrder; l++) {
        for (m = -l; m <= l; m++) {
            memcpy(Rz19[l * (l + 1) + m][l * (l + 1) + abs(m)], hBuffer[(m + AmbisonicsOrder) * 2], earlyref_Buffersize * sizeof(float));
            memcpy(Rz19[l * (l + 1) + m][l * (l + 1) - abs(m)], hBuffer[(m + AmbisonicsOrder) * 2 + 1], earlyref_Buffersize * sizeof(float));
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
            cblas_sscal(earlyref_Buffersize, 0.0, DummyMatrix1[a][b], 1);
        }
    }

    // ::::: Rxz*Rz ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                //if (NonZeroEntriesZ[i][b]==1) {

                cblas_saxpy(earlyref_Buffersize, Rxz[a][i], Rz19[i][b], 1, DummyMatrix1[a][b], 1);
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
            cblas_sscal(earlyref_Buffersize, 0.0, DummyMatrix1[a][b], 1);
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                cblas_saxpy(earlyref_Buffersize, Rzx[i][b], Rx[a][i], 1, DummyMatrix1[a][b], 1);
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
            cblas_sscal(earlyref_Buffersize, 0.0, DummyMatrix1[a][b], 1);
        }
    }

    // ::::: Rxz*Rz ::::: //

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                cblas_saxpy(earlyref_Buffersize, Ryz[a][i], Rz13[i][b], 1, DummyMatrix1[a][b], 1);
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
            cblas_sscal(earlyref_Buffersize, 0.0, DummyMatrix1[a][b], 1);
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            //Multiplication
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (i = 0; i < NumAmbisonicsChannels; i++) {
                cblas_saxpy(earlyref_Buffersize, Rzy[i][b], Ry[a][i], 1, DummyMatrix1[a][b], 1);
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
    int a, b, c;
    float NyquistBit;

    /* ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** */

    // Initialize data for the FFT routines.
    FFTSetup Setup = vDSP_create_fftsetup(earlyref_Log2N, FFT_RADIX2);

    if (Setup == NULL) {
        throw std::runtime_error("Unable to create the rotation-matrix FFT setup");
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set to zero
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            vDSP_vclr(fft_Rx[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rx[a][b].realp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Ry[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Ry[a][b].realp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rz[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rz[a][b].realp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rxyz_TEMP[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rxyz_TEMP[a][b].realp, 1, earlyref_Buffersize / 2);
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            // Reinterpret Input as SplitComplex
            vDSP_ctoz(reinterpret_cast<DSPComplex*>(Rx[a][b].get()), 2, &fft_Rx[a][b], 1, earlyref_Buffersize / 2);

            // Perform a real-to-complex FFT.
            vDSP_fft_zrip(Setup, &fft_Rx[a][b], 1, earlyref_Log2N, FFT_FORWARD);

            vDSP_ctoz(reinterpret_cast<DSPComplex*>(Ry[a][b].get()), 2, &fft_Ry[a][b], 1, earlyref_Buffersize / 2);

            vDSP_fft_zrip(Setup, &fft_Ry[a][b], 1, earlyref_Log2N, FFT_FORWARD);//???? log2n ?

            vDSP_ctoz(reinterpret_cast<DSPComplex*>(Rz10[a][b].get()), 2, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);

            vDSP_fft_zrip(Setup, &fft_Rz[a][b], 1, earlyref_Log2N, FFT_FORWARD);
        }
    }

    vDSP_destroy_fftsetup(Setup);

    /* ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** */

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (c = 0; c < NumAmbisonicsChannels; c++) {
                if (NonZeroEntriesZ[a][c] == 1 && NonZeroEntriesY[c][b] == 1) {
                    const float zNyquist = fft_Rz[a][c].imagp[0];
                    const float yNyquist = fft_Ry[c][b].imagp[0];
                    NyquistBit = zNyquist * yNyquist; //Nyquistbit Correction
                    fft_Rz[a][c].imagp[0] = 0;
                    fft_Ry[c][b].imagp[0] = 0;

                    vDSP_zvmul(&fft_Rz[a][c], 1, &fft_Ry[c][b], 1, &SplitComplexBuffer2, 1, earlyref_Buffersize / 2, 1);
                    SplitComplexBuffer2.imagp[0] = NyquistBit;
                    fft_Rz[a][c].imagp[0] = zNyquist;
                    fft_Ry[c][b].imagp[0] = yNyquist;
                    vDSP_zvadd(&SplitComplexBuffer2, 1, &fft_Rxyz_TEMP[a][b], 1, &fft_Rxyz_TEMP[a][b], 1, earlyref_Buffersize / 2);
                }
            }
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // set to zero for use as dummy variable
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            vDSP_vclr(fft_Rz[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rz[a][b].realp, 1, earlyref_Buffersize / 2);
        }
    }

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (c = 0; c < NumAmbisonicsChannels; c++) {
                if (NonZeroEntriesX[c][b] == 1) {
                    const float xNyquist = fft_Rx[c][b].imagp[0];
                    const float partialNyquist = fft_Rxyz_TEMP[a][c].imagp[0];
                    NyquistBit = xNyquist * partialNyquist; //Nyquistbit Correction
                    fft_Rx[c][b].imagp[0] = 0;
                    fft_Rxyz_TEMP[a][c].imagp[0] = 0;


                    vDSP_zvmul(&fft_Rx[c][b], 1, &fft_Rxyz_TEMP[a][c], 1, &SplitComplexBuffer2, 1, earlyref_Buffersize / 2, 1);
                    SplitComplexBuffer2.imagp[0] = NyquistBit;
                    fft_Rx[c][b].imagp[0] = xNyquist;
                    fft_Rxyz_TEMP[a][c].imagp[0] = partialNyquist;
                    vDSP_zvadd(&SplitComplexBuffer2, 1, &fft_Rz[a][b], 1, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);
                }
            }
        }
    }

    // . . . ..

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            OnsetLength = (int)(Q_TEMP * (1.0 + (float)Qx + (float)Qy) * (float)Trunc);


            // - - - - - inverse Transformation- - - - - //
            vDSP_fft_zrip(fftConvSetup, &fft_Rz[a][b], 1, earlyref_Log2N, FFT_INVERSE);

            // - - - - - Reinterpret as floats Vector - - - - - //
            vDSP_ztoc(&fft_Rz[a][b], 1, reinterpret_cast<DSPComplex*>(Rxyz[a][b].get()), 2, earlyref_Buffersize / 2);

            cblas_sscal(earlyref_Buffersize, fftScale * 8.0, Rxyz[a][b], 1);

            vDSP_vclr(fft_Rz[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rz[a][b].realp, 1, earlyref_Buffersize / 2);

            const auto onsetOffset = static_cast<std::size_t>(juce::jmax(0, OnsetLength - 10));
            jassert(onsetOffset + earlyref_Buffersize <= Rxyz[a][b].size());
            vDSP_ctoz((DSPComplex *)(Rxyz[a][b] + onsetOffset), 2, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);
            //vDSP_ctoz((DSPComplex *) Rxyz[a][b], 2, &fft_Rz[a][b], 1, earlyref_Buffersize/2);


            vDSP_fft_zrip(fftConvSetup, &fft_Rz[a][b], 1, earlyref_Log2N, FFT_FORWARD);



            //printf("wow");
        }
    }

    // . . . ..

    for (a = 0; a < NumAmbisonicsChannels; a++) {            // Copy results to fft_Rxyz_TEMP
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            vDSP_zvmov(&fft_Rz[a][b], 1, &fft_Rxyz_TEMP[a][b], 1, earlyref_Buffersize / 2);
        }
    }

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
            for (i = 0; i < static_cast<int>(earlyref_Buffersize / 2); i++) {
                if (  (fabsf(fft_Rxyz_TEMP[a][b].realp[i]) >= 0.000001) || (fabsf(fft_Rxyz_TEMP[a][b].imagp[i]) >= 0.000001)) {
                    NonZeroEntriesXYZ_TEMP[a][b] = 1;
                    //printf("Zeros[%i][%i] : Re=%f   Im=%f\n",a,b,fft_Rxyz_TEMP[a][b].realp[i],fft_Rxyz_TEMP[a][b].imagp[i]);
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
