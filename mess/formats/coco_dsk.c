#include "coco_dsk.h"
#include "mess.h"

/* ----------------------------------------------------------------------- *
 * JVC (Jeff Vavasour CoCo) format                                         *
 *                                                                         *
 * Used by Jeff Vavasour's CoCo Emulators                                  *
 *                                                                         *
 * Documented at: http://home.netcom.com/~tlindner/JVC.html                *
 *                                                                         *
 * Detailed in this bitnet.listserv.coco post                              *
 * http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&selm=Y%25TJ7.48312   *
 *	%24Ud.2359177%40news1.rdc1.bc.home.com                                 *
 * ----------------------------------------------------------------------- */

static int cocojvc_decode_header(const void *header, UINT32 file_size, UINT32 header_size, UINT8 *tracks, UINT8 *heads, UINT8 *sectors, UINT16 *bytes_per_sector, int *offset)
{
	UINT8 *header_bytes = (UINT8 *) header;
	UINT8 sectors_per_track;
	UINT8 first_sector_id;
	UINT8 sector_attribute_flag;
	UINT16 physical_bytes_per_sector;

	/* byte offset 0 - sectors per track */
	*sectors = (header_size > 0) ? header_bytes[0] : 18;

	/* byte offset 1 - side count */
	*heads = (header_size > 1) ? header_bytes[1] : 1;

	/* byte offset 2 - sector size code */
	*bytes_per_sector = 128 << ((header_size > 2) ? header_bytes[2] : 1);

	/* byte offset 3 - first sector ID */
	first_sector_id = (header_size > 3) ? header_bytes[3] : 1;

	/* byte offset 4 - sector attribute flag */
	sector_attribute_flag = (header_size > 4) ? header_bytes[4] : 0;

	/* we do not support weird first sector IDs and sector attribute flags */
	if ((first_sector_id != 1) || (sector_attribute_flag != 0))
		return -1;

	physical_bytes_per_sector = *bytes_per_sector;
	if (sector_attribute_flag)
		physical_bytes_per_sector++;

	*tracks = (file_size - header_size) / *sectors / *heads / physical_bytes_per_sector;

	if ((*tracks * *sectors * *heads * physical_bytes_per_sector) != (file_size - header_size))
		return -1;

	return 0;
}

static int cocojvc_encode_header(void *buffer, UINT32 *header_size, UINT8 tracks, UINT8 heads, UINT8 sectors, UINT16 bytes_per_sector)
{
	UINT8 *header_bytes = (UINT8 *) buffer;
	*header_size = 0;

	header_bytes[0] = sectors;
	header_bytes[1] = heads;

	switch(bytes_per_sector) {
	case 128:
		header_bytes[2] = 0x00;
		break;
	case 256:
		header_bytes[2] = 0x01;
		break;
	case 512:
		header_bytes[2] = 0x02;
		break;
	case 1024:
		header_bytes[2] = 0x03;
		break;
	default:
		return -1;
	}

	if (sectors != 18)
		*header_size = 1;
	if (heads != 1)
		*header_size = 2;
	if (bytes_per_sector != 128)
		*header_size = 3;
	return 0;
}

BLOCKDEVICE_FORMATDRIVER_START( coco_jvc )
	BDFD_EXTENSION( "dsk" )
	BDFD_TRACKS_OPTION( 35 )
	BDFD_TRACKS_OPTION( 40 )
	BDFD_TRACKS_OPTION( 80 )
	BDFD_SECTORS_BASE( 1 )
	BDFD_SECTORS_OPTION( 18 )
	BDFD_BYTES_PER_SECTOR( 256 )
	BDFD_HEADER_ENCODE( cocojvc_encode_header )
	BDFD_HEADER_DECODE( cocojvc_decode_header )
	BDFD_FLAGS( BDFD_ROUNDUP_TRACKS )
	BDFD_FILLER_BYTE( 0xFF );
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- *
 * VDK file format                                                         *
 *                                                                         *
 * Used by Paul Burgin's PC-Dragon emulator                                *
 * ----------------------------------------------------------------------- */

#define VDK_HEADER_LEN	12

#define VDK_MAGIC1			'd'
#define VDK_MAGIC2			'k'
#define VDK_VERSION			0x10
#define VDK_COMPBITS		3
#define VDKFLAGS_WP			0x01
#define VDKFLAGS_ALOCK		0x02
#define VDKFLAGS_FLOCK		0x04
#define VDKFLAGS_DISKSET	0x08

#define VDK_COMPMASK		((1 << VDK_COMPBITS) - 1)

#define logerror_vdk(err)

struct vdk_header
{
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

static int cocovdk_decode_header(const void *h, UINT32 file_size, UINT32 header_size, UINT8 *tracks, UINT8 *sides, UINT8 *sec_per_track, UINT16 *sector_length, int *offset)
{
	int err;
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);
	assert(header_size == VDK_HEADER_LEN);

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

static int cocovdk_encode_header(void *h, UINT32 *header_size, UINT8 tracks, UINT8 sides, UINT8 sec_per_track, UINT16 sector_length)
{
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);
	assert(*header_size == VDK_HEADER_LEN);

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

BLOCKDEVICE_FORMATDRIVER_START( coco_vdk )
	BDFD_EXTENSION( "vdk" )
	BDFD_HEADER_SIZE( 12 )
	BDFD_HEADER_ENCODE( cocovdk_encode_header )
	BDFD_HEADER_DECODE( cocovdk_decode_header )
	BDFD_TRACKS_OPTION( 35 )
	BDFD_TRACKS_OPTION( 40 )
	BDFD_TRACKS_OPTION( 80 )
	BDFD_SECTORS_BASE( 1 )
	BDFD_SECTORS_OPTION( 18 )
	BDFD_BYTES_PER_SECTOR( 256 )
	BDFD_FILLER_BYTE( 0xFF )
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- */

BLOCKDEVICE_FORMATCHOICES_START( coco )
	BDFC_CHOICE( coco_jvc )
	BDFC_CHOICE( coco_vdk )
BLOCKDEVICE_FORMATCHOICES_END
