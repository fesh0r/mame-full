#ifndef MESS_H
#define MESS_H

#define ORIENTATION_DEFAULT ROT0 /* hack till we change to the driver macro */


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

void showmessinfo(void);

/* driver.h - begin */
#define IPT_SELECT1		IPT_COIN1
#define IPT_SELECT2		IPT_COIN2
#define IPT_SELECT3		IPT_COIN3
#define IPT_SELECT4		IPT_COIN4
#define IPT_KEYBOARD	IPT_TILT
/* driver.h - end */


/* fileio.c */
typedef struct
{
	int crc;
	int length;
	char * name;

} image_details;

/* possible values for osd_fopen() last argument
 * OSD_FOPEN_READ
 *	open existing file in read only mode.
 *	ZIP images can be opened only in this mode, unless
 *	we add support for writing into ZIP files.
 * OSD_FOPEN_WRITE
 *	open new file in write only mode (truncate existing file).
 *	used for output images (eg. a cassette being written).
 * OSD_FOPEN_RW
 *	open existing(!) file in read/write mode.
 *	used for floppy/harddisk images. if it fails, a driver
 *	might try to open the image with OSD_FOPEN_READ and set
 *	an internal 'write protect' flag for the FDC/HDC emulation.
 * OSD_FOPEN_RW_CREATE
 *	open existing file or create new file in read/write mode.
 *	used for floppy/harddisk images. if a file doesn't exist,
 *	it shall be created. Used to 'format' new floppy or harddisk
 *	images from within the emulation. a driver might use this
 *	if both, OSD_FOPEN_RW and OSD_FOPEN_READ, failed.
 */
enum { OSD_FOPEN_READ, OSD_FOPEN_WRITE, OSD_FOPEN_RW, OSD_FOPEN_RW_CREATE };


/* mess.c functions [for external use] */
int parse_image_types(char *arg);



/******************************************************************************
 *	floppy disc controller direct access
 *	osd_fdc_init
 *		initialize the needed hardware & structures;
 *		returns 0 on success
 *	osd_fdc_exit
 *		shut down
 *	osd_fdc_motors
 *		start motors for <unit> number (0 = A:, 1 = B:)
 *	osd_fdc_density
 *		set type of drive from bios info (1: 360K, 2: 1.2M, 3: 720K, 4: 1.44M)
 *		set density (0:FM,LO 1:FM,HI 2:MFM,LO 3:MFM,HI) ( FM doesn't work )
 *		tracks, sectors per track and sector length code are given to
 *		calculate the appropriate double step and GAP II, GAP III values
 *	osd_fdc_interrupt
 *		stop motors and interrupt the current command
 *	osd_fdc_recal
 *		recalibrate the current drive and update *track if not NULL
 *	osd_fdc_seek
 *		seek to a given track number and update *track if not NULL
 *	osd_fdc_step
 *		step into a direction (+1/-1) and update *track if not NULL
 *	osd_fdc_format
 *		format track t, head h, spt sectors per track
 *		sector map at *fmt
 *	osd_fdc_put_sector
 *		put a sector from memory *buff to track 'track', side 'side',
 *		head number 'head', sector number 'sector';
 *		write deleted data address mark if ddam is non zero
 *	osd_fdc_get_sector
 *		read a sector to memory *buff from track 'track', side 'side',
 *		head number 'head', sector number 'sector'
 *
 * NOTE: side and head
 * the terms are used here in the following way:
 * side = physical side of the floppy disk
 * head = logical head number (can be 0 though side 1 is to be accessed)
 *****************************************************************************/

int  osd_fdc_init(void);
void osd_fdc_exit(void);
void osd_fdc_motors(unsigned char unit);
void osd_fdc_density(unsigned char unit, unsigned char density, unsigned char tracks, unsigned char spt, unsigned char eot, unsigned char secl);
void osd_fdc_interrupt(int param);
unsigned char osd_fdc_recal(unsigned char *track);
unsigned char osd_fdc_seek(unsigned char t, unsigned char *track);
unsigned char osd_fdc_step(int dir, unsigned char *track);
unsigned char osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char *fmt);
unsigned char osd_fdc_put_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff, unsigned char ddam);
unsigned char osd_fdc_get_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff);

#ifdef MESS /* just to be safe ;-) */
#ifdef MAX_KEYS
 #undef MAX_KEYS
 #define MAX_KEYS	128 /* for MESS but already done in INPUT.C*/
#endif
#endif

/******************************************************************************
 * This is a start at the proposed peripheral structure.
 * It will be filled with live starting with the next release (I hope).
 * For now it gets us rid of the several lines MESS specific code
 * in the GameDriver struct and replaces it by only one pointer.
 *	type				type of device (from above enum)
 *	count				maximum number of instances
 *	file_extensions;	supported file extensions
 *	_private;			to be used by the peripheral driver code
 *	id					identify file
 *	init				initialize device
 *	info				get info for device instance
 *	open				open device (with specific args)
 *	close				close device
 *	status				get device status
 *	seek				seek to file position
 *	input				input character
 *	output				output character
 *	input_chunk 		input chunk of data
 *	output_chunk		output chunk of data
 ******************************************************************************/

enum {
	IO_END = 0,
	IO_CARTSLOT,
	IO_FLOPPY,
	IO_HARDDISK,
	IO_CASSETTE,
	IO_PRINTER,
	IO_SERIAL
};

struct IODevice {
	int type;
	int count;
	const char *file_extensions;
	void *_private;
	int (*id)(const char *filename, const char *gamename);
	int (*init)(int id, const char *filename);
	void (*exit)(int id);
	const void *(*info)(int id, int whatinfo);
	int (*open)(int id, void *args);
	void (*close)(int id);
	int (*status)(int id);
	int (*seek)(int id, int offset, int whence);
	int (*input)(int id);
	void (*output)(int id, int data);
	int (*input_chunk)(int id, void *dst, int size);
	int (*output_chunk)(int id, void *src, int size);
};

/* these are called from mame.c run_game() */

/* retrieve the filenames from options.image_files */
extern int get_filenames(void);

/* initialize devices, ie. let the driver open/check the image files */
extern int init_devices(const void *game);

/* shutdown devices */
extern void exit_devices(void);

/* Return the number of installed filenames for a device of type 'type'. */
extern int device_count(int type);

/* Return the name for a device of type 'type'. */
extern const char *device_typename(int type);

/* Return the id'th filename for a device of type 'type',
   NULL if not enough image names of that type are available. */
extern const char *device_filename(int type, int id);

/* This is the dummy GameDriver with flag NOT_A_DRIVER set
 * It allows us to use an empty PARENT field in the macros.
 */
extern struct GameDriver driver_0;


//#define init_0	NULL


/******************************************************************************
 * MESS' version of the GAME() and GAMEX() macros of MAME
 * CONS and CONSX are for consoles
 * COMP and COMPX are for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME)	\
extern struct GameDriver driver_##PARENT;	\
struct GameDriver driver_##NAME =			\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
"",0,\
	&machine_driver_##MACHINE,				\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 					\
0,0,0,0,\
	input_ports_##INPUT,					\
0,0,0,\
	ROT0,									\
0,0\
};

#define CONSX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern struct GameDriver driver_##PARENT;	\
struct GameDriver driver_##NAME =			\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
"",0,\
	&machine_driver_##MACHINE,				\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 					\
0,0,0,0,\
	input_ports_##INPUT,					\
0,0,0,\
	ROT0|(FLAGS),							\
0,0\
};

#define COMP(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME)	\
extern struct GameDriver driver_##PARENT;	\
struct GameDriver driver_##NAME =			\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
"",0,\
	&machine_driver_##MACHINE,				\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 					\
0,0,0,0,\
	input_ports_##INPUT,					\
0,0,0,\
	ROT0|GAME_COMPUTER, 					\
0,0\
};

#define COMPX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern struct GameDriver driver_##PARENT;	\
struct GameDriver driver_##NAME =			\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
"",0,\
	&machine_driver_##MACHINE,				\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 					\
0,0,0,0,\
	input_ports_##INPUT,					\
0,0,0,\
	ROT0|GAME_COMPUTER|(FLAGS), 			\
0,0\
};


#endif
