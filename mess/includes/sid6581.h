#ifndef __SID_6581_H_
#define __SID_6581_H_

#include "driver.h"
#include "sound/custom.h"

/* 
   c64 / c128 sound interface
   cbmb series interface
   c16 sound card
   c64 hardware modification for second sid
   c65 has 2 inside

   selection between two type 6581 8580 would be nice 

   sid6582 6581 with other input voltages 
*/
#define MAX_SID6581 2

typedef enum { MOS6581, MOS8580 } SIDTYPE;

typedef struct
{
	/* this is here, until this sound approximation is added to
	   mame's sound devices */
	struct CustomSound_interface custom;

	struct
	{
		/* bypassed to mixer_allocate_channel, so use
		   the macros defined in src/sound/mixer.h to load values*/
		SIDTYPE type;
		int clock;
		int (*ad_read)(int channel);
	} chip;
} SID6581_interface;

extern void sid6581_set_type(int number, SIDTYPE type);

extern void sid6581_reset (int number);

extern void sid6581_update(void);

extern WRITE8_HANDLER ( sid6581_0_port_w );
extern WRITE8_HANDLER ( sid6581_1_port_w );
extern  READ8_HANDLER  ( sid6581_0_port_r );
extern  READ8_HANDLER  ( sid6581_1_port_r );

void *sid6581_custom_start (int clock, const struct CustomSound_interface *config);

#endif
