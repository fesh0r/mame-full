/****************************************************************************

	fat.c

	PC FAT disk images

****************************************************************************/

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
	UINT32 fat_bits;

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
	info->sectors_per_fat		= pick_integer(header, 22, 2);
	info->sectors_per_track		= pick_integer(header, 24, 2);
	info->heads					= pick_integer(header, 26, 2);

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

	memmove(&lfn_buf[chars], &lfn_buf[0], MAX(*lfn_len + chars + 1, lfn_buflen - 1));

	for (i = 0; i < chars; i++)
	{
		memcpy(&w, &entry[offset + i * 2], 2);
		w = LITTLE_ENDIANIZE_INT16(w);

		lfn_buf[i] = (w != 0xFFFF) ? w : 0;
	}
}



static imgtoolerr_t fat_read_dirent(imgtool_image *image, struct fat_file *file, struct fat_dirent *ent)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	UINT8 entry[FAT_DIRENT_SIZE];
	size_t bytes_read;
	utf16_char_t lfn_buf[512];
	size_t lfn_len = 0;
	int lfn_lastentry = 0;
	int i, j;
	unicode_char_t ch;

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
			if ((lfn_lastentry == 0) || ((entry[0] & 0x3F) != (lfn_lastentry - 1)))
			{
				lfn_buf[0] = 0;
				lfn_len = 0;
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
		i = 0;
		j = 0;
		do
		{
			i += uchar_from_utf16(&ch, &lfn_buf[i], sizeof(lfn_buf) / sizeof(lfn_buf[0]) - i);
			j += utf8_from_uchar(&ent->long_filename[j], sizeof(ent->long_filename) / sizeof(ent->long_filename[0]) - j, ch);
		}
		while(ch != 0);
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
	module->open						= fat_diskimage_open;
	module->begin_enum					= fat_diskimage_beginenum;
	module->next_enum					= fat_diskimage_nextenum;
	module->read_file					= fat_diskimage_readfile;
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(fat, "FAT format", pc, fat_module_populate)
