/***************************************************************************

	commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/

/*
------------------------------------
c64	commodore c64 (ntsc version)
c64pal	commodore c64 (pal version)
c64gs   commodore c64 game system (ntsc version)
sx64    commodore sx64 (pal version)
max     commodore max (vic10/ultimax/vickie prototype)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!

c64
 design like the vic20
 better videochip with sprites
 famous sid6581 sound chip
 64 kbyte ram
 2nd gameport
max  (vic10,ultimax,vickey prototype)
 delivered in japan only?
 (all modules should work with c64)
 !cartridges neccessary (nothing works without cartridge)
 low cost c64
 flat design
 only 4 kbyte sram
 simplier banking chip
  no portlines from cpu
 only 1 cia6526 chip
  restore key connection?
  no serial bus
  no userport
 keyboard
 tape port
 2 gameports
  lightpen (port a only) and joystick mentioned in advertisement
  paddles
 cartridge/expansion port (some signals different to c64)
 no rom on board (minibasic with kernel delivered as cartridge?)
c64gs
 game console without keyboard
 modified kernal
 basic rom
 keyboard connector?
 tapeport?, 
 2. cia?
 userport?, 
 cbm serial port?, 
4064/pet64/educator64
 build in monitor ?
 differences, versions???
(sx64 prototype with build in black/white monitor)
sx64
 build in vc1541
 build in small color monitor
 no tape connector
dx64 prototype
 two build in vc1541 (or 2 drives driven by one vc1541 circuit)
 
state
-----
rasterline based video system
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
simple tape support (not working, cia timing?)
serial bus
 simple disk drives
 no printer or other devices
expansion modules c64
 rom cartridges (exrom)
 ultimax rom cartridges (game)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
expansion modules ultimax
 ultimax rom cartridges
 no other expansion modules
no userport
 no rs232/v.24 interface
no super cpu modification
no second sid modification
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
loads first rom when you press quickload key (f8)

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

#include "mess/machine/c64.h"

static struct MemoryReadAddress ultimax_readmem[] =
{
	{0x0000, 0x0001, c64_m6510_port_r},
	{0x0002, 0x0fff, MRA_RAM},
	{0x8000, 0x9fff, MRA_ROM},
	{0xd000, 0xd3ff, vic2_port_r},
	{0xd400, 0xd7ff, sid6581_0_port_r},
	{0xd800, 0xdbff, MRA_RAM},		   /* colorram  */
	{0xdc00, 0xdcff, cia6526_0_port_r},
	{0xe000, 0xffff, MRA_ROM},		   /* ram or kernel rom */
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress ultimax_writemem[] =
{
	{0x0000, 0x0001, c64_m6510_port_w, &c64_memory},
	{0x0002, 0x0fff, MWA_RAM},
	{0x8000, 0x9fff, MWA_ROM, &c64_roml},
	{0xd000, 0xd3ff, vic2_port_w},
	{0xd400, 0xd7ff, sid6581_0_port_w},
	{0xd800, 0xdbff, c64_colorram_write, &c64_colorram},
	{0xdc00, 0xdcff, cia6526_0_port_w},
	{0xe000, 0xffff, MWA_ROM, &c64_romh},
	{-1}							   /* end of table */
};

static struct MemoryReadAddress c64_readmem[] =
{
	{0x0000, 0x0001, c64_m6510_port_r},
	{0x0002, 0x7fff, MRA_RAM},
	{0x8000, 0x9fff, MRA_BANK1},	   /* ram or external roml */
	{0xa000, 0xbfff, MRA_BANK3},	   /* ram or basic rom or external romh */
	{0xc000, 0xcfff, MRA_RAM},
#if 1
	{0xd000, 0xdfff, MRA_BANK5},
#else
/* dram */
/* or character rom */
	{0xd000, 0xd3ff, vic2_port_r},
	{0xd400, 0xd7ff, sid6581_0_port_r},
	{0xd800, 0xdbff, MRA_RAM},		   /* colorram  */
	{0xdc00, 0xdcff, cia6526_0_port_r},
	{0xdd00, 0xddff, cia6526_1_port_r},
	{0xde00, 0xdeff, MRA_NOP},		   /* csline expansion port */
	{0xdf00, 0xdfff, MRA_NOP},		   /* csline expansion port */
#endif
	{0xe000, 0xffff, MRA_BANK7},	   /* ram or kernel rom or external romh */
	{0x10000, 0x11fff, MRA_ROM},	   /* basic at 0xa000 */
	{0x12000, 0x13fff, MRA_ROM},	   /* kernal at 0xe000 */
	{0x14000, 0x14fff, MRA_ROM},	   /* charrom at 0xd000 */
	{0x15000, 0x153ff, MRA_RAM},	   /* colorram at 0xd800 */
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress c64_writemem[] =
{
	{0x0000, 0x0001, c64_m6510_port_w, &c64_memory},
	{0x0002, 0x7fff, MWA_RAM},
	{0x8000, 0x9fff, MWA_BANK2},
	{0xa000, 0xcfff, MWA_RAM},
#if 1
	{0xd000, 0xdfff, MWA_BANK6},
#else
	/* or dram memory */
	{0xd000, 0xd3ff, vic2_port_w},
	{0xd400, 0xd7ff, sid6581_0_port_w},
	{0xd800, 0xdbff, c64_colorram_write},
	{0xdc00, 0xdcff, cia6526_0_port_w},
	{0xdd00, 0xddff, cia6526_1_port_w},
	{0xde00, 0xdeff, MWA_NOP},		   /* csline expansion port */
	{0xdf00, 0xdfff, MWA_NOP},		   /* csline expansion port */
#endif
	{0xe000, 0xffff, MWA_BANK8},
	{0x10000, 0x11fff, MWA_ROM, &c64_basic},	/* basic at 0xa000 */
	{0x12000, 0x13fff, MWA_ROM, &c64_kernal},	/* kernal at 0xe000 */
	{0x14000, 0x14fff, MWA_ROM, &c64_chargen},	/* charrom at 0xd000 */
	{0x15000, 0x153ff, MWA_RAM, &c64_colorram},		/* colorram at 0xd800 */
	{-1}							   /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define C64_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "Arrow-Left", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "1 !   BLK   ORNG", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 \"   WHT   BRN", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #   RED   L RED", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $   CYN   D GREY", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %   PUR   GREY", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 &   GRN   L GRN", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 '   BLU   L BLU", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 (   YEL   L GREY", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 )   RVS-ON", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0     RVS-OFF", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "+", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0008, "-", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x0004, "Pound", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0002, "HOME CLR", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0001, "DEL INST", KEYCODE_BACKSPACE)\
	PORT_START \
	DIPS_HELPER( 0x8000, "CTRL", KEYCODE_RCONTROL)\
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0010, "At", KEYCODE_OPENBRACE)\
        DIPS_HELPER( 0x0008, "*", KEYCODE_ASTERISK)\
        DIPS_HELPER( 0x0004, "Arrow-Up Pi",KEYCODE_CLOSEBRACE)\
        DIPS_HELPER( 0x0002, "RESTORE", KEYCODE_PRTSCR)\
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_TAB)\
	PORT_START \
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPF_TOGGLE,\
		     "SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE)\
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)\
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)\
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)\
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)\
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0020, ": [", KEYCODE_COLON)\
	DIPS_HELPER( 0x0010, "; ]", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0008, "=", KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)\
	DIPS_HELPER( 0x0002, "CBM", KEYCODE_RALT)\
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)\
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)\
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)\
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)\
	DIPS_HELPER( 0x0100, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x0080, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0010, "CRSR-DOWN UP", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0008, "CRSR-RIGHT LEFT", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0002, "f1 f2", KEYCODE_F1)\
	DIPS_HELPER( 0x0001, "f3 f4", KEYCODE_F2)\
	PORT_START \
	DIPS_HELPER( 0x8000, "f5 f6", KEYCODE_F3)\
	DIPS_HELPER( 0x4000, "f7 f8", KEYCODE_F4)\
	DIPS_HELPER( 0x2000, "(Right-Shift Cursor-Down)Special CRSR Up", \
				 KEYCODE_8_PAD)\
	DIPS_HELPER( 0x1000, "(Right-Shift Cursor-Right)Special CRSR Left", \
				 KEYCODE_4_PAD)

INPUT_PORTS_START (ultimax)
     C64_DIPS
    PORT_START
	DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)\
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)\
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)\
     PORT_BIT (0x1c, 0x4, IPT_UNUSED)	   /* only ultimax cartridges */
	 PORT_BIT (0x2, 0x0, IPT_UNUSED)		   /* no serial bus */
	 PORT_BIT (0x1, 0x0, IPT_UNUSED)
     C64_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START (c64gs)
     C64_DIPS
     PORT_START 
	 PORT_BIT (0xff00, 0x0, IPT_UNUSED)
     PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
	 PORT_BIT (0x2, 0x0, IPT_UNUSED)		   /* no serial bus */
	 PORT_BIT (0x1, 0x0, IPT_UNUSED)
	 PORT_START /* no keyboard */
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xf000, 0x0, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START (c64)
     C64_DIPS
     PORT_START 
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
     PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
	 PORT_DIPNAME (0x02, 0x02, "Serial Bus/Device 8")
	 PORT_DIPSETTING (0, "None")
	 PORT_DIPSETTING (2, "VC1541 Floppy Drive")
	 PORT_DIPNAME (0x01, 0x01, "Serial Bus/Device 9")
	 PORT_DIPSETTING (0, "None")
	 PORT_DIPSETTING (1, "VC1541 Floppy Drive")
     C64_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START (sx64)
     C64_DIPS
     PORT_START 
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	 PORT_BIT (0x7f00, 0x0, IPT_UNUSED) /* no tape */
     PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
	 /* 1 vc1541 build in, device number selectable 8,9,10,11 */
	 PORT_DIPNAME (0x02, 0x02, "Serial Bus/Device 8")
	 PORT_DIPSETTING (0, "None")
	 PORT_DIPSETTING (2, "VC1541 Floppy Drive")
	 PORT_DIPNAME (0x01, 0x01, "Serial Bus/Device 9")
	 PORT_DIPSETTING (0, "None")
	 PORT_DIPSETTING (1, "VC1541 Floppy Drive")
     C64_KEYBOARD
INPUT_PORTS_END

static void c64_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, vic2_palette, sizeof (vic2_palette));
}

ROM_START (ultimax)
	 ROM_REGION (0x10000, REGION_CPU1)
ROM_END

ROM_START (c64gs)
	 ROM_REGION (0x15400, REGION_CPU1)
	 /* standard basic, modified kernel */
	 ROM_LOAD ("c64gs.rom",     0x10000, 0x4000, 0xb0a9c2da)
	 ROM_LOAD ("char.do",       0x14000, 0x1000, 0xec4272ee)
ROM_END

ROM_START (c64)
	 ROM_REGION (0x15400, REGION_CPU1)
	 ROM_LOAD ("basic.a0",      0x10000, 0x2000, 0xf833d117)
	 ROM_LOAD ("kernel3.e0",    0x12000, 0x2000, 0xdbe3e7c7)
	 ROM_LOAD ("char.do",       0x14000, 0x1000, 0xec4272ee)
#ifdef VC1541
	 VC1541_ROM (REGION_CPU2)
#endif
ROM_END

ROM_START (c64pal)
	 ROM_REGION (0x15400, REGION_CPU1)
	 ROM_LOAD ("basic.a0",      0x10000, 0x2000, 0xf833d117)
	 ROM_LOAD ("kernel3.e0",    0x12000, 0x2000, 0xdbe3e7c7)
	 ROM_LOAD ("char.do",       0x14000, 0x1000, 0xec4272ee)
#ifdef VC1541
	 VC1541_ROM (REGION_CPU2)
#endif
ROM_END

ROM_START (sx64)
	 ROM_REGION (0x15400, REGION_CPU1)
	 ROM_LOAD ("basic.a0",      0x10000, 0x2000, 0xf833d117)
	 ROM_LOAD( "sx64.e0",       0x12000, 0x2000, 0x2c5965d4 )
	 ROM_LOAD ("char.do",       0x14000, 0x1000, 0xec4272ee)
#ifdef VC1541
	 VC1541_ROM (REGION_CPU2)
#endif
ROM_END

#if 0
	 /* scrap */
     /* modified for alec 64, not booting */
	 ROM_LOAD( "alec64.e0",     0x12000, 0x2000, 0x2b1b7381 )
     /* unique copyright, else speeddos? */
	 ROM_LOAD( "a.e0", 0x12000, 0x2000, 0xb8f49365 )   
	 /* ? */
	 ROM_LOAD( "kernelx.e0",    0x12000, 0x2000, 0xbeed6d49 )
	 ROM_LOAD( "kernelx2.e0",   0x12000, 0x2000, 0xcfb58230 )
	 /* basic x 2 */
	 ROM_LOAD( "frodo.e0",      0x12000, 0x2000, 0x6ec94629 )

     /* commodore versions */
     /* 901227-01 */
	 ROM_LOAD( "kernel1.e0",    0x12000, 0x2000, 0xdce782fa )
     /* 901227-02 */
	 ROM_LOAD( "kernel2.e0",    0x12000, 0x2000, 0xa5c687b3 )
     /* 901227-03 */
	 ROM_LOAD( "kernel3.e0",    0x12000, 0x2000, 0xdbe3e7c7 )
	 /* basic and 901227-03 */
	 ROM_LOAD ("64c.251913-01.bin", 0x10000, 0x4000, 0x0010ec31)
	 /* sx64 251104-04 */
	 ROM_LOAD( "sx64.e0",       0x12000, 0x2000, 0x2c5965d4 )
	 /* 4064, Pet64, Educator 64 */
	 ROM_LOAD( "4064.e0",       0x12000, 0x2000, 0x789c8cc5 )
	 
	 /* not few differences to above versions */
	 ROM_LOAD( "kernel2b.e0",   0x12000, 0x2000, 0xf80eb87b )
	 ROM_LOAD( "kernel3b.e0",   0x12000, 0x2000, 0x8e5c500d )
	 ROM_LOAD( "kernel3c.e0",   0x12000, 0x2000, 0xc13310c2 )

     /* 64er system v1 
        ieee interface extension for c64 and vc1541!? */
	 ROM_LOAD( "64ersys1.e0",   0x12000, 0x2000, 0x97d9a4df )
	 /* 64er system v3 */
	 ROM_LOAD( "64ersys3.e0",   0x12000, 0x2000, 0x5096b3bd )

	 /* exos v3 */
	 ROM_LOAD( "exosv3.e0",     0x12000, 0x2000, 0x4e54d020 )
     /* 2 bytes different */
	 ROM_LOAD( "exosv3.e0",     0x12000, 0x2000, 0x26f3339e )

	 /* jiffydos v6.01 by cmd */
	 ROM_LOAD( "jiffy.e0",      0x12000, 0x2000, 0x2f79984c )

	 /* dolphin with dolphin vc1541 */
	 ROM_LOAD( "mager.e0",      0x12000, 0x2000, 0xc9bb21bc )
	 ROM_LOAD( "dos20.e0",      0x12000, 0x2000, 0xffaeb9bc )

	 /* speeddos plus
		parallel interface on userport to modified vc1541 !? */
	 ROM_LOAD( "speeddos.e0",   0x12000, 0x2000, 0x8438e77b )
	 /* speeddos plus + */
	 ROM_LOAD( "speeddos.e0",   0x12000, 0x2000, 0x10aee0ae )
	 /* speeddos plus and 80 column text */
	 ROM_LOAD( "rom80.e0",      0x12000, 0x2000, 0xe801dadc )
#endif

static struct MachineDriver machine_driver_c64 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6510,
			VIC6567_CLOCK,
			c64_readmem, c64_writemem,
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
	c64_init_machine,
	c64_shutdown_machine,

  /* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c64_init_palette,				   /* convert color prom */
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

static struct MachineDriver machine_driver_c64pal =
{
  /* basic machine hardware */
	{
		{
			CPU_M6510,
			VIC6569_CLOCK,
			c64_readmem, c64_writemem,
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
	c64_init_machine,
	c64_shutdown_machine,

  /* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c64_init_palette,				   /* convert color prom */
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

static struct MachineDriver machine_driver_c64_notape =
{
  /* basic machine hardware */
	{
		{
			CPU_M6510,
			VIC6567_CLOCK,
			c64_readmem, c64_writemem,
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
	c64_init_machine,
	c64_shutdown_machine,

  /* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c64_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
		{ 0 }
	}
};

static struct MachineDriver machine_driver_c64pal_notape =
{
  /* basic machine hardware */
	{
		{
			CPU_M6510,
			VIC6569_CLOCK,
			c64_readmem, c64_writemem,
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
	c64_init_machine,
	c64_shutdown_machine,

  /* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c64_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
		{ 0 }
	}
};

static struct MachineDriver machine_driver_ultimax =
{
  /* basic machine hardware */
	{
		{
			CPU_M6510,
			VIC6567_CLOCK,
			ultimax_readmem, ultimax_writemem,
			0, 0,
			c64_frame_interrupt, 1,
			vic2_raster_irq, VIC2_HRETRACERATE,
		}
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	c64_init_machine,
	c64_shutdown_machine,

  /* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	c64_init_palette,				   /* convert color prom */
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

static const struct IODevice io_c64[] =
{
	IODEVICE_CBM_QUICK,
	IODEVICE_CBM_ROM(c64_rom_id),
	IODEVICE_VC20TAPE,
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

static const struct IODevice io_sx64[] =
{
	IODEVICE_CBM_QUICK,
	IODEVICE_CBM_ROM(c64_rom_id),
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

static const struct IODevice io_ultimax[] =
{
	IODEVICE_CBM_QUICK,
	IODEVICE_CBM_ROM(c64_rom_id),
	IODEVICE_VC20TAPE,
	{IO_END}
};

static const struct IODevice io_c64gs[] =
{
	IODEVICE_CBM_ROM(c64_rom_id),
	{IO_END}
};

#define init_c64 c64_driver_init
#define init_c64pal c64pal_driver_init
#define init_ultimax ultimax_driver_init
#define init_sx64 sx64_driver_init
#define init_c64gs c64gs_driver_init

#define io_c64pal io_c64
#define io_max io_ultimax
#define rom_max rom_ultimax

/*	   YEAR  NAME	 PARENT MACHINE 	   INPUT	INIT	 COMPANY   FULLNAME */
COMPX( 1982, c64,	 0, 	c64,		   c64, 	c64,	 "Commodore Business Machines Co.", "Commodore C64 (NTSC)", GAME_NOT_WORKING | GAME_NO_SOUND )
COMPX( 1982, c64pal, c64,	c64pal, 	   c64, 	c64pal,  "Commodore Business Machines Co.", "Commodore C64 (PAL)", GAME_NOT_WORKING | GAME_NO_SOUND )
COMPX( 1983, sx64,	 c64,	c64pal_notape, sx64,	sx64,	 "Commodore Business Machines Co.", "Commodore SX64 (PAL)", GAME_NOT_WORKING | GAME_NO_SOUND )
CONSX( 1987, c64gs,  c64,	c64_notape,    c64gs,	c64gs,	 "Commodore Business Machines Co.", "Commodore C64GS (NTSC)", GAME_NOT_WORKING | GAME_NO_SOUND )
COMPX( 1982, max,	 c64,	ultimax,	   ultimax, ultimax, "Commodore Business Machines Co.", "Commodore Max (VIC10/Ultimax/Vickie)", GAME_NOT_WORKING | GAME_NO_SOUND )

