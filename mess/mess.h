#ifndef MESS_H
#define MESS_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

int DECL_SPEC mess_printf(char *fmt, ...);

extern void showmessinfo(void);
extern int displayimageinfo(struct osd_bitmap *bitmap, int selected);
extern int filemanager(struct osd_bitmap *bitmap, int selected);
extern int tapecontrol(struct osd_bitmap *bitmap, int selected);

/* driver.h - begin */
#define IPT_SELECT1		IPT_COIN1
#define IPT_SELECT2		IPT_COIN2
#define IPT_SELECT3		IPT_COIN3
#define IPT_SELECT4		IPT_COIN4
#define IPT_KEYBOARD	IPT_TILT
/* driver.h - end */


/* The wrapper for osd_fopen() */
void *image_fopen(int type, int id, int filetype, int read_or_write);

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
enum { INIT_OK, INIT_FAILED, INIT_UNKNOWN };


/* IODevice ID return values.  Use these to determine if */
/* the emulation can continue if image cannot be positively IDed */
enum { ID_FAILED, ID_OK, ID_UNKNOWN };


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
 *	images from within the emulation. A driver might use this
 *	if both, OSD_FOPEN_RW and OSD_FOPEN_READ modes, failed.
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

#ifdef MAX_KEYS
 #undef MAX_KEYS
 #define MAX_KEYS	128 /* for MESS but already done in INPUT.C*/
#endif

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

enum {
	IO_END = 0,
	IO_CARTSLOT,
	IO_FLOPPY,
	IO_HARDDISK,
	IO_CASSETTE,
	IO_PRINTER,
	IO_SERIAL,
	IO_SNAPSHOT,
	IO_QUICKLOAD,
	IO_ALIAS,  /* dummy type for alias names from mess.cfg */
	IO_COUNT
};

struct IODevice {
	int type;
	int count;
	const char *file_extensions;
	void *_private;
	int (*id)(int id);
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

/* these are called from mame.c run_game() */

extern int get_filenames(void);
extern int init_devices(const void *game);
extern void exit_devices(void);

/* access mess.c internal fields for a device type (instance id) */

extern int device_count(int type);
extern const char *device_typename(int type);
extern const char *briefdevice_typename(int type);
extern const char *device_typename_id(int type, int id);
extern const char *device_filename(int type, int id);
extern unsigned int device_length(int type, int id);
extern unsigned int device_crc(int type, int id);
extern void device_set_crc(int type, int id, UINT32 new_crc);
extern const char *device_longname(int type, int id);
extern const char *device_manufacturer(int type, int id);
extern const char *device_year(int type, int id);
extern const char *device_playable(int type, int id);
extern const char *device_extrainfo(int type, int id);

extern const char *device_file_extension(int type, int extnum);
extern int device_filename_change(int type, int id, const char *name);

/* access functions from the struct IODevice arrays of a driver */

extern const void *device_info(int type, int id);
extern int device_open(int type, int id, int mode, void *args);
extern void device_close(int type, int id);
extern int device_seek(int type, int id, int offset, int whence);
extern int device_tell(int type, int id);
extern int device_status(int type, int id, int newstatus);
extern int device_input(int type, int id);
extern void device_output(int type, int id, int data);
extern int device_input_chunk(int type, int id, void *dst, int chunks);
extern void device_output_chunk(int type, int id, void *src, int chunks);

/* This is the dummy GameDriver with flag NOT_A_DRIVER set
   It allows us to use an empty PARENT field in the macros. */
extern struct GameDriver driver_0;

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
	&machine_driver_##MACHINE,				\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	ROT0									\
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
	&machine_driver_##MACHINE,				\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	ROT0|(FLAGS)							\
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
	&machine_driver_##MACHINE,				\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	ROT0|GAME_COMPUTER 						\
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
	&machine_driver_##MACHINE,				\
	input_ports_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	io_##NAME, 								\
	ROT0|GAME_COMPUTER|(FLAGS)	 			\
};


#endif
