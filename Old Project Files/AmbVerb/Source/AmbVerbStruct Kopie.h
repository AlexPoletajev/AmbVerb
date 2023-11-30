//
//  AmbVerbHeader.h
//  AmbVerb
//
//  Created by anonym on 12.03.17.
//  Copyright (c) 2017 anonym. All rights reserved.
//

#ifndef AmbVerb_AmbVerbStruct_h
#define AmbVerb_AmbVerbStruct_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>    /* srand, rand */
#include "m_pd.h"
#include <string.h>
#include "time.h"
#include "math.h"
#include "cblas.h"
#include "vDSP.h"

#define earlyref_Log2N 15
#define	earlyref_Buffersize	(1u<<earlyref_Log2N)	// Number of elements.
#define Qx 1.9f
#define Qy 1.3f
#define maxNumChannels 64
#define Ambisonics_Order_MAX 7
#define Trunc	5

#define fdn_Log2N 15
#define	fdn_Buffersize	(1u<<fdn_Log2N)	// Number of elements.
#define NumDelaylines 16
#define fdn_Blocksize 64

#define Max_Gain 0.25


#define m1 2207 // Delay for ith line in Samples
#define m2 2309
#define m3 2399
#define m4 2539
#define m5 2659
#define m6 2707
#define m7 2731
#define m8 2801
#define m9 2441
#define m10 2843
#define m11 2963
#define m12 3037
#define m13 3083
#define m14 3181
#define m15 3217
#define m16 3457

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Class definition
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
static t_class *ambverb_class;

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Data structure definition
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

typedef struct _ambverb {
	t_object x_obj;
	t_sample f;			// Dummy Variable for Signal Inlet
    
	t_inlet  * inlet[maxNumChannels];
	t_outlet * outlet[maxNumChannels];
    
    /* - - - - - - earlyref - - - - - - - - */
    int numChannels;
    int Ambisonics_Order;
    int BlockSize;
    int DirectSoundDelayTime;
    bool earlyref_OnOff;
    float earlyref_DryWetIn, earlyref_DryWetOut;
    int Q, Phi;
	
    t_sample * InSignal[maxNumChannels];
    float * earlyref_InBuffer[maxNumChannels];
	t_sample * OutSignal[maxNumChannels];
	float * earlyref_OutBuffer[maxNumChannels];
	t_signal * Signal[maxNumChannels];
    float * earlyref_Output[maxNumChannels];
    
    float * earlyref_DummyMatrix1[maxNumChannels][maxNumChannels];                                // helper for matrix calculations
    float * Rx[maxNumChannels][maxNumChannels], * Ry[maxNumChannels][maxNumChannels], * Rz10[maxNumChannels][maxNumChannels], * Rz13[maxNumChannels][maxNumChannels], * Rz19[maxNumChannels][maxNumChannels]; // 3d Roationmatrix holding the Impulsresponses
    float   Rxz[maxNumChannels][maxNumChannels],Rzx[maxNumChannels][maxNumChannels],Ryz[maxNumChannels][maxNumChannels],Rzy[maxNumChannels][maxNumChannels]; //2d rotationmatrices for rotating IR matrices
    
    float * hBuffer[(Ambisonics_Order_MAX*2+1)*2];                                                   // Impulsresponses h(m,phi)
    float * earlyref_dummyBuffer1[maxNumChannels];
    float * earlyref_IR[maxNumChannels];
    float * DUMMYVECTOR1, * DUMMYVECTOR2;
    float earlyref_Filter_InitialSample[maxNumChannels];
    float * earlyref_FilterBuffer;

    int check;
    
    
    int NonZeroEntriesX[maxNumChannels][maxNumChannels],NonZeroEntriesY[maxNumChannels][maxNumChannels],NonZeroEntriesZ[maxNumChannels][maxNumChannels],NonZeroEntriesXYZ[maxNumChannels][maxNumChannels];
    
    // Variablen für fft
    DSPSplitComplex  fft_Rxyz[maxNumChannels][maxNumChannels], fft_Rx[maxNumChannels][maxNumChannels], fft_Ry[maxNumChannels][maxNumChannels], fft_Rz[maxNumChannels][maxNumChannels];//Spectral Rotation Matrices
    DSPSplitComplex earlyref_SplitComplexBuffer;
    float * earlyref_FFTconvBuffer1, *earlyref_FFTconvBuffer2;
    FFTSetup earlyref_fftConvSetup;
    float earlyref_NyquistBit;
    float earlyref_fft_scale;
    
    
    
    /* - - - - - - -  fdn - - - -  - - - - */
    float * fdn_Output[maxNumChannels];
	float * line_left[NumDelaylines], * IR_line_left[NumDelaylines]; //left end of fdn_Buffer
	float * line_delayPoint[NumDelaylines], * IR_line_delayPoint[NumDelaylines];//delay Point in fdn_Buffer
	float * line_right[NumDelaylines], * IR_line_right[NumDelaylines]; //last block on the right end of buffer
    float * fdn_IR_Buffer, *fdn_IR[NumDelaylines];
	int fdn_Feedbackmatrix[NumDelaylines][NumDelaylines];
	int m[NumDelaylines];
	double g[NumDelaylines+1],p[NumDelaylines+1],R0[NumDelaylines+1],Rpi[NumDelaylines+1],a0[NumDelaylines+1]; //LowPass Filter Koeffizienten
    float Filter_InitialSample[2*NumDelaylines], IR_Filter_InitialSample[NumDelaylines];
    
	float T60, T60Ratio;
    int DelayMin, DelayMax;
    int Prim[794];
    FFTSetup fdn_Setup;
    
	float * fdn_Filter_Buffer, * fdn_IR_Filter_Buffer;
	float * fdn_InBuffer; //fdn_Buffer
    
    DSPSplitComplex fdn_fft_IR[NumDelaylines],fdn_fft_Signal[NumDelaylines];
    float NyquistBit;
    float fdn_fft_scale;
    float fdn_Vol;
    float * fdn_TempBuffer[NumDelaylines],  * fdn_IR_SubtractionBuffer[NumDelaylines];
    float * fdn_IR_TempBuffer;
    float * Window;
    
} t_ambverb;

extern void get_IR(t_ambverb *x);

#endif
