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

#ifndef GLDEBUG
	#define NDEBUG
#endif

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

/* Camera panning stuff */

typedef enum {pan_goto,pan_moveto,pan_repeat,pan_end,pan_nocab} PanType;

struct CameraPan {
  PanType type;      /* Type of pan */
  GLfloat lx,ly,lz;  /* Location of camera */
  GLfloat px,py,pz;  /* Vector to point camera along */
  GLfloat nx,ny,nz;  /* Normal to camera direction */
  int frames;        /* Number of frames for transition */
};

/* video.c */
extern float gamma_correction;

/* xgl.c */
extern GLXContext glContext;
extern int antialias;
extern int fullscreen_width;
extern int fullscreen_height;
extern int winwidth;
extern int winheight;
extern int doublebuffer;
extern int bilinear;
extern int alphablending;
extern int bitmap_lod; /* level of bitmap-texture detail */
extern int fullscreen;

/* glvec.c */
extern float gl_beam;
extern float gl_translucency;

/* glgen.c */
extern int totalcolors;
extern int palette_dirty;
extern GLubyte *ctable;
extern unsigned char *ctable_orig;	/* for original return 2 game values */
extern GLushort *rcolmap, *gcolmap, *bcolmap, *acolmap;
extern int ctable_size; /* the true color table size */
extern GLenum gl_bitmap_format;
extern unsigned char gl_alpha_value; /* customize it :-) */
extern double scrnaspect, vscrnaspect;
extern GLsizei text_width;
extern GLsizei text_height;
extern int force_text_width_height;
extern int screendirty;
extern int dodepth;
extern int cabview;
extern int cabload_err;
extern int drawbitmap;
extern int dopersist;
extern GLboolean useGlEXT;

extern char *cabname; /* 512 bytes reserved ... */
extern int cabspecified;
extern int gl_is_initialized;
extern GLuint cablist;
#ifdef WIN32
	extern PFNGLCOLORTABLEEXTPROC _glColorTableEXT;
#else
	extern void (*_glColorTableEXT)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
#endif
extern int gl_is_initialized;

/* xgl.c */
void switch2Fullscreen();

/* glvec.c */
extern void set_gl_beam(float new_value);
extern float get_gl_beam();

/* glcab.c */
void InitCabGlobals();

/* glgen.c */
void gl_bootstrap_resources();
void gl_reset_resources();
GLboolean glHasEXT(void);
void glSetUseEXT(GLboolean val);
GLboolean glGetUseEXT(void);
void  gl_set_bilinear(int new_value);
void  gl_set_cabview(int new_value);
void  gl_set_antialias(int new_value);
void  gl_set_alphablending(int new_value);
void  xgl_fixaspectratio(int *w, int *h);
void  xgl_resize(int w, int h);

#endif /* _GLMAME_H */
