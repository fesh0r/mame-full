#include "driver.h"


extern int	pc_harddisk_init(int id);
extern void pc_harddisk_exit(int id);

#if 0
/* from machine/pc_hdc.c */
extern void *pc_hdc_file[4];

extern void pc_hdc_data_w(int n, int data);
extern void pc_hdc_reset_w(int n, int data);
extern void pc_hdc_select_w(int n, int data);
extern void pc_hdc_control_w(int n, int data);

extern int	pc_hdc_data_r(int n);
extern int	pc_hdc_status_r(int n);
extern int	pc_hdc_dipswitch_r(int n);
#endif

extern WRITE_HANDLER ( pc_HDC1_w );
extern READ_HANDLER (	pc_HDC1_r );
extern WRITE_HANDLER ( pc_HDC2_w );
extern READ_HANDLER ( pc_HDC2_r );

