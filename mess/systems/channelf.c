#include "driver.h"
#include "vidhrdw/generic.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

void channelf_init_machine(void)
{
}

void init_channelf(void)
{
}

int channelf_vh_start(void)
{
	if (generic_vh_start())
        return 1;
	return 0;
}

void channelf_vh_stop(void)
{
	generic_vh_stop();
}

void channelf_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < videoram_size; offs++ )
	{
		if( dirtybuffer[offs] )
		{
            int sx, sy, code;
			sx = (offs % 80) * 8;
			sy = (offs / 80) * 8;
			code = videoram[offs];
			drawgfx(bitmap,Machine->gfx[0],code,0,0,0,sx,sy,
                &Machine->visible_area,TRANSPARENCY_NONE,0);
        }
	}
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_ROM },
	{ 0x0400, 0x07ff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_ROM },
	{ 0x0400, 0x07ff, MWA_ROM },
    {-1}
};

INPUT_PORTS_START( channelf )
	PORT_START /* DIP switches */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)
INPUT_PORTS_END

static struct MachineDriver machine_driver_channelf =
{
	/* basic machine hardware */
	{
		{
			CPU_F8,
			2000000,	/* 2 MHz */
			readmem,writemem,0,0,
			ignore_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* single CPU */
	channelf_init_machine,
	NULL,					/* stop machine */

	/* video hardware - include overscan */
	32*8, 16*16, { 0*8, 32*8 - 1, 0*16, 16*16 - 1},
	NULL,
	2, 2,
	NULL,					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	channelf_vh_start,
	channelf_vh_stop,
	channelf_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START(channelf)
	ROM_REGION(0x10000,REGION_CPU1)
		ROM_LOAD("fcbios.rom",  0x0000, 0x0800, 0x2882c02d)
		ROM_LOAD("1.rom",       0x0800, 0x0800, 0xff4768b0)
		ROM_LOAD("2.rom",       0x1000, 0x0800, 0x1570934b)
		ROM_LOAD("4.rom",       0x1800, 0x0800, 0x5357c5f6)
		ROM_LOAD("6.rom",       0x2000, 0x0800, 0xbb4c24a2)
ROM_END

static const struct IODevice io_channelf[] = {
    { IO_END }
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	   FULLNAME */
COMP( 1988, channelf, 0,		channelf, channelf, channelf, "Fairchild", "Channel F" )



