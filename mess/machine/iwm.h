#ifndef IWM_H
#define IWM_H

#include "driver.h"

typedef struct iwm_interface
{
	void (*set_lines)(int lines);
	void (*set_enable_lines)(int enable_mask);

	int (*read_data)(void);
	void (*write_data)(int data);
	int (*read_status)(void);
} iwm_interface;

void iwm_init(const iwm_interface *intf);
int iwm_r(int offset);
void iwm_w(int offset, int data);

#endif /* IWM_H */
