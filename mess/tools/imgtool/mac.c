/*
	Handlers for macintosh images.

	Currently, we only handle the MFS format.  This was the most format I
	needed most, because modern Macintoshes do not support it any more. In the
	future, I will implement HFS as well.

	Raphael Nabet, 2003
*/

/*
	terminology:
	disk block: 512-byte logical block
	allocation block: "The File Manager always allocates logical disk blocks to
		a file in groups called allocation blocks; an allocation block is
		simply a group of consecutive logical blocks.  The size of a volume's
		allocation blocks depends on the capacity of the volume; there can be
		at most 65,535 allocation blocks on a volume."


	Organization of an MFS volume:

	Logical		Contents									Allocation block
	block

	0 - 1:		System startup information
	2 - 3:		Master directory block (MDB)
				+ unidentified data
	4 - n:		Directory file
	n+1 - p-2:	Other files and free space					0 - (p-2)/k
	p-1:		Alternate MDB
	p:			Not used
	usually, n = 16, p = 799 (SSDD 3.5" floppy) or 1599 (DSDD 3.5" floppy),
	and k = 2
*/

#include <ctype.h>
#include <string.h>
#include "osdepend.h"
#include "imgtoolx.h"

#if 0
#pragma mark MISCELLANEOUS UTILITIES
#endif

typedef struct UINT16BE
{
	UINT8 bytes[2];
} UINT16BE;

typedef struct UINT32BE
{
	UINT8 bytes[4];
} UINT32BE;

INLINE UINT16 get_UINT16BE(UINT16BE word)
{
	return (word.bytes[0] << 8) | word.bytes[1];
}

INLINE void set_UINT16BE(UINT16BE *word, UINT16 data)
{
	word->bytes[0] = (data >> 8) & 0xff;
	word->bytes[1] = data & 0xff;
}

INLINE UINT32 get_UINT32BE(UINT32BE word)
{
	return (word.bytes[0] << 24) | (word.bytes[1] << 16) | (word.bytes[2] << 8) | word.bytes[3];
}

INLINE void set_UINT32BE(UINT32BE *word, UINT32 data)
{
	word->bytes[0] = (data >> 24) & 0xff;
	word->bytes[1] = (data >> 16) & 0xff;
	word->bytes[2] = (data >> 8) & 0xff;
	word->bytes[3] = data & 0xff;
}

/*
	Macintosh string: first byte is lenght
*/
typedef UINT8 mac_str27[28];
typedef UINT8 mac_str255[255];

/*
	Macintosh type/creator code: 4 char value
*/
typedef UINT32BE mac_type;

/*
	point record, with the y and x coordinates
*/
typedef struct mac_point
{
	UINT16BE v;		/* actually signed */
	UINT16BE h;		/* actually signed */
} mac_point;


/*
	FInfo (Finder Info) record
*/
typedef struct mac_FInfo
{
	mac_type  type;			/* file type */
	mac_type  creator;		/* file creator */
	UINT16BE  flags;		/* Finder flags */
	mac_point location;		/* file's location in window */
								/* If set to {0, 0}, the Finder will place the
								item automatically (is it true with earlier
								system versions???) */
	UINT16BE  fldr;			/* MFS: window that contains file */
								/* -3: trash */
								/* -2: desktop */
								/* 0: disk window ("root") */
								/* > 0: specific folder??? */
								/* folder names are to found in the Desktop file (!!!) */
							/* HFS & HFS+: reserved (set to 0) */
} mac_FInfo;

/*
	defines for FInfo flags field
*/
enum
{
	fif_isOnDesk			= 0x0001,	/* System 6: set if item is located on desktop (files and folders) */
										/* System 7: Unused (set to 0) */
	fif_color				= 0x000E,	/* color coding (files and folders) */
	fif_colorReserved		= 0x0010,	/* System 6: reserved??? */
										/* System 7: Unused (set to 0) */
	fif_requiresSwitchLaunch= 0x0020,	/* System 6: ??? */
										/* System 7: Unused (set to 0) */


	fif_isShared			= 0x0040,	/* Applications files: if set, the */
											/* application can be executed by */
											/* multiple users simultaneously. */
										/* Otherwise, set to 0. */
	fif_hasNoINITs			= 0x0080,	/* Extensions/Control Panels: if set(?), */
											/* this file contains no INIT */
											/* resource */
										/* Otherwise, set to 0. */
	fif_hasBeenInited		= 0x0100,	/* System 6: The Finder has recorded information from
											the file’s bundle resource into the desktop
											database and given the file or folder a
											position on the desktop. */
										/* System 7? 8?: Clear if the file contains desktop database */
											/* resources ('BNDL', 'FREF', 'open', 'kind'...) */
											/* that have not been added yet. Set only by the Finder */
											/* Reserved for folders - make sure this bit is cleared for folders */

	/* bit 0x0200 was at a point (AOCE for system 7.x?) the letter bit for
	AOCE, but was reserved before and it is reserved again in recent MacOS
	releases. */

	fif_hasCustomIcon		= 0x0400,	/* (files and folders) */
	fif_isStationery		= 0x0800,	/* System 7: (files only) */
	fif_nameLocked			= 0x1000,	/* (files and folders) */
	fif_hasBundle			= 0x2000,	/* Files only */
	fif_isInvisible			= 0x4000,	/* (files and folders) */
	fif_isAlias				= 0x8000	/* System 7: (files only) */
};

/*
	mac_to_c_strncpy()

	Encode a macintosh string as a C string.  The NUL character is escaped,
	using the "%00" sequence.  To avoid any confusion, '%' is escaped with
	'%25'.

	dst (O): C string
	n (I): lenght of buffer pointed to by dst
	src (I): macintosh string (first byte is lenght)
*/
static void mac_to_c_strncpy(char *dst, int n, UINT8 *src)
{
	size_t len = src[0];
	int i, j;

	i = j = 0;
	while ((i < len) && (j < n-1))
	{
		switch (src[i+1])
		{
		case '\0':
			if (j >= n-3)
				goto exit;
			dst[j] = '%';
			dst[j+1] = '0';
			dst[j+2] = '0';
			j += 3;
			break;

		case '%':
			if (j >= n-3)
				goto exit;
			dst[j] = '%';
			dst[j+1] = '2';
			dst[j+2] = '5';
			j += 3;
			break;

		default:
			dst[j] = src[i+1];
			j++;
			break;
		}
		i++;
	}

exit:
	if (n > 0)
		dst[j] = '\0';
}

/*
	c_to_mac_strncpy()

	Decode a C string to a macintosh string.  The NUL character is escaped,
	using the "%00" sequence.  To avoid any confusion, '%' is escaped with
	'%25'.

	dst (O): macintosh string (first byte is lenght)
	src (I): C string
	n (I): lenght of string pointed to by src
*/
static void c_to_mac_strncpy(mac_str255 dst, const char *src, int n)
{
	size_t len;
	int i;
	char buf[3];


	len = 0;
	i = 0;
	while ((i < n) && (len < 255))
	{
		switch (src[i])
		{
		case '%':
			if (i >= n-2)
				goto exit;
			buf[0] = src[i+1];
			buf[1] = src[i+2];
			buf[2] = '\0';
			dst[len+1] = strtoul(buf, NULL, 16);
			i += 3;
			break;

		default:
			dst[len+1] = src[i];
			i++;
			break;
		}
		len++;
	}

exit:
	dst[0] = len;
}

/*
	check_fname()

	Check a file name.

	fname (I): file name (macintosh string)

	Return non-zero if the file name is invalid.

	TODO: how about script codes in HFS???
*/
static int check_fname(const UINT8 *fname)
{
	size_t len = fname[0];
	int i;

#if 0
	/* lenght >31 is incorrect with HFS, but not MFS */
	if (len > 31)
		return 1;
#endif

	/* check and copy file name */
	for (i=0; i<len; i++)
	{
		switch (fname[i+1])
		{
		case ':':
			/* illegal character */
			return 1;
		default:
			/* all other characters are legal (though control characters,
			particularly NUL characters, are not recommended) */
			break;
		}
	}

	return 0;
}

/*
	Check a file path.

	fpath (I): file path (C string)

	Return non-zero if the file path is invalid.

	TODO: how about foreign scripts in HFS???
*/
static int check_fpath(const char *fpath)
{
	const char *element_start, *element_end;
	int element_len;
	int i, mac_element_len;
	int level;


	/* check that each path element has an acceptable lenght */
	element_start = fpath;
	level = 0;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, ':');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		/* compute element len after decoding */
		mac_element_len = 0;
		i=0;
		while (i<element_len)
		{
			switch (element_start[i])
			{
			case '%':
				if (isxdigit(element_start[i+1]) && isxdigit(element_start[i+2]))
					i += 3;
				else
					return 1;
				break;

			default:
				i++;
				break;
			}
			mac_element_len++;
		}
		if (element_len > 255/*31*/)
			return 1;

		/* iterate */
		if (element_end)
			element_start = element_end+1;
		else
			element_start = NULL;

		if (element_len == 0)
		{	/* '::' refers to parent dir, ':::' to the parent's parent, etc */
			level--;
			if (level < 0)
				return 1;
		}
		else
			level++;
	}
	while (element_start);

	return 0;
}


#if 0
#pragma mark -
#pragma mark DISK IMAGE ROUTINES
#endif

/*
	header of diskcopy 4.2 images
*/
typedef struct diskcopy_header_t
{
	UINT8    diskName[64];	/* name of the disk */
	UINT32BE dataSize;		/* total size of data for all sectors (512*number_of_sectors) */
	UINT32BE tagSize;		/* total size of tag data for all sectors (12*number_of_sectors for GCR 3.5" floppies, 20*number_of_sectors for HD20, 0 otherwise) */
	UINT32BE dataChecksum;	/* CRC32 checksum of all sector data */
	UINT32BE tagChecksum;	/* CRC32 checksum of all tag data */
	UINT8    diskFormat;	/* 0 = 400K, 1 = 800K, 2 = 720K, 3 = 1440K  (other values reserved) */
	UINT8    formatByte;	/* should be $00 Apple II, $01 Lisa, $02 Mac MFS ??? */
							/* $12 = 400K, $22 = >400K Macintosh (DiskCopy uses this value for
							   all Apple II disks not 800K in size, and even for some of those),
							   $24 = 800K Apple II disk */
	UINT16BE private;		/* always $0100 (otherwise, the file may be in a different format. */
} diskcopy_header_t;

typedef struct mac_l1_imgref
{
	IMAGE base;
	STREAM *f;
	enum { bare, apple_diskcopy } format;
	union
	{
		/*struct
		{
		} bare;*/
		struct
		{
			long tag_offset;
		} apple_diskcopy;
	} u;
} mac_l1_imgref;

static int floppy_image_open(mac_l1_imgref *image)
{
	long image_len=0;
	diskcopy_header_t header;


	image->format = bare;	/* default */

	/* read image header */
	if (stream_read(image->f, & header, sizeof(header)) == sizeof(header))
	{
		UINT32 dataSize = get_UINT32BE(header.dataSize);
		UINT32 tagSize = get_UINT32BE(header.tagSize);

		/* various checks : */
		if ((header.diskName[0] <= 63)
				&& (stream_size(image->f) == (dataSize + tagSize + 84))
				&& (get_UINT16BE(header.private) == 0x0100))
		{
			image->format = apple_diskcopy;
			image_len = dataSize;
			image->u.apple_diskcopy.tag_offset = (tagSize == (dataSize / 512 * 12))
													? dataSize + 84
													: 0;
			/*image->formatByte = header.formatByte & 0x3F;*/
		}
	}

	if (image->format == bare)
	{
		image_len = stream_size(image->f);
	}

#if 0
	switch(image_len) {
	case 80*10*512*1:
		/* Single sided (400k) */
		if (image->format == bare)
			image->formatByte = 0x02;	/* single-sided Macintosh (?) */
		break;
	case 80*10*512*2:
		/* Double sided (800k) */
		if (image->format == bare)
			image->formatByte = 0x22;	/* double-sided Macintosh (?) */
		break;
	default:
		/* Bad floppy size */
		goto error;
		break;
	}
#endif

	return 0;
}


static int image_read_block(mac_l1_imgref *image, UINT64 block, void *dest)
{
	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_read(image->f, dest, 512) != 512)
		return IMGTOOLERR_READERROR;

#if 0
	/* now append tag data */
	if ((image->format == apple_diskcopy) && (image->u.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		if (stream_seek(image->f, image->u.apple_diskcopy.tag_offset + block*12, SEEK_SET))
			return IMGTOOLERR_READERROR;

		if (stream_read(image->f, (UINT8 *) dest + 512, 12) != 12)
			return IMGTOOLERR_READERROR;
	}
	else
	{
		/* no tag data stored in disk image - so we zero the tag data */
		for (i = 0; i < len; i++)
		{
			memset((UINT8 *) dest + 512, 0, 12);
		}
	}
#endif

	return 0;
}

static int image_write_block(mac_l1_imgref *image, UINT64 block, const void *src)
{
	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_write(image->f, src, 512) != 512)
		return IMGTOOLERR_READERROR;

#if 0
	/* now append tag data */
	if ((image->format == apple_diskcopy) && (image->u.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		if (stream_seek(image->f, image->u.apple_diskcopy.tag_offset + block*12, SEEK_SET))
			return IMGTOOLERR_READERROR;

		if (stream_read(image->f, (UINT8 *) src + 512, 12) != 12)
			return IMGTOOLERR_READERROR;
	}
	else
	{
		/* no tag data stored in disk image - so we zero the tag data */
		for (i = 0; i < len; i++)
		{
			memset((UINT8 *) src + 512, 0, 12);
		}
	}
#endif

	return 0;
}

#if 0
#pragma mark -
#pragma mark MFS IMPLEMENTATION
#endif

/*
	Master Directory Block
*/
typedef struct mfs_mdb
{
	UINT8    sigWord[2];	/* volume signature - always $D2D7 */
	UINT32BE crDate;		/* date and time of volume creation */
	UINT32BE lsMod/*lsBkUp???*/;/* date and time of last modification (backup???) */
	UINT16BE atrb;			/* volume attributes (0x0000) */
								/* bit 15 is set if volume is locked by software */
	UINT16BE nmFls;			/* number of files in directory */
	UINT16BE dirSt;			/* first block of directory */

	UINT16BE blLn;			/* lenght of directory in blocks (0x000C) */
	UINT16BE nmBlks;		/* number of allocation blocks (0x0187) */
	UINT32BE alBlkSiz;		/* size of allocation blocks (0x00000400) */
	UINT32BE clpSiz;		/* default clump size - number of bytes to allocate (0x00002000) */
	UINT16BE alBlSt;		/* first allocation block in block map (0x0010) */

	UINT32BE nxtFNum;		/* next unused file number */
	UINT16BE freeBks;		/* number of unused allocation blocks */

	mac_str27 VN;			/* volume name */

	UINT8    unknown[512-64];	/* ??? */

	/* the rest of this block and the next one seems to include a list of
	sorts; or maybe it is 68k code.  Anyway, it is not included in the
	alternate VCB. */
} mfs_mdb;

/*
	directory entry for use in the directory file

	Note the structure is variable lenght.  It is always word-aligned, and
	cannot cross block boundaries.
*/
typedef struct mfs_dir_entry
{
	UINT8    flags;				/* bit 7=1 if entry used, bit 0=1 if file locked */
								/* 0x00 means end of block: if we are not done
								with reading the directory, the remnants will
								be read from next block */
	UINT8    typ;				/* version number (usually 0x00, but I don't have the IM volume that describes it) */

	mac_FInfo flFinderInfo;		/* information used byt the Finder */

	UINT32BE flNum;				/* file number */

	UINT16BE dataStartBlock;	/* first allocation block of data fork */
	UINT32BE dataLogicalSize;	/* logical EOF of data fork */
	UINT32BE dataPhysicalSize;	/* physical EOF of data fork */
	UINT16BE rsrcStartBlock;	/* first allocation block of ressource fork */
	UINT32BE rsrcLogicalSize;	/* logical EOF of resource fork */
	UINT32BE rsrcPhysicalSize;	/* physical EOF of resource fork */

	UINT32BE createDate;		/* date and time of creation */
	UINT32BE modifyDate;		/* date and time of last modification */

	UINT8    name[1];			/* first char is lenght of file name */
								/* next chars are file name - 255 chars at most */
								/* IIRC, Finder 7 only supports 31 chars,
								wheareas earlier versions support 63 chars */
} mfs_dir_entry;

/*
	MFS image ref
*/
typedef struct mfs_l2_imgref
{
	mac_l1_imgref l1_img;

	UINT16 dir_num_files;
	UINT16 dir_start;
	UINT16 dir_blk_len;

	UINT16 num_free_blocks;

	mac_str27 volname;
} mfs_l2_imgref;

/*
	mfs_image_open

	Open a MFS image.  Image must already be open on level 1.
*/
static int mfs_image_open(mfs_l2_imgref *l2_img)
{
	mfs_mdb mdb;
	int errorcode;

	errorcode = image_read_block(&l2_img->l1_img, 2, &mdb);
	if (errorcode)
		return errorcode;

	if ((mdb.sigWord[0] != 0xd2) || (mdb.sigWord[1] != 0xd7) || (mdb.VN[0] > 27))
		return IMGTOOLERR_CORRUPTIMAGE;

	l2_img->dir_num_files = get_UINT16BE(mdb.nmFls);
	l2_img->dir_start = get_UINT16BE(mdb.dirSt);
	l2_img->dir_blk_len = get_UINT16BE(mdb.blLn);

	l2_img->num_free_blocks = get_UINT16BE(mdb.freeBks);

	memcpy(l2_img->volname, mdb.VN, mdb.VN[0]+1);


	return 0;
}

/*
	mfs_find_dir_entry

	Find a file in an MFS directory
*/
static int mfs_find_dir_entry(mfs_l2_imgref *l2_img, const char *fpath /* ... */)
{
	mac_str255 fname;

	if (strchr(fpath, ':'))
		return IMGTOOLERR_BADFILENAME;

	c_to_mac_strncpy(fname, fpath, strlen(fpath));

	/* ... */

	return 0;
}


#if 0
#pragma mark -
#pragma mark IMGTOOL MODULE IMPLEMENTATION
#endif

/*
	MFS catalog iterator, used when imgtool reads the catalog
*/
typedef struct mfs_iterator
{
	IMAGEENUM base;
	mfs_l2_imgref *image;
	UINT16 index;					/* current index in the disk catalog */
	UINT16 cur_block;				/* current block offset in directory file */
	UINT16 cur_offset;				/* current byte offset in current block of directory file */
	UINT8 block_buffer[512];		/* buffer with current directory block */
} mfs_iterator;



static int mfs_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void mfs_image_exit(IMAGE *img);
static void mfs_image_info(IMAGE *img, char *string, const int len);
static int mfs_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int mfs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void mfs_image_closeenum(IMAGEENUM *enumeration);
static size_t mfs_image_freespace(IMAGE *img);
static int mfs_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int mfs_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int mfs_image_deletefile(IMAGE *img, const char *fname);
static int mfs_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int mfs_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int mfs_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

#if 0
static struct OptionTemplate mfs_createopts[] =
{
	/*{ "label",	"Volume name", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,	0,	NULL},*/
	{ NULL, NULL, 0, 0, 0, 0 }
};
#endif

/*enum
{
	mfs_createopts_volname = 0
};*/

IMAGEMODULE(
	mfs,
	"MFS macintosh image",			/* human readable name */
	"img",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	mfs_image_init,					/* init function */
	mfs_image_exit,					/* exit function */
	mfs_image_info,					/* info function */
	mfs_image_beginenum,			/* begin enumeration */
	mfs_image_nextenum,				/* enumerate next */
	mfs_image_closeenum,			/* close enumeration */
	mfs_image_freespace,			/* free space on image */
	/*mfs_image_readfile*/NULL,		/* read file */
	/*mfs_image_writefile*/NULL,	/* write file */
	/*mfs_image_deletefile*/NULL,	/* delete file */
	/*mfs_image_create*/NULL,		/* create image */
	/*mfs_read_sector*/NULL,
	/*mfs_write_sector*/NULL,
	NULL,							/* file options */
	/*mfs_createopts*/NULL			/* create options */
)

/*
	Open a file as a ti99_image (common code).
*/
static int mfs_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	mfs_l2_imgref *image;
	int errorcode;


	image = malloc(sizeof(mfs_l2_imgref));
	* (mfs_l2_imgref **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(mfs_l2_imgref));
	image->l1_img.base.module = mod;
	image->l1_img.f = f;

	errorcode = floppy_image_open(&image->l1_img);
	if (errorcode)
		return errorcode;

	return mfs_image_open(image);
}

/*
	close a ti99_image
*/
static void mfs_image_exit(IMAGE *img)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	free(image);
}

/*
	get basic information on a ti99_image

	Currently returns the volume name
*/
static void mfs_image_info(IMAGE *img, char *string, const int len)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	mac_to_c_strncpy(string, len, image->volname);
}

/*
	Open the disk catalog for enumeration 
*/
static int mfs_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;
	mfs_iterator *iter;


	iter = malloc(sizeof(mfs_iterator));
	*((mfs_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->image = image;
	iter->index = 0;

	iter->cur_block = 0;
	iter->cur_offset = 0;
	image_read_block(&image->l1_img, image->dir_start + iter->cur_block, iter->block_buffer);

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int mfs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	mfs_iterator *iter = (mfs_iterator *) enumeration;
	mfs_dir_entry *cur_dir_entry;
	size_t cur_dir_entry_len;


	ent->corrupt = 0;
	ent->eof = 0;

	if (iter->index == iter->image->dir_num_files)
	{
		ent->eof = 1;
		return 0;
	}
	

	/* get cat entry pointer */
	cur_dir_entry = (mfs_dir_entry *) (iter->block_buffer + iter->cur_offset);
	while ((iter->cur_block == 512) || ! (cur_dir_entry->flags & 0x80))
	{
		iter->cur_block++;
		iter->cur_offset = 0;
		if (iter->cur_block > iter->image->dir_blk_len)
		{	/* aargh! */
			ent->corrupt = 1;
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		image_read_block(&iter->image->l1_img, iter->image->dir_start + iter->cur_block, iter->block_buffer);
		cur_dir_entry = (mfs_dir_entry *) (iter->block_buffer + iter->cur_offset);
	}

	cur_dir_entry_len = offsetof(mfs_dir_entry, name) + cur_dir_entry->name[0] + 1;

	if ((iter->cur_offset + cur_dir_entry_len) > 512)
	{	/* aargh! */
		ent->corrupt = 1;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	mac_to_c_strncpy(ent->fname, ent->attr_len, cur_dir_entry->name);
	snprintf(ent->attr, ent->attr_len, "%s", "");
	ent->filesize = get_UINT32BE(cur_dir_entry->dataPhysicalSize)
						+ get_UINT32BE(cur_dir_entry->rsrcPhysicalSize);

	/* update offset in block */
	iter->cur_offset += cur_dir_entry_len;
	/* align to word boundary */
	iter->cur_offset = (iter->cur_offset + 1) & ~1;

	iter->index++;

	return 0;
}

/*
	Free enumerator
*/
static void mfs_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in allocation blocks?)
*/
static size_t mfs_image_freespace(IMAGE *img)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	return image->num_free_blocks;
}

/*
	Extract a file from a ti99_image.  The file is saved in tifile format.
*/
static int mfs_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;
	int errorcode;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	errorcode = mfs_find_dir_entry(image, fname /* ... */);
	if (errorcode)
		return errorcode;

	/* ... */

	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Add a file to a ti99_image.  The file must be in tifile format.
*/
static int mfs_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Delete a file from a ti99_image.
*/
static int mfs_image_deletefile(IMAGE *img, const char *fname)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Create a blank ti99_image (common code).

	Supports MESS and V9T9 formats only
*/
static int mfs_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Read one sector from a ti99_image.
*/
static int mfs_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Write one sector to a ti99_image.
*/
static int mfs_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}
