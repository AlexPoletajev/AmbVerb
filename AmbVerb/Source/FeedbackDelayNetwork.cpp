//
//  FeedbackDelayNetwork.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include "FeedbackDelayNetwork.hpp"


FDN::FDN(){
    printf("FDN Consturctor\n");
    
    for (int i=0; i<NumAmbisonicsChannels;i++){
        
        Output[i]=      (float*) calloc(fdn_Buffersize, sizeof(Output[i]));
        if (Output[i] == nullptr) {
            perror("Error Allocating Memory9");return;
        }
    }
    
    for (int i=0; i<NumDelaylines;i++){
        Delayline_leftEnd[i] =              (float*) calloc(fdn_Buffersize, sizeof(Delayline_leftEnd[i]));
        if (Delayline_leftEnd[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        IR_Delayline_leftEnd[i] =              (float*) calloc(fdn_Buffersize, sizeof(IR_Delayline_leftEnd[i]));
        if (IR_Delayline_leftEnd[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        IR[i]=                  (float*) calloc( (fdn_Buffersize) , sizeof(IR[i]) );
        if (IR[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_IR[i].realp =       (float*) calloc(fdn_Buffersize/2, sizeof(fft_IR[i].realp));
        if (fft_IR[i].realp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_IR[i].imagp =       (float*) calloc(fdn_Buffersize/2, sizeof(fft_IR[i].imagp));
        if (fft_IR[i].imagp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_IR_temp[i].realp =       (float*) calloc(fdn_Buffersize/2, sizeof(fft_IR_temp[i].realp));
        if (fft_IR_temp[i].realp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_IR_temp[i].imagp =       (float*) calloc(fdn_Buffersize/2, sizeof(fft_IR_temp[i].imagp));
        if (fft_IR_temp[i].imagp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_Delaylines[i].realp =   (float*) calloc(fdn_Buffersize/2, sizeof(fft_Delaylines[i].realp));
        if (fft_Delaylines[i].realp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        fft_Delaylines[i].imagp =   (float*) calloc(fdn_Buffersize/2, sizeof(fft_Delaylines[i].imagp));
        if (fft_Delaylines[i].imagp == nullptr) {
            perror("Error Allocating Memory");return;
        }
        TempBuffer[i] =         (float*) calloc(fdn_Buffersize,sizeof(TempBuffer[i]));
        if (TempBuffer[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        
        EarlyReflectionsBuffer[i] = (float*) calloc(fdn_Buffersize,sizeof(EarlyReflectionsBuffer[i]));
        if (EarlyReflectionsBuffer[i] == nullptr) {
            perror("Error Allocating Memory");return;
        }
        
        Filter_initialSample[i]=0;
        IR_Filter_initialSample[i]=0;
    }
    inBuffer        = (float*) calloc(fdn_Buffersize, sizeof(inBuffer));
    if (inBuffer == nullptr) {
        perror("Error Allocating Memory");return;
    }
    IR_inBuffer        = (float*) calloc(fdn_Buffersize, sizeof(IR_inBuffer));
    if (IR_inBuffer == nullptr) {
        perror("Error Allocating Memory");return;
    }
    FilterBuffer = (float*) calloc(fdn_Buffersize, sizeof(FilterBuffer));
    if (FilterBuffer == nullptr) {
        perror("Error Allocating Memory");return;
    }
    IR_FilterBuffer = (float*) calloc(fdn_Buffersize, sizeof(IR_FilterBuffer));
    if (IR_FilterBuffer == nullptr) {
        perror("Error Allocating Memory");return;
    }
    IR_TempBuffer1 =         (float*) calloc(fdn_Buffersize,sizeof(IR_TempBuffer1));
    if (IR_TempBuffer1 == nullptr) {
        perror("Error Allocating Memory");return;
    }
    IR_TempBuffer2 =         (float*) calloc(fdn_Buffersize,sizeof(IR_TempBuffer2));
    if (IR_TempBuffer2 == nullptr) {
        perror("Error Allocating Memory");return;
    }
    Window =         (float*) calloc(fdn_Buffersize,sizeof(Window));
    if (Window == nullptr) {
        perror("Error Allocating Memory");return;
    }
    /* --- --- --- ---  FeedBackMatrix Bereitstellen --- --- --- --- */
    int Hadamard[32][32]={{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1},
        {1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1},
        {1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1},
        {1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1},
        {1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1},
        {1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1},
        {1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1},
        {1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1},
        {1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1},
        {1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1},
        {1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1},
        {1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1},
        {1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1},
        {1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1},
        {1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        {1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1},
        {1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1},
        {1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1},
        {1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1},
        {1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1},
        {1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1},
        {1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1},
        {1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,1,1,1,1},
        {1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1,1,-1,1,-1},
        {1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1,1,1,-1,-1},
        {1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,1},
        {1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,-1,-1,-1,-1},
        {1,-1,1,-1,-1,1,-1,1,-1,1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,1,-1,1,-1,-1,1,-1,1},
        {1,1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,1,1,-1,-1,-1,-1,1,1},
        {1,-1,-1,1,-1,1,1,-1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,1,-1,1,1,-1}};
    /*
    int dummy1[NumDelaylines][NumDelaylines] = {
        {1,1,-1,1,-1,-1,1,-1,-1,-1,1,-1,1,1,-1,1}, //1
        {1,1,-1,-1,1,1,-1,-1,-1,-1,1,1,-1,-1,1,1}, //2
        {1,-1,-1,1,1,-1,-1,1,-1,1,1,-1,-1,1,1,-1}, //3
        {1,1,1,-1,-1,-1,-1,1,-1,-1,-1,1,1,1,1,-1}, //4
        {1,-1,1,-1,1,-1,1,-1,-1,1,-1,1,-1,1,-1,1}, //5
        {1,-1,-1,-1,-1,1,1,1,-1,1,1,1,1,-1,-1,-1}, //6
        {1,-1,1,1,-1,1,-1,-1,-1,1,-1,-1,1,-1,1,1}, //7
        {1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1}, //8
        {1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1,1,-1}, //9
        {1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1}, //10
        {1,-1,-1,1,1,-1,-1,1,1,-1,-1,1,1,-1,-1,1}, //11
        {1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1}, //12
        {1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1}, //13
        {1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1}, //14
        {1,-1,1,1,-1,1,-1,-1,1,-1,1,1,-1,1,-1,-1}, //15
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}            //16
        
    };*/
    for (int i=0; i<NumDelaylines; i++) {
        for (int j=0; j<NumDelaylines; j++) {
            FeedbackMarix[i][j]= Hadamard[i][j];
        }
    }
    // memcpy (&FeedbackMarix, &dummy1, NumDelaylines*NumDelaylines*sizeof(int));
    
    /* --- --- --- ---  Primzahlen Bereitstellen --- --- --- --- */
    int Prim[794]={    2,     3,     5,     7,    11,    13,    17,    19,    23,    29,    31,    37,    41,    43,
        47,    53,    59,    61,    67,    71,    73,    79,    83,    89,    97,   101,   103,   107,
        109,   113,   127,   131,   137,   139,   149,   151,   157,   163,   167,   173,   179,   181,
        191,   193,   197,   199,   211,   223,   227,   229,   233,   239,   241,   251,   257,   263,
        269,   271,   277,   281,   283,   293,   307,   311,   313,   317,   331,   337,   347,   349,
        353,   359,   367,   373,   379,   383,   389,   397,   401,   409,   419,   421,   431,   433,
        439,   443,   449,   457,   461,   463,   467,   479,   487,   491,   499,   503,   509,   521,
        523,   541,   547,   557,   563,   569,   571,   577,   587,   593,   599,   601,   607,   613,
        617,   619,   631,   641,   643,   647,   653,   659,   661,   673,   677,   683,   691,   701,
        709,   719,   727,   733,   739,   743,   751,   757,   761,   769,   773,   787,   797,   809,
        811,   821,   823,   827,   829,   839,   853,   857,   859,   863,   877,   881,   883,   887,
        907,   911,   919,   929,   937,   941,   947,   953,   967,   971,   977,   983,   991,   997,
        1009,  1013,  1019,  1021,  1031,  1033,  1039,  1049,  1051,  1061,  1063,  1069,  1087,  1091,
        1093,  1097,  1103,  1109,  1117,  1123,  1129,  1151,  1153,  1163,  1171,  1181,  1187,  1193,
        1201,  1213,  1217,  1223,  1229,  1231,  1237,  1249,  1259,  1277,  1279,  1283,  1289,  1291,
        1297,  1301,  1303,  1307,  1319,  1321,  1327,  1361,  1367,  1373,  1381,  1399,  1409,  1423,
        1427,  1429,  1433,  1439,  1447,  1451,  1453,  1459,  1471,  1481,  1483,  1487,  1489,  1493,
        1499,  1511,  1523,  1531,  1543,  1549,  1553,  1559,  1567,  1571,  1579,  1583,  1597,  1601,
        1607,  1609,  1613,  1619,  1621,  1627,  1637,  1657,  1663,  1667,  1669,  1693,  1697,  1699,
        1709,  1721,  1723,  1733,  1741,  1747,  1753,  1759,  1777,  1783,  1787,  1789,  1801,  1811,
        1823,  1831,  1847,  1861,  1867,  1871,  1873,  1877,  1879,  1889,  1901,  1907,  1913,  1931,
        1933,  1949,  1951,  1973,  1979,  1987,  1993,  1997,  1999,  2003,  2011,  2017,  2027,  2029,
        2039,  2053,  2063,  2069,  2081,  2083,  2087,  2089,  2099,  2111,  2113,  2129,  2131,  2137,
        2141,  2143,  2153,  2161,  2179,  2203,  2207,  2213,  2221,  2237,  2239,  2243,  2251,  2267,
        2269,  2273,  2281,  2287,  2293,  2297,  2309,  2311,  2333,  2339,  2341,  2347,  2351,  2357,
        2371,  2377,  2381,  2383,  2389,  2393,  2399,  2411,  2417,  2423,  2437,  2441,  2447,  2459,
        2467,  2473,  2477,  2503,  2521,  2531,  2539,  2543,  2549,  2551,  2557,  2579,  2591,  2593,
        2609,  2617,  2621,  2633,  2647,  2657,  2659,  2663,  2671,  2677,  2683,  2687,  2689,  2693,
        2699,  2707,  2711,  2713,  2719,  2729,  2731,  2741,  2749,  2753,  2767,  2777,  2789,  2791,
        2797,  2801,  2803,  2819,  2833,  2837,  2843,  2851,  2857,  2861,  2879,  2887,  2897,  2903,
        2909,  2917,  2927,  2939,  2953,  2957,  2963,  2969,  2971,  2999,  3001,  3011,  3019,  3023,
        3037,  3041,  3049,  3061,  3067,  3079,  3083,  3089,  3109,  3119,  3121,  3137,  3163,  3167,
        3169,  3181,  3187,  3191,  3203,  3209,  3217,  3221,  3229,  3251,  3253,  3257,  3259,  3271,
        3299,  3301,  3307,  3313,  3319,  3323,  3329,  3331,  3343,  3347,  3359,  3361,  3371,  3373,
        3389,  3391,  3407,  3413,  3433,  3449,  3457,  3461,  3463,  3467,  3469,  3491,  3499,  3511,
        3517,  3527,  3529,  3533,  3539,  3541,  3547,  3557,  3559,  3571,  3581,  3583,  3593,  3607,
        3613,  3617,  3623,  3631,  3637,  3643,  3659,  3671,  3673,  3677,  3691,  3697,  3701,  3709,
        3719,  3727,  3733,  3739,  3761,  3767,  3769,  3779,  3793,  3797,  3803,  3821,  3823,  3833,
        3847,  3851,  3853,  3863,  3877,  3881,  3889,  3907,  3911,  3917,  3919,  3923,  3929,  3931,
        3943,  3947,  3967,  3989,  4001,  4003,  4007,  4013,  4019,  4021,  4027,  4049,  4051,  4057,
        4073,  4079,  4091,  4093,  4099,  4111,  4127,  4129,  4133,  4139,  4153,  4157,  4159,  4177,
        4201,  4211,  4217,  4219,  4229,  4231,  4241,  4243,  4253,  4259,  4261,  4271,  4273,  4283,
        4289,  4297,  4327,  4337,  4339,  4349,  4357,  4363,  4373,  4391,  4397,  4409,  4421,  4423,
        4441,  4447,  4451,  4457,  4463,  4481,  4483,  4493,  4507,  4513,  4517,  4519,  4523,  4547,
        4549,  4561,  4567,  4583,  4591,  4597,  4603,  4621,  4637,  4639,  4643,  4649,  4651,  4657,
        4663,  4673,  4679,  4691,  4703,  4721,  4723,  4729,  4733,  4751,  4759,  4783,  4787,  4789,
        4793,  4799,  4801,  4813,  4817,  4831,  4861,  4871,  4877,  4889,  4903,  4909,  4919,  4931,
        4933,  4937,  4943,  4951,  4957,  4967,  4969,  4973,  4987,  4993,  4999,  5003,  5009,  5011,
        5021,  5023,  5039,  5051,  5059,  5077,  5081,  5087,  5099,  5101,  5107,  5113,  5119,  5147,
        5153,  5167,  5171,  5179,  5189,  5197,  5209,  5227,  5231,  5233,  5237,  5261,  5273,  5279,
        5281,  5297,  5303,  5309,  5323,  5333,  5347,  5351,  5381,  5387,  5393,  5399,  5407,  5413,
        5417,  5419,  5431,  5437,  5441,  5443,  5449,  5471,  5477,  5479,  5483,  5501,  5503,  5507,
        5519,  5521,  5527,  5531,  5557,  5563,  5569,  5573,  5581,  5591,  5623,  5639,  5641,  5647,
        5651,  5653,  5657,  5659,  5669,  5683,  5689,  5693,  5701,  5711,  5717,  5737,  5741,  5743,
        5749,  5779,  5783,  5791,  5801,  5807,  5813,  5821,  5827,  5839,  5843,  5849,  5851,  5857,
        5861,  5867,  5869,  5879,  5881,  5897,  5903,  5923,  5927,  5939,  5953,  5981,  5987,  6007,
        6011,  6029,  6037,  6043,  6047,  6053,  6067,  6073,  6079,  6089};
    memcpy (&Primnumbers, &Prim, 794*sizeof(int));
    /* --- --- --- ---  FFT fftSetup --- --- --- --- */
    
    fftSetup = vDSP_create_fftsetup(fdn_Log2N, FFT_RADIX2);
    if (fftSetup == nullptr)
    {
        perror("Error, vDSP_create_fftsetup failed.\n");
        exit (EXIT_FAILURE);
    }
    
    fdnVol = 0.7;
    T60Ratio = 0.25;
    T60 = 2.0;
    fftScale =1.0f/(float)(4*fdn_Buffersize);
    check=0;
    Samplerate=44100.0;
    setDelayTimes(1000, 2000);
    unlockParamtersOnOff=0;
    
}

FDN::~FDN(){
    
    for (int i=0; i<NumDelaylines;i++){
        free(Delayline_leftEnd[i]);
        free(IR_Delayline_leftEnd[i]);
        
        free(IR[i]);
        free(fft_IR[i].realp);
        free(fft_IR[i].imagp);
        free(fft_IR_temp[i].realp);
        free(fft_IR_temp[i].imagp);
        free(fft_Delaylines[i].realp);
        free(fft_Delaylines[i].imagp);
        free(TempBuffer[i]);
        free(EarlyReflectionsBuffer[i]);
    }
    for (int i=0; i<NumAmbisonicsChannels;i++){
        free(Output[i]);
    }
    free(inBuffer);
    free(IR_inBuffer);
    free(IR_FilterBuffer);
    free(FilterBuffer);
    free(IR_TempBuffer1);
    free(IR_TempBuffer2);
    free(Window);
}


void FDN::processBlock(const float* Block, int DspBlocksize, double pB_Samplerate){
    
    if (unlockParamtersOnOff==1) {
        unlockParameters();
        unlockParamtersOnOff=0;
    }
    
    int i,u,NumCycles;
    int CustomBlocksize_temp;
    
    if (Samplerate != pB_Samplerate){
        Samplerate = pB_Samplerate;
        setFilterCoefficients();
    }
    else
        Samplerate = pB_Samplerate;
    
    // - - - - - Eigene Blocksize wählen, falls nötig - - - - - //
    if (DspBlocksize >= DelayTimes[0]) {
        NumCycles = (DspBlocksize / DelayTimes[0]) + 2;
        CustomBlocksize = DspBlocksize / NumCycles;
        CustomBlocksize_temp = CustomBlocksize;
    }
    else {
        NumCycles = 1;
        CustomBlocksize = DspBlocksize;
        CustomBlocksize_temp = 0;
    }
    
    for (int i = 0; i<NumAmbisonicsChannels; i++) {
        vDSP_vclr(Output[i], 1, DspBlocksize);
    }
    
    memcpy(inBuffer, Block , DspBlocksize*sizeof(float)); // Eingangssignalvektor setzten
    
    for (int Cycle = 0; Cycle < NumCycles; Cycle++) {
        if (Cycle == (NumCycles-1) && NumCycles != 1) {
            CustomBlocksize = DspBlocksize - (CustomBlocksize * (NumCycles-1));
        }
        
        
        // - - - - - Variables für BufferOrganisation and Handling - - - - - //
        
        for (i=0; i<NumDelaylines; i++){
            Delayline_rightEnd[i] = Delayline_leftEnd[i] + fdn_Buffersize - CustomBlocksize - 1 ;  // Pointer to Beginning of Delay Line
        }
        for (i=0; i<NumDelaylines; i++){
            Delayline_delayPoint[i] = Delayline_rightEnd[i] - DelayTimes[i] ; // Pointer to Sample inside Delay Line, (PrimNumber) ca. 50ms
        }
        
        
        // - - - - - Buffer um einen Block weiterschieben - - - - - //
        for (i=0; i<NumDelaylines; i++){
            memmove ( Delayline_leftEnd[i], Delayline_leftEnd[i]+CustomBlocksize, (fdn_Buffersize - CustomBlocksize) * sizeof(float)); //Pushing the first N-1 Blocks one step further
            memset (Delayline_rightEnd[i], 0.0 , CustomBlocksize * sizeof(float));
        }
        
        
        // - - - - - Feedback - - - - - //
        FeedbackMatrix_Multiplikation(inBuffer+(Cycle * CustomBlocksize_temp), Delayline_rightEnd, Delayline_delayPoint, CustomBlocksize ); // Apply Feeedback
        
        // - - - - - Tiefpassfilterung - - - - - //
        for (i = 0; i < NumDelaylines; i++){
            Filter_initialSample[i] = LowPass(Delayline_rightEnd[i],a0[i],p[i], Filter_initialSample[i], FilterBuffer, CustomBlocksize);
            memcpy(Delayline_rightEnd[i], FilterBuffer, CustomBlocksize * sizeof(float));
        }
        
        // ...... Output
        u=0;
        for(i=0; i<NumAmbisonicsChannels; i++){
            if((i % NumDelaylines) == 0 && i!=0){ // Falls mehr Outputs existieren als Delaylines werden die Delaylines mehrfach verteilt
                u = u+ NumDelaylines;
            }
            vDSP_vsmul( Delayline_rightEnd[i-u], 1, &fdnVol,Output[i]+(Cycle * CustomBlocksize_temp), 1,  CustomBlocksize);
        }
    }
    
    
    /*---- -- -- -- -- -- Signalspur des Direktschalls und der Ersreflexionen -- -- -- -- -- -- -- --*/
    
    for (i=0; i<NumDelaylines; i++){
        memmove ( EarlyReflectionsBuffer[i], EarlyReflectionsBuffer[i]+DspBlocksize, (fdn_Buffersize - DspBlocksize) * sizeof(float)); //Pushing the first N-1 Blocks one step further
    }
    for ( i=0; i<NumDelaylines; i++ ){ //Set first block to 0
        memset (EarlyReflectionsBuffer[i]+(fdn_Buffersize - DspBlocksize - 1), 0.0 , DspBlocksize * sizeof(float));
    }
    
   
        
        getWindowedOutput(DspBlocksize);
        u=0;
        for(i=0; i<NumAmbisonicsChannels; i++){ // Routing delay lines to outputs
            if((i % NumDelaylines) == 0 && i!=0){
                u = u+ NumDelaylines;
            }
            
            
            // ...... Direktschall und Erstreflexionen subtrahieren ....... //
            
            cblas_saxpy(DspBlocksize, -1.0*fdnVol, EarlyReflectionsBuffer[i-u], 1, Output[i], 1);
            
        }

    
}

void FDN::FeedbackMatrix_Multiplikation(float* inSignal, float* rightEnd[], float* delayPoint[], const int FB_Blocksize){
    
    int i, u, o;
    
    for( i = 0; i <FB_Blocksize; i++){
        
        for(o=0; o<NumDelaylines; o++){
            
            for(u=0; u<NumDelaylines; u++){
                
                *(rightEnd[o]+i)  += FeedbackMarix[o][u]*maxGain * *(delayPoint[u]+i);
            }
            *(rightEnd[o]+i)  += *(inSignal+i);
        }
    }
}

float FDN::LowPass ( float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float * LP_FilterBuffer, const int LP_Blocksize)
{
    
    *LP_FilterBuffer = (*LP_Input)*LP_g + LP_initialSample*LP_p;
    
    for (int n = 1; n < LP_Blocksize ; n++) {
        
        *(LP_FilterBuffer+n) =  *(LP_Input + n)*LP_g  + *(LP_FilterBuffer+n-1)*LP_p;
    }
    return *(LP_FilterBuffer+LP_Blocksize-1);
}



void FDN::getIR(){
    
    int i,j;
    
    int gIR_Blocksize=64;
    
    memset (IR_inBuffer, 0.0 , fdn_Buffersize * sizeof(float));
    
    for (i=0; i<NumDelaylines; i++) {
        memset(IR_Delayline_leftEnd[i], 0.0, fdn_Buffersize * sizeof(float));
        IR_Filter_initialSample[i]=0.0;
        
    }
    
    /*------ Variables for BufferOrganisation and Handling ------ */
    for (i=0; i<NumDelaylines; i++){
        IR_Delayline_rightEnd[i] = IR_Delayline_leftEnd[i] + fdn_Buffersize - gIR_Blocksize - 1;  // Pointer to Beginning of Delay Line
    }
    for (i=0; i<NumDelaylines; i++){
        IR_Delayline_delayPoint[i] = IR_Delayline_rightEnd[i] - DelayTimes_temp[i] ; // Pointer to Sample inside Delay Line, (PrimNumber) ca. 50-80ms
    }
    
    *IR_inBuffer=1.0;
    
    for (j=0; j<=fdn_Buffersize/2; j=j+gIR_Blocksize) {
        
        /*---- -- -- -- -- -- Filling up the inBuffer -- -- -- -- -- -- -- --*/
        for (i=0; i<NumDelaylines; i++){
            memmove ( IR_Delayline_leftEnd[i], IR_Delayline_leftEnd[i]+gIR_Blocksize, (fdn_Buffersize - gIR_Blocksize) * sizeof(float)); //Pushing the first fdn_Buffersize-1 Blocks one step further
        }
        for ( i=0; i<NumDelaylines; i++ ){ //Set first block to 0
            memset (IR_Delayline_rightEnd[i], 0.0 , gIR_Blocksize * sizeof(float));
        }
        
        FeedbackMatrix_Multiplikation(IR_inBuffer, IR_Delayline_rightEnd, IR_Delayline_delayPoint, gIR_Blocksize ); // Apply Feeedback); // Apply Feeedback
        
        for (i = 0; i < NumDelaylines; i++){
            IR_Filter_initialSample[i] = LowPass(IR_Delayline_rightEnd[i],a0_temp[i],p_temp[i], IR_Filter_initialSample[i], IR_FilterBuffer, gIR_Blocksize);
            memcpy(IR_Delayline_rightEnd[i], IR_FilterBuffer, gIR_Blocksize * sizeof(float));
            
        }
        *IR_inBuffer=0;
        
        for (i=0; i<NumDelaylines; i++) {
            memcpy(IR[i]+j, IR_Delayline_rightEnd[i], gIR_Blocksize * sizeof(float) );
            
        }
        
    }
    

}
void FDN:: windowIR(){
    
    
    vDSP_vclr(Window,1,fdn_Buffersize);
    
    
    for (int i = 0; i<tMixStart; i++) {
        *(Window + i)=1.0f;
         //printf("%f\n", *(Window + i));
    }
    
    int CrossfadeLength = tMixStart-tMixEnd;
    //printf("tMiuxStart = %i, tMixEnd = %i\n", tMixStart, tMixEnd);
    for (int i=tMixStart; i < tMixEnd; i++) {
        *(Window+i)=1.0-(sinf(-M_PI_2+M_PI*(i-tMixStart)/(CrossfadeLength-1))+1.0)/2.0;
         //printf("%f\n", *(Window + i));
        
    }
     //printf("WindowEnd\n");
    
    vDSP_vclr(IR_TempBuffer1,1,fdn_Buffersize);
    
    for (int i=0; i < NumDelaylines; i++) {
        // Reinterpret Input as SplitComplex
        vDSP_vmul(Window, 1, IR[i], 1, IR_TempBuffer1, 1, fdn_Buffersize);
        
        vDSP_ctoz((DSPComplex *) IR_TempBuffer1, 2, &fft_IR_temp[i], 1, fdn_Buffersize/2);
        
        // Perform a real-to-complex FFT.
        vDSP_fft_zrip(fftSetup, &fft_IR_temp[i], 1, fdn_Log2N, FFT_FORWARD);
    }
    
}

void FDN:: setWindowBoundries(int start, int end){
    
    tMixStart = start;
    tMixEnd = end;
    refreshWindow();
   //unlockParameters();
    unlockParamtersOnOff=1;

}

void FDN::refreshWindow(){

    getIR();
    windowIR();
}


void FDN::getWindowedOutput(const int aW_Blocksize){
    
    float NyquistBit;
    
    
    for (int i=0; i < NumDelaylines; i++) {
        vDSP_vclr(fft_Delaylines[i].imagp,1,fdn_Buffersize/2);
        vDSP_vclr(fft_Delaylines[i].realp,1,fdn_Buffersize/2);
        vDSP_vclr(IR_TempBuffer1,1,fdn_Buffersize);
    }
    
    memcpy(IR_TempBuffer1, inBuffer, aW_Blocksize*sizeof(float));
    
    for (int i=0; i < NumDelaylines; i++) {
        
        // Reinterpret Input as SplitComplex
        vDSP_ctoz((DSPComplex *) IR_TempBuffer1, 2, &fft_Delaylines[i], 1, fdn_Buffersize/2);
        
        // Perform a real-to-complex FFT.
        vDSP_fft_zrip(fftSetup, &fft_Delaylines[i], 1, fdn_Log2N, FFT_FORWARD);
        
        NyquistBit=fft_IR[i].imagp[0] * fft_Delaylines[i].imagp[0]; //Nyquistbit Correction
        
        fft_IR[i].imagp[0]=0;
        fft_Delaylines[i].imagp[0]=0;
        
        vDSP_zvmul(&fft_IR[i], 1, &fft_Delaylines[i], 1, &fft_Delaylines[i], 1, fdn_Buffersize/2, 1);
        fft_Delaylines[i].imagp[0]=NyquistBit;
        vDSP_fft_zrip(fftSetup,&fft_Delaylines[i], 1, fdn_Log2N, FFT_INVERSE);
        vDSP_ztoc(&fft_Delaylines[i], 1, (DSPComplex*) TempBuffer[i], 2, fdn_Buffersize/2);
        
        vDSP_vsma(TempBuffer[i], 1,&fftScale, EarlyReflectionsBuffer[i],1, EarlyReflectionsBuffer[i],1,fdn_Buffersize);
    }
    
}

void FDN::setFilterCoefficients(){
    
    
    if (T60 <= 0.0015) T60 = 0.0015;
    for (int i = 0; i<NumDelaylines; i++){
        
        R0[i] = pow(10.0, -3.0*DelayTimes_temp[i]/(T60*Samplerate));
        Rpi[i] = pow(10.0, -3.0*DelayTimes_temp[i]/(T60*T60Ratio*Samplerate));
        
        p_temp[i] = (R0[i] - Rpi[i])/(R0[i] + Rpi[i]);
        a0_temp[i] = (2.0*R0[i]*Rpi[i])/(R0[i] + Rpi[i]);
        
    }
    
}

void FDN::unlockParameters(){
    
    for (int i = 0; i<NumDelaylines; i++){

        p[i]=p_temp[i];
        a0[i]=a0_temp[i];

        DelayTimes[i] = DelayTimes_temp[i];
    }
    for (int i = 0; i<NumDelaylines; i++){
        TemporaryPointer=fft_IR[i].realp;
        fft_IR[i].realp=fft_IR_temp[i].realp;
        fft_IR_temp[i].realp=TemporaryPointer;
        
        TemporaryPointer=fft_IR[i].imagp;
        fft_IR[i].imagp=fft_IR_temp[i].imagp;
        fft_IR_temp[i].imagp=TemporaryPointer;
    }

    

}

void FDN::setDelayTimes( float min, float max){
    
    
    printf("set Delays\n");
    int i;
    int MinMax[2];
    int L = sizeof(Primnumbers)/sizeof(Primnumbers[0]);
    for (i = 0; i < L; i++) {
        if (Primnumbers[i] >= min) { MinMax[0] = i; break;}
        
    }
    
    for (i = 0; i < L; i++) {
        if (Primnumbers[i] >= max) { MinMax[1] = i; break;}
        
    }
    
    int Step = (int) (MinMax[1]-MinMax[0])/NumDelaylines;
    
    for (i = 0; i < NumDelaylines; i++) {
        DelayTimes_temp[i] = Primnumbers[MinMax[0]+i*Step] * (int) (Samplerate / 44100.0);
    }
    
    setFilterCoefficients();
    //printf("Delaytimes[0]=%i, Delaytimes[15]=%i\n", DelayTimes[i], DelayTimes[NumDelaylines-1]);
    
    minDelaytime = min;
    maxDelaytime = max;

}

void FDN::setT60(float t60){
    
    T60 = t60;
    printf("T60 = %f",T60);
    setFilterCoefficients();
    refreshWindow();
    //unlockParameters();
    unlockParamtersOnOff=1;

}
void FDN::setT60Ratio(float t60ratio){
    
    T60Ratio = t60ratio;
    setFilterCoefficients();
    refreshWindow();
    //unlockParameters();
    unlockParamtersOnOff=1;

}

void FDN::set_Volume(float Value){
    
    if (Value < 0)  {
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
    }
    else if( Value > 1){
        printf("(%f)-Error: Magnitude must be between 0 - 1\n",Value);
        
    }
    else if (Value == 0)
    {
        fdnVol = 0;
    }
    else fdnVol = 0.01*exp(4.605170*Value);
}
