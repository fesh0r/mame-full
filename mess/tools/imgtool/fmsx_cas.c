#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "formats/fmsx_cas.h"
#include "imgtool.h"
#include "utils.h"

/*
	MSX module

	fMSX style .cas for the MSX. Converts them to .wav files. Uses the
	mess/formats/fmsx_cas.[ch] files, for the actual conversion.
*/

typedef struct {
	IMAGE			base;
	char			*file_name;
	STREAM 			*file_handle;
	int 			size;
	unsigned char	*data;
	int 			count;
	} CAS_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	CAS_IMAGE	*image;
	int			index;
	} CAS_ITERATOR;

static int fmsx_cas_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void fmsx_cas_image_exit(IMAGE *img);
//static void fmsx_cas_image_info(IMAGE *img, char *string, const int len);
static int fmsx_cas_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int fmsx_cas_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void fmsx_cas_image_closeenum(IMAGEENUM *enumeration);
static int fmsx_cas_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	fmsx_cas,
	"fMSX style .cas file",				/* human readable name */
	"cas",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	fmsx_cas_image_init,				/* init function */
	fmsx_cas_image_exit,				/* exit function */
	NULL,								/* info function */
	fmsx_cas_image_beginenum,			/* begin enumeration */
	fmsx_cas_image_nextenum,			/* enumerate next */
	fmsx_cas_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	fmsx_cas_image_readfile,			/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,					/* file options */
	NULL					/* create options */
)

static int fmsx_cas_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
	{
	CAS_IMAGE *image;
	int len;
	char *pbase;
	char default_name[] = "msxtape";

	image = (CAS_IMAGE*)malloc (sizeof (CAS_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(CAS_IMAGE));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

    if (image->size < 9)
		{
		free (image);
		return IMGTOOLERR_CORRUPTIMAGE;
		}

	image->data = (unsigned char *) malloc(image->size);
	if (!image->data || (stream_read(f, image->data, image->size)!=image->size) )
	{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

  	if ( (image->count = fmsx_cas_to_wav_size (image->data, image->size) ) < 0)
		{
		free(image->data);
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
		}

	pbase = NULL;
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
        if (len > 4)
			if (!strncmpi (".cas", image->file_name + len - 4, 4) ) len -= 4;

		strcpy (image->file_name + len, ".wav");
		}

	return 0;
	}

static void fmsx_cas_image_exit(IMAGE *img)
	{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image->data);
	free(image);
	}

static int fmsx_cas_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	CAS_ITERATOR *iter;

	iter=*(CAS_ITERATOR**)outenum = (CAS_ITERATOR*) malloc(sizeof(CAS_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
}

static int fmsx_cas_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	CAS_ITERATOR *iter=(CAS_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
	{
		strcpy (ent->fname, iter->image->file_name);
		ent->corrupt=0;
		ent->filesize = iter->image->count * 2 + 0x2c;
		iter->index++;
	}

	return 0;
}

static void fmsx_cas_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

static int fmsx_cas_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
	{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	INT16 *wavdata;
	UINT16	temp16;
	UINT32	temp32;
	int wavlen, offset, rc;

	if (stricmp (fname, image->file_name) )
		return IMGTOOLERR_MODULENOTFOUND;

	rc = fmsx_cas_to_wav(image->data, image->size, &wavdata, &wavlen);
	if (rc == 2)
		return IMGTOOLERR_OUTOFMEMORY;
	else if (rc)
		return IMGTOOLERR_CORRUPTIMAGE;

	if (wavlen != image->count)
		{
		free (wavdata);
		return IMGTOOLERR_UNEXPECTED;
		}

    /* write the core header for a WAVE file */
	offset = stream_write(destf, "RIFF", 4);
    if( offset < 4 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
    	}

	temp32 = LITTLE_ENDIANIZE_INT32(image->count * 2 + 0x24);
	offset += stream_write(destf, &temp32, 4);
    if( offset < 8 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
    	}

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += stream_write(destf, "WAVEfmt ", 8);
	if( offset < 16 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

    /* size of the following 'fmt ' fields */
    offset += stream_write(destf, "\x10\x00\x00\x00", 4);
	if( offset < 20 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* format: PCM */
	temp16 = LITTLE_ENDIANIZE_INT16 (1);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 22 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* channels: 1 (mono) */
	temp16 = LITTLE_ENDIANIZE_INT16 (1);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 24 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* sample rate */
	temp32 = LITTLE_ENDIANIZE_INT32(22050);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 24 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* byte rate */
	temp32 = LITTLE_ENDIANIZE_INT32(22050 * 2);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 28 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* block align (size of one `sample') */
	temp16 = LITTLE_ENDIANIZE_INT32 (2);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 30 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* block align */
	temp16 = LITTLE_ENDIANIZE_INT32 (16);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 32 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* 'data' tag */
	offset += stream_write(destf, "data", 4);
	if( offset < 36 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	/* data size */
	temp32 = LITTLE_ENDIANIZE_INT32(image->count * 2);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 40 )
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	if (stream_write(destf, wavdata, (image->count*2))!=(image->count*2))
    	{
		free(wavdata);
		return IMGTOOLERR_WRITEERROR;
		}

	free (wavdata);

	return 0;
	}

