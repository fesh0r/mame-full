#ifndef DSK_H
#define DSK_H

#include "messdrv.h"

int dsk_floppy_load(mess_image *img, mame_file *fp, int open_mode);
void dsk_floppy_unload(mess_image *img);

#define CONFIG_DEVICE_LEGACY_DSK(count) \
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, (count), "dsk\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_OR_READ, NULL, NULL, dsk_floppy_load, dsk_floppy_unload, floppy_status)


#endif /* DSK_H */
