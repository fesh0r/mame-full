/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

/*#include "imgtool.h"*/
#include "imgtoolx.h"

#include "harddisk.h"
#include "imghd.h"


#define MESSHDTAG "mess_hd"

typedef struct mess_hd
{
	void *hard_disk_handle;
} mess_hd;

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


static struct chd_interface_file *imgtool_chd_open(const char *filename, const char *mode);
static void imgtool_chd_close(struct chd_interface_file *file);
static UINT32 imgtool_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 imgtool_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);

static struct chd_interface imgtool_chd_interface =
{
	imgtool_chd_open,
	imgtool_chd_close,
	imgtool_chd_read,
	imgtool_chd_write
};

/*
	MAME hard disk core interface
*/

/*
	imgtool_chd_open - interface for opening a hard disk image
*/
static struct chd_interface_file *imgtool_chd_open(const char *filename, const char *mode)
{
	STREAM *img = decode_image_ref(filename);


	/* invalid "file name"? */
	if (img == NULL)
		return NULL;

	/* read-only fp? */
	if ((stream_isreadonly(img)) && ! (mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return (struct chd_interface_file *) img;
}

/*
	imgtool_chd_close - interface for closing a hard disk image
*/
static void imgtool_chd_close(struct chd_interface_file *file)
{
	(void) file;
}

/*
	imgtool_chd_read - interface for reading from a hard disk image
*/
static UINT32 imgtool_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	stream_seek((STREAM *)file, offset, SEEK_SET);
	return stream_read((STREAM *)file, buffer, count);
}

/*
	imgtool_chd_write - interface for writing to a hard disk image
*/
static UINT32 imgtool_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	stream_seek((STREAM *)file, offset, SEEK_SET);
	return stream_write((STREAM *)file, buffer, count);
}

/*
	imghd_create()

	Create a MAME HD image
*/
static int imghd_create(STREAM *stream, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression)
{
	char encoded_image_ref[encoded_image_ref_max_len];
	struct chd_interface interface_save;
	int reply;

	if (stream_isreadonly(stream))
		return IMGTOOLERR_WRITEERROR;

	encode_image_ref(stream, encoded_image_ref);

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = chd_create(encoded_image_ref, logicalbytes, hunkbytes, compression, NULL);
	chd_set_interface(&interface_save);

	return reply ? IMGTOOLERR_UNEXPECTED : 0;
}

int imghd_create_base_v1_v2(STREAM *stream, UINT32 version, UINT32 blocksize, UINT32 cylinders, UINT32 heads, UINT32 sectors, UINT32 seclen)
{
	int errorcode;
	char *buf;
	void *disk;
	UINT32 tot_sectors;
	UINT32 i;


	if ((version != 1) && (version != 2))
		return IMGTOOLERR_PARAMCORRUPT;

	if ((version == 1) && (seclen != 512))
		return IMGTOOLERR_PARAMCORRUPT;

	if ((blocksize == 0)|| (blocksize >= 2048))
		return IMGTOOLERR_PARAMCORRUPT;

	errorcode = imghd_create(stream, cylinders * heads * sectors * seclen, blocksize, CHDCOMPRESSION_NONE);
	if (errorcode)
		return errorcode;

	/* fill with 0s */
	buf = malloc(seclen);
	if (!buf)
		return IMGTOOLERR_OUTOFMEMORY;
	memset(buf, 0, seclen);

	disk = imghd_open(stream);
	if (!disk)
		return IMGTOOLERR_UNEXPECTED;

	tot_sectors = cylinders*heads*sectors;

	for (i=0; i<tot_sectors; i++)
		if (imghd_write(disk, i, 1, buf) != 1)
			break;

	imghd_close(stream);
	if (i < tot_sectors)
		return IMGTOOLERR_WRITEERROR;

	return 0;
}

/*
	imghd_open()

	Open stream as a MAME HD image
*/
void *imghd_open(STREAM *stream)
{
	struct chd_file *chd;
	char encoded_image_ref[encoded_image_ref_max_len];
	struct chd_interface interface_save;
	void *hard_disk_handle;

	encode_image_ref(stream, encoded_image_ref);

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	chd = chd_open(encoded_image_ref, !stream_isreadonly(stream), NULL);
	hard_disk_handle = hard_disk_open(chd);
	chd_set_interface(&interface_save);

	return hard_disk_handle;
}

/*
	imghd_close()

	Close MAME HD image
*/
void imghd_close(void *disk)
{
	struct chd_interface interface_save;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	hard_disk_close(disk);
	chd_set_interface(&interface_save);
}

/*
	imghd_read()

	Read sector(s) from MAME HD image
*/
UINT32 imghd_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer)
{
	struct chd_interface interface_save;
	UINT32 reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_read(disk, lbasector, numsectors, buffer);
	chd_set_interface(&interface_save);

	return reply;
}

/*
	imghd_write()

	Write sector(s) from MAME HD image
*/
UINT32 imghd_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer)
{
	struct chd_interface interface_save;
	UINT32 reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_write(disk, lbasector, numsectors, buffer);
	chd_set_interface(&interface_save);

	return reply;
}

/*
	imghd_get_header()

	Return pointer to the header of MAME HD image
*/
const struct hard_disk_info *imghd_get_header(struct hard_disk_file *disk)
{
	struct chd_interface interface_save;
	const struct hard_disk_info *reply;

	chd_save_interface(&interface_save);
	chd_set_interface(&imgtool_chd_interface);
	reply = hard_disk_get_info(disk);
	chd_set_interface(&interface_save);

	return reply;
}


static int mess_hd_image_create(const struct ImageModule *mod, STREAM *f, option_resolution *createoptions);

enum
{
	mess_hd_createopts_version   = 'A',
	mess_hd_createopts_blocksize = 'B',
	mess_hd_createopts_cylinders = 'C',
	mess_hd_createopts_heads     = 'D',
	mess_hd_createopts_sectors   = 'E',
	mess_hd_createopts_seclen    = 'F'
};

OPTION_GUIDE_START( mess_hd_create_optionguide )
	OPTION_INT(mess_hd_createopts_version, "version", "Image format version (v1 only supports seclen = 512)" )
	OPTION_INT(mess_hd_createopts_blocksize, "blocksize", "Sectors per block" )
	OPTION_INT(mess_hd_createopts_cylinders, "cylinders", "Number of cylinders on hard disk" )
	OPTION_INT(mess_hd_createopts_heads, "heads",	"Number of heads on hard disk" )
	OPTION_INT(mess_hd_createopts_sectors, "sectors", "Number of sectors on hard disk" )
	OPTION_INT(mess_hd_createopts_seclen, "seclen", "Number of bytes per sectors" )
OPTION_GUIDE_END

#define mess_hd_create_optionspecs "A1-[2];B[1]-2048;C1-65536;D1-64;E1-4096;F1-[512]-65536"


#if 0
static struct OptionTemplate mess_hd_createopts[] =
{
	{ "version",	"Image format version (v1 only supports seclen = 512)",	IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	2,	"2"},
	{ "blocksize",	"Sectors per block",	IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	2048,	"1"},
	{ "cylinders",	"Number of cylinders on hard disk",	IMGOPTION_FLAG_TYPE_INTEGER,	1,	65536,	NULL},
	{ "heads",		"Number of heads on hard disk",	IMGOPTION_FLAG_TYPE_INTEGER,	1,	64,	NULL},
	{ "sectors",	"Number of sectors on hard disk",	IMGOPTION_FLAG_TYPE_INTEGER,	1,	4096,	NULL},
	{ "seclen",		"Number of bytes per sectors",	IMGOPTION_FLAG_TYPE_INTEGER | IMGOPTION_FLAG_HASDEFAULT,	1,	65536,	"512"},
	{ NULL, NULL, 0, 0, 0, 0 }
};

enum
{
	mess_hd_createopts_version = 0,
	mess_hd_createopts_blocksize = 1,
	mess_hd_createopts_cylinders = 2,
	mess_hd_createopts_heads = 3,
	mess_hd_createopts_sectors = 4,
	mess_hd_createopts_seclen = 5
};


IMAGEMODULE(
	mess_hd,
	"MESS hard disk image",			/* human readable name */
	"hd",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	0,								/* eoln */
	0,								/* flags */
	NULL,							/* init function */
	NULL,							/* exit function */
	NULL,							/* info function */
	NULL,							/* begin enumeration */
	NULL,							/* enumerate next */
	NULL,							/* close enumeration */
	NULL,							/* free space on image */
	NULL,							/* read file */
	NULL,							/* write file */
	NULL,							/* delete file */
	mess_hd_image_create,			/* create image */
	NULL,							/* read sector */
	NULL,							/* write sector */
	NULL,							/* file options */
	mess_hd_createopts				/* create options */
)
#endif

imgtoolerr_t mess_hd_createmodule(imgtool_library *library)
{
	imgtoolerr_t err;
	struct ImageModule *module;

	err = imgtool_library_createmodule(library, "mess_hd", &module);
	if (err)
		return err;

	module->description				= "MESS hard disk image";
	module->extensions				= "hd\0";

	module->create					= mess_hd_image_create;

	module->createimage_optguide	= mess_hd_create_optionguide;
	module->createimage_optspec		= mess_hd_create_optionspecs;

	return IMGTOOLERR_SUCCESS;
}

static int mess_hd_image_create(const struct ImageModule *mod, STREAM *f, option_resolution *createoptions)
{
	UINT32  version, blocksize, cylinders, heads, sectors, seclen;


	(void) mod;

	/* read options */
	version = option_resolution_lookup_int(createoptions, mess_hd_createopts_version);
	blocksize = option_resolution_lookup_int(createoptions, mess_hd_createopts_blocksize);
	cylinders = option_resolution_lookup_int(createoptions, mess_hd_createopts_cylinders);
	heads = option_resolution_lookup_int(createoptions, mess_hd_createopts_heads);
	sectors = option_resolution_lookup_int(createoptions, mess_hd_createopts_sectors);
	seclen = option_resolution_lookup_int(createoptions, mess_hd_createopts_seclen);

	return imghd_create_base_v1_v2(f, version, blocksize, cylinders, heads, sectors, seclen);
}
