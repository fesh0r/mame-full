/****************************************************************************

	os9.c

	CoCo OS-9 disk images

****************************************************************************/

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
	imgtool_imageenum base;
	imgtool_image *img;
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
		return -1;
	if (info->total_sectors % info->sectors_per_track)
		return -1;
	return 0;
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
		return -1;

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



static imgtoolerr_t os9_lookup_path(imgtool_image *img, const struct os9_diskinfo *disk_info,
	const char *path, UINT32 *lsn, struct os9_fileinfo *file_info)
{
	imgtoolerr_t err;
	struct os9_fileinfo dir_info;
	UINT32 index, entry_index, entry_lsn, current_lsn;
	UINT8 entry[32];
	char *filename;

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



static imgtoolerr_t os9_diskimage_beginenum(imgtool_image *img, const char *path, imgtool_imageenum **outenum)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct os9_direnum *os9enum = NULL;

	os9enum = (struct os9_direnum *) malloc(sizeof(struct os9_direnum));
	if (!os9enum)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(os9enum, '\0', sizeof(*os9enum));

	err = os9_decode_disk_header(img, &os9enum->disk_info);
	if (err)
		goto done;

	err = os9_lookup_path(img, &os9enum->disk_info, path, NULL, &os9enum->dir_info);
	if (err)
		goto done;

	/* this had better be a directory */
	if (!os9enum->dir_info.directory)
	{
		err = IMGTOOLERR_CORRUPTIMAGE;
		goto done;
	}

	os9enum->base.module = img->module;
	os9enum->img = img;

done:
	if (err && os9enum)
	{
		free(os9enum);
		os9enum = NULL;
	}
	*outenum = &os9enum->base;
	return err;
}



static imgtoolerr_t os9_enum_readlsn(struct os9_direnum *os9enum, UINT32 lsn, int offset, void *buffer, size_t buffer_len)
{
	return os9_read_lsn(os9enum->img, &os9enum->disk_info, lsn, offset, buffer, buffer_len);
}



static imgtoolerr_t os9_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	struct os9_direnum *os9enum = (struct os9_direnum *) enumeration;
	UINT32 lsn, index;
	imgtoolerr_t err;
	UINT8 dir_entry[32];
	char filename[29];
	struct os9_fileinfo file_info;

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

		err = os9_enum_readlsn(os9enum, lsn, index, dir_entry, sizeof(dir_entry));
		if (err)
			return err;

		pick_string(dir_entry, 0, 28, filename);
	}
	while(!strcmp(filename, ".") || !strcmp(filename, ".."));

	/* read file attributes */
	lsn = pick_integer(dir_entry, 29, 3);
	err = os9_decode_file_header(os9enum->img, &os9enum->disk_info, lsn, &file_info);
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



static void os9_diskimage_closeenum(imgtool_imageenum *enumeration)
{
	free(enumeration);
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

	err = os9_lookup_path(img, &disk_info, filename, NULL, &file_info);
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



static imgtoolerr_t coco_os9_module_populate(imgtool_library *library, struct ImageModule *module)
{
	module->initial_path_separator	= 1;
	module->eoln					= EOLN_CR;
	module->path_separator			= '/';
	module->begin_enum				= os9_diskimage_beginenum;
	module->next_enum				= os9_diskimage_nextenum;
	module->close_enum				= os9_diskimage_closeenum;
	module->read_file				= os9_diskimage_readfile;
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(os9, "OS-9 format", coco, coco_os9_module_populate)
