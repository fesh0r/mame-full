#ifndef MESSDRV_H
#define MESSDRV_H

#include <assert.h>
#include "formats.h"
#include "fileio.h"

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
struct mame_bitmap;

struct IODevice
{
	int type;
	int count;
	const char *file_extensions;
	int flags;
	int open_mode;
	int (*init)(int id, mame_file *fp, int open_mode);
	void (*exit)(int id);
	int (*load)(int id, mame_file *fp, int open_mode);
	void (*unload)(int id);
	int (*verify)(mame_file *fp);
	const void *(*info)(int id, int whatinfo);
	int (*open)(int id, int mode, void *args);
	void (*close)(int id);
	int (*status)(int id, int newstatus);
    int (*seek)(int id, int offset, int whence);
    int (*tell)(int id);
	int (*input)(int id);
	void (*output)(int id, int data);
	UINT32 (*partialcrc)(const unsigned char *buf, unsigned int size);
	void (*display)(struct mame_bitmap *bitmap, int id);
	void *user1;
	void *user2;
};

struct SystemConfigurationParamBlock
{
	int max_ram_options;
	int actual_ram_options;
	int default_ram_option;
	UINT32 *ram_options;
	int device_num;
	const struct IODevice *dev;
	const char *(*get_custom_devicename)(int type, int id, char *buf, size_t bufsize);
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

#define CONFIG_GET_CUSTOM_DEVICENAME(get_custom_devicename__)								\
	cfg->get_custom_devicename = get_custom_devicename_##get_custom_devicename__;			\

#define CONFIG_DEVICE_BASE(type, count, file_extensions, flags, open_mode, init, exit,		\
		info, open, close, status, seek, tell, input, output, partialcrc, display)			\
	if (cfg->device_num-- == 0)																\
	{																						\
		static struct IODevice device = { (type), (count), (file_extensions), (flags),		\
			(open_mode), (init), (exit), (NULL), (NULL), (NULL), (info), (open),			\
			(close), (status), (seek), (tell), (input), (output), (partialcrc), (display),	\
			NULL, NULL };																	\
		cfg->dev = &device;																	\
	}																						\

#define CONFIG_DEVICE_LEGACY(type, count, file_extensions, flags, open_mode,				\
		init, exit, status)																	\
	CONFIG_DEVICE_BASE((type), (count), (file_extensions), (flags), (open_mode),			\
		(init), (exit), NULL, NULL, NULL, (status), NULL, NULL, NULL, NULL, NULL, NULL)		\

#define CONFIG_DEVICE_LEGACYX(type, count, file_extensions, flags, open_mode,				\
		init, exit, open, status)															\
	CONFIG_DEVICE_BASE((type), (count), (file_extensions), (flags), (open_mode),			\
		(init), (exit), NULL, (open), NULL, (status), NULL, NULL, NULL, NULL, NULL, NULL)	\
	
/*****************************************************************************/

#define GET_CUSTOM_DEVICENAME(name)															\
	const char *get_custom_devicename_##name(int type, int id, char *buf, size_t bufsize)	\

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
	construct_sysconfig_##CONFIG,			\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};

#define construct_sysconfig_NULL	(NULL)

#endif /* MESSDRV_H */


