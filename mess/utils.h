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
	int len = strlen (name);

	while (len-- >= 0)
		{
		if ((name[len] == '\\') || (name[len] == '/'))
			return name + len + 1;
		}

	return name;
	}
#endif

#ifdef UNIX
inline const char *basename (const char *name)
	{
	const char *p;

	p = strrchr (name, '/');
	return (p == NULL ? name : p);
	}
#endif

#endif /* UTILS_H */
