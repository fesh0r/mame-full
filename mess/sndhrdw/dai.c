/***************************************************************************

  sndhrdw/dai.c

  Functions to emulate sound hardware of DAI Personal Computer

  Krzysztof Strzecha

****************************************************************************/

#include "driver.h"
#include "sound/streams.h"
#include "sound/mixer.h"
#include "machine/random.h"
#include "includes/pit8253.h"
#include "includes/dai.h"

static int dai_sh_start(const struct MachineSound*);
static void dai_sh_update(int, INT16 **, int);

static int mixer_channel;

static UINT16 dai_osc_volume_table[] = {   0,  500, 1000, 1500,
					2000, 2500, 3000, 3500,
					4000, 4500, 5000, 5500,
					6000, 6500, 7000, 7500};

static UINT16 dai_noise_volume_table[] = {   0,    0,    0,    0,
					     0,    0,    0,    0,
				           500, 1000, 1500, 2000,
					  2500, 3000, 3500, 4000};

struct CustomSound_interface dai_sound_interface =
{
	dai_sh_start,
	NULL,
	NULL
};

static int dai_sh_start(const struct MachineSound* driver)
{
	const int vol[2]={ MIXER(50, MIXER_PAN_LEFT), MIXER(50, MIXER_PAN_RIGHT) };
	const char *names[2]= { "DAI left channel", "DAI right channel" };
	
	mixer_channel = stream_init_multi(2, names, vol, Machine->sample_rate, 0, dai_sh_update);

	logerror ("sample rate: %d\n", Machine->sample_rate);

	return 0;
}

static void dai_sh_update(int param, INT16 **buffer, int length)
{
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;
	static int channel_0_incr = 0;
	static int channel_1_incr = 0;
	static int channel_2_incr = 0;
	int channel_0_baseclock;
	int channel_1_baseclock;
	int channel_2_baseclock;

	int rate = Machine->sample_rate / 2;

	INT16 *sample_left = buffer[0];
	INT16 *sample_right = buffer[1];

	channel_0_baseclock = pit8253_get_frequency(0, 0);
	channel_1_baseclock = pit8253_get_frequency(0, 1);
	channel_2_baseclock = pit8253_get_frequency(0, 2);

	channel_0_signal = pit8253_get_output (0,0) ? dai_osc_volume_table[dai_osc_volume[0]] : -dai_osc_volume_table[dai_osc_volume[0]];
	channel_1_signal = pit8253_get_output (0,1) ? dai_osc_volume_table[dai_osc_volume[1]] : -dai_osc_volume_table[dai_osc_volume[1]];
	channel_2_signal = pit8253_get_output (0,2) ? dai_osc_volume_table[dai_osc_volume[2]] : -dai_osc_volume_table[dai_osc_volume[2]];


	while (length--)
	{
		*sample_left = 0;
		*sample_right = 0;

		/* music channel 0 */

		*sample_left += channel_0_signal;
		channel_0_incr -= channel_0_baseclock;
		while( channel_0_incr < 0 )
		{
			channel_0_incr += rate;
			channel_0_signal = -channel_0_signal;
		}


		/* music channel 1 */

		*sample_left += channel_1_signal;
		channel_1_incr -= channel_1_baseclock;
		while( channel_1_incr < 0 )
		{
			channel_1_incr += rate;
			channel_1_signal = -channel_1_signal;
		}

		/* music channel 2 */

		*sample_left += channel_2_signal;
		channel_2_incr -= channel_2_baseclock;
		while( channel_2_incr < 0 )
		{
			channel_2_incr += rate;
			channel_2_signal = -channel_2_signal;
		}

		/* noise channel */

		*sample_left += mame_rand()&0x01 ? dai_noise_volume_table[dai_noise_volume] : -dai_noise_volume_table[dai_noise_volume];

		sample_left++;
		sample_right++;
	}
}

void dai_sh_change_clock(double clock)
{
	stream_update(mixer_channel,0);
}
