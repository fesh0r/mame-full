/*
  copyright peter trauner 2000
  
  based on michael schwend's sid play

  Noise generation algorithm is used courtesy of Asger Alstrup Nielsen.
  His original publication can be found on the SID home page.

  Noise table optimization proposed by Phillip Wooller. The output of
  each table does not differ.

  MOS-8580 R5 combined waveforms recorded by Dennis "Deadman" Lindroos.
*/

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include <math.h>

#include "sidtypes.h"

#include "sidvoice.h"
#include "sidenvel.h"
#include "sid.h"

filterfloat filterTable[0x800];
filterfloat bandPassParam[0x800];
#define lowPassParam filterTable
filterfloat filterResTable[16];

/* Song clock speed (PAL or NTSC). Does not affect pitch. */
static udword sidtuneClockSpeed = 985248;

/* Master clock speed. Affects pitch of SID and CIA samples. */
udword C64_clockSpeed = 985248;
static float C64_fClockSpeed = 985248.4;

/* -------------------------------------------------------------------- Speed */

static uword defaultTimer, timer;

INLINE void calcValuesPerCall(SID6581 *This)
{
	udword fastForwardFreq = This->PCMfreq;
	if ( This->fastForwardFactor != 128 )
	{
		fastForwardFreq = (This->PCMfreq * This->fastForwardFactor) >> 7;  /* divide by 128 */
	}
#if defined(DIRECT_FIXPOINT)
   	This->VALUES.l = ( This->VALUESorg.l = (((fastForwardFreq<<12)/calls)<<4) );
	This->VALUESadd.l = 0;
#else
	This->VALUES = (This->VALUESorg = (fastForwardFreq / This->calls));
	This->VALUEScomma = ((fastForwardFreq % This->calls) * 65536UL) / This->calls;
	This->VALUESadd = 0;
#endif
}


static void sidEmuChangeReplayingSpeed(SID6581 *This)
{
	calcValuesPerCall(This);
}

/* PAL: Clock speed: 985248.4 Hz */
/*      CIA 1 Timer A: $4025 (60 Hz) */
/* */
/* NTSC: Clock speed: 1022727.14 Hz */
/*      CIA 1 Timer A: $4295 (60 Hz) */

static void sidEmuSetClockSpeed(int ntsc)
{
	switch (ntsc)
	{
	 case 1:
		{
			C64_clockSpeed = 1022727;
			C64_fClockSpeed = 1022727.14;
			break;
		}
	case 0:
	 default:
		{
			C64_clockSpeed = 985248;
			C64_fClockSpeed = 985248.4;
			break;
		}
	}
}


void sidEmuSetReplayingSpeed(SID6581 *This, int ntsc, uword callsPerSec)
{
	switch (ntsc)
	{
	 case 1:
		{
			sidtuneClockSpeed = 1022727;
			timer = (defaultTimer = 0x4295);
			break;
		}
	 case 0:
	 default:
		{
			sidtuneClockSpeed = 985248;
			timer = (defaultTimer = 0x4025);
			break;
		}
	}
	switch (callsPerSec)
	{
#if 0
	 case SIDTUNE_SPEED_CIA_1A:
		{
			timer = readLEword(c64mem2+0xdc04);
			if (timer < 16)  /* prevent overflow */
			{
				timer = defaultTimer;
			}
			calls = sidtuneClockSpeed / timer;
			break;
		}
#endif
	 default:
		{
			This->calls = callsPerSec;
			break;
		}
	}
	calcValuesPerCall(This);
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------- Buffer fill */

//ubyte playRamRom;

#if defined(SIDEMU_TIME_COUNT)
static udword prevBufferLen;    /* need for fast_forward time count */
static udword scaledBufferLen;
#endif

#define maxLogicalVoices 4

static const int mix16monoMiddleIndex = 256*maxLogicalVoices/2;
static uword mix16mono[256*maxLogicalVoices];

static uword zero16bit=0;  /* either signed or unsigned */
udword splitBufferLen;

void MixerInit(bool threeVoiceAmplify)
{
	long si;
	uword ui;
	long ampDiv = maxLogicalVoices;

	if (threeVoiceAmplify)
	{
		ampDiv = (maxLogicalVoices-1);
	}

	/* Mixing formulas are optimized by sample input value. */

	si = (-128*maxLogicalVoices) * 256;
	for (ui = 0; ui < sizeof(mix16mono)/sizeof(uword); ui++ )
	{
		mix16mono[ui] = (uword)(si/ampDiv) + zero16bit;
		si+=256;
	}

}


INLINE void syncEm(SID6581 *This)
{
	bool sync1 = (This->optr1.modulator->cycleLenCount <= 0);
	bool sync2 = (This->optr2.modulator->cycleLenCount <= 0);
	bool sync3 = (This->optr3.modulator->cycleLenCount <= 0);

	This->optr1.cycleLenCount--;
	This->optr2.cycleLenCount--;
	This->optr3.cycleLenCount--;

	if (This->optr1.sync && sync1)
	{
		This->optr1.cycleLenCount = 0;
		This->optr1.outProc = &sidWaveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr1.waveStep.l = 0;
#else
		This->optr1.waveStep = (This->optr1.waveStepPnt = 0);
#endif
	}
	if (This->optr2.sync && sync2)
	{
		This->optr2.cycleLenCount = 0;
		This->optr2.outProc = &sidWaveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		This->optr2.waveStep.l = 0;
#else
		This->optr2.waveStep = (This->optr2.waveStepPnt = 0);
#endif
	}
	if (This->optr3.sync && sync3)
	{
		This->optr3.cycleLenCount = 0;
		This->optr3.outProc = &sidWaveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr3.waveStep.l = 0;
#else
		This->optr3.waveStep = (This->optr3.waveStepPnt = 0);
#endif
	}
}


void* fill16bitMono( SID6581 *This, void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer16bit++ = mix16mono[(unsigned)(mix16monoMiddleIndex
											 +(*This->optr1.outProc)(&This->optr1)
											 +(*This->optr2.outProc)(&This->optr2)
											 +(*This->optr3.outProc)(&This->optr3)
//											 +(*sampleEmuRout)()
)];
		syncEm(This);
	}
	return buffer16bit;
}

void sidEmuFillBuffer(SID6581 *This, void* buffer, udword bufferLen )
{
	{
#if defined(SIDEMU_TIME_COUNT)
		if (prevBufferLen != bufferLen)
		{
			prevBufferLen = bufferLen;
			scaledBufferLen = (bufferLen<<7) / fastForwardFactor;
		}
#endif

		while ( bufferLen > 0 )
		{
			if ( This->toFill > bufferLen )
			{
				buffer = fill16bitMono(This, buffer, bufferLen);
				This->toFill -= bufferLen;
				bufferLen = 0;
			}
			else if ( This->toFill > 0 )
			{
				buffer = fill16bitMono(This, buffer, This->toFill);
				bufferLen -= This->toFill;
				This->toFill = 0;
			}

			if ( This->toFill == 0 )
			{
				This->reg[0x1b] = This->optr3.output;
				This->reg[0x1c] = This->optr3.enveVol;

#if 0
				uword replayPC = thisTune.getPlayAddr();
				/* playRamRom was set by external player interface. */
				if ( replayPC == 0 )
				{
 					playRamRom = c64mem1[1];
 					if ((playRamRom & 2) != 0)  /* isKernal ? */
 					{
 						replayPC = readLEword(c64mem1+0x0314);  /* IRQ */
 					}
 					else
 					{
 						replayPC = readLEword(c64mem1+0xfffe);  /* NMI */
					}
				}
				/*bool retcode = */
				interpreter(replayPC, playRamRom, 0, 0, 0);

				if (thisTune->getSongSpeed() == SIDTUNE_SPEED_CIA_1A)
				{
					sidEmuUpdateReplayingSpeed();
				}
#endif

				This->masterVolume = ( This->reg[0x18] & 15 );
				This->masterVolumeAmplIndex = This->masterVolume << 8;

				This->optr1.gateOnCtrl = This->sidKeysOn[4];
				This->optr1.gateOffCtrl = This->sidKeysOff[4];
				sidEmuSet( &This->optr1 );
				This->optr2.gateOnCtrl = This->sidKeysOn[4+7];
				This->optr2.gateOffCtrl = This->sidKeysOff[4+7];
				sidEmuSet( &This->optr2 );
				This->optr3.gateOnCtrl = This->sidKeysOn[4+14];
				This->optr3.gateOffCtrl = This->sidKeysOff[4+14];
				sidEmuSet( &This->optr3 );

				if ((This->reg[0x18]&0x80) &&
                    ((This->reg[0x17]&This->optr3.filtVoiceMask)==0))
					This->optr3.outputMask = 0;     /* off */
				else
					This->optr3.outputMask = 0xff;  /* on */

				This->filter.Type = This->reg[0x18] & 0x70;
				if (This->filter.Type != This->filter.CurType)
				{
					This->filter.CurType = This->filter.Type;
					This->optr1.filtLow = (This->optr1.filtRef = 0);
					This->optr2.filtLow = (This->optr2.filtRef = 0);
					This->optr3.filtLow = (This->optr3.filtRef = 0);
				}
				if ( This->filter.Enabled )
				{
					This->filter.Value = 0x7ff & ( (This->reg[0x15]&7) | ( (uword)This->reg[0x16] << 3 ));
					if (This->filter.Type == 0x20)
						This->filter.Dy = bandPassParam[This->filter.Value];
					else
						This->filter.Dy = lowPassParam[This->filter.Value];
					This->filter.ResDy = filterResTable[This->reg[0x17] >> 4] - This->filter.Dy;
					if ( This->filter.ResDy < 1.0 )
						This->filter.ResDy = 1.0;
				}

				sidEmuSet2( &This->optr1 );
				sidEmuSet2( &This->optr2 );
				sidEmuSet2( &This->optr3 );

//				sampleEmuCheckForInit();

#if defined(DIRECT_FIXPOINT)
				This->VALUESadd.w[HI] = 0;
				This->VALUESadd.l += This->VALUES.l;
				This->toFill = This->VALUESadd.w[HI];
#else
				{
					udword temp = (This->VALUESadd + This->VALUEScomma);
					This->VALUESadd = temp & 0xFFFF;
					This->toFill = This->VALUES;
					if (temp > 65535) This->toFill++;
				}
#endif
			}
		} /* end while bufferLen */
	} /* end if status */
}

/* --------------------------------------------------------------------- Init */


void sidEmuConfigure(SID6581 *This, udword PCMfrequency, bool measuredEnveValues,
					 bool isNewSID, bool emulateFilter, int clockSpeed)
{
	sidEmuSetClockSpeed(clockSpeed);  /* set clock speed */

	This->PCMfreq = PCMfrequency;
	This->PCMsid = (udword)(PCMfrequency * (16777216.0 / C64_fClockSpeed));
	This->PCMsidNoise = (udword)((C64_fClockSpeed*256.0)/PCMfrequency);

	sidEmuChangeReplayingSpeed(This);  /* depends on frequency */
//	sampleEmuInit();               /* depends on clock speed + frequency */

	This->filter.Enabled = emulateFilter;
	sidInitWaveformTables(isNewSID);

	enveEmuInit(This->PCMfreq,measuredEnveValues);
}


/* Reset. */

bool sidEmuReset(SID6581 *This)
{
	sidClearOperator( &This->optr1 );
	enveEmuResetOperator( &This->optr1 );
	sidClearOperator( &This->optr2 );
	enveEmuResetOperator( &This->optr2 );
	sidClearOperator( &This->optr3 );
	enveEmuResetOperator( &This->optr3 );

	This->optr1.modulator = &This->optr3;
	This->optr3.carrier = &This->optr1;
	This->optr1.filtVoiceMask = 1;

	This->optr2.modulator = &This->optr1;
	This->optr1.carrier = &This->optr2;
	This->optr2.filtVoiceMask = 2;

	This->optr3.modulator = &This->optr2;
	This->optr2.carrier = &This->optr3;
	This->optr3.filtVoiceMask = 4;

	/* Used for detecting changes of the GATE-bit (aka KEY-bit). */
	/* 6510-interpreter clears these before each call. */
	This->sidKeysOff[4] = (This->sidKeysOff[4+7] =
							  (This->sidKeysOff[4+14] = false));
	This->sidKeysOn[4] = (This->sidKeysOn[4+7] =
							 (This->sidKeysOn[4+14] = false));

//	sampleEmuReset();

	This->filter.Type = (This->filter.CurType = 0);
	This->filter.Value = 0;
	This->filter.Dy = (This->filter.ResDy = 0);

	This->toFill = 0;
#if defined(SIDEMU_TIME_COUNT)
	prevBufferLen = (scaledBufferLen = 0);
#endif

	return true;
}


void filterTableInit(void)
{
	uword uk;
	/* Parameter calculation has not been moved to a separate function */
	/* by purpose. */
	const float filterRefFreq = 44100.0;

/*	extern filterfloat filterTable[0x800]; */
	float yMax = 1.0;
	float yMin = 0.01;
	float yAdd;
	float yTmp, rk, rk2;

	float resDyMax;
	float resDyMin;
	float resDy;

	uk = 0;
	for ( rk = 0; rk < 0x800; rk++ )
	{
		filterTable[uk] = (((exp(rk/0x800*log(400.0))/60.0)+0.05)
			*filterRefFreq) / options.samplerate;
		if ( filterTable[uk] < yMin )
			filterTable[uk] = yMin;
		if ( filterTable[uk] > yMax )
			filterTable[uk] = yMax;
		uk++;
	}

	/*extern filterfloat bandPassParam[0x800]; */
	yMax = 0.22;
	yMin = 0.05;  /* less for some R1/R4 chips */
	yAdd = (yMax-yMin)/2048.0;
	yTmp = yMin;
	uk = 0;
	/* Some C++ compilers still have non-local scope! */
	for ( rk2 = 0; rk2 < 0x800; rk2++ )
	{
		bandPassParam[uk] = (yTmp*filterRefFreq) / options.samplerate;
		yTmp += yAdd;
		uk++;
	}

	/*extern filterfloat filterResTable[16]; */
	resDyMax = 1.0;
	resDyMin = 2.0;
	resDy = resDyMin;
	for ( uk = 0; uk < 16; uk++ )
	{
		filterResTable[uk] = resDy;
		resDy -= (( resDyMin - resDyMax ) / 15 );
	}
	filterResTable[0] = resDyMin;
	filterResTable[15] = resDyMax;
}

void sid6581_init (SID6581 *This)
{
	int pal=1;
	sidInitMixerEngine();
	filterTableInit();
	This->calls = 50;               /* calls per second (here a default) */
	This->fastForwardFactor = 128;  /* normal speed */
	if (pal)
		sidEmuSetReplayingSpeed(This,0, 50);
	else
		sidEmuSetReplayingSpeed(This,0, 60);
	sidEmuConfigure(This,options.samplerate, true, 0, true, !pal);
	MixerInit(0);
	This->optr1.sid=This;
	This->optr2.sid=This;
	This->optr3.sid=This;

	sidEmuReset(This);
}

void sid_set_type (SID6581 *This, SIDTYPE type)
{
	sidEmuConfigure(This,options.samplerate, true, type, true, 0);
}

void sid6581_port_w (SID6581 *This, int offset, int data)
{
	DBG_LOG (1, "sid6581 write", ("offset %.2x value %.2x\n",
								  offset, data));
	offset &= 0x1f;
	switch (offset)
	{
	case 0x1d:
	case 0x1e:
	case 0x1f:
		break;
	default:
		/*stream_update(channel,0); */
		This->reg[offset] = data;
		if (data&1)
			This->sidKeysOn[offset]=1;
		else
			This->sidKeysOff[offset]=1;

		if (offset<7)
			This->optr1.reg[offset] = data;
		else if (offset<14) 
			This->optr2.reg[offset-7] = data;
		else if (offset<21) 
			This->optr3.reg[offset-14] = data;
	}
}

int sid6581_port_r (SID6581 *This, int offset)
{
	int data;
/* SIDPLAY reads last written at a sid address value */
	offset &= 0x1f;
	switch (offset)
	{
	case 0x1d:
	case 0x1e:
	case 0x1f:
		data=0xff;
		break;
	case 0x19:						   /* paddle 1 */
		if (This->ad_read != NULL)
			data=This->ad_read (0);
		else
			data=0;
		break;
	case 0x1a:						   /* paddle 2 */
		if (This->ad_read != NULL)
			data=This->ad_read (1);
		else
			data=0;
		break;
#if 0
	case 0x1b:case 0x1c: /* noise channel readback? */
		data=rand();
		break;
#endif
	default:
		data=This->reg[offset];
	}
	DBG_LOG (1, "sid6581 read", ("offset %.2x value %.2x\n",
								  offset, data));
    return data;
}

