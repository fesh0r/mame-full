/*****************************************************************************
 *
 * includes/primo.h
 *
 ****************************************************************************/

#ifndef PRIMO_H_
#define PRIMO_H_

#include "imagedev/snapquik.h"
#include "machine/cbmiec.h"

class primo_state : public driver_device
{
public:
	primo_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_iec(*this, CBM_IEC_TAG)
    { }

	required_device<cbm_iec_device> m_iec;

	UINT16 m_video_memory_base;
	UINT8 m_port_FD;
	int m_nmi;
	DECLARE_READ8_MEMBER(primo_be_1_r);
	DECLARE_READ8_MEMBER(primo_be_2_r);
	DECLARE_WRITE8_MEMBER(primo_ki_1_w);
	DECLARE_WRITE8_MEMBER(primo_ki_2_w);
	DECLARE_WRITE8_MEMBER(primo_FD_w);
	DECLARE_DRIVER_INIT(primo48);
	DECLARE_DRIVER_INIT(primo64);
	DECLARE_DRIVER_INIT(primo32);
};


/*----------- defined in machine/primo.c -----------*/

extern MACHINE_RESET( primoa );
extern MACHINE_RESET( primob );
extern INTERRUPT_GEN( primo_vblank_interrupt );
extern SNAPSHOT_LOAD( primo );
extern QUICKLOAD_LOAD( primo );


/*----------- defined in video/primo.c -----------*/

extern SCREEN_UPDATE_IND16( primo );


#endif /* PRIMO_H_ */
