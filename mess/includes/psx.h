#include "driver.h"

/*
 * Modification For OpenVMS By:  Robert Alan Byer
 *                               byer@mail.ourservers.net
 *                               December 10, 2003
 */
#if defined(__DECC) && defined(VMS)
#  include "../../src/includes/psx.h"
#else
#  include "../src/includes/psx.h"
#endif

READ32_HANDLER( psx_cd_r );
WRITE32_HANDLER( psx_cd_w );

READ32_HANDLER( psx_serial_r );
WRITE32_HANDLER( psx_serial_w );
