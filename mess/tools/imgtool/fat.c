/****************************************************************************

	fat.c

	PC FAT disk images

*****************************************************************************

  Boot sector format:

  Offset  Length  Description
  ------  ------  -----------
       0       3  Jump instruction (to skip over header on boot)
	   3       8  OEM Name
      11       2  Bytes per sector
	  13       1  Sectors per cluster
	  14       2  Reserved sector count (including boot sector)
	  16       1  Number of FATs (file allocation tables)
	  17       2  Number of root directory entries
	  19       2  Total sectors (bits 0-15)
	  21       1  Media descriptor
	  22       2  Sectors per FAT
	  24       2  Sectors per track
	  26       2  Number of heads
	  28       4  Hidden sectors
	  32       4  Total sectors (bits 16-47)
	  36       1  Physical drive number
      37       1  Current head
	  38       1  Signature
	  39       4  ID
	  43      11  Volume Label
	  54       8  FAT file system type 

  For more information:
	http://support.microsoft.com/kb/q140418/


  Directory Entry Format:

  Offset  Length  Description
  ------  ------  -----------
       0       8  DOS File Name (padded with spaces)
	   8       3  DOS File Extension (padded with spaces)
	  11       1  File Attributes
	  12       2  Unknown
	  14       4  Time of Creation
	  18       2  Last Access Time
	  20       2  EA-Index (OS/2 stuff)
	  22       4  Last Modified Time
	  26       2  First Cluster
	  28       4  File Size


  LFN Entry Format:

  Offset  Length  Description
  ------  ------  -----------
       0       1  Sequence Number (bit 6 is set on highest sequence)
	   1      10  Name characters (five UTF-16LE chars)
	  11       1  Attributes (always 0x0F)
	  12       1  Reserved (always 0x00)
	  13       1  Checksum of short filename entry
	  14      12  Name characters (six UTF-16LE chars)
	  26       2  Entry Cluster (always 0x00)
	  28       4  Name characters (two UTF-16LE chars)
  
  Valid characters in DOS file names:
	- Upper case letters A-Z
	- Numbers 0-9
	- Space (though there is no way to identify a trailing space)
	- ! # $ % & ( ) - @ ^ _ ` { } ~ 
	- Characters 128-255 (though the code page is indeterminate)

****************************************************************************/

#include <time.h>
#include <ctype.h>

#include "imgtool.h"
#include "formats/pc_dsk.h"
#include "iflopimg.h"
#include "unicode.h"

#define FAT_DIRENT_SIZE	32

struct fat_diskinfo
{
	UINT32 fat_bits;
	UINT32 sector_size;
	UINT32 sectors_per_cluster;
	UINT32 cluster_size;
	UINT32 reserved_sectors;
	UINT32 fat_count;
	UINT32 root_entries;
	UINT32 sectors_per_fat;
	UINT32 sectors_per_track;
	UINT32 heads;
	UINT64 total_sectors;
	UINT32 total_clusters;
};

struct fat_file
{
	unsigned int root : 1;
	unsigned int directory : 1;
	unsigned int eof : 1;
	UINT32 index;
	UINT32 filesize;
	UINT32 first_cluster;
	UINT32 parent_first_cluster;
	UINT32 cluster;
	UINT32 cluster_index;
	UINT32 dirent_sector_index;
	UINT32 dirent_sector_offset;
};

struct fat_dirent
{
	char long_filename[512];
	char short_filename[13];
	unsigned int directory : 1;
	unsigned int eof : 1;
	UINT32 filesize;
	UINT32 first_cluster;
	UINT32 dirent_sector_index;
	UINT32 dirent_sector_offset;
};

struct fat_freeentry_info
{
	UINT32 required_size;
	UINT32 candidate_position;
	UINT32 position;
};

struct fat_mediatype
{
	UINT8 media_descriptor;
	UINT8 heads;
	UINT8 tracks;
	UINT8 sectors;
};

typedef enum
{
	CREATE_NONE,
	CREATE_FILE,
	CREATE_DIR,
} creation_policy_t;



static struct fat_mediatype known_media[] =
{
	{ 0xF0, 2, 80, 36 },
	{ 0xF0, 2, 80, 18 },
	{ 0xF9, 2, 80,  9 },
	{ 0xF9, 2, 80, 15 },
	{ 0xFD, 2, 40,  9 },
	{ 0xFF, 2, 40,  8 },
	{ 0xFC, 1, 40,  9 },
	{ 0xFE, 1, 40,  8 },
	{ 0xF8, 0,  0,  0 }
};

/* boot sector code taken from FreeDOS */
static const UINT8 boot_sector_code[] =
{
	0xfa, 0xfc, 0x31, 0xc0, 0x8e, 0xd8, 0xbd, 0x00, 0x7c, 0xb8, 0xe0, 0x1f, 
	0x8e, 0xc0, 0x89, 0xee, 0x89, 0xef, 0xb9, 0x00, 0x01, 0xf3, 0xa5, 0xea, 
	0x5e, 0x7c, 0xe0, 0x1f, 0x00, 0x00, 0x60, 0x00, 0x8e, 0xd8, 0x8e, 0xd0, 
	0x8d, 0x66, 0xa0, 0xfb, 0x80, 0x7e, 0x24, 0xff, 0x75, 0x03, 0x88, 0x56, 
	0x24, 0xc7, 0x46, 0xc0, 0x10, 0x00, 0xc7, 0x46, 0xc2, 0x01, 0x00, 0x8b, 
	0x76, 0x1c, 0x8b, 0x7e, 0x1e, 0x03, 0x76, 0x0e, 0x83, 0xd7, 0x00, 0x89, 
	0x76, 0xd2, 0x89, 0x7e, 0xd4, 0x8a, 0x46, 0x10, 0x98, 0xf7, 0x66, 0x16, 
	0x01, 0xc6, 0x11, 0xd7, 0x89, 0x76, 0xd6, 0x89, 0x7e, 0xd8, 0x8b, 0x5e, 
	0x0b, 0xb1, 0x05, 0xd3, 0xeb, 0x8b, 0x46, 0x11, 0x31, 0xd2, 0xf7, 0xf3, 
	0x89, 0x46, 0xd0, 0x01, 0xc6, 0x83, 0xd7, 0x00, 0x89, 0x76, 0xda, 0x89, 
	0x7e, 0xdc, 0x8b, 0x46, 0xd6, 0x8b, 0x56, 0xd8, 0x8b, 0x7e, 0xd0, 0xc4, 
	0x5e, 0x5a, 0xe8, 0xac, 0x00, 0xc4, 0x7e, 0x5a, 0xb9, 0x0b, 0x00, 0xbe, 
	0xf1, 0x7d, 0x57, 0xf3, 0xa6, 0x5f, 0x26, 0x8b, 0x45, 0x1a, 0x74, 0x0b, 
	0x83, 0xc7, 0x20, 0x26, 0x80, 0x3d, 0x00, 0x75, 0xe7, 0x72, 0x68, 0x50, 
	0xc4, 0x5e, 0x5a, 0x8b, 0x7e, 0x16, 0x8b, 0x46, 0xd2, 0x8b, 0x56, 0xd4, 
	0xe8, 0x7e, 0x00, 0x58, 0x1e, 0x07, 0x8e, 0x5e, 0x5c, 0xbf, 0x00, 0x20, 
	0xab, 0x89, 0xc6, 0x01, 0xf6, 0x01, 0xc6, 0xd1, 0xee, 0xad, 0x73, 0x04, 
	0xb1, 0x04, 0xd3, 0xe8, 0x80, 0xe4, 0x0f, 0x3d, 0xf8, 0x0f, 0x72, 0xe8, 
	0x31, 0xc0, 0xab, 0x0e, 0x1f, 0xc4, 0x5e, 0x5a, 0xbe, 0x00, 0x20, 0xad, 
	0x09, 0xc0, 0x75, 0x05, 0x88, 0xd3, 0xff, 0x6e, 0x5a, 0x48, 0x48, 0x8b, 
	0x7e, 0x0d, 0x81, 0xe7, 0xff, 0x00, 0xf7, 0xe7, 0x03, 0x46, 0xda, 0x13, 
	0x56, 0xdc, 0xe8, 0x34, 0x00, 0xeb, 0xe0, 0x5e, 0xac, 0x56, 0xb4, 0x0e, 
	0xcd, 0x10, 0x3c, 0x2e, 0x75, 0xf5, 0xc3, 0xe8, 0xf1, 0xff, 0x45, 0x72, 
	0x72, 0x6f, 0x72, 0x21, 0x20, 0x48, 0x69, 0x74, 0x20, 0x61, 0x20, 0x6b, 
	0x65, 0x79, 0x20, 0x74, 0x6f, 0x20, 0x72, 0x65, 0x62, 0x6f, 0x6f, 0x74, 
	0x2e, 0x30, 0xe4, 0xcd, 0x13, 0xcd, 0x16, 0xcd, 0x19, 0x56, 0x89, 0x46, 
	0xc8, 0x89, 0x56, 0xca, 0x8c, 0x46, 0xc6, 0x89, 0x5e, 0xc4, 0xe8, 0xbe, 
	0xff, 0x2e, 0xb4, 0x41, 0xbb, 0xaa, 0x55, 0x8a, 0x56, 0x24, 0x84, 0xd2, 
	0x74, 0x19, 0xcd, 0x13, 0x72, 0x15, 0xd1, 0xe9, 0x81, 0xdb, 0x54, 0xaa, 
	0x75, 0x0d, 0x8d, 0x76, 0xc0, 0x89, 0x5e, 0xcc, 0x89, 0x5e, 0xce, 0xb4, 
	0x42, 0xeb, 0x26, 0x8b, 0x4e, 0xc8, 0x8b, 0x56, 0xca, 0x8a, 0x46, 0x18, 
	0xf6, 0x66, 0x1a, 0x91, 0xf7, 0xf1, 0x92, 0xf6, 0x76, 0x18, 0x89, 0xd1, 
	0x88, 0xc6, 0x86, 0xe9, 0xd0, 0xc9, 0xd0, 0xc9, 0x08, 0xe1, 0x41, 0xc4, 
	0x5e, 0xc4, 0xb8, 0x01, 0x02, 0x8a, 0x56, 0x24, 0xcd, 0x13, 0x0f, 0x82, 
	0x75, 0xff, 0x8b, 0x46, 0x0b, 0xf6, 0x76, 0xc0, 0x01, 0x46, 0xc6, 0x83, 
	0x46, 0xc8, 0x01, 0x83, 0x56, 0xca, 0x00, 0x4f, 0x75, 0x98, 0x8e, 0x46, 
	0xc6, 0x5e, 0xc3, 0x4d, 0x45, 0x54, 0x41, 0x4b, 0x45, 0x52, 0x4e, 0x53, 
	0x59, 0x53, 0x00, 0x00, 0x55, 0xaa
};



static UINT32 pick_integer(const void *ptr, size_t offset, size_t length)
{
	UINT32 value = 0;
	UINT8 b;

	while(length--)
	{
		b = ((UINT8 *) ptr)[offset + length];
		value <<= 8;
		value |= b;
	}
	return value;
}



static void place_integer(void *ptr, size_t offset, size_t length, UINT32 value)
{
	UINT8 b;
	size_t i = 0;

	while(length--)
	{
		b = (UINT8) value;
		value >>= 8;
		((UINT8 *) ptr)[offset + i++] = b;
	}
}



static int is_special_cluster(UINT32 fat_bits, UINT32 cluster)
{
	UINT32 mask;

	mask = 0xfffffff0;
	if (fat_bits < 32)
		mask &= (1 << fat_bits) - 1;

	return (cluster & mask) == mask;
}



static char cannonicalize_sfn_char(char ch)
{
	/* return the display version of this short file name character */
	return tolower(ch);
}



static imgtoolerr_t fat_diskimage_open(imgtool_image *image)
{
	UINT8 header[62];
	floperr_t ferr;
	struct fat_diskinfo *info;
	UINT32 fat_bits, total_sectors_l, total_sectors_h;

	ferr = floppy_read_sector(imgtool_floppy(image), 0, 0, 1, 0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	/* figure out which FAT type this is */
	if (!memcmp(&header[54], "FAT     ", 8))
		fat_bits = 8;
	else if (!memcmp(&header[54], "FAT12   ", 8))
		fat_bits = 12;
	else if (!memcmp(&header[54], "FAT16   ", 8))
		fat_bits = 16;
	else if (!memcmp(&header[54], "FAT32   ", 8))
		fat_bits = 32;
	else
		return IMGTOOLERR_CORRUPTIMAGE;
	
	info = (struct fat_diskinfo *) imgtool_floppy_extrabytes(image);
	info->fat_bits				= fat_bits;
	info->sector_size			= pick_integer(header, 11, 2);
	info->sectors_per_cluster	= pick_integer(header, 13, 1);
	info->reserved_sectors		= pick_integer(header, 14, 2);
	info->fat_count				= pick_integer(header, 16, 1);
	info->root_entries			= pick_integer(header, 17, 2);
	total_sectors_l				= pick_integer(header, 19, 2);
	info->sectors_per_fat		= pick_integer(header, 22, 2);
	info->sectors_per_track		= pick_integer(header, 24, 2);
	info->heads					= pick_integer(header, 26, 2);
	total_sectors_h				= pick_integer(header, 32, 4);

	info->total_sectors = total_sectors_l + (((UINT64) total_sectors_h) << 16);
	info->total_clusters = info->total_sectors - info->reserved_sectors
		- (info->sectors_per_fat * info->fat_count)
		- (info->root_entries * FAT_DIRENT_SIZE + info->sector_size - 1) / info->sector_size;
	info->cluster_size = info->sector_size * info->sectors_per_cluster;

	if (info->fat_count == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_fat == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sector_size == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_cluster == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->reserved_sectors == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_track == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->heads == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->total_sectors < info->heads * info->sectors_per_track)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->total_clusters * info->fat_bits > info->sectors_per_fat * info->sector_size * 8)
		return IMGTOOLERR_CORRUPTIMAGE;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_create(imgtool_image *image, option_resolution *opts)
{
	floperr_t ferr;
	UINT32 heads, tracks, sectors, sector_bytes, first_sector_id;
	UINT32 fat_bits, sectors_per_cluster, reserved_sectors, hidden_sectors;
	UINT32 root_dir_count, root_dir_sectors;
	UINT32 sectors_per_fat, fat_count, i;
	UINT32 boot_sector_offset;
	UINT64 total_sectors, total_clusters;
	UINT8 media_descriptor;
	const char *title;
	const char *fat_bits_string;
	UINT8 header[512];
	UINT64 first_fat_entries;
	int head, track, sector;

	heads = option_resolution_lookup_int(opts, 'H');
	tracks = option_resolution_lookup_int(opts, 'T');
	sectors = option_resolution_lookup_int(opts, 'S');
	sector_bytes = 512;
	first_sector_id = 1;
	
	total_sectors = ((UINT64) heads) * tracks * sectors;

	/* cap our sector count so that we only use FAT12/16 */
	sectors_per_cluster = (total_sectors + 65524 - 1) / 65524;

	/* compute the FAT file system type */
	if ((total_sectors / sectors_per_cluster) <= 4084)
	{
		fat_bits = 12;
		fat_bits_string = "FAT12   ";
	}
	else if ((total_sectors / sectors_per_cluster) <= 65524)
	{
		fat_bits = 16;
		fat_bits_string = "FAT16   ";
	}
	else
	{
		fat_bits = 32;
		fat_bits_string = "FAT32   ";
	}

	/* figure out media type */
	i = 0;
	while((known_media[i].heads > 0) && ((known_media[i].heads != heads)
		|| (known_media[i].tracks != tracks)
		|| (known_media[i].sectors != sectors)))
	{
		i++;
	}
	media_descriptor = known_media[i].media_descriptor;

	/* other miscellaneous settings */
	title = "";
	fat_count = 2;
	root_dir_count = 512;
	hidden_sectors = 0;
	reserved_sectors = 1;

	/* calculated settings */
	root_dir_sectors = (root_dir_count * FAT_DIRENT_SIZE + sector_bytes - 1) / sector_bytes;
	total_clusters = (total_sectors - reserved_sectors - hidden_sectors - root_dir_sectors)
		/ sectors_per_cluster;
	sectors_per_fat = (total_clusters * fat_bits + (sector_bytes * 8) - 1)
		/ (sector_bytes * 8);

	memset(header, 0, sizeof(header));
	memcpy(&header[3], "IMGTOOL ", 8);
	place_integer(header, 11, 2, sector_bytes);
	place_integer(header, 13, 1, sectors_per_cluster);
	place_integer(header, 14, 1, reserved_sectors);
	place_integer(header, 16, 1, fat_count);
	place_integer(header, 17, 2, root_dir_count);
	place_integer(header, 19, 2, (UINT16) (total_sectors >> 0));
	place_integer(header, 21, 1, media_descriptor);
	place_integer(header, 22, 2, sectors_per_fat);
	place_integer(header, 24, 2, sectors);
	place_integer(header, 26, 2, heads);
	place_integer(header, 28, 4, hidden_sectors);
	place_integer(header, 32, 4, (UINT32) (total_sectors >> 16));
	place_integer(header, 38, 1, 0x28);
	place_integer(header, 39, 4, rand());
	memcpy(&header[43], "           ", 11);
	memcpy(&header[54], fat_bits_string, 8);

	/* store boot code */
	boot_sector_offset = sizeof(header) - sizeof(boot_sector_code);
	if (boot_sector_offset < 62)
		return IMGTOOLERR_UNEXPECTED;	/* sanity check */
	if (boot_sector_offset > 510)
		return IMGTOOLERR_UNEXPECTED;	/* sanity check */
	memcpy(&header[boot_sector_offset], boot_sector_code, sizeof(boot_sector_code));

	/* specify jump instruction */
	if (boot_sector_offset <= 129)
	{
		header[0] = 0xEB;									 /* JMP rel8 */
		header[1] = (UINT8) (boot_sector_offset - 2);		 /* (offset) */
		header[2] = 0x90;									 /* NOP */
	}
	else
	{
		header[0] = 0xE9;									 /* JMP rel16 */
		header[1] = (UINT8) ((boot_sector_offset - 2) >> 0); /* (offset) */
		header[2] = (UINT8) ((boot_sector_offset - 2) >> 8); /* (offset) */
	}

	ferr = floppy_write_sector(imgtool_floppy(image), 0, 0, first_sector_id,
		0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	/* clear out file allocation table */
	for (i = reserved_sectors; i < (reserved_sectors + sectors_per_fat * fat_count + root_dir_sectors); i++)
	{
		head = (i / sectors) % heads;
		track = i / sectors / heads;
		sector = 1 + (i % sectors);
		ferr = floppy_clear_sector(imgtool_floppy(image), head, track, sector, 0);
		if (ferr)
			return imgtool_floppy_error(ferr);
	}

	/* set first two FAT entries */
	first_fat_entries = ((UINT64) media_descriptor) | 0xFFFFFF00;
	first_fat_entries &= (((UINT64) 1) << fat_bits) - 1;
	first_fat_entries |= ((((UINT64) 1) << fat_bits) - 1) << fat_bits;
	first_fat_entries = LITTLE_ENDIANIZE_INT64(first_fat_entries);

	for (i = 0; i < fat_count; i++)
	{
		ferr = floppy_write_sector(imgtool_floppy(image), 0, 0,
			first_sector_id + 1 + (i * sectors_per_fat), 0, &first_fat_entries,
			fat_bits * 2 / 8);
		if (ferr)
			return imgtool_floppy_error(ferr);
	}

	return IMGTOOLERR_SUCCESS;
}



static void fat_get_sector_position(imgtool_image *image, UINT32 sector_index,
	int *head, int *track, int *sector)
{
	const struct fat_diskinfo *disk_info;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	*head = (sector_index / disk_info->sectors_per_track) % disk_info->heads;
	*track = sector_index / disk_info->sectors_per_track / disk_info->heads;
	*sector = 1 + (sector_index % disk_info->sectors_per_track);
}



static imgtoolerr_t fat_read_sector(imgtool_image *image, UINT32 sector_index,
	int offset, void *buffer, size_t buffer_len)
{
	floperr_t ferr;
	int head, track, sector;

	fat_get_sector_position(image, sector_index, &head, &track, &sector);
	ferr = floppy_read_sector(imgtool_floppy(image), head, track, sector, offset, buffer, buffer_len);
	if (ferr)
		return imgtool_floppy_error(ferr);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_write_sector(imgtool_image *image, UINT32 sector_index,
	int offset, const void *buffer, size_t buffer_len)
{
	floperr_t ferr;
	int head, track, sector;

	fat_get_sector_position(image, sector_index, &head, &track, &sector);
	ferr = floppy_write_sector(imgtool_floppy(image), head, track, sector, offset, buffer, buffer_len);
	if (ferr)
		return imgtool_floppy_error(ferr);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_load_fat(imgtool_image *image, UINT8 **fat_table)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT8 *table;
	UINT32 table_size;
	UINT32 pos, len;
	UINT32 sector_index;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	table_size = disk_info->sectors_per_fat * disk_info->fat_count * disk_info->sector_size;

	/* allocate the table with extra bytes, in case we "overextend" our reads */
	table = malloc(table_size + sizeof(UINT64));
	if (!table)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(table, 0, table_size + sizeof(UINT64));

	pos = 0;
	sector_index = disk_info->reserved_sectors;

	while(pos < table_size)
	{
		len = MIN(table_size - pos, disk_info->sector_size);

		err = fat_read_sector(image, sector_index++, 0, &table[pos], len);
		if (err)
			goto done;

		pos += disk_info->sector_size;
	}

done:
	if (err && table)
	{
		free(table);
		table = NULL;
	}
	*fat_table = table;
	return err;
}



static imgtoolerr_t fat_save_fat(imgtool_image *image, const UINT8 *fat_table)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 table_size;
	UINT32 pos, len;
	UINT32 sector_index;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	table_size = disk_info->sectors_per_fat * disk_info->fat_count * disk_info->sector_size;

	pos = 0;
	sector_index = disk_info->reserved_sectors;

	while(pos < table_size)
	{
		len = MIN(table_size - pos, disk_info->sector_size);

		err = fat_write_sector(image, sector_index++, 0, &fat_table[pos], len);
		if (err)
			goto done;

		pos += disk_info->sector_size;
	}

done:
	return err;
}



static UINT32 fat_get_fat_entry(imgtool_image *image, const UINT8 *fat_table, UINT32 fat_entry)
{
	const struct fat_diskinfo *disk_info;
	UINT64 entry;
	UINT32 bit_index, i;
	UINT32 last_entry = 0;
	UINT32 bit_mask;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);
	bit_index = fat_entry * disk_info->fat_bits;
	bit_mask = 0xFFFFFFFF >> (32 - disk_info->fat_bits);

	if (fat_entry >= disk_info->total_clusters)
	{
		assert(0);
		return 1;
	}

	/* make sure that the cluster is free in all fats */
	for (i = 0; i < disk_info->fat_count; i++)
	{
		memcpy(&entry, fat_table + (i * disk_info->sector_size
			* disk_info->sectors_per_fat) + (bit_index / 8), sizeof(entry));

		/* we've extracted the bytes; we now need to normalize it */
		entry = LITTLE_ENDIANIZE_INT64(entry);
		entry >>= bit_index % 8;
		entry &= bit_mask;

		if (i == 0)
			last_entry = (UINT32) entry;
		else if (last_entry != (UINT32) entry)
			return 1;	/* if the FATs disagree; mark this as reserved */
	}
	
	/* normalize special clusters */
	if (last_entry >= (0xFFFFFFF0 & bit_mask))
		last_entry |= 0xFFFFFFF0;
	return last_entry;
}



static void fat_set_fat_entry(imgtool_image *image, UINT8 *fat_table, UINT32 fat_entry, UINT32 value)
{
	const struct fat_diskinfo *disk_info;
	UINT64 entry;
	UINT32 bit_index, i;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);
	bit_index = fat_entry * disk_info->fat_bits;
	value &= 0xFFFFFFFF >> (32 - disk_info->fat_bits);

	for (i = 0; i < disk_info->fat_count; i++)
	{
		memcpy(&entry, fat_table + (i * disk_info->sector_size
			* disk_info->sectors_per_fat) + (bit_index / 8), sizeof(entry));

		entry = LITTLE_ENDIANIZE_INT64(entry);
		entry &= ~((UINT64) 0xFFFFFFFF >> (32 - disk_info->fat_bits)) << (bit_index % 8);
		entry |= ((UINT64) value) << (bit_index % 8);
		entry = LITTLE_ENDIANIZE_INT64(entry);

		memcpy(fat_table + (i * disk_info->sector_size
			* disk_info->sectors_per_fat) + (bit_index / 8), &entry, sizeof(entry));
	}
}



static imgtoolerr_t fat_seek_file(imgtool_image *image, struct fat_file *file, UINT32 pos)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 new_cluster;
	UINT8 *fat_table = NULL;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	/* can't seek past end of file */
	if (!file->directory && (pos > file->filesize))
		pos = file->filesize;

	if (file->first_cluster == 0)
	{
		/* special case; the root directory */
		file->index = pos;
	}
	else
	{
		/* first, we need to check to see if we have to go back to the beginning */
		if (pos < file->index)
		{
			file->cluster = file->first_cluster;
			file->cluster_index = 0;
			file->eof = 0;
		}

		if (file->eof)
			pos = file->index;

		/* skip ahead clusters */
		while((file->cluster_index + disk_info->cluster_size) <= pos)
		{
			if (!fat_table)
			{
				err = fat_load_fat(image, &fat_table);
				if (err)
					goto done;
			}

			new_cluster = fat_get_fat_entry(image, fat_table, file->cluster);

			file->cluster = new_cluster;
			file->cluster_index += disk_info->cluster_size;

			/* are we at the end of the file? */
			if (new_cluster == (0xFFFFFFFF >> (32 - disk_info->fat_bits)))
			{
				pos = file->cluster_index;
				file->eof = 1;
			}
		}
		file->index = pos;
	}

done:
	if (fat_table)
		free(fat_table);
	return err;
}



static UINT32 fat_get_filepos_sector_index(imgtool_image *image, struct fat_file *file)
{
	UINT32 sector_index;
	const struct fat_diskinfo *disk_info;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	sector_index = disk_info->reserved_sectors + (disk_info->sectors_per_fat * disk_info->fat_count);
	if (file->root)
	{
		/* special case for the root file */
		sector_index += file->index / disk_info->sector_size;
	}
	else
	{
		/* cluster out of range? */
		if ((file->cluster < 2) || (file->cluster >= disk_info->total_clusters))
			return 0;

		sector_index += (disk_info->root_entries * FAT_DIRENT_SIZE + disk_info->sector_size - 1) / disk_info->sector_size;
		sector_index += (file->cluster - 2) * disk_info->sectors_per_cluster;
	}
	return sector_index;
}



static imgtoolerr_t fat_corrupt_file_error(const struct fat_file *file)
{
	imgtoolerr_t err;
	if (file->root)
		err = IMGTOOLERR_CORRUPTIMAGE;
	else if (file->directory)
		err = IMGTOOLERR_CORRUPTDIR;
	else
		err = IMGTOOLERR_CORRUPTFILE;
	return err;
}



static imgtoolerr_t fat_readwrite_file(imgtool_image *image, struct fat_file *file,
	void *buffer, size_t buffer_len, size_t *bytes_read, int read_or_write)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	UINT32 sector_index;
	int offset;
	size_t len;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);
	if (bytes_read)
		*bytes_read = 0;
	if (!file->directory)
		buffer_len = MIN(buffer_len, file->filesize - file->index);

	while(!file->eof && (buffer_len > 0))
	{
		sector_index = fat_get_filepos_sector_index(image, file);
		if (sector_index == 0)
			return fat_corrupt_file_error(file);

		offset = file->index % disk_info->sector_size;
		len = MIN(buffer_len, disk_info->sector_size - offset);

		/* read or write the data from the disk */
		if (read_or_write)
			err = fat_write_sector(image, sector_index, offset, buffer, len);
		else
			err = fat_read_sector(image, sector_index, offset, buffer, len);
		if (err)
			return err;

		/* and move the file pointer ahead */
		err = fat_seek_file(image, file, file->index + len);
		if (err)
			return err;

		buffer = ((UINT8 *) buffer) + len;
		buffer_len -= len;
		if (bytes_read)
			*bytes_read += len;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_read_file(imgtool_image *image, struct fat_file *file,
	void *buffer, size_t buffer_len, size_t *bytes_read)
{
	return fat_readwrite_file(image, file, buffer, buffer_len, bytes_read, 0);
}



static imgtoolerr_t fat_write_file(imgtool_image *image, struct fat_file *file,
	const void *buffer, size_t buffer_len, size_t *bytes_read)
{
	return fat_readwrite_file(image, file, (void *) buffer, buffer_len, bytes_read, 1);
}



static UINT32 fat_allocate_cluster(imgtool_image *image, UINT8 *fat_table)
{
	const struct fat_diskinfo *disk_info;
	UINT32 i, val;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	for (i = 2; i < disk_info->total_clusters; i++)
	{
		val = fat_get_fat_entry(image, fat_table, i);
		if (val == 0)
		{
			fat_set_fat_entry(image, fat_table, i, 1);
			return i;
		}
	}
	return 0;
}



/* sets the size of a file; 0xFFFFFFFF means 'delete' */
static imgtoolerr_t fat_set_file_size(imgtool_image *image, struct fat_file *file,
	UINT32 new_size)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 new_cluster_count;
	UINT32 old_cluster_count;
	UINT32 cluster, write_cluster, last_cluster, new_pos, i;
	UINT8 *fat_table = NULL;
	UINT8 dirent[32];
	size_t clear_size;
	void *clear_buffer = NULL;
	int delete_file = FALSE;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	/* special case */
	if (new_size == 0xFFFFFFFF)
	{
		delete_file = TRUE;
		new_size = 0;
	}
	
	/* if this is the trivial case (not changing the size), succeed */
	if (!delete_file && (file->filesize == new_size))
	{
		err = IMGTOOLERR_SUCCESS;
		goto done;
	}

	/* what is the new position? */
	new_pos = MIN(file->index, new_size);

	if (file->root)
	{
		/* this is the root directory; this is a special case */
		if (new_size > (disk_info->root_entries * FAT_DIRENT_SIZE))
		{
			err = IMGTOOLERR_NOSPACE;
			goto done;
		}
	}
	else
	{
		old_cluster_count = (file->filesize + disk_info->cluster_size - 1) / disk_info->cluster_size;
		new_cluster_count = (new_size + disk_info->cluster_size - 1) / disk_info->cluster_size;
		cluster = 0;

		/* load the dirent */
		err = fat_read_sector(image, file->dirent_sector_index, file->dirent_sector_offset, dirent, sizeof(dirent));
		if (err)
			goto done;

		/* need to load the FAT whether we are growing or shrinking the file */
		if (old_cluster_count != new_cluster_count)
		{
			err = fat_load_fat(image, &fat_table);
			if (err)
				goto done;

			cluster = 0;
			i = 0;
			do
			{
				last_cluster = cluster;
				write_cluster = 0;

				/* identify the next cluster */
				if (cluster != 0)
					cluster = fat_get_fat_entry(image, fat_table, cluster);
				else
					cluster = file->first_cluster ? file->first_cluster : 0xFFFFFFFF;

				/* do we need to grow the file by a cluster? */
				if (i < new_cluster_count && ((cluster < 2) || (cluster >= disk_info->total_clusters)))
				{
					/* grow this file by a cluster */
					cluster = fat_allocate_cluster(image, fat_table);
					if (cluster == 0)
					{
						err = IMGTOOLERR_NOSPACE;
						goto done;
					}
					
					write_cluster = cluster;
				}
				else if (i >= new_cluster_count)
				{
					/* we are shrinking the file; we need to unlink this node */
					if ((cluster < 2) || (cluster >= disk_info->total_clusters))
						cluster = 0xFFFFFFFF; /* ack file is corrupt! recover */
					write_cluster = 0xFFFFFFFF;
				}

				/* write out the entry, if appropriate */
				if (write_cluster != 0)
				{
					if (last_cluster == 0)
						file->first_cluster = (write_cluster != 0xFFFFFFFF) ? write_cluster : 0;
					else
						fat_set_fat_entry(image, fat_table, last_cluster, write_cluster);
				}
			}
			while((++i < new_cluster_count) || (cluster != 0xFFFFFFFF));
		}

		/* record the new file size */
		place_integer(dirent, 26, 2, file->first_cluster);
		place_integer(dirent, 28, 4, new_size);

		/* delete the file, if appropriate */
		if (delete_file)
			dirent[0] = 0xE5;

		/* save the dirent */
		err = fat_write_sector(image, file->dirent_sector_index, file->dirent_sector_offset, dirent, sizeof(dirent));
		if (err)
			goto done;

		/* if we've modified the FAT, save it out */
		if (fat_table)
		{
			err = fat_save_fat(image, fat_table);
			if (err)
				goto done;
		}

		/* update the file structure */
		if (!file->directory)
			file->filesize = new_size;
		file->cluster = file->first_cluster;
		file->index = 0;
	}

	/* special case; clear out stale bytes on non-root directories */
	if (file->directory && !delete_file)
	{
		if (file->root)
			clear_size = MIN(file->filesize - new_size, FAT_DIRENT_SIZE);
		else
			clear_size = (disk_info->cluster_size - (new_size % disk_info->cluster_size)) % disk_info->cluster_size;

		if (clear_size > 0)
		{
			err = fat_seek_file(image, file, new_size);
			if (err)
				goto done;

			clear_buffer = malloc(clear_size);
			if (!clear_buffer)
			{
				err = IMGTOOLERR_OUTOFMEMORY;
				goto done;
			}
			memset(clear_buffer, '\0', clear_size);

			err = fat_write_file(image, file, clear_buffer, clear_size, NULL);
			if (err)
				goto done;
		}
	}

	/* seek back to original pos */
	err = fat_seek_file(image, file, new_pos);
	if (err)
		goto done;

done:
	if (fat_table)
		free(fat_table);
	if (clear_buffer)
		free(clear_buffer);
	return err;
}



static void prepend_lfn_bytes(utf16_char_t *lfn_buf, size_t lfn_buflen, size_t *lfn_len,
	const UINT8 *entry, int offset, int chars)
{
	UINT16 w;
	int i;
	size_t move_len;

	move_len = MIN(*lfn_len + 1, lfn_buflen - chars - 1);
	memmove(&lfn_buf[chars], &lfn_buf[0], move_len * sizeof(*lfn_buf));

	for (i = 0; i < chars; i++)
	{
		/* read the character */
		memcpy(&w, &entry[offset + i * 2], 2);
		w = LITTLE_ENDIANIZE_INT16(w);

		/* append to buffer */
		lfn_buf[i] = (w != 0xFFFF) ? w : 0;
	}
	*lfn_len += chars;
}



static UINT8 fat_calc_filename_checksum(const UINT8 *short_filename)
{
	UINT8 checksum;
	int i, j;

	checksum = 0;
	for (i = 0; i < 11; i++)
	{
		j = checksum & 1;
		checksum >>= 1;
		if (j)
			checksum |= 0x80;
		checksum += short_filename[i];
	}
	return checksum;
}



static void fat_calc_dirent_lfnchecksum(UINT8 *entry, size_t entry_len)
{
	UINT8 checksum;
	int i;

	checksum = fat_calc_filename_checksum(entry + entry_len - FAT_DIRENT_SIZE);

	for (i = 0; i < (entry_len / FAT_DIRENT_SIZE - 1); i++)
		entry[i * FAT_DIRENT_SIZE + 13] = checksum;
}



static imgtoolerr_t fat_read_dirent(imgtool_image *image, struct fat_file *file,
	struct fat_dirent *ent, struct fat_freeentry_info *freeent)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	UINT8 entry[FAT_DIRENT_SIZE];
	size_t bytes_read;
	int i, j;
	unicode_char_t ch;
	utf16_char_t lfn_buf[512];
	size_t lfn_len = 0;
	int lfn_lastentry = 0;
	UINT8 lfn_checksum = 0;
	UINT32 entry_index, entry_sector_index, entry_sector_offset;

	assert(file->directory);
	lfn_buf[0] = '\0';
	memset(ent, 0, sizeof(*ent));
	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	/* The first eight bytes of a FAT directory entry is a blank padded name
	 *
	 * The first byte can be special:
	 *	0x00 - entry is available and no further entry is used
	 *	0x05 - first character is actually 0xe5
	 *	0x2E - dot entry; either '.' or '..'
	 *	0xE5 - entry has been erased and is available
	 *
	 * Byte 11 is the attributes; and 0x0F denotes a LFN entry
	 */
	do
	{
		entry_index = file->index;
		entry_sector_index = fat_get_filepos_sector_index(image, file);
		entry_sector_offset = file->index % disk_info->sector_size;

		err = fat_read_file(image, file, entry, sizeof(entry), &bytes_read);
		if (err)
			return err;
		if (bytes_read < sizeof(entry))
			memset(entry, 0, sizeof(entry));

		if (entry[11] == 0x0F)
		{
			/* this is an LFN entry */
			if ((lfn_lastentry == 0)
				|| ((entry[0] & 0x3F) != (lfn_lastentry - 1))
				|| (lfn_checksum != entry[13]))
			{
				lfn_buf[0] = 0;
				lfn_len = 0;
				lfn_checksum = entry[13];
			}
			lfn_lastentry = entry[0] & 0x3F;
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry, 28, 2);
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry, 14, 6);
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry,  1, 5);
		}
		else if (freeent && (freeent->position == ~0))
		{
			/* do a quick check to find out if we found space */
			if ((entry[0] == '\0') || (entry[0] == 0xE5))
			{
				if (freeent->candidate_position > entry_index)
					freeent->candidate_position = entry_index;

				if ((entry[0] == '\0') || (freeent->candidate_position + freeent->required_size < file->filesize))
					freeent->position = freeent->candidate_position;
			}
		}
	}
	while((entry[0] == 0x2E) || (entry[0] == 0xE5) || (entry[11] == 0x0F));

	/* no more directory entries? */
	if (entry[0] == '\0')
	{
		ent->eof = 1;
		return IMGTOOLERR_SUCCESS;
	}

	/* pick apart short filename */
	memcpy(ent->short_filename, entry, 8);
	rtrim(ent->short_filename);
	if (entry[0] == 0x05)
		entry[0] = 0xE5;
	if ((entry[8] != ' ') || (entry[9] != ' ') || (entry[10] != ' '))
	{
		strcat(ent->short_filename, ".");
		memcpy(ent->short_filename + strlen(ent->short_filename), &entry[8], 3);
		rtrim(ent->short_filename);
	}
	for (i = 0; ent->short_filename[i]; i++)
		ent->short_filename[i] = cannonicalize_sfn_char(ent->short_filename[i]);

	/* and the long filename */
	if (lfn_lastentry == 1)
	{
		/* only use the LFN if the checksum passes */
		if (lfn_checksum == fat_calc_filename_checksum(entry))
		{
			i = 0;
			j = 0;
			do
			{
				i += uchar_from_utf16(&ch, &lfn_buf[i], sizeof(lfn_buf) / sizeof(lfn_buf[0]) - i);
				j += utf8_from_uchar(&ent->long_filename[j], sizeof(ent->long_filename) / sizeof(ent->long_filename[0]) - j, ch);
			}
			while(ch != 0);
		}
	}

	/* other attributes */
	ent->filesize = pick_integer(entry, 28, 4);
	ent->directory = (entry[11] & 0x10) ? 1 : 0;
	ent->first_cluster = pick_integer(entry, 26, 2);
	ent->dirent_sector_index = entry_sector_index;
	ent->dirent_sector_offset = entry_sector_offset;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_construct_dirent(const char *filename, creation_policy_t create,
	UINT8 **entry, size_t *entry_len)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	UINT8 *created_entry = NULL;
	UINT8 *new_created_entry;
	size_t created_entry_len = FAT_DIRENT_SIZE;
	size_t created_entry_pos = 0;
	unicode_char_t ch;
	char last_short_char = ' ';
	char short_char = '\0';
	utf16_char_t buf[UTF16_CHAR_MAX];
	int i, len;
	int sfn_pos = 0;
	int sfn_sufficient = 1;
	int sfn_in_extension = 0;

	/* sanity check */
	if (*filename == '\0')
	{
		err = IMGTOOLERR_BADFILENAME;
		goto done;
	}

	/* construct intial entry */
	created_entry = (UINT8 *) malloc(FAT_DIRENT_SIZE);
	if (!created_entry)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(created_entry +  0, ' ', 11);
	memset(created_entry + 12, '\0', FAT_DIRENT_SIZE - 12);
	created_entry[11] = (create == CREATE_DIR) ? 0x10 : 0x00;

	while(*filename)
	{
		filename += uchar_from_utf8(&ch, filename, UTF8_CHAR_MAX);

		/* append to short filename, if possible */
		if ((ch < 32) || (ch > 128))
			short_char = '\0';
		else if (isalnum((char) ch))
			short_char = toupper((char) ch);
		else if (strchr(". !#$%^()-@^_`{}~", (char) ch))
			short_char = (char) ch;
		else
			short_char = '\0';
		if (!short_char || (short_char != cannonicalize_sfn_char((char) ch)))
			sfn_sufficient = 0;

		/* append the short filename char */
		if (short_char == '.')
		{
			/* multiple extensions or trailing spaces? */
			if (sfn_in_extension || (last_short_char == ' '))
				sfn_sufficient = 0;

			sfn_in_extension = 1;
			sfn_pos = 8;
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 8] = ' ';
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 9] = ' ';
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 10] = ' ';
		}
		else if (sfn_pos == (sfn_in_extension ? 11 : 8))
		{
			/* ran out of characters for short filename */
			sfn_sufficient = 0;
		}
		else
		{
			created_entry[created_entry_len - FAT_DIRENT_SIZE + sfn_pos++] = ch;
		}
		last_short_char = short_char;


		/* convert to UTF-16 and add a long filename entry */
		len = utf16le_from_uchar(buf, UTF16_CHAR_MAX, ch);
		for (i = 0; i < len; i++)
		{
			switch(created_entry_pos)
			{
				case 0:
				case 32:
					/* need to grow */
					new_created_entry = (UINT8 *) realloc(created_entry, created_entry_len + FAT_DIRENT_SIZE);
					if (!new_created_entry)
					{
						err = IMGTOOLERR_OUTOFMEMORY;
						goto done;
					}
					created_entry = new_created_entry;

					/* move existing entries forward */
					memmove(created_entry + 32, created_entry, created_entry_len);
					created_entry_len += 32;

					/* set up this LFN */
					memset(created_entry, '\0', 32);
					memset(&created_entry[1], '\xFF', 10);
					memset(&created_entry[14], '\xFF', 12);
					memset(&created_entry[28], '\xFF', 4);
					created_entry[11] = 0x0F;

					/* specify entry index */
					created_entry[0] = (created_entry_len / 32) - 1;
					if (created_entry[0] >= 0x40)
					{
						err = IMGTOOLERR_BADFILENAME;
						goto done;
					}
					created_entry_pos = 1;
					break;

				case 11:
					created_entry_pos = 14;
					break;

				case 26:
					created_entry_pos = 28;
					break;
			}

			memcpy(&created_entry[created_entry_pos], &buf[i], 2); 
			created_entry_pos += 2;
		}
	}

	/* trailing spaces? */
	if (short_char == ' ')
		sfn_sufficient = 0;

	if (sfn_sufficient)
	{
		/* the short filename suffices; remove the LFN stuff */
		memcpy(created_entry, created_entry + created_entry_len - FAT_DIRENT_SIZE, FAT_DIRENT_SIZE);
		created_entry_len = FAT_DIRENT_SIZE;

		new_created_entry = (UINT8 *) realloc(created_entry, created_entry_len);
		if (!new_created_entry)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		created_entry = new_created_entry;
	}
	else
	{
		/* need to do finishing touches on the LFN */
		created_entry[0] |= 0x40;

		i = 6;
		while((i > 0) && isspace(created_entry[created_entry_len - FAT_DIRENT_SIZE + i - 1]))
			i--;
		created_entry[created_entry_len - FAT_DIRENT_SIZE + i + 0] = '~';
		created_entry[created_entry_len - FAT_DIRENT_SIZE + i + 1] = '1';
		fat_calc_dirent_lfnchecksum(created_entry, created_entry_len);
	}

done:
	if (err && created_entry)
	{
		free(created_entry);
		created_entry = NULL;
	}
	*entry = created_entry;
	*entry_len = created_entry_len;
	return err;
}



static imgtoolerr_t fat_lookup_path(imgtool_image *image, const char *path,
	creation_policy_t create, struct fat_file *file)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	struct fat_dirent ent;
	struct fat_freeentry_info freeent = { 0, };
	const char *next_path_part;
	UINT8 *created_entry = NULL;
	size_t created_entry_len = 0;
	UINT32 entry_sector_index, entry_sector_offset;
	UINT32 parent_first_cluster;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	memset(file, 0, sizeof(*file));
	file->root = 1;
	file->directory = 1;
	file->filesize = disk_info->root_entries * FAT_DIRENT_SIZE;

	while(*path)
	{
		if (!file->directory)
		{
			err = IMGTOOLERR_PATHNOTFOUND;
			goto done;
		}

		next_path_part = path + strlen(path) + 1;
		if (create && (*next_path_part == '\0'))
		{
			/* this is the last entry, and we are creating a file */
			err = fat_construct_dirent(path, create, &created_entry, &created_entry_len);
			if (err)
				goto done;

			freeent.required_size = created_entry_len;
			freeent.candidate_position = ~0;
			freeent.position = ~0;
		}

		do
		{
			err = fat_read_dirent(image, file, &ent, created_entry ? &freeent : NULL);
			if (err)
				goto done;
		}
		while(!ent.eof && stricmp(path, ent.short_filename) && stricmp(path, ent.long_filename));

		parent_first_cluster = file->first_cluster;

		if (ent.eof)
		{
			/* it seems that we have reached the end of this directory */

			if (!created_entry)
			{
				err = IMGTOOLERR_FILENOTFOUND;
				goto done;
			}

			if (created_entry_len > FAT_DIRENT_SIZE)
			{
				/* TODO: ensure uniqueness of the short filename */
			}

			err = fat_set_file_size(image, file, MAX(file->filesize, freeent.position + created_entry_len));
			if (err)
				goto done;

			err = fat_seek_file(image, file, freeent.position);
			if (err)
				goto done;
			
			err = fat_write_file(image, file, created_entry, created_entry_len, NULL);
			if (err)
				goto done;

			/* we have to do a special seek operation to get the main dirent */
			err = fat_seek_file(image, file, freeent.position + created_entry_len - FAT_DIRENT_SIZE);
			if (err)
				goto done;
			entry_sector_index = fat_get_filepos_sector_index(image, file);
			entry_sector_offset = file->index % disk_info->sector_size;

			/* build the file struct for the newly created file/directory */
			memset(file, 0, sizeof(*file));
			file->directory = (created_entry[created_entry_len - FAT_DIRENT_SIZE + 11] & 0x10) ? 1 : 0;
			file->dirent_sector_index = entry_sector_index;
			file->dirent_sector_offset = entry_sector_offset;
		}
		else
		{
			/* update the current file */
			memset(file, 0, sizeof(*file));
			file->directory = ent.directory;
			file->filesize = ent.filesize;
			file->cluster = ent.first_cluster;
			file->first_cluster = ent.first_cluster;
			file->dirent_sector_index = ent.dirent_sector_index;
			file->dirent_sector_offset = ent.dirent_sector_offset;
		}

		path = next_path_part;
		file->parent_first_cluster = parent_first_cluster;
	}

	err = IMGTOOLERR_SUCCESS;

done:
	if (created_entry)
		free(created_entry);
	return err;
}



static imgtoolerr_t fat_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err;
	struct fat_file *file;

	file = (struct fat_file *) img_enum_extrabytes(enumeration);

	err = fat_lookup_path(img_enum_image(enumeration), path, CREATE_NONE, file);
	if (err)
		return err;
	if (!file->directory)
		return IMGTOOLERR_PATHNOTFOUND;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	struct fat_file *file;
	struct fat_dirent fatent;

	file = (struct fat_file *) img_enum_extrabytes(enumeration);
	err = fat_read_dirent(img_enum_image(enumeration), file, &fatent, NULL);
	if (err)
		return err;

	snprintf(ent->filename, ent->filename_len, "%s", fatent.long_filename[0]
		? fatent.long_filename : fatent.short_filename);
	ent->filesize = fatent.filesize;
	ent->directory = fatent.directory;
	ent->eof = fatent.eof;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_readfile(imgtool_image *image, const char *filename, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct fat_file file;
	size_t bytes_read;
	char buffer[1024];

	err = fat_lookup_path(image, filename, CREATE_NONE, &file);
	if (err)
		return err;

	if (file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	do
	{
		err = fat_read_file(image, &file, buffer, sizeof(buffer), &bytes_read);
		if (err)
			return err;

		stream_write(destf, buffer, bytes_read);
	}
	while(bytes_read > 0);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_writefile(imgtool_image *image, const char *filename, imgtool_stream *sourcef, option_resolution *opts)
{
	imgtoolerr_t err;
	struct fat_file file;
	UINT32 bytes_left, len;
	char buffer[1024];

	err = fat_lookup_path(image, filename, CREATE_FILE, &file);
	if (err)
		return err;

	if (file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	bytes_left = (UINT32) stream_size(sourcef);

	err = fat_set_file_size(image, &file, bytes_left);
	if (err)
		return err;

	while(bytes_left > 0)
	{
		len = MIN(bytes_left, sizeof(buffer));
		stream_read(sourcef, buffer, len);

		err = fat_write_file(image, &file, buffer, len, NULL);
		if (err)
			return err;

		bytes_left -= len;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_delete(imgtool_image *image, const char *filename, unsigned int dir)
{
	imgtoolerr_t err;
	struct fat_file file;
	struct fat_dirent ent;

	err = fat_lookup_path(image, filename, CREATE_NONE, &file);
	if (err)
		return err;
	if (file.directory != dir)
		return IMGTOOLERR_FILENOTFOUND;

	if (dir)
	{
		err = fat_read_dirent(image, &file, &ent, NULL);
		if (err)
			return err;
		if (!ent.eof)
			return IMGTOOLERR_DIRNOTEMPTY;
	}

	err = fat_set_file_size(image, &file, 0xFFFFFFFF);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_deletefile(imgtool_image *image, const char *filename)
{
	return fat_diskimage_delete(image, filename, 0);
}



static imgtoolerr_t fat_diskimage_freespace(imgtool_image *image, UINT64 *size)
{
	imgtoolerr_t err;
	const struct fat_diskinfo * disk_info;
	UINT8 *fat_table;
	UINT32 i;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	err = fat_load_fat(image, &fat_table);
	if (err)
		goto done;

	*size = 0;
	for (i = 2; i < disk_info->total_clusters; i++)
	{
		if (fat_get_fat_entry(image, fat_table, i) == 0)
			*size += disk_info->cluster_size;
	}

done:
	if (fat_table)
		free(fat_table);
	return err;
}



static imgtoolerr_t fat_diskimage_createdir(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct fat_file file;
	UINT8 initial_data[64];

	err = fat_lookup_path(image, path, CREATE_DIR, &file);
	if (err)
		return err;
	if (!file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	err = fat_set_file_size(image, &file, sizeof(initial_data));
	if (err)
		return err;

	memset(initial_data, 0, sizeof(initial_data));
	memcpy(&initial_data[0], ".          ", 11);
	place_integer(initial_data, 11, 1, 0x10);
	place_integer(initial_data, 26, 2, file.first_cluster);
	memcpy(&initial_data[32], ".          ", 11);
	place_integer(initial_data, 43, 1, 0x10);
	place_integer(initial_data, 58, 2, file.parent_first_cluster);

	err = fat_write_file(image, &file, initial_data, sizeof(initial_data), NULL);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_deletedir(imgtool_image *image, const char *path)
{
	return fat_diskimage_delete(image, path, 1);
}



/*********************************************************************
	Imgtool module declaration
*********************************************************************/

static imgtoolerr_t fat_module_populate(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	module->initial_path_separator		= 1;
	module->open_is_strict				= 1;
	module->path_separator				= '\\';
	module->alternate_path_separator	= '/';
	module->eoln						= EOLN_CRLF;
	module->image_extra_bytes			+= sizeof(struct fat_diskinfo);
	module->imageenum_extra_bytes		+= sizeof(struct fat_file);
	module->create						= fat_diskimage_create;
	module->open						= fat_diskimage_open;
	module->begin_enum					= fat_diskimage_beginenum;
	module->next_enum					= fat_diskimage_nextenum;
	module->read_file					= fat_diskimage_readfile;
	module->write_file					= fat_diskimage_writefile;
	module->delete_file					= fat_diskimage_deletefile;
	module->free_space					= fat_diskimage_freespace;
	module->create_dir					= fat_diskimage_createdir;
	module->delete_dir					= fat_diskimage_deletedir;
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(fat, "FAT format", pc, fat_module_populate)
