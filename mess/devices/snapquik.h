#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdlib.h>
#include "messdrv.h"

typedef int (*snapquick_loadproc)(mame_file *fp, const char *file_type, int file_size);

#define SNAPSHOT_LOAD(name)		int snapshot_load_##name(mame_file *fp, const char *file_type, int snapshot_size)
#define QUICKLOAD_LOAD(name)	int quickload_load_##name(mame_file *fp, const char *file_type, int quickload_size)

void snapshot_device_getinfo(struct IODevice *iodev,
	snapquick_loadproc loadproc, double delay);
void quickload_device_getinfo(struct IODevice *iodev,
	snapquick_loadproc loadproc, double delay);

#endif
