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

    PORT_START		/* 1: operator control panel sense switches */
	PORT_BITX(	  040, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 1", KEYCODE_1_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    040, DEF_STR( On )	 )
	PORT_BITX(	  020, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 2", KEYCODE_2_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    020, DEF_STR( On ) )
	PORT_BITX(	  010, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 3", KEYCODE_3_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    010, DEF_STR( On ) )
	PORT_BITX(	  004, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 4", KEYCODE_4_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    004, DEF_STR( On ) )
	PORT_BITX(	  002, 002, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 5", KEYCODE_5_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    002, DEF_STR( On ) )
	PORT_BITX(	  001, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 6", KEYCODE_6_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    001, DEF_STR( On ) )

	PORT_START		/* 2: various pdp1 controls */
	PORT_BITX(pdp1_control, IP_ACTIVE_HIGH, IPT_KEYBOARD, "control panel key", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(pdp1_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD, "read in mode", KEYCODE_ENTER, IP_JOY_NONE)

    PORT_START		/* 3: operator control panel test address switches */
	PORT_BITX( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 3", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 4", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 5", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 6", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 7", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 8", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 9", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 10", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 11", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 12", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 13", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 14", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 15", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 16", KEYCODE_H, IP_JOY_NONE )
   	PORT_BITX( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 17", KEYCODE_J, IP_JOY_NONE )
   	PORT_BITX( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 18", KEYCODE_K, IP_JOY_NONE )

    PORT_START		/* 4: operator control panel test word switches MSB */
	PORT_BITX(    0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 1", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX(    0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 2", KEYCODE_W, IP_JOY_NONE )

    PORT_START		/* 5: operator control panel test word switches LSB */
	PORT_BITX( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 3", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 4", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 5", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 6", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 7", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 8", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 9", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 10", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 11", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 12", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 13", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 14", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 15", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 16", KEYCODE_H, IP_JOY_NONE )
   	PORT_BITX( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 17", KEYCODE_J, IP_JOY_NONE )
   	PORT_BITX( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 18", KEYCODE_K, IP_JOY_NONE )

    PORT_START		/* 6: typewriter codes 00-17 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Space)", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 \"", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 '", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 ~", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 (implies)", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 (or)", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 (and)", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 <", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 >", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (up arrow)", KEYCODE_9, IP_JOY_NONE)

    PORT_START		/* 7: typewriter codes 20-37 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 (right arrow)", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", =", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE)

    PORT_START		/* 7: typewriter codes 40-57 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(non-spacing middle dot) _", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- +", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ") ]", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(non-spacing overstrike) |", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "( [", KEYCODE_MINUS, IP_JOY_NONE)

    PORT_START		/* 8: typewriter codes 60-77 */
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Lower Case", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". (multiply)", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Upper case", KEYCODE_RSHIFT, IP_JOY_NONE)
	/* hack to support my macintosh which does not support Right Shift key */
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Upper case", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE)

/*
	Layout:
	"1 \"",	"2 '",	"3 ~", "4 (implies)", "5 (or)", "6 (and)", "7 <", "8 >", "9 (up arrow)", "0 (right arrow)",	"( [", ") ]"
*/

/*{  001,     0001, KEYCODE_1    , KEYCODE_SLASH    ,"1"                         ,"\""           },
{  002,     0002, KEYCODE_2    , KEYCODE_QUOTE    ,"2"                         ,"'"            },
{  003,     0203, KEYCODE_3    , KEYCODE_TILDE    ,"3"                         ,"~"            },
{  004,     0004, KEYCODE_4    , 0                ,"4"                         ,"(implies)"    },
{  005,     0205, KEYCODE_5    , 0                ,"5"                         ,"(or)"         },
{  006,     0206, KEYCODE_6    , 0                ,"6"                         ,"(and)"        },
{  007,     0007, KEYCODE_7    , 0                ,"7"                         ,"<"            },
{  010,     0010, KEYCODE_8    , 0                ,"8"                         ,">"            },
{  011,     0211, KEYCODE_9    , KEYCODE_UP       ,"9"                         ,"(up arrow)"   },
{  020,     0020, KEYCODE_0    , KEYCODE_RIGHT    ,"0"                         ,"(right arrow)"},
{  057,     0057, KEYCODE_OPENBRACE, 0            ,"("                         ,"["            },
{  055,     0255, KEYCODE_CLOSEBRACE, 0           ,")"                         ,"]"            },
{  075,     0075, KEYCODE_BACKSPACE,0             ,"Backspace"                 ,""             },
{  036,     0236, KEYCODE_TAB  , 0                ,"tab"                       ,""             },
{  050,     0250, KEYCODE_Q    , KEYCODE_Q        ,"q"                         ,"Q"            },
{  026,     0026, KEYCODE_W    , KEYCODE_W        ,"w"                         ,"W"            },
{  065,     0265, KEYCODE_E    , KEYCODE_E        ,"e"                         ,"E"            },
{  051,     0051, KEYCODE_R    , KEYCODE_R        ,"r"                         ,"R"            },
{  023,     0023, KEYCODE_T    , KEYCODE_T        ,"t"                         ,"T"            },
{  030,     0230, KEYCODE_Y    , KEYCODE_Y        ,"y"                         ,"Y"            },
{  024,     0224, KEYCODE_U    , KEYCODE_U        ,"u"                         ,"U"            },
{  071,     0271, KEYCODE_I    , KEYCODE_I        ,"i"                         ,"I"            },
{  046,     0046, KEYCODE_O    , KEYCODE_O        ,"o"                         ,"O"            },
{  047,     0247, KEYCODE_P    , KEYCODE_P        ,"p"                         ,"P"            },
{  056,     0256, 0            , 0                ," (non-spacing overstrike)" ,"|"            },
{  077,     0277, KEYCODE_ENTER, 0                ,"Carriage Return"           ,""             },
{  061,     0061, KEYCODE_A    , KEYCODE_A        ,"a"                         ,"A"            },
{  022,     0222, KEYCODE_S    , KEYCODE_S        ,"s"                         ,"S"            },
{  064,     0064, KEYCODE_D    , KEYCODE_D        ,"d"                         ,"D"            },
{  066,     0266, KEYCODE_F    , KEYCODE_F        ,"f"                         ,"F"            },
{  067,     0067, KEYCODE_G    , KEYCODE_G        ,"g"                         ,"G"            },
{  070,     0070, KEYCODE_H    , KEYCODE_H        ,"h"                         ,"H"            },
{  041,     0241, KEYCODE_J    , KEYCODE_J        ,"j"                         ,"J"            },
{  042,     0242, KEYCODE_K    , KEYCODE_K        ,"k"                         ,"K"            },
{  043,     0043, KEYCODE_L    , KEYCODE_L        ,"l"                         ,"L"            },
{  054,     0054, KEYCODE_MINUS, KEYCODE_PLUS_PAD ,"-"                         ,"+"            },
{  040,     0040, 0            , 0                ," (non-spacing middle dot)" ,"_"            },
{  072,     0272, KEYCODE_LSHIFT, 0               ,"Lower Case"                ,""             },
{  031,     0031, KEYCODE_Z    , KEYCODE_Z        ,"z"                         ,"Z"            },
{  027,     0227, KEYCODE_X    , KEYCODE_X        ,"x"                         ,"X"            },
{  063,     0263, KEYCODE_C    , KEYCODE_C        ,"c"                         ,"C"            },
{  025,     0025, KEYCODE_V    , KEYCODE_V        ,"v"                         ,"V"            },
{  062,     0062, KEYCODE_B    , KEYCODE_B        ,"b"                         ,"B"            },
{  045,     0045, KEYCODE_N    , KEYCODE_N        ,"n"                         ,"N"            },
{  044,     0244, KEYCODE_M    , KEYCODE_M        ,"m"                         ,"M"            },
{  033,     0233, KEYCODE_COMMA, KEYCODE_EQUALS   ,","                         ,"="            },
{  073,     0073, KEYCODE_ASTERISK,0              ,".(multiply)"               ,""             },
{  021,     0221, KEYCODE_SLASH, 0                ,"/"                         ,"?"            },
{  074,     0274, KEYCODE_LSHIFT, 0               ,"Upper Case"                ,""             },
{  000,     0200, KEYCODE_SPACE, KEYCODE_SPACE    ,"Space"                     ,""             },

/*
52 keys.  The function for the two keys at the top is unknown, I cannot read the caption on the
picture.

CONCISE  FIO-DEC  CHARACTER
  CODE     CODE  LOWER UPPER
-----------------------------
  00      200     Space
  01       01      1     "
  02       02      2     '
  03      203      3     ~
  04       04      4           (implies)
  05      205      5           (or)
  06      206      6           (and)
  07       07      7     <
  10       10      8     >
  11      211      9           (up arrow)
  20       20      0           (right arrow)
  21      221      /     ?
  22      222      s     S
  23       23      t     T
  24      224      u     U
  25       25      v     V
  26       26      w     W
  27      227      x     X
  30      230      y     Y
  31       31      z     Z
  33      233      ,     =
  36      236       tab
  40       40            _     (non-spacing middle dot)
  41      241      j     J
  42      242      k     K
  43       43      l     L
  44      244      m     M
  45       45      n     N
  46       46      o     O
  47      247      p     P
  50      250      q     Q
  51       51      r     R
  54       54      -     +
  55      255      )     ]
  56      256            |     (non-spacing overstrike)
  57       57      (     [
  61       61      a     A
  62       62      b     B
  63      263      c     C
  64       64      d     D
  65      265      e     E
  66      266      f     F
  67       67      g     G
  70       70      h     H
  71      271      i     I
  72      272     Lower Case
  73      73       .           (multiply)
  74      274     Upper Case
  75       75     Backspace
  77      277  Carriage Return


  00       00     Tape Feed
  -       100     Delete
  -        13     Stop Code



  34       -       black *
  35       -        red *

*/

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
	pdp1_get_test_word
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
