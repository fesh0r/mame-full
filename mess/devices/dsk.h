#ifndef DSK_H
#define DSK_H

#include "messdrv.h"

DEVICE_LOAD(dsk_floppy);
DEVICE_UNLOAD(dsk_floppy);

#define CONFIG_DEVICE_LEGACY_DSK(count) \
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, (count), "dsk\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_OR_READ, NULL, NULL, device_load_dsk_floppy, device_unload_dsk_floppy, floppy_status)


#endif /* DSK_H */
