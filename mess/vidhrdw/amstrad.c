/***************************************************************************

  amstrad.c.c

  Functions to emulate the video hardware of the Amstrad CPC.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/amstrad.h"
#include "mess/vidhrdw/hd6845s.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

static unsigned long Mode0Lookup[256];
static unsigned long Mode1Lookup[256];
static struct osd_bitmap *amstrad_bitmap;
static int amstrad_scanline = 0;

struct GfxElement amstrad_gfx_element;

int amstrad_render_x, amstrad_render_y;

int amstrad_vh_start(void)
{

	int i;

	amstrad_bitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	if (amstrad_bitmap==0)
		return 1;

	for (i=0; i<256; i++)
	{
		int pen;

		pen = (
			((i & (1<<7))>>(7-0)) |
			((i & (1<<3))>>(3-1)) |
			((i & (1<<5))>>(5-2)) |
                        ((i & (1<<1))<<2)
			);

		Mode0Lookup[i] = pen;

		pen = (
			( ( (i & (1<<7)) >>7) <<0) |
		        ( ( (i & (1<<3)) >>3) <<1)
			);

		Mode1Lookup[i] = pen;

	}

	return 0;
}

void    amstrad_vh_stop(void)
{
	if (amstrad_bitmap!=NULL)
		osd_free_bitmap(amstrad_bitmap);

}

extern unsigned char *Amstrad_Memory;
extern short AmstradCPC_PenColours[18];
extern unsigned char AmstradCPC_GA_RomConfiguration;

static void amstrad_vh_decodebyte(struct osd_bitmap *bitmap, unsigned char byte1, unsigned char
byte2)
{
        struct rectangle r;

        r.min_x = amstrad_render_x;
        r.max_x = amstrad_render_x+16;
        r.min_y = amstrad_render_y;
        r.max_y = amstrad_render_y;

        // depending on the mode!
	switch (AmstradCPC_GA_RomConfiguration & 0x03)
	{
                // mode 0 - low resolution - 16 colours
		case 0:
		{
			//int i;
			int cpcpen,messpen;
			unsigned char data;

			data = byte1;

			{
				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];

                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
			}

			data = byte2;

			{
				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

                	}
		}
		break;

                // mode 1 - medium resolution - 4 colours
		case 1:
		{
                        int i;
                        int cpcpen;
                        int messpen;
                        unsigned char data;

                        data = byte1;

                        for (i=0; i<4; i++)
                        {
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

				data = data<<1;
			}

                        data = byte2;

                        for (i=0; i<4; i++)
			{
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;
                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

				data = data<<1;
			}

		}
		break;

		case 2:
		{
			int i;
			unsigned long Data = (byte1<<8) | byte2;
			int cpcpen,messpen;

			for (i=0; i<16; i++)
			{
				cpcpen = (Data>>15) & 0x01;
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];

                                plot_pixel(bitmap, amstrad_render_x, amstrad_render_y, messpen);
                                amstrad_render_x++;

				Data = Data<<1;

			}

		}
		break;
	}


}


/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void amstrad_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        int i;
        int lc;

        lc = hd6845s_getreg(HD6845S_V_SYNC_POS);
        hd6845s_index_w(HD6845S_RA);
        hd6845s_register_w(0);
        hd6845s_index_w(HD6845S_LC);
        hd6845s_register_w(lc);
        amstrad_scanline = 0;
        for (i=0; i<272;  i++)  //312; i++)
        {
            amstrad_update_scanline();
        }
    copybitmap(bitmap,amstrad_bitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

}

/* maps from hsync len programmed to HD6845S and actual
HSYNC len output to monitor. Timings in 1us cycles */
static int hsync_output_remap[]=
{
	0, 0, 0,3,4,5,6, 6,6,6,6,6,6,6,6,6
};

static int left_margin, right_margin, graphics_clocks;

/* calculate left and right margin widths */
void	amstrad_calculate_left_and_right_margins(void)
{
	int htot;
	int hsyncpos;
        int hsynclen;
        int hdisp;
        int hsync_end;

	/* get horizontal total */
        htot = hd6845s_getreg(HD6845S_H_TOT);
	/* get horizontal sync position */
        hsyncpos = hd6845s_getreg(HD6845S_H_SYNC_POS);

	/* get hsync output to monitor */
	hsynclen = hd6845s_getreg(HD6845S_SYNC) & 0x0f;
	hsynclen = hsync_output_remap[hsynclen];

	/* calculate hsync end */
	hsync_end = hsyncpos + hsynclen + HD6845S_MONITOR_HSYNC_LEN;

	if ((hsync_end)>(htot+1))
	{
		/* no left border is visible */
		left_margin = 0;
	}
	else
	{
		/* part of left border is visible */
		left_margin = (htot+1) - hsync_end;

	}

	/* get horizontal displayed */
        hdisp = hd6845s_getreg(HD6845S_H_DISP);

	if ((hdisp+left_margin)>50)
	{
		/* no right border is visible */
		right_margin = 0;
	}
	else
	{
		/* part of right border is visible */
		right_margin = 50 - hdisp - left_margin;
	}

        graphics_clocks = 50 - right_margin - left_margin;

        //fprintf(errorlog, "%d %d %d\r\n",left_margin, right_margin, graphics_clocks);
        //fprintf(errorlog, "R: %d %d %d %d\r\n",htot, hsyncpos, hsynclen, hdisp);
}

int lc1,rc1;
                #if 0
// 35 is top of screen.
// 38 chars in vtot.
// 30 is normal
// 4 lines == 36 blanking lines
void    amstrad_calculate_top_and_bottom_margins(void)
{
        int vtot;
        int vsyncpos;
        int mr;

        vtot = hd6845s_getreg(HD6845S_V_TOT);
        vsyncpos = hd6845s_getreg(HD6845S_V_SYNC_POS);
        mr = hd6845s_getreg(HD6845S_MAX_RASTER);

        vsync_end = (vsyncpos*mr) + HD6845S_MONITOR_VSYNC_LEN;

        if ((vsync_end)>(vtot+1))
	{
                int lines_over;

                lines_over = vsync_end - vtot;


                lc1 = vsync_end - v
		/* no left border is visible */
		left_margin = 0;
	}
	else
	{
		/* part of left border is visible */
		left_margin = (htot+1) - hsync_end;

	}
                     #endif



void amstrad_render_graphics(struct osd_bitmap *bitmap)        //unsigned char *bm)
{
        int i;
        int ma;
        int Addr, byte0, byte1;
        //unsigned char *bm;
        int r;

        ma = hd6845s_getreg(HD6845S_MA);
        r = hd6845s_getreg(HD6845S_RA);

	/* setup render ptr for this line */
        //bm = amstrad_bitmap->line[amstrad_scanline] + (x<<4);

        for (i=0; i<graphics_clocks; i++)
        {
				// calc mem addr to fetch data from
				//based on ma, and ra
				Addr = (((ma>>(4+8)) & 0x03)<<14) |
					((r & 0x07)<<11) |
					((ma & 0x03ff)<<1);

                // get data from memory
                byte0 = Amstrad_Memory[Addr];
                byte1 = Amstrad_Memory[Addr+1];

                // decode byte 0 and 1 depending on mode
                amstrad_vh_decodebyte(bitmap, byte0,byte1);

				// update ma
				ma++;



        }
}


void amstrad_render_scanline(void)
{
	unsigned int hd6845s_state;
        int border_colour;

	/* setup render ptr for this line */

        amstrad_render_y = amstrad_scanline;

	hd6845s_state = hd6845s_getreg(HD6845S_STATE);

        border_colour = Machine->pens[AmstradCPC_PenColours[16]];

        if (!(hd6845s_state & HD6845S_VDISP))
       {
               /* inhibit display */
                /* On Amstrad, border colour is displayed */
                int i;

                for (i=0;i<Machine->drv->screen_width; i++)
                {
                        plot_pixel(amstrad_bitmap, i, amstrad_render_y, border_colour);
                }
        }
        else
	{
                int x;

		amstrad_calculate_left_and_right_margins();

                x = 0;

		if (left_margin!=0)
                {
                        int i;

                        for (i=0;i<(left_margin<<4); i++)
                        {
                                plot_pixel(amstrad_bitmap, i, amstrad_render_y, border_colour);
                        }

                        x+=left_margin<<4;
                }

		if (graphics_clocks!=0)
		{

                        amstrad_render_x = x;

                        amstrad_render_graphics(amstrad_bitmap);

                        x+=(graphics_clocks<<4);
                }


		if (right_margin!=0)
		{
                        int i;

                        for (i=0;i<(right_margin<<4); i++)
                        {
                                plot_pixel(amstrad_bitmap, i+x, amstrad_render_y, border_colour);
                        }
		}

	}
}


void amstrad_update_scanline(void)
{
  //
    //    if (amstrad_scanline<272)
      //  {
		amstrad_render_scanline();
        amstrad_scanline++;
      //  }

        hd6845s_update_clocks(64);

     //   if (amstrad_scanline==312)
       // {
         //       amstrad_scanline=0;
        //}

}




/*
	unsigned char *bm;

	int vdisp = hd6845s_getreg(6);
	int hdisp = hd6845s_getreg(1);
	int maxras = hd6845s_getreg(9);

	int scr_addr_hi = hd6845s_getreg(12);
	int scr_addr_lo = hd6845s_getreg(13);

	int v,r,h;

	int ma_store;
	int ma;
	int l;

	l = 8;

	// initial ma
	ma = ((scr_addr_hi & 0x0ff)<<8) | (scr_addr_lo & 0x0ff);

	for (v=0; v<vdisp; v++)
	{
		ma_store = ma;

		for (r=0; r<(maxras+1); r++)
		{
			// reload ma from ma_store
			ma = ma_store;

			// setup render ptr for this line
			bm = bitmap->line[l];

			// render visible part of display
			for (h=0; h<hdisp; h++)
			{
				unsigned char byte0, byte1;
				unsigned long Addr;

				// calc mem addr to fetch data from
				//based on ma, and ra
				Addr = (((ma>>(4+8)) & 0x03)<<14) |
					((r & 0x07)<<11) |
					((ma & 0x03ff)<<1);

				// get data from memory
				byte0 = Amstrad_Memory[Addr];
				byte1 = Amstrad_Memory[Addr+1];

				// decode byte 0 and 1 depending on mode
				amstrad_vh_decodebyte(byte0,byte1, bm);
				bm+=16;

				// update ma
				ma++;
			}

			l++;
		}
	}

}
*/
