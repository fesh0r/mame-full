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

/* some irritating redefinitions */
#define read	read_
#define write	write_
#define access	access_
#define tell	tell_

#define PATH_SEPARATOR	'\\'

#define EOLN "\r\n"
