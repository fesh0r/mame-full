/*****************************************************************

  OpenGL vector routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifdef xgl

#include "xmame.h"
#include "glmame.h"
#include "driver.h"
#include "artwork.h"

#include "osinline.h"

extern GLfloat cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
  cscrx3,cscry3,cscrz3,cscrx4,cscry4,cscrz4;
extern GLfloat cscrwdx,cscrwdy,cscrwdz;
extern GLfloat cscrhdx,cscrhdy,cscrhdz;

extern int winwidth,winheight;

unsigned char *vectorram;
int vectorram_size;

GLuint veclist=0;

int listcreated=0;
int inlist=0;
int inbegin=0;

static int vec_min_x;
static int vec_min_y;
static int vec_max_x;
static int vec_max_y;
static int vec_cent_x;
static int vec_cent_y;
static int vecwidth;
static int vecheight;
static int vecshift;

static float intensity_correction = 2.0;
static GLfloat vecoldx,vecoldy;

float gl_beam=1.0;

/*
 * multiply and divide routines for drawing lines
 * can be be replaced by an assembly routine in osinline.h
 */
#ifndef vec_mult
INLINE int vec_mult(int parm1, int parm2)
{
	int temp,result;

	temp     = abs(parm1);
	result   = (temp&0x0000ffff) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp&0x0000ffff) * (parm2>>16       );
	result  += (temp>>16       ) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp>>16       ) * (parm2>>16       );

	if( parm1 < 0 )
		return(-result);
	else
		return( result);
}
#endif

/* can be be replaced by an assembly routine in osinline.h */
#ifndef vec_div
INLINE int vec_div(int parm1, int parm2)
{
	if( (parm2>>12) )
	{
		parm1 = (parm1<<4) / (parm2>>12);
		if( parm1 > 0x00010000 )
			return( 0x00010000 );
		if( parm1 < -0x00010000 )
			return( -0x00010000 );
		return( parm1 );
	}
	return( 0x00010000 );
}
#endif

void set_gl_beam(float new_value)
{
	gl_beam = new_value;
	__glLineWidth(gl_beam);
	__glPointSize(gl_beam);
	printf("GLINFO (vec): beamer size %f\n", gl_beam);
}

float get_gl_beam()
{ return gl_beam; }

/*
 * Initializes vector game video emulation
 */

int vector_vh_start (void)
{
        vec_min_x =Machine->visible_area.min_x;
        vec_min_y =Machine->visible_area.min_y;
        vec_max_x =Machine->visible_area.max_x;
        vec_max_y =Machine->visible_area.max_y;
        vecwidth  =vec_max_x-vec_min_x;
        vecheight =vec_max_y-vec_min_y;
        vec_cent_x=(vec_max_x+vec_min_x)/2;
        vec_cent_y=(vec_max_y+vec_min_y)/2;

	printf("GLINFO (vec): min %d/%d, max %d/%d, cent %d/%d,\n\t vecsize %dx%d\n", 
		vec_min_x, vec_min_y, vec_max_x, vec_max_y,
		vec_cent_x, vec_cent_y, vecwidth, vecheight);

        veclist=__glGenLists(1);
	listcreated=1;

	set_gl_beam(gl_beam);
  return 0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */

void vector_vh_stop (void)
{
	if(inlist && inbegin)
	{
		GL_END();
		inbegin=0;
	}

	CHECK_GL_BEGINEND();
	CHECK_GL_ERROR ();

	if(inlist) {
		__glEndList();
		inlist=0;
	}

	if(listcreated)
	{
		__glDeleteLists(veclist, 1);
		listcreated=0;
	}
}

/*
 * Setup scaling. Currently the Sega games are stuck at VECSHIFT 15
 * and the the AVG games at VECSHIFT 16
 */

void vector_set_shift (int shift)
{
  vecshift=shift;
}

/* Convert an xy point to xyz in the 3D scene */

int PointConvert(int x,int y,GLfloat *sx,GLfloat *sy,GLfloat *sz)
{
  int x1, y1;
  GLfloat dx,dy,tmp;

  x1=x>>vecshift;
  y1=y>>vecshift;

  /**
   * JAU: funny hack .. some coords are not within 
   *      the area [0/0] .. [vecwidth/vecheight] !
   */
   /*
  if(x1>vecwidth) x1=vecwidth;
  else if(x1<0) x1=0;

  if(y1>vecheight) y1=vecheight;
  else if(y1<0) y1=0;
  */

  dx=(GLfloat)x1/(GLfloat)vecwidth;
  dy=(GLfloat)y1/(GLfloat)vecheight;

  if(Machine->orientation&ORIENTATION_SWAP_XY) {
    tmp=dx;
    dx=dy;
    dy=tmp;
  }

  if(Machine->orientation&ORIENTATION_FLIP_X)
    dx=1.0-dx;

  if(Machine->orientation&ORIENTATION_FLIP_Y)
    dy=1.0-dy;

  if(cabview) {
	*sx=cscrx1+dx*cscrwdx+dy*cscrhdx;
	*sy=cscry1+dx*cscrwdy+dy*cscrhdy;
	*sz=cscrz1+dx*cscrwdz+dy*cscrhdz;
  }
  else {
	*sx=dx;
	*sy=dy;
  }

  if( 0<=*sx && *sx<=1.0 &&
      0<=*sy && *sy<=1.0 
    )
    return 1;

  /*
  printf("GLINFO (vec): x/y %d/%d,\n\tx1/y1 %d/%d, dx/dy %f/%f, size %dx%d\n",
  	x, y, x>>vecshift, y>>vecshift, dx, dy, vecwidth, vecheight);
  */

  return 0;
}

static void vector_begin_list(void)
{
  __glNewList(veclist,GL_COMPILE);
  inlist=1;

  CHECK_GL_ERROR ();

  __glColor4f(1.0,1.0,1.0,1.0);

  GL_BEGIN(GL_LINE_STRIP);
  inbegin=1;
}

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */

void vector_add_point (int x, int y, int color, int intensity)
{
  unsigned char r1,g1,b1;
  float red, green, blue, gamma_correction;
  GLfloat sx,sy,sz;
  int ok;

  if(!inlist)
	vector_begin_list();

  intensity *= intensity_correction/2.0;

  ok = PointConvert(x,y,&sx,&sy,&sz);

  if(intensity==0) {
  	if(inbegin) {
		GL_END();
		inbegin=0;
	}
	GL_BEGIN(GL_LINE_STRIP);
	inbegin=1;
  }

  osd_get_pen(Machine->pens[color],&r1,&g1,&b1);
  if(use_mod_ctable)
  {
	  gamma_correction = osd_get_gamma();
	  
	  red   = (float)intensity/255.0 * pow (r1 / 255.0, 1 / gamma_correction);
	  green = (float)intensity/255.0 * pow (g1 / 255.0, 1 / gamma_correction);
	  blue  = (float)intensity/255.0 * pow (b1 / 255.0, 1 / gamma_correction);
	  __glColor3f(red, green, blue);
  } else {
	  __glColor3ub(r1, g1, b1);
  }

  /**
   * Hack to draw points -- zero-length lines don't show up
   *
   * But games, e.g. tacscan have zero lines within the LINE_STRIP,
   * so we do try to continue the line strip :-)
   */
  if(sx==vecoldx&&sy==vecoldy) {
  	if(inbegin) {
		GL_END();
		inbegin=0;
	}
	GL_BEGIN(GL_POINTS);
	inbegin=1;

	if(cabview)
	  __glVertex3d(sx,sy,sz);
	else __glVertex2d(sx,sy);

  	if(inbegin) {
		GL_END();
		inbegin=0;
	}
	GL_BEGIN(GL_LINE_STRIP);
	inbegin=1;
  }

  if(cabview)
    __glVertex3f(sx,sy,sz);
  else __glVertex2f(sx,sy);

  vecoldx=sx; vecoldy=sy;
}

/*
 * Add new clipping info to the list
 */

void vector_add_clip (int x1, int yy1, int x2, int y2)
{
}

/*
 * The vector CPU creates a new display list.
 */

void vector_clear_list(void)
{
  if(inbegin) {
	GL_END();
	inbegin=0;
  }

  CHECK_GL_BEGINEND();
  CHECK_GL_ERROR ();

  if(!listcreated)
  {
	  CHECK_GL_BEGINEND();
	  veclist=__glGenLists(1);
	  listcreated=1;
  }

  CHECK_GL_ERROR ();

  if(inlist) {
	__glEndList();
	inlist=0;
  }

  CHECK_GL_ERROR ();
}

/* Called when the frame is complete */

void vector_vh_update (struct osd_bitmap *bitmap, int full_refresh)
{
  if (full_refresh && bitmap!=NULL)
  {
	fillbitmap(bitmap,Machine->uifont->colortable[0],NULL);
  }


  if(inlist && inbegin)
  {
	GL_END();
	inbegin=0;
  }

  if(inlist) {
	__glEndList();
	inlist=0;
  }
}

void vector_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	vector_vh_update (bitmap, full_refresh);
}


/*
void vector_vh_update_backdrop(struct osd_bitmap *bitmap, struct artwork *a, int full_refresh)
{
   osd_mark_dirty (0, 0, bitmap->width, bitmap->height);
   vector_vh_update(bitmap, full_refresh);
}

void vector_vh_update_overlay(struct osd_bitmap *bitmap, struct artwork *a, int full_refresh)
{
   osd_mark_dirty (0, 0, bitmap->width, bitmap->height, 0);
   vector_vh_update(bitmap, full_refresh);
}

void vector_vh_update_artwork(struct osd_bitmap *bitmap, struct artwork *o, struct artwork *b,  int full_refresh)
{
   osd_mark_dirty (0, 0, bitmap->width, bitmap->height, 0);
   vector_vh_update(bitmap, full_refresh);
}
*/

void vector_set_intensity(float _intensity)
{
	intensity_correction = _intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}

#endif /* ifdef xgl */
