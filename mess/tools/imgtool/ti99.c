/*
	Handlers for ti99 disk images

	Disk images are in v9t9/MESS format.  Files are extracted in TIFILES format.

	Raphael Nabet, 2002
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"

#include "snprintf.h"

/*
	Disk structure:

	Sector 0: see below
	Sector 1: catalog: array of 0 through 128 words, number of the fdr sector for
		the file.  Sorted according to the file name.
	Remaining sectors are used for fdr and data.
*/

/*
	SC0 record (Disk sector 0)
*/
typedef struct ti99_sc0
{
	char	name[10];			/* volume name (10 characters, pad with spaces) */
	UINT8	totsecsMSB;			/* disk length in sectors (big-endian) (usually 360, 720 or 1440) */
	UINT8	totsecsLSB;
	UINT8	secspertrack;		/* sectors per track (usually 9 (FM) or 18 (MFM)) */
	UINT8	id[4];				/* 'DSK ' */
	UINT8	tracksperside;		/* tracks per side (usually 40) */
	UINT8	sides;				/* sides (1 or 2) */
	UINT8	density;			/* 1 (FM) or 2 (MFM) */
	UINT8	res[36];			/* reserved */
	UINT8	abm[200];			/* allocation bitmap: a 1 for each sector in use (sector 0 is LSBit of byte 0, sector 7 is MSBit of byte 0, sector 8 is LSBit of byte 1, etc.) */
} ti99_sc0;

/*
	file flags found in fdr (and tifiles)
*/
enum
{
	fdr99_f_program	= 0x01,		/* set for program files */
	fdr99_f_int		= 0x02,		/* set for binary files */
	fdr99_f_wp		= 0x08,		/* set if file is write-protected */
	fdr99_f_var		= 0x80		/* set if file uses variable-length records*/
};

/*
	fdr record
*/
typedef struct ti99_fdr
{
	char name[10];			/* file name (10 characters, pad with spaces) */
	UINT8 res10[2];			/* reserved */
	UINT8 flags;			/* see enum above */
	UINT8 recspersec;		/* records per sector */
	UINT8 secsused_MSB;		/* file length in sectors (big-endian) */
	UINT8 secsused_LSB;
	UINT8 eof;				/* current position of eof in last sector (0->255)*/
	UINT8 reclen;			/* bytes per record ([1,255] 0->256) */
	UINT8 fixrecs_LSB;		/* file length in records (little-endian) */
	UINT8 fixrecs_MSB;
	UINT8 res20[8];			/* reserved */
	UINT8 lnks[76][3];		/* link table: 0 through 76 entries: 12 bits: id of first physical sector of fragment, 12 bits: id of last logical sector of fragment */
} ti99_fdr;


#define get_fdr_secsused(fdr) (((fdr)->secsused_MSB << 8) | (fdr)->secsused_LSB)

INLINE void set_fdr_secsused(ti99_fdr *fdr, int data)
{
	fdr->secsused_MSB = data >> 8;
	fdr->secsused_LSB = data;
}

/*
	tifile header: stand-alone file 
*/
typedef struct tifile_header
{
	char tifiles[8];		/* always '\7TIFILES' */
	UINT8 secsused_MSB;		/* file length in sectors (big-endian) */
	UINT8 secsused_LSB;
	UINT8 flags;			/* see enum above */
	UINT8 recspersec;		/* records per sector */
	UINT8 eof;				/* current position of eof in last sector (0->255)*/
	UINT8 reclen;			/* bytes per record ([1,255] 0->256) */
	UINT8 fixrecs_MSB;		/* file length in records (big-endian) */
	UINT8 fixrecs_LSB;
	UINT8 res[128-14];		/* reserved */
} tifile_header;


/*
	catalog entry (used for in-memory catalog)
*/
typedef struct catalog_entry
{
	UINT16 fdr_secnum;
	char fname[10];
} catalog_entry;

/*
	Disk geometry
*/
typedef struct ti99_geometry
{
	int secspertrack;
	int tracksperside;
	int sides;
} ti99_geometry;

/*
	Physical sector address
*/
typedef struct ti99_phys_sec_address
{
	int sector;
	int cylinder;
	int side;
} ti99_phys_sec_address;

/*
	ti99 disk image descriptor
*/
typedef struct ti99_image
{
	IMAGE base;
	STREAM *file_handle;		/* imgtool file handle */
	ti99_geometry geometry;		/* geometry */
	ti99_sc0 sec0;				/* cached copy of sector 0 */
	catalog_entry catalog[128];	/* catalog (fdr sector ids from sector 1, and file names from fdrs) */
	int data_offset;	/* fdr records are preferentially allocated in sectors 2 to data_offset (34)
						whereas data records are preferentially allocated in sectors starting at data_offset. */
} ti99_image;

/*
	ti99 catalog iterator, used when imgtool reads the catalog
*/
typedef struct ti99_iterator
{
	IMAGEENUM base;
	ti99_image *image;
	int index;			/* current index in the disk catalog */
} ti99_iterator;


static int ti99_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void ti99_image_exit(IMAGE *img);
static void ti99_image_info(IMAGE *img, char *string, const int len);
static int ti99_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int ti99_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void ti99_image_closeenum(IMAGEENUM *enumeration);
static size_t ti99_image_freespace(IMAGE *img);
static int ti99_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int ti99_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int ti99_image_deletefile(IMAGE *img, const char *fname);
static int ti99_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int ti99_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int ti99_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

static struct OptionTemplate ti99_createopts[] =
{
	{ "volume name",	NULL, IMGOPTION_FLAG_TYPE_STRING,	0,	0,	NULL},
	{ "density",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,	2,	NULL},
	{ "sides",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,	2,	NULL},
	{ "tracks",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,	40,	NULL},
	{ "sectors",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,	18,	NULL},
	{ NULL, NULL, 0, 0, 0, 0 }
};

enum
{
	ti99_createopts_volname = 0,
	ti99_createopts_density = 1,
	ti99_createopts_sides = 2,
	ti99_createopts_tracks = 3,
	ti99_createopts_sectors = 4
};

IMAGEMODULE(
	ti99,
	"TI99 Diskette",				/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	/*EOLN_CR*/0,					/* eoln */
	0,								/* flags */
	ti99_image_init,				/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_image_beginenum,			/* begin enumeration */
	ti99_image_nextenum,			/* enumerate next */
	ti99_image_closeenum,			/* close enumeration */
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	ti99_image_deletefile,			/* delete file */
	ti99_image_create,				/* create image */
	ti99_read_sector,
	ti99_write_sector,
	NULL,							/* file options */
	ti99_createopts					/* create options */
)

/*
	Convert a C string to a 10-character file name (padded with spaces if necessary)
*/
static void str_to_fname(char dst[10], const char *src)
{
	int i;


	i = 0;

	/* copy 10 characters at most */
	if (src)
		while ((i<10) && (src[i]!='\0'))
		{
			dst[i] = src[i];
			i++;
		}

	/* pad with spaces */
	while (i<10)
	{
		dst[i] = ' ';
		i++;
	}
}

/*
	Convert physical sector address to offset
*/
static int phys_address_to_offset(const ti99_phys_sec_address *address, const ti99_geometry *geometry)
{
	int offset;

#if 1
	/* current MESS format */
	offset = (((address->cylinder*geometry->sides) + address->side)*geometry->secspertrack + address->sector)*256;
#else
	/* V9T9 format */
	offset = (((address->side*geometry->tracksperside) + address->cylinder)*geometry->secspertrack + address->sector)*256;
#endif

	return offset;
}

/*
	Read one sector from a disk image

	file_handle: imgtool file handle
	address: physical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	dest: pointer to 256-byte destination buffer
*/
static int read_sector_physical(STREAM *file_handle, const ti99_phys_sec_address *address, const ti99_geometry *geometry, void *dest)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, phys_address_to_offset(address, geometry), SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_read(file_handle, dest, 256);
	if (reply != 256)
		return 1;

	return 0;
}

/*
	Write one sector to a disk image

	file_handle: imgtool file handle
	address: physical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	src: pointer to 256-byte source buffer
*/
static int write_sector_physical(STREAM *file_handle, const ti99_phys_sec_address *address, const ti99_geometry *geometry, const void *src)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, phys_address_to_offset(address, geometry), SEEK_SET);
	if (reply)
		return 1;
	/* write it */
	reply = stream_write(file_handle, src, 256);
	if (reply != 256)
		return 1;

	return 0;
}

/*
	Convert logical sector address to physical sector address
*/
static void log_address_to_phys_address(int secnum, const ti99_geometry *geometry, ti99_phys_sec_address *address)
{
	address->sector = secnum % geometry->secspertrack;
	secnum /= geometry->secspertrack;
	address->cylinder = secnum % geometry->tracksperside;
	address->side = secnum / geometry->tracksperside;
}

/*
	Read one sector from a disk image

	file_handle: imgtool file handle
	secnum: logical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	dest: pointer to 256-byte destination buffer
*/
static int read_sector_logical(STREAM *file_handle, int secnum, const ti99_geometry *geometry, void *dest)
{
	ti99_phys_sec_address address;


	log_address_to_phys_address(secnum, geometry, &address);

	return read_sector_physical(file_handle, &address, geometry, dest);
}

/*
	Write one sector to a disk image

	file_handle: imgtool file handle
	secnum: logical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	src: pointer to 256-byte source buffer
*/
static int write_sector_logical(STREAM *file_handle, int secnum, const ti99_geometry *geometry, const void *src)
{
	ti99_phys_sec_address address;


	log_address_to_phys_address(secnum, geometry, &address);

	return write_sector_physical(file_handle, &address, geometry, src);
}

/*
	read sector 0 with no geometry information (used when opening image)

	file_handle: imgtool file handle
	dest: pointer to 256-byte destination buffer
*/
static int read_sector0_no_geometry(STREAM *file_handle, ti99_sc0 *dest)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, 0, SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_read(file_handle, dest, 256);
	if (reply != 256)
		return 1;

	return 0;
}

/*
	Find the catalog entry and fdr record associated with a file name

	image: ti99_image image record
	fname: name of the file to search
	fdr: pointer to buffer where the fdr record should be stored (may be NULL)
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int find_fdr(ti99_image *image, char fname[10], ti99_fdr *fdr, int *catalog_index)
{
	int fdr_secnum;
	int i;


	i = 0;
	if ((image->catalog[0].fdr_secnum == 0) && (image->catalog[1].fdr_secnum != 0))
		i = 1;	/* skip empty entry 0 (it must be a non-listable catalog) */

	for (; (i<128) && ((fdr_secnum = image->catalog[i].fdr_secnum) != 0); i++)
	{
		if (memcmp(image->catalog[i].fname, fname, 10) == 0)
		{	/* file found */
			if (catalog_index)
				*catalog_index = i;

			if (fdr)
				if (read_sector_logical(image->file_handle, fdr_secnum, & image->geometry, fdr))
					return IMGTOOLERR_READERROR;

			return 0;
		}
	}

	return IMGTOOLERR_FILENOTFOUND;
}

/*
	Allocate one sector on disk, for use as a fdr record

	image: ti99_image image record
	fdr_secnum: on output, logical address of a free sector
*/
static int alloc_fdr_sector(ti99_image *image, int *fdr_secnum)
{
	int totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	int i;


	for (i=0; i<totsecs; i++)
	{
		if (! (image->sec0.abm[i >> 3] & (1 << (i & 7))))
		{
			*fdr_secnum = i;
			image->sec0.abm[i >> 3] |= 1 << (i & 7);

			return 0;
		}
	}

	return IMGTOOLERR_NOSPACE;
}

/*
	Extend a file with nb_alloc_sectors extra sectors

	image: ti99_image image record
	fdr: file fdr record
	nb_alloc_sectors: number of sectors to allocate
*/
static int alloc_file_sectors(ti99_image *image, ti99_fdr *fdr, int nb_alloc_sectors)
{
	int totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	int free_sectors;
	int secsused;
	int i;
	int lnks_index;
	int cur_sec, last_sec, p_last_sec;
	int cur_block_len, cur_block_start;
	int first_best_block_len, first_best_block_start = 0;
	int second_best_block_len, second_best_block_start = 0;
	int search_start;

	/* compute free space */
	free_sectors = 0;
	for (i=0; i<totsecs; i++)
	{
		if (! (image->sec0.abm[i >> 3] & (1 << (i & 7))))
			free_sectors++;
	}

	/* check we have enough free space */
	if (free_sectors < nb_alloc_sectors)
		return IMGTOOLERR_NOSPACE;

	/* current number of data sectors in file */
	secsused = get_fdr_secsused(fdr);

	if (secsused == 0)
	{	/* links array must be empty */
		lnks_index = 0;
	}
	else
	{	/* try to extend last block */
		last_sec = -1;
		for (lnks_index=0; lnks_index<76; lnks_index++)
		{
			p_last_sec = last_sec;
			last_sec = (fdr->lnks[lnks_index][2] << 4) | (fdr->lnks[lnks_index][1] >> 4);
			if (last_sec >= (secsused-1))
				break;
		}
		if (lnks_index == 76)
			/* that sucks */
			return IMGTOOLERR_CORRUPTIMAGE;

		if (last_sec > (secsused-1))
		{	/* some extra space has already been allocated */
			cur_block_len = last_sec - (secsused-1);
			if (cur_block_len > nb_alloc_sectors)
				cur_block_len = nb_alloc_sectors;

			secsused += cur_block_len;
			set_fdr_secsused(fdr, secsused);
			nb_alloc_sectors -= cur_block_len;
			if (! nb_alloc_sectors)
				return 0;	/* done */
		}

		/* block base */
		cur_sec = ((fdr->lnks[lnks_index][1] & 0xf) << 8) | fdr->lnks[lnks_index][0];
		/* point past block end */
		cur_sec += last_sec-p_last_sec;
		/* count free sectors after last block */
		cur_block_len = 0;
		for (i=cur_sec; (! (image->sec0.abm[i >> 3] & (1 << (i & 7)))) && (cur_block_len < nb_alloc_sectors) && (i < totsecs); i++)
			cur_block_len++;
		if (cur_block_len)
		{	/* extend last block */
			secsused += cur_block_len;
			set_fdr_secsused(fdr, secsused);
			last_sec += cur_block_len;
			nb_alloc_sectors -= cur_block_len;
			fdr->lnks[lnks_index][1] = (fdr->lnks[lnks_index][1] & 0x0f) | (last_sec << 4);
			fdr->lnks[lnks_index][2] = last_sec >> 4;
			for (i=0; i<cur_block_len; i++)
				image->sec0.abm[(i+cur_sec) >> 3] |= 1 << ((i+cur_sec) & 7);
			if (! nb_alloc_sectors)
				return 0;	/* done */
		}
		lnks_index++;
		if (lnks_index == 76)
			/* that sucks */
			return IMGTOOLERR_NOSPACE;
	}

	search_start = image->data_offset;	/* initially, search for free space only in data space */
	while (nb_alloc_sectors)
	{
		/* find smallest data block at least nb_alloc_sectors in lenght, and largest data block less than nb_alloc_sectors in lenght */
		first_best_block_len = INT_MAX;
		second_best_block_len = 0;
		for (i=search_start; i<totsecs; i++)
		{
			if (! (image->sec0.abm[i >> 3] & (1 << (i & 7))))
			{	/* found one free block */
				/* compute its lenght */
				cur_block_start = i;
				cur_block_len = 0;
				while ((i<totsecs) && (! (image->sec0.abm[i >> 3] & (1 << (i & 7)))))
				{
					cur_block_len++;
					i++;
				}
				/* compare to previous best and second-best blocks */
				if ((cur_block_len < first_best_block_len) && (cur_block_len >= nb_alloc_sectors))
				{
					first_best_block_start = cur_block_start;
					first_best_block_len = cur_block_len;
					if (cur_block_len == nb_alloc_sectors)
						/* no need to search further */
						break;
				}
				else if ((cur_block_len > second_best_block_len) && (cur_block_len < nb_alloc_sectors))
				{
					second_best_block_start = cur_block_start;
					second_best_block_len = cur_block_len;
				}
			}
		}

		if (first_best_block_len != INT_MAX)
		{	/* found one contiguous block which can hold it all */
			secsused += nb_alloc_sectors;
			set_fdr_secsused(fdr, secsused);

			fdr->lnks[lnks_index][0] = first_best_block_start;
			fdr->lnks[lnks_index][1] = ((first_best_block_start >> 8) & 0x0f) | ((secsused-1) << 4);
			fdr->lnks[lnks_index][2] = (secsused-1) >> 4;
			for (i=0; i<nb_alloc_sectors; i++)
				image->sec0.abm[(i+first_best_block_start) >> 3] |= 1 << ((i+first_best_block_start) & 7);

			nb_alloc_sectors = 0;
		}
		else if (second_best_block_len != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			secsused += second_best_block_len;
			set_fdr_secsused(fdr, secsused);

			fdr->lnks[lnks_index][0] = second_best_block_start;
			fdr->lnks[lnks_index][1] = ((second_best_block_start >> 8) & 0x0f) | ((secsused-1) << 4);
			fdr->lnks[lnks_index][2] = (secsused-1) >> 4;
			for (i=0; i<second_best_block_len; i++)
				image->sec0.abm[(i+second_best_block_start) >> 3] |= 1 << ((i+second_best_block_start) & 7);

			nb_alloc_sectors -= second_best_block_len;

			lnks_index++;
			if (lnks_index == 76)
				/* that sucks */
				return IMGTOOLERR_NOSPACE;
		}
		else if (search_start != 0)
		{	/* we did not find any free sector in the data section of the disk */
			search_start = 0;	/* time to fall back to fdr space */
		}
		else
			return IMGTOOLERR_NOSPACE;	/* This should never happen, as we pre-check that there is enough free space */
	}

	return 0;
}

/*
	Allocate a new (empty) file
*/
static int new_file(ti99_image *image, char fname[10], int *out_fdr_secnum/*, ti99_fdr *fdr,*/)
{
	int fdr_secnum;
	int catalog_index, i;
	int reply = 0;


	/* find insertion point in catalog */
	i = 0;
	if ((image->catalog[0].fdr_secnum == 0) && (image->catalog[1].fdr_secnum != 0))
		i = 1;	/* skip empty entry 0 (it must be a non-listable catalog) */

	for (; (i<128) && ((fdr_secnum = image->catalog[i].fdr_secnum) != 0) && ((reply = memcmp(image->catalog[i].fname, fname, 10)) < 0); i++)
		;

	if (i == 128)
		/* catalog is full */
		return IMGTOOLERR_NOSPACE;
	else if (fdr_secnum && (reply == 0))
		/* file already exists */
		return IMGTOOLERR_BADFILENAME;
	else
	{
		catalog_index = i;
		reply = alloc_fdr_sector(image, &fdr_secnum);
		if (reply)
			return reply;

		/* look for first free entry in catalog */
		for (i=catalog_index; image->catalog[i].fdr_secnum != 0; i++)
			;

		if (i == 128)
			/* catalog is full */
				return IMGTOOLERR_NOSPACE;

		/* shift catalog entries until the insertion point */
		for (/*i=127*/; i>catalog_index; i--)
			image->catalog[i] = image->catalog[i-1];

		/* write new catalog entry */
		image->catalog[catalog_index].fdr_secnum = fdr_secnum;
		memcpy(image->catalog[catalog_index].fname, fname, 10);
		if (out_fdr_secnum)
			*out_fdr_secnum = fdr_secnum;
	}

	return 0;
}

/*
	Compare two (possibly empty) catalog entry for qsort
*/
static int qsort_catalog_compare(const void *p1, const void *p2)
{
	const catalog_entry *entry1 = p1;
	const catalog_entry *entry2 = p2;

	if ((entry1->fdr_secnum == 0) && (entry2->fdr_secnum == 0))
		return 0;
	else if (entry1->fdr_secnum == 0)
		return +1;
	else if (entry2->fdr_secnum == 0)
		return -1;
	else
		return memcmp(entry1->fname, entry2->fname, 10);
}

/*
	Open a file as a ti99_image.
*/
static int ti99_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	ti99_image *image;
	int reply;
	int totsecs;
	UINT8 buf[256];
	ti99_fdr fdr;
	int i;


	image = malloc(sizeof(ti99_image));
	* (ti99_image **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(ti99_image));
	image->base.module = mod;
	image->file_handle = f;
	reply = read_sector0_no_geometry(f, & image->sec0);
	if (reply)
	{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
	}

	totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	if (image->sec0.tracksperside == 0)
		/* Some images are like this, because the TI controller always assumes 40. */
		image->sec0.tracksperside = 40;	// The most usual value
	if (image->sec0.sides == 0)
		/* Some images are like this, because the TI controller always assumes
		tracks beyond 40 are on side 2. */
		image->sec0.sides = totsecs / (image->sec0.secspertrack * image->sec0.tracksperside);
	if (((image->sec0.secspertrack * image->sec0.tracksperside * image->sec0.sides) != totsecs)
		|| (totsecs < 2) || (totsecs > 1600) || memcmp(image->sec0.id, "DSK", 3)
		|| (stream_size(f) != totsecs*256))
	{
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	image->geometry.secspertrack = image->sec0.secspertrack;
	image->geometry.tracksperside = image->sec0.tracksperside;
	image->geometry.sides = image->sec0.sides;

	/* read catalog */
	reply = read_sector_logical(f, 1, & image->geometry, buf);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_READERROR;
	}

	for (i=0; i<128; i++)
		image->catalog[i].fdr_secnum = (buf[i*2] << 8) | buf[i*2+1];

	for (i=0; i<128; i++)
	{
		if (image->catalog[i].fdr_secnum >= totsecs)
		{
			free(image);
			*outimg = NULL;
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (image->catalog[i].fdr_secnum)
		{
			reply = read_sector_logical(f, image->catalog[i].fdr_secnum, & image->geometry, &fdr);
			if (reply)
			{
				free(image);
				*outimg = NULL;
				return IMGTOOLERR_READERROR;
			}
			memcpy(image->catalog[i].fname, fdr.name, 10);
		}
	}

	/* check catalog */
	i = 0;
	if ((image->catalog[0].fdr_secnum == 0) && (image->catalog[1].fdr_secnum != 0))
		i = 1;	/* skip empty entry 0 (it must be a non-listable catalog) */

	for (; i<127; i++)
	{
		if (((! image->catalog[i].fdr_secnum) && image->catalog[i+1].fdr_secnum)
			|| ((image->catalog[i].fdr_secnum && image->catalog[i+1].fdr_secnum) && (memcmp(image->catalog[i].fname, image->catalog[i+1].fname, 10) >= 0)))
		{
			/* we should repair the catalog instead */
			#if 0
				free(image);
				*outimg = NULL;
				return IMGTOOLERR_CORRUPTIMAGE;
			#else
				qsort(image->catalog, sizeof(image->catalog)/sizeof(image->catalog[0]),
						sizeof(image->catalog[0]), qsort_catalog_compare);
				break;
			#endif
		}
	}

	image->data_offset = 32+2;

	return 0;
}

/*
	close a ti99_image
*/
static void ti99_image_exit(IMAGE *img)
{
	ti99_image *image = (ti99_image *) img;

	stream_close(image->file_handle);
	free(image);
}

/*
	get basic information on a ti99_image

	Currently returns the volume name
*/
static void ti99_image_info(IMAGE *img, char *string, const int len)
{
	ti99_image *image = (ti99_image *) img;
	char vol_name[11];

	memcpy(vol_name, image->sec0.name, 10);
	vol_name[10] = '\0';

	snprintf(string, len, "%s", vol_name);
}

/*
	Open the disk catalog for enumeration 
*/
static int ti99_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	ti99_image *image = (ti99_image*) img;
	ti99_iterator *iter;

	iter = malloc(sizeof(ti99_iterator));
	*((ti99_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->image = image;
	iter->index = 0;

	if ((image->catalog[0].fdr_secnum == 0) && (image->catalog[1].fdr_secnum != 0))
		iter->index = 1;	/* skip empty entry 0 (it must be a non-listable catalog) */

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int ti99_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	ti99_iterator *iter = (ti99_iterator*) enumeration;
	ti99_fdr fdr;
	int reply;
	int fdr_secnum;
	int totsecs = (iter->image->sec0.totsecsMSB << 8) | iter->image->sec0.totsecsLSB;

	ent->corrupt = 0;
	ent->eof = 0;

	if ((iter->index == 128) || ((fdr_secnum = iter->image->catalog[iter->index].fdr_secnum) == 0))
	{
		ent->eof = 1;
	}
	else
	{
		if (fdr_secnum >= totsecs)
		{
			ent->corrupt = 1;
		}
		else
		{
			reply = read_sector_logical(iter->image->file_handle, fdr_secnum, & iter->image->geometry, & fdr);
			if (reply)
				return IMGTOOLERR_READERROR;
			memcpy(ent->fname, fdr.name, (ent->fname_len < 11) ? (ent->fname_len-1) : 10);
			ent->fname[(ent->fname_len < 11) ? (ent->fname_len-1) : 10] = '\0';

			/* parse flags */
			if (fdr.flags & fdr99_f_program)
				snprintf(ent->attr, ent->attr_len, "PGM%s",
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			else
				snprintf(ent->attr, ent->attr_len, "%c/%c %d%s%s",
							(fdr.flags & fdr99_f_int) ? 'I' : 'D',
							(fdr.flags & fdr99_f_var) ? 'V' : 'F',
							fdr.reclen,
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			/* len in sectors */
			ent->filesize = get_fdr_secsused(&fdr);
		}

		iter->index++;
	}

	return 0;
}

/*
	Free enumerator
*/
static void ti99_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in 256-byte ADUs)
*/
static size_t ti99_image_freespace(IMAGE *img)
{
	ti99_image *image = (ti99_image*) img;
	int totsecs = image->sec0.totsecsMSB << 8 | image->sec0.totsecsLSB;
	int sec;
	size_t blocksfree = 0;

	blocksfree = 0;
	for (sec=2; sec<totsecs; sec++)
	{
		if (! (image->sec0.abm[sec >> 3] & (1 << (sec & 7))))
			blocksfree++;
	}

	return blocksfree;
}

/*
	Extract a file from a ti99_image.  The file is saved in tifile format.
*/
static int ti99_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	ti99_image *image = (ti99_image*) img;
	int totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	char ti_fname[10];
	ti99_fdr fdr;
	tifile_header dst_header;
	int i, lnks_index;
	int cur_sec, last_sec;
	int secsused;
	UINT8 buf[256];
	int reply;

	str_to_fname(ti_fname, fname);

	reply = find_fdr(image, ti_fname, &fdr, NULL);
	if (reply)
		return reply;

	dst_header.tifiles[0] = '\7';
	dst_header.tifiles[1] = 'T';
	dst_header.tifiles[2] = 'I';
	dst_header.tifiles[3] = 'F';
	dst_header.tifiles[4] = 'I';
	dst_header.tifiles[5] = 'L';
	dst_header.tifiles[6] = 'E';
	dst_header.tifiles[7] = 'S';
	dst_header.secsused_MSB = fdr.secsused_MSB;
	dst_header.secsused_LSB = fdr.secsused_LSB;
	dst_header.flags = fdr.flags;
	dst_header.recspersec = fdr.recspersec;
	dst_header.eof = fdr.eof;
	dst_header.reclen = fdr.reclen;
	dst_header.fixrecs_MSB = fdr.fixrecs_MSB;
	dst_header.fixrecs_LSB = fdr.fixrecs_LSB;
	for (i=0; i<(128-14); i++)
		dst_header.res[i] = 0;

	if (stream_write(destf, & dst_header, 128) != 128)
		return IMGTOOLERR_WRITEERROR;


	secsused = get_fdr_secsused(&fdr);

	i = 0;			/* file logical sector #0 */
	lnks_index = 0;	/* start of file block table */
	while (i<secsused)
	{
		if (lnks_index == 76)
			return IMGTOOLERR_CORRUPTIMAGE;

		/* read curent file block table entry */
		cur_sec = ((fdr.lnks[lnks_index][1] & 0xf) << 8) | fdr.lnks[lnks_index][0];
		last_sec = (fdr.lnks[lnks_index][2] << 4) | (fdr.lnks[lnks_index][1] >> 4);

		/* copy current file block */
		while ((i<=last_sec) && (i<secsused))
		{
			if (cur_sec >= totsecs)
				return IMGTOOLERR_CORRUPTIMAGE;

			if (read_sector_logical(image->file_handle, cur_sec, & image->geometry, buf))
				return IMGTOOLERR_READERROR;

			if (stream_write(destf, buf, 256) != 256)
				return IMGTOOLERR_WRITEERROR;

			i++;
			cur_sec++;
		}

		/* next entry in file block table */
		lnks_index++;
	}

	return 0;
}

/*
	Add a file to a ti99_image.  The file must be in tifile format.
*/
static int ti99_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *in_options)
{
	ti99_image *image = (ti99_image*) img;
	int totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	char ti_fname[10];
	ti99_fdr fdr;
	tifile_header src_header;
	int i, lnks_index;
	int cur_sec, last_sec;
	int secsused;
	UINT8 buf[256];
	int reply;
	int fdr_secnum;

	str_to_fname(ti_fname, fname);

	reply = find_fdr(image, ti_fname, &fdr, NULL);
	if (reply == 0)
	{	/* file already exists: causes an error for now */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if (reply != IMGTOOLERR_FILENOTFOUND)
	{
		return reply;
	}

	if (stream_read(sourcef, & src_header, 128) != 128)
		return IMGTOOLERR_READERROR;

	/* create new file */
	reply = new_file(image, ti_fname, &fdr_secnum);
	if (reply)
		return reply;

	memcpy(fdr.name, ti_fname, 10);
	fdr.res10[1] = fdr.res10[0] = 0;
	fdr.flags = src_header.flags;
	fdr.recspersec = src_header.recspersec;
	fdr.secsused_MSB = /*src_header.secsused_MSB*/0;
	fdr.secsused_LSB = /*src_header.secsused_LSB*/0;
	fdr.eof = src_header.eof;
	fdr.reclen = src_header.reclen;
	fdr.fixrecs_LSB = src_header.fixrecs_LSB;
	fdr.fixrecs_MSB = src_header.fixrecs_MSB;
	for (i=0; i<8; i++)
		fdr.res20[i] = 0;
	for (i=0; i<76; i++)
		fdr.lnks[i][2] = fdr.lnks[i][1] = fdr.lnks[i][0] = 0;

	/* alloc data sectors */
	secsused = (src_header.secsused_MSB << 8) | src_header.secsused_LSB;
	reply = alloc_file_sectors(image, &fdr, secsused);
	if (reply)
		return reply;

	/* copy data */
	i = 0;
	lnks_index = 0;
	while (i<secsused)
	{
		if (lnks_index == 76)
			return IMGTOOLERR_CORRUPTIMAGE;

		cur_sec = ((fdr.lnks[lnks_index][1] & 0xf) << 8) | fdr.lnks[lnks_index][0];
		last_sec = (fdr.lnks[lnks_index][2] << 4) | (fdr.lnks[lnks_index][1] >> 4);

		while ((i<secsused) && (i<=last_sec))
		{
			if (cur_sec >= totsecs)
				return IMGTOOLERR_CORRUPTIMAGE;

			if (stream_read(sourcef, buf, 256) != 256)
				return IMGTOOLERR_READERROR;

			if (write_sector_logical(image->file_handle, cur_sec, & image->geometry, buf))
				return IMGTOOLERR_WRITEERROR;

			i++;
			cur_sec++;
		}

		lnks_index++;
	}

	/* write fdr */
	if (write_sector_logical(image->file_handle, fdr_secnum, & image->geometry, &fdr))
		return IMGTOOLERR_WRITEERROR;

	/* update catalog */
	for (i=0; i<128; i++)
	{
		buf[2*i] = image->catalog[i].fdr_secnum >> 8;
		buf[2*i+1] = image->catalog[i].fdr_secnum & 0xff;
	}
	if (write_sector_logical(image->file_handle, 1, & image->geometry, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_sector_logical(image->file_handle, 0, & image->geometry, &image->sec0))
		return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Delete a file from a ti99_image.
*/
static int ti99_image_deletefile(IMAGE *img, const char *fname)
{
	ti99_image *image = (ti99_image*) img;
	int totsecs = (image->sec0.totsecsMSB << 8) | image->sec0.totsecsLSB;
	char ti_fname[10];
	ti99_fdr fdr;
	int i, lnks_index;
	int cur_sec, last_sec;
	int secsused;
	int catalog_index;
	int reply;
	UINT8 buf[256];

	str_to_fname(ti_fname, fname);

	reply = find_fdr(image, ti_fname, &fdr, &catalog_index);
	if (reply)
		return reply;

	/* free data sectors */
	secsused = get_fdr_secsused(&fdr);

	i = 0;
	lnks_index = 0;
	while (i<secsused)
	{
		if (lnks_index == 76)
			return IMGTOOLERR_CORRUPTIMAGE;

		cur_sec = ((fdr.lnks[lnks_index][1] & 0xf) << 8) | fdr.lnks[lnks_index][0];
		last_sec = (fdr.lnks[lnks_index][2] << 4) | (fdr.lnks[lnks_index][1] >> 4);

		while ((i<secsused) && (i<=last_sec))
		{
			if (cur_sec >= totsecs)
				return IMGTOOLERR_CORRUPTIMAGE;

			image->sec0.abm[cur_sec >> 3] &= ~ (1 << (cur_sec & 7));

			i++;
			cur_sec++;
		}

		lnks_index++;
	}

	/* free fdr sector */
	image->sec0.abm[image->catalog[catalog_index].fdr_secnum >> 3] &= ~ (1 << (image->catalog[catalog_index].fdr_secnum & 7));

	/* delete catalog entry */
	for (i=catalog_index; i<127; i++)
		image->catalog[i] = image->catalog[i+1];
	image->catalog[127].fdr_secnum = 0;

	/* update catalog */
	for (i=0; i<128; i++)
	{
		buf[2*i] = image->catalog[i].fdr_secnum >> 8;
		buf[2*i+1] = image->catalog[i].fdr_secnum & 0xff;
	}
	if (write_sector_logical(image->file_handle, 1, & image->geometry, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_sector_logical(image->file_handle, 0, & image->geometry, &image->sec0))
		return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Create a blank ti99_image.
*/
static int ti99_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	const char *volname;
	int density;
	ti99_geometry geometry;

	int totsecs;

	ti99_sc0 sec0;
	UINT8 empty_sec[256];

	int i;


	(void) mod;

	/* read options */
	volname = in_options[ti99_createopts_volname].s;
	density = in_options[ti99_createopts_density].i;
	geometry.sides = in_options[ti99_createopts_sides].i;
	geometry.tracksperside = in_options[ti99_createopts_tracks].i;
	geometry.secspertrack = in_options[ti99_createopts_sectors].i;

	totsecs = geometry.secspertrack * geometry.tracksperside * geometry.sides;

	/* FM disks can hold fewer sector per track than MFM disks */
	if ((density == 1) && (geometry.secspertrack > 9))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* check total disk size */
	if (totsecs < 2)
		return IMGTOOLERR_PARAMTOOSMALL;
	if (totsecs > 1600)
		return IMGTOOLERR_PARAMTOOLARGE;

	/* initialize sector 0 */

	/* set volume name */
	str_to_fname(sec0.name, volname);

	/* set every header field */
	sec0.totsecsMSB = totsecs >> 8;
	sec0.totsecsLSB = totsecs & 0xff;
	sec0.secspertrack = geometry.secspertrack;
	sec0.id[0] = 'D';
	sec0.id[1] = 'S';
	sec0.id[2] = 'K';
	sec0.id[3] = ' ';
	sec0.tracksperside = geometry.tracksperside;
	sec0.sides = geometry.sides;
	sec0.density = density;

	/* clear reserved field */
	for (i=0; i<36; i++)
		sec0.res[i] = 0;

	/* clear bitmap */
	for (i=0; i < (totsecs>>3); i++)
		sec0.abm[i] = 0;

	if (totsecs & 7)
	{
		sec0.abm[i] = 0xff << (totsecs & 7);
		i++;
	}

	for (; i < 200; i++)
		sec0.abm[i] = 0xff;

	/* mark first 2 sectors (header bitmap, and catalog) as used */
	sec0.abm[0] |= 3;

	/* write sector 0 */
	if (write_sector_logical(f, 0, & geometry, &sec0))
		return IMGTOOLERR_WRITEERROR;


	/* now clear every other sector, including catalog */
	memset(empty_sec, 0, 256);

	for (i=1; i<totsecs; i++)
		if (write_sector_logical(f, i, & geometry, empty_sec))
			return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Read one sector from a ti99_image.
*/
static int ti99_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

/*
	Write one sector to a ti99_image.
*/
static int ti99_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}
