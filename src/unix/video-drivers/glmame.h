/*****************************************************************

  GLmame include file

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifndef _GLMAME_H
#define _GLMAME_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include <math.h>
#include "MAME32.h"
#include "wgl_tool.h"
#include "wgl_GDIDisplay.h"
#else
#include <ctype.h>
#include <math.h>
#include <dlfcn.h>
#define CALLBACK
#endif

#include "gltool.h"
#include "vidhrdw/vector.h"

/* Camera panning stuff */

typedef enum {pan_goto,pan_moveto,pan_repeat,pan_end,pan_nocab} PanType;

struct CameraPan {
  PanType type;      /* Type of pan */
  GLdouble lx,ly,lz;  /* Location of camera */
  GLdouble px,py,pz;  /* Vector to point camera along */
  GLdouble nx,ny,nz;  /* Normal to camera direction */
  int frames;        /* Number of frames for transition */
};

/* glcab.c */
extern GLubyte **cabimg;
extern GLuint *cabtex;
extern struct CameraPan *cpan;
extern int numpans;
extern GLuint cablist;

/* xgl.c */
extern char * libGLName;
extern char * libGLUName;
extern GLXContext glContext;
extern int antialias;
extern int winwidth;
extern int winheight;
extern int bilinear;
extern int alphablending;
extern int force_text_width_height;

/* glvec.c */
extern float gl_beam;
extern GLuint veclist;

/* glgen.c */
extern double scrnaspect, vscrnaspect;
extern GLdouble  s__cscr_w_view, s__cscr_h_view;
extern GLdouble vx_cscr_p1, vy_cscr_p1, vz_cscr_p1, 
        vx_cscr_p2, vy_cscr_p2, vz_cscr_p2,
        vx_cscr_p3, vy_cscr_p3, vz_cscr_p3, 
	vx_cscr_p4, vy_cscr_p4, vz_cscr_p4;
extern int gl_is_initialized;

/* ? */
extern int cabview;
extern char *cabname; /* 512 bytes reserved ... */

/* xgl.c */
void SwapBuffers (void);

/* glvec.c */
void set_gl_beam(float new_value);
float get_gl_beam();
int glvec_renderer(point *start, int num_points);
void glvec_init(void);
void glvec_exit(void);

/* glcab.c */
void InitCabGlobals();
int LoadCabinet (const char *fname);

/* glgen.c
 * 
 * the calling order is the listed order:
 * 
 * first the start sequence, then the quit sequence ..
 */

/* start sequence */
void gl_bootstrap_resources();
int  sysdep_display_16bpp_capable (void);
int  InitVScreen (int vw, int vh);
void gl_reset_resources();
int  sysdep_display_alloc_palette (int writable_colors);
void InitTextures (struct mame_bitmap *bitmap);

/* quit sequence */
void CloseVScreen (void);
void gl_reset_resources();

/* misc sequence */
void UpdateVScreen(struct mame_bitmap *bitmap);
void  gl_set_bilinear(int new_value);
void  gl_init_cabview ();
void  gl_set_cabview(int new_value);
int   gl_stream_antialias (int aa);
void  gl_set_antialias(int new_value);
int   gl_stream_alphablending (int alpha);
void  gl_set_alphablending(int new_value);
void  gl_resize(int w, int h, int vw, int vh);
void  CalcCabPointbyViewpoint( 
		   GLdouble vx_gscr_view, GLdouble vy_gscr_view, 
                   GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p
		 );

/* glexport */
void gl_save_screen_snapshot();
int gl_png_write_bitmap(void *fp);
void ppm_save_snapshot (void *fp);

#endif /* _GLMAME_H */
