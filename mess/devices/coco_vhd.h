/***************************************************************************

	coco_vhd.h

	Color Computer Virtual Hard Drives

***************************************************************************/

#ifndef COCOVHD_H
#define COCOVHD_H

#include "fileio.h"
#include "mess.h"
#include "includes/dragon.h"

DEVICE_INIT(coco_vhd);
DEVICE_LOAD(coco_vhd);

READ8_HANDLER(coco_vhd_io_r);
WRITE8_HANDLER(coco_vhd_io_w);

void coco_vhd_device_getinfo(struct IODevice *dev);

#endif /* COCOVHD_H */
