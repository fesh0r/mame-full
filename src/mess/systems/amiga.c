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

INPUT_PORTS_START( input_ports )
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
			0,					/* Memory region #0 */
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

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT | VIDEO_UPDATE_BEFORE_VBLANK,
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

ROM_START( amiga_rom )
	ROM_REGION(0x1000000) /* for ram, etc */
	//ROM_LOAD( "kick13.rom",  0xf80000, 0x80000, 0xfa180000 )
	ROM_LOAD( "kick13.rom",  0xf80000, 0x80000, 0xf6290043)

ROM_END

static void amiga_rom_decode(void)
{
#ifdef LSB_FIRST
	UINT16 *rom = (UINT16 *)&Machine->memory_region[0][0xf80000];
	unsigned i;
	for( i = 0; i < 0x80000; i += 2, rom++ )
		*rom = (*rom << 8) | (*rom >> 8);
#endif
}

struct GameDriver amiga_driver =
{
	__FILE__,
	0,
	"amiga",
	"Commodore Amiga (NTSC)",
	"1984",
	"Commodore Busines Machines Co.",
	"Ernesto Corvi",
	GAME_NOT_WORKING | GAME_COMPUTER,
	&machine_driver_ntsc,
	0,

	amiga_rom,
	0,						/* load rom_file images */
	0,						/* identify rom images */
	0,						/* number of ROM slots - in this case, a CMD binary */
	4,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	amiga_rom_decode,		/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports,

	0,						/* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0						/* hiscore save */
};
