/*******************************************************************************

PK-01 Lviv driver by Krzysztof Strzecha

Big thanks go to:
Anton V. Ignatichev for informations about Lviv hardware.
Volodimir Mosorov for two Lviv machines.

What's new:
-----------
20.07.2002	"Reset" key fixed. 8080 core fixed (all BASIC commands works
		now). Unsupported .lvt files versions aren't now loaded.
xx.07.2002	Improved port and memory mapping (Raphael Nabet).
		Hardware description updated (Raphael Nabet).
27.03.2002	CPU clock changed to 2.5MHz.
		New Lviv driver added for different ROM revision.
24.03.2002	Palette emulation.
		Bit 7 of port 0xc1 emulated - speaker enabled/disabled.
		Some notes about hardware added.
		"Reset" key added.
23.03.2002	Hardware description and notes on emulation status added.
		Few changes in keyboard mapping.

Notes on emulation status and to do list:
-----------------------------------------
1. Printer is not emulated. 
2. Only simple video hardware emulation (timings are not known).
3. .RSS and .SAV files are not supported.
4. Some usage notes and trivia are needed in sysinfo.dat.


Lviv technical information
==========================

CPU:
----
	8080 2.5MHz (2MHz in first machines)


Memory map:
-----------
	start-up map (cleared by the first I/O write operation done by the CPU):
	0000-3fff ROM mirror #1
	4000-7fff ROM mirror #2
	8000-bfff ROM mirror #3
	c000-ffff ROM

	normal map with video RAM off:
	0000-3fff RAM
	4000-7fff RAM
	8000-bfff RAM
	c000-ffff ROM

	normal map with video RAM on:
	0000-3fff mirrors 8000-bfff
	4000-7fff video RAM
	8000-bfff RAM
	c000-ffff ROM

Interrupts:
-----------
	No interrupts in Lviv.

Ports:
------
	Only A4-A5 are decoded.  A2-A3 is ignored in the console, but could be used by extension
	devices.

	C0-C3	8255 PPI
		Port A: extension slot output, printer data
		Port B: palette control, extension slot input or output
			sound on/off
			bit 7 sound on/off
			bits 0-6 palette select
		Port C: memory page changing, tape input and output,
			printer control, sound
			bits 0-3 extension slot input
			bits 4-7 extension slot output
			bit 7: not used
			bit 6: printer control AC/busy
			bit 5: not used
			bit 4: tape in
			bit 3: not used
			bit 2: printer control SC/strobe
			bit 1: memory paging, 0 - video ram, 1 - ram
			bit 0: tape out, sound

	D0-D3	8255 PPI
		Port A:
			keyboard scaning
		Port B:
			keyboard reading
		Port C:
			keyboard scaning/reading

Keyboard:
---------
	Reset - connected to CPU reset line

				     Port D0 				
	--------T-------T-------T-------T-------T-------T-------T-------ª        
	|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
	+-------+-------+-------+-------+-------+-------+-------+-------+---ª
	| Shift |   ;   |       |  CLS  | Space |   R   |   G   |   6   | 0 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   Q   |Russian|       |  (G)  |   B   |   O   |   [   |   7   | 1 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   ^   |  Key  |   J   |  (B)  |   @   |   L   |   ]   |   8   | 2 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   X   |   P   |   N   |   5   |  Alt  |  Del  | Enter | Ready | 3 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+ Port D1
	|   T   |   A   |   E   |   4   |   _   |   .   |  Run  |  Tab  | 4 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   I   |   W   |   K   |   3   | Latin |   \   |   :   |   -   | 5 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   M   |   Y   |   U   |   2   |   /   |   V   |   H   |   0   | 6 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   S   |   F   |   C   |   1   |   ,   |   D   |   Z   |   9   | 7 |
	L-------+-------+-------+-------+-------+-------+-------+-------+----

		     Port D2
	--------T-------T-------T-------ª
	|   3   |   2   |   1   |   0   |
	+-------+-------+-------+-------+-----ª
	| Right | Home  |ScrPrn |PrnLock|  4  |
	+-------+-------+-------+-------+-----+
	|  Up   |  F5   |  F0   |ScrLock|  5  |
	+-------+-------+-------+-------+-----+ Port D2
	| Left  |  F4   |  F1   | Sound |  6  |
	+-------+-------+-------+-------+-----+
	| Down  |  F3   |  F2   |  (R)  |  7  |
	L-------+-------+-------+-------+------

	Notes:
		CLS	- clear screen
		(G)	- clear screen with border and set COLOR 0,0,0
		(B)	- clear screen with border and set COLOR 1,0,6
		(R)	- clear screen with border and set COLOR 0,7,3
		Sound	- sound on/off
		ScrLock	- screen lock
		PrnLock	- printer on/off
		ScrPrn	- screen and printer output mode
		Russian	- russian keyboard mode
		Latin	- latin keyboard mode
		Right	- cursor key
		Up	- cursor key
		Left	- cursor key
		Down	- cursor key
		Keyword	- BASIC keyword


Video:
-----
	Screen resolution is 256x256 pixels. 4 colors at once are possible,
	but there is a posiibility of palette change. Bits 0..6 of port 0xc1
	are used for palette setting.

	One byte of video-RAM sets 4 pixels. Colors of pixels are corrected
	by current palette. Each bits combination (2 bits sets one pixel on
	the display), corrected with palette register, sets REAL pixel color.

	PBx - bit of port 0xC1 numbered x
	R,G,B - output color components
	== - "is equal"
	! - inversion

	00   R = PB3 == PB4; G = PB5; B = PB2 == PB6;
	01   R = PB4; G = !PB5; B = PB6;
	10   R = PB0 == PB4; G = PB5; B = !PB6;
	11   R = !PB4; G = PB1 == PB5; B = PB6;

	Bit combinations are result of concatenation of approprate bits of
	high and low byte halfs.

	Example:
	~~~~~~~~

	Some byte of video RAM:  1101 0001
	Value of port 0xC1:      x000 1110

	1101
	0001
	----
	10 10 00 11

	1st pixel (10): R = 1; G = 0; B = 1;
	2nd pixel (10): R = 1; G = 0; B = 1;
	3rd pixel (00): R = 0; G = 0; B = 0;
	4th pixel (11): R = 1; G = 0; B = 0;


Sound:
------
	Buzzer connected to port 0xc2 (bit 0).
	Bit 7 of port 0xc1 - enable/disable speaker.


timings:
--------

	The CPU timing is controlled by a KR580GF24 (Sovietic copy of i8224) connected to a 18MHz(?)
	oscillator.  CPU frequency must be 18MHz/8 = 2.25MHz (no waranty).

	Memory timing uses a 8-phase clock, derived from a 20MHz(?) video clock (called VCLK0 here:
	in the schematics, it comes from pin 6 of V8, and it is labelled "0'" in the video clock bus).
	This clock is divided by G7, G6 and D5 to generate the signals we call VCLK1-VCLK11.  The memory
	clock phases Phi0-Phi7 are generated in D7, whereas PHI'14 and PHI'15 are generated in D8.

	When the CPU accesses RAM, wait states are inserted until the RAM transfer is complete.

	CPU clock: 18MHz/8 = 2.25MHz
	memory cycle time: 20MHz/8 = 2.5MHz
	CPU memory access time: (min) approx. 9/20MHz = 450ns
	                        (max) approx. 17/20MHz = 850ns
	pixel clock: 20MHz/4 = 5MHz
	screen size: 256*256
	HBL: 64 pixel clock cycles
	VBL: 64 lines
	horizontal frequency: 5MHZ/(256+64) = 15.625kHz
	vertical frequency: 15.625kHz/(256+64) = 48.83Hz

			 |<--------VIDEO WINDOW--------->|<----------CPU WINDOW--------->|<--
			_   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
	VCLK0	 |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |
			_     ___     ___     ___     ___     ___     ___     ___     ___     ___
	VCLK1	 |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |
			_         _______         _______         _______         _______
	VCLK2	 |_______|       |_______|       |_______|       |_______|       |______|
			_                 _______________                 _______________
	VCLK3	 |_______________|               |_______________|               |_______
			_                                 _______________________________
	VCLK4	 |_______________________________|                               |_______

			  _                               _                               _
	PHI0	_| |_____________________________| |_____________________________| |_____
			      _                               _                               _
	PHI1	_____| |_____________________________| |_____________________________| |_
			          _                               _
	PHI2	_________| |_____________________________| |_____________________________
			              _                               _
	PHI3	_____________| |_____________________________| |_________________________
			                  _                               _
	PHI4	_________________| |_____________________________| |_____________________
			                      _                               _
	PHI5	_____________________| |_____________________________| |_________________
			                          _                               _
	PHI6	_________________________| |_____________________________| |_____________
			                              _                               _
	PHI7	_____________________________| |_____________________________| |_________
			                                                          _
	PHI'14	_________________________________________________________| |_____________
			                                                              _
	PHI'15	_____________________________________________________________| |_________
			__________             __________________________________________________
	RAS*	          \___________/                   \_a_________/
			______________                 __________________________________________
	CAS*	              \_______________/               \_a_____________/
			_________________________________________________________________________
	WR*		                                                  \_b_________////////
			_________________________________________________________________________
	WRM*	\\\\\\\\\\\\\\\\\\\\\\\\\\_b__________________________________///////////
			_________________________________________________________________________
	RDM*	\\\\\\\\\\\\\\\\\\\\\\\\\\_c__________________________________///////////
			_________________________________________________________________________
	RA		\\\\\\\\\\\\\\\\\\\\\\\\\\_a__________________________________/

	DRAM
	ADDRESS	video row /\ video column /XXX\CPU row (a)/\  CPU column (a)  /\ video row

	a: only if the CPU is requesting a RAM read/write
	b: only if the CPU is requesting a RAM write
	c: only if the CPU is requesting a RAM read

*******************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "includes/lviv.h"

/* I/O ports */

PORT_READ_START( lviv_readport )
	{ 0x00, 0xff, lviv_io_r },
PORT_END

PORT_WRITE_START( lviv_writeport )
	{ 0x00, 0xff, lviv_io_w },
PORT_END

/* memory w/r functions */

MEMORY_READ_START( lviv_readmem )
	{0x0000, 0x3fff, MRA_BANK1},
	{0x4000, 0x7fff, MRA_BANK2},
	{0x8000, 0xbfff, MRA_BANK3},
	{0xc000, 0xffff, MRA_BANK4},
MEMORY_END

MEMORY_WRITE_START( lviv_writemem )
	{0x0000, 0x3fff, MWA_BANK1},
	{0x4000, 0x7fff, MWA_BANK2},
	{0x8000, 0xbfff, MWA_BANK3},
	{0xc000, 0xffff, MWA_BANK4},
MEMORY_END


/* keyboard input */
INPUT_PORTS_START (lviv)
	PORT_START /* 2nd PPI port A bit 0 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6",	KEYCODE_6,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7",	KEYCODE_7,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8",	KEYCODE_8,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Ready",	KEYCODE_INSERT,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab",	KEYCODE_TAB,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "=",	KEYCODE_EQUALS,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "0",	KEYCODE_0,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "9",	KEYCODE_9,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 1 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",	KEYCODE_G,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "[",	KEYCODE_OPENBRACE,	IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "]",	KEYCODE_CLOSEBRACE,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter",	KEYCODE_ENTER,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Run",	KEYCODE_DEL,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "*",	KEYCODE_QUOTE,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",	KEYCODE_H,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",	KEYCODE_Z,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 2 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",	KEYCODE_R,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",	KEYCODE_O,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",	KEYCODE_L,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Del",	KEYCODE_BACKSPACE,	IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ".",	KEYCODE_STOP,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\",	KEYCODE_BACKSLASH,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",	KEYCODE_V,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",	KEYCODE_D,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 3 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space",	KEYCODE_SPACE,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",	KEYCODE_B,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "@",	KEYCODE_ESC,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Alt",	KEYCODE_RSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "_",	KEYCODE_MINUS,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Latin",	KEYCODE_RCONTROL,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/",	KEYCODE_SLASH,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ",",	KEYCODE_COMMA,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 4 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cls",	KEYCODE_END,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(G)",	KEYCODE_F1,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(B)",	KEYCODE_F2,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5",	KEYCODE_5,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4",	KEYCODE_4,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3",	KEYCODE_3,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2",	KEYCODE_2,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1",	KEYCODE_1,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 5 low */
		PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",	KEYCODE_J,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",	KEYCODE_N,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",	KEYCODE_E,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",	KEYCODE_K,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",	KEYCODE_U,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",	KEYCODE_C,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 6 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ";",	KEYCODE_COLON,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Russian",	KEYCODE_LCONTROL,	IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Key",	KEYCODE_CAPSLOCK,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",	KEYCODE_P,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",	KEYCODE_A,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",	KEYCODE_W,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",	KEYCODE_Y,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",	KEYCODE_F,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 7 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift",	KEYCODE_LSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",	KEYCODE_Q,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "^",	KEYCODE_TILDE,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",	KEYCODE_X,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",	KEYCODE_T,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",	KEYCODE_I,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",	KEYCODE_M,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",	KEYCODE_S,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 0 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "PrnLck",	KEYCODE_F6,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "ScrLck",	KEYCODE_F5,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Sound",	KEYCODE_F4,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "(R)",	KEYCODE_F3,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 1 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "ScrPrn",	KEYCODE_F7,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0",	KEYCODE_F8,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",	KEYCODE_F9,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",	KEYCODE_F10,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 2 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Home",	KEYCODE_HOME,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",	KEYCODE_SCRLOCK,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",	KEYCODE_F12,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",	KEYCODE_F11,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 3 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right",	KEYCODE_RIGHT,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Up",	KEYCODE_UP,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left",	KEYCODE_LEFT,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Down",	KEYCODE_DOWN,		IP_JOY_NONE )
	PORT_START /* CPU */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Reset",	KEYCODE_PGUP,		IP_JOY_NONE )
INPUT_PORTS_END

static struct Speaker_interface lviv_speaker_interface=
{
	1,
	{50},
};

static struct Wave_interface lviv_wave_interface = {
	1,		/* 1 cassette recorder */
	{ 50 }		/* mixing levels in percent */
};

/* machine definition */
static MACHINE_DRIVER_START( lviv )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2500000)
	MDRV_CPU_MEMORY(lviv_readmem, lviv_writemem)
	MDRV_CPU_PORTS(lviv_readport, lviv_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( lviv )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(sizeof (lviv_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (lviv_colortable))
	MDRV_PALETTE_INIT( lviv )

	MDRV_VIDEO_START( lviv )
	MDRV_VIDEO_UPDATE( lviv )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, lviv_speaker_interface)
	MDRV_SOUND_ADD(WAVE, lviv_wave_interface)
MACHINE_DRIVER_END

ROM_START(lviv)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lviv.bin", 0x10000, 0x4000, 0x44a347d9)
ROM_END

ROM_START(lviva)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lviva.bin", 0x10000, 0x4000, 0x551622f5)
ROM_END

ROM_START(lvive)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lvive.bin", 0x10000, 0x4000, 0xf171c282)
ROM_END

SYSTEM_CONFIG_START(lviv)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE_CASSETTE(1, "lv?\0", lviv_tape_init)
SYSTEM_CONFIG_END


/*    YEAR  NAME       PARENT  MACHINE    INPUT     INIT     CONFIG,  COMPANY               FULLNAME */
COMP( 1989, lviv,      0,      lviv,      lviv,     0,       lviv,    "", "PK-01 Lviv" )
COMP( 1989, lviva,     lviv,   lviv,      lviv,     0,       lviv,    "", "PK-01 Lviv (alternate)" )
COMP( 1986, lvive,     lviv,   lviv,      lviv,     0,       lviv,    "", "PK-01 Lviv (early)" )
