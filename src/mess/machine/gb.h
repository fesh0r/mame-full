#ifndef __GB_H
#define __GB_H

#include "driver.h"

#ifdef __MACHINE_GB_C
#define EXTERN
#else
#define EXTERN extern
#endif

#define VBL_IFLAG 0x01
#define LCD_IFLAG 0x02
#define TIM_IFLAG 0x04
#define SIO_IFLAG 0x08
#define EXT_IFLAG 0x10

#define VBL_INT 0
#define LCD_INT 1
#define TIM_INT 2
#define SIO_INT 3
#define EXT_INT 4
 
extern UINT8 *gb_ram;

#define JOYPAD  gb_ram[0xFF00] /* Joystick: 1.1.P15.P14.P13.P12.P11.P10      */
#define SIODATA gb_ram[0xFF01] /* Serial IO data buffer 					 */
#define SIOCONT gb_ram[0xFF02] /* Serial IO control register				 */
#define DIVREG	gb_ram[0xFF04] /* Divider register (???)					 */
#define TIMECNT gb_ram[0xFF05] /* Timer counter. Gen. int. when it overflows */
#define TIMEMOD gb_ram[0xFF06] /* New value of TimeCount after it overflows  */
#define TIMEFRQ gb_ram[0xFF07] /* Timer frequency and start/stop switch 	 */
#define IFLAGS	gb_ram[0xFF0F] /* Interrupt flags: 0.0.0.JST.SIO.TIM.LCD.VBL */
#define ISWITCH gb_ram[0xFFFF] /* Switches to enable/disable interrupts 	 */
#define LCDCONT gb_ram[0xFF40] /* LCD control register						 */
#define LCDSTAT gb_ram[0xFF41] /* LCD status register						 */
#define SCROLLY gb_ram[0xFF42] /* Starting Y position of the background 	 */
#define SCROLLX gb_ram[0xFF43] /* Starting X position of the background 	 */
#define CURLINE gb_ram[0xFF44] /* Current screen line being scanned 		 */
#define CMPLINE gb_ram[0xFF45] /* Gen. int. when scan reaches this line 	 */
#define BGRDPAL gb_ram[0xFF47] /* Background palette						 */
#define SPR0PAL gb_ram[0xFF48] /* Sprite palette #0 						 */
#define SPR1PAL gb_ram[0xFF49] /* Sprite palette #1 						 */
#define WNDPOSY gb_ram[0xFF4A] /* Window Y position 						 */
#define WNDPOSX gb_ram[0xFF4B] /* Window X position 						 */

#define OAM  0xFE00
#define VRAM 0x8000

EXTERN UINT16 gb_bpal[4];				/* Background palette */
EXTERN UINT16 gb_spal0[4];				/* Sprite 0 palette */
EXTERN UINT16 gb_spal1[4];				/* Sprite 1 palette */
EXTERN UINT8 *gb_chrgen;				/* Character generator */
EXTERN UINT8 *gb_bgdtab;				/* Background character table */
EXTERN UINT8 *gb_wndtab;				/* Window character table */
EXTERN unsigned int gb_divcount;
EXTERN unsigned int gb_timer_count;
EXTERN UINT8 gb_timer_shift;
EXTERN UINT8 gb_tile_no_mod;

extern void gb_rom_bank_select(int offset, int data);
extern void gb_ram_bank_select(int offset, int data);
extern void gb_w_io (int offset, int data);
extern int gb_r_divreg (int offset);
extern int gb_r_timer_cnt (int offset);
extern int gb_load_rom (int id, const char *rom_name);
extern int gb_id_rom (const char *name, const char *gamename);
extern int gb_scanline_interrupt(void);
extern void gb_scanline_interrupt_set_mode2(int param);
extern void gb_scanline_interrupt_set_mode3(int param);
extern int gb_vh_start(void);
extern void gb_vh_stop(void);
extern void gb_vh_screen_refresh(struct osd_bitmap *bitmap, int full_refresh);
extern void gb_init_machine(void);

/* from vidhrdw/gb.c */
void gb_refresh_scanline(void);

#endif
