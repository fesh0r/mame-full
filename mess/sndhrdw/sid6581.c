/***************************************************************************

  MOS sound interface device sid6581

***************************************************************************/

/* uses Michael Schwendt's sidplay (copyright message in 6581_.cpp)
   problematic and much work to integrate, so better to redo bugfixes
   in the converted code

   now only 1 sid chip allowed!
   needs rework
*/

#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "6581_.h"
#include "mixing.h"
#include "includes/sid6581.h"

SID6581 sid6581[2]= {{0}};

static int channel;

static void sid6581_init (SID6581 *This, int (*paddle) (int offset), int pal)
{
	memset(This, 0, sizeof(SID6581));
	This->paddle_read = paddle;
	initMixerEngine();
	filterTableInit();
	if (pal)
		sidEmuSetReplayingSpeed(0, 50);
	else
		sidEmuSetReplayingSpeed(0, 60);
	sidEmuConfigure(options.samplerate, true, 0, true, !pal);
	MixerInit(0,0,0);
	sidEmuSetVoiceVolume(1,255,255,256);
	sidEmuSetVoiceVolume(2,255,255,256);
	sidEmuSetVoiceVolume(3,255,255,256);
	sidEmuSetVoiceVolume(4,255,255,256);
	sidEmuResetAutoPanning(0);
	sidEmuReset();
}

void sid6581_configure (SID6581 *This, SIDTYPE type)
{
	sidEmuConfigure(options.samplerate, true, type, true, 0);
}

static void sid6581_reset(SID6581 *This)
{
	sidEmuReset();
}

static void sid6581_port_w (SID6581 *This, int offset, int data)
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
	}
}

static int sid6581_port_r (SID6581 *This, int offset)
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
		if (This->paddle_read != NULL)
			data=This->paddle_read (0);
		else
			data=0;
		break;
	case 0x1a:						   /* paddle 2 */
		if (This->paddle_read != NULL)
			data=This->paddle_read (1);
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

UINT16 sid6581_read_word(SID6581 *This, int offset)
{
	return This->reg[offset&0x1f]|This->reg[(offset+1)&0x1f]<<8;
}

void sid6581_0_init (int (*paddle) (int offset), int pal)
{
	sid6581_init(sid6581, paddle, pal);
}

void sid6581_1_init (int (*paddle) (int offset), int pal)
{
	sid6581_init(sid6581+1, paddle, pal);
}

void sid6581_0_configure (SIDTYPE type)
{
	sid6581_configure(sid6581, type);
}

void sid6581_1_configure (SIDTYPE type)
{
	sid6581_configure(sid6581+1, type);
}

void sid6581_0_reset (void)
{
	sid6581_reset(sid6581);
}

void sid6581_1_reset (void)
{
	sid6581_reset(sid6581+1);
}

READ_HANDLER ( sid6581_0_port_r )
{
	return sid6581_port_r(sid6581, offset);
}

READ_HANDLER ( sid6581_1_port_r )
{
	return sid6581_port_r(sid6581+1, offset);
}

WRITE_HANDLER ( sid6581_0_port_w )
{
	sid6581_port_w(sid6581, offset, data);
}

WRITE_HANDLER ( sid6581_1_port_w )
{
	sid6581_port_w(sid6581+1, offset, data);
}

void sid6581_update(void)
{
	stream_update(channel,0);
}

void sid6581_sh_update(int param, INT16 *buffer, int length)
{
	sidEmuFillBuffer(buffer, length);
}

int sid6581_custom_start (const struct MachineSound *driver)
{
	channel = stream_init ("sid6581", 50, options.samplerate, 0, sid6581_sh_update);
	return 0;
}

void sid6581_custom_stop(void) {}
void sid6581_custom_update(void) {}

struct CustomSound_interface sid6581_sound_interface =
{
        sid6581_custom_start,
        sid6581_custom_stop,
        sid6581_custom_update
};


