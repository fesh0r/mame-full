/***************************************************************************

  nes.c

  Driver file to handle emulation of the Nintendo Entertainment System (Famicom).

  Brad Oliver
  Chuck Mason
  Richard Bannister
  Nicolas Hamel
  Jeff Mitchell
  Nicola Salmoria (sound)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/machine/nes.h"
#include "cpu/m6502/m6502.h"

/* machine/nes.c */
int nes_load_rom (int id, const char *rom_name);
int nes_id_rom (const char *name, const char *gamename);
void nes_init_machine (void);
int nes_interrupt (void);
int nes_ppu_r (int offset);
int nes_IN0_r (int offset);
int nes_IN1_r (int offset);
void nes_IN0_w (int offset, int data);
void nes_IN1_w (int offset, int data);
void nes_ppu_w (int offset, int data);

void nes_mapper_w (int offset, int data);
void nes_strange_mapper_w (int offset, int data);
int nes_strange_mapper_r (int offset);

/* vidhrdw/nes.c */
int nes_vh_start (void);
void nes_vh_stop (void);
void nes_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
void nes_vh_sprite_dma_w (int offset, int data);

unsigned char *battery_ram;
unsigned char *main_ram;
unsigned char *nes_io_0;
unsigned char *nes_io_1;

int nes_mirrorram_r (int offset)
{
	return main_ram[offset & 0x7ff];
}

void nes_mirrorram_w (int offset, int data)
{
	main_ram[offset & 0x7ff] = data;
}

void battery_ram_w (int offset, int data)
{
	extern int Mapper;
	int i;
	unsigned char *RAM = memory_region(REGION_CPU1);

	battery_ram[offset] = data;

	/* Handle mapper junk */
	switch (Mapper)
	{
		case 34:
			if (errorlog) fprintf (errorlog, "Mapper 34 (trainer) w, offset: %04x, data: %02x\n", offset, data);
			switch (offset)
			{
				case 0x1ffd:
					/* Switch 32k NES_ROM banks */
					data &= ((PRG_Rom >> 1) - 1);
					cpu_setbank (1, &RAM[data * 0x8000 + 0x10000]);
					cpu_setbank (2, &RAM[data * 0x8000 + 0x12000]);
					cpu_setbank (3, &RAM[data * 0x8000 + 0x14000]);
					cpu_setbank (4, &RAM[data * 0x8000 + 0x16000]);
					break;
				case 0x1ffe:
					/* Switch 4k VNES_ROM at 0x0000 */
					data &= ((CHR_Rom << 1) - 1);
					for (i = 0; i < 4; i ++)
						nes_vram[i] = (data) * 256 + 64*i;
					break;
				case 0x1fff:
					/* Switch 4k VNES_ROM at 0x1000 */
					data &= ((CHR_Rom << 1) - 1);
					for (i = 4; i < 8; i ++)
						nes_vram[i] = (data) * 256 + 64*(i-4);
					break;
			}
			break;
		case 79:
			if (errorlog) fprintf (errorlog, "Mapper 79 (trainer) w, offset: %04x, data: %02x\n", offset, data);
			if (offset & 0x0100)
			/* Select 8k VNES_ROM bank */
			{
				data &= (CHR_Rom - 1);
				for (i = 0; i < 8; i ++)
					nes_vram[i] = (data) * 512 + 64*i;
			}
			break;

		default:
			break;
	}
}

int nes_bogus_r (int offset)
{
	static int val = 0xff;

	val ^= 0xff;
	return val;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xffff, MRA_BANK4 },
	{ 0xc000, 0xdfff, MRA_BANK3 },
	{ 0x8000, 0x9fff, MRA_BANK1 }, /* 4 16k NES_ROM banks */
	{ 0xa000, 0xbfff, MRA_BANK2 },
	{ 0x0000, 0x07ff, MRA_RAM   },   /* RAM */
	{ 0x2000, 0x3fff, nes_ppu_r }, /* PPU registers */
	{ 0x6000, 0x7fff, MRA_RAM },   /* Trainer/battery RAM */
	{ 0x4016, 0x4016, nes_IN0_r },	/* IN0 - gamepad 1 */
	{ 0x4017, 0x4017, nes_IN1_r },	/* IN1 - gamepad 2 */
	{ 0x4015, 0x4015, nes_bogus_r },	/* ?? sound status ?? */
	{ 0x4100, 0x5fff, nes_strange_mapper_r }, /* Perform more unholy acts on the machine */
	{ 0x0800, 0x1fff, nes_mirrorram_r },   /* mirrors of RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM, &main_ram },
	{ 0x2000, 0x3fff, nes_ppu_w }, /* PPU registers */
	{ 0x8000, 0xffff, nes_mapper_w }, /* Perform unholy acts on the machine */
	{ 0x4100, 0x5fff, nes_strange_mapper_w }, /* Perform more unholy acts on the machine */
	{ 0x6000, 0x7fff, battery_ram_w, &battery_ram },   /* Battery RAM */
	{ 0x4016, 0x4016, nes_IN0_w, &nes_io_0 },	/* IN0 - gamepad 1 */
	{ 0x4017, 0x4017, nes_IN1_w, &nes_io_1 },	/* IN1 - gamepad 2 */
	{ 0x4014, 0x4014, nes_vh_sprite_dma_w }, /* transfer 0x100 of data to sprite ram */
	{ 0x4011, 0x4011, DAC_data_w },
	{ 0x4000, 0x4015, NESPSG_0_w },
	{ 0x0800, 0x1fff, nes_mirrorram_w },   /* mirrors of RAM */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( nes )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 - fake */
	//PORT_DIPNAME( 0x01, 0x00, "Renderer", IP_KEY_NONE )
	PORT_DIPNAME( 0x01, 0x00, "Renderer")
	PORT_DIPSETTING(    0x00, "Scanline" )
	PORT_DIPSETTING(    0x01, "Experimental" )

	PORT_START	/* IN3 - fake */
	//PORT_DIPNAME( 0x01, 0x00, "Split-Screen Fix", IP_KEY_NONE )
	PORT_DIPNAME( 0x01, 0x00, "Split-Screen Fix")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END

/* !! Warning: the charlayout is changed by nes_load_rom !! */
struct GfxLayout nes_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters - changed at runtime */
	2,	/* 2 bits per pixel */
	{ 8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

/* This layout is not changed at runtime */
struct GfxLayout nes_vram_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};


static struct GfxDecodeInfo nes_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &nes_charlayout,        0, 8 },
	{ 2, 0x0000, &nes_vram_charlayout,   0, 8 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x1D*4, 0x1D*4, 0x1D*4, /* Value 0 */
	0x09*4, 0x06*4, 0x23*4, /* Value 1 */
	0x00*4, 0x00*4, 0x2A*4, /* Value 2 */
	0x11*4, 0x00*4, 0x27*4, /* Value 3 */
	0x23*4, 0x00*4, 0x1D*4, /* Value 4 */
	0x2A*4, 0x00*4, 0x04*4, /* Value 5 */
	0x29*4, 0x00*4, 0x00*4, /* Value 6 */
	0x1F*4, 0x02*4, 0x00*4, /* Value 7 */
	0x10*4, 0x0B*4, 0x00*4, /* Value 8 */
	0x00*4, 0x11*4, 0x00*4, /* Value 9 */
	0x00*4, 0x14*4, 0x00*4, /* Value 10 */
	0x00*4, 0x0F*4, 0x05*4, /* Value 11 */
	0x06*4, 0x0F*4, 0x17*4, /* Value 12 */
	0x00*4, 0x00*4, 0x00*4, /* Value 13 */
	0x00*4, 0x00*4, 0x00*4, /* Value 14 */
	0x00*4, 0x00*4, 0x00*4, /* Value 15 */

	0x2F*4, 0x2F*4, 0x2F*4, /* Value 16 */
	0x00*4, 0x1C*4, 0x3B*4, /* Value 17 */
	0x08*4, 0x0E*4, 0x3B*4, /* Value 18 */
	0x20*4, 0x00*4, 0x3C*4, /* Value 19 */
	0x2F*4, 0x00*4, 0x2F*4, /* Value 20 */
	0x39*4, 0x00*4, 0x16*4, /* Value 21 */
	0x36*4, 0x0A*4, 0x00*4, /* Value 22 */
	0x32*4, 0x13*4, 0x03*4, /* Value 23 */
	0x22*4, 0x1C*4, 0x00*4, /* Value 24 */
	0x00*4, 0x25*4, 0x00*4, /* Value 25 */
	0x00*4, 0x2A*4, 0x00*4, /* Value 26 */
	0x00*4, 0x24*4, 0x0E*4, /* Value 27 */
	0x00*4, 0x20*4, 0x22*4, /* Value 28 */
	0x00*4, 0x00*4, 0x00*4, /* Value 29 */
	0x00*4, 0x00*4, 0x00*4, /* Value 30 */
	0x00*4, 0x00*4, 0x00*4, /* Value 31 */

	0x3F*4, 0x3F*4, 0x3F*4, /* Value 32 */
	0x0F*4, 0x2F*4, 0x3F*4, /* Value 33 */
	0x17*4, 0x25*4, 0x3F*4, /* Value 34 */
	0x10*4, 0x22*4, 0x3F*4, /* Value 35 */
	0x3D*4, 0x1E*4, 0x3F*4, /* Value 36 */
	0x3F*4, 0x1D*4, 0x2D*4, /* Value 37 */
	0x3F*4, 0x1D*4, 0x18*4, /* Value 38 */
	0x3F*4, 0x26*4, 0x0E*4, /* Value 39 */
	0x3C*4, 0x2F*4, 0x0F*4, /* Value 40 */
	0x20*4, 0x34*4, 0x04*4, /* Value 41 */
	0x13*4, 0x37*4, 0x12*4, /* Value 42 */
	0x16*4, 0x3E*4, 0x26*4, /* Value 43 */
	0x00*4, 0x3A*4, 0x36*4, /* Value 44 */
	0x00*4, 0x00*4, 0x00*4, /* Value 45 */
	0x00*4, 0x00*4, 0x00*4, /* Value 46 */
	0x00*4, 0x00*4, 0x00*4, /* Value 47 */

	0x3F*4, 0x3F*4, 0x3F*4, /* Value 48 */
	0x2A*4, 0x39*4, 0x3F*4, /* Value 49 */
	0x31*4, 0x35*4, 0x3F*4, /* Value 50 */
	0x35*4, 0x32*4, 0x3F*4, /* Value 51 */
	0x3F*4, 0x31*4, 0x3F*4, /* Value 52 */
	0x3F*4, 0x31*4, 0x36*4, /* Value 53 */
	0x3F*4, 0x2F*4, 0x2C*4, /* Value 54 */
	0x3F*4, 0x36*4, 0x2A*4, /* Value 55 */
	0x3F*4, 0x39*4, 0x28*4, /* Value 56 */
	0x38*4, 0x3F*4, 0x28*4, /* Value 57 */
	0x2A*4, 0x3C*4, 0x2F*4, /* Value 58 */
	0x2C*4, 0x3F*4, 0x33*4, /* Value 59 */
	0x27*4, 0x3F*4, 0x3C*4, /* Value 60 */
	0x00*4, 0x00*4, 0x00*4, /* Value 61 */
	0x00*4, 0x00*4, 0x00*4, /* Value 62 */
	0x00*4, 0x00*4, 0x00*4, /* Value 63 */

	/* I don't believe the hardware used colors > 63 */
	0x00*4, 0x00*4, 0x00*4, /* Value 64 */
	0x00*4, 0x00*4, 0x00*4, /* Value 65 */
	0x00*4, 0x00*4, 0x00*4, /* Value 66 */
	0x00*4, 0x00*4, 0x00*4, /* Value 67 */
	0x00*4, 0x00*4, 0x00*4, /* Value 68 */
	0x00*4, 0x00*4, 0x00*4, /* Value 69 */
	0x3F*4, 0x00*4, 0x00*4, /* Value 70 */
	0x00*4, 0x3F*4, 0x00*4, /* Value 71 */
	0x00*4, 0x00*4, 0x3F*4, /* Value 72 */
};

/* Changed at runtime */
static unsigned short colortable[] =
{
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
};

/* Initialise the palette */
static void nes_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}




static struct NESinterface nes_interface =
{
	1,
	//28624000 ,	/* 16 * 1.79MHz = 28.624 MHz */
	{ REGION_CPU1 },		/* no handler */
	{ 100 },
};

static struct DACinterface nes_dac_interface =
{
	1,
	{ 100 }
};


ROM_START( nes )
	ROM_REGION( 0x410000, REGION_CPU1 )
	ROM_REGION( 0x200000, REGION_GFX1 )
	ROM_REGION( 0x2000,  REGION_GFX2 )
ROM_END



static struct MachineDriver machine_driver_nes =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1789000,	/* 1.79 Mhz */
			readmem,writemem,0,0,
			nes_interrupt,SCANLINES_PER_FRAME /* one for each scanline - ugh */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	nes_init_machine,
	0,

	/* video hardware */
	32*8, 30*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	nes_gfxdecodeinfo,
	(sizeof (palette))/3, sizeof(colortable)/sizeof(unsigned short),
	nes_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	nes_vh_start,
	nes_vh_stop,
	nes_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NES,
			&nes_interface
		},
		{
			SOUND_DAC,
			&nes_dac_interface
		}
	}
};


static const struct IODevice io_nes[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"nes\0",            /* file extensions */
		NULL,               /* private */
		nes_id_rom, 		/* id */
		nes_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
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

//#define rom_nes NULL

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
CONS( 1983, nes,	   0,		 nes,	   nes, 	 0,		   "Nintendo", "Nintendo Entertainment System/Famicom" )

/* 1983 Famicom released in Japan
   1985 US Launch
   1987 Official UK distribution of the NES finally begins through toy company Mattel. */

