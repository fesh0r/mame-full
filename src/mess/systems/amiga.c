/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/**************************************************************************

	Prototypes

***************************************************************************/

/* from machine/amiga.c */
extern int amiga_cia_r( int offs );
extern void amiga_cia_w( int offs, int data );
extern int amiga_custom_r( int offs );
extern void amiga_custom_w( int offs, int data );
extern void amiga_init_machine( void );
extern int amiga_vblank_irq( void );
extern int amiga_fdc_init( int id, const char *floppy_name );

/* from vidhrdw/amiga.c */
extern void amiga_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh );
extern int amiga_vh_start( void );
extern void amiga_vh_stop( void );
extern void amiga_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_RAM },			/* Chip Ram - 1Mb / 512k */
	{ 0xbfd000, 0xbfefff, amiga_cia_r },		/* 8510's CIA A and CIA B */
	{ 0xc00000, 0xd7ffff, MRA_RAM },			/* Internal Expansion Ram - 1.5 Mb */
	{ 0xdff000, 0xdfffff, amiga_custom_r },		/* Custom Chips */
	{ 0xf00000, 0xffffff, MRA_ROM },			/* System ROM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_RAM },			/* Chip Ram - 1Mb / 512k */
	{ 0xbfd000, 0xbfefff, amiga_cia_w },		/* 8510's CIA A and CIA B */
	{ 0xc00000, 0xd7ffff, MWA_RAM },			/* Internal Expansion Ram - 1.5 Mb */
	{ 0xdff000, 0xdfffff, amiga_custom_w },		/* Custom Chips */
	{ 0xf00000, 0xffffff, MWA_ROM },			/* System ROM */
	{ -1 }	/* end of table */
};

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( amiga )
	PORT_START
INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static struct MachineDriver machine_driver_ntsc =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159090,			/* 7.15909 Mhz (NTSC) */
			readmem,writemem,0,0,
			amiga_vblank_irq,1,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	amiga_init_machine,
	0,

	/* video hardware */
	456, 262, /* screen width, screen height ( 312 for PAL ) */
	{ 120, 456 - 1, 32, 262 - 1 },			/* visible_area */

	gfxdecodeinfo,					/* graphics decode info */
	4096, 4096,						/* number of colors, colortable size */
	amiga_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | GAME_REQUIRES_16BIT | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	amiga_vh_start,
	amiga_vh_stop,
	amiga_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( amiga )
	ROM_REGIONX(0x1000000,REGION_CPU1) /* for ram, etc */
	//ROM_LOAD( "kick13.rom",  0xf80000, 0x80000, 0xfa180000 )
	ROM_LOAD_WIDE( "kick13.rom",  0xf80000, 0x80000, 0xf6290043)

ROM_END

static void amiga_rom_decode(void)
{
#ifdef LSB_FIRST
	UINT16 *rom = (UINT16 *)&memory_region(REGION_CPU1)[0xf80000];
	unsigned i;
	for( i = 0; i < 0x80000; i += 2, rom++ )
		*rom = (*rom << 8) | (*rom >> 8);
#endif
}


static const struct IODevice io_amiga[] = {
	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"adf\0",            /* file extensions */
		NULL,				/* private */
		NULL,				/* id */
		amiga_fdc_init, 	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
    },
    { IO_END }
};

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
COMPX( 1984, amiga,    0,		 ntsc,	   amiga,	 0, 	   "Commodore Business Machines Co.",  "Amiga (NTSC)", GAME_NOT_WORKING )

