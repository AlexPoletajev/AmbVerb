//
//  AmbVerb_EARLYREF.c
//  AmbVerb
//
//  Created by anonym on 12.03.17.
//  Copyright (c) 2017 anonym. All rights reserved.
//

#include <stdio.h>
#include "AmbVerb_EARLYREF.h"
#include "AmbVerb_FDN.h"

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Constructor
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void earlyrefNew(t_ambverb *x){

    int i,u;
    
    
    
	
	for (i=0; i < x->numChannels; i++){
		x->earlyref_InBuffer[i] = 	getbytes( (earlyref_Buffersize) * sizeof(t_sample) );  // Allocated Memory for earlyref_InBuffer
		x->earlyref_OutBuffer[i] = 	getbytes( (earlyref_Buffersize) * sizeof(t_sample) );  // Allocated Memory for earlyref_OutBuffer
        x->earlyref_dummyBuffer1[i] = getbytes( (earlyref_Buffersize) * sizeof(t_sample));
        x->earlyref_IR[i] = getbytes( (earlyref_Buffersize) * sizeof(t_sample));
        x->earlyref_Output[i] = getbytes(earlyref_Buffersize*sizeof(float));
        x->earlyref_Filter_InitialSample[i]=0;
        
        
	}
    
    //.....fft
    x->earlyref_FFTconvBuffer1 = getbytes(earlyref_Buffersize* sizeof(t_sample));
    x->earlyref_FFTconvBuffer2 = getbytes(earlyref_Buffersize* sizeof(t_sample));
    x->earlyref_FilterBuffer = getbytes(earlyref_Buffersize* sizeof(t_sample));

    x->earlyref_SplitComplexBuffer.realp = getbytes(earlyref_Buffersize/2* sizeof(t_sample));
    x->earlyref_SplitComplexBuffer.imagp = getbytes(earlyref_Buffersize/2* sizeof(t_sample));
    x->earlyref_fft_scale =0.5f/(float)(16.0f*earlyref_Buffersize);
    x->earlyref_fftConvSetup = vDSP_create_fftsetup(earlyref_Log2N,FFT_RADIX2);
    
    //......
    
    for (i=0; i < x->numChannels; i++){
        for (u=0; u < x->numChannels; u++){
            
            x->Rx[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            x->Ry[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around y
            x->Rz10[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around z
            x->Rz13[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around z
            x->Rz19[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around z
            
            
            x->earlyref_DummyMatrix1[i][u] = getbytes((earlyref_Buffersize) * sizeof(t_sample)); // Temporary Matrix for Calculations
            
            
            x->fft_Rx[i][u].realp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around x
            x->fft_Rx[i][u].imagp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around x
            x->fft_Ry[i][u].realp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around y
            x->fft_Ry[i][u].imagp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around y
            x->fft_Rz[i][u].realp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around z
            x->fft_Rz[i][u].imagp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around z
            x->fft_Rxyz[i][u].realp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around xyz
            x->fft_Rxyz[i][u].imagp = getbytes((earlyref_Buffersize/2) * sizeof(t_sample)); // fft Rotationmatrix around xyz
        }
        
        
        
    }
    
    for (i=0; i<(x->Ambisonics_Order*2+1)*2; i++) {
        x->hBuffer[i]=getbytes((earlyref_Buffersize) * sizeof(t_sample));
    }
    //parameter inlets:
    //inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("DelayDirectSound"));
	//inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("DryWet"));
    
    
    
    readTransformationMatrix(x);
    
    x->Phi=80.0;
    CalculateDelay(x);
    x->Q=400.0;
    x->check=1;
    
    CalculateRz(x);
    
    CalculateRx(x);
    CalculateRy(x);
    CalculateRxyz(x);
    
}

void earlyrefDestroy(t_ambverb *x){
    
    int i,u;
    
    for (i=0; i < x->numChannels; i++){
        for (u=0; u < x->numChannels; u++){
            freebytes(x->Rx[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->Ry[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->Rz10[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->Rz13[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->Rz19[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->earlyref_DummyMatrix1[i][u],(earlyref_Buffersize) * sizeof(t_sample)); // Temporary Matrix for Calculations
            
            // fft
            freebytes(x->fft_Rx[i][u].realp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Rx[i][u].imagp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Ry[i][u].realp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Ry[i][u].imagp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Rz[i][u].realp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Rz[i][u].imagp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Rotationmatrix around x
            freebytes(x->fft_Rxyz[i][u].realp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Final RotationMatrix around xyz
            freebytes(x->fft_Rxyz[i][u].imagp,(earlyref_Buffersize/2) * sizeof(t_sample)); // Final RotationMatrix around xyz
            
        }
    }
    
    for (i=0; i<(x->Ambisonics_Order*2+1)*2; i++) {
        freebytes(x->hBuffer[i],(earlyref_Buffersize) * sizeof(t_sample));
    }
    
	for (i=0; i < x->numChannels;i++){
		freebytes(x->earlyref_InBuffer[i], (earlyref_Buffersize) * sizeof(t_sample));  // Free Allocated Memory
		freebytes(x->earlyref_OutBuffer[i], (earlyref_Buffersize) * sizeof(t_sample));  // Free Allocated Memory
        freebytes(x->earlyref_dummyBuffer1[i],(earlyref_Buffersize) *sizeof(float));
        freebytes(x->earlyref_IR[i],(earlyref_Buffersize) *sizeof(float));
        freebytes(x->earlyref_Output[i], earlyref_Buffersize*sizeof(float));

        
        
	}
    
    
    
    
    //.....fft
    freebytes(x->earlyref_FFTconvBuffer1, earlyref_Buffersize*sizeof(float));
    freebytes(x->earlyref_FFTconvBuffer2, earlyref_Buffersize*sizeof(float));
    freebytes(x->earlyref_FilterBuffer, earlyref_Buffersize*sizeof(float));

    freebytes(x->earlyref_SplitComplexBuffer.realp, earlyref_Buffersize/2*sizeof(float));
    freebytes(x->earlyref_SplitComplexBuffer.imagp, earlyref_Buffersize/2*sizeof(float));
    
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Widening
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

void EARLYREF(t_ambverb *x) {
    
    int i;
    double time1,time2;
    time1= clock();
    if (x->earlyref_OnOff == 1) {
        MatrixConvolution(x);
        
    }
    if (x->check%200 == 0) {
        time2= clock();
        post("'MAtrixConvolutiom():    Time: = %f", (time2-time1)/CLOCKS_PER_SEC*44100);
        
    }x->check++;
    
    //post("IO");
    for (i=0; i<x->numChannels; i++) {
        
        
         vDSP_vsmsma(x->earlyref_InBuffer[i],1,&x->earlyref_DryWetIn,x->earlyref_OutBuffer[i]+x->DirectSoundDelayTime*x->Q,1,&x->earlyref_DryWetOut, x->earlyref_Output[i],1,x->BlockSize);
        
        x->earlyref_Filter_InitialSample[i] = simplp(x, x->earlyref_Output[i],x->a0[NumDelaylines],x->p[NumDelaylines], x->earlyref_Filter_InitialSample[i], x->earlyref_FilterBuffer, x->BlockSize);
        memcpy(x->earlyref_Output[i], x->earlyref_FilterBuffer, x->BlockSize * sizeof(float));
        //post("delay = %i",x->DirectSoundDelayTime*x->Q );
        
         if (x->check%201 == 0) {
         //post("%f",*x->earlyref_InBuffer[i]);
         
         
         }
        /*
         if (x->check%202 == 0) {
         post("%f",*x->earlyref_OutBuffer[i]);
         
         }
         */
        
        memmove(x->earlyref_OutBuffer[i], x->earlyref_OutBuffer[i]+x->BlockSize, (earlyref_Buffersize-x->BlockSize)*sizeof(float));
        vDSP_vclr(x->earlyref_OutBuffer[i]+(earlyref_Buffersize-x->BlockSize-1), 1, x->BlockSize);
  
        
        
    }
    
}




/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// calculate hankelfunktion
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
float h(t_float alpha, float beta, int lambda)
{
    return cosf(M_PI/2.0*abs(lambda) + beta) * jn(abs(lambda),fabs(alpha));
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Fill hBuffer
// numberering: m=-order .... m=0 ... m=order
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void fillhBuffer(t_ambverb *x, float phi, int offset, int Q)
{
    
    int m,i,lambda;
    
    for (i=0; i<(x->Ambisonics_Order*2+1)*2; i++) {
        cblas_sscal(earlyref_Buffersize, 0.0, x->hBuffer[i], 1);
    }
    
    
    i=0;
    for (m=-x->Ambisonics_Order; m<=x->Ambisonics_Order; m++) {
        
        for (lambda=0; lambda<=offset*2; lambda++) {
            if (m<0) {
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,-M_PI/2,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
                
            }
            else if(m>0){
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,0,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
            }
            else{
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,0,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
            }
        }
        i++;
        
        for (lambda=0; lambda<=offset*2; lambda++) {
            if (m<0) {
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,0,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
            }
            else if(m>0){
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,M_PI/2,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
            }
            else{
                *(x->hBuffer[i]+Q*lambda)=h(m*phi,0,lambda-offset);
                //post("lambda-offset=%i  m=%i  hBuffer[%i] = %f",lambda-offset,m,i,*(x->hBuffer[i]+Q*lambda));
                
            }
        }
        i++;
        
        
    }
    
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// calculate Rz
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void CalculateRz(t_ambverb *x)
{
    int l,m,i;
    
    for (l=0; l < x->numChannels; l++) {            // set Rz to zero
        for (m=0; m < x->numChannels; m++) {
            
            cblas_sscal(earlyref_Buffersize, 0.0, x->Rz10[l][m], 1);
            cblas_sscal(earlyref_Buffersize, 0.0, x->Rz13[l][m], 1);
            cblas_sscal(earlyref_Buffersize, 0.0, x->Rz19[l][m], 1);
            
            
        }
    }
    
    fillhBuffer(x, x->Phi/360.0*2.0*M_PI, Trunc, (int)(x->Q));
    
    i=0;
    for (l=0; l<=x->Ambisonics_Order; l++) {
        for (m = -l; m <= l; m++) {
            
            memcpy(x->Rz10[l*(l+1)+m][l*(l+1)+abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2], earlyref_Buffersize * sizeof(float));
            memcpy(x->Rz10[l*(l+1)+m][l*(l+1)-abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2+1], earlyref_Buffersize * sizeof(float));
            
        }
    }
    
    fillhBuffer(x, x->Phi/360.0*2.0*M_PI, Trunc, (int)(x->Q*Qy));
    i=0;
    for (l=0; l<=x->Ambisonics_Order; l++) {
        for (m = -l; m <= l; m++) {
            
            memcpy(x->Rz13[l*(l+1)+m][l*(l+1)+abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2], earlyref_Buffersize * sizeof(float));
            memcpy(x->Rz13[l*(l+1)+m][l*(l+1)-abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2+1], earlyref_Buffersize * sizeof(float));
            
        }
    }
    
    fillhBuffer(x, x->Phi/360.0*2.0*M_PI, Trunc, (int)(x->Q*Qx));
    i=0;
    for (l=0; l<=x->Ambisonics_Order; l++) {
        for (m = -l; m <= l; m++) {
            
            memcpy(x->Rz19[l*(l+1)+m][l*(l+1)+abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2], earlyref_Buffersize * sizeof(float));
            memcpy(x->Rz19[l*(l+1)+m][l*(l+1)-abs(m)], x->hBuffer[(m+x->Ambisonics_Order)*2+1], earlyref_Buffersize * sizeof(float));
            
        }
    }
    CheckforNonZeroEntriesZ(x);
    
}



/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Calculate Rx
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void CalculateRx(t_ambverb *x){
    
    int a,b,i;
    
    i=0;
    for (a=0; a < x->numChannels; a++) {            // set DummyMatrix to zero
        for (b=0; b < x->numChannels; b++) {
            
            cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_DummyMatrix1[a][b], 1);
            
        }
    }
    
    
    // ::::: Rxz*Rz ::::: //
    
    for (a=0; a < x->numChannels; a++) {            //Multiplication
        
        for (b=0; b < x->numChannels; b++) {
            
            for (i=0; i < x->numChannels; i++) {
                
                //if (x->NonZeroEntriesZ[i][b]==1) {
                
                cblas_saxpy(earlyref_Buffersize, x->Rxz[a][i],x->Rz19[i][b], 1, x->earlyref_DummyMatrix1[a][b], 1);
                //}
            }
            
        }
        
    }
    
    for (a=0; a < x->numChannels; a++) {            //Copy results to Rx
        
        for (b=0; b < x->numChannels; b++) {
            
            
            memcpy(x->Rx[a][b], x->earlyref_DummyMatrix1[a][b], earlyref_Buffersize*sizeof(float));
            
        }
        
    }
    
    
    // ::::: (Rxz*Rz)*Rzx ::::: //
    
    for (a=0; a < x->numChannels; a++) {            // set DummyMatrix to zero
        for (b=0; b < x->numChannels; b++) {
            
            cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_DummyMatrix1[a][b], 1);
            
        }
    }
    
    for (a=0; a < x->numChannels; a++) {            //Multiplication
        
        for (b=0; b < x->numChannels; b++) {
            
            for (i=0; i < x->numChannels; i++) {
                
                cblas_saxpy(earlyref_Buffersize, x->Rzx[i][b],x->Rx[a][i], 1, x->earlyref_DummyMatrix1[a][b], 1);
                
            }
            
        }
        
    }
    
    for (a=0; a < x->numChannels; a++) {            //Copy results to Rx
        
        for (b=0; b < x->numChannels; b++) {
            
            
            memcpy(x->Rx[a][b], x->earlyref_DummyMatrix1[a][b], earlyref_Buffersize*sizeof(float));
            
        }
        
    }
    
    CheckforNonZeroEntriesX(x);
    
    
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Calculate Ry
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void CalculateRy(t_ambverb *x){
    
    int a,b,i;
    
    i=0;
    for (a=0; a < x->numChannels; a++) {            // set DummyMatrix to zero
        for (b=0; b < x->numChannels; b++) {
            
            cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_DummyMatrix1[a][b], 1);
            
        }
    }
    
    
    // ::::: Rxz*Rz ::::: //
    
    for (a=0; a < x->numChannels; a++) {            //Multiplication
        
        for (b=0; b < x->numChannels; b++) {
            
            for (i=0; i < x->numChannels; i++) {
                
                cblas_saxpy(earlyref_Buffersize, x->Ryz[a][i],x->Rz13[i][b], 1, x->earlyref_DummyMatrix1[a][b], 1);
                
            }
            
        }
        
    }
    
    for (a=0; a < x->numChannels; a++) {            //Copy results to Rx
        
        for (b=0; b < x->numChannels; b++) {
            
            
            memcpy(x->Ry[a][b], x->earlyref_DummyMatrix1[a][b], earlyref_Buffersize*sizeof(float));
            
        }
        
    }
    
    
    // ::::: (Rxz*Rz)*Rzx ::::: //
    
    for (a=0; a < x->numChannels; a++) {            // set DummyMatrix to zero
        for (b=0; b < x->numChannels; b++) {
            
            cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_DummyMatrix1[a][b], 1);
            
        }
    }
    
    for (a=0; a < x->numChannels; a++) {            //Multiplication
        
        for (b=0; b < x->numChannels; b++) {
            
            for (i=0; i < x->numChannels; i++) {
                
                cblas_saxpy(earlyref_Buffersize, x->Rzy[i][b],x->Ry[a][i], 1, x->earlyref_DummyMatrix1[a][b], 1);
                
            }
            
        }
        
    }
    
    for (a=0; a < x->numChannels; a++) {            //Copy results to Rx
        
        for (b=0; b < x->numChannels; b++) {
            
            
            memcpy(x->Ry[a][b], x->earlyref_DummyMatrix1[a][b], earlyref_Buffersize*sizeof(float));
            
        }
        
    }
    
    CheckforNonZeroEntriesY(x);
    
    
}

void Analyse(t_ambverb *x)
{
    
    double time1, time2;
    int m,n;
    char buffer[100];
    int cx;
    FILE* file;
    
    int Winkel, Q;
    for (Winkel=80; Winkel<=80; Winkel=Winkel+1) {
        
        for (Q=400; Q<=400; Q=Q+1) {
            
            x->Q=Q;
            x->Phi=Winkel;
            CalculateRz(x);
            CalculateRx(x);
            CalculateRy(x);
            CalculateRxyz(x);
            time1=clock();
            
            GetImpulsResponse(x);
            time2 = clock();
            post("'GetImpulseRespone():    Time: = %f", (time2-time1)/CLOCKS_PER_SEC*44100);
            
            /*
             cx = snprintf ( buffer, 100, "/Users/anonym/Documents/MATLAB/Analyse/hBufferQxQy1_%i_%i_%i", (int)(x->Phi), x->Q, Trunc);
             file = fopen (buffer, "w");
             
             if(file == NULL)
             {
             post("Error!");
             exit(1);
             }
             
             for (m=0; m < earlyref_Buffersize; m++) {
             fprintf(file,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",*(x->hBuffer[0]+m),*(x->hBuffer[1]+m),*(x->hBuffer[2]+m),
             *(x->hBuffer[3]+m),*(x->hBuffer[4]+m),*(x->hBuffer[5]+m),*(x->hBuffer[6]+m),*(x->hBuffer[7]+m),
             *(x->hBuffer[8]+m),*(x->hBuffer[9]+m),*(x->hBuffer[10]+m),*(x->hBuffer[11]+m),*(x->hBuffer[12]+m),*(x->hBuffer[13]+m));
             
             }
             fclose(file);
             */
            cx = snprintf ( buffer, 100, "/Users/anonym/Documents/MATLAB/Analyse/IR_%i_%i_%i", (int)(x->Phi), x->Q, Trunc);
            file = fopen (buffer, "w");
            
            if(file == NULL)
            {
                post("Error!");
                exit(1);
            }
            
            for (m=1; m<x->numChannels; m++) {
                //vDSP_vadd(x->earlyref_IR[m], 1, x->earlyref_IR[0], 1, x->earlyref_IR[0], 1, earlyref_Buffersize);
            }
            
            
            
            
             for (m=0; m < earlyref_Buffersize; m++) {
             fprintf(file,"%f\n",*(x->earlyref_IR[0]+m));
             }
             /*
             for (m=0; m < earlyref_Buffersize; m++) {
             fprintf(file,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",*(x->earlyref_IR[0]+m),*(x->earlyref_IR[1]+m),*(x->earlyref_IR[2]+m),*(x->earlyref_IR[3]+m),*(x->earlyref_IR[4]+m),*(x->earlyref_IR[5]+m),*(x->earlyref_IR[6]+m),*(x->earlyref_IR[7]+m),*(x->earlyref_IR[8]+m),*(x->earlyref_IR[9]+m),*(x->earlyref_IR[10]+m),*(x->earlyref_IR[11]+m),*(x->earlyref_IR[12]+m),*(x->earlyref_IR[13]+m),*(x->earlyref_IR[14]+m),*(x->earlyref_IR[15]+m));
             
             }*/
            fclose(file);
        }
    }
    
    
    
    
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Get ImpulsRespone
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void GetImpulsResponse(t_ambverb *x){
    int m,b,i;

    //float Delta[]={1, 0, 0, 0.2823095, 0, 0, -0.5, 0, 0.244301, 0, 0, 0, 0, -0.172747, 0, 0.223016};
    float Delta[]={1, -0.270440251572327,0, -0.0754716981132076, 0.125786163522013, 0, -0.496855345911950, 0, -0.207547169811321, 0.150943396226415, 0, 0.163522012578616, 0, 0.0440251572327044,0, 0.163522012578616, -0.182389937106918,  0, -0.0817610062893082, 0, 0.377358490566038, 0, 0.132075471698113, 0, 0.100628930817610};
    
    for (i=0; i<x->numChannels; i++) { // set Buffer to zero
        cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_dummyBuffer1[i], 1);
        *(x->earlyref_dummyBuffer1[i]) = Delta[i];
        cblas_sscal(earlyref_Buffersize, 0.0, x->earlyref_IR[i], 1);
        
        
    }
    
    
    for (m=0; m < x->numChannels; m++) {
        
        
        for (b=0; b < x->numChannels; b++) {
            
            if (x->NonZeroEntriesXYZ[m][b]==1) {
                
                FFTconvolution(x, x->earlyref_dummyBuffer1[b], 1, &x->fft_Rxyz[m][b], 1, x->earlyref_OutBuffer[m], 1);
                vDSP_vsma(x->earlyref_FFTconvBuffer1, 1,&x->earlyref_fft_scale,x->earlyref_IR[m],1,x->earlyref_IR[m],1,earlyref_Buffersize);
            }
        }
    }
    
    
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Matrix Convolution
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void MatrixConvolution(t_ambverb *x){
    
    
    int m,b,i;
    
    for (i=0; i<x->numChannels; i++) { // set Buffer to zero
        cblas_sscal(x->BlockSize, 0, x->earlyref_dummyBuffer1[i], 1);
    }
    
    
    for (m=0; m < x->numChannels; m++) {
        
        
        for (b=0; b < x->numChannels; b++) {
            
            if (x->NonZeroEntriesXYZ[m][b]==1) {
                FFTconvolution(x, x->earlyref_InBuffer[b], 1, &x->fft_Rxyz[m][b], 1, x->earlyref_OutBuffer[m], 1);
                vDSP_vsma(x->earlyref_FFTconvBuffer1, 1,&x->earlyref_fft_scale,x->earlyref_OutBuffer[m],1,x->earlyref_OutBuffer[m],1,earlyref_Buffersize);
                
            }
        }
    }
    
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// FFT Convolution of IR and IR
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void FFTconvolution(t_ambverb *x, float* IR1, int Length1, DSPSplitComplex * IR2, int Length2, float* Output, int Length3 ){
    
    
    vDSP_ctoz((DSPComplex *) IR1, 2, &x->earlyref_SplitComplexBuffer, 1, earlyref_Buffersize/2);
    
    vDSP_fft_zrip(x->earlyref_fftConvSetup,&x->earlyref_SplitComplexBuffer, 1, earlyref_Log2N, FFT_FORWARD);
    
    x->earlyref_NyquistBit=x->earlyref_SplitComplexBuffer.imagp[0] * (*IR2).imagp[0]; //Nyquistbit Correction
    x->earlyref_SplitComplexBuffer.imagp[0]=0;
    (*IR2).imagp[0]=0;
    vDSP_zvmul(&x->earlyref_SplitComplexBuffer, 1, IR2, 1, &x->earlyref_SplitComplexBuffer, 1, earlyref_Buffersize/2, 1);
    x->earlyref_SplitComplexBuffer.imagp[0]=x->earlyref_NyquistBit;
    
    
    vDSP_fft_zrip(x->earlyref_fftConvSetup,&x->earlyref_SplitComplexBuffer, 1, earlyref_Log2N, FFT_INVERSE);
    
    
    vDSP_ztoc(&x->earlyref_SplitComplexBuffer, 1, (DSPComplex*) x->earlyref_FFTconvBuffer1, 2, earlyref_Buffersize/2);
    
    
}
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Matrix Convolution: Rx*Ry*Rz -> Rz
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void CalculateRxyz(t_ambverb *x){
    
    
    int a,b,c;
    
    
    
    
    /* ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** */
    
    // Initialize data for the FFT routines.
	FFTSetup Setup = vDSP_create_fftsetup(earlyref_Log2N, FFT_RADIX2);
	if (Setup == NULL)
	{
		post("Error, vDSP_create_fftsetup failed.\n");
		exit (EXIT_FAILURE);
	}
    /*	Define a stride for the array be passed to the FFT.  In many
     applications, the stride is one and is passed to the vDSP
     routine as a constant.
     */
    
    for (a=0; a < x->numChannels; a++) {            // set to zero
        for (b=0; b < x->numChannels; b++) {
            
            vDSP_vclr(x->fft_Rx[a][b].imagp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rx[a][b].realp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Ry[a][b].imagp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Ry[a][b].realp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rz[a][b].imagp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rz[a][b].realp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rxyz[a][b].imagp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rxyz[a][b].realp,1,earlyref_Buffersize/2);
        }
    }
    
    
    
    for (a=0; a < x->numChannels; a++) {
        for (b=0; b < x->numChannels; b++) {
            
            // Reinterpret Input as SplitComplex
            vDSP_ctoz((DSPComplex *) x->Rx[a][b], 2, &x->fft_Rx[a][b], 1, earlyref_Buffersize/2);
            
            // Perform a real-to-complex FFT.
            vDSP_fft_zrip(Setup, &x->fft_Rx[a][b], 1, earlyref_Log2N, FFT_FORWARD);
            
            vDSP_ctoz((DSPComplex *) x->Ry[a][b], 2, &x->fft_Ry[a][b], 1, earlyref_Buffersize/2);
            
            vDSP_fft_zrip(Setup, &x->fft_Ry[a][b], 1, earlyref_Log2N, FFT_FORWARD);//???? log2n ?
            
            vDSP_ctoz((DSPComplex *) x->Rz10[a][b], 2, &x->fft_Rz[a][b], 1, earlyref_Buffersize/2);
            
            vDSP_fft_zrip(Setup, &x->fft_Rz[a][b], 1, earlyref_Log2N, FFT_FORWARD);
            
            
        }
    }
    
    
    
    vDSP_destroy_fftsetup(Setup);
    
    
    
    
    
    /* ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** ****** */
    
    
    for (a=0; a < x->numChannels; a++) {
        for (b=0; b < x->numChannels; b++) {
            for (c=0; c < x->numChannels; c++) {
                if (x->NonZeroEntriesZ[a][c]==1 && x->NonZeroEntriesY[c][b]==1) {
                    
                    x->earlyref_NyquistBit=x->fft_Rz[a][c].imagp[0]*x->fft_Ry[c][b].imagp[0]; //Nyquistbit Correction
                    x->fft_Rz[a][c].imagp[0]=0;
                    x->fft_Ry[c][b].imagp[0]=0;
                    
                    vDSP_zvmul(&x->fft_Rz[a][c],1,&x->fft_Ry[c][b],1,&x->earlyref_SplitComplexBuffer,1,earlyref_Buffersize/2,1);
                    x->earlyref_SplitComplexBuffer.imagp[0]=x->earlyref_NyquistBit;
                    vDSP_zvadd(&x->earlyref_SplitComplexBuffer,1,&x->fft_Rxyz[a][b],1,&x->fft_Rxyz[a][b],1,earlyref_Buffersize/2);
                    
                }
                
            }
            
        }
        
    }
    
    
    for (a=0; a < x->numChannels; a++) {            // set to zero for use as dummy variable
        for (b=0; b < x->numChannels; b++) {
            
            vDSP_vclr(x->fft_Rz[a][b].imagp,1,earlyref_Buffersize/2);
            vDSP_vclr(x->fft_Rz[a][b].realp,1,earlyref_Buffersize/2);
        }
    }
    
    
    for (a=0; a < x->numChannels; a++) {
        for (b=0; b < x->numChannels; b++) {
            for (c=0; c < x->numChannels; c++) {
                if (x->NonZeroEntriesX[c][b]==1) {
                    
                    
                    x->earlyref_NyquistBit=x->fft_Rx[c][b].imagp[0]*x->fft_Rxyz[a][c].imagp[0]; //Nyquistbit Correction
                    x->fft_Rx[c][b].imagp[0]=0;
                    x->fft_Rxyz[a][c].imagp[0]=0;
                    
                    
                    vDSP_zvmul(&x->fft_Rx[c][b],1,&x->fft_Rxyz[a][c],1,&x->earlyref_SplitComplexBuffer,1,earlyref_Buffersize/2,1);
                    x->earlyref_SplitComplexBuffer.imagp[0]=x->earlyref_NyquistBit;
                    vDSP_zvadd(&x->earlyref_SplitComplexBuffer,1,&x->fft_Rz[a][b],1,&x->fft_Rz[a][b],1,earlyref_Buffersize/2);
                    
                    
                }
                
            }
            
        }
        
    }
    
    
    for (a=0; a < x->numChannels; a++) {            // Copy results to fft_Rxyz
        for (b=0; b < x->numChannels; b++) {
            vDSP_zvmov(&x->fft_Rz[a][b],1,&x->fft_Rxyz[a][b],1,earlyref_Buffersize/2);
        }
    }
    
    
    CheckforNonZeroEntriesXYZ(x);
    
}




/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Check for NonZero Entries in Rz
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
void CheckforNonZeroEntriesX(t_ambverb *x){
    
    int a,b,i,o;
    
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) { //reset Checkzeros
            x->NonZeroEntriesX[a][b] = 0;
        }
    }
    o=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            for (i=0; i<=(Trunc*2+1); i++) {
                
                if(fabsf(*(x->Rx[a][b]+i*(int)(x->Q*Qx))) >= 0.000001){
                    x->NonZeroEntriesX[a][b]=1;
                    break;
                    
                }
                
            }
            
        }
        
    }
    
}

void CheckforNonZeroEntriesY(t_ambverb *x){
    
    int a,b,i,o;
    
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) { //reset Checkzeros
            x->NonZeroEntriesY[a][b] = 0;
        }
    }
    o=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            for (i=0; i<=(Trunc*2+1); i++) {
                
                if(fabsf(*(x->Ry[a][b]+i*(int)(x->Q*Qy))) >= 0.000001){
                    x->NonZeroEntriesY[a][b]=1;
                    break;
                    
                }
                
            }
            
        }
    }
    
    
    
}

void CheckforNonZeroEntriesZ(t_ambverb *x){
    int a,b,i,o;
    
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) { //reset Checkzeros
            x->NonZeroEntriesZ[a][b] = 0;
        }
    }
    o=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            for (i=0; i<=(Trunc*2+1); i++) {
                
                if(fabsf(*(x->Rz10[a][b]+i*x->Q)) >= 0.000001){
                    x->NonZeroEntriesZ[a][b]=1;
                    break;
                    
                }
                
            }
            
        }
    }
    
    
    
}

void CheckforNonZeroEntriesXYZ(t_ambverb *x){
    int a,b,i,o;
    
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) { //reset Checkzeros
            x->NonZeroEntriesXYZ[a][b] = 0;
        }
    }
    o=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            for (i=0; i<=earlyref_Buffersize/2; i++) {
                
                if(  (fabsf(x->fft_Rxyz[a][b].realp[i]) >= 0.000001) || (fabsf(x->fft_Rxyz[a][b].imagp[i]) >= 0.000001)){
                    x->NonZeroEntriesXYZ[a][b]=1;
                    //post("Zeros[%i][%i] : Re=%f   Im=%f",a,b,x->fft_Rxyz[a][b].realp[i],x->fft_Rxyz[a][b].imagp[i]);
                    break;
                    
                }
                
            }
            
        }
    }
    
    
}

/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */
// Read Rotation matrices
/* === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === === */

void readTransformationMatrix(t_ambverb *x){
    
    int i,a,b;
    char buffer[100];
    int cx;
    FILE* file;
    float *dummy = calloc(pow(x->numChannels,2), sizeof(float));
    
    
    // ::::: Rxz ::::: //
    cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Xcode/Widening/Rxz%i_1", x->Ambisonics_Order);
    i=0;
    file = fopen (buffer, "r");
    
    while (!feof (file)){
        fscanf (file, "%f", dummy+i);
        i++;
    }
    
    fclose (file);
    
    
    //copy to Rxz
    i=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            
            x->Rxz[a][b]=*(dummy+i);
            //post("Rxz[%i][%i] = %f",a,b, x->Rxz[a][b]);
            i++;
            
        }
    }
    
    // ::::: Rzx ::::: //
    cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Xcode/Widening/Rxz%i_2", x->Ambisonics_Order);
    i=0;
    file = fopen (buffer, "r");
    
    while (!feof (file)){
        fscanf (file, "%f", dummy+i);
        i++;
    }
    
    fclose (file);
    
    
    //copy to Rzx
    i=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            
            x->Rzx[a][b]=*(dummy+i);
            //post("Rxz[%i][%i] = %f",a,b, x->Rxz[a][b]);
            i++;
            
        }
    }
    
    // ::::: Ryz ::::: //
    cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Xcode/Widening/Ryz%i_1", x->Ambisonics_Order);
    i=0;
    file = fopen (buffer, "r");
    
    while (!feof (file)){
        fscanf (file, "%f", dummy+i);
        i++;
    }
    
    fclose (file);
    
    
    //copy to Ryz
    i=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            
            x->Ryz[a][b]=*(dummy+i);
            //post("Ryz[%i][%i] = %f",a,b, x->Ryz[a][b]);
            i++;
            
        }
    }
    
    // ::::: Rzy ::::: //
    cx = snprintf ( buffer, 100, "/Users/anonym/Dropbox/HybridReverb/Xcode/Widening/Ryz%i_2", x->Ambisonics_Order);
    i=0;
    file = fopen (buffer, "r");
    
    while (!feof (file)){
        fscanf (file, "%f", dummy+i);
        i++;
    }
    
    fclose (file);
    
    
    //copy to Rzy
    i=0;
    for (a=0; a<x->numChannels; a++) {
        for (b=0; b<x->numChannels; b++) {
            
            x->Rzy[a][b]=*(dummy+i);
            //post("Ryz[%i][%i] = %f",a,b, x->Ryz[a][b]);
            i++;
            
        }
    }
    
    freebytes(dummy,pow(x->numChannels,2)*sizeof(float));
    
}
