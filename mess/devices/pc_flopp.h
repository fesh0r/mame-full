#ifndef PC_FLOPP_H
#define PC_FLOPP_H

#include "devices/basicdsk.h"

#ifdef __cplusplus
extern "C" {
#endif

DEVICE_LOAD(pc_floppy);

#define CONFIG_DEVICE_PC_FLOPPY(count)	\
	CONFIG_DEVICE_FLOPPY_BASICDSK(count, "dsk\0", device_load_pc_floppy)

#ifdef __cplusplus
}
#endif

#endif /* PC_FLOPP_H */
