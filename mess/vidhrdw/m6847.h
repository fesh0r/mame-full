/* m6847.h -- Implementation of Motorola 6847 video hardware chip */

#ifndef _M6847_H
#define _M6847_H

#include "driver.h"
#include "vidhrdw/generic.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************* Initialization & Functionality *******************/

#define M6847_TOTAL_COLORS 17

enum {
	M6847_VERSION_ORIGINAL,
	M6847_VERSION_M6847Y,
	M6847_VERSION_M6847T1
};

struct m6847_init_params {
	int version;				/* use one of the above initialization constants */
	int artifactdipswitch;		/* dip switch that controls artifacting; -1 if NA */
	UINT8 *ram;					/* the base of RAM */
	int ramsize;				/* the size of accessible RAM */
	void (*charproc)(UINT8 c);	/* the proc that gives the host a chance to change mode bits */

	mem_write_handler hs_func;	/* Horizontal sync */
	mem_write_handler fs_func;	/* Field sync */
	double callback_delay;		/* Amount of time to wait before invoking callbacks (this is a CoCo related hack */
};

void m6847_vh_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
int m6847_vh_start(const struct m6847_init_params *params);
void m6847_vh_update(struct osd_bitmap *bitmap,int full_refresh);
#define m6847_vh_stop			generic_vh_stop

/******************* Modifiers *******************/

enum {
	M6847_BORDERCOLOR_BLACK,
	M6847_BORDERCOLOR_GREEN,
	M6847_BORDERCOLOR_WHITE,
	M6847_BORDERCOLOR_ORANGE
};

int m6847_get_bordercolor(void);

/* This allows the size of accessable RAM to be resized */
void m6847_set_ram_size(int ramsize);

/* This is the base of video memory, within the ram specified by m6847_vh_start() */
void m6847_set_video_offset(int offset);
int m6847_get_video_offset(void);

/* Touches VRAM; offset is from vram base position */
void m6847_touch_vram(int offset);

/* Changes the height of each row */
void m6847_set_row_height(int rowheight);
void m6847_set_cannonical_row_height(void);

/******************* 1-bit mode port interfaces *******************/

READ_HANDLER( m6847_ag_r );
READ_HANDLER( m6847_as_r );
READ_HANDLER( m6847_intext_r );
READ_HANDLER( m6847_inv_r );
READ_HANDLER( m6847_css_r );
READ_HANDLER( m6847_gm2_r );
READ_HANDLER( m6847_gm1_r );
READ_HANDLER( m6847_gm0_r );
READ_HANDLER( m6847_hs_r );
READ_HANDLER( m6847_fs_r );

WRITE_HANDLER( m6847_ag_w );
WRITE_HANDLER( m6847_as_w );
WRITE_HANDLER( m6847_intext_w );
WRITE_HANDLER( m6847_inv_w );
WRITE_HANDLER( m6847_css_w );
WRITE_HANDLER( m6847_gm2_w );
WRITE_HANDLER( m6847_gm1_w );
WRITE_HANDLER( m6847_gm0_w );

#ifdef __cplusplus
}
#endif

#endif /* _M6847_H */

