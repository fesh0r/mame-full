#include <string.h>
#include <ctype.h>
#include "osdepend.h"
#include "imgtool.h"
#include "osd_cpu.h"
#include "mess/config.h"


/* ----------------------------------------------------------------------- */

CARTMODULE(a5200,    "Atari 5200 Cartridges",			"bin")
CARTMODULE(a7800,    "Atari 7800 Cartridges",			"a78")
CARTMODULE(advision, "AdventureVision Cartridges",		"bin")
CARTMODULE(astrocde, "Astrocade Cartridges",			"bin")
CARTMODULE(c16,      "Commodore 16 Cartridges",			"rom")
CARTMODULE(c64,      "Commodore 64 Cartridges",			"crt")
CARTMODULE(coleco,   "ColecoVision Cartridges",			"rom")
CARTMODULE(gameboy,  "Gameboy Cartridges",				"gb")
CARTMODULE(gamegear, "GameGear Cartridges",				"gg")
CARTMODULE(genesis,  "Sega Genesis Cartridges",			"bin")
CARTMODULE(max,      "Max Cartridges",					"crt")
CARTMODULE(msx,      "MSX Cartridges",					"rom")
CARTMODULE(nes,      "NES Cartridges",					"nes")
CARTMODULE(pdp1,     "PDP-1 Cartridges",				NULL)
CARTMODULE(plus4,    "Commodore +4 Cartridges",			NULL)
CARTMODULE(sms,      "Sega Master System Cartridges",	"sms")
CARTMODULE(ti99_4a,  "TI-99 4A Cartridges",				"bin")
CARTMODULE(vc20,     "Vc20 Cartridges",					NULL)
CARTMODULE(vectrex,  "Vectrex Cartridges",				"bin")
CARTMODULE(vic20,    "Commodore Vic-20 Cartridges",		"a0")

extern struct ImageModule imgmod_rsdos;	/* CoCo RS-DOS disks */
extern struct ImageModule imgmod_pchd;	/* PC HD images */

static const struct ImageModule *images[] = {
	&imgmod_rsdos,
	&imgmod_pchd,
	&imgmod_nes,
	&imgmod_a5200,
	&imgmod_a7800,
	&imgmod_advision,
	&imgmod_astrocde,
	&imgmod_c16,
	&imgmod_c64,
	&imgmod_coleco,
	&imgmod_gameboy,
	&imgmod_gamegear,
	&imgmod_genesis,
	&imgmod_max,
	&imgmod_msx,
	&imgmod_pdp1,
	&imgmod_plus4,
	&imgmod_sms,
	&imgmod_ti99_4a,
	&imgmod_vc20,
	&imgmod_vectrex,
	&imgmod_vic20
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

	return msgs[(err & ~IMGTOOLERR_SRC_MASK) - 1];
}

static int markerrorsource(int err)
{
	switch(err) {
	case IMGTOOLERR_OUTOFMEMORY:
	case IMGTOOLERR_UNEXPECTED:
	case IMGTOOLERR_BUFFERTOOSMALL:
		/* Do nothing */
		break;

	case IMGTOOLERR_FILENOTFOUND:
		err |= IMGTOOLERR_SRC_FILEONIMAGE;
		break;

	default:
		err |= IMGTOOLERR_SRC_IMAGEFILE;
		break;
	}
	return err;
}

int img_open(const struct ImageModule *module, const char *fname, int read_or_write, IMAGE **outimg)
{
	int err;
	STREAM *f;

	*outimg = NULL;

	if (!module->init)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	f = stream_open(fname, read_or_write);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;

	err = module->init(f, outimg);
	if (err) {
		stream_close(f);
		return markerrorsource(err);
	}
	return 0;
}

int img_open_byname(const char *modulename, const char *fname, int read_or_write, IMAGE **outimg)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_open(module, fname, read_or_write, outimg);
}

void img_close(IMAGE *img)
{
	if (img->module->exit)
		img->module->exit(img);
}

int img_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	int err;

	if (!img->module->beginenum)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->beginenum(img, outenum);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	int err;
	err = enumeration->module->nextenum(enumeration, ent);
	if (err)
		return markerrorsource(err);

	return 0;
}

void img_closeenum(IMAGEENUM *enumeration)
{
	if (enumeration->module->closeenum)
		enumeration->module->closeenum(enumeration);
}

int img_freespace(IMAGE *img, size_t *sz)
{
	if (!img->module->freespace)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	*sz = img->module->freespace(img);
	return 0;
}

int img_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	int err;

	if (!img->module->readfile)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->readfile(img, fname, destf);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef)
{
	int err;

	if (!img->module->writefile)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->writefile(img, fname, sourcef);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_getfile(IMAGE *img, const char *fname, const char *dest)
{
	int err;
	STREAM *f;

	if (!dest)
		dest = fname;

	f = stream_open(dest, OSD_FOPEN_WRITE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_readfile(img, fname, f);
	stream_close(f);
	return err;
}

int img_putfile(IMAGE *img, const char *fname, const char *source)
{
	int err;
	STREAM *f;

	if (!source)
		source = fname;

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile(img, fname, f);
	stream_close(f);
	return err;
}

int img_deletefile(IMAGE *img, const char *fname)
{
	int err;

	if (!img->module->deletefile)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->deletefile(img, fname);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_create(const struct ImageModule *module, const char *fname, const geometry_options *options)
{
	int err;
	STREAM *f;
	const geometry_ranges *ranges;
	static const geometry_options emptyopts = { 0, 0 };
	static const geometry_ranges emptyrange = { 0, {0,0}, {0,0} };

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	if (!options)
		options = &emptyopts;
	ranges = module->ranges ? module->ranges : &emptyrange;

	if (ranges->flags & IMAGE_USES_CYLINDERS) {
		if (!options->cylinders)
			return IMGTOOLERR_PARAMNEEDED | IMGTOOLERR_SRC_PARAM_CYLINDERS;
		if (options->cylinders < ranges->minimum.cylinders)
			return IMGTOOLERR_PARAMTOOSMALL | IMGTOOLERR_SRC_PARAM_CYLINDERS;
		if (options->cylinders > ranges->maximum.cylinders)
			return IMGTOOLERR_PARAMTOOLARGE | IMGTOOLERR_SRC_PARAM_CYLINDERS;
	}
	else {
		if (options->cylinders)
			return IMGTOOLERR_PARAMNOTNEEDED | IMGTOOLERR_SRC_PARAM_CYLINDERS;
	}

	if (ranges->flags & IMAGE_USES_HEADS) {
		if (!options->heads)
			return IMGTOOLERR_PARAMNEEDED | IMGTOOLERR_SRC_PARAM_HEADS;
		if (options->heads < ranges->minimum.heads)
			return IMGTOOLERR_PARAMTOOSMALL | IMGTOOLERR_SRC_PARAM_HEADS;
		if (options->heads > ranges->maximum.heads)
			return IMGTOOLERR_PARAMTOOLARGE | IMGTOOLERR_SRC_PARAM_HEADS;
	}

	f = stream_open(fname, OSD_FOPEN_WRITE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = module->create(f, options);
	stream_close(f);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_create_byname(const char *modulename, const char *fname, const geometry_options *options)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_create(module, fname, options);
}

static char *nextentry(char **s)
{
	char *result;
	char *p;
	char *beginspace;
	int parendeep;

	p = *s;

	if (*p) {
		beginspace = NULL;
		result = p;
		parendeep = 0;

		while(*p && ((*p != '|') || parendeep)) {
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
		if (*p) {
			if (!beginspace)
				beginspace = p;
			p++;
		}
		*s = p;
		if (beginspace)
			*beginspace = '\0';
	}
	else {
		result = NULL;
	}
	return result;
}

int img_getinfo(const struct ImageModule *module, const char *fname, imageinfo *info)
{
	int err;
	void *config;
	const char *year;
	char *s;
	char fnamebuf[32];

	info->longname = NULL;
	info->manufacturer = NULL;
	info->year = 0;
	info->playable = NULL;
	info->extrainfo = NULL;

	err = file_crc(fname, &info->crc);
	if (err)
		return markerrorsource(err);

	if (!module || !module->crcfile)
		return 0;

	sprintf(fnamebuf, "crc/%s", module->crcfile);
	config = config_open(fnamebuf);
	if (!config)
		return 0;

	sprintf(fnamebuf, "%08x", (int)info->crc);
	config_load_string(config, module->crcsysname, 0, fnamebuf, info->buffer, sizeof(info->buffer));
	if (info->buffer[0]) {
		s = info->buffer;
		info->longname = nextentry(&s);
		info->manufacturer = nextentry(&s);
		year = nextentry(&s);
		info->year = year ? atoi(year) : 0;
		info->playable = nextentry(&s);
		info->extrainfo = nextentry(&s);
	}
	config_close(config);

	return 0;
}

int img_getinfo_byname(const char *modulename, const char *fname, imageinfo *info)
{
	const struct ImageModule *module;

	if (modulename) {
		module = findimagemodule(modulename);
		if (!module)
			return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;
	}
	else {
		module = NULL;
	}

	return img_getinfo(module, fname, info);
}

int img_goodname(const struct ImageModule *module, const char *fname, const char *base, char **result)
{
	int err;
	char *s;
	char *source;
	char *dest;
	const char *ext;
	imageinfo info;

	err = img_getinfo(module, fname, &info);
	if (err)
		goto error;

	if (!info.longname) {
		*result = NULL;
		return 0;
	}

	ext = module->fileextension;
	s = malloc((base ? strlen(base)+1 : 0) + strlen(info.longname) + (ext ? strlen(ext)+1 : 0) + 1);
	if (!s) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	if (base) {
		strcpy(s, base);
		dest = s + strlen(s);
	}
	else {
		dest = s;
	}

	/* Copy the string to the destination */
	source = info.longname;
	while(*source) {
		if (isalnum(*source) || strchr("-(),!", *source))
			*(dest++) = *source;
		source++;
	}

	if (ext) {
		*(dest++) = '.';
		strcpy(dest, ext);
		dest += strlen(dest);
	}
	*dest = '\0';
	*result = s;
	return 0;

error:
	*result = NULL;
	return err;
}

int img_goodname_byname(const char *modulename, const char *fname, const char *base, char **result)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module) {
		*result = NULL;
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;
	}

	return img_goodname(module, fname, base, result);
}



