/***************************************************************************
Apple II
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
extern unsigned char *apple2_slot1;
extern unsigned char *apple2_slot2;
extern unsigned char *apple2_slot3;
extern unsigned char *apple2_slot4;
extern unsigned char *apple2_slot5;
extern unsigned char *apple2_slot6;
extern unsigned char *apple2_slot7;

extern int	apple2_id_rom(const char *name, const char * gamename);

extern int	apple2e_load_rom(void);
extern int	apple2ee_load_rom(void);

extern void apple2_init_machine(void);

extern int  apple2_interrupt(void);

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
extern void apple2_c04x_w(int offset, int data);
extern void apple2_c05x_w(int offset, int data);
extern void apple2_c06x_w(int offset, int data);
extern void apple2_c07x_w(int offset, int data);
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
	{ 0x0000, 0xbfff, MRA_RAM },
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
	{ 0xc100, 0xc1ff, apple2_slot1_r, &apple2_slot1 },
	{ 0xc200, 0xc2ff, apple2_slot2_r, &apple2_slot2 },
	{ 0xc300, 0xc3ff, apple2_slot3_r, &apple2_slot3 },
	{ 0xc400, 0xc4ff, apple2_slot4_r, &apple2_slot4 },
	{ 0xc500, 0xc5ff, apple2_slot5_r, &apple2_slot5 },
	{ 0xc600, 0xc6ff, apple2_slot6_r, &apple2_slot6 },
	{ 0xc700, 0xc7ff, apple2_slot7_r, &apple2_slot7 },
	{ 0xc800, 0xcfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_BANK2 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, apple2_lores_text1_w, &apple2_lores_text1_ram },
	{ 0x0800, 0x0bff, apple2_lores_text2_w, &apple2_lores_text2_ram },
	{ 0x0c00, 0x1fff, MWA_RAM },
	{ 0x2000, 0x3fff, apple2_hires1_w, &apple2_hires1_ram },
	{ 0x4000, 0x5fff, apple2_hires2_w, &apple2_hires2_ram },
	{ 0x6000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, apple2_c00x_w },
	{ 0xc010, 0xc01f, apple2_c01x_w },
	{ 0xc020, 0xc02f, apple2_c02x_w },
	{ 0xc030, 0xc03f, apple2_c03x_w },
	{ 0xc040, 0xc04f, apple2_c04x_w },
	{ 0xc050, 0xc05f, apple2_c05x_w },
	{ 0xc060, 0xc06f, apple2_c06x_w },
	{ 0xc070, 0xc07f, apple2_c07x_w },
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
	{ 0xc400, 0xc4ff, apple2_slot4_w },
	{ 0xc500, 0xc5ff, apple2_slot5_w },
	{ 0xc600, 0xc6ff, apple2_slot6_w },
	{ 0xc700, 0xc7ff, apple2_slot7_w },
	{ 0xd000, 0xdfff, MWA_BANK1 },
	{ 0xe000, 0xffff, MWA_BANK2 },
	{ -1 }	/* end of table */
};

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( apple2_input_ports )

	PORT_START /* VBLANK */
	PORT_BIT ( 0xBF, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* Special keys */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN | IPF_TOGGLE, "Caps Lock", OSD_KEY_CAPSLOCK, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, "Open Apple", OSD_KEY_ALT, IP_JOY_DEFAULT, 0 )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2, "Closed Apple", OSD_KEY_ALTGR, IP_JOY_DEFAULT, 0 )

INPUT_PORTS_END

static struct GfxLayout apple2_text_layout =
{
	14,16,		   /* 14*16 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },	/* x offsets */
	{ 0*8, 0x8000, 1*8, 0x8000, 2*8, 0x8000, 3*8, 0x8000,
	  4*8, 0x8000, 5*8, 0x8000, 6*8, 0x8000, 7*8, 0x8000 },
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_dbl_text_layout =
{
	7,16,		   /* 7*16 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },	/* x offsets */
	{ 0*8, 0x8000, 1*8, 0x8000, 2*8, 0x8000, 3*8, 0x8000,
	  4*8, 0x8000, 5*8, 0x8000, 6*8, 0x8000, 7*8, 0x8000 },
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_lores_layout =
{
	14,16,		   /* 14*16 characters */
	0x100,		   /* 0x100 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7+16, 6+16, 5+16, 4+16, 3+16, 2+16, 1+16,
	  0+16, 7+16, 6+16, 5+16, 4+16, 3+16, 2+16 },	/* x offsets */
	{ 0,   0x8000, 0,   0x8000, 0,   0x8000, 0,   0x8000,
	  4*8, 0x8000, 4*8, 0x8000, 4*8, 0x8000, 4*8, 0x8000},
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_dbl_lores_layout =
{
	7,16,		   /* 7*16 characters */
	0x100,		   /* 0x100 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7+16, 6+16, 5+16, 4+16, 3+16, 2+16, 1+16 },	/* x offsets */
	{ 0,   0x8000, 0,   0x8000, 0,   0x8000, 0,   0x8000,
	  4*8, 0x8000, 4*8, 0x8000, 4*8, 0x8000, 4*8, 0x8000},
	8*8		   /* every char takes 8 bytes */
};

static struct GfxLayout apple2_hires_layout =
{
	14,2,		   /* 14*2 characters */
	0x80,		   /* 0x80 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },	/* x offsets */
	{ 0, 0x8000 },
	8*8		   /* every char takes 1 byte */
};

static struct GfxLayout apple2_hires_shifted_layout =
{
	14,2,		   /* 14*2 characters */
	0x80,		   /* 0x80 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 0, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1 },	/* x offsets */
	{ 0, 0x8000 },
	8*8		   /* every char takes 1 byte */
};

static struct GfxLayout apple2_dbl_hires_layout =
{
	7,2,		   /* 7*2 characters */
	0x800,		   /* 0x800 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },	/* x offsets */
	{ 0, 0x8000 },
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

static struct DACinterface apple2_DAC_interface =
{
	1,			/* number of DACs */
	22450,		/* DAC sampling rate: CPU clock / 80*/
	{255, },	/* volume */
	{0, }		/* filter rate */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1022727,			/* 1.022 Mhz */
			0,					/* Memory region #0 */
			readmem,writemem,
			0,0,				/* no readport, writeport for 6502 */
			apple2_interrupt,1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	apple2_init_machine,
	0,

	/* video hardware */
	280*2,							/* screen width */
	192*2,							/* screen height */
	{ 0, (280*2)-1,0,(192*2)-1},	/* visible_area */
	apple2_gfxdecodeinfo,			/* graphics decode info */
	sizeof(palette)/3,							/* 2 colors used for the characters */
	sizeof(colortable)/sizeof(unsigned short),	/* 2 colors used for the characters */
	0,								/* convert color prom */

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
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2e_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2E.CD",  0xC000, 0x2000, 0xa7e1852b)
	ROM_LOAD("A2E.EF",  0xE000, 0x2000, 0x4fad1f63)

	ROM_REGION(0x2000)
	ROM_LOAD("A2E.VID", 0x0000, 0x1000, 0x7c000000)
ROM_END

struct GameDriver apple2e_driver =
{
	__FILE__,
	0,
	"apple2e",
	"Apple //e",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apple2e_rom,
	apple2e_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

ROM_START(apple2ee_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2EE.CD",  0xC000, 0x2000, 0x3bc5a7bf)
	ROM_LOAD("A2EE.EF",  0xE000, 0x2000, 0x964787c7)

	ROM_REGION(0x2000)
	ROM_LOAD("A2EE.VID", 0x0000, 0x1000, 0xaec3264d)
ROM_END

struct GameDriver apple2ee_driver =
{
	__FILE__,
	0,
	"apple2ee",
	"Apple //e (enhanced)",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apple2ee_rom,
	apple2ee_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

ROM_START(apple2ep_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2EPT.CF",  0xC000, 0x4000, 0xd90c3178)

	ROM_REGION(0x2000)
	ROM_LOAD("A2EPT.VID", 0x0000, 0x1000, 0xaec3264d)
ROM_END

struct GameDriver apple2ep_driver =
{
	__FILE__,
	0,
	"apple2ep",
	"Apple //e (Platinum)",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apple2ep_rom,
	apple2ee_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

ROM_START(apple2c_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2C.128", 0xC000, 0x4000, 0x04a2b534)

	ROM_REGION(0x2000)
	ROM_LOAD("A2C.VID", 0x0000, 0x1000, 0xaec3264d)
ROM_END

struct GameDriver apple2c_driver =
{
	__FILE__,
	0,
	"apple2c",
	"Apple //c",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apple2c_rom,
	apple2ee_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

ROM_START(apple2c0_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2C.256", 0xC000, 0x4000, 0x851d3ae1)

	ROM_REGION(0x2000)
	ROM_LOAD("A2C.VID", 0x0000, 0x1000, 0xaec3264d)
ROM_END

struct GameDriver apple2c0_driver =
{
	__FILE__,
	0,
	"apple2c0",
	"Apple //c (3.5 ROM)",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apple2c0_rom,
	apple2ee_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

ROM_START(apl2cpls_rom)
	ROM_REGION(0x14000)
	ROM_LOAD("A2CPLUS.MON", 0xC000, 0x4000, 0x3f317275)

	ROM_REGION(0x2000)
	ROM_LOAD("A2CPLUS.VID", 0x0000, 0x1000, 0xaec3264d)
ROM_END

struct GameDriver apl2cpls_driver =
{
	__FILE__,
	0,
	"apl2cpls",
	"Apple //c Plus",
	"19??",
	"Apple Computer",
	"Mike Balfour",
	0,
	&machine_driver,

	apl2cpls_rom,
	apple2ee_load_rom, 		/* load rom_file images */
	apple2_id_rom,			/* identify rom images */
	1,						/* number of ROM slots - in this case, a CMD binary */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	apple2_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
	0,						/* state load */
	0						/* state save */
};

