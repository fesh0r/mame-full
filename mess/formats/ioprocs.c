#include <stdio.h>
#include "ioprocs.h"

static void stdio_closeproc(void *file)
{
	fclose(file);
}

static int stdio_seekproc(void *file, INT64 offset, int whence)
{
	return fseek(file, (long) offset, whence);
}

static size_t stdio_readproc(void *file, void *buffer, size_t length)
{
	return fread(buffer, 1, length, file);
}

static size_t stdio_writeproc(void *file, const void *buffer, size_t length)
{
	return fwrite(buffer, 1, length, file);
}

static UINT64 stdio_filesizeproc(void *file)
{
	long l, sz;
	l = ftell(file);
	if (fseek(file, 0, SEEK_END))
		return (size_t) -1;
	sz = ftell(file);
	if (fseek(file, l, SEEK_SET))
		return (size_t) -1;
	return (size_t) sz;
}

struct io_procs stdio_ioprocs =
{
	stdio_closeproc,
	stdio_seekproc,
	stdio_readproc,
	stdio_writeproc,
	stdio_filesizeproc
};

struct io_procs stdio_ioprocs_noclose =
{
	NULL,
	stdio_seekproc,
	stdio_readproc,
	stdio_writeproc,
	stdio_filesizeproc
};

