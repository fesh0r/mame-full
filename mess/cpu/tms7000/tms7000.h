/*** tms7000: Portable tms7000 emulator ******************************************/

#ifndef _TMS7000_H
#define _TMS7000_H

#include "memory.h"
#include "osd_cpu.h"

enum { TMS7000_PC=1, TMS7000_SP, TMS7000_ST };

extern int tms7000_icount;

/* PUBLIC FUNCTIONS */
extern unsigned tms7000_get_context(void *dst);
extern void tms7000_set_context(void *src);
extern unsigned tms7000_get_pc(void);
extern void tms7000_set_pc(unsigned val);
extern unsigned tms7000_get_sp(void);
extern void tms7000_set_sp(unsigned val);
extern void tms7000_set_reg(int regnum, unsigned val);
extern void tms7000_init(void);
extern void tms7000_reset(void *param);
extern void tms7000_exit(void);
extern const char *tms7000_info(void *context, int regnum);
extern unsigned tms7000_dasm(char *buffer, unsigned pc);
extern void tms7000_set_nmi_line(int state);
extern void tms7000_set_irq_line(int irqline, int state);
extern void tms7000_set_irq_callback(int (*callback)(int irqline));
extern WRITE_HANDLER( tms7000_internal_w );
extern READ_HANDLER( tms7000_internal_r );
extern int tms7000_execute(int cycles);
extern unsigned tms7000_get_reg(int regnum);

#ifdef MAME_DEBUG
extern unsigned Dasm7000 (char *buffer, unsigned pc);
#endif

#endif /* _TMS7000_H */

