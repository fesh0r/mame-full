#include "includes/pc_aga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/crtc6845.h"
#include "vidhrdw/generic.h"

static struct {
	AGA_MODE mode;
} aga= { AGA_OFF };

void pc_aga_set_mode(AGA_MODE mode)
{
	aga.mode=mode;
	switch (aga.mode) {
	case AGA_COLOR:
		crtc6845_set_clock(crtc6845, 10000000/*?*/);
		break;
	case AGA_MONO:
		crtc6845_set_clock(crtc6845, 10000000/*?*/);
		break;
	case AGA_OFF: break;
	}
}

extern void pc_aga_timer(void)
{
	switch (aga.mode) {
	case AGA_COLOR: pc_cga_timer();break;
	case AGA_MONO: pc_mda_timer();break;
	case AGA_OFF: break;
	}
}

void pc_aga_cursor(CRTC6845_CURSOR *cursor)
{
	switch (aga.mode) {
	case AGA_COLOR: pc_cga_cursor(cursor);break;
	case AGA_MONO: pc_mda_cursor(cursor);break;
	case AGA_OFF: break;
	}
}


static CRTC6845_CONFIG config= { 14318180 /*?*/, pc_aga_cursor };

extern int  pc_aga_vh_start(void)
{
	crtc6845_init(crtc6845, &config);
	pc_mda_europc_init(crtc6845);
	pc_cga_init_video(crtc6845);

	return generic_vh_start();
}

extern void pc_aga_vh_stop(void)
{
	generic_vh_stop();
}

extern void pc_aga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	switch (aga.mode) {
	case AGA_COLOR: pc_cga_vh_screenrefresh(bitmap, full_refresh);break;
	case AGA_MONO: pc_mda_vh_screenrefresh(bitmap, full_refresh);break;
	case AGA_OFF: break;
	}
}

extern WRITE_HANDLER ( pc_aga_videoram_w )
{
	switch (aga.mode) {
	case AGA_COLOR:
		if (offset>=0x8000) pc_cga_videoram_w(offset-0x8000, data);
		break;
	case AGA_MONO:
		pc_mda_videoram_w(offset,data);
		break;
	case AGA_OFF: break;
	}
}

READ_HANDLER( pc_aga_videoram_r )
{
	switch (aga.mode) {
	case AGA_COLOR: 
		if (offset>=0x8000) return videoram[offset-0x8000];
		return 0;
	case AGA_MONO:
		return videoram[offset];
	case AGA_OFF: break;
	}
	return 0;
}

extern WRITE_HANDLER ( pc_aga_mda_w )
{
	if (aga.mode==AGA_MONO)
		pc_MDA_w(offset, data);
}

extern WRITE_HANDLER ( pc_aga_cga_w )
{
	if (aga.mode==AGA_COLOR)
		pc_CGA_w(offset, data);
}

extern READ_HANDLER ( pc_aga_mda_r )
{
	if (aga.mode==AGA_MONO)
		return pc_MDA_r(offset);
	return 0xff;
}

extern READ_HANDLER ( pc_aga_cga_r )
{
	if (aga.mode==AGA_COLOR)
		return pc_CGA_r(offset);
	return 0xff;
}




