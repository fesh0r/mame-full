#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "videomap.h"
#include "mess.h"

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

static int force_partial_update(int row);

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
static const UINT8 *videoram;
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
	FLAG_BORDER_MODIFIED	= 4
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
			thispen = &pens[(c >> ((shift)+6) & 0x3f) * 2];														\
			EMITPIXEL(decl_zoomx, (offset)*8 + 0, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 1, thispen[1]);													\
			thispen = &pens[(c >> ((shift)+4) & 0x3f) * 2];														\
			EMITPIXEL(decl_zoomx, (offset)*8 + 2, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 3, thispen[1]);													\
			thispen = &pens[(c >> ((shift)+2) & 0x3f) * 2];														\
			EMITPIXEL(decl_zoomx, (offset)*8 + 4, thispen[0]);													\
			EMITPIXEL(decl_zoomx, (offset)*8 + 5, thispen[1]);													\
			thispen = &pens[(c >> ((shift)+0) & 0x3f) * 2];														\
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
#define MUST_ALIGN(addr)	(((long) (addr)) & 3)
#else
#define MUST_ALIGN(addr)	(0)
#endif

#define DECLARE_DRAWLINE(decl_bpp, decl_zoomx, has_charproc, has_artifact)\
	static void dl##decl_bpp##_##decl_zoomx##_##has_charproc####has_artifact##(struct drawline_params *params)	\
	{																						\
		const UINT32 *v;																	\
		UINT32 l;																			\
		UINT16 *s;																			\
		UINT16 p;																			\
		UINT32 pl;																			\
		UINT32 last_l, border_value;														\
		const UINT16 *pens;																	\
		register int zoomx;																	\
		int rowlongs;																		\
																							\
		assert(params->zoomx > 0);															\
		assert((decl_zoomx == 0) || (params->zoomx == decl_zoomx));							\
		assert(has_charproc || !params->charproc);											\
		assert(!has_charproc || params->charproc);											\
		assert(videoram_pos);																\
		assert(params->charproc || params->pens);											\
		assert(params->scanline_data);														\
		assert((params->bytes_per_row % 4) == 0);											\
		assert(params->bytes_per_row > 0);													\
																							\
		v = (const UINT32 *) (videoram_pos + params->offset);								\
		rowlongs = params->bytes_per_row / 4;												\
		zoomx = decl_zoomx ? decl_zoomx : params->zoomx;									\
		pens = params->pens;																\
																							\
		if (has_artifact)																	\
			last_l = border_value = params->border_value;									\
																							\
		s = params->scanline_data;															\
		do																					\
		{																					\
			if (!MUST_ALIGN(v))																\
			{																				\
				l = *v;																		\
																							\
			}																				\
			else																			\
			{																				\
				l =		(((UINT32) (((UINT8 *) v)[0])) << SHIFT0)							\
					|	(((UINT32) (((UINT8 *) v)[1])) << SHIFT1)							\
					|	(((UINT32) (((UINT8 *) v)[2])) << SHIFT2)							\
					|	(((UINT32) (((UINT8 *) v)[3])) << SHIFT3);							\
			}																				\
			v++;																			\
																							\
			if (has_artifact)																\
			{																				\
				UINT32 this_l;																\
				this_l = BIG_ENDIANIZE_INT32(l);											\
				l = (last_l << 24) | (this_l >> 8);											\
				EMITBYTE(decl_zoomx, 0, has_charproc, (decl_bpp), 24-8-2, has_artifact);	\
				EMITBYTE(decl_zoomx, 1, has_charproc, (decl_bpp), 16-8-2, has_artifact);	\
				l = (this_l << 8) | ((rowlongs > 1) ? ((UINT8 *) v)[0] : border_value);		\
				EMITBYTE(decl_zoomx, 2, has_charproc, (decl_bpp),  8+8-2, has_artifact);	\
				EMITBYTE(decl_zoomx, 3, has_charproc, (decl_bpp),  0+8-2, has_artifact);	\
				last_l = this_l;															\
			}																				\
			else																			\
			{																				\
				if ((decl_bpp) == 16)														\
				{																			\
					l = LITTLE_ENDIANIZE_INT32(l);											\
					EMITBYTE(decl_zoomx, 0, has_charproc, (decl_bpp), 0, has_artifact);			\
					EMITBYTE(decl_zoomx, 1, has_charproc, (decl_bpp), 16, has_artifact);		\
				}																				\
				else																			\
				{																				\
					EMITBYTE(decl_zoomx, 0, has_charproc, (decl_bpp), SHIFT0, has_artifact);	\
					EMITBYTE(decl_zoomx, 1, has_charproc, (decl_bpp), SHIFT1, has_artifact);	\
					EMITBYTE(decl_zoomx, 2, has_charproc, (decl_bpp), SHIFT2, has_artifact);	\
					EMITBYTE(decl_zoomx, 3, has_charproc, (decl_bpp), SHIFT3, has_artifact);	\
				}																				\
			}																					\
			s += (has_charproc ? 8 : 1) * (decl_zoomx ? decl_zoomx : zoomx) * (32 / decl_bpp);	\
		}																						\
		while(--rowlongs);																		\
	}																							\

DECLARE_DRAWLINE(1, 0, 0, 0)
DECLARE_DRAWLINE(1, 1, 0, 0)
DECLARE_DRAWLINE(1, 2, 0, 0)
DECLARE_DRAWLINE(1, 3, 0, 0)
DECLARE_DRAWLINE(1, 4, 0, 0)
DECLARE_DRAWLINE(1, 8, 0, 0)
DECLARE_DRAWLINE(2, 0, 0, 0)
DECLARE_DRAWLINE(2, 1, 0, 0)
DECLARE_DRAWLINE(2, 2, 0, 0)
DECLARE_DRAWLINE(2, 3, 0, 0)
DECLARE_DRAWLINE(2, 4, 0, 0)
DECLARE_DRAWLINE(2, 8, 0, 0)
DECLARE_DRAWLINE(4, 0, 0, 0)
DECLARE_DRAWLINE(4, 1, 0, 0)
DECLARE_DRAWLINE(4, 2, 0, 0)
DECLARE_DRAWLINE(4, 3, 0, 0)
DECLARE_DRAWLINE(4, 4, 0, 0)
DECLARE_DRAWLINE(4, 8, 0, 0)
DECLARE_DRAWLINE(8, 0, 0, 0)
DECLARE_DRAWLINE(8, 1, 0, 0)
DECLARE_DRAWLINE(8, 2, 0, 0)
DECLARE_DRAWLINE(8, 3, 0, 0)
DECLARE_DRAWLINE(8, 4, 0, 0)
DECLARE_DRAWLINE(8, 8, 0, 0)

DECLARE_DRAWLINE(8, 0, 1, 0)
DECLARE_DRAWLINE(8, 1, 1, 0)
DECLARE_DRAWLINE(8, 2, 1, 0)
DECLARE_DRAWLINE(16, 0, 1, 0)
DECLARE_DRAWLINE(16, 1, 1, 0)
DECLARE_DRAWLINE(16, 2, 1, 0)

DECLARE_DRAWLINE(1, 0, 0, 1)
DECLARE_DRAWLINE(1, 1, 0, 1)
DECLARE_DRAWLINE(1, 2, 0, 1)
DECLARE_DRAWLINE(1, 3, 0, 1)
DECLARE_DRAWLINE(1, 4, 0, 1)
DECLARE_DRAWLINE(1, 8, 0, 1)

/* this drawline function is used to accomodate cases where an offset results in a split line */
static void draw_line_with_offset(struct drawline_params *params)
{
	UINT16 *buf = NULL;
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
			if (!buf)
			{
				buf = malloc((pixels_per_byte * 4) * sizeof(UINT16));
				if (!buf)
					return;	/*PANIC*/
			}
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
	if (buf)
		free(buf);
}

typedef void (*drawline_proc)(struct drawline_params *params);

static drawline_proc get_drawline_proc(int bits_per_pixel, int zoomx, int has_charproc, int has_artifact)
{
	struct drawline_proc_entry
	{
		int bits_per_pixel;
		int zoomx;
		drawline_proc without_charproc;
		drawline_proc with_charproc;
		drawline_proc with_artifact;
	};

	struct drawline_proc_entry procmap[] = 
	{
		{  1, 1,	dl1_1_00,	NULL,		dl1_1_01 },
		{  1, 2,	dl1_2_00,	NULL,		dl1_2_01 },
		{  1, 3,	dl1_3_00,	NULL,		dl1_3_01 },
		{  1, 4,	dl1_4_00,	NULL,		dl1_4_01 },
		{  1, 8,	dl1_8_00,	NULL,		dl1_8_01 },
		{  1, 0,	dl1_0_00,	NULL,		dl1_0_01 },
		{  2, 1,	dl2_1_00,	NULL,		NULL },
		{  2, 2,	dl2_2_00,	NULL,		NULL },
		{  2, 3,	dl2_3_00,	NULL,		NULL },
		{  2, 4,	dl2_4_00,	NULL,		NULL },
		{  2, 8,	dl2_8_00,	NULL,		NULL },
		{  2, 0,	dl2_0_00,	NULL,		NULL },
		{  4, 1,	dl4_1_00,	NULL,		NULL },
		{  4, 2,	dl4_2_00,	NULL,		NULL },
		{  4, 3,	dl4_3_00,	NULL,		NULL },
		{  4, 4,	dl4_4_00,	NULL,		NULL },
		{  4, 8,	dl4_8_00,	NULL,		NULL },
		{  4, 0,	dl4_0_00,	NULL,		NULL },
		{  8, 1,	dl8_1_00,	dl8_1_10,	NULL },
		{  8, 2,	dl8_2_00,	dl8_2_10,	NULL },
		{  8, 3,	dl8_3_00,	NULL,		NULL },
		{  8, 4,	dl8_4_00,	NULL,		NULL },
		{  8, 8,	dl8_8_00,	NULL,		NULL },
		{  8, 0,	dl8_0_00,	dl8_0_10,	NULL },
		{ 16, 1,	NULL,		dl16_1_10,	NULL },
		{ 16, 2,	NULL,		dl16_2_10,	NULL },
		{ 16, 0,	NULL,		dl16_0_10,	NULL }
	};

	int i;
	struct drawline_proc_entry *ent;
	drawline_proc draw_line = NULL;

	assert(!has_artifact || !has_charproc);

	for (i = 0; i < sizeof(procmap) / sizeof(procmap[0]); i++)
	{
		ent = &procmap[i];
		if ((bits_per_pixel == ent->bits_per_pixel) && ((ent->zoomx == zoomx) || (ent->zoomx == 0)))
		{
			draw_line = has_artifact ? ent->with_artifact : (has_charproc ? ent->with_charproc : ent->without_charproc);
			break;
		}
	}
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
	assert(scanline <= Machine->scrbitmap->height);

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
	UINT16 *pens = NULL;
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
		pens = (UINT16 *) malloc(pens_len * sizeof(UINT16));
		if (!pens)
			return;	/* PANIC */

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
	draw_line = get_drawline_proc(line_info.grid_depth, params.zoomx, params.charproc != NULL, line_info.flags & VIDEOMAP_FLAGS_ARTIFACT);

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
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
	{
		mark_dirty(
			Machine->visible_area.min_x,
			base_scanline + frame_info.bordertop_scanlines,
			Machine->visible_area.max_x,
			base_scanline + frame_info.bordertop_scanlines + scanline_count - 1);
	}

	/* free the pens, if used */
	if (pens)
		free(pens);

	profiler_mark(PROFILER_END);
}

/* used to draw the border when someone has been doing tricks within the border */
static void plot_modified_border(struct mame_bitmap *bitmap, int x, int y, int width, int height, pen_t pen)
{
	assert(height);

	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
		mark_dirty(x, y, x + width - 1, y + height - 1);

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

	plot_border = (flags & FLAG_BORDER_MODIFIED) ? plot_modified_border : plot_box;
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

static int last_partial_scanline;
static struct mame_bitmap *the_bitmap;

static int force_partial_update(int scanline)
{
	struct rectangle r;

	assert(scanline >= 0);
	assert(scanline < Machine->scrbitmap->height);

	if (the_bitmap)
	{
		if (last_partial_scanline <= scanline)
		{
#if 0
			logerror("force_partial_update(): scanline=%d\n", scanline);
#endif
			if (last_partial_scanline == 0)
				calc_videoram_pos();
			r = Machine->visible_area;
			r.min_y = last_partial_scanline;
			r.max_y = scanline;
			internal_videomap_update(the_bitmap, &r, NULL, TRUE);
			last_partial_scanline = scanline + 1;
		}
	}
	return 0;
}

void videomap_update(struct mame_bitmap *bitmap, int full_refresh)
{
	struct rectangle r;

	r = Machine->visible_area;
	r.min_y = last_partial_scanline;
	the_bitmap = bitmap;

	if (last_partial_scanline == 0)
		calc_videoram_pos();

	internal_videomap_update(bitmap, &r,
		(full_refresh || !videoram_dirtybuffer) ? NULL : &videoram_dirtybuffer_pos,
		full_refresh);
	last_partial_scanline = 0;
}

/* ----------------------------------------------------------------------- *
 * initialization                                                          *
 * ----------------------------------------------------------------------- */

void videomap_init(const struct videomap_config *config)
{
	/* check parameters for obvious problems */
	assert(config);
	assert(config->intf);
	assert(config->intf->frame_callback);
	assert(config->intf->line_callback);
	assert(config->videoram || mess_ram);
	assert(config->videoram_windowsize || mess_ram_size);

	callbacks = config->intf;
	flags = 0;
	scanline_data = (Machine->orientation) ? (UINT16 *) auto_malloc((Machine->drv->screen_width + 32) * sizeof(UINT16)) : NULL;
	border_scanline = (UINT16 *) auto_malloc(Machine->drv->screen_width * sizeof(UINT16));
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
}

#endif /* !VIDEOMAP_TEST */
