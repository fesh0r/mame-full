#ifndef __SIDVOICE_H_
#define __SIDVOICE_H_

/*
  approximation of the sid6581 chip
  this part is for 1 (of the 3) voices of a chip
*/
#include "sound/sid6581.h"

struct sw_storage
{
	uword len;
#if defined(DIRECT_FIXPOINT)
	udword stp;
#else
	udword pnt;
	sword stp;
#endif
};

struct _SID6581;

typedef struct _sidOperator
{
	struct _SID6581 *sid;
	UINT8 reg[7];
	udword SIDfreq;
	uword SIDpulseWidth;
	ubyte SIDctrl;
	ubyte SIDAD, SIDSR;
	
	struct _sidOperator* carrier;
	struct _sidOperator* modulator;
	bool sync;
	
	uword pulseIndex, newPulseIndex;
	uword curSIDfreq;
	uword curNoiseFreq;
	
    ubyte output;//, outputMask;
	
	char filtVoiceMask;
	bool filtEnabled;
	filterfloat filtLow, filtRef;
	sbyte filtIO;
	
	sdword cycleLenCount;
#if defined(DIRECT_FIXPOINT)
	cpuLword cycleLen, cycleAddLen;
#else
	udword cycleAddLenPnt;
	uword cycleLen, cycleLenPnt;
#endif
	
	sbyte(*outProc)(struct _sidOperator *);
	void(*waveProc)(struct _sidOperator *);

#if defined(DIRECT_FIXPOINT)
	cpuLword waveStep, waveStepAdd;
#else
	uword waveStep, waveStepAdd;
	udword waveStepPnt, waveStepAddPnt;
#endif
	uword waveStepOld;
	struct sw_storage wavePre[2];

#if defined(DIRECT_FIXPOINT) && defined(LARGE_NOISE_TABLE)
	cpuLword noiseReg;
#elif defined(DIRECT_FIXPOINT)
	cpuLBword noiseReg;
#else
	udword noiseReg;
#endif
	udword noiseStep, noiseStepAdd;
	ubyte noiseOutput;
	bool noiseIsLocked;

	ubyte ADSRctrl;
//	bool gateOnCtrl, gateOffCtrl;
	uword (*ADSRproc)(struct _sidOperator *);
	
#ifdef SID_FPUENVE
	filterfloat fenveStep, fenveStepAdd;
	udword enveStep;
#elif defined(DIRECT_FIXPOINT)
	cpuLword enveStep, enveStepAdd;
#else
	uword enveStep, enveStepAdd;
	udword enveStepPnt, enveStepAddPnt;
#endif
	ubyte enveVol, enveSusVol;
	uword enveShortAttackCount;
} sidOperator;

typedef sbyte (*ptr2sidFunc)(sidOperator *);
typedef uword (*ptr2sidUwordFunc)(sidOperator *);
typedef void (*ptr2sidVoidFunc)(sidOperator *);

void sidClearOperator( sidOperator* pVoice );

void sidEmuSet(sidOperator* pVoice);
void sidEmuSet2(sidOperator* pVoice);
sbyte sidWaveCalcNormal(sidOperator* pVoice);

void sidInitWaveformTables(SIDTYPE type);
void sidInitMixerEngine(void);

#if 0
extern ptr2sidVoidFunc sid6581ModeNormalTable[16];
extern ptr2sidVoidFunc sid6581ModeRingTable[16];
extern ptr2sidVoidFunc sid8580ModeNormalTable[16];
extern ptr2sidVoidFunc sid8580ModeRingTable[16];
#endif

#endif
