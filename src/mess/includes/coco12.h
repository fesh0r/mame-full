/***************************************************************************

    coco12.h

    TRS-80 Radio Shack Color Computer 1/2 Family

***************************************************************************/

#pragma once

#ifndef __COCO12__
#define __COCO12__


#include "includes/coco.h"
#include "machine/6883sam.h"
#include "video/mc6847.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define SAM_TAG			"sam"
#define VDG_TAG			"vdg"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class coco12_state : public coco_state
{
public:
	coco12_state(const machine_config &mconfig, device_type type, const char *tag);

	required_device<sam6883_device> m_sam;
	required_device<mc6847_base_device> m_vdg;

	static const mc6847_interface mc6847_config;
	static const sam6883_interface sam6883_config;

protected:
	virtual void device_start();
	virtual void update_cart_base(UINT8 *cart_base);

	/* PIA1 */
	virtual void pia1_pb_changed(void);

private:
	DECLARE_WRITE_LINE_MEMBER( horizontal_sync );
	DECLARE_WRITE_LINE_MEMBER( field_sync );

	DECLARE_READ8_MEMBER( sam_read );

	void configure_sam(void);
};


#endif /* __COCO12__ */
