#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mame.h"
#include "strconv.h"



//============================================================
//	osd_dirname
//============================================================

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
	{
		logerror("error: malloc failed in osd_dirname\n");
		return NULL;
	}

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}



//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}



//============================================================
//	osd_path_separator
//============================================================

const char *osd_path_separator(void)
{
	return "\\";
}



//============================================================
//	osd_is_path_separator
//============================================================

int osd_is_path_separator(char ch)
{
	return (ch == '\\') || (ch == '/');
}



//============================================================
//	osd_is_absolute_path
//============================================================

int osd_is_absolute_path(const char *path)
{
	int result;

	if (osd_is_path_separator(path[0]))
		result = 1;
#ifndef UNDER_CE
	else if (isalpha(path[0]))
		result = (path[1] == ':');
#endif
	else
		result = 0;
	return result;
}



//============================================================
//	osd_mkdir
//============================================================

void osd_mkdir(const char *dir)
{
	CreateDirectory(A2T(dir), NULL);
}



//============================================================
//	osd_rmdir
//============================================================

void osd_rmdir(const char *dir)
{
	RemoveDirectory(A2T(dir));
}



//============================================================
//	osd_rmfile
//============================================================

void osd_rmfile(const char *filepath)
{
	DeleteFile(A2T(filepath));
}



//============================================================
//	osd_copyfile
//============================================================

void osd_copyfile(const char *destfile, const char *srcfile)
{
	CopyFile(A2T(srcfile), A2T(destfile), TRUE);
}




//============================================================
//	osd_getcurdir
//============================================================

void osd_getcurdir(char *buffer, size_t buffer_len)
{
	GetCurrentDirectory(buffer_len, buffer);
}



//============================================================
//	osd_setcurdir
//============================================================

void osd_setcurdir(const char *dir)
{
	SetCurrentDirectory(A2T(dir));
}




