#ifndef PRINTER_H
#define PRINTER_H

#define MAX_PRINTER	(4)

#ifdef __cplusplus
extern "C" {
#endif

#include "messdrv.h"

/*
 * functions for the IODevice entry IO_PRINTER
 * 
 * Currently only a simple port which only supports status 
 * ("online/offline") and output of bytes is supported.
 */

int printer_load(int id, mame_file *fp, int open_mode);
int printer_status(int id, int newstatus);
void printer_output(int id, int data);

#define CONFIG_DEVICE_PRINTER(count)												\
	CONFIG_DEVICE_BASE(IO_PRINTER, (count), "prn\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_WRITE,		\
		NULL, NULL, printer_load, NULL, NULL, NULL, NULL, NULL, printer_status, NULL, NULL,	\
		NULL, printer_output, NULL, NULL)	\

#ifdef __cplusplus
}
#endif

#endif /* PRINTER_H */
