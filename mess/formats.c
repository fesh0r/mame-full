#include <stdlib.h>
#include <stdio.h>
#include "formats.h"
#include "utils.h"

#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

/**************************************************************************/

struct _bdf_file
{
	void *file;
	const struct bdf_procs *procs;
	struct disk_geometry geometry;
	int offset;						/* offset of the first byte in the file */
	int position;					/* current position */
	int is_readonly;
	UINT8 filler_byte;
	int (*read_sector)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
	int (*write_sector)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
	void (*get_sector_info)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
	UINT8 (*get_sector_count)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head);
	char header;
};

static int default_read_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
static int default_write_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
static void default_get_sector_info(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
static UINT8 default_get_sector_count(bdf_file *bdf, const void *header, UINT8 track, UINT8 head);

int bdf_create(const struct bdf_procs *procs, formatdriver_ctor format,
	void *file, const struct disk_geometry *geometry, void **outbdf)
{
	char buffer[1024];
	void *header = NULL;
	struct InternalBdFormatDriver drv;
	int bytes_to_write, len, err;
	UINT32 header_size;
	formatdriver_ctor formats[2];
	struct disk_geometry local_geometry;
	UINT8 track, head;
	bdf_file dummy_bdf;
	
	if (!geometry)
	{
		/* HACKHACK - the default geometry is hardcoded! This should be specified by
		 * the FormatDriver
		 */
		memset(&local_geometry, 0, sizeof(local_geometry));
		local_geometry.tracks = 35;
		local_geometry.heads = 1;
		local_geometry.sectors = 18;
		local_geometry.first_sector_id = 1;
		local_geometry.sector_size = 256;
		geometry = &local_geometry;
	}

	format(&drv);

	if( drv.header_encode == NULL )
		return BLOCKDEVICE_ERROR_CANTENCODEFORMAT;

	/* do we have a header size specified? */
	header_size = drv.header_size & ~HEADERSIZE_FLAGS;
	if (drv.header_size & HEADERSIZE_FLAG_MODULO)
		header_size--;	/* allow a maximum header size of the modulus minus one */

	if (header_size > 0)
	{
		header = malloc(header_size);
		if (!header)
			goto outofmemory;

		err = drv.header_encode(header, &header_size, geometry);
		if (err)
			goto error;

		if (header_size)
			procs->writeproc(file, header, header_size);

		free(header);
		header = NULL;
	}

	if (drv.format_track)
	{
		memset(&dummy_bdf, 0, sizeof(dummy_bdf));
		dummy_bdf.procs = procs;
		dummy_bdf.file = file;
		for(track = 0; track < geometry->tracks; track++)
		{
			for( head = 0; head < geometry->heads; head++ )
			{
				err = drv.format_track(&drv, (void *) &dummy_bdf, geometry, track, head);
				if (err)
					goto error;
			}
		}
	}
	else
	{
		bytes_to_write = ((int) drv.bytes_per_sector) * geometry->tracks * geometry->heads * geometry->sectors;
		memset(buffer, drv.filler_byte, sizeof(buffer));

		while(bytes_to_write > 0)
		{
			len = (bytes_to_write > sizeof(buffer)) ? sizeof(buffer) : bytes_to_write;
			procs->writeproc(file, buffer, len);
			bytes_to_write -= len;
		}
	}

	if (outbdf)
	{
		formats[0] = format;
		formats[1] = NULL;
		err = bdf_open(procs, formats, file, 0, NULL, outbdf);
		if (err)
			goto error;
	}
	return 0;

outofmemory:
	err = BLOCKDEVICE_ERROR_OUTOFMEMORY;
error:
	if (header)
		free(header);
	return err;
}

static int try_format_driver(const struct InternalBdFormatDriver *drv, const struct bdf_procs *procs,
	const char *extension, void *file, UINT32 file_size,
	bdf_file **bdffile)
{
	UINT32 header_size;
	int bdf_size;
	bdf_file *bdf;

	*bdffile = NULL;

	/* match the extension; if either the formatdriver or the caller do not
	 * specify the extension, count that as a match
	 */
	if (extension && drv->extension && strcmpi(extension, drv->extension))
		return 0;

	/* if there is a header, try to read it */
	header_size = drv->header_size & ~HEADERSIZE_FLAGS;
	if (drv->header_size & HEADERSIZE_FLAG_MODULO)
		header_size = file_size % header_size;

	/* is the expected header larger than the file? */
	if (header_size > file_size)
		return 0;

	/* allocate the bdf_file */
	bdf_size = sizeof(struct _bdf_file) - sizeof(char) + header_size;
	bdf = malloc(bdf_size);
	if (!bdf)
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
	memset(bdf, 0, bdf_size);

	if (header_size > 0)
	{
		/* read the header */
		procs->seekproc(file, 0, SEEK_SET);
		procs->readproc(file, &bdf->header, header_size);

		/* reallocate the bdf so that it takes up the right size */
		bdf = realloc(bdf, sizeof(struct _bdf_file) - sizeof(char) + header_size);
		if (!bdf)
			return BLOCKDEVICE_ERROR_OUTOFMEMORY;
	}

	/* try to decode the header */
	bdf->offset = header_size;	/* the default offset is the header size */
	if (!drv->header_decode(header_size ? &bdf->header : NULL, file_size, header_size, &bdf->geometry, &bdf->offset))
	{
		/* success! */
		*bdffile = bdf;
	}
	else
	{
		/* failure; free the bdf_file */
		free(bdf);
	}
	return 0;
}

int bdf_open(const struct bdf_procs *procs, const formatdriver_ctor *formats,
	void *file, int is_readonly, const char *extension, void **outbdf)
{
	int err;
	bdf_file *bdf;
	int filesize;
	void *header;
	struct InternalBdFormatDriver drv;

	assert(formats);
	assert(file);
	assert(procs);
	assert(procs->filesizeproc);
	assert(procs->seekproc);
	assert(procs->readproc);
	assert(procs->writeproc);

	header = NULL;

	/* gather some initial information about the file */
	filesize = procs->filesizeproc(file);

	/* the first task is to locate an appropriate format driver */
	bdf = NULL;
	while(!bdf && *formats)
	{
		/* build the format driver */
		(*formats)(&drv);

		err = try_format_driver(&drv, procs, extension, file, filesize, &bdf);
		if (err)
			goto done;

		if (!bdf)
			formats++;
	}

	/* did we find an appropriate format driver? */
	if (!bdf)
	{
		err = BLOCKDEVICE_ERROR_CANTDECODEFORMAT;
		goto done;
	}

	bdf->file = file;
	bdf->procs = procs;
	bdf->is_readonly = is_readonly;
	bdf->filler_byte = drv.filler_byte;
	bdf->position = 0;
	bdf->read_sector = drv.read_sector ? drv.read_sector : default_read_sector;
	bdf->write_sector = drv.write_sector ? drv.write_sector : default_write_sector;
	bdf->get_sector_info = drv.get_sector_info ? drv.get_sector_info : default_get_sector_info;
	bdf->get_sector_count = drv.get_sector_count ? drv.get_sector_count : default_get_sector_count;
	err = BLOCKDEVICE_ERROR_SUCCESS;

done:
	if (err)
	{
		if (bdf)
			free(bdf);
		bdf = NULL;
	}
	*outbdf = (void *) bdf;
	return err;
}

void bdf_close(bdf_file *bdf)
{
	if (bdf->procs->closeproc)
		bdf->procs->closeproc(bdf->file);
	free(bdf);
}

int bdf_read(bdf_file *bdf, void *buffer, int length)
{
	int file_size;
	int actual_length = length;
	int bytes_read;

	file_size = bdf->procs->filesizeproc(bdf->file);
	actual_length = MAX(MIN(length, file_size - bdf->position), 0);
	bytes_read = bdf->procs->readproc(bdf->file, buffer, actual_length);
	if (actual_length < length)
	{
		memset(((UINT8 *) buffer) + actual_length, bdf->filler_byte, length - actual_length);
		bytes_read += length - actual_length;
	}
	bdf->position += bytes_read;
	return bytes_read;
}

int bdf_write(bdf_file *bdf, const void *buffer, int length)
{
	int bytes_written;

	bytes_written = bdf->procs->writeproc(bdf->file, buffer, length);
	bdf->position += bytes_written;
	return bytes_written;
}

static void bdf_writefiller(bdf_file *bdf, int filler_length)
{
	UINT8 filler[512];
	int this_filler_length;

	memset(filler, bdf->filler_byte, MIN(sizeof(filler), filler_length));
	while(filler_length > 0)
	{
		this_filler_length = MIN(sizeof(filler), filler_length);
		bdf->procs->writeproc(bdf->file, filler, this_filler_length);
		filler_length -= this_filler_length;
	}
	bdf->position += filler_length;
}

int bdf_seek(bdf_file *bdf, int offset)
{
	int file_size;
	int actual_offset = offset;
	int grow = 0;

	file_size = bdf->procs->filesizeproc(bdf->file);
	if (file_size < offset)
	{
		grow = offset - file_size;
		actual_offset = file_size;
	}
	bdf->procs->seekproc(bdf->file, actual_offset, SEEK_SET);

	/* grow file if necessary */
	if ((grow > 0) && !bdf->is_readonly)
		bdf_writefiller(bdf, grow);
	bdf->position = offset;
	return 0;
}

const struct disk_geometry *bdf_get_geometry(bdf_file *bdf)
{
	return &bdf->geometry;
}

static int default_seek_sector(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset)
{
	int pos;

	sector -= bdf->geometry.first_sector_id;
	if ((track >= bdf->geometry.tracks) || (head >= bdf->geometry.heads) || (sector >= bdf->geometry.sectors))
		return -1;

	pos = track;
	pos *= bdf->geometry.heads;
	pos += head;
	pos *= bdf->geometry.sectors;
	pos += sector;
	pos *= bdf->geometry.sector_size;
	pos += bdf->offset;
	pos += offset;
	bdf_seek(bdf, pos);
	return 0;
}

static int default_read_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	if (default_seek_sector(bdf, track, head, sector, offset))
		return -1;
	bdf_read(bdf, buffer, length);
	return 0;
}

static int default_write_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	if (default_seek_sector(bdf, track, head, sector, offset))
		return -1;
	bdf_write(bdf, buffer, length);
	return 0;
}

static void default_get_sector_info(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size)
{
	const struct disk_geometry *geo;
	geo = bdf_get_geometry(bdf);
	*sector = geo->first_sector_id + sector_index;
	*sector_size = geo->sector_size;
}

static UINT8 default_get_sector_count(bdf_file *bdf, const void *header, UINT8 track, UINT8 head)
{
	return bdf_get_geometry(bdf)->sectors;
}

int bdf_read_sector(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	return bdf->read_sector(bdf, (const void *) &bdf->header, track, head, sector, offset, buffer, length);
}

int bdf_write_sector(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	return bdf->write_sector(bdf, (const void *) &bdf->header, track, head, sector, offset, buffer, length);
}

int bdf_is_readonly(bdf_file *bdf)
{
	return bdf->is_readonly;
}

void bdf_get_sector_info(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size)
{
	bdf->get_sector_info(bdf, (const void *) &bdf->header, track, head, sector_index, sector, sector_size);
}

UINT8 bdf_get_sector_count(bdf_file *bdf, UINT8 track, UINT8 head)
{
	return bdf->get_sector_count(bdf, (const void *) &bdf->header, track, head);
}

#ifdef MAME_DEBUG
void validate_construct_formatdriver(struct InternalBdFormatDriver *drv, int tracks_optnum, int heads_optnum, int sectors_optnum)
{
	assert(heads_optnum && tracks_optnum && sectors_optnum);
	assert(tracks_optnum < sizeof(drv->tracks_options) / sizeof(drv->tracks_options[0]));
	assert(heads_optnum < sizeof(drv->heads_options) / sizeof(drv->heads_options[0]));
	assert(sectors_optnum < sizeof(drv->sectors_options) / sizeof(drv->sectors_options[0]));
	assert(drv->header_decode);
}
#endif
