#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "formats/xsa.h"
#include "imgtool.h"
#include "osdtools.h"

/*
 * xsa.c : extracts .xsa file
 */


typedef struct {
	IMAGE			base;
	char			*file_name;
	STREAM 			*file_handle;
	int 			size;
	unsigned char	*data;
    int				file_size;
	} XSA_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	XSA_IMAGE	*image;
	int			index;
	} IMG_ITERATOR;

static int xsa_image_init(STREAM *f, IMAGE **outimg);
static void xsa_image_exit(IMAGE *img);
static int xsa_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int xsa_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void xsa_image_closeenum(IMAGEENUM *enumeration);
static int xsa_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	xsa,
	"XelaSoft Archive",		/* human readable name */
	"xsa",								/* file extension */
	0,	/* flags */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* geometry ranges */
	NULL,
	xsa_image_init,					/* init function */
	xsa_image_exit,					/* exit function */
	NULL,								/* info function */
	xsa_image_beginenum,			/* begin enumeration */
	xsa_image_nextenum,				/* enumerate next */
	xsa_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	xsa_image_readfile,				/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,
	NULL,
	NULL
)

static int xsa_image_init(STREAM *f, IMAGE **outimg)
	{
	XSA_IMAGE *image;
    int ret;

	image = (XSA_IMAGE*)malloc (sizeof (XSA_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(XSA_IMAGE));
	image->base.module = &imgmod_xsa;
	image->size=stream_size(f);
	image->file_handle=f;

	image->data = (unsigned char *) malloc(image->size);
	if (!image->data)
		{
		free (image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
		}

	if (stream_read(f, image->data, image->size)!=image->size) 
		{
		free (image->data);
		free (image);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
		}

    ret = msx_xsa_id (image->data, image->size, &image->file_name, &image->file_size);
  
    if (ret)
		{
		free (image->data);
		free (image);
		*outimg=NULL;
		return (ret == 1 ? IMGTOOLERR_CORRUPTIMAGE : IMGTOOLERR_OUTOFMEMORY);
		}

	return 0;
	}

static void xsa_image_exit(IMAGE *img)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image->data);
	free(image);
	}

static int xsa_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;
	IMG_ITERATOR *iter;

	iter=*(IMG_ITERATOR**)outenum = (IMG_ITERATOR*) malloc(sizeof(IMG_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_xsa;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int xsa_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
	{
	IMG_ITERATOR *iter=(IMG_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, iter->image->file_name);
		ent->corrupt=0;
		ent->filesize = iter->image->file_size;
		iter->index++;
		}

	return 0;
	}

static void xsa_image_closeenum(IMAGEENUM *enumeration)
	{
	free(enumeration);
	}

static int xsa_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
	{
	XSA_IMAGE *image=(XSA_IMAGE*)img;
    UINT8 *data;
    int dsklen, ret;

	if (stricmp (fname, image->file_name) )
		return IMGTOOLERR_MODULENOTFOUND;

    ret = msx_xsa_extract (image->data, image->size, &data, &dsklen);

    if (ret) return (ret == 1 ? IMGTOOLERR_CORRUPTIMAGE : IMGTOOLERR_OUTOFMEMORY);

    if (dsklen != image->file_size)
		{
		free (data);
		return IMGTOOLERR_CORRUPTIMAGE;
		}

	ret = (dsklen != stream_write(destf, data, dsklen) );

	free (data);

	return (ret ? IMGTOOLERR_WRITEERROR : 0);
	}

