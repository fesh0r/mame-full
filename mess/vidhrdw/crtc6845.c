/***************************************************************************

	Motorola 6845 CRT controller and emulation

	This code emulates the functionality of the 6845 chip, and also
	supports the functionality of chips related to the 6845

	Peter Trauner
	Nathan Woods

***************************************************************************/

#include <stdio.h>
#include "snprintf.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#include "mscommon.h"
#include "includes/crtc6845.h"

/***************************************************************************

	Constants & macros

***************************************************************************/

#define VERBOSE 0

#if VERBOSE
#define DBG_LOG(N,M,A)      \
    if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(N,M,A)
#endif

/***************************************************************************

	Type definitions

***************************************************************************/

struct crtc6845
{
	struct crtc6845_config config;
	UINT8 reg[18];
	UINT8 index_;
	double cursor_time;
	int cursor_on;
};

struct reg_mask
{
	UINT8 store_mask;
	UINT8 read_mask;
};

struct crtc6845 *crtc6845;

/***************************************************************************

	Local variables

***************************************************************************/

/*-------------------------------------------------
	crtc6845_reg_mask - an array specifying how
	much of any given register "registers", per
	m6845 personality
-------------------------------------------------*/

static const struct reg_mask crtc6845_reg_mask[2][18] =
{
	{
		/* M6845_PERSONALITY_GENUINE */
		{ 0xff, 0x00 },	{ 0xff, 0x00 },	{ 0xff, 0x00 },	{ 0xff, 0x00 },
		{ 0x7f, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x7f, 0x00 },
		{ 0x3f, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x1f, 0x00 },
		{ 0x3f, 0x3f },	{ 0xff, 0xff },	{ 0x3f, 0x3f },	{ 0xff, 0xff },
		{ 0xff, 0x3f },	{ 0xff, 0xff }
	},
	{
		/* M6845_PERSONALITY_PC1512 */
		{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },
		{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },
		{ 0x00, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x1f, 0x00 },
		{ 0x3f, 0x3f },	{ 0xff, 0xff },	{ 0x3f, 0x3f },	{ 0xff, 0xff },
		{ 0xff, 0x3f },	{ 0xff, 0xff }
	}
};

/***************************************************************************

	Functions

***************************************************************************/

struct crtc6845 *crtc6845_init(const struct crtc6845_config *config)
{
	struct crtc6845 *crtc;

	crtc = auto_malloc(sizeof(struct crtc6845));
	if (!crtc)
		return NULL;

	memset(crtc, 0, sizeof(*crtc));
	crtc->cursor_time = timer_get_time();
	crtc->config = *config;
	crtc6845 = crtc;
	return crtc;
}

#define REG(x) (crtc->reg[x])

static int crtc6845_clocks_in_frame(struct crtc6845 *crtc)
{
	int clocks=CRTC6845_COLUMNS*CRTC6845_LINES;
	switch (CRTC6845_INTERLACE_MODE) {
	case CRTC6845_INTERLACE_SIGNAL: // interlace generation of video signals only
	case CRTC6845_INTERLACE: // interlace
		return clocks/2;
	default:
		return clocks;
	}
}

void crtc6845_set_clock(struct crtc6845 *crtc, int freq)
{
	assert(crtc);
	crtc->config.freq = freq;
	schedule_full_refresh();
}

void crtc6845_time(struct crtc6845 *crtc)
{
	double neu, ftime;
	struct crtc6845_cursor cursor;

	neu = timer_get_time();

	if (crtc6845_clocks_in_frame(crtc) == 0.0)
		return;

	ftime = crtc6845_clocks_in_frame(crtc) * 16.0 / crtc->config.freq;

	if (CRTC6845_CURSOR_MODE==CRTC6845_CURSOR_32FRAMES)
		ftime*=2;

	if (neu-crtc->cursor_time>ftime)
	{
		crtc->cursor_time += ftime;
		crtc6845_get_cursor(crtc, &cursor);

		if (crtc->config.cursor_changed)
			crtc->config.cursor_changed(&cursor);

		crtc->cursor_on ^= 1;
	}
}

int crtc6845_get_char_columns(struct crtc6845 *crtc) 
{ 
	return CRTC6845_CHAR_COLUMNS;
}

int crtc6845_get_char_height(struct crtc6845 *crtc) 
{
	return CRTC6845_CHAR_HEIGHT;
}

int crtc6845_get_char_lines(struct crtc6845 *crtc) 
{ 
	return CRTC6845_CHAR_LINES;
}

int crtc6845_get_start(struct crtc6845 *crtc) 
{
	return CRTC6845_VIDEO_START;
}

int crtc6845_get_personality(struct crtc6845 *crtc)
{
	return crtc->config.personality;
}

void crtc6845_get_cursor(struct crtc6845 *crtc, struct crtc6845_cursor *cursor)
{
	switch (CRTC6845_CURSOR_MODE) {
	case CRTC6845_CURSOR_OFF:
		cursor->on = 0;
		break;

	case CRTC6845_CURSOR_16FRAMES:
	case CRTC6845_CURSOR_32FRAMES:
		cursor->on = crtc->cursor_on;
		break;

	default:
		cursor->on = 1;
		break;
	}

	cursor->pos = CRTC6845_CURSOR_POS;
	cursor->top = CRTC6845_CURSOR_TOP;
	cursor->bottom = CRTC6845_CURSOR_BOTTOM;
}

data8_t crtc6845_port_r(struct crtc6845 *crtc, int offset)
{
	data8_t val = 0xff;
	int index_;

	if (offset & 1)
	{
		index_ = crtc->index_ & 0x1f;
		if (index_ < (sizeof(crtc->reg) / sizeof(crtc->reg[0])))
			val = crtc->reg[index_] & crtc6845_reg_mask[crtc->config.personality][index_].read_mask;
	}
	else
	{
		val = crtc->index_;
	}
	return val;
}

void crtc6845_port_w(struct crtc6845 *crtc, int offset, data8_t data)
{
	struct crtc6845_cursor cursor;
	int index_;

	if (offset & 1)
	{
		/* write to a 6845 register, if supported */
		index_ = crtc->index_ & 0x1f;
		if (index_ < (sizeof(crtc->reg) / sizeof(crtc->reg[0])))
		{
			crtc->reg[crtc->index_] = data & crtc6845_reg_mask[crtc->config.personality][index_].store_mask;

			/* are there special consequences to writing to this register */
			switch (index_) {
			case 0xa:
			case 0xb:
			case 0xe:
			case 0xf:
				crtc6845_get_cursor(crtc, &cursor);
				if (crtc->config.cursor_changed)
					crtc->config.cursor_changed(&cursor);
				break;

			default:
				schedule_full_refresh();
				break;
			}
		}
	}
	else
	{
		/* change the index_ register */
		crtc->index_ = data;
	}
}

READ_HANDLER ( crtc6845_0_port_r )
{
	return crtc6845_port_r(crtc6845, offset);
}

WRITE_HANDLER ( crtc6845_0_port_w )
{
	crtc6845_port_w(crtc6845, offset, data);
}

