#ifndef _NESINTF_H_
#define _NESINTF_H_

#include "driver.h"

#define MAX_NESPSG 2

struct NESinterface {
	int num;		/* total number of chips in the machine */
	int basefreq;
	int volume[MAX_NESPSG];
	int cpunum[MAX_NESPSG];
	write8_handler apu_callback_w[MAX_NESPSG];
	read8_handler apu_callback_r[MAX_NESPSG];
};

extern int NESPSG_sh_start(const struct MachineSound *msound);
extern void NESPSG_sh_stop(void);
extern void NESPSG_sh_update(void);

extern READ_HANDLER(NESPSG_0_r);
extern READ_HANDLER(NESPSG_1_r);
extern WRITE_HANDLER(NESPSG_0_w);
extern WRITE_HANDLER(NESPSG_1_w);


#endif /* !_NESINTF_H_ */
