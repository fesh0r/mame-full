/*
 *	MacPlus emulation
 *
 *	Nate Woods
 *
 *
 *		0x000000 - 0x3fffff		RAM/ROM (switches based on overlay)
 *		0x400000 - 0x4fffff		ROM
 *		0x580000 - 0x5fffff		5380 NCR/Symbios SCSI peripherals chip
 *		0x600000 - 0x6fffff		RAM
 *		0x800000 - 0x9fffff		Zilog 8530 SCC (Serial Control Chip) Read
 *		0xa00000 - 0xbfffff		Zilog 8530 SCC (Serial Control Chip) Write
 *		0xc00000 - 0xdfffff		IWM (Integrated Woz Machine; floppy)
 *		0xe80000 - 0xefffff		Rockwell 6522 VIA
 *		0xf00000 - 0xffffef		??? (the ROM appears to be accessing here)
 *		0xfffff0 - 0xffffff		Auto Vector
 *
 *
 *	Interrupts:
 *		M68K:
 *			Level 1 from VIA
 *			Level 2 from SCC
 *
 *		VIA:
 *			CA1 from VBLANK
 *			CA2 from 1 Mhz clock (RTC)
 *			CB1 from Keyboard Clock
 *			CB2 from Keyboard Data
 *			SR  from Keyboard Data Ready
 *
 *		SCC:
 *			PB_EXT	from mouse Y circuitry
 *			PA_EXT	from mouse X circuitry
 *
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/machine/6522via.h"

/* from machine/mac.c */
extern void macplus_init_machine( void );
extern void init_macplus( void );
extern int macplus_vblank_irq( void );
extern READ_HANDLER ( macplus_via_r );
extern WRITE_HANDLER ( macplus_via_w );
extern READ_HANDLER ( macplus_autovector_r );
extern WRITE_HANDLER ( macplus_autovector_w );
extern READ_HANDLER ( macplus_iwm_r );
extern WRITE_HANDLER ( macplus_iwm_w );
extern READ_HANDLER ( macplus_scc_r );
extern WRITE_HANDLER ( macplus_scc_w );
extern READ_HANDLER ( macplus_scsi_r );
extern WRITE_HANDLER ( macplus_scsi_w );
extern int macplus_floppy_init(int id);
extern void macplus_floppy_exit(int id);
extern void macplus_nvram_handler(void *file, int read_or_write);

/* from vidhrdw/mac.c */
extern int macplus_vh_start(void);
extern void macplus_vh_stop(void);
extern void macplus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* from sndhrdw/mac.c */
extern int macplus_sh_start( const struct MachineSound *msound );
extern void macplus_sh_stop( void );
extern void macplus_sh_update( void );


static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x3fffff, MRA_BANK1 },	/* ram/rom */
	{ 0x400000, 0x41ffff, MRA_BANK3 }, /* rom */
	{ 0x420000, 0x43ffff, MRA_BANK4 }, /* rom - mirror */
	{ 0x440000, 0x45ffff, MRA_BANK5 }, /* rom - mirror */
	{ 0x460000, 0x47ffff, MRA_BANK6 }, /* rom - mirror */
	{ 0x480000, 0x49ffff, MRA_BANK7 }, /* rom - mirror */
	{ 0x4a0000, 0x4bffff, MRA_BANK8 }, /* rom - mirror */
	{ 0x4c0000, 0x4dffff, MRA_BANK9 }, /* rom - mirror */
	{ 0x4e0000, 0x4fffff, MRA_BANK10 }, /* rom - mirror */
	{ 0x580000, 0x5fffff, macplus_scsi_r },
	{ 0x600000, 0x6fffff, MRA_BANK2 },	/* ram */
	{ 0x800000, 0x9fffff, macplus_scc_r },
	{ 0xc00000, 0xdfffff, macplus_iwm_r },
	{ 0xe80000, 0xefffff, macplus_via_r },
	{ 0xfffff0, 0xffffff, macplus_autovector_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x3fffff, MWA_BANK1 }, /* ram/rom */
	{ 0x400000, 0x4fffff, MWA_ROM },
	{ 0x580000, 0x5fffff, macplus_scsi_w },
	{ 0x600000, 0x6fffff, MWA_BANK2 }, /* ram */
	{ 0xa00000, 0xbfffff, macplus_scc_w },
	{ 0xc00000, 0xdfffff, macplus_iwm_w },
	{ 0xe80000, 0xefffff, macplus_via_w },
	{ 0xfffff0, 0xffffff, macplus_autovector_w },
	{ -1 }	/* end of table */
};

static void macplus_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	palette[0*3 + 0] = 0xff;
	palette[0*3 + 1] = 0xff;
	palette[0*3 + 2] = 0xff;
	palette[1*3 + 0] = 0x00;
	palette[1*3 + 1] = 0x00;
	palette[1*3 + 2] = 0x00;
}

static struct CustomSound_interface custom_interface =
{
	macplus_sh_start,
	macplus_sh_stop,
	macplus_sh_update
};

static struct MachineDriver machine_driver_macplus =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159090,			/* 7.15909 Mhz */
			readmem,writemem,0,0,
			macplus_vblank_irq,1,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	macplus_init_machine,
	0,

	/* video hardware */
	512, 342, /* screen width, screen height */
	{ 0, 512-1, 0, 342-1 },			/* visible_area */

	0,					/* graphics decode info */
	2, 2,						/* number of colors, colortable size */
	macplus_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	macplus_vh_start,
	macplus_vh_stop,
	macplus_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	},

	macplus_nvram_handler
};

INPUT_PORTS_START( macplus )
	PORT_START /* 0 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "right button", KEYCODE_RALT, IP_JOY_DEFAULT)
	/* Not yet implemented */

	PORT_START /* Mouse - X AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - Y AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )
INPUT_PORTS_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( macplus )
	ROM_REGION(0x420000,REGION_CPU1) /* for ram, etc */
	ROM_LOAD_WIDE( "macplus.rom",  0x400000, 0x20000, 0xb2102e8e)
ROM_END

static const struct IODevice io_macplus[] = {
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"dsk\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		macplus_floppy_init,/* init */
		macplus_floppy_exit,/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
COMPX( 1986, macplus,  0,		 macplus,  macplus,	 macplus,  "Apple Computer",  "Macintosh Plus", GAME_NOT_WORKING )
