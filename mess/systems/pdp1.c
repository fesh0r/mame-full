/*

Driver for a PDP1 emulator.

	Digital Equipment Corporation
	Spacewar! was conceived in 1961 by Martin Graetz,
	Stephen Russell, and Wayne Wiitanen. It was first
	realized on the PDP-1 in 1962 by Stephen Russell,
	Peter Samson, Dan Edwards, and Martin Graetz, together
	with Alan Kotok, Steve Piner, and Robert A Saunders.
	Spacewar! is in the public domain, but this credit
	paragraph must accompany all distributed versions
	of the program.
	Brian Silverman (original Java Source)
	Vadim Gerasimov (original Java Source)
	Chris Salomon (MESS driver)
	Raphael Nabet (MESS driver)

Preliminary, this is a conversion of a JAVA emulator.
I have tried contacting the author, but heard as yet nothing of him,
so I don't know if it all right with him, but after all -> he did
release the source, so hopefully everything will be fine (no his
name is not Marat).

Note: naturally I have no PDP1, I have never seen one, nor have I any
programs for it.

The only program I found (in binary form) is

SPACEWAR!

The first Videogame EVER!

When I saw the java emulator, running that game I was quite intrigued to
include a driver for MESS.
I think the historical value of SPACEWAR! is enormous.


Not even the keyboard is fully emulated.
For more documentation look at the source for the driver,
and the pdp1/pdp1.c file (information about the whereabouts of information
and the java source).

In SPACEWAR!, meaning of the sense switches

Sense switch 1 On = low momentum            Off = high momentum (guess)
Sense switch 2 On = low gravity             Off = high gravity
Sense switch 3            something with torpedos?
Sense switch 4 On = background stars off    Off = background stars on
Sense switch 5 On = star kills              Off = star teleports
Sense switch 6 On = big star                Off = no big star

LISP interpreter is coming...
Sometime I'll implement typewriter and/or puncher...
Keys are allready in the machine...

Bug fixes...

Some fixes, but I can't get to grips with the LISP... something is
very wrong somewhere...

(to load LISP for now rename it to SPACEWAR.BIN,
though output is not done, and input via keyboard produces an
'error' of some kind (massive indirection))

Added Debugging and Disassembler...

Another PDP1 emulator (or simulator)
is at:
ftp://minnie.cs.adfa.oz.au/pub/PDP-11/Sims/Supnik_2.3

including source code.
Sometime I'll rip some devices of that one and include them in this emulation.

Also
ftp://minnie.cs.adfa.oz.au/pub/PDP-11/Sims/Supnik_2.3/software/lispswre.tar.gz
Is a packet which includes the original LISP as source and
binary form plus a makro assembler for PDP1 programs.

*/


#include "driver.h"

#include "includes/pdp1.h"
#include "cpu/pdp1/pdp1.h"

/*
 * PRECISION CRT DISPLAY (TYPE 30)
 * is the only display - hardware emulated, this is needed for SPACEWAR!
 *
 * Only keys required in SPACEWAR! are emulated.
 *
 * The loading storing OS... is not emulated (I haven't a clue where to
 * get programs for the machine (ROMS!!!))
 *
 * The only supported program is SPACEWAR!
 * For Web addresses regarding PDP1 and SPACEWAR look at the
 * pdp1/pdp1.c file
 *
 */





/* every memory handler is the same for now */

/* note: MEMORY HANDLERS used everywhere, since we don't need bytes, we
 * need 18 bit words, the handler functions return integers, so it should
 * be all right to use them.
 * This gives sometimes IO warnings!
 */
#ifdef SUPPORT_ODD_WORD_SIZES
#define pdp1_read_mem MRA32_RAM
#define pdp1_write_mem MWA32_RAM
#endif
static MEMORY_READ_START18(pdp1_readmem)
	{ 0x0000, 0xffff, pdp1_read_mem },
MEMORY_END

static MEMORY_WRITE_START18(pdp1_writemem)
	{ 0x0000, 0xffff, pdp1_write_mem },
MEMORY_END

/* defines for input port numbers */
enum
{
	pdp1_spacewar_controllers = 0,
	pdp1_sense_switches = 1,
	pdp1_control = 2,
	pdp1_test_switches_MSB = 3,
	pdp1_test_switches_LSB = 4
};

/* defines for each bit and mask in input port pdp1_control */
enum
{
	/* bit numbers */
	pdp1_read_in_bit = 0,

	/* masks */
	pdp1_read_in = (1 << pdp1_read_in_bit)
};

static int test_switches;

/*
	Not a real interrupt - just handle keyboard input
*/
static int pdp1_interrupt(void)
{
	int control_keys;

	static int old_control_keys;

	int control_transitions;


	/* update display */
	pdp1_screen_update();


	cpu_set_reg(PDP1_S, readinputport(pdp1_sense_switches));
	cpu_set_reg(PDP1_F1, pdp1_keyboard());


	test_switches = (readinputport(pdp1_test_switches_MSB) << 16) | readinputport(pdp1_test_switches_LSB);

	/* read new state of control keys */
	control_keys = readinputport(pdp1_control);

	/* compute transitions */
	control_transitions = control_keys & (~ old_control_keys);

	if (control_transitions & pdp1_read_in)
	{	/* set cpu to run and read instruction from perforated tape */
		cpunum_set_reg(0, PDP1_RUN, 1);
		cpunum_set_reg(0, PDP1_RIM, 1);
	}

	/* remember new state of control keys */
	old_control_keys = control_keys;

	return ignore_interrupt();
}

static int get_test_switches(void)
{
	return test_switches;
}

INPUT_PORTS_START( pdp1 )

    PORT_START		/* 0: spacewar controllers */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT, "Spin Left Player 1", KEYCODE_A, JOYCODE_1_LEFT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT, "Spin Right Player 1", KEYCODE_S, JOYCODE_1_RIGHT )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1, "Thrust Player 1", KEYCODE_D, JOYCODE_1_BUTTON1 )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2, "Fire Player 1", KEYCODE_F, JOYCODE_1_BUTTON2 )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT|IPF_PLAYER2, "Spin Left Player 2", KEYCODE_LEFT, JOYCODE_2_LEFT )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT|IPF_PLAYER2, "Spin Right Player 2", KEYCODE_RIGHT, JOYCODE_2_RIGHT )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1|IPF_PLAYER2, "Thrust Player 2", KEYCODE_UP, JOYCODE_2_BUTTON1 )
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2|IPF_PLAYER2, "Fire Player 2", KEYCODE_DOWN, JOYCODE_2_BUTTON2 )

    PORT_START		/* 1: controller panel sense switches */
	PORT_BITX(	  040, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 1", KEYCODE_1, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    040, DEF_STR( On )	 )
	PORT_BITX(	  020, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 2", KEYCODE_2, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    020, DEF_STR( On ) )
	PORT_BITX(	  010, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 3", KEYCODE_3, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    010, DEF_STR( On ) )
	PORT_BITX(	  004, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 4", KEYCODE_4, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    004, DEF_STR( On ) )
	PORT_BITX(	  002, 002, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 5", KEYCODE_5, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    002, DEF_STR( On ) )
	PORT_BITX(	  001, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 6", KEYCODE_6, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    001, DEF_STR( On ) )

	PORT_START		/* 2: various pdp1 controls */
	PORT_BITX(pdp1_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD, "read in mode", KEYCODE_ENTER, IP_JOY_NONE)

    PORT_START		/* 3: controller panel test word switches MSB */
	PORT_BITX(	  0002, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 1", KEYCODE_Q, IP_JOY_NONE )
    PORT_DIPSETTING(    0000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0002, DEF_STR( On ) )
	PORT_BITX(	  0001, 0000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 2", KEYCODE_W, IP_JOY_NONE )
    PORT_DIPSETTING(    0000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0001, DEF_STR( On ) )

    PORT_START		/* 4: controller panel test word switches LSB */
	PORT_BITX(	  0100000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 3", KEYCODE_E, IP_JOY_NONE )
    PORT_DIPSETTING(    0000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0100000, DEF_STR( On )	 )
	PORT_BITX(	  0040000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 4", KEYCODE_R, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0040000, DEF_STR( On ) )
	PORT_BITX(	  0020000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 5", KEYCODE_T, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0020000, DEF_STR( On ) )
	PORT_BITX(	  0010000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 6", KEYCODE_Y, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0010000, DEF_STR( On )	 )
	PORT_BITX(	  0004000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 7", KEYCODE_U, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0004000, DEF_STR( On ) )
	PORT_BITX(	  0002000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 8", KEYCODE_I, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0002000, DEF_STR( On ) )
	PORT_BITX(	  0001000, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 9", KEYCODE_O, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0001000, DEF_STR( On ) )
	PORT_BITX(	  0000400, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 10", KEYCODE_P, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000400, DEF_STR( On ) )
	PORT_BITX(	  0000200, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 11", KEYCODE_A, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000200, DEF_STR( On ) )
	PORT_BITX(	  0000100, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 12", KEYCODE_S, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000100, DEF_STR( On ) )
	PORT_BITX(	  0000040, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 13", KEYCODE_D, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000040, DEF_STR( On ) )
	PORT_BITX(	  0000020, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 14", KEYCODE_F, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000020, DEF_STR( On ) )
	PORT_BITX(	  0000010, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 15", KEYCODE_G, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000010, DEF_STR( On ) )
	PORT_BITX(	  0000004, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 16", KEYCODE_H, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000004, DEF_STR( On ) )
   	PORT_BITX(	  0000002, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 17", KEYCODE_J, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000002, DEF_STR( On )	 )
   	PORT_BITX(	  0000001, 0000000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Switch 18", KEYCODE_K, IP_JOY_NONE )
    PORT_DIPSETTING(    0000000, DEF_STR( Off ) )
    PORT_DIPSETTING(    0000001, DEF_STR( On )	 )


INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

/* palette: grey levels follow an exponential law, so that decreasing the color index periodically
will simulate the remanence of a cathode ray tube */
static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	11,11,11,
	14,14,14,
	18,18,18,
	22,22,22,
	27,27,27,
	34,34,34,
	43,43,43,
	53,53,53,
	67,67,67,
	84,84,84,
	104,104,104,
	131,131,131,
	163,163,163,
	204,204,204,
	0xFF,0xFF,0xFF  /* WHITE */
};

static unsigned short colortable[] =
{
	0x00, 0x01,
	//0x01, 0x00,
};

/* Initialise the palette */
static void pdp1_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy(sys_palette, palette, sizeof(palette));
	memcpy(sys_colortable, colortable, sizeof(colortable));
}


static pdp1_reset_param reset_param =
{
	pdp1_iot,
	pdp1_tape_read_binary,
	get_test_switches
};


/* note I don't know about the speed of the machine, I only know
 * how long each instruction takes in micro seconds
 * below speed should therefore also be read in something like
 * microseconds of instructions
 */
static struct MachineDriver machine_driver_pdp1 =
{
	/* basic machine hardware */
	{
		{
			CPU_PDP1,
			1000000,	/* 5000000 actually, but timings are wrong */
			pdp1_readmem, pdp1_writemem,0,0,
			pdp1_interrupt, 1, /* fake interrupt */
			0, 0,
			& reset_param
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,
	pdp1_init_machine,
	0,

	/* video hardware */
	VIDEO_BITMAP_WIDTH, VIDEO_BITMAP_HEIGHT, { 0, VIDEO_BITMAP_WIDTH-1, 0, VIDEO_BITMAP_HEIGHT-1 },

	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),

	pdp1_init_palette,

	/*VIDEO_TYPE_VECTOR*/VIDEO_TYPE_RASTER,
	0,
	pdp1_vh_start,
	pdp1_vh_stop,
	pdp1_vh_update,

	/* sound hardware */
	0,0,0,0
};

static const struct IODevice io_pdp1[] =
{
	{	/* one perforated tape reader, and one puncher */
		IO_PUNCHTAPE,			/* type */
		2,						/* count */
		"tap\0rim\0",			/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		pdp1_tape_init,			/* init */
		pdp1_tape_exit,			/* exit */
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
	{	/* teletyper out */
		IO_PRINTER,					/* type */
		1,							/* count */
		"typ\0",					/* file extensions */
		IO_RESET_NONE,				/* reset depth */
		NULL,						/* id */
		pdp1_teletyper_init,		/* init */
		pdp1_teletyper_exit,		/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
		NULL,						/* status */
		NULL,						/* seek */
		NULL,						/* tell */
		NULL,						/* input */
		NULL,						/* output */
		NULL,						/* input chunk */
		NULL						/* output chunk */
	},
	{ IO_END }
};

/*
	only 4096 are used for now, but pdp1 can address 65336 18 bit words when extended.
*/
#ifdef SUPPORT_ODD_WORD_SIZES
ROM_START(pdp1)
	ROM_REGION(0x10000 * sizeof(data32_t),REGION_CPU1,0)
ROM_END
#else
ROM_START(pdp1)
	ROM_REGION(0x10000 * sizeof(int),REGION_CPU1,0)
ROM_END
#endif

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMP( 1961, pdp1,	  0, 		pdp1,	  pdp1, 	pdp1,	  "Digital Equipment Corporation",  "PDP-1" )
