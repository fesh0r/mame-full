#include "driver.h"
#include "vidhrdw/stic.h"
#include "includes/intv.h"

/* STIC variables */
struct intv_sprite_type intv_sprite[8];
int intv_stic_handshake;
int intv_border_color;
int intv_color_stack[4];
int intv_color_stack_mode = 1; // for now
int intv_color_stack_offset = 0; // for now

READ16_HANDLER( stic_r )
{
	//logerror("%x = stic_r(%x)\n",0,offset);
	switch (offset)
	{
		case 0x21:
			intv_color_stack_mode = 1;
			intv_color_stack_offset = 0;
			//logerror("Setting color stack mode\n");
			break;
	}
	return 0;
}

WRITE16_HANDLER( stic_w )
{
	struct intv_sprite_type *s;

	//logerror("stic_w(%x) = %x\n",offset,data);
	switch (offset)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			s =  &intv_sprite[offset%8];
			s->xsize = 2;
			if (data&0x400)
				s->xsize *= 2;
			if (data&0x200)
				s->visible = 1;
			else
				s->visible = 0;
			if (data&0x100)
				s->coll = 1;
			else
				s->coll = 0;
			s->xpos = data&0xff;
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			s =  &intv_sprite[offset%8];
			if (data&0x800)
				s->xflip = 1;
			else
				s->xflip = 0;
			if (data&0x400)
				s->yflip = 1;
			else
				s->yflip = 0;
			s->ysize = 1;
			if (data&0x200)
				s->ysize *= 4;
			if (data&0x100)
				s->ysize *= 2;
			if (data&0x80)
				s->yres = 2;
			else
				s->yres = 1;
			s->ypos = data&0x7f;
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			s =  &intv_sprite[offset%8];
			if (data&0x2000)
				s->behind_foreground = 1;
			else
				s->behind_foreground = 0;
			if (data&0x0800)
				s->grom = 0;
			else
				s->grom = 1;
			s->card = (data>>3)&0xff;
			s->color = ((data>>9)&0x8) | (data&0x7);
			break;
		case 0x20:
			//logerror("***Writing a %x to the STIC handshake\n",data);
			intv_stic_handshake = 1;
			break;
		case 0x21:
			intv_color_stack_mode = 0;
			//logerror("Setting fg/bg mode\n");
			break;
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
			//logerror("Setting color_stack[%x] = %x\n",offset&0x3,data&0xf);
			intv_color_stack[offset&0x3] = data&0xf;
			break;
		case 0x2c:
			//logerror("***Writing a %x to the border color\n",data);
			intv_border_color = data;
			break;
	}
}
