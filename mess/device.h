#ifndef DEVICE_H
#define DEVICE_H

enum
{	/* List of all supported devices.  Refer to the device by these names only							*/
	IO_END = 0,		/*  0 - Dummy type to end IODevice enumerations 									*/
	IO_CARTSLOT,	/*  1 - Cartidge Port, as found on most console and on some computers 				*/
	IO_FLOPPY,		/*  2 - Floppy Disk unit 															*/
	IO_HARDDISK,	/*  3 - Hard Disk unit 																*/
	IO_CYLINDER,	/*  4 - Magnetically-Coated Cylinder 												*/
	IO_CASSETTE,	/*  5 - Cassette Recorder (common on early home computers) 							*/
	IO_PUNCHCARD,	/*  6 - Card Puncher/Reader 															*/
	IO_PUNCHTAPE,	/*  7 - Tape Puncher/Reader (reels instead of punchcards) 							*/
	IO_PRINTER,		/*  8 - Printer device 																*/
	IO_SERIAL,		/*  9 - some serial port 															*/
	IO_PARALLEL,    /* 10 - Generic Parallel Port														*/
	IO_SNAPSHOT,	/* 11 - Complete 'snapshot' of the state of the computer 							*/
	IO_QUICKLOAD,	/* 12 - Allow to load program/data into memory, without matching any actual device	*/
	IO_COUNT		/* 13 - Total Number of IO_devices for searching										*/
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
