/***************************************************************************

	formats.h

	Application independent (i.e. - both MESS/imgtool) interfaces for
	representing image formats

***************************************************************************/

#ifndef FORMATS_H
#define FORMATS_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "osd_cpu.h"

struct disk_geometry
{
	UINT8 tracks;
	UINT8 heads;
	UINT8 sectors;
	UINT8 first_sector_id;
	UINT16 sector_size;
	UINT16 bytes_per_sector;
};

/***************************************************************************

	The format driver itself

***************************************************************************/

enum
{
	/* If this flag is enabled, then images that have the wrong amount of tracks
	 * will be assumed to have simply been truncated.  Otherwise, image files
	 * without enough tracks will be rejected as invalid
	 */
	BDFD_ROUNDUP_TRACKS	= 1
};

#define HEADERSIZE_FLAG_MODULO	0x80000000	/* for when headers are the file size modulo something */
#define HEADERSIZE_FLAGS		0x80000000

typedef struct _bdf_file bdf_file;

struct InternalBdFormatDriver
{
	const char *name;
	const char *humanname;
	const char *extensions;
	const char *geometry_options[16];
	UINT32 header_size;
	UINT8 filler_byte;
	int (*header_decode)(const void *header, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset);
	int (*header_encode)(void *buffer, UINT32 *header_size, const struct disk_geometry *geometry);
	int (*read_sector)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
	int (*write_sector)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
	int (*format_track)(struct InternalBdFormatDriver *drv, bdf_file *bdf, const struct disk_geometry *geometry, UINT8 track, UINT8 head);
	void (*get_sector_info)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
	UINT8 (*get_sector_count)(bdf_file *bdf, const void *header, UINT8 track, UINT8 head);
	int flags;
};

/***************************************************************************

	Macros for building block device format drivers

***************************************************************************/

typedef void (*formatdriver_ctor)(struct InternalBdFormatDriver *drv);

/* no need to directly call this */
#ifdef MAME_DEBUG
void validate_construct_formatdriver(struct InternalBdFormatDriver *drv, int geometry_optnum);
#else
#define validate_construct_formatdriver(drv, geometry_optnum)
#endif

#define BLOCKDEVICE_FORMATDRIVER_START( format_name )								\
	void construct_formatdriver_##format_name(struct InternalBdFormatDriver *drv)	\
	{																				\
		int geometry_optnum = 0;													\
		memset(drv, 0, sizeof(*drv));												\

#define BLOCKDEVICE_FORMATDRIVER_END												\
		validate_construct_formatdriver(drv, geometry_optnum);						\
	}																				\

#define BLOCKDEVICE_FORMATDRIVER_EXTERN( format_name )												\
	extern void construct_formatdriver_##format_name(struct InternalBdFormatDriver *drv)

#define	BDFD_NAME(name_)							drv->name = name_;
#define	BDFD_HUMANNAME(humanname_)					drv->humanname = humanname_;
#define	BDFD_EXTENSIONS(ext)						drv->extensions = ext;
#define BDFD_GEOMETRY(tracks,heads,sectors,secsize)	drv->geometry_options[geometry_optnum++] = (#tracks "," #heads "," #sectors "," #secsize);
#define BDFD_FLAGS(flags_)							drv->flags = flags_;
#define BDFD_HEADER_SIZE(header_size_)				drv->header_size = (header_size_);
#define BDFD_HEADER_SIZE_MODULO(header_size_)		drv->header_size = (header_size_) | HEADERSIZE_FLAG_MODULO;
#define BDFD_HEADER_ENCODE(header_encode_)			drv->header_encode = header_encode_;
#define BDFD_HEADER_DECODE(header_decode_)			drv->header_decode = header_decode_;
#define BDFD_FILLER_BYTE(filler_byte_)				drv->filler_byte = filler_byte_;
#define BDFD_READ_SECTOR(read_sector_)				drv->read_sector = read_sector_;
#define BDFD_WRITE_SECTOR(write_sector_)			drv->write_sector = write_sector_;
#define BDFD_FORMAT_TRACK(format_track_)			drv->format_track = format_track_;
#define BDFD_GET_SECTOR_INFO(get_sector_info_)		drv->get_sector_info = get_sector_info_;
#define BDFD_GET_SECTOR_COUNT(get_sector_count_)	drv->get_sector_count = get_sector_count_;

/***************************************************************************

	Macros for building block device format choices

***************************************************************************/

#define BLOCKDEVICE_FORMATCHOICES_START( choices_name )			\
	formatdriver_ctor formatchoices_##choices_name[] = {


#define BLOCKDEVICE_FORMATCHOICES_END							\
		NULL };

#define BLOCKDEVICE_FORMATCHOICES_EXTERN( choices_name )		\
	extern formatdriver_ctor formatchoices_##choices_name[]

#define BDFC_CHOICE( format_name)	construct_formatdriver_##format_name,

/**************************************************************************/

struct bdf_procs
{
	void (*closeproc)(void *file);
	int (*seekproc)(void *file, INT64 offset, int whence);
	UINT32 (*readproc)(void *file, void *buffer, UINT32 length);
	UINT32 (*writeproc)(void *file, const void *buffer, UINT32 length);
	UINT64 (*filesizeproc)(void *file);
};

enum
{
	BLOCKDEVICE_ERROR_SUCCESS,
	BLOCKDEVICE_ERROR_OUTOFMEMORY,
	BLOCKDEVICE_ERROR_CANTDECODEFORMAT,
	BLOCKDEVICE_ERROR_CANTENCODEFORMAT,
	BLOCKDEVICE_ERROR_SECORNOTFOUND,
	BLOCKDEVICE_ERROR_CORRUPTDATA,
	BLOCKDEVICE_ERROR_UNEXPECTEDLENGTH,
	BLOCKDEVICE_ERROR_UNDEFINEERROR
};

int bdf_create(const struct bdf_procs *procs, formatdriver_ctor format,
	void *file, const struct disk_geometry *geometry, void **outbdf);
int bdf_open(const struct bdf_procs *procs, const formatdriver_ctor *formats,
	void *file, int is_readonly, const char *extension, void **outbdf);
void bdf_close(bdf_file *bdf);
int bdf_read(bdf_file *bdf, void *buffer, int length);
int bdf_write(bdf_file *bdf, const void *buffer, int length);
int bdf_seek(bdf_file *bdf, int offset);
const struct disk_geometry *bdf_get_geometry(bdf_file *bdf);
int bdf_read_sector(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
int bdf_write_sector(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
void bdf_get_sector_info(bdf_file *bdf, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
UINT8 bdf_get_sector_count(bdf_file *bdf, UINT8 track, UINT8 head);
int bdf_is_readonly(bdf_file *bdf);

/***************************************************************************

	Geometry string parsing

***************************************************************************/

struct geometry_test
{
	UINT32 min_size;
	UINT32 max_size;
	UINT32 size_step;
	struct
	{
		UINT32 cumulative_size;
		UINT32 size_divisor;
	} dimensions[4];
};

int process_geometry_string(const char *geometry_string, struct geometry_test *tests, int max_test_count);
int deduce_geometry(const struct geometry_test *tests, int test_count, UINT32 size, UINT32 *params);

#endif /* FORMATS_H */
