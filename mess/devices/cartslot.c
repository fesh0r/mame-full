#include "cartslot.h"
#include "mess.h"

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

const struct IODevice *cartslot_specify(struct IODevice *iodev, int count,
	const char *file_extensions,
	device_init_handler initproc,
	device_exit_handler exitproc,
	device_load_handler loadproc,
	device_unload_handler unloadproc,
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
	UINT32 (*readfile)(mame_file *file, void *buffer, UINT32 length);

	mem = memory_region(memregion);

	assert(mem);
	assert((offset + maxsize) <= memory_region_length(memregion));
	assert(minsize <= maxsize);
	assert(minsize > 0);

	size = mame_fsize(fp);
	if ((size < minsize) || (size > maxsize))
		return INIT_FAIL;

	/* check to see if 16 bit cartridge has odd size */
	if ((flags & (CARTLOAD_16BIT_LE|CARTLOAD_16BIT_BE)) && (size & 1))
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

	/* choose a proper read proc */
	if (flags & CARTLOAD_16BIT_BE)
		readfile = mame_fread_msbfirst;
	else if (flags & CARTLOAD_16BIT_LE)
		readfile = mame_fread_lsbfirst;
	else
		readfile = mame_fread;

	bytes_read = readfile(fp, mem, maxsize);
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
