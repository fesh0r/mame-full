/*
	MESS Driver for TI-99/4 and TI-99/4A Home Computers.
	Raphael Nabet, 1999-2002.

	see machine/ti99_4x.c for some details and references

	NOTE!!!!!!!!!!  Until the new TMS5220 core is added, you should uncomment three lines
	in tms5220interface like this :

	static struct TMS5220interface tms5220interface =
	{
		680000L,					// 640kHz -> 8kHz output
		50,							// Volume.  I don't know the best value.
		NULL,						// no IRQ callback
	#if 0	// 1 with new tms5220 core
		spchroms_read,				// speech ROM read handler
		spchroms_load_address,		// speech ROM load address handler
		spchroms_read_and_branch	// speech ROM read and branch handler
	#endif
	};
*/

/*
	TI99/4 preliminary infos:

Similar to TI99/4a, except for the following:
	* tms9918/9928 has no bitmap mode
	* smaller, 40-key keyboard
	* many small differences in the contents of system ROMs

Historical notes: TI made several last minute design changes.
	* TI99/4 prototypes had an extra port for an I/R joystick and keypad interface.
	* early TI99/4 prototypes were designed for a tms9985, not a tms9900.
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/tms9928a.h"
#include "vidhrdw/v9938.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/994x_ser.h"
#include "machine/99_ide.h"
#include "devices/cartslot.h"
#include "devices/basicdsk.h"

/*
	memory map
*/

static MEMORY_READ16_START (readmem)

	{ 0x0000, 0x1fff, MRA16_ROM },			/*system ROM*/
	{ 0x2000, 0x3fff, ti99_rw_null8bits },	/*lower 8kb of RAM extension - installed dynamically*/
	{ 0x4000, 0x5fff, ti99_rw_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_rw_cartmem },	/*cartridge memory*/
	{ 0x8000, 0x80ff, MRA16_BANK1 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8100, 0x81ff, MRA16_BANK2 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8200, 0x82ff, MRA16_BANK3 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8300, 0x83ff, MRA16_BANK4 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_rw_null8bits },	/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_rw_rvdp },		/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_rw_null8bits },	/*vdp write*/
	{ 0x9000, 0x93ff, ti99_rw_null8bits },	/*speech read - installed dynamically*/
	{ 0x9400, 0x97ff, ti99_rw_null8bits },	/*speech write*/
	{ 0x9800, 0x9bff, ti99_rw_rgpl },		/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_rw_null8bits },	/*GPL write*/
	{ 0xa000, 0xffff, ti99_rw_null8bits },	/*upper 24kb of RAM extension - installed dynamically*/

MEMORY_END

static MEMORY_WRITE16_START (writemem)

	{ 0x0000, 0x1fff, MWA16_ROM },			/*system ROM*/
	{ 0x2000, 0x3fff, ti99_ww_null8bits },	/*lower 8kb of RAM extension - installed dynamically*/
	{ 0x4000, 0x5fff, ti99_ww_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_ww_cartmem },	/*cartridge memory (some carts include RAM or a pager chip)*/
	{ 0x8000, 0x80ff, MWA16_BANK1 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8100, 0x81ff, MWA16_BANK2 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8200, 0x82ff, MWA16_BANK3 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8300, 0x83ff, MWA16_BANK4 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_ww_wsnd },		/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_ww_null8bits },	/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_ww_wvdp },		/*vdp write*/
	{ 0x9000, 0x93ff, ti99_ww_null8bits },	/*speech read*/
	{ 0x9400, 0x97ff, ti99_ww_null8bits },	/*speech write - installed dynamically*/
	{ 0x9800, 0x9bff, ti99_ww_null8bits },	/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_ww_wgpl },		/*GPL write*/
	{ 0xa000, 0xffff, ti99_ww_null8bits },	/*upper 24kb of RAM extension - installed dynamically*/

MEMORY_END


static MEMORY_READ16_START (readmem_4ev)

	{ 0x0000, 0x1fff, MRA16_ROM },			/*system ROM*/
	{ 0x2000, 0x3fff, ti99_rw_null8bits },	/*lower 8kb of RAM extension - installed dynamically*/
	{ 0x4000, 0x5fff, ti99_rw_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_rw_cartmem },	/*cartridge memory*/
	{ 0x8000, 0x80ff, MRA16_BANK1 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8100, 0x81ff, MRA16_BANK2 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8200, 0x82ff, MRA16_BANK3 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8300, 0x83ff, MRA16_BANK4 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_rw_null8bits },	/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_rw_rv38 },		/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_rw_null8bits },	/*vdp write*/
	{ 0x9000, 0x93ff, ti99_rw_null8bits },	/*speech read - installed dynamically*/
	{ 0x9400, 0x97ff, ti99_rw_null8bits },	/*speech write*/
	{ 0x9800, 0x9bff, ti99_rw_rgpl },		/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_rw_null8bits },	/*GPL write*/
	{ 0xa000, 0xffff, ti99_rw_null8bits },	/*upper 24kb of RAM extension - installed dynamically*/

MEMORY_END

static MEMORY_WRITE16_START (writemem_4ev)

	{ 0x0000, 0x1fff, MWA16_ROM },			/*system ROM*/
	{ 0x2000, 0x3fff, ti99_ww_null8bits },	/*lower 8kb of RAM extension - installed dynamically*/
	{ 0x4000, 0x5fff, ti99_ww_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_ww_cartmem },	/*cartridge memory (some carts include RAM or a pager chip)*/
	{ 0x8000, 0x80ff, MWA16_BANK1 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8100, 0x81ff, MWA16_BANK2 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8200, 0x82ff, MWA16_BANK3 },		/*RAM PAD, mirrors 0x8300-0x83ff*/
	{ 0x8300, 0x83ff, MWA16_BANK4 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_ww_wsnd },		/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_ww_null8bits },	/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_ww_wv38 },		/*vdp write*/
	{ 0x9000, 0x93ff, ti99_ww_null8bits },	/*speech read*/
	{ 0x9400, 0x97ff, ti99_ww_null8bits },	/*speech write - installed dynamically*/
	{ 0x9800, 0x9bff, ti99_ww_null8bits },	/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_ww_wgpl },		/*GPL write*/
	{ 0xa000, 0xffff, ti99_ww_null8bits },	/*upper 24kb of RAM extension - installed dynamically*/

MEMORY_END

/*
	CRU map
*/

static PORT_WRITE16_START(writecru)

	{0x0000<<1, 0x07ff<<1, tms9901_0_CRU_write16},
	{0x0800<<1, 0x0fff<<1, ti99_expansion_CRU_w},

PORT_END

static PORT_READ16_START(readcru)

	{0x0000<<1, 0x00ff<<1, tms9901_0_CRU_read16},
	{0x0100<<1, 0x01ff<<1, ti99_expansion_CRU_r},

PORT_END


/*
	Input ports, used by machine code for TI keyboard and joystick emulation.
*/

/* TI99/4a: 48-key keyboard, plus two optional joysticks (2 shift keys) */
INPUT_PORTS_START(ti99_4a)

	PORT_START	/* config */
		PORT_BITX( config_xRAM_mask << config_xRAM_bit, xRAM_kind_TI << config_xRAM_bit, IPT_DIPSWITCH_NAME, "RAM extension", KEYCODE_NONE, IP_JOY_NONE )
		    PORT_DIPSETTING( xRAM_kind_none << config_xRAM_bit,				"none" )
		    PORT_DIPSETTING( xRAM_kind_TI << config_xRAM_bit,				"Texas Instruments 32kb")
		    PORT_DIPSETTING( xRAM_kind_super_AMS << config_xRAM_bit,		"Super AMS 1Mb")
		    PORT_DIPSETTING( xRAM_kind_foundation_128k << config_xRAM_bit,	"Foundation 128kb")
		    PORT_DIPSETTING( xRAM_kind_foundation_512k << config_xRAM_bit,	"Foundation 512kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_128k << config_xRAM_bit,		"Myarc look-alike 128kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_512k << config_xRAM_bit,		"Myarc look-alike 512kb")
		PORT_BITX( config_speech_mask << config_speech_bit, 1 << config_speech_bit, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_BITX( config_fdc_mask << config_fdc_bit, fdc_kind_BwG << config_fdc_bit, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
		PORT_BITX( config_ide_mask << config_ide_bit, /*0*/1 << config_ide_bit, IPT_DIPSWITCH_NAME, "Nouspickel's IDE card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_BITX( config_rs232_mask << config_rs232_bit, 1 << config_rs232_bit, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_BITX(0x0088, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		/* The original control key is located on the left, but we accept the
		right control key as well */
		PORT_BITX(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE,		UCHAR_SHIFT_1)
		/* TI99/4a has a second shift key which maps the same */
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE,		UCHAR_SHIFT_1)
		/* The original function key is located on the right, but we accept the
		left alt key as well */
		PORT_KEY1(0x0010, IP_ACTIVE_LOW, "FCTN", KEYCODE_RALT, KEYCODE_LALT,		UCHAR_SHIFT_2)
		PORT_KEY1(0x0004, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, IP_JOY_NONE,		13)
		PORT_KEY1(0x0002, IP_ACTIVE_LOW, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE,		' ')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE,	'=',	'+')
				/* col 1 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE,		'x',	'X')
		PORT_KEY3(0x4000, IP_ACTIVE_LOW, "w W ~", KEYCODE_W, IP_JOY_NONE,			'w',	'W',	'~')
		PORT_KEY2(0x2000, IP_ACTIVE_LOW, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE,		's',	'S')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW, "2 @ INS", KEYCODE_2, IP_JOY_NONE,			'2',	'@')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "9 ( BACK", KEYCODE_9, IP_JOY_NONE,		'9',	'(')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "o O '", KEYCODE_O, IP_JOY_NONE,			'o',	'O',	'\'')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "l L", KEYCODE_L, IP_JOY_NONE,				'l',	'L')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, ". >", KEYCODE_STOP, IP_JOY_NONE,			'.',	'>')

	PORT_START	/* col 2 */
		PORT_KEY3(0x0080, IP_ACTIVE_LOW, "c C `", KEYCODE_C, IP_JOY_NONE,			'c',	'C',	'`')
		PORT_KEY2(0x0040, IP_ACTIVE_LOW, "e E (UP)", KEYCODE_E, IP_JOY_NONE,		'e',	'E')
		PORT_KEY2(0x0020, IP_ACTIVE_LOW, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE,		'd',	'D')
		PORT_KEY2(0x0010, IP_ACTIVE_LOW, "3 # ERASE", KEYCODE_3, IP_JOY_NONE,		'3',	'#')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "8 * REDO", KEYCODE_8, IP_JOY_NONE,		'8',	'*')
		PORT_KEY3(0x0004, IP_ACTIVE_LOW, "i I ?", KEYCODE_I, IP_JOY_NONE,			'i',	'I',	'?')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "k K", KEYCODE_K, IP_JOY_NONE,				'k',	'K')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, ", <", KEYCODE_COMMA, IP_JOY_NONE,			',',	'<')
				/* col 3 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "v V", KEYCODE_V, IP_JOY_NONE,				'v',	'V')
		PORT_KEY3(0x4000, IP_ACTIVE_LOW, "r R [", KEYCODE_R, IP_JOY_NONE,			'r',	'R',	'[')
		PORT_KEY3(0x2000, IP_ACTIVE_LOW, "f F {", KEYCODE_F, IP_JOY_NONE,			'f',	'F',	'{')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE,		'4',	'$')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "7 & AID", KEYCODE_7, IP_JOY_NONE,			'7',	'&')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "u U _", KEYCODE_U, IP_JOY_NONE,			'u',	'U',	'_')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "j J", KEYCODE_J, IP_JOY_NONE,				'j',	'J')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, "m M", KEYCODE_M, IP_JOY_NONE,				'm',	'M')

	PORT_START	/* col 4 */
		PORT_KEY2(0x0080, IP_ACTIVE_LOW, "b B", KEYCODE_B, IP_JOY_NONE,				'b',	'B')
		PORT_KEY3(0x0040, IP_ACTIVE_LOW, "t T ]", KEYCODE_T, IP_JOY_NONE,			't',	'T',	']')
		PORT_KEY3(0x0020, IP_ACTIVE_LOW, "g G }", KEYCODE_G, IP_JOY_NONE,			'g',	'G',	'}')
		PORT_KEY2(0x0010, IP_ACTIVE_LOW, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE,		'5',	'%')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE,		'6',	'^')
		PORT_KEY2(0x0004, IP_ACTIVE_LOW, "y Y", KEYCODE_Y, IP_JOY_NONE,				'y',	'Y')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "h H", KEYCODE_H, IP_JOY_NONE,				'h',	'H')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "n N", KEYCODE_N, IP_JOY_NONE,				'n',	'N')
				/* col 5 */
		PORT_KEY3(0x8000, IP_ACTIVE_LOW, "z Z \\", KEYCODE_Z, IP_JOY_NONE,			'z',	'Z',	'\\')
		PORT_KEY2(0x4000, IP_ACTIVE_LOW, "q Q", KEYCODE_Q, IP_JOY_NONE,				'q',	'Q')
		PORT_KEY3(0x2000, IP_ACTIVE_LOW, "a A |", KEYCODE_A, IP_JOY_NONE,			'a',	'A',	'|')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW,  "1 ! DEL", KEYCODE_1, IP_JOY_NONE,		'1',	'!')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "0 )", KEYCODE_0, IP_JOY_NONE,				'0',	')')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "p P \"", KEYCODE_P, IP_JOY_NONE,			'p',	'P',	'\"')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "; :", KEYCODE_COLON, IP_JOY_NONE,			';',	':')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, "/ -", KEYCODE_SLASH, IP_JOY_NONE,			'/',	'-')

	PORT_START	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BITX(0x00E0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)
			/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BITX(0xE000, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

	PORT_START	/* one more port for Alpha line */
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END


#define JOYSTICK_DELTA			10
#define JOYSTICK_SENSITIVITY	100

/* TI99/4: 41-key keyboard, plus two optional joysticks  (2 space keys) */
INPUT_PORTS_START(ti99_4)

	PORT_START	/* config */
		PORT_BITX( config_xRAM_mask << config_xRAM_bit, xRAM_kind_TI << config_xRAM_bit, IPT_DIPSWITCH_NAME, "RAM extension", KEYCODE_NONE, IP_JOY_NONE )
		    PORT_DIPSETTING( xRAM_kind_none << config_xRAM_bit,				"none" )
		    PORT_DIPSETTING( xRAM_kind_TI << config_xRAM_bit,				"Texas Instruments 32kb")
		    PORT_DIPSETTING( xRAM_kind_super_AMS << config_xRAM_bit,		"Super AMS 1Mb")
		    PORT_DIPSETTING( xRAM_kind_foundation_128k << config_xRAM_bit,	"Foundation 128kb")
		    PORT_DIPSETTING( xRAM_kind_foundation_512k << config_xRAM_bit,	"Foundation 512kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_128k << config_xRAM_bit,		"Myarc look-alike 128kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_512k << config_xRAM_bit,		"Myarc look-alike 512kb")
		PORT_BITX( config_speech_mask << config_speech_bit, 1 << config_speech_bit, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_BITX( config_fdc_mask << config_fdc_bit, fdc_kind_BwG << config_fdc_bit, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
		PORT_BITX( config_ide_mask << config_ide_bit, 0/*1 << config_ide_bit*/, IPT_DIPSWITCH_NAME, "Nouspickel's IDE card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_BITX( config_rs232_mask << config_rs232_bit, 1 << config_rs232_bit, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )
		PORT_BITX( config_handsets_mask << config_handsets_bit, /*1 << config_handsets_bit*/0, IPT_DIPSWITCH_NAME, "I/R remote handset", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_handsets_bit, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_KEY2(0x0080, IP_ACTIVE_LOW, "1 !", KEYCODE_1, IP_JOY_NONE,			'1',	'!')
		PORT_KEY1(0x0040, IP_ACTIVE_LOW, "Q QUIT", KEYCODE_Q, IP_JOY_NONE,		'Q')
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE,	' ')
		/* TI99/4 has a second space key which maps the same */
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "SPACE", KEYCODE_CAPSLOCK, IP_JOY_NONE,' ')
		PORT_KEY1(0x0010, IP_ACTIVE_LOW, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE,	UCHAR_SHIFT_1)
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "0 )", KEYCODE_0, IP_JOY_NONE,			'0',	')')
		PORT_KEY2(0x0004, IP_ACTIVE_LOW, "P \"", KEYCODE_P, IP_JOY_NONE,		'P',	'"')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "L =", KEYCODE_L, IP_JOY_NONE,			'L',	'=')
		PORT_KEY1(0x0001, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, IP_JOY_NONE,	13)
				/* col 1 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "2 @", KEYCODE_2, IP_JOY_NONE,			'2',	'@')
		PORT_KEY1(0x4000, IP_ACTIVE_LOW, "W BEGIN", KEYCODE_W, IP_JOY_NONE,		'W')
		PORT_KEY1(0x2000, IP_ACTIVE_LOW, "A AID", KEYCODE_A, IP_JOY_NONE,		'A')
		PORT_KEY1(0x1000, IP_ACTIVE_LOW, "Z BACK", KEYCODE_Z, IP_JOY_NONE,		'Z')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "9 (", KEYCODE_9, IP_JOY_NONE,			'9',	'(')
		PORT_KEY2(0x0400, IP_ACTIVE_LOW, "O +", KEYCODE_O, IP_JOY_NONE,			'O',	'+')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "K /", KEYCODE_K, IP_JOY_NONE,			'K',	'/')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, ", .", KEYCODE_STOP, IP_JOY_NONE,		',',	'.')

	PORT_START	/* col 2 */
		PORT_KEY2(0x0080, IP_ACTIVE_LOW, "3 #", KEYCODE_3, IP_JOY_NONE,			'3',	'#')
		PORT_KEY1(0x0040, IP_ACTIVE_LOW, "E UP", KEYCODE_E, IP_JOY_NONE,		'E')
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "S LEFT", KEYCODE_S, IP_JOY_NONE,		'S')
		PORT_KEY1(0x0010, IP_ACTIVE_LOW, "X DOWN", KEYCODE_X, IP_JOY_NONE,		'X')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "8 *", KEYCODE_8, IP_JOY_NONE,			'8',	'*')
		PORT_KEY2(0x0004, IP_ACTIVE_LOW, "I -", KEYCODE_I, IP_JOY_NONE,			'I',	'-')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "J ^", KEYCODE_J, IP_JOY_NONE,			'J',	'^')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "M ;", KEYCODE_M, IP_JOY_NONE,			'M',	';')
				/* col 3 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "4 $", KEYCODE_4, IP_JOY_NONE,			'4',	'$')
		PORT_KEY1(0x4000, IP_ACTIVE_LOW, "R REDO", KEYCODE_R, IP_JOY_NONE,		'R')
		PORT_KEY1(0x2000, IP_ACTIVE_LOW, "D RIGHT", KEYCODE_D, IP_JOY_NONE,		'D')
		PORT_KEY1(0x1000, IP_ACTIVE_LOW, "C CLEAR", KEYCODE_C, IP_JOY_NONE,		'C')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "7 &", KEYCODE_7, IP_JOY_NONE,			'7',	'&')
		PORT_KEY2(0x0400, IP_ACTIVE_LOW, "U _", KEYCODE_U, IP_JOY_NONE,			'U',	'_')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "H <", KEYCODE_H, IP_JOY_NONE,			'H',	'<')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, "N :", KEYCODE_N, IP_JOY_NONE,			'N',	':')

	PORT_START	/* col 4 */
		PORT_KEY2(0x0080, IP_ACTIVE_LOW, "5 %", KEYCODE_5, IP_JOY_NONE,			'5',	'%')
		PORT_KEY1(0x0040, IP_ACTIVE_LOW, "T ERASE", KEYCODE_T, IP_JOY_NONE,		'T')
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "F DEL", KEYCODE_F, IP_JOY_NONE,		'F')
		PORT_KEY1(0x0010, IP_ACTIVE_LOW, "V PROC'D", KEYCODE_V, IP_JOY_NONE,	'V')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "6 ^", KEYCODE_6, IP_JOY_NONE,			'6',	'\'')
		PORT_KEY2(0x0004, IP_ACTIVE_LOW, "Y >", KEYCODE_Y, IP_JOY_NONE,			'Y',	'>')
		PORT_KEY1(0x0002, IP_ACTIVE_LOW, "G INS", KEYCODE_G, IP_JOY_NONE,		'G')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "B ?", KEYCODE_B, IP_JOY_NONE,			'B',	'?')
				/* col 5: "wired handset 1" (= joystick 1) */
		PORT_BITX(0xE000, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

	PORT_START	/* col 6: "wired handset 2" (= joystick 2) */
		PORT_BITX(0x00E0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)
				/* col 7: never used (selects IR remote handset instead) */
		/*PORT_BITX(0xFF00, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)*/


	/* 13 pseudo-ports for IR remote handsets */

	/* 8 pseudo-ports for the 4 IR joysticks */
	PORT_START	/* joystick 1, X axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_X | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 1, Y axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_CENTER | IPF_REVERSE, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 2, X axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_X | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 2, Y axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 3, X axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_X | IPF_PLAYER3 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 3, Y axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_Y | IPF_PLAYER3 | IPF_CENTER | IPF_REVERSE, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 4, X axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_X | IPF_PLAYER4 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	PORT_START	/* joystick 4, Y axis */
		PORT_ANALOG( 0xf, 0x7,  IPT_AD_STICK_Y | IPF_PLAYER4 | IPF_CENTER | IPF_REVERSE, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xe )

	/* 5 pseudo-ports for the 4 IR remote keypads */
	PORT_START	/* keypad 1, keys 1 to 16 */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: CLR",  KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: GO",   KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: SET",  KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: NEXT", KEYCODE_LSHIFT, IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 7",    KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 4",    KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 1",    KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: STOP", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 8",    KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 5",    KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 2",    KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 0",    KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 9",    KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 6",    KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: 3",    KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: E =",  KEYCODE_C, IP_JOY_NONE)

	PORT_START	/* keypad 1, keys 17 to 20 */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: (div)",KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: (mul)",KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: NO -", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER1, "1: YES +",KEYCODE_V, IP_JOY_NONE)
				/* keypad 2, keys 1 to 12 */
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: CLR",  KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: GO",   KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: SET",  KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: NEXT", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 7",    KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 4",    KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 1",    KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: STOP", KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 8",    KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 5",    KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 2",    KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 0",    KEYCODE_M, IP_JOY_NONE)

	PORT_START	/* keypad 2, keys 13 to 20 */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 9",    KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 6",    KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: 3",    KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: E =",  KEYCODE_STOP, IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: (div)",KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: (mul)",KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: NO -", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER2, "2: YES +",KEYCODE_ENTER, IP_JOY_NONE)
				/* keypad 3, keys 1 to 8 */
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: CLR",  KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: GO",   KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: SET",  KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: NEXT", KEYCODE_LSHIFT, IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 7",    KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 4",    KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 1",    KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: STOP", KEYCODE_Z, IP_JOY_NONE)

	PORT_START	/* keypad 3, keys 9 to 20 */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 8",    KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 5",    KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 2",    KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 0",    KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 9",    KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 6",    KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: 3",    KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: E =",  KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: (div)",KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: (mul)",KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: NO -", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER3, "3: YES +",KEYCODE_V, IP_JOY_NONE)
				/* keypad 4, keys 1 to 4 */
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: CLR",  KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: GO",   KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: SET",  KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: NEXT", KEYCODE_B, IP_JOY_NONE)

	PORT_START	/* keypad 4, keys 5 to 20 */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 7",    KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 4",    KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 1",    KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: STOP", KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 8",    KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 5",    KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 2",    KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 0",    KEYCODE_M, IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 9",    KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 6",    KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: 3",    KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: E =",  KEYCODE_STOP, IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: (div)",KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: (mul)",KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: NO -", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_PLAYER4, "4: YES +",KEYCODE_ENTER, IP_JOY_NONE)

INPUT_PORTS_END

/*
	TMS9919 (a.k.a. SN76489) sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. connected to the TMS9918A CPUCLK pin... */
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

/*
	we use a DAC to emulate "audio gate", even thought
	a) there was no DAC in an actual TI99
	b) this is a 2-level output (whereas a DAC provides a 256-level output...)
*/
static struct DACinterface aux_sound_intf =
{
	1,				/* total number of DACs */
	{
		20			/* volume for audio gate*/
	}
};

/*
	2 tape units
*/
static struct Wave_interface tape_input_intf =
{
	2,
	{
		20,			/* Volume for CS1 */
		20			/* Volume for CS2 */
	}
};


static const TMS9928a_interface tms9918_interface =
{
	TMS99x8,
	0x4000,
	tms9901_set_int2
};

static const TMS9928a_interface tms9929_interface =
{
	TMS9929,
	0x4000,
	tms9901_set_int2
};

static const TMS9928a_interface tms9918a_interface =
{
	TMS99x8A,
	0x4000,
	tms9901_set_int2
};

static const TMS9928a_interface tms9929a_interface =
{
	TMS9929A,
	0x4000,
	tms9901_set_int2
};

static MACHINE_DRIVER_START(ti99_4_60hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9918_interface )

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_4_50hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9929_interface )

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_4a_60hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9918a_interface )

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_4a_50hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9929a_interface )

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_4ev_60hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem_4ev, writemem_4ev)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_4ev_hblank_interrupt, 263)	/* 262.5 in 60Hz, 312.5 in 50Hz */
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)	/* or 50Hz */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
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
	MDRV_VIDEO_START(ti99_4ev)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(v9938)

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END


/*
	ROM loading

	Note that we use the same ROMset for 50Hz and 60Hz versions.
*/

ROM_START(ti99_4)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_BYTE("u610.bin", 0x0000, 0x1000, 0x6fcf4b15) /* CPU ROMs high */
	ROM_LOAD16_BYTE("u611.bin", 0x0001, 0x1000, 0x491c21d1) /* CPU ROMs low */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("u500.bin", 0x0000, 0x1800, 0xaa757e13) /* system GROM 0 */
	ROM_LOAD("u501.bin", 0x2000, 0x1800, 0xc863e460) /* system GROM 1 */
	ROM_LOAD("u502.bin", 0x4000, 0x1800, 0xb0eda548) /* system GROM 1 */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, 0x8f7df93f) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, 0x06f1ec89) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, 0x66fbe0ed) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, 0xeab382fb) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

ROM_START(ti99_4a)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_WORD("994arom.bin", 0x0000, 0x2000, 0xdb8f33e5) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("994agrom.bin", 0x0000, 0x6000, 0xaf5c2449) /* system GROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, 0x8f7df93f) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, 0x06f1ec89) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, 0x66fbe0ed) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, 0xeab382fb) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

ROM_START(ti99_4ev)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_WORD("994arom.bin", 0x0000, 0x2000, 0xdb8f33e5) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("994agrom.bin", 0x0000, 0x6000, 0xaf5c2449) /* system GROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, 0x8f7df93f) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, 0x06f1ec89) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, 0x66fbe0ed) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, 0xeab382fb) /* TI rs232 DSR ROM */
	ROM_LOAD("evpcdsr.bin", offset_evpc_dsr, 0x10000, 0xa062b75d) /* evpc DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

/* a TI99 console only had one cartridge slot, but cutting the ROMs
 * in 3 files seems to be the only way to handle cartridges until I use
 * a headered format.
 * Note that there sometimes was a speech ROM slot in the speech synthesizer,
 * and you could plug quite a lot of GROMs in the side port.  Neither of these
 * are emulated.
 */

#define rom_ti99_4e rom_ti99_4
#define rom_ti99_4ae rom_ti99_4a

SYSTEM_CONFIG_START(ti99_4)
	CONFIG_DEVICE_CASSETTE			(2, "",												ti99_cassette_load)
	CONFIG_DEVICE_LEGACY			(IO_CARTSLOT,	3,	"bin\0c\0d\0g\0m\0crom\0drom\0grom\0mrom\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_READ,	NULL,	NULL,	ti99_rom_load,	ti99_rom_unload,	NULL)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(3,	"dsk\0",										ti99_floppy_load)
	CONFIG_DEVICE_LEGACY			(IO_PARALLEL,	1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	ti99_4_pio_load,	ti99_4_pio_unload,		NULL)
	CONFIG_DEVICE_LEGACY			(IO_SERIAL,		1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	ti99_4_rs232_load,	ti99_4_rs232_unload,	NULL)
	CONFIG_DEVICE_LEGACY			(IO_HARDDISK, 	1, "hd\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_OR_READ, NULL, NULL, ti99_ide_load, ti99_ide_unload, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT   MACHINE		 INPUT	  INIT		CONFIG	COMPANY				FULLNAME */
COMP( 1979, ti99_4,   0,	   ti99_4_60hz,  ti99_4,  ti99_4,	ti99_4,	"Texas Instruments", "TI99/4 Home Computer (US)" )
COMPX(1980, ti99_4e,  ti99_4,  ti99_4_50hz,  ti99_4,  ti99_4,	ti99_4,	"Texas Instruments", "TI99/4 Home Computer (Europe)", GAME_ALIAS )
COMP( 1981, ti99_4a,  0,	   ti99_4a_60hz, ti99_4a, ti99_4a,	ti99_4,	"Texas Instruments", "TI99/4A Home Computer (US)" )
COMPX(1981, ti99_4ae, ti99_4a, ti99_4a_50hz, ti99_4a, ti99_4a,	ti99_4,	"Texas Instruments", "TI99/4A Home Computer (Europe)", GAME_ALIAS )
COMPX(1994, ti99_4ev, ti99_4a, ti99_4ev_60hz,ti99_4a, ti99_4ev,	ti99_4,	"Texas Instruments", "TI99/4A Home Computer with EVPC", GAME_ALIAS )
