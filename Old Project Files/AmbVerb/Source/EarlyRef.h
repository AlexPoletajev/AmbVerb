//
//  EarlyRef.h
//  AmbVerb
//
//  Created by anonym on 15.04.17.
//
//

#ifndef __AmbVerb__EarlyRef__
#define __AmbVerb__EarlyRef__

#include <iostream>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>    /* srand, rand */
#include <string.h>
#include "time.h"
#include "math.h"
#include <Accelerate/Accelerate.h>
#include "CompilationFlags.h"

//==============================================================================


class EarlyRef
{
public:
    
    EarlyRef();
    ~EarlyRef();
    void processBlock(const float* Block[], int DspBlocksize, double pB_Samplerate); // DSP-Cycle
    void set_Q(float f);
    void set_RotAngle(float f);
    void set_EarlyrefVolume(float Value);
    int getQ(){return Q;};
    void readTransformationMatrix(std::string Path);//Einlesen der Rzx, Rxz, Rzy, Ryz Transformationsmatrizen
    void UnlockRotationMatrixForCalculaion(); //Freigabe der neuberechneten Matrizen für den Signalfluss (Vermeidung von Glitches)

    //==============================================================================
    
    float * OutBuffer[NumAmbisonicsChannels]; //Buffer für das Ergebnis der Rotationen
    float * Output[NumAmbisonicsChannels]; //Endgültiger Ausgabebuffer der Erstreflexionen
    int EarlyrefDelayTime, EndOfIR, IRsymmetryPoint; //Verzögerung, Anfangssample der Ersreflexionen, Ende der Erstreflexionen
    float FilterCoeffA, FilterCoeffB; // Filterkoeffizienten des Tiefpass'
    int OnsetLength;
    
private:
    float h(float alpha, float beta, int lambda); // hankelfunktion
    void fillhBuffer(float phi, int offset, int Q); // Berechnung der dünnbesetzten Impulsantworten für Rz
    void CalculateRotationMatrices(int QValue, float PhiValue); //Start der Matrizenberechnungen
    void CalculateRx();
    void CalculateRy();
    void CalculateRz(int QValue, float PhiValue);
    void CalculateRxyz();
    void CheckforNonZeroEntriesX();
    void CheckforNonZeroEntriesY();
    void CheckforNonZeroEntriesZ();
    void CheckforNonZeroEntriesXYZ();
    void MatrixConvolution(); //Mehrdimensional Faltung der Endgültigen Rotationsmatrix mit dem Eingangsvektor
    void FFTconvolution(float* IR1, DSPSplitComplex * IR2, float * FFTConvolutionBuffer); //Eindimensionale Faltung
    void On(); // Synthese An
    void Off(); //Synthese Aus
    float LowPass ( float *LP_Input, double LP_g, double LP_p, float LP_initialSample, float * LP_FilterBuffer, const int LP_Blocksize);
  
    //==============================================================================
    float earlyrefVolume;
    int Q, Q_TEMP, Phi;
	
    float * InBuffer[NumAmbisonicsChannels]; //Eingangssignal Vektor
    
    float * DummyMatrix1[NumAmbisonicsChannels][NumAmbisonicsChannels]; // Temporäre SignalMatrix für Berechnungen
    float * Rx[NumAmbisonicsChannels][NumAmbisonicsChannels], * Ry[NumAmbisonicsChannels][NumAmbisonicsChannels], * Rz10[NumAmbisonicsChannels][NumAmbisonicsChannels], * Rz13[NumAmbisonicsChannels][NumAmbisonicsChannels], * Rz19[NumAmbisonicsChannels][NumAmbisonicsChannels]; // Rotations-FaltungsMatrizen mit Impulsantworten im Zeitbereich
    float   Rxz[NumAmbisonicsChannels][NumAmbisonicsChannels],Rzx[NumAmbisonicsChannels][NumAmbisonicsChannels],Ryz[NumAmbisonicsChannels][NumAmbisonicsChannels],Rzy[NumAmbisonicsChannels][NumAmbisonicsChannels]; // Transformationsmatrizen zur Abbildung der x,y-Achse auf die z-Achse und zurück
    float * Rxyz[NumAmbisonicsChannels][NumAmbisonicsChannels];
    float * hBuffer[(AmbisonicsOrder*2+1)*2];   // dünnbesetzten Impulsantworten für Rz:  h(m,phi)
    float * dummyBuffer1[NumAmbisonicsChannels]; //Hilfsbuffer für Berechnungen
    float * IR[NumAmbisonicsChannels], *IR_TEMP[NumAmbisonicsChannels]; //Buffer für Impulsantworten des Systems
    float * DUMMYVECTOR1, * DUMMYVECTOR2; //Hilfsvektor für Berechnungen
    float Filter_InitialSample[NumAmbisonicsChannels]; //Initiales Sample für Tiefpass(Kausalität)
    float * FilterBuffer; //Buffer für Filterausgabe
        
    
    int NonZeroEntriesX[NumAmbisonicsChannels][NumAmbisonicsChannels],NonZeroEntriesY[NumAmbisonicsChannels][NumAmbisonicsChannels],NonZeroEntriesZ[NumAmbisonicsChannels][NumAmbisonicsChannels],NonZeroEntriesXYZ[NumAmbisonicsChannels][NumAmbisonicsChannels],NonZeroEntriesXYZ_TEMP[NumAmbisonicsChannels][NumAmbisonicsChannels]; //Andere Matrixeinträge können bei Berechnungen übersprungen werden
    
    // Variablen für fft
    DSPSplitComplex  fft_Rxyz[NumAmbisonicsChannels][NumAmbisonicsChannels],fft_Rxyz_TEMP[NumAmbisonicsChannels][NumAmbisonicsChannels], fft_Rx[NumAmbisonicsChannels][NumAmbisonicsChannels], fft_Ry[NumAmbisonicsChannels][NumAmbisonicsChannels], fft_Rz[NumAmbisonicsChannels][NumAmbisonicsChannels]; //Rotationsmatrizen im Spektralbereich
    DSPSplitComplex SplitComplexBuffer1, SplitComplexBuffer2 ; //HilfsMatrizen für Berechnungen im Spektralbereich
    float * FFTconvBuffer1, *FFTconvBuffer2; //Hilfs-buffer für Berechnungen im Spektralbereich
    FFTSetup fftConvSetup;
    float fftScale; //Skalierung der fft Ergebnisse
    
    //==============================================================================
};


#endif /* defined(__AmbVerb__EarlyRef__) */
