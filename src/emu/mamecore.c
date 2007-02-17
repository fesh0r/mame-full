/***************************************************************************

    mamecore.c

    Simple core functions that are defined in mamecore.h and which may
    need to be accessed by other MAME-related tools.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************/

#include "mamecore.h"
#include <ctype.h>

/* a giant string buffer for temporary strings */
char giant_string_buffer[GIANT_STRING_BUFFER_SIZE];


/*-------------------------------------------------
    mame_stricmp - case-insensitive string compare
-------------------------------------------------*/

int mame_stricmp(const char *s1, const char *s2)
{
	for (;;)
 	{
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		if (c1 == 0 || c1 != c2)
			return c1 - c2;
 	}
}


/*-------------------------------------------------
    mame_strnicmp - case-insensitive string compare
-------------------------------------------------*/

int mame_strnicmp(const char *s1, const char *s2, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
 	{
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		if (c1 == 0 || c1 != c2)
			return c1 - c2;
 	}

	return 0;
}


/*-------------------------------------------------
    mame_strwildcmp - case-insensitive wildcard
    string compare (up to 8 characters at the
    moment)
-------------------------------------------------*/

int mame_strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = (int)strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = (int)strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return mame_stricmp(s1, s2);
}


/*-------------------------------------------------
    mame_strdup - string duplication via malloc
-------------------------------------------------*/

char *mame_strdup(const char *str)
{
	char *cpy = NULL;
	if (str != NULL)
	{
		cpy = malloc(strlen(str) + 1);
		if (cpy != NULL)
			strcpy(cpy, str);
	}
	return cpy;
}
