#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "videomap.h"
#include "mess.h"
#include "vidhrdw/generic.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define PROFILER_VIDEOMAP_DRAWBORDER	PROFILER_USER3
#define PROFILER_VIDEOMAP_DRAWBODY		PROFILER_USER4

/* ----------------------------------------------------------------------- *
 * globals                                                                 *
 * ----------------------------------------------------------------------- */

/* configuation */
static const struct videomap_interface *callbacks;

/* info about the current video mode */
static struct videomap_framecallback_info frame_info;
static struct videomap_linecallback_info line_info;

/* used at draw time */
static UINT16 *scanline_data;
static const UINT8 *videoram_pos;
static int videoram_windowsize;
static UINT8 *videoram_dirtybuffer;
static UINT8 *videoram_dirtybuffer_pos;

/* border information */
static UINT16 *border_scanline;
static int border_position;
static UINT16 border_color;

enum
{
	FLAG_INVAL_FRAMEINFO	= 1,
	FLAG_INVAL_LINEINFO		= 2,
	FLAG_BORDER_MODIFIED	= 4,
	FLAG_ENDIAN_FLIP		= 8,
	FLAG_FIRST_DRAW			= 16
};
static UINT8 flags;

/* ----------------------------------------------------------------------- *
 * the uber-optimized blitters                                             *
 * ----------------------------------------------------------------------- */
struct drawline_params
{
	UINT16 *scanline_data;
	int bytes_per_row;
	int offset;
	int offset_wrap;
	int zoomx;
	const UINT16 *pens;
	charproc_callback charproc;
	int row;
	UINT8 border_value;
	void (*inner_draw_line)(struct drawline_params *_params);
};

#ifdef ALIGN_INTS
#define EMITDOUBLEDOT(offset)	s[(offset)] = p; \
								s[(offset)+1] = p;
#else
#define EMITDOUBLEDOT(offset)	*((UINT32 *) &s[(offset)]) = pl;
#endif

#define EMITPIXEL(decl_zoomx, offset, val)							\
	p = (val);														\
	pl = p;															\
	pl = (pl << 16) | pl;											\
	switch(decl_zoomx) {											\
	case 8:	EMITDOUBLEDOT(((offset)+1) * decl_zoomx - 8);			\
	case 6:	EMITDOUBLEDOT(((offset)+1) * decl_zoomx - 6);			\
	case 4:	EMITDOUBLEDOT(((offset)+1) * decl_zoomx - 4);			\
			EMITDOUBLEDOT(((offset)+1) * decl_zoomx - 2);			\
		break;														\
	case 3:	s[((offset)+1) * decl_zoomx - 3] = p;					\
	case 2:	s[((offset)+1) * decl_zoomx - 2] = p;					\
	case 1:	s[((offset)+1) * decl_zoomx - 1] = p;					\
		break;														\
	default:														\
		{															\
			int i, target;											\
			i = (offset) * zoomx;									\
			target = ((offset) + 1) * zoomx;						\
			do														\
			{														\
				s[i++] = p;											\
			} while(i < target);									\
		}															\
		break;														\
	}

#define EMITBYTE(decl_zoomx, offset, has_charproc, bpp, shift, has_artifact)									\
	{																											\
		UINT32 c;																								\
		const UINT16 *my_pens;																					\
		UINT16 charpalette[2];																					\
																												\
		if (has_artifact)																						\
		{																										\
			const UINT16 *thispen;																				\
			assert(!has_charproc);																				\
			assert((bpp) == 1);																					\
			c = l;																								\
			thispen = &pens[((c >> ((shift)+6)) & 0x3f) * 2];													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 0, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 1, thispen[1]);													\
			thispen = &pens[((c >> ((shift)+4)) & 0x3f) * 2];													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 2, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 3, thispen[1]);													\
			thispen = &pens[((c >> ((shift)+2)) & 0x3f) * 2];													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 4, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 5, thispen[1]);													\
			thispen = &pens[((c >> ((shift)+0)) & 0x3f) * 2];													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 6, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 7, thispen[1]);													\
		}																										\
		else																									\
		{																										\
			if (has_charproc)																					\
			{																									\
				c = params->charproc(l >> (shift), charpalette, params->row);									\
				my_pens = charpalette;																			\
			}																									\
			else																								\
			{																									\
				c = l;																							\
				my_pens = pens;																					\
			}																									\
			switch(has_charproc ? 1 : bpp)	{																	\
			case 1:																								\
				EMITPIXEL(decl_zoomx, (offset)*8 + 0, my_pens[(c >> ((has_charproc ? 0 : (shift))+7)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 1, my_pens[(c >> ((has_charproc ? 0 : (shift))+6)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 2, my_pens[(c >> ((has_charproc ? 0 : (shift))+5)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 3, my_pens[(c >> ((has_charproc ? 0 : (shift))+4)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 4, my_pens[(c >> ((has_charproc ? 0 : (shift))+3)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 5, my_pens[(c >> ((has_charproc ? 0 : (shift))+2)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 6, my_pens[(c >> ((has_charproc ? 0 : (shift))+1)) & 0x01]);	\
				EMITPIXEL(decl_zoomx, (offset)*8 + 7, my_pens[(c >> ((has_charproc ? 0 : (shift))+0)) & 0x01]);	\
				break;																							\
			case 2:																								\
				EMITPIXEL(decl_zoomx, (offset)*4 + 0, my_pens[(c >> ((has_charproc ? 0 : (shift))+6)) & 0x03]);	\
				EMITPIXEL(decl_zoomx, (offset)*4 + 1, my_pens[(c >> ((has_charproc ? 0 : (shift))+4)) & 0x03]);	\
				EMITPIXEL(decl_zoomx, (offset)*4 + 2, my_pens[(c >> ((has_charproc ? 0 : (shift))+2)) & 0x03]);	\
				EMITPIXEL(decl_zoomx, (offset)*4 + 3, my_pens[(c >> ((has_charproc ? 0 : (shift))+0)) & 0x03]);	\
				break;																							\
			case 4:																								\
				EMITPIXEL(decl_zoomx, (offset)*2 + 0, my_pens[(c >> ((has_charproc ? 0 : (shift))+4)) & 0x0f]);	\
				EMITPIXEL(decl_zoomx, (offset)*2 + 1, my_pens[(c >> ((has_charproc ? 0 : (shift))+0)) & 0x0f]);	\
				break;																							\
			case 8:																								\
				EMITPIXEL(decl_zoomx, (offset)*1 + 0, my_pens[(c >> ((has_charproc ? 0 : (shift))+0)) & 0xff]);	\
				break;																							\
			default:																							\
				assert(FALSE);																					\
				break;																							\
			}																									\
		}																										\
	}

#ifdef LSB_FIRST
#define SHIFT0 0
#define SHIFT1 8
#define SHIFT2 16
#define SHIFT3 24
#else
#define SHIFT3 0
#define SHIFT2 8
#define SHIFT1 16
#define SHIFT0 24
#endif

#ifdef ALIGN_INTS
#define MUST_ALIGN32(addr)	(((long) (addr)) & 3)
#define MUST_ALIGN16(addr)	(((long) (addr)) & 1)
#else
#define MUST_ALIGN32(addr)	(0)
#define MUST_ALIGN16(addr)	(0)
#endif

INLINE UINT32 get_uint32(const UINT32 *v)
{
	UINT32 l;
	if (!MUST_ALIGN32(v))
	{
		l = *v;
	}
	else
	{
		l =		(((UINT32) (((UINT8 *) v)[0])) << SHIFT0)
			|	(((UINT32) (((UINT8 *) v)[1])) << SHIFT1)
			|	(((UINT32) (((UINT8 *) v)[2])) << SHIFT2)
			|	(((UINT32) (((UINT8 *) v)[3])) << SHIFT3);
	}
	return l;
}

INLINE UINT32 get_uint32_flip(const UINT32 *v)
{
	UINT32 l;
	l =		(((UINT32) (((UINT8 *) v)[0])) << SHIFT1)
		|	(((UINT32) (((UINT8 *) v)[1])) << SHIFT0)
		|	(((UINT32) (((UINT8 *) v)[2])) << SHIFT3)
		|	(((UINT32) (((UINT8 *) v)[3])) << SHIFT2);
	return l;
}

/* step 1 - declare the functions */
#include "vmapcore.c"

/* step 2 - make the table */
typedef void (*drawline_proc)(struct drawline_params *params);
static drawline_proc drawline_table[] =
{
#define DRAWLINE_TABLE
#include "vmapcore.c"
#undef DRAWLINE_TABLE
	NULL
};

/* this drawline function is used to accomodate cases where an offset results in a split line */
static void draw_line_with_offset(struct drawline_params *params)
{
	UINT16 buf[1024];
	UINT16 *saved_scanline_data;
	UINT16 *scanline;
	int saved_offset;
	int saved_bytes_per_row;
	int pixels_per_byte;
	int segment_bytes;
	int bytes_left;
	int tiny_bit;

	assert(params->bytes_per_row > 0);

	/* save key variables */
	saved_scanline_data = params->scanline_data;
	saved_offset = params->offset;
	saved_bytes_per_row = params->bytes_per_row;

	/* initial values */
	pixels_per_byte = (8 / line_info.grid_depth) * params->zoomx;
	bytes_left = params->bytes_per_row;
	scanline = params->scanline_data;

	do
	{
		tiny_bit = 0;
		segment_bytes = MIN(bytes_left, params->offset_wrap - params->offset);
		assert(segment_bytes > 0);

		if ((segment_bytes & 3) == 0)
		{
			/* ahhh good... we are nice and int aligned */
			params->bytes_per_row = segment_bytes;
			params->scanline_data = scanline;
		}
		else if (((segment_bytes+3) & ~3) <= bytes_left)
		{
			/* we can afford an overrun */
			params->bytes_per_row = (segment_bytes+3) & ~3;
			params->scanline_data = scanline;
		}
		else if (segment_bytes < 4)
		{
			/* just a tiny bit here, use a temporary buffer */
			assert((pixels_per_byte * 4) < sizeof(buf) / sizeof(buf[0]));
			params->bytes_per_row = 4;
			params->scanline_data = buf;
			tiny_bit = 1;
		}
		else
		{
			/* place all but a tiny bit in the real buffer */
			segment_bytes &= ~3;
			params->bytes_per_row = segment_bytes;
			params->scanline_data = scanline;
		}


		/* make the call */
		params->inner_draw_line(params);

		/* if we copied into buf, copy out the result */
		if (tiny_bit)
			memcpy(scanline, buf, pixels_per_byte * segment_bytes * sizeof(buf[0]));

		/* move forward */
		bytes_left -= segment_bytes;
		params->offset += segment_bytes;
		params->offset %= params->offset_wrap;
		scanline += segment_bytes * pixels_per_byte;
	}
	while(bytes_left > 0);

	/* restore key variables */
	params->scanline_data = saved_scanline_data;
	params->offset = saved_offset;
	params->bytes_per_row = saved_bytes_per_row;
}

static drawline_proc get_drawline_proc(int bits_per_pixel, int zoomx, int has_charproc, int has_artifact, int flip)
{
	drawline_proc draw_line;
	int procnum = 0;

	if (has_charproc)
	{
		assert(!has_artifact);
		switch(bits_per_pixel) {
		case 8:		procnum += 48;	break;
		case 16:	procnum += 60;	break;
		default:	assert(0);		break;
		}
	}
	else if (has_artifact)
	{
		switch(bits_per_pixel) {
		case 1:		procnum += 72;	break;
		default:	assert(0);		break;
		}
	}
	else
	{
		switch(bits_per_pixel) {
		case 1:		procnum += 0;	break;
		case 2:		procnum += 12;	break;
		case 4:		procnum += 24;	break;
		case 8:		procnum += 36;	break;
		default:	assert(0);		break;
		}
	}

	switch(zoomx) {
	case 1:		procnum += 2;	break;
	case 2:		procnum += 4;	break;
	case 3:		procnum += 6;	break;
	case 4:		procnum += 8;	break;
	case 8:		procnum += 10;	break;
	default:	procnum += 0;	break;
	}
	if (flip)
		procnum += 1;

	assert(procnum < (sizeof(drawline_table) / sizeof(drawline_table[0])));
	draw_line = drawline_table[procnum];
	assert(draw_line);
	return draw_line;
}

#ifndef VIDEOMAP_TEST

/* ----------------------------------------------------------------------- *
 * accessors for callbacks and other helpers                               *
 * ----------------------------------------------------------------------- */

static void get_border_color(void)
{
	border_color = (callbacks->get_border_color_callback) ? callbacks->get_border_color_callback() : 0;
}

static void get_frame_info(void)
{
	memset(&frame_info, 0, sizeof(frame_info));
	callbacks->frame_callback(&frame_info);
}

static void get_line_info(void)
{
	memset(&line_info, 0, sizeof(line_info));

	callbacks->line_callback(&line_info);

	assert(line_info.grid_width);
	assert(line_info.grid_depth);
	assert(line_info.scanlines_per_row);
}

enum
{
	POSITION_DISPLAY,
	POSITION_BORDER_TOP,
	POSITION_BORDER_BOTTOM,
	POSITION_BORDER_LEFT,
	POSITION_BORDER_RIGHT
};

static int in_border(void)
{
	int scanline;
	int horzbeampos;

	scanline = cpu_getscanline();
	if (scanline < frame_info.bordertop_scanlines)
		return POSITION_BORDER_TOP;
	if (scanline >= (frame_info.bordertop_scanlines + frame_info.visible_scanlines))
		return POSITION_BORDER_BOTTOM;

	horzbeampos = cpu_gethorzbeampos();
	if (horzbeampos < line_info.borderleft_columns)
		return POSITION_BORDER_LEFT;
	if (horzbeampos >= (line_info.borderleft_columns + line_info.visible_columns))
		return POSITION_BORDER_RIGHT;
	return POSITION_DISPLAY;
}

/* ----------------------------------------------------------------------- *
 * video mode invalidation                                                 *
 * ----------------------------------------------------------------------- */

static void calc_videoram_pos(void)
{
	videoram_pos = videoram + frame_info.video_base;
	videoram_dirtybuffer_pos = videoram_dirtybuffer;
}

static void new_frame(int scanline)
{
	force_partial_update(scanline - 1);
	get_frame_info();
	flags &= ~FLAG_INVAL_FRAMEINFO;
	calc_videoram_pos();
}

static void new_line(int scanline)
{
	int i;
	if (flags & FLAG_BORDER_MODIFIED)
	{
		for (i = border_position; i < Machine->drv->screen_width; i++)
			border_scanline[i] = border_color;
		border_position = 0;
	}

	/* only force an update if there are scanlines before this new scanline */
	if (scanline > 0)
		force_partial_update(scanline - 1);

	if (flags & FLAG_INVAL_LINEINFO)
		get_line_info();

	flags &= (~FLAG_INVAL_LINEINFO & ~FLAG_BORDER_MODIFIED);
}

static void general_invalidate(UINT8 inval_flags_mask, void (*callback)(int), int scanline)
{
	double delay;

	assert(scanline >= 0);
	if (Machine->scrbitmap)
	{
		assert(scanline <= Machine->scrbitmap->height);
	}

	if (scanline <= cpu_getscanline())
	{
		flags |= inval_flags_mask;
		callback(scanline);
	}
	else if (!(flags & inval_flags_mask))
	{
		delay = cpu_getscanlinetime(scanline);
		timer_set(delay, scanline, callback);
		flags |= inval_flags_mask;
	}
	schedule_full_refresh();
}

void videomap_invalidate_frameinfo()
{
	general_invalidate(FLAG_INVAL_FRAMEINFO, new_frame, Machine->drv->screen_height);
}

void videomap_invalidate_lineinfo()
{
	int scanline, horzbeampos, adjustment;

	scanline = cpu_getscanline();
	horzbeampos = cpu_gethorzbeampos();

	if (scanline > Machine->scrbitmap->height)
		scanline = Machine->scrbitmap->height;

	assert(scanline >= 0);
	assert(scanline <= Machine->scrbitmap->height);

	/* am I in the left side? adjustment is 0 if so; 1 otherwise*/
	adjustment = (horzbeampos < (Machine->drv->screen_width / 2)) ? 0 : 1;
	general_invalidate(FLAG_INVAL_LINEINFO, new_line, scanline + adjustment);
}

void videomap_invalidate_border()
{
	int scanline;
	int new_border_position;

	if ((flags & FLAG_BORDER_MODIFIED) == 0)
	{
		scanline = cpu_getscanline();

		/* update everything up to this line; and only force an update if
		 * there are scanlines before this new scanline
		 */
		if (scanline > 0)
			force_partial_update(scanline - 1);

		/* the next scanline should be treated as new */
		general_invalidate(FLAG_BORDER_MODIFIED, new_line, scanline + 1);
	}

	new_border_position = cpu_gethorzbeampos();
	while(border_position < new_border_position)
		border_scanline[border_position++] = border_color;

	get_border_color();
}

/* ----------------------------------------------------------------------- *
 * the update function that actually renders everything based on the info  *
 * ----------------------------------------------------------------------- */

static int is_row_dirty(UINT8 *dirtyrow, int rowlength)
{
	UINT32 val;
	UINT32 *db;

	assert(dirtyrow);
	assert(rowlength > 0);
	assert((rowlength % 4) == 0);

	db = (UINT32 *) dirtyrow;
	rowlength /= 4;

	do
	{
		val = *db;
		if (val)
			goto founddirty;
		db++;
	}
	while(--rowlength);
	return 0;

founddirty:
	/* we found something dirty */
	do
	{
		*(db++) = 0;
	}
	while(--rowlength);
	return 1;
}

static void draw_body(struct mame_bitmap *bitmap, int base_scanline, int scanline_count, UINT8 **db)
{
	int row;
	int drawn_row = -1;
	int must_redraw = TRUE;
	int i, y, width, screeny;
	int pens_len;
	pen_t pen;
	UINT16 pens[512];
	const UINT8 *videoram_max;
	drawline_proc draw_line;
	struct drawline_params params;

	profiler_mark(PROFILER_VIDEOMAP_DRAWBODY);

	/* figure out vitals */
	width = line_info.visible_columns;
	videoram_max = videoram + (((frame_info.video_base / videoram_windowsize) + 1) * videoram_windowsize);
	
	/* set up pens */
	if (!line_info.charproc)
	{
		if (line_info.flags & VIDEOMAP_FLAGS_ARTIFACT)
			pens_len = 128;
		else
			pens_len = 1 << line_info.grid_depth;
		
		/* allocate the pens */
		assert(pens_len <= (sizeof(pens) / sizeof(pens[0])));

		/* ...and fill them in */
		for (i = 0; i < pens_len; i++)
		{
			pen = i;
			if (line_info.metapalette)
				pen = line_info.metapalette[pen];
			if (line_info.flags & VIDEOMAP_FLAGS_USEPALETTERAM)
				pen = paletteram[pen];
			pens[i] = (UINT16) Machine->pens[pen];
		}
	}

	/* set up parameters */
	memset(&params, 0, sizeof(params));
	params.bytes_per_row = line_info.grid_width * line_info.grid_depth / 8;
	params.offset = line_info.offset;
	params.zoomx = (line_info.visible_columns / line_info.grid_width) / (line_info.charproc ? 8 : 1);
	params.scanline_data = scanline_data;
	params.pens = pens;
	params.charproc = line_info.charproc;
	params.border_value = line_info.border_value;

	/* choose a draw_line function */
	draw_line = get_drawline_proc(line_info.grid_depth, params.zoomx, params.charproc != NULL,
		line_info.flags & VIDEOMAP_FLAGS_ARTIFACT,
		flags & FLAG_ENDIAN_FLIP);

	/* do we have to do offset wrapping? */
	if (line_info.offset_wrap && ((params.offset + params.bytes_per_row) > line_info.offset_wrap))
	{
		params.offset_wrap = line_info.offset_wrap;
		params.inner_draw_line = draw_line;
		draw_line = draw_line_with_offset;
	}

	/* go ahead and draw! */
	for (y = base_scanline; y < base_scanline+scanline_count; y++)
	{
		/* what row are we at? */
		row = y / line_info.scanlines_per_row;

		/* actual location on MAME bitmap */
		screeny = y + frame_info.bordertop_scanlines;

		/* is this dirty (assuming we support dirty buffers?) */
		if (row != drawn_row)
			must_redraw = !db || is_row_dirty(*db, frame_info.pitch);
		if (must_redraw)
		{
			/* do we have to render a new scanline? */
			if (params.charproc || (row != drawn_row))
			{
				if (!scanline_data)
					params.scanline_data = ((UINT16 *) bitmap->line[screeny]) + line_info.borderleft_columns;
				params.row = y - (row * line_info.scanlines_per_row);
				draw_line(&params);
				drawn_row = row;
			}
			else if (!scanline_data)
			{
				/* instead of using draw_scanline, we can use a straight memcpy */
				memcpy(((UINT16 *) bitmap->line[screeny]) + line_info.borderleft_columns, params.scanline_data, width * sizeof(UINT16));
			}

			/* if the orientation is off, draw the scanline */
			if (scanline_data)
				draw_scanline16(bitmap, line_info.borderleft_columns, screeny, width, scanline_data, NULL, -1);
		}

		/* time to up the row? */
		if (((y + 1) / line_info.scanlines_per_row) != row)
		{
			videoram_pos += frame_info.pitch;
			if (videoram_pos >= videoram_max)
				videoram_pos -= videoram_windowsize;
			if (db)
				*db += frame_info.pitch;
		}
	}

	/* mark us dirty, if necessary */
/*	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
	{
		mark_dirty(
			Machine->visible_area.min_x,
			base_scanline + frame_info.bordertop_scanlines,
			Machine->visible_area.max_x,
			base_scanline + frame_info.bordertop_scanlines + scanline_count - 1);
	}
*/
	profiler_mark(PROFILER_END);
}

/* used to draw the border when someone has been doing tricks within the border */
static void plot_modified_border(struct mame_bitmap *bitmap, int x, int y, int width, int height, pen_t pen)
{
	assert(height);

/*	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
		mark_dirty(x, y, x + width - 1, y + height - 1);
*/
	do
	{
		draw_scanline16(bitmap, x, y++, width, border_scanline + x, NULL, -1);
	}
	while(--height);
}

/* main video update routine; draws the border and relies on draw_body to do the active display area */
static void internal_videomap_update(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT8 **db, int draw_border)
{
	int scanline_length;
	int height, width, base;
	void (*plot_border)(struct mame_bitmap *_bitmap, int x, int y, int _width, int _height, pen_t pen);

	profiler_mark(PROFILER_VIDEOMAP_DRAWBORDER);

	plot_border = (flags & FLAG_BORDER_MODIFIED) ? plot_modified_border : bitmap->plot_box;
	scanline_length = cliprect->max_x - cliprect->min_x + 1;

	if (draw_border)
	{
		/* top border */
		height = MIN(cliprect->max_y + 1, frame_info.bordertop_scanlines) - cliprect->min_y;
		if (height > 0)
			plot_border(bitmap, cliprect->min_x, cliprect->min_y, scanline_length, height, border_color);
	}

	/* middle */
	base = MAX(cliprect->min_y, frame_info.bordertop_scanlines);
	height = MIN(cliprect->max_y + 1, frame_info.bordertop_scanlines + frame_info.visible_scanlines) - base;
	if (height > 0)
	{
		if (draw_border)
		{
			/* left border */
			width = MIN(cliprect->max_x + 1, line_info.borderleft_columns) - cliprect->min_x;
			if (width > 0)
				plot_border(bitmap, cliprect->min_x, base, width, height, border_color);

			/* right border */
			width = cliprect->max_x + 1 - MAX(line_info.borderleft_columns + line_info.visible_columns, cliprect->min_x);
			if (width > 0)
				plot_border(bitmap, line_info.borderleft_columns + line_info.visible_columns, base, width, height, border_color);
		}

		/* body */
		profiler_mark(PROFILER_END);
		draw_body(bitmap, base - frame_info.bordertop_scanlines, height, db);
		profiler_mark(PROFILER_VIDEOMAP_DRAWBORDER);
	}

	/* bottom border */
	if (draw_border)
	{
		base = MAX(cliprect->min_y, frame_info.bordertop_scanlines + frame_info.visible_scanlines);
		height = (cliprect->max_y + 1) - base;
		if (height > 0)
			plot_border(bitmap, cliprect->min_x, base, scanline_length, height, border_color);
	}

	profiler_mark(PROFILER_END);
}

void videomap_update(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	struct mame_bitmap *bmp = bitmap;
	int full_refresh = 1;

	if (tmpbitmap)
	{
		/* writing to buffered bitmap; partial refresh (except on first draw) */
		full_refresh = (flags & FLAG_FIRST_DRAW) ? 1 : 0;
		bmp = tmpbitmap;
	}
	else
	{
		/* writing to main bitmap; always refresh */
		flags = 1;
		bmp = bitmap;
	}

	/* if we are at the beginning of the frame, recalculate the video pos */
	if (cliprect->min_y == Machine->visible_area.min_y)
		calc_videoram_pos();

	internal_videomap_update(bmp, cliprect,
		(!full_refresh && videoram_dirtybuffer) ? &videoram_dirtybuffer_pos : NULL,
		((flags & FLAG_BORDER_MODIFIED) || full_refresh) ? 1 : 0);

	/* if we are writing to buffered bitmap, copy it */
	if (tmpbitmap)
		video_update_generic_bitmapped(bitmap, cliprect);
}

/* ----------------------------------------------------------------------- *
 * initialization                                                          *
 * ----------------------------------------------------------------------- */

int videomap_init(const struct videomap_config *config)
{
	int memory_flags;

	/* check parameters for obvious problems */
	assert(config);
	assert(config->intf);
	assert(config->intf->frame_callback);
	assert(config->intf->line_callback);
	assert(config->videoram || mess_ram);
	assert(config->videoram_windowsize || mess_ram_size);

	callbacks = config->intf;
	flags = FLAG_FIRST_DRAW;
	scanline_data = (Machine->orientation) ? (UINT16 *) auto_malloc((Machine->drv->screen_width + 32) * sizeof(UINT16)) : NULL;
	border_scanline = (UINT16 *) auto_malloc(Machine->drv->screen_width * sizeof(UINT16));
	if (!border_scanline)
		return 1;

	/* do we need to perform endian flipping? */
	memory_flags = config->intf->flags & VIDEOMAP_FLAGS_MEMORY_MASK;
#ifdef LSB_FIRST
	if (memory_flags == VIDEOMAP_FLAGS_MEMORY16_BE)
#else
	if (memory_flags == VIDEOMAP_FLAGS_MEMORY16_LE)
#endif
	{
		flags |= FLAG_ENDIAN_FLIP;
	}

	/* are we buffering video */
	tmpbitmap = NULL;
	if (config->intf->flags & VIDEOMAP_FLAGS_BUFFERVIDEO)
	{
		if (video_start_generic_bitmapped() != 0)
			return 1;
	}

	border_position = 0;
	border_color = 0;
	videoram = config->videoram ? config->videoram : mess_ram;
	videoram_windowsize = config->videoram_windowsize ? config->videoram_windowsize : mess_ram_size;
	videoram_dirtybuffer = config->dirtybuffer;
	videoram_dirtybuffer_pos = videoram_dirtybuffer;

	get_border_color();
	get_frame_info();
	get_line_info();
	calc_videoram_pos();
	return 0;
}

#endif /* !VIDEOMAP_TEST */
