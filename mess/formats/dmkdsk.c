/*********************************************************************

	dmkdsk.c

	David M. Keil disk format.

	tim lindner, 2001
	tlindner@ix.netcom.com

*********************************************************************/

#include "driver.h"
#include "dmkdsk.h"

#define dmkdsk_MAX_DRIVES 4
static dmkdsk     dmkdsk_drives[dmkdsk_MAX_DRIVES];

static void dmkdsk_seek_callback(int,int);
static int dmkdsk_get_sectors_per_track(int,int);
static void dmkdsk_get_id_callback(int, chrn_id *, int, int);
static void dmkdsk_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length);
static void dmkdsk_read_track_data_info_buffer( int drive, int side, char *ptr, int *length );
static void dmkdsk_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length,int ddam);

static int DMKHeuristicVerify( dmkdsk *w );
static packedIDData_P GetPackedSector( int drive, int track, int id_index, int side );
static void SetEntireTrack( int drive, int track, int side, dmkTrack_p track_data );
static dmkTrack_p GetEntireTrack( int drive, int track, int side );
static int CheckDataCRC( packedIDData_P pSector );
static int CheckIDCRC( packedIDData_P pSector );
static void calc_crc_buffer( UINT16 *crc, UINT8 *buffer, int size );
static unsigned short CALC_CRC1a(unsigned short crc, unsigned char byte);

#define VERBOSE 1

floppy_interface dmkdsk_floppy_interface=
{
	dmkdsk_seek_callback,
	dmkdsk_get_sectors_per_track,
	dmkdsk_get_id_callback,
	dmkdsk_read_sector_data_into_buffer,
	dmkdsk_write_sector_data_from_buffer,
	dmkdsk_read_track_data_info_buffer,
	NULL
};

/* crc.c
   Compute CCITT CRC-16 using the correct bit order for floppy disks.
   CRC code courtesy of Tim Mann.
*/

/* Accelerator table to compute the CRC eight bits at a time */
unsigned short const crc16_table[256] = {
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

/* Slow way, not using table */
static unsigned short CALC_CRC1a(unsigned short crc, unsigned char byte)
{
  int i = 8;
  unsigned short b = byte << 8;
  while (i--) {
    crc = (crc << 1) ^ (((crc ^ b) & 0x8000) ? 0x1021 : 0);
    b <<= 1;
  }
  return crc;
}

/* Fast way, using table */
#define CALC_CRC1b(crc, c) (((crc) << 8) ^ crc16_table[((crc) >> 8) ^ (c)])

#ifndef calc_crc1
#define calc_crc1 CALC_CRC1b
#endif

UINT16 dmkdsk_GetTrackLength( dmkHeader_p header )
{
	UINT16	value;
	
#ifdef LSB_FIRST
	value = (header->trackLength_low << 8) + header->trackLength_high;
#else
	value = (header->trackLength_high << 8) + header->trackLength_low;
#endif
	
	return value;
}

UINT16 dmkdsk_GetIDAMCRC( packedIDData_P IDAM )
{
	UINT16	value;
	
#ifdef LSB_FIRST
	value = (IDAM->idCRC_low << 8) + IDAM->idCRC_high;
#else
	value = (IDAM->idCRC_high << 8) + IDAM->idCRC_low;
#endif
	
	return value;
}

UINT32 dmkdsk_GetRealDiskCode( dmkHeader_p header )
{
	UINT32	value;
	
#ifdef LSB_FIRST
	value = (header->realDiskCode[0] << 24) + (header->realDiskCode[1] << 16) +
	        (header->realDiskCode[2] << 8) + (header->realDiskCode[3]);
#else
	value = (header->realDiskCode[1] << 24) + (header->realDiskCode[0] << 16) +
	        (header->realDiskCode[3] << 8) + (header->realDiskCode[2]);
#endif
	
	return value;
}

/* attempt to insert a disk into the drive specified with id */
int dmkdsk_floppy_init(int id)
{
	const char *name = device_filename(IO_FLOPPY, id);
	int			result;

	if (id < dmkdsk_MAX_DRIVES)
	{
		dmkdsk *w = &dmkdsk_drives[id];

		/* do we have an image name ? */
		if (!name || !name[0])
		{
			return INIT_FAIL;
		}
		w->mode = 1;
		w->image_file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
		if( !w->image_file )
		{
			w->mode = 0;
			w->image_file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
			if( !w->image_file )
			{
				/* DMK creation not supported (this will be implemented in imgtool) */
				logerror("DMK disk creation not supported");
				return INIT_FAIL;
			}
		}

		w->image_size = osd_fsize( w->image_file );
		
		result = DMKHeuristicVerify( w );
		
		if( result == INIT_PASS )
		{
			floppy_drive_set_disk_image_interface(id,&dmkdsk_floppy_interface);

			/* If file was opened "read only" mode then mirror that in file format */
			if ( w->mode == 0 )
				w->header.writeProtect = 0xff;

			if( w->header.writeProtect == 0xff )
				floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_WRITE_PROTECTED, 1);
		}
		else
		 	osd_fclose( w->image_file );
	}
	
	return result;
}

static int DMKHeuristicVerify( dmkdsk *w )
{
	int 			read = 0;
	dmkHeader_p		header = &(w->header);
	int				result = INIT_FAIL;
	unsigned long	calc_size;

	osd_fseek(w->image_file, 0, SEEK_SET);
	read = osd_fread(w->image_file, header, sizeof( dmkHeader ) );

	if ( read != 0 )
	{
		/* Adjust endian */

		header->trackLength = LITTLE_ENDIANIZE_INT16( header->trackLength );
		header->realDiskCode = LITTLE_ENDIANIZE_INT32( header->realDiskCode );

		/* If this expression is true then we are virtuality guaranteed that this is a real DMK image */

		calc_size = dmkdsk_GetTrackLength( header ) * header->trackCount * (DMKSIDECOUNT( (*header) )) + sizeof( dmkHeader );

		if( calc_size == w->image_size )
		{
			if( dmkdsk_GetRealDiskCode( header ) != 0x12345678 ) /* Real disk files unsupported */
				result = INIT_OK;
			else
				return INIT_FAILED;
			
			if( dmkdsk_GetRealDiskCode( header ) == 0x00000000 ) /* A real DMK disk will have these bytes zeroed */
				result = INIT_OK;
			else
				return INIT_FAILED;
		}
	}

	return result;
}

/* remove a disk from the drive specified by id */
void dmkdsk_floppy_exit(int id)
{
	dmkdsk *pDisk;

	/* sanity check */
	if ((id<0) || (id>=dmkdsk_MAX_DRIVES))
		return;

	pDisk = &dmkdsk_drives[id];

	/* if file was opened, close it */
	if (pDisk->image_file!=NULL)
	{
		osd_fclose(pDisk->image_file);
		pDisk->image_file = NULL;
	}
}

static void dmkdsk_seek_callback(int drive, int physical_track)
{
	dmkdsk *w = &dmkdsk_drives[drive];

	w->track = physical_track;

	#ifdef VERBOSE
	logerror("dmkdsk seek to track: %d\n", w->track );
	#endif
}


static int dmkdsk_get_sectors_per_track(int drive, int side)
{
	dmkdsk 			*w = &dmkdsk_drives[drive];
	UINT16			disp;
	dmkTrack_p		track;
	int				i = -1;

	track = GetEntireTrack( drive, w->track, side );
	if( track != NULL )
	{
		/* Interate thru track header looking for end of IDAM list */
		/* Track header is always sorted from first IDAM to last. With */
		/* any unused entries set to zero */

		for( i=0; i<80; i++ )
		{
			disp = LITTLE_ENDIANIZE_INT16(track->idamOffset[i]);
			disp &= 0x3fff;

			if( disp == 0 )
			{
				break;
			}
		}
		free( track );
	}

	#ifdef VERBOSE
	logerror("dmkdsk count sectors (track %d on side %d) result: %d\n", w->track, side, i);
	#endif

	return i;
}

static void dmkdsk_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	packedIDData_P	pSector;
	dmkdsk			*w = &dmkdsk_drives[drive];

	#ifdef VERBOSE
	logerror("dmkdsk Get_ID: Sector index %d on Track %d on Side %d\n", id_index, w->track, side);
	#endif

	pSector = GetPackedSector( drive, w->track, id_index, side );

	if( pSector != NULL )
	{
		/* construct the id value */

		id->C = pSector->trackNumber;
		id->H = pSector->sideNumber;
		id->R = pSector->sectorNumber;
	    id->N = pSector->lengthCode;

		id->data_id = id_index;
		id->flags = 0;

		logerror("dmkdsk return Get_ID: Sector %d on Track %d on Side %d\n", id->R, id->C, id->H);

		if ( CheckIDCRC( pSector ) )
		{
			id->flags |= ID_FLAG_CRC_ERROR_IN_ID_FIELD;
			logerror("dmkdsk return Get_ID: IDAM crc error\n");
		}

		if( CheckDataCRC( pSector ) )
		{
			id->flags |= ID_FLAG_CRC_ERROR_IN_DATA_FIELD;
			logerror("dmkdsk return Get_ID: Data crc error\n");
		}

		free( pSector );
	}
}

static void dmkdsk_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length)
{
	dmkdsk 			*w = &dmkdsk_drives[drive];
	packedIDData_P	pSector;
	int				size;

	pSector = GetPackedSector( drive, w->track, index1, side );

	if( pSector != NULL )
	{
		size = 128 << pSector->lengthCode;
		logerror("dmkdsk read sector: (requested length %d, sector length on disk %d)\n", length, size);

		memcpy( ptr, pSector->data, length );
		free( pSector );
	}
}

static void dmkdsk_read_track_data_info_buffer( int drive, int side, char *ptr, int *length )
{
	dmkTrack_p		temp_track;
	dmkdsk			*w = &dmkdsk_drives[drive];
	int				size;
	char			*pointer;
	
	temp_track = GetEntireTrack( drive, w->track, side );
	
	if ( temp_track != NULL )
	{
		size = dmkdsk_GetTrackLength( &(w->header) ) - sizeof( dmkTrack );
		
		if( size > *length )
			size = *length;
		else
			*length = size;
		
		pointer = (char *)(temp_track->trackData) + sizeof( dmkTrack );
		memcpy( ptr, pointer, size );
		
		free( temp_track );
	}
}

static void dmkdsk_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length,int ddam)
{
	dmkTrack_p	track_data;
	UINT16		disp, crc;
	int			i, size;
	dmkdsk 		*w = &dmkdsk_drives[drive];
	
	/* Get track */
	
	track_data = GetEntireTrack( drive, w->track, side );
	
	/* Overwrite sector */
	
	if( track_data != NULL )
	{
		disp = LITTLE_ENDIANIZE_INT16(track_data->idamOffset[index1]);
		disp &= 0x3fff;
		
		/* Check if sector index is out of bounds */
		if( disp == NULL )
		{
			logerror("dmkdsk write sector: (requested index not found (track %d, sector index %d, side %d)\n", w->track, index1, side);
			free( track_data );
			return;
		}
			
		if( track_data->trackData[ disp ] == 0xfe )
		{
			size = 128 << track_data->trackData[ disp+ 4 ];
			
			if ( size != length )
			{
				logerror("dmkdsk trying to write different length sector (sector index %d on track %d on side %d)\n", index1, w->track, side);
				free( track_data );
				return;
			}
			
			for( i=7; i<80; i++ )
			{
				if( memcmp( &(track_data->trackData[ disp + i ]), "\241\241\373", 3 ) == 0 ) 
				{
					memcpy( &(track_data->trackData[ disp + i + 3 ]), ptr, size );
					break;
				}
			}
			
			if( i == 40 )
			{
				logerror("dmkdsk data not found after IDAM (sector index %d on track %d on side %d)\n", index1, w->track, side);
				free( track_data );
				return;
			}	
		}

		/* Update sector crc */

		crc = 0xffff;
		crc = calc_crc1( crc, 0xa1 );
		crc = calc_crc1( crc, 0xa1 );
		crc = calc_crc1( crc, 0xa1 );
		crc = calc_crc1( crc, 0xfb );

		calc_crc_buffer( &crc, (UINT8 *)ptr, size );

#ifdef LSB_FIRST
		track_data->trackData[ disp + i + 3 + size + 0] = crc & 0x00ff;
		track_data->trackData[ disp + i + 3 + size + 1] = crc >> 8;
#else
		track_data->trackData[ disp + i + 3 + size + 0] = crc >> 8;
		track_data->trackData[ disp + i + 3 + size + 1] = crc & 0x00ff;
#endif		
		
		/* Write track back to file */
		
		SetEntireTrack( drive, w->track, side, track_data );
		
		free( track_data );
	}
}

static void SetEntireTrack( int drive, int track, int side, dmkTrack_p track_data )
{
	dmkdsk			*w = &dmkdsk_drives[drive];
	unsigned long	offset;

    if( track > w->header.trackCount)
	{
		logerror("dmkdsk writing track %d > %d\n", track, w->header.trackCount);
		return;
	}

    if( side > (DMKSIDECOUNT( w->header )) )
    {
		logerror("dmkdsk writing head %d > %d\n", side, DMKSIDECOUNT( w->header ));
		return;
	}
	
	/* Adjust for single sided disks */

	offset = sizeof( dmkHeader) + dmkdsk_GetTrackLength( &(w->header) ) * track + ( dmkdsk_GetTrackLength( &(w->header) ) * side );

	osd_fseek(w->image_file, offset, SEEK_SET);
	osd_fwrite(w->image_file, track_data, dmkdsk_GetTrackLength( &(w->header) ) );
}

static dmkTrack_p GetEntireTrack( int drive, int track, int side )
{
	dmkTrack_p		result;
	dmkdsk			*w = &dmkdsk_drives[drive];
	unsigned long	offset;

    if( track > w->header.trackCount)
	{
		logerror("dmkdsk requesting track %d > %d\n", track, w->header.trackCount);
		return 0;
	}

    if( side > (DMKSIDECOUNT( w->header )) )
    {
		logerror("dmkdsk requesting head %d > %d\n", side, DMKSIDECOUNT( w->header ));
		return 0;
	}

	/* Adjust for single sided disks */

	offset = sizeof( dmkHeader) + dmkdsk_GetTrackLength( &(w->header) ) * track + ( dmkdsk_GetTrackLength( &(w->header) ) * side );

	result = malloc( dmkdsk_GetTrackLength( &(w->header) ) );
	if ( result != NULL )
	{
		osd_fseek(w->image_file, offset, SEEK_SET);
		osd_fread(w->image_file, result, dmkdsk_GetTrackLength( &(w->header) ) );
	}

	return result;
}

static packedIDData_P GetPackedSector( int drive, int track, int id_index, int side )
{
	dmkTrack_p		track_data;
	int				i, size;
	packedIDData_P	result = NULL;
	UINT16			disp;

	track_data = GetEntireTrack( drive, track, side );

	if( track_data != NULL )
	{
		disp = LITTLE_ENDIANIZE_INT16(track_data->idamOffset[id_index]);
		disp &= 0x3fff;

		/* Check if sector index is out of bounds */
		if( disp == NULL )
		{
			logerror("dmkdsk read sector: (requested index not found (track %d, sector index %d, side %d)\n", track, id_index, side);
			free( track_data );
			return NULL;
		}

		if( track_data->trackData[ disp ] == 0xfe )
		{
			size = 128 << track_data->trackData[ disp+ 4 ];

			result = malloc( sizeof( packedIDData ) + size + 2 );

			if( result != NULL )
			{
				memset( result, 0x00, sizeof( packedIDData ) + size + 2 );
				/* Copy IDAM into packed array */
				memcpy( result, &(track_data->trackData[ disp ]), 7 );

				/* Sector IDAM found; now find start of data */
				/* Should be within 40 bytes after IDAM */

				for( i=7; i<80; i++ )
				{
					if( memcmp( &(track_data->trackData[ disp + i ]), "\241\241\373", 3 ) == 0 )
					{
						memcpy( &(result->DM), &(track_data->trackData[ disp + i + 2 ]), size + 3 );
						break;
					}
				}

				if( i == 40 )
				{
					logerror("dmkdsk data not found after IDAM (sector index %d on track %d on side %d)\n", id_index, track, side);
				}
			}
		}

		free( track_data );
	}

	return result;
}

static int CheckIDCRC( packedIDData_P pSector )
{
	UINT16	crc;
/*
	crc = 0xffff;
	crc = calc_crc1( crc, 0xa1 );
	crc = calc_crc1( crc, 0xa1 );
	crc = calc_crc1( crc, 0xa1 );
*/
	crc = 0xcdb4;	/* Seed crc with proper value */

	calc_crc_buffer( &crc, (unsigned char *)pSector, 5 );

	if( crc == dmkdsk_GetIDAMCRC( pSector ) )
		return 0;
	else
		return -1;
}

static int CheckDataCRC( packedIDData_P pSector )
{
	UINT16	crc, *crcOnDisk;
	int		size;

	size = 128 << pSector->lengthCode;
/*
	crc = 0xffff;
	crc = calc_crc1( crc, 0xa1 );
	crc = calc_crc1( crc, 0xa1 );
	crc = calc_crc1( crc, 0xa1 );
*/
	crc = 0xcdb4;	/* Seed crc with proper value */

	calc_crc_buffer( &crc, &(pSector->DM), size+1 );

	crcOnDisk = (UINT16 *)(&(pSector->data[ size ]));
	if( crc == BIG_ENDIANIZE_INT16( *crcOnDisk ) )
		return 0;
	else
		return -1;
}

static void calc_crc_buffer( UINT16 *crc, UINT8 *buffer, int size )
{
	int	i;

	for( i=0; i<size; i++ )
		*crc = calc_crc1( *crc, buffer[i] );
}