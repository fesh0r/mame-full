/*********************************************************************

	mflopimg.h

	MESS interface to the floppy disk image abstraction code

*********************************************************************/

#ifndef MFLOPIMG_H
#define MFLOPIMG_H

#include "devices/flopdrv.h"
#include "formats/flopimg.h"

void floppy_device_getinfo(struct IODevice *dev, const struct FloppyFormat *floppy_options);

/* hack for apple II; replace this when we think of something better */
void floppy_install_unload_proc(mess_image *image, void (*proc)(mess_image *image));

/* hack for TI99; replace this when we think of something better */
void floppy_install_tracktranslate_proc(mess_image *image, int (*proc)(mess_image *image, floppy_image *floppy, int physical_track));

#endif /* MFLOPIMG_H */



