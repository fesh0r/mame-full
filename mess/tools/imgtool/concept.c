/*
	Handlers for ti990 disk images

	Disk images are in MESS format.

	Raphael Nabet, 2003
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"

#include "snprintf.h"

/* Max sector lenght is bytes.  Both 256 and 512 byte sectors are known. */
/* I chose a limit of 512. */
#define MAX_SECTOR_SIZE 512
#define MIN_SECTOR_SIZE 256

typedef struct UINT16xE
{
	UINT8 bytes[2];
} UINT16xE;

INLINE UINT16 get_UINT16xE(int little_endian, UINT16xE word)
{
	return little_endian ? (word.bytes[0] | (word.bytes[1] << 8)) : ((word.bytes[0] << 8) | word.bytes[1]);
}

INLINE void set_UINT16xE(int little_endian, UINT16xE *word, UINT16 data)
{
	if (little_endian)
	{
		word->bytes[0] = data & 0xff;
		word->bytes[1] = (data >> 8) & 0xff;
	}
	else
	{
		word->bytes[0] = (data >> 8) & 0xff;
		word->bytes[1] = data & 0xff;
	}
}

/*
	Disk structure:

	Track 0 Sector 0 & 1: bootstrap loader
	Track 0 Sector 2 through 5: disk directory
	Remaining sectors are used for data.
*/

/*
	device directory record (Disk sector 2-5)
*/

typedef struct concept_vol_hdr_entry
{
	UINT16xE	first_block;
	UINT16xE	next_block;
	UINT16xE	ftype;

	unsigned char	volname[8];
	UINT16xE	last_block;
	UINT16xE	num_files;
	UINT16xE	last_boot;
	UINT16xE	last_access;
	char		mem_flipped;
	char		disk_flipped;
	UINT16xE	unused;
} concept_vol_hdr_entry;

typedef struct concept_file_dir_entry
{
	UINT16xE	first_block;
	UINT16xE	next_block;
	UINT16xE	ftype;

	unsigned char	fname[16];
	UINT16xE	last_byte;
	UINT16xE	last_access;
} concept_file_dir_entry;

typedef struct concept_dev_dir
{
	concept_vol_hdr_entry vol_hdr;
	concept_file_dir_entry file_dir[77];
	char unused[20];
} concept_dev_dir;

/*
	concept disk image descriptor
*/
typedef struct concept_image
{
	IMAGE base;
	STREAM *file_handle;		/* imgtool file handle */
	concept_dev_dir dev_dir;	/* cached copy of device directory */
} concept_image;

/*
	concept catalog iterator, used when imgtool reads the catalog
*/
typedef struct concept_iterator
{
	IMAGEENUM base;
	concept_image *image;
	int index;							/* current index */
} concept_iterator;


static int concept_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void concept_image_exit(IMAGE *img);
static void concept_image_info(IMAGE *img, char *string, const int len);
static int concept_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int concept_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void concept_image_closeenum(IMAGEENUM *enumeration);
static size_t concept_image_freespace(IMAGE *img);
static int concept_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int concept_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int concept_image_deletefile(IMAGE *img, const char *fname);
static int concept_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int concept_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int concept_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

IMAGEMODULE(
	concept,
	"concept floppy",				/* human readable name */
	"img",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	concept_image_init,				/* init function */
	concept_image_exit,				/* exit function */
	concept_image_info,				/* info function */
	concept_image_beginenum,		/* begin enumeration */
	concept_image_nextenum,			/* enumerate next */
	concept_image_closeenum,		/* close enumeration */
	/*concept_image_freespace*/NULL,	/* free space on image */
	concept_image_readfile,			/* read file */
	/*concept_image_writefile*/NULL,	/* write file */
	/*concept_image_deletefile*/NULL,	/* delete file */
	/*concept_image_create*/NULL,		/* create image */
	/*concept_read_sector*/NULL,
	/*concept_write_sector*/NULL,
	NULL,								/* file options */
	NULL								/* create options */
)

#if 0
/*
	Convert a C string to a 8-character file name (padded with spaces if necessary)
*/
static void str_to_fname(char dst[8], const char *src)
{
	int i;


	i = 0;

	/* copy 8 characters at most */
	if (src)
		while ((i<8) && (src[i]!='\0'))
		{
			dst[i] = src[i];
			i++;
		}

	/* pad with spaces */
	while (i<8)
	{
		dst[i] = ' ';
		i++;
	}
}
#endif

/*
	Convert a 10-character file name to a C string (removing trailing spaces if necessary)
*/
static void fname_to_str(char *dst, const char src[8], int n)
{
	int i;
	int last_nonspace;


	/* copy 8 characters at most */
	if (--n > 8)
		n = 8;

	/* copy filename */
	i = 0;
	last_nonspace = -1;

	while (i<n)
	{
		dst[i] = src[i];
		if (src[i] != ' ')
			last_nonspace = i;
		i++;
	}

	/* terminate with '\0' */
	dst[last_nonspace+1] = '\0';
}

/*
	Read one 512 byte physical record from a disk image

	file_handle: imgtool file handle
	secnum: physical record address
	dest: pointer to destination buffer
*/
static int read_physical_record(STREAM *file_handle, int secnum, void *dest)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, secnum*512, SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_read(file_handle, dest, 512);
	if (reply != 512)
		return 1;

	return 0;
}

/*
	Write one sector to a disk image

	file_handle: imgtool file handle
	secnum: logical sector address
	src: pointer to source buffer
*/
static int write_physical_record(STREAM *file_handle, int secnum, const void *src)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, secnum*512, SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_write(file_handle, src, 512);
	if (reply != 512)
		return 1;

	return 0;
}

static int get_catalog_entry(concept_image *image, unsigned char *fname, int *entry_index)
{
	int fname_len = fname[0];
	int i;

	if (fname_len > 15)
		return 1;

	for (i = 0; i < 77; i++)
	{
		if (!memcmp(fname, image->dev_dir.file_dir[i].fname, fname_len+1))
		{
			*entry_index = i;
			return 0;
		}
	}

	return 1;
}

/*
	Open a file as a concept_image.
*/
static int concept_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	concept_image *image;
	int reply;
	int i;
	unsigned totphysrecs;


	image = malloc(sizeof(concept_image));
	* (concept_image **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(concept_image));
	image->base.module = mod;
	image->file_handle = f;

	for (i=0; i<4; i++)
	{
		reply = read_physical_record(f, i+2, ((char *) & image->dev_dir)+i*512);
		if (reply)
			return IMGTOOLERR_READERROR;
	}

	totphysrecs = get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.vol_hdr.last_block)
					- get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.vol_hdr.first_block);

	if ((get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.vol_hdr.first_block) != 0)
		|| (get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.vol_hdr.next_block) != 6)
		|| (totphysrecs < 6) /*|| (stream_size(f) != totphysrecs*512)*/
		|| (image->dev_dir.vol_hdr.volname[0] > 7))
	{
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	return 0;
}

/*
	close a ti99_image
*/
static void concept_image_exit(IMAGE *img)
{
	concept_image *image = (concept_image *) img;

	stream_close(image->file_handle);
	free(image);
}

/*
	get basic information on a ti99_image

	Currently returns the volume name
*/
static void concept_image_info(IMAGE *img, char *string, const int len)
{
	concept_image *image = (concept_image *) img;
	char vol_name[8];

	memcpy(vol_name, image->dev_dir.vol_hdr.volname + 1, image->dev_dir.vol_hdr.volname[0]);
	vol_name[image->dev_dir.vol_hdr.volname[0]] = 0;

	snprintf(string, len, "%s", vol_name);
}

/*
	Open the disk catalog for enumeration 
*/
static int concept_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	concept_image *image = (concept_image*) img;
	concept_iterator *iter;

	iter = malloc(sizeof(concept_iterator));
	*((concept_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->image = image;

	iter->index = 0;

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int concept_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	concept_iterator *iter = (concept_iterator*) enumeration;


	ent->corrupt = 0;
	ent->eof = 0;

	if ((iter->image->dev_dir.file_dir[iter->index].fname[0] == 0) || (iter->index > 77))
	{
		ent->eof = 1;
	}
	else if (iter->image->dev_dir.file_dir[iter->index].fname[0] > 15)
	{
		ent->corrupt = 1;
	}
	else
	{
		int len = iter->image->dev_dir.file_dir[iter->index].fname[0];
		const char *type;

		if (len > ent->fname_len)
			len = ent->fname_len;
		memcpy(ent->fname, iter->image->dev_dir.file_dir[iter->index].fname + 1, len);
		ent->fname[len] = 0;

		/* parse flags */
		switch (get_UINT16xE(iter->image->dev_dir.vol_hdr.disk_flipped, iter->image->dev_dir.file_dir[iter->index].ftype) & 0xf)
		{
		case 0:
		case 8:
			type = "DIRHDR";
			break;
		case 2:
			type = "CODE";
			break;
		case 3:
			type = "TEXT";
			break;
		case 5:
			type = "DATA";
			break;
		default:
			type = "???";
			break;
		}
		snprintf(ent->attr, ent->attr_len, "%s", type);

		/* len in physrecs */
		ent->filesize = get_UINT16xE(iter->image->dev_dir.vol_hdr.disk_flipped, iter->image->dev_dir.file_dir[iter->index].next_block)
							- get_UINT16xE(iter->image->dev_dir.vol_hdr.disk_flipped, iter->image->dev_dir.file_dir[iter->index].first_block);

		iter->index++;
	}

	return 0;
}

/*
	Free enumerator
*/
static void concept_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image
*/
static size_t concept_image_freespace(IMAGE *img)
{
	/* ... */

	return 0;
}

/*
	Extract a file from a concept_image.
*/
static int concept_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	concept_image *image = (concept_image *) img;
	size_t fname_len = strlen(fname);
	unsigned char concept_fname[16];
	int catalog_index;
	int i;
	UINT8 buf[512];

	if (fname_len > 15)
		return IMGTOOLERR_BADFILENAME;

	concept_fname[0] = fname_len;
	memcpy(concept_fname+1, fname, fname_len);

	if (get_catalog_entry(image, concept_fname, &catalog_index))
		return IMGTOOLERR_FILENOTFOUND;

	for (i = get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.file_dir[catalog_index].first_block);
			i < get_UINT16xE(image->dev_dir.vol_hdr.disk_flipped, image->dev_dir.file_dir[catalog_index].next_block);
			i++)
	{
		if (read_physical_record(image->file_handle, i, buf))
			return IMGTOOLERR_READERROR;

		if (stream_write(destf, buf, 512) != 512)
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

/*
	Add a file to a concept_image.
*/
static int concept_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *in_options)
{
	/* ... */

	return 0;
}

/*
	Delete a file from a concept_image.
*/
static int concept_image_deletefile(IMAGE *img, const char *fname)
{
	/* ... */

	return 0;
}

/*
	Create a blank concept_image.
*/
static int concept_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	/* ... */

	return 0;
}

/*
	Read one sector from a concept_image.
*/
static int concept_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

/*
	Write one sector to a concept_image.
*/
static int concept_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}
