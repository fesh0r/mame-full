#ifndef PC_FLOPP_H
#define PC_FLOPP_H

#include "devices/basicdsk.h"

#ifdef __cplusplus
extern "C" {
#endif

int pc_floppy_load(mess_image *img, mame_file *fp, int open_mode);

#define CONFIG_DEVICE_PC_FLOPPY(count)	\
	CONFIG_DEVICE_FLOPPY_BASICDSK(count, "dsk\0", pc_floppy_load)

#ifdef __cplusplus
}
#endif

#endif /* PC_FLOPP_H */
