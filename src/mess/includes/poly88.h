/*****************************************************************************
 *
 * includes/poly88.h
 *
 ****************************************************************************/

#ifndef POLY88_H_
#define POLY88_H_

#include "machine/i8251.h"
#include "imagedev/snapquik.h"

class poly88_state : public driver_device
{
public:
	poly88_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_video_ram(*this, "video_ram"){ }

	required_shared_ptr<UINT8> m_video_ram;
	UINT8 *m_FNT;
	UINT8 m_intr;
	UINT8 m_last_code;
	UINT8 m_int_vector;
	emu_timer * m_cassette_timer;
	emu_timer * m_usart_timer;
	int m_previous_level;
	int m_clk_level;
	int m_clk_level_tape;
	DECLARE_WRITE8_MEMBER(poly88_baud_rate_w);
	DECLARE_READ8_MEMBER(poly88_keyboard_r);
	DECLARE_WRITE8_MEMBER(poly88_intr_w);
	DECLARE_DRIVER_INIT(poly88);
};


/*----------- defined in machine/poly88.c -----------*/

MACHINE_RESET(poly88);
INTERRUPT_GEN( poly88_interrupt );

extern const i8251_interface poly88_usart_interface;

extern SNAPSHOT_LOAD( poly88 );


/*----------- defined in video/poly88.c -----------*/

extern VIDEO_START( poly88 );
extern SCREEN_UPDATE_IND16( poly88 );

#endif /* POLY88_H_ */

