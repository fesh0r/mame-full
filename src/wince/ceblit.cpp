//============================================================
//
//	ceblit.cpp - WinCE blit handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <assert.h>

// MAME headers
extern "C"
{
	#include "driver.h"
	#include "blit.h"
	#include "video.h"
	#include "window.h"
	#include "emitblit.h"
	#include "invokegx.h"
	#include "profiler.h"
}

//============================================================
//	TYPE DEFINITIONS
//============================================================

typedef void (__cdecl *blitter_func)(void *dest_bits, const void *source_bits);

//============================================================
//	PARAMETERS
//============================================================

#define MAX_BLITTER_SIZE	32768
#define DEBUG_BLITTERS		1

//#define PROFILER_GAPI		PROFILER_USER4

//============================================================
//	LOCAL VARIABLES
//============================================================

static UINT8 current_blitter[MAX_BLITTER_SIZE];
static int current_source_pitch, current_source_width, current_source_height;
static const UINT32 *current_palette;
static GXDisplayProperties display_properties;



//============================================================
//	swap
//============================================================

template<class T>
void swap(T *t1, T *t2)
{
	T temp = *t1;
	*t1 = *t2;
	*t2 = temp;
}

//============================================================
//	intlog2
//============================================================

int intlog2(int val)
{
	return (int) (log(val) / log(2));
}

//============================================================
//	calc_blend_mask
//============================================================

INT32 calc_blend_mask(struct blitter_params *params, int divisor)
{
	INT32 mask;
	mask = ((1 << (params->rbits + params->gbits + params->bbits)) - 1);
	mask &= ~((divisor-1) << 0);
	mask &= ~((divisor-1) << (params->bbits));
	mask &= ~((divisor-1) << (params->bbits + params->gbits));
	return mask;
}

//============================================================
//	emit functions
//
//  we can assume that since this is emitting CPU instructions
//	that alignment faults are not an issue
//============================================================

static int preemit(struct blitter_params *params, int len)
{
	if (!params->blitter)
		return 1;
	if (params->blitter_size + len > params->blitter_max_size)
	{
		params->blitter = NULL;
		return 1;
	}
	return 0;
}

void emit_byte(struct blitter_params *params, UINT8 b)
{
	if (preemit(params, 1))
		return;
	params->blitter[params->blitter_size++] = b;
}

void emit_int16(struct blitter_params *params, INT16 i)
{
	if (preemit(params, 2))
		return;
	*((UINT16 *) &params->blitter[params->blitter_size]) = i;
	params->blitter_size += 2;
}

void emit_int32(struct blitter_params *params, INT32 i)
{
	if (preemit(params, 4))
		return;
	*((UINT32 *) &params->blitter[params->blitter_size]) = i;
	params->blitter_size += 4;
}


//============================================================
//	generate_blitter 
//============================================================

static int generate_blitter(int orientation)
{
	struct blitter_params params;
	UINT32 dest_width = display_properties.cxWidth;
	UINT32 dest_height = display_properties.cyHeight;
	INT32 dest_xpitch = display_properties.cbxPitch;
	INT32 dest_ypitch = display_properties.cbyPitch;
	INT32 dest_base_adjustment = 0;
	INT32 source_base_adjustment = 0;
	INT32 shown_width, shown_height;
	INT32 i, j;
	INT32 source_linepos, source_lineposnext, pixels_to_blit;
	size_t loop_begin;
	int flags;
	int pixel_mode;

	// modify dest based on orientation
	if (orientation & ORIENTATION_FLIP_X)
	{
		dest_base_adjustment += dest_width * dest_xpitch;
		dest_xpitch = -dest_xpitch;
	}
	if (orientation & ORIENTATION_FLIP_Y)
	{
		dest_base_adjustment += dest_height * dest_ypitch;
		dest_xpitch = -dest_xpitch;
	}
	if (orientation & ORIENTATION_SWAP_XY)
	{
		swap(&dest_xpitch, &dest_ypitch);
		swap(&dest_width, &dest_height);
	}

	// center within display
	if (dest_width >= current_source_width)
	{
		shown_width = current_source_width;
		dest_base_adjustment += ((dest_width - current_source_width) / 2) * dest_xpitch;
	}
	else
	{
		shown_width = dest_width;
	}
	if (dest_height >= current_source_height)
	{
		shown_height = current_source_height;
		dest_base_adjustment += ((dest_height - current_source_height) / 2) * dest_ypitch;
	}
	else
	{
		shown_height = dest_height;
		source_base_adjustment += ((current_source_height - dest_height) / 2) * current_source_pitch;
	}

	flags = 0;
	if (shown_width != current_source_width)
		flags |= BLIT_MUST_BLEND;

	memset(&params, 0, sizeof(params));
	params.blitter = current_blitter;
	params.blitter_max_size = sizeof(current_blitter) / sizeof(current_blitter[0]);
	params.flags = flags;
	params.source_palette = current_palette;
	params.dest_width = shown_width;
	params.dest_height = shown_height;

	// set up blend mask
	if (display_properties.ffFormat & kfDirect444)
	{
		params.rbits = 4;
		params.gbits = 4;
		params.bbits = 4;
	}
	else if (display_properties.ffFormat & kfDirect555)
	{
		params.rbits = 5;
		params.gbits = 5;
		params.bbits = 5;
	}
	else if (display_properties.ffFormat & kfDirect565)
	{
		params.rbits = 5;
		params.gbits = 6;
		params.bbits = 5;
	}
	else if (display_properties.ffFormat & kfDirect888)
	{
		params.rbits = 8;
		params.gbits = 8;
		params.bbits = 8;
	}

	emit_header(&params);
	emit_increment_sourcebits(&params, source_base_adjustment);
	emit_increment_destbits(&params, dest_base_adjustment);

	loop_begin = params.blitter_size;
	emit_begin_loop(&params);

	source_linepos = 0;
	for(i = 0; i < shown_width; i++)
	{
		source_lineposnext = (INT32) (((float) (i+1)) * current_source_width / shown_width);
		pixels_to_blit = source_lineposnext - source_linepos;
		source_linepos = source_lineposnext;
		assert(pixels_to_blit > 0);

		for(j = 0; j < pixels_to_blit; j++)
		{
			if (j == 0)
				pixel_mode = (pixels_to_blit == 1) ? PIXELMODE_SOLO : PIXELMODE_BEGIN;
			else 
				pixel_mode = (j == pixels_to_blit-1) ? PIXELMODE_END : PIXELMODE_MIDDLE;

			emit_copy_pixel(&params, pixel_mode, pixels_to_blit);
			if ((j+1 < pixels_to_blit) || (i+1 < shown_width))
				emit_increment_sourcebits(&params, 2);
		}
		if (i+1 < shown_width)
			emit_increment_destbits(&params, dest_xpitch);
	}

	emit_increment_sourcebits(&params, current_source_pitch - (current_source_width-1)*2);
	emit_increment_destbits(&params, dest_ypitch - (shown_width-1)*dest_xpitch);

	emit_finish_loop(&params, loop_begin);
	emit_footer(&params);

	if (!params.blitter)
		return 1;

#ifdef MAME_DEBUG
	emit_filler(&params.blitter[params.blitter_size], params.blitter_max_size - params.blitter_size);
#endif

#if DEBUG_BLITTERS
	// generate files with the results; use ndisasmw to disassemble them
	{
		FILE *out;

		out = fopen("ceblit.com", "wb");
		fwrite(params.blitter, 1, params.blitter_size, out);
		fclose(out);
	}
#endif
	return 0;
}


//============================================================
//	ce_blit
//============================================================


extern "C" void ce_blit(struct mame_bitmap *bitmap, int orientation, const UINT32 *palette)
{
	const void *source_bits;
	void *dest_bits;
	int source_pitch, source_width, source_height;

	if (display_properties.cxWidth == 0)
		gx_get_display_properties(&display_properties);

	source_bits = bitmap->base;
	source_pitch = bitmap->rowbytes;
	source_width = bitmap->width;
	source_height = bitmap->height;

	if ((source_pitch != current_source_pitch) || (source_width != current_source_width)
		|| (source_height != current_source_height) || (palette != current_palette))
	{
		current_source_pitch = source_pitch;
		current_source_width = source_width;
		current_source_height = source_height;
		current_palette = palette;
		if (generate_blitter(orientation))
		{
			current_source_width = 0;
			return;
		}
	}

#ifdef PROFILER_GAPI
	profiler_mark(PROFILER_GAPI);
#endif

	dest_bits = gx_begin_draw();

#ifdef PROFILER_GAPI
	profiler_mark(PROFILER_END);
#endif

	if (dest_bits)
	{
		((blitter_func) (void *) current_blitter)(dest_bits, source_bits);

#ifdef PROFILER_GAPI
		profiler_mark(PROFILER_GAPI);
#endif
		gx_end_draw();

#ifdef PROFILER_GAPI
		profiler_mark(PROFILER_END);
#endif
	}
}
