/*****************************************************************

  OpenGL vector routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk
  Copyright 2004 Hans de Goede - j.w.r.degoede@hhs.nl
   
    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

ChangeLog:

16 August 2004 (Hans de Goede):
-fixed vector support (vecshift now always = 16)
-modified to use: vector_register_aux_renderer,
 now we no longer need any core modifcations, and
 the code is somewhat cleaner.

Todo:
-add clipping support, needs someone with better openGL skills
 then me.

*****************************************************************/

#include <math.h>
#include "glmame.h"
#include "sysdep/sysdep_display_priv.h"

/* from mame's vidhrdw/vector.h */
#define VCLEAN  0
#define VDIRTY  1
#define VCLIP   2

static int vecwidth, vecheight;

/* Convert an xy point to xyz in the 3D scene */
static void PointConvert(int x,int y,GLdouble *sx,GLdouble *sy,GLdouble *sz)
{
  GLdouble dx,dy;

  x = (x+0x8000) >> 16;
  y = (y+0x8000) >> 16;

  dx=(GLdouble)x/(GLdouble)vecwidth;
  dy=(GLdouble)y/(GLdouble)vecheight;

  if(!cabview) {
	  *sx=dx;
	  *sy=dy;
  } else {
	CalcCabPointbyViewpoint ( dx*s__cscr_w_view,
	                          dy*s__cscr_h_view,
				  sx, sy, sz );
  }
}

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
static void glvec_add_point (int x, int y, rgb_t color, int intensity)
{
  double red, green, blue;
  GLdouble sx,sy,sz;
  static GLdouble vecoldx,vecoldy;
  
  PointConvert(x,y,&sx,&sy,&sz);

  red   = ((color & 0xff0000) * intensity) / (255.0 * 255.0 * 65536);
  green = ((color & 0x00ff00) * intensity) / (255.0 * 255.0 * 256);
  blue  = ((color & 0x0000ff) * intensity) / (255.0 * 255.0);

  if(intensity==0)
  {
        GL_END();
        GL_BEGIN(GL_LINE_STRIP);
  }

  disp__glColor3d(blue, green, red); /* we use bgr not rgb ! */

  if((fabs(sx-vecoldx) < 0.001) && (fabs(sy-vecoldy) < 0.001))
  {
	  /* Hack to draw points -- very short lines don't show up
	   *
	   * But games, e.g. tacscan have zero lines within the LINE_STRIP,
	   * so we do try to continue the line strip :-) */
	GL_END();
	GL_BEGIN(GL_POINTS);

	if(cabview)
	  disp__glVertex3d(sx,sy,sz);
	else disp__glVertex2d(sx,sy);

	GL_END();
	GL_BEGIN(GL_LINE_STRIP);
  }

  if(cabview)
    disp__glVertex3d(sx,sy,sz);
  else disp__glVertex2d(sx,sy);

  vecoldx=sx; vecoldy=sy;
}

int glvec_renderer(point *pt, int num_points)
{
  if (num_points)
  {
    vecwidth  =sysdep_display_params.vec_src_bounds->max_x-sysdep_display_params.vec_src_bounds->min_x;
    vecheight =sysdep_display_params.vec_src_bounds->max_y-sysdep_display_params.vec_src_bounds->min_y;

    disp__glNewList(veclist,GL_COMPILE);

    CHECK_GL_ERROR ();

    disp__glColor4d(1.0,1.0,1.0,1.0);

    GL_BEGIN(GL_LINE_STRIP);
    
    while(num_points)
    {
      if (pt->status == VCLIP)
      {
        /* FIXME !!! */
      }
      else
      {
        if (pt->callback)
          glvec_add_point(pt->x, pt->y, pt->callback(), pt->intensity);
        else
          glvec_add_point(pt->x, pt->y, pt->col, pt->intensity);
      }
      pt++;
      num_points--;
    }

    GL_END();
    disp__glEndList();
  }
  return 0;
}
