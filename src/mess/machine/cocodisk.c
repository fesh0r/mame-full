/* CoCoDisk.c - Emulation of CoCo/Dragon Disk drives for MESS
 *
 * Based on code by Paul Burgin and Stewart Orchard; while most of their code
 * was stripped out, the most important - the algorithm itself - is fully
 * intact and entirely their magificent job.  Without it,  we would still be
 * using crappy disk support the crappy disk support that I (NPW) wrote.
 */

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"

extern UINT8 *coco_rom;


/* The CoCo and Dragon have very similar disk hardware.  But they appear to be
 * different in a few key areas; the registers are in different order, some of
 * the bits are rearranged.  (Note:  All I know about the Dragon's disk drives
 * is what I see by looking at Paul's & Stewart's original code).
 *
 * ---------------------------------------------------------------------------
 * DSKREG - the control register
 * CoCo ($ff40)                            Dragon ($ff48)
 *
 * Bit                                     Bit
 *	7 halt enable flag                      7 ???
 *	6 drive select #3                       6 ???
 *	5 density flag (0=single, 1=double)     5 halt enable flag
 *	4 write precompensation                 4 ???
 *	3 drive motor activation                3 ???
 *	2 drive select #2                       2 drive motor activation
 *	1 drive select #1                       1 drive select high bit
 *	0 drive select #0                       0 drive select low bit
 * ---------------------------------------------------------------------------
 * COMREG - the status register
 * CoCo ($ff48) / Dragon ($ff40)
 *
 * Bit
 *  7 drive not ready
 *  6 write protect
 *  5 head loaded / rec type / write fault
 *  4 seek error
 *  3 crc error*
 *  2 track zero / lost data*
 *  1 index
 *  0 busy
 *       asterisk (*) indicates a fault with the medium
 * ---------------------------------------------------------------------------
 */

/* Macros for hardware registers. */
/* NPW: changed from dragon */
#define COMREG (coco_rom - 0x8000)[0xff48]
#define TRKREG (coco_rom - 0x8000)[0xff49]
#define SECREG (coco_rom - 0x8000)[0xff4a]
#define DATREG (coco_rom - 0x8000)[0xff4b]
#define DSKREG (coco_rom - 0x8000)[0xff40]

/* Macros for CART flag manipulation. */
#define SETCART /* (coco_rom - 0x8000)[0xff23] |= 0x80 */
#define CLRCART /* (coco_rom - 0x8000)[0xff23] &= 0x7f */

/* Bit meanings in 'dc_floppy_status'. */
typedef enum
{
	DCS_OK		= 0x00,
	DCS_BUSY	= 0x01, 	/* operation in progress */
	DCS_TRK0 	= 0x04, 	/* head is over track zero */
	DCS_RF		= 0x10, 	/* sector address record not found */
	DCS_WP		= 0x40,  	/* disk is write protected */
	DCS_NR		= 0x80		/* disk is not ready */
} STAT_TYPE;

/* Possible values of 'operation'. */
typedef enum
{
	DC_IDLE,
	DC_READ,	/* sector read in progress */
	DC_WRITE,   /* sector write in progress */
	DC_FORMAT,  /* format track in progress */
	DC_SEEK     /* seek in progress */
} OP_TYPE;

enum {
	DL_CLOSED,
	DL_ALERT,
	DL_RO,
	DL_RW,
	DL_READY,
	DL_FAILED
};

enum {
	HW_COCO,
	HW_DRAGON
};

/* State of the drive lights. */
static int drive_light[4] = {DL_CLOSED, DL_CLOSED, DL_CLOSED, DL_CLOSED};

/* Keeps track of number of bytes transferred during sector operations. */
static int byte_count;

/* Holds hardware status flags. */
static STAT_TYPE dc_floppy_status = DCS_OK;

/* Current internal operation. */
static OP_TYPE operation = DC_IDLE;

typedef struct {
	void *fd;
	int track;
	int wp;	/* write protected; unused now */
} dc_floppy;

static dc_floppy dc_floppies[4];

static dc_floppy *current_floppy = NULL;

/* Function prototypes. */
static void dc_floppy_nmi(STAT_TYPE ret_stat, int hardware);
static void dc_floppy_reset(void);
static void init_sector_xfer(OP_TYPE xfer_type, int hardware);
static void dc_floppy_seek(unsigned int track, int seektype, int hardware);

/* Handle write to command register $ff40. */
static void dc_floppy_command(char byte, int hardware)
{
	int side;

	side = (byte & 0x02) >> 1;  /* Disk head is encoded in command byte. */

	/* Action the command byte. */
	switch (byte & 0xfc)
	{
					/* Seek track zero. */
		case 0x00 :
		case 0x04 :
		case 0x08 : dc_floppy_seek(TRKREG = 0, (byte & 0x0c) / 4, hardware);
					break;

					/* Seek track number in $ff43. */
		case 0x10 :
		case 0x14 :
		case 0x18 :	dc_floppy_seek(TRKREG = DATREG, (byte & 0x0c) / 4, hardware);
					break;

					/* Step track in +ve direction (no update of $ff41). */
		case 0x40 :
		case 0x44 :
		case 0x48 :	if (current_floppy)
						dc_floppy_seek(current_floppy->track + 1, (byte & 0x0c) / 4, hardware);
					break;

					/* Dragon DOS sector read. */
		case 0x80 :
		case 0x88 : init_sector_xfer(DC_READ, hardware);
					break;

					/* Dragon DOS sector write. */
		case 0xa0 :
		case 0xa8 : if (current_floppy->wp)
						dc_floppy_nmi(DCS_WP, hardware);   /* Write-protected. */
					else
						init_sector_xfer(DC_WRITE, hardware);
					break;

		/* In case of 'format track' command, do nothing other than to
		   signal operation complete on next write to $ff43. */
		case 0xf0 :
		case 0xf4 : if (current_floppy) {
						if (current_floppy->wp) {
							dc_floppy_nmi(DCS_WP, hardware);   /* Write-protected. */
						}
						else {
							SETCART;
							dc_floppy_status = DCS_OK;
							operation = DC_FORMAT;
						}
					}
					break;

					/* Reset hardware status. */
		case 0xd0 :	dc_floppy_reset();
					break;

/*
		Not all valid functions are supported:

		$2x - step +ve if not track zero, no update of track register.
		$3x - step +ve if not track zero.
		$5x - step +ve
		$6x - step -ve, no update of track register.
		$7x - step -ve

		seek subvariants:
			$x4 - check track number after seek.
			$x8 - flagged interrupt.

		$80 - read long sector.

		$90 - read multiple long sectors.
		$98 - read multiple sectors.

		$a0 - write long sector?

		$b0 - write multiple long sectors?
		$b8 - write multiple sectors?

		$cx - read next sector address record.

		$dx subvariants:
			$d4 - index pulse interrupt enable.
			$d8 - interrupt on $ff40 read?

		$ex - raw read track? (complement of format track?)
*/

		default:
			logerror("Bad disk command: command=%i\n", (int) byte);
	}
}

/* Seek track. */
static void dc_floppy_seek(unsigned int track, int seektype, int hardware)
{
	operation = DC_SEEK;
	dc_floppy_status = DCS_OK;

	if (current_floppy) {
		current_floppy->track = track;

		if (current_floppy->wp)
			dc_floppy_status = DCS_WP;

		if (!track)
			dc_floppy_status |= DCS_TRK0;
	}

	dc_floppy_nmi(dc_floppy_status, hardware);
}

/* Set up for sector transfer. */
static void init_sector_xfer(OP_TYPE xfer_type, int hardware)
{
	unsigned long	filepos;
	unsigned int	sector;

	operation = xfer_type;
	dc_floppy_status = DCS_OK;

	byte_count = 256;
	sector = SECREG - 1;

	/* Decide whether we are going to find the requested sector. */
	if (current_floppy) {
		filepos = (current_floppy->track * 18 + sector) * 256;
		osd_fseek(current_floppy->fd, filepos, SEEK_SET);
		SETCART;

		if (hardware == HW_COCO)
			dc_floppy_status |= 0x02;
	}
	else {
		/* Cause immediate error rather than allow time out. */
		dc_floppy_nmi(DCS_RF, hardware);
	}
}

/*
	Produce a byte for data register.
	Triggers NMI when sector operation complete.
*/
static char dc_floppy_read_data(int hardware)
{
	if (operation == DC_READ)
	{
		if (byte_count--)
		{
			SETCART;
			osd_fread(current_floppy->fd, &DATREG, 1);
		}
		else
		{
			dc_floppy_nmi(DCS_OK, hardware);
			return -1;     /* Dummy return value. */
		}
	}
	/* If no operation in progress then register behaves like memory. */
	return (DATREG);
}

/*
	Deal with byte written to data register.
	Triggers NMI when sector operation complete.
*/
static void dc_floppy_write_data(char byte, int hardware)
{
	switch (operation)
	{
		case DC_WRITE:	SETCART;
						osd_fwrite(current_floppy->fd, &byte, 1);
						if (!--byte_count)
							dc_floppy_nmi(DCS_OK, hardware);
						break;

		case DC_FORMAT:	dc_floppy_nmi(DCS_OK, hardware);
						break;
		default:
			logerror("Bad disk drive mode when writing data\n");
        	break;
	}
}

/* Trigger NMI if enabled. */
static void dc_floppy_nmi(STAT_TYPE ret_stat, int hardware)
{
	if (DSKREG & ((hardware == HW_COCO) ? 0x80 : 0x20))
	{
		m6809_set_nmi_line(1);
		m6809_set_nmi_line(0);
		operation = DC_IDLE;
	}
	/* Keep SEEK condition if NMI disabled (for OS9 - see below). */
	else if (operation != DC_SEEK)
		operation = DC_IDLE;

	dc_floppy_status = ret_stat;

	CLRCART;
}

/* Handle read from status register at $ff40 (dragon) or $ff48 (coco). */
static char dc_floppy_read_status(void)
{
	/*
		OS9 compatibility - if NMI was disabled for a seek operation
		then pretend the hardware is busy for one read.
	*/
	if (operation == DC_SEEK)
	{
		operation = DC_IDLE;
		return (dc_floppy_status | DCS_BUSY);
	}

	return (dc_floppy_status);
}

/* Reset disk controller status. */
static void dc_floppy_reset(void)
{
	CLRCART;
	operation = DC_IDLE;
	dc_floppy_status = DCS_OK;

	/* If disk is active, update track zero & write protect bits. */
	if (current_floppy) {
		if (current_floppy->wp)
			dc_floppy_status = DCS_WP;

		if (!current_floppy->track)
			dc_floppy_status |= DCS_TRK0;
	}
}

/*
	Use writes to $ff48 to control virtual disk files.
	File is opened when a drive is accessed & all files
	are closed when the disk motors are turned off.
*/
static void dc_floppy_write_dskreg(char byte, int hardware)
{
	int drive=0, i, motor_mask=0x08;

	switch(hardware) {
	case HW_COCO:
		/* Return if nothing of interest happening. */
/*		if ((byte & 0x4f) == (DSKREG & 0x4f))
			return; */
		if (byte & 0x40)
			drive = 3;
		else if (byte & 0x04)
			drive = 2;
		else if (byte & 0x02)
			drive = 1;
		else
			drive = 0;
		motor_mask = 0x08;
		break;

	case HW_DRAGON:
		/* Return if nothing of interest happening. */
/*		if ((byte & 0x07) == (DSKREG & 0x07))
			return; */
		drive = byte & 0x03;
		motor_mask = 0x04;
		break;
	}

	current_floppy = &dc_floppies[drive];
	if (!current_floppy->fd)
		current_floppy = NULL;

	/* Disk motors activated? */
	if (byte & motor_mask)
	{
		/* Switch off existing drive light. */
		for (i=0; i<4; i++)
		{
			if ((drive_light[i] == DL_RO) || (drive_light[i] == DL_RW))
				drive_light[i] = DL_READY;
		}

		/* Change write protected disks to red and writeable disks to green. */
		if (current_floppy) {
			drive_light[drive] = current_floppy->wp ? DL_RO : DL_RW;
		}
		else {
			drive_light[drive] = DL_FAILED;
		}
	}
	else
	{
		/* Close all files when disk motors turned off. */
		for (i=0; i<4; i++)
		{
			drive_light[i] = DL_READY;
		}
	}
	/* update_drive_lights(); */
}

/* ---------------------------------------------------------------------- */

#if 0
/*
	Reset disk controller registers (power up & hard reset).

	What actually happens is that the controller does a
	'seek track zero' from track 255 which normally takes
	a few seconds. The emulator approach reflects the
	register state when DOS E6 soft-resets the cartridge.

	This code should probably be used...
*/
static void init(void)
{
	unsigned int hw_address;

	/* Set all DOS related registers to zero. */
	for (hw_address = 0xff40; hw_address < 0xff4f; hw_address++)
		(coco_rom - 0x8000)[hw_address] = 0;

	CLRCART;
	operation = DC_SEEK;

	/* Status is actually 'busy'; the OS9 mod above will look after this. */
	dc_floppy_status = DCS_OK;

	TRKREG = 0xfe;
	SECREG = 0x01;
	DATREG = 0x00;
}
#endif

int coco_floppy_init(int id)
{
	const char *name = device_filename(IO_FLOPPY,id);
    if(name==NULL)
		return INIT_OK;
	logerror("coco_floppy_init - name is %s\n", name);
	dc_floppies[id].fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if(!dc_floppies[id].fd)
		dc_floppies[id].fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    dc_floppies[id].track = 0;
	return dc_floppies[id].fd ? INIT_OK : INIT_FAILED;
}

void coco_floppy_exit(int id)
{
	if(dc_floppies[id].fd)
		osd_fclose(dc_floppies[id].fd);
	memset(&dc_floppies[id], 0, sizeof(dc_floppies[id]));

	if (current_floppy == &dc_floppies[id])
		current_floppy = NULL;
}

int coco_floppy_r(int offset)
{
	switch(offset) {
	case 8:
		return dc_floppy_read_status();
		break;
	case 11:
		return dc_floppy_read_data(HW_COCO);
		break;
	default:
		return coco_rom[0x7f40 + offset];
	}
}

void coco_floppy_w(int offset, int data)
{
	switch(offset) {
	case 0:
		dc_floppy_write_dskreg(data, HW_COCO);
		break;
	case 8:
		dc_floppy_command(data, HW_COCO);
		break;
	case 11:
		dc_floppy_write_data(data, HW_COCO);
		break;
	};
	coco_rom[0x7f40 + offset] = data;
}

int dragon_floppy_r(int offset)
{
	switch(offset) {
	case 0:
		return dc_floppy_read_status();
		break;
	case 3:
		return dc_floppy_read_data(HW_COCO);
		break;
	default:
		return coco_rom[0x7f40 + (offset^8)];
	}
}

void dragon_floppy_w(int offset, int data)
{
	switch(offset) {
	case 8:
		dc_floppy_write_dskreg(data, HW_DRAGON);
		break;
	case 0:
		dc_floppy_command(data, HW_DRAGON);
		break;
	case 3:
		dc_floppy_write_data(data, HW_DRAGON);
		break;
	};
	coco_rom[0x7f40 + (offset^8)] = data;
}
