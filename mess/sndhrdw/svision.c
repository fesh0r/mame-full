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

#include "includes/svision.h"

static int mixer_channel_left, mixer_channel_right;

SVISION_CHANNEL svision_channel[2];

void svision_soundport_w (SVISION_CHANNEL *channel, int offset, int data)
{
    stream_update(mixer_channel_left,0);
    stream_update(mixer_channel_right,0);
    logerror("%.6f channel 1 write %d %02x\n", timer_get_time(),offset&3, data);
    channel->reg[offset]=data;
    switch (offset) {
    case 0:
    case 1:
	if (channel->reg[0]) {
	    if (channel==svision_channel) 
		channel->size=(int)((options.samplerate*channel->reg[0]<<6)/4e6);
	    else
		channel->size=(int)((options.samplerate*channel->reg[0]<<6)/4e6);
	} else channel->size=0;
	channel->l_pos=0;
	channel->r_pos=0;
    }
    
}

/************************************/
/* Sound handler update             */
/************************************/
void svision_update (int param, INT16 *buffer, int length)
{
    int i, j;
    SVISION_CHANNEL *channel;
    
    for (i = 0; i < length; i++, buffer++)
    {
	*buffer = 0;
	for (channel=svision_channel, j=0; j<ARRAY_LENGTH(svision_channel); j++, channel++) {
	    if (!param) { //left
		if (channel->reg[2]&0x40) {
		    if (channel->l_pos<=channel->size/2)
			*buffer+=(channel->reg[2]&0xf)<<8;
		    if (++channel->l_pos>=channel->size) channel->l_pos=0;
		}
	    } else { 
		if (channel->reg[2]&0x20) {
		    if (channel->r_pos<=channel->size/2)
			*buffer+=(channel->reg[2]&0xf)<<8;
		    if (++channel->r_pos>=channel->size) channel->r_pos=0;
		}
	    }
	}
    }
}

/************************************/
/* Sound handler start              */
/************************************/
int svision_custom_start (const struct MachineSound *driver)
{
	if (!options.samplerate) return 0;

	mixer_channel_left = stream_init ("supervision", MIXER(50,MIXER_PAN_LEFT), options.samplerate, 0, svision_update);
	mixer_channel_right = stream_init ("supervision", MIXER(50,MIXER_PAN_RIGHT), options.samplerate, 1, svision_update);

	return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void svision_custom_stop (void)
{
}

void svision_custom_update (void)
{
}
