#ifndef DEVICE_H
#define DEVICE_H

enum
{
	IO_END = 0,		/* Dummy type to end IODevice enumerations 										*/
	IO_CARTSLOT,	/* Cartidge Port, as found on most console and on some computers 				*/
	IO_FLOPPY,		/* Floppy Disk unit 															*/
	IO_HARDDISK,	/* Hard Disk unit 																*/
	IO_CYLINDER,	/* Magnetically-Coated Cylinder 												*/
	IO_CASSETTE,	/* Cassette Recorder (common on early home computers) 							*/
	IO_PUNCHCARD,	/* Card Puncher/Reader 															*/
	IO_PUNCHTAPE,	/* Tape Puncher/Reader (reels instead of punchcards) 							*/
	IO_PRINTER,		/* Printer device 																*/
	IO_SERIAL,		/* some serial port 															*/
	IO_PARALLEL,    /* Generic Parallel Port														*/
	IO_SNAPSHOT,	/* Complete 'snapshot' of the state of the computer 							*/
	IO_QUICKLOAD,	/* Allow to load program/data into memory, without matching any actual device	*/
	IO_COUNT		/* Total Number of IO_devices for searching										*/
};

struct Devices {
	const int  id;
	const char *name;
	const char *shortname;
};


#endif
