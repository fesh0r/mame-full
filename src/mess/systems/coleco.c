/***************************************************************************

  coleco.c

  Driver file to handle emulation of the Colecovision.

  TODO:
	- Verify correctness of SN76496 sound emulation
	- Abstract TMS9928A a little better
	- Finish TMS9928A emulation
	- Clean up code
***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/sn76496.h"
#include "mess/vidhrdw/tms9928a.h"

/* vidhrdw/coleco.c */
extern int coleco_vh_start(void);
extern void coleco_vh_stop(void);
extern void coleco_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

/* machine/coleco.c */
extern unsigned char *coleco_ram;
extern unsigned char *coleco_cartridge_rom;

extern int coleco_id_rom (const char *name, const char *gamename);
extern int coleco_load_rom (void);
extern int coleco_ram_r(int offset);
extern void coleco_ram_w(int offset, int data);
extern int coleco_paddle_r(int offset);
extern void coleco_paddle_toggle_1_w(int offset, int data);
extern void coleco_paddle_toggle_2_w(int offset, int data);
extern int coleco_VDP_r(int offset);
extern void coleco_VDP_w(int offset, int data);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x1fff, MRA_ROM },  /* COLECO.ROM */
    { 0x6000, 0x63ff, coleco_ram_r },
    { 0x6400, 0x67ff, coleco_ram_r },
    { 0x6800, 0x6bff, coleco_ram_r },
    { 0x6c00, 0x6fff, coleco_ram_r },
    { 0x7000, 0x73ff, coleco_ram_r },
    { 0x7400, 0x77ff, coleco_ram_r },
    { 0x7800, 0x7bff, coleco_ram_r },
    { 0x7c00, 0x7fff, coleco_ram_r },
    { 0x8000, 0xffff, MRA_ROM },  /* Cartridge */
	{ -1 }	 /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x1fff, MWA_ROM }, /* COLECO.ROM */
    { 0x6000, 0x63ff, coleco_ram_w, &coleco_ram },
    { 0x6400, 0x67ff, coleco_ram_w },
    { 0x6800, 0x6bff, coleco_ram_w },
    { 0x6c00, 0x6fff, coleco_ram_w },
    { 0x7000, 0x73ff, coleco_ram_w },
    { 0x7400, 0x77ff, coleco_ram_w },
    { 0x7800, 0x7bff, coleco_ram_w },
    { 0x7c00, 0x7fff, coleco_ram_w },
    { 0x8000, 0xffff, MWA_ROM, &coleco_cartridge_rom }, /* Cartridge */
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0xA0, 0xBF, coleco_VDP_r },
	{ 0xE0, 0xFF, coleco_paddle_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x80, 0x9F, coleco_paddle_toggle_1_w },
	{ 0xA0, 0xBF, coleco_VDP_w },
	{ 0xC0, 0xDF, coleco_paddle_toggle_2_w },
	{ 0xE0, 0xFF, SN76496_0_w },
	{ -1 }	/* end of table */
};




INPUT_PORTS_START( coleco )
	PORT_START	/* IN0 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "0", KEYCODE_0, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "1", KEYCODE_1, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2", KEYCODE_2, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "3", KEYCODE_3, IP_JOY_DEFAULT)
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "4", KEYCODE_4, IP_JOY_DEFAULT)
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "5", KEYCODE_5, IP_JOY_DEFAULT)
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "6", KEYCODE_6, IP_JOY_DEFAULT)
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "7", KEYCODE_7, IP_JOY_DEFAULT)

	PORT_START	/* IN1 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "8", KEYCODE_8, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "9", KEYCODE_9, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "#", KEYCODE_MINUS, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, ".", KEYCODE_EQUALS, IP_JOY_DEFAULT)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 0", KEYCODE_Z, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 1", KEYCODE_X, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 2", KEYCODE_C, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 3", KEYCODE_V, IP_JOY_DEFAULT )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 4", KEYCODE_B, IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 5", KEYCODE_N, IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 6", KEYCODE_M, IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 7", KEYCODE_OTHER, IP_JOY_DEFAULT )

	PORT_START	/* IN4 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 8", KEYCODE_OTHER, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 9", KEYCODE_OTHER, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 #", KEYCODE_H, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 .", KEYCODE_J, IP_JOY_DEFAULT )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static struct SN76496interface sn76496_interface =
{
	1,	/* 1 chip */
	{3579545},	/* 3.579545 MHz */
	{ 100 }
};

static int coleco_interrupt(void)
{
	if (TMS9928A_interrupt()!=0)
		return nmi_interrupt();

	return 0;
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3579545,	/* 3.579545 Mhz */
			readmem,writemem,readport,writeport,
			coleco_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	gfxdecodeinfo,
	TMS9928A_PALETTE_SIZE,TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	coleco_vh_start,
	coleco_vh_stop,
	coleco_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (coleco)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD ("coleco.rom", 0x0000, 0x2000, 0x3aa93ef3)
ROM_END

//ROM_START (colecofb_rom)
//	ROM_REGIONX(0x10000,REGION_CPU1)
//	ROM_LOAD ("colecofb.rom", 0x0000, 0x2000, 0x640cf85b) /* fast screen */
//ROM_END

//ROM_START (coleconb_rom)
//	ROM_REGIONX(0x10000,REGION_CPU1)
//	ROM_LOAD ("coleconb.rom", 0x0000, 0x2000, 0x66cda476) /* no screen */
//ROM_END






/* list of file extensions */
static const char *coleco_file_extensions[] =
{
	"rom",
	0       /* end of array */
};


struct GameDriver coleco_driver =
{
	__FILE__,
	0,
	"coleco",
	"Colecovision",
	"1982",
	"Coleco",
	"Marat Fayzullin (ColEm source)\nMike Balfour",
	0,
	&machine_driver,
	0,
    rom_coleco,
	coleco_load_rom,
	coleco_id_rom,
	coleco_file_extensions,  /* Default file extension */
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0,
   0,
	0,
	0,	/* sound_prom */
   input_ports_coleco,
   0,
   /*TMS9928A_palette*/0,
   /*TMS9928A_colortable*/0,
	ORIENTATION_DEFAULT,
   0,
   0,
};


#ifdef COLECO_HACKS
struct GameDriver colecofb_driver =
{
	__FILE__,
	&coleco_driver,
	"colecofb",
	"Colecovision (Fast BIOS Hack)",
	"1982",
	"Coleco",
	"Marat Fayzullin (ColEm source)\nMike Balfour",
	0,
	&machine_driver,
	0,
   colecofb_rom,
	coleco_load_rom,
	coleco_id_rom,
	coleco_file_extensions,  /* Default file extension */
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0,
   0,
	0,
	0,	/* sound_prom */
   input_ports,
   0,
   TMS9928A_palette,
   TMS9928A_colortable,
	ORIENTATION_DEFAULT,
   0,
   0,
};


struct GameDriver coleconb_driver =
{
	__FILE__,
	&coleco_driver,
	"coleconb",
	"Colecovision (NO BIOS Hack)",
	"1982",
	"Coleco",
	"Marat Fayzullin (ColEm source)\nMike Balfour",
	0,
	&machine_driver,
	0,
   coleconb_rom,
	coleco_load_rom,
	coleco_id_rom,
	coleco_file_extensions,  /* Default file extension */
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0,
   0,
	0,
	0,	/* sound_prom */
   input_ports,
   0,
   TMS9928A_palette,
   TMS9928A_colortable,
	ORIENTATION_DEFAULT,
   0,
   0,
};

#endif
