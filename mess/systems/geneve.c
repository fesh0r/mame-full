/*
	Preliminary MESS Driver for the Myarc Geneve 9640.
	Raphael Nabet, 2003.

	The Geneve has two operation modes.  One is compatible with the TI-99/4a,
	the other is not.


	General architecture:

	TMS9995@12MHz (including 256 bytes of on-chip 16-bit RAM and a timer),
	V9938, SN76496 (compatible with TMS9919), TMS9901, MM58274 RTC, 512 kbytes
	of 1-wait-state CPU RAM (expandable to almost 2 Mbytes), 32 kbytes of
	0-wait-state CPU RAM (expandable to 64 kbytes), 128 kbytes of VRAM.


	Memory map:

	64-kbyte CPU address space is mapped to a 2-Mbyte virtual address space.
	8 8-kbyte pages are available simultaneously, out of a total of 256.

	Pages (regular console):
	>00->3f: 512kbytes of CPU RAM (pages >36 and >37 are used to emulate
	  cartridge CPU ROMs in ti99 mode, and pages >38 through >3f are used to
	  emulate console and cartridge GROMs in ti99 mode)
	>40->7f: optional Memex RAM
	>80->b7: PE-bus space using spare address lines (AMA-AMC)???  Used by
	  RAM extension (Memex or PE-Bus 512k card).
	>b8->bf: PE-bus space (most useful pages are >ba: DSR space and >bc: speech
	  synthesizer page; other pages are used by RAM extension)
	>e8->ef: 64kbytes of 0-wait-state RAM (with 32kbytes of SRAM installed,
	  only even pages (>e8, >ea, >ec and >ee) are available)
	>f0->f1: boot ROM
	>f2->ff: Memex RAM

	Pages (genmod console):
	>00->3f: switch-selectable(?): 512kbytes of onboard RAM (1-wait-state DRAM)
	  or first 512kbytes of the Memex Memory board (0-wait-state SRAM).  The
	  latter setting is incompatible with TI mode.
	>40->b9, >bb, >bd->ef, f2->ff: Memex RAM (it is unknown if >e8->ef is on
	  the Memex board or the Geneve motherboard)
	>ba: DSR space
	>bc: speech synthesizer space
	>f0->f1: boot ROM

	Unpaged locations (ti99 mode):
	>8000->8007: memory page registers (>8000 for page 0, >8001 for page 1,
	  etc.  register >8003 is ignored (this page is hard-wired to >36->37).
	>8008: key buffer
	>8009->801f: ???
	>8400->9fff: sound, VDP, speech, and GROM registers (according to one
	  source, the GROM and sound registers are only available if page >3
	  is mapped in at location >c000 (register 6).  I am not sure the Speech
	  registers are implemented, though I guess they are.)
	Note that >8020->83ff is mapped to regular CPU RAM according to map
	register 4.

	Unpaged locations (native mode):
	>f100: VDP data port (read/write)
	>f102: VDP address/status port (read/write)
	>f104: VDP port 2
	>f106: VDP port 3
	>f108->f10f: VDP mirror (used by Barry Boone's converted Tomy cartridges)
	>f110->f117: memory page registers (>f110 for page 0, >f111 for page 1,
	  etc.)
	>f118: key buffer
	>f119->f11f: ???
	>f120: sound chip
	>f121->f12f: ???
	>f130->f13f: clock chip

	Unpaged locations (tms9995 locations):
	>f000->f0fb and >fffc->ffff: tms9995 RAM
	>fffa->fffb: tms9995 timer register
	Note: accessing tms9995 locations will also read/write corresponding paged
	  locations.


	CRU map:

	Base >0000: tms9901
	tms9901 pin assignment:
	int1: external interrupt (INTA on PE-bus) (connected to tms9995 (int4/EC)*
	  pin as well)
	int2: VDP interrupt
	int3-int7: joystick port inputs (fire, left, right, down, up)
	int8: keyboard interrupt (asserted when key buffer full)
	int9/p13: unused
	int10/p12: some input from mouse???  Could be third mouse button, or mouse
	  present.
	int11/p11: clock interrupt???
	int12/p10: INTB from PE-bus
	int13/p9: (used as output)
	int14/p8: unused
	int15/p7: (used as output)
	p0: PE-bus reset line
	p1: VDP reset line
	p2: joystick select (0 for joystick 1, 1 for joystick 2)
	p3-p5: unused
	p6: keyboard reset line
	p7/int15: external mem cycles 0=long, 1=short
	p9/int13: vdp wait cycles 1=add 15 cycles, 0=add none

	Base >1EE0 (>0F70): tms9995 flags and geneve mode register
	bits 0-1: tms9995 flags
	Bits 2-4: tms9995 interrupt state register
	Bits 5-15: tms9995 user flags - overlaps geneve mode, but hopefully the
	  geneve registers are write-only, so no real conflict happens
	Bit 5: 0 if NTSC, 1 if PAL video
	Bit 6: unused???
	Bit 7: some keyboard flag, set to 1 if caps is on
	Bit 8: 1 = allow keyboard clock
	Bit 9: 0 = clear keyboard input buffer, 1 = allow keyboard buffer to be
	  loaded
	Bit 10: 1 = geneve mode, 0 = ti99 mode
	Bit 11: 1 = ROM mode, 0 = map mode
	Bit 12: 0 = Enable cartridge paging
	Bit 13: 0 = protect cartridge range >6000->6fff
	Bit 14: 0 = protect cartridge range >7000->7fff
	bit 15: 1 = add 1 extra wait state when accessing 0-wait-state SRAM???
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/v9938.h"

#include "includes/geneve.h"
#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "machine/99_ide.h"
#include "devices/cartslot.h"
#include "devices/basicdsk.h"

/*
	memory map
*/

static MEMORY_READ_START (readmem)

	{0x0000, 0xffff, geneve_r},

MEMORY_END

static MEMORY_WRITE_START (writemem)

	{0x0000, 0xffff, geneve_w},

MEMORY_END


/*
	CRU map
*/

static PORT_WRITE_START(writecru)

	{0x0000, 0x07ff, tms9901_0_CRU_write},
	{0x0800, 0x0fff, geneve_peb_mode_CRU_w},

PORT_END

static PORT_READ_START(readcru)

	{0x0000, 0x00ff, tms9901_0_CRU_read},
	{0x0100, 0x01ff, geneve_peb_CRU_r},

PORT_END


/*
	Input ports, used by machine code for keyboard and joystick emulation.
*/

#define AT_KEYB_HELPER(bit, text, key1) \
	PORT_BITX( bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, text, key1, CODE_NONE )

/* 101-key PC XT keyboard + TI joysticks */
INPUT_PORTS_START(geneve)

	PORT_START	/* config */
		PORT_BITX( config_speech_mask << config_speech_bit, 1 << config_speech_bit, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_BITX( config_fdc_mask << config_fdc_bit, fdc_kind_hfdc << config_fdc_bit, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments SD" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
			PORT_DIPSETTING( fdc_kind_hfdc << config_fdc_bit, "Myarc's HFDC" )
		PORT_BITX( config_ide_mask << config_ide_bit, /*1 << config_ide_bit*/0, IPT_DIPSWITCH_NAME, "Nouspickel's IDE card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_BITX( config_rs232_mask << config_rs232_bit, /*1 << config_rs232_bit*/0, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )

	PORT_START	/* col 1: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)
		PORT_BITX(0x0007, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
			/* col 2: "wired handset 2" (= joystick 2) */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)
		PORT_BITX(0x0700, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START /* mouse buttons */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "mouse button 1", KEYCODE_NONE, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "mouse button 2", KEYCODE_NONE, IP_JOY_DEFAULT)
	/*PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "mouse button 3", KEYCODE_NONE, IP_JOY_DEFAULT)*/

	PORT_START /* Mouse - X AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - Y AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START	/* IN3 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	AT_KEYB_HELPER( 0x0002, "Esc",			KEYCODE_ESC			) /* Esc						01	81 */
	AT_KEYB_HELPER( 0x0004, "1 !",			KEYCODE_1			) /* 1							02	82 */
	AT_KEYB_HELPER( 0x0008, "2 @",			KEYCODE_2			) /* 2							03	83 */
	AT_KEYB_HELPER( 0x0010, "3 #",			KEYCODE_3			) /* 3							04	84 */
	AT_KEYB_HELPER( 0x0020, "4 $",			KEYCODE_4			) /* 4							05	85 */
	AT_KEYB_HELPER( 0x0040, "5 %",			KEYCODE_5			) /* 5							06	86 */
	AT_KEYB_HELPER( 0x0080, "6 ^",			KEYCODE_6			) /* 6							07	87 */
	AT_KEYB_HELPER( 0x0100, "7 &",			KEYCODE_7			) /* 7							08	88 */
	AT_KEYB_HELPER( 0x0200, "8 *",			KEYCODE_8			) /* 8							09	89 */
	AT_KEYB_HELPER( 0x0400, "9 (",			KEYCODE_9			) /* 9							0A	8A */
	AT_KEYB_HELPER( 0x0800, "0 )",			KEYCODE_0			) /* 0							0B	8B */
	AT_KEYB_HELPER( 0x1000, "- _",			KEYCODE_MINUS		) /* -							0C	8C */
	AT_KEYB_HELPER( 0x2000, "= +",			KEYCODE_EQUALS		) /* =							0D	8D */
	AT_KEYB_HELPER( 0x4000, "<--",			KEYCODE_BACKSPACE	) /* Backspace					0E	8E */
	AT_KEYB_HELPER( 0x8000, "Tab",			KEYCODE_TAB			) /* Tab						0F	8F */

	PORT_START	/* IN4 */
	AT_KEYB_HELPER( 0x0001, "Q",			KEYCODE_Q			) /* Q							10	90 */
	AT_KEYB_HELPER( 0x0002, "W",			KEYCODE_W			) /* W							11	91 */
	AT_KEYB_HELPER( 0x0004, "E",			KEYCODE_E			) /* E							12	92 */
	AT_KEYB_HELPER( 0x0008, "R",			KEYCODE_R			) /* R							13	93 */
	AT_KEYB_HELPER( 0x0010, "T",			KEYCODE_T			) /* T							14	94 */
	AT_KEYB_HELPER( 0x0020, "Y",			KEYCODE_Y			) /* Y							15	95 */
	AT_KEYB_HELPER( 0x0040, "U",			KEYCODE_U			) /* U							16	96 */
	AT_KEYB_HELPER( 0x0080, "I",			KEYCODE_I			) /* I							17	97 */
	AT_KEYB_HELPER( 0x0100, "O",			KEYCODE_O			) /* O							18	98 */
	AT_KEYB_HELPER( 0x0200, "P",			KEYCODE_P			) /* P							19	99 */
	AT_KEYB_HELPER( 0x0400, "[ {",			KEYCODE_OPENBRACE	) /* [							1A	9A */
	AT_KEYB_HELPER( 0x0800, "] }",			KEYCODE_CLOSEBRACE	) /* ]							1B	9B */
	AT_KEYB_HELPER( 0x1000, "Enter",		KEYCODE_ENTER		) /* Enter						1C	9C */
	AT_KEYB_HELPER( 0x2000, "L-Ctrl",		KEYCODE_LCONTROL	) /* Left Ctrl					1D	9D */
	AT_KEYB_HELPER( 0x4000, "A",			KEYCODE_A			) /* A							1E	9E */
	AT_KEYB_HELPER( 0x8000, "S",			KEYCODE_S			) /* S							1F	9F */

	PORT_START	/* IN5 */
	AT_KEYB_HELPER( 0x0001, "D",			KEYCODE_D			) /* D							20	A0 */
	AT_KEYB_HELPER( 0x0002, "F",			KEYCODE_F			) /* F							21	A1 */
	AT_KEYB_HELPER( 0x0004, "G",			KEYCODE_G			) /* G							22	A2 */
	AT_KEYB_HELPER( 0x0008, "H",			KEYCODE_H			) /* H							23	A3 */
	AT_KEYB_HELPER( 0x0010, "J",			KEYCODE_J			) /* J							24	A4 */
	AT_KEYB_HELPER( 0x0020, "K",			KEYCODE_K			) /* K							25	A5 */
	AT_KEYB_HELPER( 0x0040, "L",			KEYCODE_L			) /* L							26	A6 */
	AT_KEYB_HELPER( 0x0080, "; :",			KEYCODE_COLON		) /* ;							27	A7 */
	AT_KEYB_HELPER( 0x0100, "' \"",			KEYCODE_QUOTE		) /* '							28	A8 */
	AT_KEYB_HELPER( 0x0200, "` ~",			KEYCODE_TILDE		) /* `							29	A9 */
	AT_KEYB_HELPER( 0x0400, "L-Shift",		KEYCODE_LSHIFT		) /* Left Shift					2A	AA */
	AT_KEYB_HELPER( 0x0800, "\\ |",			KEYCODE_BACKSLASH	) /* \							2B	AB */
	AT_KEYB_HELPER( 0x1000, "Z",			KEYCODE_Z			) /* Z							2C	AC */
	AT_KEYB_HELPER( 0x2000, "X",			KEYCODE_X			) /* X							2D	AD */
	AT_KEYB_HELPER( 0x4000, "C",			KEYCODE_C			) /* C							2E	AE */
	AT_KEYB_HELPER( 0x8000, "V",			KEYCODE_V			) /* V							2F	AF */

	PORT_START	/* IN6 */
	AT_KEYB_HELPER( 0x0001, "B",			KEYCODE_B			) /* B							30	B0 */
	AT_KEYB_HELPER( 0x0002, "N",			KEYCODE_N			) /* N							31	B1 */
	AT_KEYB_HELPER( 0x0004, "M",			KEYCODE_M			) /* M							32	B2 */
	AT_KEYB_HELPER( 0x0008, ", <",			KEYCODE_COMMA		) /* ,							33	B3 */
	AT_KEYB_HELPER( 0x0010, ". >",			KEYCODE_STOP		) /* .							34	B4 */
	AT_KEYB_HELPER( 0x0020, "/ ?",			KEYCODE_SLASH		) /* /							35	B5 */
	AT_KEYB_HELPER( 0x0040, "R-Shift",		KEYCODE_RSHIFT		) /* Right Shift				36	B6 */
	AT_KEYB_HELPER( 0x0080, "KP * (PrtScr)",KEYCODE_ASTERISK	) /* Keypad *  (PrtSc)			37	B7 */
	AT_KEYB_HELPER( 0x0100, "Alt",			KEYCODE_LALT		) /* Left Alt					38	B8 */
	AT_KEYB_HELPER( 0x0200, "Space",		KEYCODE_SPACE		) /* Space						39	B9 */
	AT_KEYB_HELPER( 0x0400, "Caps",			KEYCODE_CAPSLOCK	) /* Caps Lock					3A	BA */
	AT_KEYB_HELPER( 0x0800, "F1",			KEYCODE_F1			) /* F1							3B	BB */
	AT_KEYB_HELPER( 0x1000, "F2",			KEYCODE_F2			) /* F2							3C	BC */
	AT_KEYB_HELPER( 0x2000, "F3",			KEYCODE_F3			) /* F3							3D	BD */
	AT_KEYB_HELPER( 0x4000, "F4",			KEYCODE_F4			) /* F4							3E	BE */
	AT_KEYB_HELPER( 0x8000, "F5",			KEYCODE_F5			) /* F5							3F	BF */

	PORT_START	/* IN7 */
	AT_KEYB_HELPER( 0x0001, "F6",			KEYCODE_F6			) /* F6							40	C0 */
	AT_KEYB_HELPER( 0x0002, "F7",			KEYCODE_F7			) /* F7							41	C1 */
	AT_KEYB_HELPER( 0x0004, "F8",			KEYCODE_F8			) /* F8							42	C2 */
	AT_KEYB_HELPER( 0x0008, "F9",			KEYCODE_F9			) /* F9							43	C3 */
	AT_KEYB_HELPER( 0x0010, "F10",			KEYCODE_F10			) /* F10						44	C4 */
	AT_KEYB_HELPER( 0x0020, "NumLock",		KEYCODE_NUMLOCK		) /* Num Lock					45	C5 */
	AT_KEYB_HELPER( 0x0040, "ScrLock",		KEYCODE_SCRLOCK		) /* Scroll Lock				46	C6 */
	AT_KEYB_HELPER( 0x0080, "KP 7 (Home)",	KEYCODE_7_PAD		) /* Keypad 7  (Home)			47	C7 */
	AT_KEYB_HELPER( 0x0100, "KP 8 (Up)",	KEYCODE_8_PAD		) /* Keypad 8  (Up arrow)		48	C8 */
	AT_KEYB_HELPER( 0x0200, "KP 9 (PgUp)",	KEYCODE_9_PAD		) /* Keypad 9  (PgUp)			49	C9 */
	AT_KEYB_HELPER( 0x0400, "KP -",			KEYCODE_MINUS_PAD	) /* Keypad -					4A	CA */
	AT_KEYB_HELPER( 0x0800, "KP 4 (Left)",	KEYCODE_4_PAD		) /* Keypad 4  (Left arrow)		4B	CB */
	AT_KEYB_HELPER( 0x1000, "KP 5",			KEYCODE_5_PAD		) /* Keypad 5					4C	CC */
	AT_KEYB_HELPER( 0x2000, "KP 6 (Right)",	KEYCODE_6_PAD		) /* Keypad 6  (Right arrow)	4D	CD */
	AT_KEYB_HELPER( 0x4000, "KP +",			KEYCODE_PLUS_PAD	) /* Keypad +					4E	CE */
	AT_KEYB_HELPER( 0x8000, "KP 1 (End)",	KEYCODE_1_PAD		) /* Keypad 1  (End)			4F	CF */

	PORT_START	/* IN8 */
	AT_KEYB_HELPER( 0x0001, "KP 2 (Down)",	KEYCODE_2_PAD		) /* Keypad 2  (Down arrow)		50	D0 */
	AT_KEYB_HELPER( 0x0002, "KP 3 (PgDn)",	KEYCODE_3_PAD		) /* Keypad 3  (PgDn)			51	D1 */
	AT_KEYB_HELPER( 0x0004, "KP 0 (Ins)",	KEYCODE_0_PAD		) /* Keypad 0  (Ins)			52	D2 */
	AT_KEYB_HELPER( 0x0008, "KP . (Del)",	KEYCODE_DEL_PAD		) /* Keypad .  (Del)			53	D3 */
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )
	AT_KEYB_HELPER( 0x0040, "(84/102)\\",	KEYCODE_BACKSLASH2	) /* Backslash 2				56	D6 */
	AT_KEYB_HELPER( 0x0080, "(MF2)F11",		KEYCODE_F11			) /* F11						57	D7 */
	AT_KEYB_HELPER( 0x0100, "(MF2)F12",		KEYCODE_F12			) /* F12						58	D8 */
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )

	PORT_START	/* IN9 */
	AT_KEYB_HELPER( 0x0001, "(MF2)KP Enter",	KEYCODE_ENTER_PAD	) /* PAD Enter					60	e0 */
	AT_KEYB_HELPER( 0x0002, "(MF2)R-Control",	KEYCODE_RCONTROL	) /* Right Control				61	e1 */
	AT_KEYB_HELPER( 0x0004, "(MF2)KP /",		KEYCODE_SLASH_PAD	) /* PAD Slash					62	e2 */
	AT_KEYB_HELPER( 0x0008, "(MF2)PRTSCR",		KEYCODE_PRTSCR		) /* Print Screen				63	e3 */
	AT_KEYB_HELPER( 0x0010, "(MF2)ALTGR",		KEYCODE_RALT		) /* ALTGR						64	e4 */
	AT_KEYB_HELPER( 0x0020, "(MF2)Home",		KEYCODE_HOME		) /* Home						66	e6 */
	AT_KEYB_HELPER( 0x0040, "(MF2)Cursor Up",	KEYCODE_UP			) /* Up							67	e7 */
	AT_KEYB_HELPER( 0x0080, "(MF2)Page Up",		KEYCODE_PGUP		) /* Page Up					68	e8 */
	AT_KEYB_HELPER( 0x0100, "(MF2)Cursor Left",	KEYCODE_LEFT		) /* Left						69	e9 */
	AT_KEYB_HELPER( 0x0200, "(MF2)Cursor Right",KEYCODE_RIGHT		) /* Right						6a	ea */
	AT_KEYB_HELPER( 0x0400, "(MF2)End",			KEYCODE_END			) /* End						6b	eb */
	AT_KEYB_HELPER( 0x0800, "(MF2)Cursor Down",	KEYCODE_DOWN		) /* Down						6c	ec */
	AT_KEYB_HELPER( 0x1000, "(MF2)Page Down",	KEYCODE_PGDN		) /* Page Down					6d	ed */
	AT_KEYB_HELPER( 0x2000, "(MF2)Insert",		KEYCODE_INSERT		) /* Insert						6e	ee */
	AT_KEYB_HELPER( 0x4000, "(MF2)Delete",		KEYCODE_DEL			) /* Delete						6f	ef */
	AT_KEYB_HELPER( 0x8000, "(MF2)Pause",		KEYCODE_PAUSE		) /* Pause						65	e5 */

	PORT_START	/* IN10 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
#if 0
	AT_KEYB_HELPER( 0x0001, "Print Screen", KEYCODE_PRTSCR		) /* Print Screen alternate		77	f7 */
	AT_KEYB_HELPER( 0x2000, "Left Win",		CODE_NONE			) /* Left Win					7d	fd */
	AT_KEYB_HELPER( 0x4000, "Right Win",	CODE_NONE			) /* Right Win					7e	fe */
	AT_KEYB_HELPER( 0x8000, "Menu",			CODE_NONE			) /* Menu						7f	ff */
#endif

INPUT_PORTS_END


/*
	SN76496 (compatible with tms9919/SN76489) sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. */
	{ 75 }			/* Volume.  I don't know the best value. */
};

static struct TMS5220interface tms5220interface =
{
	680000L,					/* 640kHz -> 8kHz output */
	50,							/* Volume.  I don't know the best value. */
	NULL,						/* no IRQ callback */
#if 1
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
#endif
};


static MACHINE_DRIVER_START(geneve_60hz)

	/* basic machine hardware */
	/* TMS9995 CPU @ 12.0 MHz */
	MDRV_CPU_ADD(TMS9995, 12000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(geneve_hblank_interrupt, 262)	/* 262.5 in 60Hz, 312.5 in 50Hz */
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)	/* or 50Hz */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( geneve )
	MDRV_MACHINE_STOP( geneve )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(512+32, (212+16)*2)
	MDRV_VISIBLE_AREA(0, 512+32 - 1, 0, (212+16)*2 - 1)

	/*MDRV_GFXDECODE(NULL)*/
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(v9938)
	MDRV_VIDEO_START(geneve)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(v9938)

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)

MACHINE_DRIVER_END


/*
	ROM loading

	Note that we use the same ROMset for 50Hz and 60Hz versions.
*/

ROM_START(geneve)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, REGION_CPU1, 0)
	ROM_LOAD("genbt100.bin", offset_rom_geneve, 0x4000, CRC(8001e386)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f)) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7)) /* system speech ROM */
ROM_END

ROM_START(genmod)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, REGION_CPU1, 0)
	ROM_LOAD("gnmbt100.bin", offset_rom_geneve, 0x4000, CRC(19b89479)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f)) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7)) /* system speech ROM */
ROM_END

SYSTEM_CONFIG_START(geneve)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",										device_load_ti99_floppy)
	CONFIG_DEVICE_LEGACY			(IO_HARDDISK, 	/*4*/3, "hd\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_OR_READ,device_init_ti99_hd, NULL, device_load_ti99_hd, device_unload_ti99_hd, NULL)
	/*CONFIG_DEVICE_LEGACY			(IO_PARALLEL,	1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	ti99_4_pio_load,	ti99_4_pio_unload,		NULL)
	CONFIG_DEVICE_LEGACY			(IO_SERIAL,		1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	ti99_4_rs232_load,	ti99_4_rs232_unload,	NULL)*/
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE		 INPUT	  INIT		CONFIG	COMPANY		FULLNAME */
COMP( 1987?,geneve,   0,		0,		geneve_60hz,  geneve,  geneve,	geneve,	"Myarc",	"Geneve 9640" )
COMP( 199??,genmod,   geneve,	0,		geneve_60hz,  geneve,  genmod,	geneve,	"Myarc",	"Geneve 9640 (with Genmod modification)" )
