/*********************************************************************

	formats/ap2_disk.h

	Utility code for dealing with Apple II disk images

*********************************************************************/

#ifndef AP2_DISK_H
#define AP2_DISK_H

#include "mame.h"


/***************************************************************************

	Constants

***************************************************************************/

#define APPLE2_NIBBLE_SIZE	374
#define APPLE2_TRACK_COUNT	35
#define APPLE2_SECTOR_COUNT	16
#define APPLE2_SECTOR_SIZE	256

#define APPLE2_IMAGE_DO		0
#define APPLE2_IMAGE_PO		1



/***************************************************************************

	Prototypes

***************************************************************************/

int apple2_choose_image_type(const char *filetype);
int apple2_skew_sector(int sector, int image_type);
void apple2_disk_encode_nib(UINT8 *nibble, const UINT8 *data, int volume, int track, int sector);
int apple2_disk_decode_nib(UINT8 *data, const UINT8 *nibble, int *volume, int *track, int *sector);
void apple2_disk_nibble_test(void);

#endif /* AP2_DISK_H */
