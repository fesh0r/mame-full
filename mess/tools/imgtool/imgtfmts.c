#include <ctype.h>
#include "imgtoolx.h"
#include "snprintf.h"
#include "formats.h"

enum
{
	OPT_TRACKS,
	OPT_HEADS,
	OPT_SECTORS
};

static int bdf_error(int err)
{
	switch(err) {
	case BLOCKDEVICE_ERROR_OUTOFMEMORY:
		return IMGTOOLERR_OUTOFMEMORY;

	case BLOCKDEVICE_ERROR_CANTDECODEFORMAT:
		return IMGTOOLERR_CORRUPTIMAGE;
	
	case BLOCKDEVICE_ERROR_CANTENCODEFORMAT:
		return IMGTOOLERR_READONLY;
		
	default:
		return IMGTOOLERR_UNEXPECTED;
	}
}

static void imgtool_bdf_closeproc(void *file)
{
	stream_close((STREAM *) file);
}

static int imgtool_bdf_seekproc(void *file, INT64 offset, int whence)
{
	stream_seek((STREAM *) file, offset, whence);
	return 0;
}

static UINT32 imgtool_bdf_readproc(void *file, void *buffer, UINT32 length)
{
	return stream_read((STREAM *) file, buffer, length);
}

static UINT32 imgtool_bdf_writeproc(void *file, const void *buffer, UINT32 length)
{
	stream_write((STREAM *) file, buffer, length);
	return length;
}

static UINT64 imgtool_bdf_filesizeproc(void *file)
{
	return stream_size((STREAM *) file);
}

static struct bdf_procs imgtool_bdf_procs =
{
	imgtool_bdf_closeproc,
	imgtool_bdf_seekproc,
	imgtool_bdf_readproc,
	imgtool_bdf_writeproc,
	imgtool_bdf_filesizeproc
};

struct bdf_image
{
	IMAGE base;
	void *bdf;
};

static void *get_bdf(IMAGE *img)
{
	struct bdf_image *bimg = (struct bdf_image *) img;
	return bimg->bdf;
}

int imgtool_bdf_open(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	int err;
	struct bdf_image *bimg;
	formatdriver_ctor formats[2];

	bimg = (struct bdf_image *) malloc(sizeof(struct bdf_image));
	if (!bimg)
	{
		err = BLOCKDEVICE_ERROR_OUTOFMEMORY;
		goto error;
	}
	bimg->base.module = mod;

	formats[0] = (formatdriver_ctor) mod->extra;
	formats[1] = NULL;

	err = bdf_open(&imgtool_bdf_procs, formats, (void *) f, stream_isreadonly(f), NULL, &bimg->bdf);
	if (err)
	{
		err = bdf_error(err);
		goto error;
	}

	*outimg = &bimg->base;
	return 0;

error:
	if (bimg)
		free(bimg);
	return err;
}

void imgtool_bdf_close(IMAGE *img)
{
	bdf_close(get_bdf(img));
	free(img);
}

int imgtool_bdf_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *createoptions)
{
	int err;
	struct disk_geometry geometry;
	formatdriver_ctor format;

	memset(&geometry, 0, sizeof(geometry));
	geometry.tracks = createoptions[OPT_TRACKS].i;
	geometry.heads = createoptions[OPT_HEADS].i;
	geometry.sectors = createoptions[OPT_SECTORS].i;
	
	/* HACK HACK - the imgtool driver (not the formatdriver) should specify these! */
	geometry.first_sector_id = 1;
	geometry.sector_size = 256;

	format = (formatdriver_ctor) mod->extra;

	err = bdf_create(&imgtool_bdf_procs, format, (void *) f, &geometry, NULL);
	if (err)
		return bdf_error(err);

	return 0;
}

const struct disk_geometry *imgtool_bdf_get_geometry(IMAGE *img)
{
	return bdf_get_geometry(get_bdf(img));
}

int imgtool_bdf_read_sector(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int size)
{
	bdf_read_sector(get_bdf(img), track, head, sector, offset, buffer, size);
	return 0;
}

int imgtool_bdf_write_sector(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int size)
{
	bdf_write_sector(get_bdf(img), track, head, sector, offset, buffer, size);
	return 0;
}

int imgtool_bdf_read_sector_to_stream(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, int length, STREAM *s)
{
	void *buffer;
	int err = 0;

	buffer = malloc(length);
	if (!buffer)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	imgtool_bdf_read_sector(img, track, head, sector, offset, buffer, length);
	stream_write(s, buffer, length);

done:
	if (buffer)
		free(buffer);
	return err;
}

int imgtool_bdf_write_sector_from_stream(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, int length, STREAM *s)
{
	void *buffer;
	int err = 0;

	buffer = malloc(length);
	if (!buffer)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	stream_read(s, buffer, length);
	imgtool_bdf_write_sector(img, track, head, sector, offset, buffer, length);

done:
	if (buffer)
		free(buffer);
	return err;
}

int imgtool_bdf_is_readonly(IMAGE *img)
{
	return bdf_is_readonly(get_bdf(img));
}

static void process_options(const char *geo, size_t *i, int *minimum, int *maximum)
{
	int value;
	char c;

	do
	{
		while(geo[*i] == ' ')
			(*i)++;
		sscanf(geo + *i, "%i", &value);
		while((geo[*i] == ' ') || isdigit(geo[*i]))
			(*i)++;
		c = geo[(*i)++];

		if ((*minimum < 0) || (*minimum > value))
			*minimum = value;
		if ((*maximum < 0) || (*maximum < value))
			*maximum = value;
	}
	while(c == '/');
}

int imgtool_bdf_formatdrvctor(struct ImageModuleCtorParams *params, const formatdriver_ctor *formats)
{
	int format_count;
	struct ImageModule *imgmod = params->imgmod;
	struct OptionTemplate *createoptions;
	size_t max_opts;
	size_t i, j;
	struct InternalBdFormatDriver drv;
	int tracks_min = -1, tracks_max = 0;
	int heads_min = -1, heads_max = 0;
	int sectors_min = -1, sectors_max = 0;
	char buf[256];

	/* count the number of possible formats */
	for(format_count = 0; formats[format_count]; format_count++)
		;

	/* specify the format */
	assert(params->index < format_count);
	imgmod->extra = (void *) formats[params->index];

	/* get the format driver info */
	formats[params->index](&drv);

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s_%s", imgmod->name, drv.name);
	imgmod->name = strdup(buf);
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s (%s)", imgmod->humanname, drv.humanname);
	imgmod->humanname = strdup(buf);

	memset(imgmod->createoptions_template, 0, sizeof(imgmod->createoptions_template));
	createoptions = imgmod->createoptions_template;
	max_opts = sizeof(imgmod->createoptions_template) / sizeof(imgmod->createoptions_template[0]);

	for (i = 0; (i < sizeof(drv.geometry_options) / sizeof(drv.geometry_options[0])) && drv.geometry_options[i]; i++)
	{
		j = 0;
		process_options(drv.geometry_options[i], &j, &tracks_min, &tracks_max);
		process_options(drv.geometry_options[i], &j, &heads_min, &heads_max);
		process_options(drv.geometry_options[i], &j, &sectors_min, &sectors_max);
	}

	createoptions[OPT_TRACKS].name = "tracks";
	createoptions[OPT_TRACKS].description = "Tracks";
	createoptions[OPT_TRACKS].flags = IMGOPTION_FLAG_TYPE_INTEGER;
	createoptions[OPT_TRACKS].min = tracks_min;
	createoptions[OPT_TRACKS].max = tracks_max;
	createoptions[OPT_TRACKS].defaultvalue = NULL;

	createoptions[OPT_HEADS].name = "heads";
	createoptions[OPT_HEADS].description = "Heads";
	createoptions[OPT_HEADS].flags = IMGOPTION_FLAG_TYPE_INTEGER;
	createoptions[OPT_HEADS].min = heads_min;
	createoptions[OPT_HEADS].max = heads_max;
	createoptions[OPT_HEADS].defaultvalue = NULL;

	createoptions[OPT_SECTORS].name = "sectors";
	createoptions[OPT_SECTORS].description = "Sectors";
	createoptions[OPT_SECTORS].flags = IMGOPTION_FLAG_TYPE_INTEGER;
	createoptions[OPT_SECTORS].min = sectors_min;
	createoptions[OPT_SECTORS].max = sectors_max;
	createoptions[OPT_SECTORS].defaultvalue = NULL;

	return format_count;
}
