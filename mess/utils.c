#include <ctype.h>
#include <stdlib.h>
#include "utils.h"

char *strncpyz(char *dest, const char *source, size_t len)
{
	char *s;
	if (len) {
		s = strncpy(dest, source, len - 1);
		dest[len-1] = '\0';
	}
	else {
		s = dest;
	}
	return s;
}

char *strncatz(char *dest, const char *source, size_t len)
{
	size_t l;
	l = strlen(dest);
	dest += l;
	if (len > l)
		len -= l;
	else
		len = 0;
	return strncpyz(dest, source, len);
}

void rtrim(char *buf)
{
	size_t buflen;
	char *s;

	buflen = strlen(buf);
	if (buflen) {
		for (s = &buf[buflen-1]; (*s == ' '); s--)
			*s = '\0';
	}
}

#ifndef strcmpi
int strcmpi(const char *dst, const char *src)
{
	int result = 0;

	while( !result && *src && *dst )
	{
		result = tolower(*dst) - tolower(*src);
		src++;
		dst++;
	}
	return result;
}
#endif /* strcmpi */


#ifndef strncmpi
int strncmpi(const char *dst, const char *src, size_t n)
{
	int result = 0;

	while( !result && *src && *dst && n)
	{
		result = tolower(*dst) - tolower(*src);
		src++;
		dst++;
		n--;
	}
	return result;
}
#endif /* strncmpi */

#ifndef memset16
void *memset16 (void *dest, int value, size_t size)
{
	register int i;

	for (i = 0; i < size; i++)
		((short *) dest)[i] = value;
	return dest;
}
#endif

char *stripspace(const char *src)
{
	static char buff[512];
	if( src )
	{
		char *dst;
		while( *src && isspace(*src) )
			src++;
		strcpy(buff, src);
		dst = buff + strlen(buff);
		while( dst >= buff && isspace(*--dst) )
			*dst = '\0';
		return buff;
	}
	return NULL;
}

//============================================================
//	strip_extension
//============================================================

char *strip_extension(const char *filename)
{
	char *newname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	newname = (char *) malloc(strlen(filename) + 1);
	if (!newname)
		return NULL;

	// copy in the name
	strcpy(newname, filename);

	// search backward for a period, failing if we hit a slash or a colon
	for (c = newname + strlen(newname) - 1; c >= newname; c--)
	{
		// if we hit a period, NULL terminate and break
		if (*c == '.')
		{
			*c = 0;
			break;
		}

		// if we hit a slash or colon just stop
		if (*c == '\\' || *c == '/' || *c == ':')
			break;
	}

	return newname;
}
