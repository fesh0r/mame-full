#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* I don't know if these should be defined */
/*#define strcmpi	strcasecmp */
/*#define strncmpi	strncasecmp */

#define osd_mkdir(dir)	mkdir(dir, 0)
