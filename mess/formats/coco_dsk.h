/*********************************************************************

	formats/coco_dsk.h

	Tandy Color Computer / Dragon disk images

*********************************************************************/

#ifndef COCO_DSK_H
#define COCO_DSK_H

#include "formats/flopimg.h"


/**************************************************************************/

FLOPPY_OPTIONS_EXTERN(coco);

floperr_t coco_dmk_identify(floppy_image *floppy, int *vote);
floperr_t coco_dmk_construct(floppy_image *floppy, option_resolution *params);

#endif /* COCO_DSK_H */
