/***************************************************************************
TI-85 and TI-86 drivers by Krzysztof Strzecha

Notes:
1. After start TI-85 waits for ON key interrupt, so press ON key to start
   calculator.
2. Only difference beetwen all TI-85 drivers is ROM version.
3. TI-86 is TI-85 with more RAM and ROM.
4. Only difference beetwen all TI-86 drivers is ROM version.
5. Video engine (with grayscale support) based on the idea found in VTI source
   emulator written by Rusty Wagner.
6. NVRAM is saved properly only when calculator is turned off before exiting MESS.
7. To receive data from TI press "R" immediately after TI starts to send data.
8. To request screen dump from calculator press "S".
9. TI-81 have not serial link.

Needed:
1. Info about ports 3 (bit 2 seems to be allways 0) and 4.
2. Any info on TI-81 hardware.
3. ROM dumps of unemulated models.
4. Artworks.

New:
05/10/2002 TI-85 serial link works again.
17/09/2002 TI-85 snapshots loading fixed. Few code cleanups.
	   TI-86 SNAPSHOT LOADING DOESNT WORK.
	   TI-85, TI-86 SERIAL LINK DOESNT WORK.
08/09/2001 TI-81, TI-85, TI-86 modified to new core.
	   TI-81, TI-85, TI-86 reset corrected.
21/08/2001 TI-81, TI-85, TI-86 NVRAM corrected.
20/08/2001 TI-81 ON/OFF fixed.
	   TI-81 ROM bank switching added (port 5).
	   TI-81 NVRAM support added.
15/08/2001 TI-81 kayboard is now mapped as it should be.
14/08/2001 TI-81 preliminary driver added.
05/07/2001 Serial communication corrected (transmission works now after reset).
02/07/2001 Many source cleanups.
	   PCR added.
01/07/2001 Possibility to request screen dump from TI (received dumps are saved
	   as t85i file).
29/06/2001 Received variables can be saved now.
19/06/2001 Possibility to receive variables from calculator (they are nor saved
	   yet).
17/06/2001 TI-86 reset fixed.
15/06/2001 Possibility to receive memory backups from calculator.
07/06/2001 TI-85 reset fixed.
	   Work on receiving data from calculator started.
04/06/2001 TI-85 is able to receive variables and memory backups.
14/05/2001 Many source cleanups.
11/05/2001 Release years corrected. Work on serial link started.
26/04/2001 NVRAM support added.
25/04/2001 Video engine totaly rewriten so grayscale works now.
17/04/2001 TI-86 snapshots loading added.
	   ti86grom driver added.
16/04/2001 Sound added.
	   Five TI-86 drivers added (all features of TI-85 drivers without
	   snapshot loading).
13/04/2001 Snapshot loading (VTI 2.0 save state files).
18/02/2001 Palette (not perfect).
	   Contrast control (port 2) implemented.
	   LCD ON/OFF implemented (port 3).
	   Interrupts corrected (port 3) - ON/OFF and APD works now.
	   Artwork added.
09/02/2001 Keypad added.
	   200Hz timer interrupts implemented.
	   ON key and its interrupts implemented.
	   Calculator is now fully usable.
02/02/2001 Preliminary driver

To do:
- port 7 (TI-86)
- port 4 (all models)
- artwork (all models)
- add TI-82, TI-83 and TI-83+ drivers


TI-81 memory map

	CPU: Z80 2MHz
		0000-7fff ROM
		8000-ffff RAM (?)

TI-85 memory map

	CPU: Z80 6MHz
		0000-3fff ROM 0
		4000-7fff ROM 1-7 (switched)
		8000-ffff RAM

TI-86 memory map

	CPU: Z80 6MHz
		0000-3fff ROM 0
		4000-7fff ROM 0-15 or RAM 0-7 (switched)
		7000-bfff ROM 0-15 or RAM 0-7 (switched)
		c000-ffff RAM 0
		
Interrupts:

	IRQ: 200Hz timer
	     ON key	

TI-81 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Video buffer width, interrupt control (write only)
	5: ?
	6: 
	7: ?

TI-85 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Video buffer width, interrupt control (write only)
	5: Memory page
	6: Power mode
	7: Link

TI-86 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Power mode
	5: Memory page
	6: Memory page
	7: Link

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/ti85.h"
#include "devices/snapquik.h"

/* port i/o functions */

ADDRESS_MAP_START( ti81_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_READ( ti85_port_0000_r)
	AM_RANGE(0x0001, 0x0001) AM_READ( ti85_port_0001_r)
	AM_RANGE(0x0002, 0x0002) AM_READ( ti85_port_0002_r)
	AM_RANGE(0x0003, 0x0003) AM_READ( ti85_port_0003_r)
	AM_RANGE(0x0004, 0x0004) AM_READ( ti85_port_0004_r)
	AM_RANGE(0x0005, 0x0005) AM_READ( ti85_port_0005_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti81_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_WRITE( ti85_port_0000_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE( ti85_port_0001_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE( ti85_port_0002_w)
	AM_RANGE(0x0003, 0x0003) AM_WRITE( ti85_port_0003_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE( ti85_port_0004_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE( ti85_port_0005_w)
	AM_RANGE(0x0007, 0x0007) AM_WRITE( ti81_port_0007_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_READ( ti85_port_0000_r)
	AM_RANGE(0x0001, 0x0001) AM_READ( ti85_port_0001_r)
	AM_RANGE(0x0002, 0x0002) AM_READ( ti85_port_0002_r)
	AM_RANGE(0x0003, 0x0003) AM_READ( ti85_port_0003_r)
	AM_RANGE(0x0004, 0x0004) AM_READ( ti85_port_0004_r)
	AM_RANGE(0x0005, 0x0005) AM_READ( ti85_port_0005_r)
	AM_RANGE(0x0006, 0x0006) AM_READ( ti85_port_0006_r)
	AM_RANGE(0x0007, 0x0007) AM_READ( ti85_port_0007_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_WRITE( ti85_port_0000_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE( ti85_port_0001_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE( ti85_port_0002_w)
	AM_RANGE(0x0003, 0x0003) AM_WRITE( ti85_port_0003_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE( ti85_port_0004_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE( ti85_port_0005_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE( ti85_port_0006_w)
	AM_RANGE(0x0007, 0x0007) AM_WRITE( ti85_port_0007_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_READ( ti85_port_0000_r)
	AM_RANGE(0x0001, 0x0001) AM_READ( ti85_port_0001_r)
	AM_RANGE(0x0002, 0x0002) AM_READ( ti85_port_0002_r)
	AM_RANGE(0x0003, 0x0003) AM_READ( ti85_port_0003_r)
	AM_RANGE(0x0004, 0x0004) AM_READ( ti85_port_0006_r)
	AM_RANGE(0x0005, 0x0005) AM_READ( ti86_port_0005_r)
	AM_RANGE(0x0006, 0x0006) AM_READ( ti86_port_0006_r)
	AM_RANGE(0x0007, 0x0007) AM_READ( ti85_port_0007_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0000) AM_WRITE( ti85_port_0000_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE( ti85_port_0001_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE( ti85_port_0002_w)
	AM_RANGE(0x0003, 0x0003) AM_WRITE( ti85_port_0003_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE( ti85_port_0006_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE( ti86_port_0005_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE( ti86_port_0006_w)
	AM_RANGE(0x0007, 0x0007) AM_WRITE( ti85_port_0007_w)
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( ti81_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ( MRA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READ( MRA8_BANK2)
	AM_RANGE(0x8000, 0xffff) AM_READ( MRA8_RAM)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti81_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE( MWA8_BANK3)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( MWA8_BANK4)
	AM_RANGE(0x8000, 0xffff) AM_WRITE( MWA8_RAM)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ( MRA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READ( MRA8_BANK2)
	AM_RANGE(0x8000, 0xffff) AM_READ( MRA8_RAM)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE( MWA8_BANK3)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( MWA8_BANK4)
	AM_RANGE(0x8000, 0xffff) AM_WRITE( MWA8_RAM)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ( MRA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READ( MRA8_BANK2)
	AM_RANGE(0x8000, 0xbfff) AM_READ( MRA8_BANK3)
	AM_RANGE(0xc000, 0xffff) AM_READ( MRA8_BANK4)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE( MWA8_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( MWA8_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( MWA8_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_WRITE( MWA8_BANK8)
ADDRESS_MAP_END
        
/* keyboard input */
INPUT_PORTS_START (ti81)
	PORT_START   /* bit 0 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down"  , KEYCODE_DOWN,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER" , KEYCODE_ENTER,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(-)"   , KEYCODE_M,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "."     , KEYCODE_STOP,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0"     , KEYCODE_0,          IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "GRAPH" , KEYCODE_F5,         IP_JOY_NONE )
	PORT_START   /* bit 1 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left"  , KEYCODE_LEFT,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+"     , KEYCODE_EQUALS,     IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3"     , KEYCODE_3,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2"     , KEYCODE_2,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1"     , KEYCODE_1,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STORE" , KEYCODE_TAB,        IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TRACE" , KEYCODE_F4,         IP_JOY_NONE )
	PORT_START   /* bit 2 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Right" , KEYCODE_RIGHT,      IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-"     , KEYCODE_MINUS,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6"     , KEYCODE_6,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5"     , KEYCODE_5,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4"     , KEYCODE_4,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LN"    , KEYCODE_BACKSLASH,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ZOOM"  , KEYCODE_F3,         IP_JOY_NONE )
	PORT_START   /* bit 3 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up"    , KEYCODE_UP,         IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "*"     , KEYCODE_L,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9"     , KEYCODE_9,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8"     , KEYCODE_8,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7"     , KEYCODE_7,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LOG"   , KEYCODE_QUOTE,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RANGE" , KEYCODE_F2,         IP_JOY_NONE )
	PORT_START   /* bit 4 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/"     , KEYCODE_SLASH,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ")"     , KEYCODE_CLOSEBRACE, IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "("     , KEYCODE_OPENBRACE,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EE"    , KEYCODE_END,        IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x^2"   , KEYCODE_COLON,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y="    , KEYCODE_F1,         IP_JOY_NONE )
	PORT_START   /* bit 5 */                                                        
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^"     , KEYCODE_P,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAN"   , KEYCODE_PGUP,       IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "COS"   , KEYCODE_HOME,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SIN"   , KEYCODE_INSERT,     IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x^-1"  , KEYCODE_COMMA,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2nd"   , KEYCODE_LALT,       IP_JOY_NONE )
	PORT_START   /* bit 6 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CLEAR" , KEYCODE_PGDN,       IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "VARS"  , KEYCODE_F9,         IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PRGM"  , KEYCODE_F8,         IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MATRX" , KEYCODE_F7,         IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MATH"  , KEYCODE_F6,         IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "INS"   , KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_START   /* bit 7 */
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MODE"  , KEYCODE_ESC,        IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X|T"   , KEYCODE_X,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALPHA" , KEYCODE_CAPSLOCK,   IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL"   , KEYCODE_DEL,        IP_JOY_NONE )
	PORT_START   /* ON */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ON/OFF", KEYCODE_Q,          IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START (ti85)
	PORT_START   /* bit 0 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down"  , KEYCODE_DOWN,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER" , KEYCODE_ENTER,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(-)"   , KEYCODE_M,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "."     , KEYCODE_STOP,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0"     , KEYCODE_0,          IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F5"    , KEYCODE_F5,         IP_JOY_NONE )
	PORT_START   /* bit 1 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left"  , KEYCODE_LEFT,       IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+"     , KEYCODE_EQUALS,     IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3"     , KEYCODE_3,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2"     , KEYCODE_2,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1"     , KEYCODE_1,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STORE" , KEYCODE_TAB,        IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4"    , KEYCODE_F4,         IP_JOY_NONE )
	PORT_START   /* bit 2 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Right" , KEYCODE_RIGHT,      IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-"     , KEYCODE_MINUS,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6"     , KEYCODE_6,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5"     , KEYCODE_5,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4"     , KEYCODE_4,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ","     , KEYCODE_COMMA,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F3"    , KEYCODE_F3,         IP_JOY_NONE )
	PORT_START   /* bit 3 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up"    , KEYCODE_UP,         IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "*"     , KEYCODE_L,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9"     , KEYCODE_9,          IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8"     , KEYCODE_8,          IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7"     , KEYCODE_7,          IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x^2"   , KEYCODE_COLON,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2"    , KEYCODE_F2,         IP_JOY_NONE )
	PORT_START   /* bit 4 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/"     , KEYCODE_SLASH,      IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ")"     , KEYCODE_CLOSEBRACE, IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "("     , KEYCODE_OPENBRACE,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EE"    , KEYCODE_END,        IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LN"    , KEYCODE_BACKSLASH,  IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F1"    , KEYCODE_F1,         IP_JOY_NONE )
	PORT_START   /* bit 5 */                                                        
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^"     , KEYCODE_P,          IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAN"   , KEYCODE_PGUP,       IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "COS"   , KEYCODE_HOME,       IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SIN"   , KEYCODE_INSERT,     IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LOG"   , KEYCODE_QUOTE,      IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2nd"   , KEYCODE_LALT,       IP_JOY_NONE )
	PORT_START   /* bit 6 */
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CLEAR" , KEYCODE_PGDN,       IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CUSTOM", KEYCODE_F9,         IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PRGM"  , KEYCODE_F8,         IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STAT"  , KEYCODE_F7,         IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "GRAPH" , KEYCODE_F6,         IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EXIT"  , KEYCODE_ESC,        IP_JOY_NONE )
	PORT_START   /* bit 7 */
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL"   , KEYCODE_DEL,        IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x-VAR" , KEYCODE_LCONTROL,   IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALPHA" , KEYCODE_CAPSLOCK,   IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MORE"  , KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_START   /* ON */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ON/OFF", KEYCODE_Q,          IP_JOY_NONE )
	PORT_START   /* receive data from calculator */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Receive data", KEYCODE_R, IP_JOY_NONE )
	PORT_START   /* screen dump requesting */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Screen dump request", KEYCODE_S, IP_JOY_NONE )
INPUT_PORTS_END


static struct Speaker_interface ti85_speaker_interface=
{
 1,
 {50},
};

/* machine definition */
static MACHINE_DRIVER_START( ti81 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2000000)        /* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(ti81_readmem, ti81_writemem)
	MDRV_CPU_IO_MAP(ti81_readport, ti81_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( ti81 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(96, 64)
	MDRV_VISIBLE_AREA(0, 96-1, 0, 64-1)
	MDRV_PALETTE_LENGTH(32*7 + 32768)
	MDRV_COLORTABLE_LENGTH(32*7 + 32768)
	MDRV_PALETTE_INIT( ti85 )

	MDRV_VIDEO_START( ti85 )
	MDRV_VIDEO_UPDATE( ti85 )

	MDRV_NVRAM_HANDLER( ti81 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti85 )
	MDRV_IMPORT_FROM( ti81 )
	MDRV_CPU_REPLACE("main", Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_PROGRAM_MAP(ti85_readmem, ti85_writemem)
	MDRV_CPU_IO_MAP(ti85_readport, ti85_writeport)

	MDRV_MACHINE_INIT( ti85 )
	MDRV_MACHINE_STOP( ti85 )

	MDRV_SCREEN_SIZE(128, 64)
	MDRV_VISIBLE_AREA(0, 128-1, 0, 64-1)
	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, ti85_speaker_interface)

	MDRV_NVRAM_HANDLER( ti85 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti86 )
	MDRV_IMPORT_FROM( ti85 )
	MDRV_CPU_PROGRAM_MAP(ti86_readmem, ti86_writemem)
	MDRV_CPU_IO_MAP(ti86_readport, ti86_writeport)

	MDRV_MACHINE_INIT( ti86 )
	MDRV_MACHINE_STOP( ti86 )

	MDRV_NVRAM_HANDLER( ti86 )
MACHINE_DRIVER_END


ROM_START (ti81)
	ROM_REGION (0x18000, REGION_CPU1,0)
	ROM_LOAD ("ti81.bin", 0x10000, 0x8000, CRC(94ac58e2))
ROM_END

ROM_START (ti85)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v30a.bin", 0x10000, 0x20000, CRC(de4c0b1a))
ROM_END

ROM_START (ti85v40)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v40.bin", 0x10000, 0x20000, CRC(a1723a17))
ROM_END

ROM_START (ti85v50)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v50.bin", 0x10000, 0x20000, CRC(781fa403))
ROM_END

ROM_START (ti85v60)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v60.bin", 0x10000, 0x20000, CRC(b694a117))
ROM_END

ROM_START (ti85v80)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v80.bin", 0x10000, 0x20000, CRC(7f296338))
ROM_END

ROM_START (ti85v90)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v90.bin", 0x10000, 0x20000, CRC(6a0a94d0))
ROM_END

ROM_START (ti85v100)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v100.bin", 0x10000, 0x20000, CRC(053325b0))
ROM_END

ROM_START (ti86)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v12.bin", 0x10000, 0x40000, CRC(bdf16105))
ROM_END

ROM_START (ti86v13)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v13.bin", 0x10000, 0x40000, CRC(073ef70f))
ROM_END

ROM_START (ti86v14)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v14.bin", 0x10000, 0x40000, CRC(fe6e2986))
ROM_END

ROM_START (ti86v15)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v15.bin", 0x10000, 0x40000, BAD_DUMP CRC(e6e10546))
ROM_END

ROM_START (ti86v16)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v16.bin", 0x10000, 0x40000, CRC(37e02acc))
ROM_END

ROM_START (ti86grom)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86grom.bin", 0x10000, 0x40000, CRC(d2c67280))
ROM_END

SYSTEM_CONFIG_START(ti85)
	CONFIG_DEVICE_BASE ( IO_SERIAL, 1,
			"85p\085s\085i\085n\085c\085l\085k\085m\085v\085d\085e\085r\085g\085b\0",
			DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, device_init_ti85_serial, NULL, device_load_ti85_serial, device_unload_ti85_serial,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL )
	CONFIG_DEVICE_SNAPSHOT( "sav\0", ti8x )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(ti86)
	CONFIG_DEVICE_BASE ( IO_SERIAL, 1,
			"86p\086s\086i\086n\086c\086l\086k\086m\086v\086d\086e\086r\086g\0",
			DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, device_init_ti85_serial, NULL, device_load_ti85_serial, device_unload_ti85_serial,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL )
	CONFIG_DEVICE_SNAPSHOT( "sav\0", ti8x )
SYSTEM_CONFIG_END
                            
/*    YEAR  NAME		PARENT	COMPAT	MACHINE INPUT	INIT	CONFIG	COMPANY        FULLNAME */
COMP( 1990, ti81,          0,   0,		ti81,	ti81,	0,		NULL,	"Texas Instruments", "TI-81 Ver. 1.8" )

COMP( 1992, ti85,          0,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 3.0a" )
COMP( 1992, ti85v40,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 4.0" )
COMP( 1992, ti85v50,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 5.0" )
COMP( 1992, ti85v60,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 6.0" )
COMP( 1992, ti85v80,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 8.0" )
COMP( 1992, ti85v90,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 9.0" )
COMP( 1992, ti85v100,   ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 10.0" )

COMP( 1997, ti86,   	   0,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.2" )
COMP( 1997, ti86v13,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.3" )
COMP( 1997, ti86v14,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.4" )
COMP( 1997, ti86v15,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.5" )
COMP( 1997, ti86v16,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.6" )
COMP( 1997, ti86grom,   ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 homebrew rom by Daniel Foesch" )
