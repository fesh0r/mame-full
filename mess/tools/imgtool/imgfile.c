#include <ctype.h>
#include "imgtool.h"
#include "opresolv.h"

struct _imgtool_image
{
	const struct ImageModule *module;
};

struct _imgtool_imageenum
{
	imgtool_image *image;
};



static imgtoolerr_t markerrorsource(imgtoolerr_t err)
{
	assert(imgtool_error != NULL);

	switch(err)
	{
		case IMGTOOLERR_OUTOFMEMORY:
		case IMGTOOLERR_UNEXPECTED:
		case IMGTOOLERR_BUFFERTOOSMALL:
			/* Do nothing */
			break;

		case IMGTOOLERR_FILENOTFOUND:
		case IMGTOOLERR_BADFILENAME:
			err |= IMGTOOLERR_SRC_FILEONIMAGE;
			break;

		default:
			err |= IMGTOOLERR_SRC_IMAGEFILE;
			break;
	}
	return err;
}



static imgtoolerr_t internal_open(const struct ImageModule *module, const char *fname,
	int read_or_write, option_resolution *createopts, imgtool_image **outimg)
{
	imgtoolerr_t err;
	imgtool_stream *f = NULL;
	imgtool_image *image = NULL;
	size_t size;

	if (outimg)
		*outimg = NULL;

	if (createopts ? !module->create : !module->open)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	f = stream_open(fname, read_or_write);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;
		goto done;
	}

	size = sizeof(struct _imgtool_image) + module->image_extra_bytes;
	image = (imgtool_image *) malloc(size);
	if (!image)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(image, '\0', size);
	image->module = module;
	
	if (createopts)
		err = module->create(image, f, createopts);
	else
		err = module->open(image, f);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (err)
	{
		if (f)
			stream_close(f);
		if (image)
		{
			free(image);
			image = NULL;
		}
	}

	if (outimg)
		*outimg = image;
	else if (image)
		img_close(image);
	return err;
}



imgtoolerr_t img_open(const struct ImageModule *module, const char *fname, int read_or_write, imgtool_image **outimg)
{
	return internal_open(module, fname, read_or_write, NULL, outimg);
}



imgtoolerr_t img_open_byname(imgtool_library *library, const char *modulename, const char *fname, int read_or_write, imgtool_image **outimg)
{
	const struct ImageModule *module;

	module = imgtool_library_findmodule(library, modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_open(module, fname, read_or_write, outimg);
}



void img_close(imgtool_image *img)
{
	if (img->module->close)
		img->module->close(img);
	free(img);
}



imgtoolerr_t img_info(imgtool_image *img, char *string, size_t len)
{
	if (len > 0)
	{
		if (img->module->info)
			img->module->info(img, string, len);
		else
			string[0] = '\0';
	}
	return 0;
}



static imgtoolerr_t cannonicalize_path(imgtool_image *image, int mandate_dir_path,
	const char **path, char **alloc_path)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char *new_path = NULL;
	char path_separator, alt_path_separator;
	const char *s;
	int in_path_separator, i, j;
	
	path_separator = image->module->path_separator;
	alt_path_separator = image->module->alternate_path_separator;

	if (path_separator == '\0')
	{
		/* do we specify a path when paths are not supported? */
		if (mandate_dir_path && *path && **path)
		{
			err = IMGTOOLERR_CANNOTUSEPATH | IMGTOOLERR_SRC_FUNCTIONALITY;
			goto done;
		}
		*path = NULL;
	}
	else
	{
		s = *path ? *path : "";

		/* allocate space for a new cannonical path */
		new_path = malloc(strlen(s) + 4);
		if (!new_path)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}

		/* copy the path */
		in_path_separator = TRUE;
		i = j = 0;
		do
		{
			if ((s[i] != '\0') && (s[i] != path_separator) && (s[i] != alt_path_separator))
			{
				new_path[j++] = s[i];
				in_path_separator = FALSE;
			}
			else if (!in_path_separator)
			{
				new_path[j++] = '\0';
				in_path_separator = TRUE;
			}
		}
		while(s[i++] != '\0');
		new_path[j++] = '\0';
		new_path[j++] = '\0';
		*path = new_path;
	}

done:
	*alloc_path = new_path;
	return err;
}



imgtoolerr_t img_beginenum(imgtool_image *img, const char *path, imgtool_imageenum **outenum)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_imageenum *enumeration = NULL;
	char *alloc_path = NULL;
	size_t size;

	assert(img);
	assert(outenum);

	*outenum = NULL;

	if (!img->module->next_enum)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	err = cannonicalize_path(img, TRUE, &path, &alloc_path);
	if (err)
		goto done;

	size = sizeof(struct _imgtool_imageenum) + img_module(img)->imageenum_extra_bytes;
	enumeration = (imgtool_imageenum *) malloc(size);
	if (!enumeration)
		goto done;
	memset(enumeration, '\0', size);
	enumeration->image = img;

	if (img->module->begin_enum)
	{
		err = img->module->begin_enum(enumeration, path);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (alloc_path)
		free(alloc_path);
	if (err && enumeration)
	{
		free(enumeration);
		enumeration = NULL;
	}
	*outenum = enumeration;
	return err;
}



imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	const struct ImageModule *module;

	module = img_enum_module(enumeration);

	/* This makes it so that drivers don't have to take care of clearing
	 * the attributes if they don't apply
	 */
	ent->filename[0] = '\0';
	ent->attr[0] = '\0';
	ent->creation_time = 0;
	ent->lastmodified_time = 0;
	ent->eof = 0;
	ent->corrupt = 0;
	ent->filesize = 0;

	err = module->next_enum(enumeration, ent);
	if (err)
		return markerrorsource(err);

	/* don't trust the module! */
	if (!module->supports_creation_time && (ent->creation_time != 0))
		return IMGTOOLERR_UNEXPECTED;
	if (!module->supports_lastmodified_time && (ent->lastmodified_time != 0))
		return IMGTOOLERR_UNEXPECTED;

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_getdirent(imgtool_image *img, const char *path, int index, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	imgtool_imageenum *imgenum = NULL;

	if (index < 0)
	{
		err = IMGTOOLERR_PARAMTOOSMALL;
		goto done;
	}

	err = img_beginenum(img, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = img_nextenum(imgenum, ent);
		if (err)
			goto done;

		if (ent->eof)
		{
			err = IMGTOOLERR_FILENOTFOUND;
			goto done;
		}
	}
	while(index--);

done:
	if (err)
		memset(ent->filename, 0, sizeof(ent->filename));
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



void img_closeenum(imgtool_imageenum *enumeration)
{
	const struct ImageModule *module;
	module = img_enum_module(enumeration);
	if (module->close_enum)
		module->close_enum(enumeration);
	free(enumeration);
}



imgtoolerr_t img_countfiles(imgtool_image *img, int *totalfiles)
{
	int err;
	imgtool_imageenum *imgenum;
	imgtool_dirent ent;

	*totalfiles = 0;
	memset(&ent, 0, sizeof(ent));

	err = img_beginenum(img, NULL, &imgenum);
	if (err)
		goto done;

	do {
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (ent.filename[0])
			(*totalfiles)++;
	}
	while(ent.filename[0]);

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



imgtoolerr_t img_filesize(imgtool_image *img, const char *fname, UINT64 *filesize)
{
	int err;
	imgtool_imageenum *imgenum;
	imgtool_dirent ent;
	const char *path;

	path = NULL;	/* TODO: Need to parse off the path */

	*filesize = -1;
	memset(&ent, 0, sizeof(ent));

	err = img_beginenum(img, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (!strcmpi(fname, ent.filename))
		{
			*filesize = ent.filesize;
			goto done;
		}
	}
	while(ent.filename[0]);

	err = IMGTOOLERR_FILENOTFOUND;

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



imgtoolerr_t img_freespace(imgtool_image *img, UINT64 *sz)
{
	imgtoolerr_t err;
	UINT64 size;

	if (!img->module->free_space)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->free_space(img, &size);
	if (err)
		return err | IMGTOOLERR_SRC_IMAGEFILE;

	if (sz)
		*sz = size;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t process_filter(imgtool_stream **mainstream, imgtool_stream **newstream, const struct ImageModule *imgmod, FILTERMODULE filter, int purpose)
{
	imgtool_filter *f;

	if (filter)
	{
		f = filter_init(filter, imgmod, purpose);
		if (!f)
			return IMGTOOLERR_OUTOFMEMORY;

		*newstream = stream_open_filter(*mainstream, f);
		if (!(*newstream))
			return IMGTOOLERR_OUTOFMEMORY;

		*mainstream = *newstream;
	}
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf, FILTERMODULE filter)
{
	imgtoolerr_t err;
	imgtool_stream *newstream = NULL;
	char *alloc_path = NULL;

	if (!img->module->read_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* custom filter? */
	err = process_filter(&destf, &newstream, img->module, filter, PURPOSE_READ);
	if (err)
		goto done;

	/* cannonicalize path */
	if (img->module->path_separator)
	{
		err = cannonicalize_path(img, FALSE, &fname, &alloc_path);
		if (err)
			goto done;
	}

	err = img->module->read_file(img, fname, destf);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (newstream)
		stream_close(newstream);
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, option_resolution *opts, FILTERMODULE filter)
{
	imgtoolerr_t err;
	char *buf = NULL;
	char *s;
	imgtool_stream *newstream = NULL;
	option_resolution *alloc_resolution = NULL;
	char *alloc_path = NULL;
	UINT64 free_space;
	UINT64 file_size;

	if (!img->module->write_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Does this image module prefer upper case file names? */
	if (img->module->prefer_ucase)
	{
		buf = malloc(strlen(fname) + 1);
		if (!buf)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		strcpy(buf, fname);
		for (s = buf; *s; s++)
			*s = toupper(*s);
		fname = buf;
	}

	/* custom filter? */
	err = process_filter(&sourcef, &newstream, img->module, filter, PURPOSE_WRITE);
	if (err)
		goto done;

	/* cannonicalize path */
	if (img->module->path_separator)
	{
		err = cannonicalize_path(img, FALSE, &fname, &alloc_path);
		if (err)
			goto done;
	}

	/* allocate dummy options if necessary */
	if (!opts && img->module->writefile_optguide)
	{
		alloc_resolution = option_resolution_create(img->module->writefile_optguide, img->module->writefile_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	if (opts)
		option_resolution_finish(opts);

	/* if free_space is implemented; do a quick check to see if space is available */
	if (img->module->free_space)
	{
		err = img->module->free_space(img, &free_space);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}

		file_size = stream_size(sourcef);

		if (file_size > free_space)
		{
			err = markerrorsource(IMGTOOLERR_NOSPACE);
			goto done;
		}
	}

	/* actually invoke the write file handler */
	err = img->module->write_file(img, fname, sourcef, opts);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (buf)
		free(buf);
	if (alloc_path)
		free(alloc_path);
	if (newstream)
		stream_close(newstream);
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



imgtoolerr_t img_getfile(imgtool_image *img, const char *fname, const char *dest, FILTERMODULE filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	if (!dest)
		dest = fname;

	f = stream_open(dest, OSD_FOPEN_WRITE);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;
		goto done;
	}

	err = img_readfile(img, fname, f, filter);
	if (err)
		goto done;

done:
	if (f)
		stream_close(f);
	return err;
}



imgtoolerr_t img_putfile(imgtool_image *img, const char *newfname, const char *source, option_resolution *opts, FILTERMODULE filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	if (!newfname)
		newfname = (const char *) osd_basename((char *) source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile(img, newfname, f, opts, filter);
	stream_close(f);
	return err;
}



imgtoolerr_t img_deletefile(imgtool_image *img, const char *fname)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	if (!img->module->delete_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	if (img->module->path_separator)
	{
		err = cannonicalize_path(img, FALSE, &fname, &alloc_path);
		if (err)
			goto done;
	}

	err = img->module->delete_file(img, fname);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_createdir(imgtool_image *img, const char *path)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	/* implemented? */
	if (!img->module->create_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(img, TRUE, &path, &alloc_path);
	if (err)
		goto done;

	err = img->module->create_dir(img, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_deletedir(imgtool_image *img, const char *path)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	/* implemented? */
	if (!img->module->delete_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(img, TRUE, &path, &alloc_path);
	if (err)
		goto done;

	err = img->module->delete_dir(img, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_create(const struct ImageModule *module, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	imgtoolerr_t err;
	option_resolution *alloc_resolution = NULL;

	/* allocate dummy options if necessary */
	if (!opts)
	{
		alloc_resolution = option_resolution_create(module->createimage_optguide, module->createimage_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	option_resolution_finish(opts);

	err = internal_open(module, fname, OSD_FOPEN_RW_CREATE, opts, image);
	if (err)
		goto done;

done:
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	const struct ImageModule *module;

	module = imgtool_library_findmodule(library, modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_create(module, fname, opts, image);
}



const struct ImageModule *img_module(imgtool_image *img)
{
	return img->module;
}



void *img_extrabytes(imgtool_image *img)
{
	assert(img->module->image_extra_bytes > 0);
	return ((UINT8 *) img) + sizeof(*img);
}



const struct ImageModule *img_enum_module(imgtool_imageenum *enumeration)
{
	return enumeration->image->module;
}



void *img_enum_extrabytes(imgtool_imageenum *enumeration)
{
	assert(enumeration->image->module->imageenum_extra_bytes > 0);
	return ((UINT8 *) enumeration) + sizeof(*enumeration);
}



imgtool_image *img_enum_image(imgtool_imageenum *enumeration)
{
	return enumeration->image;
}

