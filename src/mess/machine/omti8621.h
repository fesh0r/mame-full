/*
 * omti8621.h - SMS OMTI 8621 disk controller
 *
 *  Created on: August 30, 2010
 *      Author: Hans Ostermeyer
 *
 *  Released for general non-commercial use under the MAME license
 *  Visit http://mamedev.org for licensing and usage restrictions.
 *
 */

#pragma once

#ifndef OMTI8621_H_
#define OMTI8621_H_

#include "emu.h"

/***************************************************************************
 TYPE DEFINITIONS
 ***************************************************************************/

typedef void (*omti8621_set_irq)(const running_machine*, int);

typedef struct _omti8621_config omti8621_config;
struct _omti8621_config {
	omti8621_set_irq set_irq;
};

/***************************************************************************
 DEVICE CONFIGURATION MACROS
 ***************************************************************************/

#define MCFG_OMTI8621_ADD(_tag, _config) \
	MCFG_FRAGMENT_ADD( omti_disk ) \
	MCFG_DEVICE_ADD(_tag, OMTI8621, 0) \
	MCFG_DEVICE_CONFIG(_config)

/***************************************************************************
 FUNCTION PROTOTYPES
 ***************************************************************************/

READ16_DEVICE_HANDLER( omti8621_r );
WRITE16_DEVICE_HANDLER( omti8621_w );

void omti8621_set_verbose(int on_off);

// get sector diskaddr of logical unit lun into data_buffer
UINT32 omti8621_get_sector(device_t *device, INT32 diskaddr, UINT8 *data_buffer, UINT32 length, UINT8 lun);

/* ----- device interface ----- */

DECLARE_LEGACY_DEVICE(OMTI8621, omti8621);
MACHINE_CONFIG_EXTERN( omti_disk );

//###############################################################NEWNEW

#endif /* OMTI8621_H_ */
