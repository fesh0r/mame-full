#ifndef Z80_MSX_H
#define Z80_MSX_H

#include "cpuintrf.h"
#include "osd_cpu.h"

extern int z80_msx_ICount;          /* T-state count                        */

extern void z80_msx_init(void);
extern void z80_msx_reset (void *param);
extern void z80_msx_exit (void);
extern int z80_msx_execute(int cycles);
extern void z80_msx_burn(int cycles);
extern unsigned z80_msx_get_context (void *dst);
extern void z80_msx_set_context (void *src);
extern const void *z80_msx_get_cycle_table (int which);
extern void z80_msx_set_cycle_table (int which, void *new_tbl);
extern unsigned z80_msx_get_pc (void);
extern void z80_msx_set_pc (unsigned val);
extern unsigned z80_msx_get_sp (void);
extern void z80_msx_set_sp (unsigned val);
extern unsigned z80_msx_get_reg (int regnum);
extern void z80_msx_set_reg (int regnum, unsigned val);
extern void z80_msx_set_nmi_line(int state);
extern void z80_msx_set_irq_line(int irqline, int state);
extern void z80_msx_set_irq_callback(int (*irq_callback)(int));
extern const char *z80_msx_info(void *context, int regnum);
extern unsigned z80_msx_dasm(char *buffer, unsigned pc);

#endif

