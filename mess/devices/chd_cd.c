/*********************************************************************

	Code to interface the MESS image code with MAME's CHD-CD core.

	Based on harddriv.c by Raphael Nabet 2003

*********************************************************************/

#include "chd_cd.h"

#define MAX_CDROMS	(4)	// up to this many drives

static const char *error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"hunk out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file"
	"operation not supported",
	"can't find metadata",
	"invalid metadata size",
	"unsupported CHD version"
};

static struct cdrom_file *drive_handles[MAX_CDROMS];

static const char *chd_get_error_string(int chderr)
{
	if ((chderr < 0 ) || (chderr >= (sizeof(error_strings) / sizeof(error_strings[0]))))
		return NULL;
	return error_strings[chderr];
}



static OPTION_GUIDE_START(mess_cd_option_guide)
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char *mess_cd_option_spec =
	"K512/1024/2048/[4096]";


#define MESSCDTAG "mess_cd"

struct mess_cd
{
	struct cdrom_file *cdrom_handle;
};

static struct mess_cd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSCDTAG);
}



/*************************************
 *
 *  chdcd_create_ref()/chdcd_open_ref()
 *
 *  These are a set of wrappers that wrap the chd_open()
 *  and chd_create() functions to provide a way to open
 *  the images with a filename.  This is just a stopgap
 *  measure until I get the core CHD code changed.  For
 *  now, this is an ugly hack but it works very well
 *
 *  When these functions get moved into the core, it will
 *  remove the need to specify an 'open' function in the
 *  CHD interface
 *
 *************************************/

#define ENCODED_IMAGE_REF_PREFIX	"/:/M/E/S/S//i/m/a/g/e//#"
#define ENCODED_IMAGE_REF_FORMAT	(ENCODED_IMAGE_REF_PREFIX "%016x")
#define ENCODED_IMAGE_REF_LEN		(sizeof(ENCODED_IMAGE_REF_PREFIX)+16)


static void encode_ptr(void *ptr, char filename[ENCODED_IMAGE_REF_LEN])
{
	snprintf(filename, ENCODED_IMAGE_REF_LEN, ENCODED_IMAGE_REF_FORMAT,
		(unsigned int) ptr);
}



int chdcd_create_ref(void *ref, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression, struct chd_file *parent)
{
	char filename[ENCODED_IMAGE_REF_LEN];
	encode_ptr(ref, filename);
	return chd_create(filename, logicalbytes, hunkbytes, compression, parent);
}



struct chd_file *chdcd_open_ref(void *ref, int writeable, struct chd_file *parent)
{
	char filename[ENCODED_IMAGE_REF_LEN];
	encode_ptr(ref, filename);
	return chd_open(filename, writeable, parent);
}



/*************************************
 *
 *	decode_image_ref()
 *
 *	This function will decode an image pointer,
 *	provided one has been encoded in the ASCII
 *	string.
 *
 *************************************/

static mess_image *decode_image_ref(const char encoded_image_ref[ENCODED_IMAGE_REF_LEN])
{
	unsigned int ptr;

	if (sscanf(encoded_image_ref, ENCODED_IMAGE_REF_FORMAT, &ptr) == 1)
		return (mess_image *) ptr;

	return NULL;
}



/*************************************
 *
 *	Interface between MAME's CHD system and MESS's image system
 *
 *************************************/

static struct chd_interface_file *mess_chd_open(const char *filename, const char *mode);
static void mess_chd_close(struct chd_interface_file *file);
static UINT32 mess_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mess_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 mess_chd_length(struct chd_interface_file *file);

static struct chd_interface mess_cdrom_interface =
{
	mess_chd_open,
	mess_chd_close,
	mess_chd_read,
	mess_chd_write,
	mess_chd_length
};


static struct chd_interface_file *mess_chd_open(const char *filename, const char *mode)
{
	mess_image *img = decode_image_ref(filename);

	/* invalid "file name"? */
	assert(img);

	/* cdroms are read-only */
	if (!(mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return (struct chd_interface_file *) image_fp(img);
}



static void mess_chd_close(struct chd_interface_file *file)
{
}



static UINT32 mess_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



static UINT32 mess_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}



static UINT64 mess_chd_length(struct chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}




/*************************************
 *
 *	device_init_mess_cd()
 *
 *	Device init
 *
 *************************************/

DEVICE_INIT(mess_cd)
{
	struct mess_cd *cd;

	cd = image_alloctag(image, MESSCDTAG, sizeof(struct mess_cd));
	if (!cd)
		return INIT_FAIL;

        cd->cdrom_handle = NULL;

	chd_set_interface(&mess_cdrom_interface);

	return INIT_PASS;
}



/*************************************
 *
 *	device_load_mess_cd()
 *  	device_create_mess_cd()
 *
 *	Device load and create
 *
 *************************************/

static int internal_load_mess_cd(mess_image *image, const char *metadata)
{
	int err = 0;
	struct mess_cd *cd;
	struct chd_file *chd;
	int id = image_index_in_device(image);

	cd = get_drive(image);

	/* open the CHD file */
	chd = chdcd_open_ref(image, 0, NULL);	// CDs are never writable

	if (!chd)
		goto error;

	/* open the CD-ROM file */
	cd->cdrom_handle = cdrom_open(chd);
	if (!cd->cdrom_handle)
		goto error;

	drive_handles[id] = cd->cdrom_handle;
	return INIT_PASS;

error:
	if (chd)
		chd_close(chd);

	err = chd_get_last_error();
	if (err)
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_get_error_string(err));
	return INIT_FAIL;
}



DEVICE_LOAD(mess_cd)
{
	return internal_load_mess_cd(image, NULL);
}



static DEVICE_CREATE(mess_cd)
{
	return INIT_FAIL;   	// cd-roms are not writable
}



/*************************************
 *
 *	device_unload_mess_cd()
 *
 *	Device unload
 *
 *************************************/

DEVICE_UNLOAD(mess_cd)
{
	struct mess_cd *cd = get_drive(image);
	assert(cd->cdrom_handle);
	cdrom_close(cd->cdrom_handle);
	cd->cdrom_handle = NULL;
}



/*************************************
 *
 *  Get the MESS/MAME cdrom handle (from the src/cdrom.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

struct cdrom_file *mess_cd_get_cdrom_file(mess_image *image)
{
	struct mess_cd *cd = get_drive(image);
	return cd->cdrom_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the src/chd.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

struct chd_file *mess_cd_get_chd_file(mess_image *image)
{
	return NULL;	// not supported by the src/cdrom.c core at this time
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void cdrom_device_getinfo(struct IODevice *iodev)
{
	iodev->createimage_options = auto_malloc(sizeof(*iodev->createimage_options) * 2);
	if (!iodev->createimage_options)
	{
		iodev->error = 1;
		return;
	}

	iodev->type = IO_CDROM;
	iodev->file_extensions = "chd\0";
	iodev->readable = 1;
	iodev->writeable = 0;
	iodev->creatable = 0;
	iodev->init = device_init_mess_cd;
	iodev->load = device_load_mess_cd;
	iodev->create = device_create_mess_cd;
	iodev->unload = device_unload_mess_cd;
	iodev->createimage_optguide = mess_cd_option_guide;
	iodev->createimage_options[0].name = "chdcd";
	iodev->createimage_options[0].description = "MAME/MESS CHD CD-ROM drive";
	iodev->createimage_options[0].extensions = iodev->file_extensions;
	iodev->createimage_options[0].optspec = mess_cd_option_spec;
	iodev->createimage_options[1].name = NULL;
	iodev->createimage_options[1].description = NULL;
	iodev->createimage_options[1].extensions = NULL;
	iodev->createimage_options[1].optspec = NULL;
}

struct cdrom_file *mess_cd_get_cdrom_file_by_number(int drivenum)
{
	if ((drivenum < 0) || (drivenum > MAX_CDROMS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}
