#include <string.h>
#include "imgtool.h"

/* ----------------------------------------------------------------------- */

extern struct ImageModule imgmod_rsdos;	/* CoCo RS-DOS disks */

static const struct ImageModule *images[] = {
	&imgmod_rsdos
};

/* ----------------------------------------------------------------------- */

const struct ImageModule *findimagemodule(const char *name)
{
	size_t i;
	for (i = 0; i < (sizeof(images) / sizeof(images[0])); i++)
		if (!stricmp(name, images[i]->name))
			return images[i];
	return NULL;
}

const struct ImageModule **getmodules(size_t *len)
{
	*len = sizeof(images) / sizeof(images[0]);
	return images;
}

const char *imageerror(int err)
{
	static const char *msgs[] = {
		"Out of memory",
		"Unexpected error",
		"Argument too long",
		"Read error",
		"Write error",
		"Image is read only",
		"Corrupt image",
		"File not found",
		"Unrecognized format",
		"Not implemented",
		"Incorrect or missing parameters",
		"Bad file name",
		"Out of space on image"
	};

	return msgs[err-1];
}

int img_open(const struct ImageModule *module, const char *fname, int read_or_write, IMAGE **outimg)
{
	int err;
	STREAM *f;

	*outimg = NULL;

	if (!module->init)
		return IMGTOOLERR_UNIMPLEMENTED;

	f = stream_open(fname, read_or_write);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND;

	err = module->init(f, outimg);
	if (err) {
		stream_close(f);
		return err;
	}
	return 0;
}

int img_open_byname(const char *modulename, const char *fname, int read_or_write, IMAGE **outimg)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND;

	return img_open(module, fname, read_or_write, outimg);
}

void img_close(IMAGE *img)
{
	if (img->module->exit)
		img->module->exit(img);
}

int img_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	if (!img->module->beginenum)
		return IMGTOOLERR_UNIMPLEMENTED;
	return img->module->beginenum(img, outenum);
}

int img_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	return enumeration->module->nextenum(enumeration, ent);
}

void img_closeenum(IMAGEENUM *enumeration)
{
	if (enumeration->module->closeenum)
		enumeration->module->closeenum(enumeration);
}

int img_freespace(IMAGE *img, size_t *sz)
{
	if (!img->module->freespace)
		return IMGTOOLERR_UNIMPLEMENTED;
	*sz = img->module->freespace(img);
	return 0;
}

int img_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	if (!img->module->readfile)
		return IMGTOOLERR_UNIMPLEMENTED;
	return img->module->readfile(img, fname, destf);
}

int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef)
{
	if (!img->module->writefile)
		return IMGTOOLERR_UNIMPLEMENTED;
	return img->module->writefile(img, fname, sourcef);
}

int img_deletefile(IMAGE *img, const char *fname)
{
	if (!img->module->deletefile)
		return IMGTOOLERR_UNIMPLEMENTED;
	return img->module->deletefile(img, fname);
}

int img_create(const struct ImageModule *module, const char *fname)
{
	int err;
	STREAM *f;

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED;

	f = stream_open(fname, OSD_FOPEN_WRITE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND;

	err = module->create(f);
	stream_close(f);
	return err;
}

int img_create_byname(const char *modulename, const char *fname)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND;

	return img_create(module, fname);
}

