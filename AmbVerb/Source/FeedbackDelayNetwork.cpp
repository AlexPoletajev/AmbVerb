//
//  FeedbackDelayNetwork.cpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#include "FeedbackDelayNetwork.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>

namespace
{
void hadamardTransform(std::array<float, NumDelaylines>& values) noexcept
{
    static_assert((NumDelaylines & (NumDelaylines - 1)) == 0,
                  "The fast Hadamard transform requires a power-of-two delay count");

    for (std::size_t halfLength = 1;
         halfLength < static_cast<std::size_t>(NumDelaylines);
         halfLength *= 2) {
        for (std::size_t base = 0;
             base < static_cast<std::size_t>(NumDelaylines);
             base += 2 * halfLength) {
            for (std::size_t offset = 0; offset < halfLength; ++offset) {
                const float left = values[base + offset];
                const float right = values[base + offset + halfLength];
                values[base + offset] = left + right;
                values[base + offset + halfLength] = left - right;
            }
        }
    }
}
}

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

        fft_IR[i].allocate(fdn_Buffersize);
        fft_IR_temp[i].allocate(fdn_Buffersize);
        fft_Delaylines[i].allocate(fdn_Buffersize);

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

    fft_Input.allocate(fdn_Buffersize);

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

void FDN::prepare(double sampleRate, int maximumBlockSize) {
    if (sampleRate <= 0.0
        || maximumBlockSize <= 0
        || static_cast<std::size_t>(maximumBlockSize) > fdn_Buffersize)
        throw std::invalid_argument("Unsupported FDN sample rate or maximum block size");

    // The processor applies its complete parameter snapshot immediately after
    // prepare(). Rebuilding the window here as well used to generate the same
    // 16-channel correction IR twice for every prepareToPlay() call.
    const juce::SpinLock::ScopedLockType lock(parameterLock);
    Samplerate = sampleRate;
    preparedMaximumBlockSize = maximumBlockSize;
    activeConvolutionBank.reset();
    pendingConvolutionBank.reset();
    unlockParamtersOnOff.store(false, std::memory_order_release);

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

    delayWritePosition = 0;
    earlyBufferReadPosition = 0;

    if (activeConvolutionBank != nullptr)
        activeConvolutionBank->reset();

    if (pendingConvolutionBank != nullptr)
        pendingConvolutionBank->reset();
}

void FDN::processBlock(const float *Block, int DspBlocksize) {
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

    int NumCycles;
    int CustomBlocksize_temp;

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
        juce::FloatVectorOperations::clear(Output[i], DspBlocksize);
    }

    memcpy(inBuffer, Block, DspBlocksize * sizeof(float)); // Eingangssignalvektor setzten

    for (int Cycle = 0; Cycle < NumCycles; Cycle++) {
        if (Cycle == (NumCycles - 1) && NumCycles != 1) {
            CustomBlocksize = DspBlocksize - (CustomBlocksize * (NumCycles - 1));
        }

        const int outputOffset = Cycle * CustomBlocksize_temp;
        std::array<float, NumDelaylines> feedbackOutput {};

        for (int sample = 0; sample < CustomBlocksize; ++sample) {
            const auto writePosition = (delayWritePosition + static_cast<std::size_t>(sample))
                & (fdn_Buffersize - 1);

            for (int inputLine = 0; inputLine < NumDelaylines; ++inputLine) {
                const auto delayPosition = (writePosition + fdn_Buffersize
                                            - static_cast<std::size_t>(DelayTimes[inputLine]))
                    & (fdn_Buffersize - 1);
                feedbackOutput[static_cast<std::size_t>(inputLine)]
                    = Delayline_leftEnd[inputLine][delayPosition];
            }

            hadamardTransform(feedbackOutput);

            for (auto& value : feedbackOutput)
                value = value * maxGain + Block[outputOffset + sample];

            for (int outputLine = 0; outputLine < NumDelaylines; ++outputLine) {
                const auto filtered = static_cast<float>(
                    feedbackOutput[static_cast<std::size_t>(outputLine)] * a0[outputLine]
                    + Filter_initialSample[outputLine] * p[outputLine]);
                Filter_initialSample[outputLine] = filtered;
                Delayline_leftEnd[outputLine][writePosition] = filtered;
            }

            for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
                Output[channel][static_cast<std::size_t>(outputOffset + sample)]
                    = Delayline_leftEnd[channel % NumDelaylines][writePosition] * fdnVol;
            }
        }

        delayWritePosition = (delayWritePosition + static_cast<std::size_t>(CustomBlocksize))
            & (fdn_Buffersize - 1);
    }

    /*---- -- -- -- -- -- Signalspur des Direktschalls und der Ersreflexionen -- -- -- -- -- -- -- --*/

    if (activeConvolutionBank != nullptr) {
        const float* inputPointers[] { Block };
        std::array<float*, NumDelaylines> correctionPointers {};

        for (int line = 0; line < NumDelaylines; ++line)
            correctionPointers[static_cast<std::size_t>(line)] = TempBuffer[line];

        activeConvolutionBank->process(inputPointers,
                                       correctionPointers.data(),
                                       DspBlocksize);

        for (int channel = 0; channel < NumAmbisonicsChannels; ++channel) {
            juce::FloatVectorOperations::addWithMultiply(
                Output[channel],
                TempBuffer[channel % NumDelaylines],
                -fdnVol,
                DspBlocksize);
        }
    } else {
        getWindowedOutput(DspBlocksize);

        const auto firstPart = juce::jmin(static_cast<std::size_t>(DspBlocksize),
                                          fdn_Buffersize - earlyBufferReadPosition);

        for (int i = 0; i < NumAmbisonicsChannels; i++) {
            juce::FloatVectorOperations::addWithMultiply(Output[i],
                                                          EarlyReflectionsBuffer[i % NumDelaylines]
                                                              + earlyBufferReadPosition,
                                                          -fdnVol,
                                                          static_cast<int>(firstPart));

            if (firstPart < static_cast<std::size_t>(DspBlocksize)) {
                juce::FloatVectorOperations::addWithMultiply(
                    Output[i] + firstPart,
                    EarlyReflectionsBuffer[i % NumDelaylines],
                    -fdnVol,
                    DspBlocksize - static_cast<int>(firstPart));
            }
        }

        for (int i = 0; i < NumDelaylines; ++i) {
            juce::FloatVectorOperations::clear(EarlyReflectionsBuffer[i]
                                                   + earlyBufferReadPosition,
                                               static_cast<int>(firstPart));

            if (firstPart < static_cast<std::size_t>(DspBlocksize)) {
                juce::FloatVectorOperations::clear(
                    EarlyReflectionsBuffer[i],
                    DspBlocksize - static_cast<int>(firstPart));
            }
        }

        earlyBufferReadPosition = (earlyBufferReadPosition
                                   + static_cast<std::size_t>(DspBlocksize))
            & (fdn_Buffersize - 1);
    }
}

void FDN::FeedbackMatrix_Multiplikation(float *inSignal, float *rightEnd[], float *delayPoint[], const int FB_Blocksize) {
    std::array<float, NumDelaylines> values {};

    for (int sample = 0; sample < FB_Blocksize; ++sample) {
        for (int inputLine = 0; inputLine < NumDelaylines; ++inputLine)
            values[static_cast<std::size_t>(inputLine)] = delayPoint[inputLine][sample];

        hadamardTransform(values);

        for (int outputLine = 0; outputLine < NumDelaylines; ++outputLine) {
            rightEnd[outputLine][sample]
                += values[static_cast<std::size_t>(outputLine)] * maxGain
                + inSignal[sample];
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
    Window.clear();

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

    IR_TempBuffer1.clear();

    for (int i = 0; i < NumDelaylines; i++) {
        juce::FloatVectorOperations::multiply(IR_TempBuffer1,
                                               Window,
                                               IR[i],
                                               fdn_Buffersize);
        parameterFft.forwardVdspCompatible(IR_TempBuffer1, fft_IR_temp[i]);
    }

    buildPendingConvolutionBank();
}

void FDN::buildPendingConvolutionBank() {
    if (!UsePartitionedRuntimeConvolution)
        return;

    if (Samplerate <= 0.0 || preparedMaximumBlockSize <= 0)
        return;

    auto bank = std::make_unique<PartitionedConvolutionBank>(Samplerate,
                                                              preparedMaximumBlockSize,
                                                              1,
                                                              NumDelaylines);

    for (int line = 0; line < NumDelaylines; ++line) {
        juce::FloatVectorOperations::multiply(IR_TempBuffer1,
                                               Window,
                                               IR[line],
                                               fdn_Buffersize);
        bank->addRoute(0,
                       line,
                       IR_TempBuffer1,
                       fdn_Buffersize);
    }

    bank->prepare();
    pendingConvolutionBank = std::move(bank);
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
    IR_TempBuffer1.clear();
    memcpy(IR_TempBuffer1, inBuffer, aW_Blocksize * sizeof(float));
    runtimeFft.forwardVdspCompatible(IR_TempBuffer1, fft_Input);

    for (int i = 0; i < NumDelaylines; i++) {
        PortableRealFft::multiply(fft_IR[i], fft_Input, fft_Delaylines[i]);
        runtimeFft.inverseVdspCompatible(fft_Delaylines[i], TempBuffer[i]);
        const auto firstPart = fdn_Buffersize - earlyBufferReadPosition;
        juce::FloatVectorOperations::addWithMultiply(
            EarlyReflectionsBuffer[i] + earlyBufferReadPosition,
            TempBuffer[i],
            fftScale,
            static_cast<int>(firstPart));

        if (firstPart < fdn_Buffersize) {
            juce::FloatVectorOperations::addWithMultiply(
                EarlyReflectionsBuffer[i],
                TempBuffer[i] + firstPart,
                fftScale,
                static_cast<int>(fdn_Buffersize - firstPart));
        }
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

    if (pendingConvolutionBank != nullptr)
        activeConvolutionBank.swap(pendingConvolutionBank);
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
