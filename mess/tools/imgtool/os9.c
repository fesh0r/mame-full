/****************************************************************************

	os9.c

	CoCo OS-9 disk images

****************************************************************************/

#include <time.h>

#include "imgtool.h"
#include "formats/coco_dsk.h"
#include "iflopimg.h"

struct os9_diskinfo
{
	UINT32 total_sectors;
	UINT32 sectors_per_track;
	UINT32 allocation_bitmap_bytes;
	UINT32 cluster_size;
	UINT32 root_dir_lsn;
	UINT32 owner_id;
	UINT32 attributes;
	UINT32 disk_id;
	UINT32 format_flags;
	UINT32 bootstrap_lsn;
	UINT32 bootstrap_size;
	UINT32 sector_size;

	unsigned int sides : 2;
	unsigned int double_density : 1;
	unsigned int double_track : 1;
	unsigned int quad_track_density : 1;
	unsigned int octal_track_density : 1;

	char name[33];
};


struct os9_fileinfo
{
	unsigned int directory : 1;
	unsigned int non_sharable : 1;
	unsigned int public_execute : 1;
	unsigned int public_write : 1;
	unsigned int public_read : 1;
	unsigned int user_execute : 1;
	unsigned int user_write : 1;
	unsigned int user_read : 1;

	UINT32 owner_id;
	UINT32 link_count;
	UINT32 file_size;

	struct
	{
		UINT32 lsn;
		UINT32 count;
	} sector_map[48];
};


struct os9_direnum
{
	struct os9_diskinfo disk_info;
	struct os9_fileinfo dir_info;
	UINT32 index;
};



static UINT32 pick_integer(const void *ptr, size_t offset, size_t length)
{
	UINT32 value = 0;
	UINT8 b;

	while(length--)
	{
		b = ((UINT8 *) ptr)[offset++];
		value <<= 8;
		value |= b;
	}
	return value;
}



static void pick_string(const void *ptr, size_t offset, size_t length, char *dest)
{
	UINT8 b;

	while(length--)
	{
		b = ((UINT8 *) ptr)[offset++];
		*(dest++) = b & 0x7F;
		if (b & 0x80)
			length = 0;
	}
	*dest = '\0';
}



static void place_integer(void *ptr, size_t offset, size_t length, UINT32 value)
{
	while(length > 0)
	{
		((UINT8 *) ptr)[offset + --length] = (UINT8) value;
		value >>= 8;
	}
}



static void place_string(void *ptr, size_t offset, size_t length, const char *s)
{
	size_t i;
	UINT8 b;
	UINT8 *bptr;

	bptr = (UINT8 *) ptr;
	bptr += offset;

	bptr[0] = 0x80;

	for (i = 0; s[i] && (i < length); i++)
	{
		b = ((UINT8) s[i]) & 0x7F;
		if (s[i+1] == '\0')
			b |= 0x80;
		bptr[i] = b;
	}
}



static imgtoolerr_t os9_decode_disk_header(imgtool_image *image, struct os9_diskinfo *info)
{
	UINT32 track_size_in_sectors, attributes;
	UINT8 header[256];
	floperr_t ferr;

	ferr = floppy_read_sector(imgtool_floppy(image), 0, 0, 1, 0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	info->total_sectors				= pick_integer(header,   0, 3);
	track_size_in_sectors			= pick_integer(header,   3, 1);
	info->allocation_bitmap_bytes	= pick_integer(header,   4, 2);
	info->cluster_size				= pick_integer(header,   6, 2);
	info->root_dir_lsn				= pick_integer(header,   8, 3);
	info->owner_id					= pick_integer(header,  11, 2);
	attributes						= pick_integer(header,  13, 1);
	info->disk_id					= pick_integer(header,  14, 2);
	info->format_flags				= pick_integer(header,  16, 1);
	info->sectors_per_track			= pick_integer(header,  17, 2);
	info->bootstrap_lsn				= pick_integer(header,  21, 3);
	info->bootstrap_size			= pick_integer(header,  24, 2);
	info->sector_size				= pick_integer(header, 104, 2);

	info->sides					= (attributes & 0x01) ? 2 : 1;
	info->double_density		= (attributes & 0x02) ? 1 : 0;
	info->double_track			= (attributes & 0x04) ? 1 : 0;
	info->quad_track_density	= (attributes & 0x08) ? 1 : 0;
	info->octal_track_density	= (attributes & 0x10) ? 1 : 0;

	pick_string(header, 31, 32, info->name);

	if (info->sector_size == 0)
		info->sector_size = 256;

	if (info->sectors_per_track != track_size_in_sectors)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_track == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->total_sectors % info->sectors_per_track)
		return IMGTOOLERR_CORRUPTIMAGE;
	return IMGTOOLERR_SUCCESS;
}



static int os9_decode_file_header(imgtool_image *image,
	const struct os9_diskinfo *diskinfo,
	int lsn, struct os9_fileinfo *info)
{
	floperr_t ferr;
	UINT32 attributes, count;
	int max_entries, i, track, sector;
	UINT8 header[256];

	track = lsn / diskinfo->sectors_per_track;
	sector = (lsn % diskinfo->sectors_per_track) + 1;
	ferr = floppy_read_sector(imgtool_floppy(image), 0, track, sector, 0, header, sizeof(header));
	if (ferr)
		return imgtool_floppy_error(ferr);

	attributes			= pick_integer(header, 0, 1);
	info->owner_id		= pick_integer(header, 1, 2);
	info->link_count	= pick_integer(header, 8, 1);
	info->file_size		= pick_integer(header, 9, 4);

	info->directory			= (attributes & 0x80) ? 1 : 0;
	info->non_sharable		= (attributes & 0x40) ? 1 : 0;
	info->public_execute	= (attributes & 0x20) ? 1 : 0;
	info->public_write		= (attributes & 0x10) ? 1 : 0;
	info->public_read		= (attributes & 0x08) ? 1 : 0;
	info->user_execute		= (attributes & 0x04) ? 1 : 0;
	info->user_write		= (attributes & 0x02) ? 1 : 0;
	info->user_read			= (attributes & 0x01) ? 1 : 0;

	if (info->directory && (info->file_size % 32 != 0))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* read all sector map entries */
	max_entries = (diskinfo->sector_size - 16) / 5;
	max_entries = MIN(max_entries, sizeof(info->sector_map) / sizeof(info->sector_map[0]) - 1);
	for (i = 0; i < max_entries; i++)
	{
		lsn = pick_integer(header, 16 + (i * 5) + 0, 3);
		count = pick_integer(header, 16 + (i * 5) + 3, 2);
		if (count <= 0)
			break;

		info->sector_map[i].lsn = lsn;
		info->sector_map[i].count = count;
	}
	info->sector_map[i].lsn = 0;
	info->sector_map[i].count = 0;
	return 0;
}



static imgtoolerr_t os9_read_lsn(imgtool_image *img, const struct os9_diskinfo *disk_info,
	UINT32 lsn, int offset, void *buffer, size_t buffer_len)
{
	floperr_t ferr;
	UINT32 head, track, sector;

	head = 0;
	track = lsn / disk_info->sectors_per_track;
	sector = (lsn % disk_info->sectors_per_track) + 1;

	ferr = floppy_read_sector(imgtool_floppy(img), head, track, sector, offset, buffer, buffer_len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static UINT32 os9_lookup_lsn(const struct os9_diskinfo *disk_info,
	const struct os9_fileinfo *file_info, UINT32 *index)
{
	int i;
	UINT32 lsn;

	lsn = *index / disk_info->sector_size;

	i = 0;
	while(lsn >= file_info->sector_map[i].count)
		lsn -= file_info->sector_map[i].count;

	lsn = file_info->sector_map[i].lsn + lsn;
	*index %= disk_info->sector_size;
	return lsn;
}



static int os9_interpret_dirent(void *entry, char **filename, UINT32 *lsn, int *corrupt)
{
	int i;
	char *entry_b = (char *) entry;

	*filename = NULL;
	*lsn = 0;
	if (corrupt)
		*corrupt = FALSE;

	if (entry_b[28] != '\0')
	{
		if (corrupt)
			*corrupt = TRUE;
	}

	for (i = 0; (i < 28) && !(entry_b[i] & 0x80); i++)
		;
	entry_b[i] &= 0x7F;
	entry_b[i+1] = '\0';

	*lsn = pick_integer(entry, 29, 3);
	if (strcmp(entry_b, ".") && strcmp(entry_b, ".."))
		*filename = entry_b;
	return *filename != NULL;
}



static imgtoolerr_t os9_lookup_path(imgtool_image *img,
	const char *path, UINT32 *lsn, struct os9_fileinfo *file_info)
{
	imgtoolerr_t err;
	struct os9_fileinfo dir_info;
	UINT32 index, entry_index, entry_lsn, current_lsn;
	UINT8 entry[32];
	char *filename;
	const struct os9_diskinfo *disk_info;

	disk_info = (const struct os9_diskinfo *) imgtool_floppy_extrabytes(img);
	current_lsn = disk_info->root_dir_lsn;

	while(*path)
	{
		err = os9_decode_file_header(img, disk_info, current_lsn, &dir_info);
		if (err)
			return err;

		/* sanity check directory */
		if (!dir_info.directory)
			return (current_lsn == disk_info->root_dir_lsn) ? IMGTOOLERR_CORRUPTIMAGE : IMGTOOLERR_INVALIDPATH;

		for (index = 0; index < dir_info.file_size; index += 32)
		{
			entry_index = index;
			entry_lsn = os9_lookup_lsn(disk_info, &dir_info, &entry_index);
			
			err = os9_read_lsn(img, disk_info, entry_lsn, entry_index, entry, sizeof(entry));
			if (err)
				return err;

			if (os9_interpret_dirent(entry, &filename, &current_lsn, NULL))
			{
				if (!strcmp(path, filename))
					break;
			}
		}

		if (index >= dir_info.file_size)
			return IMGTOOLERR_PATHNOTFOUND;
		path += strlen(path) + 1;
	}

	if (lsn)
		*lsn = current_lsn;

	if (file_info)
	{
		err = os9_decode_file_header(img, disk_info, current_lsn, file_info);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_open(imgtool_image *image)
{
	struct os9_diskinfo *info;
	info = (struct os9_diskinfo *) imgtool_floppy_extrabytes(image);
	return os9_decode_disk_header(image, info);
}



static imgtoolerr_t os9_diskimage_create(imgtool_image *img, option_resolution *opts)
{
	imgtoolerr_t err;
	UINT8 *header;
	UINT32 heads, tracks, sectors, sector_bytes, first_sector_id;
	UINT32 cluster_size, owner_id;
	UINT32 allocation_bitmap_bits, allocation_bitmap_lsns;
	UINT32 attributes, format_flags, disk_id;
	UINT32 i;
	const char *title;
	time_t t;
	struct tm *ltime;

	time(&t);
	ltime = localtime(&t);

	heads = option_resolution_lookup_int(opts, 'H');
	tracks = option_resolution_lookup_int(opts, 'T');
	sectors = option_resolution_lookup_int(opts, 'S');
	sector_bytes = option_resolution_lookup_int(opts, 'L');
	first_sector_id = option_resolution_lookup_int(opts, 'F');
	title = "";

	header = malloc(sector_bytes);
	if (!header)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	if (sector_bytes > 256)
		sector_bytes = 256;
	cluster_size = 1;
	owner_id = 1;
	disk_id = 1;
	attributes = 0;
	allocation_bitmap_bits = heads * tracks * sectors / cluster_size;
	allocation_bitmap_lsns = (allocation_bitmap_bits / 8 + sector_bytes - 1) / sector_bytes;
	format_flags = ((heads > 1) ? 0x01 : 0x00) | ((tracks > 40) ? 0x02 : 0x00);

	memset(header, 0, sector_bytes);
	place_integer(header,   0,  3, heads * tracks * sectors);
	place_integer(header,   3,  1, sectors);
	place_integer(header,   4,  2, (allocation_bitmap_bits + 7) / 8);
	place_integer(header,   6,  2, cluster_size);
	place_integer(header,   8,  3, allocation_bitmap_lsns);
	place_integer(header,  11,  2, owner_id);
	place_integer(header,  13,  1, attributes);
	place_integer(header,  14,  2, disk_id);
	place_integer(header,  16,  1, format_flags);
	place_integer(header,  17,  2, sectors);
	place_string(header,   31, 32, title);
	place_integer(header, 100,  4, 1);
	place_integer(header, 104,  4, (sector_bytes == 256) ? 0 : sector_bytes);

	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id, 0, header, sector_bytes);
	if (err)
		goto done;

	memset(header, 0, sector_bytes);
	for (i = 0; i < allocation_bitmap_lsns; i++)
	{
		if (i == 0)
		{
			header[0] = 0xFF;
			header[1] = 0xC0;
		}
		else if (i == 1)
		{
			header[0] = 0x00;
			header[1] = 0x00;
		}

		err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 1 + i, 0, header, sector_bytes);
		if (err)
			goto done;
	}

	memset(header, 0, sector_bytes);
	header[0x00] = 0xBF;
	header[0x01] = 0x00;
	header[0x02] = 0x00;
	header[0x03] = (UINT8) ltime->tm_year;
	header[0x04] = (UINT8) ltime->tm_mon + 1;
	header[0x05] = (UINT8) ltime->tm_mday;
	header[0x06] = (UINT8) ltime->tm_hour;
	header[0x07] = (UINT8) ltime->tm_min;
	header[0x08] = 0x02;
	header[0x09] = 0x00;
	header[0x0A] = 0x00;
	header[0x0B] = 0x00;
	header[0x0C] = 0x40;
	header[0x0D] = (UINT8) (ltime->tm_year % 100);
	header[0x0E] = (UINT8) ltime->tm_mon;
	header[0x0F] = (UINT8) ltime->tm_mday;
	header[0x10] = 0x00;
	header[0x11] = 0x00;
	header[0x12] = 0x03;
	header[0x13] = 0x00;
	header[0x14] = 0x07;
	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 1 + allocation_bitmap_lsns, 0, header, sector_bytes);
	if (err)
		goto done;

	memset(header, 0, sector_bytes);
	header[0x00] = 0x2E;
	header[0x01] = 0xAE;
	header[0x1F] = 1 + allocation_bitmap_lsns;
	header[0x20] = 0xAE;
	header[0x3F] = 1 + allocation_bitmap_lsns;
	err = floppy_write_sector(imgtool_floppy(img), 0, 0, first_sector_id + 2 + allocation_bitmap_lsns, 0, header, sector_bytes);
	if (err)
		goto done;

	err = os9_diskimage_open(img);
	if (err)
		goto done;

done:
	if (header)
		free(header);
	return err;
}



static imgtoolerr_t os9_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct os9_direnum *os9enum;
	imgtool_image *image;

	image = img_enum_image(enumeration);
	os9enum = (struct os9_direnum *) img_enum_extrabytes(enumeration);

	err = os9_decode_disk_header(image, &os9enum->disk_info);
	if (err)
		goto done;

	err = os9_lookup_path(image, path, NULL, &os9enum->dir_info);
	if (err)
		goto done;

	/* this had better be a directory */
	if (!os9enum->dir_info.directory)
	{
		err = IMGTOOLERR_CORRUPTIMAGE;
		goto done;
	}

done:
	return err;
}



static imgtoolerr_t os9_enum_readlsn(imgtool_imageenum *enumeration, UINT32 lsn, int offset, void *buffer, size_t buffer_len)
{
	imgtool_image *image;
	struct os9_direnum *os9enum;

	image = img_enum_image(enumeration);
	os9enum = (struct os9_direnum *) img_enum_extrabytes(enumeration);
	return os9_read_lsn(image, &os9enum->disk_info, lsn, offset, buffer, buffer_len);
}



static imgtoolerr_t os9_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	struct os9_direnum *os9enum;
	UINT32 lsn, index;
	imgtoolerr_t err;
	UINT8 dir_entry[32];
	char filename[29];
	struct os9_fileinfo file_info;

	os9enum = (struct os9_direnum *) img_enum_extrabytes(enumeration);

	do
	{
		/* check for EOF */
		if (os9enum->index >= os9enum->dir_info.file_size)
		{
			ent->eof = 1;
			return IMGTOOLERR_SUCCESS;
		}

		index = os9enum->index;
		lsn = os9_lookup_lsn(&os9enum->disk_info, &os9enum->dir_info, &index);
		os9enum->index += 32;

		err = os9_enum_readlsn(enumeration, lsn, index, dir_entry, sizeof(dir_entry));
		if (err)
			return err;

		pick_string(dir_entry, 0, 28, filename);
	}
	while(!strcmp(filename, ".") || !strcmp(filename, ".."));

	/* read file attributes */
	lsn = pick_integer(dir_entry, 29, 3);
	err = os9_decode_file_header(img_enum_image(enumeration), &os9enum->disk_info, lsn, &file_info);
	if (err)
		return err;

	/* fill out imgtool_dirent structure */
	if (ent->filename_len > 0)
	{
		snprintf(ent->filename, ent->filename_len, "%s", filename);
	}
	if (ent->attr_len > 0)
	{
		snprintf(ent->attr, ent->attr_len, "%c%c%c%c%c%c%c%c", 
			file_info.directory      ? 'd' : '-',
			file_info.non_sharable   ? 's' : '-',
			file_info.public_execute ? 'x' : '-',
			file_info.public_write   ? 'w' : '-',
			file_info.public_read    ? 'r' : '-',
			file_info.user_execute   ? 'x' : '-',
			file_info.user_write     ? 'w' : '-',
			file_info.user_read      ? 'r' : '-');
	}
	ent->directory = file_info.directory;
	ent->corrupt = (dir_entry[28] != 0);
	ent->filesize = file_info.file_size;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t os9_diskimage_readfile(imgtool_image *img, const char *filename, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct os9_diskinfo disk_info;
	struct os9_fileinfo file_info;
	UINT8 buffer[256];
	int i, j;
	UINT32 file_size;
	UINT32 used_size;

	err = os9_decode_disk_header(img, &disk_info);
	if (err)
		return err;

	err = os9_lookup_path(img, filename, NULL, &file_info);
	if (err)
		return err;
	if (file_info.directory)
		return IMGTOOLERR_FILENOTFOUND;
	file_size = file_info.file_size;

	for (i = 0; file_info.sector_map[i].count > 0; i++)
	{
		for (j = 0; j < file_info.sector_map[i].count; j++)
		{
			used_size = MIN(file_size, disk_info.sector_size);
			err = os9_read_lsn(img, &disk_info, file_info.sector_map[i].lsn + j, 0,
				buffer, used_size);
			if (err)
				return err;
			stream_write(destf, buffer, used_size);
			file_size -= used_size;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t coco_os9_module_populate(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	module->initial_path_separator	= 1;
	module->image_extra_bytes		+= sizeof(struct os9_diskinfo);
	module->imageenum_extra_bytes	+= sizeof(struct os9_direnum);
	module->eoln					= EOLN_CR;
	module->path_separator			= '/';
	module->create					= os9_diskimage_create;
	module->open					= os9_diskimage_open;
	module->begin_enum				= os9_diskimage_beginenum;
	module->next_enum				= os9_diskimage_nextenum;
	module->read_file				= os9_diskimage_readfile;
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(os9, "OS-9 format", coco, coco_os9_module_populate)
