/*****************************************************************

  Generic OpenGL routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifdef xgl

#include "driver.h"
#include "xmame.h"
#include "glmame.h"

int LoadCabinet (const char *fname);
void SwapBuffers (void);
void vector_vh_update (struct osd_bitmap *bitmap, int full_refresh);
void vector_clear_list (void);


void DeltaVec (GLfloat x1, GLfloat y1, GLfloat z1,
	       GLfloat x2, GLfloat y2, GLfloat z2,
	       GLfloat * dx, GLfloat * dy, GLfloat * dz);
void CalcFlatPoint (int x, int y, GLfloat * px, GLfloat * py);
void CalcCabPoint (int x, int y, GLfloat * px, GLfloat * py, GLfloat * pz);
void SetupFrustum (void);
void SetupOrtho (void);
void InitTextures (void);
void InitVScreen (void);
void CloseVScreen (void);
void UpdateTexture (struct osd_bitmap *bitmap);
void WAvg (GLfloat perc, GLfloat x1, GLfloat y1, GLfloat z1,
	   GLfloat x2, GLfloat y2, GLfloat z2,
	   GLfloat * ax, GLfloat * ay, GLfloat * az);
void UpdateCabDisplay (void);
void UpdateFlatDisplay (void);
void UpdateGLDisplay (struct osd_bitmap *bitmap);

int cabspecified = 0;

double scrnaspect, vscrnaspect;

GLubyte *ctable = 0;
GLushort *rcolmap = 0, *gcolmap = 0, *bcolmap = 0, *acolmap = 0;
unsigned char *ctable_orig = 0;	/* for original return 2 game values */
int ctable_size;		/* the true color table size */

GLenum	   gl_bitmap_format;        /* only GL_UNSIGNED_BYTE and GL_UNSIGNED_SHORT is supported */
GLint      gl_internal_format;
GLsizei text_width;
GLsizei text_height;

int force_text_width_height = 0;

static int palette_changed = 0;

/* int cabview=0;  .. Do we want a cabinet view or not? */
int cabload_err=0;

int drawbitmap = 1;
int dopersist = 0;
int screendirty = 1;		/* Has the bitmap been modified since the last frame? */
int dodepth = 1;

static int palette_dirty = 0;
static int totalcolors;
unsigned char gl_alpha_value = 255;
static int frame = 0;

static GLint gl_white[] = { 1, 1, 1, 1};
static GLint gl_black[] = { 0, 0, 0, 0};

/* The squares that are tiled to make up the game screen polygon */

struct TexSquare
{
  GLubyte *texture;
  GLuint texobj;
  GLfloat x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
  GLfloat fx1, fy1, fx2, fy2, fx3, fy3, fx4, fy4;
  GLfloat xcov, ycov;
};

static struct TexSquare *texgrid = NULL;

/* max texsize is fetched ..
	static int texsize=256;
*/

static int texnumx;
static int texnumy;
static GLfloat texpercx;
static GLfloat texpercy;
static GLfloat vscrntlx;
static GLfloat vscrntly;
static GLfloat vscrnwidth;
static GLfloat vscrnheight;
static GLfloat xinc, yinc;

static GLint memory_x_len;
static GLint memory_x_start_offset;
static int bytes_per_pixel;

GLfloat cscrx1, cscry1, cscrz1, cscrx2, cscry2, cscrz2,
  cscrx3, cscry3, cscrz3, cscrx4, cscry4, cscrz4;
GLfloat cscrwdx, cscrwdy, cscrwdz;
GLfloat cscrhdx, cscrhdy, cscrhdz;

extern GLubyte *cabimg[5];
extern GLuint cabtex[5];

extern struct CameraPan *cpan;
extern int numpans;
int currentpan = 0;
int lastpan = 0;
int panframe = 0;

/* Vector variables */

int vecgame = 0;

extern GLuint veclist;
extern int inlist;

/* ---------------------------------------------------------------------- */
/* ------------ New OpenGL Specials ------------------------------------- */
/* ---------------------------------------------------------------------- */

int gl_is_initialized = 0;
static int useGlEXT = 1;

/*
	void ColorTableEXT(enum target, enum internalformat, sizei width, 
		     enum format, enum type, const void *data);

	void ColorSubTableEXT(enum target, sizei start, sizei count, 
		     enum format, enum type, const void *data);
*/

#ifdef WIN32
	PFNGLCOLORTABLEEXTPROC _glColorTableEXT = NULL;
	PFNGLCOLORSUBTABLEEXTPROC _glColorSubTableEXT = NULL;
#else
	void (* _glColorTableEXT) (GLenum, GLenum, GLsizei, GLenum, GLenum,
				   const GLvoid *) = NULL;
	void (* _glColorSubTableEXT) (GLenum, GLsizei, GLsizei, GLenum, GLenum,
				      const GLvoid *) = NULL;
#endif

#ifdef WIN32
#ifndef NDEBUG
#define CHECK_WGL_ERROR(a,b,c) check_wgl_error(a,b,c)
#define CHECK_GL_ERROR(a,b)  check_gl_error(a,b)
#else
#define CHECK_WGL_ERROR(a,b,c)
#define CHECK_GL_ERROR(a,b)
#endif
#else
#ifndef NDEBUG
#define CHECK_WGL_ERROR(a,b,c)
#define CHECK_GL_ERROR(a,b)  check_gl_error(a,b)
#else
#define CHECK_WGL_ERROR(a,b,c)
#define CHECK_GL_ERROR(a,b)
#endif
#endif

#ifdef WIN32
static void
check_wgl_error (HWND wnd, const char *file, int line)
{
  LPVOID lpMsgBuf;

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | \FORMAT_MESSAGE_FROM_SYSTEM,
		 0, GetLastError (), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
		 (LPTSTR) & lpMsgBuf, 0, 0);	// Display the string.

  fprintf (stderr, "****\n** %s:%d>%s\n ****\n", file, line, lpMsgBuf);

  // Free the buffer.
  LocalFree (lpMsgBuf);
}
#endif

static void
check_gl_error (const char *file, int line)
{
  int ec = glGetError ();

  if (ec != GL_NO_ERROR)
  {
    const char *errstr = (const char *) gluErrorString (ec);
    if (errstr != 0)
      fprintf (stderr, "****\n** %s:%d>0x%X %s\n ****\n", file, line, ec, errstr);
    else
      fprintf (stderr, "****\n** %s:%d>0x%X <unknown>\n ****\n", file, line, ec);
  }
}

void *
getGLProcAddressHelper (const char *func, int *method, int debug, int verbose)
{
  void *funcPtr = NULL;
  int lmethod;

#ifdef WIN32

  HMODULE hDLL_OPENGL32 = 0;



  funcPtr = wglGetProcAddress (func);
  if (funcPtr == NULL)
  {
    lmethod = 2;


    if (hDLL_OPENGL32 == NULL)

    {
      hDLL_OPENGL32 = LoadLibrary ("OPENGL32.DLL");

      /* hDLL_OPENGL32 = GetModuleHandle ("OPENGL32.DLL"); */

    }
    if (hDLL_OPENGL32 == NULL)
    {
      printf ("cannot access OpenGL library OPENGL32.DLL\n");
      fflush (NULL);
    }
    else
    {
      funcPtr = GetProcAddress (hDLL_OPENGL32, func);
      FreeLibrary (hDLL_OPENGL32);
    }
  }
  else
    lmethod = 1;
#else

  void *libHandleGL;

  /*
   * void (*glXGetProcAddressARB(const GLubyte *procName))
   */
  static void *(*__glXGetProcAddress) (const GLubyte * procName) = NULL;
  static int __firstAccess = 1;

  if (__glXGetProcAddress == NULL && __firstAccess)
  {
    libHandleGL = dlopen ("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
    if (libHandleGL == NULL)
    {
      printf ("cannot access OpenGL library libGL.so\n");
      fflush (NULL);
    }
    else
    {
      __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressARB");

      if (__glXGetProcAddress != NULL && verbose)
      {
	printf ("found glXGetProcAddressARB in libGL.so\n");
	fflush (NULL);
      }

      if (__glXGetProcAddress == NULL)
      {
	__glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressEXT");

	if (__glXGetProcAddress != NULL && verbose)
	{
	  printf ("found glXGetProcAddressEXT in libGL.so\n");
	  fflush (NULL);
	}
      }

      if (__glXGetProcAddress == NULL)
      {
	__glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddress");

	if (__glXGetProcAddress != NULL && verbose)
	{
	  printf ("found glXGetProcAddress in libGL.so\n");
	  fflush (NULL);
	}
      }

      dlclose (libHandleGL);
      libHandleGL = NULL;

      if (__glXGetProcAddress == NULL)
      {
	printf
	  ("cannot find glXGetProcAddress* in OpenGL library libGL.so\n");
	fflush (NULL);
	libHandleGL = dlopen ("libglx.so", RTLD_LAZY | RTLD_GLOBAL);
	if (libHandleGL == NULL)
	{
	  printf ("cannot access GLX library libglx.so\n");
	  fflush (NULL);
	}
	else
	{
	  __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressARB");

	  if (__glXGetProcAddress != NULL && verbose)
	  {
	    printf ("found glXGetProcAddressARB in libglx.so\n");
	    fflush (NULL);
	  }

	  if (__glXGetProcAddress == NULL)
	  {
	    __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressEXT");

	    if (__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("found glXGetProcAddressEXT in libglx.so\n");
	      fflush (NULL);
	    }
	  }

	  if (__glXGetProcAddress == NULL)
	  {
	    __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddress");

	    if (__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("found glXGetProcAddress in libglx.so\n");
	      fflush (NULL);
	    }
	  }

	  dlclose (libHandleGL);
	  libHandleGL = NULL;
	}
      }
    }
  }
  if (__glXGetProcAddress == NULL && __firstAccess)
  {
    printf ("cannot find glXGetProcAddress* in GLX library libglx.so\n");
    fflush (NULL);
  }
  __firstAccess = 0;

  if (__glXGetProcAddress != NULL)
    funcPtr = __glXGetProcAddress (func);

  if (funcPtr == NULL)
  {
    lmethod = 2;
    libHandleGL = dlopen ("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
    if (libHandleGL == NULL)
    {
      printf ("cannot access OpenGL library libGL.so\n");
      fflush (NULL);
    }
    else
    {
      funcPtr = dlsym (libHandleGL, func);
      dlclose (libHandleGL);
    }
  }
  else
    lmethod = 1;
#endif


  if (funcPtr == NULL)
  {
    if (debug || verbose)
    {
      printf ("%s (%d): not implemented !\n", func, lmethod);
      fflush (NULL);
    }
  }
  else if (verbose)
  {
    printf ("%s (%d): loaded !\n", func, lmethod);
    fflush (NULL);
  }
  if (method != NULL)
    *method = lmethod;
  return funcPtr;
}


static void
fetchGL_EXT_FUNCS ()
{
  if (_glColorTableEXT!=NULL || _glColorSubTableEXT!=NULL)
    return;

  _glColorTableEXT = getGLProcAddressHelper ("glColorTableEXT", NULL, 1, 1);
  _glColorSubTableEXT = getGLProcAddressHelper ("glColorSubTableEXT", NULL, 1, 1);

  if (_glColorTableEXT == 0)
  {
    printf("glColorTableEXT not avaiable .. BAD & SLOW\n");
    if(_glColorSubTableEXT != 0)
	    printf("ERROR: glColorTableEXT not avaiable, but glColorSubTableEXT .. bad OpenGL impl. \n");
    CHECK_WGL_ERROR (glWnd, __FILE__, __LINE__);
    _glColorSubTableEXT = 0;
    useGlEXT = 0;
  } else
    printf("glColorTableEXT avaiable .. GOOD & FAST\n");

  if (_glColorSubTableEXT == 0)
  {
    printf("glColorSubTableEXT not avaiable .. BAD & SLOW\n");
    if(_glColorTableEXT != 0)
	    printf("ERROR: glColorSubTableEXT not avaiable, but glColorTableEXT .. bad OpenGL impl. \n");
    CHECK_WGL_ERROR (glWnd, __FILE__, __LINE__);
    _glColorTableEXT = 0;
    useGlEXT = 0;
  } else
    printf("glColorSubTableEXT avaiable .. GOOD & FAST\n");
}

int
glHasEXT (void)
{
  if (gl_is_initialized)
    fetchGL_EXT_FUNCS ();
  return _glColorTableEXT != NULL;
}

void
glSetUseEXT (int val)
{
  if (gl_is_initialized == 0 || val==0 || (_glColorTableEXT!=NULL && _glColorSubTableEXT!=NULL) )
    useGlEXT = val;
}

int
glGetUseEXT (void)
{
  return useGlEXT;
}

void
gl_set_bilinear (int new_value)
{
  bilinear = new_value;

  if (gl_is_initialized == 0)
    return;

  if (bilinear)
  {
    glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR (__FILE__, __LINE__);

    glBindTexture (GL_TEXTURE_2D, texgrid->texobj);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }
  else
  {
    glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR (__FILE__, __LINE__);

    glBindTexture (GL_TEXTURE_2D, texgrid->texobj);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }
}


void
gl_set_cabview (int new_value)
{
  cabview = new_value;

  if (gl_is_initialized == 0)
    return;

  if (cabview)
    SetupFrustum ();
  else
    SetupOrtho ();
}

void
gl_set_antialias (int new_value)
{
  antialias = new_value;

  if (gl_is_initialized == 0)
    return;

  if (antialias)
  {
    glShadeModel (GL_SMOOTH);
    glEnable (GL_POLYGON_SMOOTH);
  }
  else
  {
    glShadeModel (GL_FLAT);
    glDisable (GL_POLYGON_SMOOTH);
  }
}

void
gl_set_alphablending (int new_value)
{
  alphablending = new_value;

  if (gl_is_initialized == 0)
    return;

  if (alphablending)
  {
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable (GL_BLEND);
  }
}

static void
gl_calibrate_palette ()
{
  int col, cofs, cofs_orig;
  unsigned char red, green, blue;

  if (gl_is_initialized == 0)
    return;

#ifndef NDEBUG
  /*
  fprintf (stderr, "gl-palette calibrate [0..%d]: \n", totalcolors - 1);
  */
#endif
  for (col = 0; col < totalcolors; col++)
  {
    cofs_orig = col * 3;

    red = ctable_orig[cofs_orig];
    green = ctable_orig[cofs_orig + 1];
    blue = ctable_orig[cofs_orig + 2];

    if (useGlEXT)
    {
      double dAlpha = (double) gl_alpha_value / 255.0;

      if (alphablending)
      {
	cofs = col * 4;
	ctable[cofs] = 255 * pow (red / 255.0, 1 / gamma_correction);

	ctable[cofs + 1] = 255 * pow (green / 255.0, 1 / gamma_correction);

	ctable[cofs + 2] = 255 * pow (blue / 255.0, 1 / gamma_correction);

	ctable[cofs + 3] = gl_alpha_value;
#ifndef NDEBUG
	/*
	fprintf (stderr, "%d (%d/%d/%d/%d), ",
		 col, ctable[cofs], ctable[cofs + 1], ctable[cofs + 2],
		 ctable[cofs + 3]);
	if (col % 3 == 0 || col == totalcolors-1 )
	  fprintf (stderr, "\n");
	*/
#endif
      }
      else
      {
	cofs = col * 3;
	ctable[cofs] =
	  255 * (dAlpha * pow (red / 255.0, 1 / gamma_correction));

	ctable[cofs + 1] =
	  255 * (dAlpha * pow (green / 255.0, 1 / gamma_correction));

	ctable[cofs + 2] =
	  255 * (dAlpha * pow (blue / 255.0, 1 / gamma_correction));
#ifndef NDEBUG
	/*
	fprintf (stderr, "%d (%d/%d/%d), ",
		 col, ctable[cofs], ctable[cofs + 1], ctable[cofs + 2]);
	if (col % 3 == 0 || col == totalcolors-1 )
	  fprintf (stderr, "\n");
	*/
#endif
      }
    }
    else
    {
      if (alphablending)
      {
	cofs = col;
	rcolmap[cofs] = 65535 * pow (red / 255.0, 1 / gamma_correction);

	gcolmap[cofs] = 65535 * pow (green / 255.0, 1 / gamma_correction);

	bcolmap[cofs] = 65535 * pow (blue / 255.0, 1 / gamma_correction);

	acolmap[cofs] = 65535 * (gl_alpha_value / 255.0);
      }
      else
      {
	double dAlpha = (double) gl_alpha_value / 255.0;

	cofs = col;
	rcolmap[cofs] =
	  65535 * (dAlpha * pow (red / 255.0, 1 / gamma_correction));

	gcolmap[cofs] =
	  65535 * (dAlpha * pow (green / 255.0, 1 / gamma_correction));

	bcolmap[cofs] =
	  65535 * (dAlpha * pow (blue / 255.0, 1 / gamma_correction));
      }
    }
  }
}

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

int
sysdep_display_16bpp_capable (void)
{
  return 0; /* direct color  JAU*/
}

int
sysdep_display_alloc_palette (int writable_colors)
{
  char buf[1000];
  int col;

  if (glContext == 0)
    return 1;

  totalcolors = writable_colors;

  if (totalcolors > 256)
  {
    sprintf (buf,
	     "Warning: More than 256 colors (%d) are needed for this emulation,\n%s",
	     totalcolors,
	     "Using the full pallete within OpenGL-test-code ! \n");
    fprintf (stderr, buf);
  }

  /* indexed color ?! JAU */
  if (Machine->scrbitmap->depth != 8 && 
      Machine->scrbitmap->depth != 15 &&
      Machine->scrbitmap->depth != 16)
  {
    int new_deepth = 0;
    if (Machine->scrbitmap->depth < 8)
      new_deepth = 8;
    else if (Machine->scrbitmap->depth < 15)
      new_deepth = 15;
    else
      new_deepth = 16;

    sprintf (buf,
	     "ERROR: %d color-index bitmap deepth impossible for now, using new deepth: %d\n",
	     Machine->scrbitmap->depth, new_deepth);
    fprintf (stderr, buf);
    Machine->scrbitmap->depth = new_deepth;
  }

  /* find a OpenGL compatible colortablesize=2**n */
  ctable_size = 1;
  while (ctable_size < totalcolors)
    ctable_size *= 2;

  /* indexed color ?! JAU */
  if (Machine->scrbitmap->depth == 8)
  {
    gl_bitmap_format = GL_UNSIGNED_BYTE;
    gl_internal_format = GL_COLOR_INDEX8_EXT;
  }
  else
  {
    gl_bitmap_format = GL_UNSIGNED_SHORT;
    gl_internal_format = GL_COLOR_INDEX16_EXT;
  }

  if (useGlEXT == 0)
  {
    if (alphablending)
    {
      gl_internal_format = GL_RGBA;
    }
    else
    {
      gl_internal_format = GL_RGB;
    }
  }

  if (useGlEXT)
  {
    if (alphablending)
      ctable = (GLubyte *) calloc (ctable_size * 4, 1);
    else
      ctable = (GLubyte *) calloc (ctable_size * 3, 1);
  }
  else
  {
    rcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
    gcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
    bcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
    if (alphablending)
      acolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
    else
      acolmap = 0;
  }


  ctable_orig = (unsigned char *) malloc (ctable_size * 3);

#ifndef NDEBUG
  fprintf (stderr, "ctable_size = %d, gl_bitmap_format = %d (0x%X), gl_internal_format, %d (0x%X)\n",
  		ctable_size, gl_bitmap_format, gl_bitmap_format, 
		             gl_internal_format, gl_internal_format);

  fprintf (stderr, "totalcolors = %d, deepth = %d alphablending=%d\n",
	   totalcolors, Machine->scrbitmap->depth, alphablending);

  fprintf (stderr, "useGlEXT = %d\n", useGlEXT);
  fprintf (stderr, "pens[0..%d]: \n", totalcolors - 1);
#endif

  for (col = 0; col < totalcolors; col++)
  {
    sysdep_display_set_pen (col, 0, 0, 0);
  }

  palette_dirty = 1;

  InitTextures ();

  return 0;
}

/* Change the color of a pen */
int
sysdep_display_set_pen (int pen, unsigned char red, unsigned char green,
			unsigned char blue)
{
  int cofs_orig;

  if (glContext == 0)
    return 1;

  /* now we check the true values ;-) JAU */
  if (pen >= ctable_size)
  {
    char buf[1000];
    sprintf (buf,
	     "gl_modify_pen: pen (%d) is greater than colors in colortable(%d)",
	     pen, ctable_size);
    fprintf (stderr, buf);
  }

  cofs_orig = pen * 3;

  ctable_orig[cofs_orig] = red;
  ctable_orig[cofs_orig + 1] = green;
  ctable_orig[cofs_orig + 2] = blue;

  palette_dirty = 1;
  return 0;
}

/* Compute a delta vector between two points */

void
DeltaVec (GLfloat x1, GLfloat y1, GLfloat z1,
	  GLfloat x2, GLfloat y2, GLfloat z2,
	  GLfloat * dx, GLfloat * dy, GLfloat * dz)
{
  *dx = x2 - x1;
  *dy = y2 - y1;
  *dz = z2 - z1;
}

/* Calculate points for flat screen */

void CalcFlatPoint(int x,int y,GLfloat *px,GLfloat *py)
{
  *px=(float)x*texpercx;
  *py=(float)y*texpercy;
}

/* Calculate points for cabinet screen */

void CalcCabPoint(int x,int y,GLfloat *px,GLfloat *py,GLfloat *pz)
{
  GLfloat xperc,yperc;

  xperc=x*texpercx;
  yperc=y*texpercy;

  *px=cscrx1+xperc*cscrwdx+yperc*cscrhdx;
  *py=cscry1+xperc*cscrwdy+yperc*cscrhdy;
  *pz=cscrz1+xperc*cscrwdz+yperc*cscrhdz;
}


/* Set up a frustum projection */

void
SetupFrustum (void)
{
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glFrustum (-vscrnaspect, vscrnaspect, -1.0, 1.0, 5.0, 100.0);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0, 0.0, -20.0);
}


/* Set up an orthographic projection */

void
SetupOrtho (void)
{
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();

  glOrtho (0, 1, 1, 0, -1.0, 1.0);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
}

void
InitTextures ()
{
  int e_x, e_y, s, i=0;
  int x=0, y=0, glerrid, raw_line_len=0;
  GLuint *tobj=0;
  GLint format=0;
  unsigned char *line_1=0, *line_2=0, *mem_start=0;
  struct TexSquare *tsq=0;

  if (glContext == 0)
    return;

  /* lets clean up ... hmmm ... */
  glEnd ();
  glerrid = glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }

  texnumx = 1;
  texnumy = 1;

  if(force_text_width_height>0)
  {
  	text_height=force_text_width_height;
  	text_width=force_text_width_height;
	fprintf (stderr, "force_text_width_height := %d x %d\n",
		text_height, text_width);
  }
  
  #ifndef NDEBUG
	fprintf (stderr, "using texture size %d x %d\n",
		text_height, text_width);
  #endif

  /* achieve the 2**e_x:=text_width, 2**e_y:=text_height */
  e_x=0; s=1;
  while (s<text_width)
  { s*=2; e_x++; }
  text_width=s;

  e_y=0; s=1;
  while (s<text_height)
  { s*=2; e_y++; }
  text_height=s;

  /* Test the max texture size */
  do
  {
    glTexImage2D (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod,
		  gl_internal_format,
		  text_width, text_height,
		  0, GL_COLOR_INDEX, gl_bitmap_format, 0);

    glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod, GL_TEXTURE_INDEX_SIZE_EXT, &format);

    if(format==0 && gl_internal_format!=GL_COLOR_INDEX8_EXT)
    {
      fprintf (stderr, "Your OpenGL implementations does not support\n\tglTexImage2D with internalFormat %d (0x%X)\n",
      		gl_internal_format, gl_internal_format);
      fprintf (stderr, "\t falling back to COLOR_INDEX8_EXT\n");
      gl_internal_format = GL_COLOR_INDEX8_EXT;

      continue;

    } else if(format==0 && gl_bitmap_format!=GL_UNSIGNED_BYTE) {
      fprintf (stderr, "Your OpenGL implementations does not support\n\tglTexImage2D with type %d (0x%X)\n",
      		gl_bitmap_format, gl_bitmap_format);
      fprintf (stderr, "\t falling back to GL_UNSIGNED_BYTE");
      gl_bitmap_format = GL_UNSIGNED_BYTE;

      continue;
      
    } else {
      fprintf (stderr, "Your OpenGL implementations has unknown problems with glTexImage2D\n");
    }

    glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod, GL_TEXTURE_INTERNAL_FORMAT, &format);

    if (format == gl_internal_format &&
	force_text_width_height > 0 &&
	(force_text_width_height < text_width ||
	 force_text_width_height < text_height))
    {
      format = 0;
    }

    if (format != gl_internal_format)
    {
      fprintf (stderr, "Needed texture [%dx%d] too big, trying ",
		text_height, text_width);

      if (text_width > text_height)
      {
	e_x--;
	text_width = 1;
	for (i = e_x; i > 0; i--)
	  text_width *= 2;
      }
      else
      {
	e_y--;
	text_height = 1;
	for (i = e_y; i > 0; i--)
	  text_height *= 2;
      }

      fprintf (stderr, "[%dx%d] !\n", text_height, text_width);
    }
  }
  while (format != gl_internal_format && text_width > 1 && text_height > 1);

  texnumx = visual_width / text_width;
  if ((visual_width % text_width) > 0)
    texnumx++;

  texnumy = visual_height / text_height;
  if ((visual_height % text_height) > 0)
    texnumy++;

  /* Allocate the texture memory */

  xinc = vscrnwidth * ((double) text_width / (double) visual_width);
  yinc = vscrnheight * ((double) text_height / (double) visual_height);

  texpercx = (GLfloat) text_width / (GLfloat) visual_width;
  if (texpercx > 1.0)
    texpercx = 1.0;

  texpercy = (GLfloat) text_height / (GLfloat) visual_height;
  if (texpercy > 1.0)
    texpercy = 1.0;

  texgrid = (struct TexSquare *)
    malloc (texnumx * texnumy * sizeof (struct TexSquare));

  /* some bitmap fix data values .. 
   * independend of the mame bitmap line arrangement !
   */
  bytes_per_pixel = (Machine->scrbitmap->depth+7) / 8;

  line_1 = (unsigned char *) Machine->scrbitmap->line[visual.min_y];
  line_2 = (unsigned char *) Machine->scrbitmap->line[visual.min_y + 1];

  mem_start = (unsigned char *) Machine->scrbitmap->_private;
  raw_line_len = line_2 - line_1;
  memory_x_len = raw_line_len / bytes_per_pixel;

  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      tsq = texgrid + y * texnumx + x;

      if (x == texnumx - 1 && visual_width % text_width)
	tsq->xcov =
	  (GLfloat) (visual_width % text_width) / (GLfloat) text_width;
      else
	tsq->xcov = 1.0;

      if (y == texnumy - 1 && visual_height % text_height)
	tsq->ycov =
	  (GLfloat) (visual_height % text_height) / (GLfloat) text_height;
      else
	tsq->ycov = 1.0;

      CalcCabPoint (x, y, &(tsq->x1), &(tsq->y1), &(tsq->z1));
      CalcCabPoint (x + 1, y, &(tsq->x2), &(tsq->y2), &(tsq->z2));
      CalcCabPoint (x + 1, y + 1, &(tsq->x3), &(tsq->y3), &(tsq->z3));
      CalcCabPoint (x, y + 1, &(tsq->x4), &(tsq->y4), &(tsq->z4));

      CalcFlatPoint (x, y, &(tsq->fx1), &(tsq->fy1));
      CalcFlatPoint (x + 1, y, &(tsq->fx2), &(tsq->fy2));
      CalcFlatPoint (x + 1, y + 1, &(tsq->fx3), &(tsq->fy3));
      CalcFlatPoint (x, y + 1, &(tsq->fx4), &(tsq->fy4));

      /* calculate the pixel store data,
         to use the machine-bitmap for our texture 
      */
      memory_x_start_offset = visual.min_x * bytes_per_pixel + 
                              x * text_width * bytes_per_pixel;

      tsq->texture = line_1 +                           
                     y * text_height * raw_line_len +  
		     memory_x_start_offset;           

      #ifndef NDEBUG
      if (x == 0 && y == 0)
      {
	fprintf (stderr, "useGlEXT = %d\n", useGlEXT);
	fprintf (stderr, "bitmap (w=%d / h=%d / d=%d)\n",
		 Machine->scrbitmap->width, Machine->scrbitmap->height,
		 Machine->scrbitmap->depth);

	fprintf (stderr, "visual (min_x=%d / min_y=%d)\n",
		 visual.min_x, visual.min_y);

	fprintf (stderr, "visual (w=%d / h=%d)\n",
		 visual_width, visual_height);

	fprintf (stderr, "win (w=%d / h=%d)\n", winwidth, winheight);

	fprintf (stderr, "game (w=%d / h=%d)\n",
		 Machine->drv->default_visible_area.max_x -
		 Machine->drv->default_visible_area.min_x + 1,
		 Machine->drv->default_visible_area.max_y -
		 Machine->drv->default_visible_area.min_y + 1);

	fprintf (stderr, "texture-usage %d*width=%d, %d*height=%d\n",
		 (int) texnumx, (int) text_width, (int) texnumy,
		 (int) text_height);

	fprintf (stderr, "texture-usage x=%f%% y=%f%%\n",
		 texpercx, texpercy);

	fprintf (stderr, "texture-usage colors=(%d/%d)\n",
		 (int) totalcolors, (int) ctable_size);

	fprintf (stderr,
		 "row_len=%d, x_ofs=%d, bytes_per_pixel=%d, 1st pix. ofs=%d\n",
		 (int) memory_x_len, (int) memory_x_start_offset,
		 (int) bytes_per_pixel,
		 (int) ((mem_start - (line_1 + memory_x_start_offset)) *
			bytes_per_pixel));
	fflush(stderr);
      }
      fprintf (stderr, "%d/%d: tc (%f/%f),(%f/%f) .. (%f/%f),(%f/%f)\n",
	       x, y,
	       0        , 0        , tsq->xcov, 0,
	       tsq->xcov, tsq->ycov, 0        , tsq->ycov);
      fprintf (stderr, "%d/%d: vc (%f/%f),(%f/%f) .. (%f/%f),(%f/%f)\n",
	       x, y,
	       tsq->fx1, tsq->fy1, tsq->fx2, tsq->fy2, 
	       tsq->fx3, tsq->fy3, tsq->fx4, tsq->fy4);
      fprintf (stderr, "\t texture mem %p\n", tsq->texture);
      fflush(stderr);

      #endif

      tobj = &(tsq->texobj);

      glGenTextures (1, tobj);

      glBindTexture (GL_TEXTURE_2D, *tobj);

      if (useGlEXT)
      {
	if (alphablending)
	{
	  _glColorTableEXT (GL_TEXTURE_2D,
			   GL_RGBA,
			   ctable_size, GL_RGBA, gl_bitmap_format, ctable);
	}
	else
	{
	  _glColorTableEXT (GL_TEXTURE_2D,
			   GL_RGB,
			   ctable_size, GL_RGB, gl_bitmap_format, ctable);
	}

	CHECK_GL_ERROR (__FILE__, __LINE__);
      }
      else
      {
	glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
	glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
	glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
	if (alphablending)
	  glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);
      }

      glTexImage2D (GL_TEXTURE_2D, (GLint)bitmap_lod,
		    gl_internal_format,
		    text_width, text_height,
		    0, GL_COLOR_INDEX, gl_bitmap_format, 0);

      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, gl_white);

      glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    }	/* for all texnumx */
  }  /* for all texnumy */

  gl_is_initialized = 1;

  /* lets init the rest of the custumizings ... */
  gl_set_bilinear (bilinear);
  gl_set_cabview (cabview);
  gl_set_antialias (antialias);
  gl_set_alphablending (alphablending);

  CHECK_GL_ERROR (__FILE__, __LINE__);

}

/* Set up the virtual screen */

static void SetWindowRatio()
{
  scrnaspect = (double) visual_width / (double) visual_height;
  vscrnaspect = (double) winwidth / (double) winheight;

  if (scrnaspect < vscrnaspect)
  {
    vscrnheight = (GLfloat) winheight;
    vscrnwidth = vscrnheight * scrnaspect;
    vscrntlx = ((GLfloat) winwidth - vscrnwidth) / 2.0;
    vscrntly = 0.0;
  }
  else
  {
    vscrnwidth = (GLfloat) winwidth;
    vscrnheight = vscrnwidth / scrnaspect;
    vscrntlx = 0.0;
    vscrntly = ((GLfloat) winheight - vscrnheight) / 2.0;
  }

#ifndef NDEBUG
	fprintf (stderr, "SetWindowRatio: win (%dx%d), visual (%dx%d)\n",
		winwidth, winheight, visual_width, visual_height);
	fprintf (stderr, "\t vscrnwidth %f, vscrnheight %f\n",
		vscrnwidth, vscrnheight);
	fprintf (stderr, "\t x-diff %f, y-diff %f\n",
		(float)winwidth - vscrnwidth, (float)winheight - vscrnheight);
	fprintf (stderr, "\t scrnaspect=%f, vscrnaspect=%f\n",
		scrnaspect, vscrnaspect);
	fflush(stderr);
#endif
}

void xgl_resize(int w, int h)
{
  if (glContext!=NULL)
  {
        winheight = h;
        winwidth =  w;

	SetWindowRatio();

	if (cabview)
		glViewport (0, 0, winwidth, winheight);
	else
		glViewport ((int)(vscrntlx+0.5), (int)(vscrntly+0.5),
		            (int)(vscrnwidth+0.5), (int)(vscrnheight+0.5));

	SetupOrtho ();

	#ifndef NDEBUG
		fprintf(stderr, "RESIZED to %dx%d\n", winwidth, winheight);
		fflush(stderr);
	#endif
  }
}

void
InitVScreen (void)
{
  /* clear the buffer */

  if (glContext == 0)
    return;

  fetchGL_EXT_FUNCS ();

  InitCabGlobals ();

  glClearColor (0, 0, 0, 1);
  glClear (GL_COLOR_BUFFER_BIT);
  glFlush ();

  if (!dodepth)
    cabview = 0;

  currentpan = 0;
  lastpan = 0;
  panframe = 0;

  xgl_resize(winwidth, winheight);

  if (dodepth)
    glDepthFunc (GL_LEQUAL);

#ifdef WIN32
  __try
  {
#endif
    if (!cabspecified || !LoadCabinet (cabname))
    {
      if (cabspecified)
	printf ("Unable to load cabinet %s\n", cabname);
      strcpy (cabname, "glmame");
      cabspecified = 1;
      LoadCabinet (cabname);
      cabload_err = 0;
    }

#ifdef WIN32
  }
  __except (GetExceptionCode ())
  {
    fprintf (stderr, "\nCabinet loading is still buggy - sorry !\n");
    cabload_err = 1;
  }
#endif

  /* Calulate delta vectors for screen height and width */

  DeltaVec (cscrx1, cscry1, cscrz1, cscrx2, cscry2, cscrz2,
	    &cscrwdx, &cscrwdy, &cscrwdz);

  DeltaVec (cscrx1, cscry1, cscrz1, cscrx4, cscry4, cscrz4,
	    &cscrhdx, &cscrhdy, &cscrhdz);


  if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
  {
    printf("vector game, using alphablending ...");
    vecgame = 1;
    alphablending = 1;
  }
  else
    vecgame = 0;

  /* fill the display_palette_info struct */
  memset (&display_palette_info, 0, sizeof (struct sysdep_palette_info));
  display_palette_info.depth = 8;
  display_palette_info.writable_colors = 256;
  /*
  // last indexed color JAU
  display_palette_info.depth = 15;
  display_palette_info.writable_colors = 32768;

  // direct color JAU
  display_palette_info.depth = 16;
  display_palette_info.writable_colors = 65536;
  */
}

/* Close down the virtual screen */

void
CloseVScreen (void)
{
  if (gl_is_initialized == 0)
    return;

  if (ctable)
    free (ctable);
  ctable = 0;
  if (rcolmap)
    free (rcolmap);
  rcolmap = 0;
  if (bcolmap)
    free (bcolmap);
  bcolmap = 0;
  if (gcolmap)
    free (gcolmap);
  gcolmap = 0;
  if (acolmap)
    free (acolmap);
  acolmap = 0;

  if (cpan)
  {
    free (cpan);
    cpan = 0;
  }

  /* Free Texture stuff */

  if (texgrid)
  {
    free (texgrid);
    texgrid = 0;
  }

  gl_is_initialized = 0;
}

/* Not needed under GL */
#ifndef WIN32
void
sysdep_clear_screen (void)
{
}
#endif


/* Compute an average between two sets of 3D coordinates */

void
WAvg (GLfloat perc, GLfloat x1, GLfloat y1, GLfloat z1,
      GLfloat x2, GLfloat y2, GLfloat z2,
      GLfloat * ax, GLfloat * ay, GLfloat * az)
{
  *ax = (1.0 - perc) * x1 + perc * x2;
  *ay = (1.0 - perc) * y1 + perc * y2;
  *az = (1.0 - perc) * z1 + perc * z2;
}

void
drawTextureDisplay (int useCabinet, int updateTexture)
{
  struct TexSquare *square;
  int x, y, wd, ht;
  static const float z_pos = 0.9f;

  if (updateTexture)
  {
    glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    if (!useGlEXT)
      glPixelTransferi (GL_MAP_COLOR, GL_TRUE);
  }

  glEnable (GL_TEXTURE_2D);

  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      square = texgrid + y * texnumx + x;

      glBindTexture (GL_TEXTURE_2D, square->texobj);

      if (palette_changed)
      {

	if (useGlEXT)
	{
	  if (alphablending)
	  {
	    _glColorSubTableEXT (GL_TEXTURE_2D, 0, ctable_size, 
	    		         GL_RGBA, gl_bitmap_format, ctable);
	  }
	  else
	  {
	    _glColorSubTableEXT (GL_TEXTURE_2D, 0, ctable_size, 
	    		         GL_RGB, gl_bitmap_format, ctable);
	  }
	}
	else
	{
	  glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
	  glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
	  glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
	  if (alphablending)
	    glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);
	}

	palette_changed = 0;
      }

      /* This is the quickest way I know of to update the texture */
      if (updateTexture)
      {
	if (x < texnumx - 1)
	  wd = text_width;
	else
	  wd = visual_width - text_width * x;

	if (y < texnumy - 1)
	  ht = text_height;
	else
	  ht = visual_height - text_height * y;

	glTexSubImage2D (GL_TEXTURE_2D, (GLint)bitmap_lod, 
	                 0, 0,
			 wd, ht,
			 GL_COLOR_INDEX, gl_bitmap_format, square->texture);
      }

      if (useCabinet)
      {
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (square->x1, square->y1, square->z1);
	glTexCoord2f (square->xcov, 0);
	glVertex3f (square->x2, square->y2, square->z2);
	glTexCoord2f (square->xcov, square->ycov);
	glVertex3f (square->x3, square->y3, square->z3);
	glTexCoord2f (0, square->ycov);
	glVertex3f (square->x4, square->y4, square->z4);
	glEnd ();
      }
      else
      {
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (square->fx1, square->fy1, z_pos);
	glTexCoord2f (square->xcov, 0);
	glVertex3f (square->fx2, square->fy2, z_pos);
	glTexCoord2f (square->xcov, square->ycov);
	glVertex3f (square->fx3, square->fy3, z_pos);
	glTexCoord2f (0, square->ycov);
	glVertex3f (square->fx4, square->fy4, z_pos);
	glEnd ();
      }
    } /* for all texnumx */
  } /* for all texnumy */

  if (updateTexture)
  {
    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    if (!useGlEXT)
      glPixelTransferi (GL_MAP_COLOR, GL_FALSE);
  }

  glDisable (GL_TEXTURE_2D);
}

/* Draw a frame in Cabinet mode */

void
UpdateCabDisplay (void)
{
  int glerrid;
  GLfloat camx, camy, camz;
  GLfloat dirx, diry, dirz;
  GLfloat normx, normy, normz;
  GLfloat perc;
  struct CameraPan *pan, *lpan;

  if (gl_is_initialized == 0)
    return;

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CHECK_GL_ERROR (__FILE__, __LINE__);

  glPushMatrix ();


  /* Do the camera panning */

  if (cpan)
  {
    pan = cpan + currentpan;

    if (panframe > pan->frames)
    {
      lastpan = currentpan;
      currentpan = (currentpan + 1) % numpans;
      panframe = 0;
    }

    switch (pan->type)
    {
    case pan_goto:
      camx = pan->lx;
      camy = pan->ly;
      camz = pan->lz;
      dirx = pan->px;
      diry = pan->px;
      dirz = pan->pz;
      normx = pan->nx;
      normy = pan->ny;
      normz = pan->nz;
      break;
    case pan_moveto:
      lpan = cpan + lastpan;
      perc = (GLfloat) panframe / (GLfloat) pan->frames;
      WAvg (perc, lpan->lx, lpan->ly, lpan->lz,
	    pan->lx, pan->ly, pan->lz, &camx, &camy, &camz);
      WAvg (perc, lpan->px, lpan->py, lpan->pz,
	    pan->px, pan->py, pan->pz, &dirx, &diry, &dirz);
      WAvg (perc, lpan->nx, lpan->ny, lpan->nz,
	    pan->nx, pan->ny, pan->nz, &normx, &normy, &normz);
      break;
    default:
      break;
    }

    gluLookAt (camx, camy, camz, dirx, diry, dirz, normx, normy, normz);

    panframe++;
  }
  else
    gluLookAt (-5.0, 0.0, 5.0, 0.0, 0.0, -5.0, 0.0, 1.0, 0.0);

  glEnable (GL_DEPTH_TEST);

  /* Draw the cabinet */
  glCallList (cablist);

  /* YES - lets catch this error ... */
  glerrid = glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }

  /* Draw the screen if in vector mode */

  if (vecgame)
  {
    if (drawbitmap)
    {
      /* glClear(GL_DEPTH_BUFFER_BIT);
      // SetupOrtho();
      */

      glColor4f (1.0, 1.0, 1.0, 0.7);

      drawTextureDisplay (1 /*cabinet */ , screendirty);

      /* SetupFrustum();
       */
    }
    glDisable (GL_TEXTURE_2D);
    glShadeModel (GL_FLAT);

    if (antialias)
    {
      glEnable (GL_LINE_SMOOTH);
      glEnable (GL_POINT_SMOOTH);
    }

    glCallList (veclist);

    if (antialias)
    {
      glDisable (GL_LINE_SMOOTH);
      glDisable (GL_POINT_SMOOTH);
    }

    glShadeModel (GL_SMOOTH);
  }
  else
  {				/* Draw the screen of a bitmapped game */

    if (drawbitmap)
	    drawTextureDisplay (1 /*cabinet */ , screendirty);

  }

  glDisable (GL_DEPTH_TEST);

  glPopMatrix ();

  /*
  if (do_snapshot)
  {
    do_save_snapshot ();
    do_snapshot = 0;
  }
  */

  if (doublebuffer)
  {
#ifdef WIN32
    BOOL ret = SwapBuffers (glHDC);
    if (ret != TRUE)
    {
      CHECK_WGL_ERROR (glWnd, __FILE__, __LINE__);
      doublebuffer = FALSE;
    }
#else
    SwapBuffers ();
#endif
  }
  else
    glFlush ();

  CHECK_GL_ERROR (__FILE__, __LINE__);

}

void
UpdateFlatDisplay (void)
{
  int glerrid;

  if (gl_is_initialized == 0)
    return;

  if (!vecgame || !dopersist)
    glClear (GL_COLOR_BUFFER_BIT);

  glPushMatrix ();

  /* lets clean up ... hmmm ... */
  glEnd ();
  glerrid = glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }
  if (dopersist && vecgame)
  {
    glColor4f (0.0, 0.0, 0.0, 0.2);

    glBegin (GL_QUADS);
    glVertex2f (0.0, 0.0);
    glVertex2f ((GLfloat) winwidth, 0.0);
    glVertex2f ((GLfloat) winwidth, (GLfloat) winheight);
    glVertex2f (0.0, (GLfloat) winheight);
    glEnd ();
  }

  CHECK_GL_ERROR (__FILE__, __LINE__);


  /*glColor4f (1.0, 1.0, 1.0, 1.0); */
  glColor4f (1.0, 1.0, 1.0, 0.7);

  if(drawbitmap)
	  drawTextureDisplay (0, screendirty);

  if (vecgame)
  {
    glShadeModel (GL_FLAT);

    if (antialias)
    {
      glEnable (GL_LINE_SMOOTH);
      glEnable (GL_POINT_SMOOTH);
    }

    glCallList (veclist);

    if (antialias)
    {
      glDisable (GL_LINE_SMOOTH);
      glDisable (GL_POINT_SMOOTH);
    }

    glShadeModel (GL_SMOOTH);

    /* YES - lets catch this error ... 
     */
    glerrid = glGetError ();
    if (0x502 != glerrid)
    {
      CHECK_GL_ERROR (__FILE__, __LINE__);
    }
  }

  glPopMatrix ();

  CHECK_GL_ERROR (__FILE__, __LINE__);

  /*
  if (do_snapshot)
  {
    do_save_snapshot ();
    do_snapshot = 0;
  }
  */

  if (doublebuffer)
  {
#ifdef WIN32
    BOOL ret = SwapBuffers (glHDC);
    if (ret != TRUE)
    {
      CHECK_WGL_ERROR (glWnd, __FILE__, __LINE__);
      doublebuffer = FALSE;
    }
#else
    SwapBuffers ();
#endif
  }
  else
    glFlush ();
}

void
UpdateGLDisplay (struct osd_bitmap *bitmap)
{
  int glerrid;

  if (gl_is_initialized == 0)
    return;

  if (vecgame && inlist)
  {
    vector_vh_update (NULL, 0);

    /* YES - lets catch this error ... 
       the upper function calls glEnd at the end, tsts */
    glerrid = glGetError ();
    if (0x502 != glerrid)
    {
      CHECK_GL_ERROR (__FILE__, __LINE__);
    }
  }

  if (palette_dirty)
  {
    gl_calibrate_palette ();
    palette_dirty = 0;
    palette_changed = 1;
  }

  if (cabview && !cabload_err)
    UpdateCabDisplay ();
  else
    UpdateFlatDisplay ();


  CHECK_GL_ERROR (__FILE__, __LINE__);

  if (vecgame && inlist)
  {
    vector_clear_list ();
    /* YES - lets catch this error ... 
       the upper function calls glBegin at the end, tsts */
    glerrid = glGetError ();
    if (0x502 != glerrid)
    {
      CHECK_GL_ERROR (__FILE__, __LINE__);
    }
  }

  /* screendirty = 0; */
}

/* used when expose events received */

void
osd_refresh_screen (void)
{
  /* Just re-draw the whole screen */
  UpdateGLDisplay (NULL);
}

/* invoked by main tree code to update bitmap into screen */

void
sysdep_update_display (struct osd_bitmap *bitmap)
{
  int x, y;

  UpdateGLDisplay (bitmap);

  frame++;

  if (keyboard_pressed (KEYCODE_RCONTROL))
  {
#if 0
    if (keyboard_pressed_memory (KEYCODE_A))
      antialias = 1 - antialias;
#endif
    if (keyboard_pressed_memory (KEYCODE_C) && dodepth)
    {
      cabview = 1 - cabview;

      if (cabview)
	SetupFrustum ();
      else
	SetupOrtho ();
    }
    if (keyboard_pressed_memory (KEYCODE_B))
    {
      bilinear = 1 - bilinear;

      if (bilinear)
      {
	glBindTexture (GL_TEXTURE_2D, cabtex[0]);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (y = 0; y < texnumy; y++)
	{
	  for (x = 0; x < texnumx; x++)
	  {
	    glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  }
	}
      }
      else
      {
	glBindTexture (GL_TEXTURE_2D, cabtex[0]);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (y = 0; y < texnumy; y++)
	{
	  for (x = 0; x < texnumx; x++)
	  {
	    glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			     GL_NEAREST);
	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			     GL_NEAREST);
	  }
	}
      }

    }
#if 0
    if (keyboard_pressed_memory (KEYCODE_O))
      drawbitmap = 1 - drawbitmap;
#endif
    if (keyboard_pressed_memory (KEYCODE_T))
      dopersist = 1 - dopersist;
  }
}

void
osd_mark_dirty (int x1, int y1, int x2, int y2)
{
  screendirty = 1;
}

#endif /* ifdef xgl */
