#include "driver.h"
#include <ctype.h>
#include <stdarg.h>

/* Variables to hold the status of various game options */
static FILE *errorlog;


void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	if (errorlog)
		vfprintf(errorlog,text,arg);
	va_end(arg);
}


