#ifndef __SNPRINTF_H_
#define __SNPRINTF_H_

/* 
   snprintf, a safe sprintf 

   no bufferoverflow 

   not in ansi-c, so here is a replacement
*/


#ifndef _GNU_SOURCE
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
