/*###################################################################################################
**
**
**		armcore.h
**		ARM Dynamic recompiler support routines.
**		Written by Nathan Woods
**
**
**#################################################################################################*/

#ifndef __DRCCORE_H__
#define __DRCCORE_H__


/*###################################################################################################
**	TYPE DEFINITIONS
**#################################################################################################*/


/* core interface structure for the drc common code */
struct drccore
{
	UINT8 *		cache_base;				/* base pointer to the compiler cache */
	UINT8 *		cache_top;				/* current top of cache */
	UINT8 *		cache_danger;			/* high water mark for the end */
	UINT8 *		cache_end;				/* end of cache memory */
	void 		(*entry_point)(void);	/* pointer to asm entry point */
};

/* configuration structure for the drc common code */
struct drcconfig
{
	UINT32		cachesize;				/* size of cache to allocate */
};

/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* architectural defines */
#define COND_EQ		0x00000000
#define COND_NE		0x10000000
#define COND_CS		0x20000000
#define COND_CC		0x30000000
#define COND_MI		0x40000000
#define COND_PL		0x50000000
#define COND_VS		0x60000000
#define COND_VC		0x70000000
#define COND_HI		0x80000000
#define COND_LS		0x90000000
#define COND_GE		0xa0000000
#define COND_LT		0xb0000000
#define COND_GT		0xc0000000
#define COND_LE		0xd0000000
#define COND_AL		0xe0000000

#define OP_AND		0x00000000
#define OP_ANDS		0x00100000
#define OP_EOR		0x00200000
#define OP_EORS		0x00300000
#define OP_SUB		0x00400000
#define OP_SUBS		0x00500000
#define OP_RSB		0x00600000
#define OP_RSBS		0x00700000
#define OP_ADD		0x00800000
#define OP_ADDS		0x00900000
#define OP_ADC		0x00a00000
#define OP_ADCS		0x00b00000
#define OP_SBC		0x00c00000
#define OP_SBCS		0x00d00000
#define OP_RSC		0x00e00000
#define OP_RSCS		0x00f00000
#define OP_TST		0x01000000
#define OP_TSTS		0x01100000
#define OP_TEQ		0x01200000
#define OP_TEQS		0x01300000
#define OP_CMP		0x01400000
#define OP_CMPS		0x01500000
#define OP_CMN		0x01600000
#define OP_CMNS		0x01700000
#define OP_ORR		0x01800000
#define OP_ORRS		0x01900000
#define OP_MOV		0x01a00000
#define OP_MOVS		0x01b00000
#define OP_BIC		0x01c00000
#define OP_BICS		0x01d00000
#define OP_MVN		0x01e00000
#define OP_MVNS		0x01f00000
#define OP_LSL		0x00000000
#define OP_LSR		0x00000020
#define OP_ASR		0x00000040
#define OP_ROR		0x00000060



/*###################################################################################################
**	LOW-LEVEL OPCODE EMITTERS
**#################################################################################################*/

/* lowest-level opcode emitters */
#define OP(x)		do { *((UINT32 *) drc->cache_top) = (UINT32)(x); drc->cache_top += 4; } while (0)

#define OP3(cond, rd, rn, rm, op)	\
	OP((cond) | ((rd) << 12) | ((rn) << 16) | ((rm) << 0) | (op))

#define OP4(cond, rd, rn, rm, o1, o2)	\
	OP3((cond), (rd), (rn), (rm), (o1) | (o2))

#define OP5(cond, rd, rn, rm, o1, o2, o3)	\
	OP3((cond), (rd), (rn), (rm), (o1), (o2) | (o3))


/*###################################################################################################
**	MOVE EMITTERS
**#################################################################################################*/

#define _ldrh_r16_m16bd(cond, dreg, base, disp)	\
	OP3((cond), (dreg), (base), (disp), 0x005000b0)

#define _ldrh_r16_m16id(cond, dreg, base, index) \
	OP3((cond), (dreg), (base), (index), 0x019000b0)

/*###################################################################################################
**	BRANCH EMITTERS
**#################################################################################################*/

#define _bcc(cond, target)	\
	OP((cond) | 0x0a000000 | (((((UINT8 *) (target)) - params->blitter->cache_top - 8) >> 2) & 0x00ffffff))

/*###################################################################################################
**	FUNCTION PROTOTYPES
**#################################################################################################*/

#define drc_init	arm_drc_init
#define drc_exit	arm_drc_exit

/* init/shutdown */
struct drccore *drc_init(UINT8 cpunum, struct drcconfig *config);
void drc_exit(struct drccore *drc);

#endif /* __DRCCORE_H__ */

