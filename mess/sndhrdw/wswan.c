/**************************************************************************************

  Wonderswan sound emulation

**************************************************************************************/

#include "driver.h"
#include "includes/wswan.h"

void wswan_sh_update(int param, INT16 **buffer, int length)
{
	INT16 left, right;

	while( length-- > 0 )
	{
		left = right = 0;

		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}
}

int wswan_sh_start(const struct MachineSound* driver)
{
	const char *names[2] = { "WSwan", "WSwan" };
	const int volume[2] = { MIXER( 50, MIXER_PAN_LEFT ), MIXER( 50, MIXER_PAN_RIGHT ) };

	stream_init_multi(2, names, volume, Machine->sample_rate, 0, wswan_sh_update);

	return 0;
}
