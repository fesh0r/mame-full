/***************************************************************************
	microbee.c

	system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

	Brett Selwood, Andrew Davies (technical assistance)

	Microbee memory map (preliminary)

		0000-7FFF RAM
		8000-BFFF SYSTEM roms (bas522a.rom, bas522b.rom)
		C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
		E000-EFFF Telcom (tecl321.rom; optional)
		F000-F7FF Video RAM
		F800-FFFF PCG RAM (graphics), Colour RAM (banked)

	Microbee 56KB ROM memory map (preliminary)

		0000-DFFF RAM
		E000-EFFF ROM 56kb.rom CP/M bootstrap loader
		F000-F7FF Video RAM
		F800-FFFF PCG RAM (graphics), Colour RAM (banked)

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/generic.h"
#include "includes/wd179x.h"
#include "machine/mbee.h"
#include "includes/basicdsk.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, mbee_videoram_r },
	{ 0xf800, 0xffff, mbee_pcg_color_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_RAM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, mbee_videoram_w, &pcgram, &videoram_size },
	{ 0xf800, 0xffff, mbee_pcg_color_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress readmem_56k[] =
{
	{ 0x0000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, mbee_videoram_r },
	{ 0xf800, 0xffff, mbee_pcg_color_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_56k[] =
{
	{ 0x0000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, mbee_videoram_w, &pcgram, &videoram_size },
	{ 0xf800, 0xffff, mbee_pcg_color_w },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x00, 0x03, mbee_pio_r },
	{ 0x08, 0x08, mbee_pcg_color_latch_r },
	{ 0x0a, 0x0a, mbee_color_bank_r },
	{ 0x0b, 0x0b, mbee_video_bank_r },
	{ 0x0c, 0x0c, m6545_status_r },
	{ 0x0d, 0x0d, m6545_data_r },
	{ 0x44, 0x44, wd179x_status_r },
	{ 0x45, 0x45, wd179x_track_r },
	{ 0x46, 0x46, wd179x_sector_r },
	{ 0x47, 0x47, wd179x_data_r },
	{ 0x48, 0x48, mbee_fdc_status_r },
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x03, mbee_pio_w },
	{ 0x08, 0x08, mbee_pcg_color_latch_w },
	{ 0x0a, 0x0a, mbee_color_bank_w },
	{ 0x0b, 0x0b, mbee_video_bank_w },
	{ 0x0c, 0x0c, m6545_index_w },
	{ 0x0d, 0x0d, m6545_data_w },
	{ 0x44, 0x44, wd179x_command_w },
	{ 0x45, 0x45, wd179x_track_w },
	{ 0x46, 0x46, wd179x_sector_w },
	{ 0x47, 0x47, wd179x_data_w },
	{ 0x48, 0x48, mbee_fdc_motor_w },
	{ -1 }
};

INPUT_PORTS_START( mbee )
	PORT_START /* IN0 KEY ROW 0 [000] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@ ",             KEYCODE_ASTERISK,   IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A ",             KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B ",             KEYCODE_B,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C ",             KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D ",             KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E ",             KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F ",             KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G ",             KEYCODE_G,          IP_JOY_NONE )
	PORT_START /* IN1 KEY ROW 1 [080] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H ",             KEYCODE_H,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I ",             KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J ",             KEYCODE_J,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K ",             KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L ",             KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M ",             KEYCODE_M,          IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N ",             KEYCODE_N,          IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O ",             KEYCODE_O,          IP_JOY_NONE )
	PORT_START /* IN2 KEY ROW 2 [100] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P ",             KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q ",             KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R ",             KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S ",             KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T ",             KEYCODE_T,          IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U ",             KEYCODE_U,          IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V ",             KEYCODE_V,          IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W ",             KEYCODE_W,          IP_JOY_NONE )
	PORT_START /* IN3 KEY ROW 3 [180] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X ",             KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y ",             KEYCODE_Y,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z ",             KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ ",             KEYCODE_OPENBRACE,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ ",            KEYCODE_BACKSLASH,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] ",             KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^ ",             KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Delete",         KEYCODE_DEL,        IP_JOY_NONE )
	PORT_START /* IN4 KEY ROW 4 [200] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 ",             KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",            KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"",           KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",            KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",            KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",            KEYCODE_5,          IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &",            KEYCODE_6,          IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 '",            KEYCODE_7,          IP_JOY_NONE )
	PORT_START /* IN5 KEY ROW 5 [280] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (",            KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 )",            KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; +",            KEYCODE_COLON,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": *",            KEYCODE_QUOTE,      IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",            KEYCODE_COMMA,      IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- =",            KEYCODE_MINUS,      IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",            KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",            KEYCODE_SLASH,      IP_JOY_NONE )
	PORT_START /* IN6 KEY ROW 6 [300] */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Escape",         KEYCODE_ESC,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace",      KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab",            KEYCODE_TAB,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Linefeed",       KEYCODE_HOME,       IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter",          KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Break",          KEYCODE_END,        IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space",          KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_START /* IN7 KEY ROW 7 [380] */
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Ctrl",           KEYCODE_LCONTROL,   IP_JOY_NONE )
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L-Shift",        KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R-Shift",        KEYCODE_RSHIFT,     IP_JOY_NONE )
	PORT_START /* IN8 extra keys */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Up)",           KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Down)",         KEYCODE_DOWN,       IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Left)",         KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Right)",        KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Insert)",       KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_RESETCPU, "Reset", KEYCODE_F3,IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_START /* IN9 tape control */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape start",     KEYCODE_F5,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape stop",      KEYCODE_F6,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape rewind",    KEYCODE_F7,         IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

struct GfxLayout mbee_charlayout =
{
	8,16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets triple height: use each line three times */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static struct GfxDecodeInfo mbee_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xf000, &mbee_charlayout, 0, 256},
	{ -1 } /* end of array */
};

static UINT8 palette[] = {
	0x00,0x00,0x00, /* black	*/
	0xf0,0x00,0x00, /* red		*/
	0x00,0xf0,0x00, /* green	*/
	0xf0,0xf0,0x00, /* yellow	*/
	0x00,0x00,0xf0, /* blue 	*/
	0xf0,0x00,0xf0, /* magenta	*/
	0x00,0xf0,0xf0, /* cyan 	*/
	0xf0,0xf0,0xf0, /* white	*/
	0x08,0x08,0x08, /* black	*/
	0xe0,0x08,0x08, /* red		*/
	0x08,0xe0,0x08, /* green	*/
	0xe0,0xe0,0x08, /* yellow	*/
	0x08,0x08,0xe0, /* blue 	*/
	0xe0,0x08,0xe0, /* magenta	*/
	0x08,0xe0,0xe0, /* cyan 	*/
	0xe0,0xe0,0xe0, /* white	*/
	0x10,0x10,0x10, /* black	*/
	0xd0,0x10,0x10, /* red		*/
	0x10,0xd0,0x10, /* green	*/
	0xd0,0xd0,0x10, /* yellow	*/
	0x10,0x10,0xd0, /* blue 	*/
	0xd0,0x10,0xd0, /* magenta	*/
	0x10,0xd0,0xd0, /* cyan 	*/
	0xd0,0xd0,0xd0, /* white	*/
	0x18,0x18,0x18, /* black	*/
	0xe0,0x18,0x18, /* red		*/
	0x18,0xe0,0x18, /* green	*/
	0xe0,0xe0,0x18, /* yellow	*/
	0x18,0x18,0xe0, /* blue 	*/
	0xe0,0x18,0xe0, /* magenta	*/
	0x18,0xe0,0xe0, /* cyan 	*/
	0xe0,0xe0,0xe0	/* white	*/
};

void mbee_init_palette(unsigned char *system_palette, unsigned short *system_colortable, const unsigned char *color_prom)
{
	int i;
	memcpy(system_palette, palette, sizeof(palette));
	for( i = 0; i < 256; i++ )
	{
		system_colortable[2*i+0] = i / 32;
		system_colortable[2*i+1] = i & 31;
	}
}

Z80_DaisyChain mbee_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0 }, /* CTC number 0 */
	{ 0, 0, 0, -1}		/* end mark */
};

static struct Speaker_interface speaker_interface = {
	1,			/* number of speakers */
	{ 75 }, 	/* mixing levels */
};

static struct Wave_interface wave_interface = {
	1,			/* number of waves */
	{ 25 }		/* mixing levels */
};

static struct MachineDriver machine_driver_mbee =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3375000,	/* 3.37500 Mhz */
			readmem,writemem,
			readport,writeport,
			mbee_interrupt,1,
			0,0,&mbee_daisy_chain
		},
	},
	50, 2500,  /* frames per second, vblank duration */
	1,
	mbee_init_machine,
	mbee_shutdown_machine,

	/* video hardware */
	70*8,							/* screen width (inc. blank/sync) */
	310,							/* screen height (inc. blank/sync) */
	{ 0*8, 70*8-1, 0, 19*16-1}, 	/* visible_area */
	mbee_gfxdecodeinfo, 			/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,	/* colors used for the characters */
	256 * 2,
	mbee_init_palette,				/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	mbee_vh_start,
	mbee_vh_stop,
	mbee_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			&speaker_interface
		},
		{
			SOUND_WAVE,
			&wave_interface
		}
	}
};

static struct MachineDriver machine_driver_mbee56k =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3375000,	/* 3.37500 Mhz */
			readmem_56k,writemem_56k,
			readport,writeport,
			mbee_interrupt,1,
			0,0,&mbee_daisy_chain
		},
	},
	50, 2500,  /* frames per second, vblank duration */
	1,
	mbee_init_machine,
	mbee_shutdown_machine,

	/* video hardware */
	70*8,							/* screen width (inc. blank/sync) */
	310,							/* screen height (inc. blank/sync) */
	{ 0*8, 70*8-1, 0, 19*16-1}, 	/* visible_area */
	mbee_gfxdecodeinfo, 			/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,	/* colors used for the characters */
	256 * 2,
	mbee_init_palette,				/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	mbee_vh_start,
	mbee_vh_stop,
	mbee_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			&speaker_interface
		},
		{
			SOUND_WAVE,
			&wave_interface
		}
	}
};

ROM_START( mbee )
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("bas522a.rom",  0x8000, 0x2000, 0x7896a696)
	ROM_LOAD("bas522b.rom",  0xa000, 0x2000, 0xb21d9679)
//	ROM_LOAD("edasm.rom",    0xc000, 0x2000, 0x1af1b3a9)
	ROM_LOAD("wbee12.rom",   0xc000, 0x2000, 0x0fc21cb5)
	ROM_LOAD("8x16.fnt",     0xf000, 0x1000, BADCRC(0x47a56a9a))

	ROM_REGION(0x1000,REGION_GFX1)
	/* videoram and colorram are remapped here */
ROM_END

ROM_START( mbee56k )
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("56kb.rom",     0xe000, 0x1000, 0x28211224)
	ROM_LOAD("8x16.fnt",     0xf000, 0x1000, BADCRC(0x47a56a9a))

	ROM_REGION(0x1000,REGION_GFX1)
	/* videoram and colorram are remapped here */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/
static const struct IODevice io_mbee[] = {
	{
		IO_CARTSLOT,			/* type */
		1,						/* count */
		"rom\0",                /* file extensions */
		IO_RESET_CPU,			/* reset if file changed */
		mbee_rom_id,			/* id */
		mbee_rom_load,			/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	IO_CASSETTE_WAVE(1,"wav\0",NULL,mbee_cassette_init,mbee_cassette_exit),
	{
		IO_FLOPPY,				/* type */
		4,						/* count */
		"dsk\0",                /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		basicdsk_floppy_id, 	/* id */
		basicdsk_floppy_init,	/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

#define io_mbee56k io_mbee

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMP( 1983, mbee,	  0,		mbee,	  mbee, 	0,		  "Microbee Systems",  "Microbee 32K" )
COMP( 1983, mbee56k,  mbee, 	mbee56k,  mbee, 	0,		  "Microbee Systems",  "Microbee 56K (CP/M)" )

