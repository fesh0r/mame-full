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

static char *ramsize_opt;
static char *dev_dirs[IO_COUNT];

static int specify_ram(struct rc_option *option, const char *arg, int priority);

struct rc_option mess_opts[] =
{
	/* FIXME - these option->names should NOT be hardcoded! */
	{ "MESS specific options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "newui",			"nu",   rc_bool,	&options.disable_normal_ui,	"1", 0, 0, NULL,			"use the new MESS UI" },
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
int write_config(const char* filename, const game_driver *gamedrv)
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



//============================================================
//	Device options
//============================================================

struct device_rc_option
{
	// options for the RC system
	struct rc_option opts[2];

	// device information
	iodevice_t devtype;
	const char *tag;
	int index;

	// mounted file
	char *filename;
};

struct device_type_options
{
	int count;
	struct device_rc_option *opts[MAX_DEV_INSTANCES];
};

struct device_type_options *device_options;



static int add_device(struct rc_option *option, const char *arg, int priority)
{
	struct device_rc_option *dev_option = (struct device_rc_option *) option;

	// the user specified a device type
	options.image_files[options.image_count].device_type = dev_option->devtype;
	options.image_files[options.image_count].device_tag = dev_option->tag;
	options.image_files[options.image_count].device_index = dev_option->index;
	options.image_files[options.image_count].name = auto_strdup(arg);
	options.image_count++;

	return 0;
}



void device_dirs_load(int config_type, xml_data_node *parentnode)
{
	iodevice_t dev;

	// on init, reset the directories
	if (config_type == CONFIG_TYPE_INIT)
		memset(dev_dirs, 0, sizeof(dev_dirs));

	// only care about game-specific data
	if (config_type != CONFIG_TYPE_GAME)
		return;

	// might not have any data
	if (!parentnode)
		return;

	for (dev = 0; dev < IO_COUNT; dev++)
	{
	}
}



void device_dirs_save(int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	iodevice_t dev;

	// only care about game-specific data
	if (config_type != CONFIG_TYPE_GAME)
		return;

	for (dev = 0; dev < IO_COUNT; dev++)
	{
		if (dev_dirs[dev])
		{
			node = xml_add_child(parentnode, "device", NULL);
			if (node)
			{
				xml_set_attribute(node, "type", device_typename(dev));
				xml_set_attribute(node, "directory", dev_dirs[dev]);
			}
		}
	}
}



void win_add_mess_device_options(struct rc_struct *rc, const game_driver *gamedrv)
{
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	device_class devclass;
	iodevice_t devtype;
	int dev_count, dev, id, count;
	struct device_rc_option *dev_option;
	struct rc_option *opts;
	const char *dev_name;
	const char *dev_short_name;
	const char *dev_tag;

	// retrieve getinfo handlers
	memset(&cfg, 0, sizeof(cfg));
	memset(handlers, 0, sizeof(handlers));
	cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
	cfg.device_handlers = handlers;
	cfg.device_countoverrides = count_overrides;
	gamedrv->sysconfig_ctor(&cfg);

	// count devides
	for (dev_count = 0; handlers[dev_count]; dev_count++)
		;

	if (dev_count > 0)
	{
		// add a separator
		opts = auto_malloc(sizeof(*opts) * 2);
		memset(opts, 0, sizeof(*opts) * 2);
		opts[0].name = "MESS devices";
		opts[0].type = rc_seperator;
		opts[1].type = rc_end;
		rc_register(rc, opts);

		// we need to save all options
		device_options = auto_malloc(sizeof(*device_options) * dev_count);
		memset(device_options, 0, sizeof(*device_options) * dev_count);

		// list all options
		for (dev = 0; dev < dev_count; dev++)
		{
			devclass.gamedrv = gamedrv;
			devclass.get_info = handlers[dev];

			// retrieve info about the device
			devtype = (iodevice_t) (int) device_get_info_int(&devclass, DEVINFO_INT_TYPE);
			count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);
			dev_tag = device_get_info_string(&devclass, DEVINFO_STR_DEV_TAG);
			if (dev_tag)
				dev_tag = auto_strdup(dev_tag);

			device_options[dev].count = count;

			for (id = 0; id < count; id++)
			{
				// retrieve info about hte device instance
				dev_name = device_instancename(&devclass, id);
				dev_short_name = device_briefinstancename(&devclass, id);

				// dynamically allocate the option
				dev_option = auto_malloc(sizeof(*dev_option));
				memset(dev_option, 0, sizeof(*dev_option));

				// populate the options
				dev_option->opts[0].name = auto_strdup(dev_name);
				dev_option->opts[0].shortname = auto_strdup(dev_short_name);
				dev_option->opts[0].type = rc_string;
				dev_option->opts[0].func = add_device;
				dev_option->opts[0].dest = &dev_option->filename;
				dev_option->opts[1].type = rc_end;
				dev_option->devtype = devtype;
				dev_option->tag = dev_tag;
				dev_option->index = id;

				// register these options
				device_options[dev].opts[id] = dev_option;
				rc_register(rc, dev_option->opts);
			}
		}
	}
}



void win_mess_config_init(void)
{
	config_register("device_directories", device_dirs_load, device_dirs_save);
}



void osd_begin_final_unloading(void)
{
	int opt = 0, i;
	const struct IODevice *dev;
	mess_image *image;
	char **filename_ptr;

	if (Machine->devices)
	{
		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			for (i = 0; i < device_options[opt].count; i++)
			{
				// free existing string, if there
				filename_ptr = &device_options[opt].opts[i]->filename;
				if (*filename_ptr)
				{
					free(*filename_ptr);
					*filename_ptr = NULL;
				}

				// locate image
				image = image_from_device_and_index(dev, i);

				// place new filename, if there
				if (image)
					*filename_ptr = strdup(image_filename(image));
			}
			opt++;
		}
	}
}



int osd_select_file(mess_image *img, char *filename)
{
	return 0;
}



void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
}
