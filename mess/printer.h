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

extern int printer_init (int id);
extern void printer_exit (int id);
extern int printer_status (int id, int newstatus);
extern void printer_output (int id, int data);
extern int printer_output_chunk (int id, void *src, int chunks);

#define CONFIG_DEVICE_PRINTER_EX(count, init, output)										\
	CONFIG_DEVICE(IO_PRINTER, (count), "prn\0", IO_RESET_NONE, OSD_FOPEN_WRITE, (init),		\
		printer_exit, NULL, NULL, NULL, printer_status, NULL, NULL, NULL, (output), NULL)	\

#define CONFIG_DEVICE_PRINTER(count)														\
	CONFIG_DEVICE_PRINTER_EX((count), printer_init, printer_output)							\

#ifdef __cplusplus
}
#endif

#endif /* PRINTER_H */
