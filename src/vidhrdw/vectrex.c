#include <stdio.h>
#include "driver.h"
#include "vidhrdw/vector.h"
#include "machine/vect_via.h"
#include "vectrex.h"

static int width; int height;
static int xcenter; int ycenter;
static int xmin; int xmax;
static int ymin; int ymax;

static int colorram[16]; /* colorram entries */
static int vectrex_x_int, vectrex_y_int; /* X and Y integrators */
static int last_point_x, last_point_y, last_point_brightness, last_point=0; /* used for some point optimisations */

int T2_running;

static struct osd_bitmap *tmpbitmap;

static void shade_fill (unsigned char *palette, int rgb, int start_index, int end_index, int start_inten, int end_inten)
{
	int i, inten, index_range, inten_range;
	
	index_range = end_index-start_index; 
	inten_range = end_inten-start_inten;
	for (i = start_index; i <= end_index; i++)
	{
		inten = start_inten + (inten_range) * (i-start_index) / (index_range); 
		palette[3*i  ] = (rgb & RED  )? inten : start_inten;
		palette[3*i+1] = (rgb & GREEN)? inten : start_inten;
		palette[3*i+2] = (rgb & BLUE )? inten : start_inten;
	}
}


void vectrex_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	
	/* initialize the first 8 colors with the basic colors */
	/* Only these are selected by writes to the colorram. */
	for (i = 8; i < 16; i++)
	{
		palette[3*i  ] = (i & RED  ) ? 0xff : 0;
		palette[3*i+1] = (i & GREEN) ? 0xff : 0;
		palette[3*i+2] = (i & BLUE ) ? 0xff : 0;
	}
	
	/* initialize the colorram */
	for (i = 0; i < 16; i++)
		colorram[i] = i & 0x07; 
	
	/* fill the rest of the 256 color entries depending on the game */
	switch (color_prom[0])
	{
		/* Black and White vector colors (Asteroids,Omega Race) .ac JAN2498 */
	case  VEC_PAL_BW:
		shade_fill (palette, RED|GREEN|BLUE, 16, 128+16, 0, 255);	
		colorram[1] = 7; /* BW games use only color 1 (== white) */
		break;
		
	}
}

void stop_vectrex(void)
{
	vector_clear_list();
	vector_vh_stop();
}

static void vectrex_refresh_backup (int param)
{
	if (T2_running)
		T2_running--; /* wait some time to make sure T2 has really stopped
			       * for a longer time. */
	else
		vectrex_screen_update();
}

int start_vectrex (void)
{
	xmin=Machine->drv->visible_area.min_x;
	ymin=Machine->drv->visible_area.min_y;
	xmax=Machine->drv->visible_area.max_x;
	ymax=Machine->drv->visible_area.max_y;
	width=xmax-xmin;
	height=ymax-ymin;

	/* HJB 8/09/98 changed while looking for a reason for the offset :-/ */
	/* xcenter=((xmax+xmin)/2) << VEC_SHIFT; */
	/* ycenter=((ymax+ymin)/2) << VEC_SHIFT; */
    xcenter=((xmax-xmin)/2) << VEC_SHIFT;
	ycenter=((ymax-ymin)/2) << VEC_SHIFT;
  
	vector_set_shift (VEC_SHIFT);
  
	if (!(tmpbitmap = osd_create_bitmap(width,height)))
		return 1;

	if (vector_vh_start())
	{
		free (tmpbitmap);
		return 1;
	}
	
	ResetVia();

	/* Init. the refresh timer to the BIOS default refresh rate.
	 * In principle we could use T2 for this and actually this works
	 * in many cases. However, sometimes the games just draw vectors
	 * without rearming T2 once it's expired - so the screen doesn't
	 * get redrawn. This is why we have an additional pulsing timer
	 * which does the refresh if T2 isn't running. */

	timer_pulse (CYCLES_TO_TIME(0,30000), 0, vectrex_refresh_backup);

	return 0;
}

void vectrex_screen_update (void)
{
	vector_vh_update(tmpbitmap,1);
	vector_clear_list();
}

void vectrex_vh_update (struct osd_bitmap *bitmap, int full_refresh)
{
	copybitmap(bitmap, tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}	

void vectrex_zero_integrators(int offs)
{
	if (last_point)
		vector_add_point (last_point_x, last_point_y, colorram[1],last_point_brightness);
	last_point = 0;

	vectrex_x_int=xcenter-(offs<<VECTREX_SCALE);    
	vectrex_y_int=ycenter+(offs<<VECTREX_SCALE);
	vector_add_point (vectrex_x_int, vectrex_y_int, colorram[1], 0);
}

void vectrex_add_point_dot(int brightness)
{
	last_point_x = vectrex_x_int;
	last_point_y = vectrex_y_int;
	last_point_brightness = brightness*2;
	last_point = 1;
}

void vectrex_add_point(int scale, int x, int y, int offs, int pattern, int brightness)
{
	int shift=7, res;

	/* The BIOS draws lines as follows: First put a pattern in the VIA SR (this causes a dot). Then
	 * turn on RAMP and let the integrators do their job (this causes a line to be drawn).
	 * Obviously this first dot isn't needed so we optimize it away :) but we have to take care
	 * not to optimize _all_ dots, that's why we do draw the dot if the following line is
	 * black (a move). */
	if (last_point && (!(pattern & 0x80) || !brightness))
		vector_add_point (last_point_x, last_point_y, colorram[1],last_point_brightness);
	last_point = 0;
		
	x -= offs;
	y += offs;

	x <<= VECTREX_SCALE+1;
	y <<= VECTREX_SCALE+1;

	x *= 0.9; /* This gives an aspect ratio which is a little bit more like on the real Vectrex */
	brightness *= 1.5; /* This  will change once I get the overlays working correctly */

	res=scale%2;
	scale /= 2;

	while (shift && scale)
	{
	    
		while (shift && scale) /* Try to be smart and combine consecutive equal bits */ 
		{
			vectrex_x_int += x;
			vectrex_y_int -= y;
			scale--;
			shift--;
			if ((pattern >> (shift+1)) & 0x1)
			{
				if (!((pattern >> shift) & 0x1))
					break;
			}
			else
			{
				if ((pattern >> shift) & 0x1)
					break;
			}
		}
		vector_add_point (vectrex_x_int, vectrex_y_int, colorram[1], brightness * ((pattern >> (shift+1)) & 0x1));
	}
	
	if (scale || res) /* We shifted enough. The rest is a straight line */
	{
		vectrex_x_int += (x*(scale*2+res))>>1;
		vectrex_y_int -= (y*(scale*2+res))>>1;
		
		vector_add_point (vectrex_x_int, vectrex_y_int, colorram[1], brightness * (pattern & 0x1));
	}
}
