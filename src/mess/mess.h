#ifndef MESS_H
#define MESS_H

#include "driver.h"


//extern char messversion[];

void showmessinfo(void);



/* common.h - begin */
#define MAX_ROM 3        /* MAX_ROM is the maximum number of cartridge slots a driver supports */
#define MAX_FLOPPY 4     /* MAX_FLOPPY is the maximum number of floppy drives a driver supports */
#define MAX_HARD 2       /* MAX_HARD is the maximum number of hard drives a driver supports */
#define MAX_CASSETTE 2   /* MAX_CASSETTE is the maximum number of cassette drives a driver supports */


extern char rom_name[MAX_ROM][2048];
extern char floppy_name[MAX_FLOPPY][2048];
extern char hard_name[MAX_HARD][2048];
extern char cassette_name[MAX_CASSETTE][2048];

/*
void drawgfx_line(struct osd_bitmap *dest,
						struct GfxElement *gfx,
                  unsigned int code,
                  unsigned int color,
                  int flipx,
                  int start,
                  int sx,
                  int sy,
                  struct rectangle *clip,
                  int transparency,
                  int transparent_color);
*/
/* common.h - end */



/* driver.h - begin */
#define IPT_SELECT1		IPT_COIN1
#define IPT_SELECT2		IPT_COIN2
#define IPT_SELECT3		IPT_COIN3
#define IPT_SELECT4		IPT_COIN4
#define IPT_KEYBOARD	IPT_TILT
/* driver.h - end */


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
