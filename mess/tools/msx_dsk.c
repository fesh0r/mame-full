#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "formats/msx_dsk.h"
#include "osdepend.h"
#include "imgtool.h"
#include "osdtools.h"

/*
 * msx_dsk.c : converts .ddi/.img/.msx disk images to .dsk image
 */

typedef struct {
	IMAGE			base;
	char			*file_name;
	STREAM 			*file_handle;
	int 			size;
	unsigned char	*data;
	} DSK_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	DSK_IMAGE	*image;
	int			index;
	} DSK_ITERATOR;

static int msx_dsk_image_init(STREAM *f, IMAGE **outimg);
static void msx_dsk_image_exit(IMAGE *img);
static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void msx_dsk_image_closeenum(IMAGEENUM *enumeration);
static int msx_dsk_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	msx_dsk,
	"Various bogus MSX disk images (img/ddi/msx)",		/* human readable name */
	"ddi\0img\0msx\0",								/* file extension */
	0,	/* flags */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* geometry ranges */
	NULL,
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,
	NULL,
	NULL
)

static int msx_dsk_image_init(STREAM *f, IMAGE **outimg)
	{
	DSK_IMAGE *image;
	int len, ret;
    UINT8 *data;
	char *pbase;
	char default_name[] = "msxdisk";

	image = (DSK_IMAGE*)malloc (sizeof (DSK_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(DSK_IMAGE));
	image->base.module = &imgmod_msx_dsk;
	len=stream_size(f);
	image->file_handle=f;

	data = (UINT8*) malloc(len);
	if (!data)
		{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
		}

    if (stream_read(f, data, len)!=len) 
		{
		free(data);
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
		}

    ret = msx_dsk_convert (data, len, f->name, &image->data, &image->size);
	free (data);
	if (ret)
		{
		free(image);
		*outimg=NULL;
		switch (ret)
			{
			case MSX_OUTOFMEMORY: return IMGTOOLERR_OUTOFMEMORY;
			case MSX_IMAGE_ERROR: return IMGTOOLERR_CORRUPTIMAGE;
			}
		return IMGTOOLERR_UNEXPECTED;
		}

    if (f->name) pbase = basename (f->name);
	else pbase = NULL;
    if (pbase) len = strlen (pbase);
    else len = strlen (default_name);

    image->file_name = malloc (len + 5);
	if (!image->file_name)
		{
		free(image->data);
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
		}

    if (!pbase)
		strcpy (image->file_name, default_name);
    else
		{
		strcpy (image->file_name, pbase);
        if (len > 4 || image->file_name[len -4] == '.') len -= 4;

		strcpy (image->file_name + len, ".dsk");
		}

	return 0;
	}

static void msx_dsk_image_exit(IMAGE *img)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image->data);
	free(image);
	}

static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	DSK_ITERATOR *iter;

	iter=*(DSK_ITERATOR**)outenum = (DSK_ITERATOR*) malloc(sizeof(DSK_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_msx_dsk;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
	{
	DSK_ITERATOR *iter=(DSK_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, iter->image->file_name);
		ent->corrupt=0;
		ent->filesize = iter->image->size;
		iter->index++;
		}

	return 0;
	}

static void msx_dsk_image_closeenum(IMAGEENUM *enumeration)
	{
	free(enumeration);
	}

static int msx_dsk_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;

	if (stricmp (fname, image->file_name) )
		return IMGTOOLERR_MODULENOTFOUND;

	if (image->size != stream_write(destf, image->data, image->size) ) 
		return IMGTOOLERR_WRITEERROR;

	return 0;
	}

