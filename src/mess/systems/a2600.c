/***************************************************************************

  a2600.c

  Driver file to handle emulation of the Atari 2600.

  --Have to implement the playfield graphics register--

 Contains the addresses of the 2600 hardware

 TIA *Write* Addresses (6 bit)


	VSYNC	0x00     Vertical Sync Set-Clear
	VBLANK	0x01     Vertical Blank Set-Clear
	WSYNC	0x02     Wait for Horizontal Blank
	RSYNC	0x03     Reset Horizontal Sync Counter
	NUSIZ0	0x04     Number-Size player/missle 0
	NUSIZ1	0x05     Number-Size player/missle 1
	COLUP0	0x06     Color-Luminance Player 0
	COLUP1	0x07     Color-Luminance Player 1
	COLUPF	0x08     Color-Luminance Playfield
	COLUBK	0x09     Color-Luminance BackGround
	CTRLPF	0x0A     Control Playfield, Ball, Collisions
	REFP0	0x0B     Reflection Player 0
	REFP1	0x0C     Reflection Player 1
	PF0	    0x0D     Playfield Register Byte 0
	PF1	    0x0E     Playfield Register Byte 1
	PF2	    0x0F     Playfield Register Byte 2
	RESP0	0x10     Reset Player 0
	RESP1	0x11     Reset Player 1
	RESM0	0x12     Reset Missle 0
	RESM1	0x13     Reset Missle 1
	RESBL	0x14     Reset Ball

	AUDC0	0x15    Audio Control 0
	AUDC1	0x16    Audio Control 1
	AUDF0	0x17    Audio Frequency 0
	AUDF1	0x18    Audio Frequency 1
	AUDV0	0x19    Audio Volume 0
	AUDV1	0x1A    Audio Volume 1
	GRP0	0x1B    Graphics Register Player 0
	GRP1	0x1C    Graphics Register Player 0
	ENAM0	0x1D    Graphics Enable Missle 0
	ENAM1	0x1E    Graphics Enable Missle 1
	ENABL	0x1F    Graphics Enable Ball
	HMP0	0x20    Horizontal Motion Player 0
	HMP1	0x21    Horizontal Motion Player 0
	HMM0	0x22    Horizontal Motion Missle 0
	HMM1	0x23    Horizontal Motion Missle 1
	HMBL	0x24    Horizontal Motion Ball
	VDELP0	0x25    Vertical Delay Player 0
	VDELP1	0x26    Vertical Delay Player 1
	VDELBL	0x27    Vertical Delay Ball
	RESMP0	0x28    Reset Missle 0 to Player 0
	RESMP1	0x29    Reset Missle 1 to Player 1
	HMOVE	0x2A    Apply Horizontal Motion
	HMCLR	0x2B    Clear Horizontal Move Registers
	CXCLR	0x2C    Clear Collision Latches


 TIA *Read* Addresses
                                  bit 6  bit 7
	CXM0P	0x0    Read Collision M0-P1  M0-P0
	CXM1P	0x1                   M1-P0  M1-P1
	CXP0FB	0x2                   P0-PF  P0-BL
	CXP1FB	0x3                   P1-PF  P1-BL
	CXM0FB	0x4                   M0-PF  M0-BL
	CXM1FB	0x5                   M1-PF  M1-BL
	CXBLPF	0x6                   BL-PF  -----
	CXPPMM	0x7                   P0-P1  M0-M1
	INPT0	0x8     Read Pot Port 0
	INPT1	0x9     Read Pot Port 1
	INPT2	0xA     Read Pot Port 2
	INPT3	0xB     Read Pot Port 3
	INPT4	0xC     Read Input (Trigger) 0
	INPT5	0xD     Read Input (Trigger) 1


 RIOT Addresses

	RAM	    0x80 - 0xff           RAM 0x0180-0x01FF

	SWCHA	0x280   Port A data rwegister (joysticks)
	SWACNT	0x281   Port A data direction register (DDR)
	SWCHB	0x282   Port B data (Console Switches)
	SWBCNT	0x283   Port B DDR
	INTIM	0x284   Timer Output

	TIM1T	0x294   set 1 clock interval
	TIM8T	0x295   set 8 clock interval
	TIM64T	0x296   set 64 clock interval
	T1024T	0x297   set 1024 clock interval
                      these are also at 0x380-0x397

	ROM	0xF000	 To FFFF,0x1000-1FFF

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"
#include "machine/6821pia.h"
#include "sound/hc55516.h"
#include "mess/machine/riot.h"

/* vidhrdw/a2600.c */
extern int a2600_vh_start(void);
extern void a2600_vh_stop(void);
extern void a2600_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
//extern int a2600_interrupt(void);

/* machine/a2600.c */
extern int a2600_TIA_r(int offset);
extern void a2600_TIA_w(int offset, int data);
extern void a2600_init_machine(void);
extern void a2600_stop_machine(void);
extern int  a2600_id_rom (const char *name, const char *gamename);
extern int	a2600_load_rom(int id, const char *rom_name);
extern int  a2600_ROM_r (int offset);


/* horrid memory mirroring ahead */
static struct MemoryReadAddress readmem[] =
{

	{ 0x0000, 0x003F, a2600_TIA_r },
	{ 0x0040, 0x007F, a2600_TIA_r },
	{ 0x0080, 0x00FF, MRA_RAM     },

	{ 0x0100, 0x013F, a2600_TIA_r },
	{ 0x0140, 0x017F, a2600_TIA_r },
	{ 0x0180, 0x01FF, MRA_RAM     },

	{ 0x0200, 0x023F, a2600_TIA_r },
	{ 0x0240, 0x027F, a2600_TIA_r },

	//{ 0x0280, 0x0297, riot_0_r    },	/* RIOT reads for a2600 */
	{ 0x0280, 0x028F, riot_0_r    },	/* RIOT reads for a2600 */
	{ 0x0290, 0x0297, riot_0_r    },	/* RIOT reads for a2600 */

	{ 0x0300, 0x033F, a2600_TIA_r },
	{ 0x0340, 0x037F, a2600_TIA_r },

	//{ 0x0380, 0x0397, riot_0_r    },	/* RIOT reads for a2600 */
	{ 0x0380, 0x038F, riot_0_r    },	/* RIOT reads for a2600 */
	{ 0x0390, 0x0397, riot_0_r    },	/* RIOT reads for a2600 */

	{ 0x1000, 0x17FF, MRA_ROM     },
	{ 0x1800, 0x1FFF, MRA_ROM     },	/* ROM mirror for 2k images */
	{ 0xF000, 0xF7FF, MRA_ROM     },
	{ 0xF800, 0xFFFF, MRA_ROM     },	/* ROM mirror for 2k images */
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x003F, a2600_TIA_w },
	{ 0x0040, 0x007F, a2600_TIA_w },
	{ 0x0080, 0x00FF, MWA_RAM  },

	{ 0x0100, 0x013F, a2600_TIA_w },
	{ 0x0140, 0x017F, a2600_TIA_w },
	{ 0x0180, 0x01FF, MWA_RAM  },

	{ 0x0200, 0x023F, a2600_TIA_w },
	{ 0x0240, 0x027F, a2600_TIA_w },
	//{ 0x0280, 0x0297, riot_0_w    },	/* RIOT writes for a2600 */
	{ 0x0280, 0x028F, riot_0_w    },	/* RIOT writes for a2600 */
	{ 0x0290, 0x0297, riot_0_w    },	/* RIOT writes for a2600 */

	{ 0x0300, 0x033F, a2600_TIA_w },
	{ 0x0340, 0x037F, a2600_TIA_w },
	//{ 0x0380, 0x0397, riot_0_w    },	/* RIOT writes for a2600 */
	{ 0x0380, 0x038F, riot_0_w    },	/* RIOT writes for a2600 */
	{ 0x0390, 0x0397, riot_0_w    },	/* RIOT writes for a2600 */

	{ 0x1000, 0x17FF, MWA_ROM  },
	{ 0x1800, 0x1FFF, MWA_ROM  },	/* ROM mirror for 2k images */
	{ 0xF000, 0xF7FF, MWA_ROM  },
	{ 0xF800, 0xFFFF, MWA_ROM  },	/* ROM mirror for 2k images */
    { -1 }  /* end of table */
};


INPUT_PORTS_START( a2600 )
	PORT_START      /* IN0 DONE!*/
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


/* a7800 palette and colortable */
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

/* Initialise the palette */
static void a2600_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};


#ifdef GFX_DECODE
static struct GfxLayout pixel4_width_1 =
{
	4,1,					/* 4 x 1 pixels (PF0) */
	16, 					/* 16 codes */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3 },
	/* y offsets */
	{ 0 },
	8*1 					/* every code takes 1 byte */
};

static struct GfxLayout pixel4_width_2 =
{
	2*4,1,					/* 2*4 x 1 pixels (PF0) */
	16, 					/* 16 codes */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 0, 1, 1, 2, 2, 3, 3 },
	/* y offsets */
	{ 0 },
	8*1 					/* every code takes 1 byte */
};

static struct GfxLayout pixel8_width_1 =
{
	8,1,					/* 8 x 1 pixels (PF0) */
	256,					/* 256 codes */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0 },
	8*1 					/* every code takes 1 byte */
};

static struct GfxLayout pixel8_width_2 =
{
	2*8,1,					/* 2*8 x 1 pixels (PF0) */
	256,					/* 256 codes */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 7,7, 6,6, 5,5, 4,4, 3,3, 2,2, 1,1, 0,0 },
	/* y offsets */
	{ 0 },
	8*1 					/* every code takes 1 byte */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &pixel4_width_1, 0, 16},
	{REGION_GFX1, 0x0000, &pixel8_width_1, 0, 16},
	{REGION_GFX1, 0x0000, &pixel4_width_2, 0, 16},
	{REGION_GFX1, 0x0000, &pixel8_width_2, 0, 16},
    { -1 } /* end of array */
};
#endif

static struct TIAinterface tia_interface =
{
	31400,
	255,
    TIA_DEFAULT_GAIN,
};


static struct MachineDriver machine_driver_a2600 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            1190000,        /* 1.19Mhz */
			readmem,writemem

		}
	},
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
    a2600_init_machine, /* init_machine */
    a2600_stop_machine, /* stop_machine */

	/* video hardware */
    228,262, {0,227,37,35+192},
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	a2600_init_palette,

	VIDEO_TYPE_RASTER,
	0,
    a2600_vh_start,
    a2600_vh_stop,
    a2600_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_TIA,
			&tia_interface
		}

    }

};


/***************************************************************************

  Game driver

***************************************************************************/

/* setup a 8bit pattern from 0x00 to 0xff into the REGION_GFX1 memory */
static void init_a2600(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;
	for( i = 0; i < 256; i++ )
		gfx[i] = i;
}

ROM_START( a2600 )
	ROM_REGION( 0x10000, REGION_CPU1 ) /* 6502 memory */
	ROM_REGION( 0x00100, REGION_GFX1 ) /* memory for bit patterns */
ROM_END

static const struct IODevice io_a2600[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",            /* file extensions */
		NULL,				/* private */
		a2600_id_rom,		/* id */
		a2600_load_rom, 	/* init */
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

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
CONS( 19??, a2600,	  0,		a2600,	  a2600,	a2600,	  "Atari",  "Atari 2600 - VCS" )

