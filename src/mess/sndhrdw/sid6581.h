#ifndef __SID_6581_H_
#define __SID_6581_H_

/* c64 / c128 sound interface 
   c16 sound card
   c64 hardware modification for second sid 
   c65 has 2 inside

   selection between two type 6581 8580 would be nice */

extern void sid6581_0_init (int (*paddle) (int offset));
extern void sid6581_1_init (int (*paddle) (int offset));
extern void sid6581_0_port_w (int offset, int data);
extern void sid6581_1_port_w (int offset, int data);
extern int sid6581_0_port_r (int offset);
extern int sid6581_1_port_r (int offset);

#endif
