#ifndef EMITBLIT_H
#define EMITBLIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "mame.h"

#if defined(X86)
#include "x86drc.h"
#elif defined(ARM)
#include "armdrc.h"
#else
#error No dynamic recompilation for this architecture
#endif

enum
{
	BLIT_MUST_BLEND = 1
};

struct blitter_params
{
	struct drccore *blitter;
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

void emit_header(struct blitter_params *params);
void emit_footer(struct blitter_params *params);
void emit_increment_sourcebits(struct blitter_params *params, INT32 adjustment);
void emit_increment_destbits(struct blitter_params *params, INT32 adjustment);
int emit_copy_pixels(struct blitter_params *params, const int *pixel_spans, int max_spans, INT32 dest_xpitch);
void emit_begin_loop(struct blitter_params *params);
void emit_finish_loop(struct blitter_params *params, void *loop_begin);

#ifdef __cplusplus
};
#endif

#endif // EMITBLIT_H
