/***************************************************************************

  main part in vidhrdw

***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#include "includes/vc4000.h"

static struct {
    int channel;
    UINT8 reg[1];
    int size, pos;
    bool level;
} vc4000_sound= { 0 };

void vc4000_soundport_w (int offset, int data)
{
	stream_update(vc4000_sound.channel,0);
	vc4000_sound.reg[offset]=data;
	switch (offset)
	{
	case 0:
	    vc4000_sound.pos=0;
	    vc4000_sound.level=TRUE;
	    // frequency 7874/(data+1)
	    vc4000_sound.size=options.samplerate*(data+1)/7874;
	    break;
	}
}

/************************************/
/* Sound handler update             */
/************************************/
void vc4000_update (int param, INT16 *buffer, int length)
{
    int i;

    for (i = 0; i < length; i++, buffer++)
    {
	*buffer = 0;
	if (vc4000_sound.reg[0]!=0 && vc4000_sound.pos<=vc4000_sound.size/2) {
	    *buffer = 0x7fff;
	}
	if (vc4000_sound.pos<=vc4000_sound.size) vc4000_sound.pos++;
	if (vc4000_sound.pos>vc4000_sound.size) vc4000_sound.pos=0;
    }
}

/************************************/
/* Sound handler start              */
/************************************/
int vc4000_custom_start (const struct MachineSound *driver)
{
    if (!options.samplerate) return 0;

    vc4000_sound.channel = stream_init ("VC4000", 50, options.samplerate, 0, vc4000_update);

    return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void vc4000_custom_stop (void)
{
}

void vc4000_custom_update (void)
{
}

struct CustomSound_interface vc4000_sound_interface =
{
	vc4000_custom_start,
	vc4000_custom_stop,
	vc4000_custom_update
};
