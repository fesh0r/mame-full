#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdlib.h>
#include "messdrv.h"

typedef int (*snapquick_loadproc)(void *fp, const char *file_type, int file_size);

#define SNAPSHOT_LOAD(name)		int snapshot_load_##name(void *fp, const char *file_type, int snapshot_size)
#define QUICKLOAD_LOAD(name)	int quickload_load_##name(void *fp, const char *file_type, int quickload_size)

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
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_SNAPSHOT, (file_extensions), (snapshot_load_##load), 0.0)

#define CONFIG_DEVICE_QUICKLOAD(file_extensions, load)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_QUICKLOAD, (file_extensions), (quickload_load_##load), 0.0)

#define CONFIG_DEVICE_SNAPSHOT_DELAY(file_extensions, load, delay)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_SNAPSHOT, (file_extensions), (snapshot_load_##load), (delay))

#define CONFIG_DEVICE_QUICKLOAD_DELAY(file_extensions, load, delay)	\
	CONFIG_DEVICE_SNAPQUICKLOAD(IO_QUICKLOAD, (file_extensions), (quickload_load_##load), (delay))

#endif
