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

SPACEWAR! is public domain, so I include it with the driver.
So far only emulated is stuff needed to run SPACEWAR!

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
#include "vidhrdw/generic.h"
#include "vidhrdw/pdp1.h"

#include "includes/pdp1.h"

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
static MEMORY_READ_START18(pdp1_readmem)
	{ 0x0000, 0xffff, pdp1_read_mem },
MEMORY_END

static MEMORY_WRITE_START18(pdp1_writemem)
	{ 0x0000, 0xffff, pdp1_write_mem },
MEMORY_END

INPUT_PORTS_START( pdp1 )

    PORT_START      /* IN0 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT, "Spin Left Player 1", KEYCODE_A, JOYCODE_1_LEFT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT, "Spin Right Player 1", KEYCODE_S, JOYCODE_1_RIGHT )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1, "Thrust Player 1", KEYCODE_D, JOYCODE_1_BUTTON1 )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2, "Fire Player 1", KEYCODE_F, JOYCODE_1_BUTTON2 )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT|IPF_PLAYER2, "Spin Left Player 2", KEYCODE_LEFT, JOYCODE_2_LEFT )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT|IPF_PLAYER2, "Spin Right Player 2", KEYCODE_RIGHT, JOYCODE_2_RIGHT )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1|IPF_PLAYER2, "Thrust Player 2", KEYCODE_UP, JOYCODE_2_BUTTON1 )
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2|IPF_PLAYER2, "Fire Player 2", KEYCODE_DOWN, JOYCODE_2_BUTTON2 )

    PORT_START /* IN1 */
	PORT_BITX(	  0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 1", KEYCODE_1, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x80, DEF_STR( On )	 )
	PORT_BITX(	  0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 2", KEYCODE_2, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BITX(	  0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 3", KEYCODE_3, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BITX(	  0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 4", KEYCODE_4, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BITX(	  0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 5", KEYCODE_5, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BITX(	  0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 6", KEYCODE_6, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x04, DEF_STR( On ) )
    INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0x55,0x55,0x55, /* DK GREY - for MAME text only */
	0x80,0x80,0x80, /* LT GREY - for MAME text only */
};

static unsigned short colortable[] =
{
	0x00, 0x01,
	0x01, 0x00,
};

/* Initialise the palette */
static void pdp1_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}


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
			2000000,
			pdp1_readmem, pdp1_writemem,0,0,
			0, 0 /* no vblank interrupt */
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

	VIDEO_TYPE_VECTOR,
	0,
	pdp1_vh_start,
	pdp1_vh_stop,
	pdp1_vh_update,

	/* sound hardware */
	0,0,0,0
};

static const struct IODevice io_pdp1[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",			/* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
        pdp1_id_rom,        /* id */
		pdp1_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL                /* output_chunk */
	},
	{ IO_END }
};

#define rom_pdp1    NULL

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMP( 1962, pdp1,	  0, 		pdp1,	  pdp1, 	0,		  "Digital Equipment Company",  "PDP-1 (Spacewar!)" )
