#ifndef EMITBLIT_H
#define EMITBLIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "mame.h"
	
enum
{
	BLIT_MUST_BLEND = 1
};

enum
{
	PIXELMODE_SOLO,
	PIXELMODE_BEGIN,
	PIXELMODE_MIDDLE,
	PIXELMODE_END
};

struct blitter_params
{
	UINT8 *blitter;
	size_t blitter_size;
	size_t blitter_max_size;
	int flags;

	const UINT32 *source_palette;

	INT32 dest_width;
	INT32 dest_height;

	int rbits;
	int gbits;
	int bbits;
};

int intlog2(int val);
INT32 calc_blend_mask(struct blitter_params *params, int divisor);

void emit_byte(struct blitter_params *params, UINT8 b);
void emit_int16(struct blitter_params *params, INT16 i);
void emit_int32(struct blitter_params *params, INT32 i);
#define emit_pointer(params, ptr)	emit_int32((params), (INT32) (ptr))

void emit_header(struct blitter_params *params);
void emit_footer(struct blitter_params *params);
void emit_increment_sourcebits(struct blitter_params *params, INT32 adjustment);
void emit_increment_destbits(struct blitter_params *params, INT32 adjustment);
void emit_copy_pixel(struct blitter_params *params, int pixel_mode, int divisor);
void emit_begin_loop(struct blitter_params *params);
void emit_finish_loop(struct blitter_params *params, size_t loop_begin);
void emit_filler(void *dest, size_t sz);

#ifdef __cplusplus
};
#endif

#endif // EMITBLIT_H
