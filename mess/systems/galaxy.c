/***************************************************************************
Galaksija driver by Krzyszof Strzecha

Only source of knowladge about Galaksja is emulator of this machine created by
emulator by Miodrag Jevremovic.

There is the need to find more information about this system, without it the 
progress in developing driver is nearly impossible.

09/01/2001 Fast mode implemented (many thanks to Kevin Thacker)
07/01/2001 Keyboard corrected (still some kays unknown)
           Horizontal screen positioning in video subsystem added
05/01/2001 Keyboard implemented (some keys unknown)
03/01/2001 Snapshot loading added
01/01/2001 Preliminary driver

To do:
-Video subsystem features
-Correct palette
-Tape support
-Sound (if any)
-Hi-res graphic (if any)

Galaxsija memory map

	CPU: Z80
		0000-0fff ROM1
		1000-1fff ROM2
		2000-2035 Keyboard mapped
		2036-27ff Unknown
		2800-29ff RAM (screen)
		2a00-4000 RAM 
		
Interrupts:

	IRQ:
		50Hz vertical sync

Ports:

	Out: ?

	In: ?

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/galaxy.h"

/* port i/o functions */

PORT_READ_START( galaxy_readport )
PORT_END

PORT_WRITE_START( galaxy_writeport )
PORT_END

/* memory w/r functions */

MEMORY_READ_START( galaxy_readmem )
	{0x0000, 0x0fff, MRA_ROM},
	{0x1000, 0x1fff, MRA_ROM},
	{0x2000, 0x2035, galaxy_kbd_r},
	{0x2036, 0x27ff, MRA_RAM},
	{0x2800, 0x29ff, videoram_r},
	{0x2a00, 0x3fff, MRA_RAM},
	{0x4000, 0xffff, MRA_NOP},
MEMORY_END

MEMORY_WRITE_START( galaxy_writemem )
	{0x0000, 0x0fff, MWA_ROM},
	{0x1000, 0x1fff, MWA_ROM},
	{0x2000, 0x2035, galaxy_kbd_w},
	{0x2036, 0x27ff, MWA_RAM},
	{0x2800, 0x29ff, videoram_w, &videoram, &videoram_size},
	{0x2a00, 0x3fff, MWA_RAM},
	{0x4000, 0xffff, MWA_NOP},
MEMORY_END

/* graphics output */

struct GfxLayout galaxy_charlayout =
{
	8, 13,	/* 8x8 characters */
	128,	/* 128 characters */
	1,		/* 1 bits per pixel */
	{0},	/* no bitplanes; 1 bit per pixel */
	{7, 6, 5, 4, 3, 2, 1, 0},
	{0*128*8, 1*128*8,  2*128*8,  3*128*8,
	 4*128*8, 5*128*8,  6*128*8,  7*128*8,
	 8*128*8, 9*128*8, 10*8*128, 11*128*8, 12*128*8},
	8 	/* each character takes 1 consecutive bytes */
};

static struct GfxDecodeInfo galaxy_gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &galaxy_charlayout, 0, 2},
MEMORY_END								   /* end of array */

static unsigned char galaxy_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static unsigned short galaxy_colortable[] =
{
	0, 1
};

static void galaxy_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, galaxy_palette, sizeof (galaxy_palette));
	memcpy (sys_colortable, galaxy_colortable, sizeof (galaxy_colortable));
}

/* keyboard input */
INPUT_PORTS_START (galaxy)

	PORT_START
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up", KEYCODE_UP,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down", KEYCODE_DOWN,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left", KEYCODE_LEFT,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Righ", KEYCODE_RIGHT,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space", KEYCODE_SPACE,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_COLON,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, ":", KEYCODE_QUOTE,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_EQUALS,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP,  IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH,  IP_JOY_NONE )
	PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter", KEYCODE_ENTER,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Break", KEYCODE_PAUSE,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "???", KEYCODE_LALT,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "???", KEYCODE_BACKSPACE,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "List (?)", KEYCODE_ESC,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "???", KEYCODE_LSHIFT,  IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "???", KEYCODE_RSHIFT,  IP_JOY_NONE )
INPUT_PORTS_END


/* machine definition */

static	struct MachineDriver machine_driver_galaxy =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,				  
			galaxy_readmem, galaxy_writemem,
			galaxy_readport, galaxy_writeport,
			interrupt, 1,
		},
	},
	50,					/* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,		/* vblank duration */
	1,
	galaxy_init_machine,
	galaxy_stop_machine,

	/* video hardware */
	32 * 8,					/* screen width */
	16 * 13,				/* screen height */
	{0, 32 * 8 - 1, 0, 16 * 13 - 1},	/* visible_area */
	galaxy_gfxdecodeinfo,			/* graphics decode info */
	sizeof (galaxy_palette) / 3,
	sizeof (galaxy_colortable),		/* colors used for the characters */
	galaxy_init_palette,			/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	galaxy_vh_start,
	galaxy_vh_stop,
	galaxy_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

ROM_START (galaxy)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("galrom1.bin", 0x0000, 0x1000, 0x365f3e24)
	ROM_LOAD ("galrom2.bin", 0x1000, 0x1000, 0x5dc5a100)
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD ("galchr.bin", 0x0000, 0x0800, 0x5c3b5bb5)
ROM_END


static const struct IODevice io_galaxy[] = {
    {
	IO_SNAPSHOT,		/* type */
	1,			/* count */
	"gal\0",        	/* file extensions */
	IO_RESET_ALL,		/* reset if file changed */
        NULL,               	/* id */
	galaxy_load_snap,	/* init */
	galaxy_exit_snap,	/* exit */
        NULL,		        /* info */
        NULL,           	/* open */
        NULL,               	/* close */
        NULL,               	/* status */
        NULL,               	/* seek */
	NULL,			/* tell */
        NULL,           	/* input */
        NULL,               	/* output */
        NULL,               	/* input_chunk */
        NULL                	/* output_chunk */
    },
//    IO_CASSETTE_WAVE(1,"wav\0", NULL, galaxy_init_wav, galaxy_exit_wav),
    { IO_END }
};

/*    YEAR    NAME  PARENT  MACHINE   INPUT  INIT  COMPANY     FULLNAME */
COMP( 1983, galaxy,      0,  galaxy, galaxy,    0,      "", "Galaksija" )
