//============================================================
//
//	configms.c - Win32 MESS specific options
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MESS headers
#include "driver.h"
#include "rc.h"
#include "parallel.h"
#include "menu.h"
#include "device.h"
#include "configms.h"

//============================================================
//	IMPORTS
//============================================================

// from config.c
extern struct rc_struct *rc;

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_write_config;

//============================================================
//	LOCAL VARIABLES
//============================================================

static const char *dev_opts;

static int add_device(struct rc_option *option, const char *arg, int priority);
static int specify_ram(struct rc_option *option, const char *arg, int priority);

struct rc_option mess_opts[] =
{
	/* FIXME - these option->names should NOT be hardcoded! */
	{ "MESS specific options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "newui",			"nu",   rc_bool,	&options.disable_normal_ui,	"1", 0, 0, NULL,			"use the new MESS UI" },
	{ "cartridge",		"cart", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to cartridge device" },
	{ "floppydisk",		"flop", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to floppy disk device" },
	{ "harddisk",		"hard", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to hard disk device" },
	{ "cylinder",		"cyln", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to cylinder device" },
	{ "cassette",		"cass", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to cassette device" },
	{ "punchcard",		"pcrd", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to punch card device" },
	{ "punchtape",		"ptap", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to punch tape device" },
	{ "printer",		"prin", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to printer device" },
	{ "serial",			"serl", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to serial device" },
	{ "parallel",		"parl", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to parallel device" },
	{ "snapshot",		"dump", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to snapshot device" },
	{ "quickload",		"quik", rc_string,	&dev_opts,					NULL, 0, 0, add_device,		"attach software to quickload device" },
	{ "ramsize",		"ram",  rc_string,	&dev_opts,					NULL, 0, 0, specify_ram,	"size of RAM (if supported by driver)" },
	{ "threads",		"thr",  rc_int,		&win_task_count,			NULL, 0, 0, NULL,			"number of threads to use for parallel operations" },
	{ "natural",		"nat",  rc_bool,	&win_use_natural_keyboard,	NULL, 0, 0, NULL,			"specifies whether to use a natural keyboard or not" },
	{ "min_width",		"mw",   rc_int,		&options.min_width,			"200", 0, 0, NULL,			"specifies the minimum width for the display" },
	{ "min_height",		"mh",   rc_int,		&options.min_height,		"200", 0, 0, NULL,			"specifies the minimum height for the display" },
/*	{ "writeconfig",	"wc",	rc_bool,	&win_write_config,			NULL, 0, 0, NULL,			"writes configuration to (driver).ini on exit" }, */
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

//============================================================



/*
 * gamedrv  = NULL --> write named configfile
 * gamedrv != NULL --> write gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int write_config (const char* filename, const struct GameDriver *gamedrv)
{
	mame_file *f;
	char buffer[128];
	int retval = 1;

	if (gamedrv)
	{
		sprintf(buffer, "%s.ini", gamedrv->name);
		filename = buffer;
	}

	f = mame_fopen (buffer, NULL, FILETYPE_INI, 1);
	if (!f)
		goto done;

	if (osd_rc_write(rc, f, filename))
		goto done;

	retval = 0;

done:
	if (f)
		mame_fclose(f);
	return retval;
}

/*	add_device() is called when the MESS CLI option has been identified
 *	This searches throught the devices{} struct array to grab the ID of the
 *	option, which then registers the device using register_device()
 */
static int add_device(struct rc_option *option, const char *arg, int priority)
{
	int id;
	id = device_typeid(option->name);
	if (id < 0)
	{
		/* If we get to here, log the error - This is mostly due to a mismatch in the array */
		logerror("Command Line Option [-%s] not a valid device - ignoring\n", option->name);
		return -1;
	}

	/* A match!  we now know the ID of the device */
	option->priority = priority;
	return register_device(id, arg);
}

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

#ifdef MAME_DEBUG
int messopts_valididty_checks(void)
{
	int i;
	int id_fromshortname;
	int id_fromlongname;
	int error = 0;

	for (i = 0; i < sizeof(mess_opts) / sizeof(mess_opts[0]); i++)
	{
		if(mess_opts[i].func == add_device)
		{
			id_fromlongname = device_typeid(mess_opts[i].name);
			if (id_fromlongname < 0)
			{
				printf("option -%s is supposed to load a device, but it does not\n", mess_opts[i].name);
				error = 1;
			}

			id_fromshortname = device_typeid(mess_opts[i].shortname);
			if (id_fromshortname < 0)
			{
				printf("option -%s is supposed to load a device, but it does not\n", mess_opts[i].shortname);
				error = 1;
			}

			if (id_fromlongname != id_fromshortname)
			{
				printf("options -%s and -%s are supposed to map to the same device\n", mess_opts[i].name, mess_opts[i].shortname);
				error = 1;
			}
		}
	}
	return error;
}

#endif /* MAME_DEBUG */
