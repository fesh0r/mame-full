#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>
#include "unzip.h"
#include "osdepend.h"
#include "imgtool.h"

enum {
	IMG_FILE,
	IMG_MEM,
	IMG_FILTER
};

struct stream_internal {
	int imgtype;
	int write_protect;
	const char *name; // needed for clear
	union {
		FILE *f;
		struct {
			char *buf;
			size_t bufsz;
			size_t pos;
		} m;
		struct {
			STREAM *s;
			FILTER *f;
			size_t totalread;
		} filt;
	} u;
};

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

static STREAM *stream_open_zip(const char *zipname, const char *subname, int read_or_write)
{
	struct stream_internal *imgfile = NULL;
	ZIP *z = NULL;
	struct zipent *zipent;

	if (read_or_write)
		goto error;

	imgfile = malloc(sizeof(struct stream_internal));
	if (!imgfile)
		goto error;

	memset(imgfile, 0, sizeof(*imgfile));
	imgfile->imgtype = IMG_MEM;
	imgfile->write_protect = 1;
	imgfile->u.m.pos = 0;

	z = openzip(0, 0, zipname);
	if (!z)
		goto error;

	do
	{
		zipent = readzip(z);
		if (!zipent)
			goto error;
	}
	while(subname && strcmp(subname, zipent->name));

	imgfile->u.m.bufsz = zipent->uncompressed_size;
	imgfile->u.m.buf = malloc(zipent->uncompressed_size);
	if (!imgfile->u.m.buf)
		goto error;

	if (readuncompresszip(z, zipent, imgfile->u.m.buf))
		goto error;

	closezip(z);
	return (STREAM *) imgfile;

error:
	if (z)
		closezip(z);
	if (imgfile)
	{
		if (imgfile->u.m.buf)
			free(imgfile->u.m.buf);
		free(imgfile);
	}
	return NULL;
}

STREAM *stream_open(const char *fname, int read_or_write)
{
	const char *ext;
	struct stream_internal *imgfile = NULL;
	static const char *write_modes[] = {"rb","wb","r+b","r+b","w+b"};
	FILE *f = NULL;
	char *buf = NULL;
	int len, i;
	STREAM *s = NULL;
	char c;

	/* maybe we are just a ZIP? */
	ext = strrchr(fname, '.');
	if (ext && !stricmp(ext, ".zip"))
		return stream_open_zip(fname, NULL, read_or_write);

	f = fopen(fname, write_modes[read_or_write]);
	if (!f)
	{
		if (!read_or_write)
		{
			len = strlen(fname);

			/* can't open the file; try opening ZIP files with other names */
			buf = malloc(len + 1);
			if (!buf)
				goto error;
			strcpy(buf, fname);

			for(i = len-1; !s && (i >= 0); i--)
			{
				if ((buf[i] == '\\') || (buf[i] == '/'))
				{
					c = buf[i];
					buf[i] = '\0';
					s = stream_open_zip(buf, buf + i + 1, read_or_write);
					buf[i] = c;
				}
			}
			free(buf);
			if (s)
				return s;
		}

		/* ah well, it was worth a shot */
		goto error;
	}

	imgfile = malloc(sizeof(struct stream_internal));
	if (!imgfile)
		goto error;

	/* Normal file */
	memset(imgfile, 0, sizeof(*imgfile));
	imgfile->imgtype = IMG_FILE;
	imgfile->write_protect = read_or_write ? 0 : 1;
	imgfile->u.f = f;
	imgfile->name = fname;
	return (STREAM *) imgfile;

error:
	if (imgfile)
		free((void *) imgfile);
	if (f)
		fclose(f);
	if (buf)
		free(buf);
	return (STREAM *) NULL;
}

STREAM *stream_open_write_stream(int size)
{
	struct stream_internal *imgfile;

	imgfile = malloc(sizeof(struct stream_internal));
	if (!imgfile)
		return NULL;

	imgfile->imgtype = IMG_MEM;
	imgfile->write_protect = 0;
	imgfile->u.m.pos = 0;

	imgfile->u.m.bufsz = size;
	imgfile->u.m.buf = malloc(size);

	if (!imgfile->u.m.buf) {
		free(imgfile);
		return NULL;
	}
	return (STREAM *) imgfile;
}

STREAM *stream_open_mem(void *buf, size_t sz)
{
	struct stream_internal *imgfile;

	imgfile = malloc(sizeof(struct stream_internal));
	if (!imgfile)
		return NULL;

	imgfile->imgtype = IMG_MEM;
	imgfile->write_protect = 0;
	imgfile->u.m.pos = 0;

	imgfile->u.m.bufsz = sz;
	imgfile->u.m.buf = buf;
	return (STREAM *) imgfile;
}

STREAM *stream_open_filter(STREAM *s, FILTER *f)
{
	struct stream_internal *imgfile;

	imgfile = malloc(sizeof(struct stream_internal));
	if (!imgfile)
		return NULL;

	imgfile->imgtype = IMG_FILTER;
	imgfile->u.filt.s = s;
	imgfile->u.filt.f = f;
	imgfile->u.filt.totalread = 0;
	return (STREAM *) imgfile;
}

void stream_close(STREAM *s)
{
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		fclose(si->u.f);
		break;

	case IMG_MEM:
		free(si->u.m.buf);
		break;

	case IMG_FILTER:
		filter_term(si->u.filt.f);
		break;

	default:
		assert(0);
		break;
	}
	free((void *) si);
}

size_t stream_read(STREAM *s, void *buf, size_t sz)
{
	size_t result = 0;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		result = fread(buf, 1, sz, si->u.f);
		break;

	case IMG_MEM:
		if ((si->u.m.pos + sz) > si->u.m.bufsz)
			result = si->u.m.bufsz - si->u.m.pos;
		else
			result = sz;
		memcpy(buf, si->u.m.buf + si->u.m.pos, result);
		si->u.m.pos += result;
		break;

	case IMG_FILTER:
		result = filter_readfromstream(si->u.filt.f, si->u.filt.s, buf, sz);
		si->u.filt.totalread += result;
		break;

	default:
		assert(0);
		break;
	}
	return result;
}

size_t stream_write(STREAM *s, const void *buf, size_t sz)
{
	size_t result = 0;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_MEM:
		if (!si->write_protect) {
			if (si->u.m.bufsz<si->u.m.pos+sz) {
				si->u.m.buf=realloc(si->u.m.buf,si->u.m.pos+sz);
				si->u.m.bufsz=si->u.m.pos+sz;
			}
			memcpy(si->u.m.buf+si->u.m.pos, buf, sz);
			si->u.m.pos+=sz;
			result=sz;
		}
		break;

	case IMG_FILE:
		result = fwrite(buf, 1, sz, si->u.f);
		break;

	case IMG_FILTER:
		result = filter_writetostream(si->u.filt.f, si->u.filt.s, buf, sz);
		break;

	default:
		assert(0);
		break;
	}
	return result;
}

size_t stream_size(STREAM *s)
{
	size_t result = 0;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		result = fsize(si->u.f);
		break;

	case IMG_MEM:
		result = si->u.m.bufsz;
		break;

	case IMG_FILTER:
		si->u.filt.totalread += filter_readintobuffer(si->u.filt.f, si->u.filt.s);
		result = si->u.filt.totalread;
		break;

	default:
		assert(0);
		break;
	}
	return result;
}

int stream_seek(STREAM *s, size_t pos, int where)
{
	int result = 0;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		result = fseek(si->u.f, pos, where);
		break;

	case IMG_MEM:
		switch(where) {
		case SEEK_CUR:
			pos += si->u.m.pos;
			break;
		case SEEK_END:
			pos += si->u.m.bufsz;
			break;
		}
		if ((pos > si->u.m.bufsz) || (pos < 0)) {
			result = -1;
		}
		else {
			si->u.m.pos = pos;
		}
		break;

	default:
		assert(0);
		break;
	}
	return result;
}

size_t stream_tell(STREAM *s)
{
	int result = 0;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		result = ftell(si->u.f);
		break;

	case IMG_MEM:
		result = si->u.m.pos;
		break;

	default:
		assert(0);
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

int stream_crc(STREAM *s, unsigned long *result)
{
	size_t sz;
	void *ptr;
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_MEM:
		*result = crc32(0, (unsigned char*)si->u.m.buf, si->u.m.bufsz);
		break;

	default:
		sz = stream_size(s);
		ptr = malloc(sz);
		if (!ptr)
			return IMGTOOLERR_OUTOFMEMORY;
		stream_seek(s, 0, SEEK_SET);
		if (stream_read(s, ptr, sz) != sz)
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

void stream_clear(STREAM *s)
{
	struct stream_internal *si = (struct stream_internal *) s;

	switch(si->imgtype) {
	case IMG_FILE:
		if (!si->write_protect) {
			fclose(si->u.f);
			si->u.f=fopen(si->name,"wb+");
			if (si->u.f==NULL) ;
			fclose(si->u.f);
			si->u.f=fopen(si->name,"wb");
		}
		break;

	default:
		/* Need to implement */
		assert(0);
		break;
	}
}

int stream_isreadonly(STREAM *s)
{
	struct stream_internal *si = (struct stream_internal *) s;
	return si->write_protect;
}

