/***************************************************************************
	Spectrum memory map

	CPU:
		0000-3fff ROM
		4000-ffff RAM

Interrupts:

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int  spectrum_rom_load(void);
extern int  spectrum_rom_id(const char *name, const char * gamename);
extern int  load_snap(void);

extern int  spectrum_vh_start(void);
extern void spectrum_vh_stop(void);
extern void spectrum_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern void spectrum_init_machine(void);

extern void spectrum_characterram_w(int offset,int data);
extern int  spectrum_characterram_r(int offset);
extern void spectrum_colorram_w(int offset,int data);
extern int  spectrum_colorram_r(int offset);

int spectrum_port_fefe_r (int offset) {
	return(readinputport(0));
}

int spectrum_port_fdfe_r (int offset) {
	return(readinputport(1));
}

int spectrum_port_fbfe_r (int offset) {
	return(readinputport(2));
}

int spectrum_port_f7fe_r (int offset) {
	return(readinputport(3));
}

int spectrum_port_effe_r (int offset) {
	return(readinputport(4));
}

int spectrum_port_dffe_r (int offset) {
	return(readinputport(5));
}

int spectrum_port_bffe_r (int offset) {
	return(readinputport(6));
}

int spectrum_port_7ffe_r (int offset) {
	return(readinputport(7));
}


static struct MemoryReadAddress spectrum_readmem[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_r },
	{ 0x5800, 0x5aff, spectrum_colorram_r },
	{ 0x5b00, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spectrum_writemem[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_w },
	{ 0x5800, 0x5aff, spectrum_colorram_w },
	{ 0x5b00, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort spectrum_readport[] = {
	{ 0xfefe, 0xfefe, spectrum_port_fefe_r },
	{ 0xfdfe, 0xfdfe, spectrum_port_fdfe_r },
	{ 0xfbfe, 0xfbfe, spectrum_port_fbfe_r },
	{ 0xf7fe, 0xf7fe, spectrum_port_f7fe_r },
	{ 0xeffe, 0xeffe, spectrum_port_effe_r },
	{ 0xdffe, 0xdffe, spectrum_port_dffe_r },
	{ 0xbffe, 0xbffe, spectrum_port_bffe_r },
	{ 0x7ffe, 0x7ffe, spectrum_port_7ffe_r },
	{ -1 }
};

static struct GfxLayout spectrum_charlayout = {
	8,8,
	256,
	1,                      /* 1 bits per pixel */

	{ 0 },                  /* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8		        /* every char takes 1 consecutive byte */
};

static struct GfxDecodeInfo spectrum_gfxdecodeinfo[] = {
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( spectrum_input_ports )
	PORT_START /* 0xFEFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_RSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "Z",     KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "X",     KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "C",     KEYCODE_C,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "V",     KEYCODE_V,           IP_JOY_NONE )

	PORT_START /* 0xFDFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "A",      KEYCODE_A,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "S",      KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "D",      KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "F",      KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "G",      KEYCODE_G,           IP_JOY_NONE )

	PORT_START /* 0xFBFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "Q",      KEYCODE_Q,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "W",      KEYCODE_W,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "E",      KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "R",      KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "T",      KEYCODE_T,           IP_JOY_NONE )

	PORT_START /* 0xF7FE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "1",      KEYCODE_1,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2",      KEYCODE_2,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "3",      KEYCODE_3,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "4",      KEYCODE_4,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "5",      KEYCODE_5,           IP_JOY_NONE )

	PORT_START /* 0xEFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "0",      KEYCODE_0,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "9",      KEYCODE_9,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "8",      KEYCODE_8,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "7",      KEYCODE_7,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "6",      KEYCODE_6,           IP_JOY_NONE )

	PORT_START /* 0xDFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "P",      KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "O",      KEYCODE_O,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "I",      KEYCODE_I,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "U",      KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "Y",      KEYCODE_Y,           IP_JOY_NONE )

	PORT_START /* 0xBFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "ENTER",  KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "L",      KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "K",      KEYCODE_K,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "J",      KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "H",      KEYCODE_H,           IP_JOY_NONE )

	PORT_START /* 0x7FFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SPACE",     KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "SYM SHFT",  KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "M",         KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "N",         KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "B",         KEYCODE_B,        IP_JOY_NONE )
INPUT_PORTS_END

static unsigned char spectrum_palette[16*3] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xcf,
	0xcf, 0x00, 0x00, 0xcf, 0x00, 0xcf,
	0x00, 0xcf, 0x00, 0x00, 0xcf, 0xcf,
	0xcf, 0xcf, 0x00, 0xcf, 0xcf, 0xcf,

	0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
	0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
};

static unsigned short spectrum_colortable[128*2] = {
	0,0 , 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0 , 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0 , 2,1, 2,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	3,0 , 3,1, 3,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	4,0 , 4,1, 4,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	5,0 , 5,1, 5,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	6,0 , 6,1, 6,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	7,0 , 7,1, 7,2, 1,3, 1,4, 1,5, 1,6, 1,7,

	8,8  , 8,9,  8,10,  8 ,11, 8,12,  8,13, 8,14, 8,15,
	9,8  , 9,9,  9,10,  9 ,11, 9,12,  9,13, 9,14, 9,15,
	10,8 , 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8 , 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8 , 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8 , 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8 , 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8 , 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};

static struct MachineDriver spectrum_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,        /* 3.5 Mhz ? */
			0,
			spectrum_readmem,spectrum_writemem,
			spectrum_readport,0,
			interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
	spectrum_init_machine,
	0,

	/* video hardware */
	32*8,                                /* screen width */
	24*8,                                /* screen height */
	{ 0, 32*8-1, 0, 24*8-1},             /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 128,                             /* colors used for the characters */
	0,                                   /* convert color prom */

	VIDEO_TYPE_RASTER,
	0,
	spectrum_vh_start,
	spectrum_vh_stop,
	spectrum_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum_rom)
	ROM_REGION(0x10000)
	//ROM_LOAD("spectrum.rom", 0x0000, 0x4000, 0x6561e6a7)
	ROM_LOAD("spectrum.rom", 0x0000, 0x4000, 0xddee531f)
ROM_END

struct GameDriver spectrum_driver =
{
	__FILE__,
	0,
	"spectrum",
	"ZX-Spectrum 48k",
	"1982",
	"Sinclair Research",
	"Allard van der Bas [MESS driver]",
	GAME_COMPUTER,
	&spectrum_machine_driver,
	0,

	spectrum_rom,
	0,	/* spectrum_rom_load, */
	0,	/* spectrum_rom_id, */
	0,	/* 1, */		/* number of ROM slots */
	0,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	0,                      /* number of cassette drives supported */
	0, 0,
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	spectrum_input_ports,

	0,                         /* color_prom */
	spectrum_palette,          /* color palette */
	spectrum_colortable,       /* color lookup table */

	ORIENTATION_DEFAULT,    /* orientation */

	0, 0,
};

