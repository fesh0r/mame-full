/***************************************************************************

  PeT mess@utanet.at
  main part in vidhrdw

***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#include "includes/vc4000.h"

struct vc4000_sound
{
    sound_stream *stream;
    UINT8 reg[1];
    int size, pos;
    bool level;
};



void vc4000_soundport_w (int offset, int data)
{
	struct vc4000_sound *info;

	info = (struct vc4000_sound *) sndti_token(SOUND_CUSTOM, 0);
	stream_update(info->stream, 0);
	info->reg[offset] = data;

	switch (offset)
	{
		case 0:
			info->pos = 0;
			info->level = TRUE;
			// frequency 7874/(data+1)
			info->size = options.samplerate*(data+1)/7874;
			break;
	}
}

/************************************/
/* Sound handler update             */
/************************************/
static void vc4000_update (void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	int i;
	struct vc4000_sound *info;
	stream_sample_t *buffer = _buffer[0];

	info = (struct vc4000_sound *) param;

	for (i = 0; i < length; i++, buffer++)
	{
		*buffer = 0;
		if ((info->reg[0] != 0) && (info->pos <= info->size / 2))
		{
			*buffer = 0x7fff;
		}

		if (info->pos <= info->size)
			info->pos++;
		if (info->pos > info->size)
			info->pos = 0;
	}
}

/************************************/
/* Sound handler start              */
/************************************/

static void *vc4000_custom_start(int clock, const struct CustomSound_interface *config)
{
	struct vc4000_sound *info;

	info = (struct vc4000_sound *) auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));
	info->stream = stream_create(0, 1, options.samplerate, 0, vc4000_update);

    return (void *) info;
}



struct CustomSound_interface vc4000_sound_interface =
{
	vc4000_custom_start
};
