/*********************************************************************

	formats/basicdsk.h

	Floppy format code for basic disks

*********************************************************************/

#ifndef BASICDSK_H
#define BASICDSK_H

#include "formats/flopimg.h"

struct basicdsk_geometry
{
	int heads;
	int tracks;
	int sectors;
	int first_sector_id;
	UINT32 sector_length;
	UINT64 offset;

	int (*translate_sector)(floppy_image *floppy, int sector);
};

floperr_t basicdsk_construct(floppy_image *floppy, const struct basicdsk_geometry *geometry);

#endif /* BASICDSK_H */
