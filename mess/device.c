/***************************************************************************

	device.c

	Definitions and manipulations for device structures

***************************************************************************/

#include <assert.h>
#include "device.h"
#include "ui_text.h"


/*************************************
 *
 *	Names and shortnames
 *
 *************************************/

struct Devices
{
	iodevice_t type;
	const char *name;
	const char *shortname;
};

/* The List of Devices, with Associated Names - Be careful to ensure that   *
 * this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
static const struct Devices device_info_array[] =
{
	{ IO_CARTSLOT,	"cartridge",	"cart" }, /*  0 */
	{ IO_FLOPPY,	"floppydisk",	"flop" }, /*  1 */
	{ IO_HARDDISK,	"harddisk",		"hard" }, /*  2 */
	{ IO_CYLINDER,	"cylinder",		"cyln" }, /*  3 */
	{ IO_CASSETTE,	"cassette",		"cass" }, /*  4 */
	{ IO_PUNCHCARD,	"punchcard",	"pcrd" }, /*  5 */
	{ IO_PUNCHTAPE,	"punchtape",	"ptap" }, /*  6 */
	{ IO_PRINTER,	"printer",		"prin" }, /*  7 */
	{ IO_SERIAL,	"serial",		"serl" }, /*  8 */
	{ IO_PARALLEL,	"parallel",		"parl" }, /*  9 */
	{ IO_SNAPSHOT,	"snapshot",		"dump" }, /* 10 */
	{ IO_QUICKLOAD,	"quickload",	"quik" }, /* 11 */
	{ IO_MEMCARD,	"memcard",		"memc" }, /* 12 */
};

const char *device_typename(iodevice_t type)
{
	assert(type >= 0);
	assert(type < IO_COUNT);
	return device_info_array[type].name;
}



const char *device_brieftypename(iodevice_t type)
{
	assert(type >= 0);
	assert(type < IO_COUNT);
	return device_info_array[type].shortname;
}



int device_typeid(const char *name)
{
	int i;
	for (i = 0; i < sizeof(device_info_array) / sizeof(device_info_array[0]); i++)
	{
		if (!strcmpi(name, device_info_array[i].name) || !strcmpi(name, device_info_array[i].shortname))
			return i;
	}
	return -1;
}



/*************************************
 *
 *	Device structure construction and destruction
 *
 *************************************/

static const char *default_device_name(const struct IODevice *dev, int id,
	char *buf, size_t bufsize)
{
	const char *name;

	name = ui_getstring((UI_cartridge - IO_CARTSLOT) + dev->type);
	if (dev->count > 1)
	{
		/* for the average user counting starts at #1 ;-) */
		snprintf(buf, bufsize, "%s #%d", name, id + 1);
		name = buf;
	}
	return name;
}



static void default_device_getdispositions(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable)
{
	*readable = dev->readable;
	*writeable = dev->writeable;
	*creatable = dev->creatable;
}



struct IODevice *devices_allocate(const struct GameDriver *gamedrv)
{
	struct SystemConfigurationParamBlock params;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	int count, i;
	struct IODevice *devices = NULL;

	memset(handlers, 0, sizeof(handlers));
	memset(count_overrides, 0, sizeof(count_overrides));

	if (gamedrv->sysconfig_ctor)
	{
		memset(&params, 0, sizeof(params));
		params.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
		params.device_handlers = handlers;
		params.device_countoverrides = count_overrides;
		gamedrv->sysconfig_ctor(&params);
	}

	/* count the amount of handlers that we have available */
	for (count = 0; handlers[count]; count++)
		;
	count++; /* for our purposes, include the tailing empty device */

	devices = (struct IODevice *) malloc(count * sizeof(struct IODevice));
	if (!devices)
		goto error;
	memset(devices, 0, count * sizeof(struct IODevice));

	for (i = 0; i < count; i++)
	{
		devices[i].type = IO_COUNT;

		if (handlers[i])
		{
			handlers[i](&devices[i]);
			
			/* overriding the count? */
			if (count_overrides[i])
				devices[i].count = count_overrides[i];

			/* any problems? */
			if ((devices[i].type < 0) || (devices[i].type >= IO_COUNT))
				goto error;
			if ((devices[i].count < 0) || (devices[i].count > MAX_DEV_INSTANCES))
				goto error;
			if (devices[i].error)
				goto error;

			/* fill in defaults */
			if (!devices[i].name)
				devices[i].name = default_device_name;
			if (!devices[i].name)
				devices[i].getdispositions = default_device_getdispositions;
		}
	}

	return devices;

error:
	if (devices)
		free(devices);
	return NULL;
}



/*************************************
 *
 *	Diagnostics
 *
 *************************************/

const struct IODevice *device_find(const struct IODevice *devices, iodevice_t type)
{
	int i;
	for (i = 0; devices[i].type != IO_COUNT; i++)
	{
		if (devices[i].type == type)
			return &devices[i];
	}
	return NULL;
}



/* this function is deprecated */
int device_count(iodevice_t type)
{
	const struct IODevice *dev;

	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		if (dev->type == type)
			return dev->count;
	}
	return 0;
}



/*************************************
 *
 *	Diagnostics
 *
 *************************************/

int device_valididtychecks(void)
{
	int error = 0;
	int i;

	if ((sizeof(device_info_array) / sizeof(device_info_array[0])) != IO_COUNT)
	{
		printf("device_info_array array should match size of IO_* enum\n");
		error = 1;
	}

	/* Check the device struct array */
	for (i = 0; i < sizeof(device_info_array) / sizeof(device_info_array[0]); i++)
	{
		if (device_info_array[i].type != i)
		{
			printf("Device struct array order mismatch\n");
			error = 1;
			break;
		}

		if (!device_info_array[i].name)
		{
			printf("device_info_array[%d].name appears to be NULL\n", i);
			error = 1;
		}

		if (!device_info_array[i].shortname)
		{
			printf("device_info_array[%d].shortname appears to be NULL\n", i);
			error = 1;
		}
	}
	return error;
}

