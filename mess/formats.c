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
};

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

static int try_format_driver(const struct InternalBdFormatDriver *drv, const struct bdf_procs *procs,
	const char *extension, void *file, UINT32 file_size,
	int *success, struct disk_geometry *geometry, int *offset)
{
	void *header = NULL;
	UINT32 header_size;

	/* match the extension; if either the formatdriver or the caller do not
	 * specify the extension, count that as a match
	 */
	if (extension && drv->extension && strcmpi(extension, drv->extension))
		return 0;

	/* if there is a header, try to read it */
	header_size = drv->header_size & ~HEADERSIZE_FLAGS;
	if (drv->header_size & HEADERSIZE_FLAG_MODULO)
		header_size = file_size % header_size;

	if (header_size > 0)
	{
		if (header_size > file_size)
			return 0;

		header = malloc(header_size);
		if (!header)
			return BLOCKDEVICE_ERROR_OUTOFMEMORY;

		/* read the header */
		procs->seekproc(file, 0, SEEK_SET);
		procs->readproc(file, header, header_size);
	}

	/* try to decode the header */
	*offset = header_size;	/* the default offset is the header size */
	if (!drv->header_decode(header, file_size, header_size, geometry, offset))			
		*success = 1;	/* success!!! */

	if (header)
		free(header);
	return 0;
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

	format(&drv);

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

	bytes_to_write = ((int) drv.bytes_per_sector) * geometry->tracks * geometry->heads * geometry->sectors;
	memset(buffer, drv.filler_byte, sizeof(buffer));

	while(bytes_to_write > 0)
	{
		len = (bytes_to_write > sizeof(buffer)) ? sizeof(buffer) : bytes_to_write;
		procs->writeproc(file, buffer, len);
		bytes_to_write -= len;
	}

	if (outbdf)
	{
		formats[0] = format;
		formats[1] = NULL;
		err = bdf_open(procs, formats, file, NULL, outbdf);
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

int bdf_open(const struct bdf_procs *procs, const formatdriver_ctor *formats,
	void *file, const char *extension, void **outbdf)
{
	int err, success;
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

	/* allocate the bdffile */
	bdffile = malloc(sizeof(struct bdf_file));
	if (!bdffile)
	{
		err = BLOCKDEVICE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* gather some initial information about the file */
	filesize = procs->filesizeproc(file);

	/* the first task is to locate an appropriate format driver */
	success = 0;
	while(!success && *formats)
	{
		/* build the format driver */
		(*formats)(&drv);

		err = try_format_driver(&drv, procs, extension, file, filesize, &success,
			&bdffile->geometry, &bdffile->offset);
		if (err)
			goto done;

		if (!success)
			formats++;
	}

	/* did we find an appropriate format driver? */
	if (!(*formats))
	{
		err = BLOCKDEVICE_ERROR_CANTDECODEFORMAT;
		goto done;
	}

	bdffile->file = file;
	bdffile->procs = procs;
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

static int bdf_seek(struct bdf_file *bdffile, UINT8 track, UINT8 head, UINT8 sector, int offset)
{
	int pos;

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
	bdffile->procs->seekproc(bdffile->file, pos, SEEK_SET);
	return 0;
}

void bdf_get_geometry(void *bdf, struct disk_geometry *geometry)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	*geometry = bdffile->geometry;
}

int bdf_read_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	if (bdf_seek(bdffile, track, head, sector, offset))
		return -1;
	bdffile->procs->readproc(bdffile->file, buffer, length);
	return 0;
}

int bdf_write_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;
	if (bdf_seek(bdffile, track, head, sector, offset))
		return -1;
	bdffile->procs->writeproc(bdffile->file, buffer, length);
	return 0;
}

int bdf_is_readonly(void *bdf)
{
	struct bdf_file *bdffile = (struct bdf_file *) bdf;	
	return bdffile->procs->isreadonlyproc(bdffile->file);
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
