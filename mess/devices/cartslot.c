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
	device_partialhash_handler partialhash)
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
	iodev->partialhash = partialhash;
	return iodev;
}



int cartslot_load_generic(mame_file *fp, int memregion, UINT32 offset, UINT32 minsize, UINT32 maxsize, int flags)
{
	UINT8 *mem;
	UINT8 *p;
	UINT32 bytes_read;
	UINT64 size;
	UINT32 s, i, j;
	UINT32 modulo;
	UINT8 b;

	mem = memory_region(memregion);

	/* general assertions */
	assert(mem);
	assert((offset + maxsize) <= memory_region_length(memregion));
	assert(minsize <= maxsize);
	assert(minsize > 0);

	/* compute and approve size */
	size = mame_fsize(fp);
	if ((size < minsize) || (size > maxsize))
		return INIT_FAIL;

	/* check to see that the cartridge size is not odd */
	if (flags & CARTLOAD_32BIT)
		modulo = 4;
	else if (flags & CARTLOAD_16BIT)
		modulo = 2;
	else
		modulo = 1;
	if (size % modulo)
		return INIT_FAIL;

	/* check to see that the cartridge size is a power of two, if appropriate */
	if (flags & CARTLOAD_MUSTBEPOWEROFTWO)
	{
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

	/* perform the read */
	bytes_read = mame_fread(fp, mem, maxsize);
	if (bytes_read != size)
		return INIT_FAIL;

	/* do we need to do a byte swap */
#ifdef LSB_FIRST
	if ((flags & CARTLOAD_LSBFIRST) == 0)
#else /* !LSB_FIRST*/
	if ((flags & CARTLOAD_LSBFIRST) != 0)
#endif /* LSB_FIRST */
	{
		for (i = 0; i < bytes_read; i += modulo)
		{
			for (j = 0; j < modulo/2; j++)
			{
				b = mem[i + j];
				mem[i+j] = mem[i + modulo - j - 1];
				mem[i + modulo - j - 1] = b;
			}
		}
	}

	/* mirror the bytes, if necessary */
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
