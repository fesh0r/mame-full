#ifndef WAVE_H
#define WAVE_H

/*****************************************************************************
 *	CassetteWave interface
 *****************************************************************************/

#include "messdrv.h"

struct Wave_interface
{
	int num;
	int mixing_level[MAX_DEV_INSTANCES];
};

int wave_sh_start(const struct MachineSound *msound);
void wave_sh_stop(void);
void wave_sh_update(void);


#endif /* WAVE_H */


