/***************************************************************************

  gb.c

  Driver file to handle emulation of the Nintendo Gameboy.
  By:

  Hans de Goede               1998
  Anthony Kruize              2002

  Todo list:
  Done entries kept for historical reasons, besides that it's nice to see
  what is already done instead of what has to be done.

Priority:  Todo:                                                  Done:
  2        Replace Marat's  vidhrdw/gb.c  by Playboy code           *
  2        Clean & speed up vidhrdw/gb.c                            *
  2        Replace Marat's  Z80gb/Z80gb.c by Playboy code           *
  2        Transform Playboys Z80gb.c to big case method            *
  2        Clean up Z80gb.c                                         *
  2        Fix / optimise halt instruction                          *
  2        Do correct lcd stat timing                               In Progress
  2        Generate lcd stat interrupts                             *
  2        Replace Marat's code in machine/gb.c by Playboy code
  1        Check, and fix if needed flags bug which troubles ffa
  1        Save/restore battery backed ram                          *
  1        Add sound                                                In Progress
  0        Add supergb support
  0        Add palette editting, save & restore
  0        Add somekind of backdrop support
  0        Speedups if remotly possible

  2 = has to be done before first public release
  1 = should be added later on
  0 = bells and whistles

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/gb.h"
#include "includes/gb.h"

static MEMORY_READ_START (readmem)
	{ 0x0000, 0x3fff, MRA_ROM },			/* 16k fixed ROM BANK #0*/
	{ 0x4000, 0x7fff, MRA_BANK1 },			/* 16k switched ROM bank */
	{ 0x8000, 0x9fff, MRA_RAM },			/* 8k video ram */
	{ 0xa000, 0xbfff, MRA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xfe9f, MRA_RAM },			/* internal ram + echo + sprite Ram & IO */
	{ 0xfea0, 0xfeff, MRA_NOP },			/* Unusable */
	{ 0xff00, 0xff7f, gb_r_io },			/* gb io */
	{ 0xff80, 0xffff, MRA_RAM },			/* plain ram (high) */
MEMORY_END

static MEMORY_WRITE_START (writemem)
	{ 0x0000, 0x1fff, MWA_ROM },			/* plain rom (should really be RAM enable */
	{ 0x2000, 0x3fff, gb_rom_bank_select },	/* ROM bank select */
	{ 0x4000, 0x5fff, gb_ram_bank_select },	/* RAM bank select */
	{ 0x6000, 0x7fff, gb_mem_mode_select },	/* RAM/ROM mode select */
	{ 0x8000, 0x9fff, MWA_RAM },			/* plain ram */
	{ 0xa000, 0xbfff, MWA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xfe9f, MWA_RAM, &videoram, &videoram_size },	/* video & sprite ram */
	{ 0xfea0, 0xfeff, MWA_NOP },			/* unusable */
	{ 0xff00, 0xff7f, gb_w_io },			/* gb io */
	{ 0xff80, 0xfffe, MWA_RAM },			/* plain ram (high) */
	{ 0xffff, 0xffff, gb_w_ie },			/* gb io (interrupt enable) */
MEMORY_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

INPUT_PORTS_START( gameboy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1       )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2       )
	/*PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "Select", KEYCODE_LSHIFT, IP_JOY_DEFAULT ) */
	/*PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "Start",  KEYCODE_Z,      IP_JOY_DEFAULT ) */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Select", KEYCODE_5, IP_JOY_DEFAULT )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Start",  KEYCODE_1, IP_JOY_DEFAULT )
INPUT_PORTS_END

static unsigned char palette[] =
{
/* Simple black and white palette */
/*	0xFF,0xFF,0xFF,
	0xB0,0xB0,0xB0,
	0x60,0x60,0x60,
	0x00,0x00,0x00 */

/* Possibly needs a little more green in it */
	0xFF,0xFB,0x87,
	0xB1,0xAE,0x4E,
	0x84,0x80,0x4E,
	0x4E,0x4E,0x4E
};

static unsigned short gb_colortable[] =
{
	0,1,2,3,	/* Background colours */
	0,1,2,3,	/* Sprite 0 colours */
	0,1,2,3,	/* Sprite 1 colours */
	0,1,2,3,	/* Window colours */
};

/* Initialise the palette */
static PALETTE_INIT( gb )
{
	int i;
	for (i = 0; i < (sizeof(palette) / 3); i++)
		palette_set_color(i, palette[i*3+0], palette[i*3+1], palette[i*3+2]);
	memcpy(colortable,gb_colortable,sizeof(gb_colortable));
}

static struct CustomSound_interface gameboy_sound_interface = {
	gameboy_sh_start,
	0,
	0
};

static MACHINE_DRIVER_START( gameboy )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 154 * 3)	/* 1 int each scanline ! */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_INTERLEAVE(1)
	MDRV_MACHINE_INIT( gb )
	MDRV_MACHINE_STOP( gb )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( gb )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(160, 144)
	MDRV_VISIBLE_AREA(0, 160 - 1, 0, 144 - 1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(palette) / sizeof(palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(16)
	MDRV_PALETTE_INIT(gb)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, gameboy_sound_interface)
MACHINE_DRIVER_END


static const struct IODevice io_gameboy[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"gb\0gmb\0cgb\0gbc\0sgb\0",		/* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		0,
		gb_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define rom_gameboy NULL

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY     FULLNAME */
CONSX( 1990, gameboy,  0,        gameboy,  gameboy,  0,        "Nintendo", "GameBoy", GAME_IMPERFECT_SOUND )

