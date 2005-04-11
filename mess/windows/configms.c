//============================================================
//
//	configms.c - Win32 MESS specific options
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>

// MESS headers
#include "driver.h"
#include "windows/rc.h"
#include "parallel.h"
#include "menu.h"
#include "device.h"
#include "configms.h"
#include "mscommon.h"
#include "pool.h"

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

static char *dev_opts[IO_COUNT];
static char *dev_dirs[IO_COUNT];
static char *ramsize_opt;

static int add_device(struct rc_option *option, const char *arg, int priority);
static int specify_ram(struct rc_option *option, const char *arg, int priority);

struct rc_option mess_opts[] =
{
	/* FIXME - these option->names should NOT be hardcoded! */
	{ "MESS specific options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "newui",			"nu",   rc_bool,	&options.disable_normal_ui,	"1", 0, 0, NULL,			"use the new MESS UI" },
	{ "cartridge",		"cart", rc_string,	&dev_opts[IO_CARTSLOT],		NULL, 0, 0, add_device,		"attach software to cartridge device" },
	{ "floppydisk",		"flop", rc_string,	&dev_opts[IO_FLOPPY],		NULL, 0, 0, add_device,		"attach software to floppy disk device" },
	{ "harddisk",		"hard", rc_string,	&dev_opts[IO_HARDDISK],		NULL, 0, 0, add_device,		"attach software to hard disk device" },
	{ "cylinder",		"cyln", rc_string,	&dev_opts[IO_CYLINDER],		NULL, 0, 0, add_device,		"attach software to cylinder device" },
	{ "cassette",		"cass", rc_string,	&dev_opts[IO_CASSETTE],		NULL, 0, 0, add_device,		"attach software to cassette device" },
	{ "punchcard",		"pcrd", rc_string,	&dev_opts[IO_PUNCHCARD],	NULL, 0, 0, add_device,		"attach software to punch card device" },
	{ "punchtape",		"ptap", rc_string,	&dev_opts[IO_PUNCHTAPE],	NULL, 0, 0, add_device,		"attach software to punch tape device" },
	{ "printer",		"prin", rc_string,	&dev_opts[IO_PRINTER],		NULL, 0, 0, add_device,		"attach software to printer device" },
	{ "serial",			"serl", rc_string,	&dev_opts[IO_SERIAL],		NULL, 0, 0, add_device,		"attach software to serial device" },
	{ "parallel",		"parl", rc_string,	&dev_opts[IO_PARALLEL],		NULL, 0, 0, add_device,		"attach software to parallel device" },
	{ "snapshot",		"dump", rc_string,	&dev_opts[IO_SNAPSHOT],		NULL, 0, 0, add_device,		"attach software to snapshot device" },
	{ "quickload",		"quik", rc_string,	&dev_opts[IO_QUICKLOAD],	NULL, 0, 0, add_device,		"attach software to quickload device" },
	{ "memcard",		"memc", rc_string,	&dev_opts[IO_MEMCARD],		NULL, 0, 0, add_device,		"attach image to memcard device" },
	{ "cartridge_dir",	NULL,   rc_string,	&dev_dirs[IO_CARTSLOT],		NULL, 0, 0, NULL,			"default directory for cartridge devices" },
	{ "floppydisk_dir",	NULL,   rc_string,	&dev_dirs[IO_FLOPPY],		NULL, 0, 0, NULL,			"default directory for floppy disk device" },
	{ "harddisk_dir",	NULL,   rc_string,	&dev_dirs[IO_HARDDISK],		NULL, 0, 0, NULL,			"default directory for hard disk devices" },
	{ "cylinder_dir",	NULL,   rc_string,	&dev_dirs[IO_CYLINDER],		NULL, 0, 0, NULL,			"default directory for cylinder devices" },
	{ "cassette_dir",	NULL,   rc_string,	&dev_dirs[IO_CASSETTE],		NULL, 0, 0, NULL,			"default directory for cassette devices" },
	{ "punchcard_dir",	NULL,   rc_string,	&dev_dirs[IO_PUNCHCARD],	NULL, 0, 0, NULL,			"default directory for punch card devices" },
	{ "punchtape_dir",	NULL,   rc_string,	&dev_dirs[IO_PUNCHTAPE],	NULL, 0, 0, NULL,			"default directory for punch tape devices" },
	{ "printer_dir",	NULL,   rc_string,	&dev_dirs[IO_PRINTER],		NULL, 0, 0, NULL,			"default directory for printer devices" },
	{ "serial_dir",		NULL,   rc_string,	&dev_dirs[IO_SERIAL],		NULL, 0, 0, NULL,			"default directory for serial devices" },
	{ "parallel_dir",	NULL,   rc_string,	&dev_dirs[IO_PARALLEL],		NULL, 0, 0, NULL,			"default directory for parallel devices" },
	{ "snapshot_dir",	NULL,   rc_string,	&dev_dirs[IO_SNAPSHOT],		NULL, 0, 0, NULL,			"default directory for snapshot devices" },
	{ "quickload_dir",	NULL,   rc_string,	&dev_dirs[IO_QUICKLOAD],	NULL, 0, 0, NULL,			"default directory for quickload devices" },
	{ "memcard_dir",	NULL,   rc_string,	&dev_dirs[IO_MEMCARD],		NULL, 0, 0, NULL,			"default directory for memcard devices" },
	{ "ramsize",		"ram",  rc_string,	&ramsize_opt,				NULL, 0, 0, specify_ram,	"size of RAM (if supported by driver)" },
	{ "threads",		"thr",  rc_int,		&win_task_count,			NULL, 0, 0, NULL,			"number of threads to use for parallel operations" },
	{ "natural",		"nat",  rc_bool,	&win_use_natural_keyboard,	NULL, 0, 0, NULL,			"specifies whether to use a natural keyboard or not" },
	{ "min_width",		"mw",   rc_int,		&options.min_width,			"200", 0, 0, NULL,			"specifies the minimum width for the display" },
	{ "min_height",		"mh",   rc_int,		&options.min_height,		"200", 0, 0, NULL,			"specifies the minimum height for the display" },
	{ "writeconfig",	"wc",	rc_bool,	&win_write_config,			NULL, 0, 0, NULL,			"writes configuration to (driver).ini on exit" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

//============================================================



/*
 * gamedrv  = NULL --> write named configfile
 * gamedrv != NULL --> write gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int write_config(const char* filename, const struct GameDriver *gamedrv)
{
	mame_file *f;
	char buffer[128];
	int retval = 1;

	if (gamedrv)
	{
		sprintf(buffer, "%s.ini", gamedrv->name);
		filename = buffer;
	}

	f = mame_fopen(buffer, NULL, FILETYPE_INI, 1);
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



static int use_filename_quotes(const char *filename)
{
	while(*filename)
	{
		if (isspace(*filename))
			return TRUE;
		filename++;
	}
	return FALSE;
}



void osd_begin_final_unloading(void)
{
	int count, id;
	size_t blocksize;
	char *s;
	mess_image *img;
	const char *filename;
	size_t len;
	const struct IODevice *dev;

	memset(dev_opts, 0, sizeof(dev_opts));

	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		for (count = dev->count; count > 0; count--)
		{
			img = image_from_device_and_index(dev, count - 1);
			if (image_exists(img))
				break;
		}

		if (count > 0)
		{
			// compute length of string
			blocksize = 0;
			for (id = 0; id < count; id++)
			{
				img = image_from_device_and_index(dev, id);

				if (image_exists(img))
				{
					filename = image_filename(img);
					blocksize += strlen(filename);
					if (use_filename_quotes(filename))
						blocksize += 2;
				}
				blocksize++;
			}

			s = malloc(blocksize);
			if (!s)
				return;
			*s = '\0';

			for (id = 0; id < count; id++)
			{
				img = image_from_device_and_index(dev, id);

				if (image_exists(img))
				{
					filename = image_filename(img);
					if (use_filename_quotes(filename))
						strcat(s, "\"");
					strcat(s, filename);
					if (use_filename_quotes(filename))
						strcat(s, "\"");
				}
				if (id+1 < count)
				{
					len = strlen(s);
					s[len+0] = IMAGE_SEPARATOR;
					s[len+1] = '\0';
				}
			}

			dev_opts[dev->type] = s;
		}
	}
}

/*	add_device() is called when the MESS CLI option has been identified
 *	This searches throught the devices{} struct array to grab the ID of the
 *	option, which then registers the device using register_device()
 */
static int add_device(struct rc_option *option, const char *arg, int priority)
{
	int id;
	char *myarg;
	const char *s;
	int result;


	id = device_typeid(option->name);
	if (id < 0)
	{
		/* If we get to here, log the error - This is mostly due to a mismatch in the array */
		logerror("Command Line Option [-%s] not a valid device - ignoring\n", option->name);
		return -1;
	}

	/* A match!  we now know the ID of the device */
	option->priority = priority;

	/* device registrations can be delimited */
	while((s = strchr(arg, IMAGE_SEPARATOR)) != NULL)
	{
		if (arg == s)
		{
			myarg = NULL;
		}
		else
		{
			myarg = malloc(s - arg + 1);
			if (!myarg)
				return -1;
			memcpy(myarg, arg, s - arg);
			myarg[s - arg] = '\0';
		}
		result = register_device(id, myarg);
		if (myarg)
			free(myarg);
		if (result)
			return result;
		arg = s + 1;
	}
	return register_device(id, arg);
}

static int specify_ram(struct rc_option *option, const char *arg, int priority)
{
	UINT32 specified_ram = 0;

	if (strcmp(arg, "0"))
	{
		specified_ram = ram_parse_string(arg);
		if (specified_ram == 0)
		{
			fprintf(stderr, "Cannot recognize the RAM option %s\n", arg);
			return -1;
		}
	}
	options.ram = specified_ram;
	return 0;
}

const char *get_devicedirectory(int dev)
{
	assert(dev >= 0);
	assert(dev < IO_COUNT);
	return dev_dirs[dev];
}

void set_devicedirectory(int dev, const char *dir)
{
	assert(dev >= 0);
	assert(dev < IO_COUNT);
	if (dev_dirs[dev])
		free(dev_dirs[dev]);
	dev_dirs[dev] = strdup(dir);
}



void osd_config_save_xml(int type, struct _mame_file *file)
{
}



int win_mess_validitychecks(void)
{
	int i;
	int id_fromshortname;
	int id_fromlongname;
	int error = 0;
	char buf[32];
	int specified_devopts[IO_COUNT];
	int specified_devdirs[IO_COUNT];

	memset(specified_devopts, 0, sizeof(specified_devopts));
	memset(specified_devdirs, 0, sizeof(specified_devdirs));

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

			if ((((char **) mess_opts[i].dest) - dev_opts) != id_fromlongname)
			{
				printf("option -%s is pointed at the wrong dev_opt\n", mess_opts[i].name);
				error = 1;
			}

			if (specified_devopts[id_fromlongname])
			{
				printf("option -%s is specified multiple times\n", mess_opts[i].name);
				error = 1;
			}
			specified_devopts[id_fromlongname] = 1;
		}
		else if (mess_opts[i].name && strstr(mess_opts[i].name, "_dir"))
		{
			strncpyz(buf, mess_opts[i].name, sizeof(buf) / sizeof(buf[0]));
			*strstr(buf, "_dir") = '\0';

			id_fromlongname = device_typeid(buf);
			if (id_fromlongname < 0)
			{
				printf("option -%s is supposed to represent a default device directory, but it doesn't\n", mess_opts[i].name);
				error = 1;
			}

			if (mess_opts[i].shortname)
			{
				printf("option -%s is not supposed to represent a default device directory, but it does\n", mess_opts[i].shortname);
				error = 1;
			}

			if ((((char **) mess_opts[i].dest) - dev_dirs) != id_fromlongname)
			{
				printf("option -%s is pointed at the wrong dev_dir\n", mess_opts[i].name);
				error = 1;
			}

			if (specified_devdirs[id_fromlongname])
			{
				printf("option -%s is specified multiple times\n", mess_opts[i].name);
				error = 1;
			}
			specified_devdirs[id_fromlongname] = 1;
		}
	}

	for (i = 0; i < IO_COUNT; i++)
	{
		if (!specified_devopts[i])
		{
			printf("device '%s' is not represented\n", device_typename(i));
			error = 1;
		}

		if (!specified_devdirs[i])
		{
			printf("device dir '%s' is not represented\n", device_typename(i));
			error = 1;
		}
	}

	return error;
}
