#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"
#include "formats/coco_dsk.h"

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
	IMAGEENUM base;
	IMAGE *img;
	int index;
	int eof;
} rsdos_direnum;

static int rsdos_diskimage_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int rsdos_diskimage_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void rsdos_diskimage_closeenum(IMAGEENUM *enumeration);
static size_t rsdos_diskimage_freespace(IMAGE *img);
static int rsdos_diskimage_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int rsdos_diskimage_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *writeoptions);
static int rsdos_diskimage_deletefile(IMAGE *img, const char *fname);

#define RSDOS_OPTION_FTYPE	0
#define RSDOS_OPTION_ASCII	1

FLOPPYMODULE_BEGIN( coco_rsdos )
	FMOD_HUMANNAME("Tandy CoCo RS-DOS disk image")
	FMOD_FORMAT( coco )
	FMOD_EOLN( EOLN_CR )
	FMOD_FLAGS( IMGMODULE_FLAG_FILENAMES_PREFERUCASE )
	FMOD_ENUMERATE( rsdos_diskimage_beginenum, rsdos_diskimage_nextenum, rsdos_diskimage_closeenum)
	FMOD_FREESPACE( rsdos_diskimage_freespace )
	FMOD_READFILE( rsdos_diskimage_readfile )
	FMOD_WRITEFILE( rsdos_diskimage_writefile )
	FMOD_DELETEFILE( rsdos_diskimage_deletefile )

	/* [0] */
	FMOD_FILEOPTION(
		"ftype",
		"File type (0=basic|1=data|2=bin|3=source)",
		IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,
		0, 3, "1")

	/* [1] */
	FMOD_FILEOPTION(
		"ascii",
		"Ascii flag (A=ascii|B=binary)",
		IMGOPTION_FLAG_TYPE_CHAR | IMGOPTION_FLAG_HASDEFAULT,
		'A', 'B', "B")
FLOPPYMODULE_END


#define MAX_DIRENTS		((18-2)*(256/32))
static int get_rsdos_dirent(IMAGE *f, int index_loc, rsdos_dirent *ent)
{
	if (index_loc >= MAX_DIRENTS)
		return IMGTOOLERR_FILENOTFOUND;
	return imgtool_bdf_read_sector(f, 17, 0, 3, index_loc * 32, (void *) ent, sizeof(*ent));
}

static int put_rsdos_dirent(IMAGE *f, int index_loc, const rsdos_dirent *ent)
{
	if (index_loc >= MAX_DIRENTS)
		return IMGTOOLERR_FILENOTFOUND;
	return imgtool_bdf_write_sector(f, 17, 0, 3, index_loc * 32, (const void *) ent, sizeof(*ent));
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

static int lookup_rsdos_file(IMAGE *f, const char *fname, rsdos_dirent *ent, int *position)
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

static UINT8 get_granule_count(IMAGE *img)
{
	UINT16 tracks;
	UINT16 granules;

	tracks = imgtool_bdf_get_geometry(img)->tracks;
	granules = (tracks - 1) * 2;
	return (granules > 255) ? 255 : (UINT8) granules;
}

#define MAX_GRANULEMAP_SIZE	256

/* granule_map must be an array of MAX_GRANULEMAP_SIZE bytes */
static int get_granule_map(IMAGE *img, UINT8 *granule_map, UINT8 *granule_count)
{
	UINT8 count;

	count = get_granule_count(img);
	if (granule_count)
		*granule_count = count;

	return imgtool_bdf_read_sector(img, 17, 0, 2, 0, granule_map, count);
}

static int put_granule_map(IMAGE *img, const UINT8 *granule_map, UINT8 granule_count)
{
	return imgtool_bdf_write_sector(img, 17, 0, 2, 0, granule_map, granule_count);
}


static int transfer_granule(IMAGE *img, UINT8 granule, int length, STREAM *f, int (*proc)(IMAGE *, UINT8, UINT8, UINT8, int, int, STREAM *))
{
	UINT8 track, sector;

	track = granule / 2;
	if (track >= 17)
		track++;

	sector = (granule % 2) ? 10 : 1;

	return proc(img, track, 0, sector, 0, length, f);
}

static int transfer_from_granule(IMAGE *img, UINT8 granule, int length, STREAM *destf)
{
	return transfer_granule(img, granule, length, destf, imgtool_bdf_read_sector_to_stream);
}

static int transfer_to_granule(IMAGE *img, UINT8 granule, int length, STREAM *sourcef)
{
	return transfer_granule(img, granule, length, sourcef, imgtool_bdf_write_sector_from_stream);
}

static size_t process_rsdos_file(rsdos_dirent *ent, IMAGE *img, STREAM *destf)
{
	size_t s, lastgransize;
	UINT8 granule_count;
	unsigned char i = 0, granule;
	UINT8 usedmap[MAX_GRANULEMAP_SIZE];	/* Used to detect infinite loops */
	UINT8 granule_map[MAX_GRANULEMAP_SIZE];

	get_granule_map(img, granule_map, &granule_count);
	memset(usedmap, 0, granule_count);

	lastgransize = ent->lastsectorbytes_lsb + (((int) ent->lastsectorbytes_msb) << 8);
	s = 0;
	granule = ent->first_granule;

	while(!usedmap[granule] && ((i = granule_map[granule]) < granule_count)) {
		usedmap[granule] = 1;
		if (destf) {
			transfer_from_granule(img, granule, 9*256, destf);
		}

		/* i is the next granule */
		s += (256 * 9);
		granule = i;
	}

	if ((i < 0xc0) || (i > 0xc9))
		return (size_t) -1;

	if (lastgransize)
		i--;
	lastgransize += (256 * (i - 0xc0));

	if (destf) {
		transfer_from_granule(img, granule, lastgransize, destf);
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

static int rsdos_diskimage_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	rsdos_direnum *rsenum;

	rsenum = (rsdos_direnum *) malloc(sizeof(rsdos_direnum));
	if (!rsenum)
		return IMGTOOLERR_OUTOFMEMORY;

	rsenum->base.module = img->module;
	rsenum->img = img;
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
		err = get_rsdos_dirent(rsenum->img, rsenum->index++, &rsent);
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

		if (ent->attr_len)
			sprintf(ent->attr, "%d %c", (int) rsent.ftype, (char) (rsent.asciiflag + 'B'));
	}
	return 0;
}

static void rsdos_diskimage_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

static size_t rsdos_diskimage_freespace(IMAGE *img)
{
	UINT8 i;
	size_t s = 0;
	UINT8 granule_count;
	UINT8 granule_map[MAX_GRANULEMAP_SIZE];

	get_granule_map(img, granule_map, &granule_count);

	for (i = 0; i < granule_count; i++)
		if (granule_map[i] == 0xff)
			s += (9 * 256);
	return s;
}

static int rsdos_diskimage_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	int err;
	rsdos_dirent ent;

	err = lookup_rsdos_file(img, fname, &ent, NULL);
	if (err)
		return err;

	if (process_rsdos_file(&ent, img, destf) == (size_t) -1)
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

static int rsdos_diskimage_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *writeoptions)
{
	int err;
	rsdos_dirent ent, ent2;
	size_t sz, i;
	unsigned char g;
	unsigned char *gptr;
	UINT8 granule_count;
	UINT8 granule_map[MAX_GRANULEMAP_SIZE];

	/* Can we write to this image? */
	if (imgtool_bdf_is_readonly(img))
		return IMGTOOLERR_READONLY;

	/* Is there enough space? */
	sz = stream_size(sourcef);
	if (sz > rsdos_diskimage_freespace(img))
		return IMGTOOLERR_NOSPACE;

	/* Setup our directory entry */
	err = prepare_dirent(&ent, fname);
	if (err)
		return err;

	ent.ftype = writeoptions[RSDOS_OPTION_FTYPE].i;
	ent.asciiflag = writeoptions[RSDOS_OPTION_ASCII].i - 'B';
	ent.lastsectorbytes_lsb = sz % 256;
	ent.lastsectorbytes_msb = (((sz % 256) == 0) && (sz > 0)) ? 1 : 0;
	gptr = &ent.first_granule;

	err = get_granule_map(img, granule_map, &granule_count);
	if (err)
		return err;

	g = 0x00;

	do {
		while (granule_map[g] != 0xff) {
			g++;
			if ((g >= granule_count) || (g == 0))
				return IMGTOOLERR_UNEXPECTED;	/* We should have already verified that there is enough space */
		}
		*gptr = g;
		gptr = &granule_map[g];


		i = MIN(sz, (9*256));
		err = transfer_to_granule(img, g, i, sourcef);
		if (err)
			return err;

		sz -= i;

		/* Go to next granule */
		g++;
	}
	while(sz > 0);

	/* Now that we are done with the file, we need to specify the final entry
	 * in the file allocation table
	 */
	*gptr = 0xc0 + ((i + 255) / 256);

	/* Now we need to find an empty directory entry */
	i = -1;
	do {
		err = get_rsdos_dirent(img, ++i, &ent2);
		if (err)
			return err;
	}
	while((ent2.fname[0] != '\0') && (ent2.fname[0] != -1));

	err = put_rsdos_dirent(img, i, &ent);
	if (err)
		return err;

	/* Write the granule map back out */
	err = put_granule_map(img, granule_map, granule_count);
	if (err)
		return err;

	return 0;
}

static int rsdos_diskimage_deletefile(IMAGE *img, const char *fname)
{
	int err;
	int pos;
	unsigned char g, i;
	UINT8 granule_count;
	UINT8 granule_map[MAX_GRANULEMAP_SIZE];
	rsdos_dirent ent;

	err = lookup_rsdos_file(img, fname, &ent, &pos);
	if (err)
		return err;

	/* Write a NUL in the filename, marking it deleted */
	ent.fname[0] = 0;
	err = put_rsdos_dirent(img, pos, &ent);
	if (err)
		return err;

	err = get_granule_map(img, granule_map, &granule_count);
	if (err)
		return err;

	/* Now free up the granules */
	g = ent.first_granule;
	while (g < granule_count) {
		i = granule_map[g];
		granule_map[g] = 0xff;
		g = i;
	}

	err = put_granule_map(img, granule_map, granule_count);
	if (err)
		return err;

	return 0;
}

