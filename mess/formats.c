#include <stdlib.h>
#include <stdio.h>
#include "formats.h"
#include "utils.h"

/**************************************************************************/

struct bdf_file
{
	void *file;
	const struct bdf_procs *procs;
	struct disk_geometry geometry;
	int offset;
	int is_readonly;
	int (*read_sector)(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
	int (*write_sector)(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
	char header;
};

static int default_read_sector(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
static int default_write_sector(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);

static int find_geometry_options(const struct InternalBdFormatDriver *drv, UINT32 file_size, UINT32 header_size,
	 UINT8 *tracks, UINT8 *heads, UINT8 *sectors)
{
	int expected_size;
	int tracks_opt, heads_opt, sectors_opt;

	for(tracks_opt = 0; drv->tracks_options[tracks_opt]; tracks_opt++)
	{
		*tracks = drv->tracks_options[tracks_opt];
		for(heads_opt = 0; drv->heads_options[heads_opt]; heads_opt++)
		{
			*heads = drv->heads_options[heads_opt];
			for(sectors_opt = 0; drv->sectors_options[sectors_opt]; sectors_opt++)
			{
				*sectors = drv->sectors_options[sectors_opt];

				expected_size = *heads * *tracks * *sectors * drv->bytes_per_sector + header_size;
				if (expected_size == file_size)
					return 0;

				if ((drv->flags & BDFD_ROUNDUP_TRACKS) && (expected_size > file_size)
						&& (((expected_size - file_size) % (*heads * *sectors * drv->bytes_per_sector)) == 0))
					return 0;
			}
		}
	}
	return 1;	/* failure */
}

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
	struct bdf_file dummy_bdf;
	
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
			for( head = 1; head < geometry->heads; head++ )
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
	struct bdf_file **bdffile)
{
	UINT32 header_size;
	struct bdf_file *bdf;
	int bdf_size;

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
	bdf_size = sizeof(struct bdf_file) - sizeof(char) + header_size;
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
		bdf = realloc(bdf, sizeof(struct bdf_file) - sizeof(char) + header_size);
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
	struct bdf_file *bdffile;
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
	bdffile = NULL;
	while(!bdffile && *formats)
	{
		/* build the format driver */
		(*formats)(&drv);

		err = try_format_driver(&drv, procs, extension, file, filesize, &bdffile);
		if (err)
			goto done;

		if (!bdffile)
			formats++;
	}

	/* did we find an appropriate format driver? */
	if (!bdffile)
	{
		err = BLOCKDEVICE_ERROR_CANTDECODEFORMAT;
		goto done;
	}

	bdffile->file = file;
	bdffile->procs = procs;
	bdffile->is_readonly = is_readonly;
	bdffile->read_sector = drv.read_sector ? drv.read_sector : default_read_sector;
	bdffile->write_sector = drv.write_sector ? drv.write_sector : default_write_sector;
	err = BLOCKDEVICE_ERROR_SUCCESS;

done:
	if (err)
	{
		if (bdffile)
			free(bdffile);
		bdffile = NULL;
	}
	*outbdf = (void *) bdffile;
	return err;
}

void bdf_close(void *bdf)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	if (bdffile->procs->closeproc)
		bdffile->procs->closeproc(bdffile->file);
	free(bdffile);
}

int bdf_read(void *bdf, void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	return bdffile->procs->readproc(bdffile->file, buffer, length);
}

int bdf_write(void *bdf, const void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	return bdffile->procs->writeproc(bdffile->file, buffer, length);
}

int bdf_seek(void *bdf, int offset, int whence)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	bdffile->procs->seekproc(bdffile->file, offset, whence);
	return 0;
}

const struct disk_geometry *bdf_get_geometry(void *bdf)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	return &bdffile->geometry;
}

static int default_seek_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset)
{
	int pos;

	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	sector -= bdffile->geometry.first_sector_id;
	if ((track >= bdffile->geometry.tracks) || (head >= bdffile->geometry.heads) || (sector >= bdffile->geometry.sectors))
		return -1;

	pos = track;
	pos *= bdffile->geometry.heads;
	pos += head;
	pos *= bdffile->geometry.sectors;
	pos += sector;
	pos *= bdffile->geometry.sector_size;
	pos += bdffile->offset;
	pos += offset;
	bdf_seek(bdf, pos, SEEK_SET);
	return 0;
}

static int default_read_sector(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	if (default_seek_sector(bdf, track, head, sector, offset))
		return -1;
	bdf_read(bdf, buffer, length);
	return 0;
}

static int default_write_sector(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	if (default_seek_sector(bdf, track, head, sector, offset))
		return -1;
	bdf_write(bdf, buffer, length);
	return 0;
}

int bdf_read_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	return bdffile->read_sector(bdf, (const void *) &bdffile->header, track, head, sector, offset, buffer, length);
}

int bdf_write_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	return bdffile->write_sector(bdf, (const void *) &bdffile->header, track, head, sector, offset, buffer, length);
}

int bdf_is_readonly(void *bdf)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;	
	return bdffile->is_readonly;
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
