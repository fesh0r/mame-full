#ifndef __SNPRINTF_H_
#define __SNPRINTF_H_

/* 
   snprintf, a safe sprintf 

   no bufferoverflow 

   not in ansi-c, so here is a replacement
*/

#ifdef _GNU_SOURCE
#define SNPRINTF_PRESENT
#endif

#ifdef WIN32
#include <stdio.h>
#define snprintf _snprintf
#define SNPRINTF_PRESENT
#endif

#ifndef SNPRINTF_PRESENT
/*linking when using gnu libraries, but not defined _GNU_SOURCE?*/

#include "osdepend.h"

#ifdef __cplusplus
extern "C" {
#endif

int CLIB_DECL snprintf(char *s, size_t maxlen, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

#endif
