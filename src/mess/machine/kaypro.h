/******************************************************************************
 *
 *	kaypro.h
 *
 *	Machine interface for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#include "mess/machine/cpm_bios.h"

extern int kaypro_floppy_init(int id, const char *name);
extern void init_kaypro(void);
extern void kaypro_init_machine(void);
extern void kaypro_stop_machine(void);

extern int kaypro_interrupt(void);
