#define FM_DEBUG
/*
**
** File: fm.c -- software implementation of FM sound generator
**
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmurator development
**
** Version 0.33e
**
*/

/*
    no check:
		OPN  SSG type envelope
		YM2612 DAC output mode

	no support:
		status busy flag (already not busy )
		LFO contoller (YM2612/YM2608/YM2151)
		OPM noise mode
		OPM output pins (CT1,CT0)
		YM2608 ADPCM and RYTHM ( and Expansion status )

	preliminary :
		CSM talking mode
		key scale level rate (?)
		attack rate time rate , curve (?)
		decay  rate time rate , curve (?)
		self feedback rate

	Problem :
		sometime hiss noize is heard.

	note:
                        OPN                           OPM
		fnum          fMus * 2^20 / (fM/(12*n))
		TimerOverA    (12*n)*(1024-NA)/fFM        64*(1024-Na)/fm
		TimerOverB    (12*n)*(256-NB)/fFM       1024*(256-Nb)/fm
		output bits   10bit<<3bit               16bit * 2ch (YM3012=10bit<<3bit)
		sampling rate fFM / (12*6) ?            fFM / 64
		lfo freq                                ( fM*2^(LFRQ/16) ) / (4295*10^6)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "driver.h"
#include "psg.h"
#include "fm.h"

#ifndef INLINE
#define INLINE inline
#endif
#ifndef PI
#define PI 3.14159265357989
#endif

/* ------------------------------------------------------------------ */
//#define INTERNAL_TIMER 			/* use internal timer */
/* -------------------- speed up optimize switch -------------------- */
/* ---------- Enable ---------- */
#define MIN_UPDATE 44		/* minimum update step (use SELF_UPDATE ) */
#define TL_SAVE_MEM			/* save some memories for total level */
/* ---------- Disable ---------- */
#if 0
#define SEG_SUPPORT		   	/* OPN SSG type envelope support   */
#define LFO_SUPPORT			/* LFO support                     */
#endif
/* -------------------- preliminary define section --------------------- */
/* attack/decay rate time rate */
#define OPM_ARRATE     399128
#define OPM_DRRATE    5514396
/* It is not checked , because I haven't YM2203 rate */
#define OPN_ARRATE  OPM_ARRATE
#define OPN_DRRATE  OPM_DRRATE

#define FREQ_BITS 24			/* frequency turn          */

/* counter bits = 21 , octerve 7 */
#define FREQ_RATE   (1<<(FREQ_BITS-21))
#define TL_BITS    (FREQ_BITS+2)

/* final output shift */
#define OPN_OUTSB   (TL_BITS+2-16)		/* OPN output final shift 16bit */
#define OPN_OUTSB_8 (OPN_OUTSB+8)		/* OPN output final shift  8bit */
#define OPM_OUTSB   (TL_BITS+2-16) 		/* OPM output final shift 16bit */
#define OPM_OUTSB_8 (OPM_OUTSB+8) 		/* OPM output final shift  8bit */
/* limit minimum and maximum */
#define OPN_MAXOUT (0x7fff<<OPN_OUTSB)
#define OPN_MINOUT (-0x8000<<OPN_OUTSB)
#define OPM_MAXOUT (0x7fff<<OPM_OUTSB)
#define OPM_MINOUT (-0x8000<<OPM_OUTSB)
/* -------------------- quality selection --------------------- */

/* sinwave entries */
/* used static memory = SIN_ENT * 4 (byte) */
#define SIN_ENT 2048

/* output level entries (envelope,sinwave) */
/* used dynamic memory = EG_ENT*4*4(byte)or EG_ENT*6*4(byte) */
/* used static  memory = EG_ENT*4 (byte)                     */
#define EG_ENT   4096
#define EG_STEP (96.0/EG_ENT) /* OPL is 0.1875 dB step  */
#define ENV_BITS 16			/* envelope lower bits      */

/* LFO table entries */
#define LFO_ENT 512

/* -------------------- local defines , macros --------------------- */
/* buffer */
#ifdef FM_STEREO_MIX
  #define FM_NUMBUF 1	/* stereo mixing   L and R ch */
#else
  #define FM_NUMBUF 2	/* stereo separate L and R ch */
#endif

/* number of maximum envelope counter */
#define ENV_OFF ((EG_ENT<<ENV_BITS)-1)

/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)
#define OPM_CHAN(N) (N&7)
#define OPM_SLOT(N) ((N>>3)&3)
/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* envelope phase */
/* bit0  : not hold(Incrementabl)   */
/* bit1  : attack                   */
/* bit2  : decay downside           */
/* bit3  : decay upside(SSG-TYPE)   */
/* bit4-6: phase number             */

#define ENV_MOD_OFF 0x00
#define ENV_MOD_RR  0x15
#define ENV_MOD_SR  0x25
#define ENV_MOD_DR  0x35
#define ENV_MOD_AR  0x43
#define ENV_SSG_SR  0x59
#define ENV_SSG_DR  0x65

/* bit0 = right enable , bit1 = left enable (FOR YM2612) */
#define OPN_RIGHT  1
#define OPN_LEFT   2
#define OPN_CENTER 3

/* bit0 = left enable , bit1 = right enable */
#define OPM_LEFT   1
#define OPM_RIGHT  2
#define OPM_CENTER 3
/* */

/* YM2608 Rhythm Number */
#define RY_BD  0
#define RY_SD  1
#define RY_TOP 2
#define RY_HH  3
#define RY_TOM 4
#define RY_RIM 5


/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_chan {
	unsigned int *freq_table;	/* frequency table */
	/* registers , sound type */
	int *DT[4];					/* detune          :DT_TABLE[DT]       */
	int DT2[4];					/* multiple,Detune2:(DT2<<4)|ML for OPM*/
	int TL[4];					/* total level     :TL << 8            */
	signed int TLL[4];			/* adjusted now TL                     */
	unsigned char KSR[4];		/* key scale rate  :3-KSR              */
	int *AR[4];					/* attack rate     :&AR_TABLE[AR<<1]   */
	int *DR[4];					/* decay rate      :&DR_TALBE[DR<<1]   */
	int *SR[4];					/* sustin rate     :&DR_TABLE[SR<<1]   */
	int  SL[4];					/* sustin level    :SL_TALBE[SL]       */
	int *RR[4];					/* release rate    :&DR_TABLE[RR<<2+2] */
	unsigned char SEG[4];		/* SSG EG type     :SSGEG              */
	unsigned char PAN;			/* PAN 0=off,3=center                  */
	unsigned char FB;			/* feed back       :&FB_TABLE[FB<<8]   */
	/* algorythm state */
	int *connect1;				/* operator 1 connection pointer       */
	int *connect2;				/* operator 2 connection pointer       */
	int *connect3;				/* operator 3 connection pointer       */
	/* phase generator state */
	unsigned int  fc;			/* fnum,blk        :calcrated          */
	unsigned char fn_h;			/* freq latch      :                   */
	unsigned char kcode;		/* key code        :                   */
	unsigned char ksr[4];		/* key scale rate  :kcode>>(3-KSR)     */
	unsigned int mul[4];		/* multiple        :ML_TABLE[ML]       */
	unsigned int Cnt[4];		/* frequency count :                   */
	unsigned int Incr[4];		/* frequency step  :                   */
	int op1_out;				/* op1 output foe beedback             */
	/* envelope generator state */
	unsigned char evm[4];		/* envelope phase                      */
	signed int evc[4];			/* envelope counter                    */
	signed int arc[4];			/* envelope counter for AR             */
	signed int evsa[4];			/* envelope step for AR                */
	signed int evsd[4];			/* envelope step for DR                */
	signed int evss[4];			/* envelope step for SR                */
	signed int evsr[4];			/* envelope step for RR                */
	/* LFO */
	unsigned char ams[4];
	unsigned char pms[4];
} FM_CH;

/* generic fm state */
typedef struct fm_state {
	int bufp;					/* update buffer point */
	int clock;					/* master clock  (Hz)  */
	int rate;					/* sampling rate (Hz)  */
	int freqbase;				/* frequency base      */
	unsigned char address;		/* address register    */
	unsigned char irq;			/* interrupt level     */
	unsigned char irqmask;		/* irq mask            */
	unsigned char status;		/* status flag         */
	unsigned int mode;			/* mode  CSM / 3SLOT   */
	int TA;						/* timer a             */
	int TAC;					/* timer a counter     */
	unsigned char TB;			/* timer b             */
	int TBC;					/* timer b counter     */
	void (*handler)(void);			/* interrupt handler   */
	void *timer_a_timer;		/* timer for a         */
	void *timer_b_timer;		/* timer for b         */
	/* speedup customize */
	/* time tables */
	signed int DT_TABLE[8][32];     /* detune tables       */
	signed int AR_TABLE[94];	/* atttack rate tables */
	signed int DR_TABLE[94];	/* decay rate tables   */
	/* LFO */
	unsigned int LFOCnt;
	unsigned int LFOIncr;
}FM_ST;

/* OPN 3slot struct */
typedef struct opn_3slot {
	unsigned int  fc[3];		/* fnum3,blk3  :calcrated */
	unsigned char fn_h[3];		/* freq3 latch            */
	unsigned char kcode[3];		/* key code    :          */
}FM_3SLOT;

/* here's the virtual YM2203(OPN) (Used by YM2608 / YM2612)  */
typedef struct ym2203_f {
	FMSAMPLE *Buf[FM_NUMBUF];	/* sound buffers     */
	FM_ST ST;					/* general state     */
	FM_CH CH[3];				/* channel state     */
/*	unsigned char PR;			   freqency priscaler  */
	unsigned char mode;			/* Chip mode         */
	FM_3SLOT SL3;				/* 3 slot mode state */
	/* fnumber -> increment counter */
	unsigned int FN_TABLE[2048];
} YM2203;

/* here's the virtual YM2608 */
typedef struct ym2608_f {
	FM_CH CH[3];				/* additional channel state */
	int address1;
	/* PCM Part   (YM2608) */
	int ADMode;		/* mode of ADPCM unit */
	int ADPAN;		/* PAN */
	int ADStart;	/* Start address */
	int ADStop;		/* Stop address */
	int ADDelta;	/* Playback sampling rate */
	int ADTL;		/* output level */
	int ADLimit;	/* Limit address */
	int ADData;		/* PCM data */
	/* Rhythm Part (YM2608) */
	int RTL;		/* Total level */
	int RPAN[6];	/* PAN */
	int RIL[6];		/* IL  */
} YM2608;

/* here's the virtual YM2612 */
typedef struct ym2612_f {
	FM_CH CH[3];				/* additional channel state */
	int address1;
	/* dac output (YM2612) */
	int dacen;
	int dacout;
} YM2612;

/* here's the virtual YM2151(OPM)  */
typedef struct ym2151_f {
	FMSAMPLE *Buf[FM_NUMBUF];/* sound buffers    */
	FM_ST ST;					/* general state     */
	FM_CH CH[8];				/* channel state     */
	unsigned char NReg;			/* noise enable,freq */
	unsigned char pmd;			/* LFO pmd level     */
	unsigned char amd;			/* LFO amd level     */
	unsigned char ctw;			/* CT0,1 and waveform */
	unsigned int KC_TABLE[8*12*64+950];/* keycode,keyfunction -> count */
} YM2151;


/* -------------------- tables --------------------- */

/* key scale level */
/* !!!!! preliminary !!!!! */

#define DV (1/EG_STEP)
static unsigned char KSL[32]=
{
#if 1
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#else
 0.000/DV , 0.000/DV , 0.000/DV , 0.000/DV ,	/* OCT 0 */
 0.000/DV , 0.000/DV , 0.000/DV , 1.875/DV ,	/* OCT 1 */
 0.000/DV , 0.000/DV , 3.000/DV , 4.875/DV ,	/* OCT 2 */
 0.000/DV , 3.000/DV , 6.000/DV , 7.875/DV ,	/* OCT 3 */
 0.000/DV , 6.000/DV , 9.000/DV ,10.875/DV ,	/* OCT 4 */
 0.000/DV , 9.000/DV ,12.000/DV ,13.875/DV ,	/* OCT 5 */
 0.000/DV ,12.000/DV ,15.000/DV ,16.875/DV ,	/* OCT 6 */
 0.000/DV ,15.000/DV ,18.000/DV ,19.875/DV		/* OCT 7 */
#endif
};
#undef DV

/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static char OPN_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

static int KC_TO_SEMITONE[16]={
	/*translate note code KC into more usable number of semitone*/
	0*64, 1*64, 2*64, 3*64,
	3*64, 4*64, 5*64, 6*64,
	6*64, 7*64, 8*64, 9*64,
	9*64,10*64,11*64,12*64
};

int DT2_TABLE[4]={ /* 4 DT2 values */
/*
 *   DT2 defines offset in cents from base note
 *
 *   The table below defines offset in deltas table...
 *   User's Manual page 22
 *   Values below were calculated using formula:  value = orig.val * 1.5625
 *
 * DT2=0 DT2=1 DT2=2 DT2=3
 * 0     600   781   950
 */
	0,    384,  500,  608
};

/* sustain lebel table (3db per step) */
/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define ML ((3/EG_STEP)*(1<<ENV_BITS))
static int SL_TABLE[16]={
 ML* 0,ML* 1,ML* 2,ML* 3,ML* 4,ML* 5,ML* 6,ML* 7,
 ML* 8,ML* 9,ML*10,ML*11,ML*12,ML*13,ML*14,ML*31
};
#undef ML

#ifdef TL_SAVE_MEM
  #define TL_MAX (EG_ENT*2) /* limit(tl + ksr + envelope) + sinwave */
#else
  #define TL_MAX (EG_ENT*4) /* tl + ksr + envelope + sinwave */
#endif

/* TotalLevel : 48 24 12  6  3 1.5 0.75 (dB) */
/* TL_TABLE[ 0      to TL_MAX          ] : plus  section */
/* TL_TABLE[ TL_MAX to TL_MAX+TL_MAX-1 ] : minus section */
static int *TL_TABLE;

/* pointers to TL_TABLE with sinwave output offset */
static signed int *SIN_TABLE[SIN_ENT];

/* attack count to enveloe count conversion */
static int AR_CURVE[EG_ENT];

#define OPM_DTTABLE OPN_DTTABLE
static char OPN_DTTABLE[4 * 32]={
/* this table is YM2151 and YM2612 data */
/* FD=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8 , 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

/* multiple table */
#define ML 2
static int MUL_TABLE[4*16]= {
/* 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
   0.50*ML, 1.00*ML, 2.00*ML, 3.00*ML, 4.00*ML, 5.00*ML, 6.00*ML, 7.00*ML,
   8.00*ML, 9.00*ML,10.00*ML,11.00*ML,12.00*ML,13.00*ML,14.00*ML,15.00*ML,
/* DT2=1 *SQL(2)   */
   0.71*ML, 1.41*ML, 2.82*ML, 4.24*ML, 5.65*ML, 7.07*ML, 8.46*ML, 9.89*ML,
  11.30*ML,12.72*ML,14.10*ML,15.55*ML,16.96*ML,18.37*ML,19.78*ML,21.20*ML,
/* DT2=2 *SQL(2.5) */
   0.78*ML, 1.57*ML, 3.14*ML, 4.71*ML, 6.28*ML, 7.85*ML, 9.42*ML,10.99*ML,
  12.56*ML,14.13*ML,15.70*ML,17.27*ML,18.84*ML,20.41*ML,21.98*ML,23.55*ML,
/* DT2=3 *SQL(3)   */
   0.87*ML, 1.73*ML, 3.46*ML, 5.19*ML, 6.92*ML, 8.65*ML,10.38*ML,12.11*ML,
  13.84*ML,15.57*ML,17.30*ML,19.03*ML,20.76*ML,22.49*ML,24.22*ML,25.95*ML
};
#undef ML

#ifdef LFO_SUPPORT
/* LFO frequency timer table */
static int OPM_LFO_TABLE[256];
#endif

/* dummy attack / decay rate ( when rate == 0 ) */
static int RATE_0[32]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* -------------------- state --------------------- */

/* some globals */
#define DEVICE_SSG     0x01
#define DEVICE_OPN     0x02
#define DEVICE_OPN6CH  0x04
#define DEVICE_DAC     0x08
#define DEVICE_ADPCM   0x10

#define CHIP_YM2203 DEVICE_SSG
#define CHIP_YM2608 (DEVICE_SSG|DEVICE_OPN6CH|DEVICE_ADPCM)
#define CHIP_YM2612 (DEVICE_OPN6CH|DEVICE_DAC)
static int ChipMode;

static int FMBufSize;		/* size of sound buffer, in samples */
static int FMNumChips;		/* total # of FM emulated */

static YM2203 *FMOPN=NULL;	/* array of YM2203's */
static YM2612 *FM2612=NULL;	/* array of YM2612's */
static YM2608 *FM2608=NULL;	/* array of YM2608's */
static YM2151 *FMOPM=NULL;	/* array of YM2151's */

static unsigned char sample_16bit;/* output bits    */

/* work table */
static void *cur_chip = 0;	/* current chip point */

/* currenct chip state */
FM_ST            *State;
static FMSAMPLE  *bufL,*bufR;
static FM_CH     *chA,*chB,*chC,*chD,*chE,*chF,*chG,*chH;
static int *pan[8];			/* pointer to outd[0-3]; */
static int outd[4];			/*0=off,1=left,2=right,3=center */

/* operator connection work area */
static int feedback2;		/* connect for operator 2 */
static int feedback3;		/* connect for operator 3 */
static int feedback4;		/* connect for operator 4 */
static int carrier;			/* connect for carrier    */

/* -------------------- External TimerHandler --------------------*/
/* EXTimerHandler : Stop or Start timer    */
/* int n       = chip number               */
/* int c       = Channel 0=TimerA,1=TimerB */
/* int timeSec = Timer overflow time       */
void (* EXTimerHandler)(int n,int c,double timeSec) = 0;
/* EXIRQHandler : IRQ level are changed    */
/* int n       = chip number               */
/* int irq     = IRQ level 0=OFF,1=ON      */
void (* EXIRQHandler)(int n,int irq) = 0;


/* --------------- Customize External interface port (SSG,Timer,etc) ---------------*/
#include "fmext.c"

/* --------------------- subroutines  --------------------- */
/* ----- key on  ----- */
INLINE void FM_KEYON(FM_CH *CH , int s )
{
	if( CH->evm[s]<= ENV_MOD_RR){
		/* sin wave restart */
		CH->Cnt[s] = 0;
		if( s == SLOT1 ) CH->op1_out = 0;
		/* set attack */
		CH->evm[s] = ENV_MOD_AR;
		CH->arc[s] = ENV_OFF;
#if 1
		CH->evc[s] = 0;
#else
		CH->evc[s] = CH->evsa[s] >= ENV_OFF )  ? 0 : ENV_OFF;
#endif
	}
}
/* ----- key off ----- */
INLINE void FM_KEYOFF(FM_CH *CH , int s )
{
	if( CH->evm[s] > ENV_MOD_RR){
		CH->evm[s] = ENV_MOD_RR;
		/*if( CH->evsr[s] == 0 ) CH->evm[s] &= 0xfe;*/
	}
}

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return == -1:envelope silent */
/*        >=  0:envelope output */
INLINE signed int FM_CALC_SLOT( FM_CH *CH  , int s )
{
	signed int env_out;

	/* calcrate phage generator */
	CH->Cnt[s] += CH->Incr[s];

	/* bypass when envelope holding */
	if( !(CH->evm[s]&0x0f) ) return -1;

	switch( CH->evm[s] ){
	case ENV_MOD_AR:
		/* attack rate : linear(dB) curve */
		if( (CH->arc[s]-= CH->evsa[s]) > 0 )
		{
			CH->evc[s] = AR_CURVE[CH->arc[s]>>ENV_BITS];
			break;
		}
		/* change to next phase */
#ifdef SEG_SUPPORT
		if( !(CH->SEG[s]&8) ){
#endif
			/* next DR */
			CH->evm[s] = ENV_MOD_DR;
/* MB 980409  commented out the following line to fix Bubble Bobble */
			/*if( CH->evsd[s] == 0 ) CH->evm[s] &= 0xfe;    stop   */

			CH->evc[s] = 0;
#ifdef SEG_SUPPORT
		}else{
			/* SSG-EG mode */
			if( CH->SEG[s]&4){	/* start direction */
				/* next SSG-SR (upside start ) */
				CH->evm[s] = ENV_SSG_SR;
				CH->evc[s] = CH->SL[s];
			}else{
				/* next SSG-DR (downside start ) */
				CH->evm[s] = ENV_SSG_DR;
				CH->evc[s] = 0;
			}
		}
#endif
		break;
	case ENV_MOD_DR:
		if( (CH->evc[s]+=CH->evsd[s]) < CH->SL[s] )
		{
			break;
		}
		/* change phase */
		/* next SR */
		CH->evm[s] = ENV_MOD_SR;
/* MB 980409  commented out the following line to fix Bubble Bobble */
		/*if( CH->evss[s] == 0 ) CH->evm[s] &= 0xfe;    stop   */
		CH->evc[s] = CH->SL[s];
		break;
	case ENV_MOD_SR:
		if( (CH->evc[s]+=CH->evss[s]) <= ENV_OFF )
		{
			break;
		}
		/* wait for key off */
		/*CH->evm[s]&= 0xf0;		   off   */
		CH->evc[s] = ENV_OFF;
		break;
	case ENV_MOD_RR:
		if( (CH->evc[s]+=CH->evsr[s]) < ENV_OFF )
		{
			break;
		}
		/* wait for key on */
		CH->evm[s] = ENV_MOD_OFF;
		CH->evc[s] = ENV_OFF;
		break;
#ifdef SEG_SUPPORT
	/* SSG type envelope */
	case ENV_SSG_DR:	/* downside */
		if( (CH->evc[s]+=CH->evsd[s]) < CH->SL[s] )
		{
			break;
		}
		/* change next phase */
		if( CH->SEG[s]&2){
			/* reverce */
			CH->evm[s] = ENV_SSG_SR;
			CH->evc[s] = CH->SL[s];
		}else{
			/* again */
			CH->evc[s] = 0;
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evm[s] &= 0xf0;
		break;
	case ENV_SSG_SR:	/* upside */
		if( (CH->evc[s]-=CH->evss[s]) > 0 )
		{
			break;
		}
		/* next phase */
		if( CH->SEG[s]&2){
			/* reverce  */
			CH->evm[s] = ENV_SSG_DR;
			CH->evc[s] = 0;
		}else{
			/* again */
			CH->evc[s] = CH->SL[s];
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evm[s] &= 0xf0;
#endif
	}
	/* calcrate envelope */
	env_out = CH->TLL[s]+(CH->evc[s]>>ENV_BITS); /* LFOOut[CH->AMS[s]] */
#ifdef TL_SAVE_MEM
	if(env_out >= (EG_ENT-1) )
	{
		return -1;
	}
#endif
	return env_out;
}

/* set algorythm and self feedback */
/* b0-2 : algorythm                */
/* b3-5 : self feedback            */
static void set_algorythm( FM_CH *CH , int algo_fb )
{
	/* setup connect algorythm */
	switch( algo_fb & 7 ){
	case 0:
		/*  PG---S1---S2---S3---S4---OUT */
		CH->connect1 = &feedback2;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 1:
		/*  PG---S1-+-S3---S4---OUT */
		/*  PG---S2-+               */
		CH->connect1 = &feedback3;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 2:
		/* PG---S1------+-S4---OUT */
		/* PG---S2---S3-+          */
		CH->connect1 = &feedback4;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 3:
		/* PG---S1---S2-+-S4---OUT */
		/* PG---S3------+          */
		CH->connect1 = &feedback2;
		CH->connect2 = &feedback4;
		CH->connect3 = &feedback4;
		break;
	case 4:
		/* PG---S1---S2-+--OUT */
		/* PG---S3---S4-+      */
		CH->connect1 = &feedback2;
		CH->connect2 = &carrier;
		CH->connect3 = &feedback4;
		break;
	case 5:
		/*         +-S2-+     */
		/* PG---S1-+-S3-+-OUT */
		/*         +-S4-+     */
		CH->connect1 = 0;	/* special mark */
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
		break;
	case 6:
		/* PG---S1---S2-+     */
		/* PG--------S3-+-OUT */
		/* PG--------S4-+     */
		CH->connect1 = &feedback2;
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
		break;
	case 7:
		/* PG---S1-+     */
		/* PG---S2-+-OUT */
		/* PG---S3-+     */
		/* PG---S4-+     */
		CH->connect1 = &carrier;
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
	}
	/* self feedback */
	algo_fb = (algo_fb>>3) & 7;
	CH->FB = algo_fb ? 8 - algo_fb : 0;
	CH->op1_out = 0;
}

/* operator output calcrator */
#define OP_OUT(SLOT,CON)   SIN_TABLE[((CH->Cnt[SLOT]+CON)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env_out]
/* ---------- calcrate one of channel ---------- */
INLINE int FM_CALC_CH( FM_CH *CH )
{
	int op_out;
	int env_out;

	/* bypass all SLOT output off (SILENCE) */
	if( !(*(long *)(&CH->evm[0])) ) return 0;

	/* clear carrier output */
	feedback2 = feedback3 = feedback4 = carrier = 0;

	/* SLOT 1 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT1))>=0 )
	{
		if( CH->FB ){
			/* with self feed back */
			op_out = CH->op1_out;
			CH->op1_out = OP_OUT(SLOT1,(CH->op1_out>>CH->FB) /* +LFOOut[CH->AMS[s]]*/ );
			op_out = (op_out + CH->op1_out)/2;
		}else{
			/* without self feed back */
			op_out = OP_OUT(SLOT1,0 /* +LFOOut[CH->AMS[s]]*/ );
		}
		if( !CH->connect1 )
		{
			/* algorythm 5  */
			feedback2 = feedback3 = feedback4 = op_out;
		}else{
			/* other algorythm */
			*CH->connect1 = op_out;
		}
	}
	/* SLOT 2 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT2))>=0 )
		*CH->connect2 += OP_OUT(SLOT2, feedback2 /* +LFOOut[CH->AMS[s]]*/  );
	/* SLOT 3 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT3))>=0 )
		*CH->connect3 += OP_OUT(SLOT3, feedback3  /* +LFOOut[CH->AMS[s]]*/ );
	/* SLOT 4 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT4))>=0 )
		carrier       += OP_OUT(SLOT4, feedback4  /* +LFOOut[CH->AMS[s]]*/ );
	return carrier;
}
/* ---------- frequency counter for operater update ---------- */
INLINE void CALC_FCSLOT(FM_CH *CH , int s , int fc , int kc )
{
	int ksr;

	CH->Incr[s]= (fc * CH->mul[s]) + CH->DT[s][kc];

	ksr = kc >> CH->KSR[s];
	if( CH->ksr[s] != ksr )
	{
		CH->ksr[s] = ksr;
		/* attack . decay rate recalcration */
		CH->evsa[s] = CH->AR[s][ksr];
		CH->evsd[s] = CH->DR[s][ksr];
		CH->evss[s] = CH->SR[s][ksr];
		CH->evsr[s] = CH->RR[s][ksr];
	}
	CH->TLL[s] = CH->TL[s] + KSL[kc];
}

/* ---------- frequency counter  ---------- */
INLINE void CALC_FCOUNT(FM_CH *CH )
{
	if( CH->Incr[SLOT1]==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		CALC_FCSLOT(CH , SLOT1 , fc , kc );
		CALC_FCSLOT(CH , SLOT2 , fc , kc );
		CALC_FCSLOT(CH , SLOT3 , fc , kc );
		CALC_FCSLOT(CH , SLOT4 , fc , kc );
	}
}

/* ---------- frequency counter  ---------- */
INLINE void OPM_CALC_FCOUNT(YM2151 *OPM , FM_CH *CH )
{
	if( CH->Incr[SLOT1]==-1)
	{
		int kc = CH->kcode;
		CALC_FCSLOT(CH , SLOT1 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT1]] , kc );
		CALC_FCSLOT(CH , SLOT2 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT2]] , kc );
		CALC_FCSLOT(CH , SLOT3 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT3]] , kc );
		CALC_FCSLOT(CH , SLOT4 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT4]] , kc );
	}
}

/* ----------- initialize time tabls ----------- */
static void init_timetables( FM_ST *ST , char *DTTABLE , int ARRATE , int DRRATE )
{
	int i,d;
	float rate;

	/* make detune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = (float)DTTABLE[d*32 + i] * ST->freqbase / 4096 * FREQ_RATE;
			ST->DT_TABLE[d][i]   =  rate;
			ST->DT_TABLE[d+4][i] = -rate;
		}
	}
	/* make attack rate & decay rate tables */
	for (i = 0;i < 4;i++) ST->AR_TABLE[i] = ST->DR_TABLE[i] = 0;
	for (i = 4;i < 64;i++){
		if( i == 0 ) rate = 0;
		else
		{
			rate  = (float)ST->freqbase / 4096.0;		/* frequency rate */
			if( i < 60 ) rate *= 1.0+(i&3)*0.25;		/* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
			rate *= 1<<((i>>2)-1);						/* b2-5 : shift bit */
			rate *= (float)(EG_ENT<<ENV_BITS);
		}
		ST->AR_TABLE[i] = rate / ARRATE;
		ST->DR_TABLE[i] = rate / DRRATE;
	}
	ST->AR_TABLE[63] = ENV_OFF;

	for (i = 64;i < 94 ;i++){	/* make for overflow area */
		ST->AR_TABLE[i] = ST->AR_TABLE[63];
		ST->DR_TABLE[i] = ST->DR_TABLE[63];
	}

#if 0
	if(errorlog){
		for (i = 0;i < 64 ;i++){	/* make for overflow area */
			fprintf(errorlog,"rate %2d , ar %f ms , dr %f ms \n",i,
				((float)(EG_ENT<<ENV_BITS) / ST->AR_TABLE[i]) * (1000.0 / ST->rate),
				((float)(EG_ENT<<ENV_BITS) / ST->DR_TABLE[i]) * (1000.0 / ST->rate) );
		}
	}
#endif
}

/* ---------- reset one of channel  ---------- */
static void reset_channel( FM_ST *ST , FM_CH *ch , int chan )
{

	int c,s;

	ST->mode   = 0;	/* normal mode */
	ST->status = 0;
	ST->bufp   = 0;
	ST->TA     = 0;
	ST->TAC    = 0;
	ST->TB     = 0;
	ST->TBC    = 0;

	ST->timer_a_timer = 0;
	ST->timer_b_timer = 0;

	for( c = 0 ; c < chan ; c++ )
	{
		ch[c].fc = 0;
		ch[c].PAN = 3;
		for(s = 0 ; s < 4 ; s++ )
		{
			ch[c].SEG[s] = 0;
			ch[c].evc[s] = ENV_OFF;
			ch[c].evm[s] = ENV_MOD_OFF;
		}
	}
}

/* ---------- generic table initialize ---------- */
static int FMInitTable( void )
{
	int s,t;
	float rate;
	int i,j;
	float pom;

	/* allocate total level table */
	TL_TABLE = malloc(TL_MAX*2*sizeof(int));
	if( TL_TABLE == 0 ) return 0;
	/* make total level table */
	for (t = 0;t < EG_ENT-1 ;t++){
		rate = ((1<<TL_BITS)-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		TL_TABLE[       t] =  (int)rate;
		TL_TABLE[TL_MAX+t] = -TL_TABLE[t];
/*		if(errorlog) fprintf(errorlog,"TotalLevel(%3d) = %x\n",t,TL_TABLE[t]);*/
	}
	/* fill volume off area */
	for ( t = EG_ENT-1; t < TL_MAX ;t++){
		TL_TABLE[t] = TL_TABLE[TL_MAX+t] = 0;
	}

	/* make sinwave table (total level offet) */
	 /* degree 0 = degree 180                   = off */
	SIN_TABLE[0] = SIN_TABLE[SIN_ENT/2]         = &TL_TABLE[EG_ENT-1];
	for (s = 1;s <= SIN_ENT/4;s++){
		pom = sin(2*PI*s/SIN_ENT); /* sin     */
		pom = 20*log10(1/pom);	   /* decibel */
		j = pom / EG_STEP;         /* TL_TABLE steps */

        /* degree 0   -  90    , degree 180 -  90 : plus section */
		SIN_TABLE[          s] = SIN_TABLE[SIN_ENT/2-s] = &TL_TABLE[j];
        /* degree 180 - 270    , degree 360 - 270 : minus section */
		SIN_TABLE[SIN_ENT/2+s] = SIN_TABLE[SIN_ENT  -s] = &TL_TABLE[TL_MAX+j];
/*		if(errorlog) fprintf(errorlog,"sin(%3d) = %f:%f db\n",s,pom,(float)j * EG_STEP);*/
	}

	/* attack count -> envelope count */
	for (i=0; i<EG_ENT; i++)
	{
		/* !!!!! preliminary !!!!! */
		pom = pow( ((float)i/EG_ENT) , 8 ) * EG_ENT;
		j = pom * (1<<ENV_BITS);
		if( j > ENV_OFF ) j = ENV_OFF;

		AR_CURVE[i]=j;
/*		if (errorlog) fprintf(errorlog,"arc->env %04i = %x\n",i,j>>ENV_BITS);*/
	}

	return 1;
}

static void FMCloseTable( void )
{
	if( TL_TABLE ) free( TL_TABLE );
	return;
}

/* OPN/OPM Mode  Register Write */
static inline void FMSetMode( FM_ST *ST ,int n,int v )
{
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = irq enable b */
	/* b2 = irq enable a */
	/* b1 = load b */
	/* b0 = load a */
	ST->mode = v;
	ST->irqmask = (v >>2) & 0x03;

	if( v & 0x20 )
	{
		ST->TBC = 0;		/* timer stop */
		ST->status &=0xfd; /* reset TIMER B */
		if ( ST->irq && !(ST->status & ST->irqmask) )
		{
			ST->irq = 0;
			/* callback user interrupt handler */
			if(EXIRQHandler) EXIRQHandler(n,0);
		}
		/* External timer handler */
		if (EXTimerHandler) EXTimerHandler(n,1,0);
	}
	if( v & 0x10 )
	{
		ST->status &=0xfe; /* reset TIMER A */
		if ( ST->irq && !(ST->status & ST->irqmask) )
		{
			ST->irq = 0;
			/* callback user interrupt handler */
			if(EXIRQHandler) EXIRQHandler(n,0);
		}
		ST->TAC = 0;		/* timer stop */
		/* External timer handler */
		if (EXTimerHandler) EXTimerHandler(n,0,0);
	}
	if( v & 0x02 )
	{
		if( ST->TBC == 0 )
		{
			ST->TBC = ( 256-ST->TB)<<(4+12);
#ifdef NICK_BUBLBOBL_CHANGE
			if( ST->TAC ) ST->TBC -= 1023;
#endif
			/* External timer handler */
			if (EXTimerHandler) EXTimerHandler(n,1,(double)ST->TBC / ((double)ST->freqbase * (double)ST->rate));
		}
	}
	if( v & 0x01 )
	{
		if( ST->TAC == 0 )
		{
			ST->TAC = (1024-ST->TA)<<12;
#ifdef NICK_BUBLBOBL_CHANGE
			if( ST->TBC ) ST->TAC -= 1023;
#endif
			/* External timer handler */
			if (EXTimerHandler) EXTimerHandler(n,0,(double)ST->TAC / ((double)ST->freqbase * (double)ST->rate));
		}
	}
}


/* Timer A Overflow */
static inline void TimerAOver(int n,FM_ST *ST)
{
	/* set status flag */
	ST->status |= 0x01;
	if ( !ST->irq && (ST->status & ST->irqmask) )
	{
		ST->irq = 1;
		/* callback user interrupt handler */
		if(EXIRQHandler) EXIRQHandler(n,1);
	}
	/* update the counter */
	ST->TAC = 0;
}
/* Timer A Overflow */
static inline void TimerBOver(int n,FM_ST *ST)
{
	/* set status flag */
	ST->status |= 0x02;
	if ( !ST->irq && (ST->status & ST->irqmask) )
	{
		ST->irq = 1;
		/* callback user interrupt handler */
		if(EXIRQHandler) EXIRQHandler(n,1);
	}
	/* update the counter */
	ST->TBC = 0;
}
/* CSM Key Controll */
static inline void CSMKeyControll(FM_CH *CH)
{
	int ksl = KSL[CH->kcode];
	/* all key off */
	FM_KEYOFF(CH,SLOT1);
	FM_KEYOFF(CH,SLOT2);
	FM_KEYOFF(CH,SLOT3);
	FM_KEYOFF(CH,SLOT4);
	/* total level latch */
	CH->TLL[SLOT1] = CH->TL[SLOT1] + ksl;
	CH->TLL[SLOT2] = CH->TL[SLOT2] + ksl;
	CH->TLL[SLOT3] = CH->TL[SLOT3] + ksl;
	CH->TLL[SLOT4] = CH->TL[SLOT4] + ksl;
	/* all key on */
	FM_KEYON(CH,SLOT1);
	FM_KEYON(CH,SLOT2);
	FM_KEYON(CH,SLOT3);
	FM_KEYON(CH,SLOT4);
}

#ifdef INTERNAL_TIMER
/* ---------- calcrate timer A ---------- */
INLINE void CALC_TIMER_A(FM_ST *ST , FM_CH *CSM_CH )
{
	if( ST->TAC &&  (EXTimerHandler==0) )
	{
		TimerAOver( n,ST );
		/* CSM mode key,TL controll */
		if( ST->mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( CSM_CH );
		}
	}
}
/* ---------- calcrate timer B ---------- */
INLINE void CALC_TIMER_B(FM_ST *ST,int step)
{
	if( ST->TBC && (EXTimerHandler==0) )
		if( (ST->TBC -= ST->freqbase*step) <= 0 )
		{
			TimerBOver( n,ST );
		}
	}
}
#endif /* INTERNAL_TIMER */

void FMSetTimerHandler( void (*TimerHandler)(int n,int c,double timeSec),
                        void (*IRQHandler)(int n,int irq) )
{
	EXTimerHandler = TimerHandler;
	EXIRQHandler   = IRQHandler;
}

#ifdef BUILD_OPN
/* ---------- priscaler set(and make time tables) ---------- */
void OPNSetPris(int num , int pris , int SSGpris)
{
    YM2203 *OPN = &(FMOPN[num]);
	int fn;

/*	OPN->PR = pris; */
	if (OPN->ST.rate)
		OPN->ST.freqbase = ((float)OPN->ST.clock * 4096.0 / OPN->ST.rate) / 12 / pris;
	else OPN->ST.freqbase = 0;

	/* SSG part  priscaler set */
	if( SSGpris ) SSGClk( num, OPN->ST.clock * 2 / SSGpris , OPN->ST.rate );
	/* make time tables */
	init_timetables( &OPN->ST , OPN_DTTABLE , OPN_ARRATE , OPN_DRRATE );
	/* make fnumber -> increment counter table */
	for( fn=0 ; fn < 2048 ; fn++ )
	{
		/* it is freq table for octave 7 */
		/* opn freq counter = 20bit */
		OPN->FN_TABLE[fn] = (float)fn * OPN->ST.freqbase / 4096  * FREQ_RATE * (1<<7) / 2;
	}
/*	if(errorlog) fprintf(errorlog,"OPN %d set priscaler %d\n",num,pris);*/
}

/* ---------- write a OPN mode register 0x20-0x2f ---------- */
static void OPNWriteMode(int n, int r, int v,FM_CH *EXCH)
{
    YM2203 *OPN = &(FMOPN[n]);
	unsigned char c;
	FM_CH *CH;

	switch(r){
	case 0x21:	/* Test */
		break;
	case 0x22:	/* LFO FREQ (YM2608/YM2612) */
		/* 3.98Hz,5.56Hz,6.02Hz,6.37Hz,6.88Hz,9.63Hz,48.1Hz,72.2Hz */
		/* FM2608[n].LFOIncr = FM2608[n].LFO_TABLE[v&0x0f]; */
		break;
	case 0x24:	/* timer A High 8*/
		OPN->ST.TA = (OPN->ST.TA & 0x03)|(((int)v)<<2);
		break;
	case 0x25:	/* timer A Low 2*/
		OPN->ST.TA = (OPN->ST.TA & 0x3fc)|(v&3);
		break;
	case 0x26:	/* timer B */
		OPN->ST.TB = v;
		break;
	case 0x27:	/* mode , timer controll */
		FMSetMode( &(OPN->ST),n,v );
		break;
	case 0x28:	/* key on / off */
		c = v&0x03;
		if( c == 3 ) break;
		CH = &(OPN->CH[c]);
		if( (v&0x04) &&  EXCH )
		{
			CH = &EXCH[c];
			c +=3;
		}
		/* csm mode */
		if( c == 2 && (OPN->ST.mode & 0x80) ) break;
		if(v&0x10) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
		if(v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
		if(v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
		if(v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
/*		if(errorlog) fprintf(errorlog,"OPN %d:%d : KEY %02X\n",n,c,v&0xf0);*/
		break;
	case 0x2d:	/* divider sel ( YM2203/YM2608 ) */
		OPNSetPris( n, 6 , 4 ); /* OPN = 1/6 , SSG = 1/4 */
		break;
	case 0x2e:	/* divider sel ( YM2203/YM2608 ) */
		OPNSetPris( n, 3 , 2 ); /* OPN = 1/3 , SSG = 1/2 */
		break;
	case 0x2f:	/* divider sel ( YM2203/YM2608 ) */
		OPNSetPris( n, 2 , 1 ); /* OPN = 1/2 , SSG = 1/1 */
		break;
	}
}

/* ---------- write a OPN register (0x30-0xff) ---------- */
static void OPNWriteReg(int n, int r, int v,FM_CH *EXCH)
{
	unsigned char c,s;
	FM_CH *CH;

    YM2203 *OPN = &(FMOPN[n]);

	/* 0x30 - 0xff */
	if( (c = OPN_CHAN(r)) == 3 ) return; /* 0xX3,0xX7,0xXB,0xXF */
	if( r >= 0x100 )
	{	/* extend channel */
		if( !EXCH ) return;
		CH = &EXCH[c];
		c +=3;
	}
	else CH = &(OPN->CH[c]);

	s  = OPN_SLOT(r);
	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		CH->mul[s] = MUL_TABLE[v&0xf];
		CH->DT[s]  = OPN->ST.DT_TABLE[(v>>4)&7];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x40:	/* TL */
		v &= 0x7f;
		v = (v<<7)|v; /* 7bit -> 14bit */
		CH->TL[s] = (v*EG_ENT)>>14;
		if( c == 2 && (OPN->ST.mode & 0x80) ) break;	/* CSM MODE */
		CH->TLL[s] = CH->TL[s] + KSL[CH->kcode];
		break;
	case 0x50:	/* KS, AR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : AR %d\n",n,c,s,v&0x1f);*/
		CH->KSR[s]  = 3-(v>>6);
		CH->AR[s]   = (v&=0x1f) ? &OPN->ST.AR_TABLE[v<<1] : RATE_0;
		CH->evsa[s] = CH->AR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x60:	/*     DR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : DR %d\n",n,c,s,v&0x1f);*/
		/* bit7 = AMS ENABLE(YM2612) */
		CH->DR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		CH->evsd[s] = CH->DR[s][CH->ksr[s]];
		break;
	case 0x70:	/*     SR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SR %d\n",n,c,s,v&0x1f);*/
		CH->SR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		CH->evss[s] = CH->SR[s][CH->ksr[s]];
		break;
	case 0x80:	/* SL, RR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SL %d RR %d \n",n,c,s,v>>4,v&0x0f);*/
		CH->SL[s] = SL_TABLE[(v>>4)];
		CH->RR[s] = (v&=0x0f) ? &OPN->ST.DR_TABLE[(v<<2)|2] : RATE_0;
		CH->evsr[s] = CH->RR[s][CH->ksr[s]];
		break;
	case 0x90:	/* SSG-EG */
#ifndef SEG_SUPPORT
		if(errorlog && (v&0x08)) fprintf(errorlog,"OPN %d,%d,%d :SSG-TYPE envelope selected (not supported )\n",n,c,s );
#endif
		CH->SEG[s] = v&0x0f;
		break;
	case 0xa0:
		switch(s){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				unsigned int fn  = (((unsigned int)( (CH->fn_h)&7))<<8) + v;
				unsigned char blk = CH->fn_h>>3;
				/* make keyscale code */
				CH->kcode = (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				CH->fc = OPN->FN_TABLE[fn]>>(7-blk);
				CH->Incr[SLOT1]=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			CH->fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			if( r < 0x100)
			{
				unsigned int fn  = (((unsigned int)(OPN->SL3.fn_h[c]&7))<<8) + v;
				unsigned char blk = OPN->SL3.fn_h[c]>>3;
				/* make keyscale code */
				OPN->SL3.kcode[c]= (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				OPN->SL3.fc[c] = OPN->FN_TABLE[fn]>>(7-blk);
				OPN->CH[2].Incr[SLOT1]=-1;
			}
			break;
		case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
			if( r < 0x100)
				OPN->SL3.fn_h[c] = v&0x3f;
			break;
		}
		break;
	case 0xb0:
		switch(s){
		case 0:		/* 0xb0-0xb2 : FB,ALGO */
			set_algorythm( CH , v );
			/*if(errorlog) fprintf(errorlog,"OPN %d,%d : AL %d FB %d\n",n,c,v&7,(v>>3)&7);*/
			break;
		case 1:	/* L , R , AMS , PMS (YM2612/YM2608) */
			/* b0-2 PMS */
			/* 0,3.4,6.7,10,14,20,40,80(cent) */
			CH->pms[s] = (v>>4) & 0x07;
			/* b4-5 AMS */
			/* 0,1.4,5.9,11.8(dB) */
			CH->ams[s] = v & 0x03;
			CH->PAN = (v>>6)&0x03; /* PAN : b6 = R , b7 = L */
			cur_chip = NULL;
			/* if(errorlog) fprintf(errorlog,"OPN %d,%d : PAN %d\n",n,c,CH->PAN);*/
			break;
		}
		break;
	}
}

/* ---------- reset one of chip ---------- */
void YM2203ResetChip(int num)
{
    int i;
    YM2203 *OPN = &(FMOPN[num]);

	/* Reset Priscaler */
/*	OPNSetPris( num , 6 , 4);    1/6 , 1/4   */
	OPNSetPris( num , 3 , 2); /* 1/3 , 1/2 */
	/* reset SSG section */
	SSGReset(num);
	/* status clear */
	OPNWriteMode(num,0x27,0x30,0); /* mode 0 , timer reset */
	reset_channel( &OPN->ST , &OPN->CH[0] , 3 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- ) OPNWriteReg(num,i,0xc0,0);
	for(i = 0xb2 ; i >= 0x30 ; i-- ) OPNWriteReg(num,i,0,0);
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(num,i,0,0);
}
/* ---------- release storage for a chip ---------- */
static void _OPNFreeChip(int num)
{
	int i;

	for( i = 0 ; i < FM_NUMBUF ; i++){
	    FMOPN[num].Buf[i] = NULL;
	}
}

/* ---------- set buffer ---------- */
int YM2203SetBuffer(int n, FMSAMPLE *buf )
{
	if( buf == 0 ) return -1;
    FMOPN[n].Buf[0] = buf;
	return 0;
}

/* ----------  Initialize YM2203 emulator(s) ----------    */
/* 'num' is the number of virtual YM2203's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int YM2203Init(int num, int clock, int rate,  int bitsize, int bufsiz , FMSAMPLE **buffer )
{
    int i;

    if (FMOPN) return (-1);	/* duplicate init. */

	ChipMode = CHIP_YM2203;

    FMNumChips = num;
    FMBufSize = bufsiz;
    if( bitsize == 16 ) sample_16bit = 1;
    else                sample_16bit = 0;

	/* allocate ym2203 state space */
	if( (FMOPN = (YM2203 *)malloc(sizeof(YM2203) * FMNumChips))==NULL)
		return (-1);
	/* allocate total lebel table (128kb space) */
	if( !FMInitTable() )
	{
		free( FMOPN );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		FMOPN[i].ST.clock = clock;
		FMOPN[i].ST.rate = rate;
		FMOPN[i].ST.handler = 0;
		if( YM2203SetBuffer(i,buffer[i]) < 0 ) {
		    int j;
		    for ( j = 0 ; j < i ; j ++ ) {
				_OPNFreeChip(j);
		    }
		    return(-1);
		}
		YM2203ResetChip(i);
    }
    return(0);
}

/* ---------- shut down emurator ----------- */
void YM2203Shutdown(void)
{
    int i;

    if (!FMOPN) return;
    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_OPNFreeChip(i);
	}
	free(FMOPN); FMOPN = NULL;
	FMCloseTable();
	FMBufSize = 0;
}
#endif /* BUILD_OPN */


#ifdef BUILD_YM2203
/*******************************************************************************/
/*		YM2203 local section                                                   */
/*******************************************************************************/

/* ---------- update one of chip ----------- */
void YM2203UpdateOne(int num, int endp)
{
    YM2203 *OPN = &(FMOPN[num]);
    int i;
	int opn_data;
	FMSAMPLE *buffer = OPN->Buf[0];

	State = &OPN->ST;
	chA   = &OPN->CH[0];
	chB   = &OPN->CH[1];
	chC   = &OPN->CH[2];

	/* frequency counter channel A */
	CALC_FCOUNT( chA );
	/* frequency counter channel B */
	CALC_FCOUNT( chB );
	/* frequency counter channel C */
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( chC->Incr[SLOT1]==-1){
			/* 3 slot mode */
			CALC_FCSLOT(chC , SLOT1 , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(chC , SLOT2 , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(chC , SLOT3 , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(chC , SLOT4 , chC->fc , chC->kcode );
		}
	}else CALC_FCOUNT( chC );

    for( i=State->bufp; i < endp ; ++i )
	{
		/*            channel A         channel B         channel C      */
		opn_data  = FM_CALC_CH(chA) + FM_CALC_CH(chB) + FM_CALC_CH(chC);
		/* limit check */
		if( opn_data > OPN_MAXOUT )      opn_data = OPN_MAXOUT;
		else if( opn_data < OPN_MINOUT ) opn_data = OPN_MINOUT;
		/* store to sound buffer */
		if( sample_16bit ) ((unsigned short *)buffer)[i] = opn_data >> OPN_OUTSB;
		else           ((unsigned char  *)buffer)[i] = opn_data >> OPN_OUTSB_8;
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , chC );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void YM2203Update(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPN[i].ST.bufp <  FMBufSize ) YM2203UpdateOne(i , FMBufSize );
		FMOPN[i].ST.bufp = 0;
    }
}

#if 0
/* ---------- read status port ---------- */
unsigned char YM2203ReadStatus(int n,int a)
{
	if( FMOPN[n].ST.address < 16 )
	{
		return SSGRead(n,FMOPN[n].ST.address);
	}
	return FMOPN[n].ST.status;
}
#endif

/* ---------- return the buffer ---------- */
FMSAMPLE *YM2203Buffer(int n)
{
    return FMOPN[n].Buf[0];
}

/* ---------- YM2203 I/O interface ---------- */
int YM2203Write(int n,int a,int v)
{
	YM2203 *F2203 = &(FMOPN[n]);
#ifdef SELF_UPDATE
	int pos;
#endif

	if( !(a&1) )
	{	/* address port */
		F2203->ST.address = v & 0xff;
		/* Write register to SSG emurator */
		if( v < 16 ) SSGWrite(n,0,v);
	}
	else
	{	/* data port */
		int addr = F2203->ST.address;
		switch( addr & 0xf0 )
		{
		case 0x00:	/* 0x00-0x0f : SSG section */
			/* Write data to SSG emurator */
			SSGWrite(n,a,v);
			break;
		case 0x20:	/* 0x20-0x2f : Mode section */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2203UpdateOne(n,pos);
#endif
			/* write register */
			 OPNWriteMode(n,addr,v,0);
			break;
		default:	/* 0x30-0xff : OPN section */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2203UpdateOne(n,pos);
#endif
			/* write register */
			 OPNWriteReg(n,addr,v,0);
		}
	}
	return F2203->ST.irq;
}

unsigned char YM2203Read(int n,int a)
{
	YM2203 *F2203 = &(FMOPN[n]);
	int addr = F2203->ST.address;
	int ret = 0;

	if( !(a&1) )
	{	/* status port */
		ret = F2203->ST.status;
	}
	else
	{	/* data port (ONLY SSG) */
		if( addr < 16 ) ret = SSGRead(n,a);
	}
	return ret;
}

int YM2203TimerOver(int n,int c)
{
	YM2203 *F2203 = &(FMOPN[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( n,&(F2203->ST) );
	}
	else
	{	/* Timer A */
#ifdef SELF_UPDATE
		/* update the system */
		int pos = FMUpdatePos();
		if( F2203->ST.bufp < pos ) YM2203UpdateOne(n, pos );
#endif
		/* timer update */
		TimerAOver( n,&(F2203->ST) );
		/* CSM mode key,TL controll */
		if( F2203->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->ST.irq;
}

#endif /* BUILD_YM2203 */


#ifdef BUILD_YM2608
/*******************************************************************************/
/*		YM2608 local section                                                   */
/*******************************************************************************/

/* Get next pcm data */
static inline int YM2608ReadADPCM(int n)
{
	YM2203 *F2203 = &(FMOPN[n]);
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* buffer memory */
		return 0;
	}
	else
	{	/* from PCM data register */
		//F2203->ST.status |= 0x00;
		return F2608->ADData;
	}
}

/* Put decoded data */
static inline void YM2608WriteADPCM(int n,int v)
{
	YM2203 *F2203 = &(FMOPN[n]);
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* for buffer */
		return;
	}
	else
	{	/* for PCM data port */
		//F2203->ST.status |= 0x00;
		F2608->ADData = v;
	}
}

/* ---------- ADPCM Register Write 0x100-0x10f ---------- */
static inline void YM2608ADPCMWrite(int n,int r,int v)
{
	YM2203 *F2203 = &(FMOPN[n]);
	YM2608 *F2608 = &(FM2608[n]);

	switch(r)
	{
	case 0x00:
		{
		/* START,REC,MEMDATA,REPEAT,SPOFF,--,--,RESET */
		int cng = F2608->ADMode ^ v;
		F2608->ADMode = v;
		switch( v & 0xe0 )
		{
		case 0x80:	/* ADPCM play from PCM data port after write PCM data */
			break;
		case 0xa0:	/* ADPCM play from buffer memory */
			break;
		case 0x60:	/* write buffer MEMORY from PCM data port */
			break;
		case 0x20:	/* read  buffer MEMORY to   PCM data port */
			break;
		}
		}
		break;
	case 0x01:
		/* L,R,-,-,SAMPLE,DA/AD,RAMTYPE,ROM */
		F2608->ADPAN = v>>6;
		break;
	case 0x02:	/* Start Address L */
		F2608->ADStart = (F2608->ADStart & 0xff00) | v;
		break;
	case 0x03:	/* Start Address H */
		F2608->ADStart = (F2608->ADStart & 0x00ff) | (v<<8);
		break;
	case 0x04:	/* Stop Address L */
		F2608->ADStop = (F2608->ADStop & 0xff00) | v;
		break;
	case 0x05:	/* Stop Address H */
		F2608->ADStop = (F2608->ADStop & 0x00ff) | (v<<8);
		break;
	case 0x06:	/* Prescale L (PCM and Recoard frq) */
	case 0x07:	/* Proscale H */
		break;
	case 0x08:	/* ADPCM data */
		break;
	case 0x09:	/* DELTA-N L (ADPCM Playback Prescaler) */
		F2608->ADDelta = (F2608->ADDelta & 0xff00) | v;
		break;
	case 0x0a:	/* DELTA-N H */
		F2608->ADDelta = (F2608->ADDelta & 0x00ff) | (v<<8);
		break;
	case 0x0b:	/* Level control (volume) */
		F2608->ADTL = v;
		break;
	case 0x0c:	/* Limit address L */
		F2608->ADLimit = (F2608->ADLimit & 0xff00) | v;
		break;
	case 0x0d:	/* Limit address H */
		F2608->ADLimit = (F2608->ADLimit & 0x00ff) | (v<<8);
		break;
	case 0x0e:	/* DAC data */
		break;
	case 0x0f:	/* PCM data port */
		F2608->ADData = v;
		//F2203->ST.status &= 0xff;
		break;
	}
}

/* ---------- Rhythm Register Write 0x110-0x11f ---------- */
static inline void YM2608RhythmWrite(int n,int r,int v)
{
	YM2608 *F2608 = &(FM2608[n]);
	int c;

	switch( r ){
	case 0x10: /* DM,--,RIM,TOM,HH,TOP,SD,BD */
		if( v & 0x80 )
		{	/* dump */
			if( v & 0x20 ) RhythmStop(n,RY_RIM);
			if( v & 0x10 ) RhythmStop(n,RY_TOM);
			if( v & 0x08 ) RhythmStop(n,RY_HH);
			if( v & 0x04 ) RhythmStop(n,RY_TOP);
			if( v & 0x02 ) RhythmStop(n,RY_SD);
			if( v & 0x01 ) RhythmStop(n,RY_BD);
		}
		else
		{	/* key on */
			if( v & 0x20 ) RhythmStart(n,RY_RIM);
			if( v & 0x10 ) RhythmStart(n,RY_TOM);
			if( v & 0x08 ) RhythmStart(n,RY_HH);
			if( v & 0x04 ) RhythmStart(n,RY_TOP);
			if( v & 0x02 ) RhythmStart(n,RY_SD);
			if( v & 0x01 ) RhythmStart(n,RY_BD);
		}
		break;
	case 0x11:	/* B0-5 = RTL */
		F2608->RTL = (v & 0x3f);
		break;
	case 0x18:	/* BD  : B7=L,B6=R,B4-0=IL */
	case 0x19:	/* SD  : B7=L,B6=R,B4-0=IL */
	case 0x1A:	/* TOP : B7=L,B6=R,B4-0=IL */
	case 0x1B:	/* HH  : B7=L,B6=R,B4-0=IL */
	case 0x1C:	/* TOM : B7=L,B6=R,B4-0=IL */
	case 0x1D:	/* RIM : B7=L,B6=R,B4-0=IL */
		c = r & 0x18;
		/* B7=L,B6=R,B4-0=BD IL */
		F2608->RPAN[c] = v>>6;
		F2608->RIL[c] = v&0x1f;
		break;
	}
}

static void _YM2608FreeChip(int num)
{
	int i;

	for( i = 0 ; i < FM_NUMBUF ; i++){
	    FMOPN[num].Buf[i] = NULL;
	}
}

/* ---------- update one of chip ----------- */
void YM2608UpdateOne(int num, int endp)
{
	YM2203 *OPN  = &(FMOPN[num]);
	YM2608 *F2608 = &(FM2608[num]);
	int dataR,dataL;
	int i;

	if( (void *)F2608 != cur_chip ){
		cur_chip = (void *)F2608;

		/* set bufer */
		bufL = OPN->Buf[0];
		bufR = OPN->Buf[1];

		State = &OPN->ST;
		chA   = &OPN->CH[0];
		chB   = &OPN->CH[1];
		chC   = &OPN->CH[2];
		chD   = &F2608->CH[0];
		chE   = &F2608->CH[1];
		chF   = &F2608->CH[2];
		pan[0] = &outd[chA->PAN];
		pan[1] = &outd[chB->PAN];
		pan[2] = &outd[chC->PAN];
		pan[3] = &outd[chD->PAN];
		pan[4] = &outd[chE->PAN];
		pan[5] = &outd[chF->PAN];
	}
	/* update frequency counter */
	CALC_FCOUNT( chA );
	CALC_FCOUNT( chB );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( chC->Incr[SLOT1]==-1){
			/* 3 slot mode */
			CALC_FCSLOT(chC , SLOT1 , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(chC , SLOT2 , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(chC , SLOT3 , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(chC , SLOT4 , chC->fc , chC->kcode );
		}
	}else CALC_FCOUNT( chC );
	CALC_FCOUNT( chD );
	CALC_FCOUNT( chE );
	CALC_FCOUNT( chF );
	/* buffering */
    for( i=State->bufp; i < endp ; ++i )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/* calcrate channel output */
		*pan[0] += FM_CALC_CH( chA );
		*pan[1] += FM_CALC_CH( chB );
		*pan[2] += FM_CALC_CH( chC );
		*pan[3] += FM_CALC_CH( chD );
		*pan[4] += FM_CALC_CH( chE );
		*pan[5] += FM_CALC_CH( chF );
		/* get left & right output */
		dataL = outd[OPN_CENTER] + outd[OPN_LEFT];
		dataR = outd[OPN_CENTER] + outd[OPN_RIGHT];
		/* clipping data */
		if( dataL > OPN_MAXOUT ) dataL = OPN_MAXOUT;
		else if( dataL < OPN_MINOUT ) dataL = OPN_MINOUT;
		if( dataR > OPN_MAXOUT ) dataR = OPN_MAXOUT;
		else if( dataR < OPN_MINOUT ) dataR = OPN_MINOUT;
		/* buffering */
		if( sample_16bit )
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned long *)bufL)[i] = ((dataL>>OPN_OUTSB)<<16)(dataR>>OPN_OUTSB);
#else
			/* stereo separate */
			((unsigned short *)bufL)[i] = dataL>>OPN_OUTSB;
			((unsigned short *)bufR)[i] = dataR>>OPN_OUTSB;
#endif
		}
		else
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned shart *)bufL)[i] = ((dataL>>OPN_OUTSB_8)<<8)(dataR>>OPN_OUTSB_8);
#else
			/* stereo separate */
			((unsigned char *)bufL)[i] = dataL>>OPN_OUTSB_8;
			((unsigned char *)bufR)[i] = dataR>>OPN_OUTSB_8;
#endif
		}

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , chC );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void YM2608Update(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPN[i].ST.bufp <  FMBufSize ) YM2608UpdateOne(i , FMBufSize );
		FMOPN[i].ST.bufp = 0;
    }
}

/* -------------------------- YM2608(OPNA) ---------------------------------- */
int YM2608Init(int num, int clock, int rate,  int bitsize, int bufsiz , FMSAMPLE **buffer)
{
	int i,j;

    if (FM2608) return (-1);	/* duplicate init. */

	if( YM2203Init(num,clock,rate,bitsize,bufsiz ,buffer ) != 0)
		return (-1);
	/* allocate extend state space */
	if( (FM2608 = (YM2608 *)malloc(sizeof(YM2608) * FMNumChips))==NULL)
	{
		YM2203Shutdown();
		return (-1);
	}
	ChipMode = CHIP_YM2608;

	/* set buffer again */
	for ( i = 0 ; i < FMNumChips; i++ ) {
		if( YM2608SetBuffer(i,&buffer[i*FM_NUMBUF]) < 0 ){
		    for ( j = 0 ; j < i ; j ++ ){
				_YM2608FreeChip(j);
			}
			return(-1);
		}
		YM2608ResetChip(i);
	}
	return 0;
}

/* ---------- shut down emurator ----------- */
void YM2608Shutdown()
{
    int i;

    if (!FM2608) return;

    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_YM2608FreeChip(i);
	}
	free(FM2608); FM2608 = NULL;
	YM2203Shutdown();
}

/* ---------- reset one of chip ---------- */
void YM2608ResetChip(int num)
{
    int i;
    YM2203 *OPN = &(FMOPN[num]);
    YM2608 *F2608 = &(FM2608[num]);

	/* Reset Priscaler */
	OPNSetPris( num , 3 , 2); /* 1/3 , 1/2 */
	/* reset SSG section */
	SSGReset(num);
	/* status clear */
	OPNWriteMode(num,0x27,0x30,0); /* mode 0 , timer reset */
	reset_channel( &OPN->ST , OPN->CH , 3 );
	reset_channel( &OPN->ST , F2608->CH , 3 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(num,i      ,0xc0,0);
		OPNWriteReg(num,i|0x100,0xc0,F2608->CH);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(num,i      ,0,0);
		OPNWriteReg(num,i|0x100,0,F2608->CH);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(num,i,0,0);
	/* reset ADPCM unit */
	/* reset RITHM unit */
}

/* YM2608 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2608Write(int n, int a,int v)
{
	YM2203 *F2203 = &(FMOPN[n]);
	YM2608 *F2608 = &(FM2608[n]);
	int addr;
#ifdef SELF_UPDATE
	int pos;
#endif
	switch( a&3){
	case 0:	/* address port 0 */
		F2203->ST.address = v & 0xff;
		/* Write register to SSG emurator */
		if( v < 16 ) SSGWrite(n,0,v);
		break;
	case 1:	/* data port 0    */
		addr = F2203->ST.address;
		switch(addr & 0xf0)
		{
		case 0x00:	/* SSG section */
			/* Write data to SSG emurator */
			SSGWrite(n,a,v);
			break;
		case 0x10:	/* Rhythm section */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2608UpdateOne(n,pos);
#endif
			YM2608RhythmWrite(n,F2608->address1,v);
			break;
		case 0x20:	/* Mode Register */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2608UpdateOne(n,pos);
#endif
			 OPNWriteMode(n,addr,v,F2608->CH);
		default:	/* OPN section */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2608UpdateOne(n,pos);
#endif
			/* write register */
			 OPNWriteReg(n,addr,v,0);
		}
		break;
	case 2:	/* address port 1 */
		FM2608[n].address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2608->address1;
#ifdef SELF_UPDATE
		/* update chip */
		pos = FMUpdatePos();
		if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2608UpdateOne(n,pos);
#endif
		switch( addr & 0xf0 )
		{
		case 0x00:	/* ADPCM PORT */
			YM2608ADPCMWrite(n,addr,v);
			break;
		default:
#ifdef FM_DEBUG
			if( addr == 0x28 ) OPNWriteMode(n,addr,v|0x04,F2608->CH);
			else
#endif
			OPNWriteReg(n,addr|0x100,v,F2608->CH);
		}
	}
	return F2203->ST.irq;
}
unsigned char YM2608Read(int n,int a)
{
	YM2203 *F2203 = &(FMOPN[n]);
	int addr = F2203->ST.address;
	int ret = 0;

	switch( a&3){
	case 0:	/* status 0 : YM2203 compatible */
		ret = F2203->ST.status & 0x83;
		break;
	case 1:	/* status 0 */
		if( addr < 16 ) ret = SSGRead(n,a);
		break;
	case 2:	/* status 1 : + ADPCM status */
		ret = F2203->ST.status;
		break;
	case 3:
		ret = 0;
		break;
	}
	return ret;
}

int YM2608TimerOver(int n,int c)
{
	YM2203 *F2203 = &(FMOPN[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( n,&(F2203->ST) );
	}
	else
	{	/* Timer A */
#ifdef SELF_UPDATE
		/* update the system */
		int pos = FMUpdatePos();
		if( F2203->ST.bufp < pos ) YM2608UpdateOne(n, pos );
#endif
		/* timer update */
		TimerAOver( n,&(F2203->ST) );
		/* CSM mode key,TL controll */
		if( F2203->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->ST.irq;
}

/* ---------- return the buffer ---------- */
FMSAMPLE *YM2608Buffer(int n)
{
    return FMOPN[n].Buf[0];
}
/* ---------- set buffer ---------- */
int YM2608SetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < FM_NUMBUF ; i++){
		if( buf[i] == 0 ) return -1;
	    FMOPN[n].Buf[i] = buf[i];
		if( cur_chip == &FM2608[n] ) cur_chip = NULL;
	}
	return 0;
}

#endif /* BUILD_YM2608 */

#ifdef BUILD_YM2612
/*******************************************************************************/
/*		YM2612 local section                                                   */
/*******************************************************************************/
static void _YM2612FreeChip(int num)
{
	int i;
	for( i = 0 ; i < FM_NUMBUF ; i++)
	    FMOPN[num].Buf[i] = NULL;
}

/* ---------- update one of chip ----------- */
void YM2612UpdateOne(int num, int endp)
{
	YM2203 *OPN  = &(FMOPN[num]);
	YM2612 *F2612 = &(FM2612[num]);
	int dataR,dataL;
	int i;
	int dacen = F2612->dacen;
	int dacout  = F2612->dacout;

	if( endp > FMBufSize ) endp = FMBufSize;

	if( (void *)F2612 != cur_chip ){
		cur_chip = (void *)F2612;

		/* set bufer */
		bufL = OPN->Buf[0];
		bufR = OPN->Buf[1];

		State = &OPN->ST;
		chA   = &OPN->CH[0];
		chB   = &OPN->CH[1];
		chC   = &OPN->CH[2];
		chD   = &F2612->CH[0];
		chE   = &F2612->CH[1];
		chF   = &F2612->CH[2];
		pan[0] = &outd[chA->PAN];
		pan[1] = &outd[chB->PAN];
		pan[2] = &outd[chC->PAN];
		pan[3] = &outd[chD->PAN];
		pan[4] = &outd[chE->PAN];
		pan[5] = &outd[chF->PAN];
	}
	/* update frequency counter */
	CALC_FCOUNT( chA );
	CALC_FCOUNT( chB );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( chC->Incr[SLOT1]==-1){
			/* 3 slot mode */
			CALC_FCSLOT(chC , SLOT1 , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(chC , SLOT2 , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(chC , SLOT3 , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(chC , SLOT4 , chC->fc , chC->kcode );
		}
	}else CALC_FCOUNT( chC );
	CALC_FCOUNT( chD );
	CALC_FCOUNT( chE );
	CALC_FCOUNT( chF );
	/* buffering */
    for( i=State->bufp; i < endp ; ++i )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/* calcrate channel output */
		*pan[0] += FM_CALC_CH( chA );
		*pan[1] += FM_CALC_CH( chB );
		*pan[2] += FM_CALC_CH( chC );
		*pan[3] += FM_CALC_CH( chD );
		*pan[4] += FM_CALC_CH( chE );
		*pan[5] += dacen ?  dacout : FM_CALC_CH( chF );
		/* get left & right output */
		dataL = outd[OPN_CENTER] + outd[OPN_LEFT];
		dataR = outd[OPN_CENTER] + outd[OPN_RIGHT];
		/* clipping data */
		if( dataL > OPN_MAXOUT ) dataL = OPN_MAXOUT;
		else if( dataL < OPN_MINOUT ) dataL = OPN_MINOUT;
		if( dataR > OPN_MAXOUT ) dataR = OPN_MAXOUT;
		else if( dataR < OPN_MINOUT ) dataR = OPN_MINOUT;
		/* buffering */
		if( sample_16bit )
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned long *)bufL)[i] = ((dataL>>OPN_OUTSB)<<16)(dataR>>OPN_OUTSB);
#else
			/* stereo separate */
			((unsigned short *)bufL)[i] = dataL>>OPN_OUTSB;
			((unsigned short *)bufR)[i] = dataR>>OPN_OUTSB;
#endif
		}
		else
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned shart *)bufL)[i] = ((dataL>>OPN_OUTSB_8)<<8)(dataR>>OPN_OUTSB_8);
#else
			/* stereo separate */
			((unsigned char *)bufL)[i] = dataL>>OPN_OUTSB_8;
			((unsigned char *)bufR)[i] = dataR>>OPN_OUTSB_8;
#endif
		}

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , chC );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void YM2612Update(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPN[i].ST.bufp <  FMBufSize ) YM2612UpdateOne(i , FMBufSize );
		FMOPN[i].ST.bufp = 0;
    }
}

/* -------------------------- YM2608(OPNA) ---------------------------------- */
int YM2612Init(int num, int clock, int rate,  int bitsize, int bufsiz , FMSAMPLE **buffer)
{
	int i,j;

    if (FM2612) return (-1);	/* duplicate init. */

	if( YM2203Init(num,clock,rate,bitsize,bufsiz ,buffer ) != 0)
		return (-1);
	/* allocate extend state space */
	if( (FM2612 = (YM2612 *)malloc(sizeof(YM2612) * FMNumChips))==NULL)
	{
		YM2203Shutdown();
		return (-1);
	}
	ChipMode = CHIP_YM2612;

	/* set buffer again */
	for ( i = 0 ; i < FMNumChips; i++ ) {
		if( YM2612SetBuffer(i,&buffer[i*FM_NUMBUF]) < 0 ){
		    for ( j = 0 ; j < i ; j ++ ){
				_YM2612FreeChip(j);
			}
			return(-1);
		}
		YM2612ResetChip(i);
	}
	return 0;
}

/* ---------- shut down emurator ----------- */
void YM2612Shutdown()
{
    int i;

    if (!FM2612) return;

    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_YM2612FreeChip(i);
	}
	free(FM2612); FM2612 = NULL;
	YM2203Shutdown();
}

/* ---------- reset one of chip ---------- */
void YM2612ResetChip(int num)
{
    int i;
    YM2203 *OPN = &(FMOPN[num]);
    YM2612 *F2612 = &(FM2612[num]);

	F2612->dacen = 0;
	OPNSetPris( num , 12 , 0);
	OPNWriteMode(num,0x27,0x30,0); /* mode 0 , timer reset */
	reset_channel( &OPN->ST , OPN->CH , 3 );
	reset_channel( &OPN->ST , &F2612->CH[0] , 3 );

	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(num,i      ,0xc0,0);
		OPNWriteReg(num,i|0x100,0xc0,F2612->CH);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(num,i      ,0,0);
		OPNWriteReg(num,i|0x100,0,F2612->CH);
	}
}

/* YM2612 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2612Write(int n, int a,int v)
{
	YM2203 *F2203 = &(FMOPN[n]);
	YM2612 *F2612 = &(FM2612[n]);
	int addr;
#ifdef SELF_UPDATE
	int pos;
#endif
	switch( a&3){
	case 0:	/* address port 0 */
		F2203->ST.address = v & 0xff;
		break;
	case 1:	/* data port 0    */
		addr = F2203->ST.address;
		switch( addr & 0xf0 )
		{
		case 0x20:	/* 0x20-0x2f Mode */
			switch( addr )
			{
			case 0x2a:	/* DAC data (YM2612) */
#ifdef SELF_UPDATE
				/* update chip */
				pos = FMUpdatePos();
				if( F2203->ST.bufp <= pos ) YM2612UpdateOne(n,pos);
#endif
				F2612->dacout = v<<(TL_BITS-8);
			case 0x2b:	/* DAC Sel  (YM2612) */
				/* b7 = dac enable */
				F2612->dacen = v & 0x80;
				break;
			case 0x2d:
			case 0x2e:
			case 0x2f:
				break;
			default:	/* OPN section */
#ifdef SELF_UPDATE
				/* update chip */
				pos = FMUpdatePos();
				if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2612UpdateOne(n,pos);
#endif
				/* write register */
				 OPNWriteMode(n,addr,v,F2612->CH);
			}
			break;
		default:	/* 0x30-0xff OPN section */
#ifdef SELF_UPDATE
			/* update chip */
			pos = FMUpdatePos();
			if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2612UpdateOne(n,pos);
#endif
			/* write register */
			 OPNWriteReg(n,addr,v,F2612->CH);
		}
		break;
	case 2:	/* address port 1 */
		F2612->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2612->address1;
#ifdef SELF_UPDATE
		/* update chip */
		pos = FMUpdatePos();
		if( F2203->ST.bufp+MIN_UPDATE <= pos ) YM2612UpdateOne(n,pos);
#endif
#ifdef FM_DEBUG
		if( addr == 0x28 ) OPNWriteMode(n,addr,v|0x04,F2612->CH);
		else
#endif
		OPNWriteReg(n,addr|0x100,v,F2612->CH);
		break;
	}
	return F2203->ST.irq;
}
unsigned char YM2612Read(int n,int a)
{
	YM2203 *F2203 = &(FMOPN[n]);
	int addr = F2203->ST.address;

	switch( a&3){
	case 0:	/* status 0 */
	case 2:
		return F2203->ST.status;
	}
	return 0;
}

int YM2612TimerOver(int n,int c)
{
	YM2203 *F2203 = &(FMOPN[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( n,&(F2203->ST) );
	}
	else
	{	/* Timer A */
#ifdef SELF_UPDATE
		/* update the system */
		int pos = FMUpdatePos();
		if( F2203->ST.bufp < pos ) YM2612UpdateOne(n, pos );
#endif
		/* timer update */
		TimerAOver( n,&(F2203->ST) );
		/* CSM mode key,TL controll */
		if( F2203->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->ST.irq;
}

/* ---------- set buffer ---------- */
int YM2612SetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < FM_NUMBUF ; i++){
		if( buf[i] == 0 ) return -1;
	    FMOPN[n].Buf[i] = buf[i];
		if( cur_chip == &FM2612[n] ) cur_chip = NULL;
	}
	return 0;
}
#endif /* BUILD_YM2612 */






#ifdef BUILD_YM2151
/*******************************************************************************/
/*		YM2151 local section                                                   */
/*******************************************************************************/
/* -------------------------- OPM ---------------------------------- */
#undef SEG_SUPPORT		/* OPM has not SEG type envelope */

/* ---------- priscaler set(and make time tables) ---------- */
void OPMInitTable( int num )
{
    YM2151 *OPM = &(FMOPM[num]);
	int i;
	float pom;
	float rate = (float)(1<<FREQ_BITS) / (3579545.0 / FMOPM[num].ST.clock * FMOPM[num].ST.rate);

	for (i=0; i<8*12*64+950; i++)
	{
		/* This calculation type was used from the Jarek's YM2151 emulator */
		pom = 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so D#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		OPM->KC_TABLE[i] = (unsigned int)(pom * rate);
		/*if(errorlog) fprintf(errorlog,"OPM KC %d = %x\n",i,OPM->KC_TABLE[i]);*/
	}

	/* make time tables */
	init_timetables( &OPM->ST , OPM_DTTABLE , OPM_ARRATE , OPM_DRRATE );
}

/* ---------- reset one of chip ---------- */
void OPMResetChip(int num)
{
	int i;
    YM2151 *OPM = &(FMOPM[num]);

	OPMInitTable( num );
	reset_channel( &OPM->ST , &OPM->CH[0] , 8 );
	/* status clear */
	OPMWriteReg(num,0x1b,0x00);
	/* reset OPerator paramater */
	for(i = 0xff ; i >= 0x20 ; i-- ) OPMWriteReg(num,i,0);
}
/* ---------- release storage for a chip ---------- */
static void _OPMFreeChip(int num)
{
	int i;

	for( i = 0 ; i < FM_NUMBUF ; i++){
	    FMOPM[num].Buf[i] = NULL;
	}
}

/* ----------  Initialize YM2151 emulator(s) ----------    */
/* 'num' is the number of virtual YM2151's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPMInit(int num, int clock, int rate, int bitsize, int bufsiz , FMSAMPLE **buffer )
{
    int i,j;

    if (FMOPM) return (-1);	/* duplicate init. */

    FMNumChips = num;
    FMBufSize = bufsiz;
    if( bitsize == 16 ) sample_16bit = 1;
    else                sample_16bit = 0;

	/* allocate ym2151 state space */
	if( (FMOPM = (YM2151 *)malloc(sizeof(YM2151) * FMNumChips))==NULL)
		return (-1);
	/* allocate total lebel table (128kb space) */
	if( !FMInitTable() )
	{
		free( FMOPM );
		return (-1);
	}
	for ( i = 0 ; i < FMNumChips; i++ ) {
		FMOPM[i].ST.clock = clock;
		FMOPM[i].ST.rate = rate;
		if( rate ) FMOPM[i].ST.freqbase = ((float)clock * 4096.0 / rate) / 64;
		else       FMOPM[i].ST.freqbase = 0;
		for ( j = 0 ; j < FM_NUMBUF ; j ++ ) {
			FMOPM[i].Buf[j] = 0;
		}
		if( OPMSetBuffer(i,&buffer[i*FM_NUMBUF]) < 0 ){
		    for ( j = 0 ; j < i ; j ++ ) {
				_OPMFreeChip(j);
		    }
		    return(-1);
		}
		FMOPM[i].ST.handler = 0;
		OPMResetChip(i);
    }
    return(0);
}

/* ---------- shut down emurator ----------- */
void OPMShutdown()
{
    int i;

    if (!FMOPM) return;
    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_OPMFreeChip(i);
	}
	free(FMOPM); FMOPM = NULL;
	FMCloseTable();
	FMBufSize = 0;
}
/* ---------- write a register on YM2151 chip number 'n' ---------- */
void OPMWriteReg(int n, int r, int v)
{
	unsigned char c,s;
	FM_CH *CH;

    YM2151 *OPM = &(FMOPM[n]);

	c  = OPM_CHAN(r);
	CH = &OPM->CH[c];
	s  = OPM_SLOT(r);

	switch( r & 0xe0 ){
	case 0x00: /* 0x00-0x1f */
		switch( r ){
		case 0x01:	/* test */
			break;
		case 0x08:	/* key on / off */
			c = v&7;
			/* CSM mode */
			if( c == 7 && (OPM->ST.mode & 0x80) ) break;
			CH = &OPM->CH[c];
			if(v&0x08) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
			if(v&0x10) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
			if(v&0x20) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
			if(v&0x40) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
			break;
		case 0x0f:	/* Noise freq (ch7.op4) */
			/* b7 = Noise enable */
			/* b0-4 noise freq  */
			if( v & 0x80 ){
				/* !!!!! do not supported noise mode  !!!!! */
				if(errorlog) fprintf(errorlog,"OPM Noise mode sel ( not supported )\n");
			}
			OPM->NReg = v & 0x8f;
			break;
		case 0x10:	/* timer A High 8*/
			OPM->ST.TA = (OPM->ST.TA & 0x03)|(((int)v)<<2);
			break;
		case 0x11:	/* timer A Low 2*/
			OPM->ST.TA = (OPM->ST.TA & 0x3fc)|(v&3);
			break;
		case 0x12:	/* timer B */
			OPM->ST.TB = v;
			break;
		case 0x14:	/* mode , timer controll */
			FMSetMode( &(OPM->ST),n,v );
			break;
		case 0x18:	/* lfreq   */
			/* !!!!! pickup lfo frequency table !!!!! */
			break;
		case 0x19:	/* PMD/AMD */
			if( v & 0x80 ) OPM->pmd = v & 0x7f;
			else           OPM->amd = v & 0x7f;
			break;
		case 0x1b:	/* CT , W  */
			/* b7 = CT */
			/* b6 = CT */
			/* b0-3 = wave form(LFO) 0=nokogiri,1=houkei,2=sankaku,3=noise */
			break;
		}
		break;
	case 0x20:	/* 20-3f */
		switch( s ){
		case 0: /* 0x20-0x27 : RL,FB,CON */
			set_algorythm( CH , v );
			/* b0 = left , b1 = right */
			CH->PAN = ((v>>6)&0x03);
			if( cur_chip == OPM ) pan[c] = &outd[CH->PAN];
			break;
		case 1: /* 0x28-0x2f : Keycode */
			{
				int blk = (v>>4)&7;
				/* make keyscale code */
				CH->kcode = (v>>2)&0x1f;
				/* make basic increment counter 22bit = 1 cycle */
				CH->fc = (blk * (12*64)) + KC_TO_SEMITONE[v&0x0f] + CH->fn_h;
				CH->Incr[SLOT1]=-1;
			}
			break;
		case 2: /* 0x30-0x37 : Keyfunction */
			CH->fc -= CH->fn_h;
			CH->fn_h = v>>2;
			CH->fc += CH->fn_h;
			CH->Incr[SLOT1]=-1;
			break;
		case 3: /* 0x38-0x3f : PMS / AMS */
			/* b0-1 AMS */
			/* AMS * 23.90625db */
			CH->ams[SLOT1] = v & 0x03;
			CH->ams[SLOT2] = v & 0x03;
			CH->ams[SLOT3] = v & 0x03;
			CH->ams[SLOT4] = v & 0x03;
			/* b4-6 PMS */
			/* 0,5,10,20,50,100,400,700 (cent) */
			CH->pms[SLOT1] = (v>>4) & 0x07;
			CH->pms[SLOT2] = (v>>4) & 0x07;
			CH->pms[SLOT3] = (v>>4) & 0x07;
			CH->pms[SLOT4] = (v>>4) & 0x07;
			break;
		}
		break;
	case 0x40:	/* DT1,MUL */
		CH->mul[s] = MUL_TABLE[v&0x0f];
		CH->DT[s]  = OPM->ST.DT_TABLE[(v>>4)&7];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x60:	/* TL */
		v &= 0x7f;
		v = (v<<7)|v; /* 7bit -> 14bit */
		CH->TL[s] = (v*EG_ENT)>>14;
		/* CSM mode */
		if( c == 7 && (OPM->ST.mode & 0x80) ) break;	/* CSM MODE */
		CH->TLL[s] = CH->TL[s] + KSL[CH->kcode];
		break;
	case 0x80:	/* KS, AR */
		CH->KSR[s] = 3-(v>>6);
		CH->AR[s] = (v&=0x1f) ? &OPM->ST.AR_TABLE[v<<1] : RATE_0;
		CH->evsa[s] = CH->AR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0xa0:	/* AMS EN,D1R */
		/* bit7 = AMS ENABLE */
		CH->DR[s] = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		CH->evsd[s] = CH->DR[s][CH->ksr[s]];
		break;
	case 0xc0:	/* DT2 ,D2R */
		CH->DT2[s]  = DT2_TABLE[v>>6];
		CH->SR[s]  = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		CH->evss[s] = CH->SR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0xe0:	/* D1L, RR */
		CH->SL[s] = SL_TABLE[(v>>4)];
		/* recalrate */
		CH->RR[s] = (v&=0x0f) ? &OPM->ST.DR_TABLE[(v<<2)|2] : RATE_0 ;
		CH->evsr[s] = CH->RR[s][CH->ksr[s]];
		break;
    }
}

/* ---------- read status port ---------- */
unsigned char OPMReadStatus(int n)
{
	return FMOPM[n].ST.status;
}

int YM2151Write(int n,int a,int v)
{
	YM2151 *F2151 = &(FMOPM[n]);
#ifdef SELF_UPDATE
	int pos;
#endif

	if( !(a&1) )
	{	/* address port */
		F2151->ST.address = v & 0xff;
	}
	else
	{	/* data port */
		int addr = F2151->ST.address;
#ifdef SELF_UPDATE
		/* update chip */
		pos = FMUpdatePos();
		if( F2151->ST.bufp+MIN_UPDATE <= pos ) OPMUpdateOne(n,pos);
#endif
		/* write register */
		 OPMWriteReg(n,addr,v);
	}
	return F2151->ST.irq;
}

unsigned char YM2151Read(int n,int a)
{
	if( !(a&1) ) return 0;
	else         return FMOPM[n].ST.status;
}

/* ---------- make digital sound data ---------- */
void OPMUpdateOne(int num , int endp)
{
	YM2151 *OPM = &(FMOPM[num]);
	int i;
	int dataR,dataL;
	if( (void *)OPM != cur_chip ){
		cur_chip = (void *)OPM;

		State = &OPM->ST;
		/* set bufer */
		bufL = OPM->Buf[0];
		bufR = OPM->Buf[1];
		/* channel pointer */
		chA = &OPM->CH[0];
		chB = &OPM->CH[1];
		chC = &OPM->CH[2];
		chD = &OPM->CH[3];
		chE = &OPM->CH[4];
		chF = &OPM->CH[5];
		chG = &OPM->CH[6];
		chH = &OPM->CH[7];
		pan[0] = &outd[chA->PAN];
		pan[1] = &outd[chB->PAN];
		pan[2] = &outd[chC->PAN];
		pan[3] = &outd[chD->PAN];
		pan[4] = &outd[chE->PAN];
		pan[5] = &outd[chF->PAN];
		pan[6] = &outd[chG->PAN];
		pan[7] = &outd[chH->PAN];
	}
	OPM_CALC_FCOUNT( OPM , chA );
	OPM_CALC_FCOUNT( OPM , chB );
	OPM_CALC_FCOUNT( OPM , chC );
	OPM_CALC_FCOUNT( OPM , chD );
	OPM_CALC_FCOUNT( OPM , chE );
	OPM_CALC_FCOUNT( OPM , chF );
	OPM_CALC_FCOUNT( OPM , chG );
	/* CSM check */
	OPM_CALC_FCOUNT( OPM , chH );

    for( i=State->bufp; i < endp ; ++i )
	{
		/* clear output acc. */
		outd[OPM_LEFT] = outd[OPM_RIGHT]= outd[OPM_CENTER] = 0;
		/* calcrate channel output */
		*pan[0] += FM_CALC_CH( chA );
		*pan[1] += FM_CALC_CH( chB );
		*pan[2] += FM_CALC_CH( chC );
		*pan[3] += FM_CALC_CH( chD );
		*pan[4] += FM_CALC_CH( chE );
		*pan[5] += FM_CALC_CH( chF );
		*pan[6] += FM_CALC_CH( chG );
		*pan[7] += FM_CALC_CH( chH );
		/* get left & right output */
		dataL = outd[OPM_CENTER] + outd[OPM_LEFT];
		dataR = outd[OPM_CENTER] + outd[OPM_RIGHT];
		/* clipping data */
		if( dataL > OPM_MAXOUT ) dataL = OPM_MAXOUT;
		else if( dataL < OPM_MINOUT ) dataL = OPM_MINOUT;
		if( dataR > OPM_MAXOUT ) dataR = OPM_MAXOUT;
		else if( dataR < OPM_MINOUT ) dataR = OPM_MINOUT;

		if( sample_16bit )
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned long *)bufL)[i] = ((dataL>>OPM_OUTSB)<<16)(dataR>>OPM_OUTSB);
#else
			/* stereo separate */
			((unsigned short *)bufL)[i] = dataL>>OPM_OUTSB;
			((unsigned short *)bufR)[i] = dataR>>OPM_OUTSB;
#endif
		}
		else
		{
#ifdef FM_STEREO_MIX		/* stereo mixing */
			/* stereo mix */
			((unsigned shart *)bufL)[i] = ((dataL>>OPM_OUTSB_8)<<8)(dataR>>OPM_OUTSB_8);
#else
			/* stereo separate */
			((unsigned char *)bufL)[i] = dataL>>OPM_OUTSB_8;
			((unsigned char *)bufR)[i] = dataR>>OPM_OUTSB_8;
#endif
		}
#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif

#ifdef INTERNAL_TIMER
		CALC_TIMER_A( State , chH );
#endif
    }
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void OPMUpdate(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPM[i].ST.bufp <  FMBufSize ) OPMUpdateOne(i , FMBufSize );
		FMOPM[i].ST.bufp = 0;
    }
}

/* ---------- return the buffer ---------- */
FMSAMPLE *OPMBuffer(int n,int c)
{
    return FMOPM[n].Buf[c];
}
/* ---------- set buffer ---------- */
int OPMSetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < FM_NUMBUF ; i++){
		if( buf[i] == 0 ) return -1;
	    FMOPM[n].Buf[i] = buf[i];
		if( cur_chip == &FMOPM[n] ) cur_chip = NULL;
	}
	return 0;
}


int YM2151TimerOver(int n,int c)
{
	YM2151 *F2151 = &(FMOPM[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( n,&(F2151->ST) );
	}
	else
	{	/* Timer A */
#ifdef SELF_UPDATE
		/* update the system */
		int pos = FMUpdatePos();
		if( F2151->ST.bufp < pos ) OPMUpdateOne(n, pos);
#endif
		/* timer update */
		TimerAOver( n,&(F2151->ST) );
		/* CSM mode key,TL controll */
		if( F2151->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2151->CH[7]) );
		}
	}
	return F2151->ST.irq;
}

#endif /* BUILD_YM2151 */
