/* Frank Palazzolo - 20-July-03 */

#include <math.h>
#include "osinline.h"
#include "driver.h"
#include "osdepend.h"
#include "vidhrdw/vector.h"
#include "usrintrf.h"

#include "zvgintrf.h"
#include "zvgFrame.h"

/* from video.c */
extern int video_flipx;
extern int video_flipy;
extern int video_swapxy;
extern int stretch;

int zvg_enabled = 0;

static float cached_gamma = -1;
static UINT8 Tgamma[256];         /* quick gamma anti-alias table  */

/* Size of the MAME visible region */
static int xsize, ysize;
/* Center points (defined as midpoints of the MAME visible region) */
static int xcenter, ycenter;
/* ZVG sizes for this game, without considering overscan */
static int width, height;

static void recalc_gamma_table(float _gamma)
{
	int i, h;

	for (i = 0; i < 256; i++)
	{
		h = 255.0*pow(i/255.0, 1.0/_gamma);
		if( h > 255) h = 255;
		Tgamma[i]= h;
	}
	cached_gamma = _gamma;
}

static void init_scaling(void)
{
	/* We must handle our own flipping, swapping, and clipping */

	if (video_swapxy)
	{
		if (stretch)
		{
			width = Y_MAX - Y_MIN;
			height = X_MAX - X_MIN;
		}
		else
		{
			width = Y_MAX - Y_MIN;
			height = (Y_MAX - Y_MIN)*3/4;
		}
	}
	else
	{
		width = X_MAX - X_MIN;
		height = Y_MAX - Y_MIN;
	}
}

static void transform_coords(int *x, int *y)
{
	float xf, yf;

	xf = *x / 65536.0;
	yf = *y / 65536.0;

	/* [1] scale coordinates to ZVG */

	*x = (xf - xcenter)*width/xsize;
	*y = (ycenter - (yf))*height/ysize;

	/* [2] fix display orientation */

	if (video_flipx)
		*x = ~(*x);
	if (video_flipy)
		*y = ~(*y);

	if (video_swapxy)
	{
		int temp;
		temp = *x;
		*x = *y;
		*y = temp;
	}
}

static int x1clip = 0;
static int y1clip = 0;
static int x2clip = 0;
static int y2clip = 0;

int zvg_update(point *p, int num_points)
{
	int i;

	static int x1 = 0;
	static int yy1 = 0;
	int x2, y2;
	double current_gamma;
	int x1_limit, y1_limit, x2_limit, y2_limit;

	rgb_t col;

	/* Important Note: */
	/* This center position is calculated the same way that MAME calculates */
	/* the "center of the bitmap".  We are using is as our ZVG (0,0),       */
	/* although it may not be the same coordinate as the original beam      */
	/* center position on the real thing */

	xsize = (Machine->visible_area.max_x - Machine->visible_area.min_x);
	ysize = (Machine->visible_area.max_y - Machine->visible_area.min_y);

	xcenter = xsize/2;
	ycenter = ysize/2;

	/* if we are displaying a vertical game on a horizontal monitor */
	/* or vice versa, we want overscan in one dimension but not in  */
	/* the other */

	if (video_swapxy && (!stretch))
	{
		zvgFrameSetClipWin(X_MIN*3/4,Y_MIN_O,X_MAX*3/4,Y_MAX_O);
	}
	else
	{
		zvgFrameSetClipOverscan();
	}

	/* recalculate gamma_correction if necessary */

	current_gamma = vector_get_gamma();
	if (cached_gamma != current_gamma)
	{
		recalc_gamma_table(current_gamma);
	}

	for (i = 0; i < num_points; i++)
	{
		x2 = p->x;
		y2 = p->y;

		transform_coords(&x2, &y2);

		/* process the list entry */

		if (p->status == VCLIP)
		{
			x1clip = p->x;
			y1clip = p->y;
			x2clip = p->arg1;
			y2clip = p->arg2;

			transform_coords(&x1clip, &y1clip);
			transform_coords(&x2clip, &y2clip);

			zvgFrameSetClipWin(x1clip, y1clip, x2clip, y2clip);
		}
		else
		{
			if (p->intensity != 0)
			{
				if (p->callback)
					col = Tinten(Tgamma[p->intensity], p->callback());
				else
					col = Tinten(Tgamma[p->intensity], p->col);


				/* Check the vector boundaries to see both endpoints  */
				/* are outside the overscan area on the same side     */
				/* if so, move them to the overscan boundary, so      */
				/* that you can see the overscan effects in Star Wars */

				x1_limit = x1;
				y1_limit = yy1;
				x2_limit = x2;
				y2_limit = y2;

				if ((x1 < X_MIN_O) && (x2 < X_MIN_O))
				{
					x1_limit = x2_limit = X_MIN_O;
				}
				if ((x1 > X_MAX_O) && (x2 > X_MAX_O))
				{
					x1_limit = x2_limit = X_MAX_O;
				}
				if ((yy1 < Y_MIN_O) && (y2 < Y_MIN_O))
				{
					y1_limit = y2_limit = Y_MIN_O;
				}
				if ((yy1 > Y_MAX_O) && (y2 > Y_MAX_O))
				{
					y1_limit = y2_limit = Y_MAX_O;
				}

				zvgFrameSetRGB24(RGB_RED(col),RGB_GREEN(col),RGB_BLUE(col));
				zvgFrameVector(x1_limit, y1_limit, x2_limit, y2_limit);
			}
			x1 = x2;
			yy1 = y2;
		}

		p++;
	}

	zvgFrameSend();

	/* if 2, disable MAME renderer */

	if (zvg_enabled == 2)
		return 0;
	else
		return 1;
}

int zvg_open()
{
	int rv;

	if (zvg_enabled)
	{
		if ((rv = zvgFrameOpen()))
		{
			zvgError(rv);
			return 1;
		}
		vector_register_aux_renderer(zvg_update);

		init_scaling();
	}
	return 0;
}

void zvg_close()
{
	if (zvg_enabled)
	{
		zvgFrameClose();
	}
}
