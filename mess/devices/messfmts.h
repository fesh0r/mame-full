#ifndef MESSFMTS_H
#define MESSFMTS_H

#include "formats.h"
#include "messdrv.h"

const struct IODevice *bdf_device_specify(struct IODevice *iodev, char *extbuf, size_t extbuflen,
	int count, const formatdriver_ctor *open_formats, formatdriver_ctor create_format);

#define CONFIG_DEVICE_FLOPPY(count, open_formats, create_format)							\
	if (cfg->device_num-- == 0)																\
	{																						\
		static struct IODevice iodev;														\
		static char extbuf[33];																\
		cfg->dev = bdf_device_specify(&iodev, extbuf, sizeof(extbuf) / sizeof(extbuf[0]),	\
			count, formatchoices_##open_formats, construct_formatdriver_##create_format);	\
	}																						\

#endif
