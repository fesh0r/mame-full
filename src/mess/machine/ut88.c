/***************************************************************************

        UT88 machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "imagedev/cassette.h"
#include "machine/i8255.h"
#include "includes/ut88.h"


/* Driver initialization */
DRIVER_INIT_MEMBER(ut88_state,ut88)
{
	/* set initially ROM to be visible on first bank */
	UINT8 *RAM = memregion("maincpu")->base();
	memset(RAM,0x0000,0x0800); // make first page empty by default
	membank("bank1")->configure_entries(1, 2, RAM, 0x0000);
	membank("bank1")->configure_entries(0, 2, RAM, 0xf800);
}

READ8_MEMBER( ut88_state::ut88_8255_portb_r )
{
	UINT8 i, data = 0xff;
	char kbdrow[12];
	for (i = 0; i < 8; i++)
	{
		if (BIT(m_keyboard_mask, i))
		{
			sprintf(kbdrow,"LINE%d", i);
			data &= ioport(kbdrow)->read();
		}
	}
	return data;
}

READ8_MEMBER( ut88_state::ut88_8255_portc_r )
{
	return ioport("LINE8")->read();
}

WRITE8_MEMBER( ut88_state::ut88_8255_porta_w )
{
	m_keyboard_mask = data ^ 0xff;
}

I8255A_INTERFACE( ut88_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(ut88_state, ut88_8255_porta_w),
	DEVCB_DRIVER_MEMBER(ut88_state, ut88_8255_portb_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(ut88_state, ut88_8255_portc_r),
	DEVCB_NULL,
};

static TIMER_CALLBACK( ut88_reset )
{
	ut88_state *state = machine.driver_data<ut88_state>();
	state->membank("bank1")->set_entry(0);
}

MACHINE_RESET( ut88 )
{
	ut88_state *state = machine.driver_data<ut88_state>();
	machine.scheduler().timer_set(attotime::from_usec(10), FUNC(ut88_reset));
	state->membank("bank1")->set_entry(1);
	state->m_keyboard_mask = 0;
}


READ8_MEMBER( ut88_state::ut88_keyboard_r )
{
	return m_ppi->read(space, offset^0x03);
}


WRITE8_MEMBER( ut88_state::ut88_keyboard_w )
{
	m_ppi->write(space, offset^0x03, data);
}

WRITE8_MEMBER( ut88_state::ut88_sound_w )
{
	m_dac->write_unsigned8(data); //beeper
	m_cass->output(BIT(data, 0) ? 1 : -1);
}


READ8_MEMBER( ut88_state::ut88_tape_r )
{
	double level = m_cass->input();
	return (level <  0) ? 0 : 0xff;
}

READ8_MEMBER( ut88_state::ut88mini_keyboard_r )
{
	// This is real keyboard implementation
	UINT8 *keyrom1 = memregion("proms")->base();
	UINT8 *keyrom2 = memregion("proms")->base()+100;

	UINT8 key = keyrom2[ioport("LINE1")->read()];

	// if keyboard 2nd part returned 0 on 4th bit, output from
	// first part is used

	if (!BIT(key, 3))
		key = keyrom1[ioport("LINE0")->read()];

	// for delete key there is special key producing code 0x80

	key = (BIT(ioport("LINE2")->read(), 7)) ? key : 0x80;

	// If key 0 is pressed its value is 0x10 this is done by additional
	// discrete logic

	key = (BIT(ioport("LINE0")->read(), 0)) ? key : 0x10;
	return key;
}


WRITE8_MEMBER( ut88_state::ut88mini_write_led )
{
	switch(offset)
	{
		case 0: m_lcd_digit[4] = (data >> 4) & 0x0f;
			m_lcd_digit[5] = data & 0x0f;
			break;
		case 1: m_lcd_digit[2] = (data >> 4) & 0x0f;
			m_lcd_digit[3] = data & 0x0f;
			break;
		case 2: m_lcd_digit[0] = (data >> 4) & 0x0f;
			m_lcd_digit[1] = data & 0x0f;
			break;
		}
}

static const UINT8 hex_to_7seg[16] =
{
	 0x3F, 0x06, 0x5B, 0x4F,
	 0x66, 0x6D, 0x7D, 0x07,
	 0x7F, 0x6F, 0x77, 0x7c,
	 0x39, 0x5e, 0x79, 0x71
};

static TIMER_CALLBACK( update_display )
{
	ut88_state *state = machine.driver_data<ut88_state>();
	int i;
	for (i=0;i<6;i++)
		output_set_digit_value(i, hex_to_7seg[state->m_lcd_digit[i]]);
}


DRIVER_INIT_MEMBER(ut88_state,ut88mini)
{
}

MACHINE_START( ut88mini )
{
	machine.scheduler().timer_pulse(attotime::from_hz(60), FUNC(update_display));
}

MACHINE_RESET( ut88mini )
{
	ut88_state *state = machine.driver_data<ut88_state>();
	state->m_lcd_digit[0] = state->m_lcd_digit[1] = state->m_lcd_digit[2] = 0;
	state->m_lcd_digit[3] = state->m_lcd_digit[4] = state->m_lcd_digit[5] = 0;
}

