/*
	Handlers for ti99 disk images

	Disk images are in v9t9/MESS format.  Files are extracted in TIFILES format.

	Raphael Nabet, 2002-2003

	TODO:
	- finish and test hd support
*/

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "harddisk.h"
#include "imghd.h"


/*
	DSK disk structure (floppy disk, up to 1600 AUs and 16MBytes per volume):

	The disk sector size is normally 256 bytes.  If the size is not 256 bytes,
	the sectors should be grouped or split into 256-byte physical records.  It
	is unknown whether any FDR supports sectors of any size other than 256
	bytes.  It sounds likely that the SCSI DSR does, but I don't know is it
	supports the TI DSK format.

	The disk is allocated in allocation units (AUs).  Each AU represents one
	or several 256-byte physical records; for some reason, the number of
	physical records per AU is always a power of 2.  AUs usually represent one
	physical record each, but since we cannot have more than 1600 AUs, we need
	to have several physical records per AU in disks larger than 400kbytes.
	Note that only one disk controller (Myarc's HFDC) implements AUs that span
	multiple physical records.

	Sector 0: Volume Information Block (VIB): see below
	Sector 1: root File Descriptor Index Record (FDIR): array of 0 through 128
		words, sector (physical record?) address of the fdr record for each
		file.  Sorted by ascending file name.  Note that, while we should be
		prepared to read images images with 128 entries, we should write no
		more than 127 for compatibility with existing FDRs.
	Remaining AUs are used for fdr and data (and subdirectory fdir in the case
	of hfdc).  There is one FDIR record per directory; the FDIR points to the
	FDR for each file in the directory.  The FDR (File Descriptor Record)
	contains the file information (name, format, lenght, pointers to data
	sectors/AUs, etc).
*/

/*
	WIN disk structure (hard disk, up to 65535 AUs and 256MBytes per volume):

	The disk sector size is 256 bytes on HFDC, 512 bytes on SCSI.  512-byte
	sectors are split into 256-byte physical records.

	The disk is allocated in allocation units (AUs).  Each AU represents one
	or several sectors.  AUs usually represent one sector each, but since we
	cannot have more than 65535 AUs, we need to have several physical records
	per AU in disks larger than 16 (HFDC) or 32 (SCSI) Mbytes.

	Sector 0: Volume Information Block (VIB): see below
	Sector 1-n: Volume bitmap
	Remaining physical records are used for ddr, fdir, fdr and data.  Each
	directory (except the root directory, which uses the VIB as a DDR) has a
	DDR, that points to an FDIR.  The FDIR points to the FDR for each file in
	the directory, and to the parent DDR as well.  Each file has one FDR (File
	Descriptor Record) descriptor which provides the file information (name,
	format, lenght, pointers to data sectors/AUs, etc).  When a file has more
	than 76 fragments, we create offspring FDRs that hold additional fragments.
*/

/* Since string length is encoded with a byte, the maximum length of a string
is 255.  Since we need to add a device name (1 char minimum, typically 4 chars)
and a '.' separator, we chose a practical value of 250 (though few applications
will accept paths of such lenght). */
#define MAX_PATH_LEN 253
#define MAX_SAFE_PATH_LEN 250

#define MAX_DIR_LEVEL 125	/* We need to put a recursion limit to avoid endless recursion hazard */


#if 0
#pragma mark MISCELLANEOUS UTILITIES
#endif

/*
	Miscellaneous utilities that are used to handle TI data types
*/

typedef struct UINT16BE
{
	UINT8 bytes[2];
} UINT16BE;

typedef struct UINT16LE
{
	UINT8 bytes[2];
} UINT16LE;

INLINE UINT16 get_UINT16BE(UINT16BE word)
{
	return (word.bytes[0] << 8) | word.bytes[1];
}

INLINE void set_UINT16BE(UINT16BE *word, UINT16 data)
{
	word->bytes[0] = (data >> 8) & 0xff;
	word->bytes[1] = data & 0xff;
}

INLINE UINT16 get_UINT16LE(UINT16LE word)
{
	return word.bytes[0] | (word.bytes[1] << 8);
}

INLINE void set_UINT16LE(UINT16LE *word, UINT16 data)
{
	word->bytes[0] = data & 0xff;
	word->bytes[1] = (data >> 8) & 0xff;
}

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
	Convert a 10-character file name to a C string (removing trailing spaces if necessary)
*/
static void fname_to_str(char *dst, const char src[10], int n)
{
	int i;
	int last_nonspace;


	/* copy 10 characters at most */
	if (--n > 10)
		n = 10;

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
	Check a 10-character file name.  Return non-zero if the file name is invalid.
*/
static int check_fname(const char fname[10])
{
	int i;
	int space_found_flag;


	/* check and copy file name */
	space_found_flag = FALSE;
	for (i=0; i<10; i++)
	{
		switch (fname[i])
		{
		case '.':
		case '\0':
			/* illegal characters */
			return 1;
		case ' ':
			/* illegal in a file name, but file names shorter than 10
			characters are padded with spaces */
			space_found_flag = TRUE;
			break;
		default:
			/* all other characters are legal (though non-ASCII characters,
			control characters and even lower-case characters are not
			recommended) */
			if (space_found_flag)
				return 1;
			break;
		}
	}

	return 0;
}

/*
	Check a file path.  Return non-zero if the file path is invalid.
*/
static int check_fpath(const char *fpath)
{
	const char *element_start, *element_end;
	int element_len;


	/* first check that the path is not too long and that there is no space
	character */
	if ((strlen(fpath) > MAX_PATH_LEN) || strchr(fpath, ' '))
		return 1;

	/* check that each path element has an acceptable lenght */
	element_start = fpath;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, '.');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		if ((element_len > 10) || (element_len == 0))
			return 1;

		/* iterate */
		if (element_end)
			element_start = element_end+1;
		else
			element_start = NULL;
	}
	while (element_start);

	return 0;
}

#if 0
#pragma mark -
#pragma mark LEVEL 1 DISK ROUTINES
#endif

/*
	Level 1 deals with accessing the disk hardware (in our case, the raw disk
	image).

	Higher level routines will only know the disk as a succession of
	256-byte-long physical records addressed by a linear address.
*/

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
typedef struct ti99_sector_address
{
	int sector;
	int cylinder;
	int side;
} ti99_sector_address;

/*
	Time stamp (used in fdr)
*/
typedef struct ti99_date_time
{
	UINT8 time_MSB, time_LSB;/* 0-4: hour, 5-10: minutes, 11-15: seconds/2 */
	UINT8 date_MSB, date_LSB;/* 0-6: year, 7-10: month, 11-15: day */
} ti99_date_time;

/*
	Subdirectory descriptor (HFDC only)

	The HFDC supports up to 3 subdirectories.
*/
typedef struct ti99_subdir
{
	char name[10];			/* subdirectory name (10 characters, pad with spaces) */
	UINT16BE fdir_physrec;	/* pointer to subdirectory fdir record */
} ti99_subdir;

/*
	DSK VIB record

	Most fields in this record are only revelant to level 2 routines, but level
	1 routines need the disk geometry information extracted from the VIB.
*/
typedef struct dsk_vib
{
	char name[10];			/* disk volume name (10 characters, pad with spaces) */
	UINT16BE totphysrecs;	/* TI says it should be the total number of AUs, */
								/* but, for some reason, the only DSR that */
								/* supports disks >400kbytes (Myarc HFDC) stuck */
								/* with the total number of sectors. */
								/* (usually 360, 720, 640, 1280 or 1440) */
	UINT8 secspertrack;		/* sectors per track (usually 9 (FM SD), */
								/* 16 or 18 (MFM DD), or 36 (MFM HD) */
	UINT8 id[3];			/* 'DSK' */
	UINT8 protection;		/* 'P' if disk is protected, ' ' otherwise. */
	UINT8 tracksperside;	/* tracks per side (usually 40) */
	UINT8 sides;			/* sides (1 or 2) */
	UINT8 density;			/* density: 1 (FM SD), 2 (MFM DD), or 3 (MFM HD) */
	ti99_subdir subdir[3];	/* descriptor for up to 3 subdirectories (HFDC only) */
								/* reserved by TI */
	UINT8 abm[200];			/* allocation bitmap: there is one bit per AU. */
								/* A binary 1 in a bit position indicates that */
								/* the allocation unit associated with that */
								/* bit has been allocated. */
								/* Bits corresponding to system reserved AUs, */
								/* non-existant AUs, and bad AUs, are set to one, too. */
								/* (AU 0 is associated to LSBit of byte 0, */
								/* AU 7 to MSBit of byte 0, AU 8 to LSBit */
								/* of byte 1, etc.) */
} dsk_vib;

typedef enum
{
	if_mess,
	if_v9t9,
	if_pc99_fm,
	if_pc99_mfm,
	if_harddisk
} ti99_img_format;

/*
	level-1 disk image descriptor
*/
typedef struct ti99_lvl1_imgref
{
	ti99_img_format img_format;	/* tells the image format */
	STREAM *file_handle;		/* imgtool file handle */
	void *harddisk_handle;		/* MAME harddisk handle (harddisk format) */
	ti99_geometry geometry;		/* geometry */
	UINT32 *pc99_data_offset_array;	/* offset for each sector (pc99 format) */
} ti99_lvl1_imgref;

static int read_absolute_physrec(ti99_lvl1_imgref *l1_img, int physrec, void *dest);

/*
	calculate CRC for data address marks or sector data

	crc: current CRC value to be updated (initialize to 0xffff)
	value: new byte of data to update the CRC with
*/
static void calc_crc(UINT16 *crc, UINT8 value)
{
	UINT8 l, h;

	l = value ^ (*crc >> 8);
	*crc = (*crc & 0xff) | (l << 8);
	l >>= 4;
	l ^= (*crc >> 8);
	*crc <<= 8;
	*crc = (*crc & 0xff00) | l;
	l = (l << 4) | (l >> 4);
	h = l;
	l = (l << 2) | (l >> 6);
	l &= 0x1f;
	*crc = *crc ^ (l << 8);
	l = h & 0xf0;
	*crc = *crc ^ (l << 8);
	l = (h << 1) | (h >> 7);
	l &= 0xe0;
	*crc = *crc ^ l;
}

/*
	Parse a PC99 disk image

	file_handle: imgtool file handle
	fm_format: if true, the image is in FM format, otherwise it is in MFM
		format
	pass: 0 for first pass, 1 for second pass
	vib: buffer where the vib should be stored (first pass)
	geometry: disk image geometry (second pass)
	data_offset_array: array of data offset to generate (second pass)
*/
#define MAX_SECTOR_LEN 2048
#define DATA_OFFSET_NONE 0xffffffff
static int parse_pc99_image(STREAM *file_handle, int fm_format, int pass, dsk_vib *vib, const ti99_geometry *geometry, UINT32 *data_offset_array)
{
	int bytes_read;
	UINT8 c;
	UINT8 cylinder, head, sector;
	int seclen;
	UINT8 crc1, crc2;
	UINT16 crc;
	long save_pos;
	long data_offset;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;


	if (pass == 1)
	{
		/* initialize offset map */
		for (cylinder = 0; cylinder < geometry->tracksperside; cylinder++)
			for (head = 0; head < geometry->sides; head++)
				for (sector = 0; sector < geometry->secspertrack; sector++)
					data_offset_array[(cylinder*geometry->sides + head)*geometry->secspertrack + sector] = DATA_OFFSET_NONE;
	}
	/* rewind to start of file */
	stream_seek(file_handle, 0, SEEK_SET);

	bytes_read = stream_read(file_handle, &c, 1);
	while (bytes_read)
	{
		if (fm_format)
		{
			while ((c != 0xfe) && bytes_read)
				bytes_read = stream_read(file_handle, &c, 1);

			if (! bytes_read)
				break;

			save_pos = stream_tell(file_handle);

			crc = 0xffff;
			calc_crc(& crc, c);
		}
		else
		{
			while ((c != 0xa1) && bytes_read)
				bytes_read = stream_read(file_handle, &c, 1);

			if (! bytes_read)
				break;

			save_pos = stream_tell(file_handle);

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xa1)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xa1)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}

			crc = 0xffff;
			calc_crc(& crc, c);

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xfe)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}
		}

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		cylinder = c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		head = c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		sector = c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		seclen = 128 << c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		crc1 = c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		crc2 = c;
		calc_crc(& crc, c);

		/* CRC seems to be completely hosed */
		/*if (crc)
			printf("aargh!");*/
		if ((seclen != 256) || (crc1 != 0xf7) || (crc2 != 0xf7)
				|| ((pass == 1) && ((cylinder >= geometry->tracksperside)
									|| (head >= geometry->sides)
									|| (sector >= geometry->secspertrack))))
		{
			stream_seek(file_handle, save_pos, SEEK_SET);
			bytes_read = stream_read(file_handle, &c, 1);
			continue;
		}

		bytes_read = stream_read(file_handle, &c, 1);

		while (bytes_read && (c == (fm_format ? 0xff : 0x4e)))
			bytes_read = stream_read(file_handle, &c, 1);

		while (bytes_read && (c == 0x00))
			bytes_read = stream_read(file_handle, &c, 1);

		if (! bytes_read)
			break;

		if (fm_format)
		{
			if (c != 0xfb)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}
			crc = 0xffff;
			calc_crc(& crc, c);
		}
		else
		{
			if (c != 0xa1)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xa1)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xa1)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}

			crc = 0xffff;
			calc_crc(& crc, c);

			bytes_read = stream_read(file_handle, &c, 1);
			if (! bytes_read)
				break;

			if (c != 0xfb)
			{
				stream_seek(file_handle, save_pos, SEEK_SET);
				bytes_read = stream_read(file_handle, &c, 1);
				continue;
			}
		}
		data_offset = stream_tell(file_handle);
		if (stream_read(file_handle, buf, seclen) != seclen)
			break;
		for (i=0; i<seclen; i++)
			calc_crc(& crc, buf[i]);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		crc1 = c;
		calc_crc(& crc, c);

		bytes_read = stream_read(file_handle, &c, 1);
		if (! bytes_read)
			break;
		crc2 = c;
		calc_crc(& crc, c);

		/* CRC seems to be completely hosed */
		/*if (crc)
			printf("aargh!");*/
		if ((crc1 != 0xf7) || (crc2 != 0xf7))
		{
			stream_seek(file_handle, save_pos, SEEK_SET);
			bytes_read = stream_read(file_handle, &c, 1);
			continue;
		}

		switch (pass)
		{
		case 0:
			if ((cylinder == 0) && (head == 0) && (sector == 0))
			{
				/* return vib */
				memcpy(vib, buf, 256);
				return 0;
			}
			break;

		case 1:
			/* set up offset map */
			data_offset_array[(cylinder*geometry->sides + head)*geometry->secspertrack + sector] = data_offset;
			break;
		}
	}

	if (pass == 0)
		return IMGTOOLERR_CORRUPTIMAGE;

	if (pass == 1)
	{
		/* check offset map */
		for (cylinder = 0; cylinder < geometry->tracksperside; cylinder++)
			for (head = 0; head < geometry->sides; head++)
				for (sector = 0; sector < geometry->secspertrack; sector++)
					if (data_offset_array[(cylinder*geometry->sides + head)*geometry->secspertrack + sector] == DATA_OFFSET_NONE)
						return IMGTOOLERR_CORRUPTIMAGE;
	}

	return 0;
}

/*
	Read the volume information block (sector 0) assuming no geometry
	information.  (Called when an image is opened to figure out the
	geometry information.)

	file_handle: imgtool file handle
	img_format: image format (MESS, V9T9 or PC99)
	dest: pointer to 256-byte destination buffer
*/
static int read_image_vib_no_geometry(STREAM *file_handle, ti99_img_format img_format, dsk_vib *dest)
{
	int reply;

	switch (img_format)
	{
	case if_mess:
	case if_v9t9:
		/* seek to sector */
		reply = stream_seek(file_handle, 0, SEEK_SET);
		if (reply)
			return IMGTOOLERR_READERROR;
		/* read it */
		reply = stream_read(file_handle, dest, 256);
		if (reply != 256)
			return IMGTOOLERR_READERROR;
		return 0;
		break;

	case if_pc99_fm:
	case if_pc99_mfm:
		return parse_pc99_image(file_handle, img_format == if_pc99_fm, 0, dest, NULL, NULL);
		break;
	}

	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Open a disk image at level 1

	file_handle: imgtool file handle
	img_format: image format (MESS, V9T9, PC99, or MAME harddisk)
	l1_img: level-1 image handle (output)
	vib: buffer where the vib should be stored

	returns the imgtool error code
*/
static int open_image_lvl1(STREAM *file_handle, ti99_img_format img_format, ti99_lvl1_imgref *l1_img, dsk_vib *vib)
{
	int reply;
	int totphysrecs;


	l1_img->img_format = img_format;
	l1_img->file_handle = file_handle;

	if (img_format == if_harddisk)
	{
		const struct hard_disk_header *info; 

		l1_img->harddisk_handle = imghd_open(file_handle);
		if (!l1_img->harddisk_handle)
			return IMGTOOLERR_CORRUPTIMAGE;	/* most likely error */

		info = imghd_get_header(l1_img->harddisk_handle);
		l1_img->geometry.tracksperside = info->cylinders;
		l1_img->geometry.sides = info->heads;
		l1_img->geometry.secspertrack = info->sectors;
		if (info->seclen != 256)
		{
			imghd_close(l1_img->harddisk_handle);
			l1_img->harddisk_handle = NULL;
			return IMGTOOLERR_CORRUPTIMAGE;	/* we will need to support 512-byte sectors */
		}

		/* read vib */
		/*reply = read_absolute_physrec(l1_img, 0, vib);
		if (reply)
			return reply;*/
	}
	else
	{
		/* read vib */
		reply = read_image_vib_no_geometry(file_handle, img_format, vib);
		if (reply)
			return reply;

		/* extract geometry information */
		totphysrecs = get_UINT16BE(vib->totphysrecs);

		l1_img->geometry.secspertrack = vib->secspertrack;
		if (l1_img->geometry.secspertrack == 0)
			/* Some images might be like this, because the original SSSD TI
			controller always assumes 9. */
			l1_img->geometry.secspertrack = 9;
		l1_img->geometry.tracksperside = vib->tracksperside;
		if (l1_img->geometry.tracksperside == 0)
			/* Some images are like this, because the original SSSD TI
			controller always assumes 40. */
			l1_img->geometry.tracksperside = 40;
		l1_img->geometry.sides = vib->sides;
		if (l1_img->geometry.sides == 0)
			/* Some images are like this, because the original SSSD TI
			controller always assumes that tracks beyond 40 are on side 2. */
			l1_img->geometry.sides = totphysrecs / (l1_img->geometry.secspertrack * l1_img->geometry.tracksperside);

		/* check information */
		if ((totphysrecs != (l1_img->geometry.secspertrack * l1_img->geometry.tracksperside * l1_img->geometry.sides))
				|| (totphysrecs < 2)
				|| memcmp(vib->id, "DSK", 3) || (! strchr(" P", vib->id[3]))
				|| (((img_format == if_mess) || (img_format == if_v9t9))
					&& (stream_size(file_handle) != totphysrecs*256)))
			return IMGTOOLERR_CORRUPTIMAGE;

		if ((img_format == if_pc99_fm) || (img_format == if_pc99_mfm))
		{
			l1_img->pc99_data_offset_array = malloc(sizeof(*l1_img->pc99_data_offset_array)*totphysrecs);
			if (! l1_img->pc99_data_offset_array)
				return IMGTOOLERR_OUTOFMEMORY;
			reply = parse_pc99_image(file_handle, img_format == if_pc99_fm, 1, NULL, & l1_img->geometry, l1_img->pc99_data_offset_array);
			if (reply)
			{
				free(l1_img->pc99_data_offset_array);
				return reply;
			}
		}
	}

	return 0;
}

/*
	Close a disk image at level 1

	l1_img: level-1 image handle (input)

	returns the imgtool error code
*/
static void close_image_lvl1(ti99_lvl1_imgref *l1_img)
{
	if (l1_img->img_format == if_harddisk)
		imghd_close(l1_img->harddisk_handle);

	stream_close(l1_img->file_handle);

	if ((l1_img->img_format == if_pc99_fm) || (l1_img->img_format == if_pc99_mfm))
		free(l1_img->pc99_data_offset_array);
}

/*
	Convert physical sector address to offset in disk image
*/
INLINE int sector_address_to_image_offset(const ti99_lvl1_imgref *l1_img, const ti99_sector_address *address)
{
	int offset = 0;

	switch (l1_img->img_format)
	{
	case if_mess:
		/* current MESS format */
		offset = (((address->cylinder*l1_img->geometry.sides) + address->side)*l1_img->geometry.secspertrack + address->sector)*256;
		break;

	case if_v9t9:
		/* V9T9 format */
		if (address->side & 1)
			/* on side 1, tracks are stored in the reverse order */
			offset = (((address->side*l1_img->geometry.tracksperside) + (l1_img->geometry.tracksperside-1 - address->cylinder))*l1_img->geometry.secspertrack + address->sector)*256;
		else
			offset = (((address->side*l1_img->geometry.tracksperside) + address->cylinder)*l1_img->geometry.secspertrack + address->sector)*256;
		break;

	case if_pc99_fm:
	case if_pc99_mfm:
		/* pc99 format */
		offset = l1_img->pc99_data_offset_array[(address->cylinder*l1_img->geometry.sides + address->side)*l1_img->geometry.secspertrack + address->sector];
		break;
	}

	return offset;
}

/*
	Read one 256-byte sector from a disk image

	l1_img: level-1 image handle
	address: physical sector address
	dest: pointer to 256-byte destination buffer
*/
static int read_sector(ti99_lvl1_imgref *l1_img, const ti99_sector_address *address, void *dest)
{
	int reply;

	if (l1_img->img_format == if_harddisk)
	{
		return imghd_read(l1_img->harddisk_handle, ((address->cylinder*l1_img->geometry.sides) + address->side)*l1_img->geometry.secspertrack + address->sector, 1, dest) != 1;
	}
	else
	{
		/* seek to sector */
		reply = stream_seek(l1_img->file_handle, sector_address_to_image_offset(l1_img, address), SEEK_SET);
		if (reply)
			return 1;
		/* read it */
		reply = stream_read(l1_img->file_handle, dest, 256);
		if (reply != 256)
			return 1;
	}

	return 0;
}

/*
	Write one 256-byte sector to a disk image

	l1_img: level-1 image handle
	address: physical sector address
	src: pointer to 256-byte source buffer
*/
static int write_sector(ti99_lvl1_imgref *l1_img, const ti99_sector_address *address, const void *src)
{
	int reply;

	if (l1_img->img_format == if_harddisk)
	{
		return imghd_write(l1_img->harddisk_handle, ((address->cylinder*l1_img->geometry.sides) + address->side)*l1_img->geometry.secspertrack + address->sector, 1, src) != 1;
	}
	else
	{
		/* seek to sector */
		reply = stream_seek(l1_img->file_handle, sector_address_to_image_offset(l1_img, address), SEEK_SET);
		if (reply)
			return 1;
		/* write it */
		reply = stream_write(l1_img->file_handle, src, 256);
		if (reply != 256)
			return 1;
	}

	return 0;
}

/*
	Convert 256-byte physical record address to physical sector address
*/
static void physrec_to_sector_address(int physrec, const ti99_geometry *geometry, ti99_sector_address *address)
{
	address->sector = physrec % geometry->secspertrack;
	physrec /= geometry->secspertrack;
	address->cylinder = physrec % geometry->tracksperside;
	address->side = physrec / geometry->tracksperside;
	if (address->side & 1)
		/* on side 1, tracks are stored in the reverse order */
		address->cylinder = geometry->tracksperside-1 - address->cylinder;
}

/*
	Read one 256-byte physical record from a disk image

	l1_img: level-1 image handle
	physrec: absolute physrec address
	dest: pointer to 256-byte destination buffer
*/
static int read_absolute_physrec(ti99_lvl1_imgref *l1_img, int physrec, void *dest)
{
	ti99_sector_address address;


	if (l1_img->img_format == if_harddisk)
	{	/* ***KLUDGE*** */
		address.sector = physrec % l1_img->geometry.secspertrack;
		physrec /= l1_img->geometry.secspertrack;
		address.side = physrec % l1_img->geometry.sides;
		address.cylinder = physrec / l1_img->geometry.sides;
	}
	else
		physrec_to_sector_address(physrec, & l1_img->geometry, & address);

	return read_sector(l1_img, & address, dest);
}

/*
	Write one 256-byte physical record to a disk image

	l1_img: level-1 image handle
	physrec: absolute physrec address
	src: pointer to 256-byte source buffer
*/
static int write_absolute_physrec(ti99_lvl1_imgref *l1_img, int physrec, const void *src)
{
	ti99_sector_address address;


	physrec_to_sector_address(physrec, & l1_img->geometry, & address);

	return write_sector(l1_img, & address, src);
}

#if 0
#pragma mark -
#pragma mark LEVEL 2 DISK ROUTINES
#endif

/*
	Level 2 implements files as a succession of 256-byte-long physical records.

	Level 2 implements allocation bitmap (and AU), disk catalog, etc.
*/

/*
	WIN VIB/DDR record
*/
typedef struct win_vib_ddr
{
	char name[10];			/* disk volume name (10 characters, pad with spaces) */
	UINT16BE totAUs;		/* total number of AUs */
	UINT8 secspertrack;		/* HFDC: sectors per track (typically 32) */
							/* SCSI: reserved */
	union
	{
		struct
		{
			UINT8 id[3];	/* V1 VIB: 'WIN' */
		} vib_v1;

		struct				/* V2 VIB: extra params */
		{
			UINT8 res_AUs;	/* # AUs reserved for vib, bitmap, ddr, fdir and fdr, divided by 64*/
			UINT8 step_spd;	/* HFDC: step speed (0-7) */
							/* SCSI: reserved */
			UINT8 red_w_cur;/* HFDC: reduced write current cylinder, divided by 8 */
							/* SCSI: reserved */
		} vib_v2;

		struct
		{
			UINT8 id[3];	/* DDR: 'DIR' */
		} ddr;
	} u;
	UINT16BE params;		/* bits 0-3: sectors/AU - 1 */
							/* HFDC: */
								/* bits 4-7: # heads - 1 */
								/* bit 8: V1: buffered head stepping flag */
								/*        V2: reserved */
								/* bit 9-15: write precompensation track, divided by 16 */
							/* SCSI: */
								/* bits 4-15: reserved */
	ti99_date_time creation;/* date and time of creation */
	UINT8 num_files;		/* number of files in directory */
	UINT8 num_subdirs;		/* number of subdirectories in directory */
	UINT16BE fdir_AU;		/* points to root directory fdir */
	union
	{
		struct
		{
			UINT16BE dsk1_AU;	/* HFDC: points to current dsk1 emulation image (0 if none) */
								/* SCSI: reserved */
		} vib;

		struct
		{
			UINT16BE parent_ddr_AU;	/* points to parent directory DDR */
		} ddr;
	} u2;
	UINT16BE subdir_AU[114];/* points to all subdirectory DDRs */
} win_vib_ddr;

/*
	AU format
*/
typedef struct ti99_AUformat
{
	int totAUs;				/* total number of AUs */
	int physrecsperAU;		/* number of 256-byte physical records per AU */
} ti99_AUformat;

/*
	directory reference: 0 for root, 1 for 1st subdir, 2 for 2nd subdir, 3 for
	3rd subdir
*/
//typedef int dir_ref;

/*
	catalog entry (used for in-memory catalog)
*/
typedef struct dir_entry
{
	UINT16 fdir_physrec;	/* sector (physrec?) address of the fdir for this directory */
	char dirname[10];		/* name of this directory (copied from the VIB) */
} dir_entry;

typedef struct file_entry
{
	UINT16 fdr_physrec;		/* sector (physrec?) address of the fdr for this file */
	char fname[10];			/* name of this file (copied from fdr) */
} file_entry;

typedef struct ti99_catalog
{
	int num_subdirs;		/* number of subdirectories */
	int num_files;			/* number of files */
	dir_entry subdirs[/*3*/114];	/* description of each subdir */
	file_entry files[128];	/* description of each file */
} ti99_catalog;

/*
	level-2 disk image descriptor
*/
typedef struct ti99_lvl2_imgref_dsk
{
	dsk_vib vib;			/* cached copy of volume information block record in sector 0 */
	ti99_catalog catalog;	/* main catalog (fdr physrec address from sector 1, and file names from fdrs) */
	ti99_catalog subdir_catalog[3];	/* catalog of each subdirectory */
	int data_offset;	/* fdr records are preferentially allocated in sectors 2 to data_offset (34)
						whereas data records are preferentially allocated in sectors starting at data_offset. */
} ti99_lvl2_imgref_dsk;

typedef struct ti99_lvl2_imgref_win
{
	enum
	{
		win_vib_v1,
		win_vib_v2
	} vib_version;
	win_vib_ddr vib;	/* cached copy of volume information block record in sector 0 */
	int data_offset;	/* fdr records are preferentially allocated in sectors 2 to data_offset (34)
						whereas data records are preferentially allocated in sectors starting at data_offset. */
} ti99_lvl2_imgref_win;

typedef struct ti99_lvl2_imgref
{
	IMAGE base;
	ti99_lvl1_imgref l1_img;	/* image format, imgtool image handle, image geometry */
	ti99_AUformat AUformat;		/* AU format */

	enum
	{
		L2I_DSK,
		L2I_WIN
	} type;

	union
	{
		ti99_lvl2_imgref_dsk dsk;
		ti99_lvl2_imgref_win win;
	} u;
} ti99_lvl2_imgref;

/*
	file flags found in fdr (and tifiles)
*/
enum
{
	fdr99_f_program	= 0x01,	/* set for program files */
	fdr99_f_int		= 0x02,	/* set for binary files */
	fdr99_f_wp		= 0x08,	/* set if file is write-protected */
	/*fdr99_f_backup	= 0x10,*/	/* set if file has been modified since last backup */
	/*fdr99_f_dskimg	= 0x20,*/	/* set if file is a DSK image (HFDC HD only) */
	fdr99_f_var		= 0x80	/* set if file uses variable-length records*/
};

/*
	DSK FDR record
*/
typedef struct dsk_fdr
{
	char name[10];			/* file name (10 characters, pad with spaces) */
	UINT16BE xreclen;		/* extended record len: if record len is >= 256, */
								/* reclen is set to 0 and the actual reclen is */
								/* stored here (Myarc HFDC only).  TI reserved */
								/* this  field for data chain pointer extension, */
								/* but this was never implemented. */
	UINT8 flags;			/* file status flags (see enum above) */
	UINT8 recspersec;		/* logical records per 256-byte record (i.e. sector) */
								/* ignored for variable length record files and */
								/* program files */
	UINT16BE secsused;		/* allocated file length in 256-byte records (i.e. */
								/* sectors) */
	UINT8 eof;				/* EOF offset in last sector for variable length */
								/* record files and program files (0->256)*/
	UINT8 reclen;			/* logical record size in bytes ([1,255] 0->256) */
								/* Maximum allowable record size for variable */
								/* length record files.  Reserved for program */
								/* files (set to 0).  Set to 0 if reclen >=256 */
								/* (HFDC only). */
	UINT16LE fixrecs;		/* file length in logical records */
								/* For variable length record files, number of */
								/* 256-byte records actually used. */
	ti99_date_time creation;/* date and time of creation (HFDC and BwG only; */
								/*reserved in TI) */
	ti99_date_time update;	/* date and time of last write to file (HFDC and */
								/* BwG only; reserved in TI) */
	UINT8 clusters[76][3];	/* data cluster table: 0 through 76 entries (3 */
								/* bytes each), one entry for each file cluster. */
								/* 12 bits: address of first AU of cluster */
								/* 12 bits: offset of last 256-byte record in cluster */
} dsk_fdr;

/*
	WIN FDR record
*/
typedef struct win_fdr
{
	char name[10];			/* file name (10 characters, pad with spaces) */
	UINT16BE xreclen;		/* extended record len: if record len is >= 256, */
								/* reclen is set to 0 and the actual reclen is */
								/* stored here (Myarc HFDC only).  TI reserved */
								/* this  field for data chain pointer extension, */
								/* but this was never implemented. */
	UINT8 flags;			/* file status flags (see enum above) */
	UINT8 recspersec;		/* logical records per 256-byte record (i.e. sector) */
								/* ignored for variable length record files and */
								/* program files */
	UINT16BE secsused;		/* allocated file length in 256-byte records (i.e. */
								/* sectors) */
	UINT8 eof;				/* EOF offset in last sector for variable length */
								/* record files and program files (0->256)*/
	UINT8 reclen;			/* logical record size in bytes ([1,255] 0->256) */
								/* Maximum allowable record size for variable */
								/* length record files.  Reserved for program */
								/* files (set to 0).  Set to 0 if reclen >=256 */
								/* (HFDC only). */
	UINT16LE fixrecs;		/* file length in logical records */
								/* For variable length record files, number of */
								/* 256-byte records actually used. */
	ti99_date_time creation;/* date and time of creation (HFDC and BwG only; */
								/*reserved in TI) */
	ti99_date_time update;	/* date and time of last write to file (HFDC and */
								/* BwG only; reserved in TI) */

	char id[2];				/* 'FI' */
	UINT16BE prev_FDR_AU;	/* previous cluster table extension FDR */
	UINT16BE next_FDR_AU;	/* next cluster table extension FDR */
	UINT16BE num_FDR_AU;	/* total numebr of AUs allocated for FDR FDR */
	UINT16BE parent_FDIR_AU;/* FDIR the file is listed in */
	UINT16BE xinfo;			/* extended information */
								/* bits 0-3: MSN of secsused */
								/* bits 4-7: MSN of fixrecs */
								/* bits 8-11: sector within AU for prev_FDR_AU */
								/* bits 12-15: sector within AU for next_FDR_AU */
	UINT16BE clusters[54][2];/* data cluster table: 0 through 54 entries (4 */
								/* bytes each), one entry for each file cluster. */
								/* 16 bits: address of first AU of cluster */
								/* 16 bits: address of last AU of cluster */
} win_fdr;

/*
	tifile header: stand-alone file 
*/
typedef struct tifile_header
{
	char tifiles[8];		/* always '\7TIFILES' */
	UINT16BE secsused;		/* file length in sectors */
	UINT8 flags;			/* see enum above */
	UINT8 recspersec;		/* records per sector */
	UINT8 eof;				/* current position of eof in last sector (0->255)*/
	UINT8 reclen;			/* bytes per record ([1,255] 0->256) */
	UINT16BE fixrecs;		/* file length in records */
	UINT8 res[128-16];		/* reserved */
								/* * variant a: */
									/* 112 chars: 0xCA53 repeated 56 times */
								/* * variant b: */
									/* 4 chars: unknown */
									/* 108 chars: 0xCA53 repeated 54 times */
								/* * variant c: */
									/* 10 chars: original TI file name filed with spaces */
									/* 102 chars: spaces */
								/* * variant d: */
									/* 10 chars: original TI file name filed with spaces */
									/* 2 chars: CR+LF */
									/* 98 chars: spaces */
									/* 2 chars: CR+LF */
								/* * variant e: */
									/* 10 chars: original TI file name */
									/* 4 bytes: unknown */
									/* 4 bytes: time & date of creation */
									/* 4 bytes: time & date of last update */
									/* 90 chars: spaces */
								/* * variant f: */
									/* 6 bytes: 'MYTERM' */
									/* 4 bytes: time & date of creation */
									/* 4 bytes: time & date of last update */
									/* 2 bytes: unknown (always >0000) */
									/* 96 chars: 0xCA53 repeated 56 times */
} tifile_header;

/*
	level-2 file descriptor
*/
typedef struct ti99_lvl2_fileref_dsk
{
	ti99_lvl2_imgref *l2_img;
	int fdr_physrec;
	dsk_fdr fdr;
} ti99_lvl2_fileref_dsk;

typedef struct ti99_lvl2_fileref_win
{
	ti99_lvl2_imgref *l2_img;
	int curfdr_physrec;
	int cur_cluster_index;
	int cur_cluster_pos;
	win_fdr curfdr;
} ti99_lvl2_fileref_win;

typedef struct ti99_lvl2_fileref_tifiles
{
	STREAM *file_handle;
	tifile_header hdr;
} ti99_lvl2_fileref_tifiles;

typedef struct ti99_lvl2_fileref
{
	enum
	{
		L2F_DSK,
		L2F_WIN,
		L2F_TIFILES
	} type;
	union
	{
		ti99_lvl2_fileref_dsk dsk;
		ti99_lvl2_fileref_win win;
		ti99_lvl2_fileref_tifiles tifiles;
	} u;
} ti99_lvl2_fileref;



/*
	Compare two (possibly empty) catalog entry for qsort
*/
static int qsort_catalog_compare(const void *p1, const void *p2)
{
	const file_entry *entry1 = p1;
	const file_entry *entry2 = p2;

	if ((entry1->fdr_physrec == 0) && (entry2->fdr_physrec == 0))
		return 0;
	else if (entry1->fdr_physrec == 0)
		return +1;
	else if (entry2->fdr_physrec == 0)
		return -1;
	else
		return memcmp(entry1->fname, entry2->fname, 10);
}

/*
	Read a directory catalog from disk image

	l2_img: image reference
	physrec: physical record address of the FDIR
	dest: pointer to the destination buffer where the catalog should be written

	Returns an error code if there was an error, 0 otherwise.
*/
static int dsk_read_catalog(ti99_lvl2_imgref *l2_img, int physrec, ti99_catalog *dest)
{
	int totphysrecs = get_UINT16BE(l2_img->u.dsk.vib.totphysrecs);
	UINT8 buf[256];
	dsk_fdr fdr;
	int i;
	int reply;


	/* Read FDIR record */
	reply = read_absolute_physrec(& l2_img->l1_img, physrec, buf);
	if (reply)
		return IMGTOOLERR_READERROR;

	/* Copy FDIR info to catalog structure */
	for (i=0; i<128; i++)
		dest->files[i].fdr_physrec = (buf[i*2] << 8) | buf[i*2+1];

	/* Check FDIR pointers and check and extract file names from FDRs */
	for (i=0; i<128; i++)
	{
		if (dest->files[i].fdr_physrec >= totphysrecs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->files[i].fdr_physrec)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->files[i].fdr_physrec, &fdr);
			if (reply)
				return IMGTOOLERR_READERROR;

			/* check and copy file name */
			if (check_fname(fdr.name))
				return IMGTOOLERR_CORRUPTIMAGE;
			memcpy(dest->files[i].fname, fdr.name, 10);
		}
	}

	/* Check catalog */
	for (i=0; i<127; i++)
	{
		if (((! dest->files[i].fdr_physrec) && dest->files[i+1].fdr_physrec)
			|| ((dest->files[i].fdr_physrec && dest->files[i+1].fdr_physrec) && (memcmp(dest->files[i].fname, dest->files[i+1].fname, 10) >= 0)))
		{
			/* if the catalog is not sorted, we repair it */
			qsort(dest, sizeof(dest->files)/sizeof(dest->files[0]), sizeof(dest->files[0]),
					qsort_catalog_compare);
			break;
		}
	}

	/* Set file count */
	for (i=0; (i<128) && (dest->files[i].fdr_physrec != 0); i++)
		;
	dest->num_files = i;

	/* Set subdir count to 0 (subdirs are loaded elsewhere) */
	dest->num_subdirs = 0;

	return 0;
}

/*
	Read a directory catalog from disk image

	l2_img: image reference
	DDR_AU: AU address of the VIB/DDR
	dest: pointer to the destination buffer where the catalog should be written

	Returns an error code if there was an error, 0 otherwise.
*/
static int win_read_catalog(ti99_lvl2_imgref *l2_img, int DDR_AU, ti99_catalog *dest)
{
	win_vib_ddr ddr_buf;
	UINT16BE fdir_buf[128];
	win_fdr fdr_buf;
	int i;
	int reply;


	/* Read DDR record */
	reply = read_absolute_physrec(& l2_img->l1_img, DDR_AU*l2_img->AUformat.physrecsperAU, &ddr_buf);
	if (reply)
		return IMGTOOLERR_READERROR;

	/* sanity checks */
	if ((ddr_buf.num_files > 128) || (ddr_buf.num_subdirs > 114) || (get_UINT16BE(ddr_buf.fdir_AU) > l2_img->AUformat.totAUs))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* set file count and subdir count */
	dest->num_files = ddr_buf.num_files;
	dest->num_subdirs = ddr_buf.num_subdirs;

	/* Copy DDR info to catalog structure */
	for (i=0; i<ddr_buf.num_subdirs; i++)
		dest->subdirs[i].fdir_physrec = get_UINT16BE(ddr_buf.subdir_AU[i]);

	/* Read FDIR record */
	reply = read_absolute_physrec(& l2_img->l1_img, get_UINT16BE(ddr_buf.fdir_AU)*l2_img->AUformat.physrecsperAU, fdir_buf);
	if (reply)
		return IMGTOOLERR_READERROR;

	/* Copy FDIR info to catalog structure */
	for (i=0; i<dest->num_files; i++)
		dest->files[i].fdr_physrec = get_UINT16BE(fdir_buf[i]);

	/* Check DDR pointers and check and extract file names from FDRs */
	for (i=0; i<dest->num_subdirs; i++)
	{
		if (dest->subdirs[i].fdir_physrec >= l2_img->AUformat.totAUs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->subdirs[i].fdir_physrec)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->subdirs[i].fdir_physrec*l2_img->AUformat.physrecsperAU, &ddr_buf);
			if (reply)
				return IMGTOOLERR_READERROR;

			/* check and copy file name */
			if (check_fname(ddr_buf.name))
				return IMGTOOLERR_CORRUPTIMAGE;
			memcpy(dest->subdirs[i].dirname, ddr_buf.name, 10);
		}
	}

	/* Check FDIR pointers and check and extract file names from FDRs */
	for (i=0; i<dest->num_files; i++)
	{
		if (dest->files[i].fdr_physrec >= l2_img->AUformat.totAUs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->files[i].fdr_physrec)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->files[i].fdr_physrec*l2_img->AUformat.physrecsperAU, &fdr_buf);
			if (reply)
				return IMGTOOLERR_READERROR;

			/* check and copy file name */
			if (check_fname(fdr_buf.name))
				return IMGTOOLERR_CORRUPTIMAGE;
			memcpy(dest->files[i].fname, fdr_buf.name, 10);
		}
	}

#if 0
	/* Check catalog */
	for (i=0; i<dest->num_files-1; i++)
	{
		if (((! dest->files[i].fdr_physrec) && dest->files[i+1].fdr_physrec)
			|| ((dest->files[i].fdr_physrec && dest->files[i+1].fdr_physrec) && (memcmp(dest->files[i].fname, dest->files[i+1].fname, 10) >= 0)))
		{
			/* if the catalog is not sorted, we repair it */
			qsort(dest, sizeof(dest->files)/sizeof(dest->files[0]), sizeof(dest->files[0]),
					qsort_catalog_compare);
			break;
		}
	}
#endif

	return 0;
}

/*
	Search for a file path on a floppy image

	l2_img: image reference
	fpath: path of the file to search
	parent_ref_valid: set to TRUE if either the file was found or the file was
		not found but its parent dir was
	parent_ref: reference to parent dir (0 for root)
	out_is_dir: TRUE if element is a directory
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int dsk_find_catalog_entry(ti99_lvl2_imgref *l2_img, const char *fpath, int *parent_ref_valid, int *parent_ref, int *out_is_dir, int *catalog_index)
{
	int i;
	const ti99_catalog *cur_catalog;
	const char *element_start, *element_end;
	int element_len;
	char element[10];
	int is_dir = FALSE;


	cur_catalog = & l2_img->u.dsk.catalog;
	if (parent_ref_valid)
		(* parent_ref_valid) = FALSE;
	if (parent_ref)
		*parent_ref = 0;

	element_start = fpath;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, '.');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		if ((element_len > 10) || (element_len == 0))
			return IMGTOOLERR_BADFILENAME;
		/* last path element */
		if ((!element_end) && parent_ref_valid)
			(* parent_ref_valid) = TRUE;

		/* generate file name */
		memcpy(element, element_start, element_len);
		memset(element+element_len, ' ', 10-element_len);

		/* search entry in subdirectories */
		for (i = 0; i<cur_catalog->num_subdirs; i++)
		{
			if (! memcmp(element, cur_catalog->subdirs[i].dirname, 10))
			{
				is_dir = TRUE;
				break;
			}
		}

		/* if it failed, search entry in files */
		if (i == cur_catalog->num_subdirs)
		{
			for (i = 0; i<cur_catalog->num_files; i++)
			{
				if (! memcmp(element, cur_catalog->files[i].fname, 10))
				{
					is_dir = FALSE;
					break;
				}
			}
			/* exit if not found */
			if (i == cur_catalog->num_files)
			{
				return IMGTOOLERR_FILENOTFOUND;
			}
		}

		/* iterate */
		if (element_end)
		{
			element_start = element_end+1;

			if (! is_dir)
				/* this is not a directory */
				return IMGTOOLERR_BADFILENAME;

			/* initialize cur_catalog */
			cur_catalog = & l2_img->u.dsk.subdir_catalog[i];
			if (parent_ref)
				*parent_ref = i+1;
		}
		else
			element_start = NULL;
	}
	while (element_start);

	if (out_is_dir)
		*out_is_dir = is_dir;

	if (catalog_index)
		*catalog_index = i;

	return 0;
}

/*
	Search for a file path on a harddisk image

	l2_img: image reference
	fpath: path of the file to search
	parent_ref_valid: set to TRUE if either the file was found or the file was
		not found but its parent dir was
	parent_ref: reference to parent dir (0 for root)
	parent_catalog: catalog of parent dir (cannot be NULL)
	out_is_dir: TRUE if element is a directory
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int win_find_catalog_entry(ti99_lvl2_imgref *l2_img, const char *fpath, int *parent_ref_valid, int *parent_ref, ti99_catalog *parent_catalog, int *out_is_dir, int *catalog_index)
{
	int i;
	const char *element_start, *element_end;
	int element_len;
	char element[10];
	int is_dir = FALSE;
	int errorcode;

	if (parent_ref_valid)
		(* parent_ref_valid) = FALSE;
	if (parent_ref)
		*parent_ref = 0;

	errorcode = win_read_catalog(l2_img, 0, parent_catalog);
	if (errorcode)
		return errorcode;

	element_start = fpath;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, '.');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		if ((element_len > 10) || (element_len == 0))
			return IMGTOOLERR_BADFILENAME;
		/* last path element */
		if ((!element_end) && parent_ref_valid)
			(* parent_ref_valid) = TRUE;

		/* generate file name */
		memcpy(element, element_start, element_len);
		memset(element+element_len, ' ', 10-element_len);

		/* search entry in subdirectories */
		for (i = 0; i<parent_catalog->num_subdirs; i++)
		{
			if (! memcmp(element, parent_catalog->subdirs[i].dirname, 10))
			{
				is_dir = TRUE;
				break;
			}
		}

		/* if it failed, search entry in files */
		if (i == parent_catalog->num_subdirs)
		{
			for (i = 0; i<parent_catalog->num_files; i++)
			{
				if (! memcmp(element, parent_catalog->files[i].fname, 10))
				{
					is_dir = FALSE;
					break;
				}
			}
			/* exit if not found */
			if (i == parent_catalog->num_files)
			{
				return IMGTOOLERR_FILENOTFOUND;
			}
		}

		/* iterate */
		if (element_end)
		{
			element_start = element_end+1;

			if (! is_dir)
				/* this is not a directory */
				return IMGTOOLERR_BADFILENAME;

			if (parent_ref)
				*parent_ref = parent_catalog->subdirs[i].fdir_physrec;

			errorcode = win_read_catalog(l2_img, parent_catalog->subdirs[i].fdir_physrec, parent_catalog);
			if (errorcode)
				return errorcode;
		}
		else
			element_start = NULL;
	}
	while (element_start);

	if (out_is_dir)
		*out_is_dir = is_dir;

	if (catalog_index)
		*catalog_index = i;

	return 0;
}

/*
	Allocate one sector on disk, for use as a fdr record

	l2_img: image reference
	fdr_physrec: on output, logical address of a free sector
*/
static int alloc_fdr_sector(ti99_lvl2_imgref *l2_img, int *fdr_physrec)
{
	int totAUs = l2_img->AUformat.totAUs;
	int i;


	for (i=0; i<totAUs; i++)
	{
		if (! (l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7))))
		{
			*fdr_physrec = i * l2_img->AUformat.physrecsperAU;
			l2_img->u.dsk.vib.abm[i >> 3] |= 1 << (i & 7);

			return 0;
		}
	}

	return IMGTOOLERR_NOSPACE;
}

INLINE int get_fdr_cluster_baseAU(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/sector for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to AU address */
	if (l2_img->AUformat.physrecsperAU <= 2)
		reply /= l2_img->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_fdr_cluster_basephysrec(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/sector for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to sector address */
	if (l2_img->AUformat.physrecsperAU > 2)
		reply *= l2_img->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_fdr_cluster_lastphysrec(dsk_fdr *fdr, int cluster_index)
{
	return (fdr->clusters[cluster_index][2] << 4) | (fdr->clusters[cluster_index][1] >> 4);
}

INLINE void set_fdr_cluster_lastphysrec(dsk_fdr *fdr, int cluster_index, int data)
{
	fdr->clusters[cluster_index][1] = (fdr->clusters[cluster_index][1] & 0x0f) | (data << 4);
	fdr->clusters[cluster_index][2] = data >> 4;
}

INLINE void set_fdr_cluster(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index, int baseAU, int last_physrec)
{
	/* convert AU address to FDR value */
	if (l2_img->AUformat.physrecsperAU <= 2)
		baseAU *= l2_img->AUformat.physrecsperAU;

	/* write cluster entry */
	fdr->clusters[cluster_index][0] = baseAU;
	fdr->clusters[cluster_index][1] = ((baseAU >> 8) & 0x0f) | (last_physrec << 4);
	fdr->clusters[cluster_index][2] = last_physrec >> 4;
}

/*
	Extend a file with nb_alloc_physrecs extra physrecs

	dsk_file: file reference
	nb_alloc_physrecs: number of physical records to allocate
*/
static int dsk_alloc_file_sectors(ti99_lvl2_fileref_dsk *dsk_file, int nb_alloc_physrecs)
{
	int totAUs = dsk_file->l2_img->AUformat.totAUs;
	int free_sectors;
	int secsused;
	int i;
	int cluster_index;
	int last_sec, p_last_sec;
	int cur_block_seclen;
	int cluster_baseAU, cluster_AUlen;
	int first_best_block_baseAU = 0, first_best_block_seclen;
	int second_best_block_baseAU = 0, second_best_block_seclen;
	int search_start;

	/* compute free space */
	free_sectors = 0;
	for (i=0; i<totAUs; i++)
	{
		if (! (dsk_file->l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7))))
			free_sectors += dsk_file->l2_img->AUformat.physrecsperAU;
	}

	/* check we have enough free space */
	if (free_sectors < nb_alloc_physrecs)
		return IMGTOOLERR_NOSPACE;

	/* current number of data sectors in file */
	secsused = get_UINT16BE(dsk_file->fdr.secsused);

	if (secsused == 0)
	{	/* cluster array must be empty */
		cluster_index = 0;
	}
	else
	{	/* try to extend last block */
		last_sec = -1;
		for (cluster_index=0; cluster_index<76; cluster_index++)
		{
			p_last_sec = last_sec;
			last_sec = get_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index);
			if (last_sec >= (secsused-1))
				break;
		}
		if (cluster_index == 76)
			/* that sucks */
			return IMGTOOLERR_CORRUPTIMAGE;

		if (last_sec > (secsused-1))
		{	/* some extra space has already been allocated */
			cur_block_seclen = last_sec - (secsused-1);
			if (cur_block_seclen > nb_alloc_physrecs)
				cur_block_seclen = nb_alloc_physrecs;

			secsused += cur_block_seclen;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);
			nb_alloc_physrecs -= cur_block_seclen;
			if (! nb_alloc_physrecs)
				return 0;	/* done */
		}

		/* round up to next AU boundary */
		last_sec = last_sec + dsk_file->l2_img->AUformat.physrecsperAU - (last_sec % dsk_file->l2_img->AUformat.physrecsperAU) - 1;

		if (last_sec > (secsused-1))
		{	/* some extra space has already been allocated */
			cur_block_seclen = last_sec - (secsused-1);
			if (cur_block_seclen > nb_alloc_physrecs)
				cur_block_seclen = nb_alloc_physrecs;

			secsused += cur_block_seclen;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);
			set_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index, secsused-1);
			nb_alloc_physrecs -= cur_block_seclen;
			if (! nb_alloc_physrecs)
				return 0;	/* done */
		}

		/* read base AU address for this cluster */
		cluster_baseAU = get_fdr_cluster_baseAU(dsk_file->l2_img, &dsk_file->fdr, cluster_index);
		/* point past cluster end */
		cluster_baseAU += (last_sec-p_last_sec/*+file->l2_img->AUformat.physrecsperAU-1*/) / dsk_file->l2_img->AUformat.physrecsperAU;
		/* count free sectors after last block */
		cur_block_seclen = 0;
		for (i=cluster_baseAU; (! (dsk_file->l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7)))) && (cur_block_seclen < nb_alloc_physrecs) && (i < totAUs); i++)
			cur_block_seclen += dsk_file->l2_img->AUformat.physrecsperAU;
		if (cur_block_seclen)
		{	/* extend last block */
			if (cur_block_seclen > nb_alloc_physrecs)
				cur_block_seclen = nb_alloc_physrecs;

			secsused += cur_block_seclen;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);
			last_sec += cur_block_seclen;
			nb_alloc_physrecs -= cur_block_seclen;
			set_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index, last_sec);
			cluster_AUlen = (cur_block_seclen + dsk_file->l2_img->AUformat.physrecsperAU - 1) / dsk_file->l2_img->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				dsk_file->l2_img->u.dsk.vib.abm[(i+cluster_baseAU) >> 3] |= 1 << ((i+cluster_baseAU) & 7);
			if (! nb_alloc_physrecs)
				return 0;	/* done */
		}
		cluster_index++;
		if (cluster_index == 76)
			/* that sucks */
			return IMGTOOLERR_NOSPACE;
	}

	search_start = dsk_file->l2_img->u.dsk.data_offset;	/* initially, search for free space only in data space */
	while (nb_alloc_physrecs)
	{
		/* find smallest data block at least nb_alloc_sectors in lenght, and largest data block less than nb_alloc_sectors in lenght */
		first_best_block_seclen = INT_MAX;
		second_best_block_seclen = 0;
		for (i=search_start; i<totAUs; i++)
		{
			if (! (dsk_file->l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7))))
			{	/* found one free block */
				/* compute its lenght */
				cluster_baseAU = i;
				cur_block_seclen = 0;
				while ((i<totAUs) && (! (dsk_file->l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7)))))
				{
					cur_block_seclen += dsk_file->l2_img->AUformat.physrecsperAU;
					i++;
				}
				/* compare to previous best and second-best blocks */
				if ((cur_block_seclen < first_best_block_seclen) && (cur_block_seclen >= nb_alloc_physrecs))
				{
					first_best_block_baseAU = cluster_baseAU;
					first_best_block_seclen = cur_block_seclen;
					if (cur_block_seclen == nb_alloc_physrecs)
						/* no need to search further */
						break;
				}
				else if ((cur_block_seclen > second_best_block_seclen) && (cur_block_seclen < nb_alloc_physrecs))
				{
					second_best_block_baseAU = cluster_baseAU;
					second_best_block_seclen = cur_block_seclen;
				}
			}
		}

		if (first_best_block_seclen != INT_MAX)
		{	/* found one contiguous block which can hold it all */
			secsused += nb_alloc_physrecs;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);

			set_fdr_cluster(dsk_file->l2_img, &dsk_file->fdr, cluster_index, first_best_block_baseAU, secsused-1);
			cluster_AUlen = (nb_alloc_physrecs + dsk_file->l2_img->AUformat.physrecsperAU - 1) / dsk_file->l2_img->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				dsk_file->l2_img->u.dsk.vib.abm[(i+first_best_block_baseAU) >> 3] |= 1 << ((i+first_best_block_baseAU) & 7);

			nb_alloc_physrecs = 0;
		}
		else if (second_best_block_seclen != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			secsused += second_best_block_seclen;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);

			set_fdr_cluster(dsk_file->l2_img, &dsk_file->fdr, cluster_index, second_best_block_baseAU, secsused-1);
			cluster_AUlen = (second_best_block_seclen + dsk_file->l2_img->AUformat.physrecsperAU - 1) / dsk_file->l2_img->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				dsk_file->l2_img->u.dsk.vib.abm[(i+second_best_block_baseAU) >> 3] |= 1 << ((i+second_best_block_baseAU) & 7);

			nb_alloc_physrecs -= second_best_block_seclen;

			cluster_index++;
			if (cluster_index == 76)
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
static int new_file_lvl2_dsk(ti99_lvl2_imgref *l2_img, int parent_ref, char fname[10], ti99_lvl2_fileref *l2_file)
{
	ti99_catalog *catalog =  parent_ref ? &l2_img->u.dsk.subdir_catalog[parent_ref-1] : &l2_img->u.dsk.catalog;
	int fdr_physrec;
	int catalog_index, i;
	int reply = 0;
	int errorcode;


	if (catalog->num_files >= 127)
		/* if num_files == 128, catalog is full */
		/* if num_files == 127, catalog is not full, but we don't want to write
		a 128th entry for compatibility with some existing DSRs that detect the
		end of the FDIR array with a 0 entry (and do not check that the index
		has not reached 128) */
		return IMGTOOLERR_NOSPACE;

	/* find insertion point in catalog */
	for (i=0; (i < catalog->num_files) && ((reply = memcmp(catalog->files[i].fname, fname, 10)) < 0); i++)
		;

	if ((i<catalog->num_files) && (reply == 0))
		/* file already exists */
		return IMGTOOLERR_BADFILENAME;
	else
	{
		/* otherwise insert new entry */
		catalog_index = i;
		errorcode = alloc_fdr_sector(l2_img, &fdr_physrec);
		if (errorcode)
			return errorcode;

		/* shift catalog entries until the insertion point */
		for (i=catalog->num_files; i>catalog_index; i--)
			catalog->files[i] = catalog->files[i-1];

		/* write new catalog entry */
		catalog->files[catalog_index].fdr_physrec = fdr_physrec;
		memcpy(catalog->files[catalog_index].fname, fname, 10);

		/* update catalog len */
		catalog->num_files++;
	}

	/* set up file handle */
	l2_file->type = L2F_DSK;
	l2_file->u.dsk.l2_img = l2_img;
	l2_file->u.dsk.fdr_physrec = fdr_physrec;
	memset(&l2_file->u.dsk.fdr, 0, sizeof(l2_file->u.dsk.fdr));
	memcpy(l2_file->u.dsk.fdr.name, fname, 10);

	return 0;
}

/*
	Allocate a new (empty) file
*/
static int new_file_lvl2_tifiles(STREAM *file_handle, ti99_lvl2_fileref *l2_file)
{
	/* set up file handle */
	l2_file->type = L2F_TIFILES;
	l2_file->u.tifiles.file_handle = file_handle;
	memset(&l2_file->u.tifiles.hdr, 0, sizeof(l2_file->u.tifiles.hdr));
	l2_file->u.tifiles.hdr.tifiles[0] = '\7';
	l2_file->u.tifiles.hdr.tifiles[1] = 'T';
	l2_file->u.tifiles.hdr.tifiles[2] = 'I';
	l2_file->u.tifiles.hdr.tifiles[3] = 'F';
	l2_file->u.tifiles.hdr.tifiles[4] = 'I';
	l2_file->u.tifiles.hdr.tifiles[5] = 'L';
	l2_file->u.tifiles.hdr.tifiles[6] = 'E';
	l2_file->u.tifiles.hdr.tifiles[7] = 'S';

	return 0;
}

/*
	Open an existing file on a floppy image

	l2_img: level 2 image the file is located on
	fpath: access path to the file
	file: set up if open is successful
*/
static int open_file_lvl2_dsk(ti99_lvl2_imgref *l2_img, const char *fpath, ti99_lvl2_fileref *l2_file)
{
	int parent_ref, is_dir, catalog_index;
	int reply;


	reply = dsk_find_catalog_entry(l2_img, fpath, NULL, &parent_ref, &is_dir, &catalog_index);
	if (reply)
		return reply;

	if (is_dir)
		/* this is not a file */
		return IMGTOOLERR_BADFILENAME;

	l2_file->type = L2F_DSK;
	l2_file->u.dsk.l2_img = l2_img;
	l2_file->u.dsk.fdr_physrec = (parent_ref ? l2_img->u.dsk.subdir_catalog[parent_ref-1] : l2_img->u.dsk.catalog).files[catalog_index].fdr_physrec;
	if (read_absolute_physrec(& l2_img->l1_img, l2_file->u.dsk.fdr_physrec, &l2_file->u.dsk.fdr))
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	Open an existing file on a harddisk image

	l2_img: level 2 image the file is located on
	fpath: access path to the file
	file: set up if open is successful
*/
static int open_file_lvl2_win(ti99_lvl2_imgref *l2_img, const char *fpath, ti99_lvl2_fileref *l2_file)
{
	int parent_ref, is_dir, catalog_index;
	ti99_catalog catalog;
	int reply;

	reply = win_find_catalog_entry(l2_img, fpath, NULL, &parent_ref, &catalog, &is_dir, &catalog_index);
	if (reply)
		return reply;

	if (is_dir)
		/* this is not a file */
		return IMGTOOLERR_BADFILENAME;

	l2_file->type = L2F_WIN;
	l2_file->u.win.l2_img = l2_img;
	l2_file->u.win.curfdr_physrec = catalog.files[catalog_index].fdr_physrec * l2_img->AUformat.physrecsperAU;
	l2_file->u.win.cur_cluster_index = 0;
	l2_file->u.win.cur_cluster_pos = 0;
	if (read_absolute_physrec(& l2_img->l1_img, l2_file->u.win.curfdr_physrec, &l2_file->u.win.curfdr))
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	Open an existing file in TIFILES format
*/
static int open_file_lvl2_tifiles(STREAM *file_handle, ti99_lvl2_fileref *l2_file)
{
	/* set up file handle */
	l2_file->type = L2F_TIFILES;
	l2_file->u.tifiles.file_handle = file_handle;

	/* seek to header */
	if (stream_seek(l2_file->u.tifiles.file_handle, 0, SEEK_SET))
		return IMGTOOLERR_READERROR;
	/* read it */
	if (stream_read(l2_file->u.tifiles.file_handle, &l2_file->u.tifiles.hdr, sizeof(l2_file->u.tifiles.hdr)) != sizeof(l2_file->u.tifiles.hdr))
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	compute the absolute physrec address for a 256-byte physical record in a file

	l2_img: image where the file is located
*/
static int dsk_file_physrec_to_abs_physrec(ti99_lvl2_fileref_dsk *dsk_file, int physrecnum, int *out_physrec)
{
	int cluster_index;
	int cluster_firstphysrec, cluster_lastphysrec;
	int cluster_base_physrec;

	/* check parameter */
	if (physrecnum >= get_UINT16BE(dsk_file->fdr.secsused))
		return IMGTOOLERR_UNEXPECTED;

	/* search for the cluster in the data chain pointers array */
	cluster_lastphysrec = -1;
	for (cluster_index=0; cluster_index<76; cluster_index++)
	{
		/* read curent file block table entry */
		cluster_firstphysrec = cluster_lastphysrec+1;
		cluster_lastphysrec = get_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index);
		if (cluster_lastphysrec >= physrecnum)
			break;
	}
	if (cluster_index == 76)
		/* if not found, the file is corrupt */
		return IMGTOOLERR_CORRUPTIMAGE;
	/* read base sector address for this cluster */
	cluster_base_physrec = get_fdr_cluster_basephysrec(dsk_file->l2_img, &dsk_file->fdr, cluster_index);

	/* return absolute physrec address */
	* out_physrec = cluster_base_physrec + (physrecnum - cluster_firstphysrec);
	return 0;
}

/*
	compute the absolute physrec address for a 256-byte physical record in a file

	l2_img: image where the file is located
*/
static int win_file_physrec_to_abs_physrec(ti99_lvl2_fileref_win *dsk_file, int physrecnum, int *out_physrec)
{
	int cluster_lastphysrec;

	/* check parameter */
	if (physrecnum >= ((((unsigned) get_UINT16BE(dsk_file->curfdr.xinfo) << 4) & 0xf0000) | get_UINT16BE(dsk_file->curfdr.secsused)))
		return IMGTOOLERR_UNEXPECTED;

	/* search for the cluster in the data chain pointers array */
	if (physrecnum < dsk_file->cur_cluster_pos)
	{
		while (physrecnum < dsk_file->cur_cluster_pos)
		{
			if (dsk_file->cur_cluster_index != 0)
				dsk_file->cur_cluster_index--;
			else
			{
				if (get_UINT16BE(dsk_file->curfdr.prev_FDR_AU) == 0)
					return IMGTOOLERR_CORRUPTIMAGE;
				else
					dsk_file->curfdr_physrec = get_UINT16BE(dsk_file->curfdr.prev_FDR_AU) * dsk_file->l2_img->AUformat.physrecsperAU
												+ ((get_UINT16BE(dsk_file->curfdr.xinfo) >> 4) & 0xf);
				if (read_absolute_physrec(& dsk_file->l2_img->l1_img, dsk_file->curfdr_physrec, &dsk_file->curfdr))
					return IMGTOOLERR_READERROR;
				dsk_file->cur_cluster_index = 53;
			}
			dsk_file->cur_cluster_pos -= dsk_file->l2_img->AUformat.physrecsperAU *
						(get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][1])
						- get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][0])
						+ 1);
		}
	}
	else /*if (physrecnum >= dsk_file->cur_cluster_pos)*/
	{
		cluster_lastphysrec = dsk_file->cur_cluster_pos + dsk_file->l2_img->AUformat.physrecsperAU *
								(get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][1])
								- get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][0])
								+ 1);
		while (physrecnum >= cluster_lastphysrec)
		{
			if (dsk_file->cur_cluster_index != 53)
				dsk_file->cur_cluster_index++;
			else
			{
				if (get_UINT16BE(dsk_file->curfdr.next_FDR_AU) == 0)
					return IMGTOOLERR_CORRUPTIMAGE;
				else
					dsk_file->curfdr_physrec = get_UINT16BE(dsk_file->curfdr.next_FDR_AU) * dsk_file->l2_img->AUformat.physrecsperAU
												+ (get_UINT16BE(dsk_file->curfdr.xinfo) & 0xf);
				if (read_absolute_physrec(& dsk_file->l2_img->l1_img, dsk_file->curfdr_physrec, &dsk_file->curfdr))
					return IMGTOOLERR_READERROR;
				dsk_file->cur_cluster_index = 0;
			}
			dsk_file->cur_cluster_pos = cluster_lastphysrec;
			if (get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][0]) == 0)
				return IMGTOOLERR_CORRUPTIMAGE;
			cluster_lastphysrec = dsk_file->cur_cluster_pos + dsk_file->l2_img->AUformat.physrecsperAU *
									(get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][1])
									- get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][0])
									+ 1);
		}
	}

	/* return absolute physrec address */
	* out_physrec = get_UINT16BE(dsk_file->curfdr.clusters[dsk_file->cur_cluster_index][0]) * dsk_file->l2_img->AUformat.physrecsperAU + (physrecnum - dsk_file->cur_cluster_pos);
	return 0;
}

/*
	read a 256-byte physical record from a file
*/
static int read_file_physrec(ti99_lvl2_fileref *l2_file, int physrecnum, void *dest)
{
	int errorcode;
	int physrec;

	switch (l2_file->type)
	{
	case L2F_DSK:
		/* compute absolute physrec address */
		errorcode = dsk_file_physrec_to_abs_physrec(&l2_file->u.dsk, physrecnum, & physrec);
		if (errorcode)
			return errorcode;
		/* read physrec */
		if (read_absolute_physrec(& l2_file->u.dsk.l2_img->l1_img, physrec, dest))
			return IMGTOOLERR_READERROR;
		break;

	case L2F_WIN:
		/* compute absolute physrec address */
		errorcode = win_file_physrec_to_abs_physrec(&l2_file->u.win, physrecnum, & physrec);
		if (errorcode)
			return errorcode;
		/* read physrec */
		if (read_absolute_physrec(& l2_file->u.win.l2_img->l1_img, physrec, dest))
			return IMGTOOLERR_READERROR;
		break;

	case L2F_TIFILES:
		/* seek to physrec */
		if (stream_seek(l2_file->u.tifiles.file_handle, 128+256*physrecnum, SEEK_SET))
			return IMGTOOLERR_READERROR;
		/* read it */
		if (stream_read(l2_file->u.tifiles.file_handle, dest, 256) != 256)
			return IMGTOOLERR_READERROR;
		break;
	}

	return 0;
}

/*
	read a 256-byte physical record from a file
*/
static int write_file_physrec(ti99_lvl2_fileref *l2_file, int physrecnum, const void *src)
{
	int errorcode;
	int physrec;

	switch (l2_file->type)
	{
	case L2F_DSK:
		/* compute absolute physrec address */
		errorcode = dsk_file_physrec_to_abs_physrec(&l2_file->u.dsk, physrecnum, & physrec);
		if (errorcode)
			return errorcode;
		/* write physrec */
		if (write_absolute_physrec(& l2_file->u.dsk.l2_img->l1_img, physrec, src))
			return IMGTOOLERR_WRITEERROR;
		break;

	case L2F_WIN:
		/* compute absolute physrec address */
		errorcode = win_file_physrec_to_abs_physrec(&l2_file->u.win, physrecnum, & physrec);
		if (errorcode)
			return errorcode;
		/* write physrec */
		if (write_absolute_physrec(& l2_file->u.win.l2_img->l1_img, physrec, src))
			return IMGTOOLERR_WRITEERROR;
		break;

	case L2F_TIFILES:
		/* seek to physrec */
		if (stream_seek(l2_file->u.tifiles.file_handle, 128+256*physrecnum, SEEK_SET))
			return IMGTOOLERR_WRITEERROR;
		/* write it */
		if (stream_write(l2_file->u.tifiles.file_handle, src, 256) != 256)
			return IMGTOOLERR_WRITEERROR;
		break;
	}

	return 0;
}

static UINT8 get_file_flags(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.flags;
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.flags;
		break;

	case L2F_TIFILES:
		reply = l2_file->u.tifiles.hdr.flags;
		break;
	}

	return reply;
}

static void set_file_flags(ti99_lvl2_fileref *l2_file, UINT8 data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		l2_file->u.dsk.fdr.flags = data;
		break;

	case L2F_WIN:
		l2_file->u.win.curfdr.flags = data;
		break;

	case L2F_TIFILES:
		l2_file->u.tifiles.hdr.flags = data;
		break;
	}
}

static UINT8 get_file_recspersec(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.recspersec;
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.recspersec;
		break;

	case L2F_TIFILES:
		reply = l2_file->u.tifiles.hdr.recspersec;
		break;
	}

	return reply;
}

static void set_file_recspersec(ti99_lvl2_fileref *l2_file, UINT8 data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		l2_file->u.dsk.fdr.recspersec = data;
		break;

	case L2F_WIN:
		l2_file->u.win.curfdr.recspersec = data;
		break;

	case L2F_TIFILES:
		l2_file->u.tifiles.hdr.recspersec = data;
		break;
	}
}

static unsigned get_file_secsused(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = get_UINT16BE(l2_file->u.dsk.fdr.secsused);
		break;

	case L2F_WIN:
		reply = (((unsigned) get_UINT16BE(l2_file->u.win.curfdr.xinfo) << 4) & 0xf0000)
				| get_UINT16BE(l2_file->u.win.curfdr.secsused);
		break;

	case L2F_TIFILES:
		reply = get_UINT16BE(l2_file->u.tifiles.hdr.secsused);
		break;
	}

	return reply;
}

static int set_file_secsused(ti99_lvl2_fileref *l2_file, unsigned data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		if (data >= 65536)
			return IMGTOOLERR_UNIMPLEMENTED;
		set_UINT16BE(&l2_file->u.dsk.fdr.secsused, data);
		break;

	case L2F_WIN:
		set_UINT16BE(&l2_file->u.win.curfdr.xinfo, (get_UINT16BE(l2_file->u.win.curfdr.xinfo) & 0x0fff)
													| ((data >> 4) & 0xf000));
		set_UINT16BE(&l2_file->u.win.curfdr.secsused, data & 0xffff);
		break;

	case L2F_TIFILES:
		if (data >= 65536)
			return IMGTOOLERR_UNIMPLEMENTED;
		set_UINT16BE(&l2_file->u.tifiles.hdr.secsused, data);
		break;
	}

	return 0;
}

static UINT8 get_file_eof(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.eof;
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.eof;
		break;

	case L2F_TIFILES:
		reply = l2_file->u.tifiles.hdr.eof;
		break;
	}

	return reply;
}

static void set_file_eof(ti99_lvl2_fileref *l2_file, UINT8 data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		l2_file->u.dsk.fdr.eof = data;
		break;

	case L2F_WIN:
		l2_file->u.win.curfdr.eof = data;
		break;

	case L2F_TIFILES:
		l2_file->u.tifiles.hdr.eof = data;
		break;
	}
}

static UINT16 get_file_reclen(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.reclen;
		if ((reply == 0) && (! (l2_file->u.dsk.fdr.flags & (fdr99_f_program /*| fdr99_f_var*/))))
			reply = get_UINT16BE(l2_file->u.dsk.fdr.xreclen);
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.reclen;
		if ((reply == 0) && (! (l2_file->u.win.curfdr.flags & (fdr99_f_program /*| fdr99_f_var*/))))
			reply = get_UINT16BE(l2_file->u.win.curfdr.xreclen);
		break;

	case L2F_TIFILES:
		reply = l2_file->u.tifiles.hdr.reclen;
		break;
	}

	return reply;
}

static int set_file_reclen(ti99_lvl2_fileref *l2_file, UINT16 data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		if (data < 256)
		{
			l2_file->u.dsk.fdr.reclen = data;
			set_UINT16BE(&l2_file->u.dsk.fdr.xreclen, 0);
		}
		else
		{
			l2_file->u.dsk.fdr.reclen = 0;
			set_UINT16BE(&l2_file->u.dsk.fdr.xreclen, data);
		}
		break;

	case L2F_WIN:
		if (data < 256)
		{
			l2_file->u.win.curfdr.reclen = data;
			set_UINT16BE(&l2_file->u.win.curfdr.xreclen, 0);
		}
		else
		{
			l2_file->u.win.curfdr.reclen = 0;
			set_UINT16BE(&l2_file->u.win.curfdr.xreclen, data);
		}
		break;

	case L2F_TIFILES:
		if (data >= 256)
			return IMGTOOLERR_UNIMPLEMENTED;
		l2_file->u.tifiles.hdr.reclen = data;
		break;
	}

	return 0;
}

static unsigned get_file_fixrecs(ti99_lvl2_fileref *l2_file)
{
	int reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = get_UINT16LE(l2_file->u.dsk.fdr.fixrecs);
		break;

	case L2F_WIN:
		reply = (((unsigned) get_UINT16BE(l2_file->u.win.curfdr.xinfo) << 8) & 0xf0000)
				| get_UINT16LE(l2_file->u.win.curfdr.fixrecs);
		break;

	case L2F_TIFILES:
		reply = get_UINT16BE(l2_file->u.tifiles.hdr.fixrecs);
		break;
	}

	return reply;
}

static int set_file_fixrecs(ti99_lvl2_fileref *l2_file, unsigned data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		if (data >= 65536)
			return IMGTOOLERR_UNIMPLEMENTED;
		set_UINT16LE(&l2_file->u.dsk.fdr.fixrecs, data);
		break;

	case L2F_WIN:
		set_UINT16BE(&l2_file->u.win.curfdr.xinfo, (get_UINT16BE(l2_file->u.win.curfdr.xinfo) & 0xf0ff)
													| ((data >> 8) & 0x0f00));
		set_UINT16LE(&l2_file->u.win.curfdr.fixrecs, data & 0xffff);
		break;

	case L2F_TIFILES:
		if (data >= 65536)
			return IMGTOOLERR_UNIMPLEMENTED;
		set_UINT16BE(&l2_file->u.tifiles.hdr.fixrecs, data);
		break;
	}

	return 0;
}

static ti99_date_time get_file_creation_date(ti99_lvl2_fileref *l2_file)
{
	ti99_date_time reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.creation;
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.creation;
		break;

	case L2F_TIFILES:
		memset(&reply, 0, sizeof(reply));
		break;
	}

	return reply;
}

static void set_file_creation_date(ti99_lvl2_fileref *l2_file, ti99_date_time data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		l2_file->u.dsk.fdr.creation = data;
		break;

	case L2F_WIN:
		l2_file->u.win.curfdr.creation = data;
		break;

	case L2F_TIFILES:
		break;
	}
}

static ti99_date_time get_file_update_date(ti99_lvl2_fileref *l2_file)
{
	ti99_date_time reply;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = l2_file->u.dsk.fdr.update;
		break;

	case L2F_WIN:
		reply = l2_file->u.win.curfdr.update;
		break;

	case L2F_TIFILES:
		memset(&reply, 0, sizeof(reply));
		break;
	}

	return reply;
}

static void set_file_update_date(ti99_lvl2_fileref *l2_file, ti99_date_time data)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		l2_file->u.dsk.fdr.update = data;
		break;

	case L2F_WIN:
		l2_file->u.win.curfdr.update = data;
		break;

	case L2F_TIFILES:
		break;
	}
}

static ti99_date_time current_data_time(void)
{
	/* All these functions should be ANSI */
	time_t cur_time = time(NULL);
	struct tm expanded_time = *localtime(& cur_time);
	ti99_date_time reply;

	reply.time_MSB = (expanded_time.tm_hour << 3) | (expanded_time.tm_min >> 3);
	reply.time_LSB = (expanded_time.tm_min << 5) | (expanded_time.tm_sec >> 1);
	reply.date_MSB = ((expanded_time.tm_year % 100) << 1) | ((expanded_time.tm_mon+1) >> 3);
	reply.date_LSB = ((expanded_time.tm_mon+1) << 5) | expanded_time.tm_mday;

	return reply;
}

#if 0
#pragma mark -
#pragma mark LEVEL 3 DISK ROUTINES
#endif

/*
	Level 3 implements files as a succession of logical records.

	There are three types of files:
	* program files that are not implemented at level 3 (no logical record)
	* files with fixed-size records (random-access files)
	* files with variable-size records (sequential-access)
*/

typedef struct ti99_lvl3_fileref
{
	ti99_lvl2_fileref l2_file;

	int cur_log_rec;
	int cur_phys_rec;
	int cur_pos_in_phys_rec;
} ti99_lvl3_fileref;

/*
	Open a file on level 3.

	To open a file on level 3, you must open (or create) the file on level 2,
	then pass the file reference to open_file_lvl3.
*/
static int open_file_lvl3(ti99_lvl3_fileref *l3_file)
{
	l3_file->cur_log_rec = 0;
	l3_file->cur_phys_rec = 0;
	l3_file->cur_pos_in_phys_rec = 0;

	/*if ()*/

	return 0;
}

/*
	Test a file for EOF
*/
static int is_eof(ti99_lvl3_fileref *l3_file)
{
	int flags = get_file_flags(&l3_file->l2_file);
	int secsused = get_file_secsused(&l3_file->l2_file);
	int eof = get_file_eof(&l3_file->l2_file);

	if (flags & fdr99_f_var)
	{
		return (l3_file->cur_phys_rec >= secsused);
	}
	else
	{
		return ((l3_file->cur_phys_rec >= secsused)
				|| ((l3_file->cur_phys_rec == (secsused-1))
					&& (l3_file->cur_pos_in_phys_rec >= (eof ? eof : 256))));
	}
}

/*
	Read next record from a file
*/
static int read_next_record(ti99_lvl3_fileref *l3_file, void *dest, int *out_reclen)
{
	int errorcode;
	UINT8 physrec_buf[256];
	int reclen;
	int flags = get_file_flags(&l3_file->l2_file);
	int secsused = get_file_secsused(&l3_file->l2_file);
	int eof = get_file_eof(&l3_file->l2_file);

	if (flags & fdr99_f_program)
	{
		/* program files have no level-3 record */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if (flags & fdr99_f_var)
	{
		/* variable-lenght records */
		if (is_eof(l3_file))
			return IMGTOOLERR_UNEXPECTED;
		errorcode = read_file_physrec(&l3_file->l2_file, l3_file->cur_phys_rec, physrec_buf);
		if (errorcode)
			return errorcode;
		/* read reclen */
		reclen = physrec_buf[l3_file->cur_pos_in_phys_rec];
		/* check integrity */
		if ((reclen == 0xff) || (reclen > get_file_reclen(&l3_file->l2_file))
				|| ((l3_file->cur_pos_in_phys_rec + 1 + reclen) > 256))
			return IMGTOOLERR_CORRUPTIMAGE;
		/* copy to buffer */
		memcpy(dest, physrec_buf + l3_file->cur_pos_in_phys_rec + 1, reclen);
		l3_file->cur_pos_in_phys_rec += reclen + 1;
		/* skip to next physrec if needed */
		if ((l3_file->cur_pos_in_phys_rec == 256)
				|| (physrec_buf[l3_file->cur_pos_in_phys_rec] == 0xff))
		{
			l3_file->cur_pos_in_phys_rec = 0;
			l3_file->cur_phys_rec++;
		}
		(* out_reclen) = reclen;
	}
	else
	{
		/* fixed len records */
		reclen = get_file_reclen(&l3_file->l2_file);
		if (is_eof(l3_file))
			return IMGTOOLERR_UNEXPECTED;
		if ((l3_file->cur_pos_in_phys_rec + reclen) > 256)
		{
			l3_file->cur_pos_in_phys_rec = 0;
			l3_file->cur_phys_rec++;
		}
		if ((l3_file->cur_phys_rec >= secsused)
				|| ((l3_file->cur_phys_rec == (secsused-1))
					&& ((l3_file->cur_pos_in_phys_rec + reclen) >= (eof ? eof : 256))))
			return IMGTOOLERR_CORRUPTIMAGE;
		errorcode = read_file_physrec(&l3_file->l2_file, l3_file->cur_phys_rec, physrec_buf);
		if (errorcode)
			return errorcode;
		memcpy(dest, physrec_buf + l3_file->cur_pos_in_phys_rec, reclen);
		l3_file->cur_pos_in_phys_rec += reclen;
		if (l3_file->cur_pos_in_phys_rec == 256)
		{
			l3_file->cur_pos_in_phys_rec = 0;
			l3_file->cur_phys_rec++;
		}
		(* out_reclen) = reclen;
	}

	return 0;
}

#if 0
#pragma mark -
#pragma mark IMGTOOL IMPLEMENTATION
#endif

/*
	ti99 catalog iterator, used when imgtool reads the catalog
*/
typedef struct dsk_iterator
{
	IMAGEENUM base;
	ti99_lvl2_imgref *image;
	int level;
	int listing_subdirs;		/* true if we are listing subdirectories at current level */
	int index[/*MAX_DIR_LEVEL*/2];	/* current index in the disk catalog */
	ti99_catalog *cur_catalog;	/* current catalog */
} dsk_iterator;

typedef struct win_iterator
{
	IMAGEENUM base;
	ti99_lvl2_imgref *image;
	int level;
	int listing_subdirs;		/* true if we are listing subdirectories at current level */
	int index[MAX_DIR_LEVEL];	/* current index in the disk catalog */
	ti99_catalog catalog[MAX_DIR_LEVEL];	/* current catalog */
} win_iterator;


static int ti99_image_init_mess(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_v9t9(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_pc99_fm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_pc99_mfm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_win_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void ti99_image_exit(IMAGE *img);
static void ti99_image_info(IMAGE *img, char *string, const int len);
static int ti99_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int ti99_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void ti99_dsk_image_closeenum(IMAGEENUM *enumeration);
static int ti99_win_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int ti99_win_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void ti99_win_image_closeenum(IMAGEENUM *enumeration);
static size_t ti99_image_freespace(IMAGE *img);
static int ti99_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int ti99_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int ti99_image_deletefile(IMAGE *img, const char *fname);
static int ti99_image_create_mess(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);
static int ti99_image_create_v9t9(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int ti99_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int ti99_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

static struct OptionTemplate ti99_createopts[] =
{
	{ "label",	"Volume name", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,	0,	NULL},
	{ "sides",	NULL, IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	2,	"2" },
	{ "tracks",	NULL, IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	80,	"40" },
	{ "sectors",	"1->9 for SD, 1->18 for DD, 1->36 for HD", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	36,	"18" },
	{ "protection",	"0 for normal, 1 for protected", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	0,	1,	"0" },
	{ "density",	"0 for auto-detect, 1 for SD, 2 for DD, 3 for HD", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	0,	3,	"0" },
	{ NULL, NULL, 0, 0, 0, 0 }
};

enum
{
	ti99_createopts_volname = 0,
	ti99_createopts_sides = 1,
	ti99_createopts_tracks = 2,
	ti99_createopts_sectors = 3,
	ti99_createopts_protection = 4,
	ti99_createopts_density = 5
};

IMAGEMODULE(
	ti99_old,
	"TI99 Diskette (old MESS format)",	/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	ti99_image_init_mess,			/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_dsk_image_beginenum,			/* begin enumeration */
	ti99_dsk_image_nextenum,			/* enumerate next */
	ti99_dsk_image_closeenum,			/* close enumeration */
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	ti99_image_deletefile,			/* delete file */
	ti99_image_create_mess,			/* create image */
	ti99_read_sector,
	ti99_write_sector,
	NULL,							/* file options */
	ti99_createopts					/* create options */
)

IMAGEMODULE(
	v9t9,
	"TI99 Diskette (V9T9 format)",	/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	ti99_image_init_v9t9,			/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_dsk_image_beginenum,			/* begin enumeration */
	ti99_dsk_image_nextenum,			/* enumerate next */
	ti99_dsk_image_closeenum,			/* close enumeration */
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	ti99_image_deletefile,			/* delete file */
	ti99_image_create_v9t9,			/* create image */
	ti99_read_sector,
	ti99_write_sector,
	NULL,							/* file options */
	ti99_createopts					/* create options */
)

IMAGEMODULE(
	pc99fm,
	"TI99 Diskette (PC99 FM format)",	/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	ti99_image_init_pc99_fm,		/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_dsk_image_beginenum,			/* begin enumeration */
	ti99_dsk_image_nextenum,			/* enumerate next */
	ti99_dsk_image_closeenum,			/* close enumeration */
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	ti99_image_deletefile,			/* delete file */
	/*ti99_image_create_pc99_fm*/NULL,	/* create image */
	ti99_read_sector,
	ti99_write_sector,
	NULL,							/* file options */
	ti99_createopts					/* create options */
)

IMAGEMODULE(
	pc99mfm,
	"TI99 Diskette (PC99 MFM format)",	/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	ti99_image_init_pc99_mfm,		/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_dsk_image_beginenum,			/* begin enumeration */
	ti99_dsk_image_nextenum,			/* enumerate next */
	ti99_dsk_image_closeenum,			/* close enumeration */
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	ti99_image_deletefile,			/* delete file */
	/*ti99_image_create_pc99_mfm*/NULL,	/* create image */
	ti99_read_sector,
	ti99_write_sector,
	NULL,							/* file options */
	ti99_createopts					/* create options */
)

IMAGEMODULE(
	ti99hd,
	"TI99 Harddisk",				/* human readable name */
	"hd",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	ti99_win_image_init,			/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_win_image_beginenum,		/* begin enumeration */
	ti99_win_image_nextenum,		/* enumerate next */
	ti99_win_image_closeenum,		/* close enumeration */
	/*ti99_image_freespace*/NULL,	/* free space on image */
	ti99_image_readfile,			/* read file */
	/*ti99_image_writefile*/NULL,	/* write file */
	/*ti99_image_deletefile*/NULL,	/* delete file */
	/*ti99_image_create_mess*/NULL,	/* create image */
	/*ti99_read_sector*/NULL,
	/*ti99_write_sector*/NULL,
	NULL,							/* file options */
	/*ti99_createopts*/NULL			/* create options */
)

/*
	Open a file as a ti99_image (common code).
*/
static int ti99_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg, ti99_img_format img_format)
{
	ti99_lvl2_imgref *image;
	int reply;
	int totphysrecs;
	int fdir_physrec;
	int i;


	image = malloc(sizeof(ti99_lvl2_imgref));
	* (ti99_lvl2_imgref **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(ti99_lvl2_imgref));
	image->base.module = mod;

	/* open disk image at level 1 */
	reply = open_image_lvl1(f, img_format, & image->l1_img, & image->u.dsk.vib);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return reply;
	}

	/* open disk image at level 2 */
	image->type = L2I_DSK;

	/* first compute AU size and number of AUs */
	totphysrecs = get_UINT16BE(image->u.dsk.vib.totphysrecs);

	image->AUformat.physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < image->AUformat.physrecsperAU; i <<= 1)
		;
	image->AUformat.physrecsperAU = i;
	image->AUformat.totAUs = totphysrecs / image->AUformat.physrecsperAU;

	/* read and check main volume catalog */
	reply = dsk_read_catalog(image, 1, &image->u.dsk.catalog);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return reply;
	}

	/* read and check subdirectory catalogs */
	/* Note that the reserved areas used for HFDC subdirs may be used for other
	purposes by other FDRs, so, if we get any error, we will assume there is no
	subdir after all... */
	image->u.dsk.catalog.num_subdirs = 0;
	for (i=0; i<3; i++)
	{
		fdir_physrec = get_UINT16BE(image->u.dsk.vib.subdir[i].fdir_physrec);
		if ((! memcmp(image->u.dsk.vib.subdir[i].name, "\0\0\0\0\0\0\0\0\0\0", 10))
				|| (! memcmp(image->u.dsk.vib.subdir[i].name, "          ", 10)))
		{
			/* name is empty: fine with us unless there is a fdir pointer */
			if (fdir_physrec != 0)
			{
				image->u.dsk.catalog.num_subdirs = 0;
				break;
			}
		}
		else if (check_fname(image->u.dsk.vib.subdir[i].name))
		{
			/* name is invalid: this is not an HFDC format floppy */
			image->u.dsk.catalog.num_subdirs = 0;
			break;
		}
		else
		{
			/* there is a non-empty name */
			if ((fdir_physrec == 0) || (fdir_physrec >= totphysrecs))
			{
				/* error: fdir pointer is invalid or NULL */
				image->u.dsk.catalog.num_subdirs = 0;
				break;
			}
			/* fill in descriptor fields */
			image->u.dsk.catalog.subdirs[image->u.dsk.catalog.num_subdirs].fdir_physrec = fdir_physrec;
			memcpy(image->u.dsk.catalog.subdirs[image->u.dsk.catalog.num_subdirs].dirname, image->u.dsk.vib.subdir[i].name, 10);
			reply = dsk_read_catalog(image, fdir_physrec, &image->u.dsk.subdir_catalog[image->u.dsk.catalog.num_subdirs]);
			if (reply)
			{
				/* error: invalid fdir */
				image->u.dsk.catalog.num_subdirs = 0;
				break;
			}
			/* found valid subdirectory: increment subdir count */
			image->u.dsk.catalog.num_subdirs++;
		}
	}

	/* initialize default data_offset */
	image->u.dsk.data_offset = 32+2;

	return 0;
}

/*
	Open a file as a ti99_image (MESS format).
*/
static int ti99_image_init_mess(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	return ti99_image_init(mod, f, outimg, if_mess);
}

/*
	Open a file as a ti99_image (V9T9 format).
*/
static int ti99_image_init_v9t9(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	return ti99_image_init(mod, f, outimg, if_v9t9);
}

/*
	Open a file as a ti99_image (PC99 FM format).
*/
static int ti99_image_init_pc99_fm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	return ti99_image_init(mod, f, outimg, if_pc99_fm);
}

/*
	Open a file as a ti99_image (PC99 MFM format).
*/
static int ti99_image_init_pc99_mfm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	return ti99_image_init(mod, f, outimg, if_pc99_mfm);
}

/*
	Open a file as a ti99_image (harddisk format).
*/
static int ti99_win_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	ti99_lvl2_imgref *image;
	int reply;


	image = malloc(sizeof(ti99_lvl2_imgref));
	* (ti99_lvl2_imgref **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(ti99_lvl2_imgref));
	image->base.module = mod;

	/* open disk image at level 1 */
	reply = open_image_lvl1(f, if_harddisk, & image->l1_img, NULL);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return reply;
	}

	/* open disk image at level 2 */
	image->type = L2I_WIN;

	/* read VIB */
	reply = read_absolute_physrec(&image->l1_img, 0, &image->u.win.vib);
	if (reply)
		return reply;

	/* guess VIB version */
	image->u.win.vib_version = memcmp(image->u.win.vib.u.vib_v1.id, "WIN", 3) ? win_vib_v2 : win_vib_v1;

	/* extract AU size and number of AUs */
	image->AUformat.physrecsperAU = ((get_UINT16BE(image->u.win.vib.params) >> 12) & 0xf) + 1;
	image->AUformat.totAUs = get_UINT16BE(image->u.win.vib.totAUs);

	/* extract data_offset */
	switch (image->u.win.vib_version)
	{
	case win_vib_v1:
		image->u.win.data_offset = 64;
		break;

	case win_vib_v2:
		image->u.win.data_offset = image->u.win.vib.u.vib_v2.res_AUs * 64;
		break;
	}

	return 0;
}

/*
	close a ti99_image
*/
static void ti99_image_exit(IMAGE *img)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;

	close_image_lvl1(&image->l1_img);

	free(image);
}

/*
	get basic information on a ti99_image

	Currently returns the volume name
*/
static void ti99_image_info(IMAGE *img, char *string, const int len)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	char vol_name[11];

	switch (image->type)
	{
	case L2I_DSK:
		fname_to_str(vol_name, image->u.dsk.vib.name, 11);
		break;

	case L2I_WIN:
		fname_to_str(vol_name, image->u.win.vib.name, 11);
		break;
	}

	snprintf(string, len, "%s", vol_name);
}

/*
	Open the disk catalog for enumeration 
*/
static int ti99_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	dsk_iterator *iter;


	iter = malloc(sizeof(dsk_iterator));
	*((dsk_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->image = image;
	iter->level = 0;
	iter->listing_subdirs = 1;
	iter->index[0] = 0;
	iter->cur_catalog = &iter->image->u.dsk.catalog;

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int ti99_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	dsk_iterator *iter = (dsk_iterator*) enumeration;
	dsk_fdr fdr;
	int reply;
	int fdr_physrec;


	ent->corrupt = 0;
	ent->eof = 0;

	/* iterate through catalogs to next file or dir entry */
	while ((iter->level >= 0)
			&& (iter->index[iter->level] >= (iter->listing_subdirs
												? iter->cur_catalog->num_subdirs
												: iter->cur_catalog->num_files)))
	{
		if (iter->listing_subdirs)
		{
			iter->listing_subdirs = 0;
			iter->index[iter->level] = 0;
		}
		else
		{
			iter->listing_subdirs = 1;
			if (! iter->level)
				iter->level = -1;
			else
			{
				iter->level = 0;
				iter->index[0]++;
				iter->cur_catalog = &iter->image->u.dsk.catalog;
			}
		}
	}

	if (iter->level < 0)
	{
		ent->eof = 1;
	}
	else
	{
		if (iter->listing_subdirs)
		{
			fname_to_str(ent->fname, iter->image->u.dsk.catalog.subdirs[iter->index[iter->level]].dirname, ent->fname_len);

			/* set type of DIR */
			snprintf(ent->attr, ent->attr_len, "DIR");

			/* len in sectors */
			ent->filesize = 1;

			/* recurse subdirectory */
			iter->listing_subdirs = 0;	/* no need to list subdirs as there is no subdirs in DSK */
			iter->level = 1;
			iter->cur_catalog = &iter->image->u.dsk.subdir_catalog[iter->index[0]];
			iter->index[iter->level] = 0;
		}
		else
		{
			fdr_physrec = iter->cur_catalog->files[iter->index[iter->level]].fdr_physrec;
			reply = read_absolute_physrec(& iter->image->l1_img, fdr_physrec, & fdr);
			if (reply)
				return IMGTOOLERR_READERROR;
#if 0
			fname_to_str(ent->fname, fdr.name, ent->fname_len);
#else
			{
				char buf[11];

				if (ent->fname_len)
				{
					ent->fname[0] = '\0';
					if (iter->level)
					{
						fname_to_str(ent->fname, iter->image->u.dsk.catalog.subdirs[iter->index[0]].dirname, ent->fname_len);
						strncat(ent->fname, ".", ent->fname_len);
					}
					fname_to_str(buf, fdr.name, 11);
					strncat(ent->fname, buf, ent->fname_len);
				}
			}
#endif
			/* parse flags */
			if (fdr.flags & fdr99_f_program)
				snprintf(ent->attr, ent->attr_len, "PGM%s",
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			else
				snprintf(ent->attr, ent->attr_len, "%c/%c %d%s",
							(fdr.flags & fdr99_f_int) ? 'I' : 'D',
							(fdr.flags & fdr99_f_var) ? 'V' : 'F',
							fdr.reclen,
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			/* len in sectors */
			ent->filesize = get_UINT16BE(fdr.secsused);

			iter->index[iter->level]++;
		}
	}

	return 0;
}

/*
	Free enumerator
*/
static void ti99_dsk_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Open the disk catalog for enumeration 
*/
static int ti99_win_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	win_iterator *iter;
	int errorcode;


	iter = malloc(sizeof(win_iterator));
	*((win_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->image = image;
	iter->level = 0;
	iter->listing_subdirs = 1;
	iter->index[0] = 0;
	errorcode = win_read_catalog(image, 0, &iter->catalog[0]);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int ti99_win_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	win_iterator *iter = (win_iterator*) enumeration;
	int fdr_physrec;
	dsk_fdr fdr;
	int reply;
	int i;


	ent->corrupt = 0;
	ent->eof = 0;

	/* iterate through catalogs to next file or dir entry */
	while ((iter->level >= 0)
			&& (iter->index[iter->level] >= (iter->listing_subdirs
												? iter->catalog[iter->level].num_subdirs
												: iter->catalog[iter->level].num_files)))
	{
		if (iter->listing_subdirs)
		{
			iter->listing_subdirs = 0;
			iter->index[iter->level] = 0;
		}
		else
		{
			iter->listing_subdirs = 1;
			iter->level--;
			iter->index[iter->level]++;
		}
	}

	if (iter->level < 0)
	{
		ent->eof = 1;
	}
	else
	{
		if (iter->listing_subdirs)
		{
#if 0
			fname_to_str(ent->fname, iter->catalog[iter->level].subdirs[iter->index[iter->level]].dirname, ent->fname_len);
#else
			{
				char buf[11];

				if (ent->fname_len)
				{
					ent->fname[0] = '\0';
					for (i=0; i<iter->level; i++)
					{
						fname_to_str(buf, iter->catalog[i].subdirs[iter->index[i]].dirname, 11);
						strncat(ent->fname, buf, ent->fname_len);
						strncat(ent->fname, ".", ent->fname_len);
					}
					fname_to_str(buf, iter->catalog[iter->level].subdirs[iter->index[iter->level]].dirname, 11);
					strncat(ent->fname, buf, ent->fname_len);
				}
			}
#endif

			/* set type of DIR */
			snprintf(ent->attr, ent->attr_len, "DIR");

			/* len in sectors */
			ent->filesize = 1;

			/* recurse subdirectory */
			/*iter->listing_subdirs = 1;*/
			iter->level++;
			iter->index[iter->level] = 0;
			reply = win_read_catalog(iter->image, iter->catalog[iter->level-1].subdirs[iter->index[iter->level-1]].fdir_physrec, &iter->catalog[iter->level]);
			if (reply)
			{
				ent->corrupt = 1;
				return reply;
			}
		}
		else
		{
			fdr_physrec = iter->catalog[iter->level].files[iter->index[iter->level]].fdr_physrec*iter->image->AUformat.physrecsperAU;
			reply = read_absolute_physrec(& iter->image->l1_img, fdr_physrec, & fdr);
			if (reply)
				return IMGTOOLERR_READERROR;
#if 0
			fname_to_str(ent->fname, iter->catalog[iter->level].files[iter->index[iter->level]].fname, ent->fname_len);
#else
			{
				char buf[11];

				if (ent->fname_len)
				{
					ent->fname[0] = '\0';
					for (i=0; i<iter->level; i++)
					{
						fname_to_str(buf, iter->catalog[i].subdirs[iter->index[i]].dirname, 11);
						strncat(ent->fname, buf, ent->fname_len);
						strncat(ent->fname, ".", ent->fname_len);
					}
					fname_to_str(buf, iter->catalog[iter->level].files[iter->index[iter->level]].fname, 11);
					strncat(ent->fname, buf, ent->fname_len);
				}
			}
#endif
			/* parse flags */
			if (fdr.flags & fdr99_f_program)
				snprintf(ent->attr, ent->attr_len, "PGM%s",
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			else
				snprintf(ent->attr, ent->attr_len, "%c/%c %d%s",
							(fdr.flags & fdr99_f_int) ? 'I' : 'D',
							(fdr.flags & fdr99_f_var) ? 'V' : 'F',
							fdr.reclen,
							(fdr.flags & fdr99_f_wp) ? " R/O" : "");
			/* len in sectors */
			ent->filesize = get_UINT16BE(fdr.secsused);

			iter->index[iter->level]++;
		}
	}

	return 0;
}

/*
	Free enumerator
*/
static void ti99_win_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in AUs)
*/
static size_t ti99_image_freespace(IMAGE *img)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	size_t freeAUs;
	int i;

	freeAUs = 0;
	for (i=0; i<image->AUformat.totAUs; i++)
	{
		if (! (image->u.dsk.vib.abm[i >> 3] & (1 << (i & 7))))
			freeAUs++;
	}

	return freeAUs;
}

/*
	Extract a file from a ti99_image.  The file is saved in tifile format.
*/
static int ti99_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
#if 1

	/* extract data as TIFILES */
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	ti99_lvl2_fileref src_file;
	ti99_lvl2_fileref dst_file;
	ti99_date_time date_time;
	int secsused;
	int i;
	UINT8 buf[256];
	int errorcode;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	/* open file on TI image */
	if (image->type == L2I_WIN)
		errorcode = open_file_lvl2_win(image, fname, &src_file);
	else
		errorcode = open_file_lvl2_dsk(image, fname, &src_file);
	if (errorcode)
		return errorcode;

	errorcode = new_file_lvl2_tifiles(destf, &dst_file);
	if (errorcode)
		return errorcode;

	/* write TIFILE header */
	/* set up parameters */
	set_file_flags(&dst_file, get_file_flags(&src_file));
	set_file_recspersec(&dst_file, get_file_recspersec(&src_file));
	errorcode = set_file_secsused(&dst_file, get_file_secsused(&src_file));
	if (errorcode)
		return errorcode;
	set_file_eof(&dst_file, get_file_eof(&src_file));
	errorcode = set_file_reclen(&dst_file, get_file_reclen(&src_file));
	if (errorcode)
		return errorcode;
	errorcode = set_file_fixrecs(&dst_file, get_file_fixrecs(&src_file));
	if (errorcode)
		return errorcode;

	date_time = get_file_creation_date(&src_file);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		date_time = current_data_time();*/
	set_file_creation_date(&dst_file, date_time);

	date_time = get_file_update_date(&src_file);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		date_time = current_data_time();*/
	set_file_update_date(&dst_file, date_time);

	if (stream_write(destf, & dst_file.u.tifiles.hdr, 128) != 128)
		return IMGTOOLERR_WRITEERROR;

	/* copy data to TIFILE */
	secsused = get_file_secsused(&src_file);

	for (i=0; i<secsused; i++)
	{
		errorcode = read_file_physrec(& src_file, i, buf);
		if (errorcode)
			return errorcode;

		errorcode = write_file_physrec(& dst_file, i, buf);
		if (errorcode)
			return errorcode;
	}

	return 0;

#else

	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	ti99_lvl3_fileref src_file;
	UINT8 buf[256];
	int reclen;
	char lineend = '\r';
	int errorcode;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	/* open file on TI image */
	if (image->type == L2I_WIN)
		errorcode = open_file_lvl2_win(image, fname, &src_file);
	else
		errorcode = open_file_lvl2_dsk(image, fname, &src_file.l2_file);
	if (errorcode)
		return errorcode;

	errorcode = open_file_lvl3(& src_file);
	if (errorcode)
		return errorcode;

	/* write text data */
	while (! is_eof(& src_file))
	{
		errorcode = read_next_record(& src_file, buf, & reclen);
		if (errorcode)
			return errorcode;
		if (stream_write(destf, buf, reclen) != reclen)
			return IMGTOOLERR_WRITEERROR;
		if (stream_write(destf, &lineend, 1) != 1)
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;

#endif
}

/*
	Add a file to a ti99_image.  The file must be in tifile format.
*/
static int ti99_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	const char *fname;
	char ti_fname[10];
	ti99_lvl2_fileref src_file;
	ti99_lvl2_fileref dst_file;
	ti99_date_time date_time;
	int i;
	int secsused;
	UINT8 buf[256];
	int errorcode;
	int parent_ref_valid, parent_ref;
	ti99_catalog *catalog;


	if (check_fpath(fpath))
		return IMGTOOLERR_BADFILENAME;

	errorcode = dsk_find_catalog_entry(image, fpath, &parent_ref_valid, &parent_ref, NULL, NULL);
	if (errorcode == 0)
	{	/* file already exists: causes an error for now */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if ((! parent_ref_valid) || (errorcode != IMGTOOLERR_FILENOTFOUND))
	{
		return errorcode;
	}

	errorcode = open_file_lvl2_tifiles(sourcef, & src_file);
	if (errorcode)
		return errorcode;

	/* create new file */
	fname = strrchr(fpath, '.');
	if (fname)
		fname++;
	else
		fname = fpath;
	str_to_fname(ti_fname, fname);
	errorcode = new_file_lvl2_dsk(image, parent_ref, ti_fname, &dst_file);
	if (errorcode)
		return errorcode;

	/* set up parameters */
	set_file_flags(&dst_file, get_file_flags(&src_file));
	set_file_recspersec(&dst_file, get_file_recspersec(&src_file));
	errorcode = set_file_secsused(&dst_file, /*get_file_secsused(&src_file)*/0);
	if (errorcode)
		return errorcode;
	set_file_eof(&dst_file, get_file_eof(&src_file));
	errorcode = set_file_reclen(&dst_file, get_file_reclen(&src_file));
	if (errorcode)
		return errorcode;
	errorcode = set_file_fixrecs(&dst_file, get_file_fixrecs(&src_file));
	if (errorcode)
		return errorcode;

	date_time = get_file_creation_date(&src_file);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		date_time = current_data_time();*/
	set_file_creation_date(&dst_file, date_time);

	date_time = get_file_update_date(&src_file);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		date_time = current_data_time();*/
	set_file_update_date(&dst_file, date_time);

	/* alloc data sectors */
	secsused = get_file_secsused(&src_file);
	errorcode = dsk_alloc_file_sectors(&dst_file.u.dsk, secsused);
	if (errorcode)
		return errorcode;

	/* copy data */
	for (i=0; i<secsused; i++)
	{
		if (stream_read(sourcef, buf, 256) != 256)
			return IMGTOOLERR_READERROR;

		errorcode = write_file_physrec(& dst_file, i, buf);
		if (errorcode)
			return errorcode;
	}

	/* write fdr */
	if (write_absolute_physrec(& image->l1_img, dst_file.u.dsk.fdr_physrec, &dst_file.u.dsk.fdr))
		return IMGTOOLERR_WRITEERROR;

	/* update catalog */
	catalog = parent_ref ? &image->u.dsk.subdir_catalog[parent_ref-1] : &image->u.dsk.catalog;
	for (i=0; i<128; i++)
	{
		buf[2*i] = catalog->files[i].fdr_physrec >> 8;
		buf[2*i+1] = catalog->files[i].fdr_physrec & 0xff;
	}
	if (write_absolute_physrec(& image->l1_img, parent_ref ? image->u.dsk.catalog.subdirs[parent_ref-1].fdir_physrec : 1, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_absolute_physrec(& image->l1_img, 0, &image->u.dsk.vib))
		return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Delete a file from a ti99_image.
*/
static int ti99_image_deletefile(IMAGE *img, const char *fname)
{
	ti99_lvl2_imgref *image = (ti99_lvl2_imgref *) img;
	dsk_fdr fdr;
	int i, cluster_index;
	int cur_AU, cluster_lastphysrec;
	int secsused;
	int parent_ref, is_dir, catalog_index;
	int errorcode;
	UINT8 buf[256];
	ti99_catalog *catalog;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	errorcode = dsk_find_catalog_entry(image, fname, NULL, &parent_ref, &is_dir, &catalog_index);
	if (errorcode)
		return errorcode;

	if (is_dir)
	{
		catalog = & image->u.dsk.subdir_catalog[catalog_index];

		if ((catalog->num_files != 0) || (catalog->num_subdirs != 0))
			return IMGTOOLERR_UNIMPLEMENTED;

		catalog = & image->u.dsk.catalog;

		/* free fdir sector */
		cur_AU = catalog->subdirs[catalog_index].fdir_physrec / image->AUformat.physrecsperAU;
		image->u.dsk.vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

		/* delete catalog entry */
		for (i=catalog_index; i<2; i++)
			catalog->subdirs[i] = catalog->subdirs[i+1];
		catalog->subdirs[2].fdir_physrec = 0;
		catalog->num_subdirs--;

		/* update directory in vib */
		for (i=0; i<3; i++)
		{
			memcpy(image->u.dsk.vib.subdir[i].name, catalog->subdirs[i].dirname, 10);
			set_UINT16BE(&image->u.dsk.vib.subdir[i].fdir_physrec, catalog->subdirs[i].fdir_physrec);
		}

		/* write vib (bitmap+directory) */
		if (write_absolute_physrec(& image->l1_img, 0, &image->u.dsk.vib))
			return IMGTOOLERR_WRITEERROR;

		return IMGTOOLERR_UNIMPLEMENTED;
	}
	else
	{
		catalog = parent_ref ? &image->u.dsk.subdir_catalog[parent_ref-1] : &image->u.dsk.catalog;

		if (read_absolute_physrec(& image->l1_img, catalog->files[catalog_index].fdr_physrec, &fdr))
			return IMGTOOLERR_READERROR;

		/* free data sectors */
		secsused = get_UINT16BE(fdr.secsused);

		i = 0;
		cluster_index = 0;
		while (i<secsused)
		{
			if (cluster_index == 76)
				return IMGTOOLERR_CORRUPTIMAGE;

			cur_AU = get_fdr_cluster_baseAU(image, & fdr, cluster_index);
			cluster_lastphysrec = get_fdr_cluster_lastphysrec(& fdr, cluster_index);

			while ((i<secsused) && (i<=cluster_lastphysrec))
			{
				if (cur_AU >= image->AUformat.totAUs)
					return IMGTOOLERR_CORRUPTIMAGE;

				image->u.dsk.vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

				i += image->AUformat.physrecsperAU;
				cur_AU++;
			}

			cluster_index++;
		}

		/* free fdr sector */
		cur_AU = catalog->files[catalog_index].fdr_physrec / image->AUformat.physrecsperAU;
		image->u.dsk.vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

		/* delete catalog entry */
		for (i=catalog_index; i<127; i++)
			catalog->files[i] = catalog->files[i+1];
		catalog->files[127].fdr_physrec = 0;
		catalog->num_files--;

		/* update parent fdir */
		for (i=0; i<128; i++)
		{
			buf[2*i] = catalog->files[i].fdr_physrec >> 8;
			buf[2*i+1] = catalog->files[i].fdr_physrec & 0xff;
		}
		if (write_absolute_physrec(& image->l1_img, parent_ref ? image->u.dsk.catalog.subdirs[parent_ref-1].fdir_physrec : 1, buf))
			return IMGTOOLERR_WRITEERROR;

		/* update bitmap */
		if (write_absolute_physrec(& image->l1_img, 0, &image->u.dsk.vib))
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

/*
	Create a blank ti99_image (common code).

	Supports MESS and V9T9 formats only
*/
static int ti99_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options, ti99_img_format img_format)
{
	const char *volname;
	int density;
	ti99_lvl1_imgref l1_img;
	int protected;

	int totphysrecs, physrecsperAU, totAUs;

	dsk_vib vib;
	UINT8 empty_sec[256];

	int i;


	(void) mod;

	l1_img.img_format = img_format;
	l1_img.file_handle = f;

	/* read options */
	volname = in_options[ti99_createopts_volname].s;
	l1_img.geometry.sides = in_options[ti99_createopts_sides].i;
	l1_img.geometry.tracksperside = in_options[ti99_createopts_tracks].i;
	l1_img.geometry.secspertrack = in_options[ti99_createopts_sectors].i;
	protected = in_options[ti99_createopts_protection].i;
	density = in_options[ti99_createopts_density].i;

	totphysrecs = l1_img.geometry.secspertrack * l1_img.geometry.tracksperside * l1_img.geometry.sides;
	physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < physrecsperAU; i <<= 1)
		;
	physrecsperAU = i;
	totAUs = totphysrecs / physrecsperAU;

	/* auto-density */
	if (density == 0)
	{
		if (l1_img.geometry.secspertrack <= 9)
			density = 1;
		else if (l1_img.geometry.secspertrack <= 18)
			density = 2;
		else
			density = 3;
	}

	/* check volume name */
	if (strchr(volname, '.') || strchr(volname, ' ') || (strlen(volname) > 10))
		return /*IMGTOOLERR_BADFILENAME*/IMGTOOLERR_PARAMCORRUPT;

	/* FM disks can hold fewer sector per track than MFM disks */
	if ((density == 1) && (l1_img.geometry.secspertrack > 9))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* DD disks can hold fewer sector per track than HD disks */
	if ((density == 2) && (l1_img.geometry.secspertrack > 18))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* check total disk size */
	if (totphysrecs < 2)
		return IMGTOOLERR_PARAMTOOSMALL;

	/* initialize sector 0 */

	/* set volume name */
	str_to_fname(vib.name, volname);

	/* set every header field */
	set_UINT16BE(& vib.totphysrecs, totphysrecs);
	vib.secspertrack = l1_img.geometry.secspertrack;
	vib.id[0] = 'D';
	vib.id[1] = 'S';
	vib.id[2] = 'K';
	vib.protection = protected ? 'P' : ' ';
	vib.tracksperside = l1_img.geometry.tracksperside;
	vib.sides = l1_img.geometry.sides;
	vib.density = density;
	for (i=0; i<3; i++)
	{
		memset(vib.subdir[i].name, '\0', 10);
		set_UINT16BE(& vib.subdir[i].fdir_physrec, 0);
	}

	/* clear bitmap */
	for (i=0; i < (totAUs>>3); i++)
		vib.abm[i] = 0;

	if (totAUs & 7)
	{
		vib.abm[i] = 0xff << (totAUs & 7);
		i++;
	}

	for (; i < 200; i++)
		vib.abm[i] = 0xff;

	/* mark first 2 sectors (vib and fdir) as used */
	vib.abm[0] |= (physrecsperAU == 1) ? 3 : 1;

	/* write sector 0 */
	if (write_absolute_physrec(& l1_img, 0, &vib))
		return IMGTOOLERR_WRITEERROR;


	/* now clear every other sector, including the FDIR record */
	memset(empty_sec, 0, 256);

	for (i=1; i<totphysrecs; i++)
		if (write_absolute_physrec(& l1_img, i, empty_sec))
			return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Create a blank ti99_image (MESS format).
*/
static int ti99_image_create_mess(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	return ti99_image_create(mod, f, in_options, if_mess);
}

/*
	Create a blank ti99_image (V9T9 format).
*/
static int ti99_image_create_v9t9(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	return ti99_image_create(mod, f, in_options, if_v9t9);
}

/*
	Read one sector from a ti99_image.
*/
static int ti99_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

/*
	Write one sector to a ti99_image.
*/
static int ti99_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}
