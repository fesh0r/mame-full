#include <ctype.h>
#include "imgtool.h"
#include "opresolv.h"

struct _imgtool_imagefile
{
	const struct ImageModule *module;
};

struct _imgtool_imageenum
{
	const struct ImageModule *module;
};



static imgtoolerr_t markerrorsource(imgtoolerr_t err)
{
	switch(err) {
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



imgtoolerr_t img_identify(imgtool_library *library, const char *fname,
	ImageModuleConstPtr *modules, size_t count)
{
	imgtoolerr_t err;
	const struct ImageModule *module = NULL;
	imgtool_image *image;
	size_t i = 0;
	const char *extension;
	int null_terminate;

	if (count <= 0)
		return IMGTOOLERR_UNEXPECTED;
	null_terminate = (count > 1);
	if (null_terminate)
		count--;

	extension = strrchr(fname, '.');
	if (extension)
		extension++;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		if (!extension || findextension(module->extensions, extension))
		{
			image = NULL;
			err = img_open(module, fname, OSD_FOPEN_READ, &image);
			if (err && (ERRORCODE(err) != IMGTOOLERR_CORRUPTIMAGE))
				return err;

			if (image)
			{
				if (i < count)
					modules[i++] = module;
				img_close(image);
			}
		}
	}

	if (i <= 0)
		return IMGTOOLERR_MODULENOTFOUND;
	if (null_terminate)
		modules[i] = NULL;
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_open(const struct ImageModule *module, const char *fname, int read_or_write, imgtool_image **outimg)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	*outimg = NULL;

	if (!module->open)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	f = stream_open(fname, read_or_write);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;
	
	err = module->open(module, f, outimg);
	if (err)
	{
		stream_close(f);
		return markerrorsource(err);
	}
	return 0;
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



imgtoolerr_t img_beginenum(imgtool_image *img, const char *path, imgtool_imageenum **outenum)
{
	imgtoolerr_t err;
	char buf[2];

	assert(img);
	assert(outenum);

	*outenum = NULL;

	if (!img->module->begin_enum)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	if (!img->module->path_separator)
	{
		/* do we specify a path when paths are not supported? */
		if (path)
			return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
	}
	else if (!path)
	{
		/* must set up default path */
		if (img->module->initial_path_separator)
		{
			buf[0] = img->module->path_separator;
			buf[1] = '\0';
			path = buf;
		}
		else
		{
			path = "";
		}
	}

	err = img->module->begin_enum(img, path, outenum);
	if (err)
		return markerrorsource(err);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;

	/* This makes it so that drivers don't have to take care of clearing
	 * the attributes if they don't apply
	 */
	if (ent->attr_len)
		ent->attr[0] = '\0';

	err = enumeration->module->next_enum(enumeration, ent);
	if (err)
		return markerrorsource(err);

	return IMGTOOLERR_SUCCESS;
}



void img_closeenum(imgtool_imageenum *enumeration)
{
	if (enumeration->module->close_enum)
		enumeration->module->close_enum(enumeration);
}



imgtoolerr_t img_countfiles(imgtool_image *img, int *totalfiles)
{
	int err;
	imgtool_imageenum *imgenum;
	imgtool_dirent ent;
	char fnamebuf[256];

	*totalfiles = 0;
	memset(&ent, 0, sizeof(ent));
	ent.filename = fnamebuf;
	ent.filename_len = sizeof(fnamebuf) / sizeof(fnamebuf[0]);

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
	char fnamebuf[256];
	const char *path;

	path = NULL;	/* TODO: Need to parse off the path */

	*filesize = -1;
	memset(&ent, 0, sizeof(ent));
	ent.filename = fnamebuf;
	ent.filename_len = sizeof(fnamebuf) / sizeof(fnamebuf[0]);

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

	if (!img->module->read_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Custom filter? */
	err = process_filter(&destf, &newstream, img->module, filter, PURPOSE_READ);
	if (err)
		goto done;

	err = img->module->read_file(img, fname, destf);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (newstream)
		stream_close(newstream);
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, option_resolution *opts, FILTERMODULE filter)
{
	imgtoolerr_t err;
	char *buf = NULL;
	char *s;
	imgtool_stream *newstream = NULL;
	option_resolution *alloc_resolution = NULL;

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

	/* allocate dummy options if necessary */
	if (!opts)
	{
		alloc_resolution = option_resolution_create(img->module->writefile_optguide, img->module->writefile_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	option_resolution_finish(opts);

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

	if (!img->module->delete_file)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->delete_file(img, fname);
	if (err)
		return markerrorsource(err);

	return IMGTOOLERR_SUCCESS;
}

imgtoolerr_t img_create(const struct ImageModule *module, const char *fname, option_resolution *opts)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	/* The mess_hd imgtool module needs to read back the file it creates */
	f = stream_open(fname, OSD_FOPEN_RW_CREATE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = module->create(module, f, opts);
	stream_close(f);
	if (err)
		return markerrorsource(err);

	return IMGTOOLERR_SUCCESS;
}

imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname, option_resolution *opts)
{
	const struct ImageModule *module;

	module = imgtool_library_findmodule(library, modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_create(module, fname, opts);
}

static char *nextentry(char **s)
{
	char *result;
	char *p;
	char *beginspace;
	int parendeep;

	p = *s;

	if (*p)
	{
		beginspace = NULL;
		result = p;
		parendeep = 0;

		while(*p && ((*p != '|') || parendeep))
		{
			switch(*p) {
			case '(':
				parendeep++;
				beginspace = NULL;
				break;

			case ')':
				parendeep--;
				beginspace = NULL;
				break;

			case ' ':
				if (!beginspace)
					beginspace = p;
				break;

			default:
				beginspace = NULL;
				break;
			}
			p++;
		}
		if (*p)
		{
			if (!beginspace)
				beginspace = p;
			p++;
		}
		*s = p;
		if (beginspace)
			*beginspace = '\0';
	}
	else
	{
		result = NULL;
	}
	return result;
}
