#ifndef __SID_6581_H_
#define __SID_6581_H_

extern void sid6581_init(int(*paddle)(int offset));
extern void sid6581_port_w(int offset, int data);
extern int sid6581_port_r(int offset);

#endif
