
#include "gltool.h"

#include "gl-disp-var.hc"
#include "glu-disp-var.hc"
#include "glx-disp-var.hc"

char * libGLName=0;
char * libGLUName=0;
static int gl_begin_ctr;

void print_gl_error (const char *msg, const char *file, int line, int errno)
{
  if (errno != GL_NO_ERROR)
  {
    const char *errstr = (const char *) __gluErrorString (errno);
    if (errstr != 0)
      fprintf (stderr, "\n\n****\n**%s %s:%d>0x%X %s\n****\n", msg, file, line, errno, errstr);
    else
      fprintf (stderr, "\n\n****\n**%s %s:%d>0x%X <unknown>\n****\n", msg, file, line, errno);
  }
}

#ifdef WIN32
void check_wgl_error (HWND wnd, const char *file, int line)
{
  LPVOID lpMsgBuf;

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | \FORMAT_MESSAGE_FROM_SYSTEM,
		 0, GetLastError (), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
		 (LPTSTR) & lpMsgBuf, 0, 0);	// Display the string.

  fprintf (stderr, "\n\n****\n** %s:%d>%s\n****\n", file, line, lpMsgBuf);

  // Free the buffer.
  LocalFree (lpMsgBuf);
}
#endif

void check_gl_error (const char *file, int line)
{
  print_gl_error("GLCHECK", file, line, __glGetError());
}

void __sglBegin(const char * file, int line, GLenum mode)
{
  	print_gl_error("GL-PreBEGIN-CHECK", file, line, __glGetError());

	if(gl_begin_ctr!=0)
	{
		printf("\n\n****\n** GL-BEGIN-ERROR %s:%d> glBegin was called %d times (reset now)\n****\n", 
			file, line, gl_begin_ctr);
		gl_begin_ctr=0;
	} else
		gl_begin_ctr++;

	__glBegin(mode);
}

void __sglEnd(const char * file, int line)
{
	if(gl_begin_ctr!=1)
	{
		printf("\n\n****\n** GL-END-ERROR %s:%d> glBegin was called %d times (reset now)\n****\n", 
			file, line, gl_begin_ctr);
		gl_begin_ctr=1;
	} else
		gl_begin_ctr--;

	__glEnd();

  	print_gl_error("GL-PostEND-CHECK", file, line, __glGetError());
}

/** 
 * call this function only if you 
 * are not within a glBegin/glEnd block,
 * or you think your are not ;-)
 */
void checkGlBeginEndBalance(const char *file, int line)
{
	if(gl_begin_ctr!=0)
	{
		printf("\n****\n** GL-BeginEnd-ERROR %s:%d> glBegin was called %d times\n****\n", 
			file, line, gl_begin_ctr);
	}
  	print_gl_error("GL-BeginEnd-CHECK", file, line, __glGetError());
}

void showGlBeginEndBalance(const char *file, int line)
{
	printf("\n****\n** GL-BeginEnd %s:%d> glBegin was called %d times\n****\n", 
		file, line, gl_begin_ctr);
}

int loadGLLibrary ()
{
#ifdef WIN32

  static HMODULE hDLL_OPENGL32 = 0;
  static HMODULE hDLL_OPENGLU32 = 0;

  if(hDLL_OPENGL32!=NULL) return 1;

  hDLL_OPENGL32 = LoadLibrary (libGLName);
  hDLL_OPENGLU32 = LoadLibrary (libGLUName);

  if (hDLL_OPENGL32 == NULL)
  {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
      fflush (NULL);
      return 0;
  }

  if (hDLL_OPENGLU32 == NULL)
  {
      printf ("GLERROR: cannot access GLU library %s\n", libGLUName);
      fflush (NULL);
      return 0;
  }

#else

  static void *libHandleGL=0;
  static void *libHandleGLU=0;

  if(libHandleGL!=NULL) return 1;

  libHandleGL = dlopen (libGLName, RTLD_LAZY | RTLD_GLOBAL);
  if (libHandleGL == NULL)
  {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
      fflush (NULL);
      return 0;
  }

  libHandleGLU = dlopen (libGLUName, RTLD_LAZY | RTLD_GLOBAL);
  if (libHandleGLU == NULL)
  {
      printf ("GLERROR: cannot access GLU library %s\n", libGLUName);
      fflush (NULL);
      return 0;
  }

#endif

  printf ("GLINFO: loaded OpenGL library %s!\n", libGLName);
  printf ("GLINFO: loaded GLU    library %s!\n", libGLUName);
  fflush (NULL);
  return 1;
}

void *
getGLProcAddressHelper (const char *func, int *method, int debug, int verbose)
{
  void *funcPtr = NULL;
  int lmethod;

#ifdef WIN32

  HMODULE hDLL_OPENGL32 = 0;
  HMODULE hDLL_OPENGLU32 = 0;

  loadGLLibrary ();

  funcPtr = wglGetProcAddress (func);
  if (funcPtr == NULL)
  {
    lmethod = 2;


    if (hDLL_OPENGL32 == NULL)

    {
      hDLL_OPENGL32 = LoadLibrary (libGLName);
    }
    if (hDLL_OPENGL32 == NULL)
    {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
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

  if (funcPtr == NULL)
  {
    lmethod = 3;

    if (hDLL_OPENGLU32 == NULL)

    {
      hDLL_OPENGLU32 = LoadLibrary (libGLUName);
    }
    if (hDLL_OPENGLU32 == NULL)
    {
      printf ("GLERROR: cannot access GLU library %s\n", libGLUName);
      fflush (NULL);
    }
    else
    {
      funcPtr = GetProcAddress (hDLL_OPENGLU32, func);
      FreeLibrary (hDLL_OPENGLU32);
    }
  }

#else

  void *libHandleGL;
  void *libHandleGLU;

  /*
   * void (*glXGetProcAddressARB(const GLubyte *procName))
   */
  static void *(*__glXGetProcAddress) (const GLubyte * procName) = NULL;
  static int __firstAccess = 1;

  loadGLLibrary ();

  if (__glXGetProcAddress == NULL && __firstAccess)
  {
    libHandleGL = dlopen (libGLName, RTLD_LAZY | RTLD_GLOBAL);
    if (libHandleGL == NULL)
    {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
      fflush (NULL);
    }
    else
    {
      __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressARB");

      if (__glXGetProcAddress != NULL && verbose)
      {
	printf ("GLINFO: found glXGetProcAddressARB in %s\n", libGLName);
	fflush (NULL);
      }

      if (__glXGetProcAddress == NULL)
      {
	__glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressEXT");

	if (__glXGetProcAddress != NULL && verbose)
	{
	  printf ("GLINFO: found glXGetProcAddressEXT in %s\n", libGLName);
	  fflush (NULL);
	}
      }

      if (__glXGetProcAddress == NULL)
      {
	__glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddress");

	if (__glXGetProcAddress != NULL && verbose)
	{
	  printf ("GLINFO: found glXGetProcAddress in %s\n", libGLName);
	  fflush (NULL);
	}
      }

      dlclose (libHandleGL);
      libHandleGL = NULL;

      if (__glXGetProcAddress == NULL)
      {
	printf
	  ("GLINFO: cannot find glXGetProcAddress* in OpenGL library %s\n", libGLName);
	fflush (NULL);
	libHandleGL = dlopen ("libglx.so", RTLD_LAZY | RTLD_GLOBAL);
	if (libHandleGL == NULL)
	{
	  printf ("GLINFO: cannot access GLX library libglx.so\n");
	  fflush (NULL);
	}
	else
	{
	  __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressARB");

	  if (__glXGetProcAddress != NULL && verbose)
	  {
	    printf ("GLINFO: found glXGetProcAddressARB in libglx.so\n");
	    fflush (NULL);
	  }

	  if (__glXGetProcAddress == NULL)
	  {
	    __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddressEXT");

	    if (__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("GLINFO: found glXGetProcAddressEXT in libglx.so\n");
	      fflush (NULL);
	    }
	  }

	  if (__glXGetProcAddress == NULL)
	  {
	    __glXGetProcAddress = dlsym (libHandleGL, "glXGetProcAddress");

	    if (__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("GLINFO: found glXGetProcAddress in libglx.so\n");
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
    printf ("GLINFO: cannot find glXGetProcAddress* in GLX library libglx.so\n");
    fflush (NULL);
  }
  __firstAccess = 0;

  if (__glXGetProcAddress != NULL)
    funcPtr = __glXGetProcAddress (func);

  if (funcPtr == NULL)
  {
    lmethod = 2;
    libHandleGL = dlopen (libGLName, RTLD_LAZY | RTLD_GLOBAL);
    if (libHandleGL == NULL)
    {
      printf ("GLINFO: cannot access OpenGL library %s\n", libGLName);
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

  if (funcPtr == NULL)
  {
    lmethod = 3;
    libHandleGLU = dlopen (libGLUName, RTLD_LAZY | RTLD_GLOBAL);
    if (libHandleGLU == NULL)
    {
      printf ("GLINFO: cannot access GLU library %s\n", libGLUName);
      fflush (NULL);
    }
    else
    {
      funcPtr = dlsym (libHandleGLU, func);
      dlclose (libHandleGLU);
    }
  }
#endif


  if (funcPtr == NULL)
  {
    if (debug || verbose)
    {
      printf ("GLINFO: %s (%d): not implemented !\n", func, lmethod);
      fflush (NULL);
    }
  }
  else if (verbose)
  {
    printf ("GLINFO: %s (%d): loaded !\n", func, lmethod);
    fflush (NULL);
  }
  if (method != NULL)
    *method = lmethod;
  return funcPtr;
}


void fetch_GL_FUNCS (int force)
{
  static int _firstRun = 1;

  if(force)
        _firstRun = 1;

  if(!_firstRun)
  {
	gl_begin_ctr=0;
  	return;
  }

  #define GET_GL_PROCADDRESS(a) getGLProcAddressHelper ((a), NULL, 1, 0);

  #include "gl-disp-fetch.hc"
  #include "glu-disp-fetch.hc"
  #include "glx-disp-fetch.hc"

  _firstRun=0;
}

