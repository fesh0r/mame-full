#ifndef MESSDRV_H
#define MESSDRV_H

#include <assert.h>
#include "formats.h"

/******************************************************************************
 * This is a start at the proposed peripheral structure.
 * It will be filled with live starting with the next release (I hope).
 * For now it gets us rid of the several lines MESS specific code
 * in the GameDriver struct and replaces it by only one pointer.
 *	type				type of device (from above enum)
 *	count				maximum number of instances
 *	file_extensions 	supported file extensions
 *	_private			to be used by the peripheral driver code
 *	id					identify file
 *	init				initialize device
 *	exit				shutdown device
 *	info				get info for device instance
 *	open				open device (with specific args)
 *	close				close device
 *	status				(set a device status and) get the previous status
 *	seek				seek to file position
 *	tell				tell current file position
 *	input				input character or code
 *	output				output character or code
 *	input_chunk 		input chunk of data (eg. sector or track)
 *	output_chunk		output chunk of data (eg. sector or track)
 ******************************************************************************/
struct IODevice {
	int type;
	int count;
	const char *file_extensions;
	int reset_depth;
	int open_mode;
	char *dummy;
	int (*init)(int id);
	void (*exit)(int id);
	const void *(*info)(int id, int whatinfo);
	int (*open)(int id, int mode, void *args);
	void (*close)(int id);
	int (*status)(int id, int newstatus);
    int (*seek)(int id, int offset, int whence);
    int (*tell)(int id);
	int (*input)(int id);
	void (*output)(int id, int data);
	int (*input_chunk)(int id, void *dst, int chunks);
	int (*output_chunk)(int id, void *src, int chunks);
	UINT32 (*partialcrc)(const unsigned char *buf, unsigned int size);
};

struct SystemConfigurationParamBlock
{
	int max_ram_options;
	int actual_ram_options;
	int default_ram_option;
	UINT32 *ram_options;
	int device_num;
	const struct IODevice *dev;
};

#define SYSTEM_CONFIG_START(name)															\
	static void construct_sysconfig_##name(struct SystemConfigurationParamBlock *cfg)		\
	{																						\

#define SYSTEM_CONFIG_END																	\
	}																						\

#define CONFIG_IMPORT_FROM(name)															\
		construct_sysconfig_##name(cfg);													\

#define CONFIG_RAM(ram)																		\
	if (cfg->max_ram_options > 0)															\
	{																						\
		assert(cfg->actual_ram_options < cfg->max_ram_options);								\
		assert(cfg->ram_options);															\
		cfg->ram_options[cfg->actual_ram_options++] = (ram);								\
	}																						\

#define CONFIG_RAM_DEFAULT(ram)																\
	if (cfg->max_ram_options > 0)															\
	{																						\
		cfg->default_ram_option = cfg->actual_ram_options;									\
		CONFIG_RAM(ram);																	\
	}																						\


#define CONFIG_DEVICE(type, count, file_extensions, reset_depth, open_mode, init, exit,		\
						info, open, close, status, seek, tell, input, output, partialcrc)	\
	if (cfg->device_num-- == 0)																\
	{																						\
		static struct IODevice device = { (type), (count), (file_extensions), (reset_depth),\
			(open_mode), NULL, (init), (exit), (info), (open), (close), (status), (seek),	\
			(tell), (input), (output), NULL, NULL, (partialcrc) };							\
		cfg->dev = &device;																	\
	}																						\


#define CONFIG_DEVICE_PRINTER(init,exit,output)												\
	CONFIG_DEVICE(IO_PRINTER, 1, "prn\0", IO_RESET_NONE, OSD_FOPEN_WRITE, (init), (exit),	\
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, (output), NULL)							\

#define CONFIG_DEVICE_CARTSLOT(file_extensions,count,init,exit,partialcrc)					\
	CONFIG_DEVICE(IO_CARTSLOT, (count), (file_extensions), IO_RESET_CPU, OSD_FOPEN_READ,	\
		(init), (exit),	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (partialcrc))		\

#define CONFIG_DEVICE_SNAPSHOT(file_extensions,count,init,exit)								\
	CONFIG_DEVICE(IO_SNAPSHOT, (count), (file_extensions), IO_RESET_CPU, OSD_FOPEN_READ,	\
		(init), (exit),	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)				\

/******************************************************************************
 * MESS' version of the GAME() and GAMEX() macros of MAME
 * CONS and CONSX are for consoles
 * COMP and COMPX are for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME)	\
extern const struct GameDriver driver_##PARENT; \
extern const struct GameDriver driver_##NAME;   \
const struct GameDriver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	construct_sysconfig_##CONFIG,			\
	ROT0									\
};

#define CONSX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const struct GameDriver driver_##PARENT;   \
extern const struct GameDriver driver_##NAME;   \
const struct GameDriver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	construct_sysconfig_##CONFIG,			\
	ROT0|(FLAGS)							\
};

#define COMP(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME)	\
extern const struct GameDriver driver_##PARENT;   \
extern const struct GameDriver driver_##NAME;   \
const struct GameDriver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	construct_sysconfig_##CONFIG,			\
	ROT0|GAME_COMPUTER 						\
};

#define COMPX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const struct GameDriver driver_##PARENT;   \
extern const struct GameDriver driver_##NAME;   \
const struct GameDriver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	construct_sysconfig_##CONFIG,			\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};

#define construct_sysconfig_NULL	(NULL)

#endif /* MESSDRV_H */


