#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdlib.h>
#include "messdrv.h"

typedef int (*snapquick_loadproc)(void *fp);

const struct IODevice *snapquick_specify(struct IODevice *iodev, int type,
	const char *file_extensions, snapquick_loadproc loadproc, double delay);

#define CONFIG_DEVICE_SNAPQUICKLOAD(type, file_extensions, load, delay)	\
	if (cfg->device_num-- == 0)											\
	{																	\
		static struct IODevice iodev;									\
		cfg->dev = snapquick_specify(&iodev, (type),					\
			(file_extensions), (load), (delay));						\
	}																	\

#define CONFIG_DEVICE_SNAPSHOT(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_SNAPSHOT, (file_extensions), (load), 0.0)

#define CONFIG_DEVICE_QUICKLOAD(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_QUICKLOAD, (file_extensions), (load), 0.0)

#define CONFIG_DEVICE_SNAPSHOT_DELAY(file_extensions, load, delay)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_SNAPSHOT, (file_extensions), (load), (delay))

#define CONFIG_DEVICE_QUICKLOAD_DELAY(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_QUICKLOAD, (file_extensions), (load), (delay))

#endif
