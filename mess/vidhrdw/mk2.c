/******************************************************************************
 PeT mess@utanet.at
******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mscommon.h"

#include "includes/mk2.h"

UINT8 mk2_led[5]= {0};

VIDEO_START( mk2 )
{
	return 0;
}

VIDEO_UPDATE( mk2 )
{
	int i;

	for (i=0; i<4; i++)
		output_set_digit_value(i, mk2_led[i]);
	output_set_led_value(0, mk2_led[4]&8?1:0);
	output_set_led_value(1, mk2_led[4]&0x20?1:0);
	output_set_led_value(2, mk2_led[4]&0x10?1:0);
	output_set_led_value(3, mk2_led[4]&0x10?0:1);
	
	mk2_led[0]= mk2_led[1]= mk2_led[2]= mk2_led[3]= mk2_led[4]= 0;
	
	return 0;
}
