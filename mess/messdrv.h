#ifndef MESSDRV_H
#define MESSDRV_H

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

struct ComputerConfigEntry
{
	UINT8 type;
	UINT32 param;
};

enum
{
	CCE_END,
	CCE_RAM,
	CCE_RAM_DEFAULT
};

#define COMPUTER_CONFIG_START(name) \
	static const struct ComputerConfigEntry computer_config_##name[] = {

#define COMPUTER_CONFIG_END \
	{ CCE_END, 0 } \
	};

#define CONFIG_RAM(ram) \
	{ CCE_RAM, (ram) },

#define CONFIG_RAM_DEFAULT(ram) \
	{ CCE_RAM_DEFAULT, (ram) },

/******************************************************************************
 * MESS' version of the GAME() and GAMEX() macros of MAME
 * CONS and CONSX are for consoles
 * COMP and COMPX are for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME)	\
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
	NULL,									\
	ROT0									\
};

#define CONSX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
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
	NULL,									\
	ROT0|(FLAGS)							\
};

#define COMP(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME)	\
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
	NULL,									\
	ROT0|GAME_COMPUTER 						\
};

#define COMPX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
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
	NULL,									\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};

#define COMPC(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME)	\
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
	computer_config_##CONFIG,				\
	ROT0|GAME_COMPUTER 						\
};

#define COMPCX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
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
	computer_config_##CONFIG,				\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};

#endif /* MESSDRV_H */


