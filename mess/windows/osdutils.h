#ifdef UNDER_CE
#include <string.h>
#else
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#define strcmpi		stricmp
#define strncmpi	strnicmp

#ifndef UNDER_CE
#define osd_mkdir(dir)	mkdir(dir)
#endif

#define PATH_SEPARATOR	'\\'

#define EOLN "\r\n"
