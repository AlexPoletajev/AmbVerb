//
//  FeedbackDelayNetwork.hpp
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#ifndef FeedbackDelayNetwork_hpp
#define FeedbackDelayNetwork_hpp

//==============================================================================
#include <Accelerate/Accelerate.h>
#include <iostream>
#include "CompilationFlags.h"
#include "math.h"
#include "time.h"

//==============================================================================

#define maxGain (0.999999 / sqrt(NumDelaylines))


//==============================================================================
#define m1      2207 // Initial Delaytimes for ith line in Samples
#define m2      2309
#define m3      2399
#define m4      2539
#define m5      2659
#define m6      2707
#define m7      2731
#define m8      2801
#define m9      2441
#define m10     2843
#define m11     2963
#define m12     3037
#define m13     3083
#define m14     3181
#define m15     3217
#define m16     3457

//================ ==============================================================
class FDN
{
public:

    FDN();
    ~FDN();

    //==============================================================================
    void processBlock(const float *Block, int DspBlocksize, double Samplerate); //DSP Cycle
    void setFilterCoefficients();
    float LowPass(float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float *LP_FilterBuffer, const int LP_Blocksize);
    void setT60(float t60); // Nachhallzeit bei 0Hz
    void setT60Ratio(float t60ratio); // [T60(0)]/[T60(Fs/2)]
    void setDelayTimes(float min, float max);  //Setzten der Delaytimes - Korrespondiert zur Raumgröße
    void set_Volume(float Value);
    void setWindowBoundries(int start, int end); // Festnter Grenzen
    void unlockParameters();
    void refreshWindow();
    int getMinDelaytime() {
        return minDelaytime;
    }

    int getMaxDelaytime() {
        return maxDelaytime;
    }

    //==============================================================================
    float *Output[NumAmbisonicsChannels];  //Ausgabe Buffer
    float T60, T60Ratio, T60_temp, T60Ratio_temp;
    double Samplerate;
    int check;
    float fdnVol;
    double p[NumDelaylines + 1], a0[NumDelaylines + 1]; //LowPass Filter Koeffizienten


    //==============================================================================
private:

    void FeedbackMatrix_Multiplikation(float *inSignal, float *rightEnd[], float *delayPoint[], const int Blocksize);
    void getIR(); // Messung der IR
    void getWindowedOutput(const int aW_Blocksize); // Faltung der gefensterten IR mit dem Eingangssignal
    void windowIR(); //Fensterung der IR


    //==============================================================================
    float *inBuffer;  //EingangsSignalvektor

    //==============================================================================
    float *Delayline_leftEnd[NumDelaylines];  // Delayline: Anfang
    float *Delayline_rightEnd[NumDelaylines];  // Delayline: Rechtes Ende
    float *Delayline_delayPoint[NumDelaylines];  // Delayline: Sample am Feedbackpunkt

    float *IR_Delayline_leftEnd[NumDelaylines];  // Delayline für die IR Messung: Anfang
    float *IR_Delayline_rightEnd[NumDelaylines];  // Delayline für die IR Messung: Rechtes Ende
    float *IR_Delayline_delayPoint[NumDelaylines];  //Delayline für die IR Messung: Sample am Feedbackpunkt

    float *IR_inBuffer;  //Eingangsbuffer für die IR Messung (Deltaimpuls)
    float *IR[NumDelaylines];  //Buffer für die Impulsantworten
    float *TempBuffer[NumDelaylines];  //Hilfsbuffer fpür Berechnungen
    float *IR_TempBuffer1, *IR_TempBuffer2;  //Hilfsbuffer für Berechnungen
    float *EarlyReflectionsBuffer[NumDelaylines];  //Signalvektor mit Direktschall und Erstreflexionen der FDN (wird vom Hauptsignal Subtrahiert)
    float *Window;  //Fenster

    float *TemporaryPointer;

    int FeedbackMarix[NumDelaylines][NumDelaylines];
    int DelayTimes[NumDelaylines], DelayTimes_temp[NumDelaylines]; // Delayzeiten der Feedbackspur

    double R0[NumDelaylines + 1], Rpi[NumDelaylines + 1]; //LowPass Filter Koeffizienten
    double p_temp[NumDelaylines + 1], a0_temp[NumDelaylines + 1]; //Temporäre LowPass Filter Koeffizienten

    float Filter_initialSample[2 * NumDelaylines]; //Initiales Sample für Tiefpass(Kausalität)
    float IR_Filter_initialSample[NumDelaylines];
    float *FilterBuffer;                                      //Ausgangsbuffer des Tiefpass'
    float *IR_FilterBuffer;

    int minDelaytime, maxDelaytime; //Aktuelle Grenzen der DElaytimes der Feedbackspur
    int Primnumbers[794]; //Primzahlen für die Delaytimes
    int CustomBlocksize; //Interne Blocksize in ProcessBLock()
    int tMixStart, tMixEnd; //Anfang und Ende der Fensterung

    bool unlockParamtersOnOff;

    //==============================================================================
    FFTSetup fftSetup;
    DSPSplitComplex fft_IR[NumDelaylines], fft_Delaylines[NumDelaylines], fft_IR_temp[NumDelaylines]; //Buffer für Berechnungen im Spektralbereich
    float fftScale; //Skalierung der fft Ergebnisse
    //==============================================================================
};

#endif /* FeedbackDelayNetwork_hpp */
