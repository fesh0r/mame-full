/***************************************************************************
    commodore c128 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation:
 	 iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org
***************************************************************************/

/*
------------------------------------
c128	commodore c128 (ntsc version)
c128ger	commodore c128 german (pal version)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!
flickering affects in one video version, try the other video version

c128
 hardware emulation mode for c64 
  displayed over din/tv connector only
  c64 cartridges
  c64 userport
  only the c64 standard keys available
 country support (keyboard, character rom, kernel (keyboard polling))
  character rom german
  character rom french, belgium, italian
  editor, kernel for german
  editor, kernel for swedish
  editor, kernel for finish
  editor, kernel for norwegian
  editor, kernel for french
  editor, kernel for italian
 enhanced keyboard
 m8502 processor (additional port pin)
 additional vdc videochip for 80 column modes with own connector (rgbi)
  16k byte ram for this chip
 128 kbyte ram, 64 kbyte rom, 8k byte charrom, 2k byte static colorram
  (1 mbyte ram possible)
 sid6581 soundchip
 c64 expansion port with some additional pins
 z80 cpu for CPM mode
c128d
 c1571 floppy drive build in
 64kb vdc ram
 sid8580 sound chip
c128cr/c128dcr
 cost reduced
 only modified for cheaper production (newer rams, ...)

state
-----
uses c64 emulation for c64 mode
so only notes for the additional subsystems here

rasterline based video system
 no cpu holding
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support not finished
 rasterline not finished
no vdc emulation
z80 emulation (not finished)
no internal function rom
c64 mode not working
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
simple tape support
 (not working, cia timing?)
serial bus
 simple disk drives
 no printer or other devices
expansion modules
 no c128 modules
expansion modules c64
 rom cartridges (exrom)
 ultimax rom cartridges (game)
 c64 cartridges (only standard rom cartridges)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
no userport
 no rs232/v.24 interface
quickloader

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

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 125000 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working(no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs relies on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00) (must NOT be specified on commandline)
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
.prg .crt
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

c64 mode
--------
hold down commodore key while reseting or turning on
type go64 at the c128 command mode

cpm mode
--------
cpm disk must be inserted in device 8
turn on computer
or
type boot at the c128 command mode

when problems start with -log and look into error.log file
*/

#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/cia6526.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/sndhrdw/sid6581.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"

#include "mess/machine/c128.h"

#ifdef C128_Z80
/* shares ram with m8502
 * how to bankswitch ?
 * bank 0
 * 0x0000, 0x03ff bios rom
 * 0x1400, 0x1bff vdc videoram
 * 0x1c00, 0x23ff vdc colorram
 * 0x2c00, 0x2fff vic2e videoram
 * 0xff00, 0xff04 mmu registers
 * else ram (dram bank 0?)
 * bank 1
 * 0x0000-0xedff ram (dram bank 1?)
 * 0xee00-0xffff ram as bank 0
 */
static struct MemoryReadAddress c128_z80_readmem[] =
{
	{0x0000, 0x0fff, MRA_BANK10},	   /* rom or dram bank 1 */
	{0x1000, 0x13ff, MRA_RAM},		   /* dram bank 0 or 1 */
	{0x1400, 0x23ff, MRA_RAM},		   /* vdc ram  or dram bank 1 */
	{0x2400, 0x27ff, MRA_RAM},		   /* dram bank 0 or 1 */
	{0x2800, 0xedff, MRA_RAM},		   /* dram bank 0 or 1 */
	{0xee00, 0xfeff, MRA_RAM},		   /* dram bank 0 */
	{0xff00, 0xff04, c128_mmu8722_ff00_r},
	{0xff05, 0xffff, MRA_RAM},		   /* dram bank 0 */
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress c128_z80_writemem[] =
{
	{0x0000, 0x0fff, MWA_BANK10, &c64_memory},	/* dram bank 0 or 1 */
	{0x1000, 0x13ff, MWA_RAM},		   /* dram bank 0 or 1 */
	{0x1400, 0x23ff, MWA_RAM},		   /* vdc ram  or dram bank 1 */
	{0x2400, 0x27ff, MWA_RAM},		   /* dram bank 0 or 1 */
	{0x2800, 0xedff, MWA_RAM},		   /* dram bank 0 or 1 */
	{0xee00, 0xfeff, MWA_RAM},		   /* dram bank 0 */
	{0xff00, 0xff04, c128_mmu8722_ff00_w},
	{0xff05, 0xffff, MWA_RAM},		   /* dram bank 0 */

	{0x10000, 0x1ffff, MWA_RAM},
	{0x20000, 0x3ffff, MWA_RAM},	   /* or nothing */
	{0x40000, 0x47fff, MWA_ROM, &c128_basic},	/* maps to 0x4000 */
	{0x48000, 0x49fff, MWA_ROM, &c64_basic},	/* maps to 0xa000 */
	{0x4a000, 0x4bfff, MWA_ROM, &c64_kernal},	/* maps to 0xe000 */
	{0x4c000, 0x4cfff, MWA_ROM, &c128_editor},
	{0x4d000, 0x4dfff, MWA_ROM, &c128_z80},		/* maps to z80 0 */
	{0x4e000, 0x4ffff, MWA_ROM, &c128_kernal},
	{0x50000, 0x57fff, MWA_ROM, &c128_internal_function},
	{0x58000, 0x5ffff, MWA_ROM, &c128_external_function},
	{0x60000, 0x60fff, MWA_ROM, &c64_chargen},
	{0x61000, 0x61fff, MWA_ROM, &c128_chargen},
	{0x62000, 0x627ff, MWA_RAM, &c64_colorram},
	/* 2 kbyte by 8 bits, only 1 kbyte by 4 bits used) */
	{0x62800, 0x727ff, MWA_RAM, &c128_vdcram},	/* 16 or 64 kbyte */
	{-1}							   /* end of table */
};

static struct IOReadPort c128_z80_readio[] =
{
	{0x1000, 0x13ff, c64_colorram_read},
	{0xd000, 0xd3ff, vic2_port_r},
	{0xd400, 0xd4ff, sid6581_0_port_r},
	{0xd500, 0xd5ff, c128_mmu8722_port_r},
	/*{ 0xd600, 0xd6ff, dma_port_r }, */
	{0xdc00, 0xdcff, cia6526_0_port_r},
	{0xdd00, 0xddff, cia6526_1_port_r},
	{-1}							   /* end of table */
};

static struct IOWritePort c128_z80_writeio[] =
{
	{0x1000, 0x13ff, c64_colorram_write},
	{0xd000, 0xd3ff, vic2_port_w},
	{0xd400, 0xd4ff, sid6581_0_port_w},
	{0xd500, 0xd5ff, c128_mmu8722_port_w},
	/*{ 0xd600, 0xd6ff, dma_port_w }, */
	{0xdc00, 0xdcff, cia6526_0_port_w},
	{0xdd00, 0xddff, cia6526_1_port_w},
	{-1}							   /* end of table */
};

#endif

static struct MemoryReadAddress c128_readmem[] =
{
	{0x0000, 0x0001, c64_m6510_port_r},
	{0x0002, 0x00ff, MRA_BANK1},
	{0x0100, 0x01ff, MRA_BANK2},
	{0x0200, 0x03ff, MRA_BANK3},
	{0x0400, 0x0fff, MRA_BANK4},
	{0x1000, 0x1fff, MRA_BANK5},
	{0x2000, 0x3fff, MRA_BANK6},

	{0x4000, 0x7fff, MRA_BANK7},
	{0x8000, 0x9fff, MRA_BANK8},
	{0xa000, 0xbfff, MRA_BANK9},

	{0xc000, 0xcfff, MRA_BANK12},
	{0xd000, 0xdfff, MRA_BANK13},
	{0xe000, 0xfeff, MRA_BANK14},
	{0xff00, 0xff04, MRA_BANK15},	   /* mmu c128 modus */
	{0xff05, 0xffff, MRA_BANK16},
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress c128_writemem[] =
{
#ifdef C128_Z80
	{0x0000, 0x0001, c64_m6510_port_w},
#else
	{0x0000, 0x0001, c64_m6510_port_w, &c64_memory},
#endif
	{0x0002, 0x00ff, MWA_BANK1},
	{0x0100, 0x01ff, MWA_BANK2},
	{0x0200, 0x03ff, MWA_BANK3},
	{0x0400, 0x0fff, MWA_BANK4},
	{0x1000, 0x1fff, MWA_BANK5},
	{0x2000, 0x3fff, MWA_BANK6},

	{0x4000, 0x7fff, c128_write_4000},
	{0x8000, 0x9fff, c128_write_8000},
	{0xa000, 0xcfff, c128_write_a000},
	{0xd000, 0xdfff, c128_write_d000},
	{0xe000, 0xfeff, c128_write_e000},
	{0xff00, 0xff04, c128_write_ff00},
	{0xff05, 0xffff, c128_write_ff05},
#ifndef C128_Z80
	{0x10000, 0x1ffff, MWA_RAM},
	{0x20000, 0x3ffff, MWA_RAM},	   /* or nothing */
	{0x40000, 0x47fff, MWA_ROM, &c128_basic},	/* maps to 0x4000 */
	{0x48000, 0x49fff, MWA_ROM, &c64_basic},	/* maps to 0xa000 */
	{0x4a000, 0x4bfff, MWA_ROM, &c64_kernal},	/* maps to 0xe000 */
	{0x4c000, 0x4cfff, MWA_ROM, &c128_editor},
	{0x4d000, 0x4dfff, MWA_ROM, &c128_z80},		/* maps to z80 0 */
	{0x4e000, 0x4ffff, MWA_ROM, &c128_kernal},
	{0x50000, 0x57fff, MWA_ROM, &c128_internal_function},
	{0x58000, 0x5ffff, MWA_ROM, &c128_external_function},
	{0x60000, 0x60fff, MWA_ROM, &c64_chargen},
	{0x61000, 0x61fff, MWA_ROM, &c128_chargen},
	{0x62000, 0x627ff, MWA_RAM, &c64_colorram},
	/* 2 kbyte by 8 bits, only 1 kbyte by 4 bits used) */
	{0x62800, 0x727ff, MWA_RAM, &c128_vdcram},	/* 16 or 64 kbyte */
#endif
	{-1}							   /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
    PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define C128_DIPS \
     PORT_START \
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_SLASH_PAD)\
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )\
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )\
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_NONE)\
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_NONE)\
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_NONE)\
	 PORT_DIPNAME   ( 0x300, 0x00, "Main Memory/MMU Version")\
	 PORT_DIPSETTING(  0, "128 KByte" )\
	 PORT_DIPSETTING(0x100, "256 KByte" )\
	 PORT_DIPSETTING(0x200, "1024 KByte" )\
	 PORT_DIPNAME   ( 0x80, 0x80, "VDC Memory (RGBI)")\
	 PORT_DIPSETTING(  0, "16 KByte" )\
	 PORT_DIPSETTING(  0x80, "64 KByte" )\
     PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")\
	 PORT_DIPSETTING (0, "Automatic")\
	 PORT_DIPSETTING (4, "Ultimax (GAME)")\
	 PORT_DIPSETTING (8, "C64 (EXROM)")\
	 PORT_DIPSETTING (0x10, "C64 CBM Supergames")\
	 PORT_DIPSETTING (0x14, "C64 Ocean Robocop2")\
	 PORT_DIPSETTING (0x1c, "C128")\
	 PORT_DIPNAME (0x02, 0x02, "Serial Bus/Device 8")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (2, "VC1541 Floppy Drive")\
	 PORT_DIPNAME (0x01, 0x01, "Serial Bus/Device 9")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (1, "VC1541 Floppy Drive")

#define DIPS_KEYS_BOTH \
	PORT_START \
    DIPS_HELPER( 0x8000, "Numericblock 7", KEYCODE_NONE)\
	DIPS_HELPER( 0x4000, "Numericblock 8", KEYCODE_NONE)\
	DIPS_HELPER( 0x2000, "Numericblock 9", KEYCODE_NONE)\
	DIPS_HELPER( 0x1000, "Numericblock +", KEYCODE_NONE)\
	DIPS_HELPER( 0x0800, "Numericblock 4", KEYCODE_NONE)\
	DIPS_HELPER( 0x0400, "Numericblock 5", KEYCODE_NONE)\
	DIPS_HELPER( 0x0200, "Numericblock 6", KEYCODE_NONE)\
	DIPS_HELPER( 0x0100, "Numericblock -", KEYCODE_NONE)\
	DIPS_HELPER( 0x0080, "Numericblock 1", KEYCODE_NONE)\
	DIPS_HELPER( 0x0040, "Numericblock 2", KEYCODE_NONE)\
	DIPS_HELPER( 0x0020, "Numericblock 3", KEYCODE_NONE)\
	DIPS_HELPER( 0x0010, "Numericblock 0", KEYCODE_NONE)\
	DIPS_HELPER( 0x0008, "Numericblock .", KEYCODE_NONE)\
	DIPS_HELPER( 0x0004, "Numericblock Enter", KEYCODE_NONE)\
	DIPS_HELPER( 0x0002, "Special (Right-Shift CRSR-DOWN) CRSR-UP", \
		     KEYCODE_8_PAD)\
	DIPS_HELPER( 0x0001, "Special (Right-Shift CRSR-RIGHT) CRSR-LEFT", \
		     KEYCODE_4_PAD)\

INPUT_PORTS_START (c128)
	 C64_DIPS
     C128_DIPS
	 PORT_START
     DIPS_HELPER (0x8000, "(64)Arrow-Left", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !	BLK   ORNG", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"	 WHT   BRN", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #	RED   L RED", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $	CYN   D GREY", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %	PUR   GREY", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &	GRN   L GRN", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '	BLU   L BLU", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (	YEL   L GREY", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )	RVS-ON", KEYCODE_9)
	 DIPS_HELPER (0x0020, "(64)0	RVS-OFF", KEYCODE_0)
	 DIPS_HELPER (0x0010, "(64)+", KEYCODE_PLUS_PAD)
	 DIPS_HELPER (0x0008, "(64)-", KEYCODE_MINUS_PAD)
	 DIPS_HELPER (0x0004, "(64)Pound", KEYCODE_MINUS)
	 DIPS_HELPER (0x0002, "(64)HOME CLR", KEYCODE_EQUALS)
	 DIPS_HELPER (0x0001, "(64)DEL INST", KEYCODE_BACKSPACE)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)CONTROL", KEYCODE_RCONTROL)
	 DIPS_HELPER (0x4000, "(64)Q", KEYCODE_Q)
	 DIPS_HELPER (0x2000, "(64)W", KEYCODE_W)
	 DIPS_HELPER (0x1000, "(64)E", KEYCODE_E)
	 DIPS_HELPER (0x0800, "(64)R", KEYCODE_R)
	 DIPS_HELPER (0x0400, "(64)T", KEYCODE_T)
	 DIPS_HELPER (0x0200, "(64)Y", KEYCODE_Y)
	 DIPS_HELPER (0x0100, "(64)U", KEYCODE_U)
	 DIPS_HELPER (0x0080, "(64)I", KEYCODE_I)
	 DIPS_HELPER (0x0040, "(64)O", KEYCODE_O)
	 DIPS_HELPER (0x0020, "(64)P", KEYCODE_P)
	 DIPS_HELPER (0x0010, "(64)At", KEYCODE_OPENBRACE)
	 DIPS_HELPER (0x0008, "(64)*", KEYCODE_ASTERISK)
	 DIPS_HELPER (0x0004, "(64)Arrow-Up Pi", KEYCODE_CLOSEBRACE)
	 DIPS_HELPER (0x0002, "(64)RESTORE", KEYCODE_PRTSCR)
	 DIPS_HELPER (0x0001, "(64)STOP RUN", KEYCODE_TAB)
	 PORT_START
     PORT_BITX (0x8000, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 DIPS_HELPER (0x4000, "(64)A", KEYCODE_A)
	 DIPS_HELPER (0x2000, "(64)S", KEYCODE_S)
	 DIPS_HELPER (0x1000, "(64)D", KEYCODE_D)
	 DIPS_HELPER (0x0800, "(64)F", KEYCODE_F)
	 DIPS_HELPER (0x0400, "(64)G", KEYCODE_G)
	 DIPS_HELPER (0x0200, "(64)H", KEYCODE_H)
	 DIPS_HELPER (0x0100, "(64)J", KEYCODE_J)
	 DIPS_HELPER (0x0080, "(64)K", KEYCODE_K)
	 DIPS_HELPER (0x0040, "(64)L", KEYCODE_L)
	 DIPS_HELPER (0x0020, "(64): [", KEYCODE_COLON)
	 DIPS_HELPER (0x0010, "(64); ]", KEYCODE_QUOTE)
	 DIPS_HELPER (0x0008, "(64)=", KEYCODE_BACKSLASH)
	 DIPS_HELPER (0x0004, "(64)RETURN", KEYCODE_ENTER)
	 DIPS_HELPER (0x0002, "(64)CBM", KEYCODE_RALT)
	 DIPS_HELPER (0x0001, "(64)Left-Shift", KEYCODE_LSHIFT)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)Z", KEYCODE_Z)
	 DIPS_HELPER (0x4000, "(64)X", KEYCODE_X)
	 DIPS_HELPER (0x2000, "(64)C", KEYCODE_C)
	 DIPS_HELPER (0x1000, "(64)V", KEYCODE_V)
	 DIPS_HELPER (0x0800, "(64)B", KEYCODE_B)
	 DIPS_HELPER (0x0400, "(64)N", KEYCODE_N)
	 DIPS_HELPER (0x0200, "(64)M", KEYCODE_M)
	 DIPS_HELPER (0x0100, "(64), <", KEYCODE_COMMA)
	 DIPS_HELPER (0x0080, "(64). >", KEYCODE_STOP)
	 DIPS_HELPER (0x0040, "(64)/ ?", KEYCODE_SLASH)
	 DIPS_HELPER (0x0020, "(64)Right-Shift", KEYCODE_RSHIFT)
	 DIPS_HELPER (0x0010, "(64)CRSR-DOWN", KEYCODE_2_PAD)
	 DIPS_HELPER (0x0008, "(64)CRSR-RIGHT", KEYCODE_6_PAD)
	 DIPS_HELPER (0x0004, "(64)Space", KEYCODE_SPACE)
     PORT_START
     DIPS_HELPER (0x8000, "ESC", KEYCODE_ESC)
	 DIPS_HELPER (0x4000, "TAB", KEYCODE_F5)
	 DIPS_HELPER (0x2000, "ALT", KEYCODE_F6)
#if 1
	 PORT_DIPNAME (0x1000, 0x0000, "CAPSLOCK")
	 PORT_DIPSETTING (0x1000, DEF_STR( Off ) )
	 PORT_DIPSETTING (0, DEF_STR( On ) )
#else
	 PORT_BITX (0x1000, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"CAPSLOCK (switch)", KEYCODE_NONE, IP_JOY_NONE)
#endif
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
#if 1
	 PORT_DIPNAME (0x0400, 0x0000, "Display Select (not C64 Mode)(booting)")
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0400, "80 Columns (RGBI)")
#else
	 PORT_BITX (0x0200, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				KEYCODE_NONE, IP_JOY_NONE)
#endif
	 PORT_BITX (0x0100, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 DIPS_HELPER (0x0080, "Up", KEYCODE_NONE)
	 DIPS_HELPER (0x0040, "Down", KEYCODE_NONE)
	 DIPS_HELPER (0x0020, "Left", KEYCODE_NONE)
	 DIPS_HELPER (0x0010, "Right", KEYCODE_NONE)
	 DIPS_HELPER (0x0008, "f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

INPUT_PORTS_START (c128ger)
	 C64_DIPS
     C128_DIPS
	 PORT_START
     DIPS_HELPER (0x8000, "(64)_                    < >", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !	BLK   ORNG", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"  WHT   BRN", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #	RED   L RED       Paragraph", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $	CYN   D GREY", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %	PUR   GREY", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &	GRN   L GRN", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '  BLU   L BLU	      /", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (	YEL   L GREY", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )	RVS-ON", KEYCODE_9)
	 DIPS_HELPER (0x0020, "(64)0    RVS-OFF           =", KEYCODE_0)
	 DIPS_HELPER (0x0010, "(64)+                    Sharp-S ?",
				  KEYCODE_PLUS_PAD)
	 DIPS_HELPER (0x0008, "(64)-                    '  `",
				  KEYCODE_MINUS_PAD)
	 DIPS_HELPER (0x0004, "(64)\\                    [ Arrow-Up",
				  KEYCODE_MINUS)
	 DIPS_HELPER (0x0002, "(64)HOME CLR", KEYCODE_EQUALS)
	 DIPS_HELPER (0x0001, "(64)DEL INST", KEYCODE_BACKSPACE)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)CONTROL", KEYCODE_RCONTROL)
	 DIPS_HELPER (0x4000, "(64)Q", KEYCODE_Q)
	 DIPS_HELPER (0x2000, "(64)W", KEYCODE_W)
	 DIPS_HELPER (0x1000, "(64)E", KEYCODE_E)
	 DIPS_HELPER (0x0800, "(64)R", KEYCODE_R)
	 DIPS_HELPER (0x0400, "(64)T", KEYCODE_T)
	 DIPS_HELPER (0x0200, "(64)Y                    Z", KEYCODE_Y)
	 DIPS_HELPER (0x0100, "(64)U", KEYCODE_U)
	 DIPS_HELPER (0x0080, "(64)I", KEYCODE_I)
	 DIPS_HELPER (0x0040, "(64)O", KEYCODE_O)
	 DIPS_HELPER (0x0020, "(64)P", KEYCODE_P)
	 DIPS_HELPER (0x0010, "(64)Paragraph Arrow-Up   Diaresis-U",
				  KEYCODE_OPENBRACE)
	 DIPS_HELPER (0x0008, "(64)* `                  + *",
				  KEYCODE_ASTERISK)
	 DIPS_HELPER (0x0004, "(64)Sum Pi               ] \\",
				  KEYCODE_CLOSEBRACE)
	 DIPS_HELPER (0x0002, "(64)RESTORE", KEYCODE_PRTSCR)
	 DIPS_HELPER (0x0001, "(64)STOP RUN", KEYCODE_TAB)
	 PORT_START
     PORT_BITX (0x8000, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 DIPS_HELPER (0x4000, "(64)A", KEYCODE_A)
	 DIPS_HELPER (0x2000, "(64)S", KEYCODE_S)
	 DIPS_HELPER (0x1000, "(64)D", KEYCODE_D)
	 DIPS_HELPER (0x0800, "(64)F", KEYCODE_F)
	 DIPS_HELPER (0x0400, "(64)G", KEYCODE_G)
	 DIPS_HELPER (0x0200, "(64)H", KEYCODE_H)
	 DIPS_HELPER (0x0100, "(64)J", KEYCODE_J)
	 DIPS_HELPER (0x0080, "(64)K", KEYCODE_K)
	 DIPS_HELPER (0x0040, "(64)L", KEYCODE_L)
	 DIPS_HELPER (0x0020, "(64): [                  Diaresis-O",
				  KEYCODE_COLON)
	 DIPS_HELPER (0x0010, "(64); ]                  Diaresis-A",
				  KEYCODE_QUOTE)
	 DIPS_HELPER (0x0008, "(64)=                    # '",
				  KEYCODE_BACKSLASH)
	 DIPS_HELPER (0x0004, "(64)RETURN", KEYCODE_ENTER)
	 DIPS_HELPER (0x0002, "(64)CBM", KEYCODE_RALT)
	 DIPS_HELPER (0x0001, "(64)Left-Shift", KEYCODE_LSHIFT)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)Z                    Y", KEYCODE_Z)
	 DIPS_HELPER (0x4000, "(64)X", KEYCODE_X)
	 DIPS_HELPER (0x2000, "(64)C", KEYCODE_C)
	 DIPS_HELPER (0x1000, "(64)V", KEYCODE_V)
	 DIPS_HELPER (0x0800, "(64)B", KEYCODE_B)
	 DIPS_HELPER (0x0400, "(64)N", KEYCODE_N)
	 DIPS_HELPER (0x0200, "(64)M", KEYCODE_M)
	 DIPS_HELPER (0x0100, "(64), <                    ;", KEYCODE_COMMA)
	 DIPS_HELPER (0x0080, "(64). >                    :", KEYCODE_STOP)
	 DIPS_HELPER (0x0040, "(64)/ ?                  - _", KEYCODE_SLASH)
	 DIPS_HELPER (0x0020, "(64)Right-Shift", KEYCODE_RSHIFT)
	 DIPS_HELPER (0x0010, "(64)CRSR-DOWN", KEYCODE_2_PAD)
	 DIPS_HELPER (0x0008, "(64)CRSR-RIGHT", KEYCODE_6_PAD)
	 DIPS_HELPER (0x0004, "(64)Space", KEYCODE_SPACE)
     PORT_START
     DIPS_HELPER (0x8000, "ESC", KEYCODE_ESC)
	 DIPS_HELPER (0x4000, "TAB", KEYCODE_F5)
	 DIPS_HELPER (0x2000, "ALT", KEYCODE_F6)
#if 1
	 PORT_DIPNAME (0x1000, 0x0000, "Keyboard Mode")
	 PORT_DIPSETTING (0, "ASCII")
	 PORT_DIPSETTING (0x1000, "DIN")
#else
	 PORT_BITX (0x1000, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"ASCII DIN (switch)", KEYCODE_NONE, IP_JOY_NONE)
#endif
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
#if 1
	 PORT_DIPNAME (0x0400, 0x0000, "Display Select (not C64 Mode)(booting)")
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0400, "80 Columns (RGBI)")
#else
	 PORT_BITX (0x0200, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				KEYCODE_NONE, IP_JOY_NONE)
#endif
	 PORT_BITX (0x0100, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 DIPS_HELPER (0x0080, "Up", KEYCODE_NONE)
	 DIPS_HELPER (0x0040, "Down", KEYCODE_NONE)
	 DIPS_HELPER (0x0020, "Left", KEYCODE_NONE)
	 DIPS_HELPER (0x0010, "Right", KEYCODE_NONE)
	 DIPS_HELPER (0x0008, "f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

static void c128_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, vic2_palette, sizeof (vic2_palette));
}

#if 0
	 ROM_LOAD ("basic-4000.318018-02.bin", 0x40000, 0x4000, 0x2ee6e2fa)
	 ROM_LOAD ("basic-8000.318019-02.bin", 0x44000, 0x4000, 0xd551fce0)

	 ROM_LOAD ("basic.252343-03.bin", 0x40000, 0x8000, 0xbc07ed87)

	 ROM_LOAD ("basic-4000.318018-04.bin", 0x40000, 0x4000, 0x9f9c355b)
	 ROM_LOAD ("basic-8000.318019-04.bin", 0x44000, 0x4000, 0x6e2c91a7)

	 /* c64 basic, c64 kernel, editor, z80 bios, c128kernel */
	 ROM_LOAD ("64c.251913-01.bin", 0x48000, 0x4000, 0x0010ec31)
	 ROM_LOAD ("kernal.318020-05.bin", 0x4c000, 0x4000, 0xba456b8e)

	 ROM_LOAD ("complete.252343-04.bin", 0x48000, 0x8000, 0xcc6bdb69)
	 ROM_LOAD ("complete.german.318077-01.bin", 0x48000, 0x8000, 0xeb6e2c8f)

	 ROM_LOAD ("characters.390059-01.bin", 0x60000, 0x2000, 0x6aaaafe6)
	 ROM_LOAD ("characters.german.315079-01.bin", 0x60000, 0x2000, 0xfe5a2db1)
#endif

ROM_START (c128)
	 ROM_REGION (0x72800, REGION_CPU1)
#if 1
	 ROM_LOAD ("318018.04", 0x40000, 0x4000, 0x9f9c355b)
	 ROM_LOAD ("318019.04", 0x44000, 0x4000, 0x6e2c91a7)
	 ROM_LOAD ("251913.01", 0x48000, 0x4000, 0x0010ec31)
	 ROM_LOAD ("318020.05", 0x4c000, 0x4000, 0xba456b8e)
	 ROM_LOAD ("390059.01", 0x60000, 0x2000, 0x6aaaafe6)
#else
	 /* 128d  */
	 ROM_LOAD ("252343.03", 0x40000, 0x8000, 0xbc07ed87)
	 ROM_LOAD ("252343.04", 0x48000, 0x8000, 0xcc6bdb69)
	 ROM_LOAD ("390059.01", 0x60000, 0x2000, 0x6aaaafe6)
	 /*     ROM_LOAD("super_chip.bin", 0x50000, 0x8000, 0xa66f73c5) */
#endif
#ifdef C128_Z80
	 ROM_REGION (0x10000, REGION_CPU2)
#endif
#ifdef VC1541
	 VC1541_ROM (REGION_CPU2)
#endif
ROM_END

ROM_START (c128ger)
		 /* c128d german */
	 ROM_REGION (0x72800, REGION_CPU1)
	 ROM_LOAD ("252343.03", 0x40000, 0x8000, 0xbc07ed87)
	 ROM_LOAD ("318077.01", 0x48000, 0x8000, 0xeb6e2c8f)
	 ROM_LOAD ("315079.01", 0x60000, 0x2000, 0xfe5a2db1)
#ifdef C128_Z80
	 ROM_REGION (0x10000, REGION_CPU2)
#endif
#ifdef VC1541
	 VC1541_ROM (REGION_CPU2)
#endif
ROM_END

static struct MachineDriver machine_driver_c128 =
{
  /* basic machine hardware */
	{
#ifdef C128_Z80
		{
			CPU_Z80 | CPU_16BIT_PORT,
			VIC6569_CLOCK,
			c128_z80_readmem, c128_z80_writemem,
			c128_z80_readio, c128_z80_writeio,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
#endif
		{
			CPU_M6510,				   /* m8502 */
			VIC6567_CLOCK,
			c128_readmem, c128_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
#ifdef VC1541
		VC1541_CPU (REGION_CPU2)
#endif
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	c128_init_machine,
	c128_shutdown_machine,

	/* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c128_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static struct MachineDriver machine_driver_c128pal =
{
  /* basic machine hardware */
	{
#ifdef C128_Z80
		{
			CPU_Z80 | CPU_16BIT_PORT,
			VIC6569_CLOCK,
			c128_z80_readmem, c128_z80_writemem,
			c128_z80_readio, c128_z80_writeio,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
#endif
		{
			CPU_M6510,				   /* m8502 */
			VIC6569_CLOCK,
			c128_readmem, c128_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
#ifdef VC1541
		VC1541_CPU (REGION_CPU2)
#endif
	},
	VIC6569_VRETRACERATE,
	DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	0,
	c128_init_machine,
	c128_shutdown_machine,

	/* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c128_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static const struct IODevice io_c128[] =
{
	IODEVICE_CBM_QUICK,
#if 0
	IODEVICE_ROM_SOCKET,
#endif
	IODEVICE_CBM_ROM(c64_rom_id),
	IODEVICE_VC20TAPE,
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

#define init_c128 c128_driver_init
#define init_c128pal c128pal_driver_init
#define io_c128ger io_c128

/*	   YEAR  NAME	  PARENT MACHINE  INPUT    INIT 	COMPANY   FULLNAME */
COMPX( 1985, c128,	  0,	 c128,	  c128,    c128,	"Commodore Business Machines Co.", "Commodore C128 (NTSC)", GAME_NOT_WORKING | GAME_NO_SOUND )
COMPX( 1985, c128ger, c128,  c128pal, c128ger, c128pal, "Commodore Business Machines Co.", "Commodore C128 German (PAL)", GAME_NOT_WORKING | GAME_NO_SOUND )
