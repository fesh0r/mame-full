#ifndef AY3600_H
#define AY3600_H

#include "inputx.h"

/* machine/ay3600.c */
int AY3600_init(void);
void AY3600_interrupt(void);
int AY3600_anykey_clearstrobe_r(void);
int AY3600_keydata_strobe_r(void);

QUEUE_CHARS( AY3600 );
ACCEPT_CHAR( AY3600 );

#endif /* AY3600_H */
