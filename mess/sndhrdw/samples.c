/* */
/* /home/ms/source/sidplay/libsidplay/emu/RCS/samples.cpp,v */
/* */

#define VERBOSE_DBG 0
#include "includes/cbm.h"

#include "includes/sid6581.h"
#include "samples.h"


/*extern ubyte* c64mem1; */
/*extern ubyte* c64mem2; */
extern uword PCMfreq;
extern udword C64_clockSpeed;

static void sampleEmuTryStopAll(void);
static sbyte sampleEmuSilence(void);
static sbyte sampleEmu(void);
static sbyte sampleEmuTwo(void);

static void GalwayInit(void);
static sbyte GalwayReturnSample(void);
INLINE void GetNextFour(void);

sbyte (*sampleEmuRout)(void) = &sampleEmuSilence;

/* --- */

static udword sampleClock;

enum
{
	FM_NONE,
	FM_GALWAYON,
	FM_GALWAYOFF,
	FM_HUELSON,
	FM_HUELSOFF
};

/* Sample Order Modes. */
static const ubyte SO_LOWHIGH = 0;
static const ubyte SO_HIGHLOW = 1;

typedef struct
{
	bool Active;
	char Mode;
	ubyte Counter;  /* Galway */
	ubyte Repeat;
	ubyte Scale;
	ubyte SampleOrder;
	sbyte VolShift;

	uword Address;
	uword EndAddr;
	uword RepAddr;

	uword SamAddr;  /* Galway */
	uword SamLen;
	uword GalLastPos;
	uword LoopWait;
	uword NullWait;

	uword Period;
#if defined(DIRECT_FIXPOINT)
	cpuLword Period_stp;
	cpuLword Pos_stp;
	cpuLword PosAdd_stp;
#elif defined(PORTABLE_FIXPOINT)
	uword Period_stp, Period_pnt;
	uword Pos_stp, Pos_pnt;
	uword PosAdd_stp, PosAdd_pnt;
#else
	udword Period_stp;
	udword Pos_stp;
	udword PosAdd_stp;
#endif
} sampleChannel;

static sampleChannel ch4, ch5;


static const sbyte galwayNoiseTab1[16] =
{
	(sbyte)0x80,(sbyte)0x91,(sbyte)0xa2,(sbyte)0xb3,
	(sbyte)0xc4,(sbyte)0xd5,(sbyte)0xe6,(sbyte)0xf7,
	0x08,0x19,0x2a,0x3b,0x4c,0x5d,0x6e,0x7f
};

static const sbyte sampleConvertTab[16] =
{
/*  0x81,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff, */
/*  0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x7f */
	(sbyte)0x81,(sbyte)0x90,(sbyte)0xa0,(sbyte)0xb0,
	(sbyte)0xc0,(sbyte)0xd0,(sbyte)0xe0,(sbyte)0xf0,
	0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70
};

static ubyte galwayNoiseTab2[64*16];

static void channelReset(sampleChannel* ch)
{
	ch->Active = false;
	ch->Mode = FM_NONE;
	ch->Period = 0;
#if defined(DIRECT_FIXPOINT)
	ch->Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch->Pos_stp = (ch->Pos_pnt = 0);
#else
	ch->Pos_stp = 0;
#endif
	ch->GalLastPos = 0;
}

INLINE void channelFree(sampleChannel* ch, const uword regBase)
{
	ch->Active = false;
	ch->Mode = FM_NONE;
	/*c64mem2[regBase+0x1d] = 0x00; */
}

INLINE void channelTryInit(sampleChannel* ch, const uword regBase)
{
	/*uword tempPeriod; */
	if ( ch->Active && ( ch->Mode == FM_GALWAYON ))
		return;

#if 0
	ch->VolShift = ( 0 - (sbyte)c64mem2[regBase+0x1d] ) >> 1;
	c64mem2[regBase+0x1d] = 0x00;
	ch->Address = readLEword(c64mem2+regBase+0x1e);
	ch->EndAddr = readLEword(c64mem2+regBase+0x3d);
	if (ch->EndAddr <= ch->Address)
	{
		return;
	}
	ch->Repeat = c64mem2[regBase+0x3f];
	ch->RepAddr = readLEword(c64mem2+regBase+0x7e);
	ch->SampleOrder = c64mem2[regBase+0x7d];
	tempPeriod = readLEword(c64mem2+regBase+0x5d);
	if ( (ch->Scale=c64mem2[regBase+0x5f]) != 0 )
	{
		tempPeriod >>= ch->Scale;
	}
	if ( tempPeriod == 0 )
	{
		ch->Period = 0;
#if defined(DIRECT_FIXPOINT)
		ch->Pos_stp.l = (ch->PosAdd_stp.l = 0);
#elif defined(PORTABLE_FIXPOINT)
		ch->Pos_stp = (ch->Pos_pnt = 0);
		ch->PosAdd_stp = (ch->PosAdd_pnt = 0);
#else
		ch->Pos_stp = (ch->PosAdd_stp = 0);
#endif
		ch->Active = false;
		ch->Mode = FM_NONE;
	}
	else
	{
		if ( tempPeriod != ch->Period )
		{
			ch->Period = tempPeriod;
#if defined(DIRECT_FIXPOINT)
			ch->Pos_stp.l = sampleClock / ch->Period;
#elif defined(PORTABLE_FIXPOINT)
			udword tempDiv = sampleClock / ch->Period;
			ch->Pos_stp = tempDiv >> 16;
			ch->Pos_pnt = tempDiv & 0xFFFF;
#else
			ch->Pos_stp = sampleClock / ch->Period;
#endif
		}
#if defined(DIRECT_FIXPOINT)
		ch->PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch->PosAdd_stp = (ch->PosAdd_pnt = 0);
#else
		ch->PosAdd_stp = 0;
#endif
		ch->Active = true;
		ch->Mode = FM_HUELSON;
	}
#endif
}

INLINE ubyte channelProcess(sampleChannel* ch, const uword regBase)
{
	ubyte tempSample;
#if defined(DIRECT_FIXPOINT)
	uword sampleIndex = ch->PosAdd_stp.w[HI] + ch.Address;
#elif defined(PORTABLE_FIXPOINT)
	uword sampleIndex = ch->PosAdd_stp + ch.Address;
#else
	uword sampleIndex = (ch->PosAdd_stp >> 16) + ch->Address;
#endif
    if ( sampleIndex >= ch->EndAddr )
	{
		if ( ch->Repeat != 0xFF )
		{
			if ( ch->Repeat != 0 )
				ch->Repeat--;
			else
			{
				channelFree(ch,regBase);
				return 8;
			}
		}
		sampleIndex = ( ch->Address = ch->RepAddr );
#if defined(DIRECT_FIXPOINT)
		ch->PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch->PosAdd_stp = (ch->PosAdd_pnt = 0);
#else
		ch->PosAdd_stp = 0;
#endif
		if ( sampleIndex >= ch->EndAddr )
		{
			channelFree(ch,regBase);
			return 8;
		}
	}

	tempSample = 0;/*c64mem1[sampleIndex]; */
	if (ch->SampleOrder == SO_LOWHIGH)
	{
		if (ch->Scale == 0)
		{
#if defined(DIRECT_FIXPOINT)
			if (ch->PosAdd_stp.w[LO] >= 0x8000)
#elif defined(PORTABLE_FIXPOINT)
			if ( ch->PosAdd_pnt >= 0x8000 )
#else
		    if ( (ch->PosAdd_stp & 0x8000) != 0 )
#endif
			{
			    tempSample >>= 4;
			}
		}
		/* AND 15 further below. */
	}
	else  /* if (ch->SampleOrder == SO_HIGHLOW) */
	{
		if (ch->Scale == 0)
		{
#if defined(DIRECT_FIXPOINT)
			if ( ch->PosAdd_stp.w[LO] < 0x8000 )
#elif defined(PORTABLE_FIXPOINT)
		    if ( ch->PosAdd_pnt < 0x8000 )
#else
		    if ( (ch->PosAdd_stp & 0x8000) == 0 )
#endif
			{
			    tempSample >>= 4;
			}
			/* AND 15 further below. */
		}
		else  /* if (ch.Scale != 0) */
		{
			tempSample >>= 4;
		}
	}

#if defined(DIRECT_FIXPOINT)
	ch->PosAdd_stp.l += ch->Pos_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch->PosAdd_pnt + (udword)ch->Pos_pnt;
	ch->PosAdd_pnt = temp & 0xffff;
	ch->PosAdd_stp += ( ch->Pos_stp + ( temp > 65535 ));
#else
	ch->PosAdd_stp += ch->Pos_stp;
#endif

	return (tempSample&0x0F);
}
/* --- */

void sampleEmuReset(void)
{
	channelReset(&ch4);
	channelReset(&ch5);
	sampleClock = (udword) (((C64_clockSpeed / 2.0) / PCMfreq) * 65536UL);
	sampleEmuRout = &sampleEmuSilence;
/*	if ( c64mem2 != 0 ) */
	{
		channelFree(&ch4,0xd400);
		channelFree(&ch5,0xd500);
	}
}

void sampleEmuInit(void)
{
	int i, k = 0;
	for ( i = 0; i < 16; i++ )
	{
		int j, l = 0;
		for ( j = 0; j < 64; j++ )
		{
			galwayNoiseTab2[k++] = galwayNoiseTab1[l];
			l = (l+i) & 15;
		}
	}
	/*sampleEmuReset(); */
}

static sbyte sampleEmuSilence(void)
{
	return 0;
}

INLINE void sampleEmuTryStopAll(void)
{
	if ( !ch4.Active && !ch5.Active )
	{
		sampleEmuRout = &sampleEmuSilence;
	}
}

void sampleEmuCheckForInit(void)
{
	/* Try first sample channel. */
	switch ( sid6581->reg[0x1d] )
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(&ch4,0x00);
		break;
	 case 0xFD:
		channelFree(&ch4,0x00);
		break;
	 case 0xFC:
		channelTryInit(&ch4,0x00);
		break;
	 case 0x00:
		break;
	 default:
		GalwayInit();
		break;
	}

	if (ch4.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmu;
	}


#if 0
	/* Try second sample channel. */
	switch ( sid6581[1].reg[0x1d] )
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(&ch5,0x00);
		break;
	 case 0xFD:
		channelFree(&ch5,0x00);
		break;
	 case 0xFC:
		channelTryInit(&ch5,0x00);
		break;
	 default:
		break;
	}
#endif
	if (ch5.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmuTwo;
	}
	sampleEmuTryStopAll();
}

static sbyte sampleEmu(void)
{
	/* One sample channel. Return signed 8-bit sample. */
	if ( !ch4.Active )
		return 0;
	else
		return (sampleConvertTab[channelProcess(&ch4,0x00)]>>ch4.VolShift);
}

static sbyte sampleEmuTwo(void)
{
	sbyte sample = 0;
	if ( ch4.Active )
		sample += (sampleConvertTab[channelProcess(&ch4,0x00)]>>ch4.VolShift);
#if 0
	if ( ch5.Active )
		sample += (sampleConvertTab[channelProcess(&ch5,0x00)]>>ch5.VolShift);
#endif
	return sample;
}

/* --- */

INLINE void GetNextFour(void)
{
#if 0
	uword tempMul = (uword)(c64mem1[ch4.Address+(uword)ch4.Counter])
		* ch4.LoopWait + ch4.NullWait;
	ch4.Counter--;
#if defined(DIRECT_FIXPOINT)
	if ( tempMul != 0 )
		ch4.Period_stp.l = (sampleClock<<1) / tempMul;
	else
		ch4.Period_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	udword tempDiv;
	if ( tempMul != 0 )
		tempDiv = (sampleClock<<1) / tempMul;
	else
		tempDiv = 0;
	ch4.Period_stp = tempDiv >> 16;
	ch4.Period_pnt = tempDiv & 0xFFFF;
#else
	if ( tempMul != 0 )
		ch4.Period_stp = (sampleClock<<1) / tempMul;
	else
		ch4.Period_stp = 0;
#endif
	ch4.Period = tempMul;
#endif
}

static void GalwayInit(void)
{
	if (ch4.Active)
		return;

#if 0
	sampleEmuRout = &sampleEmuSilence;

	ch4.Counter = sid6581->reg[0x1d];
	sid6581->reg[0x1d] = 0;

	if ((ch4.Address=sid6581_read_word(0x1e)) == 0)
		return;

	if ((ch4.LoopWait=c64mem2[0xd43f]) == 0)
		return;

	if ((ch4.NullWait=c64mem2[0xd45d]) == 0)
		return;

	if (c64mem2[0xd43e] == 0)
		return;
	ch4.SamAddr = ((uword)c64mem2[0xd43e]&15) << 6;

	if ( c64mem2[0xd43d] == 0 )
		return;
	ch4.SamLen = (uword)c64mem2[0xd43d];

	ch4.Active = true;
	ch4.Mode = FM_GALWAYON;

#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch4.Pos_stp = 0;
	ch4.Pos_pnt = 0;
#else
	ch4.Pos_stp = 0;
#endif
	GetNextFour();

	sampleEmuRout = &GalwayReturnSample;
#endif
}

static sbyte GalwayReturnSample(void)
{
#if defined(DIRECT_FIXPOINT)
	uword samAddr = ch4.SamAddr+((ch4.GalLastPos+ch4.Pos_stp.w[HI])&63);
#elif defined(PORTABLE_FIXPOINT)
	uword samAddr = ch4.SamAddr+((ch4.GalLastPos+ch4.Pos_stp)&63);
#else
	uword samAddr = ch4.SamAddr+((ch4.GalLastPos+(ch4.Pos_stp>>16))&63);
#endif
	sbyte tempSample = galwayNoiseTab2[samAddr];

#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l += ch4.Period_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch4.Pos_pnt;
	temp += (udword)ch4.Period_pnt;
	ch4.Pos_pnt = temp & 0xffff;
	ch4.Pos_stp += ( ch4.Period_stp + ( temp > 65535 ));
#else
	ch4.Pos_stp += ch4.Period_stp;
#endif

#if defined(DIRECT_FIXPOINT)
	if ( ch4.Pos_stp.w[HI] >= ch4.SamLen )
#elif defined(PORTABLE_FIXPOINT)
	if ( ch4.Pos_stp >= ch4.SamLen )
#else
	if ( (ch4.Pos_stp >> 16) >= ch4.SamLen )
#endif
	{
#if defined(DIRECT_FIXPOINT)
        ch4.GalLastPos = ch4.Pos_stp.w[HI];
		ch4.Pos_stp.w[HI] = 0;
#elif defined(PORTABLE_FIXPOINT)
        ch4.GalLastPos = ch4.Pos_stp;
		ch4.Pos_stp = 0;
#else
        ch4.GalLastPos = (ch4.Pos_stp>>16);
		ch4.Pos_stp &= 0xFFFF;
#endif
        if ( ch4.Counter == 0xff )
        {
			ch4.Active = false;
            ch4.Mode = FM_GALWAYOFF;
            sampleEmuRout = &sampleEmuSilence;
        }
        else
        {
            GetNextFour();
        }
    }
    return tempSample;
}
