/***************************************************************************

	wave.c

	Code that interfaces 
	Functions to handle loading, creation, recording and playback
	of wave samples for IO_CASSETTE

****************************************************************************/

#include "driver.h"
#include "utils.h"
#include "devices/cassette.h"

#define ALWAYS_PLAY_SOUND	0



static void wave_sound_update(int num, INT16 *buffer, int length)
{
	mess_image *image;
	cassette_image *cassette;
	cassette_state state;
	double time_index;
	double duration;

	image = image_from_devtype_and_index(IO_CASSETTE, num);
	state = cassette_get_state(image);

	state &= CASSETTE_MASK_UISTATE | CASSETTE_MASK_MOTOR | CASSETTE_MASK_SPEAKER;
	if (image_exists(image) && (ALWAYS_PLAY_SOUND || (state == (CASSETTE_PLAY | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED))))
	{
		cassette = cassette_get_image(image);
		time_index = cassette_get_position(image);
		duration = ((double) length) / Machine->sample_rate;

		cassette_get_samples(cassette, 0, time_index, duration, length, 2, buffer, CASSETTE_WAVEFORM_16BIT);
	}
	else
	{
		memset(buffer, 0, sizeof(*buffer) * length);
	}
}



int wave_sh_start(const struct MachineSound *msound)
{
	int i;
	int cassette_count;
	char buf[32];
	const struct Wave_interface *intf;

    intf = msound->sound_interface;

	cassette_count = device_count(IO_CASSETTE);

    for (i = 0; i < cassette_count; i++ )
	{
		if (cassette_count > 1)
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "Cassette #%d", i+1);
		else
			strncpyz(buf, "Cassette", sizeof(buf) / sizeof(buf[0]));
		stream_init(buf, intf ? intf->mixing_level[i] : 25, Machine->sample_rate, i, wave_sound_update);
	}
    return 0;
}



void wave_sh_stop(void)
{
}



void wave_sh_update(void)
{
}
