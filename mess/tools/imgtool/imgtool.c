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
/* extern struct ImageModule imgmod_svi_cas; */	/* SVI .cas file */
extern struct ImageModule imgmod_xsa;	/* XelaSoft Archive */
extern struct ImageModule imgmod_msx_img;	/* bogus MSX images */
extern struct ImageModule imgmod_msx_ddi;	/* bogus MSX images */
extern struct ImageModule imgmod_msx_msx;	/* bogus MSX images */
extern struct ImageModule imgmod_msx_mul;	/* bogus MSX images */
extern struct ImageModule imgmod_rom16;
extern struct ImageModule imgmod_nccard;	/* NC100/NC150/NC200 PCMCIA Card ram image */
extern struct ImageModule imgmod_ti85p;		/* TI-85 program file */
extern struct ImageModule imgmod_ti85s;		/* TI-85 string file */
extern struct ImageModule imgmod_ti85i;		/* TI-85 picture file */
extern struct ImageModule imgmod_ti85n;		/* TI-85 real number file */
extern struct ImageModule imgmod_ti85c;		/* TI-85 complex number file */
extern struct ImageModule imgmod_ti85l;		/* TI-85 list file */
extern struct ImageModule imgmod_ti85k;		/* TI-85 constant file */
extern struct ImageModule imgmod_ti85m;		/* TI-85 matrix file */
extern struct ImageModule imgmod_ti85v;		/* TI-85 vector file */
extern struct ImageModule imgmod_ti85d;		/* TI-85 graphics database file */
extern struct ImageModule imgmod_ti85e;		/* TI-85 equation file */
extern struct ImageModule imgmod_ti85r;		/* TI-85 range settings file */
extern struct ImageModule imgmod_ti85g;		/* TI-85 grouped file */
extern struct ImageModule imgmod_ti85;		/* TI-85 file */
extern struct ImageModule imgmod_ti85b;		/* TI-85 memory backup file */
extern struct ImageModule imgmod_ti86p;		/* TI-86 program file */
extern struct ImageModule imgmod_ti86s;		/* TI-86 string file */
extern struct ImageModule imgmod_ti86i;		/* TI-86 picture file */
extern struct ImageModule imgmod_ti86n;		/* TI-86 real number file */
extern struct ImageModule imgmod_ti86c;		/* TI-86 complex number file */
extern struct ImageModule imgmod_ti86l;		/* TI-86 list file */
extern struct ImageModule imgmod_ti86k;		/* TI-86 constant file */
extern struct ImageModule imgmod_ti86m;		/* TI-86 matrix file */
extern struct ImageModule imgmod_ti86v;		/* TI-86 vector file */
extern struct ImageModule imgmod_ti86d;		/* TI-86 graphics database file */
extern struct ImageModule imgmod_ti86e;		/* TI-86 equation file */
extern struct ImageModule imgmod_ti86r;		/* TI-86 range settings file */
extern struct ImageModule imgmod_ti86g;		/* TI-86 grouped file */
extern struct ImageModule imgmod_ti86;		/* TI-86 file */

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
	&imgmod_msx_img,
	&imgmod_msx_ddi,
	&imgmod_msx_msx,
	&imgmod_msx_mul,
	&imgmod_xsa,
/*	&imgmod_svi_cas,  -- doesn't work yet! */
	&imgmod_rom16,
	&imgmod_nccard,
	&imgmod_ti85p,
	&imgmod_ti85s,
	&imgmod_ti85i,
	&imgmod_ti85n,
	&imgmod_ti85c,
	&imgmod_ti85l,
	&imgmod_ti85k,
	&imgmod_ti85m,
	&imgmod_ti85v,
	&imgmod_ti85d,
	&imgmod_ti85e,
	&imgmod_ti85r,
	&imgmod_ti85g,
	&imgmod_ti85,
	&imgmod_ti85b,
	&imgmod_ti86p,
	&imgmod_ti86s,
	&imgmod_ti86i,
	&imgmod_ti86n,
	&imgmod_ti86c,
	&imgmod_ti86l,
	&imgmod_ti86k,
	&imgmod_ti86m,
	&imgmod_ti86v,
	&imgmod_ti86d,
	&imgmod_ti86e,
	&imgmod_ti86r,
	&imgmod_ti86g,
	&imgmod_ti86
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
	
	err = module->init(module, f, outimg);
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
	assert(outenum);

	*outenum = NULL;

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

int img_countfiles(IMAGE *img, int *totalfiles)
{
	int err;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char fnamebuf[256];

	*totalfiles = 0;
	memset(&ent, 0, sizeof(ent));
	ent.fname = fnamebuf;
	ent.fname_len = sizeof(fnamebuf) / sizeof(fnamebuf[0]);

	err = img_beginenum(img, &imgenum);
	if (err)
		goto done;

	do {
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (ent.fname[0])
			(*totalfiles)++;
	}
	while(ent.fname[0]);

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}

int img_filesize(IMAGE *img, const char *fname, int *filesize)
{
	int err;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char fnamebuf[256];

	*filesize = -1;
	memset(&ent, 0, sizeof(ent));
	ent.fname = fnamebuf;
	ent.fname_len = sizeof(fnamebuf) / sizeof(fnamebuf[0]);

	err = img_beginenum(img, &imgenum);
	if (err)
		goto done;

	do {
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (!strcmpi(fname, ent.fname)) {
			*filesize = ent.filesize;
			goto done;
		}
	}
	while(ent.fname[0]);

	err = IMGTOOLERR_FILENOTFOUND;

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}

int img_freespace(IMAGE *img, int *sz)
{
	if (!img->module->freespace)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	*sz = img->module->freespace(img);
	return 0;
}

static int process_filter(STREAM **mainstream, STREAM **newstream, const struct ImageModule *imgmod, FILTERMODULE filter, int purpose)
{
	FILTER *f;

	if (filter) {
		f = filter_init(filter, imgmod, purpose);
		if (!f)
			return IMGTOOLERR_OUTOFMEMORY;

		*newstream = stream_open_filter(*mainstream, f);
		if (!(*newstream))
			return IMGTOOLERR_OUTOFMEMORY;

		*mainstream = *newstream;
	}
	return 0;
}

int img_readfile(IMAGE *img, const char *fname, STREAM *destf, FILTERMODULE filter)
{
	int err;
	STREAM *newstream = NULL;

	if (!img->module->readfile) {
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Custom filter? */
	err = process_filter(&destf, &newstream, img->module, filter, PURPOSE_READ);
	if (err)
		goto done;

	err = img->module->readfile(img, fname, destf);
	if (err) {
		err = markerrorsource(err);
		goto done;
	}

done:
	if (newstream)
		stream_close(newstream);
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

static int check_minmax(const struct OptionTemplate *o, int optnum, const ResolvedOption *ropts/*, int i*/)
{
	if (ropts[optnum].i < o->min)
		return PARAM_TO_ERROR(IMGTOOLERR_PARAMTOOSMALL, optnum);
	if (ropts[optnum].i > o->max)
		return PARAM_TO_ERROR(IMGTOOLERR_PARAMTOOLARGE, optnum);
	return 0;
}

static int resolve_options(const struct OptionTemplate *opttemplate, const struct NamedOption *nopts, ResolvedOption *ropts, int roptslen)
{
	//int i = 0;
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
				
				err = check_minmax(o, optnum, ropts/*, i*/);
				if (err)
					return err;
				break;
			case IMGOPTION_FLAG_TYPE_CHAR:
				if (strlen(val) != 1)
					return PARAM_TO_ERROR(IMGTOOLERR_PARAMCORRUPT, optnum);
				ropts[optnum].i = toupper(val[0]);

				err = check_minmax(o, optnum, ropts/*, i*/);
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

int img_writefile_resolved(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *ropts, FILTERMODULE filter)
{
	int err;
	char *buf = NULL;
	char *s;
	STREAM *newstream = NULL;

	if (!img->module->writefile) {
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Does this image module prefer upper case file names? */
	if (img->module->flags & IMGMODULE_FLAG_FILENAMES_PREFERUCASE) {
		/*buf = strdup(fname);*/
		buf = malloc(strlen(fname)+1);
		if (buf)
			strcpy(buf, fname);
		if (!buf) {
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		for (s = buf; *s; s++)
			*s = toupper(*s);
		fname = buf;
	}

	/* Custom filter? */
	err = process_filter(&sourcef, &newstream, img->module, filter, PURPOSE_WRITE);
	if (err)
		goto done;

	err = img->module->writefile(img, fname, sourcef, ropts);
	if (err) {
		err = markerrorsource(err);
		goto done;
	}

done:
	if (buf)
		free(buf);
	if (newstream)
		stream_close(newstream);
	return err;
}

int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const struct NamedOption *nopts, FILTERMODULE filter)
{
	int err;
	ResolvedOption ropts[MAX_OPTIONS];
	char *buf = NULL;
	char *s;
	STREAM *newstream = NULL;

	if (!img->module->writefile) {
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	err = resolve_options(img->module->fileoptions_template, nopts, ropts, sizeof(ropts) / sizeof(ropts[0]));
	if (err) {
		err |= IMGTOOLERR_SRC_PARAM_FILE;
		goto done;
	}

	/* Does this image module prefer upper case file names? */
	if (img->module->flags & IMGMODULE_FLAG_FILENAMES_PREFERUCASE) {
		/*buf = strdup(fname);*/
		buf = malloc(strlen(fname)+1);
		if (buf)
			strcpy(buf, fname);
		if (!buf) {
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		for (s = buf; *s; s++)
			*s = toupper(*s);
		fname = buf;
	}

	/* Custom filter? */
	err = process_filter(&sourcef, &newstream, img->module, filter, PURPOSE_WRITE);
	if (err)
		goto done;

	err = img->module->writefile(img, fname, sourcef, ropts);
	if (err) {
		err = markerrorsource(err);
		goto done;
	}

done:
	if (buf)
		free(buf);
	if (newstream)
		stream_close(newstream);
	return err;
}

int img_getfile(IMAGE *img, const char *fname, const char *dest, FILTERMODULE filter)
{
	int err;
	STREAM *f;

	if (!dest)
		dest = fname;

	f = stream_open(dest, OSD_FOPEN_WRITE);
	if (!f) {
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

int img_putfile_resolved(IMAGE *img, const char *newfname, const char *source, const ResolvedOption *ropts, FILTERMODULE filter)
{
	int err;
	STREAM *f;

	if (!newfname)
		newfname = (const char *) osd_basename((char *) source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile_resolved(img, newfname, f, ropts, filter);
	stream_close(f);
	return err;
}

int img_putfile(IMAGE *img, const char *newfname, const char *source, const struct NamedOption *nopts, FILTERMODULE filter)
{
	int err;
	STREAM *f;

	if (!newfname)
		newfname = (const char *) osd_basename((char *) source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile(img, newfname, f, nopts, filter);
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

int img_create_resolved(const struct ImageModule *module, const char *fname, const ResolvedOption *ropts)
{
	int err;
	STREAM *f;

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	f = stream_open(fname, OSD_FOPEN_WRITE);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = module->create(module, f, ropts);
	stream_close(f);
	if (err)
		return markerrorsource(err);

	return 0;
}

int img_create(const struct ImageModule *module, const char *fname, const struct NamedOption *nopts)
{
	int err;
	ResolvedOption ropts[MAX_OPTIONS];

	if (!module->create)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = resolve_options(module->createoptions_template, nopts, ropts, sizeof(ropts) / sizeof(ropts[0]));
	if (err)
		return err | IMGTOOLERR_SRC_PARAM_CREATE;

	img_create_resolved(module, fname, ropts);

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

/* copying osd_basename() here because we cannot separate it from fileio.c */
char *osd_basename (char *filename)
{
	char *c;

	if (!filename)
		return NULL;

	c = filename + strlen(filename);

	while (c != filename)
	{
		c--;
		if (*c == '\\' || *c == '/' || *c == ':')
			return (c+1);
	}

	return filename;
}




