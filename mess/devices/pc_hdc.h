#ifndef PC_HDC_H
#define PC_HDC_H

#include "driver.h"

#define CONFIG_DEVICE_PC_HARDDISK(count) \
	CONFIG_DEVICE_LEGACY(IO_HARDDISK, 4, "img\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_RW, NULL, NULL, NULL, NULL, NULL)


#if 0
/* from machine/pc_hdc.c */
extern mame_file *pc_hdc_file[4];

void pc_hdc_data_w(int n, int data);
void pc_hdc_reset_w(int n, int data);
void pc_hdc_select_w(int n, int data);
void pc_hdc_control_w(int n, int data);

int	pc_hdc_data_r(int n);
int	pc_hdc_status_r(int n);
int	pc_hdc_dipswitch_r(int n);
#endif

WRITE_HANDLER ( pc_HDC1_w );
READ_HANDLER (	pc_HDC1_r );
WRITE_HANDLER ( pc_HDC2_w );
READ_HANDLER ( pc_HDC2_r );

void pc_harddisk_state(void);

#endif /* PC_HDC_H */
