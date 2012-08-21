/***************************************************************************

    TEC-1 driver, written by Robbbert in April, 2009 for MESS.

The TEC-1 was a single-board "computer" described in Talking Electronics
magazine, issues number 10 and 11. Talking Electronics do not have dates on
their issues, so the date is uncertain, although 1984 seems a reasonable
guess. Talking Electronics operated from Cheltenham, a suburb of Melbourne.

The hardware is quite simple, consisting of a Z80 cpu, 2x 8212 8-bit latch,
74C923 keyboard scanner, 20 push-button keys, 6-digit LED display, a speaker,
a 2k EPROM and sundry parts.

The cpu speed could be adjusted by using a potentiometer, the range being
250 kHz to 2MHz. This is a simple method of adjusting a game's difficulty.

We emulate the original version. Later enhancements included more RAM, speech
synthesis and various attachments, however I have no information on these.

Pasting:
        0-F : as is
        + (inc) : ^
        - (dec) : V
        AD : -
        GO : X

Keys:
0 to 9, A to F are on the key of the same name.
AD (input an address) is the '-' key.
+ and - (increment / decrement address) are the up and down-arrow keys.
GO (execute program at current address) is the X key.
SHIFT - later monitor versions utilised an extra shift button. Hold
        it down and press another key (use Left Shift).

Whenever a program listing mentions RESET, do a Soft Reset.

Each key causes a beep to be heard. You may need to press more than once
to get it to register.

Inbuilt games - press the following sequence of keys:
- Welcome: RESET D 1 + 0 2 AD 0 2 7 0 GO GO (Paste: D1^02 -0270XX)
- Nim: RESET AD 3 E 0 GO GO (Paste: -3E0XX)
- Invaders: RESET AD 3 2 0 GO GO (Paste: -320XX)
- Luna Lander: RESET AD 4 9 0 GO GO (Paste: -490XX)

Thanks to Chris Schwartz who dumped his ROM for me way back in the old days.
It's only taken 25 years to get around to emulating it...


Differences between tec1 and tecjmon:

On the tec1 a keypress is indicated by an NMI from the 74C923; but on
the jmon it sets bit 6 of port 3 low instead. The NMI code is simply
a 'retn' in the jmon rom, so we can use the same code.
The jmon includes a cassette interface, a serial input connection,
and an optional LCD, but the games of the tec1 have been removed.


ToDo:
- After a Soft Reset, pressing keys can crash the emulation.
- The 74C923 code may need to be revisited to improve keyboard response.
  Sometimes have to press a key a few times before it registers.
- The 10ms debounce is not emulated.


JMON ToDo:
- Add LCD display (2 rows by 16 characters)


***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "imagedev/cassette.h"
#include "tec1.lh"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define MACHINE_START_MEMBER(name) void name::machine_start()

class tec1_state : public driver_device
{
public:
	tec1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_speaker(*this, SPEAKER_TAG),
	m_cass(*this, CASSETTE_TAG),
	m_wave(*this, WAVE_TAG),
	m_key_pressed(0)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	optional_device<cassette_image_device> m_cass;
	optional_device<device_t> m_wave;
	emu_timer *m_kbd_timer;
	DECLARE_READ8_MEMBER( tec1_kbd_r );
	DECLARE_READ8_MEMBER( latch_r );
	DECLARE_WRITE8_MEMBER( tec1_digit_w );
	DECLARE_WRITE8_MEMBER( tecjmon_digit_w );
	DECLARE_WRITE8_MEMBER( tec1_segment_w );
	UINT8 m_kbd;
	UINT8 m_segment;
	UINT8 m_digit;
	UINT8 m_kbd_row;
	UINT8 m_refresh[6];
	bool m_key_pressed;
	UINT8 tec1_convert_col_to_bin( UINT8 col, UINT8 row );
	virtual void machine_reset();
	virtual void machine_start();
};




/***************************************************************************

    Display

***************************************************************************/

WRITE8_MEMBER( tec1_state::tec1_segment_w )
{
/*  d7 segment d
    d6 segment e
    d5 segment c
    d4 segment dot
    d3 segment b
    d2 segment g
    d1 segment f
    d0 segment a */

	m_segment = BITSWAP8(data, 4, 2, 1, 6, 7, 5, 3, 0);
}

WRITE8_MEMBER( tec1_state::tec1_digit_w )
{
/*  d7 speaker
    d6 not used
    d5 data digit 1
    d4 data digit 2
    d3 address digit 1
    d2 address digit 2
    d1 address digit 3
    d0 address digit 4 */

	speaker_level_w(m_speaker, BIT(data, 7));

	m_digit = data & 0x3f;
}

WRITE8_MEMBER( tec1_state::tecjmon_digit_w )
{
/*  d7 speaker & cassout
    d6 not used
    d5 data digit 1
    d4 data digit 2
    d3 address digit 1
    d2 address digit 2
    d1 address digit 3
    d0 address digit 4 */

	speaker_level_w(m_speaker, BIT(data, 7));
	m_cass->output(BIT(data, 7) ? -1.0 : +1.0);
	m_digit = data & 0x3f;
}


/***************************************************************************

    Keyboard

***************************************************************************/

READ8_MEMBER( tec1_state::latch_r )
{
// bit 7 - cass in ; bit 6 low = key pressed
	UINT8 data = (m_key_pressed) ? 0 : 0x40;

	if (m_cass->input() > 0.03)
		data |= 0x80;

	return data;
}


READ8_MEMBER( tec1_state::tec1_kbd_r )
{
	cputag_set_input_line(machine(), "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
	return m_kbd | ioport("SHIFT")->read();
}

UINT8 tec1_state::tec1_convert_col_to_bin( UINT8 col, UINT8 row )
{
	UINT8 data = row;

	if (BIT(col, 1))
		data |= 4;
	else
	if (BIT(col, 2))
		data |= 8;
	else
	if (BIT(col, 3))
		data |= 12;
	else
	if (BIT(col, 4))
		data |= 16;

	return data;
}

static TIMER_CALLBACK( tec1_kbd_callback )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };
	tec1_state *state = machine.driver_data<tec1_state>();
	UINT8 i;

    // Display the digits. Blank any digits that haven't been refreshed for a while.
    // This will fix the problem reported by a user.
	for (i = 0; i < 6; i++)
	{
		if (BIT(state->m_digit, i))
		{
			state->m_refresh[i] = 1;
			output_set_digit_value(i, state->m_segment);
		}
		else
		if (state->m_refresh[i] == 0x80)
		{
			output_set_digit_value(i, 0);
			state->m_refresh[i] = 0;
		}
		else
		if (state->m_refresh[i])
			state->m_refresh[i]++;
	}

    // 74C923 4 by 5 key encoder.
    // if previous key is still held, bail out
	if (machine.root_device().ioport(keynames[state->m_kbd_row])->read())
		if (state->tec1_convert_col_to_bin(machine.root_device().ioport(keynames[state->m_kbd_row])->read(), state->m_kbd_row) == state->m_kbd)
			return;

	state->m_kbd_row++;
	state->m_kbd_row &= 3;

	/* see if a key pressed */
	if (machine.root_device().ioport(keynames[state->m_kbd_row])->read())
	{
		state->m_kbd = state->tec1_convert_col_to_bin(machine.root_device().ioport(keynames[state->m_kbd_row])->read(), state->m_kbd_row);
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, HOLD_LINE);
		state->m_key_pressed = TRUE;
	}
	else
		state->m_key_pressed = FALSE;
}


/***************************************************************************

    Machine

***************************************************************************/

MACHINE_START_MEMBER( tec1_state )
{
	m_kbd_timer = machine().scheduler().timer_alloc(FUNC(tec1_kbd_callback));
	m_kbd_timer->adjust( attotime::zero, 0, attotime::from_hz(500) );
}

MACHINE_RESET_MEMBER( tec1_state )
{
	m_kbd = 0;
}



/***************************************************************************

    Address Map

***************************************************************************/

static ADDRESS_MAP_START( tec1_map, AS_PROGRAM, 8, tec1_state )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x0fff) AM_RAM // on main board
	AM_RANGE(0x1000, 0x3fff) AM_RAM // expansion
ADDRESS_MAP_END

static ADDRESS_MAP_START( tec1_io, AS_IO, 8, tec1_state )
	ADDRESS_MAP_GLOBAL_MASK(0x07)
	AM_RANGE(0x00, 0x00) AM_READ(tec1_kbd_r)
	AM_RANGE(0x01, 0x01) AM_WRITE(tec1_digit_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tec1_segment_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( tecjmon_map, AS_PROGRAM, 8, tec1_state )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x37ff) AM_RAM
	AM_RANGE(0x3800, 0x3fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tecjmon_io, AS_IO, 8, tec1_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READ(tec1_kbd_r)
	AM_RANGE(0x01, 0x01) AM_WRITE(tecjmon_digit_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tec1_segment_w)
	AM_RANGE(0x03, 0x03) AM_READ(latch_r)
	//AM_RANGE(0x04, 0x04) AM_WRITE(lcd_en_w)
	//AM_RANGE(0x84, 0x84) AM_WRITE(lcd_2nd_w)
ADDRESS_MAP_END


/**************************************************************************

    Keyboard Layout

***************************************************************************/

static INPUT_PORTS_START( tec1 )
	PORT_START("LINE0") /* KEY ROW 0 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)	PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)	PORT_CHAR('4')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)	PORT_CHAR('8')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_UP)	PORT_CHAR('^')

	PORT_START("LINE1") /* KEY ROW 1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_DOWN)	PORT_CHAR('V')

	PORT_START("LINE2") /* KEY ROW 2 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)	PORT_CHAR('2')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)	PORT_CHAR('6')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)	PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_X)	PORT_CHAR('X')

	PORT_START("LINE3") /* KEY ROW 3 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)	PORT_CHAR('3')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)	PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("AD") PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')

	PORT_START("SHIFT")
	PORT_BIT(0x1f, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


/***************************************************************************

    Machine driver

***************************************************************************/

static MACHINE_CONFIG_START( tec1, tec1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 1000000)	/* speed can be varied between 250kHz and 2MHz */
	MCFG_CPU_PROGRAM_MAP(tec1_map)
	MCFG_CPU_IO_MAP(tec1_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_tec1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( tecjmon, tec1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_579545MHz / 2)
	MCFG_CPU_PROGRAM_MAP(tecjmon_map)
	MCFG_CPU_IO_MAP(tecjmon_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_tec1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* Devices */
	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )
MACHINE_CONFIG_END


/***************************************************************************

    Game driver

***************************************************************************/

ROM_START(tec1)
	ROM_REGION(0x10000, "maincpu", ROMREGION_ERASEFF)
	ROM_SYSTEM_BIOS(0, "mon1", "MON1")
	ROMX_LOAD("mon1.rom",    0x0000, 0x0800, CRC(b3390c36) SHA1(18aabc68d473206b7fc4e365c6b57a4e218482c3), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "mon1b", "MON1B")
	ROMX_LOAD("mon1b.rom",   0x0000, 0x0800, CRC(60daea3c) SHA1(383b7e7f02e91fb18c87eb03c5949e31156771d4), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "mon2", "MON2")
	ROMX_LOAD("mon2.rom",   0x0000, 0x0800, CRC(082fd7e7) SHA1(7659add30ca22b15a03d1cbac0892a5c25e47ecd), ROM_BIOS(3))
ROM_END

ROM_START(tecjmon)
	ROM_REGION(0x10000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD("jmon.rom",    0x0000, 0x0800, CRC(202c47a2) SHA1(701588ec5640d633d90d94b2ccd6f65422e19a70) )
	ROM_LOAD("util.rom",    0x3800, 0x0800, CRC(7c19700d) SHA1(dc5b3ade66bb11c54430056966ed99cdd299d82b) )
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE     INPUT    INIT       COMPANY                     FULLNAME */
COMP( 1984, tec1,     0,      0,      tec1,       tec1, driver_device,    0,    "Talking Electronics magazine",  "TEC-1" , 0 )
COMP( 1984, tecjmon,  tec1,   0,      tecjmon,    tec1, driver_device,    0,    "Talking Electronics magazine",  "TEC-1A with JMON" , 0 )
