/***************************************************************************

	device.h

	Definitions and manipulations for device structures

***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include "osdepend.h"

typedef enum
{
	/* List of all supported devices.  Refer to the device by these names only							*/
	IO_CARTSLOT,	/*  0 - Cartridge Port, as found on most console and on some computers 				*/
	IO_FLOPPY,		/*  1 - Floppy Disk unit 															*/
	IO_HARDDISK,	/*  2 - Hard Disk unit 																*/
	IO_CYLINDER,	/*  3 - Magnetically-Coated Cylinder 												*/
	IO_CASSETTE,	/*  4 - Cassette Recorder (common on early home computers) 							*/
	IO_PUNCHCARD,	/*  5 - Card Puncher/Reader 														*/
	IO_PUNCHTAPE,	/*  6 - Tape Puncher/Reader (reels instead of punchcards) 							*/
	IO_PRINTER,		/*  7 - Printer device 																*/
	IO_SERIAL,		/*  8 - Generic Serial Port */
	IO_PARALLEL,    /*  9 - Generic Parallel Port														*/
	IO_SNAPSHOT,	/* 10 - Complete 'snapshot' of the state of the computer 							*/
	IO_QUICKLOAD,	/* 11 - Allow to load program/data into memory, without matching any actual device	*/
	IO_MEMCARD,		/* 12 - Memory card																	*/
	IO_COUNT		/* 13 - Total Number of IO_devices for searching									*/
} iodevice_t;

/* Call this from the CLI to add a DEVICE (with its arg) to the options struct */
int register_device (const int type, const char *arg);

/* Device handlers */
int device_open(mess_image *img, int mode, void *args);
void device_close(mess_image *img);
int device_seek(mess_image *img, int offset, int whence);
int device_tell(mess_image *img);
int device_status(mess_image *img, int newstatus);

#endif
