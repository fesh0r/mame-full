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
		bytes_to_write = ((int) geometry->bytes_per_sector) * geometry->tracks * geometry->heads * geometry->sectors;
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
	const char *s;
	int result, i;

	*bdffile = NULL;

	/* match the extension; if either the formatdriver or the caller do not
	 * specify the extension, count that as a match
	 */
	if (extension && drv->extensions)
	{
		s = drv->extensions;
		while(*s && strcmpi(s, extension))
			s += strlen(s) + 1;
		if (*s == '\0')
			return 0;
	}

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
	if (drv->header_decode)
	{
		/* we have a header decode function; use it */
		result = drv->header_decode(header_size ? &bdf->header : NULL, file_size, header_size, &bdf->geometry, &bdf->offset);
	}
	else
	{
		/* deduce the geometry from the various options */
		struct geometry_test tests[128];
		UINT32 params[4];
		int test_count = 0;

		for(i = 0; (i < sizeof(drv->geometry_options) / sizeof(drv->geometry_options[0])) && drv->geometry_options[i]; i++)
		{
			test_count += process_geometry_string(drv->geometry_options[i], tests + test_count,
				sizeof(tests) / sizeof(tests[0]) - test_count);
		}

		if (deduce_geometry(tests, test_count, file_size - header_size, params))
		{
			/* success */
			result = 0;
			bdf->geometry.tracks			= (UINT8) params[0];
			bdf->geometry.heads				= (UINT8) params[1];
			bdf->geometry.sectors			= (UINT8) params[2];
			bdf->geometry.bytes_per_sector	= (UINT16) params[3];
		}
		else
		{
			/* failure */
			result = 1;
		}
	}

	if (result)
		free(bdf);		/* failure; free the bdf_file */
	else
		*bdffile = bdf;	/* success! */
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

/***************************************************************************

	Geometry string parsing

***************************************************************************/

static void accumulate_test(struct geometry_test *tests, int test_count, int dimension, int value)
{
	int i, j;
	UINT32 multiple;

	for (i = 0; i < test_count; i++)
	{
		tests[i].min_size *= value;
		tests[i].max_size *= value;
		tests[i].size_step *= value;
		multiple = 1;
		for (j = 0; j < dimension; j++)
		{
			multiple *= tests[i].dimensions[j].cumulative_size;
			tests[i].dimensions[j].size_divisor *= value;
		}
		tests[i].dimensions[dimension].cumulative_size = value;
		tests[i].dimensions[dimension].size_divisor = multiple;
	}
}

int process_geometry_string(const char *geometry_string, struct geometry_test *tests, int max_test_count)
{
	int value = -1;
	int dimension = 0;
	int test_count = 1;
	int incremental_tests = 0;
	char c;

	if (!geometry_string || *geometry_string == '\0')
		return 0;

	if (tests)
	{
		if (max_test_count < 1)
			return 0;
		tests[0].min_size = 1;
		tests[0].max_size = 1;
		tests[0].size_step = 1;
	}

	do
	{
		c = *(geometry_string++);

		switch(c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (value == -1)
				value = 0;
			else
				value *= 10;
			value += c - '0';
			break;

		case '/':
			if (tests)
			{
				if (max_test_count <= test_count * (incremental_tests+1))
					return 0;
				memcpy(tests + test_count * (incremental_tests+1), tests + test_count * (incremental_tests+0), sizeof(tests[0]) * test_count);
				accumulate_test(tests + test_count * incremental_tests, test_count, dimension, value);
			}
			incremental_tests++;
			value = -1;
			break;

		case ',':
		case '\0':
			if (value == -1)
				return 0;
			if (tests)
				accumulate_test(tests + test_count * incremental_tests, test_count, dimension, value);
			dimension++;

			value = -1;
			test_count *= 1 + incremental_tests;
			incremental_tests = 0;
			if (c != '\0' && dimension >= (sizeof(tests[0].dimensions) / sizeof(tests[0].dimensions[0])))
				return 0;
			break;

		case ' ':
			break;

		default:
			return 0;
		}

	}
	while(c);

	/* verify that the string is the correct size */
	if (dimension != (sizeof(tests[0].dimensions) / sizeof(tests[0].dimensions[0])))
		return 0;

	return test_count;
}

int deduce_geometry(const struct geometry_test *tests, int test_count, UINT32 size, UINT32 *params)
{
	int i, j;

	for (i = 0; i < test_count; i++)
	{
		if ((size >= tests[i].min_size) && (size <= tests[i].max_size))
		{
			if (!tests[i].size_step || ((size - tests[i].min_size) % tests[i].size_step == 0))
			{
				for (j = 0; j < sizeof(tests[i].dimensions) / sizeof(tests[i].dimensions[0]); j++)
				{
					params[j] = size / tests[i].dimensions[j].size_divisor;
				}
				return 1;
			}
		}
	}

	for (i = 0; i < sizeof(tests[0].dimensions) / sizeof(tests[0].dimensions[0]); i++)
		params[i] = (UINT32) -1;
	return 0;
}

/*-------------------------------------------------
	validate_construct_formatdriver - consistency
	checks on format drivers
-------------------------------------------------*/

#ifdef MAME_DEBUG
void validate_construct_formatdriver(struct InternalBdFormatDriver *drv, int geometry_optnum)
{
	int i;
	if (geometry_optnum)
	{
		for(i = 0; (i < sizeof(drv->geometry_options) / sizeof(drv->geometry_options[0])) && drv->geometry_options[i]; i++)
			assert(process_geometry_string(drv->geometry_options[i], NULL, 0));
	}
	else
	{
		assert(drv->header_decode);
	}
}
#endif
