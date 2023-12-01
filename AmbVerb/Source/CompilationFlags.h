//
//  CompilationFlags.h
//  AmbVerb
//
//  Created by Alexander Poletajev on 30/11/23.
//  Copyright © 2023 Alexander Poletajev. All rights reserved.
//

#ifndef CompilationFlags_h
#define CompilationFlags_h

#define Qmin 200
#define Qmax 400
#define MinRoomsize 1000
#define MaxRoomsize 2000
#define WindowStartsAt_xRoomsize 2.5
#define WindowEndsAt_xRoomsize 0.3
#define maxDelayTimeAt_xRoomsize 2.5

#define AmbisonicsOrder 3 //max 10
#define NumAmbisonicsChannels (AmbisonicsOrder+1)*(AmbisonicsOrder+1)
#define earlyref_Log2N 14
#define    earlyref_Buffersize    (1u<<earlyref_Log2N)    // Number of elements.
#define Qx 1.9f
#define Qy 1.3f
#define Trunc    8 // bigger Trunk -> bigger Buffersize

#define fdn_Log2N 14
#define    fdn_Buffersize    (1u<<fdn_Log2N)    // Number of elements.
#define NumDelaylines 16 // possible Values: 2,4,8,16,32


#endif /* CompilationFlags_h */
