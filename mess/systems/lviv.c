/*******************************************************************************

PK-01 Lviv driver by Krzysztof Strzecha

Big thanks go to:
Anton V. Ignatichev for informations about Lviv hardware.
Dr. Volodimir Mosorov for two Lviv machines.

What's new:
-----------
28.02.2003      Snapshot veryfing function added.
07.01.2003	Support for .SAV snapshots. Joystick support (there are strange
		problems with "Doroga (1991)(-)(Ru).lvt".
21.12.2002	Cassette support rewritten, WAVs saving and loading are working now.
08.12.2002	Comments on emulation status updated. Changed 'lvive' to 'lvivp'.
		ADC r instruction in 8080 core fixed (Arkanoid works now).
		Orginal keyboard layout added.
20.07.2002	"Reset" key fixed. 8080 core fixed (all BASIC commands works).
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
1. LIMITATION: Printer is not emulated. 
2. LIMITATION: Timings are not implemented, due to it emulated machine runs
   twice fast as orginal.
3. LIMITATION: .RSS files are not supported.
4. LIMITATION: Some usage notes and trivia are needed in sysinfo.dat.

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
			bits 0-4 joystick scanner output
		Port B: palette control, extension slot input or output
			sound on/off
			bit 7 sound on/off
			bits 0-6 palette select
		Port C: memory page changing, tape input and output,
			printer control, sound
			bits 0-3 extension slot input
			bits 4-7 extension slot output
			bit 7: joystick scanner input
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


Timings:
--------

	The CPU timing is controlled by a KR580GF24 (Sovietic copy of i8224) connected to a 18MHz(?)
	oscillator. CPU frequency must be 18MHz/9 = 2MHz.

	Memory timing uses a 8-phase clock, derived from a 20MHz(?) video clock (called VCLK0 here:
	in the schematics, it comes from pin 6 of V8, and it is labelled "0'" in the video clock bus).
	This clock is divided by G7, G6 and D5 to generate the signals we call VCLK1-VCLK11.  The memory
	clock phases Phi0-Phi7 are generated in D7, whereas PHI'14 and PHI'15 are generated in D8.

	When the CPU accesses RAM, wait states are inserted until the RAM transfer is complete.

	CPU clock: 18MHz/9 = 2MHz
	memory cycle time: 20MHz/8 = 2.5MHz
	CPU memory access time: (min) approx. 9/20MHz = 450ns
	                        (max) approx. 25/20MHz = 1250ns
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
        RDM*    \\\\\\\\\\\\\\\\\\\\\\\\\\_c __________________________________///////////
                        _________________________________________________________________________
	RA		\\\\\\\\\\\\\\\\\\\\\\\\\\_a__________________________________/

	DRAM
	ADDRESS	video row /\ video column /XXX\CPU row (a)/\  CPU column (a)  /\ video row

	a: only if the CPU is requesting a RAM read/write
	b: only if the CPU is requesting a RAM write
	c: only if the CPU is requesting a RAM read

*******************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "includes/lviv.h"
#include "devices/snapquik.h"
#include "devices/cassette.h"
#include "formats/lviv_lvt.h"

/* I/O ports */

ADDRESS_MAP_START( lviv_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_READ( lviv_io_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( lviv_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_WRITE( lviv_io_w )
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( lviv_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
ADDRESS_MAP_END


/* keyboard input */
INPUT_PORTS_START (lviv)
	PORT_START /* 2nd PPI port A bit 0 low */
		PORT_KEY2(0x01, IP_ACTIVE_LOW, "6  &",	KEYCODE_6,		CODE_NONE,	'6',	'&')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "7  '",	KEYCODE_7,		CODE_NONE,	'7',	'\'')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "8  (",	KEYCODE_8,		CODE_NONE,	'8',	'(')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "Ready",	KEYCODE_INSERT,		CODE_NONE,	UCHAR_MAMEKEY(INSERT))
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Tab",	KEYCODE_TAB,		CODE_NONE,	9)
		PORT_KEY2(0x20, IP_ACTIVE_LOW, "-  =",	KEYCODE_MINUS,		CODE_NONE,	'-',	'=')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "0",	KEYCODE_0,		CODE_NONE,	'0')
		PORT_KEY2(0x80, IP_ACTIVE_LOW, "9  )",	KEYCODE_9,		CODE_NONE,	'9',	')')
	PORT_START /* 2nd PPI port A bit 1 low */
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "G",	KEYCODE_U,		CODE_NONE,	'g')
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "[",	KEYCODE_I,		CODE_NONE,	'[')
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "]",	KEYCODE_O,		CODE_NONE,	']')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "Enter",	KEYCODE_ENTER,		CODE_NONE,	13)
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Run",	KEYCODE_DEL,		CODE_NONE,	UCHAR_MAMEKEY(DEL))
		PORT_KEY2(0x20, IP_ACTIVE_LOW, "*  :",	KEYCODE_CLOSEBRACE,	CODE_NONE,	'*',	':')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "H",	KEYCODE_OPENBRACE,	CODE_NONE,	'h')
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "Z",	KEYCODE_P,		CODE_NONE,	'z')
	PORT_START /* 2nd PPI port A bit 2 low */
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "R",	KEYCODE_H,		CODE_NONE,	'r')
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "O",	KEYCODE_J,		CODE_NONE,	'o')
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "L",	KEYCODE_K,		CODE_NONE,	'l')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "Del",	KEYCODE_BACKSPACE,	CODE_NONE,	8)
		PORT_KEY2(0x10, IP_ACTIVE_LOW, ".  >",	KEYCODE_END,		CODE_NONE,	'.',	'>')
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "\\",	KEYCODE_BACKSLASH,	CODE_NONE,	'\\')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "V",	KEYCODE_COLON,		CODE_NONE,	'v')
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "D",	KEYCODE_L,		CODE_NONE,	'd')
	PORT_START /* 2nd PPI port A bit 3 low */
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "Space",	KEYCODE_SPACE,		CODE_NONE,	' ')
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "B",	KEYCODE_COMMA,		CODE_NONE,	'b')
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "@",	KEYCODE_STOP,		CODE_NONE,	'@')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "Alt",	KEYCODE_RSHIFT,		CODE_NONE,	UCHAR_MAMEKEY(RSHIFT))
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "_",	KEYCODE_EQUALS,		CODE_NONE,	'_')
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "Latin",	KEYCODE_RCONTROL,	CODE_NONE,	UCHAR_MAMEKEY(RCONTROL))
		PORT_KEY2(0x40, IP_ACTIVE_LOW, "/  ?",	KEYCODE_SLASH,		CODE_NONE,	'/',	'?')
		PORT_KEY2(0x80, IP_ACTIVE_LOW, ",  <",	KEYCODE_HOME,		CODE_NONE,	',',	'<')
	PORT_START /* 2nd PPI port A bit 4 low */
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "Cls",	KEYCODE_ESC,		CODE_NONE,	27)
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "(G)",	KEYCODE_F1,		CODE_NONE,	UCHAR_MAMEKEY(F1))
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "(B)",	KEYCODE_F2,		CODE_NONE,	UCHAR_MAMEKEY(F2))
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "5  %",	KEYCODE_5,		CODE_NONE,	'5',	'%')
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "4  $",	KEYCODE_4,		CODE_NONE,	'4',	'$')
		PORT_KEY2(0x20, IP_ACTIVE_LOW, "3  #",	KEYCODE_3,		CODE_NONE,	'3',	'#')
		PORT_KEY2(0x40, IP_ACTIVE_LOW, "2  \"",	KEYCODE_2,		CODE_NONE,	'2',	'\"')
		PORT_KEY2(0x80, IP_ACTIVE_LOW, "1  !",	KEYCODE_1,		CODE_NONE,	'1',	'!')
	PORT_START /* 2nd PPI port A bit 5 low */
		PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "J",	KEYCODE_Q,		CODE_NONE,	'j')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "N",	KEYCODE_Y,		CODE_NONE,	'n')
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "E",	KEYCODE_T,		CODE_NONE,	'e')
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "K",	KEYCODE_R,		CODE_NONE,	'k')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "U",	KEYCODE_E,		CODE_NONE,	'u')
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "C",	KEYCODE_W,		CODE_NONE,	'c')
	PORT_START /* 2nd PPI port A bit 6 low */
		PORT_KEY2(0x01, IP_ACTIVE_LOW, ";  +",	KEYCODE_TILDE,		CODE_NONE,	';',	'+')
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "Russian",KEYCODE_LCONTROL,	CODE_NONE,	UCHAR_MAMEKEY(LCONTROL))
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "Key",	KEYCODE_CAPSLOCK,	CODE_NONE,	UCHAR_MAMEKEY(CAPSLOCK))
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "P",	KEYCODE_G,		CODE_NONE,	'p')
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "A",	KEYCODE_F,		CODE_NONE,	'a')
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "W",	KEYCODE_D,		CODE_NONE,	'w')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "Y",	KEYCODE_S,		CODE_NONE,	'y')
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "F",	KEYCODE_A,		CODE_NONE,	'f')
	PORT_START /* 2nd PPI port A bit 7 low */
		PORT_KEY1(0x01, IP_ACTIVE_LOW, "Shift",	KEYCODE_LSHIFT,		CODE_NONE,	UCHAR_SHIFT_1)
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "Q",	KEYCODE_Z,		CODE_NONE,	'q')
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "^",	KEYCODE_X,		CODE_NONE,	'^')
		PORT_KEY1(0x08, IP_ACTIVE_LOW, "X",	KEYCODE_M,		CODE_NONE,	'x')
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "T",	KEYCODE_N,		CODE_NONE,	't')
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "I",	KEYCODE_B,		CODE_NONE,	'i')
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "M",	KEYCODE_V,		CODE_NONE,	'm')
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "S",	KEYCODE_C,		CODE_NONE,	's')
	PORT_START /* 2nd PPI port C bit 0 low */
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "PrnLck",KEYCODE_F6,		CODE_NONE,	UCHAR_MAMEKEY(F6))
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "ScrLck",KEYCODE_F5,		CODE_NONE,	UCHAR_MAMEKEY(F5))
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "Sound",	KEYCODE_F4,		CODE_NONE,	UCHAR_MAMEKEY(F4))
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "(R)",	KEYCODE_F3,		CODE_NONE,	UCHAR_MAMEKEY(F3))
	PORT_START /* 2nd PPI port C bit 1 low */
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "ScrPrn",KEYCODE_F7,		CODE_NONE,	UCHAR_MAMEKEY(F7))
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "F0",	KEYCODE_F8,		CODE_NONE,	UCHAR_MAMEKEY(F8))
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "F1",	KEYCODE_F9,		CODE_NONE,	UCHAR_MAMEKEY(F9))
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "F2",	KEYCODE_F10,		CODE_NONE,	UCHAR_MAMEKEY(F10))
	PORT_START /* 2nd PPI port C bit 2 low */
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Home",	KEYCODE_PGUP,		CODE_NONE,	UCHAR_MAMEKEY(PGUP))
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "F5",	KEYCODE_SCRLOCK,	CODE_NONE,	UCHAR_MAMEKEY(SCRLOCK))
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "F4",	KEYCODE_F12,		CODE_NONE,	UCHAR_MAMEKEY(F12))
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "F3",	KEYCODE_F11,		CODE_NONE,	UCHAR_MAMEKEY(F11))
	PORT_START /* 2nd PPI port C bit 3 low */
		PORT_KEY1(0x10, IP_ACTIVE_LOW, "Right",	KEYCODE_RIGHT,		CODE_NONE,	UCHAR_MAMEKEY(RIGHT))
		PORT_KEY1(0x20, IP_ACTIVE_LOW, "Up",	KEYCODE_UP,		CODE_NONE,	UCHAR_MAMEKEY(UP))
		PORT_KEY1(0x40, IP_ACTIVE_LOW, "Left",	KEYCODE_LEFT,		CODE_NONE,	UCHAR_MAMEKEY(LEFT))
		PORT_KEY1(0x80, IP_ACTIVE_LOW, "Down",	KEYCODE_DOWN,		CODE_NONE,	UCHAR_MAMEKEY(DOWN))
	PORT_START /* CPU */
		PORT_KEY1(0x01, IP_ACTIVE_HIGH, "Reset",KEYCODE_PGDN,		CODE_NONE,	UCHAR_MAMEKEY(PGDN))
	PORT_START /* Joystick */
		PORT_BIT(0x01,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_UP)
		PORT_BIT(0x02,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_DOWN)
		PORT_BIT(0x04,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_RIGHT)
		PORT_BIT(0x08,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_LEFT)
		PORT_BIT(0x10,	IP_ACTIVE_HIGH,	IPT_BUTTON1)
		PORT_BIT(0x20,	IP_ACTIVE_HIGH,	IPT_UNUSED)
		PORT_BIT(0x40,	IP_ACTIVE_HIGH,	IPT_UNUSED)
		PORT_BIT(0x80,	IP_ACTIVE_HIGH,	IPT_UNUSED)
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
	MDRV_CPU_PROGRAM_MAP(lviv_mem, 0)
	MDRV_CPU_IO_MAP(lviv_readport, lviv_writeport)
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
	ROM_LOAD("lviv.bin", 0x10000, 0x4000, CRC(44a347d9))
ROM_END

ROM_START(lviva)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lviva.bin", 0x10000, 0x4000, CRC(551622f5))
ROM_END

ROM_START(lvivp)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lvive.bin", 0x10000, 0x4000, CRC(f171c282))
ROM_END

SYSTEM_CONFIG_START(lviv)
	CONFIG_RAM_DEFAULT(64 * 1024)
	/* 9-Oct-2003 - Changed to lvt because lv? is an invalid file extension */
	CONFIG_DEVICE_CASSETTEX(1, lviv_lvt_format, NULL, CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED)
	CONFIG_DEVICE_SNAPSHOT( "sav\0", lviv )
SYSTEM_CONFIG_END


/*    YEAR  NAME       PARENT  COMPAT	MACHINE    INPUT     INIT     CONFIG,  COMPANY	FULLNAME */
COMP( 1989, lviv,      0,      0,		lviv,      lviv,     0,       lviv,    "",	"PK-01 Lviv" )
COMP( 1989, lviva,     lviv,   0,		lviv,      lviv,     0,       lviv,    "",	"PK-01 Lviv (alternate)" )
COMP( 1986, lvivp,     lviv,   0,		lviv,      lviv,     0,       lviv,    "",	"PK-01 Lviv (prototype)" )
