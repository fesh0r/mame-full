#include <assert.h>
#include "cocovdk.h"
#include "includes/basicdsk.h"

#if MAME_DEBUG
#define LOG_VDK 1
#else
#define LOG_VDK 0
#endif

#if LOG_VDK
#define logerror_vdk(msg)	logerror(msg)
#else
#define logerror_vdk(msg)
#endif /* LOG_VDK */

/* ----------------------------------------------------------------------- *
 * VDK file format                                                         *
 * ----------------------------------------------------------------------- */

#define VDK_MAGIC1			'd'
#define VDK_MAGIC2			'k'
#define VDK_VERSION			0x10
#define VDK_COMPBITS		3
#define VDKFLAGS_WP			0x01
#define VDKFLAGS_ALOCK		0x02
#define VDKFLAGS_FLOCK		0x04
#define VDKFLAGS_DISKSET	0x08

#define VDK_COMPMASK		((1 << VDK_COMPBITS) - 1)

struct vdk_header {
	UINT8  magic1;		/* signature byte 1 */
	UINT8  magic2;		/* signature byte 2 */
	UINT16 header_len;	/* total header length (offset to data) */
	UINT8  ver_actual;	/* version of VDK format */
	UINT8  ver_compat;	/* backwards compatibility version */
	UINT8  source_id;	/* identity of file source */
	UINT8  source_ver;	/* version of file source */
	UINT8  tracks;		/* number of tracks (35 or 40 or 80) */
	UINT8  sides;		/* number of sides (1 or 2) */
	UINT8  flags;		/* various flags */
	UINT8  compression;	/* compression flags and name length */
};

static int validate_header(struct vdk_header *hdr)
{
	/* Check magic bytes */
	if ((hdr->magic1 != VDK_MAGIC1) || (hdr->magic2 != VDK_MAGIC2)) {
		logerror_vdk("validate_header(): Invalid vdk header magic bytes\n");
		return INIT_FAIL;
	}

	/* Check compatibility version */
	if (hdr->ver_compat != VDK_VERSION) {
		logerror_vdk("validate_header(): Virtual disk version not supported!\n");
		return INIT_FAIL;
	}

	/* The VDK format has provisions for compression; this feature has never
	 * been used at all
	 */
	if (hdr->compression & VDK_COMPMASK) {
		logerror_vdk("validate_header(): Compressed vdk not supported!\n");
		return INIT_FAIL;
	}

	/* Invalid number of sides */
	if ((hdr->sides < 1) || (hdr->sides > 2)) {
		logerror_vdk("validate_header(): Compressed vdk not supported!\n");
		return INIT_FAIL;
	}

	return INIT_PASS;
}

int cocovdk_decode_header(void *h, UINT8 *tracks, UINT8 *sides, UINT8 *sec_per_track, UINT16 *sector_length, size_t *offset)
{
	int err;
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);

	hdr = (struct vdk_header *) h;

	err = validate_header(hdr);
	if (err != INIT_PASS)
		return err;

	*offset = LITTLE_ENDIANIZE_INT16(hdr->header_len);
	*tracks = hdr->tracks;
	*sides = hdr->sides;
	*sec_per_track = 18;
	*sector_length = 256;
	return INIT_PASS;
}

int cocovdk_encode_header(void *h, UINT8 tracks, UINT8 sides, UINT8 sec_per_track, UINT16 sector_length)
{
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);

	if ((sec_per_track != 18) || (sector_length != 256) || (sides < 1) || (sides > 2))
		return INIT_FAIL;

	hdr = (struct vdk_header *) h;
	memset(hdr, 0, sizeof(struct vdk_header));
	hdr->magic1 = VDK_MAGIC1;
	hdr->magic2 = VDK_MAGIC2;
	hdr->header_len = sizeof(struct vdk_header);
	hdr->ver_actual = VDK_VERSION;
	hdr->ver_compat = VDK_VERSION;
	hdr->tracks = tracks;
	hdr->sides = sides;
	return INIT_PASS;
}

/* attempt to insert a disk into the drive specified with id */
int cocovdk_floppy_init(int id)
{
	const char			*name = device_filename(IO_FLOPPY, id);
	int					result = INIT_FAIL, read = 0;
	void				*image_file;
	struct vdk_header	hdr;

	/* do we have an image name ? */
	if (!name || !name[0])
	{
		return INIT_FAIL;
	}
	image_file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if( !image_file )
	{
		image_file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
		if( !image_file )
		{
			/* VDK creation not supported */
			logerror("VDK disk creation not supported");
			return INIT_FAIL;
		}
	}

	osd_fseek(image_file, 0, SEEK_SET);
	read = osd_fread(image_file, &hdr, sizeof( struct vdk_header ) );
	osd_fclose( image_file );
	
	if( read != 0 )
	{
		UINT8 tracks;
		UINT8 sides;
		UINT8 sec_per_track;
		UINT16 sector_length;
		size_t offset;
		
		result = cocovdk_decode_header( &hdr, &tracks, &sides, &sec_per_track, &sector_length, &offset);

		if( result == INIT_PASS )
		{
			result = basicdsk_floppy_init(id);
			if( result == INIT_PASS )
				basicdsk_set_geometry(id, tracks, sides, sec_per_track, sector_length, 1, offset);			
		}
	}
	else
		result = INIT_FAIL;
	
	return result;
}

