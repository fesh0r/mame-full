#ifndef MESS_H
#define MESS_H

//#include "driver.h"


//extern char messversion[];

#define ORIENTATION_DEFAULT ROT0 /* hack till we change to the driver macro */

void showmessinfo(void);


/* common.h - begin */
#define MAX_ROM 3         /* MAX_ROM is the maximum number of cartridge slots a driver supports */
#define MAX_FLOPPY 4      /* MAX_FLOPPY is the maximum number of floppy drives a driver supports */
#define MAX_HARD 2        /* MAX_HARD is the maximum number of hard drives a driver supports */
#define MAX_CASSETTE 2    /* MAX_CASSETTE is the maximum number of cassette drives a driver supports */
#define MAX_PATHLEN 2048  /* Maximum Number of char for the path length */

extern char rom_name[MAX_ROM][MAX_PATHLEN];
extern char floppy_name[MAX_FLOPPY][MAX_PATHLEN];
extern char hard_name[MAX_HARD][MAX_PATHLEN];
extern char cassette_name[MAX_CASSETTE][MAX_PATHLEN];

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

char * get_alias(char *driver_name, char *argv);
int check_crc(int crc, int length, char * driver);

/* mess.c functions [for external use] */
int parse_image_types(char *arg);
int load_image(int argc, char **argv, char *driver, int j);



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

#endif
