/*
	Handlers for ti99 disk images

	We support the following types of image:
	* floppy disks ("DSK format") in v9t9, pc99 and old mess image format
	* hard disks ("WIN format") in MAME harddisk format (256-byte sectors only,
	  i.e. no SCSI)
	Files are extracted in TIFILES format.  There is a compile-time option to
	extract text files in flat format, but I need to re-implement it properly
	(using filters comes to mind).

	Raphael Nabet, 2002-2003

	TODO:
	- finish and test hd support ***finish and test sibling FDR support***
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
	Concepts shared by both disk structures:
	----------------------------------------

	In both formats, the entire disk surface is split into physical records,
	each 256-byte-long.  For best efficiency, the sector size is usually 256
	bytes so that one sector is equivalent to a physical record (this is true
	with floppies and HFDC hard disks).  If the sector size is not 256 bytes
	(e.g. 512 bytes with SCSI hard disks, possibly 128 bytes on a TI-99
	simulator running on TI-990), sectors are grouped or split to form 256-byte
	physical records.  Physical records are numbered from 0, with the first
	starting on sector 0 track 0.  To avoid confusion with physical records in
	files, these physical records will be called "absolute physical records",
	abbreviated as "absolute physrecs" or "absphysrecs".

	Disk space is allocated in units referred to as AUs.  Each AU represents an
	integral number of 256-byte physical records.  The number of physrecs per
	AU varies depending on the disk format and size.  AUs are numbered from 0,
	with the first starting with absolute physrec 0.

	Absolute physrec 0 contains a 256-byte record with volume information,
	which is called "Volume Information Block", abbreviated as "vib".

	To keep track of which areas on the disk are allocated and which are free,
	disk managers maintain a bit map of allocated AUs (called "allocation
	bitmap", abbreviated as "abm").  Each bit in the allocation bitmap
	represents an AU: if a bit is 0, the AU is free; if a bit is 1, the AU is
	allocated (or possibly bad if the disk manager can track bad sectors).
	Where on the disk the abm is located depends on the disk structure (see
	structure-specific description for details).

	Files are implemented as a succession 256-byte physical records.  To
	avoid confusion with absolute physical records, these physical records will
	be called "file physical records", abbreviated as "file physrecs" or
	"filephysrecs".

	Programs do normally not access file physical records directly.  They may
	call high-level file routines that enable to create either fixed-lenght
	logical records of any size from 1 through 255, or variable-lenght records
	of any size from 0 through 254: logical records are grouped by file
	managers into 256-byte physical records.  Some disk managers (HFDC and
	SCSI) allow programs to create fixed-lenght records larger than 255 bytes,
	too, but few programs use this possibility.  Additionally, programs may
	create program files, that do not implement any logical record, and can be
	seen as a flat byte stream, not unlike files under MSDOS, UNIX and the C
	standard library.  (Unfortunately, the API for program files lacks
	flexibility, and most programs that need to process a flat byte stream
	will use fixed-size records of 128 bytes.)

		Fixed-length records (reclen < 256) are grouped like this:
			file physrec 0    file physrec 1
			_______________   _______________
			|R0 |R1 |R2 |X|   |R3 |R4 |R5 |X|
			---------------   ---------------
			            \_/               \_/
			           unused            unused
			      (size < reclen)   (size < reclen)

		Fixed-length records (reclen > 256) are grouped like this:
			file physrec 0     file physrec 1     file physrec 2      file physrec 3      ...
			_________________  _________________  _________________   _________________
			|  R0 (#0-255)  |  | R0 (#256-511) |  |R0 (#512-end)|X|   |  R1 (#0-255)  |   ...
			-----------------  -----------------  -----------------   -----------------
			                                                    \_/
			                                                   unused

		Variable lenght records are grouped like this:
			           file physrec 0:
			byte:
			        ------------------------------
			      0 |l0 = lenght of record 0 data|
			        |----------------------------|
			      1 |                            |
			        |                            |
			      : |       record 0 data        |
			        |                            |
			     l0 |                            |
			        |----------------------------|
			   l0+1 |l1 = lenght of record 1 data|
			        |----------------------------|
			   l0+2 |                            |
			        |                            |
			      : |       record 1 data        |
			        |                            |
			l0+l1+1 |                            |
			        |----------------------------|
			      : :                            :
			      : :                            :
			        |----------------------------|
			      n |lm = lenght of record m data|
			        |----------------------------|
			    n+1 |                            |
			        |                            |
			      : |       record m data        |
			        |                            |
			   n+lm |                            |
			        |----------------------------|
			 n+lm+1 |>ff: EOR mark               |
			        |----------------------------|
			 n+lm+2 |                            |
			        |                            |
			      : |           unused           |
			      : |(we should have size < lm+1)|
			        |                            |
			    255 |                            |
			        ------------------------------

			           file physrec 1:
			byte:
			        ------------------------------
			      0 |lm+1=lenght of record 0 data|
			        |----------------------------|
			      1 |                            |
			        |                            |
			      : |      record m+1 data       |
			        |                            |
			   lm+1 |                            |
			        |----------------------------|
			      : :                            :
			      : :                            :

			I think the EOR mark is not required if all 256 bytes of a physical
			record are used by logical records.


	All files are associated with a "file descriptor record" ("fdr") that holds
	file information (name, format, length) and points to the data physrecs.
	The WIN disk structure also supports sibling FDRs, in case a file is so
	fragmented that all the data pointers cannot fit in one FDR; the DSK disk
	structure does not implement any such feature, and you may be unable to
	append data to an existing file if it is too fragmented, even though the
	disk is not full.


	DSK disk structure:
	-------------------

	The DSK disk structure is used for floppy disks, and a few RAMDISKs as
	well.  It supports 1600 AUs and 16 MBytes at most (the 16 MByte value is a
	theorical maximum, as I doubt anyone has ever created so big a DSK volume).

	The disk sector size is always 256 bytes (the only potential exceptions are
	the SCSI floppy disk units and the TI-99 simulator on TI-990, but I don't
	know anything about either).

	For some reason, the number of physrecs per AU is always a power of 2.
	Originally, AUs always were one physical record each.  However, since the
	allocation bitmap cannot support more than 1600 AUs, disks larger than
	400kbytes need to have several physical records per AU.  Note that
	relatively few disk controllers implement AUs that span multiple physical
	records (IIRC, these are Myarc's HFDC, SCSI DSR with floppy disk interface,
	and some upgraded Myarc and Corcomp DD floppy controllers).

	The allocation bitmap is located in the VIB.  It is 200 bytes long, and
	supports 1600 AUs at most.

	Directories are implemented with a "File Descriptor Index Record" (FDIR)
	for each directory.  The FDIR is array of 0 through 128 words, containing
	the absolute physrec address of each fdr in the directory, sorted by
	ascending file name, terminated with a 0.  Note that, while we should be
	prepared to read images images with 128 entries, I think (not 100% sure)
	that we should write no more than 127 for compatibility with some existing
	disk managers.

	Originally, the DSK structure only supported one directory level (i.e. the
	root directory level).  The FDIR record for the root directory is always
	located in absolute physrec 1.  Moreover, Myarc extended the DSK structure
	to support up to 3 subdirectories in the root directory (note that there is
	no support for more subdirs, or for subdirs located in subdirs).  To do so,
	they used an unused field of the VIB to hold the 10-char name of each
	directory and the absolute physrec address of the associated FDIR record.

	absphysrec 0: Volume Information Block (VIB): see below
	absphysrec 1: FDIR for root directory
	Remaining AUs are used for fdr and data (and subdirectory FDIR if
	applicable).  There is one FDIR record per directory; the FDIR points to the
	FDR for each file in the directory.  The FDR (File Descriptor Record)
	contains the file information (name, format, lenght, pointers to data
	physrecs/AUs, etc).


	WIN disk structure:
	-------------------

	The WIN disk structure is used for hard disks.  It supports 65535 AUs and
	256 MBytes at most.

	The disk sector size is 256 bytes on HFDC, 512 bytes on SCSI.  512-byte
	sectors are split into 256-byte physical records.

	We may have from 1 through 16 physrecs per AUs.  Since we cannot have more
	than 65535 AUs, we need to have several physical records per AU in disks
	larger than 16 Mbytes.

	Contrary to the DSK disk structure, the WIN disk structure supports
	hierarchic subdirectories, with a limit of 114 subdirectories and 127 files
	per directory.  Each directory is associated with both a "Directory
	Descriptor Record" (DDR) and a "File Descriptor Index Record" (FDIR).  The
	only exception is the root directory, which uses the VIB instead of a DDR
	(the VIB includes all the important fields of the DDR).  The DDR contains
	some directory info (name, number of files and subdirectories directly
	enclosed in the directory, AU address of the FDIR of the parent directory,
	etc), the AU address of the associated FDIR, and the AU address of the DDR
	of up to 114 subdirectories, sorted by ascending directory name.  The WIN
	FDIR is similar to, yet different from, the DSK FDIR: it contains up to 127
	(vs. 128) AU address (vs. absphysrec address) of each fdr in the directory,
	sorted by ascending file name.  Additionally, it includes the AU address of
	the associated DDR.

	When a file has more than 54 data fragments, the disk manager creates
	sibling FDRs that hold additional fragments.  (Sibling FDRs are called
	offspring FDRs in most existing documentation, but I prefer "sibling FDRs"
	as "offspring FDRs" wrongly suggests a directory-like hierarchy.
	Incidentally, I cannot really figure out why they chose to duplicate the
	FDR rather than create a file extension record, but there must be a
	reason.)  Note that sibling FDRs usually fill unused physrecs in the AU
	allocated for the eldest FDR, and a new AU is allocated for new sibling
	FDRs only when the first AU is full.

	absphysrec 0: Volume Information Block (VIB): see below
	absphysrec 1-n (where n = 1+SUP(number_of_AUs/2048)): Volume bitmap
	Remaining AUs are used for ddr, fdir, fdr and data.
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
	UINT16BE totphysrecs;	/* total number of physrecs on disk (usually 360, */
								/* 720, 640, 1280 or 1440).  (TI originally */
								/* said it should be the total number of AUs, */
								/* but, in practice, DSRs that supports disks */
								/* over 400kbytes (e.g. HFDC) write the total */
								/* number of physrecs.) */
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
	Read the volume information block (physrec 0) assuming no geometry
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

	case if_pc99_fm:
	case if_pc99_mfm:
		return parse_pc99_image(file_handle, img_format == if_pc99_fm, 0, dest, NULL, NULL);

	case if_harddisk:
		/* not implemented, because we don't need it */
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
			return IMGTOOLERR_CORRUPTIMAGE;	/* TODO: support 512-byte sectors */
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

	case if_harddisk:
		/* not implemented */
		assert(1);
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


	if (l1_img->img_format == if_harddisk)
	{	/* ***KLUDGE*** */
		address.sector = physrec % l1_img->geometry.secspertrack;
		physrec /= l1_img->geometry.secspertrack;
		address.side = physrec % l1_img->geometry.sides;
		address.cylinder = physrec / l1_img->geometry.sides;
	}
	else
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
	UINT16 dir_ptr;			/* DSK: absphysrec address of the FDIR for this directory */
							/* WIN: AU address of the DDR for this directory */
	char dirname[10];		/* name of this directory (copied from the VIB for DSK, DDR for WIN) */
} dir_entry;

typedef struct file_entry
{
	UINT16 fdr_ptr;			/* DSK: physrec address of the FDR for this file */
							/* WIN: AU address of the FDR for this file */
	char fname[10];			/* name of this file (copied from FDR) */
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
	dsk_vib vib;			/* cached copy of volume information block record in physrec 0 */
	ti99_catalog catalog;	/* main catalog (fdr physrec/AU address from sector 1, and file names from fdrs) */
	ti99_catalog subdir_catalog[3];	/* catalog of each subdirectory */
	int data_offset;	/* fdr (and fdir) records are preferentially allocated
						in AUs 2 (if 1 physrec per AU) or 1 (if 2 physrecs per
						AU or more) to data_offset (34), whereas data records
						are preferentially allocated in AUs starting at
						data_offset. */
} ti99_lvl2_imgref_dsk;

typedef struct ti99_lvl2_imgref_win
{
	enum
	{
		win_vib_v1,
		win_vib_v2
	} vib_version;
	win_vib_ddr vib;	/* cached copy of volume information block record in physrec 0 */
	UINT8 abm[8192];	/* allocation bitmap */
	int data_offset;	/* fdr records are preferentially allocated in physrecs
						n (n<=33) to data_offset, whereas data records are
						preferentially allocated in physrecs starting at
						data_offset. */
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
	UINT8 recspersec;		/* logical records per physrec */
								/* ignored for variable length record files and */
								/* program files */
	UINT16BE secsused;		/* allocated file length in physrecs */
	UINT8 eof;				/* EOF offset in last physrec for variable length */
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
	UINT8 recspersec;		/* logical records per physrec */
								/* ignored for variable length record files and */
								/* program files */
	UINT16BE secsused_LSW;	/* for root FDR: allocated file length in physrecs */
							/* for sibling FDRs: index of the first file
								physrec in this sibling FDR */
	UINT8 eof;				/* EOF offset in last physrec for variable length */
								/* record files and program files (0->256)*/
	UINT8 reclen;			/* logical record size in bytes ([1,255] 0->256) */
								/* Maximum allowable record size for variable */
								/* length record files.  Reserved for program */
								/* files (set to 0).  Set to 0 if reclen >=256 */
								/* (HFDC only). */
	UINT16LE fixrecs_LSW;	/* file length in logical records */
								/* For variable length record files, number of */
								/* 256-byte records actually used. */
	ti99_date_time creation;/* date and time of creation (HFDC and BwG only; */
								/*reserved in TI) */
	ti99_date_time update;	/* date and time of last write to file (HFDC and */
								/* BwG only; reserved in TI) */

	char id[2];				/* 'FI' */
	UINT16BE prev_FDR_AU;	/* previous cluster table extension FDR */
	UINT16BE next_FDR_AU;	/* next cluster table extension FDR */
	UINT16BE num_FDR_AU;	/* total number of data AUs allocated to this particular sibling FDR */
	UINT16BE parent_FDIR_AU;/* FDIR the file is listed in */
	UINT8 xinfo_MSB;		/* extended information (MSByte) */
								/* bits 0-3: MSN of secsused */
								/* bits 4-7: MSN of fixrecs */
	UINT8 xinfo_LSB;		/* extended information (LSByte) */
								/* bits 8-11: physrec within AU for prev_FDR_AU */
								/* bits 12-15: physrec within AU for next_FDR_AU */
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
	UINT16BE secsused;		/* file length in physrecs */
	UINT8 flags;			/* see enum above */
	UINT8 recspersec;		/* records per physrec */
	UINT8 eof;				/* current position of eof in last physrec (0->255)*/
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
	unsigned secsused;
	unsigned eldestfdr_physrec;
	unsigned curfdr_physrec;
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

	if ((entry1->fdr_ptr == 0) && (entry2->fdr_ptr == 0))
		return 0;
	else if (entry1->fdr_ptr == 0)
		return +1;
	else if (entry2->fdr_ptr == 0)
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
	UINT16BE fdir_buf[128];
	dsk_fdr fdr;
	int i;
	int reply;


	/* Read FDIR record */
	reply = read_absolute_physrec(& l2_img->l1_img, physrec, fdir_buf);
	if (reply)
		return IMGTOOLERR_READERROR;

	/* Copy FDIR info to catalog structure */
	for (i=0; i<128; i++)
		dest->files[i].fdr_ptr = get_UINT16BE(fdir_buf[i]);

	/* Check FDIR pointers and check and extract file names from DDRs */
	for (i=0; i<128; i++)
	{
		if (dest->files[i].fdr_ptr >= totphysrecs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->files[i].fdr_ptr)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->files[i].fdr_ptr, &fdr);
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
		if (((! dest->files[i].fdr_ptr) && dest->files[i+1].fdr_ptr)
			|| ((dest->files[i].fdr_ptr && dest->files[i+1].fdr_ptr) && (memcmp(dest->files[i].fname, dest->files[i+1].fname, 10) >= 0)))
		{
			/* if the catalog is not sorted, we repair it */
			qsort(dest->files, sizeof(dest->files)/sizeof(dest->files[0]), sizeof(dest->files[0]),
					qsort_catalog_compare);
			break;
		}
	}

	/* Set file count */
	for (i=0; (i<128) && (dest->files[i].fdr_ptr != 0); i++)
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
	if ((ddr_buf.num_files > 127) || (ddr_buf.num_subdirs > 114) || (get_UINT16BE(ddr_buf.fdir_AU) > l2_img->AUformat.totAUs))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* set file count and subdir count */
	dest->num_files = ddr_buf.num_files;
	dest->num_subdirs = ddr_buf.num_subdirs;

	/* Copy DDR info to catalog structure */
	for (i=0; i<ddr_buf.num_subdirs; i++)
		dest->subdirs[i].dir_ptr = get_UINT16BE(ddr_buf.subdir_AU[i]);

	/* Read FDIR record */
	reply = read_absolute_physrec(& l2_img->l1_img, get_UINT16BE(ddr_buf.fdir_AU)*l2_img->AUformat.physrecsperAU, fdir_buf);
	if (reply)
		return IMGTOOLERR_READERROR;

	/* Copy FDIR info to catalog structure */
	for (i=0; i<dest->num_files; i++)
		dest->files[i].fdr_ptr = get_UINT16BE(fdir_buf[i]);

	/* Check DDR pointers and check and extract file names from FDRs */
	for (i=0; i<dest->num_subdirs; i++)
	{
		if (dest->subdirs[i].dir_ptr >= l2_img->AUformat.totAUs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->subdirs[i].dir_ptr)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->subdirs[i].dir_ptr*l2_img->AUformat.physrecsperAU, &ddr_buf);
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
		if (dest->files[i].fdr_ptr >= l2_img->AUformat.totAUs)
		{
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (dest->files[i].fdr_ptr)
		{
			reply = read_absolute_physrec(& l2_img->l1_img, dest->files[i].fdr_ptr*l2_img->AUformat.physrecsperAU, &fdr_buf);
			if (reply)
				return IMGTOOLERR_READERROR;

			/* check and copy file name */
			if (check_fname(fdr_buf.name))
				return IMGTOOLERR_CORRUPTIMAGE;
			memcpy(dest->files[i].fname, fdr_buf.name, 10);
		}
	}

	/* Check catalog */
	for (i=0; i<dest->num_files-1; i++)
	{
		if (((! dest->files[i].fdr_ptr) && dest->files[i+1].fdr_ptr)
			|| ((dest->files[i].fdr_ptr && dest->files[i+1].fdr_ptr) && (memcmp(dest->files[i].fname, dest->files[i+1].fname, 10) >= 0)))
		{
			/* if the catalog is not sorted, we repair it */
			qsort(dest->files, dest->num_files, sizeof(dest->files[0]), qsort_catalog_compare);
			break;
		}
	}

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
	parent_ddr_AU: parent DDR AU address (0 for root)
	parent_catalog: catalog of parent dir (cannot be NULL)
	out_is_dir: TRUE if element is a directory
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int win_find_catalog_entry(ti99_lvl2_imgref *l2_img, const char *fpath,
									int *parent_ref_valid, int *parent_ddr_AU, ti99_catalog *parent_catalog,
									int *out_is_dir, int *catalog_index)
{
	int i;
	const char *element_start, *element_end;
	int element_len;
	char element[10];
	int is_dir = FALSE;
	int errorcode;

	if (parent_ref_valid)
		(* parent_ref_valid) = FALSE;
	if (parent_ddr_AU)
		*parent_ddr_AU = 0;

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

			if (parent_ddr_AU)
				*parent_ddr_AU = parent_catalog->subdirs[i].dir_ptr;

			errorcode = win_read_catalog(l2_img, parent_catalog->subdirs[i].dir_ptr, parent_catalog);
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
	Allocate one physrec on disk, for use as a fdr record

	l2_img: image reference
	fdr_physrec: on output, logical address of a free physrec
*/
static int alloc_fdr_physrec(ti99_lvl2_imgref *l2_img, int *fdr_physrec)
{
	int totAUs = l2_img->AUformat.totAUs;
	UINT8 *abm;
	int i;

	switch (l2_img->type)
	{
	case L2I_DSK:
		abm = l2_img->u.dsk.vib.abm;
		break;

	case L2I_WIN:
		abm = l2_img->u.win.abm;
		break;

	default:
		abm = NULL;
		break;
	}

	if (abm)
	{
		for (i=0; i<totAUs; i++)
		{
			if (! (abm[i >> 3] & (1 << (i & 7))))
			{
				*fdr_physrec = i * l2_img->AUformat.physrecsperAU;
				abm[i >> 3] |= 1 << (i & 7);

				return 0;
			}
		}
	}

	return IMGTOOLERR_NOSPACE;
}

INLINE int get_dsk_fdr_cluster_baseAU(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/physrec for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to AU address */
	if (l2_img->AUformat.physrecsperAU <= 2)
		reply /= l2_img->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_dsk_fdr_cluster_basephysrec(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index)
{
	int reply;

	/* read base AU/physrec for this cluster */
	reply = ((fdr->clusters[cluster_index][1] & 0xf) << 8) | fdr->clusters[cluster_index][0];
	/* convert to physrec address */
	if (l2_img->AUformat.physrecsperAU > 2)
		reply *= l2_img->AUformat.physrecsperAU;

	return reply;
}

INLINE int get_dsk_fdr_cluster_lastphysrec(dsk_fdr *fdr, int cluster_index)
{
	return (fdr->clusters[cluster_index][2] << 4) | (fdr->clusters[cluster_index][1] >> 4);
}

INLINE void set_dsk_fdr_cluster_lastphysrec(dsk_fdr *fdr, int cluster_index, int data)
{
	fdr->clusters[cluster_index][1] = (fdr->clusters[cluster_index][1] & 0x0f) | (data << 4);
	fdr->clusters[cluster_index][2] = data >> 4;
}

INLINE void set_dsk_fdr_cluster(ti99_lvl2_imgref *l2_img, dsk_fdr *fdr, int cluster_index, int baseAU, int last_physrec)
{
	/* convert AU address to FDR value */
	if (l2_img->AUformat.physrecsperAU <= 2)
		baseAU *= l2_img->AUformat.physrecsperAU;

	/* write cluster entry */
	fdr->clusters[cluster_index][0] = baseAU;
	fdr->clusters[cluster_index][1] = ((baseAU >> 8) & 0x0f) | (last_physrec << 4);
	fdr->clusters[cluster_index][2] = last_physrec >> 4;
}

INLINE unsigned get_win_fdr_secsused(win_fdr *fdr)
{
	return (((unsigned) fdr->xinfo_MSB << 12) & 0xf0000) | get_UINT16BE(fdr->secsused_LSW);
}

INLINE void set_win_fdr_secsused(win_fdr *fdr, unsigned data)
{
	fdr->xinfo_MSB = (fdr->xinfo_MSB & 0x0f) | ((data >> 12) & 0xf0);
	set_UINT16BE(&fdr->secsused_LSW, data & 0xffff);
}

INLINE unsigned get_win_fdr_fixrecs(win_fdr *fdr)
{
	return (((unsigned) fdr->xinfo_MSB << 16) & 0xf0000) | get_UINT16LE(fdr->fixrecs_LSW);
}

INLINE void set_win_fdr_fixrecs(win_fdr *fdr, unsigned data)
{
	fdr->xinfo_MSB = (fdr->xinfo_MSB & 0xf0) | ((data >> 16) & 0x0f);
	set_UINT16LE(&fdr->fixrecs_LSW, data & 0xffff);
}

INLINE unsigned get_win_fdr_prev_FDR_physrec(ti99_lvl2_imgref *l2_img, win_fdr *fdr)
{
	unsigned prev_FDR_AU = get_UINT16BE(fdr->prev_FDR_AU);

	return prev_FDR_AU
			? (prev_FDR_AU * l2_img->AUformat.physrecsperAU + ((fdr->xinfo_LSB >> 4) & 0xf))
			: 0;
}

INLINE unsigned get_win_fdr_next_FDR_physrec(ti99_lvl2_imgref *l2_img, win_fdr *fdr)
{
	unsigned next_FDR_AU = get_UINT16BE(fdr->next_FDR_AU);

	return next_FDR_AU
			? (next_FDR_AU * l2_img->AUformat.physrecsperAU + (fdr->xinfo_LSB & 0xf))
			: 0;
}

INLINE unsigned get_win_fdr_cursib_base_filephysrec(win_fdr *fdr)
{
	return get_UINT16BE(fdr->prev_FDR_AU) ? get_win_fdr_secsused(fdr) : 0;
}

/*
	Extend a file with nb_alloc_physrecs extra physrecs

	dsk_file: file reference
	nb_alloc_physrecs: number of physical records to allocate
*/
static int dsk_alloc_file_physrecs(ti99_lvl2_fileref_dsk *dsk_file, int nb_alloc_physrecs)
{
	int totAUs = dsk_file->l2_img->AUformat.totAUs;
	int free_physrecs;
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
	free_physrecs = 0;
	for (i=0; i<totAUs; i++)
	{
		if (! (dsk_file->l2_img->u.dsk.vib.abm[i >> 3] & (1 << (i & 7))))
			free_physrecs += dsk_file->l2_img->AUformat.physrecsperAU;
	}

	/* check we have enough free space */
	if (free_physrecs < nb_alloc_physrecs)
		return IMGTOOLERR_NOSPACE;

	/* current number of data physrecs in file */
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
			last_sec = get_dsk_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index);
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
			set_dsk_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index, secsused-1);
			nb_alloc_physrecs -= cur_block_seclen;
			if (! nb_alloc_physrecs)
				return 0;	/* done */
		}

		/* read base AU address for this cluster */
		cluster_baseAU = get_dsk_fdr_cluster_baseAU(dsk_file->l2_img, &dsk_file->fdr, cluster_index);
		/* point past cluster end */
		cluster_baseAU += (last_sec-p_last_sec/*+file->l2_img->AUformat.physrecsperAU-1*/) / dsk_file->l2_img->AUformat.physrecsperAU;
		/* count free physrecs after last block */
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
			set_dsk_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index, last_sec);
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
		/* find smallest data block at least nb_alloc_physrecs in lenght, and largest data block less than nb_alloc_physrecs in lenght */
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

			set_dsk_fdr_cluster(dsk_file->l2_img, &dsk_file->fdr, cluster_index, first_best_block_baseAU, secsused-1);
			cluster_AUlen = (nb_alloc_physrecs + dsk_file->l2_img->AUformat.physrecsperAU - 1) / dsk_file->l2_img->AUformat.physrecsperAU;
			for (i=0; i<cluster_AUlen; i++)
				dsk_file->l2_img->u.dsk.vib.abm[(i+first_best_block_baseAU) >> 3] |= 1 << ((i+first_best_block_baseAU) & 7);

			nb_alloc_physrecs = 0;
		}
		else if (second_best_block_seclen != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			secsused += second_best_block_seclen;
			set_UINT16BE(& dsk_file->fdr.secsused, secsused);

			set_dsk_fdr_cluster(dsk_file->l2_img, &dsk_file->fdr, cluster_index, second_best_block_baseAU, secsused-1);
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
		{	/* we did not find any free physrec in the data section of the disk */
			search_start = 0;	/* time to fall back to fdr space */
		}
		else
			return IMGTOOLERR_NOSPACE;	/* This should never happen, as we pre-check that there is enough free space */
	}

	return 0;
}

/*
	Append a new sibling FDR at the end of the sibling FDR list
*/
static int win_alloc_sibFDR(ti99_lvl2_fileref_win *win_file)
{
	/*unsigned*/int newfdr_absphysrec;
	int allocated = FALSE;
	int errorcode;
	unsigned cursib_base_filephysrec;

	if (get_UINT16BE(win_file->curfdr.next_FDR_AU))
		return IMGTOOLERR_UNEXPECTED;

	if ((win_file->curfdr_physrec % win_file->l2_img->AUformat.physrecsperAU)
			!= (win_file->l2_img->AUformat.physrecsperAU - 1))
	{	/* current AU is not full */
		newfdr_absphysrec = win_file->curfdr_physrec + 1;
	}
	else
	{	/* current AU is full: allocate another */
		errorcode = alloc_fdr_physrec(win_file->l2_img, &newfdr_absphysrec);
		if (errorcode)
			return errorcode;
		allocated = TRUE;
	}

	set_UINT16BE(&win_file->curfdr.next_FDR_AU, newfdr_absphysrec / win_file->l2_img->AUformat.physrecsperAU);
	win_file->curfdr.xinfo_LSB = (win_file->curfdr.xinfo_LSB & 0xf0) | (newfdr_absphysrec % win_file->l2_img->AUformat.physrecsperAU);

	/* save current sibling FDR */
	if (write_absolute_physrec(& win_file->l2_img->l1_img, win_file->curfdr_physrec, &win_file->curfdr))
	{
		/* clear pointer */
		set_UINT16BE(&win_file->curfdr.next_FDR_AU, 0);
		win_file->curfdr.xinfo_LSB = win_file->curfdr.xinfo_LSB & 0xf0;
		if (allocated)
		{	/* free AU */
			int newfdr_AU = newfdr_absphysrec / win_file->l2_img->AUformat.physrecsperAU;
			win_file->l2_img->u.win.abm[newfdr_AU >> 3] |= 1 << (newfdr_AU & 7);
		}
		return IMGTOOLERR_WRITEERROR;
	}

	/* now update in-memory structure to describe new sibling FDR */
	cursib_base_filephysrec = get_win_fdr_cursib_base_filephysrec(&win_file->curfdr)
								+ get_UINT16BE(win_file->curfdr.num_FDR_AU) * win_file->l2_img->AUformat.physrecsperAU;

	
	set_UINT16BE(&win_file->curfdr.next_FDR_AU, 0);
	set_UINT16BE(&win_file->curfdr.prev_FDR_AU, win_file->curfdr_physrec / win_file->l2_img->AUformat.physrecsperAU);
	win_file->curfdr.xinfo_LSB = (win_file->curfdr_physrec % win_file->l2_img->AUformat.physrecsperAU) << 4;

	win_file->curfdr_physrec = newfdr_absphysrec;

	set_win_fdr_secsused(&win_file->curfdr, cursib_base_filephysrec);
	set_UINT16BE(&win_file->curfdr.num_FDR_AU, 0);
	memset(win_file->curfdr.clusters, 0, sizeof(win_file->curfdr.clusters));

	return 0;
}

/*
	Extend a file with nb_alloc_physrecs extra physrecs (unfinished)

	win_file: file reference
	nb_alloc_physrecs: number of physical records to allocate
*/
static int win_alloc_file_physrecs(ti99_lvl2_fileref_win *win_file, int nb_alloc_physrecs)
{
	int totAUs = win_file->l2_img->AUformat.totAUs;
	int free_physrecs;
	int secsused;
	int i;
	int cluster_index;
	int num_filephysrec;
	int cur_block_seclen;
	int cluster_baseAU, cluster_AUlen;
	int first_best_block_baseAU = 0, first_best_block_seclen;
	int second_best_block_baseAU = 0, second_best_block_seclen;
	int search_start;
	int errorcode;

	/* compute free space */
	free_physrecs = 0;
	for (i=0; i<totAUs; i++)
	{
		if (! (win_file->l2_img->u.win.abm[i >> 3] & (1 << (i & 7))))
			free_physrecs += win_file->l2_img->AUformat.physrecsperAU;
	}

	/* check we have enough free space */
	if (free_physrecs < nb_alloc_physrecs)
		return IMGTOOLERR_NOSPACE;

	/* move to last sibling non-empty FDR */
	while ((get_UINT16BE(win_file->curfdr.next_FDR_AU) != 0) &&
				(get_UINT16BE(win_file->curfdr.clusters[53][0]) != 0))
	{
		win_file->curfdr_physrec = get_win_fdr_next_FDR_physrec(win_file->l2_img, &win_file->curfdr);
		if (read_absolute_physrec(& win_file->l2_img->l1_img, win_file->curfdr_physrec, &win_file->curfdr))
			return IMGTOOLERR_READERROR;
	}
	if ((get_UINT16BE(win_file->curfdr.clusters[0][0]) == 0) && (get_UINT16BE(win_file->curfdr.prev_FDR_AU) != 0))
	{	/* this is annoying: we have found a sibling FDR filled with 0s: rewind
		to last non-empty sibling if applicable */
		win_file->curfdr_physrec = get_win_fdr_prev_FDR_physrec(win_file->l2_img, &win_file->curfdr);
		if (read_absolute_physrec(& win_file->l2_img->l1_img, win_file->curfdr_physrec, &win_file->curfdr))
			return IMGTOOLERR_READERROR;
	}

	/* current number of data physrecs in file */
	secsused = win_file->secsused;

	/* current number of allocated physrecs */
	num_filephysrec = get_win_fdr_cursib_base_filephysrec(&win_file->curfdr)
						+ get_UINT16BE(win_file->curfdr.num_FDR_AU) * win_file->l2_img->AUformat.physrecsperAU;

	if (num_filephysrec > secsused)
	{	/* some extra space has already been allocated */
		cur_block_seclen = num_filephysrec - secsused;
		if (cur_block_seclen > nb_alloc_physrecs)
			cur_block_seclen = nb_alloc_physrecs;

		secsused += cur_block_seclen;
		win_file->secsused = secsused;
		/* TODO: update eldest FDR secsused field */
		/*set_win_fdr_secsused(& win_file->rootfdr.secsused, secsused);*/
		nb_alloc_physrecs -= cur_block_seclen;
		if (! nb_alloc_physrecs)
			return 0;	/* done */
	}

	/* find last non-empty cluster */
	for (cluster_index=0; (cluster_index<54) && (get_UINT16BE(win_file->curfdr.clusters[cluster_index][0]) != 0); cluster_index++)
		;
	/* if we are dealing with an empty file, we will have (cluster_index == 0)... */
	if (cluster_index != 0)
	{
		cluster_index--;
		/* try to extend last cluster */
		/* point past cluster end */
		cluster_baseAU = get_UINT16BE(win_file->curfdr.clusters[cluster_index][1]) + 1;
		/* count free physrecs after last block */
		cur_block_seclen = 0;
		for (i=cluster_baseAU; (! (win_file->l2_img->u.win.abm[i >> 3] & (1 << (i & 7)))) && (cur_block_seclen < nb_alloc_physrecs) && (i < totAUs); i++)
			cur_block_seclen += win_file->l2_img->AUformat.physrecsperAU;
		if (cur_block_seclen)
		{	/* extend last block */
			if (cur_block_seclen > nb_alloc_physrecs)
				cur_block_seclen = nb_alloc_physrecs;

			secsused += cur_block_seclen;
			cluster_AUlen = (cur_block_seclen + win_file->l2_img->AUformat.physrecsperAU - 1) / win_file->l2_img->AUformat.physrecsperAU;
			win_file->secsused = secsused;
			/* TODO: update eldest FDR secsused field */
			/*set_win_fdr_secsused(& win_file->rootfdr.secsused, secsused);*/
			set_UINT16BE(&win_file->curfdr.num_FDR_AU,
							get_UINT16BE(win_file->curfdr.num_FDR_AU)+cluster_AUlen);
			set_UINT16BE(&win_file->curfdr.clusters[cluster_index][1],
							get_UINT16BE(win_file->curfdr.clusters[cluster_index][1])+cluster_AUlen);
			for (i=0; i<cluster_AUlen; i++)
				win_file->l2_img->u.win.abm[(i+cluster_baseAU) >> 3] |= 1 << ((i+cluster_baseAU) & 7);
			nb_alloc_physrecs -= cur_block_seclen;
			if (! nb_alloc_physrecs)
				return 0;	/* done */
		}
		/* now point to first free entry in cluster table */
		if (cluster_index != 53)
			cluster_index++;
		else
		{
			if (get_UINT16BE(win_file->curfdr.next_FDR_AU) != 0)
			{
				win_file->curfdr_physrec = get_win_fdr_next_FDR_physrec(win_file->l2_img, &win_file->curfdr);
				if (read_absolute_physrec(& win_file->l2_img->l1_img, win_file->curfdr_physrec, &win_file->curfdr))
					return IMGTOOLERR_READERROR;
			}
			else
			{
				errorcode = win_alloc_sibFDR(win_file);
				if (errorcode)
					return errorcode;
			}
			cluster_index = 0;
		}
	}

	search_start = win_file->l2_img->u.win.data_offset;	/* initially, search for free space only in data space */
	while (nb_alloc_physrecs)
	{
		/* find smallest data block at least nb_alloc_physrecs in lenght, and largest data block less than nb_alloc_physrecs in lenght */
		first_best_block_seclen = INT_MAX;
		second_best_block_seclen = 0;
		for (i=search_start; i<totAUs; i++)
		{
			if (! (win_file->l2_img->u.win.abm[i >> 3] & (1 << (i & 7))))
			{	/* found one free block */
				/* compute its lenght */
				cluster_baseAU = i;
				cur_block_seclen = 0;
				while ((i<totAUs) && (! (win_file->l2_img->u.win.abm[i >> 3] & (1 << (i & 7)))))
				{
					cur_block_seclen += win_file->l2_img->AUformat.physrecsperAU;
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
			cluster_AUlen = (nb_alloc_physrecs + win_file->l2_img->AUformat.physrecsperAU - 1) / win_file->l2_img->AUformat.physrecsperAU;
			win_file->secsused = secsused;
			/* TODO: update eldest FDR secsused field */
			/*set_win_fdr_secsused(& win_file->rootfdr.secsused, secsused);*/
			set_UINT16BE(&win_file->curfdr.num_FDR_AU,
								get_UINT16BE(win_file->curfdr.num_FDR_AU)+cluster_AUlen);
			set_UINT16BE(&win_file->curfdr.clusters[cluster_index][0], first_best_block_baseAU);
			set_UINT16BE(&win_file->curfdr.clusters[cluster_index][1],
								first_best_block_baseAU+cluster_AUlen-1);

			for (i=0; i<cluster_AUlen; i++)
				win_file->l2_img->u.win.abm[(i+first_best_block_baseAU) >> 3] |= 1 << ((i+first_best_block_baseAU) & 7);

			nb_alloc_physrecs = 0;
		}
		else if (second_best_block_seclen != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			secsused += second_best_block_seclen;
			cluster_AUlen = (second_best_block_seclen + win_file->l2_img->AUformat.physrecsperAU - 1) / win_file->l2_img->AUformat.physrecsperAU;
			win_file->secsused = secsused;
			/* TODO: update eldest FDR secsused field */
			/*set_win_fdr_secsused(& win_file->rootfdr.secsused, secsused);*/
			set_UINT16BE(&win_file->curfdr.num_FDR_AU,
								get_UINT16BE(win_file->curfdr.num_FDR_AU)+cluster_AUlen);
			set_UINT16BE(&win_file->curfdr.clusters[cluster_index][0], second_best_block_baseAU);
			set_UINT16BE(&win_file->curfdr.clusters[cluster_index][1],
								second_best_block_baseAU+cluster_AUlen-1);

			for (i=0; i<cluster_AUlen; i++)
				win_file->l2_img->u.win.abm[(i+second_best_block_baseAU) >> 3] |= 1 << ((i+second_best_block_baseAU) & 7);

			nb_alloc_physrecs -= second_best_block_seclen;

			/* now point to first free entry in cluster table */
			if (cluster_index != 53)
				cluster_index++;
			else
			{
				if (get_UINT16BE(win_file->curfdr.next_FDR_AU) != 0)
				{
					win_file->curfdr_physrec = get_win_fdr_next_FDR_physrec(win_file->l2_img, &win_file->curfdr);
					if (read_absolute_physrec(& win_file->l2_img->l1_img, win_file->curfdr_physrec, &win_file->curfdr))
						return IMGTOOLERR_READERROR;
				}
				else
				{
					errorcode = win_alloc_sibFDR(win_file);
					if (errorcode)
						return errorcode;
				}
				cluster_index = 0;
			}
		}
		else if (search_start != 0)
		{	/* we did not find any free physrec in the data section of the disk */
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
		errorcode = alloc_fdr_physrec(l2_img, &fdr_physrec);
		if (errorcode)
			return errorcode;

		/* shift catalog entries until the insertion point */
		for (i=catalog->num_files; i>catalog_index; i--)
			catalog->files[i] = catalog->files[i-1];

		/* write new catalog entry */
		catalog->files[catalog_index].fdr_ptr = fdr_physrec;
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
static int new_file_lvl2_win(ti99_lvl2_imgref *l2_img, ti99_catalog *parent_catalog, char fname[10], ti99_lvl2_fileref *l2_file)
{
	int fdr_physrec;
	int catalog_index, i;
	int reply = 0;
	int errorcode;


	if (parent_catalog->num_files >= 127)
		/* if num_files == 127, catalog is full */
		return IMGTOOLERR_NOSPACE;

	/* find insertion point in catalog */
	for (i=0; (i < parent_catalog->num_files) && ((reply = memcmp(parent_catalog->files[i].fname, fname, 10)) < 0); i++)
		;

	if ((i<parent_catalog->num_files) && (reply == 0))
		/* file already exists */
		return IMGTOOLERR_BADFILENAME;
	else
	{
		/* otherwise insert new entry */
		catalog_index = i;
		errorcode = alloc_fdr_physrec(l2_img, &fdr_physrec);
		if (errorcode)
			return errorcode;

		/* shift catalog entries until the insertion point */
		for (i=parent_catalog->num_files; i>catalog_index; i--)
			parent_catalog->files[i] = parent_catalog->files[i-1];

		/* write new catalog entry */
		parent_catalog->files[catalog_index].fdr_ptr = fdr_physrec / l2_img->AUformat.physrecsperAU;
		memcpy(parent_catalog->files[catalog_index].fname, fname, 10);

		/* update catalog len */
		parent_catalog->num_files++;
	}

	/* set up file handle */
	l2_file->type = L2F_WIN;
	l2_file->u.win.l2_img = l2_img;
	l2_file->u.win.secsused = 0;
	l2_file->u.win.eldestfdr_physrec = fdr_physrec;
	l2_file->u.win.curfdr_physrec = fdr_physrec;
	memset(&l2_file->u.win.curfdr, 0, sizeof(l2_file->u.win.curfdr));
	memcpy(l2_file->u.win.curfdr.name, fname, 10);

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
	l2_file->u.dsk.fdr_physrec = (parent_ref ? l2_img->u.dsk.subdir_catalog[parent_ref-1] : l2_img->u.dsk.catalog).files[catalog_index].fdr_ptr;
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
	l2_file->u.win.eldestfdr_physrec = catalog.files[catalog_index].fdr_ptr * l2_img->AUformat.physrecsperAU;
	l2_file->u.win.curfdr_physrec = l2_file->u.win.eldestfdr_physrec;
	if (read_absolute_physrec(& l2_img->l1_img, l2_file->u.win.curfdr_physrec, &l2_file->u.win.curfdr))
		return IMGTOOLERR_READERROR;
	l2_file->u.win.secsused = get_win_fdr_secsused(&l2_file->u.win.curfdr);

	/* check integrity of FDR sibling chain */
	/* note that as we check that the back chain is consistent with the forward
	chain, we also check against cycles in the sibling chain */
	if (get_UINT16BE(l2_file->u.win.curfdr.prev_FDR_AU))
		return IMGTOOLERR_CORRUPTIMAGE;

	{
		int i, pastendoflist_flag;
		unsigned cur_filephysrec, num_FDR_AU;
		win_fdr *cur_fdr, fdr_buf;
		unsigned curfdr_physrec, prevfdr_physrec;

		cur_filephysrec = 0;
		pastendoflist_flag = 0;
		cur_fdr = &l2_file->u.win.curfdr;
		curfdr_physrec = l2_file->u.win.eldestfdr_physrec;

		while (1)
		{
			num_FDR_AU = 0;
			i=0;
			if (! pastendoflist_flag)
			{
				/* compute number of allocated AUs and check number of AUs */
				for (; i<54; i++)
				{
					if (get_UINT16BE(cur_fdr->clusters[i][0]) == 0)
					{
						pastendoflist_flag = TRUE;
						break;
					}
					num_FDR_AU += get_UINT16BE(cur_fdr->clusters[i][1])
									- get_UINT16BE(cur_fdr->clusters[i][0])
									+ 1;
				}
			}
			/* clear remainder of cluster table */
			for (; i<54; i++)
			{
				set_UINT16BE(&cur_fdr->clusters[i][0], 0);
				set_UINT16BE(&cur_fdr->clusters[i][1], 0);
			}

			if (get_UINT16BE(cur_fdr->num_FDR_AU) != num_FDR_AU)
				return IMGTOOLERR_CORRUPTIMAGE;

			cur_filephysrec += num_FDR_AU * l2_file->u.win.l2_img->AUformat.physrecsperAU;

			if (! get_UINT16BE(cur_fdr->next_FDR_AU))
				break;

			if (get_UINT16BE(cur_fdr->next_FDR_AU) >= l2_file->u.win.l2_img->AUformat.totAUs)
				return IMGTOOLERR_CORRUPTIMAGE;
			prevfdr_physrec = curfdr_physrec;
			curfdr_physrec = get_win_fdr_next_FDR_physrec(l2_file->u.win.l2_img, cur_fdr);
			if (read_absolute_physrec(& l2_file->u.win.l2_img->l1_img, curfdr_physrec, &fdr_buf))
				return IMGTOOLERR_READERROR;
			cur_fdr = &fdr_buf;
			if (get_win_fdr_prev_FDR_physrec(l2_file->u.win.l2_img, cur_fdr) != prevfdr_physrec)
				return IMGTOOLERR_CORRUPTIMAGE;
			if (get_win_fdr_secsused(cur_fdr) != cur_filephysrec)
				return IMGTOOLERR_CORRUPTIMAGE;
			/* TODO: check consistency of informative fields (record format, flags, etc) */
		}
	}

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
static int dsk_file_physrec_to_abs_physrec(ti99_lvl2_fileref_dsk *dsk_file, unsigned physrecnum, int *out_physrec)
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
		cluster_lastphysrec = get_dsk_fdr_cluster_lastphysrec(&dsk_file->fdr, cluster_index);
		if (cluster_lastphysrec >= physrecnum)
			break;
	}
	if (cluster_index == 76)
		/* if not found, the file is corrupt */
		return IMGTOOLERR_CORRUPTIMAGE;
	/* read base physrec address for this cluster */
	cluster_base_physrec = get_dsk_fdr_cluster_basephysrec(dsk_file->l2_img, &dsk_file->fdr, cluster_index);

	/* return absolute physrec address */
	* out_physrec = cluster_base_physrec + (physrecnum - cluster_firstphysrec);
	return 0;
}

/*
	compute the absolute physrec address for a 256-byte physical record in a file

	l2_img: image where the file is located
*/
static int win_file_physrec_to_abs_physrec(ti99_lvl2_fileref_win *dsk_file, unsigned physrecnum, int *out_physrec)
{
	int cluster_base_filephysrec;
	int cluster_last_filephysrec;
	int i;


	/* check parameter */
	if (physrecnum >= dsk_file->secsused)
		return IMGTOOLERR_UNEXPECTED;

	/* look for correct sibling */
	if (physrecnum < get_win_fdr_cursib_base_filephysrec(& dsk_file->curfdr))
	{
		while (physrecnum < get_win_fdr_cursib_base_filephysrec(& dsk_file->curfdr))
		{
			if (get_UINT16BE(dsk_file->curfdr.prev_FDR_AU) == 0)
				return IMGTOOLERR_CORRUPTIMAGE;
			else
				dsk_file->curfdr_physrec = get_win_fdr_prev_FDR_physrec(dsk_file->l2_img, &dsk_file->curfdr);
			if (read_absolute_physrec(& dsk_file->l2_img->l1_img, dsk_file->curfdr_physrec, &dsk_file->curfdr))
				return IMGTOOLERR_READERROR;
		}
	}
	else /*if (physrecnum >= get_win_fdr_cursib_base_filephysrec(& dsk_file->curfdr))*/
	{
		while (physrecnum >= (get_win_fdr_cursib_base_filephysrec(& dsk_file->curfdr)
								+ get_UINT16BE(dsk_file->curfdr.num_FDR_AU) * dsk_file->l2_img->AUformat.physrecsperAU))
		{
			if (get_UINT16BE(dsk_file->curfdr.next_FDR_AU) == 0)
				return IMGTOOLERR_CORRUPTIMAGE;
			else
				dsk_file->curfdr_physrec = get_win_fdr_next_FDR_physrec(dsk_file->l2_img, &dsk_file->curfdr);
			if (read_absolute_physrec(& dsk_file->l2_img->l1_img, dsk_file->curfdr_physrec, &dsk_file->curfdr))
				return IMGTOOLERR_READERROR;
		}
	}


	cluster_base_filephysrec = get_win_fdr_cursib_base_filephysrec(& dsk_file->curfdr);
	for (i = 0; i<54; i++)
	{
		cluster_last_filephysrec = cluster_base_filephysrec
									+ (get_UINT16BE(dsk_file->curfdr.clusters[i][1])
											- get_UINT16BE(dsk_file->curfdr.clusters[i][0])
											+ 1)
										* dsk_file->l2_img->AUformat.physrecsperAU;
		if (physrecnum < cluster_last_filephysrec)
			break;

		cluster_base_filephysrec = cluster_last_filephysrec;
	}

	if (i == 54)
		return IMGTOOLERR_CORRUPTIMAGE;


	/* return absolute physrec address */
	*out_physrec = get_UINT16BE(dsk_file->curfdr.clusters[i][0]) * dsk_file->l2_img->AUformat.physrecsperAU + (physrecnum - cluster_base_filephysrec);
	return 0;
}

/*
	read a 256-byte physical record from a file
*/
static int read_file_physrec(ti99_lvl2_fileref *l2_file, unsigned physrecnum, void *dest)
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
static int write_file_physrec(ti99_lvl2_fileref *l2_file, unsigned physrecnum, const void *src)
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

/*
	Write a field in every fdr record associated to a file
*/
static int set_win_fdr_field(ti99_lvl2_fileref *l2_file, size_t offset, size_t size, void *data)
{
	win_fdr fdr_buf;
	unsigned fdr_physrec;
	int errorcode = 0;

	for (fdr_physrec = l2_file->u.win.eldestfdr_physrec;
			fdr_physrec && ((errorcode = (read_absolute_physrec(&l2_file->u.win.l2_img->l1_img, fdr_physrec, &fdr_buf) ? IMGTOOLERR_READERROR : 0)) == 0);
			fdr_physrec = get_win_fdr_next_FDR_physrec(l2_file->u.win.l2_img, &fdr_buf))
	{
		memcpy(((UINT8 *) &fdr_buf) + offset, data, size);
		if (write_absolute_physrec(&l2_file->u.win.l2_img->l1_img, fdr_physrec, &fdr_buf))
		{
			errorcode = IMGTOOLERR_WRITEERROR;
			break;
		}
	}

	return errorcode;
}

static UINT8 get_file_flags(ti99_lvl2_fileref *l2_file)
{
	int reply = 0;

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
	int reply = 0;

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
	int reply = 0;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = get_UINT16BE(l2_file->u.dsk.fdr.secsused);
		break;

	case L2F_WIN:
		reply = l2_file->u.win.secsused;
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
		l2_file->u.win.secsused = data;
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
	int reply = 0;

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
	int reply = 0;

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
	int reply = 0;

	switch (l2_file->type)
	{
	case L2F_DSK:
		reply = get_UINT16LE(l2_file->u.dsk.fdr.fixrecs);
		break;

	case L2F_WIN:
		reply = get_win_fdr_fixrecs(&l2_file->u.win.curfdr);
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
		set_win_fdr_fixrecs(&l2_file->u.win.curfdr, data);
		break;

	case L2F_TIFILES:
		if (data >= 65536)
			return IMGTOOLERR_UNIMPLEMENTED;
		set_UINT16BE(&l2_file->u.tifiles.hdr.fixrecs, data);
		break;
	}

	return 0;
}

static void get_file_creation_date(ti99_lvl2_fileref *l2_file, ti99_date_time *reply)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		*reply = l2_file->u.dsk.fdr.creation;
		break;

	case L2F_WIN:
		*reply = l2_file->u.win.curfdr.creation;
		break;

	case L2F_TIFILES:
		memset(reply, 0, sizeof(*reply));
		break;
	}
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

static void get_file_update_date(ti99_lvl2_fileref *l2_file, ti99_date_time *reply)
{
	switch (l2_file->type)
	{
	case L2F_DSK:
		*reply = l2_file->u.dsk.fdr.update;
		break;

	case L2F_WIN:
		*reply = l2_file->u.win.curfdr.update;
		break;

	case L2F_TIFILES:
		memset(reply, 0, sizeof(*reply));
		break;
	}
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

static void current_data_time(ti99_date_time *reply)
{
	/* All these functions should be ANSI */
	time_t cur_time = time(NULL);
	struct tm expanded_time = *localtime(& cur_time);

	reply->time_MSB = (expanded_time.tm_hour << 3) | (expanded_time.tm_min >> 3);
	reply->time_LSB = (expanded_time.tm_min << 5) | (expanded_time.tm_sec >> 1);
	reply->date_MSB = ((expanded_time.tm_year % 100) << 1) | ((expanded_time.tm_mon+1) >> 3);
	reply->date_LSB = ((expanded_time.tm_mon+1) << 5) | expanded_time.tm_mday;
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
	int fdr_eof = get_file_eof(&l3_file->l2_file);

	if (flags & fdr99_f_var)
	{
		return (l3_file->cur_phys_rec >= secsused);
	}
	else
	{
		return ((l3_file->cur_phys_rec >= secsused)
				|| ((l3_file->cur_phys_rec == (secsused-1))
					&& (l3_file->cur_pos_in_phys_rec >= (fdr_eof ? fdr_eof : 256))));
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
	int fdr_eof = get_file_eof(&l3_file->l2_file);

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
					&& ((l3_file->cur_pos_in_phys_rec + reclen) >= (fdr_eof ? fdr_eof : 256))))
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
	ti99_dsk_image_beginenum,		/* begin enumeration */
	ti99_dsk_image_nextenum,		/* enumerate next */
	ti99_dsk_image_closeenum,		/* close enumeration */
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
	ti99_dsk_image_beginenum,		/* begin enumeration */
	ti99_dsk_image_nextenum,		/* enumerate next */
	ti99_dsk_image_closeenum,		/* close enumeration */
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
	ti99_dsk_image_beginenum,		/* begin enumeration */
	ti99_dsk_image_nextenum,		/* enumerate next */
	ti99_dsk_image_closeenum,		/* close enumeration */
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
	ti99_dsk_image_beginenum,		/* begin enumeration */
	ti99_dsk_image_nextenum,		/* enumerate next */
	ti99_dsk_image_closeenum,		/* close enumeration */
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
	ti99_image_freespace,			/* free space on image */
	ti99_image_readfile,			/* read file */
	ti99_image_writefile,			/* write file */
	/*ti99_image_deletefile*/NULL,	/* delete file */
	/*ti99_image_create_hd*/NULL,	/* create image */
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
			image->u.dsk.catalog.subdirs[image->u.dsk.catalog.num_subdirs].dir_ptr = fdir_physrec;
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
	int i;


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

	/* read allocation bitmap (physrecs 1 through n, n<=33) */
	for (i=0; i < (image->AUformat.totAUs+2047)/2048; i++)
	{
		reply = read_absolute_physrec(&image->l1_img, i+1, image->u.win.abm+i*256);
		if (reply)
			return reply;
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

			/* len in physrecs */
			ent->filesize = 1;

			/* recurse subdirectory */
			iter->listing_subdirs = 0;	/* no need to list subdirs as there is no subdirs in DSK */
			iter->level = 1;
			iter->cur_catalog = &iter->image->u.dsk.subdir_catalog[iter->index[0]];
			iter->index[iter->level] = 0;
		}
		else
		{
			fdr_physrec = iter->cur_catalog->files[iter->index[iter->level]].fdr_ptr;
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
			/* len in physrecs */
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
	win_fdr fdr;
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

			/* len in physrecs */
			ent->filesize = 2;

			/* recurse subdirectory */
			/*iter->listing_subdirs = 1;*/
			iter->level++;
			iter->index[iter->level] = 0;
			reply = win_read_catalog(iter->image, iter->catalog[iter->level-1].subdirs[iter->index[iter->level-1]].dir_ptr, &iter->catalog[iter->level]);
			if (reply)
			{
				ent->corrupt = 1;
				return reply;
			}
		}
		else
		{
			fdr_physrec = iter->catalog[iter->level].files[iter->index[iter->level]].fdr_ptr*iter->image->AUformat.physrecsperAU;
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
			/* len in physrecs */
			ent->filesize = get_win_fdr_secsused(&fdr);

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
	UINT8 *abm;
	int i;

	switch (image->type)
	{
	case L2I_DSK:
		abm = image->u.dsk.vib.abm;
		break;

	case L2I_WIN:
		abm = image->u.win.abm;
		break;

	default:
		abm = NULL;
		break;
	}

	freeAUs = 0;
	if (abm)
	{
		for (i=0; i<image->AUformat.totAUs; i++)
		{
			if (! (abm[i >> 3] & (1 << (i & 7))))
				freeAUs++;
		}
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
	switch (image->type)
	{
	case L2I_DSK:
		errorcode = open_file_lvl2_dsk(image, fname, &src_file);
		break;

	case L2I_WIN:
		errorcode = open_file_lvl2_win(image, fname, &src_file);
		break;

	default:
		errorcode = -1;
		break;
	}
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

	get_file_creation_date(&src_file, &date_time);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		current_data_time(&date_time);*/
	set_file_creation_date(&dst_file, date_time);

	get_file_update_date(&src_file, &date_time);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		current_data_time(&date_time);*/
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
	switch (image->type)
	{
	case L2I_DSK:
		errorcode = open_file_lvl2_dsk(image, fname, &src_file);
		break;

	case L2I_WIN:
		errorcode = open_file_lvl2_win(image, fname, &src_file);
		break;
	}
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
	ti99_catalog *catalog, catalog_buf;


	if (check_fpath(fpath))
		return IMGTOOLERR_BADFILENAME;

	switch (image->type)
	{
	case L2I_DSK:
		errorcode = dsk_find_catalog_entry(image, fpath, &parent_ref_valid, &parent_ref, NULL, NULL);
		if (errorcode == 0)
			/* file already exists: causes an error for now */
			return IMGTOOLERR_UNEXPECTED;
		else if ((! parent_ref_valid) || (errorcode != IMGTOOLERR_FILENOTFOUND))
			return errorcode;
		break;

	case L2I_WIN:
		errorcode = win_find_catalog_entry(image, fpath, &parent_ref_valid, &parent_ref, &catalog_buf, NULL, NULL);
		if (errorcode == 0)
			/* file already exists: causes an error for now */
			return IMGTOOLERR_UNEXPECTED;
		else if ((! parent_ref_valid) || (errorcode != IMGTOOLERR_FILENOTFOUND))
			return errorcode;
		break;
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
	switch (image->type)
	{
	case L2I_DSK:
		errorcode = new_file_lvl2_dsk(image, parent_ref, ti_fname, &dst_file);
		if (errorcode)
			return errorcode;
		break;

	case L2I_WIN:
		errorcode = new_file_lvl2_win(image, &catalog_buf, ti_fname, &dst_file);
		if (errorcode)
			return errorcode;
		break;
	}

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

	get_file_creation_date(&src_file, &date_time);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		current_data_time(&date_time);*/
	set_file_creation_date(&dst_file, date_time);

	get_file_update_date(&src_file, &date_time);
	/*if ((date_time.time_MSB == 0) || (date_time.time_LSB == 0)
			|| (date_time.date_MSB == 0) || (date_time.date_LSB == 0))
		current_data_time(&date_time);*/
	set_file_update_date(&dst_file, date_time);

	/* alloc data physrecs */
	secsused = get_file_secsused(&src_file);
	switch (dst_file.type)
	{
	case L2F_DSK:
		errorcode = dsk_alloc_file_physrecs(&dst_file.u.dsk, secsused);
		if (errorcode)
			return errorcode;
		break;

	case L2F_WIN:
		errorcode = win_alloc_file_physrecs(&dst_file.u.win, secsused);
		if (errorcode)
			return errorcode;
		break;

	case L2F_TIFILES:
		break;
	}

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
	switch (image->type)
	{
	case L2I_DSK:
		if (write_absolute_physrec(& image->l1_img, dst_file.u.dsk.fdr_physrec, &dst_file.u.dsk.fdr))
			return IMGTOOLERR_WRITEERROR;
		break;

	case L2I_WIN:
		/* save secsused field as well */
		if (dst_file.u.win.curfdr_physrec == dst_file.u.win.eldestfdr_physrec)
			set_win_fdr_secsused(&dst_file.u.win.curfdr, dst_file.u.win.secsused);
		if (write_absolute_physrec(& image->l1_img, dst_file.u.win.curfdr_physrec, &dst_file.u.win.curfdr))
			return IMGTOOLERR_WRITEERROR;
		if (dst_file.u.win.curfdr_physrec != dst_file.u.win.eldestfdr_physrec)
		{
			dst_file.u.win.curfdr_physrec = dst_file.u.win.eldestfdr_physrec;
			if (read_absolute_physrec(& image->l1_img, dst_file.u.win.curfdr_physrec, &dst_file.u.win.curfdr))
				return IMGTOOLERR_WRITEERROR;
			set_win_fdr_secsused(&dst_file.u.win.curfdr, dst_file.u.win.secsused);
			if (write_absolute_physrec(& image->l1_img, dst_file.u.win.curfdr_physrec, &dst_file.u.win.curfdr))
				return IMGTOOLERR_WRITEERROR;
		}
		break;
	}

	/* update catalog */
	switch (image->type)
	{
	case L2I_DSK:
		catalog = parent_ref ? &image->u.dsk.subdir_catalog[parent_ref-1] : &image->u.dsk.catalog;
		for (i=0; i<128; i++)
		{
			buf[2*i] = catalog->files[i].fdr_ptr >> 8;
			buf[2*i+1] = catalog->files[i].fdr_ptr & 0xff;
		}
		if (write_absolute_physrec(& image->l1_img, parent_ref ? image->u.dsk.catalog.subdirs[parent_ref-1].dir_ptr : 1, buf))
			return IMGTOOLERR_WRITEERROR;
		break;

	case L2I_WIN:
		{
			int fdir_AU;

			/* update VIB/DDR and get FDIR AU address */
			if (parent_ref == 0)
			{	/* parent is root directory, use VIB */
				image->u.win.vib.num_files = catalog_buf.num_files;
				if (write_absolute_physrec(& image->l1_img, 0, &image->u.win.vib))
					return IMGTOOLERR_WRITEERROR;
				fdir_AU = get_UINT16BE(image->u.win.vib.fdir_AU);
			}
			else
			{	/* parent is not root, read DDR */
				win_vib_ddr parent_ddr;

				if (read_absolute_physrec(& image->l1_img, parent_ref * image->AUformat.physrecsperAU, &parent_ddr))
					return IMGTOOLERR_READERROR;
				parent_ddr.num_files = catalog_buf.num_files;
				if (write_absolute_physrec(& image->l1_img, parent_ref * image->AUformat.physrecsperAU, &parent_ddr))
					return IMGTOOLERR_WRITEERROR;
				fdir_AU = get_UINT16BE(parent_ddr.fdir_AU);
			}
			/* generate FDIR and save it */
			for (i=0; i<127; i++)
			{
				buf[2*i] = catalog_buf.files[i].fdr_ptr >> 8;
				buf[2*i+1] = catalog_buf.files[i].fdr_ptr & 0xff;
			}
			buf[254] = parent_ref >> 8;
			buf[255] = parent_ref & 0xff;
			if (write_absolute_physrec(& image->l1_img, fdir_AU * image->AUformat.physrecsperAU, buf))
				return IMGTOOLERR_WRITEERROR;
		}
		break;
	}

	/* update bitmap */
	switch (image->type)
	{
	case L2I_DSK:
		if (write_absolute_physrec(& image->l1_img, 0, &image->u.dsk.vib))
			return IMGTOOLERR_WRITEERROR;
		break;
	
	case L2I_WIN:
		/* save allocation bitmap (physrecs 1 through n, n<=33) */
		for (i=0; i < (image->AUformat.totAUs+2047)/2048; i++)
			if (write_absolute_physrec(&image->l1_img, i+1, image->u.win.abm+i*256))
				return IMGTOOLERR_WRITEERROR;
		break;
	}

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

		/* free fdir AU */
		cur_AU = catalog->subdirs[catalog_index].dir_ptr / image->AUformat.physrecsperAU;
		image->u.dsk.vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

		/* delete catalog entry */
		for (i=catalog_index; i<2; i++)
			catalog->subdirs[i] = catalog->subdirs[i+1];
		catalog->subdirs[2].dir_ptr = 0;
		catalog->num_subdirs--;

		/* update directory in vib */
		for (i=0; i<3; i++)
		{
			memcpy(image->u.dsk.vib.subdir[i].name, catalog->subdirs[i].dirname, 10);
			set_UINT16BE(&image->u.dsk.vib.subdir[i].fdir_physrec, catalog->subdirs[i].dir_ptr);
		}

		/* write vib (bitmap+directory) */
		if (write_absolute_physrec(& image->l1_img, 0, &image->u.dsk.vib))
			return IMGTOOLERR_WRITEERROR;

		return IMGTOOLERR_UNIMPLEMENTED;
	}
	else
	{
		catalog = parent_ref ? &image->u.dsk.subdir_catalog[parent_ref-1] : &image->u.dsk.catalog;

		if (read_absolute_physrec(& image->l1_img, catalog->files[catalog_index].fdr_ptr, &fdr))
			return IMGTOOLERR_READERROR;

		/* free data AUs */
		secsused = get_UINT16BE(fdr.secsused);

		i = 0;
		cluster_index = 0;
		while (i<secsused)
		{
			if (cluster_index == 76)
				return IMGTOOLERR_CORRUPTIMAGE;

			cur_AU = get_dsk_fdr_cluster_baseAU(image, & fdr, cluster_index);
			cluster_lastphysrec = get_dsk_fdr_cluster_lastphysrec(& fdr, cluster_index);

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

		/* free fdr AU */
		cur_AU = catalog->files[catalog_index].fdr_ptr / image->AUformat.physrecsperAU;
		image->u.dsk.vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

		/* delete catalog entry */
		for (i=catalog_index; i<127; i++)
			catalog->files[i] = catalog->files[i+1];
		catalog->files[127].fdr_ptr = 0;
		catalog->num_files--;

		/* update parent fdir */
		for (i=0; i<128; i++)
		{
			buf[2*i] = catalog->files[i].fdr_ptr >> 8;
			buf[2*i+1] = catalog->files[i].fdr_ptr & 0xff;
		}
		if (write_absolute_physrec(& image->l1_img, parent_ref ? image->u.dsk.catalog.subdirs[parent_ref-1].dir_ptr : 1, buf))
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

	/* initialize vib in physrec 0 */

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

	/* mark first 2 physrecs (vib and fdir) as used */
	vib.abm[0] |= (physrecsperAU == 1) ? 3 : 1;

	/* write physrec 0 */
	if (write_absolute_physrec(& l1_img, 0, &vib))
		return IMGTOOLERR_WRITEERROR;


	/* now clear every other physrecs, including the FDIR record */
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
