/***************************************************************************
TI-85 driver by Krzysztof Strzecha

Notes:
1. After start TI-85 waits for ON key interrupt, so press ON key to start
   calculator.
2. Only difference beetwen all TI-85 drivers is ROM version.

09/02/2001 Keypad added.
	   200Hz timer interrupts implemented.
	   ON key and its interrupts implemented.
	   Calculator is now fully usable.
02/02/2001 Preliminary driver

To do:
- LCD on/off
- pallete and contrast control
- NVRAM
- link (port 7)
- port 4 and 6
- sound
- artwork
- snapshot loading


TI-85 memory map

	CPU: Z80 6MHz
		0000-3fff ROM 0
		4000-7fff ROM 1-7 (switched)
		8000-ffff RAM
		
Interrupts:

	IRQ: 200Hz timer
	     ON key	

Ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Video buffer width, interrupt control (write only)
	5: Memory page
	6: Power mode
	7: Link

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/ti85.h"

/* port i/o functions */

PORT_READ_START( ti85_readport )
	{0x0000, 0x0000, ti85_port_0000_r},
	{0x0001, 0x0001, ti85_port_0001_r},
	{0x0002, 0x0002, ti85_port_0002_r},
	{0x0003, 0x0003, ti85_port_0003_r},
	{0x0004, 0x0004, ti85_port_0004_r},
	{0x0005, 0x0005, ti85_port_0005_r},
	{0x0006, 0x0006, ti85_port_0006_r},
	{0x0007, 0x0007, ti85_port_0007_r},
PORT_END

PORT_WRITE_START( ti85_writeport )
	{0x0000, 0x0000, ti85_port_0000_w},
	{0x0001, 0x0001, ti85_port_0001_w},
	{0x0002, 0x0002, ti85_port_0002_w},
	{0x0003, 0x0003, ti85_port_0003_w},
	{0x0004, 0x0004, ti85_port_0004_w},
	{0x0005, 0x0005, ti85_port_0005_w},
	{0x0006, 0x0006, ti85_port_0006_w},
	{0x0007, 0x0007, ti85_port_0007_w},
PORT_END

/* memory w/r functions */

MEMORY_READ_START( ti85_readmem )
	{0x0000, 0x3fff, MRA_BANK1},
	{0x4000, 0x7fff, MRA_BANK2},
	{0x8000, 0xffff, MRA_RAM},
MEMORY_END

MEMORY_WRITE_START( ti85_writemem )
	{0x0000, 0x3fff, MWA_BANK3},
	{0x4000, 0x7fff, MWA_BANK4},
	{0x8000, 0xffff, MWA_RAM},
MEMORY_END

static unsigned char ti85_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static unsigned short ti85_colortable[] =
{
	0, 1
};


static void ti85_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, ti85_palette, sizeof (ti85_palette));
	memcpy (sys_colortable, ti85_colortable, sizeof (ti85_colortable));
}


/* keyboard input */
INPUT_PORTS_START (ti85)
	PORT_START   /* bit 0 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down"  , KEYCODE_DOWN,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER" , KEYCODE_ENTER,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(-)"   , KEYCODE_M,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "."     , KEYCODE_STOP,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0"     , KEYCODE_0,          IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F5"    , KEYCODE_F5,         IP_JOY_NONE )
	PORT_START   /* bit 1 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left"  , KEYCODE_LEFT,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+"     , KEYCODE_EQUALS,     IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3"     , KEYCODE_3,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2"     , KEYCODE_2,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1"     , KEYCODE_1,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STORE" , KEYCODE_TAB,          IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4"    , KEYCODE_F4,         IP_JOY_NONE )
	PORT_START   /* bit 2 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Right" , KEYCODE_RIGHT,      IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-"     , KEYCODE_MINUS,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6"     , KEYCODE_6,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5"     , KEYCODE_5,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4"     , KEYCODE_4,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ","     , KEYCODE_COMMA,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F3"    , KEYCODE_F3,         IP_JOY_NONE )
	PORT_START   /* bit 3 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up"    , KEYCODE_UP,         IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "*"     , KEYCODE_L,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9"     , KEYCODE_9,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8"     , KEYCODE_8,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7"     , KEYCODE_7,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x^2"   , KEYCODE_COLON,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2"    , KEYCODE_F2,         IP_JOY_NONE )
	PORT_START   /* bit 4 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/"     , KEYCODE_SLASH,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ")"     , KEYCODE_CLOSEBRACE, IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "("     , KEYCODE_OPENBRACE,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EE"    , KEYCODE_END,        IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LN"    , KEYCODE_BACKSLASH,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F1"    , KEYCODE_F1,         IP_JOY_NONE )
	PORT_START   /* bit 5 */                                                        
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^"     , KEYCODE_P,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAN"   , KEYCODE_PGUP,       IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "COS"   , KEYCODE_HOME,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SIN"   , KEYCODE_INSERT,     IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LOG"   , KEYCODE_QUOTE,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2nd"   , KEYCODE_LALT,       IP_JOY_NONE )
	PORT_START   /* bit 6 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CLEAR" , KEYCODE_PGDN,       IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CUSTOM", KEYCODE_F9,         IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PRGM"  , KEYCODE_F8,         IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STAT"  , KEYCODE_F7,         IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "GRAPH" , KEYCODE_F6,         IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EXIT"  , KEYCODE_ESC,        IP_JOY_NONE )
	PORT_START   /* bit 7 */
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL"   , KEYCODE_DEL,        IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x-VAR" , KEYCODE_LCONTROL,   IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALPHA" , KEYCODE_CAPSLOCK,   IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MORE"  , KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_START   /* ON */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ON/OFF", KEYCODE_Q,          IP_JOY_NONE )
INPUT_PORTS_END


/* machine definition */

static	struct MachineDriver machine_driver_ti85 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* 6 MHz */
			ti85_readmem, ti85_writemem,
			ti85_readport, ti85_writeport,
			0, 0,
		},
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti85_init_machine,
	ti85_stop_machine,

	/* video hardware */
	640,					/* screen width */
	480,					/* screen height */
	{0, 640-1, 0, 480-1},			/* visible_area */
	0,					/* graphics decode info */
	sizeof (ti85_palette) / 3,
	sizeof (ti85_colortable),		/* colors used for the characters */
	ti85_init_palette,			/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	ti85_vh_start,
	ti85_vh_stop,
	ti85_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

ROM_START (ti85v30a)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v30a.bin", 0x10000, 0x20000, 0xde4c0b1a)
ROM_END

ROM_START (ti85v40)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v40.bin", 0x10000, 0x20000, 0xa1723a17)
ROM_END

ROM_START (ti85v50)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v50.bin", 0x10000, 0x20000, 0x781fa403)
ROM_END

ROM_START (ti85v60)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v60.bin", 0x10000, 0x20000, 0xb694a117)
ROM_END

ROM_START (ti85v80)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v80.bin", 0x10000, 0x20000, 0x7f296338)
ROM_END

ROM_START (ti85v90)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v90.bin", 0x10000, 0x20000, 0x6a0a94d0)
ROM_END

ROM_START (ti85v100)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v100.bin", 0x10000, 0x20000, 0x053325b0)
ROM_END


static const struct IODevice io_ti85[] = {
    { IO_END }
};

#define	io_ti85v30a	io_ti85
#define	io_ti85v40	io_ti85
#define	io_ti85v50	io_ti85
#define	io_ti85v60	io_ti85
#define	io_ti85v80	io_ti85
#define	io_ti85v90	io_ti85
#define	io_ti85v100	io_ti85
                                                       
/*    YEAR     NAME  PARENT  MACHINE  INPUT  INIT              COMPANY        FULLNAME */
COMP( 1983, ti85v30a,      0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 3.0a" )
COMP( 1983, ti85v40,       0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 4.0" )
COMP( 1983, ti85v50,       0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 5.0" )
COMP( 1983, ti85v60,       0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 6.0" )
COMP( 1983, ti85v80,       0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 8.0" )
COMP( 1983, ti85v90,       0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 9.0" )
COMP( 1983, ti85v100,      0,    ti85,  ti85,    0, "Texas Instruments", "TI-85 ver. 10.0" )
