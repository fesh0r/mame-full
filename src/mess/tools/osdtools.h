/* osdtools.h
 *
 * OS dependant code for the tools
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
//#include <direct.h>
#include <string.h>

#ifdef WIN32

#define osd_mkdir(dir)	mkdir(dir)
#define PATH_SEPARATOR	"\\"
#define strncasecmp	strnicmp

#else

#include <unistd.h>
#define osd_mkdir(dir)	mkdir(dir, 0)
#define PATH_SEPARATOR	"/"

#endif

#include "osdepend.h"
