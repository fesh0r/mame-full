/***************************************************************************

  gb.c

  Driver file to handle emulation of the Nintendo Gameboy.
  By:

  Hans de Goede               1998

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
  2        Do correct lcd stat timing
  2        Generate lcd stat interrupts
  2        Replace Marat's code in machine/gb.c by Playboy code
  1        Check, and fix if needed flags bug which troubles ffa
  1        Save/restore battery backed ram
           (urgent needed to play zelda ;)
  1        Add sound
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
#include "mess/machine/gb.h"

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },   /* 16k fixed ROM BANK #0*/
	{ 0x4000, 0x7fff, MRA_BANK1 }, /* 16k switched ROM bank */
	{ 0x8000, 0x9fff, MRA_RAM },   /* 8k video ram */
	{ 0xa000, 0xbfff, MRA_BANK2 }, /* 8k RAM bank (on cartridge) */
	{ 0xc000, 0xff03, MRA_RAM },   /* internal ram + echo + sprite Ram & IO */
	{ 0xff04, 0xff04, gb_r_divreg },    /* special case for the division reg */
	{ 0xff05, 0xff05, gb_r_timer_cnt }, /* special case for the timer count reg */
	{ 0xff06, 0xffff, MRA_RAM },   /* IO */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },            /* plain rom */
	{ 0x2000, 0x3fff, gb_rom_bank_select }, /* rom bank select */
	{ 0x4000, 0x5fff, gb_ram_bank_select }, /* ram bank select */
	{ 0x6000, 0x7fff, MWA_ROM },            /* plain rom */
	{ 0x8000, 0x9fff, MWA_RAM },            /* plain ram */
	{ 0xa000, 0xbfff, MWA_BANK2 },          /* banked (cartridge) ram */
	{ 0xc000, 0xfeff, MWA_RAM, &videoram, &videoram_size }, /* video & sprite ram */
	{ 0xff00, 0xffff, gb_w_io },	        /* gb io */
	{ -1 }	/* end of table */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

INPUT_PORTS_START( gb )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1       )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2       )
	//PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "select", KEYCODE_LSHIFT, IP_JOY_DEFAULT )
	//PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "start",  KEYCODE_Z,      IP_JOY_DEFAULT )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "select", KEYCODE_3, IP_JOY_DEFAULT )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "start",  KEYCODE_1, IP_JOY_DEFAULT )
INPUT_PORTS_END

static unsigned char palette[] =
{
  0xFF,0xFF,0xFF,        /* Background colours */
  0xB0,0xB0,0xB0,
  0x60,0x60,0x60,
  0x00,0x00,0x00,
};

static unsigned short colortable[] = {
    0,1,2,3,    /* Background colours */
    0,1,2,3,    /* Sprite 0 colours */
    0,1,2,3,    /* Sprite 1 colours */
    0,1,2,3,    /* Window colours */
};

/* Initialise the palette */
static void gb_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80GB,
			4194304,	  /* 4.194304 Mhz */
			readmem,writemem,0,0,
			gb_scanline_interrupt, 154 *3 /* 1 int each scanline ! */
		}
	},
	60, 0,	/* frames per second, vblank duration */
	1,
	gb_init_machine,
	NULL,	/* shutdown machine */

	/* video hardware (double size) */
	160, 144,
	{ 0, 160-1, 0, 144-1 },
	gfxdecodeinfo,
	(sizeof (palette))/sizeof(palette[0])/3,
	sizeof(colortable)/sizeof(colortable[0]),
	gb_init_palette,				/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	gb_vh_start,					/* vh_start */
    gb_vh_stop,                     /* vh_stop */
	gb_vh_screen_refresh,

	/* sound hardware */
	0,0,0,0,
};

static const char *gb_file_extensions[] = {
	"gb", NULL
};

struct GameDriver gameboy_driver =
{
	__FILE__,
	NULL,
	"gameboy",
	"GameBoy",
	"198?",
	"Nintendo",
	NULL,
	0,
	&machine_driver,
	NULL,				/* init driver */

	NULL,				/* ROM structure */
    gb_load_rom,
	gb_id_rom,
	gb_file_extensions,
	1,					/* number of ROM slots */
	0,					/* number of floppy drives supported */
	0,					/* number of hard drives supported */
	0,					/* number of cassette drives supported */
	0,					/* rom decoder */
	0,					/* opcode decoder */
	0,					/* pointer to sample names */
	0,					/* sound_prom */

	input_ports_gb,

	0,					/* color_prom */
	0,					/* color palette */
	0,					/* color lookup table */
	ORIENTATION_DEFAULT | GAME_NOT_WORKING | GAME_NO_SOUND,
	0,
	0
};
