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

static int cocojvc_decode_header(const void *header, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset)
{
	UINT8 *header_bytes = (UINT8 *) header;
	UINT8 sector_attribute_flag;
	UINT16 physical_bytes_per_sector;

	/* byte offset 0 - sectors per track */
	geometry->sectors = (header_size > 0) ? header_bytes[0] : 18;
	if (geometry->sectors <= 0)
		return -1;

	/* byte offset 1 - side count */
	geometry->heads = (header_size > 1) ? header_bytes[1] : 1;
	if (geometry->heads <= 0)
		return -1;

	/* byte offset 2 - sector size code */
	geometry->sector_size = 128 << ((header_size > 2) ? header_bytes[2] : 1);
	if (geometry->sectors <= 0)
		return -1;

	/* byte offset 3 - first sector ID */
	geometry->first_sector_id = (header_size > 3) ? header_bytes[3] : 1;

	/* byte offset 4 - sector attribute flag */
	sector_attribute_flag = (header_size > 4) ? header_bytes[4] : 0;
	if (sector_attribute_flag != 0)
		return -1;	/* we do not support sector attribute flags */

	physical_bytes_per_sector = geometry->sector_size;
	if (sector_attribute_flag)
		physical_bytes_per_sector++;

	geometry->tracks = (file_size - header_size) / geometry->sectors / geometry->heads / physical_bytes_per_sector;

	/* do we have an oddball size?  reject this file if not */
	if ((file_size - header_size) % physical_bytes_per_sector)
		return -1;

	/* minimum of 35 tracks; support degenerate JVC files */
	if (geometry->tracks < 35)
		geometry->tracks = 35;

	return 0;
}

static int cocojvc_encode_header(void *buffer, UINT32 *header_size, const struct disk_geometry *geometry)
{
	UINT8 *header_bytes = (UINT8 *) buffer;
	*header_size = 0;

	header_bytes[0] = geometry->sectors;
	header_bytes[1] = geometry->heads;

	switch(geometry->sector_size) {
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

	if (geometry->sectors != 18)
		*header_size = 1;
	if (geometry->heads != 1)
		*header_size = 2;
	if (geometry->sector_size != 256)
		*header_size = 3;
	return 0;
}

BLOCKDEVICE_FORMATDRIVER_START( coco_jvc )
	BDFD_NAME( "jvc" )
	BDFD_HUMANNAME( "JVC disk image" )
	BDFD_EXTENSIONS( "dsk\0" )
	BDFD_GEOMETRY( 35/40/80,1,18,256 )
	BDFD_HEADER_SIZE_MODULO( 256 )
	BDFD_HEADER_ENCODE( cocojvc_encode_header )
	BDFD_HEADER_DECODE( cocojvc_decode_header )
	BDFD_FLAGS( BDFD_ROUNDUP_TRACKS )
	BDFD_FILLER_BYTE( 0xFF );
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- *
 * OS9 type file format                                                    *
 *                                                                         *
 * Used by the OS-9 Source project on SourceForge                          *
 * ----------------------------------------------------------------------- */

static int cocoo9t_decode_header(const void *header, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset)
{
    UINT8 *hb = (UINT8 *) header;
    int	totalSectors;
    
    totalSectors = (hb[0x00] << 16) + (hb[0x01] << 8) + hb[0x02];
    
    *offset = 0;
    geometry->first_sector_id = 1;
    geometry->sector_size = 256;
    geometry->sectors = (hb[0x11] << 8) + hb[0x12];
    geometry->heads = (hb[0x10] & 0x01) ? 2 : 1;    
    geometry->tracks = totalSectors / (geometry->sectors * geometry->heads);   
    
    return 0;
}

static int cocoo9t_encode_header(void *buffer, UINT32 *header_size, const struct disk_geometry *geometry)
{
    return 1;
}

BLOCKDEVICE_FORMATDRIVER_START( coco_o9t )
	BDFD_NAME( "o9t" )
	BDFD_HUMANNAME( "OS-9 disk image" )
	BDFD_EXTENSIONS( "os9\0" )
	BDFD_GEOMETRY( 35/40/80,1,18,256 )
	BDFD_HEADER_SIZE( 256 )
	BDFD_HEADER_ENCODE( cocoo9t_encode_header )
	BDFD_HEADER_DECODE( cocoo9t_decode_header )
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

static int validate_vdk_header(struct vdk_header *hdr)
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

static int cocovdk_decode_header(const void *h, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset)
{
	int err;
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);
	assert(header_size == VDK_HEADER_LEN);

	hdr = (struct vdk_header *) h;

	err = validate_vdk_header(hdr);
	if (err != INIT_PASS)
		return err;

	*offset = LITTLE_ENDIANIZE_INT16(hdr->header_len);
	geometry->tracks = hdr->tracks;
	geometry->heads = hdr->sides;
	geometry->sectors = 18;
	geometry->sector_size = 256;
	geometry->first_sector_id = 1;
	return INIT_PASS;
}

static int cocovdk_encode_header(void *h, UINT32 *header_size, const struct disk_geometry *geometry)
{
	struct vdk_header *hdr;

	assert(sizeof(struct vdk_header) == VDK_HEADER_LEN);
	assert(*header_size == VDK_HEADER_LEN);

	if ((geometry->sectors != 18) || (geometry->sector_size != 256) || (geometry->heads < 1) || (geometry->heads > 2)
			|| (geometry->first_sector_id != 1))
		return INIT_FAIL;

	hdr = (struct vdk_header *) h;
	memset(hdr, 0, sizeof(struct vdk_header));
	hdr->magic1 = VDK_MAGIC1;
	hdr->magic2 = VDK_MAGIC2;
	hdr->header_len = sizeof(struct vdk_header);
	hdr->ver_actual = VDK_VERSION;
	hdr->ver_compat = VDK_VERSION;
	hdr->tracks = geometry->tracks;
	hdr->sides = geometry->heads;
	return INIT_PASS;
}

BLOCKDEVICE_FORMATDRIVER_START( coco_vdk )
	BDFD_NAME( "vdk" )
	BDFD_HUMANNAME( "VDK disk image" )
	BDFD_EXTENSIONS( "vdk\0" )
	BDFD_HEADER_SIZE( 12 )
	BDFD_HEADER_ENCODE( cocovdk_encode_header )
	BDFD_HEADER_DECODE( cocovdk_decode_header )
	BDFD_GEOMETRY( 35/40/80,1,18,256 )
	BDFD_FILLER_BYTE( 0xFF )
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- *
 * DMK file format                                                         *
 *                                                                         *
 * David M. Keil's disk image format is aptly called an 'on disk' image    *
 * format. This means that whatever written to the disk is enocded into    *
 * the image file. IDAMS, sector headers, traling CRCs, and intra          *
 * sector spacing.                                                         *
 * ----------------------------------------------------------------------- */

#define DMK_HEADER_LEN	16
#define DMK_TOC_LEN 64
#define DMK_DATA_GAP 80
#define DMK_IDFIELD 7
#define DMK_TRACK_LEN 0x1900
#define DMK_RSDOS_INTERLEAVE 4

struct dmk_header
{
	UINT8		writeProtect; /* 0xff = writed protected, 0x00 = OK to write											*/
	UINT8		trackCount;   /* Ones based. Per side.																	*/
	UINT16		trackLength;  /* Bytes used for each track. This number includes DMK track header. Little endian.		*/
	UINT8		diskOptions;  /* Bit 0: Unused. 																		*/
							  /* Bit 1: Unused.																			*/
							  /* Bit 2: Unused.																			*/
							  /* Bit 3: Unused.																			*/
							  /* Bit 4: Single sided?																	*/
							  /* Bit 5: Unused.																			*/
							  /* Bit 6: Single density?																	*/
							  /* Bit 7: Ignore density flags? (always write one byte, depreciated)						*/
	UINT8		reserved[7];
	UINT32		realDiskCode; /* If this is 0x12345678 (little endian) then access a real disk drive (unsupported)		*/
};

struct dmk_track_toc
{
	UINT16		idamOffset[DMK_TOC_LEN];	/* Note: little endian in file				*/
											/* Bit 15: Sector double density?			*/
											/* Bit 14: Undefined (reserved)				*/
											/* Bit 13-0: Offset from begining of track
										         header to 'FE' byte of IDAM
										         Note these are always sorted
										         from first to last. All empty
										         entires are 0x00						*/
};

struct dmk_IDAM
{
	UINT8	data[7];	/*	UINT8	type;
							UINT8	trackNumber;
							UINT8	sideNumber;
							UINT8	sectorNumber;
							UINT8	sectorLength;
							UINT16	crc;				Big endian
						*/
};

#define dmk_idam_type(x)			x.data[0]
#define dmk_idam_trackNumber(x)		x.data[1]
#define dmk_idam_sideNumber(x)		x.data[2]
#define dmk_idam_sectorNumber(x)	x.data[3]
#define dmk_idam_sectorLength(x)	x.data[4]
#define dmk_idam_crc(x)				((x.data[5] << 8) + x.data[6])

#define dmk_set_idam_type(x, y)				x.data[0] = y
#define dmk_set_idam_trackNumber(x, y)		x.data[1] = y
#define dmk_set_iaam_sideNumber(x, y)		x.data[2] = y
#define dmk_set_idam_sectorNumber(x, y)		x.data[3] = y
#define dmk_set_idam_sectorLength(x, y)		x.data[4] = y
#define dmk_set_idam_crc(x, y)				{ x.data[5] = (y >> 8 & 0xff); x.data[6] = (y & 0xff); }

/* crc.c
   Compute CCITT CRC-16 using the correct bit order for floppy disks.
   CRC code courtesy of Tim Mann.
*/

/* Accelerator table to compute the CRC eight bits at a time */
static const UINT16 dmk_crc16_table[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

#define COCODMK_CALC_CRC1(crc, c) (((crc) << 8) ^ dmk_crc16_table[((crc) >> 8) ^ (c)])

static void cocodmk_calculate_crc( UINT16 *crc, UINT8 *buffer, int size )
{
	int	i;

	for( i=0; i<size; i++ )
		*crc = COCODMK_CALC_CRC1( *crc, buffer[i] );
}

static int validate_dmk_header(struct dmk_header *hdr, UINT32 file_size)
{
	int				result = INIT_FAIL;
	unsigned long	calc_size;
	int				side_count = (hdr->diskOptions && 0x10) ? 1 : 2;

	calc_size = LITTLE_ENDIANIZE_INT16( hdr->trackLength ) * hdr->trackCount * side_count + sizeof( struct dmk_header );

	/* If this expression is true then we are virtuality guaranteed that this is a real DMK image */
	if( calc_size == file_size )
	{
		if( LITTLE_ENDIANIZE_INT32( hdr->realDiskCode ) == 0x00000000 ) /* Real disk files not supported */
			result = INIT_PASS;
		else
			result = INIT_FAIL;
	}

	return result;
}

static int cocodmk_decode_header(const void *header, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset)
{
	int err = INIT_FAIL;
	struct dmk_header *hdr;
	
	assert( sizeof(struct dmk_header) == DMK_HEADER_LEN);
	assert( header_size == DMK_HEADER_LEN );
	assert( sizeof( struct dmk_IDAM ) == DMK_IDFIELD );
	assert( sizeof( struct dmk_track_toc ) == DMK_TOC_LEN * sizeof( UINT16 ) );
	
	hdr = (struct dmk_header *) header;

	err = validate_dmk_header( hdr, file_size );
	
	if( err == INIT_FAIL )
		return err;
		
	*offset = DMK_HEADER_LEN;
	geometry->tracks = hdr->trackCount;
	geometry->heads = (hdr->diskOptions && 0x10) ? 1 : 2;
	geometry->sectors = 18; /* hmm. */
	geometry->sector_size = 256;  /* Hmm. */
	geometry->first_sector_id = 1;  /* Hmm. */

	return err;
}

static int cocodmk_encode_header(void *buffer, UINT32 *header_size, const struct disk_geometry *geometry)
{
	struct dmk_header	*hdr;
	
	hdr = (struct dmk_header *)buffer;
	
	hdr->writeProtect = 0;
	hdr->trackCount = geometry->tracks;
	hdr->trackLength = LITTLE_ENDIANIZE_INT16( DMK_TRACK_LEN );
	
	hdr->diskOptions = 0;
	if( geometry->heads == 1 )
		hdr->diskOptions = 0x10;

	hdr->reserved[0] = 0;
	hdr->reserved[1] = 0;
	hdr->reserved[2] = 0;
	hdr->reserved[3] = 0;
	hdr->reserved[4] = 0;
	hdr->reserved[5] = 0;
	hdr->reserved[6] = 0;
	
	hdr->realDiskCode = 0;
	
	return 0;
}

static int cocodmk_seek_to_start_of_idam_at_index( void *bdf, struct dmk_header *hdr, UINT8 track, UINT8 head, UINT8 sector_index)
{
	int	offset, IDAM_offset, count;
	struct dmk_track_toc	t_toc;
	int	sideCount = (hdr->diskOptions && 0x10) ? 1 : 2,
                trackLength = LITTLE_ENDIANIZE_INT16(hdr->trackLength);	

	/* Seek to start of track */
	offset = DMK_HEADER_LEN + (trackLength * track * sideCount) + (trackLength * head);
	bdf_seek(bdf, offset);
	
	/* Read in DMK track table of contents */
	count = bdf_read( bdf, &t_toc, sizeof( struct dmk_track_toc ) );
	if( count != sizeof( struct dmk_track_toc ) )
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;

	IDAM_offset = (LITTLE_ENDIANIZE_INT16( t_toc.idamOffset[sector_index] )) & 0x3fff;

        if (IDAM_offset == 0 )
            return BLOCKDEVICE_ERROR_SECORNOTFOUND;
            
        if( IDAM_offset > trackLength )
            return BLOCKDEVICE_ERROR_SECORNOTFOUND;
            
        bdf_seek( bdf, offset+IDAM_offset );

        return 0;

}

static int cocodmk_seek_to_start_of_sector( void *bdf, struct dmk_header *hdr, UINT8 track, UINT8 head, UINT8 sector, int *length )
{
	int	error = 0, offset, i, IDAM_offset, count, state;
	UINT8	val;
	UINT16	calculated_crc;
	struct dmk_track_toc	t_toc;
	struct dmk_IDAM	idam;
	int	sideCount = (hdr->diskOptions && 0x10) ? 1 : 2,
                trackLength = LITTLE_ENDIANIZE_INT16(hdr->trackLength);	

	*length = 0;
	
	/* Seek to start of track */
	offset = DMK_HEADER_LEN + (trackLength * track * sideCount) + (trackLength * head);
	bdf_seek(bdf, offset);
	
	/* Read in DMK track table of contents */
	count = bdf_read( bdf, &t_toc, sizeof( struct dmk_track_toc ) );
	if( count != sizeof( struct dmk_track_toc ) )
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
		
	/* Search for matching IDAM */
	for( i=0; i<DMK_TOC_LEN; i++ )
	{
                IDAM_offset = (LITTLE_ENDIANIZE_INT16( t_toc.idamOffset[i] )) & 0x3fff;
		
		if( IDAM_offset == 0 )
		{
			i = DMK_TOC_LEN;
			break;
		}
		
		bdf_seek( bdf, offset+IDAM_offset );
		count = bdf_read( bdf, &idam, sizeof( struct dmk_IDAM ) );
		if( count != sizeof( struct dmk_IDAM ) )
			return BLOCKDEVICE_ERROR_OUTOFMEMORY;
		
		calculated_crc = 0xcdb4;	/* Seed crc with proper value */
		cocodmk_calculate_crc( &calculated_crc, (UINT8 *)&idam, sizeof( struct dmk_IDAM ) - 2 );

		/* Check IDAM integraity. Check for matching sector */
		if( calculated_crc == dmk_idam_crc(idam) && dmk_idam_trackNumber(idam) == track && dmk_idam_sideNumber(idam) == head && dmk_idam_sectorNumber(idam) == sector )
			break;
	}
	
	if( i == DMK_TOC_LEN )
		return BLOCKDEVICE_ERROR_SECORNOTFOUND;

	/* Hey, we found a matching sector ID */
	*length = 128 << dmk_idam_sectorLength(idam);

	state = 0;
	
	/* Find pattern: 0xA1A1FB
	   This represents the start of a data sector */
	for( i=0; i < DMK_DATA_GAP; i++ )
	{
		bdf_read( bdf, &val, 1 );
		
		if( val == 0xA1 )
			state++;
		else if( val == 0xFB && state > 0 )
			break;
		else
			state = 0;
	}
	
	if( i >= DMK_DATA_GAP )
		error = BLOCKDEVICE_ERROR_SECORNOTFOUND;
		
	return error;
}

static int cocodmk_read_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length)
{
	int					error, actual_sector_length, size;
	UINT16				crc_on_disk, calculated_crc;
	struct dmk_header	*hdr;
	UINT8				*sectorData;
	
	hdr = (struct dmk_header *) header;
	
	if( track > hdr->trackCount)
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;

	if( head > ((hdr->diskOptions && 0x10) ? 0 : 1) )
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;
		
	error = cocodmk_seek_to_start_of_sector( bdf, hdr, track, head, sector, &actual_sector_length );
	
	if( error )
		return error;

	if( (offset + length) > actual_sector_length )
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;
		
	sectorData = (UINT8 *)malloc( actual_sector_length );
	
	/* Read sector data into buffer */
	size = bdf_read( bdf, sectorData, actual_sector_length);
	if( size != actual_sector_length )
	{
		free( sectorData );
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
	}
	
	/* Read CRC bytes into variable */
	size = bdf_read( bdf, &crc_on_disk, 2 );
	if( size != 2 )
	{
		free( sectorData );
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
	}

	calculated_crc = 0xE295;	/* Seed crc with proper value */
	cocodmk_calculate_crc( &calculated_crc, sectorData, actual_sector_length );

	if( calculated_crc != BIG_ENDIANIZE_INT16( crc_on_disk ) )
		error =  BLOCKDEVICE_ERROR_CORRUPTDATA;
	
	memcpy( buffer, sectorData + offset, length );
	
	free( sectorData );
	
	return error;
}

static int cocodmk_write_sector(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length)
{
	int					error, actual_sector_length, size;
	UINT16				calculated_crc;
	struct dmk_header	*hdr;
	UINT8				*sectorData;
	
	hdr = (struct dmk_header *) header;
	
	if( track > hdr->trackCount)
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;

	if( head > ((hdr->diskOptions && 0x10) ? 0 : 1) )
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;
		
	error = cocodmk_seek_to_start_of_sector( bdf, hdr, track, head, sector, &actual_sector_length );
	
	if( error )
		return error;

	if( (offset + length) > actual_sector_length )
		return BLOCKDEVICE_ERROR_UNDEFINEERROR;
		
	sectorData = (UINT8 *)malloc( actual_sector_length );
	
	/* Read sector data into buffer */
	size = bdf_read( bdf, sectorData, actual_sector_length);
	if( size != actual_sector_length )
	{
		free( sectorData );
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
	}

	memcpy( sectorData + offset, buffer, length );
	
	calculated_crc = 0xE295;	/* Seed crc with proper value */
	cocodmk_calculate_crc( &calculated_crc, sectorData, actual_sector_length );

	bdf_seek( bdf, -actual_sector_length );
	bdf_write( bdf, sectorData, actual_sector_length );

	calculated_crc = BIG_ENDIANIZE_INT16( calculated_crc );
	bdf_write( bdf, &calculated_crc, 2 );
	
	free( sectorData );
	
	return error;
}

static int dmK_setUpSkipping( UINT8 *sector_array, UINT8 factor, const struct disk_geometry *geometry )
{
	int		i, j,
			first = geometry->first_sector_id,
			size = geometry->sectors;

	if( factor > size )
		return BLOCKDEVICE_ERROR_CANTENCODEFORMAT;

	j = 0;

	for( i = first; i <= size + first; i++ )
	{
		sector_array[j] = i;

		j= j + factor + 1;

		if( j > size )
			j = j - size;
	}
	
	return BLOCKDEVICE_ERROR_SUCCESS;
}

static void bdf_write_repeat( void *bdf, UINT8 value, int count, int *track_length )
{
	int i;
	
	for( i=0; i<count; i++ )
		bdf_write( bdf, &value, 1 );
		
	*track_length += count;
}

static int cocodmk_format_track(struct InternalBdFormatDriver *drv, bdf_file *bdf, const struct disk_geometry *geometry, UINT8 track, UINT8 head)
{
	int						i, j, track_length = 0, error;
	UINT16					offset, calculated_crc;
	struct dmk_IDAM			idam;
	UINT8					sector_array[256], *sector_body;

	if( geometry->sectors > DMK_TOC_LEN )
		return BLOCKDEVICE_ERROR_CANTENCODEFORMAT;
		
	/* Set up track table of contents */
	
	for( i = 0, j = ( DMK_TOC_LEN * sizeof( UINT16 ) ) + 32 + 8 + 3; i < DMK_TOC_LEN; i++ )
	{
		if( i >= geometry->sectors )
			offset = 0;
		else
		{
			if( j > DMK_TRACK_LEN )
				return BLOCKDEVICE_ERROR_CANTENCODEFORMAT;
				
			offset = j | 0x8000;
			offset = LITTLE_ENDIANIZE_INT16(offset);
		}
		
		bdf_write( bdf, &offset, sizeof( UINT16 ) );
		track_length += sizeof( UINT16 );

		j += 5 + 2 + 22 + 12 + 3 + 1 + geometry->sector_size + 2 + 24 + 8 + 3;
	}
	
	/* Write track lead in */
	bdf_write_repeat( bdf, 0x4e, 32, &track_length );
	
	/* Setup interleave */
	error = dmK_setUpSkipping( sector_array, DMK_RSDOS_INTERLEAVE, geometry );
	if( error != BLOCKDEVICE_ERROR_SUCCESS )
		return error;
	
	/* Now write each sector */
	for(i=0; i<geometry->sectors; i++ )
	{
		bdf_write_repeat( bdf, 0x00, 8, &track_length );
		bdf_write_repeat( bdf, 0xa1, 3, &track_length );
		dmk_set_idam_type( idam, 0xfe );
		dmk_set_idam_trackNumber( idam, track );
		dmk_set_iaam_sideNumber( idam, head );
		dmk_set_idam_sectorNumber( idam, sector_array[i] );
		
		switch( geometry->sector_size )
		{
			case 128 << 0:
				dmk_set_idam_sectorLength( idam, 0 );
				break;
			case 128 << 1:
				dmk_set_idam_sectorLength( idam, 1 );
				break;
			case 128 << 2:
				dmk_set_idam_sectorLength( idam, 2 );
				break;
			case 128 << 3:
				dmk_set_idam_sectorLength( idam, 3 );
				break;
			case 128 << 4:
				dmk_set_idam_sectorLength( idam, 4 );
				break;
			case 128 << 5:
				dmk_set_idam_sectorLength( idam, 5 );
				break;
			case 128 << 6:
				dmk_set_idam_sectorLength( idam, 6 );
				break;
			default:
				return BLOCKDEVICE_ERROR_CANTDECODEFORMAT;
		}
		
		/* Calculate sector header CRC */
		calculated_crc = 0xcdb4;	/* Seed crc with proper value */
		cocodmk_calculate_crc( &calculated_crc, (UINT8 *)&idam, sizeof( struct dmk_IDAM ) - 2 );
		dmk_set_idam_crc( idam, calculated_crc );
		
		/* Write sector header */
		bdf_write( bdf, &idam, sizeof( struct dmk_IDAM ) );
		track_length += sizeof( struct dmk_IDAM );
		
		/* Write padding between sector header and sector body */
		bdf_write_repeat( bdf, 0x4e, 22, &track_length );
		bdf_write_repeat( bdf, 0x00, 12, &track_length );

		/* Setup sector body */
		sector_body = (UINT8 *)malloc( geometry->sector_size + 1 );
		if( sector_body == NULL )
			return BLOCKDEVICE_ERROR_OUTOFMEMORY;
		
		sector_body[0] = 0xfb;
		memset( sector_body+1, drv->filler_byte, geometry->sector_size );

		calculated_crc = 0xcdb4;	/* Seed crc with proper value */
		cocodmk_calculate_crc( &calculated_crc, sector_body, geometry->sector_size + 1 );
		calculated_crc = BIG_ENDIANIZE_INT16( calculated_crc );

		/* Write sector body */
		bdf_write_repeat( bdf, 0xa1, 3, &track_length );
		bdf_write( bdf, sector_body, geometry->sector_size + 1 );
		track_length += geometry->sector_size + 1;
		free( sector_body );
		sector_body = NULL;
		bdf_write( bdf, &calculated_crc, 2 );
		track_length += 2;
		
		/* Write sector footer */
		bdf_write_repeat( bdf, 0x4e, 24, &track_length );
	}
	
	if( track_length > DMK_TRACK_LEN )
		return BLOCKDEVICE_ERROR_CANTDECODEFORMAT;
	
	bdf_write_repeat( bdf, 0x4e, DMK_TRACK_LEN - track_length, &track_length );
	
	assert( track_length == DMK_TRACK_LEN);

	return BLOCKDEVICE_ERROR_SUCCESS;
}



static void cocodmk_sector_info(bdf_file *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size)
{
    int	count;
    struct dmk_header *hdr = (struct dmk_header *) header;
    struct dmk_IDAM idam;
    
    cocodmk_seek_to_start_of_idam_at_index( bdf, hdr, track, head, sector_index);
    
    count = bdf_read( bdf, &idam, sizeof( struct dmk_IDAM ) );
    if( count != sizeof( struct dmk_IDAM ) )
        return;

    *sector = dmk_idam_sectorNumber(idam);
    *sector_size = 128 << dmk_idam_sectorLength(idam);
}

static UINT8 cocodmk_sector_count(bdf_file *bdf, const void *header, UINT8 track, UINT8 head)
{
	int	offset, i, IDAM_offset, count;
	UINT8	IDAM_count = 0;
	struct dmk_header *hdr = (struct dmk_header *) header;
	struct dmk_track_toc t_toc;
	int	sideCount = (hdr->diskOptions && 0x10) ? 1 : 2,
                trackLength = LITTLE_ENDIANIZE_INT16(hdr->trackLength);	
	
	/* Seek to start of track */
	offset = DMK_HEADER_LEN + (trackLength * track * sideCount) + (trackLength * head);
	bdf_seek(bdf, offset);
	
	/* Read in DMK track table of contents */
	count = bdf_read( bdf, &t_toc, sizeof( struct dmk_track_toc ) );
	if( count != sizeof( struct dmk_track_toc ) )
		return BLOCKDEVICE_ERROR_OUTOFMEMORY;
		
	/* Count IDAMs */
	for( i=0; i<DMK_TOC_LEN; i++ )
	{
            IDAM_offset = (LITTLE_ENDIANIZE_INT16( t_toc.idamOffset[i] )) & 0x3fff;
		
            if( IDAM_offset == 0 )
            {
                i = DMK_TOC_LEN;
                break;
            }
            
            IDAM_count++;
	}
        
    return IDAM_count;
}

BLOCKDEVICE_FORMATDRIVER_START( coco_dmk )
	BDFD_NAME( "dmk" )
	BDFD_HUMANNAME( "DMK disk image" )
	BDFD_EXTENSIONS( "dsk\0" )
	BDFD_GEOMETRY( 35/40/80,1,18,256 )
	BDFD_HEADER_SIZE( DMK_HEADER_LEN )
	BDFD_HEADER_ENCODE( cocodmk_encode_header )
	BDFD_HEADER_DECODE( cocodmk_decode_header )
	BDFD_FILLER_BYTE( 0xFF );
	BDFD_READ_SECTOR(cocodmk_read_sector)
	BDFD_WRITE_SECTOR(cocodmk_write_sector)
	BDFD_FORMAT_TRACK(cocodmk_format_track)
	BDFD_GET_SECTOR_INFO(cocodmk_sector_info)
	BDFD_GET_SECTOR_COUNT(cocodmk_sector_count)
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- */

BLOCKDEVICE_FORMATCHOICES_START( coco )
	BDFC_CHOICE( coco_o9t )
	BDFC_CHOICE( coco_jvc )
	BDFC_CHOICE( coco_vdk )
	BDFC_CHOICE( coco_dmk )
BLOCKDEVICE_FORMATCHOICES_END
