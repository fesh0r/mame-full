#ifndef COCOVHD_H
#define COCOVHD_H

#include "fileio.h"
#include "mess.h"
#include "includes/dragon.h"

int coco_vhd_init(mess_image *img);
int coco_vhd_load(mess_image *img, mame_file *fp, int open_mode);

READ_HANDLER(coco_vhd_io_r);
WRITE_HANDLER(coco_vhd_io_w);

#define CONFIG_DEVICE_COCOVHD \
	CONFIG_DEVICE_LEGACY(IO_VHD, 1, "vhd\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE, coco_vhd_init, NULL, coco_vhd_load, NULL, NULL)

#endif /* COCOVHD_H */
