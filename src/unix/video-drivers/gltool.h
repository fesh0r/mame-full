#ifndef _GLTOOL_H
	#define _GLTOOL_H

	#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <windowsx.h>
	#include <assert.h>
	#include <math.h>
	#else
	#include <ctype.h>
	#include <math.h>
	#include <dlfcn.h>
	#include <X11/Xlib.h>
	#include <X11/Xutil.h>
	#include <X11/Xatom.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
	#define CALLBACK
	#endif

	#include <stdio.h>

	#include "gl-disp-var.h"
	#include "glu-disp-var.h"
	#include "glx-disp-var.h"

	#ifndef GLDEBUG
		#define NDEBUG
	#endif

	#ifdef WIN32
		#ifndef NDEBUG
			#define CHECK_WGL_ERROR(a,b,c) check_wgl_error(a,b,c)
		#else
			#define CHECK_WGL_ERROR(a,b,c)
		#endif
	#else
		#define CHECK_WGL_ERROR(a,b,c)
	#endif

	#ifndef NDEBUG
		#define PRINT_GL_ERROR(a, b)	print_gl_error((a), __FILE__, __LINE__, (b))
		#define CHECK_GL_ERROR()  	check_gl_error(__FILE__,__LINE__)
		#define GL_BEGIN(m) 		__sglBegin(__FILE__, __LINE__, (m))
		#define GL_END()    		__sglEnd  (__FILE__, __LINE__)
		#define SHOW_GL_BEGINEND()	showGlBeginEndBalance(__FILE__, __LINE__)
		#define CHECK_GL_BEGINEND()	checkGlBeginEndBalance(__FILE__, __LINE__)
	#else
		#define PRINT_GL_ERROR(a, b)	
		#define CHECK_GL_ERROR()  
		#define GL_BEGIN(m) 		__glBegin(m)
		#define GL_END()    		__glEnd  ()
		#define SHOW_GL_BEGINEND()	
		#define CHECK_GL_BEGINEND()	
	#endif

	extern char * libGLName;
	extern char * libGLUName;

	#ifdef WIN32
		void check_wgl_error (HWND wnd, const char *file, int line);
	#endif

	void print_gl_error (const char *msg, const char *file, int line, int errno);
	void check_gl_error (const char *file, int line);

	void showGlBeginEndBalance(const char *file, int line);
	void checkGlBeginEndBalance(const char *file, int line);
	void __sglBegin(const char * file, int line, GLenum mode);
	void __sglEnd(const char * file, int line);

	int loadGLLibrary ();

	void * getGLProcAddressHelper (const char *func, int *method, int debug, int verbose);

	void fetch_GL_FUNCS (int force);
#endif
