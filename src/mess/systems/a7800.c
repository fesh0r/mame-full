/***************************************************************************

  a7800.c

  Driver file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"

/* vidhrdw/a7800.c */
extern int a7800_vh_start(void);
extern void a7800_vh_stop(void);
extern void a7800_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern int a7800_interrupt(void);
extern int a7800_MARIA_r(int offset);
extern void a7800_MARIA_w(int offset, int data);

/* machine/a7800.c */
extern unsigned char *a7800_ram;
extern unsigned char *a7800_cartridge_rom;
extern void a7800_init_machine(void);
extern void a7800_stop_machine(void);
extern int a7800_id_rom (const char *name, const char *gamename);
extern int a7800_load_rom (void);
extern int a7800_TIA_r(int offset);
extern void a7800_TIA_w(int offset, int data);
extern int a7800_RIOT_r(int offset);
extern void a7800_RIOT_w(int offset, int data);
extern int a7800_MAINRAM_r(int offset);
extern void a7800_MAINRAM_w(int offset, int data);
extern int a7800_RAM0_r(int offset);
extern void a7800_RAM0_w(int offset, int data);
extern int a7800_RAM1_r(int offset);
extern void a7800_RAM1_w(int offset, int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x001f, a7800_TIA_r },
	{ 0x0020, 0x003f, a7800_MARIA_r },
	{ 0x0040, 0x00FF, a7800_RAM0_r },
	{ 0x0100, 0x011f, a7800_TIA_r },
	{ 0x0120, 0x013f, a7800_MARIA_r },
	{ 0x0140, 0x01FF, a7800_RAM1_r },
	{ 0x0200, 0x021f, a7800_TIA_r },
	{ 0x0220, 0x023f, a7800_MARIA_r },
	{ 0x0280, 0x02FF, a7800_RIOT_r },
	{ 0x0300, 0x031f, a7800_TIA_r },
	{ 0x0320, 0x033f, a7800_MARIA_r },
	{ 0x0480, 0x04ff, MRA_RAM },    /* RIOT RAM */
	{ 0x1800, 0x27FF, MRA_RAM },
	{ 0x2800, 0x2FFF, a7800_MAINRAM_r },
	{ 0x3000, 0x37FF, a7800_MAINRAM_r },
	{ 0x3800, 0x3FFF, a7800_MAINRAM_r },
	{ 0x4000, 0xFFFF, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x001f, a7800_TIA_w },
	{ 0x0020, 0x003f, a7800_MARIA_w },
	{ 0x0040, 0x00FF, a7800_RAM0_w },
	{ 0x0100, 0x011f, a7800_TIA_w },
	{ 0x0120, 0x013f, a7800_MARIA_w },
	{ 0x0140, 0x01FF, a7800_RAM1_w },
	{ 0x0200, 0x021f, a7800_TIA_w },
	{ 0x0220, 0x023f, a7800_MARIA_w },
	{ 0x0280, 0x02FF, a7800_RIOT_w },
	{ 0x0300, 0x031f, a7800_TIA_w },
	{ 0x0320, 0x033f, a7800_MARIA_w },
	{ 0x0480, 0x04ff, MWA_RAM },  /* RIOT RAM */
	{ 0x1800, 0x27FF, MWA_RAM },
	{ 0x2800, 0x2FFF, a7800_MAINRAM_w },
	{ 0x3000, 0x37FF, a7800_MAINRAM_w },
	{ 0x3800, 0x3FFF, a7800_MAINRAM_w },
	{ 0x4000, 0xFFFF, MWA_ROM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)

	PORT_START      /* IN1 */
    PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )        
	PORT_BIT ( 0xF0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT (0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_VBLANK)

	PORT_START      /* IN3 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "Reset", KEYCODE_R, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "Start", KEYCODE_S, IP_JOY_DEFAULT)
	PORT_BIT ( 0xFC, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

#define RANGE(r0,g0,b0,r1,g1,b1) \
	 ( 0*(r1-r0)/15+r0)*4, ( 0*(g1-g0)/15+g0)*4, ( 0*(b1-b0)/15+b0)*4, \
	 ( 1*(r1-r0)/15+r0)*4, ( 1*(g1-g0)/15+g0)*4, ( 1*(b1-b0)/15+b0)*4, \
	 ( 2*(r1-r0)/15+r0)*4, ( 2*(g1-g0)/15+g0)*4, ( 2*(b1-b0)/15+b0)*4, \
	 ( 3*(r1-r0)/15+r0)*4, ( 3*(g1-g0)/15+g0)*4, ( 3*(b1-b0)/15+b0)*4, \
	 ( 4*(r1-r0)/15+r0)*4, ( 4*(g1-g0)/15+g0)*4, ( 4*(b1-b0)/15+b0)*4, \
	 ( 5*(r1-r0)/15+r0)*4, ( 5*(g1-g0)/15+g0)*4, ( 5*(b1-b0)/15+b0)*4, \
	 ( 6*(r1-r0)/15+r0)*4, ( 6*(g1-g0)/15+g0)*4, ( 6*(b1-b0)/15+b0)*4, \
	 ( 7*(r1-r0)/15+r0)*4, ( 7*(g1-g0)/15+g0)*4, ( 7*(b1-b0)/15+b0)*4, \
	 ( 8*(r1-r0)/15+r0)*4, ( 8*(g1-g0)/15+g0)*4, ( 8*(b1-b0)/15+b0)*4, \
	 ( 9*(r1-r0)/15+r0)*4, ( 9*(g1-g0)/15+g0)*4, ( 9*(b1-b0)/15+b0)*4, \
	 (10*(r1-r0)/15+r0)*4, (10*(g1-g0)/15+g0)*4, (10*(b1-b0)/15+b0)*4, \
	 (11*(r1-r0)/15+r0)*4, (11*(g1-g0)/15+g0)*4, (11*(b1-b0)/15+b0)*4, \
	 (12*(r1-r0)/15+r0)*4, (12*(g1-g0)/15+g0)*4, (12*(b1-b0)/15+b0)*4, \
	 (13*(r1-r0)/15+r0)*4, (13*(g1-g0)/15+g0)*4, (13*(b1-b0)/15+b0)*4, \
	 (14*(r1-r0)/15+r0)*4, (14*(g1-g0)/15+g0)*4, (14*(b1-b0)/15+b0)*4, \
	 (15*(r1-r0)/15+r0)*4, (15*(g1-g0)/15+g0)*4, (15*(b1-b0)/15+b0)*4

static unsigned char palette[] =
{
	RANGE( 0, 0, 0, 63,63,63), /* gray                 */
	RANGE( 4, 0, 0, 63,55, 0), /* gold                 */
	RANGE(12, 0, 0, 63,43, 0), /* orange       */
	RANGE(18, 0, 0, 63,23,23), /* red orange   */
	RANGE(15, 0,24, 63,39,55), /* pink                 */
	RANGE(10, 0, 8, 35,35,63), /* purple       */
	RANGE(10, 0,32, 39,39,63), /* violet       */
	RANGE( 0, 0,32, 24,39,63), /* blue1        */
	RANGE( 0, 4,32, 31,43,59), /* blue2        */
	RANGE( 0, 6,18,  0,47,63), /* light blue   */
	RANGE( 0, 8, 8, 15,63,43), /* light cyan   */
	RANGE( 0,10, 0, 39,63,13), /* cyan                 */
	RANGE( 0,10, 0,  0,63, 0), /* green        */
	RANGE(16,24, 0, 31,63, 0), /* light green  */
	RANGE( 4, 4, 0, 63,63, 0), /* green orange */
	RANGE( 8, 4, 0, 63,43, 0)  /* light orange */
};

static unsigned short colortable[] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static struct TIAinterface tia_interface =
{
	31400,
	255,
    TIA_DEFAULT_GAIN,
};

static struct POKEYinterface pokey_interface = {
	1,
    1790000,  
    { 255 },
	POKEY_DEFAULT_GAIN / 2,
    USE_CLIP
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            1790000,        /* 1.79Mhz (note: The clock switches to 1.19Mhz */
                            /* when the TIA or RIOT are accessed) */
			0,
			readmem,writemem,0,0,
			a7800_interrupt,262
		}
	},
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
    a7800_init_machine, /* init_machine */
    a7800_stop_machine, /* stop_machine */

	/* video hardware */
    640,263, {0,319,35,35+199},
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	0,

	VIDEO_TYPE_RASTER,
	0,
    a7800_vh_start,
    a7800_vh_stop,
    a7800_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_TIA,
			&tia_interface
		},
		{
		    SOUND_POKEY,
		    &pokey_interface
        }                   
    }

};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (a7800_rom)
	ROM_REGION (0x10000)
	ROM_LOAD ("7800.rom", 0xf000, 0x1000, 0x649913e5)       
ROM_END

struct GameDriver a7800_driver =
{
	__FILE__,
	0,      
	"a7800",
	"Atari 7800",
	"1986",
	"Atari",
	"Dan Boris",
	0,
	&machine_driver,
	0,

    a7800_rom,
    a7800_load_rom,
    a7800_id_rom,
	1,      /* number of ROM slots */
	0,      /* number of floppy drives supported */
	0,      /* number of hard drives supported */
	0,      /* number of cassette drives supported */
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

    0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0,
};
