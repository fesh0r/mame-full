/*********************************************************************

	ap_dsk35.h

	Apple 3.5" disk images

*********************************************************************/

#ifndef AP_DSK35_H
#define AP_DSK35_H

#include "formats/flopimg.h"

void sony_filltrack(UINT8 *buffer, size_t buffer_len, size_t *pos, UINT8 data);
UINT8 sony_fetchtrack(const UINT8 *buffer, size_t buffer_len, size_t *pos);

/**************************************************************************/

extern const UINT8 apple35_tracklen_800kb[];

FLOPPY_OPTIONS_EXTERN(apple35_mac);
FLOPPY_OPTIONS_EXTERN(apple35_iigs);


#endif /* AP_DSK35_H */
