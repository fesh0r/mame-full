#include "driver.h"
#include <ctype.h>
#include <stdarg.h>

/* Variables to hold the status of various game options */
struct GameOptions	options;



void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	if (options.errorlog)
		vfprintf(options.errorlog,text,arg);
	va_end(arg);
}


