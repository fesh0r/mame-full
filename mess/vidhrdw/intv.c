#include "driver.h"
#include "includes/intv.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/stic.h"

int intv_vh_start(void)
{
	int i;
	for(i=0;i<64;i++)
		intv_gramdirty[i] = 1;

    return 0;
}

void intv_vh_stop(void)
{
}

void intv_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* First, draw the background */
	int offs = 0;
	int value = 0;
	int row,col;
	int i;
	int code,fgcolor,bgcolor = 0;
	if (intv_stic_handshake != 0)
	{
		intv_stic_handshake = 0;
	if (intv_color_stack_mode)
	{
	for(row = 0; row < 12; row++)
	{
		for(col = 0; col < 20; col++)
		{
			value = intv_ram16[offs];
			if (value&0x2000)
			{
			    intv_color_stack_offset += 1;
			    intv_color_stack_offset %= 4;
			}

			if (value&0x1000)
				fgcolor = (code&0x7) + 8;
			else
				fgcolor = code&0x7;

			fgcolor = value&0x7;
			bgcolor = intv_color_stack[intv_color_stack_offset];
			code = (value>>3)&0xff;

			if (value&0x0800)
			{
				code %= 64;  /* keep from going outside the array */
				if (intv_gramdirty[code] == 1)
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
					0,TRANSPARENCY_NONE,0);
			}
			else
			{
				/* Draw GROM char */
				drawgfx(bitmap,Machine->gfx[0],
					code,
			        bgcolor*16+fgcolor,
					0,0,col*16,row*16,
					0,TRANSPARENCY_NONE,0);
			}
			offs++;

#if 0
			pic_offset += (((code>>3)&0xff)*8);

			for(i=0;i<8;i++)
			{
				for(j=0;j<8;j++)
				{
					if ((cp1600_readmem16(pic_offset)<<j) & 0x80)
					{
						plot_pixel(bitmap, col*16+j*2, row*16+i*2, Machine->pens[fgcolor]);
						plot_pixel(bitmap, col*16+j*2+1, row*16+i*2, Machine->pens[fgcolor]);
						plot_pixel(bitmap, col*16+j*2, row*16+i*2+1, Machine->pens[fgcolor]);
						plot_pixel(bitmap, col*16+j*2+1, row*16+i*2+1, Machine->pens[fgcolor]);
					}
					else
					{
						plot_pixel(bitmap, col*16+j*2, row*16+i*2, Machine->pens[intv_color_stack[intv_color_stack_offset]]);
						plot_pixel(bitmap, col*16+j*2+1, row*16+i*2, Machine->pens[intv_color_stack[intv_color_stack_offset]]);
						plot_pixel(bitmap, col*16+j*2, row*16+i*2+1, Machine->pens[intv_color_stack[intv_color_stack_offset]]);
						plot_pixel(bitmap, col*16+j*2+1, row*16+i*2+1, Machine->pens[intv_color_stack[intv_color_stack_offset]]);
					}
					if (code&0x0800) // blue underline for GRAM
					{
						//plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, Machine->pens[1]);
						//plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, Machine->pens[1]);
					}
					else // red underline for GRAM
					{
						//plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, Machine->pens[2]);
						//plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, Machine->pens[2]);
					}
				}
				pic_offset++;
			}
#endif
		}
	}
	}
	for(i=0;i<8;i++)
	{
		struct intv_sprite_type *s = &intv_sprite[i];
		if (s->visible)
		{
			code = s->card;
		if (!s->grom)
		{
			code %= 64;  /* keep from going outside the array */
			if (s->yres == 1)
			{
				if (intv_gramdirty[code] == 1)
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
					0,0,s->xpos*2,s->ypos*2-16,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					0x8000*s->xsize, 0x8000*s->ysize);
			}
			else
			{
				if ((intv_gramdirty[code] == 1) || (intv_gramdirty[code+1] == 1))
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
					0,0,s->xpos*2,s->ypos*2-16,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					0x8000*s->xsize, 0x8000*s->ysize);
				drawgfxzoom(bitmap,Machine->gfx[1],
					code+1,
					s->color,
					0,0,s->xpos*2,s->ypos*2-16+s->ysize*8,
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
				0,0,s->xpos*2,s->ypos*2-16,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				0x8000*s->xsize, 0x8000*s->ysize);
			}
			else
			{
			drawgfxzoom(bitmap,Machine->gfx[0],
				code,
				s->color,
				0,0,s->xpos*2,s->ypos*2-16,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				0x8000*s->xsize, 0x8000*s->ysize);
			drawgfxzoom(bitmap,Machine->gfx[0],
				code+1,
				s->color,
				0,0,s->xpos*2,s->ypos*2-16+s->ysize*8,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				0x8000*s->xsize, 0x8000*s->ysize);
			}
		}
		}

		//plot_pixel(bitmap,sprite[s].xpos*2,sprite[s].ypos*2, Machine->pens[1]);
		//plot_pixel(bitmap,sprite[s].hpos*2,sprite[s].vpos*2+1, Machine->pens[1]);
		//plot_pixel(bitmap,sprite[s].hpos*2+1,sprite[s].vpos*2, Machine->pens[1]);
		//plot_pixel(bitmap,sprite[s].xpos*2+sprite[s].xsize*8,
		//			sprite[s].ypos*2+sprite[s].ysize*8,
		//			Machine->pens[2]);
	}
	}
	else
	//fillbitmap(bitmap,Machine->pens[2],&Machine->visible_area);
	fillbitmap(bitmap,Machine->pens[intv_border_color],&Machine->visible_area);
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

void intvkbd_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x,y,offs;
//	char c;

	intv_vh_screenrefresh(bitmap, full_refresh);

	if (!intvkbd_text_blanked)
	{
		for(y=0;y<24;y++)
		{
			for(x=0;x<40;x++)
			{
				offs = y*64+x;
				drawgfx(bitmap,Machine->gfx[2],
					videoram[offs],
					7,
					0,0,
					x*8,y*8,
					&Machine->visible_area, TRANSPARENCY_PEN, 0);
			}
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
