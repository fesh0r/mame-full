/***************************************************************************
	zx.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

/* from machine/zx80.c */

extern void zx_init_driver(void);
extern void zx80_init_machine(void);
extern void zx81_init_machine(void);
extern void lambda_init_machine(void);
extern void zx_shutdown_machine(void);

extern int zx_rom_load(void);
extern int zx_rom_id(const char *name, const char * gamename);

extern int zx_io_r(int offs);
extern void zx_io_w(int offs, int data);

extern int zx_tape_get_bit(void);

/* from vidhrdw/zx80.c */
extern void zx_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static struct MemoryReadAddress readmem_zx80[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_zx80[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_zx81[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_zx81[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_lambda[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },	/* Lamba comes with 32K RAM */
	{ 0x8000, 0xffff, MRA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_lambda[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },	/* Lamba comes with 32K RAM */
	{ 0x8000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{0x0000,0xffff,zx_io_r},
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{0x0000,0xffff,zx_io_w},
    { -1 }
};

INPUT_PORTS_START( zx80 )
	PORT_START /* IN0 */
	PORT_DIPNAME( 0x80, 0x00, "32K RAM module" )
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x80, "yes")
	PORT_BIT(	  0x7e, 0x0f, IPT_UNUSED )
    PORT_BITX(    0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset",   KEYCODE_F3, IP_JOY_NONE )

	PORT_START /* IN1 KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",       KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z  ",         KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X  ",         KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C  ",         KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V  ",         KEYCODE_V,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN2 KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A  ",         KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S  ",         KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D  ",         KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F  ",         KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G  ",         KEYCODE_G,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN3 KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q  ",         KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W  ",         KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E  ",         KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R  ",         KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T  ",         KEYCODE_T,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN4 KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  EDIT",     KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  AND",      KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  THEN",     KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  TO",       KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  LEFT",     KEYCODE_5,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN5 KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0  RUBOUT",   KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  GRAPHICS", KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  RIGHT",    KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  UP",       KEYCODE_7,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  DOWN",     KEYCODE_6,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN6 KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P  PRINT",    KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O  POKE",     KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I  INPUT",    KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U  IF",       KEYCODE_U,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y  RETURN",   KEYCODE_Y,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN7 KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER",       KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L  ",         KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K  ",         KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J  ",         KEYCODE_J,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H  ",         KEYCODE_H,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN8 KEY ROW 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE Pound", KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  ,",        KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M  >",        KEYCODE_M,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N  <",        KEYCODE_N,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B  ",         KEYCODE_B,          IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN9 special keys 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BACKSPACE)", KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(GRAPHICS)",  KEYCODE_LALT,       IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(RIGHT)",     KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(UP)",        KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(DOWN)",      KEYCODE_DOWN,       IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN10 special keys 2 */
	PORT_BIT (0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(LEFT)",      KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( zx81 )
	PORT_START /* IN0 */
	PORT_DIPNAME( 0x80, 0x00, "32K RAM module" )
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x80, "yes")
	PORT_BIT(	  0x7e, 0x0f, IPT_UNUSED )
    PORT_BITX(    0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset",   KEYCODE_F3, IP_JOY_NONE )

	PORT_START /* IN1 KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",               KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z     :",             KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X     ;",             KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C     ?",             KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V     /",             KEYCODE_V,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN2 KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A     NEW     STOP",  KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S     SAVE    LPRINT",KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D     DIM     SLOW",  KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F     FOR     FAST",  KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G     GOTO    LLIST", KEYCODE_G,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN3 KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q     PLOT    \"\"",  KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W     UNPLOT  OR",    KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E     REM     STEP",  KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R     RUN     <=",    KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T     RAND    <>",    KEYCODE_T,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN4 KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1     EDIT",          KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2     AND",           KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3     THEN",          KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4     TO",            KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5     LEFT",          KEYCODE_5,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN5 KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0     RUBOUT",        KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9     GRAPHICS",      KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8     RIGHT",         KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7     UP",            KEYCODE_7,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6     DOWN",          KEYCODE_6,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN6 KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P     PRINT   \"",    KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O     POKE    )",     KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I     INPUT   (",     KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U     IF      $",     KEYCODE_U,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y     RETURN  >=",    KEYCODE_Y,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN7 KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER FUNCTION",      KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L     LET     =",     KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K     LIST    +",     KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J     LOAD    -",     KEYCODE_J,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H     GOSUB   **",    KEYCODE_H,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN8 KEY ROW 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE Pound",         KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".     ,",             KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M     PAUSE   >",     KEYCODE_M,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N     NEXT    <",     KEYCODE_N,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B     SCROLL  *",     KEYCODE_B,          IP_JOY_NONE )
    PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN9 special keys 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BACKSPACE)",         KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(GRAPHICS)",          KEYCODE_LALT,       IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(RIGHT)",             KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(UP)",                KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(DOWN)",              KEYCODE_DOWN,       IP_JOY_NONE )
	PORT_BIT (0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN10 special keys 2 */
	PORT_BIT (0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(LEFT)",              KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BIT (0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

static struct GfxLayout zx_pixel_layout =
{
	8,1,					/* 8x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{  0  },
	8						/* one byte per code */
};

static struct GfxDecodeInfo zx_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &zx_pixel_layout, 0, 2 },
	{ -1 } /* end of array */
};

static unsigned char zx80_palette[] =
{
	  0,  0,  0,	/* black */
    255,255,255,    /* white */
};

static unsigned char zx81_palette[] =
{
    255,255,255,    /* white */
	  0,  0,  0,	/* black */
};

static unsigned short zx_colortable[] =
{
	0,1,		/* white on black */
	1,0 		/* black on white */
};


/* Initialise the palette */
static void zx80_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,zx80_palette,sizeof(zx80_palette));
	memcpy(sys_colortable,zx_colortable,sizeof(zx_colortable));
}


static void zx81_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,zx81_palette,sizeof(zx81_palette));
	memcpy(sys_colortable,zx_colortable,sizeof(zx_colortable));
}

static struct DACinterface dac_interface =
{
    1,			/* number of DACs */
	{ 25 }		/* volume */
};

static struct MachineDriver machine_driver_zx80 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			310*207*50+1000, /* 3,2275MHz */
            readmem_zx80,writemem_zx80,readport,writeport,
			0,0
        },
	},
	50, 0, /* frames per second, vblank handled by vidhrdw */
	1,
	zx80_init_machine,
	zx_shutdown_machine,

	/* video hardware */
	32*8,									/* screen width (inc. blank/sync) */
	310,									/* screen height (inc. blank/sync) */
	{ 0*8, 32*8-1, 48, 310-32-1},			/* visible area (inc. some vertical overscan) */
	zx_gfxdecodeinfo,						/* graphics decode info */
    6, 4,                                   /* colors used for the characters */
	zx80_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	zx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_zx81 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			310*207*50+1000, /* 3,2275MHz */
            readmem_zx81,writemem_zx81,readport,writeport,
			0,0
        },
	},
	50, 0, /* frames per second, vblank handled by vidhrdw */
	1,
	zx81_init_machine,
	zx_shutdown_machine,

	/* video hardware */
	32*8,									/* screen width (inc. blank/sync) */
	310,									/* screen height (inc. blank/sync) */
	{ 0*8, 32*8-1, 48, 310-32-1},			/* visible area (inc. some vertical overscan) */
    zx_gfxdecodeinfo,                       /* graphics decode info */
    6, 4,                                   /* colors used for the characters */
	zx81_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	zx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_ts1000 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			262*207*60+1000, /* 3,2275MHz */
            readmem_zx81,writemem_zx81,readport,writeport,
			0,0
        },
	},
	60, 0, /* frames per second, vblank handled by vidhrdw */
	1,
	zx81_init_machine,
	zx_shutdown_machine,

	/* video hardware */
	32*8,									/* screen width (inc. blank/sync) */
	262,									/* screen height (inc. blank/sync) */
	{ 0*8, 32*8-1, 20, 262-12-1},			 /* visible area (inc. some vertical overscan) */
    zx_gfxdecodeinfo,                       /* graphics decode info */
    6, 4,                                   /* colors used for the characters */
	zx81_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	zx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_lambda =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			262*207*60+1000, /* 3,2275MHz */
            readmem_lambda,writemem_lambda,readport,writeport,
			0,0
        },
	},
	60, 0, /* frames per second, vblank handled by vidhrdw */
    1,
	lambda_init_machine,
	zx_shutdown_machine,

	/* video hardware */
	32*8,									/* screen width (inc. blank/sync) */
	262,									/* screen height (inc. blank/sync) */
	{ 0*8, 32*8-1, 20, 262-12-1},			/* visible area (inc. some vertical overscan) */
    zx_gfxdecodeinfo,                       /* graphics decode info */
    6, 4,                                   /* colors used for the characters */
	zx81_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	zx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START(zx80)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("zx80.rom",  0x0000, 0x1000, 0x4c7fc597)
	ROM_REGIONX(0x100,REGION_GFX1)
ROM_END


ROM_START(zx81)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("zx81.rom",  0x0000, 0x2000, 0x522c37b8)
	ROM_REGIONX(0x100,REGION_GFX1)
ROM_END

ROM_START(aszmic)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("aszmic.rom", 0x0000, 0x1000, 0x6c123536)
	ROM_REGIONX(0x100,REGION_GFX1)
ROM_END

ROM_START(ts1000)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("ts1000.rom", 0x0000, 0x2000, 0x4b1dd6eb)
	ROM_REGIONX(0x100,REGION_GFX1)
ROM_END

ROM_START(lambda)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("8300.rom", 0x0000, 0x2000, 0xa350f2b1)
	ROM_REGIONX(0x00100,REGION_GFX1)
	ROM_REGIONX(0x00200,REGION_GFX2)
	ROM_LOAD("8300.chr", 0x0000, 0x0200, 0x1c42fe46)
ROM_END

ROM_START(pow3000)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("pow3000.rom", 0x0000, 0x2000, 0x8a49b2c3)
	ROM_REGIONX(0x100,REGION_GFX1)
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
static const char *zx80_file_extensions[] = {
	"80", "o", NULL
};

static const char *zx81_file_extensions[] = {
	"p", "81", NULL
};


struct GameDriver zx80_driver =
{
	__FILE__,
	0,
	"zx80",
	"ZX-80",
	"1980",
	"Sinclair",
	NULL,
	0,
	&machine_driver_zx80,
	zx_init_driver,

	rom_zx80,
	zx_rom_load,			/* load rom_file images */
	zx_rom_id,				/* identify rom images */
	zx80_file_extensions,	/* file extensions */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_zx80,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
    0,                      /* hiscore save */
};

struct GameDriver aszmic_driver =
{
    __FILE__,
	&zx80_driver,
    "aszmic",
    "Aszmic",
    "1981",
    "Sinclair",
    NULL,
    0,
	&machine_driver_zx80,
    zx_init_driver,

    rom_aszmic,
	zx_rom_load,			/* load rom_file images */
    zx_rom_id,              /* identify rom images */
	zx80_file_extensions,	/* file extensions */
    1,                      /* number of ROM slots */
    0,                      /* number of floppy drives supported */
    0,                      /* number of hard drives supported */
    1,                      /* number of cassette drives supported */
    0,                      /* rom decoder */
    0,                      /* opcode decoder */
    0,                      /* pointer to sample names */
    0,                      /* sound_prom */

	input_ports_zx80,

    0,                      /* color_prom */
    0,                      /* color palette */
    0,                      /* color lookup table */

    GAME_COMPUTER | ORIENTATION_DEFAULT,    /* orientation */

    0,                      /* hiscore load */
    0,                      /* hiscore save */
};

struct GameDriver zx81_driver =
{
	__FILE__,
	0,
	"zx81",
	"ZX-81",
	"1981",
	"Sinclair",
	NULL,
	0,
	&machine_driver_zx81,
	zx_init_driver,

	rom_zx81,
	zx_rom_load,			/* load rom_file images */
    zx_rom_id,              /* identify rom images */
	zx81_file_extensions,	/* file extensions */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
    0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_zx81,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};

struct GameDriver ts1000_driver =
{
	__FILE__,
	&zx81_driver,
	"ts1000",
	"Timex Sinclair 1000",
	"1981",
	"Sinclair",
	NULL,
	0,
	&machine_driver_ts1000,
	zx_init_driver,

	rom_ts1000,
	zx_rom_load,			/* load rom_file images */
    zx_rom_id,              /* identify rom images */
	zx81_file_extensions,	/* file extensions */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
    0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_zx81,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};

struct GameDriver lambda_driver =
{
	__FILE__,
	&zx81_driver,
    "lambda",
	"Lambda 8300",
	"1981",
	"Lambda Electronics Ltd., Hongkong",
	NULL,
	0,
	&machine_driver_lambda,
	zx_init_driver,

	rom_lambda,
	zx_rom_load,			/* load rom_file images */
    zx_rom_id,              /* identify rom images */
	zx81_file_extensions,	/* file extensions */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
    0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_zx81,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};

struct GameDriver pow3000_driver =
{
	__FILE__,
	&zx81_driver,
	"pow3000",
	"Power 3000",
	"1981",
	"Creon Enterprises Hongkong",
	NULL,
	0,
	&machine_driver_ts1000,
	zx_init_driver,

	rom_pow3000,
	zx_rom_load,			/* load rom_file images */
    zx_rom_id,              /* identify rom images */
	zx81_file_extensions,	/* file extensions */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
    0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_zx81,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_NOT_WORKING | GAME_COMPUTER | ORIENTATION_DEFAULT,    /* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};


