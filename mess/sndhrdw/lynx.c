/***************************************************************************

  MOS ted 7360 (and sound interface)

  main part in vidhrdw
***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"
#include "timer.h"
#include "sound/mixer.h"

#include "includes/lynx.h"

/*
AUDIO_A	EQU $FD20
AUDIO_B	EQU $FD28
AUDIO_C	EQU $FD30
AUDIO_D	EQU $FD38

VOLUME_CNTRL	EQU 0
FEEDBACK_ENABLE EQU 1	; enables 11/10/5..0
OUTPUT_VALUE	EQU 2
SHIFTER_L	EQU 3
AUD_BAKUP	EQU 4
AUD_CNTRL1	EQU 5
AUD_COUNT	EQU 6
AUD_CNTRL2	EQU 7

; AUD_CNTRL1
FEEDBACK_7	EQU %10000000
AUD_RESETDONE	EQU %01000000
AUD_INTEGRATE	EQU %00100000
AUD_RELOAD	EQU %00010000
AUD_CNTEN	EQU %00001000
AUD_LINK	EQU %00000111	
; link timers (0->2->4 / 1->3->5->7->Aud0->Aud1->Aud2->Aud3->1
AUD_64us	EQU %00000110
AUD_32us	EQU %00000101
AUD_16us	EQU %00000100
AUD_8us	EQU %00000011
AUD_4us	EQU %00000010
AUD_2us	EQU %00000001
AUD_1us	EQU %00000000

; AUD_CNTRL2 (read only)
; B7..B4	; shifter bits 11..8
; B3	; who knows
; B2	; last clock state (0->1 causes count)
; B1	; borrow in (1 causes count)
; B0	; borrow out (count EQU 0 and borrow in)

ATTEN_A	EQU $FD40
ATTEN_B	EQU $FD41
ATTEN_C	EQU $FD42
ATTEN_D	EQU $FD43
; B7..B4 attenuation left ear (0 silent ..15/16 volume)
; B3..B0       "     right ear

MPAN	EQU $FD44
; B7..B4 left ear
; B3..B0 right ear
; B7/B3 EQU Audio D
; a 1 enables attenuation for channel and side


MSTEREO	EQU $FD50	; a 1 disables audio connection
AUD_D_LEFT	EQU %10000000
AUD_C_LEFT	EQU %01000000
AUD_B_LEFT	EQU %00100000
AUD_A_LEFT	EQU %00010000
AUD_D_RIGHT	EQU %00001000
AUD_C_RIGHT	EQU %00000100
AUD_B_RIGHT	EQU %00000010
AUD_A_RIGHT	EQU %00000001

 */
static int mixer_channel;
static int usec_per_sample;

typedef struct {
    int nr;
    union {
	UINT8 data[8];
	struct {
	    UINT8 volume;
	    UINT8 feedback;
	    INT8 output;
	    UINT8 shifter;
	    UINT8 bakup;
	    UINT8 control1;
	    UINT8 counter;
	    UINT8 control2;
	} n;
    } reg;
    UINT8 attenuation;
    int ticks;
    int count;
} LYNX_AUDIO;
static LYNX_AUDIO lynx_audio[4]= { 
	{ 0 },
	{ 1 },
	{ 2 },
	{ 3 } 
};

void lynx_audio_debug(struct osd_bitmap *bitmap)
{
    char str[40];
    sprintf(str,"%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x",
	    lynx_audio[0].reg.data[0],
	    lynx_audio[0].reg.data[1],
	    lynx_audio[0].reg.data[2],
	    lynx_audio[0].reg.data[3],
	    lynx_audio[0].reg.data[4],
	    lynx_audio[0].reg.data[5],
	    lynx_audio[0].reg.data[6],
	    lynx_audio[0].reg.data[7]);

//    ui_text(bitmap, str, 0,0);
}

static void lynx_audio_execute(LYNX_AUDIO *channel)
{
    if (channel->reg.n.control1&8) { // count_enable
	channel->ticks+=usec_per_sample;
	if ((channel->reg.n.control1&7)==7) { // timer input
	} else {
	    int t=1<<(channel->reg.n.control1&7);
	    for (;;) {
		for (;(channel->ticks>=t)&&channel->count<=channel->reg.n.counter/2; channel->ticks-=t)
		    channel->count++;
		if (channel->ticks<t) break;
		for (;(channel->ticks>=t)&&channel->count<=channel->reg.n.counter; channel->ticks-=t)
		    channel->count++;
		if (channel->count>=channel->reg.n.counter) channel->count=0;
		if (channel->ticks<t) break;
	    }
	    channel->reg.n.output=channel->count<channel->reg.n.counter/2?channel->reg.n.volume:0;
	}
    } else {
	channel->ticks=0;
	channel->count=0;
    }
}

static UINT8 attenuation_enable;
static UINT8 master_enable;

UINT8 lynx_audio_read(int offset)
{
    UINT8 data=0;
    stream_update(mixer_channel,0);
    switch (offset) {
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	data=lynx_audio[(offset>>3)&3].reg.data[offset&7];
	logerror("%.6f audio channel %d read %.2x %.2x\n", timer_get_time(), (offset>>3)&3, offset&7, data);
	break;
    case 0x40: case 0x41: case 0x42: case 0x43: 
	data=lynx_audio[offset&3].attenuation;
	logerror("%.6f audio read %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    case 0x44: 
	data=attenuation_enable;
	logerror("%.6f audio read %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    case 0x50:
	data=master_enable;
	logerror("%.6f audio read %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    }
    return data;
}

void lynx_audio_write(int offset, UINT8 data)
{
    stream_update(mixer_channel,0);
    switch (offset) {
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	lynx_audio[(offset>>3)&3].reg.data[offset&7]=data;
	logerror("%.6f audio channel %d write %.2x %.2x\n", timer_get_time(), (offset>>3)&3, offset&7, data);
	break;
    case 0x40: case 0x41: case 0x42: case 0x43: 
	lynx_audio[offset&3].attenuation=data;
	logerror("%.6f audio write %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    case 0x44: 
	attenuation_enable=data;
	logerror("%.6f audio write %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    case 0x50:
	master_enable=data;
	logerror("%.6f audio write %.2x %.2x\n", timer_get_time(), offset, data);
	break;
    }
}

/************************************/
/* Sound handler update             */
/************************************/
void lynx_update (int param, INT16 **buffer, int length)
{
    INT16 *left=buffer[0], *right=buffer[1];
    int i, j;
    LYNX_AUDIO *channel;
    int v;
    
    for (i = 0; i < length; i++, left++, right++)
    {
	*left = 0;
	*right= 0;
	for (channel=lynx_audio, j=0; j<ARRAY_LENGTH(lynx_audio); j++, channel++) {
	    lynx_audio_execute(channel);
	    v=channel->reg.n.output;
	    if (!(master_enable&(0x10<<j))) {		    
		if (attenuation_enable&(0x10<<j)) {
		    *left+=v*(channel->attenuation>>4);
		} else {
		    *left+=v*15;
		}
	    }
	    if (!(master_enable&(1<<j))) {
		if (attenuation_enable&(1<<j)) {
		    *right+=v*(channel->attenuation&0xf);
		} else {
		    *right+=v*15;
		}
	    }
	}
    }
}

/************************************/
/* Sound handler start              */
/************************************/
int lynx_custom_start (const struct MachineSound *driver)
{
    const int vol[2]={ MIXER(50, MIXER_PAN_LEFT), MIXER(50, MIXER_PAN_LEFT) };
    const char *names[2]= { "lynx", "lynx" };
	
    if (!options.samplerate) return 0;

    mixer_channel = stream_init_multi(2, names, vol, options.samplerate, 0, lynx_update);

    usec_per_sample=1000000/options.samplerate;
    
    return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void lynx_custom_stop (void)
{
}

void lynx_custom_update (void)
{
}
