/*********************************************************************

	sonydriv.h
	
	Apple/Sony 3.5" floppy drive emulation (to be interfaced with iwm.c)

*********************************************************************/

#ifndef SONYDRIV_H
#define SONYDRIV_H

#include "osdepend.h"
#include "fileio.h"
#include "image.h"

/* defines for the allowablesizes param below */
enum
{
	SONY_FLOPPY_ALLOW400K	= 1,
	SONY_FLOPPY_ALLOW800K	= 2,

	SONY_FLOPPY_EXT_SPEED_CONTROL = 0x8000	/* means the speed is controlled by computer */
};

void sonydriv_device_getinfo(struct IODevice *dev, int allowablesizes);

void sony_set_lines(data8_t lines);
void sony_set_enable_lines(int enable_mask);
void sony_set_sel_line(int sel);

void sony_set_speed(int speed);

data8_t sony_read_data(void);
void sony_write_data(data8_t data);
int sony_read_status(void);

#endif /* SONYDRIV_H */
