/*
    TX-0

    Raphael Nabet, 2004
*/

#include "emu.h"
#include "cpu/pdp1/tx0.h"
#include "includes/tx0.h"
#include "video/crt.h"


/*
    driver init function
*/
DRIVER_INIT_MEMBER(tx0_state,tx0)
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[tx0_fontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	};

	/* set up our font */
	dst = machine().root_device().memregion("gfx1")->base();

	memcpy(dst, fontdata6x8, tx0_fontdata_size);
}


static ADDRESS_MAP_START(tx0_64kw_map, AS_PROGRAM, 32, tx0_state )
	AM_RANGE(0x0000, 0xffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START(tx0_8kw_map, AS_PROGRAM, 32, tx0_state )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
ADDRESS_MAP_END


static INPUT_PORTS_START( tx0 )

	PORT_START("CSW")		/* 0: various tx0 operator control panel switches */
	PORT_BIT(tx0_control, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("control panel key") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(tx0_stop_cyc0, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop at cycle 0") PORT_CODE(KEYCODE_Q)
	PORT_BIT(tx0_stop_cyc1, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop at cycle 1") PORT_CODE(KEYCODE_W)
	PORT_BIT(tx0_gbl_cm_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CM select") PORT_CODE(KEYCODE_E)
	PORT_BIT(tx0_stop, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop") PORT_CODE(KEYCODE_P)
	PORT_BIT(tx0_restart, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("restart") PORT_CODE(KEYCODE_O)
	PORT_BIT(tx0_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("read in") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(tx0_toggle_dn, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("edit next toggle switch register") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(tx0_toggle_up, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("edit previous toggle switch register") PORT_CODE(KEYCODE_UP)
	PORT_BIT(tx0_cm_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TSS CM switch") PORT_CODE(KEYCODE_A)
	PORT_BIT(tx0_lr_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TSS LR switch") PORT_CODE(KEYCODE_SLASH)

    PORT_START("MSW")		/* 1: operator control panel toggle switch register switches MS */
	PORT_BIT(    0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 0") PORT_CODE(KEYCODE_S)
	PORT_BIT(    0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 1") PORT_CODE(KEYCODE_D)

    PORT_START("LSW")		/* 2: operator control panel toggle switch register switches LS */
	PORT_BIT( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 2") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 3") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 4") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 5") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 6") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 7") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 8") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 9") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 10") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 11") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 12") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 13") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 14") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 15") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 16") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 17") PORT_CODE(KEYCODE_STOP)

    PORT_START("TWR0")		/* 3: typewriter codes 00-17 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("| _") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Space)") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= :") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ /") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)

    PORT_START("TWR1")		/* 4: typewriter codes 20-37 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". )") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". (") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- +") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)

    PORT_START("TWR2")		/* 5: typewriter codes 40-57 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab Key") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

    PORT_START("TWR3")		/* 6: typewriter codes 60-77 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Upper case") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Lower Case") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)

INPUT_PORTS_END


static const gfx_layout fontlayout =
{
	6, 8,			/* 6*8 characters */
	tx0_charnum,	/* 96+xx characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};


/*
    The static palette only includes the pens for the control panel and
    the typewriter, as the CRT palette is generated dynamically.

    The CRT palette defines various levels of intensity between white and
    black.  Grey levels follow an exponential law, so that decrementing the
    color index periodically will simulate the remanence of a cathode ray tube.
*/
static const UINT8 tx0_colors[] =
{
	0x00,0x00,0x00,	/* black */
	0xFF,0xFF,0xFF,	/* white */
	0x00,0xFF,0x00,	/* green */
	0x00,0x40,0x00,	/* dark green */
	0xFF,0x00,0x00,	/* red */
	0x80,0x80,0x80	/* light gray */
};

static const UINT8 tx0_palette[] =
{
	pen_panel_bg, pen_panel_caption,	/* captions */
	pen_typewriter_bg, pen_black,		/* black typing in typewriter */
	pen_typewriter_bg, pen_red		/* red typing in typewriter */
};

static const UINT8 total_colors_needed = pen_crt_num_levels + sizeof(tx0_colors) / 3;

static GFXDECODE_START( tx0 )
	GFXDECODE_ENTRY( "gfx1", 0, fontlayout, pen_crt_num_levels + sizeof(tx0_colors) / 3, 3 )
GFXDECODE_END

/* Initialise the palette */
static PALETTE_INIT( tx0 )
{
	/* rgb components for the two color emissions */
	const double r1 = .1, g1 = .1, b1 = .924, r2 = .7, g2 = .7, b2 = .076;
	/* half period in seconds for the two color emissions */
	const double half_period_1 = .05, half_period_2 = .20;
	/* refresh period in seconds */
	const double update_period = 1./refresh_rate;
	double decay_1, decay_2;
	double cur_level_1, cur_level_2;
#if 0
#ifdef MAME_DEBUG
	/* level at which we stop emulating the decay and say the pixel is black */
	double cut_level = .02;
#endif
#endif
	UINT8 i, r, g, b;

	machine.colortable = colortable_alloc(machine, total_colors_needed);

	/* initialize CRT palette */

	/* compute the decay factor per refresh frame */
	decay_1 = pow(.5, update_period / half_period_1);
	decay_2 = pow(.5, update_period / half_period_2);

	cur_level_1 = cur_level_2 = 255.;	/* start with maximum level */

	for (i=pen_crt_max_intensity; i>0; i--)
	{
		/* compute the current color */
		r = (int) ((r1*cur_level_1 + r2*cur_level_2) + .5);
		g = (int) ((g1*cur_level_1 + g2*cur_level_2) + .5);
		b = (int) ((b1*cur_level_1 + b2*cur_level_2) + .5);
		/* write color in palette */
		colortable_palette_set_color(machine.colortable, i, MAKE_RGB(r, g, b));
		/* apply decay for next iteration */
		cur_level_1 *= decay_1;
		cur_level_2 *= decay_2;
	}
#if 0
#ifdef MAME_DEBUG
	{
		int recommended_pen_crt_num_levels;
		if (decay_1 > decay_2)
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_1))+1;
		else
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_2))+1;
		if (recommended_pen_crt_num_levels != pen_crt_num_levels)
			mame_printf_debug("File %s line %d: recommended value for pen_crt_num_levels is %d\n", __FILE__, __LINE__, recommended_pen_crt_num_levels);
	}
	/*if ((cur_level_1 > 255.*cut_level) || (cur_level_2 > 255.*cut_level))
        mame_printf_debug("File %s line %d: Please take higher value for pen_crt_num_levels or smaller value for decay\n", __FILE__, __LINE__);*/
#endif
#endif
	colortable_palette_set_color(machine.colortable, 0, MAKE_RGB(0, 0, 0));

	/* load static palette */
	for ( i = 0; i < 6; i++ )
	{
		r = tx0_colors[i*3]; g = tx0_colors[i*3+1]; b = tx0_colors[i*3+2];
		colortable_palette_set_color(machine.colortable, pen_crt_num_levels + i, MAKE_RGB(r, g, b));
	}

	/* copy colortable to palette */
	for( i = 0; i < total_colors_needed; i++ )
		colortable_entry_set_value(machine.colortable, i, i);

	/* set up palette for text */
	for( i = 0; i < 6; i++ )
		colortable_entry_set_value(machine.colortable, total_colors_needed + i, tx0_palette[i]);
}

static void tx0_io_cpy(device_t *device);
static void tx0_io_r1l(device_t *device);
static void tx0_io_r3l(device_t *device);
static void tx0_io_p6h(device_t *device);
static void tx0_io_p7h(device_t *device);
static void tx0_io_prt(device_t *device);
static void tx0_io_dis(device_t *device);
static void tx0_sel(device_t *device);
static void tx0_io_reset_callback(device_t *device);

static const tx0_reset_param_t tx0_reset_param =
{
	{
		tx0_io_cpy,
		tx0_io_r1l,
		tx0_io_dis,
		tx0_io_r3l,
		tx0_io_prt,
		/*tx0_io_typ*/NULL,
		tx0_io_p6h,
		tx0_io_p7h
	},
	tx0_sel,
	tx0_io_reset_callback
};

static const crt_interface tx0_crt_interface =
{
	pen_crt_num_levels,
	crt_window_offset_x, crt_window_offset_y,
	crt_window_width, crt_window_height
};


/*
    TX-0
*
    Raphael Nabet, 2004
*/


static TIMER_CALLBACK(reader_callback);
static TIMER_CALLBACK(puncher_callback);
static TIMER_CALLBACK(prt_callback);
static TIMER_CALLBACK(dis_callback);








/* crt display timer */



enum
{
	PF_RWC = 040,
	PF_EOR = 020,
	PF_PC  = 010,
	PF_EOT = 004
};


static MACHINE_RESET( tx0 )
{
	tx0_state *state = machine.driver_data<tx0_state>();
	/* reset device state */
	state->m_tape_reader.rcl = state->m_tape_reader.rc = 0;
}


static void tx0_machine_stop(running_machine &machine)
{
	tx0_state *state = machine.driver_data<tx0_state>();
	/* the core will take care of freeing the timers, BUT we must set the variables
    to NULL if we don't want to risk confusing the tape image init function */
	state->m_tape_reader.timer = state->m_tape_puncher.timer = state->m_typewriter.prt_timer = state->m_dis_timer = NULL;
}


static MACHINE_START( tx0 )
{
	tx0_state *state = machine.driver_data<tx0_state>();
	state->m_tape_reader.timer = machine.scheduler().timer_alloc(FUNC(reader_callback));
	state->m_tape_puncher.timer = machine.scheduler().timer_alloc(FUNC(puncher_callback));
	state->m_typewriter.prt_timer = machine.scheduler().timer_alloc(FUNC(prt_callback));
	state->m_dis_timer = machine.scheduler().timer_alloc(FUNC(dis_callback));

	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(tx0_machine_stop),&machine));
}


/*
    perforated tape handling
*/

class tx0_readtape_image_device :	public device_t,
									public device_image_interface
{
public:
	// construction/destruction
	tx0_readtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const { return IO_PUNCHTAPE; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 0; }
	virtual bool is_creatable() const { return 0; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 0; }
	virtual const char *image_interface() const { return NULL; }
	virtual const char *file_extensions() const { return "tap,rim"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	virtual bool call_load();
	virtual void call_unload();
protected:
	// device-level overrides
    virtual void device_config_complete() { update_names(); }
	virtual void device_start() { }
};

const device_type TX0_READTAPE = &device_creator<tx0_readtape_image_device>;

tx0_readtape_image_device::tx0_readtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, TX0_READTAPE, "TX0 Tape Reader", tag, owner, clock),
	  device_image_interface(mconfig, *this)
{
}

class tx0_punchtape_image_device :	public device_t,
									public device_image_interface
{
public:
	// construction/destruction
	tx0_punchtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const { return IO_PUNCHTAPE; }

	virtual bool is_readable()  const { return 0; }
	virtual bool is_writeable() const { return 1; }
	virtual bool is_creatable() const { return 1; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 0; }
	virtual const char *image_interface() const { return NULL; }
	virtual const char *file_extensions() const { return "tap,rim"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	virtual bool call_load();
	virtual void call_unload();
protected:
	// device-level overrides
    virtual void device_config_complete() { update_names(); }
	virtual void device_start() { }
};

const device_type TX0_PUNCHTAPE = &device_creator<tx0_punchtape_image_device>;

tx0_punchtape_image_device::tx0_punchtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, TX0_PUNCHTAPE, "TX0 Tape Puncher", tag, owner, clock),
	  device_image_interface(mconfig, *this)
{
}


class tx0_printer_image_device :	public device_t,
									public device_image_interface
{
public:
	// construction/destruction
	tx0_printer_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const { return IO_PRINTER; }

	virtual bool is_readable()  const { return 0; }
	virtual bool is_writeable() const { return 1; }
	virtual bool is_creatable() const { return 1; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 0; }
	virtual const char *image_interface() const { return NULL; }
	virtual const char *file_extensions() const { return "typ"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	virtual bool call_load();
	virtual void call_unload();
protected:
	// device-level overrides
    virtual void device_config_complete() { update_names(); }
	virtual void device_start() { }
};

const device_type TX0_PRINTER = &device_creator<tx0_printer_image_device>;

tx0_printer_image_device::tx0_printer_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, TX0_PRINTER, "TX0 Typewriter", tag, owner, clock),
	  device_image_interface(mconfig, *this)
{
}

class tx0_magtape_image_device :	public device_t,
									public device_image_interface
{
public:
	// construction/destruction
	tx0_magtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const { return IO_MAGTAPE; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 1; }
	virtual bool is_creatable() const { return 1; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 0; }
	virtual const char *image_interface() const { return NULL; }
	virtual const char *file_extensions() const { return "tap"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	virtual bool call_load();
	virtual void call_unload();
protected:
	// device-level overrides
    virtual void device_config_complete() { update_names(); }
	virtual void device_start();
};

const device_type TX0_MAGTAPE = &device_creator<tx0_magtape_image_device>;

tx0_magtape_image_device::tx0_magtape_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, TX0_MAGTAPE, "TX0 Magnetic Tape", tag, owner, clock),
	  device_image_interface(mconfig, *this)
{
}

/*
    Open a perforated tape image

    unit 0 is reader (read-only), unit 1 is puncher (write-only)
*/
bool tx0_readtape_image_device::call_load()
{
	tx0_state *state = machine().driver_data<tx0_state>();

	/* reader unit */
	state->m_tape_reader.fd = this;

	/* start motor */
	state->m_tape_reader.motor_on = 1;

		/* restart reader IO when necessary */
		/* note that this function may be called before tx0_init_machine, therefore
        before tape_reader.timer is allocated.  It does not matter, as the clutch is never
        down at power-up, but we must not call timer_enable with a NULL parameter! */

	if (state->m_tape_reader.timer)
	{
		if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
		{
			/* delay is approximately 1/400s */
			state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
		}
		else
		{
			state->m_tape_reader.timer->enable(0);
		}
	}

	return IMAGE_INIT_PASS;
}

void tx0_readtape_image_device::call_unload()
{
	tx0_state *state = machine().driver_data<tx0_state>();

	/* reader unit */
	state->m_tape_reader.fd = NULL;

	/* stop motor */
	state->m_tape_reader.motor_on = 0;

	if (state->m_tape_reader.timer)
		state->m_tape_reader.timer->enable(0);
}

/*
    Read a byte from perforated tape
*/
static int tape_read(tx0_state *state, UINT8 *reply)
{
	if (state->m_tape_reader.fd && (state->m_tape_reader.fd->fread(reply, 1) == 1))
		return 0;	/* unit OK */
	else
		return 1;	/* unit not ready */
}

/*
    Write a byte to perforated tape
*/
static void tape_write(tx0_state *state, UINT8 data)
{
	if (state->m_tape_puncher.fd)
		state->m_tape_puncher.fd->fwrite(& data, 1);
}

/*
    common code for tape read commands (R1C, R3C, and read-in mode)
*/
static void begin_tape_read(tx0_state *state, int binary)
{
	state->m_tape_reader.rcl = 1;
	state->m_tape_reader.rc = (binary) ? 1 : 3;

	/* set up delay if tape is advancing */
	if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
	{
		/* delay is approximately 1/400s */
		state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
	}
	else
	{
		state->m_tape_reader.timer->enable(0);
	}
}


/*
    timer callback to simulate reader IO
*/
static TIMER_CALLBACK(reader_callback)
{
	tx0_state *state = machine.driver_data<tx0_state>();
	int not_ready;
	UINT8 data;
	int ac;

	if (state->m_tape_reader.rc)
	{
		not_ready = tape_read(state, & data);
		if (not_ready)
		{
			state->m_tape_reader.motor_on = 0;	/* let us stop the motor */
		}
		else
		{
			if (data & 0100)
			{
				/* read current AC */
				ac = cpu_get_reg(machine.device("maincpu"), TX0_AC);
				/* cycle right */
				ac = (ac >> 1) | ((ac & 1) << 17);
				/* shuffle and insert data into AC */
				ac = (ac /*& 0333333*/) | ((data & 001) << 17) | ((data & 002) << 13) | ((data & 004) << 9) | ((data & 010) << 5) | ((data & 020) << 1) | ((data & 040) >> 3);
				/* write modified AC */
				cpu_set_reg(machine.device("maincpu"), TX0_AC, ac);

				state->m_tape_reader.rc = (state->m_tape_reader.rc+1) & 3;

				if (state->m_tape_reader.rc == 0)
				{	/* IO complete */
					state->m_tape_reader.rcl = 0;
					cpu_set_reg(machine.device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
				}
			}
		}
	}

	if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
		/* delay is approximately 1/400s */
		state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
	else
		state->m_tape_reader.timer->enable(0);
}

/*
    timer callback to generate punch completion pulse
*/
bool tx0_punchtape_image_device::call_load()
{
	tx0_state *state = machine().driver_data<tx0_state>();

	/* punch unit */
	state->m_tape_puncher.fd = this;

	return IMAGE_INIT_PASS;
}

void tx0_punchtape_image_device::call_unload()
{
	tx0_state *state = machine().driver_data<tx0_state>();

	/* punch unit */
	state->m_tape_puncher.fd = NULL;
}

static TIMER_CALLBACK(puncher_callback)
{
	cpu_set_reg(machine.device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    Initiate read of a 6-bit word from tape
*/
static void tx0_io_r1l(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	begin_tape_read(state, 0);
}

/*
    Initiate read of a 18-bit word from tape (used in read-in mode)
*/
static void tx0_io_r3l(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	begin_tape_read(state, 1);
}

/*
    Write a 7-bit word to tape (7th bit clear)
*/
static void tx0_io_p6h(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	int ac;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and punch 6-bit word */
	tape_write(state, ((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5));

	state->m_tape_puncher.timer->adjust(attotime::from_usec(15800));
}

/*
    Write a 7-bit word to tape (7th bit set)
*/
static void tx0_io_p7h(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	int ac;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and punch 6-bit word */
	tape_write(state, ((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5) | 0100);

	state->m_tape_puncher.timer->adjust(attotime::from_usec(15800));
}


/*
    Typewriter handling

    The alphanumeric on-line typewriter is a standard device on tx-0: it can
    both handle keyboard input and print output text.
*/

/*
    Open a file for typewriter output
*/
bool tx0_printer_image_device::call_load()
{
	tx0_state *state = machine().driver_data<tx0_state>();
	/* open file */
	state->m_typewriter.fd = this;

	return IMAGE_INIT_PASS;
}

void tx0_printer_image_device::call_unload()
{
	tx0_state *state = machine().driver_data<tx0_state>();
	state->m_typewriter.fd = NULL;
}

/*
    Write a character to typewriter
*/
static void typewriter_out(running_machine &machine, UINT8 data)
{
	tx0_state *state = machine.driver_data<tx0_state>();
	tx0_typewriter_drawchar(machine, data);
	if (state->m_typewriter.fd)
		state->m_typewriter.fd->fwrite(& data, 1);
}

/*
    timer callback to generate typewriter completion pulse
*/
static TIMER_CALLBACK(prt_callback)
{
	cpu_set_reg(machine.device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    prt io callback
*/
static void tx0_io_prt(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	int ac;
	int ch;

	/* read current AC */
	ac = cpu_get_reg(device, TX0_AC);
	/* shuffle and print 6-bit word */
	ch = ((ac & 0100000) >> 15) | ((ac & 0010000) >> 11) | ((ac & 0001000) >> 7) | ((ac & 0000100) >> 3) | ((ac & 0000010) << 1) | ((ac & 0000001) << 5);
	typewriter_out(device->machine(), ch);

	state->m_typewriter.prt_timer->adjust(attotime::from_msec(100));
}


/*
    timer callback to generate crt completion pulse
*/
static TIMER_CALLBACK(dis_callback)
{
	cpu_set_reg(machine.device("maincpu"), TX0_IO_COMPLETE, (UINT64)0);
}

/*
    Plot one point on crt
*/
static void tx0_io_dis(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	int ac;
	int x;
	int y;

	ac = cpu_get_reg(device, TX0_AC);
	x = ac >> 9;
	y = ac & 0777;
	tx0_plot(device->machine(), x, y);

	state->m_dis_timer->adjust(attotime::from_usec(50));
}


/*
    Magtape support

    Magtape format:

    7-track tape, 6-bit data, 1-bit parity


*/

static void schedule_select(tx0_state *state)
{
	attotime delay = attotime::zero;

	switch (state->m_magtape.command)
	{
	case 0:	/* backspace */
		delay = attotime::from_usec(4600);
		break;
	case 1:	/* read */
		delay = attotime::from_usec(8600);
		break;
	case 2:	/* rewind */
		delay = attotime::from_usec(12000);
		break;
	case 3:	/* write */
		delay = attotime::from_usec(4600);
		break;
	}
	state->m_magtape.timer->adjust(delay);
}

static void schedule_unselect(tx0_state *state)
{
	attotime delay = attotime::zero;

	switch (state->m_magtape.command)
	{
	case 0:	/* backspace */
		delay = attotime::from_usec(5750);
		break;
	case 1:	/* read */
		delay = attotime::from_usec(1750);
		break;
	case 2:	/* rewind */
		delay = attotime::from_usec(0);
		break;
	case 3:	/* write */
		delay = attotime::from_usec(5750);
		break;
	}
	state->m_magtape.timer->adjust(delay);
}

void tx0_magtape_image_device::device_start()
{
	tx0_state *state = machine().driver_data<tx0_state>();
	state->m_magtape.img = this;
}

/*
    Open a magnetic tape image
*/
bool tx0_magtape_image_device::call_load()
{
	tx0_state *state = machine().driver_data<tx0_state>();
	state->m_magtape.img = this;

	state->m_magtape.irg_pos = MTIRGP_END;

	/* restart IO when necessary */
	/* note that this function may be called before tx0_init_machine, therefore
    before magtape.timer is allocated.  We must not call timer_enable with a
    NULL parameter! */
	if (state->m_magtape.timer)
	{
		if (state->m_magtape.state == MTS_SELECTING)
			schedule_select(state);
	}

	return IMAGE_INIT_PASS;
}

void tx0_magtape_image_device::call_unload()
{
	tx0_state *state = machine().driver_data<tx0_state>();
	state->m_magtape.img = NULL;

	if (state->m_magtape.timer)
	{
		if (state->m_magtape.state == MTS_SELECTING)
			/* I/O has not actually started, we can cancel the selection */
			state->m_tape_reader.timer->enable(0);
		if ((state->m_magtape.state == MTS_SELECTED) || ((state->m_magtape.state == MTS_SELECTING) && (state->m_magtape.command == 2)))
		{	/* unit has become unavailable */
			state->m_magtape.state = MTS_UNSELECTING;
			cpu_set_reg(machine().device("maincpu"), TX0_PF, cpu_get_reg(machine().device("maincpu"), TX0_PF) | PF_RWC);
			schedule_unselect(state);
		}
	}
}

static void magtape_callback(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	UINT8 buf = 0;
	int lr;

	switch (state->m_magtape.state)
	{
	case MTS_UNSELECTING:
		state->m_magtape.state = MTS_UNSELECTED;

	case MTS_UNSELECTED:
		if (state->m_magtape.sel_pending)
		{
			int mar;

			mar = cpu_get_reg(device, TX0_MAR);

			if ((mar & 03) != 1)
			{	/* unimplemented device: remain in unselected state and set rwc
                flag? */
				cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
			}
			else
			{
				state->m_magtape.state = MTS_SELECTING;

				state->m_magtape.command = (mar & 014 >> 2);

				state->m_magtape.binary_flag = (mar & 020 >> 4);

				if (state->m_magtape.img)
					schedule_select(state);
			}

			state->m_magtape.sel_pending = FALSE;
			cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
		}
		break;

	case MTS_SELECTING:
		state->m_magtape.state = MTS_SELECTED;
		switch (state->m_magtape.command)
		{
		case 0:	/* backspace */
			state->m_magtape.long_parity = 0177;
			state->m_magtape.u.backspace_state = MTBSS_STATE0;
			break;
		case 1:	/* read */
			state->m_magtape.long_parity = 0177;
			state->m_magtape.u.read.state = MTRDS_STATE0;
			break;
		case 2:	/* rewind */
			break;
		case 3:	/* write */
			state->m_magtape.long_parity = 0177;
			state->m_magtape.u.write.state = MTWTS_STATE0;
			switch (state->m_magtape.irg_pos)
			{
			case MTIRGP_START:
				state->m_magtape.u.write.counter = 150;
				break;
			case MTIRGP_ENDMINUS1:
				state->m_magtape.u.write.counter = 1;
				break;
			case MTIRGP_END:
				state->m_magtape.u.write.counter = 0;
				break;
			}
			break;
		}

	case MTS_SELECTED:
		switch (state->m_magtape.command)
		{
		case 0:	/* backspace */
			if (state->m_magtape.img->ftell() == 0)
			{	/* tape at ldp */
				state->m_magtape.state = MTS_UNSELECTING;
				cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
				schedule_unselect(state);
			}
			else if (state->m_magtape.img->fseek( -1, SEEK_CUR))
			{	/* eject tape */
				state->m_magtape.img->unload();
			}
			else if (state->m_magtape.img->fread(&buf, 1) != 1)
			{	/* eject tape */
				state->m_magtape.img->unload();
			}
			else if (state->m_magtape.img->fseek( -1, SEEK_CUR))
			{	/* eject tape */
				state->m_magtape.img->unload();
			}
			else
			{
				buf &= 0x7f;	/* 7-bit tape, ignore 8th bit */
				state->m_magtape.long_parity ^= buf;
				switch (state->m_magtape.u.backspace_state)
				{
				case MTBSS_STATE0:
					/* STATE0 -> initial interrecord gap, longitudinal parity;
                    if longitudinal parity was all 0s, gap between longitudinal
                    parity and data, first byte of data */
					if (buf != 0)
						state->m_magtape.u.backspace_state = MTBSS_STATE1;
					break;
				case MTBSS_STATE1:
					/* STATE1 -> first byte of gap between longitudinal parity and
                    data, second byte of data */
					if (buf == 0)
						state->m_magtape.u.backspace_state = MTBSS_STATE2;
					else
						state->m_magtape.u.backspace_state = MTBSS_STATE5;
					break;
				case MTBSS_STATE2:
					/* STATE2 -> second byte of gap between longitudinal parity and
                    data */
					if (buf == 0)
						state->m_magtape.u.backspace_state = MTBSS_STATE3;
					else
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					break;
				case MTBSS_STATE3:
					/* STATE3 -> third byte of gap between longitudinal parity and
                    data */
					if (buf == 0)
						state->m_magtape.u.backspace_state = MTBSS_STATE4;
					else
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					break;
				case MTBSS_STATE4:
					/* STATE4 -> first byte of data word, first byte of
                    interrecord gap after data */
					if (buf == 0)
					{
						if (state->m_magtape.long_parity)
							logerror("invalid longitudinal parity\n");
						/* set EOR and unselect... */
						state->m_magtape.state = MTS_UNSELECTING;
						cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOR);
						schedule_unselect(state);
						state->m_magtape.irg_pos = MTIRGP_ENDMINUS1;
					}
					else
						state->m_magtape.u.backspace_state = MTBSS_STATE5;
					break;
				case MTBSS_STATE5:
					/* STATE5 -> second byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					else
						state->m_magtape.u.backspace_state = MTBSS_STATE6;
					break;
				case MTBSS_STATE6:
					/* STATE6 -> third byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					else
						state->m_magtape.u.backspace_state = MTBSS_STATE6;
					break;
				}
				if (state->m_magtape.state != MTS_UNSELECTING)
					state->m_magtape.timer->adjust(attotime::from_usec(66));
			}
			break;

		case 1:	/* read */
			if (state->m_magtape.img->fread(&buf, 1) != 1)
			{	/* I/O error or EOF? */
				/* The MAME fileio layer makes it very hard to make the
                difference...  MAME seems to assume that I/O errors never
                happen, whereas it is really easy to cause one by
                deconnecting an external drive the image is located on!!! */
				UINT64 offs;
				offs = state->m_magtape.img->ftell();
				if (state->m_magtape.img->fseek( 0, SEEK_END) || (offs != state->m_magtape.img->ftell()))
				{	/* I/O error */
					/* eject tape */
					state->m_magtape.img->unload();
				}
				else
				{	/* end of tape -> ??? */
					/* maybe we run past end of tape, so that tape is ejected from
                    upper reel and unit becomes unavailable??? */
					/*state->m_magtape.img->unload();*/
					/* Or do we stop at EOT mark??? */
					state->m_magtape.state = MTS_UNSELECTING;
					cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOT);
					schedule_unselect(state);
				}
			}
			else
			{
				buf &= 0x7f;	/* 7-bit tape, ignore 8th bit */
				state->m_magtape.long_parity ^= buf;
				switch (state->m_magtape.u.read.state)
				{
				case MTRDS_STATE0:
					/* STATE0 -> interrecord blank or first byte of data */
					if (buf != 0)
					{
						if (state->m_magtape.cpy_pending)
						{	/* read command */
							state->m_magtape.u.read.space_flag = FALSE;
							cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
							cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
														| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
							/* check parity */
							if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ state->m_magtape.binary_flag))
								cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						}
						else
						{	/* space command */
							state->m_magtape.u.read.space_flag = TRUE;
						}
						state->m_magtape.u.read.state = MTRDS_STATE1;
					}
					break;
				case MTRDS_STATE1:
					/* STATE1 -> second byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					if (!state->m_magtape.u.read.space_flag)
					{
						cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
													| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
						/* check parity */
						if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ state->m_magtape.binary_flag))
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
					}
					state->m_magtape.u.read.state = MTRDS_STATE2;
					break;
				case MTRDS_STATE2:
					/* STATE2 -> third byte of data word */
					if (buf == 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					if (!state->m_magtape.u.read.space_flag)
					{
						cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
													| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
						/* check parity */
						if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ state->m_magtape.binary_flag))
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						/* synchronize with cpy instruction */
						if (state->m_magtape.cpy_pending)
							cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
						else
							cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_RWC);
					}
					state->m_magtape.u.read.state = MTRDS_STATE3;
					break;
				case MTRDS_STATE3:
					/* STATE3 -> first byte of new word of data, or first byte
                    of gap between data and longitudinal parity */
					if (buf != 0)
					{
						state->m_magtape.u.read.state = MTRDS_STATE1;
						if (!state->m_magtape.u.read.space_flag)
						{
							cpu_set_reg(device, TX0_LR, ((cpu_get_reg(device, TX0_LR) >> 1) & 0333333)
														| ((buf & 040) << 12) | ((buf & 020) << 10) | ((buf & 010) << 8) | ((buf & 004) << 6) | ((buf & 002) << 4) | ((buf & 001) << 2));
							/* check parity */
							if (! (((buf ^ (buf >> 1) ^ (buf >> 2) ^ (buf >> 3) ^ (buf >> 4) ^ (buf >> 5) ^ (buf >> 6) ^ (buf >> 7)) & 1) ^ state->m_magtape.binary_flag))
								cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
						}
					}
					else
						state->m_magtape.u.read.state = MTRDS_STATE4;
					break;
				case MTRDS_STATE4:
					/* STATE4 -> second byte of gap between data and
                    longitudinal parity */
					if (buf != 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					else
						state->m_magtape.u.read.state = MTRDS_STATE5;
					break;

				case MTRDS_STATE5:
					/* STATE5 -> third byte of gap between data and
                    longitudinal parity */
					if (buf != 0)
					{
						logerror("tape seems to be corrupt\n");
						/* eject tape */
						state->m_magtape.img->unload();
					}
					else
						state->m_magtape.u.read.state = MTRDS_STATE6;
					break;

				case MTRDS_STATE6:
					/* STATE6 -> longitudinal parity */
					/* check parity */
					if (state->m_magtape.long_parity)
					{
						logerror("invalid longitudinal parity\n");
						/* no idea if the original tx-0 magtape controller
                        checks parity, but can't harm if we do */
						cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_PC);
					}
					/* set EOR and unselect... */
					state->m_magtape.state = MTS_UNSELECTING;
					cpu_set_reg(device, TX0_PF, cpu_get_reg(device, TX0_PF) | PF_EOR);
					schedule_unselect(state);
					state->m_magtape.irg_pos = MTIRGP_START;
					break;
				}
				if (state->m_magtape.state != MTS_UNSELECTING)
					state->m_magtape.timer->adjust(attotime::from_usec(66));
			}
			break;

		case 2:	/* rewind */
			state->m_magtape.state = MTS_UNSELECTING;
			/* we rewind at 10*read speed (I don't know the real value) */
			state->m_magtape.timer->adjust((attotime::from_nsec(6600) * state->m_magtape.img->ftell()));
			//schedule_unselect(state);
			state->m_magtape.img->fseek( 0, SEEK_END);
			state->m_magtape.irg_pos = MTIRGP_END;
			break;

		case 3:	/* write */
			switch (state->m_magtape.u.write.state)
			{
			case MTWTS_STATE0:
				if (state->m_magtape.u.write.counter != 0)
				{
					state->m_magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					state->m_magtape.u.write.state = MTWTS_STATE1;
				}

			case MTWTS_STATE1:
				if (state->m_magtape.u.write.counter)
				{
					state->m_magtape.u.write.counter--;
					lr = cpu_get_reg(device, TX0_LR);
					buf = ((lr >> 10) & 040) | ((lr >> 8) & 020) | ((lr >> 6) & 010) | ((lr >> 4) & 004) | ((lr >> 2) & 002) | (lr & 001);
					buf |= ((buf << 1) ^ (buf << 2) ^ (buf << 3) ^ (buf << 4) ^ (buf << 5) ^ (buf << 6) ^ ((!state->m_magtape.binary_flag) << 6)) & 0100;
					cpu_set_reg(device, TX0_LR, lr >> 1);
				}
				else
				{
					if (state->m_magtape.cpy_pending)
					{
						cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
						lr = cpu_get_reg(device, TX0_LR);
						buf = ((lr >> 10) & 040) | ((lr >> 8) & 020) | ((lr >> 6) & 010) | ((lr >> 4) & 004) | ((lr >> 2) & 002) | (lr & 001);
						buf |= ((buf << 1) ^ (buf << 2) ^ (buf << 3) ^ (buf << 4) ^ (buf << 5) ^ (buf << 6) ^ ((!state->m_magtape.binary_flag) << 6)) & 0100;
						cpu_set_reg(device, TX0_LR, lr >> 1);
						state->m_magtape.u.write.counter = 2;
						break;
					}
					else
					{
						state->m_magtape.u.write.state = MTWTS_STATE2;
						state->m_magtape.u.write.counter = 3;
					}
				}

			case MTWTS_STATE2:
				if (state->m_magtape.u.write.counter != 0)
				{
					state->m_magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					buf = state->m_magtape.long_parity;
					state->m_magtape.state = (state_t)MTWTS_STATE3;
					state->m_magtape.u.write.counter = 150;
				}
				break;

			case MTWTS_STATE3:
				if (state->m_magtape.u.write.counter != 0)
				{
					state->m_magtape.u.write.counter--;
					buf = 0;
					break;
				}
				else
				{
					state->m_magtape.state = MTS_UNSELECTING;
					schedule_unselect(state);
					state->m_magtape.irg_pos = MTIRGP_END;
				}
				break;
			}
			if (state->m_magtape.state != MTS_UNSELECTING)
			{	/* write data word */
				state->m_magtape.long_parity ^= buf;
				if (state->m_magtape.img->fwrite(&buf, 1) != 1)
				{	/* I/O error */
					/* eject tape */
					state->m_magtape.img->unload();
				}
				else
					state->m_magtape.timer->adjust(attotime::from_usec(66));
			}
			break;
		}
		break;
	}
}

static void tx0_sel(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	state->m_magtape.sel_pending = TRUE;

	if (state->m_magtape.state == MTS_UNSELECTED)
	{
		if (0)
			magtape_callback(device);
		state->m_magtape.timer->adjust(attotime::zero);
	}
}

static void tx0_io_cpy(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	switch (state->m_magtape.state)
	{
	case MTS_UNSELECTED:
	case MTS_UNSELECTING:
		/* ignore instruction and set rwc flag? */
		cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
		break;

	case MTS_SELECTING:
	case MTS_SELECTED:
		switch (state->m_magtape.command)
		{
		case 0:	/* backspace */
		case 2:	/* rewind */
			/* ignore instruction and set rwc flag? */
			cpu_set_reg(device, TX0_IO_COMPLETE, (UINT64)0);
			break;
		case 1:	/* read */
		case 3:	/* write */
			state->m_magtape.cpy_pending = TRUE;
			break;
		}
		break;
	}
}


/*
    callback which is called when reset line is pulsed

    IO devices should reset
*/
static void tx0_io_reset_callback(device_t *device)
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	state->m_tape_reader.rcl = state->m_tape_reader.rc = 0;
	if (state->m_tape_reader.timer)
		state->m_tape_reader.timer->enable(0);

	if (state->m_tape_puncher.timer)
		state->m_tape_puncher.timer->enable(0);

	if (state->m_typewriter.prt_timer)
		state->m_typewriter.prt_timer->enable(0);

	if (state->m_dis_timer)
		state->m_dis_timer->enable(0);
}


/*
    typewriter keyboard handler
*/
static void tx0_keyboard(running_machine &machine)
{
	tx0_state *state = machine.driver_data<tx0_state>();
	int i;
	int j;

	int typewriter_keys[4];

	int typewriter_transitions;
	int charcode, lr;
	static const char *const twrnames[] = { "TWR0", "TWR1", "TWR2", "TWR3" };

	for (i=0; i<4; i++)
	{
		typewriter_keys[i] = machine.root_device().ioport(twrnames[i])->read();
	}

	for (i=0; i<4; i++)
	{
		typewriter_transitions = typewriter_keys[i] & (~ state->m_old_typewriter_keys[i]);
		if (typewriter_transitions)
		{
			for (j=0; (((typewriter_transitions >> j) & 1) == 0) /*&& (j<16)*/; j++)
				;
			charcode = (i << 4) + j;
			/* shuffle and insert data into LR */
			/* BTW, I am not sure how the char code is combined with the
            previous LR */
			lr = (1 << 17) | ((charcode & 040) << 10) | ((charcode & 020) << 8) | ((charcode & 010) << 6) | ((charcode & 004) << 4) | ((charcode & 002) << 2) | ((charcode & 001) << 1);
			/* write modified LR */
			cpu_set_reg(machine.device("maincpu"), TX0_LR, lr);
			tx0_typewriter_drawchar(machine, charcode);	/* we want to echo input */
			break;
		}
	}

	for (i=0; i<4; i++)
		state->m_old_typewriter_keys[i] = typewriter_keys[i];
}

/*
    Not a real interrupt - just handle keyboard input
*/
static INTERRUPT_GEN( tx0_interrupt )
{
	tx0_state *state = device->machine().driver_data<tx0_state>();
	int control_keys;
	int tsr_keys;

	int control_transitions;
	int tsr_transitions;


	/* read new state of control keys */
	control_keys = state->ioport("CSW")->read();

	if (control_keys & tx0_control)
	{
		/* compute transitions */
		control_transitions = control_keys & (~ state->m_old_control_keys);

		if (control_transitions & tx0_stop_cyc0)
		{
			cpu_set_reg(device->machine().device("maincpu"), TX0_STOP_CYC0, !cpu_get_reg(device->machine().device("maincpu"), TX0_STOP_CYC0));
		}
		if (control_transitions & tx0_stop_cyc1)
		{
			cpu_set_reg(device->machine().device("maincpu"), TX0_STOP_CYC1, !cpu_get_reg(device->machine().device("maincpu"), TX0_STOP_CYC1));
		}
		if (control_transitions & tx0_gbl_cm_sel)
		{
			cpu_set_reg(device->machine().device("maincpu"), TX0_GBL_CM_SEL, !cpu_get_reg(device->machine().device("maincpu"), TX0_GBL_CM_SEL));
		}
		if (control_transitions & tx0_stop)
		{
			cpu_set_reg(device->machine().device("maincpu"), TX0_RUN, (UINT64)0);
			cpu_set_reg(device->machine().device("maincpu"), TX0_RIM, (UINT64)0);
		}
		if (control_transitions & tx0_restart)
		{
			cpu_set_reg(device->machine().device("maincpu"), TX0_RUN, 1);
			cpu_set_reg(device->machine().device("maincpu"), TX0_RIM, (UINT64)0);
		}
		if (control_transitions & tx0_read_in)
		{	/* set cpu to read instructions from perforated tape */
			cpu_set_reg(device->machine().device("maincpu"), TX0_RESET, (UINT64)0);
			cpu_set_reg(device->machine().device("maincpu"), TX0_RUN, (UINT64)0);
			cpu_set_reg(device->machine().device("maincpu"), TX0_RIM, 1);
		}
		if (control_transitions & tx0_toggle_dn)
		{
			state->m_tsr_index++;
			if (state->m_tsr_index == 18)
				state->m_tsr_index = 0;
		}
		if (control_transitions & tx0_toggle_up)
		{
			state->m_tsr_index--;
			if (state->m_tsr_index == -1)
				state->m_tsr_index = 17;
		}
		if (control_transitions & tx0_cm_sel)
		{
			if (state->m_tsr_index >= 2)
			{
				UINT32 cm_sel = (UINT32) cpu_get_reg(device->machine().device("maincpu"), TX0_CM_SEL);
				cpu_set_reg(device->machine().device("maincpu"), TX0_CM_SEL, cm_sel ^ (1 << (state->m_tsr_index - 2)));
			}
		}
		if (control_transitions & tx0_lr_sel)
		{
			if (state->m_tsr_index >= 2)
			{
				UINT32 lr_sel = (UINT32) cpu_get_reg(device->machine().device("maincpu"), TX0_LR_SEL);
				cpu_set_reg(device->machine().device("maincpu"), TX0_LR_SEL, (lr_sel ^ (1 << (state->m_tsr_index - 2))));
			}
		}

		/* remember new state of control keys */
		state->m_old_control_keys = control_keys;


		/* handle toggle switch register keys */
		tsr_keys = (device->machine().root_device().ioport("MSW")->read() << 16) | device->machine().root_device().ioport("LSW")->read();

		/* compute transitions */
		tsr_transitions = tsr_keys & (~ state->m_old_tsr_keys);

		/* update toggle switch register */
		if (tsr_transitions)
			cpu_set_reg(device->machine().device("maincpu"), TX0_TBR+state->m_tsr_index, cpu_get_reg(device->machine().device("maincpu"), TX0_TBR+state->m_tsr_index) ^ tsr_transitions);

		/* remember new state of toggle switch register keys */
		state->m_old_tsr_keys = tsr_keys;
	}
	else
	{
		state->m_old_control_keys = 0;
		state->m_old_tsr_keys = 0;

		tx0_keyboard(device->machine());
	}
}

static MACHINE_CONFIG_START( tx0_64kw, tx0_state )
	/* basic machine hardware */
	/* TX0 CPU @ approx. 167 kHz (no master clock, but the memory cycle time is approximately 6usec) */
	MCFG_CPU_ADD("maincpu", TX0_64KW, 166667)
	MCFG_CPU_CONFIG(tx0_reset_param)
	MCFG_CPU_PROGRAM_MAP(tx0_64kw_map)
	/* dummy interrupt: handles input */
	MCFG_CPU_VBLANK_INT("screen", tx0_interrupt)

	MCFG_MACHINE_START( tx0 )
	MCFG_MACHINE_RESET( tx0 )

	/* video hardware (includes the control panel and typewriter output) */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(refresh_rate)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(virtual_width, virtual_height)
	MCFG_SCREEN_VISIBLE_AREA(0, virtual_width-1, 0, virtual_height-1)
	MCFG_SCREEN_UPDATE_STATIC(tx0)
	MCFG_SCREEN_VBLANK_STATIC(tx0)

	MCFG_CRT_ADD( "crt", tx0_crt_interface )
	MCFG_DEVICE_ADD("readt", TX0_READTAPE, 0)
	MCFG_DEVICE_ADD("punch", TX0_PUNCHTAPE, 0)
	MCFG_DEVICE_ADD("typewriter", TX0_PRINTER, 0)
	MCFG_DEVICE_ADD("magtape", TX0_MAGTAPE, 0)

	MCFG_GFXDECODE(tx0)
	MCFG_PALETTE_LENGTH(pen_crt_num_levels + sizeof(tx0_colors) / 3 + sizeof(tx0_palette))

	MCFG_PALETTE_INIT(tx0)
	MCFG_VIDEO_START(tx0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( tx0_8kw, tx0_64kw )

	/* basic machine hardware */
	/* TX0 CPU @ approx. 167 kHz (no master clock, but the memory cycle time is
    approximately 6usec) */
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_CONFIG(tx0_reset_param)
	MCFG_CPU_PROGRAM_MAP(tx0_8kw_map)
	/*MCFG_CPU_PORTS(readport, writeport)*/
MACHINE_CONFIG_END

ROM_START(tx0_64kw)
	/*CPU memory space*/
	ROM_REGION(0x10000 * sizeof(UINT32),"maincpu",ROMREGION_ERASEFF)
		/* Note this computer has no ROM... */

	ROM_REGION(tx0_fontdata_size, "gfx1", ROMREGION_ERASEFF)
		/* space filled with our font */
ROM_END

ROM_START(tx0_8kw)
	/*CPU memory space*/
	ROM_REGION(0x2000 * sizeof(UINT32),"maincpu",ROMREGION_ERASEFF)
		/* Note this computer has no ROM... */

	ROM_REGION(tx0_fontdata_size, "gfx1", ROMREGION_ERASEFF)
		/* space filled with our font */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT INIT     COMPANY                   FULLNAME */
COMP( 1956, tx0_64kw, 0,	0,	tx0_64kw, tx0, tx0_state,	tx0,			"MIT", "TX-0 original demonstrator (64 kWords of RAM)" , GAME_NO_SOUND_HW | GAME_NOT_WORKING)
COMP( 1962, tx0_8kw,  tx0_64kw,	0,	tx0_8kw,  tx0, tx0_state,	tx0,		"MIT", "TX-0 upgraded system (8 kWords of RAM)" , GAME_NO_SOUND_HW | GAME_NOT_WORKING)
