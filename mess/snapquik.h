#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdlib.h>
#include "messdrv.h"

typedef int (*snapquick_loadproc)(void *fp);

const struct IODevice *snapquick_specify(struct IODevice *iodev,
	int type, const char *file_extensions, snapquick_loadproc loadproc);

#define CONFIG_DEVICE_SNAPQUICKLOAD(type, file_extensions, load)					\
	if (cfg->device_num-- == 0)														\
	{																				\
		static struct IODevice iodev;												\
		cfg->dev = snapquick_specify(&iodev, (type), (file_extensions), (load));	\
	}																				\

#define CONFIG_DEVICE_SNAPSHOT(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_SNAPSHOT, (file_extensions), (load))

#define CONFIG_DEVICE_QUICKLOAD(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_QUICKLOAD, (file_extensions), (load))

#endif
