#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "unzip.h"
#include "osdepend.h"
#include "imgtool.h"
//#include "zlib/zlib.h"

static size_t fsize(FILE *f)
{
	long l, sz;
	l = ftell(f);
	if (fseek(f, 0, SEEK_END))
		return (size_t) -1;
	sz = ftell(f);
	if (fseek(f, l, SEEK_SET))
		return (size_t) -1;
	return (size_t) sz;
}

STREAM *stream_open(const char *fname, int read_or_write)
{
	const char *ext;
	ZIP *z = NULL;
	STREAM *imgfile;
	struct zipent *zipent;
	static char *write_modes[] = {"rb","wb","r+b","r+b","w+b"};

	imgfile = malloc(sizeof(STREAM));

	ext = strrchr(fname, '.');
	if (ext && !stricmp(ext, ".zip")) {
		/* Zip file */
		imgfile->imgtype = IMG_MEM;
		imgfile->write_protect = 1;
		imgfile->u.m.pos = 0;

		if (read_or_write)
			goto error;

		z = openzip(fname);
		if (!z)
			goto error;

		zipent = readzip(z);
		if (!zipent)
			goto error;

		imgfile->u.m.bufsz = zipent->uncompressed_size;
		imgfile->u.m.buf = malloc(zipent->uncompressed_size);
		if (!imgfile->u.m.buf)
			goto error;

		if (readuncompresszip(z, zipent, imgfile->u.m.buf)) {
			free(imgfile->u.m.buf);
			goto error;
		}

		closezip(z);
	}
	else {
		/* Normal file */
		imgfile->imgtype = IMG_FILE;
		imgfile->write_protect = read_or_write ? 0 : 1;

		imgfile->u.f = fopen(fname, write_modes[read_or_write]);
		if (!imgfile->u.f)
			goto error;
	}
	return (STREAM *) imgfile;

error:
	if (z)
		closezip(z);
	free((void *) imgfile);
	return (STREAM *) NULL;
}

void stream_close(STREAM *f)
{
	switch(f->imgtype) {
	case IMG_FILE:
		fclose(f->u.f);
		break;

	case IMG_MEM:
		free(f->u.m.buf);
		break;
	}
	free((void *) f);
}

size_t stream_read(STREAM *f, void *buf, size_t sz)
{
	size_t result = 0;

	switch(f->imgtype) {
	case IMG_FILE:
		result = fread(buf, 1, sz, f->u.f);
		break;

	case IMG_MEM:
		if ((f->u.m.pos + sz) > f->u.m.bufsz)
			result = f->u.m.bufsz - f->u.m.pos;
		else
			result = sz;
		memcpy(buf, f->u.m.buf + f->u.m.pos, result);
		f->u.m.pos += result;
		break;
	}
	return result;
}

size_t stream_write(STREAM *f, const void *buf, size_t sz)
{
	size_t result = 0;

	switch(f->imgtype) {
	case IMG_FILE:
		result = fwrite(buf, 1, sz, f->u.f);
		break;
	}
	return result;
}

size_t stream_size(STREAM *f)
{
	size_t result = 0;

	switch(f->imgtype) {
	case IMG_FILE:
		result = fsize(f->u.f);
		break;

	case IMG_MEM:
		result = f->u.m.bufsz;
		break;
	}
	return result;
}

int stream_seek(STREAM *f, size_t pos, int where)
{
	int result = 0;

	switch(f->imgtype) {
	case IMG_FILE:
		result = fseek(f->u.f, pos, where);
		break;

	case IMG_MEM:
		switch(where) {
		case SEEK_CUR:
			pos += f->u.m.pos;
			break;
		case SEEK_END:
			pos += f->u.m.bufsz;
			break;
		}
		if ((pos > f->u.m.bufsz) || (pos < 0)) {
			result = -1;
		}
		else {
			f->u.m.pos = pos;
		}
		break;
	}
	return result;
}

size_t stream_transfer(STREAM *dest, STREAM *source, size_t sz)
{
	size_t result = 0;
	size_t readsz;
	char buf[1024];

	while(sz && (readsz = stream_read(source, buf, MIN(sz, sizeof(buf))))) {
		stream_write(dest, buf, readsz);
		sz -= readsz;
		result += readsz;
	}
	return result;
}

size_t stream_transfer_all(STREAM *dest, STREAM *source)
{
	return stream_transfer(dest, source, stream_size(source));
}

int stream_crc(STREAM *f, unsigned long *result)
{
	size_t sz;
	void *ptr;

	switch(f->imgtype) {
	case IMG_MEM:
		*result = crc32(0, (unsigned char*)f->u.m.buf, f->u.m.bufsz);
		break;

	default:
		sz = stream_size(f);
		ptr = malloc(sz);
		if (!ptr)
			return IMGTOOLERR_OUTOFMEMORY;
		stream_seek(f, 0, SEEK_SET);
		if (stream_read(f, ptr, sz) != sz)
			return IMGTOOLERR_READERROR;
		*result = crc32(0, ptr, sz);
		free(ptr);
		break;
	}
	return 0;
}

int file_crc(const char *fname,  unsigned long *result)
{
	int err;
	STREAM *f;

	f = stream_open(fname, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND;

	err = stream_crc(f, result);
	stream_close(f);
	return err;
}

size_t stream_fill(STREAM *f, unsigned char b, size_t sz)
{
	size_t outsz;
	char buf[1024];

	outsz = 0;
	memset(buf, b, MIN(sz, sizeof(buf)));

	while(sz) {
		outsz += stream_write(f, buf, MIN(sz, sizeof(buf)));
		sz -= MIN(sz, sizeof(buf));
	}
	return outsz;
}

