/******************************************************************************

	Tatung Einstein
	system driver


	TMS9129 VDP Graphics
	Z80 CPU (4Mhz)
	Z80 CTC (4Mhz)
	Intel 8251 Serial (2Mhz clock?)
	WD1770 Floppy Disc controller
	AY-3-8910 PSG (2Mhz)

	printer connected to port A of PIO
	user port is port B of PIO
	keyboard connected to port A and port B of PSG

	Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/tms9928a.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "includes/basicdsk.h"
#include "includes/msm8251.h"

#define EINSTEIN_SYSTEM_CLOCK 4000000

static unsigned char *einstein_ram = NULL;

static int einstein_rom_enabled = 1;

int einstein_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY,id)==NULL)
		return INIT_PASS;

	if (basicdsk_floppy_init(id)==INIT_OK)
	{
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void einstein_z80fmly_interrupt(int state)
{
	cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}


static z80ctc_interface	einstein_ctc_intf =
{
	1,
	{EINSTEIN_SYSTEM_CLOCK},
	{0},
	{einstein_z80fmly_interrupt},
	{0},
	{0},
    {0}
};


static z80pio_interface einstein_pio_intf = 
{
	1,
	{einstein_z80fmly_interrupt},
	{0},
	{NULL}
};

static Z80_DaisyChain einstein_daisy_chain[] =
{
    {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
	{z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
    {0,0,0,-1}
};

int einstein_vh_init(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);
}

static READ_HANDLER(einstein_vdp_r)
{
	logerror("vdp r: %04x\n",offset);

	if (offset & 0x01)
	{
		return TMS9928A_register_r(offset & 0x01);
	}

	return TMS9928A_vram_r(offset & 0x01);
}

static WRITE_HANDLER(einstein_vdp_w)
{
	logerror("vdp w: %04x %02x\n",offset,data);

	if (offset & 0x01)
	{
		TMS9928A_register_w(offset & 0x01,data);
		return;
	}

	TMS9928A_vram_w(offset & 0x01,data);
}

static WRITE_HANDLER(einstein_fdc_w)
{
	int reg = offset & 0x03;

	logerror("fdc w: %04x %02x\n",offset,data);

	switch (reg)
	{
		case 0:
		{
			wd179x_command_w(reg, data);
		}
		break;
	
		case 1:
		{
			wd179x_track_w(reg, data);
		}
		break;
	
		case 2:
		{
			wd179x_sector_w(reg, data);
		}
		break;

		case 3:
		{
			wd179x_data_w(reg, data);
		}
		break;
	}
}


static READ_HANDLER(einstein_fdc_r)
{
	int reg = offset & 0x03;

	logerror("fdc r: %04x\n",offset);

	switch (reg)
	{
		case 0:
		{
			return wd179x_status_r(reg);
		}
		break;
	
		case 1:
		{
			return wd179x_track_r(reg);
		}
		break;
	
		case 2:
		{
			return wd179x_sector_r(reg);
		}
		break;

		case 3:
		{
			return wd179x_data_r(reg);
		}
		break;
	}

	return 0x0ff;
}

static WRITE_HANDLER(einstein_pio_w)
{
	logerror("pio w: %04x %02x\n",offset,data);

	if ((offset & 0x01)==0)
	{
		z80pio_d_w( 0, (offset>>1) & 0x01,data);
		return;
	}

	z80pio_c_w( 0, (offset>>1) & 0x01,data);
}

static READ_HANDLER(einstein_pio_r)
{
	logerror("pio r: %04x\n",offset);

	if ((offset & 0x01)==0)
	{
		return z80pio_d_r( 0, (offset>>1) & 0x01);
	}

	return z80pio_c_r( 0, (offset>>1) & 0x01);
}

static READ_HANDLER(einstein_ctc_r)
{
	logerror("ctc r: %04x\n",offset);
	
	return z80ctc_0_r(offset & 0x03);
}

static WRITE_HANDLER(einstein_ctc_w)
{
	logerror("ctc w: %04x %02x\n",offset,data);

	z80ctc_0_w(offset & 0x03,data);
}

static WRITE_HANDLER(einstein_serial_w)
{
	int reg = offset & 0x01;

	logerror("serial w: %04x %02x\n",offset,data);

	if ((reg)==0)
	{
		msm8251_data_w(reg,data);
		return;
	}
	
	msm8251_control_w(reg,data);
}


static READ_HANDLER(einstein_serial_r)
{
	int reg = offset & 0x01;

	logerror("serial r: %04x\n",offset);

	if ((reg)==0)
	{
		return msm8251_data_r(reg);
	}
	
	return msm8251_status_r(reg);
}

static WRITE_HANDLER(einstein_psg_w)
{
	int reg = offset & 0x01;

	logerror("psg w: %04x %02x\n",offset,data);

	if (reg==0)
	{
		AY8910_control_port_0_w(0, data);
		return;
	}
	
	AY8910_write_port_0_w(0, data);
}

static READ_HANDLER(einstein_psg_r)
{
	int reg = offset & 0x01;

	logerror("psg r: %04x\n",offset);
	if (reg==0)
	{
		return 0x0ff;
	}
	
	return AY8910_read_port_0_r(0);
}
	

/* int priority */
/* keyboard int->ctc/adc->pio */


/* 0x00-0x07 PSG */
/* 0x08-0x0f VDP ok */
/* 0x10-0x17 SERIAL ok */
/* 0x18-0x1f FDC ok */
/* 0x20-0x27 MISC */
/* 0x28-0x2f CTC ok */
/* 0x30-0x37 PIO ok */
/* 0x38-0x3f ADC */

/* MISC */
/* 0x20 - bit 0 is keyboard int mask */
/* 0x21 - bit 0 is adc int mask */
/* 0x22 - alph */
/* 0x23 - drive parameters */
/* 0x24 - rom */
/* 0x25 - bit 0 is fire int mask */
/* 0x25 - */
/* 0x26 - */
/* 0x27 - */

/* 0x40-0xff for expansion */

MEMORY_READ_START( readmem_einstein )
	{0x0000, 0x01fff, MRA_BANK1},
	{0x2000, 0x0ffff, MRA_BANK2},
MEMORY_END



MEMORY_WRITE_START( writemem_einstein )
	{0x0000, 0x0ffff, MWA_BANK3},
MEMORY_END

static void einstein_page_rom(void)
{
	if (einstein_rom_enabled)
	{
		cpu_setbank(1, memory_region(REGION_CPU1)+0x010000);
	}
	else
	{
		cpu_setbank(1, einstein_ram);
	}
}



static WRITE_HANDLER(einstein_rom_w)
{
	einstein_rom_enabled^=1;
	einstein_page_rom();
}


PORT_READ_START( readport_einstein )
	{0x000, 0x007, einstein_psg_r},
	{0x008, 0x00f, einstein_vdp_r},
	{0x010, 0x017, einstein_serial_r},
	{0x018, 0x01f, einstein_fdc_r},
	{0x028, 0x02f, einstein_ctc_r},
	{0x030, 0x037, einstein_pio_r},
PORT_END

PORT_WRITE_START( writeport_einstein )
	{0x000, 0x007, einstein_psg_w},
	{0x008, 0x00f, einstein_vdp_w},
	{0x010, 0x017, einstein_serial_w},
	{0x018, 0x01f, einstein_fdc_w},
	{0x024, 0x024, einstein_rom_w},
	{0x028, 0x02f, einstein_ctc_w},
	{0x030, 0x037, einstein_pio_w},
PORT_END



#define EINSTEIN_DUMP_RAM

#ifdef EINSTEIN_DUMP_RAM
void einstein_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "einstein.bin", OSD_FILETYPE_NVRAM,OSD_FOPEN_WRITE);
 
	if (file)
	{
		osd_fwrite(file, einstein_ram, 0x10000);

		/* close file */
		osd_fclose(file);
	}
}
#endif

void einstein_init_machine(void)
{
	einstein_ram = malloc(65536);
	memset(einstein_ram, 0x0aa, 65536);
	cpu_setbank(2,einstein_ram+0x02000);
	cpu_setbank(3,einstein_ram);

	z80ctc_init(&einstein_ctc_intf);
	z80pio_init(&einstein_pio_intf);

	z80ctc_reset(0);
	z80pio_reset(0);

	TMS9928A_reset ();

	einstein_rom_enabled = 1;
	einstein_page_rom();

	wd179x_init(NULL);
}


void einstein_shutdown_machine(void)
{
	einstein_dump_ram();

	if (einstein_ram)
	{
		free(einstein_ram);
		einstein_ram = NULL;
	}

	wd179x_exit();
}

INPUT_PORTS_START(einstein)
	/* line 0 */
	PORT_START
INPUT_PORTS_END



static struct AY8910interface einstein_ay_interface =
{
	1,								   /* 1 chips */
	2000000,						   /* 2.0 MHz  */
	{25, 25},
	{0},
	{0},
	{0},
	{0}
};


static struct MachineDriver machine_driver_einstein =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			EINSTEIN_SYSTEM_CLOCK,
			readmem_einstein,		   /* MemoryReadAddress */
			writemem_einstein,		   /* MemoryWriteAddress */
			readport_einstein,		   /* IOReadPort */
			writeport_einstein,		   /* IOWritePort */
            0, 0,
			0, 0,	
			einstein_daisy_chain
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	einstein_init_machine,			   /* init machine */
	einstein_shutdown_machine,
	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	einstein_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&einstein_ay_interface
		},
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(einstein)
	ROM_REGION(0x010000+0x02000, REGION_CPU1,0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, 0x01)
ROM_END

static const struct IODevice io_einstein[] =
{
	{
		IO_FLOPPY,				/* type */
		4,						/* count */
		"dsk\0",                /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL, /*basicdsk_floppy_id,*/ 	/* id */
		einstein_floppy_init, /* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 19??, einstein,      0,            einstein,          einstein,      0,       "Tatung", "Tatung Einstein TC-01")
