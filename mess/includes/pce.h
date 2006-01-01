/***************************************************************************

  pce.h

  Headers for the NEC PC Engine/TurboGrafx16.

***************************************************************************/

#ifndef PCE_H
#define PCE_H

/* from machine\pce.c */
extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
DEVICE_LOAD(pce_cart);
NVRAM_HANDLER( pce );
WRITE8_HANDLER ( pce_joystick_w );
 READ8_HANDLER ( pce_joystick_r );

#define PCE_FAST_CLOCK		7195090

#define TG_16_JOY_SIG		0x00
#define PCE_JOY_SIG			0x40
#define NO_CD_SIG			0x80
#define CD_SIG				0x00

struct pce_struct
{
	UINT8 io_port_options; /*driver-specific options for the PCE*/
};
extern struct pce_struct pce;
DRIVER_INIT( pce );
#endif
