/*
 * osdutils.h
 * ----------
 *
 */

#include <sys/stat.h>

#define osd_mkdir(dir) mkdir(dir, S_IWUSR)

#define EOLN "\r\n"
