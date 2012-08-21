/***************************************************************************

  audio/special.c

  Functions to emulate sound hardware of Specialist MX
  ( based on code of DAI interface )

****************************************************************************/

#include "includes/special.h"

typedef struct _specimx_sound_state specimx_sound_state;
struct _specimx_sound_state
{
	sound_stream *mixer_channel;
	int specimx_input[3];
};

static STREAM_UPDATE( specimx_sh_update );

INLINE specimx_sound_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SPECIMX);
	return (specimx_sound_state *)downcast<legacy_device_base *>(device)->token();
}

static DEVICE_START(specimx_sound)
{
	specimx_sound_state *state = get_safe_token(device);
	state->specimx_input[0] = state->specimx_input[1] = state->specimx_input[2] = 0;
	state->mixer_channel = device->machine().sound().stream_alloc(*device, 0, 1, device->machine().sample_rate(), 0, specimx_sh_update);
}

static STREAM_UPDATE( specimx_sh_update )
{
	specimx_sound_state *state = get_safe_token(device);
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;

	stream_sample_t *sample_left = outputs[0];

	channel_0_signal = state->specimx_input[0] ? 3000 : -3000;
	channel_1_signal = state->specimx_input[1] ? 3000 : -3000;
	channel_2_signal = state->specimx_input[2] ? 3000 : -3000;

	while (samples--)
	{
		*sample_left = 0;

		/* music channel 0 */
		*sample_left += channel_0_signal;

		/* music channel 1 */
		*sample_left += channel_1_signal;

		/* music channel 2 */
		*sample_left += channel_2_signal;

		sample_left++;
	}
}

void specimx_set_input(device_t *device, int index, int state)
{
	specimx_sound_state *sndstate = get_safe_token(device);
	if (sndstate->mixer_channel!=NULL) {
		sndstate->mixer_channel->update();
	}
	sndstate->specimx_input[index] = state;
}


DEVICE_GET_INFO( specimx_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(specimx_sound_state);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(specimx_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Specialist MX Custom");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}

DEFINE_LEGACY_SOUND_DEVICE(SPECIMX, specimx_sound);
