#ifndef IWM_H
#define IWM_H

#include "driver.h"

enum {
	IWM_FLOPPY_ALLOW400K	= 1,
	IWM_FLOPPY_ALLOW800K	= 2
};

int iwm_lisa_floppy_init(int id, int allowablesizes);
void iwm_lisa_floppy_exit(int id);

void iwm_lisa_init(void);
int iwm_lisa_r(int offset);
void iwm_lisa_w(int offset, int data);
void iwm_lisa_set_head_line(int head);
/*int iwm_lisa_get_sel_line(void);*/

#endif /* IWM_H */
