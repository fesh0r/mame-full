#ifndef __STIC_H
#define __STIC_H

READ16_HANDLER( stic_r );
WRITE16_HANDLER( stic_w );

struct intv_sprite_type
{
	int visible;
	int xpos;
	int ypos;
	int coll;
	int xsize;
	int ysize;
	int xflip;
	int yflip;
	int behind_foreground;
	int grom;
	int card;
	int color;
	int yres;
};

extern struct intv_sprite_type intv_sprite[];
extern int intv_color_stack_mode;
extern int intv_color_stack_offset;
extern int intv_color_stack[];
extern int intv_stic_handshake;
extern int intv_border_color;

#endif
