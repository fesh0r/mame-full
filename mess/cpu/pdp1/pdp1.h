#ifndef _PDP1_H
#define _PDP1_H

#include "memory.h"
#include "osd_cpu.h"


/* register ids for pdp1_get_reg/pdp1_set_reg */
enum {
	PDP1_PC=1, PDP1_IR, PDP1_MB, PDP1_MA, PDP1_AC, PDP1_IO,
	PDP1_PF, PDP1_PF1, PDP1_PF2, PDP1_PF3, PDP1_PF4, PDP1_PF5, PDP1_PF6,
	PDP1_TA, PDP1_TW,
	PDP1_SS, PDP1_SS1, PDP1_SS2, PDP1_SS3, PDP1_SS4, PDP1_SS5, PDP1_SS6,
	PDP1_SNGL_STEP, PDP1_SNGL_INST, PDP1_EXTEND_SW,
	PDP1_RUN, PDP1_CYC, PDP1_DEFER, PDP1_OV, PDP1_RIM, PDP1_EXD,
	PDP1_START_CLEAR	/* hack, do not use directly, use pdp1_pulse_start_clear instead */
};

#define pdp1_pulse_start_clear()	pdp1_set_reg(PDP1_START_CLEAR, 0)


typedef struct pdp1_reset_param_t
{
	/* callback for the iot instruction (required for CPU I/O) */
	int (*extern_iot)(int *, int);
	/* read a byte from the perforated tape reader (required for read-in mode) */
	int (*read_binary_word)(UINT32 *reply);

	/* 0: no extend support, 1: extend with 15-bit address, 2: extend with 16-bit address */
	int extend_support;
} pdp1_reset_param_t;


/* PUBLIC FUNCTIONS */
extern void pdp1_init(void);
extern unsigned pdp1_get_context (void *dst);
extern void pdp1_set_context (void *src);
extern unsigned pdp1_get_reg (int regnum);
extern void pdp1_set_reg (int regnum, unsigned val);
extern void pdp1_set_irq_line(int irqline, int state);
extern void pdp1_set_irq_callback(int (*callback)(int irqline));
extern void pdp1_reset(void *param);
extern void pdp1_exit (void);
extern int pdp1_execute(int cycles);
extern const char *pdp1_info(void *context, int regnum);
extern unsigned pdp1_dasm(char *buffer, unsigned pc);

extern int pdp1_ICount;

#ifndef SUPPORT_ODD_WORD_SIZES
#define READ_PDP_18BIT(A) ((signed)cpu_readmem24bedw_dword((A)<<2))
#define WRITE_PDP_18BIT(A,V) (cpu_writemem24bedw_dword((A)<<2,(V)))
#else
#define READ_PDP_18BIT(A) ((signed)cpu_readmem16_18(A))
#define WRITE_PDP_18BIT(A,V) (cpu_writemem16_18((A),(V)))
#endif

#define AND 001
#define IOR 002
#define XOR 003
#define XCT 004
#define CALJDA 007
#define LAC 010
#define LIO 011
#define DAC 012
#define DAP 013
#define DIP 014
#define DIO 015
#define DZM 016
#define ADD 020
#define SUB 021
#define IDX 022
#define ISP 023
#define SAD 024
#define SAS 025
#define MUS 026
#define DIS 027
#define JMP 030
#define JSP 031
#define SKP 032
#define SFT 033
#define LAW 034
#define IOT 035
#define OPR 037

#ifdef MAME_DEBUG
extern unsigned dasmpdp1(char *buffer, unsigned pc);
#endif

#endif /* _PDP1_H */
