#include <ctype.h>
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

