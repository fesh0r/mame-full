/*
	Handlers for ti99 disk images

	Disk images are in v9t9/MESS format.  Files are extracted in TIFILES format.

	Raphael Nabet, 2002-2003
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
	Sector 1: File Descriptor Index Record (FDIR):
		array of 0 through 128 words, sector (physical record?) address of the fdr sector
		for each file.  Sorted by ascending file name.
	Remaining physical records are used for fdr and data.  Each file has one
	FDR (File Descriptor Record) descriptor which provides the file information
	(name, format, lenght, pointers to data sectors/AUs, etc).
*/

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

	Most fields this record are only revelant to level 2 routines, but level 1
	routines need the disk geometry information extracted from the VIB.
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

/*
	Read the volume information block (sector 0) assuming no geometry
	information.  (Called when an image is opened to figure out the
	geometry information.)

	file_handle: imgtool file handle
	dest: pointer to 256-byte destination buffer
*/
static int read_image_vib_no_geometry(STREAM *file_handle, ti99_vib *dest)
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
	Convert physical sector address to offset in disk image
*/
INLINE int sector_address_to_image_offset(const ti99_sector_address *address, const ti99_geometry *geometry)
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
	Read one 256-byte sector from a disk image

	file_handle: imgtool file handle
	address: physical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	dest: pointer to 256-byte destination buffer
*/
static int read_sector(STREAM *file_handle, const ti99_sector_address *address, const ti99_geometry *geometry, void *dest)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, sector_address_to_image_offset(address, geometry), SEEK_SET);
	if (reply)
		return 1;
	/* read it */
	reply = stream_read(file_handle, dest, 256);
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
static int write_sector(STREAM *file_handle, const ti99_sector_address *address, const ti99_geometry *geometry, const void *src)
{
	int reply;

	/* seek to sector */
	reply = stream_seek(file_handle, sector_address_to_image_offset(address, geometry), SEEK_SET);
	if (reply)
		return 1;
	/* write it */
	reply = stream_write(file_handle, src, 256);
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
static int read_absolute_physrec(STREAM *file_handle, int physrec, const ti99_geometry *geometry, void *dest)
{
	ti99_sector_address address;


	physrec_to_sector_address(physrec, geometry, &address);

	return read_sector(file_handle, &address, geometry, dest);
}

/*
	Write one 256-byte physical record to a disk image

	file_handle: imgtool file handle
	physrec: logical sector address
	geometry: disk geometry (sectors per track, tracks per side, sides)
	src: pointer to 256-byte source buffer
*/
static int write_absolute_physrec(STREAM *file_handle, int physrec, const ti99_geometry *geometry, const void *src)
{
	ti99_sector_address address;


	physrec_to_sector_address(physrec, geometry, &address);

	return write_sector(file_handle, &address, geometry, src);
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
	catalog entry (used for in-memory catalog)
*/
typedef struct catalog_entry
{
	UINT16 fdr_physrec;
	char fname[10];
} catalog_entry;

/*
	ti99 disk image descriptor
*/
typedef struct ti99_image
{
	IMAGE base;
	STREAM *file_handle;		/* imgtool file handle */
	ti99_geometry geometry;		/* geometry */
	ti99_vib vib;				/* cached copy of volume information block record in sector 0 */
	ti99_AUformat AUformat;		/* AU format */
	catalog_entry catalog[128];	/* catalog (fdr AU address from sector 1, and file names from fdrs) */
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
								/* often filled by 0xCA53 repeated 56 times */
								/* 10 chars: original TI file name */
								/* 4 bytes: unknown */
								/* 4 bytes: time & date of creation */
								/* 4 bytes: time & date of last update */
} tifile_header;


/*
	Find the catalog entry and fdr record associated with a file name

	image: ti99_image image record
	fname: name of the file to search
	fdr: pointer to buffer where the fdr record should be stored (may be NULL)
	catalog_index: on output, index of file catalog entry (may be NULL)
*/
static int find_fdr(ti99_image *image, char fname[10], ti99_fdr *fdr, int *catalog_index)
{
	int fdr_physrec;
	int i;


	for (i=0; (i<128) && ((fdr_physrec = image->catalog[i].fdr_physrec) != 0); i++)
	{
		if (memcmp(image->catalog[i].fname, fname, 10) == 0)
		{	/* file found */
			if (catalog_index)
				*catalog_index = i;

			if (fdr)
				if (read_absolute_physrec(image->file_handle, fdr_physrec, & image->geometry, fdr))
					return IMGTOOLERR_READERROR;

			return 0;
		}
	}

	return IMGTOOLERR_FILENOTFOUND;
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
static int new_file(ti99_image *image, char fname[10], int *out_fdr_physrec/*, ti99_fdr *fdr,*/)
{
	int fdr_physrec;
	int catalog_index, i;
	int reply = 0;
	int errorcode;


	/* find insertion point in catalog */
	for (i=0; (i<128) && ((fdr_physrec = image->catalog[i].fdr_physrec) != 0) && ((reply = memcmp(image->catalog[i].fname, fname, 10)) < 0); i++)
		;

	if (i == 128)
		/* catalog is full */
		return IMGTOOLERR_NOSPACE;
	else if (fdr_physrec && (reply == 0))
		/* file already exists */
		return IMGTOOLERR_BADFILENAME;
	else
	{
		/* otherwise insert new entry */
		catalog_index = i;
		errorcode = alloc_fdr_sector(image, &fdr_physrec);
		if (errorcode)
			return errorcode;

		/* look for first free entry in catalog */
		for (i=catalog_index; image->catalog[i].fdr_physrec != 0; i++)
			;

		if (i == 128)
			/* catalog is full */
				return IMGTOOLERR_NOSPACE;

		/* shift catalog entries until the insertion point */
		for (/*i=127*/; i>catalog_index; i--)
			image->catalog[i] = image->catalog[i-1];

		/* write new catalog entry */
		image->catalog[catalog_index].fdr_physrec = fdr_physrec;
		memcpy(image->catalog[catalog_index].fname, fname, 10);
		if (out_fdr_physrec)
			*out_fdr_physrec = fdr_physrec;
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
	if (read_absolute_physrec(image->file_handle, physrec, & image->geometry, dest))
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
	if (write_absolute_physrec(image->file_handle, physrec, & image->geometry, src))
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

INLINE void initialize_file_pos(ti99_file_pos *file_pos)
{
	memset(file_pos, 0, sizeof(*file_pos));

	/*if ()*/
}

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
	{ "label",	"Volume name", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,	0,	NULL},
	{ "density",	"1 for SD, 2 for 40-track DD, 3 for 80-track DD and HD", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	3,	"2" },
	{ "sides",	NULL, IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	2,	"2" },
	{ "tracks",	NULL, IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	80,	"40" },
	{ "sectors",	"1->9 for SD, 1->18 for DD, 1->36 for HD", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	36,	"18" },
	{ "protection",	"0 for normal, 1 for protected", IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	0,	1,	"0" },
	{ NULL, NULL, 0, 0, 0, 0 }
};

enum
{
	ti99_createopts_volname = 0,
	ti99_createopts_density = 1,
	ti99_createopts_sides = 2,
	ti99_createopts_tracks = 3,
	ti99_createopts_sectors = 4,
	ti99_createopts_protection = 5
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
	Compare two (possibly empty) catalog entry for qsort
*/
static int qsort_catalog_compare(const void *p1, const void *p2)
{
	const catalog_entry *entry1 = p1;
	const catalog_entry *entry2 = p2;

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
	Open a file as a ti99_image.
*/
static int ti99_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	ti99_image *image;
	int reply;
	UINT8 buf[256];
	ti99_fdr fdr;
	int totphysrecs;
	int i;


	image = malloc(sizeof(ti99_image));
	* (ti99_image **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(ti99_image));
	image->base.module = mod;
	image->file_handle = f;

	/* read vib */
	reply = read_image_vib_no_geometry(f, & image->vib);
	if (reply)
	{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_READERROR;
	}

	totphysrecs = get_UINT16BE(image->vib.totphysrecs);

	image->geometry.secspertrack = image->vib.secspertrack;
	if (image->geometry.secspertrack == 0)
		/* Some images might be like this, because the original SSSD TI
		controller always assumes 9. */
		image->geometry.secspertrack = 9;
	image->geometry.tracksperside = image->vib.tracksperside;
	if (image->geometry.tracksperside == 0)
		/* Some images are like this, because the original SSSD TI controller
		always assumes 40. */
		image->geometry.tracksperside = 40;
	image->geometry.sides = image->vib.sides;
	if (image->geometry.sides == 0)
		/* Some images are like this, because the original SSSD TI controller
		always assumes that tracks beyond 40 are on side 2. */
		image->geometry.sides = totphysrecs / (image->geometry.secspertrack * image->geometry.tracksperside);

	image->AUformat.physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < image->AUformat.physrecsperAU; i <<= 1)
		;
	image->AUformat.physrecsperAU = i;
	image->AUformat.totAUs = totphysrecs / image->AUformat.physrecsperAU;

	if ((totphysrecs != (image->geometry.secspertrack * image->geometry.tracksperside * image->geometry.sides))
		|| (totphysrecs < 2)
		|| memcmp(image->vib.id, "DSK", 3) || (! strchr(" P", image->vib.id[3]))
		|| (stream_size(f) != totphysrecs*256))
	{
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
	}

#if 0
	if (image->AUformat.physrecsperAU != 1)
	{	/* We only support 1 record per AU - for now */
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_UNIMPLEMENTED;
	}
#endif

	/* read catalog */
	reply = read_absolute_physrec(f, 1, & image->geometry, buf);
	if (reply)
	{
		free(image);
		*outimg = NULL;
		return IMGTOOLERR_READERROR;
	}

	for (i=0; i<128; i++)
		image->catalog[i].fdr_physrec = (buf[i*2] << 8) | buf[i*2+1];

	for (i=0; i<128; i++)
	{
		if (image->catalog[i].fdr_physrec >= totphysrecs)
		{
			free(image);
			*outimg = NULL;
			return IMGTOOLERR_CORRUPTIMAGE;
		}
		else if (image->catalog[i].fdr_physrec)
		{
			reply = read_absolute_physrec(f, image->catalog[i].fdr_physrec, & image->geometry, &fdr);
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
	for (i=0; i<127; i++)
	{
		if (((! image->catalog[i].fdr_physrec) && image->catalog[i+1].fdr_physrec)
			|| ((image->catalog[i].fdr_physrec && image->catalog[i+1].fdr_physrec) && (memcmp(image->catalog[i].fname, image->catalog[i+1].fname, 10) >= 0)))
		{
			/* if the catalog is not sorted, we repair it */
			qsort(image->catalog, sizeof(image->catalog)/sizeof(image->catalog[0]),
					sizeof(image->catalog[0]), qsort_catalog_compare);
			break;
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
	iter->index = 0;

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
	int totphysrecs = get_UINT16BE(iter->image->vib.totphysrecs);

	ent->corrupt = 0;
	ent->eof = 0;

	if ((iter->index == 128) || ((fdr_physrec = iter->image->catalog[iter->index].fdr_physrec) == 0))
	{
		ent->eof = 1;
	}
	else
	{
		if (fdr_physrec >= totphysrecs)
		{
			ent->corrupt = 1;
		}
		else
		{
			reply = read_absolute_physrec(iter->image->file_handle, fdr_physrec, & iter->image->geometry, & fdr);
			if (reply)
				return IMGTOOLERR_READERROR;
			fname_to_str(ent->fname, fdr.name, ent->fname_len);

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
	int totphysrecs;
	char ti_fname[10];
	ti99_fdr fdr;
	tifile_header dst_header;
	int i;
	int secsused;
	UINT8 buf[256];
	int errorcode;

	totphysrecs = get_UINT16BE(image->vib.totphysrecs);

	str_to_fname(ti_fname, fname);

	/* open file on TI image */
	errorcode = find_fdr(image, ti_fname, &fdr, NULL);
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
static int ti99_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *in_options)
{
	ti99_image *image = (ti99_image*) img;
	int totphysrecs;
	char ti_fname[10];
	ti99_fdr fdr;
	tifile_header src_header;
	int i;
	int secsused;
	UINT8 buf[256];
	int errorcode;
	int fdr_physrec;

	totphysrecs = get_UINT16BE(image->vib.totphysrecs);

	if (strchr(fname, '.') || strchr(fname, ' ') || (strlen(fname) > 10))
		return IMGTOOLERR_BADFILENAME;
	str_to_fname(ti_fname, fname);

	errorcode = find_fdr(image, ti_fname, &fdr, NULL);
	if (errorcode == 0)
	{	/* file already exists: causes an error for now */
		return IMGTOOLERR_UNEXPECTED;
	}
	else if (errorcode != IMGTOOLERR_FILENOTFOUND)
	{
		return errorcode;
	}

	if (stream_read(sourcef, & src_header, 128) != 128)
		return IMGTOOLERR_READERROR;

	/* create new file */
	errorcode = new_file(image, ti_fname, &fdr_physrec);
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
	if (write_absolute_physrec(image->file_handle, fdr_physrec, & image->geometry, &fdr))
		return IMGTOOLERR_WRITEERROR;

	/* update catalog */
	for (i=0; i<128; i++)
	{
		buf[2*i] = image->catalog[i].fdr_physrec >> 8;
		buf[2*i+1] = image->catalog[i].fdr_physrec & 0xff;
	}
	if (write_absolute_physrec(image->file_handle, 1, & image->geometry, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_absolute_physrec(image->file_handle, 0, & image->geometry, &image->vib))
		return IMGTOOLERR_WRITEERROR;


	return 0;
}

/*
	Delete a file from a ti99_image.
*/
static int ti99_image_deletefile(IMAGE *img, const char *fname)
{
	ti99_image *image = (ti99_image*) img;
	char ti_fname[10];
	ti99_fdr fdr;
	int i, cluster_index;
	int cur_AU, cluster_lastphysrec;
	int secsused;
	int catalog_index;
	int errorcode;
	UINT8 buf[256];

	str_to_fname(ti_fname, fname);

	errorcode = find_fdr(image, ti_fname, &fdr, &catalog_index);
	if (errorcode)
		return errorcode;

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
	cur_AU = image->catalog[catalog_index].fdr_physrec / image->AUformat.physrecsperAU;
	image->vib.abm[cur_AU >> 3] &= ~ (1 << (cur_AU & 7));

	/* delete catalog entry */
	for (i=catalog_index; i<127; i++)
		image->catalog[i] = image->catalog[i+1];
	image->catalog[127].fdr_physrec = 0;

	/* update catalog */
	for (i=0; i<128; i++)
	{
		buf[2*i] = image->catalog[i].fdr_physrec >> 8;
		buf[2*i+1] = image->catalog[i].fdr_physrec & 0xff;
	}
	if (write_absolute_physrec(image->file_handle, 1, & image->geometry, buf))
		return IMGTOOLERR_WRITEERROR;

	/* update bitmap */
	if (write_absolute_physrec(image->file_handle, 0, & image->geometry, &image->vib))
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
	int protected;

	int totphysrecs, physrecsperAU, totAUs;

	ti99_vib vib;
	UINT8 empty_sec[256];

	int i;


	(void) mod;

	/* read options */
	volname = in_options[ti99_createopts_volname].s;
	density = in_options[ti99_createopts_density].i;
	geometry.sides = in_options[ti99_createopts_sides].i;
	geometry.tracksperside = in_options[ti99_createopts_tracks].i;
	geometry.secspertrack = in_options[ti99_createopts_sectors].i;
	protected = in_options[ti99_createopts_protection].i;

	totphysrecs = geometry.secspertrack * geometry.tracksperside * geometry.sides;
	physrecsperAU = (totphysrecs + 1599) / 1600;
	/* round to next larger power of 2 */
	for (i = 1; i < physrecsperAU; i <<= 1)
		;
	physrecsperAU = i;
	totAUs = totphysrecs / physrecsperAU;

	/* check number of tracks if 40-track image */
	if ((density < 3) && (geometry.tracksperside > 40))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* FM disks can hold fewer sector per track than MFM disks */
	if ((density == 1) && (geometry.secspertrack > 9))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* DD disks can hold fewer sector per track than HD disks */
	if ((density == 2) && (geometry.secspertrack > 18))
		return IMGTOOLERR_PARAMTOOLARGE;

	/* check total disk size */
	if (totphysrecs < 2)
		return IMGTOOLERR_PARAMTOOSMALL;

	/* initialize sector 0 */

	/* set volume name */
	str_to_fname(vib.name, volname);

	/* set every header field */
	set_UINT16BE(& vib.totphysrecs, totphysrecs);
	vib.secspertrack = geometry.secspertrack;
	vib.id[0] = 'D';
	vib.id[1] = 'S';
	vib.id[2] = 'K';
	vib.protection = protected ? 'P' : ' ';
	vib.tracksperside = geometry.tracksperside;
	vib.sides = geometry.sides;
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
	if (write_absolute_physrec(f, 0, & geometry, &vib))
		return IMGTOOLERR_WRITEERROR;


	/* now clear every other sector, including catalog */
	memset(empty_sec, 0, 256);

	for (i=1; i<totphysrecs; i++)
		if (write_absolute_physrec(f, i, & geometry, empty_sec))
			return IMGTOOLERR_WRITEERROR;


	return 0;
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
