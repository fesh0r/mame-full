/*****************************************************************

  Generic OpenGL routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifdef xgl

#include "xmame.h"
#include "driver.h"
#include "glmame.h"
#include <math.h>
#include "effect.h"

int LoadCabinet (const char *fname);
void SwapBuffers (void);
void vector_vh_update (struct mame_bitmap *bitmap, int full_refresh);
void vector_clear_list (void);


void DeltaVec (GLfloat x1, GLfloat y1, GLfloat z1,
	       GLfloat x2, GLfloat y2, GLfloat z2,
	       GLfloat * dx, GLfloat * dy, GLfloat * dz);
void CalcFlatPoint (int x, int y, GLfloat * px, GLfloat * py);
void CalcCabPoint (int x, int y, GLfloat * px, GLfloat * py, GLfloat * pz);

void SetupFrustum (void);
void SetupOrtho (void);


void WAvg (GLfloat perc, GLfloat x1, GLfloat y1, GLfloat z1,
	   GLfloat x2, GLfloat y2, GLfloat z2,
	   GLfloat * ax, GLfloat * ay, GLfloat * az);
void UpdateCabDisplay (void);
void UpdateFlatDisplay (void);
void UpdateGLDisplayBegin (struct mame_bitmap *bitmap);
void UpdateGLDisplayEnd   (struct mame_bitmap *bitmap);

int cabspecified;

double scrnaspect, vscrnaspect;

int use_mod_ctable;
GLubyte  *ctable;
GLushort *rcolmap, *gcolmap, *bcolmap, *acolmap;
int ctable_size;		/* the true color table size */

GLint      gl_internal_format;
GLenum     gl_bitmap_format;
GLenum	   gl_bitmap_type;
GLenum     gl_ctable_format;
GLenum	   gl_ctable_type;
GLsizei text_width;
GLsizei text_height;

int force_text_width_height;

static int palette_changed;

/* int cabview=0;  .. Do we want a cabinet view or not? */
int cabload_err;

int drawbitmap;
int dopersist;
int dodepth;

int totalcolors;
unsigned char gl_alpha_value;
static int frame;

static int screendirty;
/* The squares that are tiled to make up the game screen polygon */

struct TexSquare
{
  GLubyte *texture;
  GLuint texobj;
  int isTexture;
  GLfloat x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
  GLfloat fx1, fy1, fx2, fy2, fx3, fy3, fx4, fy4;
  GLfloat xcov, ycov;
  int isDirty;
  int dirtyXoff, dirtyYoff, dirtyWidth, dirtyHeight;
};

static struct TexSquare *texgrid;

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

/**
 * VERY IMPORTANT: osd_alloc_bitmap must allocate also a "safety area" 16 pixels 
 * wide all
 * around the bitmap. This is required because, for performance reasons, some graphic
 * routines don't clip at boundaries of the bitmap.
 */
#define BITMAP_SAFETY			16
static unsigned char *colorBlittedMemory;

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

int useGLEXT78; /* paletted texture */
int useColorIndex; 
int isGL12;

int useColorBlitter = 0;

static int do_snapshot;
static int do_xgl_resize=0;

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
  ctable_size=0;
  gl_bitmap_type=0;
  gl_bitmap_format=0;
  gl_ctable_format=0;
  gl_ctable_type=0;
  text_width=0;
  text_height=0;
  force_text_width_height = 0;
  palette_changed = 0;
  cabload_err=0;
  drawbitmap = 1;
  dopersist = 0;
  dodepth = 1;
  totalcolors=0;
  gl_alpha_value = 255;
  frame = 0;
  screendirty = 1;

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
  colorBlittedMemory=NULL;
  
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
  useGLEXT78 = GL_TRUE;
  isGL12=GL_TRUE;
  useColorIndex = GL_FALSE; 
  use_mod_ctable = 1;
  useColorBlitter = 0;

  do_snapshot=0;
  do_xgl_resize=0;
}

void gl_reset_resources()
{
  #ifndef NDEBUG
    printf("GLINFO: gl_reset_resources\n");
  #endif

  if(texgrid) free(texgrid);
  if(ctable) free(ctable);
  if(rcolmap) free(rcolmap);
  if(gcolmap) free(gcolmap);
  if(bcolmap) free(bcolmap);
  if(acolmap) free(acolmap);
  if(cpan) free(cpan);

  ctable = 0;
  rcolmap = 0; gcolmap = 0; bcolmap = 0; acolmap = 0;
  texgrid = 0;
  cpan=0;

  scrnaspect=0; vscrnaspect=0;
  ctable_size=0;
  gl_bitmap_type=0;
  gl_bitmap_format=0;
  gl_ctable_format=0;
  gl_ctable_type=0;
  palette_changed = 0;
  cabload_err=0;
  dopersist = 0;
  dodepth = 1;
  gl_alpha_value = 255;
  frame = 0;
  screendirty = 1;

  texnumx=0;
  texnumy=0;
  texpercx=0;
  texpercy=0;
  vscrntlx=0;
  vscrntly=0;
  vscrnwidth=0;
  vscrnheight=0;
  xinc=0; yinc=0;
  
  memory_x_len=0;
  memory_x_start_offset=0;
  bytes_per_pixel=0;
  colorBlittedMemory=NULL;
  
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
  
  do_snapshot=0;
  do_xgl_resize=0;
}

/* ---------------------------------------------------------------------- */
/* ------------ New OpenGL Specials ------------------------------------- */
/* ---------------------------------------------------------------------- */

int glHasEXT78 (void)
{
  if (gl_is_initialized)
    fetch_GL_FUNCS (libGLName, libGLUName, 0);
  return disp__glColorTableEXT != NULL;
}

void glSetUseEXT78 (int val)
{
  if (gl_is_initialized == 0 || val==GL_FALSE || 
      (disp__glColorTableEXT!=NULL && disp__glColorSubTableEXT!=NULL) )
    useGLEXT78 = val;
}

int glGetUseEXT78 (void)
{ return useGLEXT78; }

void gl_set_bilinear (int new_value)
{
  int x, y;
  bilinear = new_value;

  if (gl_is_initialized == 0)
    return;

  if (bilinear)
  {
    disp__glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR ();

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        disp__glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
        CHECK_GL_ERROR ();
      }
    }
  }
  else
  {
    disp__glBindTexture (GL_TEXTURE_2D, cabtex[0]);

    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    CHECK_GL_ERROR ();

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        disp__glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);

        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
        CHECK_GL_ERROR ();
      }
    }
  }
}

void gl_init_cabview ()
{
  if (glContext == 0)
    return;

        InitCabGlobals();

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
}

void gl_set_cabview (int new_value)
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

int gl_stream_antialias (int aa)
{
  if (gl_is_initialized == 0)
    return aa;

  if (aa)
  {
    disp__glShadeModel (GL_SMOOTH);
    disp__glEnable (GL_POLYGON_SMOOTH);
    disp__glEnable (GL_LINE_SMOOTH);
    disp__glEnable (GL_POINT_SMOOTH);
  }
  else
  {
    disp__glShadeModel (GL_FLAT);
    disp__glDisable (GL_POLYGON_SMOOTH);
    disp__glDisable (GL_LINE_SMOOTH);
    disp__glDisable (GL_POINT_SMOOTH);
  }

  return aa;
}

void gl_set_antialias (int new_value)
{
  antialias = gl_stream_antialias(new_value);
}

int gl_stream_alphablending (int alpha)
{
  if (gl_is_initialized == 0)
    return alpha;

  if (alpha)
  {
    disp__glEnable (GL_BLEND);
    disp__glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    disp__glDisable (GL_BLEND);
  }

  return alpha;
}

void gl_set_alphablending (int new_value)
{
  alphablending = gl_stream_alphablending(new_value);
}

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

static int blit_notified = 0;

void gl_update_16_to_16bpp (struct mame_bitmap *bitmap)
{
   if(current_palette->lookup)
   {
   	if(!blit_notified) printf("GLINFO: 16bpp palette lookup bitblit\n");
	#define SRC_PIXEL  unsigned short
	#define DEST_PIXEL unsigned short
	#define DEST texgrid->texture
	#define DEST_WIDTH memory_x_len
	#define INDIRECT current_palette->lookup
	#include "blit.h"
	#undef INDIRECT
	#undef DEST
	#undef DEST_WIDTH
	#undef SRC_PIXEL
	#undef DEST_PIXEL
   } else {
   	if(!blit_notified) printf("GLINFO: no 16bpp bitblit needed\n");
	/**
	 * nothing must be done here ;-)
	 */
   }
   blit_notified = 1;
}

void gl_update_32_to_32bpp(struct mame_bitmap *bitmap)
{
   if(current_palette->lookup)
   {
   	if(!blit_notified) printf("GLINFO: 32bpp palette lookup bitblit\n");
	#define SRC_PIXEL  unsigned int
	#define DEST_PIXEL unsigned int
	#define DEST texgrid->texture
	#define DEST_WIDTH memory_x_len
	#define INDIRECT current_palette->lookup
	#include "blit.h"
	#undef INDIRECT
	#undef DEST
	#undef DEST_WIDTH
	#undef SRC_PIXEL
	#undef DEST_PIXEL
   } else {
   	if(!blit_notified) printf("GLINFO: no 32bpp bitblit needed\n");
	/**
	 * nothing must be done here ;-)
	 */
   }
   blit_notified = 1;
}

int sysdep_display_16bpp_capable (void)
{
  #ifndef NDEBUG
    printf("GLINFO: sysdep_display_16bpp_capable\n");
  #endif

  return 1; /* direct color */
}

void InitVScreen (int depth)
{
  const unsigned char * glVersion;

  #ifndef NDEBUG
    printf("GLINFO: InitVScreen (glContext=%p)\n", glContext);
    printf("GLINFO: InitVScreen: useColorIndex=%d, alphablending=%d, doublebuffer=%d\n",
	   useColorIndex, alphablending, doublebuffer);
  #endif

  gl_reset_resources();

  /* clear the buffer */

  if (glContext == 0)
    return;

  fetch_GL_FUNCS (libGLName, libGLUName, 0);

  CHECK_GL_BEGINEND();

  glVersion = disp__glGetString(GL_VERSION);

  printf("\nGLINFO: OpenGL Driver Information:\n");
  printf("\tvendor: %s,\n\trenderer %s,\n\tversion %s\n", 
  	disp__glGetString(GL_VENDOR), 
	disp__glGetString(GL_RENDERER),
	glVersion);

  printf("GLINFO: GLU Driver Information:\n");
  printf("\tversion %s\n", 
	disp__gluGetString(GLU_VERSION));

  if(glVersion[0]>'1' ||
     (glVersion[0]=='1' && glVersion[2]>='2') )
	  isGL12 = GL_TRUE;
  else
	  isGL12 = GL_FALSE;

  if(isGL12)
  {
	printf("GLINFO: You have an OpenGL >= 1.2 capable drivers, GOOD (16bpp is ok !)\n");
  } else {
	printf("GLINFO: You have an OpenGL < 1.2 drivers, BAD (16bpp may be damaged  !)\n");
  }

  glSetUseEXT78 (useGLEXT78);

  InitCabGlobals ();

  disp__glClearColor (0, 0, 0, 1);
  disp__glClear (GL_COLOR_BUFFER_BIT);
  disp__glFlush ();

  if (!dodepth)
    cabview = 0;

  gl_init_cabview ();

  xgl_resize(winwidth, winheight,1);

  if (dodepth)
    disp__glDepthFunc (GL_LEQUAL);

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

  if(depth==8 && useColorIndex)
  {
	  display_palette_info.depth=depth;
	  display_palette_info.writable_colors = 256;

	  if(useGLEXT78)
		 gl_internal_format=GL_COLOR_INDEX8_EXT;
	  else
		 gl_internal_format=(alphablending)?GL_RGBA:GL_RGB;
	  gl_bitmap_format = GL_COLOR_INDEX;
	  gl_bitmap_type   = GL_UNSIGNED_BYTE;

	  printf("GLINFO: Offering depth=%d, writable colors=%d (color index mode 8)\n",
	  	display_palette_info.depth, display_palette_info.writable_colors);
          fflush(NULL);
  } else {
	  useColorBlitter = useColorIndex;

	  useColorIndex = 0;

	  if(depth<24 && !isGL12)
	  {
	  	depth=32;
		printf("GLINFO: you have no OpenGL 1.2 capable drivers, so the 16bpp is disabled -> 32bpp!\n");
	  }

	  display_palette_info.depth = depth;

	  /* no alpha .. important, because mame has no alpha set ! */
	  gl_internal_format=GL_RGB;

	  switch(display_palette_info.depth)
	  {
	  	case  8:  /* ... */
			  printf("GLERROR: blitter mode not supported for 8bpp color map's, use 15bpp, 16bpp or 32bpp instead !\n");
			  exit(1);
		case 15:
		case 16:
			  if(!useColorBlitter)
			  {
			    /* ARGB1555 */
			    display_palette_info.red_mask   = 0x00007C00;
			    display_palette_info.green_mask = 0x000003E0;
			    display_palette_info.blue_mask  = 0x0000001F;
			    gl_bitmap_format = GL_BGRA;
			    /*                                   A R G B */
			    gl_bitmap_type   = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		          } 
			  else 
			  {
			    /* RGBA5551 */
			    display_palette_info.red_mask   = 0x0000F800;
			    display_palette_info.green_mask = 0x000007C0;
			    display_palette_info.blue_mask  = 0x0000003E;
			    gl_bitmap_format = GL_RGBA;
			    /*                                   R G B A */
			    gl_bitmap_type   = GL_UNSIGNED_SHORT_5_5_5_1;
		          }
			  break;
		case 24:
			  /* RGB888 */
			  display_palette_info.red_mask   = 0xFF000000;
			  display_palette_info.green_mask = 0x00FF0000;
			  display_palette_info.blue_mask  = 0x0000FF00;
			  gl_bitmap_format = GL_RGB;         
			  gl_bitmap_type   = GL_UNSIGNED_BYTE;
			  break;
		case 32:
			 /**
			  * skip the D of DRGB 
			  */
			  /* ABGR8888 */
			  display_palette_info.blue_mask   = 0x00FF0000;
			  display_palette_info.green_mask  = 0x0000FF00;
			  display_palette_info.red_mask    = 0x000000FF;
			  if(!useColorBlitter)
				  gl_bitmap_format = GL_BGRA;
			  else
				  gl_bitmap_format = GL_RGBA;
			  /*                                 A B G R */
			  gl_bitmap_type   = GL_UNSIGNED_INT_8_8_8_8_REV;
			  break;
	  }

	  printf("GLINFO: Offering colors=%d, depth=%d, rgb 0x%X, 0x%X, 0x%X (true color mode)\n",
		display_palette_info.writable_colors,
		display_palette_info.depth, 
		display_palette_info.red_mask, display_palette_info.green_mask, 
		display_palette_info.blue_mask);
  }
  if(useColorBlitter) {
	printf("GLINFO: Using bit blit to map color indices !!\n");
	heightscale =1; widthscale=1; use_scanlines=0;
  }
}

/* Close down the virtual screen */

void CloseVScreen (void)
{
  #ifndef NDEBUG
    printf("GLINFO: CloseVScreen (gl_is_initialized=%d)\n", gl_is_initialized);
  #endif

  if(colorBlittedMemory!=NULL)
  	free(colorBlittedMemory);

  if (gl_is_initialized == 0)
    return;

  CHECK_GL_BEGINEND();

  gl_reset_resources();
}

/* Not needed under GL */
#ifndef WIN32
void sysdep_clear_screen (void)
{
}
#endif

int sysdep_display_alloc_palette (int writable_colors)
{
  #ifndef NDEBUG
    fprintf (stderr, "GLINFO: sysdep_display_alloc_palette(%d);\n",writable_colors);
  #endif

  if (glContext == 0)
    return 1;

  totalcolors = writable_colors;

  if (totalcolors > 256)
  {
    if(useColorIndex)
    {
        printf("GLERROR: OpenGL color index mode not supported for colors == %d / only for depth == 8 (<= 256 colors) !\n", totalcolors);
        exit(1);
    }
  }

  ctable_size = 1;
  while(ctable_size<totalcolors)
  	ctable_size*=2;

  /* some bitmap fix data values .. 
   * independend of the mame bitmap line arrangement !
   */
  bytes_per_pixel = (Machine->scrbitmap->depth+7) / 8;

  if(useColorIndex)
  {
      gl_ctable_type   = GL_UNSIGNED_BYTE;

      if (useGLEXT78)
      {
      	    int err;

  	    gl_ctable_format = (alphablending)?GL_RGBA:GL_RGB;
  
	    ctable = (GLubyte *) 
		  calloc (ctable_size * (alphablending?4:3), 1);

	    disp__glColorTableEXT (GL_TEXTURE_2D,
			       gl_ctable_format,
			       ctable_size, gl_ctable_format, 
			       gl_ctable_type, ctable);

	    err = disp__glGetError ();
	    if(err!=GL_NO_ERROR) 
	    {
	      /**
	       * ok .. but, we only use this mode for 256 color's,
	       * which is usually supported 
	       */
	      printf("GLERROR: ColorTable (glColorTableEXT) is unallocabel (color table too large := %d)!\n", err==GL_TABLE_TOO_LARGE);
	      exit(1);
	    }
      }

      if (!useGLEXT78)
      {
      	    int max_pixel_map_table;

      	    disp__glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &max_pixel_map_table);
	    if(max_pixel_map_table<ctable_size)
	    {
	       /**
	        * ok .. but, we only use this mode for 256 color's,
	        * which is usually supported 
	        */
	    	printf("GLERROR: ColorTable (glPixelMapusv) size %d is too large, maximum is = %d !! \n", ctable_size, max_pixel_map_table);
		exit(1);
	    }
	    rcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
	    gcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
	    bcolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
	    if (alphablending)
	      acolmap = (GLushort *) calloc (ctable_size * sizeof (GLushort), 1);
	    else
	      acolmap = 0;
      }
  }

  fprintf (stderr, "GLINFO: totalcolors = %d / colortable size= %d,\n\tdepth = %d alphablending=%d,\n\tuse_mod_ctable=%d\n\tuseColorIndex=%d\n",
	   totalcolors, ctable_size,
	   Machine->scrbitmap->depth, alphablending, 
	   use_mod_ctable,
	   useColorIndex);

  if(useColorIndex)
  {
	  if(useGLEXT78)
	  {
	      if (disp__glColorTableEXT == 0)
	        printf("GLINFO: glColorTableEXT not available .. BAD & SLOW\n");
	      else
	        printf("GLINFO: glColorTableEXT available .. GOOD & FAST\n");

	      if (disp__glColorSubTableEXT == 0)
	        printf("GLINFO: glColorSubTableEXT not available .. BAD & SLOW\n");
	      else
	        printf("GLINFO: glColorSubTableEXT available .. GOOD & FAST\n");
	  } else
	      fprintf (stderr, "GLINFO: useGLEXT78 = 0 (BAD & SLOW)\n");
  }

  palette_changed = useColorIndex;

  InitTextures ();

  return 0;
}

void InitTextures ()
{
  int e_x, e_y, s, i=0;
  int x=0, y=0, raw_line_len=0;
  GLint format=0;
  GLenum err;
  unsigned char *line_1, *line_2=0;
  struct TexSquare *tsq=0;

  if (glContext == 0)
    return;

  text_width  = visual_width;
  text_height = visual_height;

  CHECK_GL_BEGINEND();

  texnumx = 1;
  texnumy = 1;

  if(force_text_width_height>0)
  {
  	text_height=force_text_width_height;
  	text_width=force_text_width_height;
	fprintf (stderr, "GLINFO: force_text_width_height := %d x %d\n",
		text_height, text_width);
  }
  
  /* achieve the 2**e_x:=text_width, 2**e_y:=text_height */
  e_x=0; s=1;
  while (s<text_width)
  { s*=2; e_x++; }
  text_width=s;

  e_y=0; s=1;
  while (s<text_height)
  { s*=2; e_y++; }
  text_height=s;

  CHECK_GL_BEGINEND();

#ifndef NDEBUG
  fprintf (stderr, "GLINFO: gl_internal_format= 0x%X,\n\tgl_bitmap_format =  0x%X,\n\tgl_bitmap_type = 0x%X\n", gl_internal_format, gl_bitmap_format, gl_bitmap_type);
#endif

  /* Test the max texture size */
  do
  {
    disp__glTexImage2D (GL_PROXY_TEXTURE_2D, 0,
		  gl_internal_format,
		  text_width, text_height,
		  0, gl_bitmap_format, gl_bitmap_type, 0);

    CHECK_GL_ERROR ();

    disp__glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

    CHECK_GL_ERROR ();

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

      if(text_width < 64 || text_height < 64)
      {
      	fprintf (stderr, "GLERROR: Give up .. usable texture size not available, or texture config error !\n");
	exit(1);
      }
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
    calloc (texnumx * texnumy, sizeof (struct TexSquare));

  line_1 = (unsigned char *) Machine->scrbitmap->line[visual.min_y];
  line_2 = (unsigned char *) Machine->scrbitmap->line[visual.min_y + 1];

  raw_line_len = line_2 - line_1;

  memory_x_len = raw_line_len / bytes_per_pixel;

  fprintf (stderr, "GLINFO: texture-usage %d*width=%d, %d*height=%d\n",
		 (int) texnumx, (int) text_width, (int) texnumy,
		 (int) text_height);

  if(useColorBlitter)
  {
    colorBlittedMemory = malloc( (text_width+2*BITMAP_SAFETY)*texnumx*
                                 (text_height+2*BITMAP_SAFETY)*texnumy*
				 bytes_per_pixel);
    line_1 = colorBlittedMemory;
  }

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
	fprintf(stderr, "bitmap: w=%d, h=%d, d=%d,\n\trowpixels=%d, rowbytes=%d\n",
		Machine->scrbitmap->width,  Machine->scrbitmap->height, 
		Machine->scrbitmap->depth,
		Machine->scrbitmap->rowpixels, Machine->scrbitmap->rowbytes);

	fprintf(stderr, "bmsize=%d, mysize=%d,\n\twidthscale=%d, heightscale=%d, use_scanlines=%d\n",
		Machine->scrbitmap->width*Machine->scrbitmap->height*bytes_per_pixel,
                text_width*texnumx*text_height*texnumy*bytes_per_pixel,
		heightscale, widthscale, use_scanlines);

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
		 "row_len=%d, x_ofs=%d, bytes_per_pixel=%d\n",
		 (int) memory_x_len, (int) memory_x_start_offset,
		 (int) bytes_per_pixel);

	fflush(stderr);
      }
      fprintf (stderr, "\t texture mem %p\n", tsq->texture);
      fflush(stderr);

      #endif

      tsq->isTexture=GL_FALSE;
      tsq->texobj=0;

      tsq->dirtyXoff=0; tsq->dirtyYoff=0; 
      if (use_dirty)
      {
	      tsq->isDirty=GL_FALSE;
	      tsq->dirtyWidth=-1; tsq->dirtyHeight=-1;
      } else {
	      tsq->isDirty=GL_TRUE;
	      tsq->dirtyWidth=text_width; tsq->dirtyHeight=text_height;
      }

      disp__glGenTextures (1, &(tsq->texobj));

      disp__glBindTexture (GL_TEXTURE_2D, tsq->texobj);
      err = disp__glGetError ();
      if(err==GL_INVALID_ENUM)
      {
	fprintf (stderr, "GLERROR glBindTexture (glGenText) := GL_INVALID_ENUM, texnum x=%d, y=%d, texture=%d\n", x, y, tsq->texobj);
      } 
      #ifndef NDEBUG
	      else if(err==GL_INVALID_OPERATION) {
		fprintf (stderr, "GLERROR glBindTexture (glGenText) := GL_INVALID_OPERATION, texnum x=%d, y=%d, texture=%d\n", x, y, tsq->texobj);
	      }
      #endif

      if(disp__glIsTexture(tsq->texobj) == GL_FALSE)
      {
	fprintf (stderr, "GLERROR ain't a texture (glGenText): texnum x=%d, y=%d, texture=%d\n",
		x, y, tsq->texobj);
      } else {
        tsq->isTexture=GL_TRUE;
      }

      CHECK_GL_ERROR ();

      if(useColorIndex)
      {
	      if (useGLEXT78)
	      {
		  disp__glColorTableEXT (GL_TEXTURE_2D,
				     gl_ctable_format,
				     ctable_size, gl_ctable_format, 
				     gl_ctable_type, ctable);

		  CHECK_GL_ERROR ();
	      }
	      else
	      {
		disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
		disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
		disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
		if (alphablending)
		  disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);

		CHECK_GL_ERROR ();
	      }
      }

      disp__glTexImage2D (GL_TEXTURE_2D, 0,
		    gl_internal_format,
		    text_width, text_height,
		    0, gl_bitmap_format, gl_bitmap_type, NULL);

      CHECK_GL_ERROR ();

      disp__glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1.0);

      CHECK_GL_ERROR ();

      disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      disp__glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      CHECK_GL_ERROR ();
    }	/* for all texnumx */
  }  /* for all texnumy */

  gl_is_initialized = 1;

  CHECK_GL_BEGINEND();

  /* lets init the rest of the custumizings ... */
  gl_set_bilinear (bilinear);
  gl_set_antialias (antialias);
  gl_set_alphablending (alphablending);
  gl_set_cabview (cabview);

  CHECK_GL_BEGINEND();

  CHECK_GL_ERROR ();

}

/* Change the color of a pen */
int
sysdep_display_set_pen (int pen, unsigned char red, unsigned char green,
			unsigned char blue)
{
    int cofs=pen*(alphablending?4:3);

    printf("trying to set color table: %d (%X/%X/%X)\n",
    	pen, (int)red, (int)green, (int)blue);

    if(pen>ctable_size)
    	return 1;

    if (useGLEXT78 && pen < ctable_size)
    {
      double dAlpha = (double) gl_alpha_value / 255.0;

      if(!use_mod_ctable)
      {
	ctable[cofs] =     red;
	ctable[cofs + 1] = green;
	ctable[cofs + 2] = blue;
        if(alphablending)
		ctable[cofs + 3] = gl_alpha_value;
      }
      else if (alphablending)
      {
	ctable[cofs] = 255 * pow (red / 255.0, 1 / gamma_correction);
	ctable[cofs + 1] = 255 * pow (green / 255.0, 1 / gamma_correction);
	ctable[cofs + 2] = 255 * pow (blue / 255.0, 1 / gamma_correction);
	ctable[cofs + 3] = gl_alpha_value;
      }
      else
      {
	ctable[cofs] =
	  255 * (dAlpha * pow (red / 255.0, 1 / gamma_correction));
	ctable[cofs + 1] =
	  255 * (dAlpha * pow (green / 255.0, 1 / gamma_correction));
	ctable[cofs + 2] =
	  255 * (dAlpha * pow (blue / 255.0, 1 / gamma_correction));
      }
    }
    else
    {
      if(!use_mod_ctable)
      {
	    rcolmap[pen] = 65535 * (red / 255.0);
	    gcolmap[pen] = 65535 * (green / 255.0);
	    bcolmap[pen] = 65535 * (blue / 255.0);
	    if (alphablending)
		acolmap[pen] = 65535 * (gl_alpha_value / 255.0);
      }
      else if (alphablending)
      {
	rcolmap[pen] = 65535 * pow (red / 255.0, 1 / gamma_correction);
	gcolmap[pen] = 65535 * pow (green / 255.0, 1 / gamma_correction);
	bcolmap[pen] = 65535 * pow (blue / 255.0, 1 / gamma_correction);
	acolmap[pen] = 65535 * (gl_alpha_value / 255.0);
      }
      else
      {
	double dAlpha = (double) gl_alpha_value / 255.0;

	rcolmap[pen] =
	  65535 * (dAlpha * pow (red / 255.0, 1 / gamma_correction));
	gcolmap[pen] =
	  65535 * (dAlpha * pow (green / 255.0, 1 / gamma_correction));
	bcolmap[pen] =
	  65535 * (dAlpha * pow (blue / 255.0, 1 / gamma_correction));
      }
    }

    palette_changed = 1;
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
  CHECK_GL_BEGINEND();

  disp__glMatrixMode (GL_PROJECTION);
  disp__glLoadIdentity ();

  if(!do_snapshot)
	  disp__glFrustum (-vscrnaspect, vscrnaspect, -1.0, 1.0, 5.0, 100.0);
  else
	  disp__glFrustum (-vscrnaspect, vscrnaspect, 1.0, -1.0, 5.0, 100.0);

  CHECK_GL_ERROR ();
  disp__glMatrixMode (GL_MODELVIEW);
  disp__glLoadIdentity ();
  disp__glTranslatef (0.0, 0.0, -20.0);
}


/* Set up an orthographic projection */

void SetupOrtho (void)
{
  CHECK_GL_BEGINEND();

  disp__glMatrixMode (GL_PROJECTION);
  disp__glLoadIdentity ();

  if(!do_snapshot)
	  disp__glOrtho (0, 1, 1, 0, -1.0, 1.0);
  else
	  disp__glOrtho (0, 1, 0, 1, -1.0, 1.0);

  disp__glMatrixMode (GL_MODELVIEW);
  disp__glLoadIdentity ();
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

void xgl_resize(int w, int h, int now)
{
  winheight= h;
  winwidth = w;

  if(!now)
  {
	do_xgl_resize=1;
	return;
  }

  do_xgl_resize=0;

  CHECK_GL_BEGINEND();

  if (glContext!=NULL)
  {
	SetWindowRatio();

	if (cabview)
		disp__glViewport (0, 0, winwidth, winheight);
	else
		disp__glViewport ((int)(vscrntlx+0.5), (int)(vscrntly+0.5),
		            (int)(vscrnwidth+0.5), (int)(vscrnheight+0.5));

	if (cabview)
		SetupFrustum ();
	else
		SetupOrtho ();

	fprintf(stderr, "GLINFO: xgl_resize to %dx%d\n", winwidth, winheight);
	fflush(stderr);
  }
}

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

void gl_dirty_init(void)
{
	printf ("GLINFO: dirty strategie on!\n");
}

void gl_dirty_close(void)
{
	printf ("GLINFO: dirty strategie off!\n");
}

void gl_mark_dirty(int x, int y, int x2, int y2)
{
	struct TexSquare *square;
	int xi, yi, wd, ht, wh, hh;
	int wdecr, hdecr, xh, yh;
	int w = (x2-x)+1;
	int h = (y2-y)+1;

	screendirty = 1;

	if (!use_dirty)
		return;

	wdecr=w; hdecr=h; xh=x; yh=y;

	for (yi = 0; hdecr>0 && yi < texnumy; yi++)
	{
	    if (yi < texnumy - 1)
	      ht = text_height;
	    else
	      ht = visual_height - text_height * yi;

	    xh =x;
	    wdecr =w;

	    for (xi = 0; wdecr>0 && xi < texnumx; xi++)
	    {
		square = texgrid + yi * texnumx + xi;

		if (xi < texnumx - 1)
		  wd = text_width;
		else
		  wd = visual_width - text_width * xi;

		if( 0 <= xh && xh < wd &&
		    0 <= yh && yh < ht
		  )
		{
			square->isDirty=GL_TRUE;

			wh=(wdecr<wd)?wdecr:wd-xh;
			if(wh<0) wh=0;

			hh=(hdecr<ht)?hdecr:ht-yh;
			if(hh<0) hh=0;

			/*
			#ifndef NDEBUG
			     printf("\t %dx%d, %d/%d (%dx%d): %d/%d (%dx%d)\n", 
				xi, yi, xh, yh, wdecr, hdecr, xh, yh, wh, hh);
			#endif
			*/

			if(xh<square->dirtyXoff)
				square->dirtyXoff=xh;

			if(yh<square->dirtyYoff)
				square->dirtyYoff=yh;

			square->dirtyWidth = wd-square->dirtyXoff;
			square->dirtyHeight = ht-square->dirtyYoff;
			
			wdecr-=wh;

			if ( xi == texnumx - 1 )
				hdecr-=hh;
		}

		xh-=wd;
		if(xh<0) xh=0;
	    }
	    yh-=ht;
	    if(yh<0) yh=0;
       }
}

void
drawTextureDisplay (int useCabinet, int updateTexture)
{
  struct TexSquare *square;
  int x, y;
  GLenum err;
  static const float z_pos = 0.9f;

  if(gl_is_initialized == 0)
  	return;

  CHECK_GL_BEGINEND();

  if (updateTexture)
  {
    disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, memory_x_len);
    disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 8);
    if (!useGLEXT78 && useColorIndex)
    	disp__glPixelTransferi (GL_MAP_COLOR, GL_TRUE);

    if( useColorBlitter )
    {
	if(bytes_per_pixel==2)
		gl_update_16_to_16bpp (Machine->scrbitmap);
	else if(bytes_per_pixel==4)
		gl_update_32_to_32bpp(Machine->scrbitmap);
	else {
		printf("GLERROR: blitter mode only for 8bpp, 16bpp and 32bpp !\n");
		exit(1);
	}
    }
  }

  disp__glEnable (GL_TEXTURE_2D);

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

      disp__glBindTexture (GL_TEXTURE_2D, square->texobj);
      err = disp__glGetError ();
      if(err==GL_INVALID_ENUM)
      {
	fprintf (stderr, "GLERROR glBindTexture := GL_INVALID_ENUM, texnum x=%d, y=%d, texture=%d\n", x, y, square->texobj);
      }
      #ifndef NDEBUG
	      else if(err==GL_INVALID_OPERATION) {
		fprintf (stderr, "GLERROR glBindTexture := GL_INVALID_OPERATION, texnum x=%d, y=%d, texture=%d\n", x, y, square->texobj);
	      }
      #endif

      if(disp__glIsTexture(square->texobj) == GL_FALSE)
      {
        square->isTexture=GL_FALSE;
	fprintf (stderr, "GLERROR ain't a texture(update): texnum x=%d, y=%d, texture=%d\n",
		x, y, square->texobj);
      }

      if (palette_changed && useColorIndex)
      {

	if (useGLEXT78)
	{
	    disp__glColorTableEXT (GL_TEXTURE_2D, gl_ctable_format,
			       ctable_size, gl_ctable_format, 
			       gl_ctable_type, ctable);
	}
	else
	{
	  disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_R, ctable_size, rcolmap);
	  disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_G, ctable_size, gcolmap);
	  disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_B, ctable_size, bcolmap);
	  if (alphablending)
	    disp__glPixelMapusv (GL_PIXEL_MAP_I_TO_A, ctable_size, acolmap);
	}
      }

      /* This is the quickest way I know of to update the texture */
      if (updateTexture && square->isDirty)
      {
	   disp__glTexSubImage2D (GL_TEXTURE_2D, 0, 
		 square->dirtyXoff, square->dirtyYoff,
		 square->dirtyWidth, square->dirtyHeight,
		 gl_bitmap_format, gl_bitmap_type, square->texture);

           square->dirtyXoff=0; square->dirtyYoff=0; 
	   if (use_dirty)
	   {
	      square->isDirty=GL_FALSE;
	      square->dirtyWidth=-1; square->dirtyHeight=-1;
           } else {
	      square->isDirty=GL_TRUE;
	      square->dirtyWidth=text_width; square->dirtyHeight=text_height;
	   }
      }

      if (useCabinet)
      {
	GL_BEGIN(GL_QUADS);
	disp__glTexCoord2f (0, 0);
	disp__glVertex3f (square->x1, square->y1, square->z1);
	disp__glTexCoord2f (square->xcov, 0);
	disp__glVertex3f (square->x2, square->y2, square->z2);
	disp__glTexCoord2f (square->xcov, square->ycov);
	disp__glVertex3f (square->x3, square->y3, square->z3);
	disp__glTexCoord2f (0, square->ycov);
	disp__glVertex3f (square->x4, square->y4, square->z4);
	GL_END();
      }
      else
      {
	GL_BEGIN(GL_QUADS);
	disp__glTexCoord2f (0, 0);
	disp__glVertex3f (square->fx1, square->fy1, z_pos);
	disp__glTexCoord2f (square->xcov, 0);
	disp__glVertex3f (square->fx2, square->fy2, z_pos);
	disp__glTexCoord2f (square->xcov, square->ycov);
	disp__glVertex3f (square->fx3, square->fy3, z_pos);
	disp__glTexCoord2f (0, square->ycov);
	disp__glVertex3f (square->fx4, square->fy4, z_pos);
	GL_END();
      }
    } /* for all texnumx */
  } /* for all texnumy */

  if (updateTexture)
  {
    disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    disp__glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    disp__glPixelTransferi (GL_MAP_COLOR, GL_FALSE);
  }

  disp__glDisable (GL_TEXTURE_2D);

  palette_changed = 0;

  CHECK_GL_BEGINEND();

  /* YES - lets catch this error ... 
   */
  (void) disp__glGetError ();
}

/* Draw a frame in Cabinet mode */

void UpdateCabDisplay (void)
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

  CHECK_GL_BEGINEND();

  disp__glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CHECK_GL_ERROR ();

  disp__glPushMatrix ();


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

    disp__gluLookAt (camx, camy, camz, dirx, diry, dirz, normx, normy, normz);

    panframe++;
  }
  else
    disp__gluLookAt (-5.0, 0.0, 5.0, 0.0, 0.0, -5.0, 0.0, 1.0, 0.0);

  disp__glEnable (GL_DEPTH_TEST);

  /* Draw the cabinet */
  disp__glCallList (cablist);

  /* YES - lets catch this error ... */
  glerrid = disp__glGetError ();
  if (0x502 != glerrid)
  {
    CHECK_GL_ERROR ();
  }

  /* Draw the screen if in vector mode */

  if (vecgame)
  {
    if (drawbitmap)
    {
      disp__glColor4f (1.0, 1.0, 1.0, 1.0);

      drawTextureDisplay (1 /*cabinet */ , screendirty);
    }
    disp__glDisable (GL_TEXTURE_2D);
    disp__glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    disp__glShadeModel (GL_FLAT);

    if (antialiasvec)
    {
      disp__glEnable (GL_LINE_SMOOTH);
      disp__glEnable (GL_POINT_SMOOTH);
    }

    disp__glCallList (veclist);

    if (antialiasvec && !antialias)
    {
      disp__glDisable (GL_LINE_SMOOTH);
      disp__glDisable (GL_POINT_SMOOTH);
    }

    disp__glShadeModel (shadeModel);
  }
  else
  {				/* Draw the screen of a bitmapped game */

    if (drawbitmap)
	    drawTextureDisplay (1 /*cabinet */ , screendirty);

  }

  disp__glDisable (GL_DEPTH_TEST);

  disp__glPopMatrix ();

  if (do_snapshot)
  {
    gl_save_screen_snapshot();
    do_snapshot = 0;
    /* reset upside down .. */
    xgl_resize(winwidth, winheight, 1);
  }

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
    disp__glFlush ();

  CHECK_GL_BEGINEND();

  CHECK_GL_ERROR ();

}

void
UpdateFlatDisplay (void)
{
  int shadeModel;

  if (gl_is_initialized == 0)
    return;

  CHECK_GL_BEGINEND();

  if (!vecgame || !dopersist)
    disp__glClear (GL_COLOR_BUFFER_BIT);

  disp__glPushMatrix ();

  CHECK_GL_BEGINEND();

  if (dopersist && vecgame)
  {
    disp__glColor4f (0.0, 0.0, 0.0, 0.2);

    GL_BEGIN(GL_QUADS);
    disp__glVertex2f (0.0, 0.0);
    disp__glVertex2f ((GLfloat) winwidth, 0.0);
    disp__glVertex2f ((GLfloat) winwidth, (GLfloat) winheight);
    disp__glVertex2f (0.0, (GLfloat) winheight);
    GL_END();
  }

  CHECK_GL_ERROR ();


  disp__glColor4f (1.0, 1.0, 1.0, 1.0);

  if(drawbitmap)
	  drawTextureDisplay (0, screendirty);

  if (vecgame)
  {
    disp__glDisable (GL_TEXTURE_2D);
    disp__glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    disp__glShadeModel (GL_FLAT);

    if (antialiasvec)
    {
      disp__glEnable (GL_LINE_SMOOTH);
      disp__glEnable (GL_POINT_SMOOTH);
    }

    disp__glCallList (veclist);

    if (antialiasvec && !antialias)
    {
      disp__glDisable (GL_LINE_SMOOTH);
      disp__glDisable (GL_POINT_SMOOTH);
    }

    disp__glShadeModel (shadeModel);

  }

  CHECK_GL_BEGINEND();

  /* YES - lets catch this error ... 
   */
  (void) disp__glGetError ();

  disp__glPopMatrix ();

  CHECK_GL_ERROR ();

  if (do_snapshot)
  {
    gl_save_screen_snapshot();
    do_snapshot = 0;
    /* reset upside down .. */
    xgl_resize(winwidth, winheight, 1);
  }

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
    disp__glFlush ();
}

void UpdateGLDisplayBegin (struct mame_bitmap *bitmap)
{
  if (gl_is_initialized == 0)
    return;

  if (vecgame)
  {
    vector_vh_update (NULL, 0);
    CHECK_GL_BEGINEND();

    /**
     * after this vh_update, everything from vector
     * (begin/end, list) should be closed ..)
     */
  }

  if (do_xgl_resize)
	xgl_resize(winwidth, winheight, 1);

  /* upside down .. to make a good snapshot ;-) */
  if (do_snapshot)
	xgl_resize(winwidth, winheight, 1);

  if (cabview && !cabload_err)
    UpdateCabDisplay ();
  else
    UpdateFlatDisplay ();


  CHECK_GL_BEGINEND();
  CHECK_GL_ERROR ();
}

void UpdateGLDisplayEnd (struct mame_bitmap *bitmap)
{
  if (vecgame)
  {
    vector_clear_list ();
    /* be aware: a GL_BEGIN was called at last .. */
  }

  /* screendirty = 0; */
}

/* used when expose events received */

void
osd_refresh_screen (void)
{
  /* Just re-draw the whole screen */
  UpdateGLDisplayBegin (NULL);
  UpdateGLDisplayEnd (NULL);
}

/* invoked by main tree code to update bitmap into screen */

void
sysdep_update_display (struct mame_bitmap *bitmap)
{
  UpdateGLDisplayBegin (bitmap);

  frame++;

  if (keyboard_pressed (KEYCODE_RALT))
  {
    if (keyboard_pressed_memory (KEYCODE_A))
    {
    	  if(vecgame)
	  {
		  antialiasvec = 1-antialiasvec;
		  printf("GLINFO: switched antialias := %d\n", antialiasvec);
	  } else {
		  gl_set_antialias (1-antialias);
		  printf("GLINFO: switched antialias := %d\n", antialias);
	  }
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
	toggleFullscreen();
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

  UpdateGLDisplayEnd (bitmap);
}

void osd_save_snapshot(struct mame_bitmap *bitmap)
{
	do_snapshot = 1;
}

#endif /* ifdef xgl */
