#ifndef MM58274C_H
#define MM58274C_H

#include "mame.h"

extern void mm58274c_init(int which);
extern int mm58274c_r(int which, int offset);
extern void mm58274c_w(int which, int offset, int data);

/*extern READ_HANDLER(mm58274c_0_r);
extern WRITE_HANDLER(mm58274c_0_w);

extern READ_HANDLER(mm58274c_1_r);
extern WRITE_HANDLER(mm58274c_1_w);*/

#endif /* MM58274C_H */
