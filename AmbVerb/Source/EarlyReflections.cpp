//
//  EarlyReflections.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include <unistd.h>
#include "EarlyReflections.hpp"

EarlyRef::EarlyRef() {
    printf("Earlyref Consturctor\n");


    earlyrefVolume = 0.7;
    EarlyrefDelayTime = 0;
    FilterCoeffA = 1;
    FilterCoeffB = 0;

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        InBuffer[i] =              (float *)calloc(earlyref_Buffersize, sizeof(InBuffer[i]));

        if (InBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        OutBuffer[i] =              (float *)calloc(earlyref_Buffersize, sizeof(OutBuffer[i]));

        if (OutBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        dummyBuffer1[i] =   (float *)calloc(earlyref_Buffersize, sizeof(dummyBuffer1[i]));

        if (dummyBuffer1[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        IR[i] =    (float *)calloc(earlyref_Buffersize, sizeof(IR[i]));

        if (IR[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        IR_TEMP[i] =    (float *)calloc(earlyref_Buffersize, sizeof(IR_TEMP[i]));

        if (IR_TEMP[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        Output[i] =  (float *)calloc(earlyref_Buffersize, sizeof(Output[i]));

        if (Output[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        Filter_InitialSample[i] = 0;
    }

    //.....fft
    FFTconvBuffer1 =    (float *)calloc(earlyref_Buffersize, sizeof(FFTconvBuffer1));

    if (FFTconvBuffer1 == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    FFTconvBuffer2 = (float *)calloc(earlyref_Buffersize, sizeof(FFTconvBuffer2));

    if (FFTconvBuffer2 == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    FilterBuffer = (float *)calloc(earlyref_Buffersize, sizeof(FilterBuffer));

    if (FilterBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    SplitComplexBuffer1.realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(SplitComplexBuffer1.realp));

    if (SplitComplexBuffer1.realp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    SplitComplexBuffer1.imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(SplitComplexBuffer1.imagp));

    if (SplitComplexBuffer1.imagp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    SplitComplexBuffer2.realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(SplitComplexBuffer2.realp));

    if (SplitComplexBuffer2.realp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    SplitComplexBuffer2.imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(SplitComplexBuffer2.imagp));

    if (SplitComplexBuffer2.imagp == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    fftScale = 0.5f / (float)(16.0f * earlyref_Buffersize);
    fftConvSetup = vDSP_create_fftsetup(earlyref_Log2N, FFT_RADIX2);

    //......

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        for (int u = 0; u < NumAmbisonicsChannels; u++) {
            Rx[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Rx[i][u]));

            if (Rx[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Ry[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Ry[i][u]));

            if (Ry[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz10[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Rz10[i][u]));

            if (Rz10[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz13[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Rz13[i][u]));

            if (Rz13[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rz19[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Rz19[i][u]));

            if (Rz19[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            DummyMatrix1[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(DummyMatrix1[i][u]));

            if (DummyMatrix1[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            Rxyz[i][u] = (float *)calloc(earlyref_Buffersize, sizeof(Rxyz[i][u]));

            if (Rx[i][u] == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rx[i][u].realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rx[i][u].realp));

            if (fft_Rx[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rx[i][u].imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rx[i][u].imagp));

            if (fft_Rx[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Ry[i][u].realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Ry[i][u].realp));

            if (fft_Ry[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Ry[i][u].imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Ry[i][u].imagp));

            if (fft_Ry[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rz[i][u].realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rz[i][u].realp));

            if (fft_Rz[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rz[i][u].imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rz[i][u].imagp));

            if (fft_Rz[i][u].imagp  == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz[i][u].realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rxyz[i][u].realp));

            if (fft_Rxyz[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz[i][u].imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rxyz[i][u].imagp));

            if (fft_Rxyz[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz_TEMP[i][u].realp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rxyz_TEMP[i][u].realp));

            if (fft_Rxyz_TEMP[i][u].realp == nullptr) {
                perror("Error Allocating Memory"); return;
            }

            fft_Rxyz_TEMP[i][u].imagp = (float *)calloc(earlyref_Buffersize / 2, sizeof(fft_Rxyz_TEMP[i][u].imagp));

            if (fft_Rxyz_TEMP[i][u].imagp == nullptr) {
                perror("Error Allocating Memory"); return;
            }
        }
    }

    for (int i = 0; i < (AmbisonicsOrder * 2 + 1) * 2; i++) {
        hBuffer[i] = (float *)calloc(earlyref_Buffersize, sizeof(hBuffer[i]));

        if (hBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }
    }

    Phi = 80.0;
    Q = 400.0;
}

EarlyRef::~EarlyRef() {
    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        for (int u = 0; u < NumAmbisonicsChannels; u++) {
            free(Rx[i][u]);
            free(Ry[i][u]);
            free(Rz10[i][u]);
            free(Rz13[i][u]);
            free(Rz19[i][u]);
            free(DummyMatrix1[i][u]);

            // fft
            free(fft_Rx[i][u].realp);
            free(fft_Rx[i][u].imagp);
            free(fft_Ry[i][u].realp);
            free(fft_Ry[i][u].imagp);
            free(fft_Rz[i][u].realp);
            free(fft_Rz[i][u].imagp);
            free(fft_Rxyz[i][u].realp);
            free(fft_Rxyz[i][u].imagp);
            free(fft_Rxyz_TEMP[i][u].realp);
            free(fft_Rxyz_TEMP[i][u].imagp);
        }
    }

    for (int i = 0; i < (AmbisonicsOrder * 2 + 1) * 2; i++) {
        free(hBuffer[i]);
    }

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        free(InBuffer[i]);
        free(OutBuffer[i]);
        free(dummyBuffer1[i]);
        free(IR[i]);
        free(IR_TEMP[i]);

        free(Output[i]);
    }

    free(FFTconvBuffer1);
    free(FFTconvBuffer2);
    free(FilterBuffer);

    free(SplitComplexBuffer1.realp);
    free(SplitComplexBuffer1.imagp);
    free(SplitComplexBuffer2.realp);
    free(SplitComplexBuffer2.imagp);
}

void EarlyRef::processBlock(const float *const Block[], int DspBlocksize, double pB_Samplerate) {
    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        vDSP_vclr(InBuffer[i], 1, earlyref_Buffersize);
        memcpy(InBuffer[i], Block[i], DspBlocksize * sizeof(float));
    }

    MatrixConvolution();

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        // - - - - - Ausgabe - - - - - -
        memcpy(Output[i], OutBuffer[i], DspBlocksize * sizeof(float)); //Verzögerung der Erstreflexionen
        //memcpy(Output[i], OutBuffer[i]+ (int) (EarlyrefDelayTime * Q/100.0), DspBlocksize * sizeof(float)); //Verzögerung der Erstreflexionen

        Filter_InitialSample[i] = LowPass(Output[i], FilterCoeffA, FilterCoeffB, Filter_InitialSample[i], FilterBuffer, DspBlocksize); //Lowpass Filterung
        memcpy(Output[i], FilterBuffer, DspBlocksize * sizeof(float));
        vDSP_vsmul(Output[i], 1, &earlyrefVolume, Output[i], 1, DspBlocksize);

        // - - - - - Vorbereitung für den nächsten Block - - - - -
        memmove(OutBuffer[i], OutBuffer[i] + DspBlocksize, (earlyref_Buffersize - DspBlocksize) * sizeof(float));
        vDSP_vclr(OutBuffer[i] + (earlyref_Buffersize - DspBlocksize - 1), 1, DspBlocksize);
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
        perror("Error, vDSP_create_fftsetup failed.\n");
        exit(EXIT_FAILURE);
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
            vDSP_ctoz((DSPComplex *)Rx[a][b], 2, &fft_Rx[a][b], 1, earlyref_Buffersize / 2);

            // Perform a real-to-complex FFT.
            vDSP_fft_zrip(Setup, &fft_Rx[a][b], 1, earlyref_Log2N, FFT_FORWARD);

            vDSP_ctoz((DSPComplex *)Ry[a][b], 2, &fft_Ry[a][b], 1, earlyref_Buffersize / 2);

            vDSP_fft_zrip(Setup, &fft_Ry[a][b], 1, earlyref_Log2N, FFT_FORWARD);//???? log2n ?

            vDSP_ctoz((DSPComplex *)Rz10[a][b], 2, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);

            vDSP_fft_zrip(Setup, &fft_Rz[a][b], 1, earlyref_Log2N, FFT_FORWARD);
        }
    }

    vDSP_destroy_fftsetup(Setup);

    /* ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** */

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            for (c = 0; c < NumAmbisonicsChannels; c++) {
                if (NonZeroEntriesZ[a][c] == 1 && NonZeroEntriesY[c][b] == 1) {
                    NyquistBit = fft_Rz[a][c].imagp[0] * fft_Ry[c][b].imagp[0]; //Nyquistbit Correction
                    fft_Rz[a][c].imagp[0] = 0;
                    fft_Ry[c][b].imagp[0] = 0;

                    vDSP_zvmul(&fft_Rz[a][c], 1, &fft_Ry[c][b], 1, &SplitComplexBuffer2, 1, earlyref_Buffersize / 2, 1);
                    SplitComplexBuffer2.imagp[0] = NyquistBit;
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
                    NyquistBit = fft_Rx[c][b].imagp[0] * fft_Rxyz_TEMP[a][c].imagp[0]; //Nyquistbit Correction
                    fft_Rx[c][b].imagp[0] = 0;
                    fft_Rxyz_TEMP[a][c].imagp[0] = 0;


                    vDSP_zvmul(&fft_Rx[c][b], 1, &fft_Rxyz_TEMP[a][c], 1, &SplitComplexBuffer2, 1, earlyref_Buffersize / 2, 1);
                    SplitComplexBuffer2.imagp[0] = NyquistBit;
                    vDSP_zvadd(&SplitComplexBuffer2, 1, &fft_Rz[a][b], 1, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);
                }
            }
        }
    }

    // . . . ..

    printf("Q=%i\n Qx=%f\n Qy=%f\n CutPoint=%i\n", Q, Qx, Qy, (int)(Q * (1.0 + (float)Qx + (float)Qy) * (float)Trunc - 10));

    for (a = 0; a < NumAmbisonicsChannels; a++) {
        for (b = 0; b < NumAmbisonicsChannels; b++) {
            OnsetLength = (int)(Q * (1.0 + (float)Qx + (float)Qy) * (float)Trunc);


            // - - - - - inverse Transformation- - - - - //
            vDSP_fft_zrip(fftConvSetup, &fft_Rz[a][b], 1, earlyref_Log2N, FFT_INVERSE);

            // - - - - - Reinterpret as floats Vector - - - - - //
            vDSP_ztoc(&fft_Rz[a][b], 1, (DSPComplex *)Rxyz[a][b], 2, earlyref_Buffersize / 2);

            cblas_sscal(earlyref_Buffersize, fftScale * 8.0, Rxyz[a][b], 1);

            vDSP_vclr(fft_Rz[a][b].imagp, 1, earlyref_Buffersize / 2);
            vDSP_vclr(fft_Rz[a][b].realp, 1, earlyref_Buffersize / 2);

            vDSP_ctoz((DSPComplex *)(Rxyz[a][b] + (OnsetLength - 10)), 2, &fft_Rz[a][b], 1, earlyref_Buffersize / 2);
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
            for (i = 0; i <= earlyref_Buffersize / 2; i++) {
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
    printf("Calculate Rotation Matrices\n");
    Q_TEMP = QValue;
    CalculateRz(QValue, PhiValue);
    CalculateRx();
    CalculateRy();
    CalculateRxyz();
}

void EarlyRef::  UnlockRotationMatrixForCalculaion() {
    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            vDSP_zvmov(&fft_Rxyz_TEMP[a][b], 1, &fft_Rxyz[a][b], 1, earlyref_Buffersize / 2);
            NonZeroEntriesXYZ[a][b] = NonZeroEntriesXYZ_TEMP[a][b];
        }
    }

    Q = Q_TEMP;
}

void EarlyRef::readTransformationMatrix(std::string Path) {
    int i;
    char buffer[1000];
    int cx;
    FILE *file;
    float *dummy = (float *)calloc(pow(NumAmbisonicsChannels, 2), sizeof(float));


    // ::::: Rxz ::::: //
    cx = snprintf(buffer, 1000, "%s/Rxz%i_1", Path.c_str(), AmbisonicsOrder);

    //cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Juce/AmbVerb/RotationMatrices/Rxz%i_1", AmbisonicsOrder);
    i = 0;
    file = fopen(buffer, "r");

    while (!feof(file)) {
        fscanf(file, "%f", dummy + i);
        i++;
    }

    fclose(file);


    //copy to Rxz
    i = 0;

    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            Rxz[a][b] = *(dummy + i);
            //printf("Rxz[%i][%i] = %f\n",a,b, Rxz[a][b]);
            i++;
        }
    }

    // ::::: Rzx ::::: //
    //cx = snprintf ( buffer, 100,"/Users/anonym/Dropbox/HybridReverb/Juce/AmbVerb/RotationMatrices//Rxz%i_2", AmbisonicsOrder);
    cx = snprintf(buffer, 1000, "%s/Rxz%i_2", Path.c_str(), AmbisonicsOrder);

    i = 0;
    file = fopen(buffer, "r");

    while (!feof(file)) {
        fscanf(file, "%f", dummy + i);
        i++;
    }

    fclose(file);


    //copy to Rzx
    i = 0;

    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            Rzx[a][b] = *(dummy + i);
            //printf("Rxz[%i][%i] = %f\n",a,b, Rxz[a][b]);
            i++;
        }
    }

    // ::::: Ryz ::::: //
    //cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Juce/AmbVerb/RotationMatrices/Ryz%i_1", AmbisonicsOrder);
    cx = snprintf(buffer, 1000, "%s/Ryz%i_1", Path.c_str(), AmbisonicsOrder);

    i = 0;
    file = fopen(buffer, "r");

    while (!feof(file)) {
        fscanf(file, "%f", dummy + i);
        i++;
    }

    fclose(file);


    //copy to Ryz
    i = 0;

    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            Ryz[a][b] = *(dummy + i);
            //printf("Ryz[%i][%i] = %f\n",a,b, Ryz[a][b]);
            i++;
        }
    }

    // ::::: Rzy ::::: //
    //cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Juce/AmbVerb/RotationMatrices/Ryz%i_2", AmbisonicsOrder);
    cx = snprintf(buffer, 1000, "%s/Ryz%i_2", Path.c_str(), AmbisonicsOrder);

    i = 0;
    file = fopen(buffer, "r");

    while (!feof(file)) {
        fscanf(file, "%f", dummy + i);
        i++;
    }

    fclose(file);


    //copy to Rzy
    i = 0;

    for (int a = 0; a < NumAmbisonicsChannels; a++) {
        for (int b = 0; b < NumAmbisonicsChannels; b++) {
            Rzy[a][b] = *(dummy + i);
            //printf("Ryz[%i][%i] = %f\n",a,b, Ryz[a][b]);
            i++;
        }
    }

    free(dummy);
}

float EarlyRef::LowPass(float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float *LP_FilterBuffer, const int LP_Blocksize) {
    *LP_FilterBuffer = (*LP_Input) * LP_g + LP_initialSample * LP_p;

    for (int n = 1; n < LP_Blocksize; n++) {
        *(LP_FilterBuffer + n) =  *(LP_Input + n) * LP_g  + *(LP_FilterBuffer + n - 1) * LP_p;
    }

    return *(LP_FilterBuffer + LP_Blocksize - 1);
}

void EarlyRef:: set_RotAngle(float f) {
    CalculateRotationMatrices(Q, f);
    UnlockRotationMatrixForCalculaion();
}

void EarlyRef:: set_Q(float f) {
    int QMax = (int)(earlyref_Buffersize / (Trunc * 5.7));

    if (f < 1 || f > QMax) {
        printf("Q must be between 1 and %i\n", QMax);
        //return;
    }

    Q = f;
    printf("Q: %f      Phi: %i\n", f, Phi);
    CalculateRotationMatrices(f, Phi);
}

void EarlyRef:: set_EarlyrefVolume(float Value) {
    if (Value < 0) {
        printf("(%f)-Error: Magnitude must be between 0 - 1\n", Value);
    } else if (Value > 1) {
        printf("(%f)-Error: Magnitude must be between 0 - 1\n", Value);
    } else if (Value == 0) {
        earlyrefVolume = 0;
    } else {
        earlyrefVolume = 0.01 * exp(4.605170 * Value);
    }
}
