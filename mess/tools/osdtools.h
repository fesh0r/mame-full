/* osdtools.h
 *
 * OS dependant code for the tools
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

//#include "mess/utils.h"
#include "utils.h"

#ifdef WIN32

#include <direct.h>
#define osd_mkdir(dir)	mkdir(dir)
#define PATH_SEPARATOR	"\\"

#else

#include <unistd.h>
#define osd_mkdir(dir)	mkdir(dir, 0)
#define PATH_SEPARATOR	"/"

#endif

#include "osdepend.h"
