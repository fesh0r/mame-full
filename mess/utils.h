#ifndef UTILS_H
#define UTILS_H

#include <string.h>

#ifdef WIN32
#define strcmpi		stricmp
#define strncmpi	strnicmp
#endif

/* -----------------------------------------------------------------------
 * strncpyz
 * strncatz
 *
 * strncpy done right :-)
 * ----------------------------------------------------------------------- */
char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);

/* -----------------------------------------------------------------------
 * rtrim
 *
 * Removes all trailing spaces from a string
 * ----------------------------------------------------------------------- */
void rtrim(char *buf);

/* -----------------------------------------------------------------------
 * strcmpi
 * strncmpi
 *
 * Case insensitive compares.  If your platform has this function then
 * #define it above and our implementation will be ignored
 * ----------------------------------------------------------------------- */

#ifndef strcmpi
int strcmpi(const char *dst, const char *src);
#endif /* strcmpi */

#ifndef strncmpi
int strncmpi(const char *dst, const char *src, size_t n);
#endif /* strcmpi */

#ifdef WIN32 
const char *basename (const char *name);
#endif

#ifdef UNIX
const char *basename (const char *name);
#endif

#endif /* UTILS_H */
