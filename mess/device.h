#ifndef DEVICE_H
#define DEVICE_H

enum
{	/* List of all supported devices.  Refer to the device by these names only							*/
	IO_CARTSLOT,	/*  0 - Cartidge Port, as found on most console and on some computers 				*/
	IO_FLOPPY,		/*  1 - Floppy Disk unit 															*/
	IO_HARDDISK,	/*  2 - Hard Disk unit 																*/
	IO_CYLINDER,	/*  3 - Magnetically-Coated Cylinder 												*/
	IO_CASSETTE,	/*  4 - Cassette Recorder (common on early home computers) 							*/
	IO_PUNCHCARD,	/*  5 - Card Puncher/Reader 															*/
	IO_PUNCHTAPE,	/*  6 - Tape Puncher/Reader (reels instead of punchcards) 							*/
	IO_PRINTER,		/*  7 - Printer device 																*/
	IO_SERIAL,		/*  8 - some serial port 															*/
	IO_PARALLEL,    /*  9 - Generic Parallel Port														*/
	IO_SNAPSHOT,	/* 10 - Complete 'snapshot' of the state of the computer 							*/
	IO_QUICKLOAD,	/* 11 - Allow to load program/data into memory, without matching any actual device	*/
	IO_COUNT		/* 12 - Total Number of IO_devices for searching										*/
};

/* Call this from the CLI to add a DEVICE (with its arg) to the options struct */
int register_device (const int type, const char *arg);

/* Device handlers */
extern int device_open(int type, int id, int mode, void *args);
extern void device_close(int type, int id);
extern int device_seek(int type, int id, int offset, int whence);
extern int device_tell(int type, int id);
extern int device_status(int type, int id, int newstatus);
extern int device_input(int type, int id);
extern void device_output(int type, int id, int data);

#endif
