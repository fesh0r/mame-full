#ifndef MESSFMTS_H
#define MESSFMTS_H

#include "formats.h"
#include "messdrv.h"

int bdf_floppy_init(int id);
void bdf_floppy_exit(int id);

#define CONFIG_DEVICE_FLOPPY(count,fileext,open_formats,create_format)										\
	CONFIG_DEVICE(IO_FLOPPY, (count), (fileext), IO_RESET_NONE, OSD_FOPEN_RW_CREATE, bdf_floppy_init,		\
		bdf_floppy_exit, NULL, NULL, NULL, NULL, NULL, NULL,												\
		(void *) (formatchoices_##open_formats), (void *) (construct_formatdriver_##create_format), NULL)	\

#endif
