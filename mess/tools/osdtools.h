/* osdtools.h
 *
 * OS dependant code for the tools
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#ifdef WIN32

#include <direct.h>
#define osd_mkdir(dir)	mkdir(dir)
#define PATH_SEPARATOR	"\\"
#define strncasecmp	strnicmp

inline const char *basename(const char *name)
{
	const char *s = name;
	const char *result = name;

	for (s = name; *s; s++) {
		if ((*s == '\\') || (*s == '/'))
			result = s + 1;
	}
	return result;
}

#else

#include <unistd.h>
#define osd_mkdir(dir)	mkdir(dir, 0)
#define PATH_SEPARATOR	"/"

#endif

#include "osdepend.h"
