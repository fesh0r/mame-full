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
#include "sndhrdw/SN76496.h"
#include "vidhrdw/TMS9928A.h"

/* vidhrdw/coleco.c */
extern int coleco_vh_start(void);
extern void coleco_vh_stop(void);
extern void coleco_vh_screenrefresh(struct osd_bitmap *bitmap);

/* machine/coleco.c */
extern unsigned char *coleco_ram;
extern unsigned char *coleco_rom;
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
    { 0x0000, 0x1fff, MRA_ROM, &coleco_rom }, /* COLECO.ROM */
    { 0x6000, 0x63ff, coleco_ram_r, &coleco_ram },
    { 0x6400, 0x67ff, coleco_ram_r },
    { 0x6800, 0x6bff, coleco_ram_r },
    { 0x6c00, 0x6fff, coleco_ram_r },
    { 0x7000, 0x73ff, coleco_ram_r },
    { 0x7400, 0x77ff, coleco_ram_r },
    { 0x7800, 0x7bff, coleco_ram_r },
    { 0x7c00, 0x7fff, coleco_ram_r },
    { 0x8000, 0xffff, MRA_ROM, &coleco_cartridge_rom }, /* Cartridge */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x1fff, MWA_ROM }, /* COLECO.ROM */
    { 0x6000, 0x63ff, coleco_ram_w },
    { 0x6400, 0x67ff, coleco_ram_w },
    { 0x6800, 0x6bff, coleco_ram_w },
    { 0x6c00, 0x6fff, coleco_ram_w },
    { 0x7000, 0x73ff, coleco_ram_w },
    { 0x7400, 0x77ff, coleco_ram_w },
    { 0x7800, 0x7bff, coleco_ram_w },
    { 0x7c00, 0x7fff, coleco_ram_w },
    { 0x8000, 0xffff, MWA_ROM }, /* Cartridge */
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




INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "0", OSD_KEY_0, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "1", OSD_KEY_1, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2", OSD_KEY_2, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "3", OSD_KEY_3, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "4", OSD_KEY_4, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "5", OSD_KEY_5, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "6", OSD_KEY_6, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "7", OSD_KEY_7, IP_JOY_DEFAULT, 0 )

	PORT_START	/* IN1 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "8", OSD_KEY_8, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "9", OSD_KEY_9, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "#", OSD_KEY_MINUS, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, ".", OSD_KEY_EQUALS, IP_JOY_DEFAULT, 0 )
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
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 0", OSD_KEY_Z, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 1", OSD_KEY_X, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 2", OSD_KEY_C, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 3", OSD_KEY_V, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 4", OSD_KEY_B, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 5", OSD_KEY_N, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 6", OSD_KEY_M, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 7", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0 )

	PORT_START	/* IN4 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 8", OSD_KEY_STOP, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 9", OSD_KEY_SLASH, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 #", OSD_KEY_H, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "2 .", OSD_KEY_J, IP_JOY_DEFAULT, 0 )
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
	3579545,	/* 3.579545 MHz */
	{ 255 }
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
			0,
			readmem,writemem,readport,writeport,
			coleco_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0, /* init_machine */

	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	gfxdecodeinfo,
	TMS9928A_PALETTE_SIZE,TMS9928A_COLORTABLE_SIZE/sizeof(unsigned short),
	0,

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


struct GameDriver coleco_driver =
{
	"Colecovision",
	"coleco",
	"Marat Fayzullin (ColEm source)\nMike Balfour",
	&machine_driver,

	coleco_load_rom,
	coleco_id_rom,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, TMS9928A_palette, TMS9928A_colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
