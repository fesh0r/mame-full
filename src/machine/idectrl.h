/***************************************************************************

	Generic (PC-style) IDE controller implementation

***************************************************************************/

#include "harddisk.h"


#define MAX_IDE_CONTROLLERS			1

struct ide_interface
{
	void 	(*interrupt)(int state);
};

int ide_controller_init(int which, struct ide_interface *intf);
int ide_controller_init_custom(int which, struct ide_interface *intf, void *diskhandle);
void ide_controller_reset(int which);
UINT8 *ide_get_features(int which);

int ide_bus_0_r(int select, int offset);
void ide_bus_0_w(int select, int offset, int data);

READ32_HANDLER( ide_controller32_0_r );
WRITE32_HANDLER( ide_controller32_0_w );
READ32_HANDLER( ide_bus_master32_0_r );
WRITE32_HANDLER( ide_bus_master32_0_w );

READ16_HANDLER( ide_controller16_0_r );
WRITE16_HANDLER( ide_controller16_0_w );
READ16_HANDLER( ide_bus_master16_0_r );
WRITE16_HANDLER( ide_bus_master16_0_w );
