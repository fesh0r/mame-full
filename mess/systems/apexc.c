/*
	systems/apexc.c : APEXC driver

	By Raphael Nabet

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

/*
	APEXC control panel

	I know really little about the details, although the general purpose is obvious.

	* "As well as starting and stopping the machine, [it] enables information to be inserted
	manually and provides for the inspection of the contents of the memory via various
	storage registers." (Booth, p. 2)
	* "Data can be inserted manually from the control panel [into the control register]".
	(Booth, p. 3)
	* There is no trace mode (Booth, p. 213)
*/



/*
	APEXC tape support

	APEXC does all I/O on paper tape.  There are 5 punch rows on tape.

	Both a reader (read-only), and a puncher (write-only) are provided.

	Tape output can be fed into a teletyper, in order to have text output :

	code					Symbol
	(binary)		Letters			Figures
	00000							0
	00001			T				1
	00010			B				2
	00011			O				3
	00100			E				4
	00101			H				5
	00110			N				6
	00111			M				7
	01000			A				8
	01001			L				9
	01010			R				+
	01011			G				-
	01100			I				z
	01101			P				.
	01110			C				d
	01111			V				=
	10000					Space
	10001			Z				y
	10010			D				theta (greek letter)
	10011					Line Space (i.e. LF)
	10100			S				,
	10101			Y				Sigma (greek letter)
	10110			F				x
	10111			X				/
	11000					Carriage Return
	11001			W				phi (greek letter)
	11010			J				- (dash ?)
	11011					Figures
	11100			U				pi (greek letter)
	11101			Q				)
	11110			K				(
	11111					Letters
*/

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

typedef struct cylinder
{
	void *fd;
	int writable;
} cylinder;

cylinder apexc_cylinder;

enum
{
	IO_CYLINDER = IO_QUICKLOAD	/* this is not a MESS standard ;-) */
};

static int apexc_cylinder_init(int id)
{
	/* open file */
	/* first try read/write mode */
	apexc_cylinder.fd = image_fopen(IO_CYLINDER, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if (apexc_cylinder.fd)
		apexc_cylinder.writable = 1;
	else
	{	/* else try read-only mode */
		apexc_cylinder.writable = 0;
		apexc_cylinder.fd = image_fopen(IO_CYLINDER, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	}

	if (apexc_cylinder.fd)
	{	/* load RAM contents */
		/* in an actual APEXC, the RAM is not loaded with the cylinder contents,
		the cylinder IS the RAM */
		osd_fread(apexc_cylinder.fd, memory_region(REGION_CPU1), 0x8000);
#ifdef LSB_FIRST
		{	/* fix endianness */
			UINT32 *RAM;
			int i;

			RAM = (UINT32 *) memory_region(REGION_CPU1);

			for (i=0; i < 0x2000; i++)
				RAM[i] = BIG_ENDIANIZE_INT32(RAM[i]);
		}
#endif
	}

	return INIT_OK;
}

static void apexc_cylinder_exit(int id)
{
	if (apexc_cylinder.fd && apexc_cylinder.writable)
	{	/* save RAM contents */

	}
	if (apexc_cylinder.fd)
	{
		osd_fclose(apexc_cylinder.fd);
	}
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
	{
		IO_CYLINDER,			/* type */
		1,						/* count */
		"apc\0",				/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		apexc_cylinder_init,	/* init */
		apexc_cylinder_exit,	/* exit */
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
