/*****************************************************************************
 *
 *	 cp1600.h
 *	 Portable General Instruments CP1600 emulator interface
 *
 *	 Copyright (c) 2000 Frank Palazzolo, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   palazzol@home.com
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _CP1600_H
#define _CP1600_H

#include "cpuintrf.h"
#include "osd_cpu.h"

enum {
	CP1600_R0=1, CP1600_R1, CP1600_R2, CP1600_R3,
	CP1600_R4, CP1600_R5, CP1600_R6, CP1600_R7
};

#define CP1600_INT_NONE		0
#define CP1600_INT_INTRM	1	/* Maskable */
#define CP1600_INT_INTR		2	/* Non-Maskable */
#define CP1600_RESET		3	/* Non-Maskable */

extern int cp1600_icount;				 /* cycle count */

extern void cp1600_init (void);
extern void cp1600_reset (void *param); 		 /* Reset registers to the initial values */
extern void cp1600_exit  (void);				 /* Shut down CPU core */
extern int	cp1600_execute(int cycles); 		 /* Execute cycles - returns number of cycles actually run */
extern unsigned cp1600_get_context (void *dst);  /* Get registers, return context size */
extern void cp1600_set_context (void *src); 	 /* Set registers */
extern unsigned cp1600_get_reg (int regnum);
extern void cp1600_set_reg (int regnum, unsigned val);
extern void cp1600_set_nmi_line(int state);
extern void cp1600_set_irq_line(int irqline, int state);
extern void cp1600_set_irq_callback(int (*callback)(int irqline));
extern void cp1600_state_save(void *file);
extern void cp1600_state_load(void *file);
extern const char *cp1600_info(void *context, int regnum);
extern unsigned cp1600_dasm(char *buffer, unsigned pc);

/* WRITE_HANDLER( cp1600_internal_w ); */
/* READ_HANDLER( cp1600_internal_r ); */

#ifdef MAME_DEBUG
extern unsigned DasmCP1600( char *dst, unsigned pc );
#endif

#define cp1600_readop(A) cpu_readmem24bew_word(((A)<<1)&0x1fffe)
#define cp1600_readmem16(A) cpu_readmem24bew_word(((A)<<1)&0x1fffe)
#define cp1600_writemem16(A,B) cpu_writemem24bew_word(((A)<<1)&0x1fffe,B)

#endif /* _CP1600_H */
