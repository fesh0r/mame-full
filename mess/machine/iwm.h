/*********************************************************************

	iwm.h
	
	Implementation of the IWM (Integrated Woz Machine) chip

	Nate Woods
	Raphael Nabet

*********************************************************************/

#ifndef IWM_H
#define IWM_H

#include "driver.h"

typedef struct iwm_interface
{
	void (*set_lines)(data8_t lines);
	void (*set_enable_lines)(int enable_mask);

	data8_t (*read_data)(void);
	void (*write_data)(data8_t data);
	int (*read_status)(void);
} iwm_interface;

void iwm_init(const iwm_interface *intf);
data8_t iwm_r(offs_t offset);
void iwm_w(offs_t offset, data8_t data);
data8_t iwm_get_lines(void);

/* mask for IWM lines */
enum {
	IWM_PH0		= 0x01,
	IWM_PH1		= 0x02,
	IWM_PH2		= 0x04,
	IWM_PH3		= 0x08,
	IWM_MOTOR	= 0x10,	/* private, do not use ! */
	IWM_DRIVE	= 0x20,	/* private, do not use ! */
	IWM_Q6		= 0x40,	/* private, do not use ! */
	IWM_Q7		= 0x80	/* private, do not use ! */
};


#endif /* IWM_H */
