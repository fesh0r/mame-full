/*
	APEXC driver

	see cpu/apexc.c for background and tech info
*/

#include "driver.h"

static MEMORY_READ32_START(readmem)
	{ 0x0000, 0xfff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START(writemem)
	{ 0x0000, 0xfff, MWA32_RAM },
MEMORY_END

static READ32_HANDLER(tape_read);
static WRITE32_HANDLER(tape_write);

static PORT_READ32_START(readport)

	{0, 0, tape_read},

PORT_END

static PORT_WRITE32_START(writeport)

	{0, 0, tape_write},

PORT_END

INPUT_PORTS_START(apexc)

INPUT_PORTS_END

/*
  apexc video emulation.

  No video or terminal, but there was a control panel.
*/

static void apexc_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *dummy)
{
/*	memcpy(palette, & apexc_palette, sizeof(apexc_palette));
	memcpy(colortable, & apexc_colortable, sizeof(apexc_colortable));*/
}

static int apexc_vh_start(void)
{
	return 0; /*generic_vh_start();*/
}

/*#define apexc_vh_stop generic_vh_stop*/

static void apexc_vh_stop(void)
{
}

static void apexc_vh_refresh(struct osd_bitmap *bitmap, int full_refresh)
{

}

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }	/* end of array */
};

static void apexc_init_machine(void)
{
}

static void apexc_stop_machine(void)
{
}

static struct MachineDriver machine_driver_apexc =
{
	/* basic machine hardware */
	{
		{
			CPU_APEXC,
			2000,	/* 2.0 kHz (memory word clock frequency) */
			readmem, writemem, readport, writeport,
			0, 0,	/* no interupt handler right now, but it may be useful to handle the control panel */
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration : meaningless */
	1,
	apexc_init_machine,
	apexc_stop_machine,

	/* video hardware */
	256,						/* screen width */
	192,						/* screen height */
	{ 0, 256-1, 0, 192-1},		/* visible_area */
	gfxdecodeinfo,				/* graphics decode info (???)*/
	0/*APEXC_PALETTE_SIZE*/,		/* palette is 3*total_colors bytes long */
	0/*APEXC_COLORTABLE_SIZE*/,	/* length in shorts of the color lookup table */
	apexc_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	apexc_vh_start,
	apexc_vh_stop,
	apexc_vh_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
	},

	/* NVRAM handler */
	NULL
};

typedef struct tape
{
	void *fd;
} tape;

tape apexc_tapes[2];

static int apexc_tape_init(int id)
{
	tape *t = &apexc_tapes[id];

	/* open file */
	/* unit 0 is read-only, unit 1 is write-only */
	t->fd = image_fopen(IO_PUNCHTAPE, id, OSD_FILETYPE_IMAGE_RW,
							(id==0) ? OSD_FOPEN_READ : OSD_FOPEN_WRITE);

	return INIT_OK;
}

static void apexc_tape_exit(int id)
{
	tape *t = &apexc_tapes[id];

	if (t->fd)
		osd_fclose(t->fd);
}

static READ32_HANDLER(tape_read)
{
	UINT8 reply;

	if (apexc_tapes[0].fd && (osd_fread(apexc_tapes[0].fd, & reply, 1) == 1))
		return reply & 0x1f;
	else
		return 0;	/* unit not ready - I don't know what we should do */
}

static WRITE32_HANDLER(tape_write)
{
	UINT8 data5 = (data & 0x1f);

	osd_fwrite(apexc_tapes[1].fd, & data5, 1);
}

static const struct IODevice io_apexc[] =
{
	{
		IO_PUNCHTAPE,			/* type */
		2,						/* count */
		"tap\0",				/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		apexc_tape_init,		/* init */
		apexc_tape_exit,		/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

ROM_START(apexc)
	/*CPU memory space*/
	ROM_REGION32_BE(0x10000, REGION_CPU1,0)
		/* Note this computer has no ROM... */
ROM_END

/*		YEAR	NAME		PARENT	MACHINE		INPUT	INIT	COMPANY		FULLNAME */
COMP( 1951(?),	apexc,		0,		apexc,		apexc,	0,		"Booth",	"APEXC" )
