#ifndef CARTSLOT_H
#define CARTSLOT_H

#include <stdlib.h>
#include "messdrv.h"

enum
{
	CARTLOAD_MUSTBEPOWEROFTWO	= 1,
	CARTLOAD_MIRROR				= 2,

	CARTLOAD_MSBFIRST			= 0,
	CARTLOAD_LSBFIRST			= 4,

	CARTLOAD_8BIT				= 0,
	CARTLOAD_16BIT				= 8,
	CARTLOAD_32BIT				= 16,

	CARTLOAD_16BIT_BE			= CARTLOAD_16BIT | CARTLOAD_MSBFIRST,
	CARTLOAD_32BIT_BE			= CARTLOAD_32BIT | CARTLOAD_MSBFIRST,
	CARTLOAD_16BIT_LE			= CARTLOAD_16BIT | CARTLOAD_LSBFIRST,
	CARTLOAD_32BIT_LE			= CARTLOAD_32BIT | CARTLOAD_LSBFIRST
};

int cartslot_load_generic(mame_file *fp, int memregion, UINT32 offset, UINT32 minsize, UINT32 maxsize, int flags);

void cartslot_device_getinfo(struct IODevice *iodev);

#endif /* CARTSLOT_H */
