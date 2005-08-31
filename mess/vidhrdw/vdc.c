#include <assert.h>

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vdc.h"

/* Our VDP context */

VDC vdc;

/* Function prototypes */

void pce_refresh_line(int line);
void pce_refresh_sprites(int line);


VIDEO_START( pce )
{
	logerror("*** pce_vh_start\n");

	/* clear context */
	memset(&vdc, 0, sizeof(vdc));

	/* allocate VRAM */
	vdc.vram = auto_malloc(0x10000);
	if(!vdc.vram)
		return 1;
	memset(vdc.vram, 0, 0x10000);

	/* create display bitmap */
	vdc.bmp = auto_bitmap_alloc(360, 256);
	if(!vdc.bmp)
		return 1;

	return 0;
}


VIDEO_UPDATE( pce )
{
    /* only refresh the visible portion of the display */
    rectangle pce_visible_area;

    int center_x = ((360/2) - (vdc.physical_width/2));
    int center_y = ((256/2) - (vdc.physical_height/2));

    pce_visible_area.min_x = center_x;
    pce_visible_area.min_y = center_y;
    pce_visible_area.max_x = (center_x + vdc.physical_width) - 1;
    pce_visible_area.max_y = (center_x + vdc.physical_height) - 1;

    /* copy our rendering buffer to the display */
    copybitmap (bitmap,vdc.bmp,0,0,0,0,&pce_visible_area,TRANSPARENCY_NONE,0);
}


WRITE8_HANDLER ( vdc_w )
{
    switch(offset)
    {
        case 0x00: /* VDC register select */
             vdc.vdc_register = (data & 0x1F);
             break;

        case 0x02: /* VDC data (LSB) */
             vdc.vdc_data[vdc.vdc_register].b.l = data;
             switch(vdc.vdc_register)
             {
                case VxR: /* LSB of data to write to VRAM */
                     vdc.vdc_latch = data;
                     break;

                case HDR:
                     vdc.physical_width = ((data & 0x003F) + 1) << 3;
                     break;

                case VDW:
                     vdc.physical_height &= 0xFF00;
                     vdc.physical_height |= (data & 0xFF);
                     vdc.physical_height &= 0x01FF;
                     break;

                case LENR:
//                   logerror("LENR LSB = %02X\n", data);
                     break;
                case SOUR:
//                   logerror("SOUR LSB = %02X\n", data);
                     break;
                case DESR:
//                   logerror("DESR LSB = %02X\n", data);
                     break;
             }
             break;

        case 0x03: /* VDC data (MSB) */
             vdc.vdc_data[vdc.vdc_register].b.h = data;
             switch(vdc.vdc_register)
             {
                case VxR: /* MSB of data to write to VRAM */
                     vdc.vdc_data[MAWR].w &= 0x7FFF;
                     vdc.vram[vdc.vdc_data[MAWR].w*2+0] = vdc.vdc_latch;
                     vdc.vram[vdc.vdc_data[MAWR].w*2+1] = data;
                     vdc.vdc_data[MAWR].w += vdc.inc;
                     vdc.vdc_data[MAWR].w &= 0x7FFF;
                     vdc.vdc_latch = 0;
                     break;

                case CR:
                     {
                        static unsigned char inctab[] = {1, 32, 64, 128};
                        vdc.inc = inctab[(data >> 3) & 3];
                     }
                     break;

                case VDW:
                     vdc.physical_height &= 0x00FF;
                     vdc.physical_height |= (data << 8);
                     vdc.physical_height &= 0x01FF;
                     break;

                case DVSSR:
                     /* Force VRAM <> SATB DMA for this frame */
                     vdc.dvssr_write = 1;
                     break;

                case LENR:
//                   logerror("LENR MSB = %02X\n", data);
                     break;
                case SOUR:
//                   logerror("SOUR MSB = %02X\n", data);
                     break;
                case DESR:
//                   logerror("DESR MSB = %02X\n", data);
                     break;
             }
             break;
    }
}


 READ8_HANDLER ( vdc_r )
{
    int temp = 0;
    switch(offset)
    {
        case 0x00:
             temp = vdc.status;
             vdc.status &= ~(VDC_VD | VDC_RR | VDC_DS);
             break;

        case 0x02:
             switch(vdc.vdc_register)
             {
                case VxR:
                     temp = vdc.vram[vdc.vdc_data[MARR].w*2+0];
                     break;
             }
             break;

        case 0x03:
             switch(vdc.vdc_register)
             {
                case VxR:
                     temp = vdc.vram[vdc.vdc_data[MARR].w*2+1];
                     break;
             }
             break;
    }
    return (temp);
}


 READ8_HANDLER ( vce_r )
{
    int temp = 0;
    switch(offset & 7)
    {
        case 0x04: /* color table data (LSB) */
             temp = vdc.vce_data[vdc.vce_address.w].b.l;
             break;

        case 0x05: /* color table data (MSB) */
             temp = vdc.vce_data[vdc.vce_address.w].b.h;
             vdc.vce_address.w = (vdc.vce_address.w + 1) & 0x01FF;
             break;
    }
    return (temp);
}


WRITE8_HANDLER ( vce_w )
{
    switch(offset & 7)
    {
        case 0x00: /* control reg. */
             break;

        case 0x02: /* color table address (LSB) */
             vdc.vce_address.b.l = data;
             break;

        case 0x03: /* color table address (MSB) */
             vdc.vce_address.b.h = data;
             break;

        case 0x04: /* color table data (LSB) */
             vdc.vce_data[vdc.vce_address.w].b.l = data;
             break;

        case 0x05: /* color table data (MSB) */
             vdc.vce_data[vdc.vce_address.w].b.h = data;

             /* set color */
             {
                int /*c,*/ i, r, g, b;
                /*pair f;*/

                /* mirror entry 0 of palette 0 */
                for(i=1;i<32;i++)
                {
                    vdc.vce_data[i << 4].w = vdc.vce_data[0].w;
                }

                i = vdc.vce_address.w;
                r = ((vdc.vce_data[i].w >> 3) & 7) << 5;
                g = ((vdc.vce_data[i].w >> 6) & 7) << 5;
                b = ((vdc.vce_data[i].w >> 0) & 7) << 5;
                palette_set_color(i, r, g, b);
             }

             /* bump internal address */
             vdc.vce_address.w = (vdc.vce_address.w + 1) & 0x01FF;
             break;
    }
}


void pce_refresh_line(int line)
{
    static int width_table[4] = {5, 6, 7, 7};

    int center_x = ((360/2) - (vdc.physical_width/2));
    int center_y = ((256/2) - (vdc.physical_height/2));

    int scroll_y = (vdc.vdc_data[BYR].w & 0x01FF);
    int scroll_x = (vdc.vdc_data[BXR].w & 0x03FF);
    int nt_index;

    /* is virtual map 32 or 64 characters tall ? (256 or 512 pixels) */
    int v_line = (line + scroll_y) & (vdc.vdc_data[MWR].w & 0x0040 ? 0x1FF : 0x0FF);

    /* row within character */
    int v_row = (v_line & 7);

    /* row of characters in BAT */
    int nt_row = (v_line >> 3);

    /* virtual X size (# bits to shift) */
    int v_width =        width_table[(vdc.vdc_data[MWR].w >> 4) & 3];

    /* our line buffer */
    UINT8 line_buffer[(0x40 << 3) + 8];
#ifdef MAME_DEBUG
	int line_buffer_size;
#endif

    /* pointer to the name table (Background Attribute Table) in VRAM */
    UINT16 *bat = (UINT16 *)&(vdc.vram[nt_row << (v_width+1)]);

    int b0, b1, b2, b3;
    int i0, i1, i2, i3;
    int cell_pattern_index;
    int cell_palette;
    int x, c, i;

#ifdef MAME_DEBUG
	line_buffer_size = vdc.physical_width + 8;
	assert(line_buffer_size <= sizeof(line_buffer));
#endif

    /* character blanking bit */
    if(!(vdc.vdc_data[CR].w & CR_BB))
    {
        memset(line_buffer, Machine->pens[0], vdc.physical_width);
    }
	else
	{
		for(i=0;i<(vdc.physical_width >> 3)+1;i++)
		{
			nt_index = (i + (scroll_x >> 3)) & ((2 << (v_width-1))-1);

			/* get name table data: */

			/* palette # = index from 0-15 */
			cell_palette = (bat[nt_index] >> 12) & 0x0F;

			/* This is the 'character number', from 0-0x0FFF         */
			/* then it is shifted left 4 bits to form a VRAM address */
			/* and one more bit to convert VRAM word offset to a     */
			/* byte-offset within the VRAM space                     */
			cell_pattern_index = (bat[nt_index] & 0x0FFF) << 5;

			b0 = vdc.vram[(cell_pattern_index) + (v_row << 1) + 0x00];
			b1 = vdc.vram[(cell_pattern_index) + (v_row << 1) + 0x01];
			b2 = vdc.vram[(cell_pattern_index) + (v_row << 1) + 0x10];
			b3 = vdc.vram[(cell_pattern_index) + (v_row << 1) + 0x11];

			for(x=0;x<8;x++)
			{
				i0 = (b0 >> (7-x)) & 1;
				i1 = (b1 >> (7-x)) & 1;
				i2 = (b2 >> (7-x)) & 1;
				i3 = (b3 >> (7-x)) & 1;
				c = (cell_palette << 4 | i3 << 3 | i2 << 2 | i1 << 1 | i0);

#ifdef MAME_DEBUG
				assert((i<<3)+x < (line_buffer_size / sizeof(line_buffer[0])));
#endif
				line_buffer[(i<<3)+x] = Machine->pens[c];
			}
		}

		if(vdc.vdc_data[CR].w & CR_SB)
		{
			pce_refresh_sprites(line);
		}
	}

	draw_scanline8(vdc.bmp, (center_x-8)+(8-(scroll_x & 7)), center_y+line, vdc.physical_width, line_buffer, Machine->pens, -1);
}



static void conv_obj(int i, int l, int hf, int vf, char *buf)
{
    int b0, b1, b2, b3, i0, i1, i2, i3, x;
    int xi;
    UINT16 *ptr = (UINT16 *)&(vdc.vram[0]);

    l &= 0x0F;
    if(vf) l = (15 - l);

    b0 = ptr[(i << 5) + l + 0x00];
    b1 = ptr[(i << 5) + l + 0x10];
    b2 = ptr[(i << 5) + l + 0x20];
    b3 = ptr[(i << 5) + l + 0x30];

    for(x=0;x<16;x++)
    {
        if(hf) xi = x; else xi = (15 - x);
        i0 = (b0 >> xi) & 1;
        i1 = (b1 >> xi) & 1;
        i2 = (b2 >> xi) & 1;
        i3 = (b3 >> xi) & 1;
        buf[x] = (i3 << 3 | i2 << 2 | i1 << 1 | i0);
    }
}

void pce_refresh_sprites(int line)
{
    static int cgx_table[] = {16, 32};
    static int cgy_table[] = {16, 32, 64, 64};

    int center_x = ((360/2) - (vdc.physical_width/2));
    int center_y = ((256/2) - (vdc.physical_height/2));

    int obj_x, obj_y, obj_i, obj_a;
    int obj_w, obj_h, obj_l, cgypos;
    int hf, vf;
    int cgx, cgy, palette;
    int i, x, c /*, b*/;
    char buf[16];

    UINT8 *line_buffer;
	
	line_buffer = malloc(vdc.physical_width);
	if (!line_buffer)
		return;

    if ((vdc.vdc_data[DVSSR].w & 0x8000) == 0)
	{
		for(i=63; i>=-1; i--)
		{
			obj_y = (vdc.sprite_ram[(i<<2)+0] & 0x03FF) - 64;
			obj_x = (vdc.sprite_ram[(i<<2)+1] & 0x03FF) - 32;

			if ((obj_y == -64) || (obj_y > line)) continue;
			if ((obj_x == -32) || (obj_x > vdc.physical_width)) continue;

			obj_a = (vdc.sprite_ram[(i<<2)+3]);

	//      if ((obj_a & 0x80) == 0) continue;

			cgx   = (obj_a >> 8) & 1;   /* sprite width */
			cgy   = (obj_a >> 12) & 3;  /* sprite height */
			hf    = (obj_a >> 11) & 1;  /* horizontal flip */
			vf    = (obj_a >> 15) & 1;  /* vertical flip */
			palette = (obj_a & 0x000F);

			obj_i = (vdc.sprite_ram[(i<<2)+2] & 0x07FE);

			obj_w = cgx_table[cgx];
			obj_h = cgy_table[cgy];
			obj_l = (line - obj_y);

			if (obj_l < obj_h)
			{
				cgypos = (obj_l >> 4);
				if(vf) cgypos = ((obj_h - 1) >> 4) - cgypos;

				if(cgx == 0)
				{
					conv_obj(obj_i + (cgypos << 2), obj_l, hf, vf, buf);
					for(x=0;x<16;x++)
					{
						c = buf[x];
						if(c) line_buffer[obj_x + x] = Machine->pens[0x100 + (palette << 4) + c];
					}
				}
				else
				{
					conv_obj(obj_i + (cgypos << 2) + (hf ? 2 : 0), obj_l, hf, vf, buf);
					for(x=0;x<16;x++)
					{
						c = buf[x];
						if(c) line_buffer[obj_x + x] = Machine->pens[0x100 + (palette << 4) + c];
					}

					conv_obj(obj_i + (cgypos << 2) + (hf ? 0 : 2), obj_l, hf, vf, buf);
					for(x=0;x<16;x++)
					{
						c = buf[x];
						if(c) line_buffer[obj_x + 0x10 + x] = Machine->pens[0x100 + (palette << 4) + c];
					}
				}
			}
		}
	}

	draw_scanline8(vdc.bmp, center_x, center_y+line, vdc.physical_width, line_buffer, Machine->pens, -1);
	free(line_buffer);
}
