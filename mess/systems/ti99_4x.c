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
		spchroms_read_and_branch,	// speech ROM read and branch handler
	#endif
	#if 0
		tms5220_ready_callback
	#endif
	};
*/

/*
	TI99/4 preliminary infos:

References:
	* ...

Similar to TI99/4a, except for the following:
	* tms9918/9928 has no bitmap mode
	* smaller, 40-key keyboard
	* many small differences in the contents of system ROMs

Historical notes: TI made several last minute design changes.
	* TI99/4 prototypes had an extra port for an I/R joystick and keypad interface.
	* early TI99/4 prototypes were designed for a tms9985, not a tms9900.
*/

#include "driver.h"
#include "vidhrdw/tms9928a.h"
#include "vidhrdw/v9938.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "includes/basicdsk.h"

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

	{0x0000<<1, 0x07ff<<1, tms9901_CRU_write},
	{0x0800<<1, 0x0fff<<1, ti99_expansion_CRU_w},

PORT_END

static PORT_READ16_START(readcru)

	{0x0000<<1, 0x00ff<<1, tms9901_CRU_read},
	{0x0100<<1, 0x01ff<<1, ti99_expansion_CRU_r},

PORT_END


/*
	Input ports, used by machine code for TI keyboard and joystick emulation.
*/

/* TI99/4a: 48-key keyboard, plus two optional joysticks (2 shift keys) */
INPUT_PORTS_START(ti99_4a)

	PORT_START	/* config */
		PORT_BITX( 0x0007, 0x0001, IPT_DIPSWITCH_NAME, "RAM extension", KEYCODE_NONE, IP_JOY_NONE )
		    PORT_DIPSETTING( xRAM_kind_none,			"none" )
		    PORT_DIPSETTING( xRAM_kind_TI,				"Texas Instrument 32kb")
		    PORT_DIPSETTING( xRAM_kind_super_AMS,		"Super AMS 1Mb")
		    PORT_DIPSETTING( xRAM_kind_foundation_128k,	"Foundation 128kb")
		    PORT_DIPSETTING( xRAM_kind_foundation_512k,	"Foundation 512kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_128k,		"Myarc look-alike 128kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_512k,		"Myarc look-alike 512kb")
		PORT_BITX( 0x0008, 0x0008, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 0x0008, DEF_STR( On ) )
		PORT_BITX( 0x0010, 0x0010, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 0x0010, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_BITX(0x88, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_RCONTROL, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
		/* TI99/4a has a second shift key which maps the same */
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_RALT, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

	PORT_START	/* col 1 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

	PORT_START	/* col 2 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 # ERASE", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)

	PORT_START	/* col 3 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)

	PORT_START	/* col 4 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)

	PORT_START	/* col 5 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A |", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

	PORT_START	/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

	PORT_START	/* one more port for Alpha line */
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END


/* TI99/4: 41-key keyboard, plus two optional joysticks  (2 space keys) */
INPUT_PORTS_START(ti99_4)

	PORT_START	/* config */
		PORT_BITX( 0x0007, 0x0001, IPT_DIPSWITCH_NAME, "RAM extension", KEYCODE_NONE, IP_JOY_NONE )
		    PORT_DIPSETTING( xRAM_kind_none,			"none" )
		    PORT_DIPSETTING( xRAM_kind_TI,				"Texas Instrument 32kb")
		    PORT_DIPSETTING( xRAM_kind_super_AMS,		"Super AMS 1Mb")
		    PORT_DIPSETTING( xRAM_kind_foundation_128k,	"Foundation 128kb")
		    PORT_DIPSETTING( xRAM_kind_foundation_512k,	"Foundation 512kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_128k,		"Myarc look-alike 128kb")
		    PORT_DIPSETTING( xRAM_kind_myarc_512k,		"Myarc look-alike 512kb")
		PORT_BITX( 0x0008, 0x0008, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 0x0008, DEF_STR( On ) )
		PORT_BITX( 0x0010, 0x0010, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 0x0010, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q QUIT", KEYCODE_Q, IP_JOY_NONE)
		/* TI99/4a has a second space key which maps the same */
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "P \"", KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L ???", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)

	PORT_START	/* col 1 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "W BEGIN", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "A AID", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z BACK", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (", KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O '", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "K ???", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

	PORT_START	/* col 2 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "E UP", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "S LEFT", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "X DOWN", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I ?", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "J ???", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M ???", KEYCODE_M, IP_JOY_NONE)

	PORT_START	/* col 3 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "R REDO", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "D RIGHT", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "C CLEAR", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "U _", KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "H ???", KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N ???", KEYCODE_N, IP_JOY_NONE)

	PORT_START	/* col 4 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "T ERASE", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F DEL", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V PROC'D", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y ???", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "G INS", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "B ???", KEYCODE_B, IP_JOY_NONE)

	PORT_START	/* col 5: "wired handset 1" (= joystick 1) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

	PORT_START	/* col 6: "wired handset 2" (= joystick 2) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

	PORT_START	/* col 7 */
		PORT_BITX(0xFF, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

INPUT_PORTS_END

#if 0
static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }		/* end of array */
};
#endif

/*
	TMS9919 (i.e. SN76489) sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. connected to the TMS9918A CPUCLK pin... */
	{ 75 }			/* Volume.  I don't know the best value. */
};

/*static void tms5220_ready_callback(int state)
{
	cpu_set_halt_line(0, state ? CLEAR_LINE : ASSERT_LINE);
}*/

static struct TMS5220interface tms5220interface =
{
	680000L,					/* 640kHz -> 8kHz output */
	50,							/* Volume.  I don't know the best value. */
	NULL,						/* no IRQ callback */
#if 1
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch/*,*/	/* speech ROM read and branch handler */
#endif
#if 0
	tms5220_ready_callback
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



/*
	machine description.
*/
static struct MachineDriver machine_driver_ti99_4_60hz =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,	/* 3.0 Mhz*/
			readmem, writemem, readcru, writecru,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,						/* screen width */
	192,						/* screen height */
	{ 0, 256-1, 0, 192-1},		/* visible_area */
	/*gfxdecodeinfo*/NULL,				/* graphics decode info */
	TMS9928A_PALETTE_SIZE,		/* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE,	/* length in shorts of the color lookup table */
	tms9928A_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_4_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		},
		{
			SOUND_WAVE,
			&tape_input_intf
		}
	},

	/* NVRAM handler */
	NULL
};

static struct MachineDriver machine_driver_ti99_4_50hz =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,	/* 3.0 Mhz*/
			readmem, writemem, readcru, writecru,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,						/* screen width */
	192,						/* screen height */
	{ 0, 256-1, 0, 192-1},		/* visible_area */
	/*gfxdecodeinfo*/NULL,				/* graphics decode info */
	TMS9928A_PALETTE_SIZE,		/* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE,	/* length in shorts of the color lookup table */
	tms9928A_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_4_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		},
		{
			SOUND_WAVE,
			&tape_input_intf
		}
	},

	/* NVRAM handler */
	NULL
};


static struct MachineDriver machine_driver_ti99_4a_60hz =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,	/* 3.0 Mhz*/
			readmem, writemem, readcru, writecru,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,						/* screen width */
	192,						/* screen height */
	{ 0, 256-1, 0, 192-1},		/* visible_area */
	/*gfxdecodeinfo*/NULL,				/* graphics decode info (???)*/
	TMS9928A_PALETTE_SIZE,		/* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE,	/* length in shorts of the color lookup table */
	tms9928A_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_4a_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		},
		{
			SOUND_WAVE,
			&tape_input_intf
		}
	},

	/* NVRAM handler */
	NULL
};

static struct MachineDriver machine_driver_ti99_4a_50hz =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,	/* 3.0 Mhz*/
			readmem, writemem, readcru, writecru,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,						/* screen width */
	192,						/* screen height */
	{ 0, 256-1, 0, 192-1},		/* visible_area */
	/*gfxdecodeinfo*/NULL,				/* graphics decode info (???)*/
	TMS9928A_PALETTE_SIZE,		/* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE,	/* length in shorts of the color lookup table */
	tms9928A_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_4a_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		},
		{
			SOUND_WAVE,
			&tape_input_intf
		}
	},

	/* NVRAM handler */
	NULL
};

static struct MachineDriver machine_driver_ti99_4ev_60hz =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,	/* 3.0 Mhz*/
			readmem_4ev, writemem_4ev, readcru, writecru,
			ti99_4ev_vblank_interrupt, 313,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	512 + 32,					/* screen width */
	(212 + 16) * 2,				/* screen height */
	{ 0, 512 + 32 - 1, 0, (212 + 16) * 2 - 1 },		/* visible_area */
	NULL,						/* graphics decode info */
	512,						/* palette is 3*total_colors bytes long */
	512,						/* length in shorts of the color lookup table */
	v9938_init_palette,			/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_4ev_vh_start,
	v9938_exit,
	v9938_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		},
		{
			SOUND_WAVE,
			&tape_input_intf
		}
	},

	/* NVRAM handler */
	NULL
};

/*
	ROM loading

	Note that we use the same ROMset for 50Hz and 60Hz version.
*/

ROM_START(ti99_4)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_WORD("994rom.bin", 0x0000, 0x2000, 0x00000000) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("994grom.bin", 0x0000, 0x6000, 0x00000000) /* system GROMs */

	/* Used for disk DSR */
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD("disk.bin", 0x0000, 0x2000, 0x8f7df93f) /* disk DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

ROM_START(ti99_4a)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_WORD("994arom.bin", 0x0000, 0x2000, 0xdb8f33e5) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("994agrom.bin", 0x0000, 0x6000, 0xaf5c2449) /* system GROMs */

	/* Used for disk DSR */
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD("disk.bin", 0x0000, 0x2000, 0x8f7df93f) /* disk DSR ROM */
	/*ROM_LOAD("evpcdsr.bin", 0x2000, 0x10000, 0xa062b75d)*/ /* evpc DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

ROM_START(ti99_4ev)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len, REGION_CPU1, 0)
	ROM_LOAD16_WORD("994arom.bin", 0x0000, 0x2000, 0xdb8f33e5) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("994agrom.bin", 0x0000, 0x6000, 0xaf5c2449) /* system GROMs */

	/* Used for disk DSR */
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD("disk.bin", 0x0000, 0x2000, 0x8f7df93f) /* disk DSR ROM */
	ROM_LOAD("evpcdsr.bin", 0x2000, 0x10000, 0xa062b75d) /* evpc DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

/* a TI99 console only had one cartidge slot, but cutting the ROMs
 * in 3 files seems to be the only way to handle cartidges until I use
 * a headered format.
 * Note that there sometimes was a speech ROM slot in the speech synthesizer,
 * and you could plug up to 16 additonnal DSR roms and quite a lot of GROMs
 * in the side port.  None of these is emulated.
 */

static const struct IODevice io_ti99_4[] =
{
	{
		IO_CARTSLOT,		/* type */
		3,					/* count */
		"bin\0c\0d\0g\0m\0crom\0drom\0grom\0mrom\0",	/* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		0,
		ti99_load_rom,		/* init */
		ti99_rom_cleanup,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	IO_CASSETTE_WAVE(2,"wav\0",NULL,ti99_cassette_init,ti99_cassette_exit),
	{
		IO_FLOPPY,				/* type */
		3,						/* count */
		"dsk\0",				/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		ti99_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};




#define io_ti99_4e io_ti99_4
#define io_ti99_4a io_ti99_4
#define io_ti99_4ae io_ti99_4a
#define io_ti99_4ev io_ti99_4a

#define rom_ti99_4e rom_ti99_4
#define rom_ti99_4ae rom_ti99_4a

/*	  YEAR	NAME	  PARENT   MACHINE		ÊINPUT	  INIT	   COMPANY				FULLNAME */
COMP( 1979, ti99_4,   0,	   ti99_4_60hz,  ti99_4,  ti99_4,  "Texas Instruments", "TI99/4 Home Computer (US)" )
COMPX(1980, ti99_4e,  ti99_4,  ti99_4_50hz,  ti99_4,  ti99_4,  "Texas Instruments", "TI99/4 Home Computer (Europe)", GAME_ALIAS )
COMP( 1981, ti99_4a,  0,	   ti99_4a_60hz, ti99_4a, ti99_4a, "Texas Instruments", "TI99/4A Home Computer (US)" )
COMPX(1981, ti99_4ae, ti99_4a, ti99_4a_50hz, ti99_4a, ti99_4a, "Texas Instruments", "TI99/4A Home Computer (Europe)", GAME_ALIAS )
COMPX(1994, ti99_4ev, ti99_4a, ti99_4ev_60hz,ti99_4a, ti99_4ev,"Texas Instruments", "TI99/4A Home Computer with EVPC", GAME_ALIAS )
