/***************************************************************************

  motorola cathode ray tube controller 6845

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include <stdio.h>
#include "snprintf.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#include "mscommon.h"
#include "includes/crtc6845.h"

#define VERBOSE 0

#if VERBOSE
#define DBG_LOG(N,M,A)      \
    if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(N,M,A)
#endif

typedef struct _CRTC6845
{
	CRTC6845_CONFIG config;
	UINT8 reg[18];
	UINT8 index;
	int lightpen_pos;
	double cursor_time;
	int cursor_on;
} CRTC6845;

static CRTC6845 crtc6845_static;
CRTC6845 *crtc6845=&crtc6845_static;

void crtc6845_init (CRTC6845 *crtc, CRTC6845_CONFIG *config)
{
	memset(crtc, 0, sizeof(*crtc));
	crtc->cursor_time=timer_get_time();
	crtc->config=*config;
}

static const struct { 
	int stored, 
		read;
} reg_mask[]= { 
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0x7f, 0 },
	{ 0x1f, 0 },
	{ 0x7f, 0 },
	{ 0x7f, 0 },
	{  0x3f, 0 },
	{  0x1f, 0 },
	{  0x7f, 0 },
	{  0x1f, 0 },
	{  0x3f, 0x3f },
	{  0xff, 0xff },
	{  0x3f, 0x3f },
	{  0xff, 0xff },
	{  -1, 0x3f },
	{  -1, 0xff },
};
#define REG(x) (crtc->reg[x]&reg_mask[x].stored)

static int crtc6845_clocks_in_frame(CRTC6845 *crtc)
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

void crtc6845_set_clock(struct _CRTC6845 *crtc, int freq)
{
	crtc->config.freq=freq;
	schedule_full_refresh();
}

void crtc6845_time(CRTC6845 *crtc)
{
	double neu, ftime;
	CRTC6845_CURSOR cursor;

	neu=timer_get_time();

	if (crtc6845_clocks_in_frame(crtc)==0.0) return;
	ftime=crtc6845_clocks_in_frame(crtc)*16.0/crtc6845->config.freq;
	if ( CRTC6845_CURSOR_MODE==CRTC6845_CURSOR_32FRAMES) ftime*=2;
	if (neu-crtc->cursor_time>ftime) {
		crtc->cursor_time+=ftime;
		crtc6845_get_cursor(crtc, &cursor);
		if (crtc->config.cursor_changed) crtc->config.cursor_changed(&cursor);
		crtc->cursor_on^=1;
	}
}

int crtc6845_get_char_columns(struct _CRTC6845 *crtc) 
{ 
	return CRTC6845_CHAR_COLUMNS;
}

int crtc6845_get_char_height(struct _CRTC6845 *crtc) 
{
	return CRTC6845_CHAR_HEIGHT;
}

int crtc6845_get_char_lines(struct _CRTC6845 *crtc) 
{ 
	return CRTC6845_CHAR_LINES;
}

int crtc6845_get_start(struct _CRTC6845 *crtc) 
{
	return CRTC6845_VIDEO_START;
}

void crtc6845_get_cursor(struct _CRTC6845 *crtc, CRTC6845_CURSOR *cursor)
{
	cursor->pos=CRTC6845_CURSOR_POS;
	switch (CRTC6845_CURSOR_MODE) {
	default: cursor->on=1;break;
	case CRTC6845_CURSOR_OFF: cursor->on=0;break;
	case CRTC6845_CURSOR_16FRAMES:
		cursor->on=crtc->cursor_on;
	case CRTC6845_CURSOR_32FRAMES:
		cursor->on=crtc->cursor_on;
		break;
	}
	cursor->top=CRTC6845_CURSOR_TOP;
	cursor->bottom=CRTC6845_CURSOR_BOTTOM;
}

void crtc6845_port_w(struct _CRTC6845 *crtc, int offset, data8_t data)
{
	CRTC6845_CURSOR cursor;
	if (offset & 1)
	{
		if ((crtc->index & 0x1f) < 18) {
			switch (crtc->index & 0x1f) {
			case 0xa:case 0xb:
			case 0xe:case 0xf:
				crtc6845_get_cursor(crtc, &cursor);
				crtc->reg[crtc->index]=data;
				if (crtc->config.cursor_changed) crtc->config.cursor_changed(&cursor);
				break;
			default:
				schedule_full_refresh();
				crtc->reg[crtc->index]=data;
				break;
			}
				DBG_LOG (2, "crtc_port_w", ("%.2x:%.2x\n", crtc->index, data));
		} else { 
			DBG_LOG (1, "crtc6845_port_w", ("%.2x:%.2x\n", crtc->index, data));
		}
	}
	else
	{
		crtc->index = data;
	}
}

data8_t crtc6845_port_r(struct _CRTC6845 *crtc, int offset)
{
	int val;

	val = 0xff;
	if (offset & 1)
	{
		if ((crtc->index & 0x1f) < 18)
		{
			switch (crtc->index & 0x1f)
			{
			case 0x10: val=crtc->lightpen_pos>>8;break;
			case 0x11: val=crtc->lightpen_pos&0xff;break;
				break;
			default:
				val=crtc->reg[crtc->index&0x1f]&reg_mask[crtc->index&0x1f].read;
			}
		}
		DBG_LOG (1, "crtc6845_port_r", ("%.2x:%.2x\n", crtc->index, val));
	}
	else
	{
		val = crtc->index;
	}
	return val;
}

void crtc6845_state (void)
{
	char text[50]="";

	snprintf (text, sizeof (text), "crtc6845 %.2x %.2x %.2x %.2x",
			  crtc6845->reg[0xc], crtc6845->reg[0xd], crtc6845->reg[0xe],crtc6845->reg[0xf]);

	statetext_display_text(text);
}

WRITE_HANDLER ( crtc6845_0_port_w ) { crtc6845_port_w(crtc6845, offset, data); }
READ_HANDLER ( crtc6845_0_port_r ) { return crtc6845_port_r(crtc6845, offset); }
