
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
/*
 *  ambverb~.c
 *  ambverb
 *
 *  Created by YesMachine on  26.02.16.
 *  Copyright 2016 __MyCompanyName__. All rights reserved.
 *
 */
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === #+++*/

#include "AmbVerb_FDN.h"
#include <AmbVerb_EARLYREF.h>

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Funktion Signatures - - - Funktion Signatures - - - Funktion Signatures - - - Funktion Signatures - - - Funktion Signatures
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

void * ambverb_new(t_floatarg order_arg);                                                  //Constructor
void ambverb_destroy(t_ambverb *x);                                                       //Destructor
void ambverb_dsp(t_ambverb *x, t_signal **sp);
t_int * DSPcycle(t_int *w);
void set_Distance(t_ambverb *x, t_floatarg r);

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

// DSP FUNCTION - - - - DSP FUNCTION - - - - DSP FUNCTION - - - - DSP FUNCTION - - - - DSP FUNCTION - - - - DSP FUNCTION - - - -
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

void ambverb_dsp(t_ambverb *x, t_signal **sp){
	int i;
	for(i=0; i <= (x->numChannels)*2;i++){ x->Signal[i] = sp[i];}  //safe signalpointers for dsp use
    
	dsp_add(DSPcycle, 2, x,sp[0]->s_n);

}

t_int * DSPcycle(t_int *w){
    
    int i,o;
    double time1,time2;
    
    t_ambverb *x = (t_ambverb *)(w[1]);			// Pointer on the t_class Object
	x->BlockSize = (int)(w[2]);					// BlockSize - (PD Preferences)
    
    for(i=0; i <= (x->numChannels)*2;i++){
        
        if(i <= (x->numChannels))
        {x->InSignal[i] = (t_sample*) x->Signal[i]->s_vec;
            //post("inSig%i = %f",i,*x->InSignal[i]);

        }// Input
        else
        {x->OutSignal[i-(x->numChannels)-1] = (t_sample*) x->Signal[i]->s_vec;
            
        }// Output
	}


    
    for (i=0; i<x->numChannels; i++) {
        vDSP_vclr(x->earlyref_InBuffer[i], 1, earlyref_Buffersize);
        memcpy(x->earlyref_InBuffer[i], x->InSignal[i], x->BlockSize*sizeof(float));
    }
    
    vDSP_vclr(x->fdn_InBuffer, 1, fdn_Buffersize);
    memcpy(x->fdn_InBuffer, x->InSignal[x->numChannels], x->BlockSize*sizeof(float));

    for (i=0; i<x->numChannels; i++){//Set to 0
        vDSP_vclr(x->OutSignal[i], 1 , x->BlockSize);
        vDSP_vclr(x->earlyref_Output[i], 1, earlyref_Buffersize);
        vDSP_vclr(x->fdn_Output[i], 1, fdn_Buffersize);



	}
    /*
    float MaximumValue;
    vDSP_vabs(x->DUMMYVECTOR1, 1, x->DUMMYVECTOR2, 1, x->BlockSize);
    vDSP_maxv(x->DUMMYVECTOR2, 1, &MaximumValue, x->BlockSize);
    post("Max = %f",MaximumValue);
    vDSP_vsdiv(x->DUMMYVECTOR1, 1, &MaximumValue, x->fdn_InBuffer, 1, x->BlockSize);
    
    */

    time1= clock();

        
        EARLYREF(x);
        FDN(x);
    
    if (x->check%200 == 0) {
        time2= clock();
        post("'AmbVerb_DSP():    Time: = %f", (time2-time1)/CLOCKS_PER_SEC*44100);
    }

    //vDSP_vclr(x->DUMMYVECTOR1, 1,  x->BlockSize);
    
    for (i=0; i<x->numChannels; i++){//Set to 0
        vDSP_vadd(x->fdn_Output[i], 1, x->earlyref_Output[i], 1, x->OutSignal[i], 1, x->BlockSize);
        
	}

    return (w+3);
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Constructor of the class
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void * ambverb_new(t_floatarg order_arg){
    
    
	t_ambverb *x		= (t_ambverb *) pd_new(ambverb_class);
    x->numChannels=pow(order_arg+1,2);
    x->Ambisonics_Order=order_arg;
    x->earlyref_OnOff = 1;
    x->earlyref_DryWetIn = x->earlyref_DryWetOut = 0.5;
    x->fdn_Vol = 0.5;
	
    post("channels = %i", x->numChannels);
    int i;
    
        
	for (i=0; i < x->numChannels; i++){
		x->inlet[i] = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

	}
	for (i=0; i < x->numChannels; i++){
		x->outlet[i] = outlet_new(&x->x_obj, &s_signal);
	}
    x->DUMMYVECTOR1 = getbytes(fdn_Buffersize*sizeof(float));
    earlyrefNew(x);
    fdnNew(x);
    set_Distance(x, 0);
    
    WindowIR(x);

    Analyse(x);
    //GetImpulsResponse(x);
	return (void *) x;
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Destroy the class
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void ambverb_destroy(t_ambverb *x) {
    
	post("free");
	int i;
    

    for (i=0; i < x->numChannels;i++){
		outlet_free(x->outlet[i]);

    }
    for (i=0; i < x->numChannels;i++){
		inlet_free(x->inlet[i]);

	}

    freebytes(x->DUMMYVECTOR1, fdn_Buffersize*sizeof(float));
    
    fdnDestroy(x);
    earlyrefDestroy(x);

    
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// EARLYREF Methods
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void set_DryWet(t_ambverb *x, t_floatarg f)
{
    if (f < 0 || f>1.0) {
        post("DryWet must be between 0 and 1");
        return;
    }
    x->earlyref_DryWetOut = f;
    x->earlyref_DryWetIn = 1.0 - f;
    
}

void set_RotAngle(t_ambverb *x, t_floatarg f)
{
    int temp = x->Q;
    x->Phi=f;
    CalculateDelay(x);
    x->Q = temp;
    
    CalculateRz(x);
    CalculateRx(x);
    CalculateRy(x);
    CalculateRxyz(x);
}

void set_Q(t_ambverb *x, t_floatarg f)
{
    int QMax = (int)(earlyref_Buffersize/(Trunc * 5.7));
    
    if (f<1 || f > QMax) {
        post("Q must be between 1 and %i", QMax);
        return;
        
    }
    
    x->Q=f;
    CalculateRz(x);
    CalculateRx(x);
    CalculateRy(x);
    CalculateRxyz(x);
    WindowIR(x);
}



void On(t_ambverb *x){    x->earlyref_OnOff = 1;}

void Off(t_ambverb *x){   x->earlyref_OnOff = 0;}

void DelayDirectSound(t_ambverb *x, t_floatarg f){
    
    int ts = f;
    
    if (ts < 0 || ts >= (earlyref_Buffersize-x->BlockSize)) {
        post("Delay must be between 0 and %i", (earlyref_Buffersize-x->BlockSize));
        return;
        
    }
    x->DirectSoundDelayTime = f;
}
void CalculateDelay(t_ambverb *x){
    
    int m;
    
    x->Q=1;
    CalculateRz(x);
    CalculateRx(x);
    CalculateRy(x);
    CalculateRxyz(x);
    
    GetImpulsResponse(x);
    
    for (m=0; m<x->numChannels; m++) {
        vDSP_vabs(x->earlyref_IR[m], 1, x->earlyref_IR[m], 1, earlyref_Buffersize);
    }
    for (m=1; m<x->numChannels; m++) {

        vDSP_vadd(x->earlyref_IR[m], 1, x->earlyref_IR[0], 1, x->earlyref_IR[0], 1, earlyref_Buffersize);
    }
    for (m=0; m<earlyref_Buffersize; m++) {
        if(fabs(*(x->earlyref_IR[0]+m)) > (0.001*x->numChannels)){
            x->DirectSoundDelayTime=m;
            post("IR(m) = %f, DirectSoundDelayTime= %i\n ",fabs(*(x->earlyref_IR[0]+m)),m);
            break;
            
        }
    }
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// FDN Methods
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void set_T60(t_ambverb *x, t_floatarg f)//, float Samplerate)
{
    
    post("set_T60");
    x->T60 = f;
    
    
    set_Gains(x);
    
    
    
}
void set_T60Ratio(t_ambverb *x, t_floatarg f){
    x->T60Ratio = f;
    set_Gains(x);
    
}

void reset_DelayTimes(t_ambverb *x){
    
    int dummy2[NumDelaylines] = {m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16};
	memcpy (&x->m, &dummy2, NumDelaylines*sizeof(int));
    set_Gains(x);
    
}
void set_DelayTimeMinMax(t_ambverb *x, t_floatarg min, t_floatarg max){
    get_IR(x);
    x->DelayMin = min;
    x->DelayMax = max;
    
    post("set Delays");
    int i;
    int MinMax[2];
    int L = sizeof(x->Prim)/sizeof(x->Prim[0]);
    for (i = 0; i < L; i++) {
        if (x->Prim[i] >= min) { MinMax[0] = i; break;}
        
    }
    
    for (i = 0; i < L; i++) {
        if (x->Prim[i] >= max) { MinMax[1] = i; break;}
        
    }
    
    int Step = (int) (MinMax[1]-MinMax[0])/NumDelaylines;
    
    //Variation = (int)Variation % Step;
    for (i = 0; i < NumDelaylines; i++) {
        x->m[i] = x->Prim[MinMax[0]+i*Step];
    }
    
    set_Gains(x);
}


void set_Gains(t_ambverb *x){
    
    
    int i;
    float Samplerate = sys_getsr();
    post("Samplerate = %f",Samplerate);
    
    if (x->T60 <= 0.0015) x->T60 = 0.0015;
    for (i = 0; i<NumDelaylines; i++){
        
        x->R0[i] = pow(10, -3.0*x->m[i]/(Samplerate*x->T60));
        x->Rpi[i] = pow(10, -3.0*x->m[i]/(Samplerate*x->T60*x->T60Ratio));
        
        x->p[i] = (x->R0[i] - x->Rpi[i])/(x->R0[i] + x->Rpi[i]);
        x->a0[i] = (2.0*x->R0[i]*x->Rpi[i])/(x->R0[i] + x->Rpi[i]);
        
        
        post("a:%lf p:%lf",x->a0[i],x->p[i]);
        
    }
    
    
    get_IR(x);
    
    
    /*
     int i;
     for (i = 0; i<NumDelaylines; i++){
     
     x->g[i] = Max_Gain * pow(x->alpha, x->m[i]);
     post("g:%f p:%f",x->g[i],x->p[i]);
     }
     */
}
void set_Distance(t_ambverb *x, t_floatarg r){
    if (r < 0 || r > 1) {
        post("(%f)-Error: Distance must be between 0 - 1",r);
        //return;
    }
    float Samplerate = sys_getsr();

    r = r*x->m[NumDelaylines-1];
    x->R0[NumDelaylines] = pow(10, -3.0*r/(Samplerate*x->T60));
    x->Rpi[NumDelaylines] = pow(10, -3.0*r/(Samplerate*x->T60*x->T60Ratio));
    
    x->p[NumDelaylines] = (x->R0[NumDelaylines] - x->Rpi[NumDelaylines])/(x->R0[NumDelaylines] + x->Rpi[NumDelaylines]);
    x->a0[NumDelaylines] = (2.0*x->R0[NumDelaylines]*x->Rpi[NumDelaylines])/(x->R0[NumDelaylines] + x->Rpi[NumDelaylines]);
    

}

void Volume1(t_ambverb *x, t_floatarg vol){
    
    if (vol < 0 || vol > 1) {
        post("(%f)-Error: Magnitude must be between 0 - 1",vol);
    }
    else x->earlyref_DryWetIn = vol;
}
void Volume2(t_ambverb *x, t_floatarg vol){
    
    if (vol < 0 || vol > 1) {
        post("(%f)-Error: Magnitude must be between 0 - 1",vol);
    }
    else x->earlyref_DryWetOut = vol;
}
void Volume3(t_ambverb *x, t_floatarg vol){
    
    if (vol < 0 || vol > 1) {
        post("(%f)-Error: Magnitude must be between 0 - 1",vol);
    }
    else x->fdn_Vol = vol;
}


/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Setup
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void ambverb_tilde_setup(void) {
	//post("setup");
    
	ambverb_class = class_new(gensym("ambverb~"),
                              (t_newmethod) ambverb_new, // Constructor
                              (t_method) ambverb_destroy, // Destructor
                              sizeof (t_ambverb),
                              CLASS_DEFAULT,
                              A_DEFFLOAT,
                              0);//Must always ends with a zero
	class_addmethod(ambverb_class, (t_method) ambverb_dsp, gensym("dsp"), 0);
  

	CLASS_MAINSIGNALIN(ambverb_class, t_ambverb, f);
	class_addmethod(ambverb_class, (t_method)set_RotAngle, gensym("RotAngle"), A_DEFFLOAT, 0);
	class_addmethod(ambverb_class, (t_method)set_DryWet, gensym("DryWet"), A_DEFFLOAT, 0);
	class_addmethod(ambverb_class, (t_method)set_Q, gensym("Q"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)On, gensym("On"), 0);
    class_addmethod(ambverb_class, (t_method)Off, gensym("Off"), 0);
    class_addmethod(ambverb_class, (t_method)DelayDirectSound, gensym("DelayDirectSound"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)set_T60, gensym("T60"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)set_T60Ratio, gensym("T60Ratio"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)set_DelayTimeMinMax, gensym("DelayTimeMinMax"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)reset_DelayTimes, gensym("ResetDelayTimes"), 0);
    class_addmethod(ambverb_class, (t_method)Volume1, gensym("MagDirect"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)Volume2, gensym("MagEarly"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)Volume3, gensym("MagLate"), A_DEFFLOAT, 0);
    class_addmethod(ambverb_class, (t_method)set_Distance, gensym("Distance"), A_DEFFLOAT, 0);



    
}


// EOF---------------------------------------------------//	-
