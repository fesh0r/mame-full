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
		3c00-47ff RAM (standard)
		4800-87ff RAM (16K expansion)
		8800-ffff RAM (Expansion)

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

extern int jupiter_load_ace(int id);
extern void jupiter_exit_ace(int id);
extern int jupiter_load_tap(int id);
extern void jupiter_exit_tap(int id);

extern int jupiter_vh_start (void);
extern void jupiter_vh_stop (void);
extern void jupiter_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( jupiter_vh_charram_w );
extern unsigned char *jupiter_charram;
extern size_t jupiter_charram_size;

/* functions */

READ_HANDLER ( jupiter_port_fefe_r )
{
	return (readinputport (0));
}

READ_HANDLER ( jupiter_port_fdfe_r )
{
	return (readinputport (1));
}

READ_HANDLER ( jupiter_port_fbfe_r )
{
	return (readinputport (2));
}

READ_HANDLER ( jupiter_port_f7fe_r )
{
	return (readinputport (3));
}

READ_HANDLER ( jupiter_port_effe_r )
{
	return (readinputport (4));
}

READ_HANDLER ( jupiter_port_dffe_r )
{
	return (readinputport (5));
}

READ_HANDLER ( jupiter_port_bffe_r )
{
	return (readinputport (6));
}

READ_HANDLER ( jupiter_port_7ffe_r )
{
	speaker_level_w(0,0);
	return (readinputport (7));
}

WRITE_HANDLER ( jupiter_port_fe_w )
{
	speaker_level_w(0,1);
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
	{0x3c00, 0x47ff, MRA_RAM},
	{0x4800, 0x87ff, MRA_RAM},
	{0x8800, 0xffff, MRA_RAM},
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
	{0x3c00, 0x47ff, MWA_RAM},
	{0x4800, 0x87ff, MWA_RAM},
	{0x8800, 0xffff, MWA_RAM},
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
	{REGION_CPU1, 0x2c00, &jupiter_charlayout, 0, 2},
	{-1}							   /* end of array */
};

static unsigned char jupiter_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static unsigned short jupiter_colortable[] =
{
	0, 1,
	1, 0
};

static void jupiter_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, jupiter_palette, sizeof (jupiter_palette));
	memcpy (sys_colortable, jupiter_colortable, sizeof (jupiter_colortable));
}

/* keyboard input */

INPUT_PORTS_START (jupiter)
	PORT_START	/* 0: 0xFEFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYM SHFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)

	PORT_START	/* 1: 0xFDFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START	/* 2: 0xFBFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START	/* 3: 0xF7FE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)

	PORT_START	/* 4: 0xEFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)

	PORT_START	/* 5: 0xDFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START	/* 6: 0xBFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)

	PORT_START	/* 7: 0x7FFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_START	/* 8: machine config */
	PORT_DIPNAME( 0x03, 2, "RAM Size")
	PORT_DIPSETTING(0, "3Kb")
	PORT_DIPSETTING(1, "19Kb")
	PORT_DIPSETTING(2, "49Kb")
INPUT_PORTS_END

/* Sound output */

static struct Speaker_interface speaker_interface =
{
	1,			/* one speaker */
	{ 100 },	/* mixing levels */
	{ 0 },		/* optional: number of different levels */
	{ NULL }	/* optional: level lookup table */
};

/* machine definition */

static	struct MachineDriver machine_driver_jupiter =
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
	sizeof (jupiter_palette) / 3,
	sizeof (jupiter_colortable),	   /* colors used for the characters */
	jupiter_init_palette,			   /* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	jupiter_vh_start,
	jupiter_vh_stop,
	jupiter_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SPEAKER,
			&speaker_interface
        }
    }
};

ROM_START (jupiter)
ROM_REGION (0x10000, REGION_CPU1)
ROM_LOAD ("jupiter.rom", 0x0000, 0x2000, 0xe5b1f5f6)
ROM_END

static const struct IODevice io_jupiter[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"ace\0",            /* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		jupiter_load_ace,	/* init */
		jupiter_exit_ace,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
		IO_CASSETTE,		/* type */
		1,					/* count */
		"tap\0",            /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		jupiter_load_tap,	/* init */
		jupiter_exit_tap,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1981, jupiter,  0,		jupiter,  jupiter,	0,		  "Cantab",  "Jupiter Ace 49k" )
