/*
	Code to interface the MESS image code with MAME's ide core.

	Raphael Nabet 2003
*/

#include "machine/idectrl.h"
#include "devices/harddriv.h"
#include "idedrive.h"

/*
	ide_hd_init()

	Init an IDE hard disk device

	img: parameter passed by the MESS image code to the init function
	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
int ide_hd_init(mess_image *img, int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	if (device_init_mess_hd(img) == INIT_PASS)
	{
		ide_controller_init_custom(which_bus, intf, NULL);
		ide_controller_reset(which_bus);
	}

	return INIT_PASS;
}

/*
	ide_hd_load()

	Load an IDE hard disk image

	img: parameter passed by the MESS image code to the load function
	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
int ide_hd_load(mess_image *img, int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	if (device_load_mess_hd(img, image_fp(img)) == INIT_PASS)
	{
		ide_controller_init_custom(which_bus, intf, mess_hd_get_chd_file(img));
		ide_controller_reset(which_bus);
		return INIT_PASS;
	}	

	return INIT_FAIL;
}

/*
	ide_hd_unload()

	Unload an IDE hard disk image

	img: parameter passed by the MESS image code to the load function
	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
void ide_hd_unload(mess_image *img, int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	device_unload_mess_hd(img);
	ide_controller_init_custom(which_bus, intf, NULL);
	ide_controller_reset(which_bus);
}

/*
	ide_machine_init()

	Perform machine initialization for an IDE hard disk device

	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
void ide_hd_machine_init(int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	ide_controller_reset(which_bus);
}
