/******************************************************************************

  MESS - device.c

  List of all available devices and Device handling interfaces.

******************************************************************************/

#include "driver.h"
#include "device.h"

/* The List of Devices, with Associated Names - Be careful to ensure that 	*/
/* this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
const struct Devices devices[] =
{
	{IO_END,		"NONE",			"NONE"}, /*  0 */
	{IO_CARTSLOT,	"cartridge",	"cart"}, /*  1 */
	{IO_FLOPPY,		"floppydisk",	"flop"}, /*  2 */
	{IO_HARDDISK,	"harddisk",		"hard"}, /*  3 */
	{IO_CYLINDER,	"cylinder",		"cyln"}, /*  4 */
	{IO_CASSETTE,	"cassette",		"cass"}, /*  5 */
	{IO_PUNCHCARD,	"punchcard",	"pcrd"}, /*  6 */
	{IO_PUNCHTAPE,	"punchtape",	"ptap"}, /*  7 */
	{IO_PRINTER,	"printer",		"prin"}, /*  8 */
	{IO_SERIAL,		"serial",		"serl"}, /*  9 */
	{IO_PARALLEL,   "parallel",		"parl"}, /* 10 */
	{IO_SNAPSHOT,	"snapshot",		"dump"}, /* 11 */
	{IO_QUICKLOAD,	"quickload",	"quik"}, /* 12 */
	{IO_COUNT,		NULL,			NULL  }, /* 13 Always at end of this array! */
};


/* register_device() - used to register the device in the options struct...	*/
/* Call this from the CLI or UI to add a DEVICE (with its arg) to the 		*/
/* options struct.  Return 0 for success, -1 for error 						*/
int register_device (const int type, const char *arg)
{
	extern struct GameOptions options;

	/* Check the the device type is valid, otherwise this lookup will be bad*/
	if (type <= IO_END || type >= IO_COUNT || !type)
	{
		mess_printf("register_device() failed! - device type [%d] is not valid\n",type);
		return -1;
	}

	/* Next, check that we havent loaded too many images					*/
	if (options.image_count >= MAX_IMAGES)
	{
		mess_printf("Too many image names specified!\n");
		return -1;
	}

	/* All seems ok to add device and argument to options{} struct			*/
	#ifdef MAME_DEBUG
	mess_printf("Image [%s] Registered for Device [%s]\n", arg, device_typename(type));
	#endif
	/* the user specified a device type */
	options.image_files[options.image_count].type = type;
	options.image_files[options.image_count].name = strdup(arg);
	options.image_count++;
	return 0;

}

int device_open(int type, int id, int mode, void *args)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->open )
			return (*dev->open)(id,mode,args);
	}
	return 1;
}

void device_close(int type, int id)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->close )
		{
			(*dev->close)(id);
			return;
		}
	}
}

int device_seek(int type, int id, int offset, int whence)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->seek )
			return (*dev->seek)(id,offset,whence);
		dev++;
	}
	return 0;
}

int device_tell(int type, int id)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->tell )
			return (*dev->tell)(id);
	}
	return 0;
}

int device_status(int type, int id, int newstatus)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->status )
			return (*dev->status)(id,newstatus);
	}
	return 0;
}

int device_input(int type, int id)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->input )
			return (*dev->input)(id);
	}
	return 0;
}

void device_output(int type, int id, int data)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->output )
		{
			(*dev->output)(id,data);
			return;
		}
	}
}

int device_input_chunk(int type, int id, void *dst, int chunks)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->input_chunk )
			return (*dev->input_chunk)(id,dst,chunks);
	}
	return 1;
}

void device_output_chunk(int type, int id, void *src, int chunks)
{
	const struct IODevice *dev;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type && dev->output )
		{
			(*dev->output_chunk)(id,src,chunks);
			return;
		}
	}
}

static const struct IODevice *get_sysconfig_device(const struct GameDriver *gamedrv, int device_num)
{
	struct SystemConfigurationParamBlock params;
	memset(&params, 0, sizeof(params));
	params.device_num = device_num;
	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&params);
	return params.dev;
}

const struct IODevice *device_first(const struct GameDriver *gamedrv)
{
	assert(gamedrv);

	if ((gamedrv->dev_) && (gamedrv->dev_->type != IO_END))
		return gamedrv->dev_;
	else
		return get_sysconfig_device(gamedrv, 0);
}

const struct IODevice *device_next(const struct GameDriver *gamedrv, const struct IODevice *dev)
{
	int i;
	const struct IODevice *dev2;

	assert(gamedrv);
	assert(dev);

	/* is dev in the legacy IODevice array? */
	dev2 = gamedrv->dev_;
	while((dev2->type != IO_END) && (dev2 != dev))
		dev2++;

	if (dev2 == dev)
	{
		dev2++;
		if (dev2->type == IO_END)
			dev2 = get_sysconfig_device(gamedrv, 0);
	}
	else
	{
		i = 0;
		do
		{
			dev2 = get_sysconfig_device(gamedrv, i++);
		}
		while(dev2 && (dev2 != dev));
		if (dev2 == dev)
			dev2 = get_sysconfig_device(gamedrv, i);
	}
	return dev2;
}

const struct IODevice *device_find(const struct GameDriver *gamedrv, int type)
{
    const struct IODevice *dev;
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		if (dev->type == type)
			return dev;
	}
	return NULL;
}

