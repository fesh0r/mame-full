/*********************************************************************

	dmkdsk.h

	David M. Keil disk format.
	
	tim lindner, 2001
	tlindner@ix.netcom.com

*********************************************************************/

#include "flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma warn_padding on
#pragma options align= packed

typedef struct
{
	UINT8	IDAM;
	UINT8	trackNumber;
	UINT8	sideNumber;
	UINT8	sectorNumber;
	UINT8	lengthCode;
	UINT16	idCRC;
	UINT8	DM;
	UINT8	data[];
} packedIDData, *packedIDData_P;

typedef struct
{
	UINT8		writeProtect; /* 0xff = writed protected, 0x00 = OK to write	*/
	UINT8		trackCount;   /* Ones based 									*/
	UINT16		trackLength;  /* This number includes artifical track header
								 (little endian in file)						*/
	UINT8		diskOptions;  /* Bit 0: Unused. 								*/
							  /* Bit 1: Unused.									*/
							  /* Bit 2: Unused.									*/
							  /* Bit 3: Unused.									*/
							  /* Bit 4: Single sided?							*/
							  /* Bit 5: Unused.									*/
							  /* Bit 6: Single density?							*/
							  /* Bit 7: Ignore density flags?
							  	 (always write one byte, depreciated)			*/
	UINT8		reserved[7];
	UINT32		realDiskCode; /* If this is 0x12345678 (little endian)
							     then access a real disk drive (unsurported)	*/
} dmkHeader, *dmkHeader_p;

#pragma options align=reset
#pragma warn_padding reset

typedef struct 
{
	const char	*image_name; 		/* file name for disc image					*/
	void		*image_file;		/* file handle for disc image */
	int			mode;				/* open mode == 0 read only,!= 0 read/write	*/
	unsigned	long image_size;	/* size of image file						*/

	UINT8		unit;				/* unit number if image_file == REAL_FDD	*/

    dmkHeader	header;				/* DMK header from file						*/
	UINT8		head;				/* current head #							*/
	UINT8		track;				/* current track #							*/

} dmkdsk;

typedef union
{

	UINT16		idamOffset[80];	/* Note: little endian in file				*/
								/* Bit 15: Sector double density?			*/
								/* Bit 14: Undefined (reserved)				*/
								/* Bit 13-0: Offset from begining of track
									         header to 'FE' byte of IDAM
									         Note these are always sorted
									         from first to last. All empty 
									         entires are 0x00				*/
	UINT8		trackData[];	/* Actual track data (including header)		*/
} dmkTrack, *dmkTrack_p;

#define DMKSIDECOUNT( x )  ((x.diskOptions & 0x10) == 0) ? 0 : 1

/* init */
int     dmkdsk_floppy_init(int id);
/* exit and free up data */
void    dmkdsk_floppy_exit(int id);
/* id */
int     dmkdsk_floppy_id(int id);

#ifdef __cplusplus
}
#endif
