//
// /home/ms/source/sidplay/libsidplay/emu/RCS/samples.h,v
//

#ifndef __SAMPLES_H_
#define __SAMPLES_H_


#include "mytypes.h"
//#include "myendian.h"


extern void sampleEmuCheckForInit(void);
extern void sampleEmuInit(void);          // precalculate tables + reset
extern void sampleEmuReset(void);         // reset some important variables

extern sbyte (*sampleEmuRout)(void);


#endif
