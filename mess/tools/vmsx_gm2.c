#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtool.h"

/*
   vMSX gmaster2.ram

   Virtual MSX 1.x (being replaced by the MSX driver in mess) had
   it's on format for Konami's Game Master 2 SRAM. The file was
   always saved as gmaster2.ram; however the file was 16kB when
   the actual data is 8kB. 8kB is redundant, which is not used
   by MESS, Virtual MSX 2.0 (never finished), and the patches for
   fMSX written by me (Sean Young). So suppose you'd like to use
   the SRAM from Virtual MSX, use this very simple converter.

*/

#define NewName "gmaster2.mem"

typedef struct {
	IMAGE			base;
	STREAM 			*file_handle;
	int 			size;
	unsigned char	*data;
	int 			count;
	} GM2_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	GM2_IMAGE	*image;
	int			index;
	} TAP_ITERATOR;

static int vmsx_gm2_image_init(STREAM *f, IMAGE **outimg);
static void vmsx_gm2_image_exit(IMAGE *img);
//static void vmsx_gm2_image_info(IMAGE *img, char *string, const int len);
static int vmsx_gm2_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int vmsx_gm2_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void vmsx_gm2_image_closeenum(IMAGEENUM *enumeration);
static int vmsx_gm2_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	vmsx_gm2,
	"Virtual MSX Game Master 2 SRAM",		/* human readable name */
	"ram",								/* file extension */
	0,	/* flags */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* geometry ranges */
	NULL,
	vmsx_gm2_image_init,				/* init function */
	vmsx_gm2_image_exit,				/* exit function */
	NULL,								/* info function */
	vmsx_gm2_image_beginenum,			/* begin enumeration */
	vmsx_gm2_image_nextenum,			/* enumerate next */
	vmsx_gm2_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	vmsx_gm2_image_readfile,			/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,
	NULL,
	NULL
)

static int vmsx_gm2_image_init(STREAM *f, IMAGE **outimg)
	{
	GM2_IMAGE *image;

	image = (GM2_IMAGE*)malloc (sizeof (GM2_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(GM2_IMAGE));
	image->base.module = &imgmod_vmsx_gm2;
	image->size=stream_size(f);
	image->file_handle=f;

    if (image->size != 0x4000)
		{
		free (image);
		return IMGTOOLERR_READERROR;
		}

	image->data = (unsigned char *) malloc(image->size);
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) )
		{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
		}

	return 0;
	}

static void vmsx_gm2_image_exit(IMAGE *img)
	{
	GM2_IMAGE *image=(GM2_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->data);
	free(image);
	}

static int vmsx_gm2_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
	{
	GM2_IMAGE *image=(GM2_IMAGE*)img;
	TAP_ITERATOR *iter;

	iter=*(TAP_ITERATOR**)outenum = (TAP_ITERATOR*) malloc(sizeof(TAP_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_vmsx_gm2;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int vmsx_gm2_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
	{
	TAP_ITERATOR *iter=(TAP_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, NewName);
		ent->corrupt=0;
		ent->filesize = 0x2000;
		iter->index++;
		}

	return 0;
	}

static void vmsx_gm2_image_closeenum(IMAGEENUM *enumeration)
	{
	free(enumeration);
	}

static int vmsx_gm2_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
	{
	GM2_IMAGE *image=(GM2_IMAGE*)img;

	if (stricmp (fname, NewName) )
		return IMGTOOLERR_MODULENOTFOUND;

	if (stream_write(destf, image->data + 0x1000, 0x1000)!=0x1000)
			return IMGTOOLERR_WRITEERROR;

	if (stream_write(destf, image->data + 0x3000, 0x1000)!=0x1000)
			return IMGTOOLERR_WRITEERROR;

	return 0;
	}

