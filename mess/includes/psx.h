#include "driver.h"

READ32_HANDLER( psx_cd_r );
WRITE32_HANDLER( psx_cd_w );

READ32_HANDLER( psx_serial_r );
WRITE32_HANDLER( psx_serial_w );

void psx_irq_trigger( UINT32 data );
void psx_irq_clear( UINT32 data );

