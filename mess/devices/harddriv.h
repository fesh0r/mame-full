/*********************************************************************

	harddriv.h

	MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "harddisk.h"
#include "image.h"
#include "messdrv.h"

DEVICE_INIT( mess_hd );
DEVICE_LOAD( mess_hd );
DEVICE_UNLOAD( mess_hd );

const struct IODevice *mess_hd_device_specify(struct IODevice *iodev, int count);

hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image);
hard_disk_file *mess_hd_get_hard_disk_file_by_number(int drivenum);
chd_file *mess_hd_get_chd_file(mess_image *image);

void harddisk_device_getinfo(struct IODevice *iodev);

#endif /* MESS_HD_H */
