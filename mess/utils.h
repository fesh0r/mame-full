#ifndef UTILS_H
#define UTILS_H

#include <string.h>

char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);

void rtrim(char *buf);


#endif /* UTILS_H */
