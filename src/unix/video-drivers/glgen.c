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
void WAvg (GLfloat perc, GLfloat x1, GLfloat y1, GLfloat z1,
	   GLfloat x2, GLfloat y2, GLfloat z2,
	   GLfloat * ax, GLfloat * ay, GLfloat * az);
void UpdateCabDisplay (void);
void UpdateFlatDisplay (void);
void UpdateGLDisplay (struct osd_bitmap *bitmap);

int cabspecified;

double scrnaspect, vscrnaspect;

GLubyte *ctable;
GLushort *rcolmap, *gcolmap, *bcolmap, *acolmap;
unsigned char *ctable_orig;	/* for original return 2 game values */
int ctable_size;		/* the true color table size */

GLenum	   gl_bitmap_format;    /* only GL_UNSIGNED_BYTE and GL_UNSIGNED_SHORT is supported */
GLint      gl_internal_format;
GLsizei text_width;
GLsizei text_height;

int force_text_width_height;

static int palette_changed;

/* int cabview=0;  .. Do we want a cabinet view or not? */
int cabload_err;

int drawbitmap;
int dopersist;
int screendirty;		/* Has the bitmap been modified since the last frame? */
int dodepth;

int palette_dirty;
int totalcolors;
unsigned char gl_alpha_value;
static int frame;

static GLint gl_white[] = { 1, 1, 1, 1};
static GLint gl_black[] = { 0, 0, 0, 0};

/* The squares that are tiled to make up the game screen polygon */

struct TexSquare
{
  GLubyte *texture;
  GLuint texobj;
  GLboolean isTexture;
  GLfloat x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
  GLfloat fx1, fy1, fx2, fy2, fx3, fy3, fx4, fy4;
  GLfloat xcov, ycov;
};

static struct TexSquare *texgrid;

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

int gl_is_initialized;
GLboolean useGlEXT;

void gl_bootstrap_resources()
{
  #ifndef NDEBUG
    printf("GLINFO: gl_bootstrap_resources\n");
  #endif

  cabspecified = 0;
  if(cabname!=NULL) cabname[0]=0;

  scrnaspect=0; vscrnaspect=0;
  ctable = 0;
  rcolmap = 0; gcolmap = 0; bcolmap = 0; acolmap = 0;
  ctable_orig = 0;
  ctable_size=0;
  gl_bitmap_format=0;
  gl_internal_format=0;
  text_width=0;
  text_height=0;
  force_text_width_height = 0;
  palette_changed = 0;
  cabload_err=0;
  drawbitmap = 1;
  dopersist = 0;
  screendirty = 1;
  dodepth = 1;
  palette_dirty = 0;
  totalcolors=0;
  gl_alpha_value = 255;
  frame = 0;

  texnumx=0;
  texnumy=0;
  texpercx=0;
  texpercy=0;
  vscrntlx=0;
  vscrntly=0;
  vscrnwidth=0;
  vscrnheight=0;
  xinc=0; yinc=0;
  
  memory_x_len=0;;
  memory_x_start_offset=0;;
  bytes_per_pixel=0;
  
  cscrx1=0; cscry1=0; cscrz1=0; cscrx2=0; cscry2=0; cscrz2=0;
  cscrx3=0; cscry3=0; cscrz3=0; cscrx4=0; cscry4=0; cscrz4=0;
  cscrwdx=0; cscrwdy=0; cscrwdz=0;
  cscrhdx=0; cscrhdy=0; cscrhdz=0;
  
  cabimg[0]=0;
  cabtex[0]=0;
  cpan=0;
  numpans=0;
  currentpan = 0;
  lastpan = 0;
  panframe = 0;
  
  vecgame = 0;
  veclist=0;
  inlist=0;

  gl_is_initialized = 0;
  useGlEXT = GL_TRUE;
}

void gl_reset_resources()
{
  #ifndef NDEBUG
    printf("GLINFO: gl_reset_resources\n");
  #endif

  if(ctable) free(ctable);
  if(rcolmap) free(rcolmap);
  if(gcolmap) free(gcolmap);
  if(bcolmap) free(bcolmap);
  if(acolmap) free(acolmap);
  if(ctable_orig) free(ctable_orig);
  if(texgrid) free(texgrid);
  if(cpan) free(cpan);

  ctable = 0;
  rcolmap = 0; gcolmap = 0; bcolmap = 0; acolmap = 0;
  ctable_orig = 0;
  texgrid = 0;
  cpan=0;

  scrnaspect=0; vscrnaspect=0;
  ctable_size=0;
  gl_bitmap_format=0;
  gl_internal_format=0;
  palette_changed = 0;
  cabload_err=0;
  drawbitmap = 1;
  dopersist = 0;
  screendirty = 1;
  dodepth = 1;
  palette_dirty = 0;
  gl_alpha_value = 255;
  frame = 0;

  texnumx=0;
  texnumy=0;
  texpercx=0;
  texpercy=0;
  vscrntlx=0;
  vscrntly=0;
  vscrnwidth=0;
  vscrnheight=0;
  xinc=0; yinc=0;
  
  memory_x_len=0;;
  memory_x_start_offset=0;;
  bytes_per_pixel=0;
  
  cscrx1=0; cscry1=0; cscrz1=0; cscrx2=0; cscry2=0; cscrz2=0;
  cscrx3=0; cscry3=0; cscrz3=0; cscrx4=0; cscry4=0; cscrz4=0;
  cscrwdx=0; cscrwdy=0; cscrwdz=0;
  cscrhdx=0; cscrhdy=0; cscrhdz=0;
  
  cabimg[0]=0;
  cabtex[0]=0;
  cpan=0;
  numpans=0;
  currentpan = 0;
  lastpan = 0;
  panframe = 0;
  
  vecgame = 0;
  veclist=0;
  inlist=0;

  if(cabname!=NULL && strlen(cabname)>0)
  	cabspecified = 1;

  gl_is_initialized = 0;
}

/* ---------------------------------------------------------------------- */
/* ------------ New OpenGL Specials ------------------------------------- */
/* ---------------------------------------------------------------------- */

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

GLboolean glHasEXT (void)
{
  if (gl_is_initialized)
    fetch_GL_FUNCS (0);
  return __glColorTableEXT != NULL;
}

void
glSetUseEXT (GLboolean val)
{
  if (gl_is_initialized == 0 || val==GL_FALSE || 
      (__glColorTableEXT!=NULL && __glColorSubTableEXT!=NULL) )
    useGlEXT = val;
}

GLboolean
glGetUseEXT (void)
{
  return useGlEXT;
}

void
gl_set_bilinear (int new_value)
{
  int x, y;
  bilinear = new_value;

  if (gl_is_initialized == 0)
    return;

  if (bilinear)
  {
    __glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    __glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    __glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR (__FILE__, __LINE__);

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        __glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

        __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        __glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        __glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
        CHECK_GL_ERROR (__FILE__, __LINE__);
      }
    }
  }
  else
  {
    __glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    __glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    __glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR (__FILE__, __LINE__);

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        __glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

        __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        __glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        __glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
        CHECK_GL_ERROR (__FILE__, __LINE__);
      }
    }
  }
}


void
gl_set_cabview (int new_value)
{
  if (glContext == 0)
    return;

  cabview = new_value;

  if (cabview)
  {
        currentpan=0;
	currentpan = 0;
	lastpan = 0;
	panframe = 0;

	#ifdef WIN32
	  __try
	  {
	#endif
	    if (!cabspecified || !LoadCabinet (cabname))
	    {
	      if (cabspecified)
		printf ("GLERROR: Unable to load cabinet %s\n", cabname);
	      strcpy (cabname, "glmamejau");
	      cabspecified = 1;
	      LoadCabinet (cabname);
	      cabload_err = 0;
	    }

	#ifdef WIN32
	  }
	  __except (GetExceptionCode ())
	  {
	    fprintf (stderr, "\nGLERROR: Cabinet loading is still buggy - sorry !\n");
	    cabload_err = 1;
	  }
	#endif

	SetupFrustum ();
  } else {
        InitCabGlobals();
	SetupOrtho ();
  }
}

void
gl_set_antialias (int new_value)
{
  antialias = new_value;

  if (gl_is_initialized == 0)
    return;

  if (antialias)
  {
    __glShadeModel (GL_SMOOTH);
    __glEnable (GL_POLYGON_SMOOTH);
    __glEnable (GL_LINE_SMOOTH);
    __glEnable (GL_POINT_SMOOTH);
  }
  else
  {
    __glShadeModel (GL_FLAT);
    __glDisable (GL_POLYGON_SMOOTH);
    __glDisable (GL_LINE_SMOOTH);
    __glDisable (GL_POINT_SMOOTH);
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
    __glEnable (GL_BLEND);
    __glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    __glDisable (GL_BLEND);
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
  #ifndef NDEBUG
    printf("GLINFO: sysdep_display_16bpp_capable\n");
  #endif

  return 0; /* direct color  JAU*/
}

int
sysdep_display_alloc_palette (int writable_colors)
{
  char buf[1000];
  int col;

  #ifndef NDEBUG
    printf("GLINFO: sysdep_display_alloc_palette (glContext=%p, then .. InitTexture)\n", glContext);
  #endif

  if (glContext == 0)
    return 1;

  totalcolors = writable_colors;

  if (totalcolors > 256)
  {
    sprintf (buf,
	     "GLWarning: More than 256 colors (%d) are needed for this emulation,\n%s",
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
	     "GLERROR: %d color-index bitmap deepth impossible for now, using new deepth: %d\n",
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

  if (!useGlEXT)
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
  fprintf (stderr, "GLINFO: ctable_size = %d, gl_bitmap_format = %d (0x%X), gl_internal_format, %d (0x%X)\n",
  		ctable_size, gl_bitmap_format, gl_bitmap_format, 
		             gl_internal_format, gl_internal_format);
#endif
  fprintf (stderr, "GLINFO: totalcolors = %d, deepth = %d alphablending=%d\n",
	   totalcolors, Machine->scrbitmap->depth, alphablending);

  if (__glColorTableEXT == 0)
    printf("GLINFO: glColorTableEXT not avaiable .. BAD & SLOW\n");
  else
    printf("GLINFO: glColorTableEXT avaiable .. GOOD & FAST\n");

  if (__glColorSubTableEXT == 0)
    printf("GLINFO: glColorSubTableEXT not avaiable .. BAD & SLOW\n");
  else
    printf("GLINFO: glColorSubTableEXT avaiable .. GOOD & FAST\n");

  fprintf (stderr, "GLINFO: useGlEXT = %d", useGlEXT);
  if(useGlEXT)
	  fprintf (stderr, " (GOOD & FAST)\n");
  else
	  fprintf (stderr, " (BAD & SLOW)\n");

  fprintf (stderr, "GLINFO: texture-usage colors=(%d/%d)\n",
		 (int) totalcolors, (int) ctable_size);

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
	     "GLINFO: gl_modify_pen: pen (%d) is greater than colors in colortable(%d)",
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
  if(*px>1.0) *px=1.0;
  *py=(float)y*texpercy;
  if(*py>1.0) *py=1.0;
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
  __glMatrixMode (GL_PROJECTION);
  __glLoadIdentity ();
  __glFrustum (-vscrnaspect, vscrnaspect, -1.0, 1.0, 5.0, 100.0);
  __glMatrixMode (GL_MODELVIEW);
  __glLoadIdentity ();
  __glTranslatef (0.0, 0.0, -20.0);
}


/* Set up an orthographic projection */

void
SetupOrtho (void)
{
  __glMatrixMode (GL_PROJECTION);
  __glLoadIdentity ();

  __glOrtho (0, 1, 1, 0, -1.0, 1.0);

  __glMatrixMode (GL_MODELVIEW);
  __glLoadIdentity ();
}

void
InitTextures ()
{
  int e_x, e_y, s, i=0;
  int x=0, y=0, glerrid, raw_line_len=0;
  GLint format=0;
  GLenum err;
  unsigned char *line_1=0, *line_2=0, *mem_start=0;
  struct TexSquare *tsq=0;

  if (glContext == 0)
    return;

  /* lets clean up ... hmmm ... */
  __glEnd ();
  glerrid = __glGetError ();
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
	fprintf (stderr, "GLINFO: force_text_width_height := %d x %d\n",
		text_height, text_width);
  }
  
  fprintf (stderr, "GLINFO: using texture size %d x %d\n",
	text_height, text_width);

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
    __glTexImage2D (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod,
		  gl_internal_format,
		  text_width, text_height,
		  0, GL_COLOR_INDEX, gl_bitmap_format, 0);

    CHECK_GL_ERROR (__FILE__, __LINE__);

    /* JAU: should be done later for > 8bpp color index ... 
     *
    __glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod, GL_TEXTURE_INDEX_SIZE_EXT, &format);

    CHECK_GL_ERROR (__FILE__, __LINE__);

    if(format==0 && gl_internal_format!=GL_COLOR_INDEX8_EXT)
    {
      fprintf (stderr, "GLINFO: Your OpenGL implementations does not support\n\tglTexImage2D with internalFormat %d (0x%X)\n",
      		gl_internal_format, gl_internal_format);
      fprintf (stderr, "\t falling back to COLOR_INDEX8_EXT\n");
      gl_internal_format = GL_COLOR_INDEX8_EXT;

      continue;

    } else if(format==0 && gl_bitmap_format!=GL_UNSIGNED_BYTE) {
      fprintf (stderr, "GLINFO: Your OpenGL implementations does not support\n\tglTexImage2D with type %d (0x%X)\n",
      		gl_bitmap_format, gl_bitmap_format);
      fprintf (stderr, "\t falling back to GL_UNSIGNED_BYTE");
      gl_bitmap_format = GL_UNSIGNED_BYTE;

      continue;
      
    } else {
      fprintf (stderr, "GLINFO: Your OpenGL implementations has unknown problems with glTexImage2D\n");
    }
    */

    __glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, (GLint)bitmap_lod, GL_TEXTURE_INTERNAL_FORMAT, &format);

    CHECK_GL_ERROR (__FILE__, __LINE__);

    if (format == gl_internal_format &&
	force_text_width_height > 0 &&
	(force_text_width_height < text_width ||
	 force_text_width_height < text_height))
    {
      format = 0;
    }

    if (format != gl_internal_format)
    {
      fprintf (stderr, "GLINFO: Needed texture [%dx%d] too big, trying ",
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

  fprintf (stderr, "GLINFO: texture-usage %d*width=%d, %d*height=%d\n",
		 (int) texnumx, (int) text_width, (int) texnumy,
		 (int) text_height);

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

	fprintf (stderr, "texture-usage x=%f%% y=%f%%\n",
		 texpercx, texpercy);

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

      tsq->isTexture=GL_FALSE;
      tsq->texobj=0;

      __glGenTextures (1, &(tsq->texobj));

      __glBindTexture (GL_TEXTURE_2D, tsq->texobj);
      err = __glGetError ();
      if(err==GL_INVALID_ENUM)
      {
	fprintf (stderr, "GLERROR glBindTexture (glGenText) := GL_INVALID_ENUM, texnum x=%d, y=%d, texture=%d\n", x, y, tsq->texobj);
      } 
      #ifndef NDEBUG
	      else if(err==GL_INVALID_OPERATION) {
		fprintf (stderr, "GLERROR glBindTexture (glGenText) := GL_INVALID_OPERATION, texnum x=%d, y=%d, texture=%d\n", x, y, tsq->texobj);
	      }
      #endif

      if(__glIsTexture(tsq->texobj) == GL_FALSE)
      {
	fprintf (stderr, "GLERROR ain't a texture (glGenText): texnum x=%d, y=%d, texture=%d\n",
		x, y, tsq->texobj);
      }

      tsq->isTexture=GL_TRUE;

      CHECK_GL_ERROR (__FILE__, __LINE__);

      if (useGlEXT)
      {
	if (alphablending)
	{
	  __glColorTableEXT (GL_TEXTURE_2D,
			   GL_RGBA,
			   ctable_size, GL_RGBA, GL_UNSIGNED_BYTE, ctable);
	}
	else
	{
	  __glColorTableEXT (GL_TEXTURE_2D,
			   GL_RGB,
			   ctable_size, GL_RGB, GL_UNSIGNED_BYTE, ctable);
	}

	CHECK_GL_ERROR (__FILE__, __LINE__);
      }
      else
      {
	__glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
	__glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
	__glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
	if (alphablending)
	  __glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);

        CHECK_GL_ERROR (__FILE__, __LINE__);

      }

      __glTexImage2D (GL_TEXTURE_2D, (GLint)bitmap_lod,
		    gl_internal_format,
		    text_width, text_height,
		    0, GL_COLOR_INDEX, gl_bitmap_format, 0);

      CHECK_GL_ERROR (__FILE__, __LINE__);

      __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      __glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      __glTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, gl_white);

      __glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      CHECK_GL_ERROR (__FILE__, __LINE__);
    }	/* for all texnumx */
  }  /* for all texnumy */

  gl_is_initialized = 1;

  /* lets init the rest of the custumizings ... */
  gl_set_bilinear (bilinear);
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
	fprintf (stderr, "GLINFO: SetWindowRatio: win (%dx%d), visual (%dx%d)\n",
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

void xgl_fixaspectratio(int *w, int *h)
{
	vscrnaspect = (double) *w / (double) *h;

	if (scrnaspect < vscrnaspect)
		*w = *h * scrnaspect;
	else
		*h= *w / scrnaspect;
}

void xgl_resize(int w, int h)
{
  if (glContext!=NULL)
  {
	winheight= h;
  	winwidth = w;

	SetWindowRatio();

	if (cabview)
		__glViewport (0, 0, winwidth, winheight);
	else
		__glViewport ((int)(vscrntlx+0.5), (int)(vscrntly+0.5),
		            (int)(vscrnwidth+0.5), (int)(vscrnheight+0.5));

	if (cabview)
		SetupFrustum ();
	else
		SetupOrtho ();

	fprintf(stderr, "GLINFO: xgl_resize to %dx%d\n", winwidth, winheight);
	fflush(stderr);
  }
}

void
InitVScreen (void)
{
  int _cabview;

  printf("GLINFO: InitVScreen (glContext=%p)\n", glContext);

  gl_reset_resources();

  /* clear the buffer */

  if (glContext == 0)
    return;

  fetch_GL_FUNCS (0);
  glSetUseEXT (useGlEXT);

  printf("\nGLINFO: OpenGL Driver Information:\n");
  printf("\n\tvendor: %s,\n\trenderer %s,\n\tversion %s\n", 
  	__glGetString(GL_VENDOR), 
	__glGetString(GL_RENDERER),
	__glGetString(GL_VERSION));

  printf("\nGLINFO: GLU Driver Information:\n");
  printf("\n\tversion %s\n\n", 
	__gluGetString(GLU_VERSION));

  InitCabGlobals ();

  __glClearColor (0, 0, 0, 1);
  __glClear (GL_COLOR_BUFFER_BIT);
  __glFlush ();

  if (!dodepth)
    cabview = 0;

  /* forgive me, dirty dirty hack ... */
  _cabview = cabview;

  gl_set_cabview (1);

  cabview = _cabview;

  xgl_resize(winwidth, winheight);

  if (dodepth)
    __glDepthFunc (GL_LEQUAL);

  /* Calulate delta vectors for screen height and width */

  DeltaVec (cscrx1, cscry1, cscrz1, cscrx2, cscry2, cscrz2,
	    &cscrwdx, &cscrwdy, &cscrwdz);

  DeltaVec (cscrx1, cscry1, cscrz1, cscrx4, cscry4, cscrz4,
	    &cscrhdx, &cscrhdy, &cscrhdz);


  if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    vecgame = 1;
  else
    vecgame = 0;

  /* fill the display_palette_info struct */
  memset (&display_palette_info, 0, sizeof (struct sysdep_palette_info));
  display_palette_info.depth = 8;
  display_palette_info.writable_colors = 256;

  /*
      display_palette_info.red_mask   = xvisual->red_mask;
      display_palette_info.green_mask = xvisual->green_mask;
      display_palette_info.blue_mask  = xvisual->blue_mask;
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
  printf("GLINFO: CloseVScreen (gl_is_initialized=%d)\n", gl_is_initialized);

  if (gl_is_initialized == 0)
    return;

  gl_reset_resources();
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
  GLenum err;
  static const float z_pos = 0.9f;

  if(gl_is_initialized == 0)
  	return;

  if (updateTexture)
  {
    __glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    __glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    if (!useGlEXT)
      __glPixelTransferi (GL_MAP_COLOR, GL_TRUE);
  }

  __glEnable (GL_TEXTURE_2D);

  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      square = texgrid + y * texnumx + x;

      if(square->isTexture==GL_FALSE)
      {
	#ifndef NDEBUG
	  fprintf (stderr, "GLINFO ain't a texture(update): texnum x=%d, y=%d, texture=%d\n",
	  	x, y, square->texobj);
	#endif
      	continue;
      }

      __glBindTexture (GL_TEXTURE_2D, square->texobj);
      err = __glGetError ();
      if(err==GL_INVALID_ENUM)
      {
	fprintf (stderr, "GLERROR glBindTexture := GL_INVALID_ENUM, texnum x=%d, y=%d, texture=%d\n", x, y, square->texobj);
      }
      #ifndef NDEBUG
	      else if(err==GL_INVALID_OPERATION) {
		fprintf (stderr, "GLERROR glBindTexture := GL_INVALID_OPERATION, texnum x=%d, y=%d, texture=%d\n", x, y, square->texobj);
	      }
      #endif

      if(__glIsTexture(square->texobj) == GL_FALSE)
      {
        square->isTexture=GL_FALSE;
	fprintf (stderr, "GLERROR ain't a texture(update): texnum x=%d, y=%d, texture=%d\n",
		x, y, square->texobj);
      }

      if (palette_changed)
      {

	if (useGlEXT)
	{
	  if (alphablending)
	  {
	    /*  .. too buggy ?
	     __glColorSubTableEXT (GL_TEXTURE_2D, 0, ctable_size,
	    		         GL_RGBA, GL_UNSIGNED_BYTE, ctable);
             */
	    __glColorTableEXT (GL_TEXTURE_2D, GL_RGBA,
			       ctable_size, GL_RGBA, GL_UNSIGNED_BYTE, ctable);
	  }
	  else
	  {
	    /* .. too buggy ?
	    __glColorSubTableEXT (GL_TEXTURE_2D, 0, ctable_size, 
	    		         GL_RGB, GL_UNSIGNED_BYTE, ctable);
	     */
	    __glColorTableEXT (GL_TEXTURE_2D, GL_RGB,
			       ctable_size, GL_RGB, GL_UNSIGNED_BYTE, ctable);
	  }
	}
	else
	{
	  __glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
	  __glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
	  __glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
	  if (alphablending)
	    __glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);
	}
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

	__glTexSubImage2D (GL_TEXTURE_2D, (GLint)bitmap_lod, 
	                 0, 0,
			 wd, ht,
			 GL_COLOR_INDEX, gl_bitmap_format, square->texture);
      }

      if (useCabinet)
      {
	__glBegin (GL_QUADS);
	__glTexCoord2f (0, 0);
	__glVertex3f (square->x1, square->y1, square->z1);
	__glTexCoord2f (square->xcov, 0);
	__glVertex3f (square->x2, square->y2, square->z2);
	__glTexCoord2f (square->xcov, square->ycov);
	__glVertex3f (square->x3, square->y3, square->z3);
	__glTexCoord2f (0, square->ycov);
	__glVertex3f (square->x4, square->y4, square->z4);
	__glEnd ();
      }
      else
      {
	__glBegin (GL_QUADS);
	__glTexCoord2f (0, 0);
	__glVertex3f (square->fx1, square->fy1, z_pos);
	__glTexCoord2f (square->xcov, 0);
	__glVertex3f (square->fx2, square->fy2, z_pos);
	__glTexCoord2f (square->xcov, square->ycov);
	__glVertex3f (square->fx3, square->fy3, z_pos);
	__glTexCoord2f (0, square->ycov);
	__glVertex3f (square->fx4, square->fy4, z_pos);
	__glEnd ();
      }
    } /* for all texnumx */
  } /* for all texnumy */

  if (updateTexture)
  {
    __glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    __glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    if (!useGlEXT)
      __glPixelTransferi (GL_MAP_COLOR, GL_FALSE);
  }

  __glDisable (GL_TEXTURE_2D);

  palette_changed = 0;

  /* YES - lets catch this error ... 
   */
  (void) __glGetError ();
}

/* Draw a frame in Cabinet mode */

void
UpdateCabDisplay (void)
{
  int glerrid;
  int shadeModel;
  GLfloat camx, camy, camz;
  GLfloat dirx, diry, dirz;
  GLfloat normx, normy, normz;
  GLfloat perc;
  struct CameraPan *pan, *lpan;

  if (gl_is_initialized == 0)
    return;

  __glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CHECK_GL_ERROR (__FILE__, __LINE__);

  __glPushMatrix ();


  /* Do the camera panning */

  if (cpan)
  {
    pan = cpan + currentpan;

    if (panframe > pan->frames)
    {
      /*
       printf("GLINFO (glcab): finished pan %d/%d\n", currentpan, numpans);
      */
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

    __gluLookAt (camx, camy, camz, dirx, diry, dirz, normx, normy, normz);

    panframe++;
  }
  else
    __gluLookAt (-5.0, 0.0, 5.0, 0.0, 0.0, -5.0, 0.0, 1.0, 0.0);

  __glEnable (GL_DEPTH_TEST);

  /* Draw the cabinet */
  __glCallList (cablist);

  /* YES - lets catch this error ... */
  glerrid = __glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }

  /* Draw the screen if in vector mode */

  if (vecgame)
  {
    if (drawbitmap)
    {
      __glColor4f (1.0, 1.0, 1.0, 1.0);

      drawTextureDisplay (1 /*cabinet */ , screendirty);

      /* SetupFrustum();
       */
    }
    __glDisable (GL_TEXTURE_2D);
    __glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    __glShadeModel (GL_FLAT);

    if (antialias)
    {
      __glEnable (GL_LINE_SMOOTH);
      __glEnable (GL_POINT_SMOOTH);
    }

    __glCallList (veclist);

    if (antialias)
    {
      __glDisable (GL_LINE_SMOOTH);
      __glDisable (GL_POINT_SMOOTH);
    }

    __glShadeModel (shadeModel);
  }
  else
  {				/* Draw the screen of a bitmapped game */

    if (drawbitmap)
	    drawTextureDisplay (1 /*cabinet */ , screendirty);

  }

  __glDisable (GL_DEPTH_TEST);

  __glPopMatrix ();

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
    __glFlush ();

  CHECK_GL_ERROR (__FILE__, __LINE__);

}

void
UpdateFlatDisplay (void)
{
  int glerrid;
  int shadeModel;

  if (gl_is_initialized == 0)
    return;

  if (!vecgame || !dopersist)
    __glClear (GL_COLOR_BUFFER_BIT);

  __glPushMatrix ();

  /* lets clean up ... hmmm ... */
  __glEnd ();
  glerrid = __glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR (__FILE__, __LINE__);
  }

  if (dopersist && vecgame)
  {
    __glColor4f (0.0, 0.0, 0.0, 0.2);

    __glBegin (GL_QUADS);
    __glVertex2f (0.0, 0.0);
    __glVertex2f ((GLfloat) winwidth, 0.0);
    __glVertex2f ((GLfloat) winwidth, (GLfloat) winheight);
    __glVertex2f (0.0, (GLfloat) winheight);
    __glEnd ();
  }

  CHECK_GL_ERROR (__FILE__, __LINE__);


  __glColor4f (1.0, 1.0, 1.0, 1.0);

  if(drawbitmap)
	  drawTextureDisplay (0, screendirty);

  if (vecgame)
  {
    __glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    __glShadeModel (GL_FLAT);

    if (antialias)
    {
      __glEnable (GL_LINE_SMOOTH);
      __glEnable (GL_POINT_SMOOTH);
    }

    __glCallList (veclist);

    if (antialias)
    {
      __glDisable (GL_LINE_SMOOTH);
      __glDisable (GL_POINT_SMOOTH);
    }

    __glShadeModel (shadeModel);

  }

  /* YES - lets catch this error ... 
   */
  (void) __glGetError ();

  __glPopMatrix ();

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
    __glFlush ();
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
    glerrid = __glGetError ();
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
    glerrid = __glGetError ();
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
  UpdateGLDisplay (bitmap);

  frame++;

  if (keyboard_pressed (KEYCODE_RALT))
  {
    if (keyboard_pressed_memory (KEYCODE_A))
    {
	  gl_set_antialias (1-antialias);
          printf("GLINFO: switched antialias := %d\n", antialias);
    }
    else if (keyboard_pressed_memory (KEYCODE_B))
    {
      gl_set_bilinear (1 - bilinear);
      printf("GLINFO: switched bilinear := %d\n", bilinear);
    }
    else if (keyboard_pressed_memory (KEYCODE_C) && dodepth)
    {
      gl_set_cabview (1-cabview);
      printf("GLINFO: switched cabinet := %d\n", cabview);
    }
    else if (keyboard_pressed_memory (KEYCODE_F) && dodepth)
    {
	switch2Fullscreen();
    }
    else if (keyboard_pressed_memory (KEYCODE_O))
    {
      drawbitmap = 1 - drawbitmap;
      printf("GLINFO: switched drawbitmap := %d\n", drawbitmap);
    }
    else if (keyboard_pressed_memory (KEYCODE_T))
    {
      dopersist = 1 - dopersist;
      drawbitmap = 1 - dopersist;
      printf("GLINFO: switched dopersist := %d (drawbitmap:=%d) \n", dopersist, drawbitmap);
    }
    else if (keyboard_pressed_memory (KEYCODE_PLUS_PAD))
    {
	set_gl_beam(get_gl_beam()+0.5);
    }
    else if (keyboard_pressed_memory (KEYCODE_MINUS_PAD))
    {
	set_gl_beam(get_gl_beam()-0.5);
    }
  }
}

void
osd_mark_dirty (int x1, int y1, int x2, int y2)
{
  screendirty = 1;
}

#endif /* ifdef xgl */
