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
#include "mess/machine/cbm.h"

#include "6581_.h"
#include "mixing.h"
#include "sid6581.h"

SID6581 sid6581[2]= {{0}};

static int channel;

static void sid6581_init (SID6581 *this, int (*paddle) (int offset), int pal)
{
	memset(this, 0, sizeof(SID6581));
	this->paddle_read = paddle;
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

void sid6581_configure (SID6581 *this, SIDTYPE type)
{
	sidEmuConfigure(options.samplerate, true, type, true, 0);
}

static void sid6581_reset(SID6581 *this)
{
	sidEmuReset();
}

static void sid6581_port_w (SID6581 *this, int offset, int data)
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
		//stream_update(channel,0);
		this->reg[offset] = data;
		if (data&1)
			this->sidKeysOn[offset]=1;
		else
			this->sidKeysOff[offset]=1;
	}
}

static int sid6581_port_r (SID6581 *this, int offset)
{
/* SIDPLAY reads last written at a sid address value */
	offset &= 0x1f;
	switch (offset)
	{
	case 0x1d:
	case 0x1e:
	case 0x1f:
		return 0xff;
	case 0x19:						   /* paddle 1 */
		if (this->paddle_read != NULL)
			return this->paddle_read (0);
		return 0;
	case 0x1a:						   /* paddle 2 */
		if (this->paddle_read != NULL)
			return this->paddle_read (1);
		return 0;
#if 0
	case 0x1b:case 0x1c: /* noise channel readback? */
		return rand();
#endif
	default:
		return this->reg[offset];
	}
}

UINT16 sid6581_read_word(SID6581 *this, int offset)
{
	return this->reg[offset&0x1f]|this->reg[(offset+1)&0x1f]<<8;
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


