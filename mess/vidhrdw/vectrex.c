#include "driver.h"
#include "vidhrdw/vector.h"
#include "machine/6522via.h"

#include "includes/vectrex.h"

#define RAMP_DELAY 6.333e-6
#define BLANK_DELAY 0
#define SH_DELAY 6.333e-6

#define BLACK 0x00
#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE
#define DARKRED 0x08
#define VEC_SHIFT 16
#define INT_PER_CLOCK 900
#define VECTREX_CLOCK 1500000

#define PORTB 0
#define PORTA 1

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

/*********************************************************************
  Local variables
 *********************************************************************/
static WRITE_HANDLER ( v_via_pa_w );
static WRITE_HANDLER( v_via_pb_w );
static void vectrex_screen_update (double time_);
static void vectrex_shift_reg_w (int via_sr);
static WRITE_HANDLER ( v_via_ca2_w );

static struct via6522_interface vectrex_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, v_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ v_via_pa_w, v_via_pb_w, v_via_ca2_w, 0,
	/*irq                      */ v_via_irq,

	vectrex_shift_reg_w, vectrex_screen_update
};

static struct osd_bitmap *tmpbitmap;

static int x_center, y_center, x_max;
static int x_int, y_int; /* X, Y integrators IC LF347*/

/* Analog signals: 0 : X-axis sample and hold IC LF347
 *                 coupled by the MUX CD4052
 *                 1 : Y-axis sample and hold IC LF347
 *                 2 : "0" reference charge capacitor
 *                 3 : Z-axis (brightness signal) sample and hold IC LF347
 *                 4 : MPU sound resistive netowrk
 */
static int analog_sig[5];

static double start_time;
static int old_via_sr = 0;
static int last_point_x, last_point_y, last_point_z, last_point=0;
static double last_point_starttime;
static float z_factor;

static int T2_running; /* This turns zero if VIA timer 2 (the refresh timer) isn't running */
static void *backup_timer = NULL;
static int vectrex_full_refresh;

void (*vector_add_point_function) (int, int, int, int) = vector_add_point;

/*********************************************************************
  Screen updating
 *********************************************************************/
static void vectrex_screen_update (double time_)
{
	if (vectrex_imager_status)
		vectrex_imager_left_eye(time_);

	if (vectrex_refresh_with_T2)
	{
		T2_running = 3;
		vector_vh_screenrefresh(tmpbitmap, vectrex_full_refresh);
		vector_clear_list();
	}
}

static void vectrex_screen_update_backup (int param)
{
	if (T2_running)
		T2_running--; /* wait some time to make sure T2 has really stopped
			       * for a longer time. */
	else
	{
		vector_vh_screenrefresh(tmpbitmap, vectrex_full_refresh);
		vector_clear_list();
	}
}

void vectrex_vh_update (struct osd_bitmap *bitmap, int full_refresh)
{
	vectrex_full_refresh = full_refresh;

	palette_recalc();
	vectrex_configuration();
	copybitmap(bitmap, tmpbitmap,0,0,0,0,0,TRANSPARENCY_NONE,0);
}

/*********************************************************************
  Vector functions
 *********************************************************************/
void vector_add_point_stereo (int x, int y, int color, int intensity)
{
	if (vectrex_imager_status == 1) /* left = 2, right = 1 */
		vector_add_point ((int)(y*M_SQRT1_2), (int)(((x_max-x)*M_SQRT1_2)+y_center), color, intensity);
	else
		vector_add_point ((int)(y*M_SQRT1_2), (int)((x_max-x)*M_SQRT1_2), color, intensity);
}

INLINE void vectrex_zero_integrators(void)
{
	if (last_point)
		vector_add_point_function (last_point_x, last_point_y, vectrex_beam_color,
					   MIN((int)(last_point_z*((timer_get_time()-last_point_starttime)*3E4)),255));
	last_point = 0;

	x_int=x_center-(analog_sig[2]*INT_PER_CLOCK);
	y_int=y_center+(analog_sig[2]*INT_PER_CLOCK);
	vector_add_point_function (x_int, y_int, vectrex_beam_color, 0);
}

INLINE void vectrex_dot(void)
{
	last_point_x = x_int;
	last_point_y = y_int;
	last_point_z = analog_sig[3] > 0? (int)(analog_sig[3] * z_factor): 0;
	last_point_starttime = timer_get_time();
	last_point = 1;
}

INLINE void vectrex_shift_out(int shift, int pattern)
{
	int x = (analog_sig[0] - analog_sig[2]) * INT_PER_CLOCK * 2;
	int y = (analog_sig[1] + analog_sig[2]) * INT_PER_CLOCK * 2;
	int z = analog_sig[3] > 0? (int)(analog_sig[3] * z_factor): 0;

	if (last_point && (!(pattern & 0x80) || !z))
		vector_add_point_function(last_point_x, last_point_y, vectrex_beam_color,
				  MIN( (int)(last_point_z*((timer_get_time()-last_point_starttime)*3E4)),255));
	last_point = 0;

	while (shift)
	{
		while (shift)
		{
			x_int += x;
			y_int -= y;
			shift--;
			if ((pattern >> shift) & 0x1)
			{
				if (!((pattern >> (shift - 1)) & 0x1))
					break;
			}
			else
			{
				if ((pattern >> (shift - 1)) & 0x1)
					break;
			}
		}
		vector_add_point_function(x_int, y_int, vectrex_beam_color,
					  z * ((pattern >> shift) & 0x1));
	}
}

INLINE void vectrex_solid_line(double time_, int pattern)
{
	int z = analog_sig[3] > 0? (int)(analog_sig[3]*z_factor): 0;
    int length = (int)(VECTREX_CLOCK * INT_PER_CLOCK * time_);

	/* The BIOS draws lines as follows: First put a pattern in the VIA SR (this causes a dot). Then
	 * turn on RAMP and let the integrators do their job (this causes a line to be drawn).
	 * Obviously this first dot isn't needed so we optimize it away :) but we have to take care
	 * not to optimize _all_ dots, that's why we do draw the dot if the following line is
	 * black (a move). */
	if (last_point && (!(pattern & 0x80) || !z))
		vector_add_point_function(last_point_x, last_point_y, vectrex_beam_color,
				  MIN((int)(last_point_z*((timer_get_time()-last_point_starttime)*3E4)),255));
	last_point = 0;

	x_int += (int)(length * (analog_sig[0] - analog_sig[2]));
	y_int -= (int)(length * (analog_sig[1] + analog_sig[2]));
	vector_add_point_function(x_int, y_int, vectrex_beam_color, z * (pattern & 0x1));
}

/*********************************************************************
  Color init.
 *********************************************************************/
static void shade_fill (unsigned char *palette, int rgb, int start_index, int end_index, int start_inten, int end_inten)
{
	int i, inten, index_range, inten_range;

	index_range = end_index - start_index;
	inten_range = end_inten - start_inten;
	for (i = start_index; i <= end_index; i++)
	{
		inten = start_inten + (inten_range) * (i-start_index) / (index_range);
		palette[3*i  ] = (rgb & RED  )? inten : 0;
		palette[3*i+1] = (rgb & GREEN)? inten : 0;
		palette[3*i+2] = (rgb & BLUE )? inten : 0;
	}
}

void vectrex_new_palette (unsigned char *palette)
{
	int i, nextfree;
	char overlay_name[1024];
	const struct IODevice *dev = Machine->gamedrv->dev;

	/* initialize the first 8 colors with the basic colors */
	for (i = 0; i < 8; i++)
	{
		palette[3*i+0] = (i & RED  ) ? 0xff : 0;
		palette[3*i+1] = (i & GREEN) ? 0xff : 0;
		palette[3*i+2] = (i & BLUE ) ? 0xff : 0;
	}
	nextfree = 8;

	/* 16 shades of gray for the vector functions */
	shade_fill (palette, WHITE, nextfree, nextfree+15, 0, 255);
	nextfree +=16;

	/* try to load an overlay for game.bin named game.png */
	if (device_filename(dev->type,0))
	{
		sprintf(overlay_name,"%s", device_filename(dev->type,0));
		sprintf(strchr(overlay_name,'.'),".png");
	}
	else
		sprintf(overlay_name,"mine.png"); /* load the minestorm overlay (built in game) */

	artwork_kill(); /* remove existing overlay */
	overlay_load(overlay_name, nextfree, Machine->drv->total_colors-nextfree);

	if ((artwork_overlay != NULL))
	{
		overlay_set_palette (palette, MIN(256, Machine->drv->total_colors) - nextfree);
	}
	else
	{
		/* Dark red for red/blue glasses mode */
		palette[3 * 8 + 0] = 160;
		palette[3 * 8 + 1] = palette[3 * 8 + 2] = 0;

		/* some more white */
		nextfree -=15;
		shade_fill (palette, WHITE, nextfree, nextfree + 31, 0, 255);
		nextfree +=32;

		/* No overlay, so we just put shades of '3D colors' into the palette */
		shade_fill (palette, RED, nextfree, nextfree + 31, 0, 255);
		nextfree += 32;
		shade_fill (palette, GREEN, nextfree, nextfree + 31, 0, 255);
		nextfree += 32;
		shade_fill (palette, BLUE, nextfree, nextfree + 31, 0, 255);
		nextfree += 32;
		shade_fill (palette, RED|GREEN, nextfree, nextfree + 15, 0, 255);
		nextfree += 16;
		shade_fill (palette, RED|BLUE, nextfree, nextfree + 15, 0, 255);
		nextfree += 16;
		shade_fill (palette, GREEN|BLUE, nextfree, nextfree + 15, 0, 255);
	}
}

void vectrex_set_palette (void)
{
	unsigned char *palette;
	int i;

	palette = (unsigned char *)malloc(Machine->drv->total_colors * 3);

	if (palette == 0)
	{
		logerror("Not enough memory!\n");
		return;
	}

	memset (palette, 0, Machine->drv->total_colors * 3);
	vectrex_new_palette (palette);

	i = (Machine->scrbitmap->depth == 8) ? MIN(256,Machine->drv->total_colors)
		: Machine->drv->total_colors;

	while (--i >= 0)
		palette_change_color(i, palette[i*3], palette[i*3+1], palette[i*3+2]);

	palette_recalc();
	artwork_remap(); /* this should be done in the core */

	free (palette);
	return;
}

/*********************************************************************
  Startup and stop
 *********************************************************************/
int vectrex_start (void)
{
	int width, height;

	vectrex_set_palette ();

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		width = Machine->scrbitmap->height;
		height = Machine->scrbitmap->width;
	}
	else
	{
		width = Machine->scrbitmap->width;
		height = Machine->scrbitmap->height;
	}

	if (!(tmpbitmap = bitmap_alloc(width,height)))
		return 1;

	x_center=((Machine->visible_area.max_x
		  -Machine->visible_area.min_x) / 2) << VEC_SHIFT;
	y_center=((Machine->visible_area.max_y
		  -Machine->visible_area.min_y) / 2 - 10) << VEC_SHIFT;
	x_max = Machine->visible_area.max_x << VEC_SHIFT;

	vector_set_shift (VEC_SHIFT);

	/* Init. the refresh timer to the BIOS default refresh rate.
	 * In principle we could use T2 for this and actually this works
	 * in many cases. However, sometimes the games just draw vectors
	 * without rearming T2 once it's expired - so the screen doesn't
	 * get redrawn. This is why we have an additional pulsing timer
	 * which does the refresh if T2 isn't running. */

	if (!backup_timer)
		backup_timer = timer_pulse (TIME_IN_CYCLES(30000, 0), 0, vectrex_screen_update_backup);

	via_config(0, &vectrex_via6522_interface);
	via_reset();
	z_factor =  translucency? 1.5: 2;

	if (vector_vh_start())
		return 1;

	return 0;
}

void vectrex_stop(void)
{
	if (tmpbitmap) osd_free_bitmap (tmpbitmap);
	vector_clear_list();
	vector_vh_stop();
	if (backup_timer) timer_remove (backup_timer);
	backup_timer=NULL;
}

/*********************************************************************
  VIA interface functions
 *********************************************************************/
INLINE void vectrex_multiplexer (int mux)
{
	analog_sig[mux + 1]=(signed char)vectrex_via_out[PORTA];
	if (mux == 3)
		/* the DAC driver expects unsigned samples */
		DAC_data_w(0,(signed char)vectrex_via_out[PORTA]+0x80);
}

static WRITE_HANDLER ( v_via_pb_w )
{
	if (!(data & 0x80))
	{
		/* RAMP is active */
		if ((vectrex_via_out[PORTB] & 0x80))
			/* RAMP was inactive before */
			start_time = timer_get_time()+RAMP_DELAY;
		if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
			/* MUX has been enabled */
			/* This is a rare case used by some new games */
		{
			double time_ = timer_get_time()+SH_DELAY;
			vectrex_solid_line(time_-start_time, old_via_sr);
			start_time = time_;
		}
	}
	else
		/* RAMP is inactive */
	{
		if (!(vectrex_via_out[PORTB] & 0x80))
			/* RAMP was active before - we can draw the line */
			vectrex_solid_line(timer_get_time()-start_time+RAMP_DELAY, old_via_sr);
	}

	/* Sound */
	if (data & 0x10)
	{
		/* BDIR active, PSG latches */
		if (data & 0x08) /* BC1 (do we select a reg or write it ?) */
			AY8910_control_port_0_w (0, vectrex_via_out[PORTA]);
		else
			AY8910_write_port_0_w (0, vectrex_via_out[PORTA]);
	}

	if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
		/* MUX has been enabled, so check with which signal the MUX
		 * coulpes the DAC output.  */
		vectrex_multiplexer ((data >> 1) & 0x3);

	vectrex_via_out[PORTB] = data;
}

static WRITE_HANDLER ( v_via_pa_w )
{
	double time_;

	if (!(vectrex_via_out[PORTB] & 0x80))  /* RAMP active (low) ? */
	{
		/* The game changes the sample and hold ICs (X/Y axis)
		 * during line draw (curved vectors)
		 * Draw the vector with the current settings
		 * before updating the signals.
		 */
		time_ = timer_get_time() + SH_DELAY;
		vectrex_solid_line(time_ - start_time, old_via_sr);
		start_time = time_;
	}
	/* DAC output always goes into X integrator */
	vectrex_via_out[PORTA] = analog_sig[0] = (signed char)data;

	if (!(vectrex_via_out[PORTB] & 0x1))
		/* MUX is enabled, so check with which signal the MUX
		 * coulpes the DAC output.  */
		vectrex_multiplexer ((vectrex_via_out[PORTB] >> 1) & 0x3);
}

static void vectrex_shift_reg_w (int via_sr)
{
	double time_;

	if (vectrex_via_out[PORTB] & 0x80)
	{
		/* RAMP inactive */
		if (via_sr & 0x01)
			/* This generates a dot (here we take the dwell time into account) */
			vectrex_dot();
	}
	else
	{
		/* RAMP active */
		time_ = timer_get_time() + BLANK_DELAY;
		vectrex_solid_line(time_ - start_time, old_via_sr);
		vectrex_shift_out(8, via_sr);
		start_time = time_ + TIME_IN_CYCLES(16, 0);
	}
	old_via_sr = via_sr;
}

static WRITE_HANDLER ( v_via_ca2_w )
{
	if  (!data)    /* ~ZERO low ? Then zero integrators*/
		vectrex_zero_integrators();
}

/*****************************************************************

  RA+A Spectrum I+

*****************************************************************/

extern int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, unsigned char **palette, int *num_palette, unsigned char **trans, int *num_trans);
extern READ_HANDLER ( s1_via_pb_r );

static struct via6522_interface spectrum1_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, s1_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ v_via_pa_w, v_via_pb_w, v_via_ca2_w, 0,
	/*irq                      */ v_via_irq,

	vectrex_shift_reg_w, vectrex_screen_update
};

static struct artwork_info *buttons, *led;
static int transparent_pen;

void raaspec_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* initialize the first 8 colors with the basic colors */
	for (i = 0; i < 8; i++)
	{
		palette[3*i+0] = (i & RED  ) ? 0xff : 0;
		palette[3*i+1] = (i & GREEN) ? 0xff : 0;
		palette[3*i+2] = (i & BLUE ) ? 0xff : 0;
	}

	/* 16 shades of gray for the vector functions */
	shade_fill (palette, WHITE, 8, 63, 0, 255);
	vectrex_refresh_with_T2=1;

	/* artwork */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		artwork_load_size(&buttons, "spec_bt.png", 64, Machine->drv->total_colors - 64, 
						  (int)(Machine->scrbitmap->height * 0.1151961), Machine->scrbitmap->height);
		if (buttons)
			artwork_load_size(&led, "led.png", buttons->start_pen + buttons->num_pens_used, Machine->drv->total_colors - 64 - buttons->num_pens_used, buttons->artwork->width, buttons->artwork->height / 8);
	}
	else
	{
		artwork_load_size(&buttons, "spec_bt.png", 64, Machine->drv->total_colors - 64, 
						  Machine->scrbitmap->width, (int)(Machine->scrbitmap->width * 0.1151961));
		if (buttons)
			artwork_load_size(&led, "led.png", buttons->start_pen + buttons->num_pens_used, Machine->drv->total_colors - 64 - buttons->num_pens_used, buttons->artwork->width / 8, buttons->artwork->height);
	}

	if (buttons && led)
	{
		memcpy (palette+(buttons->start_pen * 3), buttons->orig_palette, 3 * buttons->num_pens_used);
		memcpy (palette+(led->start_pen * 3), led->orig_palette, 3 * led->num_pens_used);
		for (i = 0; i < led->num_pens_used; i++)
			if (led->orig_palette[i*3]+led->orig_palette[i*3+1]+led->orig_palette[i*3+2] == 0)
				transparent_pen = buttons->start_pen + buttons->num_pens_used + i;
		}
}

WRITE_HANDLER( raaspec_led_w )
{
	int i, y, width;
	struct rectangle clip;
	static int old_data=0;

	logerror("Spectrum I+ LED: %i%i%i%i%i%i%i%i\n",
				 (data>>7)&0x1, (data>>6)&0x1, (data>>5)&0x1, (data>>4)&0x1,
				 (data>>3)&0x1, (data>>2)&0x1, (data>>1)&0x1, data&0x1);

	if (led && buttons)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			y = clip.min_y = tmpbitmap->width - led->artwork->width-1;
			width = led->artwork->height;
			clip.max_y = tmpbitmap->width -1;
		}
		else
		{
			y = clip.min_y = tmpbitmap->height - led->artwork->height-1;
			width = led->artwork->width;
			clip.max_y = tmpbitmap->height -1;
		}

		for (i=0; i<8; i++)
			if (((data^old_data) >> i) & 0x1)
			{
				clip.min_x = i*width;
				clip.max_x = (i+1)*width-1;

				if ((data >> i) & 0x1)
					copybitmap(tmpbitmap, buttons->artwork, 0, 0, 0, y, &clip, TRANSPARENCY_NONE, 0);
				else
					copybitmap(tmpbitmap, led->artwork, 0,0,i*width, y,&clip,TRANSPARENCY_PEN, Machine->pens[transparent_pen]);
				osd_mark_dirty (clip.min_x,clip.min_y,clip.max_x,clip.max_y);
			}
		old_data=data;
	}
}

int raaspec_start (void)
{
	int width, height;

	if (vector_vh_start())
		return 1;

	x_center=((Machine->visible_area.max_x
		  -Machine->visible_area.min_x)/2) << VEC_SHIFT;
	y_center=((Machine->visible_area.max_y
		  -Machine->visible_area.min_y)/2-10) << VEC_SHIFT;
	x_max = Machine->visible_area.max_x << VEC_SHIFT;

	vector_set_shift (VEC_SHIFT);

	via_config(0, &spectrum1_via6522_interface);
	via_reset();
	z_factor =  translucency? 1.5:2;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		width = Machine->scrbitmap->height;
		height = Machine->scrbitmap->width;
	}
	else
	{
		width = Machine->scrbitmap->width;
		height = Machine->scrbitmap->height;
	}

	if (!(tmpbitmap = bitmap_alloc(width,height)))
		return 1;

	if (led && buttons)
	{
		backdrop_refresh(buttons);
		backdrop_refresh(led);
		raaspec_led_w (0, 0xff);
	}
	return 0;
}

void raaspec_vh_update (struct osd_bitmap *bitmap, int full_refresh)
{
	copybitmap(bitmap, tmpbitmap,0,0,0,0,0,TRANSPARENCY_NONE,0);
}
