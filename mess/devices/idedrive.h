#include "machine/idectrl.h"

int ide_hd_load(int id, mame_file *fp, int open_mode, int which_bus, int which_address, struct ide_interface *intf);
void ide_hd_unload(int id, int which_bus, int which_address, struct ide_interface *intf);
void ide_hd_machine_init(int which_bus, int which_address, struct ide_interface *intf);
