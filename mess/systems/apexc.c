/*
	systems/apexc.c : APEXC driver

	By Raphael Nabet

	see cpu/apexc.c for background and tech info
*/

#include "driver.h"
#include "ui_text.h"
#include "cpu/apexc/apexc.h"

static void apexc_init_machine(void)
{
}

static void apexc_stop_machine(void)
{
}


/*
	APEXC RAM loading/writing from cylinder image

	Note that, in an actual APEXC, the RAM is not loaded with the cylinder contents :
	the cylinder IS the RAM.

	This feature is important : of course, the tape reader allows to enter programs, but you
	still need an object code loader in memory.  (Of course, the control panel should enable
	the user to enter it manually, but it would be a real pain...)
*/

typedef struct cylinder
{
	void *fd;
	int writable;
} cylinder;

cylinder apexc_cylinder;

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
		/* rewind file */
		osd_fseek(apexc_cylinder.fd, 0, SEEK_SET);
		/* write */
		osd_fwrite(apexc_cylinder.fd, memory_region(REGION_CPU1), 0x8000);
	}
	if (apexc_cylinder.fd)
	{
		osd_fclose(apexc_cylinder.fd);
	}
}


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

/*
	APEXC control panel

	I know really little about the details, although the big picture is obvious.

	Things I know :
	* "As well as starting and stopping the machine, [it] enables information to be inserted
	manually and provides for the inspection of the contents of the memory via various
	storage registers." (Booth, p. 2)
	* "Data can be inserted manually from the control panel [into the control register]".
	(Booth, p. 3)
	* The contents of the R register can be edited, too.  A button allows to clear
	a complete X (or Y ???) field.  (forgot the reference, but must be somewhere in Booth)
	* There is no trace mode (Booth, p. 213)

	Since the control panel is necessary for the operation os the APEXC, I tried to
	implement an commonplace control panel.  I cannot tell how close the feature set and
	operation of this control panel is to the original APEXC control panel, but it
	cannot be too different in the basic principles.
*/


/* defines for input ports */
enum
{
	panel_control = 0,
	panel_edit1,
	panel_edit2
};

enum
{
	panel_run_bit = 0,
	panel_CR_bit,
	panel_A_bit,
	panel_R_bit,
	panel_HB_bit,
	panel_ML_bit,
	panel_mem_bit,
	panel_write_bit,

	panel_run = (1 << panel_run_bit),
	panel_CR  = (1 << panel_CR_bit),
	panel_A   = (1 << panel_A_bit),
	panel_R   = (1 << panel_R_bit),
	panel_HB  = (1 << panel_HB_bit),
	panel_ML  = (1 << panel_ML_bit),
	panel_mem = (1 << panel_mem_bit),
	panel_write = (1 << panel_write_bit)
};

/* fake input ports with keyboard keys */
INPUT_PORTS_START(apexc)

	PORT_START	/* 0 : panel control */
	PORT_BITX(panel_run, IP_ACTIVE_HIGH, IPT_KEYBOARD, "run/stop", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(panel_CR,  IP_ACTIVE_HIGH, IPT_KEYBOARD, "read CR",  KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(panel_A,   IP_ACTIVE_HIGH, IPT_KEYBOARD, "read A",   KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(panel_R,   IP_ACTIVE_HIGH, IPT_KEYBOARD, "read R",   KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(panel_HB,  IP_ACTIVE_HIGH, IPT_KEYBOARD, "read HB",  KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(panel_ML,  IP_ACTIVE_HIGH, IPT_KEYBOARD, "read ML",  KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(panel_mem, IP_ACTIVE_HIGH, IPT_KEYBOARD, "read mem", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(panel_write, IP_ACTIVE_HIGH, IPT_KEYBOARD, "write instead of reading", KEYCODE_LSHIFT, IP_JOY_NONE)

	PORT_START	/* 1 : data edit #1 */
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #10", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #11", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #12", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #13", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #14", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #15", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #16", KEYCODE_Y, IP_JOY_NONE)

	PORT_START	/* 2 : data edit #2 */
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #17", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #18", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #19", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #20", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #21", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #22", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #23", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #24", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #25", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #26", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #27", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #28", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #29", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #30", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #31", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "toggle bit #32", KEYCODE_X, IP_JOY_NONE)

INPUT_PORTS_END


static UINT32 panel_data_reg;	/* value of a data register on the control panel which can
								be edited - the existence of this register is a personnal
								guess */


static int apexc_interrupt(void)
{
	UINT32 edit_keys;
	int control_keys;

	static UINT32 old_edit_keys;
	static int old_control_keys;

	int control_transitions;


	/* read new state of edit keys */
	edit_keys = (readinputport(panel_edit1) << 16) | readinputport(panel_edit2);

	/* toggle data reg according to transitions */
	panel_data_reg ^= edit_keys & (~ old_edit_keys);

	/* recall new state of edit keys */
	old_edit_keys = edit_keys;


	/* read new state of edit keys */
	control_keys = readinputport(panel_control);

	/* compute transitions */
	control_transitions = control_keys & (~ old_control_keys);

	/* process commands */
	if (control_transitions & panel_run)
	{	/* toggle run/stop state */
		cpunum_set_reg(0, APEXC_STATE, ! cpunum_get_reg(0, APEXC_STATE));
	}

	while (control_transitions & (panel_CR | panel_A | panel_R | panel_ML | panel_HB))
	{	/* read/write a register */
		/* note that we must take into account the possibility of simulteanous keypresses
		(which would be a goofy thing to do when reading, but a normal one when writing,
		if the user wants to clear several registers at once) */
		int reg_id;

		/* determinate value of reg_id */
		if (control_transitions & panel_CR)
		{	/* CR register selected ? */
			control_transitions &= ~panel_CR;	/* clear so that it is ignored on next iteration */
			reg_id = APEXC_CR;			/* matching register ID */
		}
		else if (control_transitions & panel_A)
		{
			control_transitions &= ~panel_A;
			reg_id = APEXC_A;
		}
		else if (control_transitions & panel_R)
		{
			control_transitions &= ~panel_R;
			reg_id = APEXC_R;
		}
		else if (control_transitions & panel_HB)
		{
			control_transitions &= ~panel_HB;
			reg_id = APEXC_WS;
		}
		else if (control_transitions & panel_ML)
		{
			control_transitions &= ~panel_ML;
			reg_id = APEXC_ML;
		}

		/* read/write register #reg_id */
		if (control_keys & panel_write)
			/* write reg */
			cpunum_set_reg(0, reg_id, panel_data_reg);
		else
			/* read reg */
			panel_data_reg = cpunum_get_reg(0, reg_id);
	}

	if (control_transitions & panel_mem)
	{	/* read/write memory */

		if (control_keys & panel_write)
			/* write memory */
			apexc_writemem(cpunum_get_reg(0, APEXC_ML_FULL), panel_data_reg);
		else
			/* read memory */
			panel_data_reg = apexc_readmem(cpunum_get_reg(0, APEXC_ML_FULL));
	}

	/* recall new state of control keys */
	old_control_keys = control_keys;


	return ignore_interrupt();
}

/*
	apexc video emulation.

	Since the APEXC has no video display, we display the control panel.

	We could display the teletyper output, too.
*/

static unsigned char apexc_palette[] =
{
	0, 0, 0,
	255, 0, 0,
	50, 0, 0
};

static unsigned short apexc_colortable[] =
{
	0, 0,
	1, 1,
	2, 2
};

#define APEXC_PALETTE_SIZE sizeof(apexc_palette)/3
#define APEXC_COLORTABLE_SIZE sizeof(apexc_colortable)/2

static void apexc_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *dummy)
{
	memcpy(palette, & apexc_palette, sizeof(apexc_palette));
	memcpy(colortable, & apexc_colortable, sizeof(apexc_colortable));
}

static int apexc_vh_start(void)
{
	/*return generic_vh_start();*/
	return 0;
}

/*#define apexc_vh_stop generic_vh_stop*/

static void apexc_vh_stop(void)
{
}

static void apexc_draw_led(struct osd_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;

	for (yy=1; yy<7; yy++)
		for (xx=1; xx<7; xx++)
			plot_pixel (bitmap, x+xx, y+yy, Machine->pens[state ? 1 : 2]);
}

static void apexc_vh_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int i;
	char buffer[2] = "x";

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	apexc_draw_led(bitmap, 0, 0, 1);
	ui_text(bitmap, "power", 8, 0);

	apexc_draw_led(bitmap, 0, 8, cpunum_get_reg(0, APEXC_STATE));
	ui_text(bitmap, "running", 8, 8);

	ui_text(bitmap, "data :", 0, 24);
	for (i=0; i<32; i++)
	{
		apexc_draw_led(bitmap, i*8, 32, (panel_data_reg << i) & 0x80000000UL);
		buffer[0] = '0' + ((i + 1) % 10);
		ui_text(bitmap, buffer, i*8, 40);
		if (((i + 1) % 10) == 0)
		{
			buffer[0] = '0' + ((i + 1) / 10);
			ui_text(bitmap, buffer, i*8, 48);
		}
	}
}

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }	/* end of array */
};


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

static struct MachineDriver machine_driver_apexc =
{
	/* basic machine hardware */
	{
		{
			CPU_APEXC,
			2000,	/* 2.0 kHz (memory word clock frequency) */
			readmem, writemem, readport, writeport,
			apexc_interrupt, 1,	/* handles the control panel */
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
	APEXC_PALETTE_SIZE,			/* palette is 3*total_colors bytes long */
	APEXC_COLORTABLE_SIZE,		/* length in shorts of the color lookup table */
	apexc_init_palette,			/* palette init */

	VIDEO_TYPE_RASTER,
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

static const struct IODevice io_apexc[] =
{
	{
		IO_CYLINDER,			/* type */
		1,						/* count */
		"apc\0",				/* file extensions */
		IO_RESET_ALL,			/* reset if file changed */
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
