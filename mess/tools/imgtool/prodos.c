/****************************************************************************

	prodos.c

	Apple II ProDOS disk images

*****************************************************************************

  Notes:
  - ProDOS disks are split into 512 byte blocks.

  ProDOS directory structure:

  Offset  Length  Description
  ------  ------  -----------
       0       2  ???
	   2       2  Next block (0 if end)
       4      39  Directory Entry
      43      39  Directory Entry
                  ...
     472      39  Directory Entry
	 511       1  ???


  ProDOS directory entry structure:

  Offset  Length  Description
  ------  ------  -----------
       0       1  Storage type (bits 7-4)
	                  10 - Seedling File (1 block)
					  20 - Sapling File (2-256 blocks)
					  30 - Tree File (257-32768 blocks)
					  E0 - Subdirectory Header
				      F0 - Volume Header
	   1      15  File name (NUL padded)
	  16       1  File type
	  17       2  Key pointer
	  19       2  Blocks used
	  21       3  File size
	  24       4  Creation date
	  28       1  ProDOS version that created the file
	  29       1  Minimum ProDOS version needed to read the file
	  30       1  Access byte
	  31       2  Auxilary file type
	  33       4  Last modified date
	  37       2  Header pointer

  In "seedling files", the key pointer points to a single block that is the
  whole file.  In "sapling files", the key pointer points to an index block
  that contains 256 2-byte index pointers that point to the actual blocks of
  the files.  These 2-byte values are not contiguous; the low order byte is
  in the first half of the block, and the high order byte is in the second
  half of the block.  In "tree files", the key pointer points to an index
  block of index blocks.

  ProDOS dates are 32-bit little endian values

	bits  0- 4	Day
	bits  5- 8	Month (0-11)
	bits  9-15	Year (0-49 is 2000-2049, 50-99 is 1950-1999)
	bits 16-21	Minute
	bits 24-28	Hour

*****************************************************************************/

#include "imgtool.h"
#include "formats/ap2_dsk.h"
#include "formats/ap_dsk35.h"
#include "iflopimg.h"

#define BLOCK_SIZE				512
#define PRODOS_DIRENT_SIZE		39

struct prodos_diskinfo
{
	imgtoolerr_t (*load_block)(imgtool_image *image, int block, void *buffer);
	imgtoolerr_t (*save_block)(imgtool_image *image, int block, const void *buffer);
};

struct prodos_direnum
{
	UINT32 first_block;
	UINT32 block;
	UINT32 index;
	UINT8 block_data[BLOCK_SIZE];
};

struct prodos_dirent
{
	char filename[16];
	UINT8 storage_type;
	UINT16 key_pointer;
	UINT32 filesize;
	UINT32 lastmodified_time;
	UINT32 creation_time;
};



static UINT32 pick_integer(const void *data, size_t offset, size_t length)
{
	const UINT8 *data_int = (const UINT8 *) data;
	UINT32 result = 0;

	data_int += offset;

	while(length--)
	{
		result <<= 8;
		result |= data_int[length];
	}
	return result;
}


static time_t prodos_crack_time(UINT32 prodos_time)
{
	struct tm t;
	time_t now;

	time(&now);
	t = *localtime(&now);

	t.tm_sec	= 0;
	t.tm_min	= (prodos_time >> 16) & 0x3F;
	t.tm_hour	= (prodos_time >> 24) & 0x1F;
	t.tm_mday	= (prodos_time >>  0) & 0x1F;
	t.tm_mon	= (prodos_time >>  5) & 0x0F;
	t.tm_year	= (prodos_time >>  9) & 0x7F;

	if (t.tm_year <= 49)
		t.tm_year += 100;
	return mktime(&t);
}




static int is_file_storagetype(UINT8 storage_type)
{
	return (storage_type >= 0x10) && (storage_type <= 0x3F);
}



static int is_dir_storagetype(UINT8 storage_type)
{
	return (storage_type >= 0xE0) && (storage_type <= 0xEF);
}



static struct prodos_diskinfo *get_prodos_info(imgtool_image *image)
{
	struct prodos_diskinfo *info;
	info = (struct prodos_diskinfo *) imgtool_floppy_extrabytes(image);
	return info;
}



/* ----------------------------------------------------------------------- */

static void prodos_find_block_525(imgtool_image *image, int block,
	UINT32 *track, UINT32 *head, UINT32 *sector1, UINT32 *sector2)
{
	static const UINT8 skewing[] =
	{
		0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
		0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F
	};

	block *= 2;

	*track = block / APPLE2_SECTOR_COUNT;
	*head = 0;
	*sector1 = skewing[block % APPLE2_SECTOR_COUNT + 0];
	*sector2 = skewing[block % APPLE2_SECTOR_COUNT + 1];
}



static imgtoolerr_t prodos_load_block_525(imgtool_image *image,
	int block, void *buffer)
{
	floperr_t ferr;
	UINT32 track, head, sector1, sector2;

	prodos_find_block_525(image, block, &track, &head, &sector1, &sector2);

	/* read first sector */
	ferr = floppy_read_sector(imgtool_floppy(image), head, track, 
		sector1, 0, ((UINT8 *) buffer) + 0, 256);
	if (ferr)
		return imgtool_floppy_error(ferr);

	/* read second sector */
	ferr = floppy_read_sector(imgtool_floppy(image), head, track,
		sector2, 0, ((UINT8 *) buffer) + 256, 256);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_save_block_525(imgtool_image *image,
	int block, const void *buffer)
{
	floperr_t ferr;
	UINT32 track, head, sector1, sector2;

	prodos_find_block_525(image, block, &track, &head, &sector1, &sector2);

	/* read first sector */
	ferr = floppy_write_sector(imgtool_floppy(image), head, track, 
		sector1, 0, ((const UINT8 *) buffer) + 0, 256);
	if (ferr)
		return imgtool_floppy_error(ferr);

	/* read second sector */
	ferr = floppy_write_sector(imgtool_floppy(image), head, track,
		sector2, 0, ((const UINT8 *) buffer) + 256, 256);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_open_525(imgtool_image *image)
{
	struct prodos_diskinfo *info;
	info = get_prodos_info(image);
	info->load_block = prodos_load_block_525;
	info->save_block = prodos_save_block_525;
	return IMGTOOLERR_SUCCESS;
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_find_block_35(imgtool_image *image, int block,
	UINT32 *track, UINT32 *head, UINT32 *sector)
{
	int sides = 2;

	*track = 0;
	while(block >= (apple35_tracklen_800kb[*track] * sides))
	{
		block -= (apple35_tracklen_800kb[(*track)++] * sides);
		if (*track >= 80)
			return IMGTOOLERR_SEEKERROR;
	}

	*head = block / apple35_tracklen_800kb[*track];
	*sector = block % apple35_tracklen_800kb[*track];
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_load_block_35(imgtool_image *image,
	int block, void *buffer)
{
	imgtoolerr_t err;
	floperr_t ferr;
	UINT32 track, head, sector;

	err = prodos_find_block_35(image, block, &track, &head, &sector);
	if (err)
		return err;

	ferr = floppy_read_sector(imgtool_floppy(image), head, track, sector, 0, buffer, 512);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_save_block_35(imgtool_image *image,
	int block, const void *buffer)
{
	imgtoolerr_t err;
	floperr_t ferr;
	UINT32 track, head, sector;

	err = prodos_find_block_35(image, block, &track, &head, &sector);
	if (err)
		return err;

	ferr = floppy_write_sector(imgtool_floppy(image), head, track, sector, 0, buffer, 512);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_open_35(imgtool_image *image)
{
	struct prodos_diskinfo *info;
	info = get_prodos_info(image);
	info->load_block = prodos_load_block_35;
	info->save_block = prodos_save_block_35;
	return IMGTOOLERR_SUCCESS;
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_load_block(imgtool_image *image,
	int block, void *buffer)
{
	struct prodos_diskinfo *diskinfo;
	diskinfo = get_prodos_info(image);
	return diskinfo->load_block(image, block, buffer);
}



static imgtoolerr_t prodos_save_block(imgtool_image *image,
	int block, const void *buffer)
{
	struct prodos_diskinfo *diskinfo;
	diskinfo = get_prodos_info(image);
	return diskinfo->save_block(image, block, buffer);
}



static imgtoolerr_t prodos_load_enum_block(imgtool_image *image,
	struct prodos_direnum *appleenum, int block)
{
	imgtoolerr_t err;
	UINT8 buffer[BLOCK_SIZE];

	err = prodos_load_block(image, block, buffer);
	if (err)
		return err;

	/* copy the data */
	appleenum->block = block;
	memcpy(appleenum->block_data, buffer, sizeof(buffer));
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_create(imgtool_image *image, option_resolution *opts)
{
	imgtoolerr_t err;
	UINT8 buffer[BLOCK_SIZE];

	memset(buffer, 0, sizeof(buffer));
	err = prodos_save_block(image, 2, buffer);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_create_525(imgtool_image *image, option_resolution *opts)
{
	prodos_diskimage_open_525(image);
	return prodos_diskimage_create(image, opts);
}



static imgtoolerr_t prodos_diskimage_create_35(imgtool_image *image, option_resolution *opts)
{
	prodos_diskimage_open_35(image);
	return prodos_diskimage_create(image, opts);
}



static imgtoolerr_t prodos_get_next_dirent(imgtool_image *image,
	struct prodos_direnum *appleenum, struct prodos_dirent *ent)
{
	imgtoolerr_t err;
	UINT32 next_block;
	UINT32 offset;

	do
	{
		/* go to next image */
		if (appleenum->index >= (BLOCK_SIZE / PRODOS_DIRENT_SIZE))
		{
			if (appleenum->block == 0)
				next_block = appleenum->first_block;
			else
				next_block = pick_integer(appleenum->block_data, 2, 2);

			if (next_block > 0)
			{
				err = prodos_load_enum_block(image, appleenum, next_block);
				if (err)
					return err;
			}
			else
			{
				appleenum->block = 0;
			}
			appleenum->index = 0;
		}
		else
		{
			appleenum->index++;
		}

		offset = appleenum->index * PRODOS_DIRENT_SIZE + 4;
		if (appleenum->block)
			ent->storage_type = appleenum->block_data[offset + 0];
		else
			ent->storage_type = 0x00;
	}
	while(!is_file_storagetype(ent->storage_type)
		&& !is_dir_storagetype(ent->storage_type)
		&& appleenum->block);

	memcpy(ent->filename, &appleenum->block_data[offset + 1], 15);
	ent->filename[15] = '\0';
	ent->key_pointer		= pick_integer(appleenum->block_data, offset + 17, 2);
	ent->filesize			= pick_integer(appleenum->block_data, offset + 21, 3);
	ent->creation_time		= pick_integer(appleenum->block_data, offset + 24, 4);
	ent->lastmodified_time	= pick_integer(appleenum->block_data, offset + 33, 4);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_lookup_path(imgtool_image *image, const char *path,
	struct prodos_dirent *ent)
{
	imgtoolerr_t err;
	struct prodos_direnum direnum;
	UINT32 block = 2;

	while(*path)
	{
		memset(&direnum, 0, sizeof(direnum));
		direnum.first_block = block;

		do
		{
			err = prodos_get_next_dirent(image, &direnum, ent);
			if (err)
				return err;
		}
		while(strcmp(path, ent->filename));

		path += strlen(path) + 1;
		if (*path)
		{
			if (!is_dir_storagetype(ent->storage_type))
				return IMGTOOLERR_FILENOTFOUND;
			block = ent->key_pointer;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static UINT32 prodos_get_storagetype_maxfilesize(UINT8 storage_type)
{
	UINT32 max_filesize = 0;
	switch(storage_type & 0xF0)
	{
		case 0x10:
			max_filesize = BLOCK_SIZE * 1;
			break;
		case 0x20:
			max_filesize = BLOCK_SIZE * 256;
			break;
		case 0x30:
			max_filesize = BLOCK_SIZE * 32768;
			break;
	}
	return max_filesize;
}



static imgtoolerr_t prodos_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	struct prodos_direnum *appleenum;

	appleenum = (struct prodos_direnum *) img_enum_extrabytes(enumeration);

	/* first directory is at block 2 */
	appleenum->first_block = 2;
	appleenum->index = ~0;

	if (*path)
		return IMGTOOLERR_UNIMPLEMENTED;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	imgtool_image *image;
	struct prodos_direnum *appleenum;
	struct prodos_dirent pd_ent;
	UINT32 max_filesize;

	image = img_enum_image(enumeration);
	appleenum = (struct prodos_direnum *) img_enum_extrabytes(enumeration);

	err = prodos_get_next_dirent(image, appleenum, &pd_ent);
	if (err)
		return err;

	/* end of file? */
	if (pd_ent.storage_type == 0x00)
	{
		ent->eof = 1;
		return IMGTOOLERR_SUCCESS;
	}

	strcpy(ent->filename, pd_ent.filename);
	ent->directory			= is_dir_storagetype(pd_ent.storage_type);
	ent->creation_time		= prodos_crack_time(pd_ent.creation_time);
	ent->lastmodified_time	= prodos_crack_time(pd_ent.lastmodified_time);

	if (!ent->directory)
	{
		ent->filesize = pd_ent.filesize;

		max_filesize = prodos_get_storagetype_maxfilesize(pd_ent.storage_type);
		if (ent->filesize > max_filesize)
		{
			ent->corrupt = 1;
			ent->filesize = max_filesize;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_send_file(imgtool_image *image, UINT32 *filesize,
	UINT32 block, int nest_level, imgtool_stream *destf)
{
	imgtoolerr_t err;
	UINT8 buffer[BLOCK_SIZE];
	UINT16 subblock;
	size_t bytes_to_write;
	int i;

	err = prodos_load_block(image, block, buffer);
	if (err)
		return err;

	if (nest_level > 0)
	{
		/* this is an index block */
		for (i = 0; i < 256; i++)
		{
			subblock = buffer[i + 256];
			subblock <<= 8;
			subblock |= buffer[i + 0];

			if (subblock != 0)
			{
				err = prodos_send_file(image, filesize, subblock, nest_level - 1, destf);
				if (err)
					return err;
			}
		}
	}
	else
	{
		/* this is a leaf block */
		bytes_to_write = MIN(*filesize, sizeof(buffer));
		stream_write(destf, buffer, bytes_to_write);
		*filesize -= bytes_to_write;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_readfile(imgtool_image *image, const char *filename, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;

	err = prodos_lookup_path(image, filename, &ent);
	if (err)
		return err;

	if (is_dir_storagetype(ent.storage_type))
		return IMGTOOLERR_FILENOTFOUND;

	err = prodos_send_file(image, &ent.filesize, ent.key_pointer, 
		(ent.storage_type >> 4) - 1, destf);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t apple2_prodos_module_populate(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	module->initial_path_separator		= 1;
	module->open_is_strict				= 1;
	module->supports_creation_time		= 1;
	module->supports_lastmodified_time	= 1;
	module->image_extra_bytes			+= sizeof(struct prodos_diskinfo);
	module->imageenum_extra_bytes		+= sizeof(struct prodos_direnum);
	module->eoln						= EOLN_CR;
	module->path_separator				= '/';
	module->begin_enum					= prodos_diskimage_beginenum;
	module->next_enum					= prodos_diskimage_nextenum;
	module->read_file					= prodos_diskimage_readfile;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t apple2_prodos_module_populate_525(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	apple2_prodos_module_populate(library, module);
	module->create = prodos_diskimage_create_525;
	module->open = prodos_diskimage_open_525;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t apple2_prodos_module_populate_35(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	apple2_prodos_module_populate(library, module);
	module->create = prodos_diskimage_create_35;
	module->open = prodos_diskimage_open_35;
	return IMGTOOLERR_SUCCESS;
}



FLOPPYMODULE(prodos_525, "ProDOS format", apple2,       apple2_prodos_module_populate_525)
FLOPPYMODULE(prodos_35,  "ProDOS format", apple35_iigs, apple2_prodos_module_populate_35)
