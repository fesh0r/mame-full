#ifndef UNISTD_H
#define UNISTD_H

#include <stdlib.h>

#define S_IFDIR		0x01
#define _S_IFDIR	0x01

struct stat
{
	int st_mode;
};

int stat( const char *path, struct stat *buffer );

#ifndef errno
#define errno GetLastError()
#endif

#endif /* UNISTD_H */


