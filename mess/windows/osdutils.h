#include <windows.h>

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
#define osd_mkdir(dir)			CreateDirectory((dir), NULL)
#define osd_rmdir(dir)			RemoveDirectory(dir)
#define osd_rmfile(file)		DeleteFile(file)
#define osd_copyfile(dest, src)	CopyFile((src), (dest), TRUE)
#endif

INLINE int osd_is_path_separator(char c)
{
	return ((c) == '\\') || ((c) == '/');
}

/* some irritating redefinitions */
#define read	read_
#define write	write_
#define access	access_
#define tell	tell_

#define tcsicmp		_tcsicmp
#define tcsnicmp	_tcsnicmp
#define tcslen		_tcslen

#define PATH_SEPARATOR	'\\'

#define EOLN "\r\n"
