/*********************************************************************

	harddriv.h

	MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "harddisk.h"
#include "image.h"
#include "messdrv.h"

DEVICE_INIT(mess_hd);
DEVICE_LOAD(mess_hd);
DEVICE_UNLOAD(mess_hd);

const struct IODevice *mess_hd_device_specify(struct IODevice *iodev, int count);

struct hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image);
struct chd_file *mess_hd_get_chd_file(mess_image *image);


#define CONFIG_DEVICE_HARDDISK(count)						\
	if (cfg->device_num-- == 0)								\
	{														\
		static struct IODevice iodev;						\
		cfg->dev = mess_hd_device_specify(&iodev, count);	\
	}														\

#endif /* MESS_HD_H */
