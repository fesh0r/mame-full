#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "unzip.h"

#include "imgtool.h"

typedef struct {
	IMAGE base;
	STREAM *file_handle;
	ZIP *zip;
} zip_image;

typedef struct {
	IMAGEENUM base;
	zip_image *image;
} zip_iterator;

static int zip_image_init(const char *name, IMAGE **outimg);
static void zip_image_exit(IMAGE *img);
//static void zip_image_info(IMAGE *img, char *string, const int len);
static int zip_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int zip_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void zip_image_closeenum(IMAGEENUM *enumeration);
/*static size_t zip_image_freespace(IMAGE *img); */
static int zip_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
//static int zip_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options);
//static int zip_image_deletefile(IMAGE *img, const char *fname);
//static int zip_image_create(STREAM *f, const geometry_options *options);

IMAGEMODULE(
	zip,
	"Zip Archive",	/* human readable name */
	"zip",								/* file extension */
	0,	/* flags */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* geometry ranges */
	NULL,								/* eoln */
	zip_image_init,				/* init function */
	NULL,				/* init function */
	zip_image_exit,				/* exit function */
	NULL,//zip_image_info,		/* info function */
	zip_image_beginenum,			/* begin enumeration */
	zip_image_nextenum,			/* enumerate next */
	zip_image_closeenum,			/* close enumeration */
	NULL, /*zip_image_freespace,			   free space on image    */
	zip_image_readfile,			/* read file */
	NULL,//zip_image_writefile,			/* write file */
	NULL,//zip_image_deletefile,			/* delete file */
	NULL,//zip_image_create,				/* create image */
	NULL,
	NULL,
	NULL
)

static int zip_image_init(const char *name, IMAGE **outimg)
{
	zip_image *image;

	image=*(zip_image**)outimg=(zip_image *) malloc(sizeof(zip_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(zip_image));
	image->base.module = &imgmod_zip;

	image->zip=openzip(name);

	if ( (!image->zip) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_FILENOTFOUND;
	}

	return 0;
}

static void zip_image_exit(IMAGE *img)
{
	zip_image *image=(zip_image*)img;
	closezip(image->zip);
	free(image);
}

#if 0
static void zip_image_info(IMAGE *img, char *string, const int len)
{
	zip_image *image=(zip_image*)img;

	return 0;
}
#endif

static int zip_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	zip_image *image=(zip_image*)img;
	zip_iterator *iter;

	iter=*(zip_iterator**)outenum = (zip_iterator *) malloc(sizeof(zip_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_zip;

	iter->image=image;
	rewindzip(image->zip);
	
	return 0;
}

static int zip_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	zip_iterator *iter=(zip_iterator*)enumeration;
	struct zipent *entry;
	
	ent->corrupt=0;
	entry=readzip(iter->image->zip);
	ent->eof=entry==NULL;

	if (!ent->eof) {
		strcpy(ent->fname, entry->name);

#if 0
		if (ent->attr)
			sprintf(ent->attr,"start:%.4x end:%.4x type:%d file:%d",
					GET_UWORD( ENTRY(iter->image,iter->index)->start_address),
					GET_UWORD( ENTRY(iter->image,iter->index)->end_address),
					ENTRY(iter->image,iter->index)->type,
					ENTRY(iter->image,iter->index)->file_type );
#endif
		ent->filesize=entry->uncompressed_size;
	}
	return 0;
}

static void zip_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

#if 0
static size_t zip_image_freespace(IMAGE *img)
{
	int i;
	rsdos_diskimage *rsimg = (rsdos_diskimage *) img;
	size_t s = 0;

	for (i = 0; i < GRANULE_COUNT; i++)
		if (rsimg->granulemap[i] == 0xff)
			s += (9 * 256);
	return s;
}
#endif

static struct zipent *zip_image_findfile(zip_image *image, const char *fname)
{
	struct zipent *entry;

	rewindzip(image->zip);

	entry=readzip(image->zip);
	while (entry) {
		if (strcmp(entry->name, fname)==0 ) return entry;
		entry=readzip(image->zip);
	}
	return NULL;
}

static int zip_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	zip_image *image=(zip_image*)img;
	struct zipent *entry;
	char *buffer;
	int rc;

	if ((entry=zip_image_findfile(image, fname))==NULL ) 
		return IMGTOOLERR_FILENOTFOUND;

	if ((buffer=malloc(entry->uncompressed_size))==NULL )
		return IMGTOOLERR_OUTOFMEMORY;

	rc=readuncompresszip(image->zip, entry, buffer);

	if (rc<0) {
		free(buffer);
		return IMGTOOLERR_READERROR;
	}

	if (stream_write(destf, buffer, entry->uncompressed_size)
		!=entry->uncompressed_size) {
		free(buffer);
		return IMGTOOLERR_WRITEERROR;
	}
	free(buffer);

	return 0;
}

#if 0
static int zip_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options)
{
	zip_image *image=(zip_image*)img;
	int size, fsize;
	int ind;
	int pos, i, t;

	fsize=stream_size(sourcef);
	image->modified=1;

	return 0;
}

static int t64_image_deletefile(IMAGE *img, const char *fname)
{
	t64_image *image=(t64_image*)img;
	int pos, size;
	int ind, i;

	if ((ind=t64_image_findfile(image, fname))==-1 ) {
		return IMGTOOLERR_MODULENOTFOUND;
	}
	pos=GET_ULONG(ENTRY(image,ind)->offset);
	image->modified=1;

	return 0;
}

static int zip_image_create(STREAM *f, const geometry_options *options)
{
	int entries=options->entries;
	t64_header header={ "T64 Tape archiv created by MESS\x1a" };
	t64_entry entry= { 0 };
	int i;

	if (entries==0) entries=10;
	SET_UWORD(header.version, 0x0101);
	SET_UWORD(header.max_entries, options->entries);
	if (options->label) strcpy(header.description, options->label);
	if (stream_write(f, &header, sizeof(t64_header)) != sizeof(t64_header)) 
		return  IMGTOOLERR_WRITEERROR;
	for (i=0; i<entries; i++) {
	if (stream_write(f, &entry, sizeof(t64_entry)) != sizeof(t64_entry)) 
		return  IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

#endif
