/**************************************************************************************

  sndhrdw/odyssey2.c

  The VDC handles the audio in the Odyssey 2, so most of the code can be found
  in vidhrdw/odyssey2.c.

***************************************************************************************/
#include "driver.h"
#include "includes/odyssey2.h"

int odyssey2_sh_channel;

struct CustomSound_interface odyssey2_sound_interface = 
{
	odyssey2_sh_start,
	0,
	0
};

int odyssey2_sh_start(const struct MachineSound* driver)
{
	odyssey2_sh_channel = stream_init( "Odyssey2", 50, Machine->sample_rate, 0, odyssey2_sh_update );
	return 0;
}
