#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zlib.h"
#include "osdepend.h"
#include "imgtool.h"

typedef struct {
	IMAGE base;
	STREAM *file_handle;
	int size;
	int modified;
	unsigned char *data;
} rom16_image;

typedef struct {
	IMAGEENUM base;
	rom16_image *image;
	int index;
} rom16_iterator;

static int rom16_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void rom16_image_exit(IMAGE *img);
//static void rom16_image_info(IMAGE *img, char *string, const int len);
static int rom16_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int rom16_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void rom16_image_closeenum(IMAGEENUM *enumeration);
static int rom16_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int rom16_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options);
//static int rom16_image_create(STREAM *f, const geometry_options *options);

IMAGEMODULE(
	rom16,
	"16 bit wide Romimage",	/* human readable name */
	"rom",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	rom16_image_init,				/* init function */
	rom16_image_exit,				/* exit function */
	NULL, /* info function */
	rom16_image_beginenum,			/* begin enumeration */
	rom16_image_nextenum,			/* enumerate next */
	rom16_image_closeenum,			/* close enumeration */
	NULL, //crt_image_freespace,			/* free space on image */
	rom16_image_readfile,			/* read file */
	rom16_image_writefile,			/* write file */
	NULL,			/* delete file */
	NULL,//	rom16_image_create,				/* create image */
	NULL,
	NULL,
	NULL,							/* file options */
	NULL							/* create options */
)

static int rom16_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	rom16_image *image;

	image=*(rom16_image**)outimg=(rom16_image *) malloc(sizeof(rom16_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(rom16_image));
	image->base.module = &imgmod_rom16;
	image->size=stream_size(f);
	image->file_handle=f;

	image->data = (unsigned char *) malloc(image->size);
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	return 0;
}

static void rom16_image_exit(IMAGE *img)
{
	rom16_image *image=(rom16_image*)img;
	if (image->modified) {
		stream_clear(image->file_handle);
		stream_write(image->file_handle, image->data, image->size);
	}
	stream_close(image->file_handle);
	free(image->data);
	free(image);
}

#if 0
static void rom16_image_info(IMAGE *img, char *string, const int len)
{
	_image *image=(rom16_image*)img;
	sprintf(string, "%-32s\nversion:%.4x type:%d:%s exrom:%d game:%d",
			HEADER(image)->name,
			GET_UWORD(HEADER(image)->version),
			GET_UWORD(HEADER(image)->hardware_type),
			hardware_types[GET_UWORD(HEADER(image)->hardware_type)],
			HEADER(image)->exrom_line, HEADER(image)->game_line);
	return;
}
#endif

static int rom16_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	rom16_image *image=(rom16_image*)img;
	rom16_iterator *iter;

	iter=*(rom16_iterator**)outenum = (rom16_iterator *) malloc(sizeof(rom16_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_rom16;

	iter->image=image;
	iter->index = 0;
	return 0;
}

static int rom16_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
    int pos;
	rom16_iterator *iter=(rom16_iterator*)enumeration;

	ent->corrupt=0;
	ent->eof=0;
	
	switch (iter->index) {
	case 0:
	    pos=0;
	    strcpy(ent->fname,"even");
	    ent->filesize= iter->image->size/2;
	    iter->index++;
	    break;
	case 1:
	    pos=1;
	    iter->index++;
	    ent->filesize= iter->image->size/2;
	    strcpy(ent->fname,"odd");
	    break;
	default:
	    ent->eof=1;
	}
	if (ent->eof==0 && ent->attr) {
	    unsigned crc=0;
	    for (;pos<iter->image->size; pos+=2) {
		crc=crc32(crc, iter->image->data+pos, 1);
	    }
	    sprintf(ent->attr, "0x%x", crc);
	}
	return 0;
}

static void rom16_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

static int rom16_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	rom16_image *image=(rom16_image*)img;
	int pos;
	int i;

	if (stricmp(fname, "even")==0) {
		pos=0;
	} else if (stricmp(fname,"odd")==0) {
		pos=1;
	} else {
		return IMGTOOLERR_MODULENOTFOUND;
	}

	for (i=0; i<image->size; i+=2) {
		if (stream_write(destf, image->data+pos+i, 1)!=1)
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

static int rom16_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, 
							   const ResolvedOption *options)
{
	rom16_image *image=(rom16_image*)img;
	int size;
	int pos;
	int i;

	size=stream_size(sourcef);
	if (size*2!=image->size) return IMGTOOLERR_READERROR;
	if (stricmp(fname, "even")==0) {
		pos=0;
	} else if (stricmp(fname,"odd")==0) {
		pos=1;
	} else {
		return IMGTOOLERR_MODULENOTFOUND;
	}

	for (i=0; i<size; i++) {
		if (stream_read(sourcef, image->data+pos+i*2, 1)!=1)
			return IMGTOOLERR_READERROR;
	}
	image->modified=1;

	return 0;
}

#if 0
static int rom16_image_create(STREAM *f, const ResolvedOption *options)
{
//	if (options->label) strcpy(header.name, options->label);
	return (stream_write(f, &header, sizeof(crt_header)) == sizeof(crt_header)) 
		? 0 : IMGTOOLERR_WRITEERROR;
}
#endif
