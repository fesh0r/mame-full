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
//		R5		scratch
//		R6		scratch
//		R7-R12	multiscratch
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
	RSC2	= 5,
	RSC3	= 6,

	RMSCB	= 7,
	RMSCE	= 12,

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

	// stmfd	sp!,	{r4-r12}
	_stmfd_wb(COND_AL, RSP, 0x1ff0);
	
	// mov		r4, dest_height
	emit_regi(params, MOV, RROWS, 0, params->dest_height);

	if (params->source_palette)
	{
		// mov	r2, palette
		emit_regi(params, MOV, RPAL, 0, (INT32) params->source_palette);
	}
}

void emit_footer(struct blitter_params *params)
{
	struct drccore *drc = params->blitter;

	// ldmfd	sp!, {r12-r4}
	_ldmfd_wb(COND_AL, RSP, 0x1ff0);

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

int emit_copy_pixels(struct blitter_params *params, const int *pixel_spans, int max_spans, int dest_xpitch)
{
	struct calculated_blend_mask
	{
		int pixel_span;
		int reg;
	};

	struct drccore *drc = params->blitter;
	struct calculated_blend_mask blend_masks[2];
	int blend_mask_count = 0;
	int source_index = 0;
	int next_reg = RMSCB;
	int i, j, k;

	for (i = 0; i < max_spans; i++)
	{
		switch(pixel_spans[i]) {
		case 2:
		case 4:
		case 8:
		case 16:
			for (j = 0; j < blend_mask_count; j++)
			{
				if (blend_masks[j].pixel_span == pixel_spans[i])
					break;
			}
			if (j == blend_mask_count)
			{
				// mov	r#, calc_blend_mask
				emit_regi(params, MOV, next_reg, next_reg, calc_blend_mask(params, pixel_spans[i]));
				blend_masks[blend_mask_count].pixel_span = pixel_spans[i];
				blend_masks[blend_mask_count].reg = next_reg++;
				blend_mask_count++;
			}
			break;
		};
		if (blend_mask_count == (sizeof(blend_masks) / sizeof(blend_masks[0])))
			break;
	}

	i = 0;
	while(
		(i < max_spans) &&
		isvalid_disp(source_index + pixel_spans[i]*2) &&
		isvalid_disp(i * dest_xpitch))
	{
		for (j = 0; j < pixel_spans[i]; j++)
		{
			_ldrh_r16_m16bd(AL, next_reg+0, RSRC, source_index);

			if (params->source_palette)
			{
				emit_regreg_shift(params, MOV, next_reg+1, 0, next_reg+0, LSL, 2);
				_ldrh_r16_m16id(AL, next_reg+0, next_reg+1, RPAL);
			}

			switch(pixel_spans[i]) {
			case 1:
				break;

			case 2:
			case 4:
			case 8:
			case 16:
				for (k = 0; k < blend_mask_count; k++)
				{
					if (blend_masks[k].pixel_span == pixel_spans[i])
					{
						emit_regreg(params, AND, next_reg+0, next_reg+0, blend_masks[k].reg);
						break;
					}
				}
				if (k == blend_mask_count)
					emit_regi(params, AND, next_reg+0, next_reg+0, calc_blend_mask(params, pixel_spans[i]));

				emit_regreg_shift(params, MOV, next_reg+0, 0, next_reg+0, LSR, intlog2(pixel_spans[i]));
				break;

			default:
				assert(0);
				break;
			}

			if (j+1 == pixel_spans[i])
			{
				if (pixel_spans[i] > 1)
					emit_regreg(params, ADD, next_reg+0, next_reg+0, RBLEND);
			}
			else if (j == 0)
			{
				emit_regreg(params, MOV, RBLEND, 0, next_reg+0);
			}
			else
			{
				emit_regreg(params, ADD, RBLEND, RBLEND, next_reg+0);
			}
			source_index += 2;
		}

		_strh_m16bd_r16(AL, RDEST, i * dest_xpitch, next_reg+0);
		i++;
	}
	return i;
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

