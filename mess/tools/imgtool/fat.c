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

****************************************************************************/

#include <time.h>

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
	UINT32 reserved_sectors;
	UINT32 fat_count;
	UINT32 root_entries;
	UINT32 sectors_per_fat;
	UINT32 sectors_per_track;
	UINT32 heads;
	UINT64 total_sectors;
};

struct fat_file
{
	unsigned int root : 1;
	unsigned int directory : 1;
	UINT32 index;
	UINT32 filesize;
	UINT32 cluster;
	UINT32 cluster_index;
};

struct fat_dirent
{
	char long_filename[512];
	char short_filename[13];
	unsigned int directory : 1;
	unsigned int eof : 1;
	UINT32 filesize;
	UINT32 first_cluster;
};

struct fat_mediatype
{
	UINT8 media_descriptor;
	UINT8 heads;
	UINT8 tracks;
	UINT8 sectors;
};



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
	root_dir_sectors = (root_dir_count + sector_bytes - 1) / sector_bytes;
	total_clusters = (total_sectors - reserved_sectors - hidden_sectors - root_dir_sectors)
		/ sector_bytes;
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

	/* store boot sector */
	boot_sector_offset = sizeof(header) - sizeof(boot_sector_code);
	assert(boot_sector_offset >= 62);
	assert(boot_sector_offset <= 129);
	memcpy(&header[boot_sector_offset], boot_sector_code, sizeof(boot_sector_code));
	header[0] = 0xEB;								/* JMP imm8 */
	header[1] = (UINT8) (boot_sector_offset - 2);	/* (offset) */
	header[2] = 0x90;								/* NOP */

	ferr = floppy_write_sector(imgtool_floppy(image), 0, 0, first_sector_id,
		0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	/* clear out file allocation table */
	for (i = 0; i < (sectors_per_fat * fat_count + root_dir_sectors); i++)
	{
		ferr = floppy_clear_sector(imgtool_floppy(image), 0, 0, first_sector_id + 1 + i, 0);
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



static imgtoolerr_t fat_read_file(imgtool_image *image, struct fat_file *file,
	void *buffer, size_t buffer_len, size_t *bytes_read)
{
	floperr_t ferr;
	const struct fat_diskinfo *disk_info;
	UINT32 sector_index;
	int head, track, sector, offset;
	UINT32 bit_index;
	size_t len;
	UINT32 new_cluster;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);
	*bytes_read = 0;
	if (!file->directory)
		buffer_len = MIN(buffer_len, file->filesize - file->index);

	while(buffer_len > 0)
	{
		sector_index = disk_info->reserved_sectors + (disk_info->sectors_per_fat * disk_info->fat_count);
		if (file->root)
		{
			/* special case for the root file */
			sector_index += file->index / disk_info->sector_size;
		}
		else
		{
			/* do we need to move on to the next cluster? */
			if (file->index >= (file->cluster_index + disk_info->sectors_per_cluster * disk_info->sector_size))
			{
				bit_index = file->cluster * disk_info->fat_bits;

				ferr = floppy_read_sector(imgtool_floppy(image), 0, 0, 1 + disk_info->reserved_sectors,
					bit_index / 8, &new_cluster, sizeof(new_cluster));
				if (ferr)
					return imgtool_floppy_error(ferr);

				new_cluster = (UINT32) LITTLE_ENDIANIZE_INT32(new_cluster);
				new_cluster >>= bit_index % 8;

				if (disk_info->fat_bits < 32)
					new_cluster &= (1 << disk_info->fat_bits) - 1;

				file->cluster = new_cluster;
				file->cluster_index += disk_info->sectors_per_cluster * disk_info->sector_size;
			}

			/* cluster values 0 and 1 are special */
			if (file->cluster < 2)
				return IMGTOOLERR_CORRUPTIMAGE;

			sector_index += (disk_info->root_entries * FAT_DIRENT_SIZE + disk_info->sector_size - 1) / disk_info->sector_size;
			sector_index += (file->cluster - 2) * disk_info->sectors_per_cluster;
		}

		head = (sector_index / disk_info->sectors_per_track) % disk_info->heads;
		track = sector_index / disk_info->sectors_per_track / disk_info->heads;
		sector = 1 + (sector_index % disk_info->sectors_per_track);
		offset = file->index % disk_info->sector_size;
		len = MIN(buffer_len, disk_info->sector_size - offset);

		ferr = floppy_read_sector(imgtool_floppy(image), head, track, sector,
			offset, buffer, len);
		if (ferr)
			return imgtool_floppy_error(ferr);

		buffer = ((UINT8 *) buffer) + len;
		buffer_len -= len;
		file->index += len;
		*bytes_read += len;
	}
	return IMGTOOLERR_SUCCESS;
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



static imgtoolerr_t fat_read_dirent(imgtool_image *image, struct fat_file *file, struct fat_dirent *ent)
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
	UINT8 lfn_checksum = 0, checksum;

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
		err = fat_read_file(image, file, entry, sizeof(entry), &bytes_read);
		if (err)
			return err;
		if ((bytes_read < sizeof(entry)) || (entry[0] == '\0'))
		{
			ent->eof = 1;
			return IMGTOOLERR_SUCCESS;
		}

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
	}
	while((entry[0] == 0x2E) || (entry[0] == 0xE5) || (entry[11] == 0x0F));

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
		ent->short_filename[i] = tolower(ent->short_filename[i]);

	/* and the long filename */
	if (lfn_lastentry == 1)
	{
		checksum = 0;
		for (i = 0; i < 11; i++)
		{
			j = checksum & 1;
			checksum >>= 1;
			if (j)
				checksum |= 0x80;
			checksum += entry[i];
		}

		/* only use the LFN if the checksum passes */
		if (checksum == lfn_checksum)
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
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_lookup_file(imgtool_image *image, const char *path, struct fat_file *file)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	struct fat_dirent ent;

	disk_info = (const struct fat_diskinfo *) imgtool_floppy_extrabytes(image);

	memset(file, 0, sizeof(*file));
	file->root = 1;
	file->directory = 1;
	file->filesize = disk_info->root_entries * FAT_DIRENT_SIZE;

	while(*path)
	{
		if (!file->directory)
			return IMGTOOLERR_PATHNOTFOUND;

		do
		{
			err = fat_read_dirent(image, file, &ent);
			if (err)
				return err;
			if (ent.eof)
				return IMGTOOLERR_FILENOTFOUND;
		}
		while(stricmp(path, ent.short_filename) && stricmp(path, ent.long_filename));

		path += strlen(path) + 1;

		/* update the current file */
		memset(file, 0, sizeof(*file));
		file->directory = ent.directory;
		file->filesize = ent.filesize;
		file->cluster = ent.first_cluster;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err;
	struct fat_file *file;

	file = (struct fat_file *) img_enum_extrabytes(enumeration);

	err = fat_lookup_file(img_enum_image(enumeration), path, file);
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
	err = fat_read_dirent(img_enum_image(enumeration), file, &fatent);
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

	err = fat_lookup_file(image, filename, &file);
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



/*********************************************************************
	Imgtool module declaration
*********************************************************************/

static imgtoolerr_t fat_module_populate(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	module->initial_path_separator		= 1;
	module->prefer_ucase				= 1;
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
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(fat, "FAT format", pc, fat_module_populate)
