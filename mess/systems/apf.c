/******************************************************************************

 ******************************************************************************/
#include "driver.h"
#include "vidhrdw/m6847.h"
#include "includes/apf.h"

unsigned char *apf_video_ram;

READ_HANDLER(apf_video_r)
{
	return apf_video_ram[offset];
}

WRITE_HANDLER(apf_video_w)
{
	apf_video_ram[offset] = data;
}

void apf_common_init(void)
{
	apf_video_ram = (unsigned char *)malloc(0x0400);
}

void apf_common_exit(void)
{
	if (apf_video_ram)
	{
		free(apf_video_ram);
		apf_video_ram = NULL;
	}
}

void apf_imagination_init_machine(void)
{
	apf_common_init();
}

void apf_imagination_stop_machine(void)
{
	apf_common_exit();
}

void apf_m1000_init_machine(void)
{
	apf_common_init();
}

void apf_m1000_stop_machine(void)
{
	apf_common_exit();
}

static MEMORY_READ_START( apf_imagination_readmem )
	{ 0x00000, 0x003ff, apf_video_r},
	{ 0x00400, 0x007ff, apf_video_r},
	{ 0x00800, 0x00bff, apf_video_r},
	{ 0x00c00, 0x00fff, apf_video_r},
	{ 0x01000, 0x01fff, apf_video_r},
	{ 0x01000, 0x013ff, apf_video_r},
	{ 0x01400, 0x017ff, apf_video_r},
	{ 0x01800, 0x01bff, apf_video_r},
	{ 0x01c00, 0x01fff, apf_video_r},
	{ 0x02000, 0x03fff, MRA_NOP},
	{ 0x04000, 0x047ff, MRA_ROM},
	{ 0x04800, 0x04fff, MRA_BANK1},
	{ 0x05000, 0x057ff, MRA_BANK1},
	{ 0x05800, 0x05fff, MRA_BANK1},
	{ 0x06800, 0x077ff, MRA_ROM},
	{ 0x07800, 0x07fff, MRA_NOP},
	{ 0x08000, 0x09fff, MRA_ROM},
	{ 0x0a000, 0x0dfff, MRA_RAM},
	{ 0x0e000, 0x0e7ff, MRA_BANK1},
	{ 0x0e800, 0x0efff, MRA_BANK1},
	{ 0x0f000, 0x0f7ff, MRA_BANK1},
	{ 0x0f800, 0x0ffff, MRA_BANK1},
MEMORY_END

static MEMORY_WRITE_START( apf_imagination_writemem )
	{ 0x00000, 0x003ff, apf_video_w},
	{ 0x00400, 0x007ff, apf_video_w},
	{ 0x00800, 0x00bff, apf_video_w},
	{ 0x00c00, 0x00fff, apf_video_w},
	{ 0x01000, 0x01fff, apf_video_w},
	{ 0x01000, 0x013ff, apf_video_w},
	{ 0x01400, 0x017ff, apf_video_w},
	{ 0x01800, 0x01bff, apf_video_w},
	{ 0x01c00, 0x01fff, apf_video_w},
	{ 0x02000, 0x03fff, MWA_NOP},
	{ 0x04000, 0x05fff, MWA_ROM},
	{ 0x0a000, 0x0dfff, MWA_RAM},
	{ 0x0e000, 0x0ffff, MWA_ROM},
MEMORY_END

static MEMORY_READ_START(apf_m1000_readmem)
	{ 0x00000, 0x003ff, apf_video_r},
	{ 0x00400, 0x007ff, apf_video_r},
	{ 0x00800, 0x00bff, apf_video_r},
	{ 0x00c00, 0x00fff, apf_video_r},
	{ 0x01000, 0x01fff, apf_video_r},
	{ 0x01000, 0x013ff, apf_video_r},
	{ 0x01400, 0x017ff, apf_video_r},
	{ 0x01800, 0x01bff, apf_video_r},
	{ 0x01c00, 0x01fff, apf_video_r},
	{ 0x02000, 0x03fff, MRA_NOP},
	{ 0x04000, 0x047ff, MRA_ROM},
	{ 0x04800, 0x04fff, MRA_BANK1},
	{ 0x05000, 0x057ff, MRA_BANK1},
	{ 0x05800, 0x05fff, MRA_BANK1},
	{ 0x06800, 0x077ff, MRA_ROM},
	{ 0x08000, 0x09fff, MRA_ROM},
	{ 0x0a000, 0x0dfff, MRA_RAM},
	{ 0x0e000, 0x0e7ff, MRA_BANK1},
	{ 0x0e800, 0x0efff, MRA_BANK1},
	{ 0x0f000, 0x0f7ff, MRA_BANK1},
	{ 0x0f800, 0x0ffff, MRA_BANK1},
MEMORY_END

static MEMORY_WRITE_START( apf_m1000_writemem )
	{ 0x00000, 0x003ff, apf_video_w},
	{ 0x00400, 0x007ff, apf_video_w},
	{ 0x00800, 0x00bff, apf_video_w},
	{ 0x00c00, 0x00fff, apf_video_w},
	{ 0x01000, 0x01fff, apf_video_w},
	{ 0x01000, 0x013ff, apf_video_w},
	{ 0x01400, 0x017ff, apf_video_w},
	{ 0x01800, 0x01bff, apf_video_w},
	{ 0x01c00, 0x01fff, apf_video_w},
	{ 0x04000, 0x05fff, MWA_ROM},
	{ 0x0a000, 0x0dfff, MWA_RAM},
	{ 0x0e000, 0x0ffff, MWA_ROM},
MEMORY_END


INPUT_PORTS_START( apfimag )
INPUT_PORTS_END

INPUT_PORTS_START( apfm1000 )
INPUT_PORTS_END

static struct MachineDriver machine_driver_apfimag =
{
	/* basic machine hardware */
	{
		{
			CPU_M6800,
			3570000,
			apf_imagination_readmem,apf_imagination_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	apf_imagination_init_machine,
	apf_imagination_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	apf_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
};


static struct MachineDriver machine_driver_apfm1000 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6800,
			3570000,
			apf_m1000_readmem,apf_m1000_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	apf_m1000_init_machine,
	apf_m1000_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	apf_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apfimag)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x04000, 0x00800, 0x2a331a33)
	ROM_LOAD("basic_68.rom",0x06800, 0x01000, 0xef049ab8)
	ROM_LOAD("basic_80.rom",0x08000, 0x02000, 0xa4c69fae)
ROM_END

ROM_START(apfm1000)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x06800, 0x0800, 0x2a331a33)
ROM_END

static const struct IODevice io_apf[] =
{
	{ IO_END }
};

#define io_apfimag io_apf
#define io_apfm1000 io_apf

/*     YEAR  NAME       PARENT  MACHINE    INPUT     INIT     COMPANY               FULLNAME */
COMP(  1977, apfimag,      0,		apfimag,      apfimag,     0,		  "APF Electronics Inc.",  "APF Imagination Machine" )
COMP(  1978, apfm1000,     0,		apfm1000,     apfimag,     0,		  "APF Electronics Inc.",  "APF M-1000" )
