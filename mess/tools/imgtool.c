#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtool.h"
#include "osd_cpu.h"
#include "config.h"
#include "utils.h"

/* Arbitrary */
#define MAX_OPTIONS	32

/* ----------------------------------------------------------------------- */

CARTMODULE(a5200,    "Atari 5200 Cartridge",			"bin")
CARTMODULE(a7800,    "Atari 7800 Cartridge",			"a78")
CARTMODULE(advision, "AdventureVision Cartridge",		"bin")
CARTMODULE(astrocde, "Astrocade Cartridge",				"bin")
CARTMODULE(c16,      "Commodore 16 Cartridge",			"rom")
CARTMODULE(coleco,   "ColecoVision Cartridge",			"rom")
CARTMODULE(gameboy,  "Gameboy Cartridge",				"gb")
CARTMODULE(gamegear, "GameGear Cartridge",				"gg")
CARTMODULE(genesis,  "Sega Genesis Cartridge",			"bin")
CARTMODULE(max,      "Max Cartridge",					"crt")
CARTMODULE(msx,      "MSX Cartridge",					"rom")
CARTMODULE(nes,      "NES Cartridge",					"nes")
CARTMODULE(pdp1,     "PDP-1 Cartridge",					NULL)
CARTMODULE(plus4,    "Commodore +4 Cartridge",			NULL)
CARTMODULE(sms,      "Sega Master System Cartridge",	"sms")
CARTMODULE(ti99_4a,  "TI-99 4A Cartridge",				"bin")
CARTMODULE(vc20,     "Vc20 Cartridge",					NULL)
CARTMODULE(vectrex,  "Vectrex Cartridge",				"bin")
CARTMODULE(vic20,    "Commodore Vic-20 Cartridge",		"a0")

extern struct ImageModule imgmod_rsdos;	/* CoCo RS-DOS disks */
extern struct ImageModule imgmod_cococas;	/* CoCo cassettes */
extern struct ImageModule imgmod_msdos;	/* FAT/MSDOS diskett images */
extern struct ImageModule imgmod_msdoshd;	/* FAT/MSDOS harddisk images */
extern struct ImageModule imgmod_lynx;	/* c64 archiv */
extern struct ImageModule imgmod_t64;	/* c64 archiv */
extern struct ImageModule imgmod_d64;	/* commodore sx64/vc1541/2031/1551 disketts */
extern struct ImageModule imgmod_x64;	/* commodore vc1541 disketts */
extern struct ImageModule imgmod_d71;	/* commodore 128d/1571 disketts */
extern struct ImageModule imgmod_d81;	/* commodore 65/1565/1581 disketts */
extern struct ImageModule imgmod_c64crt;	/* c64 cartridge */
extern struct ImageModule imgmod_vmsx_tap;	/* vMSX .tap archiv */
extern struct ImageModule imgmod_vmsx_gm2;	/* vMSX gmaster2.ram file */
extern struct ImageModule imgmod_fmsx_cas;	/* fMSX style .cas file */
extern struct ImageModule imgmod_msx_dsk;	/* bogus MSX images */
extern struct ImageModule imgmod_rom16;

static const struct ImageModule *images[] = {
	&imgmod_rsdos,
	&imgmod_cococas,
	&imgmod_msdos,
	&imgmod_msdoshd,
	&imgmod_nes,
	&imgmod_a5200,
	&imgmod_a7800,
	&imgmod_advision,
	&imgmod_astrocde,
	&imgmod_c16,
	&imgmod_c64crt,
	&imgmod_t64,
	&imgmod_lynx,
	&imgmod_d64,
	&imgmod_x64,
	&imgmod_d71,
	&imgmod_d81,
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
	&imgmod_vic20,
	&imgmod_vmsx_tap,
	&imgmod_vmsx_gm2,
	&imgmod_fmsx_cas,
	&imgmod_msx_dsk,
	&imgmod_rom16
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
	"Parameter too small",
	"Parameter too large",
	"Missing parameter not found",
	"Inappropriate parameter",
	"Invalid parameter",
	"Bad file name",
	"Out of space on image",
	"Input past end of file"
};

const char *imageerror(int err)
{
	err = ERRORCODE(err) - 1;
	assert(err >= 0);
	assert(err < (sizeof(msgs) / sizeof(msgs[0])));
	return msgs[err];
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
	case IMGTOOLERR_BADFILENAME:
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

	assert(img);

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

	/* This makes it so that drivers don't have to take care of clearing
	 * the attributes if they don't apply
	 */
	if (ent->attr_len)
		ent->attr[0] = '\0';

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

int img_freespace(IMAGE *img, int *sz)
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

static const struct NamedOption *findnamedoption(const struct NamedOption *nopts, const char *name)
{
	if (nopts) {
		while(nopts->name) {
			if (!strcmp(nopts->name, name))
				return nopts;
			nopts++;
		}
	}
	return NULL;
}

static int check_minmax(const struct OptionTemplate *o, int optnum, const ResolvedOption *ropts, int i)
{
	if (ropts[optnum].i < o->min)
		return PARAM_TO_ERROR(IMGTOOLERR_PARAMTOOSMALL, optnum);
	if (ropts[optnum].i > o->max)
		return PARAM_TO_ERROR(IMGTOOLERR_PARAMTOOLARGE, optnum);
	return 0;
}

static int resolve_options(const struct OptionTemplate *opttemplate, const struct NamedOption *nopts, ResolvedOption *ropts, int roptslen)
{
	int i = 0;
	int optnum, err;
	const char *val;
	const struct NamedOption *n;
	const struct OptionTemplate *o;

	if (opttemplate) {
		for (optnum = 0; opttemplate[optnum].name; optnum++) {
			o = &opttemplate[optnum];

			n = findnamedoption(nopts, o->name);
			if (!n) {
				/* Parameter wasn't specified */
				if ((o->flags & IMGOPTION_FLAG_HASDEFAULT) == 0)
					return PARAM_TO_ERROR(IMGTOOLERR_PARAMNEEDED, optnum);
				val = o->defaultvalue;
			}
			else {
				/* Parameter was specified */
				val = n->value;
			}

			assert(optnum < roptslen);
			switch(o->flags & IMGOPTION_FLAG_TYPE_MASK) {
			case IMGOPTION_FLAG_TYPE_INTEGER:
				ropts[optnum].i = atoi(val);
				
				err = check_minmax(o, optnum, ropts, i);
				if (err)
					return err;
				break;
			case IMGOPTION_FLAG_TYPE_CHAR:
				if (strlen(val) != 1)
					return PARAM_TO_ERROR(IMGTOOLERR_PARAMCORRUPT, optnum);
				ropts[optnum].i = toupper(val[0]);

				err = check_minmax(o, optnum, ropts, i);
				if (err)
					return err;
				break;
			case IMGOPTION_FLAG_TYPE_STRING:
				ropts[optnum].s = val;
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	return 0;
}

int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const struct NamedOption *nopts)
{
	int err;
	ResolvedOption ropts[MAX_OPTIONS];

	if (!img->module->writefile)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = resolve_options(img->module->fileoptions_template, nopts, ropts, sizeof(ropts) / sizeof(ropts[0]));
	if (err)
		return err | IMGTOOLERR_SRC_PARAM_FILE;

	err = img->module->writefile(img, fname, sourcef, ropts);
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

int img_putfile(IMAGE *img, const char *newfname, const char *source, const struct NamedOption *nopts)
{
	int err;
	STREAM *f;

	if (!newfname)
		newfname = osd_basename(source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile(img, newfname, f, nopts);
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

int img_create(const struct ImageModule *module, const char *fname, const struct NamedOption *nopts)
{
	int err;
	STREAM *f;
	ResolvedOption ropts[MAX_OPTIONS];

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = resolve_options(module->createoptions_template, nopts, ropts, sizeof(ropts) / sizeof(ropts[0]));
	if (err)
		return err | IMGTOOLERR_SRC_PARAM_FILE;

	f = stream_open(fname, OSD_FOPEN_WRITE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = module->create(f, ropts);
	stream_close(f);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_create_byname(const char *modulename, const char *fname, const struct NamedOption *nopts)
{
	const struct ImageModule *module;

	module = findimagemodule(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_create(module, fname, nopts);
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



