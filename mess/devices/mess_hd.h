/*********************************************************************

	mess_hd.h

	MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "harddisk.h"
#include "image.h"

DEVICE_INIT(mess_hd);
DEVICE_LOAD(mess_hd);
DEVICE_UNLOAD(mess_hd);

struct hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image);
struct chd_file *mess_hd_get_chd_file(mess_image *image);

int mess_hd_is_writable(mess_image *image);

#endif /* MESS_HD_H */
