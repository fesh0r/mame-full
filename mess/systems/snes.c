/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.
  Sound emulation currently consists of the SPC700 and that's about it. Without
  the DSP being emulated, there's no sound even if the code is being executed.
  I need to figure out how to get the 65816 and the SPC700 to stay in sync.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add sound emulation. Currently the SPC700 is emulated, but that's it.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Add support for fullgraphic mode(partially done).
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58Mhz at the appropriate time.
    - I'm sure there's lots more ...

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"
#include "devices/cartslot.h"
#include "inputx.h"

static MEMORY_READ_START( snes_readmem )
	{ 0x000000, 0x2fffff, snes_r_bank1 },	/* I/O and ROM (repeats for each bank) */
	{ 0x300000, 0x3fffff, snes_r_bank2 },	/* I/O and ROM (repeats for each bank) */
	{ 0x400000, 0x5fffff, snes_r_bank3 },	/* ROM (and reserved in Mode 20) */
	{ 0x600000, 0x6fffff, MRA_NOP },		/* Reserved */
	{ 0x700000, 0x77ffff, snes_r_sram },	/* 256KB Mode 20 save ram + reserved from 0x8000 - 0xffff */
	{ 0x780000, 0x7dffff, MRA_NOP },		/* Reserved */
	{ 0x7e0000, 0x7fffff, MRA_RAM },		/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	{ 0x800000, 0xffffff, snes_r_bank4 },	/* Mirror and ROM */
MEMORY_END

static MEMORY_WRITE_START( snes_writemem )
	{ 0x000000, 0x2fffff, snes_w_bank1 },	/* I/O and ROM (repeats for each bank) */
	{ 0x300000, 0x3fffff, snes_w_bank2 },	/* I/O and ROM (repeats for each bank) */
	{ 0x400000, 0x5fffff, MWA_ROM },		/* ROM (and reserved in Mode 20) */
	{ 0x600000, 0x6fffff, MWA_NOP },		/* Reserved */
	{ 0x700000, 0x77ffff, MWA_RAM },		/* 256KB Mode 20 save ram + reserved from 0x8000 - 0xffff */
	{ 0x780000, 0x7dffff, MWA_NOP },		/* Reserved */
	{ 0x7e0000, 0x7fffff, MWA_RAM },		/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	{ 0x800000, 0xffffff, snes_w_bank4 },	/* Mirror and ROM */
MEMORY_END

static MEMORY_READ_START( spc_readmem )
	{ 0x0000, 0x00ef, MRA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_io_r },			/* spc io */
	{ 0x0100, 0x7fff, MRA_RAM },			/* lower 32k ram continued */
	{ 0x8000, 0xffbf, MRA_RAM },			/* upper 32k ram */
	{ 0xffc0, 0xffff, spc_bank_r },			/* upper 32k ram continued or Initial Program Loader ROM */
MEMORY_END

static MEMORY_WRITE_START( spc_writemem )
	{ 0x0000, 0x00ef, MWA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_io_w },			/* spc io */
	{ 0x0100, 0x7fff, MWA_RAM },			/* lower 32k ram continued */
	{ 0x8000, 0xffbf, MWA_RAM },			/* upper 32k ram */
	{ 0xffc0, 0xffff, spc_bank_w },			/* upper 32k ram continued or Initial Program Loader ROM */
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
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT         | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START          | IPF_PLAYER1 )
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
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT         | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START          | IPF_PLAYER2 )
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
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT         | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START          | IPF_PLAYER3 )
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
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT         | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START          | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )

	PORT_START	/* IN 8 : Configuration */
	PORT_CONFNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_CONFSETTING(   0x0, DEF_STR( No )  )
	PORT_CONFSETTING(   0x1, DEF_STR( Yes ) )

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
	PORT_BIT_NAME( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON7  | IPF_PLAYER2,  "Toggle BG 1" )
	PORT_BIT_NAME( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON8  | IPF_PLAYER2,  "Toggle BG 2" )
	PORT_BIT_NAME( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON9  | IPF_PLAYER2,  "Toggle BG 3" )
	PORT_BIT_NAME( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON10 | IPF_PLAYER2,  "Toggle BG 4" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER3,  "Toggle Objects" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER3,  "Toggle Main/Sub" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON9 | IPF_PLAYER3,  "Toggle Back col" )
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON10 | IPF_PLAYER3, "Toggle Windows" )

	PORT_START	/* IN 11 : debug input */
	PORT_BIT_NAME( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON9,  "Pal prev" )
	PORT_BIT_NAME( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON10, "Pal next" )
	PORT_BIT_NAME( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER4, "Toggle Transparency" )
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
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, SNES_MAX_LINES_NTSC)

	MDRV_CPU_ADD_TAG("sound", SPC700, 2048000)	/* 2.048 Mhz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(spc_readmem, spc_writemem)
	MDRV_CPU_VBLANK_INT(NULL, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( snes )
	MDRV_MACHINE_STOP( snes )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(SNES_SCR_WIDTH * 2, SNES_SCR_HEIGHT * 2)
	MDRV_VISIBLE_AREA(0, SNES_SCR_WIDTH-1, 0, SNES_SCR_HEIGHT-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(257)
	MDRV_PALETTE_INIT( snes )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, snes_sound_interface)
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, SNES_MAX_LINES_PAL)
	MDRV_FRAMES_PER_SECOND(50)
MACHINE_DRIVER_END

SYSTEM_CONFIG_START(snes)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "smc\0sfc\0fig\0swc\0", NULL, NULL, device_load_snes_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(snes)
	ROM_REGION(0x1000000,       REGION_CPU1,  0)		/* 65C816 */
	ROM_REGION(SNES_VRAM_SIZE,  REGION_GFX1,  0)		/* VRAM */
	ROM_REGION(SNES_CGRAM_SIZE, REGION_USER1, 0)		/* CGRAM */
	ROM_REGION(SNES_OAM_SIZE,   REGION_USER2, 0)		/* OAM */
	ROM_REGION(0x10000,         REGION_CPU2,  0)		/* SPC700 */
	ROM_LOAD("spc700.rom", 0xFFC0, 0x40, CRC(38000B6B))	/* boot rom */
ROM_END

ROM_START(snespal)
	ROM_REGION(0x1000000,       REGION_CPU1,  0)		/* 65C816 */
	ROM_REGION(SNES_VRAM_SIZE,  REGION_GFX1,  0)		/* VRAM */
	ROM_REGION(SNES_CGRAM_SIZE, REGION_USER1, 0)		/* CGRAM */
	ROM_REGION(SNES_OAM_SIZE,   REGION_USER2, 0)		/* OAM */
	ROM_REGION(0x10000,         REGION_CPU2,  0)		/* SPC700 */
	ROM_LOAD("spc700.rom", 0xFFC0, 0x40, CRC(38000B6B))	/* boot rom */
ROM_END

/*     YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY     FULLNAME                                      FLAGS */
CONSX( 1989, snes,    0,      0,      snes,    snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (NTSC)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
CONSX( 1991, snespal, snes,   0,      snespal, snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
