/*
	Code to interface the MESS image code with MAME's ide and harddisk cores.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

/*#include "harddisk.h"
#include "machine/idectrl.h"*/
#include "idedrive.h"

static mame_file *ide_fp;
static void *ide_hard_disk_handle;
static int ide_fp_wp;

static void *mess_hard_disk_open(const char *filename, const char *mode);
static void mess_hard_disk_close(void *file);
static UINT32 mess_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mess_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer);

struct hard_disk_interface mess_hard_disk_interface =
{
	mess_hard_disk_open,
	mess_hard_disk_close,
	mess_hard_disk_read,
	mess_hard_disk_write
};

/*
	MAME hard disk core interface
*/

/*
	mess_hard_disk_open - interface for opening a hard disk image
*/
static void *mess_hard_disk_open(const char *filename, const char *mode)
{
	/* read-only fp? */
	if (ide_fp_wp && ! (mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return ide_fp;
}

/*
	mess_hard_disk_close - interface for closing a hard disk image
*/
static void mess_hard_disk_close(void *file)
{
	//mame_fclose((mame_file *)file);
}

/*
	mess_hard_disk_read - interface for reading from a hard disk image
*/
static UINT32 mess_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}

/*
	mess_hard_disk_write - interface for writing to a hard disk image
*/
static UINT32 mess_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*
	MAME IDE core interface
*/

/*
	ide_hd_load()

	Load an IDE image

	id, fp, open_mode: parameters passed by the MESS image code to the load
		function
	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
int ide_hd_load(int id, mame_file *fp, int open_mode, int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	hard_disk_set_interface(& mess_hard_disk_interface);

	ide_fp = fp;
	ide_fp_wp = ! is_effective_mode_writable(open_mode);

	ide_hard_disk_handle = hard_disk_open(image_filename(IO_HARDDISK, id), is_effective_mode_writable(open_mode), NULL);
	if (ide_hard_disk_handle != NULL)
	{
		ide_controller_init_custom(which_bus, intf, ide_hard_disk_handle);
		ide_controller_reset(which_bus);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

/*
	ide_hd_unload()

	Unload an IDE image

	id: parameter passed by the MESS image code to the load function
	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
void ide_hd_unload(int id, int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	if (ide_hard_disk_handle)
	{
		hard_disk_close(ide_hard_disk_handle);
		ide_hard_disk_handle = NULL;
	}
	ide_fp = NULL;
	ide_controller_init_custom(which_bus, intf, NULL);
	ide_controller_reset(which_bus);
}

/*
	ide_machine_init()

	Perform machine initialization

	which_bus: IDE bus the drive is attached to (only bus 0 is supported now)
	which_address: address of the drive on the bus (0->master, 1->slave, only
		master is supported now)
	intf: ide_interface required by the idectrl.c core
*/
void ide_hd_machine_init(int which_bus, int which_address, struct ide_interface *intf)
{
	assert(which_address == 0);

	ide_controller_init_custom(which_bus, intf, NULL);
	ide_controller_reset(which_bus);
}
