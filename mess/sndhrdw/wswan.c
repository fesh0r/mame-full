/**************************************************************************************

  Wonderswan sound emulation

**************************************************************************************/

#include "driver.h"
#include "includes/wswan.h"
#include "streams.h"

static void wswan_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	stream_sample_t left, right;

	while( length-- > 0 )
	{
		left = right = 0;

		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}
}

void *wswan_sh_start(int clock, const struct CustomSound_interface *config)
{
	stream_create(0, 2, Machine->sample_rate, 0, wswan_sh_update);

	return (void *) ~0;
}
