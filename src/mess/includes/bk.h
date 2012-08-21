/*****************************************************************************
 *
 * includes/bk.h
 *
 ****************************************************************************/

#ifndef BK_H_
#define BK_H_


class bk_state : public driver_device
{
public:
	bk_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_bk0010_video_ram(*this, "video_ram"){ }

	UINT16 m_scrool;
	required_shared_ptr<UINT16> m_bk0010_video_ram;
	UINT16 m_kbd_state;
	UINT16 m_key_code;
	UINT16 m_key_pressed;
	UINT16 m_key_irq_vector;
	UINT16 m_drive;
	DECLARE_READ16_MEMBER(bk_key_state_r);
	DECLARE_READ16_MEMBER(bk_key_code_r);
	DECLARE_READ16_MEMBER(bk_vid_scrool_r);
	DECLARE_READ16_MEMBER(bk_key_press_r);
	DECLARE_WRITE16_MEMBER(bk_key_state_w);
	DECLARE_WRITE16_MEMBER(bk_vid_scrool_w);
	DECLARE_WRITE16_MEMBER(bk_key_press_w);
	DECLARE_READ16_MEMBER(bk_floppy_cmd_r);
	DECLARE_WRITE16_MEMBER(bk_floppy_cmd_w);
	DECLARE_READ16_MEMBER(bk_floppy_data_r);
	DECLARE_WRITE16_MEMBER(bk_floppy_data_w);
};


/*----------- defined in machine/bk.c -----------*/

extern MACHINE_START( bk0010 );
extern MACHINE_RESET( bk0010 );



/*----------- defined in video/bk.c -----------*/

extern VIDEO_START( bk0010 );
extern SCREEN_UPDATE_IND16( bk0010 );

#endif /* BK_H_ */
