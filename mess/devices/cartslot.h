#ifndef CARTSLOT_H
#define CARTSLOT_H

#include <stdlib.h>
#include "messdrv.h"

enum
{
	CARTLOAD_MUSTBEPOWEROFTWO	= 1,
	CARTLOAD_MIRROR				= 2
};

int cartslot_load_generic(mame_file *fp, int memregion, UINT32 offset, UINT32 minsize, UINT32 maxsize, int flags);

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	device_init_handler initproc,
	device_exit_handler exitproc,
	device_load_handler loadproc,
	device_unload_handler unloadproc,
	int (*verify)(const UINT8 *buf, size_t size),
	UINT32 (*partialcrc)(const UINT8 *buf, size_t size));

/* required cartridge slot */
#define CONFIG_DEVICE_CARTSLOT_REQ(count, file_extensions, init, exit, load, unload, imgverify, partialcrc)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = cartslot_specify(&iodev, (count),					\
			(file_extensions), (init), (exit), (load), (unload),		\
			(imgverify), (partialcrc));									\
		iodev.flags |= DEVICE_MUST_BE_LOADED;							\
	}																	\

/* optional cartridge slot */
#define CONFIG_DEVICE_CARTSLOT_OPT(count, file_extensions, init, exit, load, unload, imgverify, partialcrc)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = cartslot_specify(&iodev, (count),					\
			(file_extensions), (init), (exit), (load), (unload),		\
			(imgverify), (partialcrc));									\
	}																	\

#endif /* CARTSLOT_H */
