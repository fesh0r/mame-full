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
vdc emulation
 dirtybuffered video system
 text mode
  only standard 8x8 characters supported
 graphic mode not tested
 lightpen not supported
 scrolling not supported
z80 emulation
 floppy simulation not enough for booting CPM
 so simplified z80 memory management not tested
no cpu clock doubling
no internal function rom
c64 mode
 differences to real c64???
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
 (not working, cia timing, cpu timing?)
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
esc-x switch between two videosystems

additional keys (to c64) are in c64mode not useable

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

rom dumping
-----------

Dumping of the roms from the running machine:
in the monitor program
s "drive:name",device,start,end

s "0:basic",8,f4000,fc000
s "0:editor",8,fc000,fd000
s "0:kernel",8,ee000,f0000
s "0:char128",8,ed000,ee000

*z80bios missing (funet says only 1 version!?)
I don't know, maybe there is a cpm utility allowing saving
the memory area 0-0xfff of bank 0.
(I don't want to develope (and cant test) this short complicated
program)

*c64basic and kernel (only 1 version!?)
in c64 mode
poke43,0:poke44,160:poke45,0:poke46,192:save"0:basic64",8

in c64 mode
for i=0 to 8191:poke 32*256+i, peek(224*256+i): next
poke43,0:poke44,32:poke45,0:poke46,64:save"0:kernel64",8

*c64 charset (swedish version or original c64 version)
in c128 mode
monitor
a 2000 sei
lda #33
sta 1
ldy #0
sty fa
sty fc
lda #c0
sta fd
lda #d0
sta fb
ldx #10
lda (fa),y
sta (fc),y
iny
bne 2015
inc fb
inc fd
dex
bne 2015
lda #37
sta 1
cli
rts
(additional enter to end assembler input)
x (to leave monitor)
go64 (answer with y)
sys 32*256
poke 43,0:poke44,192:poke45,0:poke46,208:save"0:char64",8

or in c64 mode
load the program in the attachment
load"savechar64",8,1
sys 32*256
poke 43,0:poke44,192:poke45,0:poke46,208:save"0:char64",8

c128d floppy disk bios:
I think you have to download a program
copying the bios to puffers.
Then you could read this buffer into the computer, or write
these buffers to disk.

Transportation to your pc:
1571 writing to mfm encoded disketts (in cpm mode only, or use program)
 maybe the IBM CPM-86 formats are like the standard DOS formats.
 but using dd may create images known by some other emulators.
1581 writes mfm encoded:
 can one of these drives to a format know by linux?
Some years ago I build a simple adapter pc/parport to vc1541
 floppy disk drive.

Dumping roms with epromer
-------------------------
c128
U18       (read compatible 2764?) 8kB c64 character rom, c128 character rom
U32 23128 (read compatible 27128?) 16kB c64 Basic, c64 Kernel
U33 23128 (read compatible 27128?) 16kB c128 Basic at 0x4000
U34 23128 (read compatible 27128?) 16kB c128 Basic at 0x8000
U35 23128 (read compatible 27128?) 16kB c128 Editor, Z80Bios, C128 Kernel
c128 cost reduced
U18       (read compatible 2764?) 8kB c64 character rom, c128 character rom
U32 23256 (read compatible 27256?) 32kB c64 Basic, c64 Kernel, c128 Editor, Z80Bios, C128 Kernel
U34 23256 (read compatible 27256?) 32kB C128 Basic
c128dcr
as c128 cr
U102 23256 (read compatible 27256?) 32kB 1571 system rom

*/

#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/cia6526.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/vidhrdw/vdc8563.h"
#include "mess/sndhrdw/sid6581.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"

#include "mess/machine/c128.h"

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
 * 0xe000-0xffff ram as bank 0
 */
static struct MemoryReadAddress c128_z80_readmem[] =
{
#if 1
	{0x0000, 0x0fff, MRA_BANK10},
	{0x1000, 0xbfff, MRA_BANK11},
	{0xc000, 0xffff, MRA_RAM},
#else
	/* best to do reuse bankswitching numbers */
	{0x0000, 0x03ff, MRA_BANK10},
	{0x0400, 0x0fff, MRA_BANK11},
	{0x1000, 0x1fff, MRA_BANK3},
	{0x2000, 0x3fff, MRA_BANK4},

	{0x4000, 0xbfff, MRA_BANK5},
	{0xc000, 0xdfff, MRA_BANK6},
	{0xe000, 0xefff, MRA_BANK7},
	{0xf000, 0xfeff, MRA_BANK8},
	{0xff00, 0xff04, c128_mmu8722_ff00_r},
	{0xff05, 0xffff, MRA_BANK9},
#endif
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress c128_z80_writemem[] =
{
#if 1
	{0x0000, 0x0fff, c128_write_0000, &c64_memory},
	{0x1000, 0xbfff, c128_write_1000 },
	{0xc000, 0xffff, MWA_RAM },
#else
	{0x0000, 0x03ff, MWA_BANK1, &c64_memory},
	{0x0400, 0x0fff, MWA_BANK2},
	{0x1000, 0x1fff, MWA_BANK3},
	{0x2000, 0x3fff, MWA_BANK4},
	{0x4000, 0xbfff, MWA_BANK5},
	{0xc000, 0xdfff, MWA_BANK6},
	{0xe000, 0xefff, MWA_BANK7},
	{0xf000, 0xfeff, MWA_BANK8},
	{0xff00, 0xff04, c128_mmu8722_ff00_w},
	{0xff05, 0xffff, MWA_BANK9},
#endif
	{0x10000, 0x1ffff, MWA_RAM},
	{0x20000, 0xfffff, MWA_RAM},	   /* or nothing */
	{0x100000, 0x107fff, MWA_ROM, &c128_basic},	/* maps to 0x4000 */
	{0x108000, 0x109fff, MWA_ROM, &c64_basic},	/* maps to 0xa000 */
	{0x10a000, 0x10bfff, MWA_ROM, &c64_kernal},	/* maps to 0xe000 */
	{0x10c000, 0x10cfff, MWA_ROM, &c128_editor},
	{0x10d000, 0x10dfff, MWA_ROM, &c128_z80},		/* maps to z80 0 */
	{0x10e000, 0x10ffff, MWA_ROM, &c128_kernal},
	{0x110000, 0x117fff, MWA_ROM, &c128_internal_function},
	{0x118000, 0x11ffff, MWA_ROM, &c128_external_function},
	{0x120000, 0x120fff, MWA_ROM, &c64_chargen},
	{0x121000, 0x121fff, MWA_ROM, &c128_chargen},
	{0x122000, 0x1227ff, MWA_RAM, &c64_colorram},
	{0x122800, 0x1327ff, MWA_RAM, &c128_vdcram},
	/* 2 kbyte by 8 bits, only 1 kbyte by 4 bits used) */
	{-1}							   /* end of table */
};

static struct IOReadPort c128_z80_readio[] =
{
	{0x1000, 0x13ff, c64_colorram_read},
	{0xd000, 0xd3ff, vic2_port_r},
	{0xd400, 0xd4ff, sid6581_0_port_r},
	{0xd500, 0xd5ff, c128_mmu8722_port_r},
	{0xd600, 0xd7ff, vdc8563_port_r},
	{0xdc00, 0xdcff, cia6526_0_port_r},
	{0xdd00, 0xddff, cia6526_1_port_r},
	/*{ 0xdf00, 0xdfff, dma_port_r }, */
	{-1}							   /* end of table */
};

static struct IOWritePort c128_z80_writeio[] =
{
	{0x1000, 0x13ff, c64_colorram_write},
	{0xd000, 0xd3ff, vic2_port_w},
	{0xd400, 0xd4ff, sid6581_0_port_w},
	{0xd500, 0xd5ff, c128_mmu8722_port_w},
	{0xd600, 0xd7ff, vdc8563_port_w},
	{0xdc00, 0xdcff, cia6526_0_port_w},
	{0xdd00, 0xddff, cia6526_1_port_w},
	/*{ 0xdf00, 0xdfff, dma_port_w }, */
	{-1}							   /* end of table */
};

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
	{0x0000, 0x0001, c64_m6510_port_w},
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
	{-1}							   /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
    PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, CODE_NONE)

#define C128_DIPS \
     PORT_START \
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_SLASH_PAD)\
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )\
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )\
   DIPS_HELPER( 0x1000, "Tape Drive Play",       CODE_DEFAULT)\
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     CODE_DEFAULT)\
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       CODE_DEFAULT)\
	 PORT_DIPNAME   ( 0x300, 0x00, "Main Memory/MMU Version")\
	 PORT_DIPSETTING(  0, "128 KByte" )\
	 PORT_DIPSETTING(0x100, "256 KByte" )\
	 PORT_DIPSETTING(0x200, "1024 KByte" )\
	PORT_DIPNAME   ( 0x80, 0x80, "Sid Chip Type")\
	PORT_DIPSETTING(  0, "MOS6581" )\
	PORT_DIPSETTING(0x80, "MOS8580" )\
	 PORT_DIPNAME   ( 0x40, 0x40, "VDC Memory (RGBI)")\
	 PORT_DIPSETTING(  0, "16 KByte" )\
	 PORT_DIPSETTING(  0x40, "64 KByte" )\
	 PORT_BITX (0x20, 0x20, IPT_DIPSWITCH_NAME|IPF_TOGGLE,\
				"DIN,TV/RGBI Monitor (switch)",\
				KEYCODE_ENTER_PAD, IP_JOY_NONE)\
	 PORT_DIPSETTING(  0, "DIN,TV" )\
	 PORT_DIPSETTING(  0x20, "RGBI" )\
	 PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")\
	 PORT_DIPSETTING (0, "Automatic")\
	 PORT_DIPSETTING (4, "Ultimax (GAME)")\
	 PORT_DIPSETTING (8, "C64 (EXROM)")\
	 /*PORT_DIPSETTING (0x10, "C64 CBM Supergames")*/\
	 /*PORT_DIPSETTING (0x14, "C64 Ocean Robocop2")*/\
	 /*PORT_DIPSETTING (0x1c, "C128")*/\
	 PORT_DIPNAME (0x02, 0x02, "Serial Bus/Device 8")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (2, "VC1541 Floppy Drive")\
	 PORT_DIPNAME (0x01, 0x01, "Serial Bus/Device 9")\
	 PORT_DIPSETTING (0, "None")\
	 PORT_DIPSETTING (1, "VC1541 Floppy Drive")

#define DIPS_KEYS_BOTH \
	PORT_START \
    DIPS_HELPER( 0x8000, "Numericblock 7", CODE_DEFAULT)\
	DIPS_HELPER( 0x4000, "Numericblock 8", CODE_DEFAULT)\
	DIPS_HELPER( 0x2000, "Numericblock 9", CODE_DEFAULT)\
	DIPS_HELPER( 0x1000, "Numericblock +", CODE_DEFAULT)\
	DIPS_HELPER( 0x0800, "Numericblock 4", CODE_DEFAULT)\
	DIPS_HELPER( 0x0400, "Numericblock 5", CODE_DEFAULT)\
	DIPS_HELPER( 0x0200, "Numericblock 6", CODE_DEFAULT)\
	DIPS_HELPER( 0x0100, "Numericblock -", CODE_DEFAULT)\
	DIPS_HELPER( 0x0080, "Numericblock 1", CODE_DEFAULT)\
	DIPS_HELPER( 0x0040, "Numericblock 2", CODE_DEFAULT)\
	DIPS_HELPER( 0x0020, "Numericblock 3", CODE_DEFAULT)\
	DIPS_HELPER( 0x0010, "Numericblock 0", CODE_DEFAULT)\
	DIPS_HELPER( 0x0008, "Numericblock .", CODE_DEFAULT)\
	DIPS_HELPER( 0x0004, "Numericblock Enter", CODE_DEFAULT)\
	DIPS_HELPER( 0x0002, "Special (Right-Shift CRSR-DOWN) CRSR-UP", \
			 KEYCODE_8_PAD)\
	DIPS_HELPER( 0x0001, "Special (Right-Shift CRSR-RIGHT) CRSR-LEFT", \
			 KEYCODE_4_PAD)\

INPUT_PORTS_START (c128)
	 C64_DIPS
	 C128_DIPS
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)Arrow-Left", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !  BLK   ORNG", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"  WHT   BRN", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #  RED   L RED", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $  CYN   D GREY", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %  PUR   GREY", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &  GRN   L GRN", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '  BLU   L BLU", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (  YEL   L GREY", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )  RVS-ON", KEYCODE_9)
	 DIPS_HELPER (0x0020, "(64)0    RVS-OFF", KEYCODE_0)
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
     PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 PORT_DIPSETTING (0, DEF_STR( Off ) )
	 PORT_DIPSETTING (0x8000, DEF_STR( On ) )
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
	 PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"CAPSLOCK (switch)", CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, DEF_STR( Off ) )
	 PORT_DIPSETTING (0x1000, DEF_STR( On ) )
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
	 PORT_BITX (0x0200, 0x200, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0200, "80 Columns (RGBI)")
	 PORT_BITX (0x0100, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 PORT_DIPSETTING (0, DEF_STR( Off ) )
	 PORT_DIPSETTING (0x100, DEF_STR( On ) )
	 DIPS_HELPER (0x0080, "Up", CODE_DEFAULT)
	 DIPS_HELPER (0x0040, "Down", CODE_DEFAULT)
	 DIPS_HELPER (0x0020, "Left", CODE_DEFAULT)
	 DIPS_HELPER (0x0010, "Right", CODE_DEFAULT)
	 DIPS_HELPER (0x0008, "(64)f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "(64)f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "(64)f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "(64)f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

INPUT_PORTS_START (c128ger)
	 C64_DIPS
	 C128_DIPS
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)_                    < >", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !  BLK   ORNG", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"  WHT   BRN", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #  RED   L RED       Paragraph", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $  CYN   D GREY", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %  PUR   GREY", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &  GRN   L GRN", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '  BLU   L BLU	      /", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (  YEL   L GREY", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )  RVS-ON", KEYCODE_9)
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
     PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x8000, DEF_STR( On ) )
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
	 PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"ASCII DIN (switch)", CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "ASCII")
	 PORT_DIPSETTING (0x1000, "DIN")
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
	 PORT_BITX (0x0200, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0200, "80 Columns (RGBI)")
	 PORT_BITX (0x0100, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x100, DEF_STR( On ) )
	 DIPS_HELPER (0x0080, "Up", CODE_DEFAULT)
	 DIPS_HELPER (0x0040, "Down", CODE_DEFAULT)
	 DIPS_HELPER (0x0020, "Left", CODE_DEFAULT)
	 DIPS_HELPER (0x0010, "Right", CODE_DEFAULT)
	 DIPS_HELPER (0x0008, "(64)f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "(64)f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "(64)f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "(64)f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

INPUT_PORTS_START (c128fra)
	 C64_DIPS
	 C128_DIPS
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)_                    < >", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !  BLK   ORNG      & 1", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"  WHT   BRN       Acute-e 2", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #  RED   L RED     \" 3", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $  CYN   D GREY    ' 4", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %  PUR   GREY      ( 5", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &  GRN   L GRN     Paragraph 6", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '  BLU   L BLU     Grave-e 7", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (  YEL   L GREY    ! 8", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )  RVS-ON          Cedilla-c 9", KEYCODE_9)
	 DIPS_HELPER (0x0020, "(64)0    RVS-OFF         Grave-a 0", KEYCODE_0)
	 DIPS_HELPER (0x0010, "(64)+ Diaresis-e         ) Overcircle",
				  KEYCODE_PLUS_PAD)
	 DIPS_HELPER (0x0008, "(64)- Overcircle         - _",
				  KEYCODE_MINUS_PAD)
	 DIPS_HELPER (0x0004, "(64)\\                    At #",
				  KEYCODE_MINUS)
	 DIPS_HELPER (0x0002, "(64)HOME CLR", KEYCODE_EQUALS)
	 DIPS_HELPER (0x0001, "(64)DEL INST", KEYCODE_BACKSPACE)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)CONTROL", KEYCODE_RCONTROL)
	 DIPS_HELPER (0x4000, "(64)Q                    A", KEYCODE_Q)
	 DIPS_HELPER (0x2000, "(64)W                    Z", KEYCODE_W)
	 DIPS_HELPER (0x1000, "(64)E", KEYCODE_E)
	 DIPS_HELPER (0x0800, "(64)R", KEYCODE_R)
	 DIPS_HELPER (0x0400, "(64)T", KEYCODE_T)
	 DIPS_HELPER (0x0200, "(64)Y", KEYCODE_Y)
	 DIPS_HELPER (0x0100, "(64)U", KEYCODE_U)
	 DIPS_HELPER (0x0080, "(64)I", KEYCODE_I)
	 DIPS_HELPER (0x0040, "(64)O", KEYCODE_O)
	 DIPS_HELPER (0x0020, "(64)P", KEYCODE_P)
	 DIPS_HELPER (0x0010, "(64)At Circumflex-u      Circumflex Diaresis",
				  KEYCODE_OPENBRACE)
	 DIPS_HELPER (0x0008, "(64)* `                  $ [",
				  KEYCODE_ASTERISK)
	 DIPS_HELPER (0x0004, "(64)Arrow-Up Pi          * ]",
				  KEYCODE_CLOSEBRACE)
	 DIPS_HELPER (0x0002, "(64)RESTORE", KEYCODE_PRTSCR)
	 DIPS_HELPER (0x0001, "(64)STOP RUN", KEYCODE_TAB)
	 PORT_START
     PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x8000, DEF_STR( On ) )
	 DIPS_HELPER (0x4000, "(64)A                    Q", KEYCODE_A)
	 DIPS_HELPER (0x2000, "(64)S", KEYCODE_S)
	 DIPS_HELPER (0x1000, "(64)D", KEYCODE_D)
	 DIPS_HELPER (0x0800, "(64)F", KEYCODE_F)
	 DIPS_HELPER (0x0400, "(64)G", KEYCODE_G)
	 DIPS_HELPER (0x0200, "(64)H", KEYCODE_H)
	 DIPS_HELPER (0x0100, "(64)J", KEYCODE_J)
	 DIPS_HELPER (0x0080, "(64)K Large-\\", KEYCODE_K)
	 DIPS_HELPER (0x0040, "(64)L", KEYCODE_L)
	 DIPS_HELPER (0x0020, "(64): [                  M Large-/",
				  KEYCODE_COLON)
	 DIPS_HELPER (0x0010, "(64); ]                  Grave-u %",
				  KEYCODE_QUOTE)
	 DIPS_HELPER (0x0008, "(64)=                    Arrow-Up \\",
				  KEYCODE_BACKSLASH)
	 DIPS_HELPER (0x0004, "(64)RETURN", KEYCODE_ENTER)
	 DIPS_HELPER (0x0002, "(64)CBM", KEYCODE_RALT)
	 DIPS_HELPER (0x0001, "(64)Left-Shift", KEYCODE_LSHIFT)
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)Z                    W", KEYCODE_Z)
	 DIPS_HELPER (0x4000, "(64)X", KEYCODE_X)
	 DIPS_HELPER (0x2000, "(64)C", KEYCODE_C)
	 DIPS_HELPER (0x1000, "(64)V", KEYCODE_V)
	 DIPS_HELPER (0x0800, "(64)B", KEYCODE_B)
	 DIPS_HELPER (0x0400, "(64)N", KEYCODE_N)
	 DIPS_HELPER (0x0200, "(64)M Large-/            , ?", KEYCODE_M)
	 DIPS_HELPER (0x0100, "(64), <                  ; .", KEYCODE_COMMA)
	 DIPS_HELPER (0x0080, "(64). >                  : /", KEYCODE_STOP)
	 DIPS_HELPER (0x0040, "(64)/ ?                  = +", KEYCODE_SLASH)
	 DIPS_HELPER (0x0020, "(64)Right-Shift", KEYCODE_RSHIFT)
	 DIPS_HELPER (0x0010, "(64)CRSR-DOWN", KEYCODE_2_PAD)
	 DIPS_HELPER (0x0008, "(64)CRSR-RIGHT", KEYCODE_6_PAD)
	 DIPS_HELPER (0x0004, "(64)Space", KEYCODE_SPACE)
     PORT_START
	 DIPS_HELPER (0x8000, "ESC", KEYCODE_ESC)
	 DIPS_HELPER (0x4000, "TAB", KEYCODE_F5)
	 DIPS_HELPER (0x2000, "ALT", KEYCODE_F6)
	 PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"ASCII ?French? (switch)", CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "ASCII")
	 PORT_DIPSETTING (0x1000, "?French?")
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
	 PORT_BITX (0x0200, 0x200, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0200, "80 Columns (RGBI)")
	 PORT_BITX (0x0100, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x100, DEF_STR( On ) )
	 DIPS_HELPER (0x0080, "Up", CODE_DEFAULT)
	 DIPS_HELPER (0x0040, "Down", CODE_DEFAULT)
	 DIPS_HELPER (0x0020, "Left", CODE_DEFAULT)
	 DIPS_HELPER (0x0010, "Right", CODE_DEFAULT)
	 DIPS_HELPER (0x0008, "(64)f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "(64)f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "(64)f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "(64)f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

INPUT_PORTS_START (c128ita)
	 C64_DIPS
	 C128_DIPS
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)_                    < >", KEYCODE_TILDE)
	 DIPS_HELPER (0x4000, "(64)1 !  BLK   ORNG      Pound 1", KEYCODE_1)
	 DIPS_HELPER (0x2000, "(64)2 \"  WHT   BRN       Acute-e 2", KEYCODE_2)
	 DIPS_HELPER (0x1000, "(64)3 #  RED   L RED     \" 3", KEYCODE_3)
	 DIPS_HELPER (0x0800, "(64)4 $  CYN   D GREY    ' 4", KEYCODE_4)
	 DIPS_HELPER (0x0400, "(64)5 %  PUR   GREY      ( 5", KEYCODE_5)
	 DIPS_HELPER (0x0200, "(64)6 &  GRN   L GRN     _ 6", KEYCODE_6)
	 DIPS_HELPER (0x0100, "(64)7 '  BLU   L BLU     Grave-e 7", KEYCODE_7)
	 DIPS_HELPER (0x0080, "(64)8 (  YEL   L GREY    & 8", KEYCODE_8)
	 DIPS_HELPER (0x0040, "(64)9 )  RVS-ON          Cedilla-c 9", KEYCODE_9)
	 DIPS_HELPER (0x0020, "(64)0    RVS-OFF         Grave-a 0", KEYCODE_0)
	 DIPS_HELPER (0x0010, "(64)+ Diaresis-e         ) Overcircle",
				  KEYCODE_PLUS_PAD)
	 DIPS_HELPER (0x0008, "(64)- Overcircle         - +",
				  KEYCODE_MINUS_PAD)
	 DIPS_HELPER (0x0004, "(64)\\                    At #",
				  KEYCODE_MINUS)
	 DIPS_HELPER (0x0002, "(64)HOME CLR", KEYCODE_EQUALS)
	 DIPS_HELPER (0x0001, "(64)DEL INST", KEYCODE_BACKSPACE)
	 PORT_START
     DIPS_HELPER (0x8000, "(64)CONTROL", KEYCODE_RCONTROL)
	 DIPS_HELPER (0x4000, "(64)Q", KEYCODE_Q)
	 DIPS_HELPER (0x2000, "(64)W                    Z", KEYCODE_W)
	 DIPS_HELPER (0x1000, "(64)E", KEYCODE_E)
	 DIPS_HELPER (0x0800, "(64)R", KEYCODE_R)
	 DIPS_HELPER (0x0400, "(64)T", KEYCODE_T)
	 DIPS_HELPER (0x0200, "(64)Y", KEYCODE_Y)
	 DIPS_HELPER (0x0100, "(64)U", KEYCODE_U)
	 DIPS_HELPER (0x0080, "(64)I", KEYCODE_I)
	 DIPS_HELPER (0x0040, "(64)O", KEYCODE_O)
	 DIPS_HELPER (0x0020, "(64)P", KEYCODE_P)
	 DIPS_HELPER (0x0010, "(64)At Circumflex-u      Grave-i =",
				  KEYCODE_OPENBRACE)
	 DIPS_HELPER (0x0008, "(64)* `                  $ [",
				  KEYCODE_ASTERISK)
	 DIPS_HELPER (0x0004, "(64)Arrow-Up Pi          * ]",
				  KEYCODE_CLOSEBRACE)
	 DIPS_HELPER (0x0002, "(64)RESTORE", KEYCODE_PRTSCR)
	 DIPS_HELPER (0x0001, "(64)STOP RUN", KEYCODE_TAB)
	 PORT_START
     PORT_BITX (0x8000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"(64)(Left-Shift)SHIFT-LOCK (switch)",
				KEYCODE_CAPSLOCK, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x8000, DEF_STR( On ) )
	 DIPS_HELPER (0x4000, "(64)A", KEYCODE_A)
	 DIPS_HELPER (0x2000, "(64)S", KEYCODE_S)
	 DIPS_HELPER (0x1000, "(64)D", KEYCODE_D)
	 DIPS_HELPER (0x0800, "(64)F", KEYCODE_F)
	 DIPS_HELPER (0x0400, "(64)G", KEYCODE_G)
	 DIPS_HELPER (0x0200, "(64)H", KEYCODE_H)
	 DIPS_HELPER (0x0100, "(64)J", KEYCODE_J)
	 DIPS_HELPER (0x0080, "(64)K", KEYCODE_K)
	 DIPS_HELPER (0x0040, "(64)L", KEYCODE_L)
	 DIPS_HELPER (0x0020, "(64): [                  M",
				  KEYCODE_COLON)
	 DIPS_HELPER (0x0010, "(64); ]                  Grave-u %",
				  KEYCODE_QUOTE)
	 DIPS_HELPER (0x0008, "(64)=                    Arrow-Up \\",
				  KEYCODE_BACKSLASH)
	 DIPS_HELPER (0x0004, "(64)RETURN", KEYCODE_ENTER)
	 DIPS_HELPER (0x0002, "(64)CBM", KEYCODE_RALT)
	 DIPS_HELPER (0x0001, "(64)Left-Shift", KEYCODE_LSHIFT)
	 PORT_START
	 DIPS_HELPER (0x8000, "(64)Z                    W", KEYCODE_Z)
	 DIPS_HELPER (0x4000, "(64)X", KEYCODE_X)
	 DIPS_HELPER (0x2000, "(64)C", KEYCODE_C)
	 DIPS_HELPER (0x1000, "(64)V", KEYCODE_V)
	 DIPS_HELPER (0x0800, "(64)B", KEYCODE_B)
	 DIPS_HELPER (0x0400, "(64)N", KEYCODE_N)
	 DIPS_HELPER (0x0200, "(64)M Large-/            , ?", KEYCODE_M)
	 DIPS_HELPER (0x0100, "(64), <                  ; .", KEYCODE_COMMA)
	 DIPS_HELPER (0x0080, "(64). >                  : /", KEYCODE_STOP)
	 DIPS_HELPER (0x0040, "(64)/ ?                  Grave-o !", KEYCODE_SLASH)
	 DIPS_HELPER (0x0020, "(64)Right-Shift", KEYCODE_RSHIFT)
	 DIPS_HELPER (0x0010, "(64)CRSR-DOWN", KEYCODE_2_PAD)
	 DIPS_HELPER (0x0008, "(64)CRSR-RIGHT", KEYCODE_6_PAD)
	 DIPS_HELPER (0x0004, "(64)Space", KEYCODE_SPACE)
     PORT_START
	 DIPS_HELPER (0x8000, "ESC", KEYCODE_ESC)
	 DIPS_HELPER (0x4000, "TAB", KEYCODE_F5)
	 DIPS_HELPER (0x2000, "ALT", KEYCODE_F6)
	 PORT_BITX (0x1000, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"Capslock (switch)", CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "ASCII")
	 PORT_DIPSETTING (0x1000, "Italian")
	 DIPS_HELPER (0x0800, "HELP", KEYCODE_F7)
	 DIPS_HELPER (0x0400, "LINE FEED", KEYCODE_F8)
	 PORT_BITX (0x0200, 0x200, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
				"40 80 Display (switch)(booting)",
				CODE_DEFAULT, IP_JOY_NONE)
	 PORT_DIPSETTING (0, "40 Columns (DIN/TV)")
	 PORT_DIPSETTING (0x0200, "80 Columns (RGBI)")
	 PORT_BITX (0x0100, IP_ACTIVE_HIGH, IPF_TOGGLE,
				"NO SCROLL (switch)", KEYCODE_F9, IP_JOY_NONE)
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x100, DEF_STR( On ) )
	 DIPS_HELPER (0x0080, "Up", CODE_DEFAULT)
	 DIPS_HELPER (0x0040, "Down", CODE_DEFAULT)
	 DIPS_HELPER (0x0020, "Left", CODE_DEFAULT)
	 DIPS_HELPER (0x0010, "Right", CODE_DEFAULT)
	 DIPS_HELPER (0x0008, "(64)f1 f2", KEYCODE_F1)
	 DIPS_HELPER (0x0004, "(64)f3 f4", KEYCODE_F2)
	 DIPS_HELPER (0x0002, "(64)f5 f6", KEYCODE_F3)
	 DIPS_HELPER (0x0001, "(64)f7 f8", KEYCODE_F4)
	 DIPS_KEYS_BOTH
INPUT_PORTS_END

static void c128_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, vic2_palette, sizeof (vic2_palette));
	memcpy (sys_palette+sizeof(vic2_palette), vdc8563_palette, sizeof (vdc8563_palette));
}

#if 0
/* usa first 318018-02 318019-02 318020-03 rev 0*/

/* usa mid 318018-03 318019-03 318020-04 */

/* usa last 318018-04 318019-04 318020-05 rev 1*/
/* usa dcr 318022-02 318023-02, same as usa last */

/* between rev0 and rev1 252343-03+252343-04 */

	 ROM_LOAD ("basic-4000.318018-02.bin", 0x100000, 0x4000, 0x2ee6e2fa)
	 ROM_LOAD ("basic-8000.318019-02.bin", 0x104000, 0x4000, 0xd551fce0)
     /* same as above, but in one chip */
     ROM_LOAD ("basic.318022-01.bin", 0x100000, 0x8000, 0xe857df90)

	/* maybe 318018-03+318019-03 */
	 ROM_LOAD ("basic.252343-03.bin", 0x100000, 0x8000, 0xbc07ed87)

	/* 1986 final upgrade */
	 ROM_LOAD ("basic-4000.318018-04.bin", 0x100000, 0x4000, 0x9f9c355b)
	 ROM_LOAD ("basic-8000.318019-04.bin", 0x104000, 0x4000, 0x6e2c91a7)
     /* same as above, but in one chip */
	 ROM_LOAD ("basic.318022-02.bin", 0x100000, 0x8000, 0xaf1ae1e8)

	 ROM_LOAD ("64c.251913-01.bin", 0x108000, 0x4000, 0x0010ec31)

	 /* editor, z80 bios, c128kernel */
	 ROM_LOAD ("kernal.318020-03.bin", 0x10c000, 0x4000, 0x1e94bb02)
	 ROM_LOAD ("kernal.318020-05.bin", 0x10c000, 0x4000, 0xba456b8e)
	 ROM_LOAD ("kernal.german.315078-01.bin", 0x10c000, 0x4000, 0xa51e2168)
	 ROM_LOAD ("kernal.german.315078-02.bin", 0x10c000, 0x4000, 0xb275bb2e)
	 /* 0x3e086a24 z80bios 0xca5e1179*/
	 ROM_LOAD ("french.bin", 0x10c000, 0x4000, 0x2df282b8)
	 /* 0x71002a97 z80bios 0x167b8364*/
	 ROM_LOAD ("finnish1.bin", 0x10c000, 0x4000, 0xd3ecea84)
	 /* 0xb7ff5efe z80bios 0x5ce42fc8 */
	 ROM_LOAD ("finnish2.bin", 0x10c000, 0x4000, 0x9526fac4)
	/* 0x8df58148 z80bios 0x7b0d2140 */
	 ROM_LOAD ("italian.bin", 0x10c000, 0x4000, 0x74d6b084)
	 /* 0x84c55911 z80bios 0x3ba48012 */
	 ROM_LOAD ("norwegian.bin", 0x10c000, 0x4000, 0xa5406848)

	 /* c64 basic, c64 kernel, editor, z80 bios, c128kernel */
	/* 252913-01+318020-05 */
	 ROM_LOAD ("complete.318023-02.bin", 0x100000, 0x8000, 0xeedc120a)
	/* 252913-01+0x98f2a2ed maybe 318020-04*/
	 ROM_LOAD ("complete.252343-04.bin", 0x108000, 0x8000, 0xcc6bdb69)
	/* 251913-01+0xbff7550b */
	 ROM_LOAD ("complete.german.318077-01.bin", 0x108000, 0x8000, 0xeb6e2c8f)
     /* chip label says Ker.Sw/Fi  */
	/* 901226.01+ 0xf10c2c25 +0x1cf7f729 */
	 ROM_LOAD ("complete.swedish.318034-01.bin", 0x108000, 0x8000, 0xcb4e1719)

	 ROM_LOAD ("characters.390059-01.bin", 0x120000, 0x2000, 0x6aaaafe6)
	 ROM_LOAD ("characters.german.315079-01.bin", 0x120000, 0x2000, 0xfe5a2db1)
	 /* chip label says I/F/B (belgium, italian, french)  characters */
     /* italian and french verified to be the same*/
	 ROM_LOAD ("characters.french.325167-01.bin", 0x120000, 0x2000, 0xbad36b88)

	 /* only parts of system roms, so not found in any c128 variant */
	 ROM_LOAD ("editor.finnish1.bin", 0x10c000, 0x1000, 0x71002a97)
	 ROM_LOAD ("editor.finnish2.bin", 0x10c000, 0x1000, 0xb7ff5efe)
	 ROM_LOAD ("editor.french.bin", 0x10c000, 0x1000, 0x3e086a24)
	 ROM_LOAD ("editor.italian.bin", 0x10c000, 0x1000, 0x8df58148)
	 ROM_LOAD ("editor.norwegian.bin", 0x10c000, 0x1000, 0x84c55911)

	 ROM_LOAD ("kernalpart.finnish1.bin", 0x10e000, 0x2000, 0x167b8364)
	 ROM_LOAD ("kernalpart.finnish2.bin", 0x10e000, 0x2000, 0x5ce42fc8)
	 ROM_LOAD ("kernalpart.french.bin", 0x10e000, 0x2000, 0xca5e1179)
	 ROM_LOAD ("kernalpart.italian.bin", 0x10e000, 0x2000, 0x7b0d2140)
	 ROM_LOAD ("kernalpart.norwegian.bin", 0x10e000, 0x2000, 0x3ba48012)

	 ROM_LOAD ("z80bios.bin", 0x10d000, 0x1000, 0xc38d83c6)

	 /* function rom in internal socket */
	 ROM_LOAD("super_chip.bin", 0x110000, 0x8000, 0xa66f73c5)
#endif

ROM_START (c128)
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, 0x9f9c355b)
	ROM_LOAD ("318019.04", 0x104000, 0x4000, 0x6e2c91a7)
	ROM_LOAD ("251913.01", 0x108000, 0x4000, 0x0010ec31)
	ROM_LOAD ("318020.05", 0x10c000, 0x4000, 0xba456b8e)
	ROM_LOAD ("390059.01", 0x120000, 0x2000, 0x6aaaafe6)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

ROM_START (c128d)
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, 0xaf1ae1e8)
	ROM_LOAD ("318023.02", 0x100000, 0x8000, 0xeedc120a)
	ROM_LOAD ("390059.01", 0x120000, 0x2000, 0x6aaaafe6)
	ROM_REGION (0x10000, REGION_CPU2)
	C1571_ROM(REGION_CPU3)
ROM_END

ROM_START (c128ger)
	 /* c128d german */
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, 0xaf1ae1e8)
	ROM_LOAD ("318077.01", 0x108000, 0x8000, 0xeb6e2c8f)
	ROM_LOAD ("315079.01", 0x120000, 0x2000, 0xfe5a2db1)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

ROM_START (c128fra)
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, 0x9f9c355b)
	ROM_LOAD ("318019.04", 0x104000, 0x4000, 0x6e2c91a7)
	ROM_LOAD ("251913.01", 0x108000, 0x4000, 0x0010ec31)
#if 1
	ROM_LOAD ("french.bin", 0x10c000, 0x4000, 0x2df282b8)
#else
	ROM_LOAD ("editor.french.bin", 0x10c000, 0x1000, 0x3e086a24)
	ROM_LOAD ("z80bios.bin", 0x10d000, 0x1000, 0xc38d83c6)
	ROM_LOAD ("kernalpart.french.bin", 0x10e000, 0x2000, 0xca5e1179)
#endif
	ROM_LOAD ("325167.01", 0x120000, 0x2000, 0xbad36b88)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

ROM_START (c128ita)
	ROM_REGION (0x132800, REGION_CPU1)
	/* original 318022-01 */
	ROM_LOAD ("318018.04", 0x100000, 0x4000, 0x9f9c355b)
	ROM_LOAD ("318019.04", 0x104000, 0x4000, 0x6e2c91a7)
	ROM_LOAD ("251913.01", 0x108000, 0x4000, 0x0010ec31)
	ROM_LOAD ("italian.bin", 0x10c000, 0x4000, 0x74d6b084)
	ROM_LOAD ("325167.01", 0x120000, 0x2000, 0xbad36b88)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

ROM_START (c128swe)
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, 0xaf1ae1e8)
	ROM_LOAD ("318034.01", 0x108000, 0x8000, 0xcb4e1719)
	/* vic64s doubled */
	ROM_LOAD ("char.swe", 0x120000, 0x2000, 0x22d4d095)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

ROM_START (c128nor)
	ROM_REGION (0x132800, REGION_CPU1)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, 0x9f9c355b)
	ROM_LOAD ("318019.04", 0x104000, 0x4000, 0x6e2c91a7)
	ROM_LOAD ("251913.01", 0x108000, 0x4000, 0x0010ec31)
	ROM_LOAD ("nor.bin", 0x10c000, 0x4000, 0xa5406848)
	/* standard c64, vic20 based norwegian */
	ROM_LOAD ("char.nor", 0x120000, 0x2000, 0xba95c625)
	ROM_REGION (0x10000, REGION_CPU2)
ROM_END

static struct MachineDriver machine_driver_c128 =
{
  /* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			VIC6567_CLOCK,
			c128_z80_readmem, c128_z80_writemem,
			c128_z80_readio, c128_z80_writeio,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
		{
			CPU_M8502,
			VIC6567_CLOCK,
			c128_readmem, c128_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	c128_init_machine,
	c128_shutdown_machine,

	/* video hardware */
	656,							   /* screen width */
	216,							   /* screen height */
	{0, 656 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	(sizeof (vic2_palette) +sizeof(vdc8563_palette))
	 / sizeof (vic2_palette[0]) / 3,
	0,
	c128_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	c128_vh_start,
	c128_vh_stop,
	c128_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid6581_sound_interface },
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static struct MachineDriver machine_driver_c128d =
{
  /* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			VIC6567_CLOCK,
			c128_z80_readmem, c128_z80_writemem,
			c128_z80_readio, c128_z80_writeio,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
		{
			CPU_M8502,
			VIC6567_CLOCK,
			c128_readmem, c128_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
		C1571_CPU
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	c128_init_machine,
	c128_shutdown_machine,

	/* video hardware */
	656,							   /* screen width */
	216,							   /* screen height */
	{0, 656 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	(sizeof (vic2_palette) +sizeof(vdc8563_palette))
	 / sizeof (vic2_palette[0]) / 3,
	0,
	c128_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	c128_vh_start,
	c128_vh_stop,
	c128_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid6581_sound_interface },
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static struct MachineDriver machine_driver_c128pal =
{
  /* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			VIC6569_CLOCK,
			c128_z80_readmem, c128_z80_writemem,
			c128_z80_readio, c128_z80_writeio,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
		{
			CPU_M8502,
			VIC6569_CLOCK,
			c128_readmem, c128_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			c128_raster_irq, VIC2_HRETRACERATE,
		},
	},
	VIC6569_VRETRACERATE,
	DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	0,
	c128_init_machine,
	c128_shutdown_machine,

	/* video hardware */
	656,							   /* screen width */
	216,							   /* screen height */
	{0, 656 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	(sizeof (vic2_palette) +sizeof(vdc8563_palette))
	 / sizeof (vic2_palette[0]) / 3,
	0,
	c128_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	c128_vh_start,
	c128_vh_stop,
	c128_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid6581_sound_interface },
		{SOUND_DAC, &vc20tape_sound_interface}
	}
};

static const struct IODevice io_c128[] =
{
	IODEVICE_CBM_QUICK,
#if 0
	IODEVICE_ROM_SOCKET,
	IODEVICE_CBM_ROM("crt\080\0", c64_rom_id),
	IODEVICE_VC20TAPE,
#endif
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

static const struct IODevice io_c128d[] =
{
	IODEVICE_CBM_QUICK,
#if 0
	IODEVICE_ROM_SOCKET,
	IODEVICE_CBM_ROM(c64_rom_id),
	IODEVICE_VC20TAPE,
#endif
	IODEVICE_C1571,
	{IO_END}
};

#define init_c128 c128_driver_init
#define init_c128pal c128pal_driver_init
#define io_c128ger io_c128
#define io_c128fra io_c128
#define io_c128ita io_c128
#define io_c128swe io_c128
#define io_c128nor io_c128

/*	  YEAR	NAME		PARENT	MACHINE 	INPUT		INIT		COMPANY   FULLNAME */
COMPX (1985, c128,		0,		c128,		c128,		c128,		"Commodore Business Machines Co.","Commodore C128 NTSC",      GAME_IMPERFECT_SOUND)
COMPX (1985, c128ger,	c128,	c128pal,	c128ger,	c128pal,	"Commodore Business Machines Co.","Commodore C128 German (PAL)",GAME_IMPERFECT_SOUND)
COMPX (1985, c128fra,	c128,	c128pal,	c128fra,	c128pal,	"Commodore Business Machines Co.","Commodore C128 French (PAL)",GAME_IMPERFECT_SOUND)
COMPX (1985, c128ita,	c128,	c128pal,	c128ita,	c128pal,	"Commodore Business Machines Co.","Commodore C128 Italian (PAL)",GAME_IMPERFECT_SOUND)
/* other countries spanish, belgium, finnish, swedish, norwegian */
// please leave the following as testdriver
COMP (1985, c128swe,	c128,	c128pal,	c128ita,	c128pal,	"Commodore Business Machines Co.","Commodore C128 Swedish (PAL)")
COMP (1985, c128nor,	c128,	c128pal,	c128ita,	c128pal,	"Commodore Business Machines Co.","Commodore C128 Norwegian (PAL)")
COMP (1985, c128d,		0,		c128d,		c128,		c128,		"Commodore Business Machines Co.","Commodore C128D NTSC")
