#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "imgtool.h"

#define GRANULE_COUNT	68

typedef struct {
	char fname[8];
	char fext[3];
	char ftype;
	char asciiflag;
	unsigned char first_granule;
	unsigned char lastsectorbytes_msb;
	unsigned char lastsectorbytes_lsb;
} rsdos_dirent;

typedef struct {
	IMAGE base;
	STREAM *f;
	int granulemap_dirty;
	unsigned char granulemap[GRANULE_COUNT];
} rsdos_diskimage;

typedef struct {
	IMAGEENUM base;
	rsdos_diskimage *img;
	int index;
	int eof;
} rsdos_direnum;

#define OFFSET(track, sector)	((((track) * 18) + ((sector) - 1)) * 256)
#define GOFFSET(granule)		(((granule) + (((granule) >= 34) ? 2 : 0)) * (9*256))

static int rsdos_diskimage_init(STREAM *f, IMAGE **outimg);
static void rsdos_diskimage_exit(IMAGE *img);
static int rsdos_diskimage_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int rsdos_diskimage_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void rsdos_diskimage_closeenum(IMAGEENUM *enumeration);
static size_t rsdos_diskimage_freespace(IMAGE *img);
static int rsdos_diskimage_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int rsdos_diskimage_writefile(IMAGE *img, const char *fname, STREAM *sourcef);
static int rsdos_diskimage_deletefile(IMAGE *img, const char *fname);
static int rsdos_diskimage_create(STREAM *f);

IMAGEMODULE(rsdos,
	"Tandy CoCo RS-DOS disk images",
	rsdos_diskimage_init,
	rsdos_diskimage_exit,
	rsdos_diskimage_beginenum,
	rsdos_diskimage_nextenum,
	rsdos_diskimage_closeenum,
	rsdos_diskimage_freespace,
	rsdos_diskimage_readfile,
	rsdos_diskimage_writefile,
	rsdos_diskimage_deletefile,
	rsdos_diskimage_create
)

static void rtrim(char *buf)
{
	size_t buflen;
	char *s;

	buflen = strlen(buf);
	if (buflen) {
		for (s = &buf[buflen-1]; (*s == ' '); s--)
			*s = '\0';
	}
}

static int get_rsdos_dirent(STREAM *f, int index_loc, rsdos_dirent *ent)
{
	if (stream_seek(f, OFFSET(17, 3) + (index_loc * 32), SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_read(f, (void *) ent, sizeof(*ent)) != sizeof(*ent))
		return IMGTOOLERR_READERROR;

	return 0;
}

/* fnamebuf must have at least 13 bytes */
static void get_dirent_fname(char *fnamebuf, const rsdos_dirent *ent)
{
	char *s;

	memset(fnamebuf, 0, 13);
	memcpy(fnamebuf, ent->fname, sizeof(ent->fname));
	rtrim(fnamebuf);
	s = fnamebuf + strlen(fnamebuf);
	*(s++) = '.';
	memcpy(s, ent->fext, sizeof(ent->fext));
	rtrim(s);

	/* If no extension, remove period */
	if (*s == '\0')
		s[-1] = '\0';
}

static int lookup_rsdos_file(STREAM *f, const char *fname, rsdos_dirent *ent, int *position)
{
	int i, err;
	char fnamebuf[13];

	i = 0;
	fnamebuf[0] = '\0';

	do {
		do {
			err = get_rsdos_dirent(f, i++, ent);
			if (err)
				return err;
		}
		while(ent->fname[0] == '\0');


		if (ent->fname[0] != -1)
			get_dirent_fname(fnamebuf, ent);
	}
	while((ent->fname[0] != -1) && stricmp(fnamebuf, fname));

	if (ent->fname[0] == -1)
		return IMGTOOLERR_FILENOTFOUND;

	if (position)
		*position = i - 1;
	return 0;
}

static size_t process_rsdos_file(rsdos_dirent *ent, rsdos_diskimage *img, STREAM *destf)
{
	size_t s, lastgransize;
	unsigned char i, granule;

	lastgransize = ent->lastsectorbytes_lsb + (((int) ent->lastsectorbytes_msb) << 8);
	s = 0;
	granule = ent->first_granule;

	while((i = img->granulemap[granule]) < GRANULE_COUNT) {
		if (destf) {
			stream_seek(img->f, GOFFSET(granule), SEEK_SET);
			stream_transfer(destf, img->f, 9*256);
		}

		/* i is the next granule */
		s += (256 * 9);
		granule = i;
	}

	if ((i < 0xc0) || (i > 0xc9))
		return (size_t) -1;

	lastgransize += (256 * (i - 0xc0));

	if (destf) {
		stream_seek(img->f, GOFFSET(granule), SEEK_SET);
		stream_transfer(destf, img->f, lastgransize);
	}

	return s + lastgransize;
}

static int prepare_dirent(rsdos_dirent *ent, const char *fname)
{
	const char *fname_end;
	const char *fname_ext;
	int fname_ext_len;

	memset(ent->fname, ' ', sizeof(ent->fname));
	memset(ent->fext, ' ', sizeof(ent->fext));

	fname_end = strchr(fname, '.');
	if (fname_end)
		fname_ext = fname_end + 1;
	else
		fname_end = fname_ext = fname + strlen(fname);

	fname_ext_len = strlen(fname_ext);

	/* We had better be an 8.3 filename */
	if (((fname_end - fname) > 8) || (fname_ext_len > 3))
		return IMGTOOLERR_BADFILENAME;

	memcpy(ent->fname, fname, fname_end - fname);
	memcpy(ent->fext, fname_ext, fname_ext_len);

	/* For now, all files are type 2 binary files */
	ent->ftype = 2;
	ent->asciiflag = 0;
	return 0;
}

static unsigned char find_unused_granule(rsdos_diskimage *rsimg, int granule_to_ignore)
{
	int i;
	for (i = 0; i < GRANULE_COUNT; i++)
		if ((rsimg->granulemap[i] == 0xff) && (i != granule_to_ignore))
			return i;
	return 0xff;
}

static int rsdos_diskimage_init(STREAM *f, IMAGE **outimg)
{
	int err;
	rsdos_diskimage *img = NULL;

	if (stream_size(f) != (35*18*256)) {
		err = IMGTOOLERR_CORRUPTIMAGE;
		goto error;
	}

	img = (rsdos_diskimage *) malloc(sizeof(rsdos_diskimage));
	if (!img) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	if (stream_seek(f, OFFSET(17, 2), SEEK_SET)) {
		err = IMGTOOLERR_READERROR;
		goto error;
	}

	if (stream_read(f, img->granulemap, sizeof(img->granulemap)) != sizeof(img->granulemap)) {
		err = IMGTOOLERR_READERROR;
		goto error;
	}

	img->base.module = &imgmod_rsdos;
	img->f = f;
	img->granulemap_dirty = 0;
	*outimg = &img->base;
	return 0;

error:
	if (img)
		free(img);
	*outimg = NULL;
	return err;
}

static void rsdos_diskimage_exit(IMAGE *img)
{
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;

	if (rsimg->granulemap_dirty) {
		stream_seek(rsimg->f, OFFSET(17,2), SEEK_SET);
		stream_write(rsimg->f, rsimg->granulemap, sizeof(rsimg->granulemap));
	}

	stream_close(rsimg->f);
	free(img);
}

static int rsdos_diskimage_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	rsdos_direnum *rsenum;

	rsenum = (rsdos_direnum *) malloc(sizeof(rsdos_direnum));
	if (!rsenum)
		return IMGTOOLERR_OUTOFMEMORY;

	rsenum->base.module = &imgmod_rsdos;
	rsenum->img = (rsdos_diskimage *) img;
	rsenum->index = 0;
	rsenum->eof = 0;
	*outenum = &rsenum->base;
	return 0;
}

static int rsdos_diskimage_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	int err;
	size_t filesize;
	rsdos_direnum *rsenum = (rsdos_direnum *) enumeration;
	rsdos_dirent rsent;
	char fname[13];

	/* Did we hit the end of file before? */
	if (rsenum->eof)
		goto eof;

	do {
		err = get_rsdos_dirent(rsenum->img->f, rsenum->index++, &rsent);
		if (err)
			return err;
	}
	while(rsent.fname[0] == '\0');

	/* Now are we at the eof point? */
	if (rsent.fname[0] == -1) {
		rsenum->eof = 1;
eof:
		ent->filesize = 0;
		ent->corrupt = 0;
		ent->eof = 1;
		if (ent->fname_len > 0)
			ent->fname[0] = '\0';
	}
	else {
		/* Not the end of file */
		filesize = process_rsdos_file(&rsent, rsenum->img, NULL);
		if (filesize == ((size_t) -1)) {
			/* corrupt! */
			ent->filesize = 0;
			ent->corrupt = 1;
		}
		else {
			ent->filesize = filesize;
			ent->corrupt = 0;
		}
		ent->eof = 0;

		get_dirent_fname(fname, &rsent);

		if (strlen(fname) >= ent->fname_len)
			return IMGTOOLERR_BUFFERTOOSMALL;
		strcpy(ent->fname, fname);
	}
	return 0;
}

static void rsdos_diskimage_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

static size_t rsdos_diskimage_freespace(IMAGE *img)
{
	int i;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	size_t s = 0;

	for (i = 0; i < GRANULE_COUNT; i++)
		if (rsimg->granulemap[i] == 0xff)
			s += (9 * 256);
	return s;
}

static int rsdos_diskimage_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	int err;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	rsdos_dirent ent;

	err = lookup_rsdos_file(rsimg->f, fname, &ent, NULL);
	if (err)
		return err;

	if (process_rsdos_file(&ent, rsimg, destf) == (size_t) -1)
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

static int rsdos_diskimage_writefile(IMAGE *img, const char *fname, STREAM *sourcef)
{
	int err;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	rsdos_dirent ent, ent2;
	size_t sz, i;
	unsigned char g;
	unsigned char *gptr;

	/* Can we write to this image? */
	if (rsimg->f->write_protect)
		return IMGTOOLERR_READONLY;

	/* Is there enough space? */
	sz = stream_size(sourcef);
	if (sz > rsdos_diskimage_freespace(img))
		return IMGTOOLERR_NOSPACE;

	/* Setup our directory entry */
	err = prepare_dirent(&ent, fname);
	if (err)
		return err;

	ent.lastsectorbytes_lsb = sz % 256;
	ent.lastsectorbytes_msb = (((sz % 256) == 0) && (sz > 0)) ? 1 : 0;
	gptr = &ent.first_granule;

	rsimg->granulemap_dirty = 1;
	g = 0xff;

	stream_seek(sourcef, 0, SEEK_SET);

	do {
		g = find_unused_granule(rsimg, g);
		if (g == 0xff)
			return IMGTOOLERR_UNEXPECTED;	/* We should have already verified that there is enough space */
		*gptr = g;
		gptr = &rsimg->granulemap[g];

		stream_seek(rsimg->f, GOFFSET(g), SEEK_SET);

		i = MIN(sz, (9*256));
		if (stream_transfer(rsimg->f, sourcef, i) < i)
			return IMGTOOLERR_READERROR;

		sz -= i;
	}
	while(sz > 0);

	*gptr = 0xc0 + ((i - 1) / 256);

	i = 0;
	do {
		err = get_rsdos_dirent(rsimg->f, i++, &ent2);
		if (err)
			return err;
	}
	while((ent2.fname[0] != '\0') && (ent2.fname[0] != -1));

	stream_seek(rsimg->f, 0 - sizeof(ent2), SEEK_CUR);
	if (stream_write(rsimg->f, &ent, sizeof(ent)) != sizeof(ent))
		return IMGTOOLERR_WRITEERROR;

	return 0;
}

static int rsdos_diskimage_deletefile(IMAGE *img, const char *fname)
{
	int err;
	int pos;
	unsigned char g, i;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	rsdos_dirent ent;

	err = lookup_rsdos_file(rsimg->f, fname, &ent, &pos);
	if (err)
		return err;

	/* Seek to the directory entry */
	stream_seek(rsimg->f, OFFSET(17,3) + (pos * 32), SEEK_SET);

	/* Write a NUL in the filename, marking it deleted */
	if (stream_fill(rsimg->f, 0, 1) != 1)
		return IMGTOOLERR_WRITEERROR;

	/* Now free up the granules */
	g = ent.first_granule;
	while (g < GRANULE_COUNT) {
		i = rsimg->granulemap[g];
		rsimg->granulemap[g] = 0xff;
		g = i;
	}
	rsimg->granulemap_dirty = 1;

	return 0;
}

static int rsdos_diskimage_create(STREAM *f)
{
	return (stream_fill(f, 0xff, 35*18*256) == 35*18*256) ? 0 : IMGTOOLERR_WRITEERROR;
}

