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

#ifdef xgl

#include "xmame.h"
#include "glmame.h"
#include "driver.h"
#include "artwork.h"
#include "vidhrdw/vector.h"

#include "osinline.h"

void CalcCabPointbyViewpoint( 
		   GLdouble vx_gscr_view, GLdouble vy_gscr_view, 
                   GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p
		 );
extern GLdouble  s__cscr_w_view, s__cscr_h_view;
static int glvec_renderer(point *start, int num_points);

GLuint veclist=0;
float gl_beam=1.0;
static int vecwidth, vecheight;

void set_gl_beam(float new_value)
{
	gl_beam = new_value;
	disp__glLineWidth(gl_beam);
	disp__glPointSize(gl_beam);
	printf("GLINFO (vec): beamer size %f\n", gl_beam);
}

float get_gl_beam()
{ return gl_beam; }

void glvec_init(void)
{
        veclist=disp__glGenLists(1);

	set_gl_beam(gl_beam);
	
	vector_register_aux_renderer(glvec_renderer);
}

void glvec_exit(void)
{
	disp__glDeleteLists(veclist, 1);
}

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
  unsigned char r1,g1,b1;
  double red=0.0, green=0.0, blue=0.0;
  GLdouble sx,sy,sz;
  int ptHack=0;
  double gamma_correction = palette_get_global_gamma();
  static GLdouble vecoldx,vecoldy;
  
  PointConvert(x,y,&sx,&sy,&sz);

  b1 =   color        & 0xff ;
  g1 =  (color >>  8) & 0xff ;
  r1 =  (color >> 16) & 0xff ;

  if(sx==vecoldx&&sy==vecoldy&&intensity>0)
  {
	  /**
	   * Hack to draw points -- zero-length lines don't show up
	   *
	   * But games, e.g. tacscan have zero lines within the LINE_STRIP,
	   * so we do try to continue the line strip :-)
	   *
	   * Part 1
	   */
	  GL_END();
  	  ptHack=1;
  } else {
  	  
	  /**
	   * the usual "normal" way ..
	   */
	  if(intensity==0)
	  {
		GL_END();
		GL_BEGIN(GL_LINE_STRIP);
	  }
  	  ptHack=0;
  }

  if(use_mod_ctable)
  {
	  red   = (double)intensity/255.0 * pow (r1 / 255.0, 1 / gamma_correction);
	  green = (double)intensity/255.0 * pow (g1 / 255.0, 1 / gamma_correction);
	  blue  = (double)intensity/255.0 * pow (b1 / 255.0, 1 / gamma_correction);
	  disp__glColor3d(red, green, blue);
  } else {
	  disp__glColor3ub(r1, g1, b1);
  }

  if(ptHack)
  {
	/**
	 * Hack to draw points -- zero-length lines don't show up
	 *
	 * But games, e.g. tacscan have zero lines within the LINE_STRIP,
	 * so we do try to continue the line strip :-)
	 *
	 * Part 2
	 */
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

static int glvec_renderer(point *pt, int num_points)
{
  if (num_points)
  {
    vecwidth  =Machine->visible_area.max_x-Machine->visible_area.min_x;
    vecheight =Machine->visible_area.max_y-Machine->visible_area.min_y;

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
 
#endif /* ifdef xgl */
