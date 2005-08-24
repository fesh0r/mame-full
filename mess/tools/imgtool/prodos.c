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
					  40 - Pascal Areas on ProFile HDs (???)
					  50 - GS/OS Extended File (data and rsrc fork)
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


  ProDOS directory and volume headers have this information:

  Offset  Length  Description
  ------  ------  -----------
      31       1  Length of the entry; generally is 39
	  32       1  Number of entries per block; generally is 13
	  33       2  Active entry count in directory
	  35       2  Volume bitmap block number
	  37       2  Total blocks on volume

  GS/OS Extended Files (storage type $5) point to an extended key block that
  contains information about the two forks.  The first half of the extended
  key block contains info about the data form, and the second half the
  resource fork.  Both sides have the following format:

  Offset  Length  Description
  ------  ------  -----------
       0       1  Storage type (bits 3-0, unlike the directory entry)
	   1       2  Key pointer
	   3       2  Blocks used
	   5       3  File size
	   8       1  Size of secondary info #1 (must be 18 to be valid)
	   9       1  Secondary info #1 type (1=FInfo 2=xFInfo)
	  10      16  FInfo or xFInfo
	  26       1  Size of secondary info #2 (must be 18 to be valid)
	  27       1  Secondary info #2 type (1=FInfo 2=xFInfo)
	  28      16  FInfo or xFInfo

  For more info, consult ProDOS technical note #25
	(http://web.pdx.edu/~heiss/technotes/pdos/tn.pdos.25.html)

*****************************************************************************/

#include "imgtool.h"
#include "formats/ap2_dsk.h"
#include "formats/ap_dsk35.h"
#include "iflopimg.h"

#define	ROOTDIR_BLOCK			2
#define BLOCK_SIZE				512

struct prodos_diskinfo
{
	imgtoolerr_t (*load_block)(imgtool_image *image, int block, void *buffer);
	imgtoolerr_t (*save_block)(imgtool_image *image, int block, const void *buffer);
	UINT8 dirent_size;
	UINT8 dirents_per_block;
	UINT16 volume_bitmap_block;
	UINT16 total_blocks;
};

struct prodos_direnum
{
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

typedef enum
{
	CREATE_NONE,
	CREATE_FILE,
	CREATE_DIR,
} creation_policy_t;



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



static UINT32 prodos_setup_time(time_t ansi_time)
{
	struct tm t;
	UINT32 result = 0;

	t = *localtime(&ansi_time);
	if ((t.tm_year >= 100) && (t.tm_year <= 149))
		t.tm_year -= 100;

	result |= (((UINT32) t.tm_min)	& 0x003F) << 16;
	result |= (((UINT32) t.tm_hour)	& 0x001F) << 24;
	result |= (((UINT32) t.tm_mday)	& 0x001F) <<  0;
	result |= (((UINT32) t.tm_mon)	& 0x000F) <<  5;
	result |= (((UINT32) t.tm_year)	& 0x007F) <<  9;
	return result;
}



static UINT32 prodos_time_now(void)
{
	time_t now;
	time(&now);
	return prodos_setup_time(now);
}



static int is_file_storagetype(UINT8 storage_type)
{
	return ((storage_type >= 0x10) && (storage_type <= 0x3F))
		|| ((storage_type >= 0x50) && (storage_type <= 0x5F));
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



static void prodos_setprocs_525(imgtool_image *image)
{
	struct prodos_diskinfo *info;
	info = get_prodos_info(image);
	info->load_block = prodos_load_block_525;
	info->save_block = prodos_save_block_525;
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



static void prodos_setprocs_35(imgtool_image *image)
{
	struct prodos_diskinfo *info;
	info = get_prodos_info(image);
	info->load_block = prodos_load_block_35;
	info->save_block = prodos_save_block_35;
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



static imgtoolerr_t prodos_clear_block(imgtool_image *image, int block)
{
	UINT8 buffer[BLOCK_SIZE];
	memset(buffer, 0, sizeof(buffer));
	return prodos_save_block(image, block, buffer);
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_diskimage_open(imgtool_image *image)
{
	imgtoolerr_t err;
	UINT8 buffer[BLOCK_SIZE];
	struct prodos_diskinfo *di;
	const UINT8 *ent;

	di = get_prodos_info(image);

	/* specify defaults */
	di->dirent_size = 39;
	di->dirents_per_block = 13;

	/* load the first block, hoping that the volume header is first */
	err = prodos_load_block(image, ROOTDIR_BLOCK, buffer);
	if (err)
		return err;

	ent = &buffer[4];

	/* did we find the volume header? */
	if ((ent[0] & 0xF0) == 0xF0)
	{
		di->dirent_size			= pick_integer(ent, 31, 1);
		di->dirents_per_block	= pick_integer(ent, 32, 1);
		di->volume_bitmap_block	= pick_integer(ent, 35, 2);
		di->total_blocks		= pick_integer(ent, 37, 2);
	}

	/* sanity check these values */
	if (di->dirent_size < 39)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (di->dirents_per_block * di->dirent_size >= BLOCK_SIZE)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (di->volume_bitmap_block >= di->total_blocks)
		return IMGTOOLERR_CORRUPTIMAGE;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_open_525(imgtool_image *image)
{
	prodos_setprocs_525(image);
	return prodos_diskimage_open(image);
}



static imgtoolerr_t prodos_diskimage_open_35(imgtool_image *image)
{
	prodos_setprocs_35(image);
	return prodos_diskimage_open(image);
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_load_volume_bitmap(imgtool_image *image, UINT8 **bitmap)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT8 *alloc_bitmap;
	UINT32 bitmap_blocks, i;

	di = get_prodos_info(image);

	bitmap_blocks = (di->total_blocks + (BLOCK_SIZE * 8) - 1) / (BLOCK_SIZE * 8);
	alloc_bitmap = malloc(bitmap_blocks * BLOCK_SIZE);
	if (!alloc_bitmap)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	for (i = 0; i < bitmap_blocks; i++)
	{
		err = prodos_load_block(image, di->volume_bitmap_block + i,
			&alloc_bitmap[i * BLOCK_SIZE]);
		if (err)
			goto done;
	}

	err = IMGTOOLERR_SUCCESS;

done:
	if (err && alloc_bitmap)
	{
		free(alloc_bitmap);
		alloc_bitmap = NULL;
	}
	*bitmap = alloc_bitmap;
	return err;
}



static imgtoolerr_t prodos_save_volume_bitmap(imgtool_image *image, const UINT8 *bitmap)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT32 bitmap_blocks, i;

	di = get_prodos_info(image);

	bitmap_blocks = (di->total_blocks + (BLOCK_SIZE * 8) - 1) / (BLOCK_SIZE * 8);

	for (i = 0; i < bitmap_blocks; i++)
	{
		err = prodos_save_block(image, di->volume_bitmap_block + i,
			&bitmap[i * BLOCK_SIZE]);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



static void prodos_set_volume_bitmap_bit(UINT8 *buffer, UINT16 block, int value)
{
	UINT8 mask;
	buffer += block / 8;
	mask = 1 << (7 - (block % 8));
	if (value)
		*buffer |= mask;
	else
		*buffer &= ~mask;
}



static int prodos_get_volume_bitmap_bit(const UINT8 *buffer, UINT16 block)
{
	UINT8 mask;
	buffer += block / 8;
	mask = 1 << (7 - (block % 8));
	return (*buffer & mask) ? 1 : 0;
}



static imgtoolerr_t prodos_alloc_block(imgtool_image *image, UINT8 *bitmap,
	UINT16 *block)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct prodos_diskinfo *di;
	UINT16 bitmap_blocks, i;
	UINT8 *alloc_bitmap = NULL;

	di = get_prodos_info(image);
	*block = 0;
	bitmap_blocks = (di->total_blocks + (BLOCK_SIZE * 8) - 1) / (BLOCK_SIZE * 8);

	if (!bitmap)
	{
		err = prodos_load_volume_bitmap(image, &alloc_bitmap);
		if (err)
			goto done;
		bitmap = alloc_bitmap;
	}

	for (i = (di->volume_bitmap_block + bitmap_blocks); i < di->total_blocks; i++)
	{
		if (!prodos_get_volume_bitmap_bit(bitmap, i))
		{
			prodos_set_volume_bitmap_bit(bitmap, i, 1);
			*block = i;
			break;
		}
	}

	if (*block > 0)
	{
		if (alloc_bitmap)
		{
			err = prodos_save_volume_bitmap(image, bitmap);
			if (err)
				goto done;
		}
	}
	else
	{
		err = IMGTOOLERR_NOSPACE;
	}

done:
	if (err)
		*block = 0;
	if (alloc_bitmap)
		free(alloc_bitmap);
	return err;
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_diskimage_create(imgtool_image *image, option_resolution *opts)
{
	imgtoolerr_t err;
	UINT32 heads, tracks, sectors, sector_bytes;
	UINT32 dirent_size, volume_bitmap_block, i;
	UINT32 volume_bitmap_block_count, total_blocks;
	UINT8 buffer[BLOCK_SIZE];

	heads = option_resolution_lookup_int(opts, 'H');
	tracks = option_resolution_lookup_int(opts, 'T');
	sectors = option_resolution_lookup_int(opts, 'S');
	sector_bytes = option_resolution_lookup_int(opts, 'L');

	dirent_size = 39;
	volume_bitmap_block = 6;
	total_blocks = tracks * heads * sectors * sector_bytes / BLOCK_SIZE;
	volume_bitmap_block_count = (total_blocks + (BLOCK_SIZE * 8) - 1) / (BLOCK_SIZE * 8);

	/* prepare initial dir block */
	memset(buffer, 0, sizeof(buffer));
	place_integer(buffer, 4 +  0, 1, 0xF0);
	place_integer(buffer, 4 + 31, 1, dirent_size);
	place_integer(buffer, 4 + 32, 1, BLOCK_SIZE / dirent_size);
	place_integer(buffer, 4 + 35, 2, volume_bitmap_block);
	place_integer(buffer, 4 + 37, 2, total_blocks);

	err = prodos_save_block(image, ROOTDIR_BLOCK, buffer);
	if (err)
		return err;

	/* setup volume bitmap */
	memset(buffer, 0, sizeof(buffer));
	for (i = 0; i < (volume_bitmap_block + volume_bitmap_block_count); i++)
		prodos_set_volume_bitmap_bit(buffer, i, 1);
	prodos_save_block(image, volume_bitmap_block, buffer);

	/* and finally open the image */
	return prodos_diskimage_open(image);
}



static imgtoolerr_t prodos_diskimage_create_525(imgtool_image *image, option_resolution *opts)
{
	prodos_setprocs_525(image);
	return prodos_diskimage_create(image, opts);
}



static imgtoolerr_t prodos_diskimage_create_35(imgtool_image *image, option_resolution *opts)
{
	prodos_setprocs_35(image);
	return prodos_diskimage_create(image, opts);
}



/* ----------------------------------------------------------------------- */

static imgtoolerr_t prodos_enum_seek(imgtool_image *image,
	struct prodos_direnum *appleenum, UINT32 block, UINT32 index)
{
	imgtoolerr_t err;
	UINT8 buffer[BLOCK_SIZE];

	if (appleenum->block != block)
	{
		if (block != 0)
		{
			err = prodos_load_block(image, block, buffer);
			if (err)
				return err;
			memcpy(appleenum->block_data, buffer, sizeof(buffer));
		}
		appleenum->block = block;
	}

	appleenum->index = index;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_get_next_dirent(imgtool_image *image,
	struct prodos_direnum *appleenum, struct prodos_dirent *ent)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT32 next_block, next_index;
	UINT32 offset;

	di = get_prodos_info(image);
	memset(ent, 0, sizeof(*ent));

	/* have we hit the end of the file? */
	if (appleenum->block == 0)
		return IMGTOOLERR_SUCCESS;

	/* populate the resulting dirent */
	offset = appleenum->index * di->dirent_size + 4;
	ent->storage_type = appleenum->block_data[offset + 0];
	memcpy(ent->filename, &appleenum->block_data[offset + 1], 15);
	ent->filename[15] = '\0';
	ent->key_pointer		= pick_integer(appleenum->block_data, offset + 17, 2);
	ent->filesize			= pick_integer(appleenum->block_data, offset + 21, 3);
	ent->creation_time		= pick_integer(appleenum->block_data, offset + 24, 4);
	ent->lastmodified_time	= pick_integer(appleenum->block_data, offset + 33, 4);

	/* identify next entry */
	next_block = appleenum->block;
	next_index = appleenum->index + 1;
	if (next_index >= di->dirents_per_block)
	{
		next_block = pick_integer(appleenum->block_data, 2, 2);
		next_index = 0;
	}

	/* seek next block */
	err = prodos_enum_seek(image, appleenum, next_block, next_index);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_put_dirent(imgtool_image *image,
	struct prodos_direnum *appleenum, const struct prodos_dirent *ent)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT32 offset;

	di = get_prodos_info(image);
	offset = appleenum->index * di->dirent_size + 4;

	appleenum->block_data[offset + 0] = ent->storage_type;
	memcpy(&appleenum->block_data[offset + 1], ent->filename, 15);
	place_integer(appleenum->block_data, offset + 17, 2, ent->key_pointer);
	place_integer(appleenum->block_data, offset + 21, 3, ent->filesize);
	place_integer(appleenum->block_data, offset + 24, 4, ent->creation_time);
	place_integer(appleenum->block_data, offset + 33, 4, ent->lastmodified_time);

	err = prodos_save_block(image, appleenum->block, appleenum->block_data);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_lookup_path(imgtool_image *image, const char *path,
	creation_policy_t create, struct prodos_direnum *direnum, struct prodos_dirent *ent)
{
	imgtoolerr_t err;
	struct prodos_direnum my_direnum;
	UINT32 block = ROOTDIR_BLOCK;
	const char *old_path;
	UINT16 this_block;
	UINT32 this_index;
	UINT16 free_block = 0;
	UINT32 free_index = 0;
	UINT16 new_file_block;
	UINT8 buffer[BLOCK_SIZE];

	if (!direnum)
		direnum = &my_direnum;

	while(*path)
	{
		memset(direnum, 0, sizeof(*direnum));
		err = prodos_enum_seek(image, direnum, block, 0);
		if (err)
			goto done;

		do
		{
			this_block = direnum->block;
			this_index = direnum->index;

			err = prodos_get_next_dirent(image, direnum, ent);
			if (err)
				goto done;

			/* if we need to create a file entry and this is free, track it */
			if (create && this_block && !free_block && !ent->storage_type)
			{
				free_block = this_block;
				free_index = this_index;
			}
		}
		while(direnum->block && (strcmp(path, ent->filename) || (
			!is_file_storagetype(ent->storage_type) &&
			!is_dir_storagetype(ent->storage_type))));

		old_path = path;
		path += strlen(path) + 1;	
		if (*path)
		{
			/* we have found part of the path; we are not finished yet */
			if (!is_dir_storagetype(ent->storage_type))
			{
				err = IMGTOOLERR_FILENOTFOUND;
				goto done;
			}
			block = ent->key_pointer;
		}
		else if (!direnum->block)
		{
			/* did not find file; maybe we need to create it */
			if (create == CREATE_NONE)
			{
				err = IMGTOOLERR_FILENOTFOUND;
				goto done;
			}

			/* do we need to expand the directory? */
			if (!free_block)
			{
				if (this_block == 0)
				{
					err = IMGTOOLERR_CORRUPTFILE;
					goto done;
				}

				err = prodos_load_block(image, this_block, buffer);
				if (err)
					goto done;

				/* allocate a block */
				err = prodos_alloc_block(image, NULL, &free_block);
				if (err)
					goto done;

				/* clear out this new block */
				err = prodos_clear_block(image, free_block);
				if (err)
					goto done;

				/* save this link */
				place_integer(buffer, 2, 2, free_block);
				err = prodos_save_block(image, this_block, buffer);
				if (err)
					goto done;

				free_index = 0;
			}

			/* seek back to the free space */
			err = prodos_enum_seek(image, direnum, free_block, free_index);
			if (err)
				goto done;

			new_file_block = 0;
			if (create == CREATE_DIR)
			{
				/* if we are creating a directory, we need to create a new block */
				err = prodos_alloc_block(image, NULL, &new_file_block);
				if (err)
					goto done;

				err = prodos_clear_block(image, new_file_block);
				if (err)
					goto done;
			}

			/* prepare the dirent */
			memset(ent, 0, sizeof(*ent));
			ent->storage_type = (create == CREATE_DIR) ? 0xe0 : 0x10;
			ent->creation_time = ent->lastmodified_time = prodos_time_now();
			ent->key_pointer = new_file_block;
			strncpy(ent->filename, old_path, sizeof(ent->filename) / sizeof(ent->filename[0]));

			/* and place it */
			err = prodos_put_dirent(image, direnum, ent);
			if (err)
				goto done;

			this_block = free_block;
			this_index = free_index;
		}
		else
		{
			/* we've found the file; seek that dirent */
			err = prodos_enum_seek(image, direnum, this_block, this_index);
			if (err)
				goto done;
		}
	}

	err = IMGTOOLERR_SUCCESS;
done:
	return err;
}



static imgtoolerr_t prodos_fill_file(imgtool_image *image, UINT8 *bitmap,
	UINT16 key_block, int key_block_allocated,
	int depth, UINT32 blockcount, UINT32 block_index)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	int dirty;
	int sub_block_allocated;
	UINT16 i, sub_block, new_sub_block;
	UINT8 buffer[BLOCK_SIZE];

	di = get_prodos_info(image);

	if (key_block_allocated)
	{
		/* we are on a recently allocated key block; start fresh */
		memset(buffer, 0, sizeof(buffer));
		dirty = TRUE;
	}
	else
	{
		/* this is a preexisting key block */
		err = prodos_load_block(image, key_block, buffer);
		if (err)
			return err;
		dirty = FALSE;
	}

	for (i = 0; i < 256; i++)
	{		
		sub_block_allocated = FALSE;

		sub_block = buffer[i + 256];
		sub_block <<= 8;
		sub_block += buffer[i + 0];

		new_sub_block = sub_block;
		if ((block_index < blockcount) && (sub_block == 0))
		{
			err = prodos_alloc_block(image, bitmap, &new_sub_block);
			if (err)
				return err;
			sub_block_allocated = TRUE;
		}
		else if ((block_index >= blockcount) && (sub_block != 0))
		{
			new_sub_block = 0;
			if (sub_block < di->total_blocks)
				prodos_set_volume_bitmap_bit(bitmap, sub_block, 0);
		}

		/* did we change the block? */
		if (new_sub_block != sub_block)
		{
			dirty = TRUE;
			buffer[i + 0] = new_sub_block >> 0;
			buffer[i + 256] = new_sub_block >> 8;
			if (sub_block == 0)
				sub_block = new_sub_block;
		}

		/* call recursive function */
		if (depth > 2)
		{
			err = prodos_fill_file(image, bitmap, sub_block, sub_block_allocated, depth - 1, blockcount, block_index);
			if (err)
				return err;
		}

		/* increment index */
		block_index += 1 << ((depth - 2) * 8);
	}

	/* if we changed anything, then save the block */
	if (dirty)
	{
		err = prodos_save_block(image, key_block, buffer);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_set_file_block_count(imgtool_image *image, struct prodos_direnum *direnum,
	struct prodos_dirent *ent, UINT8 *bitmap, UINT32 new_blockcount)
{
	imgtoolerr_t err;
	int depth, new_depth, i;
	UINT16 new_block, block;
	UINT8 buffer[BLOCK_SIZE];

	/* determine the current tree depth */
	depth = ent->storage_type / 0x10;

	/* determine the new tree depth */
	if (new_blockcount <= 1)
		new_depth = 1;
	else if (new_blockcount <= 256)
		new_depth = 2;
	else
		new_depth = 3;

	/* are we zero length, and do we have to create a block? */
	if ((new_blockcount >= 1) && (ent->key_pointer == 0))
	{
		err = prodos_alloc_block(image, bitmap, &new_block);
		if (err)
			return err;
		ent->key_pointer = new_block;
	}

	/* do we have to grow the tree? */
	while(new_depth > depth)
	{
		err = prodos_alloc_block(image, bitmap, &new_block);
		if (err)
			return err;

		/* create this new key block, with a link to the previous one */
		memset(buffer, 0, sizeof(buffer));
		buffer[0] = (UINT8) (ent->key_pointer >> 0);
		buffer[256] = (UINT8) (ent->key_pointer >> 8);
		err = prodos_save_block(image, new_block, buffer);
		if (err)
			return err;

		depth++;
		ent->key_pointer = new_block;
		ent->storage_type &= ~0xF0;
		ent->storage_type |= depth * 0x10;
	}

	/* do we have to shrink the tree? */
	while(new_depth < depth)
	{
		err = prodos_load_block(image, ent->key_pointer, buffer);
		if (err)
			return err;

		for (i = 1; i < 256; i++)
		{
			block = buffer[i + 256];
			block <<= 8;
			block |= buffer[i + 0];

			if (block > 0)
			{
				if (depth > 2)
				{
					/* remove this block's children */
					err = prodos_fill_file(image, bitmap, block, FALSE, depth - 1, 0, 0);
					if (err)
						return err;
				}

				/* and remove this block */
				prodos_set_volume_bitmap_bit(bitmap, block, 0);
			}
		}

		/* remove this key block */
		prodos_set_volume_bitmap_bit(bitmap, ent->key_pointer, 0);

		/* set the new key pointer */
		block = buffer[256];
		block <<= 8;
		block |= buffer[0];
		ent->key_pointer = block;
	
		depth--;
		ent->storage_type &= ~0xF0;
		ent->storage_type |= depth * 0x10;
	}

	if (new_blockcount > 0)
	{
		/* fill out the file tree */
		err = prodos_fill_file(image, bitmap, ent->key_pointer, FALSE, depth, new_blockcount, 0);
		if (err)
			return err;
	}
	else if (ent->key_pointer != 0)
	{
		/* we are now zero length, and don't need a key pointer */
		prodos_set_volume_bitmap_bit(bitmap, ent->key_pointer, 0);
		ent->key_pointer = 0;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_set_file_size(imgtool_image *image, struct prodos_direnum *direnum,
	struct prodos_dirent *ent, UINT32 new_size)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	UINT32 blockcount, new_blockcount;
	UINT8 *bitmap = NULL;

	if (ent->filesize != new_size)
	{
		blockcount = (ent->filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;
		new_blockcount = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

		/* do we need to change the block chain? */
		if (new_blockcount != blockcount)
		{
			err = prodos_load_volume_bitmap(image, &bitmap);
			if (err)
				goto done;

			err = prodos_set_file_block_count(image, direnum, ent, bitmap, new_blockcount);
			if (err)
				goto done;

			err = prodos_save_volume_bitmap(image, bitmap);
			if (err)
				goto done;
		}

		ent->filesize = new_size;
		err = prodos_put_dirent(image, direnum, ent);
		if (err)
			goto done;
	}

done:
	if (bitmap)
		free(bitmap);
	return err;
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
		case 0x50:
			max_filesize = BLOCK_SIZE * 32768;
			break;
	}
	return max_filesize;
}



static imgtoolerr_t prodos_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err;
	imgtool_image *image;
	struct prodos_direnum *appleenum;
	struct prodos_dirent ent;
	UINT16 block = ROOTDIR_BLOCK;

	image = img_enum_image(enumeration);
	appleenum = (struct prodos_direnum *) img_enum_extrabytes(enumeration);

	/* find subdirectory, if appropriate */
	if (*path)
	{
		err = prodos_lookup_path(image, path, CREATE_NONE, NULL, &ent);
		if (err)
			return err;

		/* only work on directories */
		if (!is_dir_storagetype(ent.storage_type))
			return IMGTOOLERR_FILENOTFOUND;

		block = ent.key_pointer;
	}

	/* seek initial block */
	err = prodos_enum_seek(image, appleenum, block, 0);
	if (err)
		return err;

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

	do
	{
		err = prodos_get_next_dirent(image, appleenum, &pd_ent);
		if (err)
			return err;
	}
	while(appleenum->block
		&& !is_file_storagetype(pd_ent.storage_type)
		&& !is_dir_storagetype(pd_ent.storage_type));

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



static imgtoolerr_t prodos_read_file_tree(imgtool_image *image, UINT32 *filesize,
	UINT32 block, int nest_level, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT8 buffer[BLOCK_SIZE];
	UINT16 sub_block;
	size_t bytes_to_write;
	int i;

	/* check bounds */
	di = get_prodos_info(image);
	if (block >= di->total_blocks)
		return IMGTOOLERR_CORRUPTFILE;

	err = prodos_load_block(image, block, buffer);
	if (err)
		return err;

	if (nest_level > 0)
	{
		/* this is an index block */
		for (i = 0; i < 256; i++)
		{
			/* retrieve the block pointer; the two bytes are on either half
			 * of the block */
			sub_block = buffer[i + 256];
			sub_block <<= 8;
			sub_block |= buffer[i + 0];

			if (sub_block != 0)
			{
				err = prodos_read_file_tree(image, filesize, sub_block, nest_level - 1, destf);
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



static imgtoolerr_t prodos_write_file_tree(imgtool_image *image, UINT32 *filesize,
	UINT32 block, int nest_level, imgtool_stream *sourcef)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT8 buffer[BLOCK_SIZE];
	UINT16 sub_block;
	size_t bytes_to_read;
	int i;

	/* nothing more to read? bail */
	if (*filesize == 0)
		return IMGTOOLERR_SUCCESS;

	/* check bounds */
	di = get_prodos_info(image);
	if (block >= di->total_blocks)
		return IMGTOOLERR_CORRUPTFILE;

	err = prodos_load_block(image, block, buffer);
	if (err)
		return err;

	if (nest_level > 0)
	{
		for (i = 0; i < 256; i++)
		{
			sub_block = buffer[i + 256];
			sub_block <<= 8;
			sub_block |= buffer[i + 0];

			if (sub_block != 0)
			{
				err = prodos_write_file_tree(image, filesize, sub_block, nest_level - 1, sourcef);
				if (err)
					return err;
			}
		}
	}
	else
	{
		/* this is a leaf block */
		bytes_to_read = MIN(*filesize, sizeof(buffer));
		stream_read(sourcef, buffer, bytes_to_read);
		*filesize -= bytes_to_read;

		err = prodos_save_block(image, block, buffer);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_freespace(imgtool_image *image, UINT64 *size)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	UINT8 *bitmap = NULL;
	UINT16 i;

	di = get_prodos_info(image);
	*size = 0;

	err = prodos_load_volume_bitmap(image, &bitmap);
	if (err)
		goto done;

	for (i = 0; i < di->total_blocks; i++)
	{
		if (!prodos_get_volume_bitmap_bit(bitmap, i))
			*size += BLOCK_SIZE;
	}

done:
	if (bitmap)
		free(bitmap);
	return err;
}



static imgtoolerr_t prodos_diskimage_readfile(imgtool_image *image, const char *filename, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	UINT8 buffer[BLOCK_SIZE];
	UINT16 key_pointer;
	int nest_level;

	err = prodos_lookup_path(image, filename, CREATE_NONE, NULL, &ent);
	if (err)
		return err;

	if (is_dir_storagetype(ent.storage_type))
		return IMGTOOLERR_FILENOTFOUND;

	if ((ent.storage_type & 0xF0) == 0x50)
	{
		/* special case for storage class $5 */
		err = prodos_load_block(image, ent.key_pointer, buffer);
		if (err)
			return err;

		key_pointer = pick_integer(buffer, 1, 2);
		nest_level = (buffer[0] & 0x0F) - 1;

		if ((nest_level < 0) || (nest_level > 2))
			return IMGTOOLERR_CORRUPTFILE;
	}
	else
	{
		/* conventional ProDOS file */
		key_pointer = ent.key_pointer;
		nest_level = (ent.storage_type >> 4) - 1;
	}

	err = prodos_read_file_tree(image, &ent.filesize, key_pointer, 
		nest_level, destf);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_writefile(imgtool_image *image, const char *filename, imgtool_stream *sourcef, option_resolution *opts)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	struct prodos_direnum direnum;
	UINT64 file_size;

	file_size = stream_size(sourcef);

	err = prodos_lookup_path(image, filename, CREATE_FILE, &direnum, &ent);
	if (err)
		return err;

	/* only work on files */
	if (is_dir_storagetype(ent.storage_type))
		return IMGTOOLERR_FILENOTFOUND;

	/* set the file size */
	err = prodos_set_file_size(image, &direnum, &ent, file_size);
	if (err)
		return err;

	err = prodos_write_file_tree(image, &ent.filesize, ent.key_pointer,
		(ent.storage_type >> 4) - 1, sourcef);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_deletefile(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	struct prodos_direnum direnum;

	err = prodos_lookup_path(image, path, CREATE_NONE, &direnum, &ent);
	if (err)
		return err;

	/* only work on files */
	if (is_dir_storagetype(ent.storage_type))
		return IMGTOOLERR_FILENOTFOUND;

	err = prodos_set_file_size(image, &direnum, &ent, 0);
	if (err)
		return err;

	memset(&ent, 0, sizeof(ent));
	err = prodos_put_dirent(image, &direnum, &ent);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_createdir(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	struct prodos_direnum direnum;

	err = prodos_lookup_path(image, path, CREATE_DIR, &direnum, &ent);
	if (err)
		return err;

	/* only work on directories */
	if (!is_dir_storagetype(ent.storage_type))
		return IMGTOOLERR_FILENOTFOUND;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_free_directory(imgtool_image *image, UINT8 *volume_bitmap, UINT16 key_pointer)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	int i;
	UINT16 next_block;
	UINT32 offset;
	UINT8 buffer[BLOCK_SIZE];

	di = get_prodos_info(image);

	if (key_pointer != 0)
	{
		err = prodos_load_block(image, key_pointer, buffer);
		if (err)
			return err;

		for (i = 0; i < di->dirents_per_block; i++)
		{
			offset = i * di->dirent_size + 4;

			if (is_file_storagetype(buffer[offset]) || is_file_storagetype(buffer[offset]))
				return IMGTOOLERR_DIRNOTEMPTY;
		}

		next_block = pick_integer(buffer, 2, 2);

		err = prodos_free_directory(image, volume_bitmap, next_block);
		if (err)
			return err;

		prodos_set_volume_bitmap_bit(volume_bitmap, key_pointer, 0);
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_diskimage_deletedir(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	struct prodos_direnum direnum;
	UINT8 *volume_bitmap = NULL;

	err = prodos_lookup_path(image, path, CREATE_NONE, &direnum, &ent);
	if (err)
		goto done;

	/* only work on directories */
	if (!is_dir_storagetype(ent.storage_type))
	{
		err = IMGTOOLERR_FILENOTFOUND;
		goto done;
	}

	err = prodos_load_volume_bitmap(image, &volume_bitmap);
	if (err)
		goto done;

	err = prodos_free_directory(image, volume_bitmap, ent.key_pointer);
	if (err)
		goto done;

	err = prodos_save_volume_bitmap(image, volume_bitmap);
	if (err)
		goto done;

	memset(&ent, 0, sizeof(ent));
	err = prodos_put_dirent(image, &direnum, &ent);
	if (err)
		goto done;

done:
	if (volume_bitmap)
		free(volume_bitmap);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t prodos_get_file_tree(imgtool_image *image, imgtool_chainent *chain, size_t chain_size,
	size_t *chain_pos, UINT16 block, UINT8 total_depth, UINT8 cur_depth)
{
	imgtoolerr_t err;
	struct prodos_diskinfo *di;
	int i;
	UINT16 sub_block;
	UINT8 buffer[BLOCK_SIZE];

	if (block == 0)
		return IMGTOOLERR_SUCCESS;
	if (*chain_pos >= chain_size)
		return IMGTOOLERR_SUCCESS;

	/* check bounds */
	di = get_prodos_info(image);
	if (block >= di->total_blocks)
		return IMGTOOLERR_CORRUPTFILE;

	chain[*chain_pos].level = cur_depth;
	chain[*chain_pos].block = block;
	(*chain_pos)++;

	/* must we recurse into the tree? */
	if (cur_depth < total_depth)
	{
		err = prodos_load_block(image, block, buffer);
		if (err)
			return err;

		for (i = 0; i < 256; i++)
		{
			sub_block = buffer[i + 256];
			sub_block <<= 8;
			sub_block |= buffer[i + 0];

			err = prodos_get_file_tree(image, chain, chain_size, chain_pos, sub_block, total_depth, cur_depth + 1);
			if (err)
				return err;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t	prodos_diskimage_getchain(imgtool_image *image, const char *path, imgtool_chainent *chain, size_t chain_size)
{
	imgtoolerr_t err;
	struct prodos_dirent ent;
	size_t chain_pos;

	err = prodos_lookup_path(image, path, CREATE_NONE, NULL, &ent);
	if (err)
		return err;

	switch(ent.storage_type & 0xF0)
	{
		case 0x10:
		case 0x20:
		case 0x30:
			chain_pos = 0;
			err = prodos_get_file_tree(image, chain, chain_size, &chain_pos,
				ent.key_pointer, (ent.storage_type / 0x10) - 1, 0);
			if (err)
				return err;
			break;

		case 0xE0:
			return IMGTOOLERR_UNIMPLEMENTED;

		default:
			return IMGTOOLERR_UNEXPECTED;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t apple2_prodos_module_populate(imgtool_library *library, struct ImgtoolFloppyCallbacks *module)
{
	module->initial_path_separator		= 1;
	module->open_is_strict				= 1;
	module->supports_creation_time		= 1;
	module->supports_lastmodified_time	= 1;
	module->writing_untested			= 1;
	module->image_extra_bytes			+= sizeof(struct prodos_diskinfo);
	module->imageenum_extra_bytes		+= sizeof(struct prodos_direnum);
	module->eoln						= EOLN_CR;
	module->path_separator				= '/';
	module->begin_enum					= prodos_diskimage_beginenum;
	module->next_enum					= prodos_diskimage_nextenum;
	module->free_space					= prodos_diskimage_freespace;
	module->read_file					= prodos_diskimage_readfile;
	module->write_file					= prodos_diskimage_writefile;
	module->delete_file					= prodos_diskimage_deletefile;
	module->create_dir					= prodos_diskimage_createdir;
	module->delete_dir					= prodos_diskimage_deletedir;
	module->get_chain					= prodos_diskimage_getchain;
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
