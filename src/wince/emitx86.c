//============================================================
//
//	emitx86.cpp - WinCE Intel x86 specific code generation
//
//============================================================
//
//	registers used by blitter:
//		EAX		scratch
//		EBX		palette index
//		ECX		source bits
//		EDX		dest bits
//		EBP		blend register
//============================================================

#include "emitblit.h"

//============================================================

#define _mov_m32bd_r32(base, disp, dreg) \
	do { OP1(0x89); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_m16bd_r16(base, disp, dreg) \
	do { OP1(0x66); OP1(0x89); MODRM_MBD(dreg, base, disp); } while (0)

//============================================================

void emit_header(struct blitter_params *params)
{
	struct drccore *drc = params->blitter;

	// mov ecx, dword ptr [source_bits]
	_mov_r32_m32bd(REG_ECX, REG_ESP, 0x08);

	// mov edx, dword ptr [dest_bits]
	_mov_r32_m32bd(REG_EDX, REG_ESP, 0x04);

	if (params->source_palette)
	{
		// mov ebx, palette
		_mov_r32_imm(REG_EBX, params->source_palette);
	}

	// push ebp
	_push_r32(REG_EBP);

	// push	shown_height
	_push_imm(params->dest_height);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// push 0
		_push_imm(0);
	}
}

void emit_footer(struct blitter_params *params)
{
	struct drccore *drc = params->blitter;

	// pop eax
	_pop_r32(REG_EAX);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// pop eax
		_pop_r32(REG_EAX);
	}

	// pop ebp
	_pop_r32(REG_EBP);

	// ret
	_ret();
}

void emit_increment_sourcebits(struct blitter_params *params, INT32 adjustment)
{
	struct drccore *drc = params->blitter;
	_add_r32_imm(REG_ECX, adjustment);
}

void emit_increment_destbits(struct blitter_params *params, INT32 adjustment)
{
	struct drccore *drc = params->blitter;
	_add_r32_imm(REG_EDX, adjustment);
}

int emit_copy_pixels(struct blitter_params *params, const int *pixel_spans, int max_spans, int dest_xpitch)
{
	struct drccore *drc = params->blitter;
	int i, j;
	int source_index = 0;

	for (i = 0; i < max_spans; i++)
	{
		for (j = 0; j < pixel_spans[i]; j++)
		{
			if (params->source_palette)
			{
				// mov	eax, 0
				_mov_r32_imm(REG_EAX, 0);

				// mov	eax, word ptr [ecx+source_index*2]
				_mov_r16_m16bd(REG_EAX, REG_ECX, source_index*2);

				// mov	eax, dword ptr [ebx+eax*4]
				_mov_r32_m32bisd(REG_EAX, REG_EBX, REG_EAX, 4, 0);
			}
			else
			{
				// mov	eax, dword ptr [ecx+source_index*2]
				_mov_r32_m32bd(REG_EAX, REG_ECX, source_index*2);
			}

			switch(pixel_spans[i]) {
			case 1:
				break;

			case 2:
			case 4:
			case 8:
			case 16:
				// and	eax, mask
				_and_r32_imm(REG_EAX, calc_blend_mask(params, pixel_spans[i]));

				// shr	eax, (log2 pixel_total)
				_shr_r32_imm(REG_EAX, intlog2(pixel_spans[i]));
				break;

			default:
				assert(0);
				break;
			}

			if (j+1 == pixel_spans[i])
			{
				if (pixel_spans[i] > 1)
				{
					//	add	eax, ebp
					_add_r32_r32(REG_EAX, REG_EBP);
				}
				// mov	word ptr [edx+(dest_xpitch*i)], ax
				_mov_m16bd_r16(REG_EDX, dest_xpitch*i, REG_AX);
			}
			else if (j == 0)
			{
				// mov	ebp, eax
				_mov_r32_r32(REG_EBP, REG_EAX);
			}
			else
			{
				// add	ebp, eax
				_add_r32_r32(REG_EBP, REG_EAX);
			}
			source_index++;
		}
	}
	return max_spans;
}

void emit_begin_loop(struct blitter_params *params)
{
}

void emit_finish_loop(struct blitter_params *params, void *loop_begin)
{
	struct drccore *drc = params->blitter;
	if (params->flags & BLIT_MUST_BLEND)
	{
		// mov eax, [esp+4]
		_mov_r32_m32bd(REG_EAX, REG_ESP, 4);
	}
	else
	{
		// mov eax, [esp]
		_mov_r32_m32bd(REG_EAX, REG_ESP, 0);
	}

	// dec eax
	_sub_r32_imm(REG_EAX, 1);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// mov [esp+4], eax
		_mov_m32bd_r32(REG_ESP, 4, REG_EAX);
	}
	else
	{
		// mov [esp], eax
		_mov_m32bd_r32(REG_ESP, 0, REG_EAX);
	}

	// test eax, eax
	_cmp_r32_imm(REG_EAX, 0);

	// jne begin
	_jcc(COND_NE, loop_begin);
}

