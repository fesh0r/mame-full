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

/* -----------------------------------------------------------------------
 * basename
 *
 * Like the GNU C version of basename
 *
 * What is the best way to do this???
 * ----------------------------------------------------------------------- */

#ifdef WIN32
inline const char *basename(const char *name)
{
	const char *s = name;
	const char *result = name;

	for (s = name; *s; s++) {
		if ((*s == '\\') || (*s == '/'))
			result = s + 1;
	}
	return result;
}
#endif

#endif /* UTILS_H */
