/***************************************************************************
Jupiter Ace memory map

	CPU: Z80
		0000-1fff ROM
		2000-22ff unused
		2300-23ff RAM (cassette buffer)
		2400-26ff RAM (screen)
		2700-27ff RAM (edit buffer)
		2800-2bff unused
		2c00-2fff RAM (char set)
		3000-3bff unused
		3c00-ffff RAM (user area)

Interrupts:

	IRQ:
		50Hz vertical sync

Ports:

	Out 0xfe:
		Tape and buzzer

	In 0xfe:
		Keyboard input and buzzer
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"

/* prototypes */

extern void jupiter_init_machine (void);
extern void jupiter_stop_machine (void);

extern int jupiter_rom_load (void);
extern int jupiter_rom_id (const char *name, const char *gamename);

extern int jupiter_vh_start (void);
extern void jupiter_vh_stop (void);
extern void jupiter_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
extern void jupiter_vh_scrnram_w (int offset, int data);
extern int jupiter_vh_scrnram_r (int offset);
extern void jupiter_vh_charram_w (int offset, int data);
extern int jupiter_vh_charram_r (int offset);

extern int jupiter_sh_start (const struct MachineSound *driver);
extern void jupiter_sh_stop (void);

extern unsigned char *jupiter_charram;
extern int jupiter_charram_size;

/* functions */

int jupiter_port_fefe_r (int offset)
{
	return (readinputport (0));
}

int jupiter_port_fdfe_r (int offset)
{
	return (readinputport (1));
}

int jupiter_port_fbfe_r (int offset)
{
	return (readinputport (2));
}

int jupiter_port_f7fe_r (int offset)
{
	return (readinputport (3));
}

int jupiter_port_effe_r (int offset)
{
	return (readinputport (4));
}

int jupiter_port_dffe_r (int offset)
{
	return (readinputport (5));
}

int jupiter_port_bffe_r (int offset)
{
	return (readinputport (6));
}

int jupiter_port_7ffe_r (int offset)
{
	UINT16	pcreg;
	pcreg = cpu_get_reg ( Z80_PC );
	//fprintf (stdout, "%04X: IN (0x%X).\n", pcreg, offset);
	return (readinputport (7));
}

void jupiter_port_fe_w (int offset, int data)
{
	UINT16	pcreg;
	pcreg = cpu_get_reg ( Z80_PC );
	//fprintf (stdout, "%04X: OUT (0x%X), %d.\n", pcreg, offset, data);
	// play sound
}

/* port i/o functions */

static struct IOReadPort jupiter_readport[] =
{
	{0xfefe, 0xfefe, jupiter_port_fefe_r},
	{0xfdfe, 0xfdfe, jupiter_port_fdfe_r},
	{0xfbfe, 0xfbfe, jupiter_port_fbfe_r},
	{0xf7fe, 0xf7fe, jupiter_port_f7fe_r},
	{0xeffe, 0xeffe, jupiter_port_effe_r},
	{0xdffe, 0xdffe, jupiter_port_dffe_r},
	{0xbffe, 0xbffe, jupiter_port_bffe_r},
	{0x7ffe, 0x7ffe, jupiter_port_7ffe_r},
	{-1}
};

static struct IOWritePort jupiter_writeport[] =
{
	{0x00fe, 0xfffe, jupiter_port_fe_w},
	{-1}
};

/* memory w/r functions */

static struct MemoryReadAddress jupiter_readmem[] =
{
	{0x0000, 0x1fff, MRA_ROM},
	{0x2000, 0x22ff, MRA_NOP},
	{0x2300, 0x23ff, MRA_RAM},
	{0x2400, 0x26ff, videoram_r},
	{0x2700, 0x27ff, MRA_RAM},
	{0x2800, 0x2bff, MRA_NOP},
	{0x2c00, 0x2fff, MRA_RAM},	/* char RAM */
	{0x3000, 0x3bff, MRA_NOP},
	{0x3c00, 0xffff, MRA_RAM},
	{-1}
};

static struct MemoryWriteAddress jupiter_writemem[] =
{
	{0x0000, 0x1fff, MWA_ROM},
	{0x2000, 0x22ff, MWA_NOP},
	{0x2300, 0x23ff, MWA_RAM},
	{0x2400, 0x26ff, videoram_w, &videoram, &videoram_size},
	{0x2700, 0x27ff, MWA_RAM},
	{0x2800, 0x2bff, MWA_NOP},
	{0x2c00, 0x2fff, jupiter_vh_charram_w, &jupiter_charram, &jupiter_charram_size},
	{0x3000, 0x3bff, MWA_NOP},
	{0x3c00, 0xffff, MWA_RAM},
	{-1}
};

/* graphics output */

struct GfxLayout jupiter_charlayout =
{
	8, 8,	/* 8x8 characters */
	128,	/* 128 characters */
	1,		/* 1 bits per pixel */
	{0},	/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8 	/* each character takes 8 consecutive bytes */
};

static struct GfxDecodeInfo jupiter_gfxdecodeinfo[] =
{
	{0, 0x2c00, &jupiter_charlayout, 0, 2},
	{-1}							   /* end of array */
};

static unsigned char jupiter_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff,
};

static unsigned short jupiter_colortable[2 * 2] =
{
	1, 0,
	0, 1,
};

static void jupiter_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, jupiter_palette, sizeof (jupiter_palette));
	memcpy (sys_colortable, jupiter_colortable, sizeof (jupiter_colortable));
}

/* keyboard input */

INPUT_PORTS_START (jupiter)
	PORT_START							   /* 0xFEFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "SYM SHFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "C", KEYCODE_C, IP_JOY_NONE)

	PORT_START							   /* 0xFDFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START							   /* 0xFBFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START							   /* 0xF7FE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "5", KEYCODE_5, IP_JOY_NONE)

	PORT_START							   /* 0xEFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "6", KEYCODE_6, IP_JOY_NONE)

	PORT_START							   /* 0xDFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START							   /* 0xBFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "H", KEYCODE_H, IP_JOY_NONE)

	PORT_START							   /* 0x7FFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "V", KEYCODE_V, IP_JOY_NONE)
INPUT_PORTS_END

/* Sound output */

/* static	struct	CustomSound_interface jupiter_sh_interface = {
	jupiter_sh_start,
	jupiter_sh_stop,
	0
}; */

/* machine definition */

static	struct MachineDriver jupiter_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			3250000,				   /* 3.25 Mhz */
			jupiter_readmem, jupiter_writemem,
			jupiter_readport, jupiter_writeport,
			interrupt, 1,
		},
	},
	50, 2500,			   /* frames per second, vblank duration */
	1,
	jupiter_init_machine,
	jupiter_stop_machine,

	/* video hardware */
	32 * 8,							   /* screen width */
	24 * 8,							   /* screen height */
	{0, 32 * 8 - 1, 0, 24 * 8 - 1},	   /* visible_area */
	jupiter_gfxdecodeinfo,			   /* graphics decode info */
	2, 4,							   /* colors used for the characters */
	jupiter_init_palette,			   /* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	jupiter_vh_start,
	jupiter_vh_stop,
	jupiter_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	//{
		//{ SOUND_CUSTOM, &jupiter_sh_interface },
	//}
};

ROM_START (jupiter)
ROM_REGIONX (0x10000, REGION_CPU1)
ROM_LOAD ("jupiter.rom", 0x0000, 0x2000, 0xe5b1f5f6)
ROM_END

struct GameDriver jupiter_driver =
{
	__FILE__,
	0,
	"jupiter",
	"Jupiter Ace 49k",
	"1981",
	"Cantab",
	"Paul Daniels [MESS driver]",
	0,
	&jupiter_machine_driver,
	0,
	rom_jupiter,
	jupiter_rom_load,
	jupiter_rom_id,
	0,			   /* default file extensions */
	1,			   /* number of ROM slots */
	0,			   /* number of floppy drives supported */
	0,			   /* number of hard drives supported */
	0,			   /* number of cassette drives supported */
	0, 0,
	0,			   /* pointer to sample names */
	0,			   /* sound_prom */
	input_ports_jupiter,
	0,			   /* color_prom */
	0,			   /* color palette */
	0,			   /* color lookup table */
	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */
	0, 0,
};
