#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define strcmpi		stricmp
#define strncmpi	strnicmp

#define osd_mkdir(dir)	mkdir(dir)
#define PATH_SEPARATOR	'\\'
