#include "cartslot.h"
#include "mess.h"

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	int (*initproc)(int id, mame_file *fp, int open_mode),
	void (*exitproc)(int id),
	UINT32 (*partialcrc)(const unsigned char *buf, unsigned int size))
{
	memset(iodev, 0, sizeof(*iodev));
	iodev->type = IO_CARTSLOT;
	iodev->count = count;
	iodev->file_extensions = file_extensions;
	iodev->flags = DEVICE_LOAD_RESETS_CPU;
	iodev->open_mode = OSD_FOPEN_READ;
	iodev->init = initproc;
	iodev->exit = exitproc;
	iodev->partialcrc = partialcrc;
	return iodev;
}

