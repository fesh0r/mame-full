#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/generic.h"
#include "machine/6522via.h"
#include "mscommon.h"

#include "includes/vectrex.h"

#define RAMP_DELAY 6.333e-6
#define BLANK_DELAY 0
#define SH_DELAY 6.333e-6

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
static WRITE8_HANDLER ( v_via_pa_w );
static WRITE8_HANDLER( v_via_pb_w );
static void vectrex_shift_reg_w (int via_sr);
static WRITE8_HANDLER ( v_via_ca2_w );

static struct via6522_interface vectrex_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, v_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ v_via_pa_w, v_via_pb_w, v_via_ca2_w, 0,
	/*irq                      */ v_via_irq,

	vectrex_shift_reg_w
};

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
static int vectrex_point_index = 0;
static double vectrex_persistance = 0.05;

struct vectrex_point
{
	int x; int y;
	rgb_t col;
	int intensity;
	double time;
};

static struct vectrex_point vectrex_points[MAX_POINTS];
void (*vector_add_point_function) (int, int, rgb_t, int) = vectrex_add_point;

void vectrex_add_point (int x, int y, rgb_t color, int intensity)
{
	struct vectrex_point *newpoint;

	vectrex_point_index = (vectrex_point_index+1) % MAX_POINTS;
	newpoint = &vectrex_points[vectrex_point_index];

	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
	newpoint->time = timer_get_time();
}

/*********************************************************************
  Screen updating
 *********************************************************************/

VIDEO_UPDATE( vectrex )
{
	int i, v, intensity;
	double starttime, correction;

	vectrex_configuration();

	starttime = timer_get_time() - vectrex_persistance;
	if (starttime < 0) starttime = 0;

	i = vectrex_point_index;

	/* Find the oldest vector we want to display */
	for (v=0; vectrex_points[i].time > starttime && v < MAX_POINTS-1; v++)
	{
		i--;
		if (i<0) i=MAX_POINTS-1;
	}

	/* start black */
	vector_add_point(vectrex_points[i].x, vectrex_points[i].y, vectrex_points[i].col, 0);

	while (i != vectrex_point_index)
	{
		correction = MIN(1, (vectrex_points[i].time-starttime)/(vectrex_persistance/2));
		intensity = vectrex_points[i].intensity*correction;
		vector_add_point(vectrex_points[i].x,vectrex_points[i].y,vectrex_points[i].col,intensity);
		i = (i+1) % MAX_POINTS;
	}

	video_update_vector(bitmap, &Machine->visible_area, do_skip);
	vector_clear_list();
}

/*********************************************************************
  Vector functions
 *********************************************************************/
void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity)
{
	if (vectrex_imager_status == 2) /* left = 1, right = 2 */
		vectrex_add_point ((int)(y*M_SQRT1_2), (int)(((x_max-x)*M_SQRT1_2)+y_center), color, intensity);
	else
		vectrex_add_point ((int)(y*M_SQRT1_2), (int)((x_max-x)*M_SQRT1_2), color, intensity);
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
  Startup and stop
 *********************************************************************/
VIDEO_START( vectrex )
{
	int width, height;

	width = Machine->scrbitmap->width;
	height = Machine->scrbitmap->height;

	x_center=((Machine->visible_area.max_x
		  -Machine->visible_area.min_x) / 2) << VEC_SHIFT;
	y_center=((Machine->visible_area.max_y
		  -Machine->visible_area.min_y) / 2 - 10) << VEC_SHIFT;
	x_max = Machine->visible_area.max_x << VEC_SHIFT;

	via_config(0, &vectrex_via6522_interface);
	via_reset();
	z_factor =  translucency? 1.5: 2;

	imager_freq = 1;

	imager_timer = timer_alloc(vectrex_imager_right_eye);
	if (!imager_timer)
		return 1;
	timer_adjust(imager_timer, 1.0/imager_freq, 2, 1.0/imager_freq);


	if (video_start_vector())
		return 1;

	return 0;
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

static WRITE8_HANDLER ( v_via_pb_w )
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

static WRITE8_HANDLER ( v_via_pa_w )
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

static WRITE8_HANDLER ( v_via_ca2_w )
{
	if  (!data)    /* ~ZERO low ? Then zero integrators*/
		vectrex_zero_integrators();
}

/*****************************************************************

  RA+A Spectrum I+

*****************************************************************/

static struct via6522_interface spectrum1_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, s1_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ v_via_pa_w, v_via_pb_w, v_via_ca2_w, 0,
	/*irq                      */ v_via_irq,

	vectrex_shift_reg_w
};
/*
static const char *radius_8_led =
	"     111111     \r"
	"   1111111111   \r"
	"  111111111111  \r"
	" 11111111111111 \r"
	" 11111111111111 \r"
	"1111111111111111\r"
	"1111111111111111\r"
	"1111111111111111\r"
	"1111111111111111\r"
	"1111111111111111\r"
	"1111111111111111\r"
	" 11111111111111 \r"
	" 11111111111111 \r"
	"  111111111111  \r"
	"   1111111111   \r"
	"     111111     \r";
*/

WRITE8_HANDLER( raaspec_led_w )
{
	struct rectangle;

	logerror("Spectrum I+ LED: %i%i%i%i%i%i%i%i\n",
				 (data>>7)&0x1, (data>>6)&0x1, (data>>5)&0x1, (data>>4)&0x1,
				 (data>>3)&0x1, (data>>2)&0x1, (data>>1)&0x1, data&0x1);

#if 0
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		y = clip.min_y = tmpbitmap->width - 16;
		width = 16;
		clip.max_y = tmpbitmap->width -1;
	}
	else
	{
		y = clip.min_y = tmpbitmap->height - 16;
		width = 16;
		clip.max_y = tmpbitmap->height -1;
	}

	for (i=0; i<8; i++)
	{
		if (((data^old_data) >> i) & 0x1)
		{
			clip.min_x = i*width;
			clip.max_x = (i+1)*width-1;

			if (((data >> i) & 0x1) == 0)
				draw_led(tmpbitmap, radius_8_led, 1, i*width, y);
		}
	}
	old_data=data;
#endif
}

VIDEO_START( raaspec )
{
	if (video_start_vector())
		return 1;

	x_center=((Machine->visible_area.max_x
		  -Machine->visible_area.min_x)/2) << VEC_SHIFT;
	y_center=((Machine->visible_area.max_y
		  -Machine->visible_area.min_y)/2-10) << VEC_SHIFT;
	x_max = Machine->visible_area.max_x << VEC_SHIFT;

	via_config(0, &spectrum1_via6522_interface);
	via_reset();
	z_factor =  translucency? 1.5:2;

	raaspec_led_w (0, 0xff);
	return 0;
}
