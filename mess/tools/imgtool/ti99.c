/*
	Handlers for ti99 disk images

	Disk images are in v9t9/MESS format.  Files are extracted in TIFILES format.

	Raphael Nabet, 2002-2003

	TODO:
	- test subdirectory support
	- add hd support
*/

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"


/*
	DSK Disk structure:

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
	Sector 1: File Descriptor Index Record (FDIR): array of 0 through 128
		words, sector (physical record?) address of the fdr record for each
		file.  Sorted by ascending file name.  Note that, while we should be
		prepared to read images images with 128 entries, we should write write
		no more than 127 for compatibility with existing FDRs.
	Remaining physical records are used for fdr and data.  Each file has one
	FDR (File Descriptor Record) descriptor which provides the file information
	(name, format, lenght, pointers to data sectors/AUs, etc).
*/

/* Since string length is encoded with a byte, the maximum length of a string
is 255.  Since we need to add a device name (1 char minimum, typically 4 chars)
and a '.' separator, we chose a practical value of 250 (though few applications
will accept paths of such lenght). */
#define MAX_PATH_LEN /*253*/250


#if 0
#pragma mark -
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
	Subdirectory descriptor (HFDC only)

	The HFDC supports up to 3 subdirectories.
*/
typedef struct ti99_subdir
{
	char name[10];			/* subdirectory name (10 characters, pad with spaces) */
	UINT16BE fdir_physrec;	/* pointer to subdirectory fdir record */
} ti99_subdir;

/*
	VIB record

	Most fields in this record are only revelant to level 2 routines, but level
	1 routines need the disk geometry information extracted from the VIB.
*/
typedef struct ti99_vib
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
	UINT8 density;			/* density: 1 (FM SD), 2 (MFM DD), or 3 (MFM 80-track DD, and HD) */
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
} ti99_vib;

typedef enum
{
	if_mess,
	if_v9t9,
	if_pc99_fm,
	if_pc99_mfm
} ti99_img_format;

/*
	level-1 disk image descriptor
*/
typedef struct ti99_lvl1_ref
{
	ti99_img_format img_format;	/* tells the image format */
	STREAM *file_handle;		/* imgtool file handle */
	ti99_geometry geometry;		/* geometry */
	UINT32 *pc99_data_offset_array;	/* offset for each sector (pc99 format) */
} ti99_lvl1_ref;

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
static int parse_pc99_image(STREAM *file_handle, int fm_format, int pass, ti99_vib *vib, const ti99_geometry *geometry, UINT32 *data_offset_array)
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
	dest: pointer to 256-byte destination buffer
*/
static int read_image_vib_no_geometry(STREAM *file_handle, ti99_img_format img_format, ti99_vib *dest)
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
	img_format: image format (MESS, V9T9 or PC99)
	lvl1_ref: level-1 image handle (output)
	vib: buffer where the vib should be stored

	returns the imgtool error code
*/
static int open_image_lvl1(STREAM *file_handle, ti99_img_format img_format, ti99_lvl1_ref *lvl1_ref, ti99_vib *vib)
{
	int reply;
	int totphysrecs;


	lvl1_ref->img_format = img_format;
	lvl1_ref->file_handle = file_handle;

	/* read vib */
	reply = read_image_vib_no_geometry(file_handle, img_format, vib);
	if (reply)
		return reply;

	/* extract geometry information */
	totphysrecs = get_UINT16BE(vib->totphysrecs);

	lvl1_ref->geometry.secspertrack = vib->secspertrack;
	if (lvl1_ref->geometry.secspertrack == 0)
		/* Some images might be like this, because the original SSSD TI
		controller always assumes 9. */
		lvl1_ref->geometry.secspertrack = 9;
	lvl1_ref->geometry.tracksperside = vib->tracksperside;
	if (lvl1_ref->geometry.tracksperside == 0)
		/* Some images are like this, because the original SSSD TI controller
		always assumes 40. */
		lvl1_ref->geometry.tracksperside = 40;
	lvl1_ref->geometry.sides = vib->sides;
	if (lvl1_ref->geometry.sides == 0)
		/* Some images are like this, because the original SSSD TI controller
		always assumes that tracks beyond 40 are on side 2. */
		lvl1_ref->geometry.sides = totphysrecs / (lvl1_ref->geometry.secspertrack * lvl1_ref->geometry.tracksperside);

	/* check information */
	if ((totphysrecs != (lvl1_ref->geometry.secspertrack * lvl1_ref->geometry.tracksperside * lvl1_ref->geometry.sides))
			|| (totphysrecs < 2)
			|| memcmp(vib->id, "DSK", 3) || (! strchr(" P", vib->id[3]))
			|| (((img_format == if_mess) || (img_format == if_v9t9))
				&& (stream_size(file_handle) != totphysrecs*256)))
		return IMGTOOLERR_CORRUPTIMAGE;

	if ((img_format == if_pc99_fm) || (img_format == if_pc99_mfm))
	{
		lvl1_ref->pc99_data_offset_array = malloc(sizeof(*lvl1_ref->pc99_data_offset_array)*totphysrecs);
		if (! lvl1_ref->pc99_data_offset_array)
			return IMGTOOLERR_OUTOFMEMORY;
		reply = parse_pc99_image(file_handle, img_format == if_pc99_fm, 1, NULL, & lvl1_ref->geometry, lvl1_ref->pc99_data_offset_array);
		if (reply)
		{
			free(lvl1_ref->pc99_data_offset_array);
			return reply;
		}
	}

	return 0;
}

/*
	Convert physical sector address to offset in disk image
*/
INLINE int sector_address_to_image_offset(const ti99_lvl1_ref *lvl1_ref, const ti99_sector_address *address)
{
	int offset = 0;

	switch (lvl1_ref->img_format)
	{
	case if_mess:
		/* current MESS format */
		offset = (((address->cylinder*lvl1_ref->geometry.sides) + address->side)*lvl1_ref->geometry.secspertrack + address->sector)*256;
		break;

	case if_v9t9:
		/* V9T9 format */
		if (address->side & 1)
			/* on side 1, tracks are stored in the reverse order */
			offset = (((address->side*lvl1_ref->geometry.tracksperside) + (lvl1_ref->geometry.tracksperside-1 - address->cylinder))*lvl1_ref->geometry.secspertrack + address->sector)*256;
		else
			offset = (((address->side*lvl1_ref->geometry.tracksperside) + address->cylinder)*lvl1_ref->geometry.secspertrack + address->sector)*256;
		break;

	case if_pc99_fm:
	case if_pc99_mfm:
		/* pc99 format */
		offset = lvl1_ref->pc99_data_offset_array[(address->cylinder*lvl1_ref->geometry.sides + address->side)*lvl1_ref->geometry.secspertrack + address->sector];
		break;
	}

	return offset;
}

/*
	Read one 256-byte sector from a disk image

	file_handle: imgtool file handle
	address: physical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	dest: pointer to 256-byte destination buffer
*/
static int read_sector(ti99_lvl1_ref *lvl1_ref, const ti99_sector_address *address, void *dest)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(lvl1_ref->file_handle, sector_address_to_image_offset(lvl1_ref, address), SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_read(lvl1_ref->file_handle, dest, 256);
	if (reply != 256)
		return 1;

	return 0;
}

/*
	Write one 256-byte sector to a disk image

	file_handle: imgtool file handle
	address: physical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	src: pointer to 256-byte source buffer
*/
static int write_sector(ti99_lvl1_ref *lvl1_ref, const ti99_sector_address *address, const void *src)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(lvl1_ref->file_handle, sector_address_to_image_offset(lvl1_ref, address), SEEK_SET);
	if (reply)
		return 1;
	/* write it */
	reply = stream_write(lvl1_ref->file_handle, src, 256);
	if (reply != 256)
		return 1;

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

	file_handle: imgtool file handle
	physrec: logical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	dest: pointer to 256-byte destination buffer
*/
static int read_absolute_physrec(ti99_lvl1_ref *lvl1_ref, int physrec, void *dest)
{
	ti99_sector_address address;


	physrec_to_sector_address(physrec, & lvl1_ref->geometry, & address);

	return read_sector(lvl1_ref, & address, dest);
}

/*
	Write one 256-byte physical record to a disk image

	file_handle: imgtool file handle
	physrec: logical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	src: pointer to 256-byte source buffer
*/
static int write_absolute_physrec(ti99_lvl1_ref *lvl1_ref, int physrec, const void *src)
{
	ti99_sector_address address;


	physrec_to_sector_address(physrec, & lvl1_ref->geometry, & address);

	return write_sector(lvl1_ref, & address, src);
}

#if 0
#pragma mark -
#pragma mark LEVEL 2 DISK ROUTINES
#endif

/*
	Level 2 implements files as a succession of 256-byte-long physical records.

	Level 2 knows about allocation bitmap (and AU), disk catalog, etc.
*/

/*
	AU format
*/
typedef struct ti99_AUformat
{
	int totAUs;				/* total number of AUs (big-endian) */
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
	char dirname[10];		/* name of this file (copied from the VIB) */
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
	dir_entry subdirs[3];	/* description of each subdir */
	file_entry files[128];	/* description of each file */
} ti99_catalog;

/*
	ti99 disk image descriptor
*/
typedef struct ti99_image
{
	IMAGE base;
	ti99_lvl1_ref lvl1_ref;		/* image format, imgtool image handle, image geometry */
	ti99_vib vib;				/* cached copy of volume information block record in sector 0 */
	ti99_AUformat AUformat;		/* AU format */
	ti99_catalog catalog;		/* main catalog (fdr physrec address from sector 1, and file names from fdrs) */
	ti99_catalog subdir_catalog[3];	/* catalog of each subdirectory */
	int data_offset;	/* fdr records are preferentially allocated in sectors 2 to data_offset (34)
						whereas data records are preferentially allocated in sectors starting at data_offset. */
} ti99_image;

/*
	file flags found in fdr (and tifiles)
*/
enum
{
	fdr99_f_program	= 0x01,	/* set for program files */
	fdr99_f_int		= 0x02,	/* set for binary files */
	fdr99_f_wp		= 0x08,	/* set if file is write-protected */
	fdr99_f_var		= 0x80	/* set if file uses variable-length records*/
};

/*
	Time stamp (used in fdr)
*/
typedef struct ti99_date_time
{
	UINT8 time_MSB, time_LSB;/* 0-4: hour, 5-10: minutes, 11-15: seconds/2 */
	UINT8 date_MSB, date_LSB;/* 0-6: year, 7-10: month, 11-15: day */
} ti99_date_time;

/*
	FDR record
*/
typedef struct ti99_fdr
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
} ti99_fdr;


INLINE int get_fdr_cluster_baseAU(ti99_image *image, ti99_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/sector for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to AU address */
	if (image->AUformat.physrecsperAU <= 2)
		reply /= image->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_fdr_cluster_basephysrec(ti99_image *image, ti99_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/sector for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to sector address */
	if (image->AUformat.physrecsperAU > 2)
		reply *= image->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_fdr_cluster_lastphysrec(ti99_fdr *fdr, int cluster_index)
{
	return (fdr->clusters[cluster_index][2] << 4) | (fdr->clusters[cluster_index][1] >> 4);
}

INLINE void set_fdr_cluster_lastphysrec(ti99_fdr *fdr, int cluster_index, int data)
{
	fdr->clusters[cluster_index][1] = (fdr->clusters[cluster_index][1] & 0x0f) | (data << 4);
	fdr->clusters[cluster_index][2] = data >> 4;
}

INLINE void set_fdr_cluster(ti99_image *image, ti99_fdr *fdr, int cluster_index, int baseAU, int last_physrec)
{
	/* convert AU address to FDR value */
	if (image->AUformat.physrecsperAU <= 2)
		baseAU *= image->AUformat.physrecsperAU;

	/* write cluster entry */
	fdr->clusters[cluster_index][0] = baseAU;
	fdr->clusters[cluster_index][1] = ((baseAU >> 8) & 0x0f) | (last_physrec << 4);
	fdr->clusters[cluster_index][2] = last_physrec >> 4;
}

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
								/* * variant f: */
									/* 6 bytes: 'MYTERM' */
									/* 4 bytes: time & date of creation */
									/* 4 bytes: time & date of last update */
									/* 2 bytes: unknown (always >0000) */
									/* 96 chars: 0xCA53 repeated 56 times */
} tifile_header;


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

	image: ti99_image image record
	physrec: physical record address of the FDIR
	dest: pointer to the destination buffer where the catalog should be written

	Returns an error code if there was an error, 0 otherwise.
*/
static int read_catalog(ti99_image *image, int physrec, ti99_catalog *dest)
{
	int totphysrecs = get_UINT16BE(image->vib.totphysrecs);
	UINT8 buf[256];
	ti99_fdr fdr;
	int i;
	int reply;


	/* Read FDIR record */
	reply = read_absolute_physrec(& image->lvl1_ref, physrec, buf);
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
			reply = read_absolute_physrec(& image->lvl1_ref, dest->files[i].fdr_physrec, &fdr);
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
	Find the catalog entry and fdr record associated with a file name

	image: ti99_image image record
	fname: name of the file to search
	fdr: pointer to buffer where the fdr record should be stored (may be NULL)
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int find_catalog_entry(ti99_image *image, const char *fpath, int *parent_ref_valid, int *parent_ref, int *out_is_dir, int *catalog_index)
{
	int i;
	const ti99_catalog *cur_catalog;
	const char *element_start, *element_end;
	int element_len;
	char element[10];
	int is_dir;


#if 0
	str_to_fname(element, fpath);

	if (parent_ref)
		*parent_ref = 0;

	if (out_is_dir)
		*out_is_dir = 0;

	for (i=0; i<image->catalog.num_files; i++)
	{
		if (memcmp(image->catalog.files[i].fname, element, 10) == 0)
		{	/* file found */
			if (catalog_index)
				*catalog_index = i;

			return 0;
		}
	}

	return IMGTOOLERR_FILENOTFOUND;

#else

	cur_catalog = & image->catalog;
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
			cur_catalog = & image->subdir_catalog[i];
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
#endif
}

static int find_fdr(ti99_image *image, const char *fpath, ti99_fdr *fdr)
{
#if 0
	int i;
	char ti_fname[10];

	str_to_fname(ti_fname, fpath);

	for (i=0; i<image->catalog.num_files; i++)
	{
		if (memcmp(image->catalog.files[i].fname, ti_fname, 10) == 0)
		{	/* file found */
			if (fdr)
				if (read_absolute_physrec(& image->lvl1_ref, image->catalog.files[i].fdr_physrec, fdr))
					return IMGTOOLERR_READERROR;

			return 0;
		}
	}

	return IMGTOOLERR_FILENOTFOUND;
#else
	int parent_ref, is_dir, catalog_index;
	int reply;


	reply = find_catalog_entry(image, fpath, NULL, &parent_ref, &is_dir, &catalog_index);
	if (reply)
		return reply;

	if (is_dir)
		/* this is not a file */
		return IMGTOOLERR_BADFILENAME;

	if (fdr)
		if (read_absolute_physrec(& image->lvl1_ref, (parent_ref ? image->subdir_catalog[parent_ref-1] : image->catalog).files[catalog_index].fdr_physrec, fdr))
			return IMGTOOLERR_READERROR;

	return 0;
#endif
}

/*
	Allocate one sector on disk, for use as a fdr record

	image: ti99_image image record
	fdr_physrec: on output, logical address of a free sector
*/
static int alloc_fdr_sector(ti99_image *image, int *fdr_physrec)
{
	int totAUs = image->AUformat.totAUs;
	int i;


	for (i=0; i<totAUs; i++)
	{
		if (! (image->vib.abm[i >> 3] & (1 << (i & 7))))
		{
			*fdr_physrec = i * image->AUformat.physrecsperAU;
			image->vib.abm[i >> 3] |= 1 << (i & 7);

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
	int totAUs = image->AUformat.totAUs;
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
		if (! (image->vib.abm[i >> 3] & (1 << (i & 7))))
			free_sectors += image->AUformat.physrecsperAU;
	}

	/* check we have enough free space */
	if (free_sectors < nb_alloc_sectors)
		return IMGTOOLERR_NOSPACE;

	/* current number of data sectors in file */
	secsused = get_UINT16BE(fdr->secsused);

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
			last_sec = get_fdr_cluster_lastphysrec(fdr, cluster_index);
			if (last_sec >= (secsused-1))
				break;
		}
		if (cluster_index == 76)
			/* that sucks */
			return IMGTOOLERR_CORRUPTIMAGE;

		if (last_sec > (secsused-1))
		{	/* some extra space has already been allocated */
			cur_block_seclen = last_sec - (secsused-1);
			if (cur_block_seclen > nb_alloc_sectors)
				cur_block_seclen = nb_alloc_sectors;

			secsused += cur_block_seclen;
			set_UINT16BE(& fdr->secsused, secsused);
			nb_alloc_sectors -= cur_block_seclen;
			if (! nb_alloc_sectors)
				return 0;	/* done */
		}

		/* round up to next AU boundary */
		last_sec = last_sec + image->AUformat.physrecsperAU - (last_sec % image->AUformat.physrecsperAU) - 1;

		if (last_sec > (secsused-1))
		{	/* some extra space has already been allocated */
			cur_block_seclen = last_sec - (secsused-1);
			if (cur_block_seclen > nb_alloc_sectors)
				cur_block_seclen = nb_alloc_sectors;

			secsused += cur_block_seclen;
			set_UINT16BE(& fdr->secsused, secsused);
			set_fdr_cluster_lastphysrec(fdr, cluster_index, secsused-1);
			nb_alloc_sectors -= cur_block_seclen;
			if (! nb_alloc_sectors)
				return 0;	/* done */
		}

		/* read base AU address for this cluster */
		cluster_baseAU = get_fdr_cluster_baseAU(image, fdr, cluster_index);
		/* point past cluster end */
		cluster_baseAU += (last_sec-p_last_sec/*+image->AUformat.physrecsperAU-1*/) / image->AUformat.physrecsperAU;
		/* count free sectors after last block */
		cur_block_seclen = 0;
		for (i=cluster_baseAU; (! (image->vib.abm[i >> 3] & (1 << (i & 7)))) && (cur_block_seclen < nb_alloc_sectors) && (i < totAUs); i++)
			cur_block_seclen += image->AUformat.physrecsperAU;
		if (cur_block_seclen)
		{	/* extend last block */
			if (cur_block_seclen > nb_alloc_sectors)
				cur_block_seclen = nb_alloc_sectors;

			secsused += cur_block_seclen;
			set_UINT16BE(& fdr->secsused, secsused);
			last_sec += cur_block_seclen;
			nb_alloc_sectors -= cur_block_seclen;
			set_fdr_cluster_lastphysrec(fdr, cluster_index, last_sec);
			cluster_AUlen = (cur_block_seclen + image->AUformat.physrecsperAU - 1) / image->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				image->vib.abm[(i+cluster_baseAU) >> 3] |= 1 << ((i+cluster_baseAU) & 7);
			if (! nb_alloc_sectors)
				return 0;	/* done */
		}
		cluster_index++;
		if (cluster_index == 76)
			/* that sucks */
			return IMGTOOLERR_NOSPACE;
	}

	search_start = image->data_offset;	/* initially, search for free space only in data space */
	while (nb_alloc_sectors)
	{
		/* find smallest data block at least nb_alloc_sectors in lenght, and largest data block less than nb_alloc_sectors in lenght */
		first_best_block_seclen = INT_MAX;
		second_best_block_seclen = 0;
		for (i=search_start; i<totAUs; i++)
		{
			if (! (image->vib.abm[i >> 3] & (1 << (i & 7))))
			{	/* found one free block */
				/* compute its lenght */
				cluster_baseAU = i;
				cur_block_seclen = 0;
				while ((i<totAUs) && (! (image->vib.abm[i >> 3] & (1 << (i & 7)))))
				{
					cur_block_seclen += image->AUformat.physrecsperAU;
					i++;
				}
				/* compare to previous best and second-best blocks */
				if ((cur_block_seclen < first_best_block_seclen) && (cur_block_seclen >= nb_alloc_sectors))
				{
					first_best_block_baseAU = cluster_baseAU;
					first_best_block_seclen = cur_block_seclen;
					if (cur_block_seclen == nb_alloc_sectors)
						/* no need to search further */
						break;
				}
				else if ((cur_block_seclen > second_best_block_seclen) && (cur_block_seclen < nb_alloc_sectors))
				{
					second_best_block_baseAU = cluster_baseAU;
					second_best_block_seclen = cur_block_seclen;
				}
			}
		}

		if (first_best_block_seclen != INT_MAX)
		{	/* found one contiguous block which can hold it all */
			secsused += nb_alloc_sectors;
			set_UINT16BE(& fdr->secsused, secsused);

			set_fdr_cluster(image, fdr, cluster_index, first_best_block_baseAU, secsused-1);
			cluster_AUlen = (nb_alloc_sectors + image->AUformat.physrecsperAU - 1) / image->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				image->vib.abm[(i+first_best_block_baseAU) >> 3] |= 1 << ((i+first_best_block_baseAU) & 7);

			nb_alloc_sectors = 0;
		}
		else if (second_best_block_seclen != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			secsused += second_best_block_seclen;
			set_UINT16BE(& fdr->secsused, secsused);

			set_fdr_cluster(image, fdr, cluster_index, second_best_block_baseAU, secsused-1);
			cluster_AUlen = (second_best_block_seclen + image->AUformat.physrecsperAU - 1) / image->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				image->vib.abm[(i+second_best_block_baseAU) >> 3] |= 1 << ((i+second_best_block_baseAU) & 7);

			nb_alloc_sectors -= second_best_block_seclen;

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
static int new_file(ti99_image *image, int parent_ref, char fname[10], int *out_fdr_physrec/*, ti99_fdr *fdr,*/)
{
	ti99_catalog *catalog = & (parent_ref ? image->subdir_catalog[parent_ref-1] : image->catalog);
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
		errorcode = alloc_fdr_sector(image, &fdr_physrec);
		if (errorcode)
			return errorcode;

		/* shift catalog entries until the insertion point */
		for (i=catalog->num_files; i>catalog_index; i--)
			catalog->files[i] = catalog->files[i-1];

		/* write new catalog entry */
		catalog->files[catalog_index].fdr_physrec = fdr_physrec;
		memcpy(catalog->files[catalog_index].fname, fname, 10);
		if (out_fdr_physrec)
			*out_fdr_physrec = fdr_physrec;

		/* update catalog len */
		catalog->num_files++;
	}

	return 0;
}

/*
	compute the sector address for a given 256-byte physical record in a file
*/
static int file_physrec_to_log_sector_address(ti99_image *image, ti99_fdr *fdr, int physrecnum, int *out_physrec)
{
	int cluster_index;
	int cluster_firstphysrec, cluster_lastphysrec;
	int cluster_base_physrec;

	/* check parameter */
	if (physrecnum >= get_UINT16BE(fdr->secsused))
		return IMGTOOLERR_UNEXPECTED;

	/* search for the cluster in the data chain pointers array */
	cluster_lastphysrec = -1;
	for (cluster_index=0; cluster_index<76; cluster_index++)
	{
		/* read curent file block table entry */
		cluster_firstphysrec = cluster_lastphysrec+1;
		cluster_lastphysrec = get_fdr_cluster_lastphysrec(fdr, cluster_index);
		if (cluster_lastphysrec >= physrecnum)
			break;
	}
	if (cluster_index == 76)
		/* if not found, the file is corrupt */
		return IMGTOOLERR_CORRUPTIMAGE;
	/* read base sector address for this cluster */
	cluster_base_physrec = get_fdr_cluster_basephysrec(image, fdr, cluster_index);

	/* return sector logical sector address */
	* out_physrec = cluster_base_physrec + (physrecnum - cluster_firstphysrec);
	return 0;
}

/*
	read a 256-byte physical record from a file
*/
static int read_file_physrec(ti99_image *image, ti99_fdr *fdr, int physrecnum, void *dest)
{
	int errorcode;
	int physrec;

	/* compute sector logical address */
	errorcode = file_physrec_to_log_sector_address(image, fdr, physrecnum, & physrec);
	if (errorcode)
		return errorcode;
	/* read sector */
	if (read_absolute_physrec(& image->lvl1_ref, physrec, dest))
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	read a 256-byte physical record from a file
*/
static int write_file_physrec(ti99_image *image, ti99_fdr *fdr, int physrecnum, const void *src)
{
	int errorcode;
	int physrec;

	/* compute sector logical address */
	errorcode = file_physrec_to_log_sector_address(image, fdr, physrecnum, & physrec);
	if (errorcode)
		return errorcode;
	/* write sector */
	if (write_absolute_physrec(& image->lvl1_ref, physrec, src))
		return IMGTOOLERR_READERROR;

	return 0;
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

typedef struct ti99_file_pos
{
	int cur_log_rec;
	int cur_phys_rec;
	int cur_pos_in_phys_rec;
} ti99_file_pos;

/*
	Initialize a record of type ti99_file_pos.  To open a file on level 3, you
	must get the fdr record for the file (which is equivalent to opening the
	file on level 2), then initialize the file_pos record.
*/
INLINE void initialize_file_pos(ti99_file_pos *file_pos)
{
	memset(file_pos, 0, sizeof(*file_pos));

	/*if ()*/
}

/*
	Test a file for EOF
*/
static int is_eof(/*ti99_image *image,*/ ti99_fdr *fdr, ti99_file_pos *file_pos)
{
	int secsused = get_UINT16BE(fdr->secsused);

	if (fdr->flags & fdr99_f_var)
	{
		return (file_pos->cur_phys_rec >= secsused);
	}
	else
	{
		return ((file_pos->cur_phys_rec >= secsused)
				|| ((file_pos->cur_phys_rec == (secsused-1))
					&& (file_pos->cur_pos_in_phys_rec >= (fdr->eof ? fdr->eof : 256))));
	}
}

/*
	Read next record from a file
*/
static int read_next_record(ti99_image *image, ti99_fdr *fdr, ti99_file_pos *file_pos, void *dest, int *out_reclen)
{
	int errorcode;
	UINT8 physrec_buf[256];
	int reclen;

	if (fdr->flags & fdr99_f_program)
	{
		/* program files have no level-3 record */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if (fdr->flags & fdr99_f_var)
	{
		/* variable-lenght records */
		if (is_eof(fdr, file_pos))
			return IMGTOOLERR_UNEXPECTED;
		errorcode = read_file_physrec(image, fdr, file_pos->cur_phys_rec, physrec_buf);
		if (errorcode)
			return errorcode;
		/* read reclen */
		reclen = physrec_buf[file_pos->cur_pos_in_phys_rec];
		/* check integrity */
		if ((reclen == 0xff) || (reclen > fdr->reclen)
				|| ((file_pos->cur_pos_in_phys_rec + 1 + reclen) > 256))
			return IMGTOOLERR_CORRUPTIMAGE;
		/* copy to buffer */
		memcpy(dest, physrec_buf + file_pos->cur_pos_in_phys_rec + 1, reclen);
		file_pos->cur_pos_in_phys_rec += reclen + 1;
		/* skip to next physrec if needed */
		if ((file_pos->cur_pos_in_phys_rec == 256)
				|| (physrec_buf[file_pos->cur_pos_in_phys_rec] == 0xff))
		{
			file_pos->cur_pos_in_phys_rec = 0;
			file_pos->cur_phys_rec++;
		}
		(* out_reclen) = reclen;
	}
	else
	{
		/* fixed len records */
		reclen = fdr->reclen;
		if (is_eof(fdr, file_pos))
			return IMGTOOLERR_UNEXPECTED;
		if ((file_pos->cur_pos_in_phys_rec + reclen) > 256)
		{
			file_pos->cur_pos_in_phys_rec = 0;
			file_pos->cur_phys_rec++;
		}
		if ((file_pos->cur_phys_rec >= get_UINT16BE(fdr->secsused))
				|| ((file_pos->cur_phys_rec == (get_UINT16BE(fdr->secsused)-1))
					&& ((file_pos->cur_pos_in_phys_rec + reclen) >= (fdr->eof ? fdr->eof : 256))))
			return IMGTOOLERR_CORRUPTIMAGE;
		errorcode = read_file_physrec(image, fdr, file_pos->cur_phys_rec, physrec_buf);
		if (errorcode)
			return errorcode;
		memcpy(dest, physrec_buf + file_pos->cur_pos_in_phys_rec, reclen);
		file_pos->cur_pos_in_phys_rec += reclen;
		if (file_pos->cur_pos_in_phys_rec == 256)
		{
			file_pos->cur_pos_in_phys_rec = 0;
			file_pos->cur_phys_rec++;
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
typedef struct ti99_iterator
{
	IMAGEENUM base;
	ti99_image *image;
#if 1
	int level;
	int listing_subdirs;	/* true if we are listing subdirectories at level 0 */
	int index[2];			/* current index in the disk catalog */
#else
	int index;			/* current index in the disk catalog */
#endif
} ti99_iterator;


static int ti99_image_init_mess(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_v9t9(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_pc99_fm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static int ti99_image_init_pc99_mfm(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void ti99_image_exit(IMAGE *img);
static void ti99_image_info(IMAGE *img, char *string, const int len);
static int ti99_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int ti99_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void ti99_image_closeenum(IMAGEENUM *enumeration);
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
	{ "density",	"0 for auto-detect, 1 for SD, 2 for 40-track DD, 3 for 80-track DD and HD", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	0,	3,	"0" },
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
	/*EOLN_CR*/0,					/* eoln */
	0,								/* flags */
	ti99_image_init_mess,			/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_image_beginenum,			/* begin enumeration */
	ti99_image_nextenum,			/* enumerate next */
	ti99_image_closeenum,			/* close enumeration */
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
	/*EOLN_CR*/0,					/* eoln */
	0,								/* flags */
	ti99_image_init_v9t9,			/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_image_beginenum,			/* begin enumeration */
	ti99_image_nextenum,			/* enumerate next */
	ti99_image_closeenum,			/* close enumeration */
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
	/*EOLN_CR*/0,					/* eoln */
	0,								/* flags */
	ti99_image_init_pc99_fm,		/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_image_beginenum,			/* begin enumeration */
	ti99_image_nextenum,			/* enumerate next */
	ti99_image_closeenum,			/* close enumeration */
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
	/*EOLN_CR*/0,					/* eoln */
	0,								/* flags */
	ti99_image_init_pc99_mfm,		/* init function */
	ti99_image_exit,				/* exit function */
	ti99_image_info,				/* info function */
	ti99_image_beginenum,			/* begin enumeration */
	ti99_image_nextenum,			/* enumerate next */
	ti99_image_closeenum,			/* close enumeration */
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

/*
	Open a file as a ti99_image (common code).
*/
static int ti99_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg, ti99_img_format img_format)
{
	ti99_image *image;
	int reply;
	int totphysrecs;
	int fdir_physrec;
	int i;


	image = malloc(sizeof(ti99_image));
	* (ti99_image **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(ti99_image));
	image->base.module = mod;

	/* open disk image at level 1 */
	reply = open_image_lvl1(f, img_format, & image->lvl1_ref, & image->vib);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return reply;
	}

	/* open disk image at level 2 */
	/* first compute AU size and number of AUs */
	totphysrecs = get_UINT16BE(image->vib.totphysrecs);

	image->AUformat.physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < image->AUformat.physrecsperAU; i <<= 1)
		;
	image->AUformat.physrecsperAU = i;
	image->AUformat.totAUs = totphysrecs / image->AUformat.physrecsperAU;

	/* read and check main volume catalog */
	reply = read_catalog(image, 1, &image->catalog);
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
	image->catalog.num_subdirs = 0;
	for (i=0; i<3; i++)
	{
		fdir_physrec = get_UINT16BE(image->vib.subdir[i].fdir_physrec);
		if ((! memcmp(image->vib.subdir[i].name, "\0\0\0\0\0\0\0\0\0\0", 10))
				|| (! memcmp(image->vib.subdir[i].name, "          ", 10)))
		{
			/* name is empty: fine with us unless there is a fdir pointer */
			if (fdir_physrec != 0)
			{
				image->catalog.num_subdirs = 0;
				break;
			}
		}
		else if (check_fname(image->vib.subdir[i].name))
		{
			/* name is invalid: this is not an HFDC format floppy */
			image->catalog.num_subdirs = 0;
			break;
		}
		else
		{
			/* there is a non-empty name */
			if ((fdir_physrec == 0) || (fdir_physrec >= totphysrecs))
			{
				/* error: fdir pointer is invalid or NULL */
				image->catalog.num_subdirs = 0;
				break;
			}
			/* fill in descriptor fields */
			image->catalog.subdirs[image->catalog.num_subdirs].fdir_physrec = fdir_physrec;
			memcpy(image->catalog.subdirs[image->catalog.num_subdirs].dirname, image->vib.subdir[i].name, 10);
			reply = read_catalog(image, fdir_physrec, &image->subdir_catalog[image->catalog.num_subdirs]);
			if (reply)
			{
				/* error: invalid fdir */
				image->catalog.num_subdirs = 0;
				break;
			}
			/* found valid subdirectory: increment subdir count */
			image->catalog.num_subdirs++;
		}
	}

	/* initialize default data_offset */
	image->data_offset = 32+2;

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
	close a ti99_image
*/
static void ti99_image_exit(IMAGE *img)
{
	ti99_image *image = (ti99_image *) img;

	stream_close(image->lvl1_ref.file_handle);
	if ((image->lvl1_ref.img_format == if_pc99_fm) || (image->lvl1_ref.img_format == if_pc99_mfm))
		free(image->lvl1_ref.pc99_data_offset_array);
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

	fname_to_str(vol_name, image->vib.name, 11);

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
	iter->level = 0;
	iter->listing_subdirs = 1;
	iter->index[0] = 0;

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
	int fdr_physrec;


	ent->corrupt = 0;
	ent->eof = 0;

	/* iterate through catalogs to next file or dir entry */
	while ((iter->level >= 0)
			&& (((iter->level == 0) && (iter->listing_subdirs))
				? (iter->index[0] >= iter->image->catalog.num_subdirs)
				: (iter->index[iter->level] >= (iter->level ? iter->image->subdir_catalog[iter->index[0]-1].num_files : iter->image->catalog.num_files))))
	{
		if ((iter->level == 0) && (iter->listing_subdirs))
		{
			iter->listing_subdirs = 0;
			iter->index[0] = 0;
		}
		else
			iter->level--;
	}

	if (iter->level < 0)
	{
		ent->eof = 1;
	}
	else
	{
		if ((iter->level == 0) && (iter->listing_subdirs))
		{
			fname_to_str(ent->fname, iter->image->catalog.subdirs[iter->index[0]].dirname, ent->fname_len);

			/* set type of DIR */
			snprintf(ent->attr, ent->attr_len, "DIR");

			/* len in sectors */
			ent->filesize = 1;

			iter->index[iter->level]++;

			/* recurse subdirectory */
			iter->level++;
			iter->index[iter->level] = 0;
		}
		else
		{
			fdr_physrec = (iter->level ? iter->image->subdir_catalog[iter->index[0]-1].files : iter->image->catalog.files)[iter->index[iter->level]].fdr_physrec;
			reply = read_absolute_physrec(& iter->image->lvl1_ref, fdr_physrec, & fdr);
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
						fname_to_str(ent->fname, iter->image->catalog.subdirs[iter->index[0]-1].dirname, ent->fname_len);
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
static void ti99_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in AUs)
*/
static size_t ti99_image_freespace(IMAGE *img)
{
	ti99_image *image = (ti99_image*) img;
	size_t freeAUs;
	int i;

	freeAUs = 0;
	for (i=0; i<image->AUformat.totAUs; i++)
	{
		if (! (image->vib.abm[i >> 3] & (1 << (i & 7))))
			freeAUs++;
	}

	return freeAUs;
}

/*
	Extract a file from a ti99_image.  The file is saved in tifile format.
*/
static int ti99_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	ti99_image *image = (ti99_image*) img;
	ti99_fdr fdr;
	tifile_header dst_header;
	int i;
	int secsused;
	UINT8 buf[256];
	int errorcode;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	/* open file on TI image */
	errorcode = find_fdr(image, fname, &fdr);
	if (errorcode)
		return errorcode;

#if 1
	/* write TIFILE header */
	dst_header.tifiles[0] = '\7';
	dst_header.tifiles[1] = 'T';
	dst_header.tifiles[2] = 'I';
	dst_header.tifiles[3] = 'F';
	dst_header.tifiles[4] = 'I';
	dst_header.tifiles[5] = 'L';
	dst_header.tifiles[6] = 'E';
	dst_header.tifiles[7] = 'S';
	dst_header.secsused = fdr.secsused;
	dst_header.flags = fdr.flags;
	dst_header.recspersec = fdr.recspersec;
	dst_header.eof = fdr.eof;
	dst_header.reclen = fdr.reclen;
	set_UINT16BE(& dst_header.fixrecs, get_UINT16LE(fdr.fixrecs));
	for (i=0; i<(128-14); i++)
		dst_header.res[i] = 0;

	if (stream_write(destf, & dst_header, 128) != 128)
		return IMGTOOLERR_WRITEERROR;

	/* copy data to TIFILE */
	secsused = get_UINT16BE(fdr.secsused);

	for (i=0; i<secsused; i++)
	{
		errorcode = read_file_physrec(image, & fdr, i, buf);
		if (errorcode)
			return errorcode;

		if (stream_write(destf, buf, 256) != 256)
			return IMGTOOLERR_WRITEERROR;
	}
#else
	/* write text data */
	{
		ti99_file_pos file_pos;
		int reclen;
		char lineend = '\n';

		initialize_file_pos(& file_pos);
		while (! is_eof(& fdr, & file_pos))
		{
			errorcode = read_next_record(image, & fdr, & file_pos, buf, & reclen);
			if (errorcode)
				return errorcode;
			if (stream_write(destf, buf, reclen) != reclen)
				return IMGTOOLERR_WRITEERROR;
			if (stream_write(destf, &lineend, 1) != 1)
				return IMGTOOLERR_WRITEERROR;
		}
	}
#endif

	return 0;
}

/*
	Add a file to a ti99_image.  The file must be in tifile format.
*/
static int ti99_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	ti99_image *image = (ti99_image*) img;
	const char *fname;
	char ti_fname[10];
	ti99_fdr fdr;
	tifile_header src_header;
	int i;
	int secsused;
	UINT8 buf[256];
	int errorcode;
	int fdr_physrec;
	int parent_ref_valid, parent_ref;
	ti99_catalog *catalog;


	if (check_fpath(fpath))
		return IMGTOOLERR_BADFILENAME;

	errorcode = find_catalog_entry(image, fpath, &parent_ref_valid, &parent_ref, NULL, NULL);
	if (errorcode == 0)
	{	/* file already exists: causes an error for now */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if ((! parent_ref_valid) || (errorcode != IMGTOOLERR_FILENOTFOUND))
	{
		return errorcode;
	}

	if (stream_read(sourcef, & src_header, 128) != 128)
		return IMGTOOLERR_READERROR;

	/* create new file */
	fname = strrchr(fpath, '.');
	if (fname)
		fname++;
	else
		fname = fpath;
	str_to_fname(ti_fname, fname);
	errorcode = new_file(image, parent_ref, ti_fname, &fdr_physrec);
	if (errorcode)
		return errorcode;

	memcpy(fdr.name, ti_fname, 10);
	set_UINT16BE(& fdr.xreclen, 0);
	fdr.flags = src_header.flags;
	fdr.recspersec = src_header.recspersec;
	/*fdr.secsused = src_header.secsused;*/set_UINT16BE(& fdr.secsused, 0);
	fdr.eof = src_header.eof;
	fdr.reclen = src_header.reclen;
	set_UINT16LE(& fdr.fixrecs, get_UINT16BE(src_header.fixrecs));
	{
		/* Now we copy the host date and time into the fdr */
		/* All these functions should be ANSI */
		time_t cur_time = time(NULL);
		struct tm expanded_time = *localtime(& cur_time);

		fdr.update.time_MSB = fdr.creation.time_MSB = (expanded_time.tm_hour << 3) | (expanded_time.tm_min >> 3);
		fdr.update.time_LSB = fdr.creation.time_LSB = (expanded_time.tm_min << 5) | (expanded_time.tm_sec >> 1);
		fdr.update.date_MSB = fdr.creation.date_MSB = ((expanded_time.tm_year % 100) << 1) | ((expanded_time.tm_mon+1) >> 3);
		fdr.update.date_LSB = fdr.creation.date_LSB = ((expanded_time.tm_mon+1) << 5) | expanded_time.tm_mday;
	}

	for (i=0; i<76; i++)
		fdr.clusters[i][2] = fdr.clusters[i][1] = fdr.clusters[i][0] = 0;

	/* alloc data sectors */
	secsused = get_UINT16BE(src_header.secsused);
	errorcode = alloc_file_sectors(image, &fdr, secsused);
	if (errorcode)
		return errorcode;

	/* copy data */
	for (i=0; i<secsused; i++)
	{
		if (stream_read(sourcef, buf, 256) != 256)
			return IMGTOOLERR_READERROR;

		errorcode = write_file_physrec(image, & fdr, i, buf);
		if (errorcode)
			return errorcode;
	}

	/* write fdr */
	if (write_absolute_physrec(& image->lvl1_ref, fdr_physrec, &fdr))
		return IMGTOOLERR_WRITEERROR;

	/* update catalog */
	catalog = & (parent_ref ? image->subdir_catalog[parent_ref-1] : image->catalog);
	for (i=0; i<128; i++)
	{
		buf[2*i] = catalog->files[i].fdr_physrec >> 8;
		buf[2*i+1] = catalog->files[i].fdr_physrec & 0xff;
	}
	if (write_absolute_physrec(& image->lvl1_ref, parent_ref ? image->catalog.subdirs[parent_ref-1].fdir_physrec : 1, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_absolute_physrec(& image->lvl1_ref, 0, &image->vib))
		return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Delete a file from a ti99_image.
*/
static int ti99_image_deletefile(IMAGE *img, const char *fname)
{
	ti99_image *image = (ti99_image*) img;
	ti99_fdr fdr;
	int i, cluster_index;
	int cur_AU, cluster_lastphysrec;
	int secsused;
	int parent_ref, is_dir, catalog_index;
	int errorcode;
	UINT8 buf[256];
	ti99_catalog *catalog;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	errorcode = find_catalog_entry(image, fname, NULL, &parent_ref, &is_dir, &catalog_index);
	if (errorcode)
		return errorcode;

	if (is_dir)
	{
		catalog = & image->subdir_catalog[catalog_index];

		if ((catalog->num_files != 0) || (catalog->num_subdirs != 0))
			return IMGTOOLERR_UNIMPLEMENTED;

		catalog = & image->catalog;

		/* free fdir sector */
		cur_AU = catalog->subdirs[catalog_index].fdir_physrec / image->AUformat.physrecsperAU;
		image->vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

		/* delete catalog entry */
		for (i=catalog_index; i<2; i++)
			catalog->subdirs[i] = catalog->subdirs[i+1];
		catalog->subdirs[2].fdir_physrec = 0;
		catalog->num_subdirs--;

		/* update directory in vib */
		for (i=0; i<3; i++)
		{
			memcpy(image->vib.subdir[i].name, catalog->subdirs[i].dirname, 10);
			set_UINT16BE(&image->vib.subdir[i].fdir_physrec, catalog->subdirs[i].fdir_physrec);
		}

		/* write vib (bitmap+directory) */
		if (write_absolute_physrec(& image->lvl1_ref, 0, &image->vib))
			return IMGTOOLERR_WRITEERROR;

		return IMGTOOLERR_UNIMPLEMENTED;
	}
	else
	{
		catalog = & (parent_ref ? image->subdir_catalog[parent_ref-1] : image->catalog);

		if (read_absolute_physrec(& image->lvl1_ref, catalog->files[catalog_index].fdr_physrec, &fdr))
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

				image->vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

				i += image->AUformat.physrecsperAU;
				cur_AU++;
			}

			cluster_index++;
		}

		/* free fdr sector */
		cur_AU = catalog->files[catalog_index].fdr_physrec / image->AUformat.physrecsperAU;
		image->vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

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
		if (write_absolute_physrec(& image->lvl1_ref, parent_ref ? image->catalog.subdirs[parent_ref-1].fdir_physrec : 1, buf))
			return IMGTOOLERR_WRITEERROR;

		/* update bitmap */
		if (write_absolute_physrec(& image->lvl1_ref, 0, &image->vib))
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

/*
	Create a blank ti99_image (common code).
*/
static int ti99_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options, ti99_img_format img_format)
{
	const char *volname;
	int density;
	ti99_lvl1_ref lvl1_ref;
	int protected;

	int totphysrecs, physrecsperAU, totAUs;

	ti99_vib vib;
	UINT8 empty_sec[256];

	int i;


	(void) mod;

	lvl1_ref.img_format = img_format;
	lvl1_ref.file_handle = f;

	/* read options */
	volname = in_options[ti99_createopts_volname].s;
	lvl1_ref.geometry.sides = in_options[ti99_createopts_sides].i;
	lvl1_ref.geometry.tracksperside = in_options[ti99_createopts_tracks].i;
	lvl1_ref.geometry.secspertrack = in_options[ti99_createopts_sectors].i;
	protected = in_options[ti99_createopts_protection].i;
	density = in_options[ti99_createopts_density].i;

	totphysrecs = lvl1_ref.geometry.secspertrack * lvl1_ref.geometry.tracksperside * lvl1_ref.geometry.sides;
	physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < physrecsperAU; i <<= 1)
		;
	physrecsperAU = i;
	totAUs = totphysrecs / physrecsperAU;

	/* auto-density */
	if (density == 0)
	{
		if ((lvl1_ref.geometry.tracksperside <= 40) && (lvl1_ref.geometry.secspertrack <= 9))
			density = 1;
		else if ((lvl1_ref.geometry.tracksperside <= 40) && (lvl1_ref.geometry.secspertrack <= 18))
			density = 2;
		else
			density = 3;
	}

	/* check volume name */
	if (strchr(volname, '.') || strchr(volname, ' ') || (strlen(volname) > 10))
		return /*IMGTOOLERR_BADFILENAME*/IMGTOOLERR_PARAMCORRUPT;

	/* check number of tracks if 40-track image */
	if ((density < 3) && (lvl1_ref.geometry.tracksperside > 40))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* FM disks can hold fewer sector per track than MFM disks */
	if ((density == 1) && (lvl1_ref.geometry.secspertrack > 9))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* DD disks can hold fewer sector per track than HD disks */
	if ((density == 2) && (lvl1_ref.geometry.secspertrack > 18))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* check total disk size */
	if (totphysrecs < 2)
		return IMGTOOLERR_PARAMTOOSMALL;

	/* initialize sector 0 */

	/* set volume name */
	str_to_fname(vib.name, volname);

	/* set every header field */
	set_UINT16BE(& vib.totphysrecs, totphysrecs);
	vib.secspertrack = lvl1_ref.geometry.secspertrack;
	vib.id[0] = 'D';
	vib.id[1] = 'S';
	vib.id[2] = 'K';
	vib.protection = protected ? 'P' : ' ';
	vib.tracksperside = lvl1_ref.geometry.tracksperside;
	vib.sides = lvl1_ref.geometry.sides;
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
	if (write_absolute_physrec(& lvl1_ref, 0, &vib))
		return IMGTOOLERR_WRITEERROR;


	/* now clear every other sector, including the FDIR record */
	memset(empty_sec, 0, 256);

	for (i=1; i<totphysrecs; i++)
		if (write_absolute_physrec(& lvl1_ref, i, empty_sec))
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
