#ifndef CARTSLOT_H
#define CARTSLOT_H

#include <stdlib.h>
#include "messdrv.h"

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	int (*initproc)(int id, mame_file *fp, int open_mode),
	void (*exitproc)(int id),
	UINT32 (*partialcrc)(const unsigned char *buf, unsigned int size));

#define CONFIG_DEVICE_CARTSLOT(count, file_extensions, init, exit, partialcrc)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = cartslot_specify(&iodev, (count),					\
			(file_extensions), (init), (exit), (partialcrc));			\
	}																	\

#endif /* CARTSLOT_H */
