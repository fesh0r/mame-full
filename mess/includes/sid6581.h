#ifndef __SID_6581_H_
#define __SID_6581_H_

#include "driver.h"

/* c64 / c128 sound interface
   cbmb series interface
   c16 sound card
   c64 hardware modification for second sid
   c65 has 2 inside

   selection between two type 6581 8580 would be nice */

extern void sid6581_0_init (int (*paddle) (int offset), int pal);
extern void sid6581_1_init (int (*paddle) (int offset), int pal);

typedef enum { MOS6581, MOS8580 } SIDTYPE;
extern void sid6581_0_configure (SIDTYPE type);
extern void sid6581_1_configure (SIDTYPE type);

extern void sid6581_0_reset (void);
extern void sid6581_1_reset (void);

extern WRITE_HANDLER ( sid6581_0_port_w );
extern WRITE_HANDLER ( sid6581_1_port_w );
extern READ_HANDLER  ( sid6581_0_port_r );
extern READ_HANDLER  ( sid6581_1_port_r );

extern void sid6581_update(void);

extern struct CustomSound_interface sid6581_sound_interface;

/* private area */
typedef struct {
	int (*paddle_read) (int offset);
	int reg[0x20];
	bool sidKeysOn[0x20], sidKeysOff[0x20];
} SID6581;

extern SID6581 sid6581[2];

UINT16 sid6581_read_word(SID6581 *this, int offset);


#endif
