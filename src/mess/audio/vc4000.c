/***************************************************************************

  PeT mess@utanet.at
  main part in video/

***************************************************************************/

#include "includes/vc4000.h"


typedef struct _vc4000_sound vc4000_sound;
struct _vc4000_sound
{
    sound_stream *channel;
    UINT8 reg[1];
    int size, pos;
    unsigned level;
};


static vc4000_sound *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == VC4000);
	return (vc4000_sound *) downcast<legacy_device_base *>(device)->token();
}


void vc4000_soundport_w (device_t *device, int offset, int data)
{
	vc4000_sound *token = get_token(device);

	token->channel->update();
	token->reg[offset] = data;
	switch (offset)
	{
		case 0:
			token->pos = 0;
			token->level = TRUE;
			// frequency 7874/(data+1)
			token->size = device->machine().sample_rate() * (data + 1) /7874;
			break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/

static STREAM_UPDATE( vc4000_update )
{
	int i;
	vc4000_sound *token = get_token(device);
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = 0;
		if (token->reg[0] && token->pos <= token->size / 2)
		{
			*buffer = 0x7fff;
		}
		if (token->pos <= token->size)
			token->pos++;
		if (token->pos > token->size)
			token->pos = 0;
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(vc4000_sound)
{
	vc4000_sound *token = get_token(device);
	memset(token, 0, sizeof(*token));
    token->channel = device->machine().sound().stream_alloc(*device, 0, 1, device->machine().sample_rate(), 0, vc4000_update);
}


DEVICE_GET_INFO( vc4000_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vc4000_sound);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vc4000_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "VC 4000 Custom");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}

DEFINE_LEGACY_SOUND_DEVICE(VC4000, vc4000_sound);
