#ifndef IWM_H
#define IWM_H

#include "driver.h"

enum {
	IWM_FLOPPY_ALLOW400K	= 1,
	IWM_FLOPPY_ALLOW800K	= 2
};

int iwm_floppy_init(int id, int allowablesizes);
void iwm_floppy_exit(int id);

void iwm_init(void);
int iwm_r(int offset);
void iwm_w(int offset, int data);
void iwm_set_sel_line(int sel);
int iwm_get_sel_line(void);

#endif /* IWM_H */
