/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

#include "harddisk.h"
#include "mess_hd.h"



#define MESSHDTAG "mess_hd"

typedef struct mess_hd
{
	void *hard_disk_handle;
} mess_hd;

static mess_hd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSHDTAG);
}

/* Encoded mess_image: in order to have hard_disk_open handle mess_image
pointers, we encode the reference as an ASCII string and pass it as a file
name.  mess_hard_disk_open then decodes the file name to get the original
mess_image pointer. */
#define encoded_image_ref_prefix "/:/M/E/S/S//i/m/a/g/e//#"
#define encoded_image_ref_format encoded_image_ref_prefix "%06d"
enum
{
	encoded_image_ref_len = sizeof(encoded_image_ref_prefix)+6
};

/*
	encode_image_ref()

	Encode an image pointer into an ASCII string that can be passed to
	hard_disk_open as a file name.
*/
static void encode_image_ref(/*const*/ mess_image *image, char encoded_image_ref[encoded_image_ref_len])
{
	snprintf(encoded_image_ref, encoded_image_ref_len, encoded_image_ref_format, image_absolute_index(image));
}

/*
	decode_image_ref()

	This function will decode an image pointer, provided one has been encoded
	in the ASCII string.
*/
static mess_image *decode_image_ref(const char encoded_image_ref[encoded_image_ref_len])
{
	int index_;


	if (sscanf(encoded_image_ref, encoded_image_ref_format, & index_) == 1)
		return image_from_absolute_index(index_);

	return NULL;
}


static void *mess_hard_disk_open(const char *filename, const char *mode);
static void mess_hard_disk_close(void *file);
static UINT32 mess_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mess_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer);

static struct hard_disk_interface mess_hard_disk_interface =
{
	mess_hard_disk_open,
	mess_hard_disk_close,
	mess_hard_disk_read,
	mess_hard_disk_write
};

/*
	MAME hard disk core interface
*/

/*
	mess_hard_disk_open - interface for opening a hard disk image
*/
static void *mess_hard_disk_open(const char *filename, const char *mode)
{
	mess_image *img = decode_image_ref(filename);


	/* invalid "file name"? */
	if (img == NULL)
		return NULL;

	/* read-only fp? */
	if ((! image_is_writable(img)) && ! (mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return image_fp(img);
}

/*
	mess_hard_disk_close - interface for closing a hard disk image
*/
static void mess_hard_disk_close(void *file)
{
	//mame_fclose((mame_file *)file);
}

/*
	mess_hard_disk_read - interface for reading from a hard disk image
*/
static UINT32 mess_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}

/*
	mess_hard_disk_write - interface for writing to a hard disk image
*/
static UINT32 mess_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
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

	hard_disk_set_interface(& mess_hard_disk_interface);

	return INIT_PASS;
}

/*
	device_load_mess_hd()

	Device load
*/
DEVICE_LOAD(mess_hd)
{
	mess_hd *hd = get_drive(image);
	char encoded_image_ref[encoded_image_ref_len];

	encode_image_ref(image, encoded_image_ref);

	hd->hard_disk_handle = hard_disk_open(encoded_image_ref, image_is_writable(image), NULL);
	if (hd->hard_disk_handle != NULL)
	{
		return INIT_PASS;
	}

	return INIT_FAIL;
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

/*
	mess_hd_get_hard_disk_handle()

	Get the MESS hard drive handle after an image has been opened with the
	mess_hd core
*/
void *mess_hd_get_hard_disk_handle(mess_image *image)
{
	mess_hd *hd = get_drive(image);

	return hd->hard_disk_handle;
}
