/*
 * msx_dsk.c : converts .ddi/.img/.msx disk images to .dsk image
 *
 * Sean Young
 */

#include "osdepend.h"
#include "imgtool.h"
#include "utils.h"

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | \
                       (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

#define 	FORMAT_IMG		(1)
#define 	FORMAT_DDI		(2)
#define 	FORMAT_MSX		(3)
#define 	FORMAT_MULTI	(4)

typedef struct {
	IMAGE			base; 
	char			*file_name;
	STREAM 			*file_handle;
	int 			size, format, disks;
	} DSK_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	DSK_IMAGE	*image;
	int			index;
	} DSK_ITERATOR;

static int msx_img_image_init(STREAM *f, IMAGE **outimg);
static int msx_ddi_image_init(STREAM *f, IMAGE **outimg);
static int msx_msx_image_init(STREAM *f, IMAGE **outimg);
static int msx_multi_image_init(STREAM *f, IMAGE **outimg);
static int msx_dsk_image_init(STREAM *f, IMAGE **outimg, int format);
static void msx_dsk_image_exit(IMAGE *img);
static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void msx_dsk_image_closeenum(IMAGEENUM *enumeration);
static int msx_dsk_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	msx_img,
	"MSX .img diskimage",				/* human readable name */
	"img\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_img_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_ddi,
	"MSX DiskDupe .ddi diskimage",		/* human readable name */
	"ddi\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_ddi_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_msx,
	"MSX .msx diskimage",		/* human readable name */
	"ddi\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_msx_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

IMAGEMODULE(
	msx_mul,
	"MSX multi disk image",				/* human readable name */
	"dsk\0",							/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_multi_image_init,				/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,								/* write file */
	NULL,								/* delete file */
	NULL,								/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,								/* file options */
	NULL								/* create options */
)

static int msx_img_image_init(STREAM *f, IMAGE **outimg) 
	{ return msx_dsk_image_init (f, outimg, FORMAT_IMG); }

static int msx_ddi_image_init(STREAM *f, IMAGE **outimg) 
	{ return msx_dsk_image_init (f, outimg, FORMAT_DDI); }

static int msx_msx_image_init(STREAM *f, IMAGE **outimg) 
	{ return msx_dsk_image_init (f, outimg, FORMAT_MSX); }

static int msx_multi_image_init(STREAM *f, IMAGE **outimg) 
	{ return msx_dsk_image_init (f, outimg, FORMAT_MULTI); }

static int msx_dsk_image_init(STREAM *f, IMAGE **outimg, int format)
	{
	DSK_IMAGE *image;
	int len, disks, correct;
    UINT8 header;

	len=stream_size(f);
	if (len < (360*1024) ) return IMGTOOLERR_MODULENOTFOUND;
	if (1 != stream_read (f, &header, 1) ) return IMGTOOLERR_READERROR;
	
	disks = 1;

	correct = 0;

	switch (format)
		{
		case FORMAT_IMG:
			if ( (len == (720*1024+1) ) && (header == 2) )
				{
				correct = 1;
				len--;
				}
			if ( (len == (360*1024+1) ) && (header == 1) )
				{
				correct = 1;
				len--;
				}
			break;
		case FORMAT_MSX:
			if (len == (720*1024) )
				correct = 1;
			break;
		case FORMAT_DDI:
			if (len == (720*1024+0x1800) )
				{
				len -= 0x1800;
				correct = 1;
				}
			break;
		case FORMAT_MULTI:
			if ( (len > 720*1024) && !(len % (720*1024) ) )
				{
				disks = len / (720*1024);
				correct = 1;
				len = 720*1024;
				}
			break;
		}
	
	if (!correct) return IMGTOOLERR_MODULENOTFOUND;

	image = (DSK_IMAGE*)malloc (sizeof (DSK_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(DSK_IMAGE));
	image->base.module = &imgmod_msx_img; 
	image->file_handle = f;
	image->size = len;
	image->format = format;
	image->file_name = NULL;
	image->disks = disks;

	return 0;
	}

static void msx_dsk_image_exit(IMAGE *img)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	stream_close(image->file_handle);
	if (image->file_name) free(image->file_name);
	free(image);
	}

static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	DSK_ITERATOR *iter;

	iter=*(DSK_ITERATOR**)outenum = (DSK_ITERATOR*) malloc(sizeof(DSK_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_msx_img; 

	iter->image=image;
	iter->index = 1;
	return 0;
	}

static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
	{
	DSK_ITERATOR *iter=(DSK_ITERATOR*)enumeration;

	ent->eof = (iter->index > iter->image->disks);
	if (!ent->eof)
		{
		if (iter->image->file_name)
			strcpy (ent->fname, iter->image->file_name);
		else
			{
			if (iter->image->disks ==  1)
				strcpy (ent->fname, "msx.dsk");
			else
				sprintf (ent->fname, "msx-%d.dsk", iter->index);
			}

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
	UINT8	buf[0x1200];
	int 	i, offset, n1, n2, disks;

	/*  check file name */
	switch (image->format)
		{
		case FORMAT_IMG: 
		case FORMAT_DDI: 
		case FORMAT_MSX: 
			if (stricmp (fname, "msx.dsk") ) 
				return IMGTOOLERR_MODULENOTFOUND;
			break;
		case FORMAT_MULTI:
			if (1 != sscanf (fname, "msx-%d.dsk", &disks) )
				return IMGTOOLERR_MODULENOTFOUND;
			if (disks < 1 || disks > image->disks)
				return IMGTOOLERR_MODULENOTFOUND;
			break;
		}

	switch (image->format)
		{
		case FORMAT_IMG: offset = 1; i = (image->size / 0x1200); break;
		case FORMAT_DDI: offset = 0x1800; i = 160; break;
		case FORMAT_MSX:
			{
			i = 80; n1 = 0; n2 = 80;
			while (i--)
				{
				stream_seek (image->file_handle, n1++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;

				stream_seek (image->file_handle, n2++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;
				}

			return 0;
			}
		case FORMAT_MULTI:	/* multi disk */
			i = 160; offset = 720*1024 * (disks - 1);
			break;
		default:
			return IMGTOOLERR_UNEXPECTED;
		}

	stream_seek (image->file_handle, offset, SEEK_SET);

	while (i--)
		{
		if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
			return IMGTOOLERR_READERROR;

		if (0x1200 != stream_write (destf, buf, 0x1200) )
			return IMGTOOLERR_WRITEERROR;
		}

	return 0;
	}

