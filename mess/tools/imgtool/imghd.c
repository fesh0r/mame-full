/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

#include "imgtool.h"

#include "harddisk.h"
#include "imghd.h"


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
#define encoded_image_ref_prefix "/:/i/m/g/t/o/o/l//S/T/R/E/A/M//#"
#define encoded_image_ref_len_format "%03d"
#define encoded_image_ref_format encoded_image_ref_prefix encoded_image_ref_len_format "@%p"
enum
{
	ptr_max_len = 100,
	encoded_image_ref_len_offset = sizeof(encoded_image_ref_prefix)-1,
	encoded_image_ref_len_len = 3,
	encoded_image_ref_max_len = sizeof(encoded_image_ref_prefix)-1+encoded_image_ref_len_len+1+ptr_max_len+1
};

/*
	encode_image_ref()

	Encode an image pointer into an ASCII string that can be passed to
	hard_disk_open as a file name.
*/
static void encode_image_ref(const STREAM *stream, char encoded_image_ref[encoded_image_ref_max_len])
{
	int actual_len;
	char buf[encoded_image_ref_len_len+1];

	/* print, leaving len as 0 */
	snprintf(encoded_image_ref, encoded_image_ref_max_len, encoded_image_ref_format "%n",
				0, (void *) stream, &actual_len);

	/* debug check: has the buffer been filled */
	assert(actual_len < (encoded_image_ref_max_len-1));

	/* print actual lenght */
	sprintf(buf, encoded_image_ref_len_format, actual_len);
	memcpy(encoded_image_ref+encoded_image_ref_len_offset, buf, encoded_image_ref_len_len);
}

/*
	decode_image_ref()

	This function will decode an image pointer, provided one has been encoded
	in the ASCII string.
*/
static STREAM *decode_image_ref(const char *encoded_image_ref)
{
	int expected_len;
	void *ptr;

	/* read original lenght and ptr */
	if (sscanf(encoded_image_ref, encoded_image_ref_format, &expected_len, &ptr) == 2)
	{
		/* only return ptr if lenght match */
		if (expected_len == strlen(encoded_image_ref))
			return (STREAM *) ptr;
	}

	return NULL;
}


static void *imgtool_hard_disk_open(const char *filename, const char *mode);
static void imgtool_hard_disk_close(void *file);
static UINT32 imgtool_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 imgtool_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer);

static struct hard_disk_interface imgtool_hard_disk_interface =
{
	imgtool_hard_disk_open,
	imgtool_hard_disk_close,
	imgtool_hard_disk_read,
	imgtool_hard_disk_write
};

/*
	MAME hard disk core interface
*/

/*
	imgtool_hard_disk_open - interface for opening a hard disk image
*/
static void *imgtool_hard_disk_open(const char *filename, const char *mode)
{
	STREAM *img = decode_image_ref(filename);


	/* invalid "file name"? */
	if (img == NULL)
		return NULL;

	/* read-only fp? */
	if ((stream_isreadonly(img)) && ! (mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return img;
}

/*
	imgtool_hard_disk_close - interface for closing a hard disk image
*/
static void imgtool_hard_disk_close(void *file)
{
	(void) file;
}

/*
	imgtool_hard_disk_read - interface for reading from a hard disk image
*/
static UINT32 imgtool_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	stream_seek((STREAM *)file, offset, SEEK_SET);
	return stream_read((STREAM *)file, buffer, count);
}

/*
	imgtool_hard_disk_write - interface for writing to a hard disk image
*/
static UINT32 imgtool_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
{
	stream_seek((STREAM *)file, offset, SEEK_SET);
	return stream_write((STREAM *)file, buffer, count);
}

/*
	imghd_open()

	Open stream as a MAME HD image
*/
void *imghd_open(STREAM *stream)
{
	char encoded_image_ref[encoded_image_ref_max_len];
	struct hard_disk_interface interface_save;
	void *hard_disk_handle;

	encode_image_ref(stream, encoded_image_ref);

	hard_disk_save_interface(&interface_save);
	hard_disk_set_interface(&imgtool_hard_disk_interface);
	hard_disk_handle = hard_disk_open(encoded_image_ref, ! stream_isreadonly(stream), NULL);
	hard_disk_set_interface(&interface_save);

	return hard_disk_handle;
}

/*
	imghd_close()

	Close MAME HD image
*/
void imghd_close(void *disk)
{
	struct hard_disk_interface interface_save;

	hard_disk_save_interface(&interface_save);
	hard_disk_set_interface(&imgtool_hard_disk_interface);
	hard_disk_close(disk);
	hard_disk_set_interface(&interface_save);
}

/*
	imghd_read()

	Read sector(s) from MAME HD image
*/
UINT32 imghd_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer)
{
	struct hard_disk_interface interface_save;
	UINT32 reply;

	hard_disk_save_interface(&interface_save);
	hard_disk_set_interface(&imgtool_hard_disk_interface);
	reply = hard_disk_read(disk, lbasector, numsectors, buffer);
	hard_disk_set_interface(&interface_save);

	return reply;
}

/*
	imghd_write()

	Write sector(s) from MAME HD image
*/
UINT32 imghd_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer)
{
	struct hard_disk_interface interface_save;
	UINT32 reply;

	hard_disk_save_interface(&interface_save);
	hard_disk_set_interface(&imgtool_hard_disk_interface);
	reply = hard_disk_write(disk, lbasector, numsectors, buffer);
	hard_disk_set_interface(&interface_save);

	return reply;
}

/*
	hard_disk_get_header()

	Return pointer to the header of MAME HD image
*/
const struct hard_disk_header *imghd_get_header(void *disk)
{
	struct hard_disk_interface interface_save;
	const struct hard_disk_header *reply;

	hard_disk_save_interface(&interface_save);
	hard_disk_set_interface(&imgtool_hard_disk_interface);
	reply = hard_disk_get_header(disk);
	hard_disk_set_interface(&interface_save);

	return reply;
}


/*
	imghd_get_hard_disk_handle()

	Get the MAME hard drive handle after an image has been opened with the
	mess_hd core
*/
/*void *imghd_get_hard_disk_handle(mess_image *image)
{
	mess_hd *hd = get_drive(image);

	return hd->hard_disk_handle;
}*/

/*
	imghd_is_writable()

	Tells if a hard disk image is writable (the image must be open)
*/
/*int imghd_is_writable(mess_image *image)
{
	mess_hd *hd = get_drive(image);

	return image_is_writable(image) && (hard_disk_get_header(hd->hard_disk_handle)->flags & HDFLAGS_IS_WRITEABLE);
}*/
