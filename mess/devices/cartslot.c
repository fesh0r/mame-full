#include "cartslot.h"
#include "mess.h"

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	int (*initproc)(int id),
	void (*exitproc)(int id),
	int (*loadproc)(int id, mame_file *fp, int open_mode),
	void (*unloadproc)(int id),
	int (*verify)(const UINT8 *buf, size_t size),
	UINT32 (*partialcrc)(const UINT8 *buf, size_t size))
{
	memset(iodev, 0, sizeof(*iodev));
	iodev->type = IO_CARTSLOT;
	iodev->count = count;
	iodev->file_extensions = file_extensions;
	iodev->flags = DEVICE_LOAD_RESETS_CPU | DEVICE_LOAD_AT_INIT;
	iodev->open_mode = OSD_FOPEN_READ;
	iodev->init = initproc;
	iodev->exit = exitproc;
	iodev->load = loadproc;
	iodev->unload = unloadproc;
	iodev->verify = verify;
	iodev->partialcrc = partialcrc;
	return iodev;
}

int cartslot_load_generic(mame_file *fp, int memregion, UINT32 offset, UINT32 minsize, UINT32 maxsize)
{
	UINT8 *mem;
	UINT32 bytes_read;

	mem = memory_region(memregion);

	assert(mem);
	assert((offset + maxsize) <= memory_region_length(memregion));
	assert(minsize <= maxsize);

	mem += offset;
	memset(mem, 0, maxsize);

	bytes_read = mame_fread(fp, mem, maxsize);
	return ((bytes_read >= minsize) && (bytes_read <= maxsize)) ? INIT_PASS : INIT_FAIL;
}
