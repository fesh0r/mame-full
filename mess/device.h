/***************************************************************************

	device.h

	Definitions and manipulations for device structures

***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include "osd_mess.h"
#include "opresolv.h"
#include "fileio.h"
#include "mamecore.h"

#define MAX_DEV_INSTANCES	5

typedef enum
{
	/* List of all supported devices.  Refer to the device by these names only */
	IO_CARTSLOT,	/*  0 - Cartridge Port, as found on most console and on some computers */
	IO_FLOPPY,		/*  1 - Floppy Disk unit */
	IO_HARDDISK,	/*  2 - Hard Disk unit */
	IO_CYLINDER,	/*  3 - Magnetically-Coated Cylinder */
	IO_CASSETTE,	/*  4 - Cassette Recorder (common on early home computers) */
	IO_PUNCHCARD,	/*  5 - Card Puncher/Reader */
	IO_PUNCHTAPE,	/*  6 - Tape Puncher/Reader (reels instead of punchcards) */
	IO_PRINTER,		/*  7 - Printer device */
	IO_SERIAL,		/*  8 - Generic Serial Port */
	IO_PARALLEL,    /*  9 - Generic Parallel Port */
	IO_SNAPSHOT,	/* 10 - Complete 'snapshot' of the state of the computer */
	IO_QUICKLOAD,	/* 11 - Allow to load program/data into memory, without matching any actual device */
	IO_MEMCARD,		/* 12 - Memory card */
	IO_CDROM,	/* 13 - optical CD-ROM disc */
	IO_COUNT		/* 14 - Total Number of IO_devices for searching */
} iodevice_t;

struct IODevice;

typedef void (*device_getinfo_handler)(struct IODevice *dev);

typedef int (*device_init_handler)(mess_image *img);
typedef void (*device_exit_handler)(mess_image *img);
typedef int (*device_load_handler)(mess_image *img, mame_file *fp);
typedef int (*device_create_handler)(mess_image *img, mame_file *fp, int format_type, option_resolution *format_options);
typedef void (*device_unload_handler)(mess_image *img);
typedef void (*device_partialhash_handler)(char *, const unsigned char *, unsigned long, unsigned int);


struct CreateImageOptions
{
	const char *name;
	const char *description;
	const char *extensions;
	const char *optspec;
};

struct IODevice
{
	/* the basics */
	const char *tag;
	iodevice_t type;
	int count;
	const char *file_extensions;

	/* open dispositions */
	unsigned int readable : 1;
	unsigned int writeable : 1;
	unsigned int creatable : 1;

	/* miscellaneous flags */
	unsigned int reset_on_load : 1;
	unsigned int must_be_loaded : 1;
	unsigned int load_at_init : 1;
	unsigned int error : 1;
	unsigned int not_working : 1;

	/* image handling callbacks */
	device_init_handler init;
	device_exit_handler exit;
	device_load_handler load;
	device_create_handler create;
	device_unload_handler unload;
	int (*imgverify)(const UINT8 *buf, size_t size);
	device_partialhash_handler partialhash;
	void (*getdispositions)(const struct IODevice *dev, int id,
		unsigned int *readable, unsigned int *writeable, unsigned int *creatable);

	/* cosmetic/UI callbacks */
	void (*display)(mess_image *img, mame_bitmap *bitmap);
	const char *(*name)(const struct IODevice *dev, int id, char *buf, size_t bufsize);

	/* image creation options */
	const struct OptionGuide *createimage_optguide;
	struct CreateImageOptions *createimage_options; 

	/* user data pointers */
	void *user1;
	void *user2;
	void *user3;
	void *user4;
	genf *genf1;
	genf *genf2;
};


/* device naming */
const char *device_typename(iodevice_t type);
const char *device_brieftypename(iodevice_t type);
int device_typeid(const char *name);

/* device allocation */
struct IODevice *devices_allocate(const game_driver *gamedrv);

/* device lookup */
const struct IODevice *device_find_tag(const struct IODevice *devices, const char *tag);
int device_count_tag(const struct IODevice *devices, const char *tag);

/* device lookup; both of these function assume only one of each type of device */
const struct IODevice *device_find(const struct IODevice *devices, iodevice_t type);
int device_count(iodevice_t type);

/* diagnostics */
int device_valididtychecks(void);

#endif /* DEVICE_H */
