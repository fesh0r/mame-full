/***************************************************************************

Apple II

This family of computers bank-switches everything up the wazoo.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/apple2.c */
extern int	apple2_vh_start(void);
extern void apple2_vh_stop(void);
extern void apple2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern unsigned char *apple2_lores_text1_ram;
extern unsigned char *apple2_lores_text2_ram;
extern unsigned char *apple2_hires1_ram;
extern unsigned char *apple2_hires2_ram;

extern void apple2_lores_text1_w(int offset, int data);
extern void apple2_lores_text2_w(int offset, int data);
extern void apple2_hires1_w(int offset, int data);
extern void apple2_hires2_w(int offset, int data);

/* machine/apple2.c */
extern unsigned char *apple2_slot_rom;
extern unsigned char *apple2_slot1;
extern unsigned char *apple2_slot2;
extern unsigned char *apple2_slot3;
extern unsigned char *apple2_slot4;
extern unsigned char *apple2_slot5;
extern unsigned char *apple2_slot6;
extern unsigned char *apple2_slot7;

extern int	apple2_floppy_init(int id, const char *name);

extern int  apple2_id_rom(const char *name, const char * gamename);

extern int	apple2e_load_rom(int id, const char *name);
extern int	apple2ee_load_rom(int id, const char *name);

extern void apple2e_init_machine(void);

extern int  apple2_interrupt(void);
extern void apple2_slotrom_disable(int offset, int data);

extern int  apple2_c00x_r(int offset);
extern int  apple2_c01x_r(int offset);
extern int  apple2_c02x_r(int offset);
extern int  apple2_c03x_r(int offset);
extern int  apple2_c04x_r(int offset);
extern int  apple2_c05x_r(int offset);
extern int  apple2_c06x_r(int offset);
extern int  apple2_c07x_r(int offset);
extern int  apple2_c08x_r(int offset);
extern int  apple2_c0xx_slot1_r(int offset);
extern int  apple2_c0xx_slot2_r(int offset);
extern int  apple2_c0xx_slot3_r(int offset);
extern int  apple2_c0xx_slot4_r(int offset);
extern int  apple2_c0xx_slot5_r(int offset);
extern int  apple2_c0xx_slot6_r(int offset);
extern int  apple2_c0xx_slot7_r(int offset);
extern int  apple2_slot1_r(int offset);
extern int  apple2_slot2_r(int offset);
extern int  apple2_slot3_r(int offset);
extern int  apple2_slot4_r(int offset);
extern int  apple2_slot5_r(int offset);
extern int  apple2_slot6_r(int offset);
extern int  apple2_slot7_r(int offset);

extern void apple2_c00x_w(int offset, int data);
extern void apple2_c01x_w(int offset, int data);
extern void apple2_c02x_w(int offset, int data);
extern void apple2_c03x_w(int offset, int data);
//extern void apple2_c04x_w(int offset, int data);
extern void apple2_c05x_w(int offset, int data);
//extern void apple2_c06x_w(int offset, int data);
//extern void apple2_c07x_w(int offset, int data);
extern void apple2_c08x_w(int offset, int data);
extern void apple2_c0xx_slot1_w(int offset, int data);
extern void apple2_c0xx_slot2_w(int offset, int data);
extern void apple2_c0xx_slot3_w(int offset, int data);
extern void apple2_c0xx_slot4_w(int offset, int data);
extern void apple2_c0xx_slot5_w(int offset, int data);
extern void apple2_c0xx_slot6_w(int offset, int data);
extern void apple2_c0xx_slot7_w(int offset, int data);
extern void apple2_slot1_w(int offset, int data);
extern void apple2_slot2_w(int offset, int data);
extern void apple2_slot3_w(int offset, int data);
extern void apple2_slot4_w(int offset, int data);
extern void apple2_slot5_w(int offset, int data);
extern void apple2_slot6_w(int offset, int data);
extern void apple2_slot7_w(int offset, int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_BANK4 },
	{ 0x0200, 0xbfff, MRA_BANK5 },
//	{ 0x0200, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc00f, apple2_c00x_r },
	{ 0xc010, 0xc01f, apple2_c01x_r },
	{ 0xc020, 0xc02f, apple2_c02x_r },
	{ 0xc030, 0xc03f, apple2_c03x_r },
	{ 0xc040, 0xc04f, apple2_c04x_r },
	{ 0xc050, 0xc05f, apple2_c05x_r },
	{ 0xc060, 0xc06f, apple2_c06x_r },
	{ 0xc070, 0xc07f, apple2_c07x_r },
	{ 0xc080, 0xc08f, apple2_c08x_r },
	{ 0xc090, 0xc09f, apple2_c0xx_slot1_r },
	{ 0xc0a0, 0xc0af, apple2_c0xx_slot2_r },
	{ 0xc0b0, 0xc0bf, apple2_c0xx_slot3_r },
	{ 0xc0c0, 0xc0cf, apple2_c0xx_slot4_r },
	{ 0xc0d0, 0xc0df, apple2_c0xx_slot5_r },
	{ 0xc0e0, 0xc0ef, apple2_c0xx_slot6_r },
	{ 0xc0f0, 0xc0ff, apple2_c0xx_slot7_r },
	{ 0xc400, 0xc4ff, apple2_slot4_r },
//  { 0xc100, 0xc7ff, MRA_BANK3, &apple2_slot_rom },
//	{ 0xc100, 0xc1ff, apple2_slot1_r, &apple2_slot1 },
//	{ 0xc200, 0xc2ff, apple2_slot2_r, &apple2_slot2 },
//	{ 0xc300, 0xc3ff, apple2_slot3_r, &apple2_slot3 },
//	{ 0xc500, 0xc5ff, apple2_slot5_r, &apple2_slot5 },
//	{ 0xc600, 0xc6ff, apple2_slot6_r, &apple2_slot6 },
//	{ 0xc700, 0xc7ff, apple2_slot7_r, &apple2_slot7 },
	{ 0xc800, 0xcffe, MRA_BANK6 },
//	{ 0xcfff, 0xcfff, apple2_slotrom_disable },
	{ 0xd000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_BANK2 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_BANK4 },
	{ 0x0200, 0xbfff, MWA_BANK5 },
//	{ 0x0200, 0x03ff, MWA_RAM },
//	{ 0x0400, 0x07ff, apple2_lores_text1_w, &apple2_lores_text1_ram },
//	{ 0x0800, 0x0bff, apple2_lores_text2_w, &apple2_lores_text2_ram },
//	{ 0x0c00, 0x1fff, MWA_RAM },
//	{ 0x2000, 0x3fff, apple2_hires1_w, &apple2_hires1_ram },
//	{ 0x4000, 0x5fff, apple2_hires2_w, &apple2_hires2_ram },
//	{ 0x6000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, apple2_c00x_w },
	{ 0xc010, 0xc01f, apple2_c01x_w },
	{ 0xc020, 0xc02f, apple2_c02x_w },
	{ 0xc030, 0xc03f, apple2_c03x_w },
//	{ 0xc040, 0xc04f, apple2_c04x_w },
	{ 0xc050, 0xc05f, apple2_c05x_w },
//	{ 0xc060, 0xc06f, apple2_c06x_w },
//	{ 0xc070, 0xc07f, apple2_c07x_w },
	{ 0xc080, 0xc08f, apple2_c08x_w },
	{ 0xc090, 0xc09f, apple2_c0xx_slot1_w },
	{ 0xc0a0, 0xc0af, apple2_c0xx_slot2_w },
	{ 0xc0b0, 0xc0bf, apple2_c0xx_slot3_w },
	{ 0xc0c0, 0xc0cf, apple2_c0xx_slot4_w },
	{ 0xc0d0, 0xc0df, apple2_c0xx_slot5_w },
	{ 0xc0e0, 0xc0ef, apple2_c0xx_slot6_w },
	{ 0xc0f0, 0xc0ff, apple2_c0xx_slot7_w },
	{ 0xc100, 0xc1ff, apple2_slot1_w },
	{ 0xc200, 0xc2ff, apple2_slot2_w },
	{ 0xc300, 0xc3ff, apple2_slot3_w },
	{ 0xc400, 0xc4ff, apple2_slot4_w, &apple2_slot4 },
	{ 0xc500, 0xc5ff, apple2_slot5_w },
	{ 0xc600, 0xc6ff, apple2_slot6_w },
	{ 0xc700, 0xc7ff, apple2_slot7_w },
	{ 0xd000, 0xdfff, MWA_BANK1 },
	{ 0xe000, 0xffff, MWA_BANK2 },
	{ -1 }	/* end of table */
};

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( apple2 )

	PORT_START /* VBLANK */
	PORT_BIT ( 0xBF, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* Special keys */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_DEFAULT )
//	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, "Open Apple", KEYCODE_ALT, IP_JOY_DEFAULT )
//	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2, "Closed Apple", KEYCODE_ALTGR, IP_JOY_DEFAULT )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3, "Button 3", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

INPUT_PORTS_END

static struct GfxLayout apple2_text_layout =
{
	14,8,		   /* 14*8 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },	/* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_dbl_text_layout =
{
	7,8,		   /* 7*8 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },	/* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_lores_layout =
{
	14,8,		   /* 14*8 characters */
	0x100,		   /* 0x100 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7+16, 6+16, 5+16, 4+16, 3+16, 2+16, 1+16,
	  0+16, 7+16, 6+16, 5+16, 4+16, 3+16, 2+16 },	/* x offsets */
	{ 0, 0, 0, 0, 4*8, 4*8, 4*8, 4*8 },
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_dbl_lores_layout =
{
	7,8,		   /* 7*16 characters */
	0x100,		   /* 0x100 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7+16, 6+16, 5+16, 4+16, 3+16, 2+16, 1+16 },	/* x offsets */
	{ 0, 0, 0, 0, 4*8, 4*8, 4*8, 4*8 },
    8*8        /* every char takes 8 bytes */
};

static struct GfxLayout apple2_hires_layout =
{
	14,1,		   /* 14*1 characters */
	0x80,		   /* 0x80 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },	/* x offsets */
	{ 0 },
	8*8		   /* every char takes 1 byte */
};

static struct GfxLayout apple2_hires_shifted_layout =
{
	14,1,		   /* 14*1 characters */
	0x80,		   /* 0x80 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 0, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1 },	/* x offsets */
	{ 0 },
	8*8		   /* every char takes 1 byte */
};

static struct GfxLayout apple2_dbl_hires_layout =
{
	7,1,		   /* 7*2 characters */
	0x800,		   /* 0x800 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },	/* x offsets */
	{ 0 },
	8*1		   /* every char takes 1 byte */
};

static struct GfxDecodeInfo apple2_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &apple2_text_layout, 0, 16 },
	{ 1, 0x0000, &apple2_dbl_text_layout, 0, 16 },
	{ 1, 0x0800, &apple2_lores_layout, 0, 16 },		/* Even characters */
	{ 1, 0x0801, &apple2_lores_layout, 0, 16 },		/* Odd characters */
	{ 1, 0x0800, &apple2_dbl_lores_layout, 0, 16 },
	{ 1, 0x0800, &apple2_hires_layout, 0, 16 },
	{ 1, 0x0C00, &apple2_hires_shifted_layout, 0, 16 },
	{ 1, 0x0800, &apple2_dbl_hires_layout, 0, 16 },
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	0x00, 0x00, 0x00, /* Black */
	0xD0, 0x00, 0x30, /* Dark Red */
	0x00, 0x00, 0x90, /* Dark Blue */
	0xD0, 0x20, 0xD0, /* Purple */
	0x00, 0x70, 0x20, /* Dark Green */
	0x50, 0x50, 0x50, /* Dark Grey */
	0x20, 0x20, 0xF0, /* Medium Blue */
	0x60, 0xA0, 0xF0, /* Light Blue */
	0x80, 0x50, 0x00, /* Brown */
	0xF0, 0x60, 0x00, /* Orange */
	0xA0, 0xA0, 0xA0, /* Light Grey */
	0xF0, 0x90, 0x80, /* Pink */
	0x10, 0xD0, 0x00, /* Light Green */
	0xF0, 0xF0, 0x00, /* Yellow */
	0x40, 0xF0, 0x90, /* Aquamarine */
	0xF0, 0xF0, 0xF0, /* White */
};

static unsigned short colortable[] =
{
	0,0,
	1,0,
	2,0,
	3,0,
	4,0,
	5,0,
	6,0,
	7,0,
	8,0,
	9,0,
	10,0,
	11,0,
	12,0,
	13,0,
	14,0,
	15,0,
};


/* Initialise the palette */
static void apple_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}


static struct DACinterface apple2_DAC_interface =
{
	1,			/* number of DACs */
	{ 100 }		/* volume */
};

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1022727,	/* 1.023 MHz */
	{ 100, 100 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver machine_driver_standard =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1022727,			/* 1.023 Mhz */
			readmem,writemem,
			0,0,				/* no readport, writeport for 6502 */
			apple2_interrupt,1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	apple2e_init_machine,
	0,

	/* video hardware */
	280*2,							/* screen width */
	192,							/* screen height */
	{ 0, (280*2)-1,0,192-1},		/* visible_area */
	apple2_gfxdecodeinfo,			/* graphics decode info */
	sizeof(palette)/3,							/* 2 colors used for the characters */
	sizeof(colortable)/sizeof(unsigned short),	/* 2 colors used for the characters */
	apple_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	apple2_vh_start,
	apple2_vh_stop,
	apple2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&apple2_DAC_interface
		},
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_enhanced =
{
	/* basic machine hardware */
	{
		{
			CPU_M65C02,
			1022727,			/* 1.023 Mhz */
			readmem,writemem,
			0,0,				/* no readport, writeport for 6502 */
			apple2_interrupt,1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	apple2e_init_machine,
	0,

	/* video hardware */
	280*2,							/* screen width */
	192,							/* screen height */
	{ 0, (280*2)-1,0,192-1},		/* visible_area */
	apple2_gfxdecodeinfo,			/* graphics decode info */
	sizeof(palette)/3,							/* 2 colors used for the characters */
	sizeof(colortable)/sizeof(unsigned short),	/* 2 colors used for the characters */
	apple_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	apple2_vh_start,
	apple2_vh_stop,
	apple2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&apple2_DAC_interface
		},
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2e)
	ROM_REGION(0x24700,REGION_CPU1)
	/* 64k main RAM, 64k aux RAM */
	ROM_LOAD ( "a2e.cd", 0x20000, 0x2000, 0xe248835e )
	ROM_LOAD ( "a2e.ef", 0x22000, 0x2000, 0xfc3d59d8 )
	/* 0x700 for individual slot ROMs */
	//ROM_LOAD ( "disk2_33.rom", 0x24500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */

	ROM_REGION(0x2000,REGION_GFX1)
	ROM_LOAD ( "a2e.vid", 0x0000, 0x1000, 0x816a86f1 )
ROM_END

ROM_START(apple2ee)
	ROM_REGION(0x24700,REGION_CPU1)
	ROM_LOAD ( "a2ee.cd", 0x20000, 0x2000, 0x443aa7c4 )
	ROM_LOAD ( "a2ee.ef", 0x22000, 0x2000, 0x95e10034 )
	/* 0x4000 for bankswitched RAM */
	/* 0x700 for individual slot ROMs */
	//ROM_LOAD ( "disk2_33.rom", 0x24500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */

	ROM_REGION(0x2000,REGION_GFX1)
	ROM_LOAD ( "a2ee.vid", 0x0000, 0x1000, 0x2651014d)
ROM_END

ROM_START(apple2ep)
	ROM_REGION(0x24700,REGION_CPU1)
	ROM_LOAD ("a2ept.cf", 0x20000, 0x4000, 0x02b648c8)
	/* 0x4000 for bankswitched RAM */
	/* 0x700 for individual slot ROMs */
	//ROM_LOAD ("disk2_33.rom", 0x24500, 0x0100, 0xce7144f6) /* Disk II ROM - DOS 3.3 version */

	ROM_REGION(0x2000,REGION_GFX1)
	ROM_LOAD("a2ept.vid", 0x0000, 0x1000, 0x2651014d)
ROM_END

ROM_START(apple2c)
	ROM_REGION(0x24700,REGION_CPU1)
	ROM_LOAD ( "a2c.128", 0x20000, 0x4000, 0xf0edaa1b )

	ROM_REGION(0x2000,REGION_GFX1)
	ROM_LOAD ( "a2c.vid", 0x0000, 0x1000, 0x2651014d )
ROM_END

ROM_START(apple2c0)
	ROM_REGION(0x28000,REGION_CPU1)
	ROM_LOAD("a2c.256", 0x20000, 0x8000, 0xc8b979b3)

	ROM_REGION(0x2000,REGION_GFX1)
	ROM_LOAD("a2c.vid", 0x0000, 0x1000, 0x2651014d)
ROM_END

ROM_START(apple2cp)
    ROM_REGION(0x28000,REGION_CPU1)
    ROM_LOAD("a2cplus.mon", 0x20000, 0x8000, 0x0b996420)

    ROM_REGION(0x2000,REGION_GFX1)
    ROM_LOAD("a2cplus.vid", 0x0000, 0x1000, 0x2651014d)
ROM_END

static const struct IODevice io_apple2c[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"???\0",				/* file extensions */
        NULL,               /* private */
		apple2_id_rom,		/* id */
		apple2e_load_rom,	/* init */
		NULL,				/* exit */
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
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"???\0",				/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		apple2_floppy_init, /* init */
		NULL,				/* exit */
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

static const struct IODevice io_apple2e[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"???\0",				/* file extensions */
        NULL,               /* private */
		apple2_id_rom,		/* id */
		apple2ee_load_rom,	/* init */
		NULL,				/* exit */
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
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"???\0",				/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		apple2_floppy_init, /* init */
		NULL,				/* exit */
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
#define io_apple2c0 io_apple2e
#define io_apple2cp io_apple2e
#define io_apple2ee io_apple2e
#define io_apple2ep io_apple2e

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY			  FULLNAME */
COMPX( 1982, apple2e,  0,		 standard, apple2,	 0, 	   "Apple Computers", "Apple IIe", GAME_NOT_WORKING )
COMPX( 19??, apple2ee, apple2e,  enhanced, apple2,	 0, 	   "Apple Computers", "Apple IIe (enhanced)", GAME_NOT_WORKING  )
COMPX( 19??, apple2ep, apple2e,  enhanced, apple2,	 0, 	   "Apple Computers", "Apple IIe (Platinum)", GAME_NOT_WORKING  )
COMP ( 1984, apple2c,  0,		 enhanced, apple2,	 0, 	   "Apple Computers", "Apple IIc" )
COMP ( 19??, apple2c0, apple2c,  enhanced, apple2,	 0, 	   "Apple Computers", "Apple IIc (3.5 ROM)" )
COMP ( 19??, apple2cp, apple2c,  enhanced, apple2,	 0, 	   "Apple Computers", "Apple IIc Plus" )

