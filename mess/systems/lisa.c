/*
	experimental LISA driver

	Raphael Nabet, 2000
*/

#include "driver.h"
#include "vidhrdw/generic.h"
/*#include "machine/6522via.h"*/
#include "machine/lisa.h"


static MEMORY_READ16_START (lisa_readmem )

	{ 0x000000, 0xffffff, lisa_r },

MEMORY_END

static MEMORY_WRITE16_START (lisa_writemem)

	{ 0x000000, 0xffffff, lisa_w },

MEMORY_END

static MEMORY_READ_START (lisa_fdc_readmem)

	{ 0x0000, 0x03ff, MRA_RAM },		/* RAM (shared with 68000) */
	{ 0x0400, 0x07ff, lisa_fdc_io_r },	/* disk controller (IWM and TTL logic) */
	{ 0x0800, 0x0fff, MRA_NOP },
	{ 0x1000, 0x1fff, MRA_ROM },		/* ROM */
	{ 0x2000, 0xffff, lisa_fdc_r },		/* handler for wrap-around */

MEMORY_END

static MEMORY_WRITE_START (lisa_fdc_writemem)

	{ 0x0000, 0x03ff, MWA_RAM },		/* RAM (shared with 68000) */
	{ 0x0400, 0x07ff, lisa_fdc_io_w },	/* disk controller (IWM and TTL logic) */
	{ 0x0800, 0x0fff, MWA_NOP },
	{ 0x1000, 0x1fff, MWA_ROM },		/* ROM */
	{ 0x2000, 0xffff, lisa_fdc_w },		/* handler for wrap-around */

MEMORY_END

static MEMORY_READ_START (lisa210_fdc_readmem)

	{ 0x0000, 0x03ff, MRA_RAM },		/* RAM (shared with 68000) */
	{ 0x0400, 0x07ff, MRA_NOP },
	{ 0x0800, 0x0bff, lisa_fdc_io_r },	/* disk controller (IWM and TTL logic) */
	{ 0x0c00, 0x0fff, MRA_NOP },
	{ 0x1000, 0x1fff, MRA_ROM },		/* ROM */
	{ 0x2000, 0xffff, lisa_fdc_r },		/* handler for wrap-around */

MEMORY_END

static MEMORY_WRITE_START (lisa210_fdc_writemem)

	{ 0x0000, 0x03ff, MWA_RAM },		/* RAM (shared with 68000) */
	{ 0x0400, 0x07ff, MWA_NOP },
	{ 0x0800, 0x0bff, lisa_fdc_io_w },	/* disk controller (IWM and TTL logic) */
	{ 0x0c00, 0x0fff, MWA_NOP },
	{ 0x1000, 0x1fff, MWA_ROM },		/* ROM */
	{ 0x2000, 0xffff, lisa_fdc_w },		/* handler for wrap-around */

MEMORY_END

static void lisa_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	palette[0*3 + 0] = 0xff;
	palette[0*3 + 1] = 0xff;
	palette[0*3 + 2] = 0xff;
	palette[1*3 + 0] = 0x00;
	palette[1*3 + 1] = 0x00;
	palette[1*3 + 2] = 0x00;
}

/* sound output */

static	struct	Speaker_interface lisa_sh_interface =
{
	1,
	{ 100 },
	{ 0 },
	{ NULL }
};

/* Lisa1 and Lisa 2 machine */
static struct MachineDriver machine_driver_lisa =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			5000000,			/* +/- 5 Mhz */
			lisa_readmem,lisa_writemem,0,0,
			lisa_interrupt,1,
		},
		{
			/*CPU_M6504*/CPU_M6502,
			2000000,			/* 16/8 in when DIS asserted, 16/9 otherwise */
			lisa_fdc_readmem,lisa_fdc_writemem,0,0,
			0,0,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	lisa_init_machine,
	0,

	/* video hardware */
	720, 364, /* screen width, screen height */
	{ 0, 720-1, 0, 364-1 },			/* visible_area */

	0,					/* graphics decode info */
	2, 2,						/* number of colors, colortable size */
	lisa_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	lisa_vh_start,
	lisa_vh_stop,
	lisa_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			& lisa_sh_interface
		}
	},

	/*lisa_nvram_handler*/
};

/* Lisa 210 machine (different fdc map) */
static struct MachineDriver machine_driver_lisa210 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			5000000,			/* +/- 5 Mhz */
			lisa_readmem,lisa_writemem,0,0,
			lisa_interrupt,1,
		},
		{
			/*CPU_M6504*/CPU_M6502,
			2000000,			/* 16/8 in when DIS asserted, 16/9 otherwise ??? */
			lisa210_fdc_readmem,lisa210_fdc_writemem,0,0,
			0,0,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	lisa_init_machine,
	0,

	/* video hardware */
	720, 364, /* screen width, screen height */
	{ 0, 720-1, 0, 364-1 },			/* visible_area */

	0,					/* graphics decode info */
	2, 2,						/* number of colors, colortable size */
	lisa_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	lisa_vh_start,
	lisa_vh_stop,
	lisa_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			& lisa_sh_interface
		}
	},

	/*lisa_nvram_handler*/
};

/* Mac XL machine (different video resolution) */
static struct MachineDriver machine_driver_macxl =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			5000000,			/* +/- 5 Mhz */
			lisa_readmem,lisa_writemem,0,0,
			lisa_interrupt,1,
		},
		{
			/*CPU_M6504*/CPU_M6502,
			2000000,			/* 16/8 in when DIS asserted, 16/9 otherwise ??? */
			lisa210_fdc_readmem,lisa210_fdc_writemem,0,0,
			0,0,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	lisa_init_machine,
	0,

	/* video hardware */
	608, 431, /* screen width, screen height */
	{ 0, 608-1, 0, 431-1 },			/* visible_area */

	0,					/* graphics decode info */
	2, 2,						/* number of colors, colortable size */
	lisa_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	lisa_vh_start,
	lisa_vh_stop,
	lisa_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			& lisa_sh_interface
		}
	},

	/*lisa_nvram_handler*/
};


INPUT_PORTS_START( lisa )
	PORT_START /* Mouse - X AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - Y AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	/* pseudo-input ports with (unverified) keyboard layout */

	PORT_START	/* 2 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON1, "mouse button", KEYCODE_NONE, IP_JOY_DEFAULT)

	PORT_START	/* 3 */
	PORT_BITX(0xFFFF, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_START	/* 4 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Clear", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ (KP)", KEYCODE_SLASH_PAD, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "* (KP)", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= (KP)", KEYCODE_NUMLOCK, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 (KP)", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (KP)", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (KP)", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- (KP)", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 (KP)", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 (KP)", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ (KP)", KEYCODE_PLUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". (KP)", KEYCODE_DEL_PAD, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 (KP)", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 (KP)", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter (KP)", KEYCODE_ENTER_PAD, IP_JOY_NONE)

	PORT_START	/* 5 */
	PORT_BITX(0xFFFF, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_START	/* 6 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x000C, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x00C0, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 (KP)", KEYCODE_0_PAD, IP_JOY_NONE)
	PORT_BITX(0x0C00, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 (KP)", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(0xC000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_START	/* 7 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START	/* 8 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Right Command", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)

	PORT_START	/* 9 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Option", KEYCODE_LALT, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left Command", KEYCODE_LCONTROL, IP_JOY_NONE)

INPUT_PORTS_END


ROM_START( lisa2 )
	ROM_REGION16_BE(0x204000,REGION_CPU1,0)	/* 68k rom and ram */
	ROM_LOAD16_BYTE( "booth.hi", 0x000000, 0x2000, 0xadfd4516)
	ROM_LOAD16_BYTE( "booth.lo", 0x000001, 0x2000, 0x546d6603)

	ROM_REGION(0x2000,REGION_CPU2,0)		/* 6504 RAM and ROM */
	ROM_LOAD( "ioa8.rom", 0x1000, 0x1000, 0xbc6364f1)

	ROM_REGION(0x100,REGION_GFX1,0)		/* video ROM (includes S/N) */
	ROM_LOAD( "vidstate.rom", 0x00, 0x100, 0x75904783)

	ROM_REGION(0x040000, REGION_USER1, 0)	/* 1 bit per byte of CPU RAM - used for parity check emulation */

ROM_END

ROM_START( lisa210 )
	ROM_REGION16_BE(0x204000,REGION_CPU1, 0)	/* 68k rom and ram */
	ROM_LOAD16_BYTE( "booth.hi", 0x000000, 0x2000, 0xadfd4516)
	ROM_LOAD16_BYTE( "booth.lo", 0x000001, 0x2000, 0x546d6603)

#if 1
	ROM_REGION(0x2000,REGION_CPU2, 0)		/* 6504 RAM and ROM */
	ROM_LOAD( "io88.rom", 0x1000, 0x1000, 0xe343fe74)
#else
	ROM_REGION(0x2000,REGION_CPU2, 0)		/* 6504 RAM and ROM */
	ROM_LOAD( "io88800k.rom", 0x1000, 0x1000, 0x8c67959a)
#endif

	ROM_REGION(0x100,REGION_GFX1, 0)		/* video ROM (includes S/N) */
	ROM_LOAD( "vidstate.rom", 0x00, 0x100, 0x75904783)

	ROM_REGION(0x040000, REGION_USER1, 0)	/* 1 bit per byte of CPU RAM - used for parity check emulation */

ROM_END

ROM_START( macxl )
	ROM_REGION16_BE(0x204000,REGION_CPU1, 0)	/* 68k rom and ram */
	ROM_LOAD16_BYTE( "booth.hi", 0x000000, 0x2000, 0xadfd4516)
	ROM_LOAD16_BYTE( "booth.lo", 0x000001, 0x2000, 0x546d6603)

#if 1
	ROM_REGION(0x2000,REGION_CPU2, 0)		/* 6504 RAM and ROM */
	ROM_LOAD( "io88.rom", 0x1000, 0x1000, 0xe343fe74)
#else
	ROM_REGION(0x2000,REGION_CPU2, 0)		/* 6504 RAM and ROM */
	ROM_LOAD( "io88800k.rom", 0x1000, 0x1000, 0x8c67959a)
#endif

	ROM_REGION(0x100,REGION_GFX1, 0)		/* video ROM (includes S/N) */
	ROM_LOAD( "vidstate.rom", 0x00, 0x100, 0x75904783)

	ROM_REGION(0x040000, REGION_USER1, 0)	/* 1 bit per byte of CPU RAM - used for parity check emulation */

ROM_END

/* Lisa should eventually support floppies, hard disks, etc. */
static const struct IODevice io_lisa2[] = {
	{
		IO_FLOPPY,			/* type */
		1,					/* count */
		"img\0image\0",		/* file extensions */
		IO_RESET_NONE,		/* reset if file changed */
        NULL,               /* id */
		lisa_floppy_init,	/* init */
		lisa_floppy_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

#define io_lisa210 io_lisa2 /* actually, there is an additionnal 10 meg HD, but it is not implemented... */
#define io_macxl io_lisa210

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
COMPX( 1984, lisa2,    0,        lisa,     lisa,	 lisa2,    "Apple Computer",  "Lisa2", GAME_NOT_WORKING )
COMPX( 1984, lisa210,  lisa2,    lisa210,  lisa,	 lisa210,  "Apple Computer",  "Lisa2/10", GAME_NOT_WORKING )
COMPX( 1985, macxl,    lisa2,    macxl,    lisa,	 mac_xl,   "Apple Computer",  "Macintosh XL", GAME_NOT_WORKING )

