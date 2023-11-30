//
//  AmbVerb_Widening.h
//  AmbVerb
//
//  Created by anonym on 12.03.17.
//  Copyright (c) 2017 anonym. All rights reserved.
//
#include "AmbVerbStruct.h"

#ifndef AmbVerb_AmbVerb_EARLYREF_h
#define AmbVerb_AmbVerb_EARLYREF_h


void EARLYREF(t_ambverb *x);
void readTransformationMatrix(t_ambverb *x);                                                               //read rotation Matrices
void CalculateRx(t_ambverb *x);                                                            //(Rotation of Impulsresponses Rz)
void CalculateRy(t_ambverb *x);                                                            //(Rotation of Impulsresponses Rz)
void CalculateRxyz(t_ambverb *x);                                                          //Convolution : Rx*Ry*Rz

float h(t_float alpha, t_float beta, int lambda);                                           //Calculate hankelfunctions
void fillhBuffer(t_ambverb *x, float phi, int offset, int Q);                              //Calculate Impuslresponses
void CalculateRz(t_ambverb *x);                              //Fill rotationmatrix wit IR's
void MatrixConvolution(t_ambverb *x);                                                      //Convolution of Rz with Signal
void CheckforNonZeroEntriesX(t_ambverb *x);
void CheckforNonZeroEntriesY(t_ambverb *x);
void CheckforNonZeroEntriesZ(t_ambverb *x);
void CheckforNonZeroEntriesXYZ(t_ambverb *x);
void FFTconvolution(t_ambverb *x, float* IR1, int Length1, DSPSplitComplex * IR2, int Length2, float* Output, int Length3 );
void GetImpulsResponse(t_ambverb *x);
void Analyse(t_ambverb *x);
void CalculateDelay(t_ambverb *x);
void earlyrefNew(t_ambverb *x);
void earlyrefDestroy(t_ambverb *x);
#endif
