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

FLOPPY_OPTIONS_EXTERN(apple35);


#endif /* AP_DSK35_H */
