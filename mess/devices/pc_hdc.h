#ifndef PC_HDC_H
#define PC_HDC_H

#include "driver.h"

#define CONFIG_DEVICE_PC_HARDDISK(count) \
	CONFIG_DEVICE_LEGACY(IO_HARDDISK, 4, "img\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_RW, device_init_pc_hdc, NULL, NULL, NULL, NULL)

DEVICE_INIT( pc_hdc );

WRITE_HANDLER ( pc_HDC1_w );
READ_HANDLER (	pc_HDC1_r );
WRITE_HANDLER ( pc_HDC2_w );
READ_HANDLER ( pc_HDC2_r );

void pc_harddisk_state(void);

#endif /* PC_HDC_H */
