/***************************************************************************

    dgnalpha.h

    Dragon Alpha

***************************************************************************/

#pragma once

#ifndef __DGNALPHA__
#define __DGNALPHA__


#include "includes/dragon.h"
#include "sound/ay8910.h"
#include "machine/wd17xx.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

/* devices */
#define PIA2_TAG					"pia2"
#define AY8912_TAG					"ay8912"
#define WD2797_TAG					"wd2797"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class dragon_alpha_state : public dragon64_state
{
public:
	dragon_alpha_state(const machine_config &mconfig, device_type type, const char *tag);

	required_device<pia6821_device> m_pia_2;
	required_device<device_t> m_ay8912;
	required_device<device_t> m_fdc;

	static const pia6821_interface pia2_config;
	static const ay8910_interface ay8912_interface;
	static const wd17xx_interface fdc_interface;

protected:
	/* driver overrides */
	virtual void device_start(void);
	virtual void device_reset(void);

	/* interrupts */
	virtual bool firq_get_line(void);

	/* PIA1 */
	virtual DECLARE_READ8_MEMBER( ff20_read );
	virtual DECLARE_WRITE8_MEMBER( ff20_write );

private:
	UINT8 m_just_reset;

	/* pia2 */
	DECLARE_WRITE8_MEMBER( pia2_pa_w );
	DECLARE_WRITE_LINE_MEMBER( pia2_firq_a );
	DECLARE_WRITE_LINE_MEMBER( pia2_firq_b );

	/* psg */
	DECLARE_READ8_MEMBER( psg_porta_read );
	DECLARE_WRITE8_MEMBER( psg_porta_write );

	/* fdc */
	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );

	/* modem */
	UINT8 modem_r(offs_t offset);
	void modem_w(offs_t offset, UINT8 data);
};

#endif /* __DGNALPHA__ */
