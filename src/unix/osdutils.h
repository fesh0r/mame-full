#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define strcmpi	strcasecmp
#define strncmpi strncasecmp

#define osd_mkdir(dir)	mkdir(dir, 0)

#define EOLN "\r\n"
