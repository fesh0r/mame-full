/***************************************************************************

	ay3600.h

	Include file for AY3600 keyboard; used by Apple IIs

***************************************************************************/

#ifndef AY3600_H
#define AY3600_H

#include "inputx.h"

typedef enum
{
	AP2_KEYBOARD_2,
	AP2_KEYBOARD_2P,
	AP2_KEYBOARD_2E,
	AP2_KEYBOARD_2GS
} ay3600_keyboard_type_t;

/* machine/ay3600.c */
int AY3600_init(ay3600_keyboard_type_t keyboard_type);
int AY3600_anykey_clearstrobe_r(void);
int AY3600_keydata_strobe_r(void);

QUEUE_CHARS( AY3600 );
ACCEPT_CHAR( AY3600 );

#endif /* AY3600_H */
