/*********************************************************************

	chd_cd.h

	MESS interface to the MAME CHD CDROM code

*********************************************************************/

#ifndef MESS_CD_H
#define MESS_CD_H

#include "cdrom.h"
#include "image.h"
#include "messdrv.h"

DEVICE_INIT( mess_cd );
DEVICE_LOAD( mess_cd );
DEVICE_UNLOAD( mess_cd );

const struct IODevice *mess_cd_device_specify(struct IODevice *iodev, int count);

cdrom_file *mess_cd_get_cdrom_file(mess_image *image);
chd_file *mess_cd_get_chd_file(mess_image *image);

void cdrom_device_getinfo(struct IODevice *iodev);

cdrom_file *mess_cd_get_cdrom_file_by_number(int drivenum);

#endif /* MESS_CD_H */
