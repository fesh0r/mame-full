#include <math.h>
#include "emitblit.h"

enum
{
	EAX,
	EBX,
	ECX,
	EDX
};

static void emit_increment_register(struct blitter_params *params, int reg, INT32 value)
{
	UINT8 inc_opcode;
	UINT8 dec_opcode;
	INT32 abs_value;

	if (value != 0)
	{
		if (reg == EAX)
		{
			emit_byte(params, 0x05);
			emit_int32(params, value);
		}
		else
		{
			switch(reg)
			{
				case ECX:	inc_opcode = 0xc1;	dec_opcode = 0xe9;	break;
				case EDX:	inc_opcode = 0xc2;	dec_opcode = 0xea;	break;
				default:	assert(0);								return;
			}
		
			abs_value = (value < 0) ? -value : value;

			emit_byte(params, (UINT8) (abs_value < 128 ? 0x83 : 0x81));
			emit_byte(params, (UINT8) (value > 0 ? inc_opcode : dec_opcode));

			if (abs_value < 128)
				emit_byte(params, (UINT8) abs_value);
			else
				emit_int32(params, abs_value);
		}
	}
}

void emit_header(struct blitter_params *params)
{
	// mov ecx, dword ptr [source_bits]
	emit_byte(params, 0x8b);
	emit_byte(params, 0x4c);
	emit_byte(params, 0x24);
	emit_byte(params, 0x08);

	// mov edx, dword ptr [dest_bits]
	emit_byte(params, 0x8b);
	emit_byte(params, 0x54);
	emit_byte(params, 0x24);
	emit_byte(params, 0x04);

	if (params->source_palette)
	{
		// mov ebx, palette
		emit_byte(params, 0xbb);
		emit_pointer(params, params->source_palette);
	}

	// push ebp
	emit_byte(params, 0x55);

	// push	shown_height
	emit_byte(params, 0x68);
	emit_int32(params, params->dest_height);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// push 0
		emit_byte(params, 0x68);
		emit_int32(params, 0);
	}
}

void emit_footer(struct blitter_params *params)
{
	// pop eax
	emit_byte(params, 0x58);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// pop eax
		emit_byte(params, 0x58);
	}

	// pop ebp
	emit_byte(params, 0x5d);

	// ret
	emit_byte(params, 0xc3);
}

void emit_increment_sourcebits(struct blitter_params *params, INT32 adjustment)
{
	emit_increment_register(params, ECX, adjustment);
}

void emit_increment_destbits(struct blitter_params *params, INT32 adjustment)
{
	emit_increment_register(params, EDX, adjustment);
}

void emit_copy_pixel(struct blitter_params *params, int pixel_mode, int divisor)
{
	INT32 mask;

	if (params->source_palette)
	{
		// xor	eax, eax
		emit_byte(params, 0x33);
		emit_byte(params, 0xc0);

		// mov	eax, word ptr [ecx]
		emit_byte(params, 0x66);
		emit_byte(params, 0x8b);
		emit_byte(params, 0x01);

		// mov	eax, dword ptr [ebx+eax*4]
		emit_byte(params, 0x8b);
		emit_byte(params, 0x04);
		emit_byte(params, 0x83);
	}
	else
	{
		// mov	eax, dword ptr [ecx]
		emit_byte(params, 0x8b);
		emit_byte(params, 0x01);
	}

	switch(divisor) {
	case 1:
		break;

	case 2:
	case 4:
	case 8:
	case 16:
		mask = ((1 << (params->rbits + params->gbits + params->bbits)) - 1);
		mask &= ~((divisor-1) << 0);
		mask &= ~((divisor-1) << (params->bbits));
		mask &= ~((divisor-1) << (params->bbits + params->gbits));

		// and	eax, mask
		emit_byte(params, 0x25);
		emit_int32(params, mask);

		// shr	eax, (log2 pixel_total)
		emit_byte(params, 0xc1);
		emit_byte(params, 0xe8);
		emit_byte(params, (UINT8) (log(divisor) / log(2)));
		break;


	default:
		assert(0);
		break;
	}

	switch(pixel_mode) {
	case PIXELMODE_END:
		//	add	eax, ebp
		emit_byte(params, 0x03);
		emit_byte(params, 0xc5);
		// fallthrough

	case PIXELMODE_SOLO:
		// mov	word ptr [edx], ax
		emit_byte(params, 0x66);
		emit_byte(params, 0x89);
		emit_byte(params, 0x02);
		break;

	case PIXELMODE_BEGIN:
		// mov	ebp, eax
		emit_byte(params, 0x8b);
		emit_byte(params, 0xe8);
		break;

	case PIXELMODE_MIDDLE:
		// add	ebp, eax
		emit_byte(params, 0x03);
		emit_byte(params, 0xe8);
		break;
	}
}

void emit_finish_loop(struct blitter_params *params, size_t loop_begin)
{
	if (params->flags & BLIT_MUST_BLEND)
	{
		// mov eax, [esp+4]
		emit_byte(params, 0x8b);
		emit_byte(params, 0x44);
		emit_byte(params, 0x24);
		emit_byte(params, 0x04);
	}
	else
	{
		// mov eax, [esp]
		emit_byte(params, 0x8b);
		emit_byte(params, 0x04);
		emit_byte(params, 0x24);
	}

	// dec eax
	emit_byte(params, 0x48);

	if (params->flags & BLIT_MUST_BLEND)
	{
		// mov [esp+4], eax
		emit_byte(params, 0x89);
		emit_byte(params, 0x44);
		emit_byte(params, 0x24);
		emit_byte(params, 0x04);
	}
	else
	{
		// mov [esp], eax
		emit_byte(params, 0x89);
		emit_byte(params, 0x04);
		emit_byte(params, 0x24);
	}

	// test eax, eax
	emit_byte(params, 0x85);
	emit_byte(params, 0xc0);

	// jne begin
	emit_byte(params, 0x0f);
	emit_byte(params, 0x85);
	emit_int32(params, loop_begin - (params->blitter_size + 4));
}

void emit_filler(void *dest, size_t sz)
{
	memset(dest, 0xcc, sz);
}

