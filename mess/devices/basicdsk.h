/* DISK IMAGE FORMAT WHICH USED TO BE PART OF WD179X - NOW SEPERATED */

#ifndef BASICDSK_H
#define BASICDSK_H

#include "driver.h"
#include "devices/flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
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

    UINT8   N;
	UINT16	sector_length;			/* sector length (byte) */

	int		track_divider;			/* 2 if using a 40-track image in a 80-track drive, 1 otherwise */

	/* a bit for each sector in the image. If the bit is set, this sector
	has a deleted data address mark. If the bit is not set, this sector
	has a data address mark */
	UINT8	*ddam_map;
	unsigned long ddam_map_size;
} basicdsk;

/* init */
int basicdsk_floppy_init(int id);
int basicdsk_floppy_load(int id, mame_file *fp, int open_mode);
void basicdsk_floppy_unload(int id);

/* set the disk image geometry for the specified drive */
/* this is required to read the disc image correct */

/* geometry details:

  If a disk image is specified with 2 sides, they are stored as follows:

  track 0 side 0, track 0 side 1, track 1 side 0.....

  If a disk image is specified with 1 sides, they are stored as follows:

  track 0 side 0, track 1 side 0, ....

  tracks = the number of tracks stored in the image
  sec_per_track = the number of sectors on each track
  sector_length = size of sector (must be power of 2) e.g. 128,256,512 bytes
  first_sector_id = this is the sector id base. If there are 10 sectors, with the first_sector_id is 1,
  the sector id's will be:

	1,2,3,4,5,6,7,8,9,10

  track_skipping = set to TRUE if a 40-track image has been inserted in a 80-track drive, FALSE otherwise
*/

void basicdsk_set_geometry(UINT8 drive, UINT16 tracks, UINT8 sides, UINT8 sec_per_track, UINT16 sector_length/*, UINT16 dir_sector, UINT16 dir_length*/, UINT8 first_sector_id, UINT16 offset_track_zero, int track_skipping);

/* set data mark/deleted data mark for the sector specified. If ddam!=0, the sector will
have a deleted data mark, if ddam==0, the sector will have a data mark */
void basicdsk_set_ddam(UINT8 physical_drive, UINT8 physical_track, UINT8 physical_side, UINT8 sector_id,UINT8 ddam);


#define CONFIG_DEVICE_FLOPPY_BASICDSK(count, file_extensions, load)		\
	CONFIG_DEVICE_BASE(IO_FLOPPY, (count), (file_extensions), DEVICE_LOAD_RESETS_NONE,	\
		OSD_FOPEN_RW_CREATE_OR_READ, basicdsk_floppy_init, NULL, (load), basicdsk_floppy_unload, NULL, NULL,\
		NULL, NULL, floppy_status, NULL, NULL, NULL, NULL, NULL, NULL)	\

#ifdef __cplusplus
}
#endif

#endif /* BASICDSK_H */
