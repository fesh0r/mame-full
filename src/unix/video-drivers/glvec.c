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

extern GLfloat cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
  cscrx3,cscry3,cscrz3,cscrx4,cscry4,cscrz4;
extern GLfloat cscrwdx,cscrwdy,cscrwdz;
extern GLfloat cscrhdx,cscrhdy,cscrhdz;

extern int winwidth,winheight;

unsigned char *vectorram;
int vectorram_size;

GLuint veclist;

int inlist=0;
int incommand=0;
static int listalloced=0;

static int vecshift=0;
static float intensity_correction = 1.5;
static GLfloat vecwidth,vecheight;
static GLfloat vecoldx,vecoldy;

float gl_beam=1.0;
float gl_translucency=1.0;

/*
 * Initializes vector game video emulation
 */

int vector_vh_start (void)
{
	glLineWidth(gl_beam);
	printf("beamer size %f\n", gl_beam);

  vecwidth=(GLfloat)(Machine->drv->default_visible_area.max_x-
	Machine->drv->default_visible_area.min_x);
  vecheight=(GLfloat)(Machine->drv->default_visible_area.max_y-
	Machine->drv->default_visible_area.min_y);
  veclist=glGenLists(1);

  return 0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */

void vector_vh_stop (void)
{
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

void PointConvert(int x,int y,GLfloat *sx,GLfloat *sy,GLfloat *sz)
{
  GLfloat dx,dy,tmp;

  dx=(GLfloat)(x>>vecshift)/vecwidth;
  dy=(GLfloat)(y>>vecshift)/vecheight;

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
  	/*
	*sx=dx*(GLfloat)winwidth;
	*sy=(GLfloat)winheight-dy*(GLfloat)winheight;
	*/
	*sx=dx;
	*sy=dy;
  }
}

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */

void vector_add_point (int x, int y, int color, int intensity)
{
  GLfloat sx,sy,sz;
  int col;

  intensity *= intensity_correction;
  if(intensity==0) {
	glEnd();
	glBegin(GL_LINE_STRIP);
  }

  col=Machine->pens[color];

  if(glGetUseEXT() && _glColorTableEXT)
  {
    /* Usage of intensity AND translucency ... ? NOPE !
  glColor4f((GLfloat)ctable[col*4]/255.0,
			(GLfloat)ctable[col*4+1]/255.0,
			(GLfloat)ctable[col*4+2]/255.0,
			  (GLfloat)intensity/(gl_translucency?450.0:149.0));
	 */
    if(alphablending)
    {
		glColor4ub(ctable[col*4],
				  ctable[col*4+1],
				  ctable[col*4+2],
				  ctable[col*4+3]);
	} else {
		glColor3ub(ctable[col*3],
				  ctable[col*3+1],
				  ctable[col*3+2]);
	}
  } else {
    if(alphablending)
    {
		glColor4us(rcolmap[col],
		    gcolmap[col],
			bcolmap[col],
				  acolmap[col]);
	} else {
		glColor3us(rcolmap[col],
				  gcolmap[col],
				  bcolmap[col]);
	}
  }

  PointConvert(x,y,&sx,&sy,&sz);

  /* Hack to draw points -- zero-length lines don't show up */

  if(sx==vecoldx&&sy==vecoldy) {
	glEnd();
	glBegin(GL_POINTS);

	if(cabview)
	  glVertex3f(sx,sy,sz);
	else glVertex2f(sx,sy);

	glEnd();
	glBegin(GL_LINE_STRIP);
  }
  else {
	if(cabview)
	  glVertex3f(sx,sy,sz);
	else
	  glVertex2f(sx,sy);
  }
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
  glNewList(veclist,GL_COMPILE);
  glColor4f(1.0,1.0,1.0,1.0);

  glBegin(GL_LINE_STRIP);

  inlist=1;
  listalloced=1;
  incommand=1;
}

/* Called when the frame is complete */

void vector_vh_update(struct osd_bitmap *bitmap,int full_refresh)
{
  if(incommand) {
	glEnd();
	incommand=0;
  }

  if(inlist) {
	glEndList();
	inlist=0;
  }
}
void vector_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	vector_vh_update(bitmap,full_refresh) ;
}


/*
void vector_vh_update_backdrop(struct osd_bitmap *bitmap, struct artwork *a, int full_refresh)
{
   osd_mark_dirty (0, 0, bitmap->width, bitmap->height, 0);
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

void vector_set_gamma(float _gamma)
{
	#ifdef WIN32
		gl_set_gamma(_gamma);
	#else
		osd_set_gamma(_gamma);
	#endif
}

float vector_get_gamma(void)
{
	return 
		#ifdef WIN32
			gl_get_gamma();
		#else
			osd_get_gamma();
		#endif
}

void vector_set_intensity(float _intensity)
{
	intensity_correction = _intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}


#endif /* ifdef xgl */
