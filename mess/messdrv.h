/***************************************************************************

	messdrv.h

	MESS specific driver stuff

***************************************************************************/

#ifndef MESSDRV_H
#define MESSDRV_H

#include <assert.h>

#include "formats/flopimg.h"
#include "fileio.h"
#include "osdepend.h"
#include "unicode.h"
#include "device.h"



struct SystemConfigurationParamBlock
{
	/* for RAM options */
	int max_ram_options;
	int actual_ram_options;
	int default_ram_option;
	UINT32 *ram_options;

	/* for natural keyboard input */
	int (*queue_chars)(const unicode_char_t *text, size_t text_len);
	int (*accept_char)(unicode_char_t ch);
	int (*charqueue_empty)(void);

	/* device specification */
	int device_slotcount;
	device_getinfo_handler *device_handlers;
	int *device_countoverrides;
	int device_position;
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

#define CONFIG_QUEUE_CHARS(queue_chars_)													\
	cfg->queue_chars = inputx_queue_chars_##queue_chars_;									\

#define CONFIG_ACCEPT_CHAR(accept_char_)													\
	cfg->accept_char = inputx_accept_char_##accept_char_;									\

#define CONFIG_CHARQUEUE_EMPTY(charqueue_empty_)											\
	cfg->charqueue_empty = inputx_charqueue_empty_##charqueue_empty_;						\

#define CONFIG_DEVICE(getinfo)										\
	if (cfg->device_position < cfg->device_slotcount)				\
	{																\
		cfg->device_handlers[cfg->device_position++] = (getinfo);	\
	}																\

#define CONFIG_DEVICE_COUNT(getinfo, count)							\
	if (cfg->device_position < cfg->device_slotcount)				\
	{																\
		cfg->device_handlers[cfg->device_position] = (getinfo);		\
		cfg->device_countoverrides[cfg->device_position] = (count);	\
		cfg->device_position++;										\
	}																\

/*****************************************************************************/

#define QUEUE_CHARS(name)																	\
	int inputx_queue_chars_##name(const unicode_char_t *text, size_t text_len)				\

#define ACCEPT_CHAR(name)																	\
	int inputx_accept_char_##name(unicode_char_t ch)										\

#define CHARQUEUE_EMPTY(name)																	\
	int inputx_charqueue_empty_##name(void)										\

/******************************************************************************
 * MESS' version of the GAME() and GAMEX() macros of MAME
 * CONS and CONSX are for consoles
 * COMP and COMPX are for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME)	\
extern const game_driver driver_##PARENT; \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0									\
};

#define CONSX(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|(FLAGS)							\
};

#define COMP(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|GAME_COMPUTER 						\
};

#define COMPX(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};

#define construct_sysconfig_NULL	(NULL)

#endif /* MESSDRV_H */


