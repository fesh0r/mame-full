#ifndef PC_HDC_H
#define PC_HDC_H

#include "driver.h"

int pc_hdc_setup(void);

WRITE_HANDLER ( pc_HDC1_w );
READ_HANDLER (	pc_HDC1_r );
WRITE_HANDLER ( pc_HDC2_w );
READ_HANDLER ( pc_HDC2_r );

int pc_hdc_dack_r(void);
void pc_hdc_dack_w(int data);

#endif /* PC_HDC_H */
