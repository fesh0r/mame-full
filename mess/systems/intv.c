/******************************************************************
 *  Mattel Intellivision + Keyboard Component Drivers
 *
 *  Frank Palazzolo
 *
 *  TBD:
 *      Everything
 *
 ******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/intv.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/* should be in machine/intv.c */

int intvkbd_load_rom (int id)
{
	return 0;
}

void init_intvkbd(void)
{
}

/* graphics output */

struct GfxLayout intvkbd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo intvkbd_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &intvkbd_charlayout, 0, 1},
	{ -1 }
};

INPUT_PORTS_START( intvkbd )
INPUT_PORTS_END

#define MEM16(A,B,C) { A<<1, (B<<1)+1, C }

static MEMORY_READ16_START( readmem )
	MEM16( 0x0200, 0x0fff, MRA16_RAM ),
	MEM16( 0x1000, 0x1fff, MRA16_ROM ),
	MEM16( 0x3000, 0x37ff, MRA16_ROM ), /* GROM */
	MEM16( 0x3800, 0x39ff, MRA16_RAM ),	/* GRAM */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	MEM16( 0x0200, 0x0fff, MWA16_RAM ),
	MEM16( 0x1000, 0x1fff, MWA16_ROM ),
	MEM16( 0x3000, 0x37ff, MWA16_ROM ),	/* GROM */
	MEM16( 0x3800, 0x39ff, MWA16_RAM ),	/* GRAM */
MEMORY_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x0fff, MRA_RAM }, /* ??? */
	{ 0x4000, 0x4fff, MRA_RAM }, /* ??? */
	{ 0xb7f8, 0xb7ff, MRA_RAM }, /* ??? */
	{ 0xb800, 0xbfff, &videoram_r }, 	/* videoram */
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x0fff, MWA_RAM }, /* ??? */
	{ 0x4000, 0x4fff, MWA_RAM }, /* ??? */
	{ 0xb7f8, 0xb7ff, MWA_RAM }, /* ??? */
	{ 0xb800, 0xbfff, &videoram_w }, 	/* videoram */
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

#if 0
static struct IOReadPort readport[] =
{
    {-1}
};

static struct IOWritePort writeport[] =
{
    {-1}
};
#endif

static unsigned char intvkbd_palette[2][3] =
{
	{ 0, 128, 0 },
	{ 255,255,255 }
};

static unsigned short intvkbd_colortable[1][2] = {
	{ 0, 1 },
};

static void intvkbd_init_palette(unsigned char *palette,
						  unsigned short *colortable,
						  const unsigned char *color_prom)
{
	memcpy(palette, intvkbd_palette, sizeof (intvkbd_palette));
	memcpy(colortable,intvkbd_colortable,sizeof(intvkbd_colortable));
}

int intvkbd_vh_start(void)
{
	videoram_size = 0x0800;
	videoram = malloc(videoram_size);

    if (generic_vh_start())
        return 1;

    return 0;
}

void intvkbd_vh_stop(void)
{
	free(videoram);
	generic_vh_stop();
}

void intvkbd_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x,y,offs;

	for(y=0;y<24;y++)
	{
		for(x=0;x<40;x++)
		{
			offs = y*64+x;
			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,
					x*8,y*8,
					&Machine->visible_area, TRANSPARENCY_NONE, 0);
		}
	}
}

static struct MachineDriver machine_driver_intvkbd =
{
	/* basic machine hardware */
	{
		/* Main CPU (in Master System) */
		{
			CPU_CP1600,
			894000,
			readmem,writemem,0,0,
			ignore_interrupt,0
		},
		/* Slave CPU - runs tape drive, display */
		{
			CPU_M6502,
			3579545/2,	/* Colorburst/2 */
			readmem2,writemem2,0,0,
			ignore_interrupt,1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* single CPU */
	NULL,					/* init machine */
	NULL,					/* stop machine */

	/* video hardware */
	40*8, 24*8, { 0, 40*8-1, 0, 24*8-1},
	intvkbd_gfxdecodeinfo,
	sizeof (intvkbd_palette) / sizeof (intvkbd_palette[0]) ,
	sizeof (intvkbd_colortable) / sizeof(intvkbd_colortable[0][0]),
	intvkbd_init_palette,					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */

	intvkbd_vh_start,
	intvkbd_vh_stop,
	intvkbd_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START(intvkbd)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, 0xcbce86f7 )
		ROM_LOAD16_BYTE ( "grom.bin", 0x3000<<1, 0x2000, 0xcbce86f7 )
	ROM_REGION(0x10000,REGION_CPU2,0)
		ROM_LOAD( "0104.u20",  0xc000, 0x1000, 0x5c6f1256)
		ROM_LOAD("cpu2d.u21",  0xd000, 0x1000, 0x2c2dba33)
		/* temporarily requiring the BASIC as well */
		ROM_LOAD( "0106.u1",   0xe000, 0x1000, 0x5ae9546a)
		ROM_LOAD( "0107.u2",   0xf000, 0x1000, 0x72ea3d34)
	ROM_REGION(0x00800,REGION_GFX1,0)
		ROM_LOAD( "4c52.u34",  0x0000, 0x0800, 0xcbeb2e96)
ROM_END

static const struct IODevice io_intvkbd[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",            /* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		NULL,				/* id */
		//intvkbd_load_rom,	/* init */
		NULL,				/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY      FULLNAME */
/* CONS( 1900, intv,     0,		intv,     intv, 	intv,	  "Mattel",    "Intellivision" ) */
COMP( 1981, intvkbd,  0,		intvkbd,  intvkbd, 	intvkbd,  "Mattel",    "Intellivision Keyboard Component (Unreleased)" )
