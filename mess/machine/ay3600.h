#ifndef AY3600_H
#define AY3600_H

#include "inputx.h"

#define AP2_KEYBOARD_2		0
#define AP2_KEYBOARD_2P		1
#define AP2_KEYBOARD_REPT	10 /* values < 10 will have REPT key */
#define AP2_KEYBOARD_2E		20
#define AP2_KEYBOARD_2GS	30

/* machine/ay3600.c */
int AY3600_init(int);
int AY3600_anykey_clearstrobe_r(void);
int AY3600_keydata_strobe_r(void);

QUEUE_CHARS( AY3600 );
ACCEPT_CHAR( AY3600 );

#endif /* AY3600_H */
