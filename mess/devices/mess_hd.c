/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

#include "mess_hd.h"



#define MESSHDTAG "mess_hd"

typedef struct mess_hd
{
	struct hard_disk_file *hard_disk_handle;
} mess_hd;

static mess_hd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSHDTAG);
}

/* Encoded mess_image: in order to have hard_disk_open handle mess_image
pointers, we encode the reference as an ASCII string and pass it as a file
name.  mess_hard_disk_open then decodes the file name to get the original
mess_image pointer. */
#define ENCODED_IMAGE_REF_PREFIX	"/:/M/E/S/S//i/m/a/g/e//#"
#define ENCODED_IMAGE_REF_FORMAT	(ENCODED_IMAGE_REF_PREFIX "%06d")
#define ENCODED_IMAGE_REF_LEN		(sizeof(ENCODED_IMAGE_REF_PREFIX)+6)


/*
	encode_image_ref()

	Encode an image pointer into an ASCII string that can be passed to
	hard_disk_open as a file name.
*/
static void encode_image_ref(/*const*/ mess_image *image, char encoded_image_ref[ENCODED_IMAGE_REF_LEN])
{
	snprintf(encoded_image_ref, ENCODED_IMAGE_REF_LEN, ENCODED_IMAGE_REF_FORMAT, image_absolute_index(image));
}

/*
	decode_image_ref()

	This function will decode an image pointer, provided one has been encoded
	in the ASCII string.
*/
static mess_image *decode_image_ref(const char encoded_image_ref[ENCODED_IMAGE_REF_LEN])
{
	int index_;


	if (sscanf(encoded_image_ref, ENCODED_IMAGE_REF_FORMAT, & index_) == 1)
		return image_from_absolute_index(index_);

	return NULL;
}


static struct chd_interface_file *mess_chd_open(const char *filename, const char *mode);
static void mess_chd_close(struct chd_interface_file *file);
static UINT32 mess_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mess_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);

static struct chd_interface mess_hard_disk_interface =
{
	mess_chd_open,
	mess_chd_close,
	mess_chd_read,
	mess_chd_write
};

/*
	MAME hard disk core interface
*/



/*************************************
 *
 *	Interface for opening a hard disk image
 *
 *************************************/

static struct chd_interface_file *mess_chd_open(const char *filename, const char *mode)
{
	mess_image *img = decode_image_ref(filename);

	/* invalid "file name"? */
	assert(img);

	/* read-only fp? */
	if (!image_is_writable(img) && !(mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return (struct chd_interface_file *) image_fp(img);
}



/*************************************
 *
 *	Interface for closing a hard disk image
 *
 *************************************/

static void mess_chd_close(struct chd_interface_file *file)
{
	//mame_fclose((mame_file *)file);
}



/*************************************
 *
 *	Interface for reading from a hard disk image
 *
 *************************************/

static UINT32 mess_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



/*************************************
 *
 *	Interface for writing to a hard disk image
 *
 *************************************/

static UINT32 mess_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}



/*
	device_init_mess_hd()

	Device init
*/
DEVICE_INIT(mess_hd)
{
	mess_hd *hd;

	hd = image_alloctag(image, MESSHDTAG, sizeof(mess_hd));
	if (!hd)
		return INIT_FAIL;

	hd->hard_disk_handle = NULL;

	chd_set_interface(&mess_hard_disk_interface);

	return INIT_PASS;
}



/*
	device_load_mess_hd()

	Device load
*/
DEVICE_LOAD(mess_hd)
{
	mess_hd *hd;
	struct chd_file *chd;
	char encoded_image_ref[ENCODED_IMAGE_REF_LEN];

	hd = get_drive(image);

	/* open the CHD file */
	encode_image_ref(image, encoded_image_ref);
	chd = chd_open(encoded_image_ref, image_is_writable(image), NULL);
	if (!chd)
		return INIT_FAIL;

	/* open the hard disk file */
	hd->hard_disk_handle = hard_disk_open(chd);
	if (!hd->hard_disk_handle)
	{
		chd_close(chd);
		return INIT_FAIL;
	}

	return INIT_PASS;
}



/*
	device_unload_mess_hd()

	Device unload
*/
DEVICE_UNLOAD(mess_hd)
{
	mess_hd *hd = get_drive(image);

	if (hd->hard_disk_handle)
	{
		hard_disk_close(hd->hard_disk_handle);
		hd->hard_disk_handle = NULL;
	}
}



/*************************************
 *
 *	Get the MESS/MAME hard disk handle (from the src/harddisk.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

struct hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image)
{
	mess_hd *hd = get_drive(image);
	return hd->hard_disk_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the src/chd.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

struct chd_file *mess_hd_get_chd_file(mess_image *image)
{
	return hard_disk_get_chd(mess_hd_get_hard_disk_file(image));
}



/*************************************
 *
 *	Tells if a hard disk image is writable (the image must be open)
 *
 *************************************/

int mess_hd_is_writable(mess_image *image)
{
	mess_hd *hd = get_drive(image);

	return image_is_writable(image) &&
		(chd_get_header(hard_disk_get_chd(hd->hard_disk_handle))->flags & CHDFLAGS_IS_WRITEABLE);
}
