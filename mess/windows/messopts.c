#include "driver.h"
#include "rc.h"

/* add_device() is called when the MESS CLI option has been identified 		*/
/* This searches throught the devices{} struct array to grab the ID of the 	*/
/* option, which then registers the device using register_device()			*/
static int add_device(struct rc_option *option, const char *arg, int priority)
{
	extern const struct Devices devices[]; /* from mess device.c */
	int i=0;

	/* First, we need to find the ID of this option - kludge alert!			*/
	while(devices[i].id != IO_COUNT)
	{
		if (!stricmp(option->name, devices[i].name)  		||
			!stricmp(option->shortname, devices[i].shortname))
		{
			/* A match!  we now know the ID of the device */
			option->priority = priority;
			return (register_device (devices[i].id, arg));
		}
		else
		{
			/* No match - keep looking */
			i++;
		}
	}

	/* If we get to here, log the error - This is mostly due to a mismatch in the array */
	logerror("Command Line Option [-%s] not a valid device - ignoring\n", option->name);
    return -1;

}
static const char *dev_opts;

static int specify_ram(struct rc_option *option, const char *arg, int priority)
{
	UINT32 specified_ram;

	specified_ram = ram_parse_string(arg);
	if (specified_ram == 0)
	{
		fprintf(stderr, "Cannot recognize the RAM option %s; aborting\n", arg);
		return -1;
	}
	options.ram = specified_ram;
	return 0;
}

struct rc_option mess_opts[] = {
	/* FIXME - these option->names should NOT be hardcoded! */
	{ "MESS specific options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "cartridge", "cart", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to cartridge device" },
	{ "floppydisk","flop", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to floppy disk device" },
	{ "harddisk",  "hard", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to hard disk device" },
	{ "cylinder",  "cyln", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to cylinder device" },
	{ "cassette",  "cass", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to cassette device" },
	{ "punchcard", "pcrd", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to punch card device" },
	{ "punchtape", "ptap", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to punch tape device" },
	{ "printer",   "prin", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to printer device" },
	{ "serial",    "serl", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to serial device" },
	{ "parallel",  "parl", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to parallel device" },
	{ "snapshot",  "dump", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to snapshot device" },
	{ "quickload", "quik", rc_string, &dev_opts, NULL, 0, 0, add_device, "Attach software to quickload device" },
	{ "ramsize",   "ram",  rc_string, &dev_opts, NULL, 0, 0, specify_ram, "Specifies size of RAM (if supported by driver)" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

