/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is very preliminary right now.  Some games run, only a few are
  actually playable.  Video emulation is very basic at the moment, enough to
  get by with on most games though.
  Sound emulation currently consists of the SPC700 and that's about it. Without
  the DSP being emulated, there's no sound even if the code is being executed.
  I need to figure out how to get the 65816 and the SPC700 to stay in sync.

  Todo:
    - Emulate extra chips. (fx, dsp2 etc).
    - Add sound emulation. Currently the SPC700 is emulated, but that's it.
    - Add transparency(main/sub screens), windows etc to the video emulation.
    - Add support for Mode 7.
    - Fix up HDMA and possibly GDMA too.
    - Figure out how games determine if they are running on a PAL or NTSC
      system.
    - Handle more rom formats, like .fig, and maybe multi-part roms???
    - Figure out how to handle HIROMS.
    - Add support for running at 2.68Mhz and 3.58Mhz at appropriate times.
    - Add HBlank interrupts.
    - I'm sure there's lots more.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"

static MEMORY_READ_START( snes_readmem )
	{ 0x000000, 0x3fffff, snes_r_bank },	/* This section of RAM repeats heaps */
	{ 0x400000, 0x6fffff, MRA_ROM },		/* ROM */
	{ 0x700000, 0x7cffff, MRA_RAM },		/* SRAM - i think...? */

	/* For LoROM */
	{ 0x7d0000, 0x7dffff, MRA_RAM },		/* 4kb SRAM - or is it? */
	/* For HiROM */
/*	{ 0x7d0000, 0x7dffff, MRA_ROM },*/		/* ROM */

	{ 0x7e0000, 0x7e1fff, MRA_RAM },		/* 8kb Shadow RAM - shadowed in banks 00-3f */
	{ 0x7e2000, 0x7fffff, MRA_RAM },		/* System RAM */
	{ 0x800000, 0xefffff, snes_r_mirror },	/* Mirror of 0x000000-0x6fffff */
	{ 0xfd0000, 0xfdffff, MRA_RAM },		/* 4kb SRAM */

	/* For LoROM */
	{ 0xfe0000, 0xfe1fff, MRA_RAM },		/* Shadow RAM  - is it the same as the previous shadow ram? */
	{ 0xfe2000, 0xffffff, MRA_RAM },		/* System RAM */
	/* For HiROM */
/*	{ 0xfe0000, 0xffffff, MRA_ROM },*/		/* 64k ROM chunk */
MEMORY_END

static MEMORY_WRITE_START( snes_writemem )
	{ 0x000000, 0x3fffff, snes_w_bank },	/* This section of RAM repeats heaps */
	{ 0x400000, 0x6fffff, MWA_ROM },		/* ROM */
	{ 0x700000, 0x7cffff, MWA_RAM },		/* SRAM - i think...? */

	/* For LoROM */
	{ 0x7d0000, 0x7dffff, MWA_RAM },		/* 4kb SRAM  - or is it? */
	/* For HiROM */
/*	{ 0x7d0000, 0x7dffff, MWA_ROM },*/		/* ROM */

	{ 0x7e0000, 0x7e1fff, MWA_RAM },		/* 8kb Shadow RAM - shadowed in banks 00-3f */
	{ 0x7e2000, 0x7fffff, MWA_RAM },		/* System RAM */
	{ 0x800000, 0xefffff, snes_w_mirror },	/* Mirror of 0x000000-0x6fffff */
	{ 0xfd0000, 0xfdffff, MWA_RAM },		/* 4kb SRAM */

	/* For LoROM */
	{ 0xfe0000, 0xfe1fff, MWA_RAM },		/* Shadow RAM  - is it the same as the previous shadow ram? */
	{ 0xfe2000, 0xffffff, MWA_RAM },		/* System RAM */
	/* For HiROM */
/*	{ 0xfe0000, 0xffffff, MWA_ROM },*/		/* 64k ROM chunk */
MEMORY_END

static MEMORY_READ_START( spc_readmem )
	{ 0x0000, 0x00ef, MRA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_r_io },			/* spc io */
	{ 0x0100, 0x7fff, MRA_RAM },			/* lower 32k ram */
	{ 0x8000, 0xffbf, MRA_NOP },			/* Not connected in SNES - normally upper 32k ram */
	{ 0xffc0, 0xffff, MRA_ROM },			/* Initial program loader ROM */
MEMORY_END

static MEMORY_WRITE_START( spc_writemem )
	{ 0x0000, 0x00ef, MWA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_w_io },			/* spc io */
	{ 0x0100, 0x7fff, MWA_RAM },			/* lower 32k ram */
	{ 0x8000, 0xffbf, MWA_NOP },			/* Not connected in SNES - normally upper 32k ram  */
	{ 0xffc0, 0xffff, MWA_ROM },			/* Initial program loader ROM */
MEMORY_END

INPUT_PORTS_START( snes )

	PORT_START  /* IN 0 : Joypad 1 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER1, "P1 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER1, "P1 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER1, "P1 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER1, "P1 Button R" )
	PORT_START  /* IN 1 : Joypad 1 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER1, "P1 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER1, "P1 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SELECT1, "P1 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START1,  "P1 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START  /* IN 2 : Joypad 2 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER2, "P2 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER2, "P2 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER2, "P2 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER2, "P2 Button R" )
	PORT_START  /* IN 3 : Joypad 2 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER2, "P2 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER2, "P2 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SELECT2, "P2 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START2,  "P2 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START  /* IN 4 : Joypad 3 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER3, "P3 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER3, "P3 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER3, "P3 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER3, "P3 Button R" )
	PORT_START  /* IN 5 : Joypad 3 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER3, "P3 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER3, "P3 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SELECT3, "P3 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START3,  "P3 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )

	PORT_START  /* IN 6 : Joypad 4 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER4, "P4 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER4, "P4 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER4, "P4 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER4, "P4 Button R" )
	PORT_START  /* IN 7 : Joypad 4 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER4, "P4 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER4, "P4 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SELECT4, "P4 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START4,  "P4 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )

	PORT_START	/* IN 8 : Internal switches */
	PORT_DIPNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_DIPSETTING(   0x0, DEF_STR( No )  )
	PORT_DIPSETTING(   0x1, DEF_STR( Yes ) )

#ifdef MAME_DEBUG
	PORT_START	/* IN 9 : debug switches */
	PORT_DIPNAME( 0x3, 0x0, "Browse tiles" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, "2bpl"  )
	PORT_DIPSETTING(   0x2, "4bpl"  )
	PORT_DIPSETTING(   0x3, "8bpl"  )
	PORT_DIPNAME( 0xc, 0x0, "Browse maps" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, "2bpl"  )
	PORT_DIPSETTING(   0x8, "4bpl"  )
	PORT_DIPSETTING(   0xc, "8bpl"  )

	PORT_START	/* IN 10 : debug switches */
	PORT_DIPNAME( 0x1, 0x1, "BG 1" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, DEF_STR( On ) )
	PORT_DIPNAME( 0x2, 0x2, "BG 2" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x2, DEF_STR( On ) )
	PORT_DIPNAME( 0x4, 0x4, "BG 3" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, DEF_STR( On ) )
	PORT_DIPNAME( 0x8, 0x8, "BG 4" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x8, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Objects" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Windows" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x20, DEF_STR( On ) )
#endif
INPUT_PORTS_END

static struct CustomSound_interface snes_sound_interface =
{ snes_sh_start, 0, 0 };

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static PALETTE_INIT( snes )
{
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1F) << 3;
		g = ((i >> 5) & 0x1F) << 3;
		b = ((i >> 10) & 0x1F) << 3;
		palette_set_color( i, r, g, b );
	}

	/* The colortable can be black */
	for( i = 0; i < 256; i++ )
		colortable[i] = 0;
}

static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", G65816, 2680000)	/* 2.68Mhz, also 3.58Mhz */
	MDRV_CPU_MEMORY(snes_readmem, snes_writemem)
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, 262)

	MDRV_CPU_ADD_TAG("sound", SPC700, 2048000)	/* 2.048 Mhz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(spc_readmem, spc_writemem)
	MDRV_CPU_VBLANK_INT(NULL, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( snes )
	MDRV_MACHINE_STOP( snes )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(SNES_SCR_WIDTH, SNES_SCR_HEIGHT)
	MDRV_VISIBLE_AREA(0, SNES_SCR_WIDTH-1, 0, SNES_SCR_HEIGHT-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(257)
	MDRV_PALETTE_INIT( snes )

	MDRV_SOUND_ADD(CUSTOM, snes_sound_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_FRAMES_PER_SECOND(50)
MACHINE_DRIVER_END

static const struct IODevice io_snes[] =
{
	{
		IO_CARTSLOT,        /* type */
		1,                  /* count */
		"smc,sfc,fig\0",    /* file extensions */
		IO_RESET_ALL,       /* reset if file changed */
		OSD_FOPEN_DUMMY,	/* open mode */
		0,
		snes_load_rom,      /* init */
		NULL,               /* exit */
		NULL,               /* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,               /* tell */
		NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL                /* output_chunk */
	},
	{ IO_END }
};

#define io_snespal  io_snes

SYSTEM_CONFIG_START(snes)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(snes)
	ROM_REGION(0x1000000, REGION_CPU1,  0)				/* 65C816 */
	ROM_REGION(0x20000,   REGION_GFX1,  0)				/* VRAM */
	ROM_REGION(0x202,     REGION_USER1, 0)				/* CGRAM */
	ROM_REGION(0x440,     REGION_USER2, 0)				/* OAM */
	ROM_REGION(0x10000,   REGION_CPU2,  0)				/* SPC700 */
	ROM_LOAD_OPTIONAL("spc700.rom", 0xFFC0, 0x40, 0x38000B6B)	/* boot rom */
ROM_END

ROM_START(snespal)
	ROM_REGION(0x1000000, REGION_CPU1,  0)				/* 65C816 */
	ROM_REGION(0x20000,   REGION_GFX1,  0)				/* VRAM */
	ROM_REGION(0x202,     REGION_USER1, 0)				/* CGRAM */
	ROM_REGION(0x440,     REGION_USER2, 0)				/* OAM */
	ROM_REGION(0x10000,   REGION_CPU2,  0)				/* SPC700 */
	ROM_LOAD_OPTIONAL("spc700.rom", 0xFFC0, 0x40, 0x38000B6B)	/* boot rom */
ROM_END

/*     YEAR  NAME     PARENT  MACHINE  INPUT  INIT	CONFIG	COMPANY     FULLNAME                                      FLAGS */
CONSX( 1989, snes,    0,      snes,    snes,  0,	snes,	"Nintendo", "Super Nintendo Entertainment System (NTSC)", GAME_NOT_WORKING )
CONSX( 1989, snespal, snes,   snespal, snes,  0,	snes,	"Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_NOT_WORKING )
