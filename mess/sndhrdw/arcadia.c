/***************************************************************************

  main part in vidhrdw

***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#include "includes/arcadia.h"


#define VOLUME (arcadia_sound.reg[2]&0xf)
#define ON (!(arcadia_sound.reg[2]&0x10))
static struct {
    int channel;
    UINT8 reg[3];
    int size, pos;
    bool level;
} arcadia_sound= { 0 };

void arcadia_soundport_w (int offset, int data)
{
	stream_update(arcadia_sound.channel,0);
	arcadia_sound.reg[offset]=data;
	switch (offset)
	{
	case 1:
	    arcadia_sound.pos=0;
	    arcadia_sound.level=TRUE;
	    // frequency 7874/(data+1)
	    arcadia_sound.size=options.samplerate*(data+1)/7874;
	    break;
	}
}

/************************************/
/* Sound handler update             */
/************************************/
void arcadia_update (int param, INT16 *buffer, int length)
{
    int i;
    
    for (i = 0; i < length; i++, buffer++)
    {	
	*buffer = 0;
	if (arcadia_sound.reg[1]&&arcadia_sound.pos<=arcadia_sound.size/2) {
	    *buffer = 0x2ff*VOLUME; // depends on the volume between sound and noise
	}
	if (arcadia_sound.pos<=arcadia_sound.size) arcadia_sound.pos++;
	if (arcadia_sound.pos>arcadia_sound.size) arcadia_sound.pos=0;
    }
}

/************************************/
/* Sound handler start              */
/************************************/
int arcadia_custom_start (const struct MachineSound *driver)
{
    if (!options.samplerate) return 0;
    
    arcadia_sound.channel = stream_init ("Arcadia", 50, options.samplerate, 0, arcadia_update);
    
    return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void arcadia_custom_stop (void)
{
}

void arcadia_custom_update (void)
{
}

struct CustomSound_interface arcadia_sound_interface =
{
	arcadia_custom_start,
	arcadia_custom_stop,
	arcadia_custom_update
};
