/*
	sonydriv.h : interface file for sonydriv.c
*/
#ifndef SONYDRIV_H
#define SONYDRIV_H

#include "osdepend.h"
#include "fileio.h"

/* defines for the allowablesizes param below */
enum
{
	SONY_FLOPPY_ALLOW400K	= 1,
	SONY_FLOPPY_ALLOW800K	= 2,

	SONY_FLOPPY_EXT_SPEED_CONTROL = 0x8000	/* means the speed is controlled by computer */
};

int sony_floppy_load(mess_image *img, mame_file *fp, int open_mode, int allowablesizes);
void sony_floppy_unload(mess_image *img);

void sony_set_lines(int lines);
void sony_set_enable_lines(int enable_mask);
void sony_set_sel_line(int sel);
/*int sony_get_sel_line(void);*/

void sony_set_speed(int speed);

int sony_read_data(void);
void sony_write_data(int data);
int sony_read_status(void);

#endif /* SONYDRIV_H */
