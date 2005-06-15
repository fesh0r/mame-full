#ifndef __GB_H
#define __GB_H

#include "mame.h"
#include "sound/custom.h"

void gameboy_sound_w(int offset, int data);

#ifdef __MACHINE_GB_C
#define EXTERN
#else
#define EXTERN extern
#endif

/* Interrupt flags */
#define VBL_IFLAG	0x01	/* V-Blank    */
#define LCD_IFLAG	0x02	/* LCD Status */
#define TIM_IFLAG	0x04	/* Timer      */
#define SIO_IFLAG	0x08	/* Serial I/O */
#define EXT_IFLAG	0x10	/* Joypad     */

/* Interrupts */
#define VBL_INT		0		/* V-Blank    */
#define LCD_INT		1		/* LCD Status */
#define TIM_INT		2		/* Timer      */
#define SIO_INT		3		/* Serial I/O */
#define EXT_INT		4		/* Joypad     */

/* Memory bank controller types */
#define NONE		0x000	/*  32KB ROM - No memory bank controller         */
#define MBC1		0x001	/*  ~2MB ROM,   8KB RAM -or- 512KB ROM, 32KB RAM */
#define MBC2		0x002	/* 256KB ROM,  32KB RAM                          */
#define MBC3		0x003	/*   2MB ROM,  32KB RAM, RTC                     */
#define MBC5		0x004	/*   8MB ROM, 128KB RAM (32KB w/ Rumble)         */
#define TAMA5		0x005	/*    ?? ROM     ?? RAM - What is this?          */
#define HUC1		0x006	/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
#define HUC3		0x007	/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
#define MEGADUCK	0x100	/* MEGADUCK style banking, using 0x100 to not    */
                            /* interfere with possible GBx banking methods   */
#ifdef TIMER
#undef TIMER
#endif

/* Cartridge types */
#define RAM			0x01	/* Cartridge has RAM                             */
#define BATTERY		0x02	/* Cartridge has a battery to save RAM           */
#define TIMER		0x04	/* Cartridge has a real-time-clock (MBC3 only)   */
#define RUMBLE		0x08	/* Cartridge has a rumble motor (MBC5 only)      */
#define SRAM		0x10	/* Cartridge has SRAM                            */
#define UNKNOWN		0x80	/* Cartridge is of an unknown type               */

extern UINT8 *gb_ram;

#define JOYPAD  gb_ram[0xFF00] /* Joystick: 1.1.P15.P14.P13.P12.P11.P10		 */
#define SIODATA gb_ram[0xFF01] /* Serial IO data buffer 					 */
#define SIOCONT gb_ram[0xFF02] /* Serial IO control register				 */
#define DIVREG  gb_ram[0xFF04] /* Divider register (???)					 */
#define TIMECNT gb_ram[0xFF05] /* Timer counter. Gen. int. when it overflows */
#define TIMEMOD gb_ram[0xFF06] /* New value of TimeCount after it overflows  */
#define TIMEFRQ gb_ram[0xFF07] /* Timer frequency and start/stop switch 	 */
#define IFLAGS  gb_ram[0xFF0F] /* Interrupt flags: 0.0.0.JST.SIO.TIM.LCD.VBL */
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

#define OAM						0xFE00
#define VRAM					0x8000
#define DMG_FRAMES_PER_SECOND	59.732155
#define SGB_FRAMES_PER_SECOND	61.17

EXTERN UINT8 gb_bpal[4];				/* Background palette			*/
EXTERN UINT8 gb_spal0[4];				/* Sprite 0 palette				*/
EXTERN UINT8 gb_spal1[4];				/* Sprite 1 palette				*/
EXTERN UINT8 *gb_chrgen;				/* Character generator			*/
EXTERN UINT8 *gb_bgdtab;				/* Background character table	*/
EXTERN UINT8 *gb_wndtab;				/* Window character table		*/
EXTERN unsigned int gb_divcount;
EXTERN unsigned int gb_timer_count;
EXTERN UINT8 gb_timer_shift;
EXTERN UINT8 gb_tile_no_mod;

extern WRITE8_HANDLER( gb_rom_bank_select_mbc1 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc1 );
extern WRITE8_HANDLER( gb_mem_mode_select_mbc1 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc2 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc3 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc3 );
extern WRITE8_HANDLER( gb_mem_mode_select_mbc3 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc5 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc5 );
extern WRITE8_HANDLER( gb_ram_enable );
extern WRITE8_HANDLER( gb_io_w );
extern  READ8_HANDLER ( gb_io_r );
extern WRITE8_HANDLER( gb_bios_w );
extern WRITE8_HANDLER( gb_ie_w );
extern DEVICE_INIT(gb_cart);
extern DEVICE_LOAD(gb_cart);
extern void gb_scanline_interrupt(void);
extern void gb_scanline_interrupt_set_mode0(int param);
extern void gb_scanline_interrupt_set_mode3(int param);

extern MACHINE_INIT( gb );
extern MACHINE_STOP( gb );

extern MACHINE_INIT( gbpocket );

/* from vidhrdw/gb.c */
extern WRITE8_HANDLER( gb_video_w );
extern VIDEO_START( gb );
extern VIDEO_UPDATE( gb );
void gb_refresh_scanline(void);
EXTERN double lcd_time;
/* Custom Sound Interface */
extern WRITE8_HANDLER( gb_sound_w );
void *gameboy_sh_start(int clock, const struct CustomSound_interface *config);

/* -- Super GameBoy specific -- */
#define SGB_BORDER_PAL_OFFSET	64	/* Border colours stored from pal 4-7	*/
#define SGB_XOFFSET				48	/* GB screen starts at column 48		*/
#define SGB_YOFFSET				40	/* GB screen starts at row 40			*/

EXTERN UINT16 sgb_pal_data[4096];	/* 512 palettes of 4 colours			*/
EXTERN UINT8 sgb_pal_map[20][18];	/* Palette tile map						*/
EXTERN UINT8 *sgb_tile_data;		/* 256 tiles of 32 bytes each			*/
EXTERN UINT8 sgb_tile_map[2048];	/* 32x32 tile map data (0-tile,1-attribute)	*/
EXTERN UINT8 sgb_window_mask;		/* Current GB screen mask				*/
EXTERN UINT8 sgb_hack;				/* Flag set if we're using a hack		*/

extern MACHINE_INIT( sgb );
extern WRITE8_HANDLER ( sgb_io_w );
/* from vidhrdw/gb.c */
void sgb_refresh_scanline(void);
void sgb_refresh_border(void);

/* -- GameBoy Color specific -- */
#define KEY1    gb_ram[0xFF4D]		/* Prepare speed switch					*/
#define HDMA1   gb_ram[0xFF51]		/* HDMA source high byte				*/
#define HDMA2   gb_ram[0xFF52]		/* HDMA source low byte					*/
#define HDMA3   gb_ram[0xFF53]		/* HDMA destination high byte			*/
#define HDMA4   gb_ram[0xFF54]		/* HDMA destination low byte			*/
#define HDMA5   gb_ram[0xFF55]		/* HDMA length/mode/start				*/
#define GBCBCPS gb_ram[0xFF68]		/* Backgound palette spec				*/
#define GBCBCPD gb_ram[0xFF69]		/* Backgound palette data				*/
#define GBCOCPS gb_ram[0xFF6A]		/* Object palette spec					*/
#define GBCOCPD gb_ram[0xFF6B]		/* Object palette data					*/
#define GBC_MODE_GBC		1		/* GBC is in colour mode				*/
#define GBC_MODE_MONO		2		/* GBC is in mono mode					*/
#define GBC_PAL_OBJ_OFFSET	32		/* Object palette offset				*/

extern UINT8 *GBC_VRAMMap[2];		/* Addressses of GBC video RAM banks	*/
extern UINT8 GBC_VRAMBank;			/* VRAM bank currently used				*/
EXTERN UINT8 *gbc_chrgen;			/* Character generator					*/
EXTERN UINT8 *gbc_bgdtab;			/* Background character table			*/
EXTERN UINT8 *gbc_wndtab;			/* Window character table				*/
EXTERN UINT8 gbc_mode;				/* is the GBC in mono/colour mode?		*/
EXTERN UINT8 gbc_hdma_enabled;		/* is HDMA enabled?						*/

extern MACHINE_INIT( gbc );
extern WRITE8_HANDLER ( gbc_video_w );
extern void gbc_hdma(UINT16 length);
/* from vidhrdw/gb.c */
void gbc_refresh_scanline(void);

/* -- Megaduck specific -- */
extern DEVICE_LOAD(megaduck_cart);
extern MACHINE_INIT( megaduck );
extern  READ8_HANDLER( megaduck_video_r );
extern WRITE8_HANDLER( megaduck_video_w );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type1 );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type2 );
extern  READ8_HANDLER( megaduck_sound_r1 );
extern WRITE8_HANDLER( megaduck_sound_w1 );
extern  READ8_HANDLER( megaduck_sound_r2 );
extern WRITE8_HANDLER( megaduck_sound_w2 );

#endif
