/***************************************************************************
    commodore c128 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation:
 	 iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org
***************************************************************************/
#ifndef __C128_H_
#define __C128_H_

#include "driver.h"

#include "mess/vidhrdw/praster.h"
#include "c64.h"

#define C128_MAIN_MEMORY		(input_port_8_r(0)&0x300)
#define RAM128KB (0)
#define RAM256KB (0x100)
#define RAM1MB (0x200)

#define VDC_RAM_64KBYTE		(input_port_8_r(0)&0x80) /* else 16KB */
#define MONITOR_TV		(input_port_8_r(0)&0x40) /* else RGBI */

#define C128_KEY_RIGHT_SHIFT ((input_port_12_r(0)&0x20)\
			 ||C128_KEY_CURSOR_UP||C128_KEY_CURSOR_LEFT)

#define C128_KEY_CURSOR_DOWN ((input_port_12_r(0)&0x10)||C128_KEY_CURSOR_UP)
#define C128_KEY_CURSOR_RIGHT ((input_port_12_r(0)&8)||C128_KEY_CURSOR_LEFT)

#define KEY_ESCAPE (input_port_13_r(0)&0x8000)
#define KEY_TAB (input_port_13_r(0)&0x4000)
#define KEY_ALT (input_port_13_r(0)&0x2000)
#define KEY_DIN (input_port_13_r(0)&0x1000)

#define KEY_HELP (input_port_13_r(0)&0x0800)
#define KEY_LINEFEED (input_port_13_r(0)&0x0400)
#define KEY_4080 (input_port_13_r(0)&0x0200)
#define KEY_NOSCRL (input_port_13_r(0)&0x0100)

#define KEY_UP (input_port_13_r(0)&0x0080)
#define KEY_DOWN (input_port_13_r(0)&0x0040)
#define KEY_LEFT (input_port_13_r(0)&0x0020)
#define KEY_RIGHT (input_port_13_r(0)&0x0010)

#define C128_KEY_F1 (input_port_13_r(0)&0x0008)
#define C128_KEY_F3 (input_port_13_r(0)&0x0004)
#define C128_KEY_F5 (input_port_13_r(0)&0x0002)
#define C128_KEY_F7 (input_port_13_r(0)&0x0001)

#define KEY_NUM7 (input_port_14_r(0)&0x8000)
#define KEY_NUM8 (input_port_14_r(0)&0x4000)
#define KEY_NUM9 (input_port_14_r(0)&0x2000)
#define KEY_NUMPLUS (input_port_14_r(0)&0x1000)
#define KEY_NUM4 (input_port_14_r(0)&0x800)
#define KEY_NUM5 (input_port_14_r(0)&0x400)
#define KEY_NUM6 (input_port_14_r(0)&0x200)
#define KEY_NUMMINUS (input_port_14_r(0)&0x100)
#define KEY_NUM1 (input_port_14_r(0)&0x80)
#define KEY_NUM2 (input_port_14_r(0)&0x40)
#define KEY_NUM3 (input_port_14_r(0)&0x20)
#define KEY_NUM0 (input_port_14_r(0)&0x10)
#define KEY_NUMPOINT (input_port_14_r(0)&8)
#define KEY_NUMENTER (input_port_14_r(0)&4)
#define C128_KEY_CURSOR_UP (input_port_14_r(0)&2)
#define C128_KEY_CURSOR_LEFT (input_port_14_r(0)&1)

extern UINT8 *c128_basic;
extern UINT8 *c128_kernal;
extern UINT8 *c128_chargen;
extern UINT8 *c128_z80;
extern UINT8 *c128_editor;
extern UINT8 *c128_internal_function;
extern UINT8 *c128_external_function;
extern UINT8 *c128_vdcram;

extern void c128_m6510_port_w (int offset, int data);
extern int c128_m6510_port_r (int offset);
void c128_mmu8722_port_w (int offset, int data);
int c128_mmu8722_port_r (int offset);
void c128_mmu8722_ff00_w (int offset, int data);
int c128_mmu8722_ff00_r (int offset);
int c128_capslock_r (void);
void c128_colorram_write (int offset, int value);
void c128_write_0000 (int offset, int value);
void c128_write_1000 (int offset, int value);
void c128_write_4000 (int offset, int value);
void c128_write_8000 (int offset, int value);
void c128_write_a000 (int offset, int value);
void c128_write_d000 (int offset, int value);
void c128_write_e000 (int offset, int value);
void c128_write_ff00 (int offset, int value);
void c128_write_ff05 (int offset, int value);

extern void c128_driver_init (void);
extern void c128pal_driver_init (void);
extern void c128pal2_driver_init (void);
extern void c128_driver_shutdown (void);
extern void c128_init_machine (void);
extern void c128_shutdown_machine (void);

extern int c128_vh_start (void);
extern void c128_vh_stop (void);
extern void c128_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int c128_raster_irq (void);

extern void c128_state(PRASTER *this);

/* private area */
extern int c128;					   /* computer is commodore c128 */
extern UINT8 c128_keyline[3];

void c128_bankswitch_64 (void);

#endif
