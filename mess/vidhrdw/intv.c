#include "driver.h"
#include "includes/intv.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/stic.h"

int intv_vh_start(void)
{
	int i;

	if (generic_bitmapped_vh_start())
		return 1;

	for(i=0;i<64;i++)
		intv_gramdirty[i] = 1;

    return 0;
}

void intv_vh_stop(void)
{
	generic_bitmapped_vh_stop();
}

static void draw_background(struct mame_bitmap *bitmap, int transparency)
{
	/* First, draw the background */
	int offs = 0;
	int value = 0;
	int row,col;
	int fgcolor,bgcolor = 0;
	int code;

	int colora, colorb, colorc, colord;

	int n_bit;
	int p_bit;
	int g_bit;

	int j;

	if (intv_color_stack_mode == 1)
	{
		intv_color_stack_offset = 0;
		for(row = 0; row < 12; row++)
		{
			for(col = 0; col < 20; col++)
			{
				value = intv_ram16[offs];

				n_bit = value & 0x2000;
				p_bit = value & 0x1000;
				g_bit = value & 0x0800;

				if (p_bit && (!g_bit)) /* colored squares mode */
				{
					colora = value&0x7;
					colorb = (value>>3)&0x7;
					colorc = (value>>6)&0x7;
					colord = ((n_bit>>11)&0x4) + ((value>>9)&0x3);
					/* color 7 if the top of the color stack in this mode */
					if (colora == 7) colora = intv_color_stack[3];
					if (colorb == 7) colorb = intv_color_stack[3];
					if (colorc == 7) colorc = intv_color_stack[3];
					if (colord == 7) colord = intv_color_stack[3];
					plot_box(bitmap,col*16,row*16,8,8,Machine->pens[colora]);
					plot_box(bitmap,col*16+8,row*16,8,8,Machine->pens[colorb]);
					plot_box(bitmap,col*16,row*16+8,8,8,Machine->pens[colorc]);
					plot_box(bitmap,col*16+8,row*16+8,8,8,Machine->pens[colord]);
				}
				else /* normal color stack mode */
				{
					if (n_bit) /* next color */
					{
						intv_color_stack_offset += 1;
						intv_color_stack_offset &= 3;
					}

					if (p_bit) /* pastel color set */
						fgcolor = (value&0x7) + 8;
					else
						fgcolor = value&0x7;

					bgcolor = intv_color_stack[intv_color_stack_offset];
					code = (value>>3)&0xff;

					if (g_bit) /* read from gram */
					{
						code %= 64;  /* keep from going outside the array */
						//if (intv_gramdirty[code] == 1)
						{
							decodechar(Machine->gfx[1],
								   code,
								   intv_gram,
								   Machine->drv->gfxdecodeinfo[1].gfxlayout);
							intv_gramdirty[code] = 0;
						}
						/* Draw GRAM char */
						drawgfx(bitmap,Machine->gfx[1],
							code,
							bgcolor*16+fgcolor,
							0,0,col*16,row*16,
							0,transparency,bgcolor);

						for(j=0;j<8;j++)
						{
							//plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, Machine->pens[1]);
							//plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, Machine->pens[1]);
						}

					}
					else /* read from grom */
					{
						drawgfx(bitmap,Machine->gfx[0],
							code,
							bgcolor*16+fgcolor,
							0,0,col*16,row*16,
							0,transparency,bgcolor);

						for(j=0;j<8;j++)
						{
							//plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, Machine->pens[2]);
							//plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, Machine->pens[2]);
						}
					}
				}
				offs++;
			} /* next col */
		} /* next row */
	}
	else
	{
		/* fg/bg mode goes here */
		for(row = 0; row < 12; row++)
		{
			for(col = 0; col < 20; col++)
			{
				value = intv_ram16[offs];
				fgcolor = value & 0x07;
				bgcolor = ((value & 0x2000)>>11)+((value & 0x1600)>>9);
				code = (value & 0x01f8)>>3;

				if (value & 0x0800) /* read for GRAM */
				{
					//if (intv_gramdirty[code] == 1)
					{
						decodechar(Machine->gfx[1],
							   code,
							   intv_gram,
							   Machine->drv->gfxdecodeinfo[1].gfxlayout);
						intv_gramdirty[code] = 0;
					}
					/* Draw GRAM char */
					drawgfx(bitmap,Machine->gfx[1],
						code,
						bgcolor*16+fgcolor,
						0,0,col*16,row*16,
						0,transparency,bgcolor);
				}
				else /* read from GROM */
				{
					drawgfx(bitmap,Machine->gfx[0],
						code,
						bgcolor*16+fgcolor,
						0,0,col*16,row*16,
						0,transparency,bgcolor);
				}
				offs++;
			} /* next col */
		} /* next row */
	}
}

/* TBD: need to handle sprites behind foreground? */
static void draw_sprites(struct mame_bitmap *bitmap, int behind_foreground)
{
	int i;
	int code;

	for(i=7;i>=0;--i)
	{
		struct intv_sprite_type *s = &intv_sprite[i];
		if (s->visible && (s->behind_foreground == behind_foreground))
		{
			code = s->card;
			if (!s->grom)
			{
				code %= 64;  /* keep from going outside the array */
				if (s->yres == 1)
				{
					//if (intv_gramdirty[code] == 1)
					{
						decodechar(Machine->gfx[1],
						   code,
						   intv_gram,
						   Machine->drv->gfxdecodeinfo[1].gfxlayout);
						intv_gramdirty[code] = 0;
					}
					/* Draw GRAM char */
					drawgfxzoom(bitmap,Machine->gfx[1],
						code,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
				}
				else
				{
					//if ((intv_gramdirty[code] == 1) || (intv_gramdirty[code+1] == 1))
					{
						decodechar(Machine->gfx[1],
						   code,
						   intv_gram,
						   Machine->drv->gfxdecodeinfo[1].gfxlayout);
						decodechar(Machine->gfx[1],
						   code+1,
						   intv_gram,
						   Machine->drv->gfxdecodeinfo[1].gfxlayout);
						intv_gramdirty[code] = 0;
						intv_gramdirty[code+1] = 0;
					}
					/* Draw GRAM char */
					drawgfxzoom(bitmap,Machine->gfx[1],
						code,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16+(s->yflip)*s->ysize*8,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
					drawgfxzoom(bitmap,Machine->gfx[1],
						code+1,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16+(1-s->yflip)*s->ysize*8,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
				}
			}
			else
			{
				if (s->yres == 1)
				{
					/* Draw GROM char */
					drawgfxzoom(bitmap,Machine->gfx[0],
						code,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
				}
				else
				{
					drawgfxzoom(bitmap,Machine->gfx[0],
						code,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16+(s->yflip)*s->ysize*8,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
					drawgfxzoom(bitmap,Machine->gfx[0],
						code+1,
						s->color,
						s->xflip,s->yflip,
						s->xpos*2-16,s->ypos*2-16+(1-s->yflip)*s->ysize*8,
						&Machine->visible_area,TRANSPARENCY_PEN,0,
						0x8000*s->xsize, 0x8000*s->ysize);
				}
			}
		}
	}
}

static void draw_borders(struct mame_bitmap *bm)
{
	if (intv_left_edge_inhibit)
	{
		plot_box(bm,0,0,16-intv_col_delay*2,16*12,Machine->pens[intv_border_color]);
	}
	if (intv_top_edge_inhibit)
	{
		plot_box(bm,0,0,16*20,16-intv_row_delay*2,Machine->pens[intv_border_color]);
	}
}

static int col_delay = 0;
static int row_delay = 0;

void stic_screenrefresh()
{

	logerror("%g: SCREEN_REFRESH\n",timer_get_time());

	if (intv_stic_handshake != 0)
	{
		intv_stic_handshake = 0;
		/* Draw the complete char set */
		draw_background(tmpbitmap, TRANSPARENCY_NONE);
		/* draw any sprites that go behind the foreground */
		draw_sprites(tmpbitmap,1);
		/* redraw the foreground of the char set */
		draw_background(tmpbitmap, TRANSPARENCY_PEN);
		/* draw sprites that go on top (normal) */
		draw_sprites(tmpbitmap,0);
		/* draw the screen borders if enabled */
		draw_borders(tmpbitmap);
	}
	else
	{
		/* STIC disabled, just fill with border color */
		fillbitmap(tmpbitmap,Machine->pens[intv_border_color],&Machine->visible_area);
	}
	col_delay = intv_col_delay;
	row_delay = intv_row_delay;
}

void intv_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	copybitmap(bitmap,tmpbitmap,0,0,
	           col_delay*2,row_delay*2,
			   &Machine->visible_area,TRANSPARENCY_NONE,0);
}

int intvkbd_vh_start(void)
{
	videoram_size = 0x0800;
	videoram = malloc(videoram_size);

    if (generic_vh_start())
        return 1;

	return intv_vh_start();
    return 0;
}

void intvkbd_vh_stop(void)
{
	intv_vh_stop();
	free(videoram);
	generic_vh_stop();
}

/* very rudimentary support for the tms9927 character generator IC */

static UINT8 tms9927_cursor_col;
static UINT8 tms9927_cursor_row;
static UINT8 tms9927_last_row;
/* initialized to non-zero, because we divide by it */
static UINT8 tms9927_num_rows = 25;

READ_HANDLER( intvkbd_tms9927_r )
{
	UINT8 rv;
	switch (offset)
	{
		case 8:
			rv = tms9927_cursor_row;
			break;
		case 9:
			/* note: this is 1-based */
			rv = tms9927_cursor_col;
			break;
		case 11:
			tms9927_last_row = (tms9927_last_row + 1) % tms9927_num_rows;
			rv = tms9927_last_row;
			break;
		default:
			rv = 0;
	}
	return rv;
}

WRITE_HANDLER( intvkbd_tms9927_w )
{
	switch (offset)
	{
		case 3:
			tms9927_num_rows = (data & 0x3f) + 1;
			break;
		case 6:
			tms9927_last_row = data;
			break;
		case 11:
			tms9927_last_row = (tms9927_last_row + 1) % tms9927_num_rows;
			break;
		case 12:
			/* note: this is 1-based */
			tms9927_cursor_col = data;
			break;
		case 13:
			tms9927_cursor_row = data;
			break;
	}
}

void intvkbd_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int x,y,offs;
	int current_row;
//	char c;

	/* Draw the underlying INTV screen first */
	intv_vh_screenrefresh(bitmap, full_refresh);

	/* if the intvkbd text is not blanked, overlay it */
	if (!intvkbd_text_blanked)
	{
		current_row = (tms9927_last_row + 1) % tms9927_num_rows;
		for(y=0;y<24;y++)
		{
			for(x=0;x<40;x++)
			{
				offs = current_row*64+x;
				drawgfx(bitmap,Machine->gfx[2],
					videoram[offs],
					7, /* white */
					0,0,
					x*8,y*8,
					&Machine->visible_area, TRANSPARENCY_PEN, 0);
			}
			if (current_row == tms9927_cursor_row)
			{
				/* draw the cursor as a solid white block */
				/* (should use a filled rect here!) */
				drawgfx(bitmap,Machine->gfx[2],
					191, /* a block */
					7,   /* white   */
					0,0,
					(tms9927_cursor_col-1)*8,y*8,
					&Machine->visible_area, TRANSPARENCY_PEN, 0);
			}
			current_row = (current_row + 1) % tms9927_num_rows;
		}
	}

#if 0
	// debugging
	c = tape_motor_mode_desc[tape_motor_mode][0];
	drawgfx(bitmap,Machine->gfx[2],
		c,
		1,
		0,0,
		0*8,0*8,
		&Machine->visible_area, TRANSPARENCY_PEN, 0);
	for(y=0;y<5;y++)
	{
		drawgfx(bitmap,Machine->gfx[2],
			tape_unknown_write[y]+'0',
			1,
			0,0,
			0*8,(y+2)*8,
			&Machine->visible_area, TRANSPARENCY_PEN, 0);
	}
	drawgfx(bitmap,Machine->gfx[2],
			tape_unknown_write[5]+'0',
			1,
			0,0,
			0*8,8*8,
			&Machine->visible_area, TRANSPARENCY_PEN, 0);
	drawgfx(bitmap,Machine->gfx[2],
			tape_interrupts_enabled+'0',
			1,
			0,0,
			0*8,10*8,
			&Machine->visible_area, TRANSPARENCY_PEN, 0);
#endif
}
