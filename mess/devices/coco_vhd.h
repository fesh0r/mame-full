#ifndef COCOVHD_H
#define COCOVHD_H

#include "fileio.h"
#include "mess.h"
#include "includes/dragon.h"

DEVICE_INIT(coco_vhd);
DEVICE_LOAD(coco_vhd);

READ_HANDLER(coco_vhd_io_r);
WRITE_HANDLER(coco_vhd_io_w);

#define CONFIG_DEVICE_COCOVHD \
	CONFIG_DEVICE_LEGACY(IO_VHD, 1, "vhd\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE, device_init_coco_vhd, NULL, device_load_coco_vhd, NULL, NULL)

#endif /* COCOVHD_H */
