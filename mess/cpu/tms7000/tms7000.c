/*****************************************************************************
 *
 *	 tms7000.c
 *	 Portable TMS7000 emulator (Texas Instruments 7000)
 *
 *	 Copyright (c) 2001 tim lindner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   tlindner@ix.netcom.com
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "tms7000.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* public globals */
int tms7000_icount;
int	tms7000_MC;

void tms7000_set_mc_line( int value )
{
	tms7000_MC = value;
}

static UINT8 tms7000_reg_layout[] = {
	TMS7000_PC, TMS7000_SP, TMS7000_ST, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 tms7000_win_layout[] = {
	27, 0,53, 4,	/* register window (top, right rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

#define RM(Addr) ((unsigned)cpu_readmem16(Addr))
#define SRM(Addr) ((signed)cpu_readmem16(Addr))
#define WM(Addr,Value) (cpu_writemem16(Addr,Value))

UINT16 RM16( UINT32 mAddr );	/* Read memory (16-bit) */
UINT16 RM16( UINT32 mAddr )
{
	UINT32 result = RM(mAddr) << 8;
	return result | RM((mAddr+1)&0xffff);
}

void WM16( UINT32 mAddr, PAIR p );	/*Write memory file (16 bit) */
void WM16( UINT32 mAddr, PAIR p )
{
	WM( mAddr, p.b.h );
	WM( (mAddr+1)&0xffff, p.b.l );
}

PAIR RRF16( UINT32 mAddr ); /*Read register file (16 bit) */
PAIR RRF16( UINT32 mAddr )
{
	PAIR result;
	result.b.h = RM((mAddr-1)&0xffff);
	result.b.l = RM(mAddr);
	return result;
}

void WRF16( UINT32 mAddr, PAIR p ); /*Write register file (16 bit) */
void WRF16( UINT32 mAddr, PAIR p )
{
	WM( (mAddr-1)&0xffff, p.b.h );
	WM( mAddr, p.b.l );
}

UINT16	bcd_add( UINT16 a, UINT16 b );
UINT16	bcd_add( UINT16 a, UINT16 b )
{
	UINT16	t1,t2,t3,t4,t5,t6;
	
	/* Sure it is a lot of code, but it works! */
	t1 = a + 0x0666;
	t2 = t1 + b;
	t3 = t1 ^ b;
	t4 = t2 ^ t3;
	t5 = ~t4 & 0x1110;
	t6 = (t5 >> 2) | (t5 >> 3);
	return t2-t6;
}

UINT16 bcd_tencomp( UINT16 a );
UINT16 bcd_tencomp( UINT16 a )
{
	UINT16	t1,t2,t3,t4,t5,t6;
	
	t1 = 0xffff - a;
	t2 = -a;
	t3 = t1 ^ 0x0001;
	t4 = t2 ^ t3;
	t5 = ~t4 & 0x1110;
	t6 = (t5 >> 2)|(t5>>3);
	return t2-t6;
}

UINT16 bcd_sub( UINT16 a, UINT16 b);
UINT16 bcd_sub( UINT16 a, UINT16 b)
{
	return bcd_tencomp(b) - bcd_tencomp(a);
}

#define IMMBYTE(b)	b = ((unsigned)cpu_readop_arg(pPC)); pPC++
#define SIMMBYTE(b)	b = ((signed)cpu_readop_arg(pPC)); pPC++
#define IMMWORD(w)	w.b.h = (unsigned)cpu_readop_arg(pPC++); w.b.l = (unsigned)cpu_readop_arg(pPC++)

#define PUSHBYTE(b) pSP++; WM(pSP,b)
#define PUSHWORD(w) pSP++; WM(pSP,w.b.h); pSP++; WM(pSP,w.b.l)
#define PULLBYTE(b) b = RM(pSP); pSP--
#define PULLWORD(w) w.b.l = RM(pSP); pSP--; w.b.h = RM(pSP); pSP--

typedef struct
{
	PAIR		pc; 		/* Program counter */
	UINT8		sp;			/* Stack Pointer */
	UINT8		sr;			/* Status Register */
} tms7000_Regs;

static tms7000_Regs tms7000;

#define pPC		tms7000.pc.w.l
#define PC		tms7000.pc
#define pSP		tms7000.sp
#define pSR		tms7000.sr

#define RDA		RM(0x0000)
#define RDB		RM(0x0001)
#define WRA(Value) (cpu_writemem16(0x0000,Value))
#define WRB(Value) (cpu_writemem16(0x0001,Value))

#define SR_C	0x80		/* Carry */
#define SR_N	0x40		/* Negative */
#define SR_Z	0x20		/* Zero */
#define SR_I	0x10		/* Interrupt */

#define CLR_NZC 	pSR&=~(SR_N|SR_Z|SR_C)
#define CLR_NZCI 	pSR&=~(SR_N|SR_Z|SR_C|SR_I)
#define SET_C8(a)	pSR|=((a&0x0100)>>1)
#define SET_N8(a)	pSR|=((a&0x0080)>>1)
#define SET_Z(a)	if(!a)pSR|=SR_Z
#define SET_Z8(a)	SET_Z((UINT8)a)
#define SET_Z16(a)	SET_Z((UINT8)a>>8)
#define GET_C		(pSR >> 7)

/* Not working */
#define SET_C16(a)	pSR|=((a&0x010000)>>9)

#define SETC		pSR |= SR_C
#define SETZ		pSR |= SR_Z
#define SETN		pSR |= SR_N

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned tms7000_get_context(void *dst)
{
	if( dst )
		*(tms7000_Regs*)dst = tms7000;
	return sizeof(tms7000_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void tms7000_set_context(void *src)
{
	if( src )
		tms7000 = *(tms7000_Regs*)src;
}

/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned tms7000_get_pc(void)
{
	return pPC;
}


/****************************************************************************
 * Set program counter
 ****************************************************************************/
void tms7000_set_pc(unsigned val)
{
	pPC = val;
}


/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned tms7000_get_sp(void)
{
	return pSP;
}


/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void tms7000_set_sp(unsigned val)
{
	pSP = val;
}

unsigned tms7000_get_reg(int regnum)
{
	switch( regnum )
	{
		case TMS7000_PC: return pPC;
		case TMS7000_SP: return pSP;
		case TMS7000_ST: return pSR;
	}
	return 0;
}

void tms7000_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case TMS7000_PC: pPC = val; break;
		case TMS7000_SP: pSP = val; break;
		case TMS7000_ST: pSR = val; break;
	}
}

void tms7000_init(void)
{
	int cpu = cpu_getactivecpu();
	state_save_register_UINT16("tms7000", cpu, "PC", &pPC, 1);
	state_save_register_UINT8("tms7000", cpu, "SP", &pSP, 1);
	state_save_register_UINT8("tms7000", cpu, "SR", &pSR, 1);
}

void tms7000_reset(void *param)
{
	
	pSP = 0x01;
	pSR = 0x00;
	WRA( tms7000.pc.b.h );
	WRB( tms7000.pc.b.l );
	pPC = RM16(0xfffe);
}

void tms7000_exit(void)
{
	/* nothing to do ? */
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *tms7000_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	tms7000_Regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &tms7000;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "TMS7000";
		case CPU_INFO_FAMILY: return "Texas Instruments TMS7000";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) tim lindner 2001";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms7000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms7000_win_layout;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->sr & 0x80 ? 'C':'c',
				r->sr & 0x40 ? 'N':'n',
				r->sr & 0x20 ? 'Z':'z',
				r->sr & 0x10 ? 'I':'i',
				r->sr & 0x08 ? '?':'.',
				r->sr & 0x04 ? '?':'.',
				r->sr & 0x02 ? '?':'.',
				r->sr & 0x01 ? '?':'.' );
			break;
		case CPU_INFO_REG+TMS7000_PC: sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+TMS7000_SP: sprintf(buffer[which], "SP:%02X", r->sp); break;
		case CPU_INFO_REG+TMS7000_ST: sprintf(buffer[which], "ST:%02X", r->sr); break;
	}
	return buffer[which];
}

unsigned tms7000_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm7000(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

void tms7000_set_nmi_line(int state)
{
}

void tms7000_set_irq_line(int irqline, int state)
{
}

void tms7000_set_irq_callback(int (*callback)(int irqline))
{
}

WRITE_HANDLER( tms7000_internal_w )
{
}

READ_HANDLER( tms7000_internal_r )
{
	return 0xff;
}

#include "tms70op.c"
#include "tms70tb.c"

int tms7000_execute(int cycles)
{
	int op;
	
	tms7000_icount = cycles;

	do
	{
		CALL_MAME_DEBUG;
		op = cpu_readop(pPC);
		pPC++;

		opfn[op]();
	} while( tms7000_icount > 0 );
	
	return cycles - tms7000_icount;

}