#include "cartslot.h"
#include "mess.h"

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	int (*initproc)(mess_image *img),
	void (*exitproc)(mess_image *img),
	int (*loadproc)(mess_image *img, mame_file *fp, int open_mode),
	void (*unloadproc)(mess_image *img),
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
	iodev->imgverify = verify;
	iodev->partialcrc = partialcrc;
	return iodev;
}

int cartslot_load_generic(mame_file *fp, int memregion, UINT32 offset, UINT32 minsize, UINT32 maxsize, int flags)
{
	UINT8 *mem;
	UINT8 *p;
	UINT32 bytes_read;
	UINT64 size;
	UINT32 s;

	mem = memory_region(memregion);

	assert(mem);
	assert((offset + maxsize) <= memory_region_length(memregion));
	assert(minsize <= maxsize);
	assert(minsize > 0);

	size = mame_fsize(fp);
	if ((size < minsize) || (size > maxsize))
		return INIT_FAIL;

	if (flags & CARTLOAD_MUSTBEPOWEROFTWO)
	{
		/* quick test to see if size is a power of two */
		s = size;
		while(s > 1)
		{
			if (s & 1)
				return INIT_FAIL;
			s = s / 2;
		}
	}

	mem += offset;
	memset(mem, 0, maxsize);

	bytes_read = mame_fread(fp, mem, maxsize);
	if (bytes_read != size)
		return INIT_FAIL;

	if (flags & CARTLOAD_MIRROR)
	{
		p = mem + bytes_read;
		maxsize -= bytes_read;

		while(maxsize > 0)
		{
			s = MIN(bytes_read, maxsize);
			memcpy(p, mem, s);
			p += s;
			maxsize -= s;
		}
	}
	return INIT_PASS;
}
