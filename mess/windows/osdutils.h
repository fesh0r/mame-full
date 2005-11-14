#ifdef UNDER_CE
#include <string.h>
#endif

#define strncmpi	strnicmp

/* some irritating redefinitions */
#define access	access_
#define tell	tell_

#define tcsicmp		_tcsicmp
#define tcsnicmp	_tcsnicmp
#define tcslen		_tcslen

#define PATH_SEPARATOR	'\\'

#define EOLN "\r\n"
