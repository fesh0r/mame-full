//============================================================
//
//	emitarm.cpp - WinCE ARM specific code generation
//
//============================================================
//
//	registers used by blitter:
//		R0		dest bits
//		R1		source bits
//		R2		palette index
//		R3		blend register
//		R4		dest height
//		R5		scratch half word
//		R6		scratch
//		R7		scratch
//		R8		blend mask
//============================================================

#include "emitblit.h"

//============================================================
// enumerations
//============================================================

enum
{
	RDEST	= 0,
	RSRC	= 1,
	RPAL	= 2,
	RBLEND	= 3,
	RROWS	= 4,
	RSCH	= 5,
	RSC2	= 6,
	RSC3	= 7,
	RSP		= 13,
	RLINK	= 14,
	RPC		= 15
};

enum
{
	EQ		= 0x00000000,
	NE		= 0x10000000,
	CS		= 0x20000000,
	CC		= 0x30000000,
	MI		= 0x40000000,
	PL		= 0x50000000,
	VS		= 0x60000000,
	VC		= 0x70000000,
	HI		= 0x80000000,
	LS		= 0x90000000,
	GE		= 0xa0000000,
	LT		= 0xb0000000,
	GT		= 0xc0000000,
	LE		= 0xd0000000,
	AL		= 0xe0000000
};

enum
{
	AND		= 0x00000000,
	ANDS	= 0x00100000,
	EOR		= 0x00200000,
	EORS	= 0x00300000,
	SUB		= 0x00400000,
	SUBS	= 0x00500000,
	RSB		= 0x00600000,
	RSBS	= 0x00700000,
	ADD		= 0x00800000,
	ADDS	= 0x00900000,
	ADC		= 0x00a00000,
	ADCS	= 0x00b00000,
	SBC		= 0x00c00000,
	SBCS	= 0x00d00000,
	RSC		= 0x00e00000,
	RSCS	= 0x00f00000,
	TST		= 0x01000000,
	TSTS	= 0x01100000,
	TEQ		= 0x01200000,
	TEQS	= 0x01300000,
	CMP		= 0x01400000,
	CMPS	= 0x01500000,
	CMN		= 0x01600000,
	CMNS	= 0x01700000,
	ORR		= 0x01800000,
	ORRS	= 0x01900000,
	MOV		= 0x01a00000,
	MOVS	= 0x01b00000,
	BIC		= 0x01c00000,
	BICS	= 0x01d00000,
	MVN		= 0x01e00000,
	MVNS	= 0x01f00000
};

enum
{
	LSL		= 0x00000000,
	LSR		= 0x00000020,
	ASR		= 0x00000040,
	ROR		= 0x00000060
};

//============================================================

static void emit_regreg_shift(struct blitter_params *params, INT32 opcode, unsigned int rd, unsigned int r1,
	unsigned int r2, INT32 shift_type, unsigned int shift)
{
	struct drccore *drc = params->blitter;

	assert(rd < 16);
	assert(r1 < 16);
	assert(r2 < 16);
	assert(shift < 16);

	if (shift == 0)
		shift_type = LSL;

	opcode |= AL  | (rd << 12) | (r1 << 16) | (r2 << 0) | shift_type | (shift << 7);
	OP(opcode);
}

static void emit_regreg(struct blitter_params *params, INT32 opcode, unsigned int rd, unsigned int r1, unsigned int r2)
{
	emit_regreg_shift(params, opcode, rd, r1, r2, LSR, 0);
}

static int count_bits(INT32 word)
{
	int i, bits = 0;
	UINT32 uword = (UINT32) word;

	for (i = 0; i < 32; i++)
	{
		if (word & 1)
			bits++;
		uword >>= 1;
	}
	return bits;
}

static void emit_regi(struct blitter_params *params, INT32 opcode, int rd, int r1, INT32 operand)
{
	int ror;
	int shift;
	int new_operand;
	int rtemp;
	UINT32 tmp;
	INT32 subopcode1;
	INT32 subopcode2;
	struct drccore *drc = params->blitter;

	assert(rd < 16);
	assert(r1 < 16);

	// are we superfulous?
	if (r1 == rd)
	{
		switch(opcode) {
		case ADD:
		case SUB:
		case ORR:
		case EOR:
		case BIC:
			if (operand == 0)
				return;
			break;

		case AND:
			if (operand == 0xffffffff)
				return;
			break;
		}
	}

	// minor transformations
	switch(opcode) {
	case ADD:
	case ADDS:
		if (count_bits(operand) > count_bits(-operand))
		{
			operand = -operand;
			opcode = SUB | (opcode & 0x00100000);
		}
		break;

	case AND:
	case ANDS:
		if (count_bits(operand) > count_bits(~operand))
		{
			operand = ~operand;
			opcode = BIC | (opcode & 0x00100000);
		}
		break;

	case MOV:
	case MOVS:
		if ((~operand & ~0xff) == 0)
		{
			operand = ~operand;
			opcode = MVN | (opcode & 0x00100000);
		}
		break;
	}

	ror = 0;
	new_operand = operand;
	while(new_operand & ~0xff)
	{
		if (ror >= 15)
		{
			// this immediate operand does not fit on one instruction
			rtemp = ((opcode == MOV) || (opcode == MVN)) ? rd : RSC3;

			shift = 0;
			while((operand & (3 << shift)) == 0)
				shift += 2;

			if (opcode == MVN)
			{
				subopcode1 = MVN;
				subopcode2 = BIC;
			}
			else
			{
				subopcode1 = MOV;
				subopcode2 = ORR;
			}
			emit_regi(params, subopcode1, rtemp, 0,		operand & ~(0xff << shift));
			emit_regi(params, subopcode2, rtemp, rtemp,	operand & (0xff << shift));

			if ((opcode != MOV) && (opcode != MVN))
				emit_regreg(params, opcode, rd, r1, RSC3);
			return;
		}

		ror++;
		tmp = new_operand & 0xc0000000;
		new_operand <<= 2;
		new_operand |= tmp >> 30;
	}

	opcode |= AL | (1 << 25) | (rd << 12) | (r1 << 16) | (ror << 8) | (new_operand << 0);
	OP(opcode);
}

void emit_header(struct blitter_params *params)
{
	struct drccore *drc = params->blitter;

	// stmfd	sp!,	{r4, r5, r6, r7, r8}
	OP(AL | 0x09200000 | (0x10000 * RSP) | 0x01f0);
	
	// mov		r4, dest_height
	emit_regi(params, MOV, RROWS, 0, params->dest_height);
	
	// mov		r5, #0
	emit_regi(params, MOV, RSCH, 0, 0);

	if (params->source_palette)
	{
		// mov	r2, palette
		emit_regi(params, MOV, RPAL, 0, (INT32) params->source_palette);
	}


}

void emit_footer(struct blitter_params *params)
{
	struct drccore *drc = params->blitter;

	// ldmfd	sp!, {r8, r7, r6, r5, r4}
	OP(AL | 0x08b00000 | (0x10000 * RSP) | 0x01f0);

	// mov		pc, r14
	emit_regreg(params, MOV, RPC, 0, RLINK);
}

void emit_increment_sourcebits(struct blitter_params *params, INT32 adjustment)
{
	emit_regi(params, ADD, RSRC, RSRC, adjustment);
}

void emit_increment_destbits(struct blitter_params *params, INT32 adjustment)
{
	emit_regi(params, ADD, RDEST, RDEST, adjustment);
}

void emit_copy_pixel(struct blitter_params *params, int pixel_mode, int divisor)
{
	struct drccore *drc = params->blitter;

	// ldrh	r5, [r1]
	_ldrh_r16_m16bd(AL, RSCH, RSRC, 0);

	if (params->source_palette)
	{
		// mov	r6, r5, LSL #2
		emit_regreg_shift(params, MOV, RSC2, 0, RSCH, LSL, 2);

		// ldrh	r5, [r6 + r2]
		_ldrh_r16_m16id(AL, RSCH, RSC2, RPAL);
	}

	switch(divisor) {
	case 1:
		break;

	case 2:
	case 4:
	case 8:
	case 16:
		// and	r5, r5, calc_blend_mask
		emit_regi(params, AND, RSCH, RSCH, calc_blend_mask(params, divisor));

		// mov	r5, r5, lsr #(log2 pixel_total)
		emit_regreg_shift(params, MOV, RSCH, 0, RSCH, LSR, intlog2(divisor));
		break;

	default:
		assert(0);
		break;
	}

	switch(pixel_mode) {
	case PIXELMODE_END:
		// add	r5, r5, r3
		emit_regreg(params, ADD, RSCH, RSCH, RBLEND);
		// fallthrough

	case PIXELMODE_SOLO:
		// strh	r5, [r1]
		OP(AL | 0x00400000 | (RDEST << 16) | (RSCH << 12) | 0x000000b0);
		break;

	case PIXELMODE_BEGIN:
		// mov	r3,	r5
		emit_regreg(params, MOV, RBLEND, 0, RSCH);
		break;

	case PIXELMODE_MIDDLE:
		// add	r3, r3, r5
		emit_regreg(params, ADD, RBLEND, RBLEND, RSCH);
		break;
	}
}

void emit_begin_loop(struct blitter_params *params)
{
	// subs	r4, r4, #1
	emit_regi(params, SUBS, RROWS, RROWS, 1);
}

void emit_finish_loop(struct blitter_params *params, void *loop_begin)
{
	struct drccore *drc = params->blitter;

	// bne	begin
	_bcc(COND_NE, loop_begin);
}

