/***************************************************************************

	commodore c65 home computer
	peter.trauner@jk.uni-linz.ac.at
    documention
     www.funet.fi

***************************************************************************/

/*
------------------------------------
c65	commodore c65 (ntsc version)
c65pal	commodore c65 (pal version)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!
flickering affects in one video version, try the other video version

c65 prototype
 hardware emulation mode for c64
  adapter needed for c64 cartridges (many should work)
  only standard c64 keys plus cursor-up and cursor-left
  cpu not full compatible (no illegal instructions)
 enhanced keyboard
 m65ce02 processor core (1 or 3.5 Mhz frequency)
 vic3 (4567) with additional capabilities
  vic2 compatibility mode
  graphic modes
  80 column modes
 second sid audio chip (both 8580)
 uart 6511 rs232 chip
 special dma controller
 128 kb ram
 128 kb rom (kernel, editor, basic, monitor, characters, c64 roms)
 build in mfm floppy disk 3 1/2 inch double sided double density
 connector for second mem floppy disk
 special ram expansion slot
 rs232 connector (round din)
 no tape connector
 new expansion slot

state
-----
only booting
major memory management problems

rasterline based video system
 quick modified vic6567/c64 video chip
 no support for enhanced features, only 80 column mode
 no cpu holding
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support not finished
 rasterline not finished
no sound
cia6526's look in machine/cia6526.c
keyboard
gameport a
 paddles 1,2
 joystick 1
 2 button joystick/mouse joystick emulation
 no mouse
 lightpen (not finished)
gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse
serial bus
 simple disk drives
 no printer or other devices
expansion modules
 none (did there any exist)
expansion modules c64 (adapter needed)
 ultimax rom cartridges
 rom cartridges (exrom)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
no userport
 no rs232/v.24 interface
preliminary quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads pogram from type and starts it

Lightpen
--------
Paddle 5 x-axe
Paddle 6 y-axe

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00) (must NOT be specified on commandline
 or file from d64 image (here also directory LOAD"$",8 supported)
use LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate sys call to start them

several programs rely on more features
(loading other file types, writing, ...)

most games rely on starting own programs in the floppy drive
(and therefor cpu level emulation is needed)

Roms
----
.prg
.crt
.80 .90 .a0 .b0 .e0 .f0
files with boot-sign in it
  recogniced as roms

.prg files loaded at address in its first two bytes
.?0 files to address specified in extension
.crt roms to addresses in crt file

Quickloader
-----------
.prg and .p00 files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
loads first rom when you press quickload key (numeric slash)

when problems start with -log and look into error.log file
*/

#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/cia6526.h"
#include "includes/vic6567.h"
#include "includes/sid6581.h"
#include "includes/c1551.h"
#include "includes/vc1541.h"

#include "includes/c65.h"

static struct MemoryReadAddress c65_readmem[] =
{
	{0x00000, 0x00001, c64_m6510_port_r},
	{0x00002, 0x07fff, MRA_RAM},
	{0x08000, 0x09fff, MRA_BANK1},
	{0x0a000, 0x0bfff, MRA_BANK2},
	{0x0c000, 0x0cfff, MRA_BANK3},
	{0x0d000, 0x0d7ff, MRA_BANK4},
	{0x0d800, 0x0dbff, MRA_BANK6},
	{0x0dc00, 0x0dfff, MRA_BANK8},
	{0x0e000, 0x0ffff, MRA_BANK10},
	{0x10000, 0x1ffff, MRA_RAM},
	{0x20000, 0x3ffff, MRA_ROM},
	{0x40000, 0x7ffff, MRA_NOP},
	{0x80000, 0xfffff, MRA_RAM},
	/* 8 megabyte full address space! */
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress c65_writemem[] =
{
	{0x00000, 0x00001, c64_m6510_port_w, &c64_memory},
	{0x00002, 0x07fff, MWA_RAM},
	{0x08000, 0x09fff, MWA_RAM},
	{0x0a000, 0x0cfff, MWA_RAM},
	{0x0d000, 0x0d7ff, MWA_BANK5},
	{0x0d800, 0x0dbff, MWA_BANK7},
	{0x0dc00, 0x0dfff, MWA_BANK9},
	{0x0e000, 0x0ffff, MWA_RAM},
	{0x10000, 0x1f7ff, MWA_RAM},
	{0x1f800, 0x1ffff, MWA_RAM, &c64_colorram},
	{0x20000, 0x23fff, MWA_ROM}, /* &c65_dos},	   maps to 0x8000    */
	{0x24000, 0x28fff, MWA_ROM}, /* reserved */
	{0x29000, 0x29fff, MWA_ROM, &c65_chargen},
	{0x2a000, 0x2bfff, MWA_ROM, &c64_basic},
	{0x2c000, 0x2cfff, MWA_ROM, &c65_interface},
	{0x2d000, 0x2dfff, MWA_ROM, &c64_chargen},
	{0x2e000, 0x2ffff, MWA_ROM, &c64_kernal},
	{0x30000, 0x31fff, MWA_ROM}, /*&c65_monitor},	  monitor maps to 0x6000    */
	{0x32000, 0x37fff, MWA_ROM}, /*&c65_basic}, */
	{0x38000, 0x3bfff, MWA_ROM}, /*&c65_graphics}, */
	{0x3c000, 0x3dfff, MWA_ROM}, /* reserved */
	{0x3e000, 0x3ffff, MWA_ROM}, /* &c65_kernal}, */
	{0x40000, 0x7ffff, MWA_NOP},
	{0x80000, 0xfffff, MWA_RAM},
/*	{0x80000, 0xfffff, MWA_BANK16}, */
	{-1}							   /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define C65_DIPS \
     PORT_START \
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_SLASH_PAD)\
	 PORT_BIT (0x7c00, 0x0, IPT_UNUSED) /* no tape */\
     PORT_DIPNAME (0x300, 0x0, "Memory Expansion")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (0x100, "512 KByte")\
	 PORT_DIPSETTING (0x200, "4 MByte")\
	PORT_DIPNAME   ( 0x80, 0x80, "Sid Chip Type")\
	PORT_DIPSETTING(  0, "MOS6581" )\
	PORT_DIPSETTING(0x80, "MOS8580" )\
	 /*PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")*/\
	 /*PORT_DIPSETTING (0, "Automatic")*/\
	 /*PORT_DIPSETTING (4, "Ultimax (GAME)")*/\
	 /*PORT_DIPSETTING (8, "C64 (EXROM)")*/\
	 /*PORT_DIPSETTING (0x10, "CBM Supergames")*/\
	 /*PORT_DIPSETTING (0x14, "Ocean Robocop2")*/\
	 PORT_DIPNAME (0x02, 0x02, "Serial Bus/Device 10")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (2, "Floppy Drive Simulation")\
	 PORT_DIPNAME (0x01, 0x01, "Serial Bus/Device 11")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (1, "Floppy Drive Simulation")


INPUT_PORTS_START (c65)
	C64_DIPS
	C65_DIPS
	PORT_START
	DIPS_HELPER (0x8000, "Arrow-Left", KEYCODE_TILDE)
	DIPS_HELPER (0x4000, "1 !   BLK   ORNG", KEYCODE_1)
	DIPS_HELPER (0x2000, "2 \"   WHT   BRN", KEYCODE_2)
	DIPS_HELPER (0x1000, "3 #   RED   L RED", KEYCODE_3)
	DIPS_HELPER (0x0800, "4 $   CYN   D GREY", KEYCODE_4)
	DIPS_HELPER (0x0400, "5 %   PUR   GREY", KEYCODE_5)
	DIPS_HELPER (0x0200, "6 &   GRN   L GRN", KEYCODE_6)
	DIPS_HELPER (0x0100, "7 '   BLU   L BLU", KEYCODE_7)
	DIPS_HELPER (0x0080, "8 (   YEL   L GREY", KEYCODE_8)
	DIPS_HELPER (0x0040, "9 )   RVS-ON", KEYCODE_9)
	DIPS_HELPER (0x0020, "0     RVS-OFF", KEYCODE_0)
	DIPS_HELPER (0x0010, "+", KEYCODE_PLUS_PAD)
	DIPS_HELPER (0x0008, "-", KEYCODE_MINUS_PAD)
	DIPS_HELPER (0x0004, "Pound", KEYCODE_MINUS)
	DIPS_HELPER (0x0002, "HOME CLR", KEYCODE_EQUALS)
	DIPS_HELPER (0x0001, "DEL INST", KEYCODE_BACKSPACE)
	PORT_START
	DIPS_HELPER (0x8000, "(C65)TAB", KEYCODE_TAB)
	DIPS_HELPER (0x4000, "Q", KEYCODE_Q)
	DIPS_HELPER (0x2000, "W", KEYCODE_W)
	DIPS_HELPER (0x1000, "E", KEYCODE_E)
	DIPS_HELPER (0x0800, "R", KEYCODE_R)
	DIPS_HELPER (0x0400, "T", KEYCODE_T)
	DIPS_HELPER (0x0200, "Y", KEYCODE_Y)
	DIPS_HELPER (0x0100, "U", KEYCODE_U)
	DIPS_HELPER (0x0080, "I", KEYCODE_I)
	DIPS_HELPER (0x0040, "O", KEYCODE_O)
	DIPS_HELPER (0x0020, "P", KEYCODE_P)
	DIPS_HELPER (0x0010, "At", KEYCODE_OPENBRACE)
	DIPS_HELPER (0x0008, "*", KEYCODE_ASTERISK)
	DIPS_HELPER (0x0004, "Arrow-Up Pi", KEYCODE_CLOSEBRACE)
	DIPS_HELPER (0x0002, "RESTORE", KEYCODE_PRTSCR)
	DIPS_HELPER (0x0001, "CTRL", KEYCODE_RCONTROL)
	PORT_START
	PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
			   "(Left-Shift) SHIFT-LOCK (switch)",
			   KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING(0x8000, DEF_STR(On) )
	DIPS_HELPER (0x4000, "A", KEYCODE_A)
	DIPS_HELPER (0x2000, "S", KEYCODE_S)
	DIPS_HELPER (0x1000, "D", KEYCODE_D)
	DIPS_HELPER (0x0800, "F", KEYCODE_F)
	DIPS_HELPER (0x0400, "G", KEYCODE_G)
	DIPS_HELPER (0x0200, "H", KEYCODE_H)
	DIPS_HELPER (0x0100, "J", KEYCODE_J)
	DIPS_HELPER (0x0080, "K", KEYCODE_K)
	DIPS_HELPER (0x0040, "L", KEYCODE_L)
	DIPS_HELPER (0x0020, ": [", KEYCODE_COLON)
	DIPS_HELPER (0x0010, "; ]", KEYCODE_QUOTE)
	DIPS_HELPER (0x0008, "=", KEYCODE_BACKSLASH)
	DIPS_HELPER (0x0004, "RETURN", KEYCODE_ENTER)
	DIPS_HELPER (0x0002, "CBM", KEYCODE_RALT)
	DIPS_HELPER (0x0001, "Left-Shift", KEYCODE_LSHIFT)
	PORT_START
	DIPS_HELPER (0x8000, "Z", KEYCODE_Z)
	DIPS_HELPER (0x4000, "X", KEYCODE_X)
	DIPS_HELPER (0x2000, "C", KEYCODE_C)
	DIPS_HELPER (0x1000, "V", KEYCODE_V)
	DIPS_HELPER (0x0800, "B", KEYCODE_B)
	DIPS_HELPER (0x0400, "N", KEYCODE_N)
	DIPS_HELPER (0x0200, "M", KEYCODE_M)
	DIPS_HELPER (0x0100, ", <", KEYCODE_COMMA)
	DIPS_HELPER (0x0080, ". >", KEYCODE_STOP)
	DIPS_HELPER (0x0040, "/ ?", KEYCODE_SLASH)
	DIPS_HELPER (0x0020, "Right-Shift", KEYCODE_RSHIFT)
	DIPS_HELPER (0x0010, "(Right-Shift Cursor-Down)CRSR-UP",
				 KEYCODE_8_PAD)
	DIPS_HELPER (0x0008, "Space", KEYCODE_SPACE)
	DIPS_HELPER (0x0004, "(Right-Shift Cursor-Right)CRSR-LEFT",
				 KEYCODE_4_PAD)
	DIPS_HELPER (0x0002, "CRSR-DOWN", KEYCODE_2_PAD)
	DIPS_HELPER (0x0001, "CRSR-RIGHT", KEYCODE_6_PAD)
	PORT_START
	DIPS_HELPER (0x8000, "STOP RUN", KEYCODE_ESC)
	DIPS_HELPER (0x4000, "(C65)ESC", KEYCODE_F1)
	DIPS_HELPER (0x2000, "(C65)ALT", KEYCODE_F2)
	PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
			   "(C65)CAPSLOCK(switch)",
			   KEYCODE_F3, IP_JOY_NONE)
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING(0x1000, DEF_STR(On) )
	DIPS_HELPER (0x0800, "(C65)NO SCRL", KEYCODE_F4)
	DIPS_HELPER (0x0400, "f1 f2", KEYCODE_F5)
	DIPS_HELPER (0x0200, "f3 f4", KEYCODE_F6)
	DIPS_HELPER (0x0100, "f5 f6", KEYCODE_F7)
	DIPS_HELPER (0x0080, "f7 f8", KEYCODE_F8)
	DIPS_HELPER (0x0040, "(C65)f9 f10", KEYCODE_F9)
	DIPS_HELPER (0x0020, "(C65)f11 f12", KEYCODE_F10)
	DIPS_HELPER (0x0010, "(C65)f13 f14", KEYCODE_F11)
	DIPS_HELPER (0x0008, "(C65)HELP", KEYCODE_F12)
INPUT_PORTS_END

INPUT_PORTS_START (c65ger)
	 C64_DIPS
     C65_DIPS
	 PORT_START
     DIPS_HELPER (0x8000, "_                    < >", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "1 !  BLK   ORNG", KEYCODE_1)
	 DIPS_HELPER (0x2000, "2 \"  WHT   BRN", KEYCODE_2)
	 DIPS_HELPER (0x1000, "3 #  RED   L RED       Paragraph", KEYCODE_3)
	 DIPS_HELPER (0x0800, "4 $  CYN   D GREY", KEYCODE_4)
	 DIPS_HELPER (0x0400, "5 %  PUR   GREY", KEYCODE_5)
	 DIPS_HELPER (0x0200, "6 &  GRN   L GRN", KEYCODE_6)
	 DIPS_HELPER (0x0100, "7 '  BLU   L BLU	      /", KEYCODE_7)
	 DIPS_HELPER (0x0080, "8 (  YEL   L GREY", KEYCODE_8)
	 DIPS_HELPER (0x0040, "9 )  RVS-ON", KEYCODE_9)
	 DIPS_HELPER (0x0020, "0    RVS-OFF           =", KEYCODE_0)
	 DIPS_HELPER (0x0010, "+                    Sharp-S ?", KEYCODE_PLUS_PAD)
	 DIPS_HELPER (0x0008, "-                    '  `",KEYCODE_MINUS_PAD)
	 DIPS_HELPER (0x0004, "\\                    [ Arrow-Up", KEYCODE_MINUS)
	 DIPS_HELPER (0x0002, "HOME CLR", KEYCODE_EQUALS)
	 DIPS_HELPER (0x0001, "DEL INST", KEYCODE_BACKSPACE)
	 PORT_START
     DIPS_HELPER (0x8000, "(C65)TAB", KEYCODE_TAB)
	 DIPS_HELPER (0x4000, "Q", KEYCODE_Q)
	 DIPS_HELPER (0x2000, "W", KEYCODE_W)
	 DIPS_HELPER (0x1000, "E", KEYCODE_E)
	 DIPS_HELPER (0x0800, "R", KEYCODE_R)
	 DIPS_HELPER (0x0400, "T", KEYCODE_T)
	 DIPS_HELPER (0x0200, "Y                    Z", KEYCODE_Y)
	 DIPS_HELPER (0x0100, "U", KEYCODE_U)
	 DIPS_HELPER (0x0080, "I", KEYCODE_I)
	 DIPS_HELPER (0x0040, "O", KEYCODE_O)
	 DIPS_HELPER (0x0020, "P", KEYCODE_P)
	 DIPS_HELPER (0x0010, "Paragraph Arrow-Up   Diaresis-U",KEYCODE_OPENBRACE)
	 DIPS_HELPER (0x0008, "* `                  + *",KEYCODE_ASTERISK)
	 DIPS_HELPER (0x0004, "Sum Pi               ] \\",KEYCODE_CLOSEBRACE)
	 DIPS_HELPER (0x0002, "RESTORE", KEYCODE_PRTSCR)
	 DIPS_HELPER (0x0001, "CTRL", KEYCODE_RCONTROL)
	 PORT_START
     PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_DIPSETTING(  0, DEF_STR(Off) )\
	PORT_DIPSETTING(0x8000, DEF_STR(On) )\
	 DIPS_HELPER (0x4000, "A", KEYCODE_A)
	 DIPS_HELPER (0x2000, "S", KEYCODE_S)
	 DIPS_HELPER (0x1000, "D", KEYCODE_D)
	 DIPS_HELPER (0x0800, "F", KEYCODE_F)
	 DIPS_HELPER (0x0400, "G", KEYCODE_G)
	 DIPS_HELPER (0x0200, "H", KEYCODE_H)
	 DIPS_HELPER (0x0100, "J", KEYCODE_J)
	 DIPS_HELPER (0x0080, "K", KEYCODE_K)
	 DIPS_HELPER (0x0040, "L", KEYCODE_L)
	 DIPS_HELPER (0x0020, ": [                  Diaresis-O",
				  KEYCODE_COLON)
	 DIPS_HELPER (0x0010, "; ]                  Diaresis-A",
				  KEYCODE_QUOTE)
	 DIPS_HELPER (0x0008, "=                    # '",
				  KEYCODE_BACKSLASH)
	 DIPS_HELPER (0x0004, "RETURN", KEYCODE_ENTER)
	 DIPS_HELPER (0x0002, "CBM", KEYCODE_RALT)
	 DIPS_HELPER (0x0001, "Left-Shift", KEYCODE_LSHIFT)
	 PORT_START
     DIPS_HELPER (0x8000, "Z                    Y", KEYCODE_Z)
	 DIPS_HELPER (0x4000, "X", KEYCODE_X)
	 DIPS_HELPER (0x2000, "C", KEYCODE_C)
	 DIPS_HELPER (0x1000, "V", KEYCODE_V)
	 DIPS_HELPER (0x0800, "B", KEYCODE_B)
	 DIPS_HELPER (0x0400, "N", KEYCODE_N)
	 DIPS_HELPER (0x0200, "M", KEYCODE_M)
	 DIPS_HELPER (0x0100, ", <                    ;", KEYCODE_COMMA)
	 DIPS_HELPER (0x0080, ". >                    :", KEYCODE_STOP)
	 DIPS_HELPER (0x0040, "/ ?                  - _", KEYCODE_SLASH)
	 DIPS_HELPER (0x0020, "Right-Shift", KEYCODE_RSHIFT)
	 DIPS_HELPER (0x0010, "(Right-Shift Cursor-Down)CRSR-UP",
				  KEYCODE_8_PAD)
	 DIPS_HELPER (0x0008, "Space", KEYCODE_SPACE)
	 DIPS_HELPER (0x0004, "(Right-Shift Cursor-Right)CRSR-LEFT",
				  KEYCODE_4_PAD)
	 DIPS_HELPER (0x0002, "CRSR-DOWN", KEYCODE_2_PAD)
	 DIPS_HELPER (0x0001, "CRSR-RIGHT", KEYCODE_6_PAD)
     PORT_START
     DIPS_HELPER (0x8000, "STOP RUN", KEYCODE_ESC)
	 DIPS_HELPER (0x4000, "(C65)ESC", KEYCODE_F1)
	 DIPS_HELPER (0x2000, "(C65)ALT", KEYCODE_F2)
	 PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(C65)DIN ASC(switch)",
				KEYCODE_F3, IP_JOY_NONE)
	PORT_DIPSETTING(  0, "ASC" )
	PORT_DIPSETTING(0x1000, "DIN" )
	DIPS_HELPER (0x0800, "(C65)NO SCRL", KEYCODE_F4)
	DIPS_HELPER (0x0400, "f1 f2", KEYCODE_F5)
	DIPS_HELPER (0x0200, "f3 f4", KEYCODE_F6)
	DIPS_HELPER (0x0100, "f5 f6", KEYCODE_F7)
	DIPS_HELPER (0x0080, "f7 f8", KEYCODE_F8)
	DIPS_HELPER (0x0040, "(C65)f9 f10", KEYCODE_F9)
	DIPS_HELPER (0x0020, "(C65)f11 f12", KEYCODE_F10)
	DIPS_HELPER (0x0010, "(C65)f13 f14", KEYCODE_F11)
	DIPS_HELPER (0x0008, "(C65)HELP", KEYCODE_F12)
INPUT_PORTS_END

static void c65_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, vic3_palette, sizeof (vic3_palette));
}

#if 0
	/* caff */
	/* dma routine alpha 1 (0x400000 reversed copy)*/
	ROM_LOAD ("910111.bin", 0x20000, 0x20000, 0xc5d8d32e)
	/* b96b */
	/* dma routine alpha 2 */
	ROM_LOAD ("910523.bin", 0x20000, 0x20000, 0xe8235dd4)
	/* 888c */
	/* dma routine alpha 2 */
	ROM_LOAD ("910626.bin", 0x20000, 0x20000, 0x12527742)
	/* c9cd */
	/* dma routine alpha 2 */
	ROM_LOAD ("910828.bin", 0x20000, 0x20000, 0x3ee40b06)
	/* 4bcf loading demo disk??? */
	/* basic program stored at 0x4000 ? */
	/* dma routine alpha 2 */
	ROM_LOAD ("911001.bin", 0x20000, 0x20000, 0x0888b50f)
	/* german e96a */
	/* dma routine alpha 1 */
	ROM_LOAD ("910429.bin", 0x20000, 0x20000, 0xb025805c)
#endif

ROM_START (c65)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("911001.bin", 0x20000, 0x20000, 0x0888b50f)
ROM_END

ROM_START (c65e)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("910828.bin", 0x20000, 0x20000, 0x3ee40b06)
ROM_END

ROM_START (c65d)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("910626.bin", 0x20000, 0x20000, 0x12527742)
ROM_END

ROM_START (c65c)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("910523.bin", 0x20000, 0x20000, 0xe8235dd4)
ROM_END

ROM_START (c65ger)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("910429.bin", 0x20000, 0x20000, 0xb025805c)
ROM_END

ROM_START (c65a)
	ROM_REGION (0x800000, REGION_CPU1)
/*	ROM_REGION (0x100000, REGION_CPU1) */
	ROM_LOAD ("910111.bin", 0x20000, 0x20000, 0xc5d8d32e)
ROM_END


static struct MachineDriver machine_driver_c65 =
{
  /* basic machine hardware */
	{
		{
			CPU_M4510,
			3500000, /* or VIC6567_CLOCK, */
			c65_readmem, c65_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	c65_init_machine,
	c65_shutdown_machine,

  /* video hardware */
	656,							   /* screen width */
	416,							   /* screen height */
	{0, 656 - 1, 0, 416 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic3_palette) / sizeof (vic3_palette[0]) / 3,
	0,
	c65_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid6581_sound_interface },
		{ 0 }
	}
};

static struct MachineDriver machine_driver_c65pal =
{
  /* basic machine hardware */
	{
		{
			CPU_M4510,
			3500000, /* or VIC6569_CLOCK,*/
			c65_readmem, c65_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
	},
	VIC6569_VRETRACERATE,
	DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	0,
	c65_init_machine,
	c65_shutdown_machine,

  /* video hardware */
	656,							   /* screen width */
	416,							   /* screen height */
	{0, 656 - 1, 0, 416 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic3_palette) / sizeof (vic3_palette[0]) / 3,
	0,
	c65_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid6581_sound_interface },
		{ 0 }
	}
};

static const struct IODevice io_c65[] =
{
	IODEVICE_CBM_C65_QUICK,
#if 0
	IODEVICE_CBM_ROM(c64_rom_id),
#endif
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

#define init_c65 c65_driver_init
#define init_c65_alpha1 c65_driver_init
#define init_c65pal c65pal_driver_init

#define io_c65ger io_c65
#define io_c65e io_c65
#define io_c65d io_c65
#define io_c65c io_c65
#define io_c65a io_c65

/*    	YEAR	NAME    PARENT	MACHINE	INPUT	INIT 	COMPANY   							FULLNAME */
COMPX (	199?, 	c65,	0,		c65,	c65,	c65,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, NTSC, 911001)", 		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMPX (	199?, 	c65e,	c65,		c65,	c65,	c65,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, NTSC, 910828)", 		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMPX (	199?, 	c65d,	c65,		c65,	c65,	c65,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, NTSC, 910626)", 		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMPX (	199?, 	c65c,	c65,		c65,	c65,	c65,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, NTSC, 910523)", 		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMPX (	199?, 	c65ger,	c65,	c65pal,	c65ger,	c65pal,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, German PAL, 910429)",	GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMPX (	199?, 	c65a,	c65,		c65,	c65,	c65_alpha1,	"Commodore Business Machines Co.",	"C65 / C64DX (Prototype, NTSC, 910111)", 		GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)

