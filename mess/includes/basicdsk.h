
/* DISK IMAGE FORMAT WHICH USED TO BE PART OF WD179X - NOW SEPERATED */

#include "flopdrv.h"

typedef struct {
	UINT8	 track;
	UINT8	 sector;
	UINT8	 status;
}	SECMAP;

typedef struct 
{
	const char *image_name; 		/* file name for disc image */
	void	*image_file;			/* file handle for disc image */
	int 	mode;					/* open mode == 0 read only, != 0 read/write */
	unsigned long image_size;		/* size of image file */

	SECMAP	*secmap;

	UINT8	unit;					/* unit number if image_file == REAL_FDD */

	UINT8	tracks; 				/* maximum # of tracks */
	UINT8	heads;					/* maximum # of heads */

	UINT16	offset; 				/* track 0 offset */
	UINT8	first_sector_id;		/* id of first sector */
	UINT8	sec_per_track;			/* sectors per track */

	UINT8	head;					/* current head # */
	UINT8	track;					/* current track # */

	UINT8	sector_dam; 			/* current sector # to fake read DAM command */
        UINT8   N; 
	UINT16	dir_sector; 			/* directory track for deleted DAM */
	UINT16	dir_length; 			/* directory length for deleted DAM */
	UINT16	sector_length;			/* sector length (byte) */

} basicdsk;

/* init */
int     basicdsk_floppy_init(int id);
/* exit */
void basicdsk_floppy_exit(int id);
/* id */
int     basicdsk_floppy_id(int id);

void basicdsk_set_geometry(UINT8 drive, UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT16 dir_sector, UINT16 dir_length, UINT8 first_sector_id);


