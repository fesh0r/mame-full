#ifndef _PDP1_H
#define _PDP1_H

#include "memory.h"
/* PDP1 Registers */
typedef struct
{
 int pc;
 int ac;
 int io;
 int y;
 int ib;
 int ov;
 int f;
 int flag[8];
 int sense[8];
 int pending_interrupts;
} pdp1_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

/* PUBLIC FUNCTIONS */
void pdp1_SetRegs(pdp1_Regs *Regs);
void pdp1_GetRegs(pdp1_Regs *Regs);
void pdp1_set_regs_true(pdp1_Regs *Regs);
unsigned pdp1_GetPC(void);
void pdp1_reset(void);
int pdp1_execute(int cycles);
void pdp1_Cause_Interrupt(int type);
void pdp1_Clear_Pending_Interrupts(void);
int dasmpdp1 (char *buffer, int pc);

extern int pdp1_ICount;
extern int (* extern_iot)(int *, int);

#define READ_PDP_18BIT(A) ((signed)cpu_readmem16(A))
#define WRITE_PDP_18BIT(A,V) (cpu_writemem16(A,V))

#define AND 001
#define IOR 002
#define XOR 003
#define XCT 004
#define CALJDA 007
#define LAC 010
#define LIO 011
#define DAC 012
#define DAP 013
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


#endif /* _PDP1_H */
