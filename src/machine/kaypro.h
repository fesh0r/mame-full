/******************************************************************************
 *
 *	kaypro.h
 *
 *	Machine interface for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#include "machine/cpm_bios.h"

extern void kaypro_init_machine(void);
extern void kaypro_stop_machine(void);
extern int	kaypro_rom_load(void);
extern int	kaypro_rom_id(const char * name, const char * gamename);

extern int kaypro_interrupt(void);
