/*
**
**
** File: scc.h -- header file for software implementation of
**	Konami's SCC sound chip for MSX. SCC and SCC+.
**
** Based on Sound.c in fMSX by (C) Ville Hallik (ville@physic.ut.ee) 1996
**
** By Sean Young <sean@msxnet.org>
*/

#ifndef _H_SCC_SCC_
#define _H_SCC_SCC_

/* different types of SCC modes/types */
#define SCC_MEGAROM	(0)	/* if emulating megarom SCC */
#define SCC_PLUSCOMP	(1)	/* if emulating SCC+ in compatiblity mode */
#define SCC_PLUSEXT	(2)	/* if emulating SCC+ in extended mode */

int SCC_sh_start (const struct MachineSound *msound);
void SCCSetClock (int chip, int clock);
void SCCResetChip (int chip);
void SCCWriteReg (int chip, int reg, int value, int type);

#define MAX_SCC		(4)
#define SCC_INTERPOLATE

#endif /* _H_SCC_SCC_ */

