#ifndef _NESINTF_H_
#define _NESINTF_H_

#include "driver.h"

#define MAX_NESPSG 2

struct NESinterface
{
	int basefreq;
	int cpunum;
	write8_handler apu_callback_w;
	read8_handler apu_callback_r;
};

extern READ8_HANDLER(NESPSG_0_r);
extern READ8_HANDLER(NESPSG_1_r);
extern WRITE8_HANDLER(NESPSG_0_w);
extern WRITE8_HANDLER(NESPSG_1_w);


#endif /* !_NESINTF_H_ */
