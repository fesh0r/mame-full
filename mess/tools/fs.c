/*
  archiv of files in the given directory
  too allow access to the native filesystem
  with the same interface as for the other images !*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "osd_dir.h"
#include "imgtool.h"

typedef struct {
	IMAGE base;
	const char *path;
	const char *name;
//	char name[FILENAME_MAX];
} fs_image;

typedef struct {
	IMAGEENUM base;
	fs_image *image;
	void *dir;
} fs_iterator;

static int fs_image_init(const char *name, IMAGE **outimg);
static void fs_image_exit(IMAGE *img);
static void fs_image_info(IMAGE *img, char *string, const int len);
static int fs_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int fs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void fs_image_closeenum(IMAGEENUM *enumeration);
//static size_t fs_image_freespace(IMAGE *img); */
static int fs_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
//static int fs_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options);
//static int fs_image_deletefile(IMAGE *img, const char *fname);
//static int fs_image_create(STREAM *f, const geometry_options *options);

IMAGEMODULE(
	fs,
	"Directory",	/* human readable name */
	".",								/* file extension */
	0,	/* flags */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* geometry ranges */
	fs_image_init,				/* init function */
	NULL,				/* init function */
	fs_image_exit,				/* exit function */
	fs_image_info,		/* info function */
	fs_image_beginenum,			/* begin enumeration */
	fs_image_nextenum,			/* enumerate next */
	fs_image_closeenum,			/* close enumeration */
	NULL,//fs_image_freespace,			   free space on image    */
	fs_image_readfile,			/* read file */
	NULL,//fs_image_writefile,			/* write file */
	NULL,//fs_image_deletefile,			/* delete file */
	NULL,//fs_image_create,				/* create image */
	NULL,
	NULL,
	NULL
)

static int fs_image_init(const char *name, IMAGE **outimg)
{
	fs_image *image;
	void *dir;

//	path=osd_get_cwd();
	dir=osd_dir_open(name, "*");
	if (!dir) return IMGTOOLERR_FILENOTFOUND;
	osd_dir_close(dir);

	image=*(fs_image**)outimg=(fs_image *) malloc(sizeof(fs_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(fs_image));
	image->base.module = &imgmod_fs;
	image->name=name;

	return 0;
}

static void fs_image_exit(IMAGE *img)
{
	fs_image *image=(fs_image*)img;
	free(image);
}

static void fs_image_info(IMAGE *img, char *string, const int len)
{
	fs_image *image=(fs_image*)img;
	sprintf(string,"%s", image->name);
}

static int fs_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	fs_image *image=(fs_image*)img;
	fs_iterator *iter;

	iter=*(fs_iterator**)outenum = (fs_iterator *) malloc(sizeof(fs_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_fs;

	iter->image=image;
	iter->dir=osd_dir_open(image->name,"*");
	if (!iter->dir){
		free(iter);
		return IMGTOOLERR_CORRUPTIMAGE;
	}
	return 0;
}

static int fs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	fs_iterator *iter=(fs_iterator*)enumeration;
	int is_dir=1;

	ent->corrupt=0;
	
	for (;;) {
		ent->eof=!osd_dir_get_entry(iter->dir, ent->fname, 
									ent->fname_len,&is_dir);
		if (ent->eof) break;
		if (is_dir) continue;

#if 0
		if (ent->attr)
			sprintf(ent->attr,"start:%.4x end:%.4x type:%d file:%d",
					GET_UWORD( ENTRY(iter->image,iter->index)->start_address),
					GET_UWORD( ENTRY(iter->image,iter->index)->end_address),
					ENTRY(iter->image,iter->index)->type,
					ENTRY(iter->image,iter->index)->file_type );
		ent->filesize=GET_UWORD( ENTRY(iter->image, iter->index)->end_address )
			-GET_UWORD( ENTRY(iter->image, iter->index)->start_address );
#endif
		break;
	}
	return 0;
}

static void fs_image_closeenum(IMAGEENUM *enumeration)
{
	fs_iterator *iter=(fs_iterator*)enumeration;
	osd_dir_close(iter->dir);
	free(enumeration);
}

#if 0
static size_t fs_image_freespace(IMAGE *img)
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

static int fs_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
//	fs_image *image=(fs_image*)img;
	FILE *file;
	size_t size;
	char *buffer;

	//build filename
	if ( (file=fopen(fname, "rb"))==0) return IMGTOOLERR_MODULENOTFOUND;
	if (fseek(file, 0, SEEK_END) ) {
		fclose(file);
		return IMGTOOLERR_READERROR;
	}
	size=ftell(file);
	if (fseek(file, 0, SEEK_SET)) {
		fclose(file);
		return IMGTOOLERR_READERROR;
	}
	
	if ( (buffer=malloc(size))==NULL) {
		fclose(file);
		return IMGTOOLERR_OUTOFMEMORY;
	}

	if (fread(buffer, 1, size, file)!=size) {
		fclose(file);
		return IMGTOOLERR_READERROR;
	}
	fclose(file);
	
	if (stream_write(destf, buffer, size)!=size) {
		return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

#if 0
static int fs_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options)
{
	fs_image *image=(fs_image*)img;
	int size, fsize;
	int ind;
	int pos, i, t;

	fsize=stream_size(sourcef);
	if ((ind=t64_image_findfile(image, fname))==-1 ) {
		/* appending */
		for (ind=0; ind<GET_UWORD(HEADER(image)->max_entries)&&(ENTRY(image,ind)->type!=0); ind++) ;

	return 0;
}

static int fs_image_deletefile(IMAGE *img, const char *fname)
{
	fs_image *image=(fs_image*)img;
	int pos, size;
	int ind, i;

	if ((ind=t64_image_findfile(image, fname))==-1 ) {
		return IMGTOOLERR_MODULENOTFOUND;
	}
	pos=GET_ULONG(ENTRY(image,ind)->offset);
	/* find the size of the data in this area */
	return 0;
}

static int fs_image_create(STREAM *f, const geometry_options *options)
{
	if (stream_write(f, &header, sizeof(t64_header)) != sizeof(t64_header)) 
		return  IMGTOOLERR_WRITEERROR;
	for (i=0; i<entries; i++) {
	if (stream_write(f, &entry, sizeof(t64_entry)) != sizeof(t64_entry)) 
		return  IMGTOOLERR_WRITEERROR;
	}

	return 0;
}
#endif
