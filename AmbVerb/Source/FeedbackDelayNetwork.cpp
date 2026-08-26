//
//  FeedbackDelayNetwork.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include "FeedbackDelayNetwork.hpp"

#include <vecLib/cblas.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>


FDN::FDN() {

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        Output[i].allocate(fdn_Buffersize);

        if (Output[i] == nullptr) {
            perror("Error Allocating Memory9"); return;
        }
    }

    for (int i = 0; i < NumDelaylines; i++) {
        Delayline_leftEnd[i].allocate(fdn_Buffersize);

        if (Delayline_leftEnd[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        IR_Delayline_leftEnd[i].allocate(fdn_Buffersize);

        if (IR_Delayline_leftEnd[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        IR[i].allocate(fdn_Buffersize);

        if (IR[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        fft_IR[i].allocate(fdn_Buffersize / 2);

        if (fft_IR[i].realp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        if (fft_IR[i].imagp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        fft_IR_temp[i].allocate(fdn_Buffersize / 2);

        if (fft_IR_temp[i].realp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        if (fft_IR_temp[i].imagp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        fft_Delaylines[i].allocate(fdn_Buffersize / 2);

        if (fft_Delaylines[i].realp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        if (fft_Delaylines[i].imagp == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        TempBuffer[i].allocate(fdn_Buffersize);

        if (TempBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        EarlyReflectionsBuffer[i].allocate(fdn_Buffersize);

        if (EarlyReflectionsBuffer[i] == nullptr) {
            perror("Error Allocating Memory"); return;
        }

        Filter_initialSample[i] = 0;
        IR_Filter_initialSample[i] = 0;
    }

    inBuffer.allocate(fdn_Buffersize);

    if (inBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    IR_inBuffer.allocate(fdn_Buffersize);

    if (IR_inBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    FilterBuffer.allocate(fdn_Buffersize);

    if (FilterBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    IR_FilterBuffer.allocate(fdn_Buffersize);

    if (IR_FilterBuffer == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    IR_TempBuffer1.allocate(fdn_Buffersize);

    if (IR_TempBuffer1 == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    Window.allocate(fdn_Buffersize);

    if (Window == nullptr) {
        perror("Error Allocating Memory"); return;
    }

    fft_Input.allocate(fdn_Buffersize / 2);

    /* --- --- --- ---  FeedBackMatrix Bereitstellen --- --- --- --- */
    int Hadamard[32][32] = { { 1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1  },
        { 1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1 },
        { 1, 1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1 },
        { 1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1  },
        { 1, 1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1 },
        { 1, -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1  },
        { 1, 1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1  },
        { 1, -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1 },
        { 1, 1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1 },
        { 1, -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1  },
        { 1, 1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1  },
        { 1, -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1 },
        { 1, 1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1  },
        { 1, -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1 },
        { 1, 1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1 },
        { 1, -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1  },
        { 1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1  },
        { 1, 1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1  },
        { 1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1 },
        { 1, 1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1  },
        { 1, -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1 },
        { 1, 1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1 },
        { 1, -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1  },
        { 1, 1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,  1,  1,  1  },
        { 1, -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, 1,  -1, 1,  -1 },
        { 1, 1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1, 1,  1,  -1, -1 },
        { 1, -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1,  1,  -1, -1, 1  },
        { 1, 1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  1,  1,  1,  1,  -1, -1, -1, -1 },
        { 1, -1, 1,  -1, -1, 1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, 1,  -1, 1,  -1, -1, 1,  -1, 1  },
        { 1, 1,  -1, -1, -1, -1, 1,  1,  -1, -1, 1,  1,  1,  1,  -1, -1, -1, -1, 1,  1,  1,  1,  -1, -1, 1,  1,  -1, -1, -1, -1, 1,  1  },
        { 1, -1, -1, 1,  -1, 1,  1,  -1, -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  1,  -1, -1, 1,  -1, 1,  1,  -1 } };

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
    for (int i = 0; i < NumDelaylines; i++) {
        for (int j = 0; j < NumDelaylines; j++) {
            FeedbackMarix[i][j] = Hadamard[i][j];
        }
    }

    // memcpy (&FeedbackMarix, &dummy1, NumDelaylines*NumDelaylines*sizeof(int));

    /* --- --- --- ---  Primzahlen Bereitstellen --- --- --- --- */
    int Prim[794] = {    2,    3,     5,     7,     11,    13,    17,    19,    23,    29,    31,    37,    41,    43,
                         47,   53,    59,    61,    67,    71,    73,    79,    83,    89,    97,    101,   103,   107,
                         109,  113,   127,   131,   137,   139,   149,   151,   157,   163,   167,   173,   179,   181,
                         191,  193,   197,   199,   211,   223,   227,   229,   233,   239,   241,   251,   257,   263,
                         269,  271,   277,   281,   283,   293,   307,   311,   313,   317,   331,   337,   347,   349,
                         353,  359,   367,   373,   379,   383,   389,   397,   401,   409,   419,   421,   431,   433,
                         439,  443,   449,   457,   461,   463,   467,   479,   487,   491,   499,   503,   509,   521,
                         523,  541,   547,   557,   563,   569,   571,   577,   587,   593,   599,   601,   607,   613,
                         617,  619,   631,   641,   643,   647,   653,   659,   661,   673,   677,   683,   691,   701,
                         709,  719,   727,   733,   739,   743,   751,   757,   761,   769,   773,   787,   797,   809,
                         811,  821,   823,   827,   829,   839,   853,   857,   859,   863,   877,   881,   883,   887,
                         907,  911,   919,   929,   937,   941,   947,   953,   967,   971,   977,   983,   991,   997,
                         1009, 1013,  1019,  1021,  1031,  1033,  1039,  1049,  1051,  1061,  1063,  1069,  1087,  1091,
                         1093, 1097,  1103,  1109,  1117,  1123,  1129,  1151,  1153,  1163,  1171,  1181,  1187,  1193,
                         1201, 1213,  1217,  1223,  1229,  1231,  1237,  1249,  1259,  1277,  1279,  1283,  1289,  1291,
                         1297, 1301,  1303,  1307,  1319,  1321,  1327,  1361,  1367,  1373,  1381,  1399,  1409,  1423,
                         1427, 1429,  1433,  1439,  1447,  1451,  1453,  1459,  1471,  1481,  1483,  1487,  1489,  1493,
                         1499, 1511,  1523,  1531,  1543,  1549,  1553,  1559,  1567,  1571,  1579,  1583,  1597,  1601,
                         1607, 1609,  1613,  1619,  1621,  1627,  1637,  1657,  1663,  1667,  1669,  1693,  1697,  1699,
                         1709, 1721,  1723,  1733,  1741,  1747,  1753,  1759,  1777,  1783,  1787,  1789,  1801,  1811,
                         1823, 1831,  1847,  1861,  1867,  1871,  1873,  1877,  1879,  1889,  1901,  1907,  1913,  1931,
                         1933, 1949,  1951,  1973,  1979,  1987,  1993,  1997,  1999,  2003,  2011,  2017,  2027,  2029,
                         2039, 2053,  2063,  2069,  2081,  2083,  2087,  2089,  2099,  2111,  2113,  2129,  2131,  2137,
                         2141, 2143,  2153,  2161,  2179,  2203,  2207,  2213,  2221,  2237,  2239,  2243,  2251,  2267,
                         2269, 2273,  2281,  2287,  2293,  2297,  2309,  2311,  2333,  2339,  2341,  2347,  2351,  2357,
                         2371, 2377,  2381,  2383,  2389,  2393,  2399,  2411,  2417,  2423,  2437,  2441,  2447,  2459,
                         2467, 2473,  2477,  2503,  2521,  2531,  2539,  2543,  2549,  2551,  2557,  2579,  2591,  2593,
                         2609, 2617,  2621,  2633,  2647,  2657,  2659,  2663,  2671,  2677,  2683,  2687,  2689,  2693,
                         2699, 2707,  2711,  2713,  2719,  2729,  2731,  2741,  2749,  2753,  2767,  2777,  2789,  2791,
                         2797, 2801,  2803,  2819,  2833,  2837,  2843,  2851,  2857,  2861,  2879,  2887,  2897,  2903,
                         2909, 2917,  2927,  2939,  2953,  2957,  2963,  2969,  2971,  2999,  3001,  3011,  3019,  3023,
                         3037, 3041,  3049,  3061,  3067,  3079,  3083,  3089,  3109,  3119,  3121,  3137,  3163,  3167,
                         3169, 3181,  3187,  3191,  3203,  3209,  3217,  3221,  3229,  3251,  3253,  3257,  3259,  3271,
                         3299, 3301,  3307,  3313,  3319,  3323,  3329,  3331,  3343,  3347,  3359,  3361,  3371,  3373,
                         3389, 3391,  3407,  3413,  3433,  3449,  3457,  3461,  3463,  3467,  3469,  3491,  3499,  3511,
                         3517, 3527,  3529,  3533,  3539,  3541,  3547,  3557,  3559,  3571,  3581,  3583,  3593,  3607,
                         3613, 3617,  3623,  3631,  3637,  3643,  3659,  3671,  3673,  3677,  3691,  3697,  3701,  3709,
                         3719, 3727,  3733,  3739,  3761,  3767,  3769,  3779,  3793,  3797,  3803,  3821,  3823,  3833,
                         3847, 3851,  3853,  3863,  3877,  3881,  3889,  3907,  3911,  3917,  3919,  3923,  3929,  3931,
                         3943, 3947,  3967,  3989,  4001,  4003,  4007,  4013,  4019,  4021,  4027,  4049,  4051,  4057,
                         4073, 4079,  4091,  4093,  4099,  4111,  4127,  4129,  4133,  4139,  4153,  4157,  4159,  4177,
                         4201, 4211,  4217,  4219,  4229,  4231,  4241,  4243,  4253,  4259,  4261,  4271,  4273,  4283,
                         4289, 4297,  4327,  4337,  4339,  4349,  4357,  4363,  4373,  4391,  4397,  4409,  4421,  4423,
                         4441, 4447,  4451,  4457,  4463,  4481,  4483,  4493,  4507,  4513,  4517,  4519,  4523,  4547,
                         4549, 4561,  4567,  4583,  4591,  4597,  4603,  4621,  4637,  4639,  4643,  4649,  4651,  4657,
                         4663, 4673,  4679,  4691,  4703,  4721,  4723,  4729,  4733,  4751,  4759,  4783,  4787,  4789,
                         4793, 4799,  4801,  4813,  4817,  4831,  4861,  4871,  4877,  4889,  4903,  4909,  4919,  4931,
                         4933, 4937,  4943,  4951,  4957,  4967,  4969,  4973,  4987,  4993,  4999,  5003,  5009,  5011,
                         5021, 5023,  5039,  5051,  5059,  5077,  5081,  5087,  5099,  5101,  5107,  5113,  5119,  5147,
                         5153, 5167,  5171,  5179,  5189,  5197,  5209,  5227,  5231,  5233,  5237,  5261,  5273,  5279,
                         5281, 5297,  5303,  5309,  5323,  5333,  5347,  5351,  5381,  5387,  5393,  5399,  5407,  5413,
                         5417, 5419,  5431,  5437,  5441,  5443,  5449,  5471,  5477,  5479,  5483,  5501,  5503,  5507,
                         5519, 5521,  5527,  5531,  5557,  5563,  5569,  5573,  5581,  5591,  5623,  5639,  5641,  5647,
                         5651, 5653,  5657,  5659,  5669,  5683,  5689,  5693,  5701,  5711,  5717,  5737,  5741,  5743,
                         5749, 5779,  5783,  5791,  5801,  5807,  5813,  5821,  5827,  5839,  5843,  5849,  5851,  5857,
                         5861, 5867,  5869,  5879,  5881,  5897,  5903,  5923,  5927,  5939,  5953,  5981,  5987,  6007,
                         6011, 6029,  6037,  6043,  6047,  6053,  6067,  6073,  6079,  6089 };
    memcpy(&Primnumbers, &Prim, 794 * sizeof(int));
    /* --- --- --- ---  FFT fftSetup --- --- --- --- */

    fftSetup = vDSP_create_fftsetup(fdn_Log2N, FFT_RADIX2);

    if (fftSetup == nullptr) {
        throw std::runtime_error("Unable to create the FDN FFT setup");
    }

    fdnVol = 0.7;
    T60Ratio = 0.25;
    T60 = 2.0;
    fftScale = 1.0f / (float)(4 * fdn_Buffersize);
    Samplerate = 44100.0;
    setParameters(MinRoomsize,
                  MaxRoomsize,
                  T60,
                  T60Ratio,
                  static_cast<int>(WindowStartsAt_xRoomsize * MinRoomsize),
                  static_cast<int>(WindowStartsAt_xRoomsize * MinRoomsize + 1000));

    const juce::SpinLock::ScopedLockType lock(parameterLock);
    unlockParameters();
    unlockParamtersOnOff.store(false, std::memory_order_release);
}

FDN::~FDN() {
    if (fftSetup != nullptr)
        vDSP_destroy_fftsetup(fftSetup);
}

void FDN::prepare(double sampleRate, int maximumBlockSize) {
    if (sampleRate <= 0.0
        || maximumBlockSize <= 0
        || static_cast<std::size_t>(maximumBlockSize) > fdn_Buffersize)
        throw std::invalid_argument("Unsupported FDN sample rate or maximum block size");

    {
        const juce::SpinLock::ScopedLockType lock(parameterLock);
        Samplerate = sampleRate;
        setDelayTimesUnchecked(static_cast<float>(minDelaytime),
                               static_cast<float>(maxDelaytime));
        refreshWindow();
        unlockParameters();
        unlockParamtersOnOff.store(false, std::memory_order_release);
    }

    reset();
}

void FDN::reset() {
    inBuffer.clear();
    IR_inBuffer.clear();
    FilterBuffer.clear();
    IR_FilterBuffer.clear();
    IR_TempBuffer1.clear();

    for (int i = 0; i < NumDelaylines; ++i) {
        Delayline_leftEnd[i].clear();
        IR_Delayline_leftEnd[i].clear();
        TempBuffer[i].clear();
        EarlyReflectionsBuffer[i].clear();
        Filter_initialSample[i] = 0.0f;
        IR_Filter_initialSample[i] = 0.0f;
    }

    for (auto& output : Output)
        output.clear();
}

void FDN::processBlock(const float *Block, int DspBlocksize, double pB_Samplerate) {
    if (DspBlocksize <= 0 || static_cast<std::size_t>(DspBlocksize) > fdn_Buffersize) {
        jassertfalse;
        return;
    }

    if (unlockParamtersOnOff.load(std::memory_order_acquire)) {
        const juce::SpinLock::ScopedTryLockType lock(parameterLock);

        if (lock.isLocked()) {
            unlockParameters();
            unlockParamtersOnOff.store(false, std::memory_order_release);
        }
    }

    int i, u, NumCycles;
    int CustomBlocksize_temp;

    jassert(std::abs(Samplerate - pB_Samplerate) < 0.5);

    // - - - - - Eigene Blocksize wählen, falls nötig - - - - - //
    if (DspBlocksize >= DelayTimes[0]) {
        NumCycles = (DspBlocksize / DelayTimes[0]) + 2;
        CustomBlocksize = DspBlocksize / NumCycles;
        CustomBlocksize_temp = CustomBlocksize;
    } else {
        NumCycles = 1;
        CustomBlocksize = DspBlocksize;
        CustomBlocksize_temp = 0;
    }

    for (int i = 0; i < NumAmbisonicsChannels; i++) {
        vDSP_vclr(Output[i], 1, DspBlocksize);
    }

    memcpy(inBuffer, Block, DspBlocksize * sizeof(float)); // Eingangssignalvektor setzten

    for (int Cycle = 0; Cycle < NumCycles; Cycle++) {
        if (Cycle == (NumCycles - 1) && NumCycles != 1) {
            CustomBlocksize = DspBlocksize - (CustomBlocksize * (NumCycles - 1));
        }

        // - - - - - Variables für BufferOrganisation and Handling - - - - - //

        for (i = 0; i < NumDelaylines; i++) {
            Delayline_rightEnd[i] = Delayline_leftEnd[i] + fdn_Buffersize - CustomBlocksize;   // Pointer to Beginning of Delay Line
        }

        for (i = 0; i < NumDelaylines; i++) {
            Delayline_delayPoint[i] = Delayline_rightEnd[i] - DelayTimes[i];  // Pointer to Sample inside Delay Line, (PrimNumber) ca. 50ms
        }

        // - - - - - Buffer um einen Block weiterschieben - - - - - //
        for (i = 0; i < NumDelaylines; i++) {
            memmove(Delayline_leftEnd[i], Delayline_leftEnd[i] + CustomBlocksize, (fdn_Buffersize - CustomBlocksize) * sizeof(float)); //Pushing the first N-1 Blocks one step further
            memset(Delayline_rightEnd[i], 0.0, CustomBlocksize * sizeof(float));
        }

        // - - - - - Feedback - - - - - //
        FeedbackMatrix_Multiplikation(inBuffer + (Cycle * CustomBlocksize_temp), Delayline_rightEnd, Delayline_delayPoint, CustomBlocksize); // Apply Feeedback

        // - - - - - Tiefpassfilterung - - - - - //
        for (i = 0; i < NumDelaylines; i++) {
            Filter_initialSample[i] = LowPass(Delayline_rightEnd[i], a0[i], p[i], Filter_initialSample[i], FilterBuffer, CustomBlocksize);
            memcpy(Delayline_rightEnd[i], FilterBuffer, CustomBlocksize * sizeof(float));
        }

        // ...... Output
        u = 0;

        for (i = 0; i < NumAmbisonicsChannels; i++) {
            if ((i % NumDelaylines) == 0 && i != 0) { // Falls mehr Outputs existieren als Delaylines werden die Delaylines mehrfach verteilt
                u = u + NumDelaylines;
            }

            vDSP_vsmul(Delayline_rightEnd[i - u], 1, &fdnVol, Output[i] + (Cycle * CustomBlocksize_temp), 1,  CustomBlocksize);
        }
    }

    /*---- -- -- -- -- -- Signalspur des Direktschalls und der Ersreflexionen -- -- -- -- -- -- -- --*/

    for (i = 0; i < NumDelaylines; i++) {
        memmove(EarlyReflectionsBuffer[i], EarlyReflectionsBuffer[i] + DspBlocksize, (fdn_Buffersize - DspBlocksize) * sizeof(float)); //Pushing the first N-1 Blocks one step further
    }

    for (i = 0; i < NumDelaylines; i++) { //Set first block to 0
        memset(EarlyReflectionsBuffer[i] + (fdn_Buffersize - DspBlocksize), 0.0, DspBlocksize * sizeof(float));
    }

    getWindowedOutput(DspBlocksize);
    u = 0;

    for (i = 0; i < NumAmbisonicsChannels; i++) { // Routing delay lines to outputs
        if ((i % NumDelaylines) == 0 && i != 0) {
            u = u + NumDelaylines;
        }

        // ...... Direktschall und Erstreflexionen subtrahieren ....... //

        cblas_saxpy(DspBlocksize, -1.0 * fdnVol, EarlyReflectionsBuffer[i - u], 1, Output[i], 1);
    }
}

void FDN::FeedbackMatrix_Multiplikation(float *inSignal, float *rightEnd[], float *delayPoint[], const int FB_Blocksize) {
    int i, u, o;

    for (i = 0; i < FB_Blocksize; i++) {
        for (o = 0; o < NumDelaylines; o++) {
            for (u = 0; u < NumDelaylines; u++) {
                *(rightEnd[o] + i) += FeedbackMarix[o][u] * maxGain * *(delayPoint[u] + i);
            }

            *(rightEnd[o] + i) += *(inSignal + i);
        }
    }
}

float FDN::LowPass(float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float *LP_FilterBuffer, const int LP_Blocksize) {
    *LP_FilterBuffer = (*LP_Input) * LP_g + LP_initialSample * LP_p;

    for (int n = 1; n < LP_Blocksize; n++) {
        *(LP_FilterBuffer + n) =  *(LP_Input + n) * LP_g  + *(LP_FilterBuffer + n - 1) * LP_p;
    }

    return *(LP_FilterBuffer + LP_Blocksize - 1);
}

void FDN::getIR() {
    int i, j;

    int gIR_Blocksize = 64;

    memset(IR_inBuffer, 0.0, fdn_Buffersize * sizeof(float));

    for (i = 0; i < NumDelaylines; i++) {
        memset(IR_Delayline_leftEnd[i], 0.0, fdn_Buffersize * sizeof(float));
        IR_Filter_initialSample[i] = 0.0;
    }

    /*------ Variables for BufferOrganisation and Handling ------ */
    for (i = 0; i < NumDelaylines; i++) {
        IR_Delayline_rightEnd[i] = IR_Delayline_leftEnd[i] + fdn_Buffersize - gIR_Blocksize;  // Pointer to Beginning of Delay Line
    }

    for (i = 0; i < NumDelaylines; i++) {
        IR_Delayline_delayPoint[i] = IR_Delayline_rightEnd[i] - DelayTimes_temp[i];  // Pointer to Sample inside Delay Line, (PrimNumber) ca. 50-80ms
    }

    *IR_inBuffer = 1.0;

    for (j = 0; j <= fdn_Buffersize / 2; j = j + gIR_Blocksize) {
        /*---- -- -- -- -- -- Filling up the inBuffer -- -- -- -- -- -- -- --*/
        for (i = 0; i < NumDelaylines; i++) {
            memmove(IR_Delayline_leftEnd[i], IR_Delayline_leftEnd[i] + gIR_Blocksize, (fdn_Buffersize - gIR_Blocksize) * sizeof(float)); //Pushing the first fdn_Buffersize-1 Blocks one step further
        }

        for (i = 0; i < NumDelaylines; i++) { //Set first block to 0
            memset(IR_Delayline_rightEnd[i], 0.0, gIR_Blocksize * sizeof(float));
        }

        FeedbackMatrix_Multiplikation(IR_inBuffer, IR_Delayline_rightEnd, IR_Delayline_delayPoint, gIR_Blocksize);  // Apply Feeedback); // Apply Feeedback

        for (i = 0; i < NumDelaylines; i++) {
            IR_Filter_initialSample[i] = LowPass(IR_Delayline_rightEnd[i], a0_temp[i], p_temp[i], IR_Filter_initialSample[i], IR_FilterBuffer, gIR_Blocksize);
            memcpy(IR_Delayline_rightEnd[i], IR_FilterBuffer, gIR_Blocksize * sizeof(float));
        }

        *IR_inBuffer = 0;

        for (i = 0; i < NumDelaylines; i++) {
            memcpy(IR[i] + j, IR_Delayline_rightEnd[i], gIR_Blocksize * sizeof(float) );
        }
    }
}

void FDN:: windowIR() {
    vDSP_vclr(Window, 1, fdn_Buffersize);

    const int windowStart = juce::jlimit(0, static_cast<int>(fdn_Buffersize), tMixStart);
    const int windowEnd = juce::jlimit(windowStart, static_cast<int>(fdn_Buffersize), tMixEnd);

    for (int i = 0; i < windowStart; i++) {
        *(Window + i) = 1.0f;
        //printf("%f\n", *(Window + i));
    }

    const int CrossfadeLength = windowEnd - windowStart;

    //printf("tMiuxStart = %i, tMixEnd = %i\n", tMixStart, tMixEnd);
    for (int i = windowStart; i < windowEnd; i++) {
        const auto position = CrossfadeLength > 1
            ? static_cast<float>(i - windowStart) / static_cast<float>(CrossfadeLength - 1)
            : 1.0f;
        *(Window + i) = 1.0f - (std::sin(-juce::MathConstants<float>::halfPi
                                          + juce::MathConstants<float>::pi * position) + 1.0f) / 2.0f;
        //printf("%f\n", *(Window + i));
    }

    //printf("WindowEnd\n");

    vDSP_vclr(IR_TempBuffer1, 1, fdn_Buffersize);

    for (int i = 0; i < NumDelaylines; i++) {
        // Reinterpret Input as SplitComplex
        vDSP_vmul(Window, 1, IR[i], 1, IR_TempBuffer1, 1, fdn_Buffersize);

        vDSP_ctoz(reinterpret_cast<DSPComplex*>(IR_TempBuffer1.get()), 2, &fft_IR_temp[i], 1, fdn_Buffersize / 2);

        // Perform a real-to-complex FFT.
        vDSP_fft_zrip(fftSetup, &fft_IR_temp[i], 1, fdn_Log2N, FFT_FORWARD);
    }
}

void FDN:: setWindowBoundries(int start, int end) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    tMixStart = start;
    tMixEnd = end;
    refreshWindow();
    unlockParamtersOnOff.store(true, std::memory_order_release);
}

void FDN::refreshWindow() {
    getIR();
    windowIR();
}

void FDN::getWindowedOutput(const int aW_Blocksize) {
    float NyquistBit;

    for (int i = 0; i < NumDelaylines; i++) {
        vDSP_vclr(fft_Delaylines[i].imagp, 1, fdn_Buffersize / 2);
        vDSP_vclr(fft_Delaylines[i].realp, 1, fdn_Buffersize / 2);
    }

    vDSP_vclr(IR_TempBuffer1, 1, fdn_Buffersize);
    memcpy(IR_TempBuffer1, inBuffer, aW_Blocksize * sizeof(float));

    vDSP_ctoz((DSPComplex *)IR_TempBuffer1.get(), 2, &fft_Input, 1, fdn_Buffersize / 2);
    vDSP_fft_zrip(fftSetup, &fft_Input, 1, fdn_Log2N, FFT_FORWARD);

    for (int i = 0; i < NumDelaylines; i++) {
        vDSP_zvmov(&fft_Input, 1, &fft_Delaylines[i], 1, fdn_Buffersize / 2);

        NyquistBit = fft_IR[i].imagp[0] * fft_Delaylines[i].imagp[0]; //Nyquistbit Correction
        const float impulseNyquist = fft_IR[i].imagp[0];

        fft_IR[i].imagp[0] = 0;
        fft_Delaylines[i].imagp[0] = 0;

        vDSP_zvmul(&fft_IR[i], 1, &fft_Delaylines[i], 1, &fft_Delaylines[i], 1, fdn_Buffersize / 2, 1);
        fft_Delaylines[i].imagp[0] = NyquistBit;
        fft_IR[i].imagp[0] = impulseNyquist;
        vDSP_fft_zrip(fftSetup, &fft_Delaylines[i], 1, fdn_Log2N, FFT_INVERSE);
        vDSP_ztoc(&fft_Delaylines[i], 1, reinterpret_cast<DSPComplex*>(TempBuffer[i].get()), 2, fdn_Buffersize / 2);

        vDSP_vsma(TempBuffer[i], 1, &fftScale, EarlyReflectionsBuffer[i], 1, EarlyReflectionsBuffer[i], 1, fdn_Buffersize);
    }
}

void FDN::setFilterCoefficients() {
    T60 = juce::jmax(T60, 0.0015f);
    T60Ratio = juce::jmax(T60Ratio, 0.001f);

    for (int i = 0; i < NumDelaylines; i++) {
        R0[i] = pow(10.0, -3.0 * DelayTimes_temp[i] / (T60 * Samplerate));
        Rpi[i] = pow(10.0, -3.0 * DelayTimes_temp[i] / (T60 * T60Ratio * Samplerate));

        p_temp[i] = (R0[i] - Rpi[i]) / (R0[i] + Rpi[i]);
        a0_temp[i] = (2.0 * R0[i] * Rpi[i]) / (R0[i] + Rpi[i]);
    }
}

void FDN::unlockParameters() {
    for (int i = 0; i < NumDelaylines; i++) {
        p[i] = p_temp[i];
        a0[i] = a0_temp[i];

        DelayTimes[i] = DelayTimes_temp[i];
    }

    for (int i = 0; i < NumDelaylines; i++) {
        fft_IR[i].swapWith(fft_IR_temp[i]);
    }
}

void FDN::setDelayTimesUnchecked(float min, float max) {
    constexpr int numberOfPrimes = static_cast<int>(sizeof(Primnumbers) / sizeof(Primnumbers[0]));
    const auto* begin = std::begin(Primnumbers);
    const auto* end = std::end(Primnumbers);
    const auto minIterator = std::lower_bound(begin, end, static_cast<int>(min));
    const auto maxIterator = std::lower_bound(begin, end, static_cast<int>(max));
    const int minIndex = juce::jlimit(0, numberOfPrimes - 1, static_cast<int>(std::distance(begin, minIterator)));
    const int maxIndex = juce::jlimit(minIndex,
                                      numberOfPrimes - 1,
                                      static_cast<int>(std::distance(begin, maxIterator)));
    const double sampleRateScale = Samplerate / 44100.0;

    for (int i = 0; i < NumDelaylines; i++) {
        const int primeIndex = minIndex
            + ((maxIndex - minIndex) * i) / juce::jmax(1, NumDelaylines - 1);
        DelayTimes_temp[i] = juce::jlimit(1,
                                         static_cast<int>(fdn_Buffersize) - 1,
                                         static_cast<int>(std::lround(Primnumbers[primeIndex]
                                                                      * sampleRateScale)));
    }

    setFilterCoefficients();
    //printf("Delaytimes[0]=%i, Delaytimes[15]=%i\n", DelayTimes[i], DelayTimes[NumDelaylines-1]);

    minDelaytime = static_cast<int>(min);
    maxDelaytime = static_cast<int>(max);
}

void FDN::setDelayTimes(float min, float max) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    setDelayTimesUnchecked(min, max);
    refreshWindow();
    unlockParamtersOnOff.store(true, std::memory_order_release);
}

void FDN::setParameters(float minDelay,
                        float maxDelay,
                        float t60,
                        float t60Ratio,
                        int windowStart,
                        int windowEnd) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    T60 = juce::jmax(t60, 0.0015f);
    T60Ratio = juce::jmax(t60Ratio, 0.001f);
    tMixStart = windowStart;
    tMixEnd = windowEnd;
    setDelayTimesUnchecked(minDelay, maxDelay);
    refreshWindow();
    unlockParamtersOnOff.store(true, std::memory_order_release);
}

void FDN::setT60(float t60) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    T60 = juce::jmax(t60, 0.0015f);
    setFilterCoefficients();
    refreshWindow();
    unlockParamtersOnOff.store(true, std::memory_order_release);
}

void FDN::setT60Ratio(float t60ratio) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    T60Ratio = juce::jmax(t60ratio, 0.001f);
    setFilterCoefficients();
    refreshWindow();
    unlockParamtersOnOff.store(true, std::memory_order_release);
}

void FDN::getPendingFilterCoefficients(float& gain, float& pole) {
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    gain = static_cast<float>(a0_temp[NumDelaylines - 1]);
    pole = static_cast<float>(p_temp[NumDelaylines - 1]);
}

void FDN::set_Volume(float Value) {
    Value = juce::jlimit(0.0f, 1.0f, Value);

    if (Value == 0) {
        fdnVol = 0;
    } else {
        fdnVol = 0.01 * exp(4.605170 * Value);
    }
}
