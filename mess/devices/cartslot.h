#ifndef CARTSLOT_H
#define CARTSLOT_H

#include <stdlib.h>
#include "messdrv.h"

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	int (*initproc)(int id),
	void (*exitproc)(int id),
	int (*loadproc)(int id, mame_file *fp, int open_mode),
	void (*unloadproc)(int id),
	int (*verify)(const UINT8 *buf, size_t size),
	UINT32 (*partialcrc)(const UINT8 *buf, size_t size));

/* required cartridge slot */
#define CONFIG_DEVICE_CARTSLOT_REQ(count, file_extensions, init, exit, load, unload, verify, partialcrc)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = cartslot_specify(&iodev, (count),					\
			(file_extensions), (init), (exit), (load), (unload),		\
			(verify), (partialcrc));									\
		iodev.flags |= DEVICE_MUST_BE_LOADED;							\
	}																	\

/* optional cartridge slot */
#define CONFIG_DEVICE_CARTSLOT_OPT(count, file_extensions, init, exit, load, unload, verify, partialcrc)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = cartslot_specify(&iodev, (count),					\
			(file_extensions), (init), (exit), (load), (unload),		\
			(verify), (partialcrc));									\
	}																	\

#endif /* CARTSLOT_H */
