/***********************************************************

  utils.h

  Nifty utility code

***********************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <string.h>

/* -----------------------------------------------------------------------
 * osdutils.h is a header file that gives overrides for functions we
 * define below, so if a native implementation is available, we can use
 * it
 * ----------------------------------------------------------------------- */

#include "osdutils.h"

/* -----------------------------------------------------------------------
 * GCC related optimizations
 * ----------------------------------------------------------------------- */

#ifdef __GNUC__
#if (__GNUC__ > 2)
#define FUNCATTR_MALLOC		__attribute__ ((malloc))
#endif

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define FUNCATTR_PURE		__attribute__ ((pure))
#endif

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define FUNCATTR_CONST		__attribute__ ((const))
#endif
#endif /* __GNUC__ */

#ifndef FUNCATTR_MALLOC
#define FUNCATTR_MALLOC
#endif

#ifndef FUNCATTR_PURE
#define FUNCATTR_PURE
#endif

#ifndef FUNCATTR_CONST
#define FUNCATTR_CONST
#endif

#ifndef MIN
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#endif /* MIN */

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

/* -----------------------------------------------------------------------
 * strncpyz
 * strncatz
 *
 * strncpy done right :-)
 * ----------------------------------------------------------------------- */
char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);

/* -----------------------------------------------------------------------
 * rtrim
 *
 * Removes all trailing spaces from a string
 * ----------------------------------------------------------------------- */
void rtrim(char *buf);

/* -----------------------------------------------------------------------
 * strcmpi
 * strncmpi
 *
 * Case insensitive compares.  If your platform has this function then
 * #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef strcmpi
int strcmpi(const char *dst, const char *src);
#endif /* strcmpi */

#ifndef strncmpi
int strncmpi(const char *dst, const char *src, size_t n);
#endif /* strcmpi */

/* -----------------------------------------------------------------------
 * osd_basename
 *
 * Given a pathname, returns the partially qualified path.  If your
 * platform has this function then #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef osd_basename
char *osd_basename (char *name);
#endif /* osd_basename */



/* -----------------------------------------------------------------------
 * osd_mkdir
 * osd_rmdir
 * osd_rmfile
 * osd_copyfile
 * osd_getcurdir
 * osd_setcurdir
 *
 * Misc platform independent dir/file functions.
 * ----------------------------------------------------------------------- */

#ifndef osd_mkdir
void osd_mkdir(const char *dir);
#endif /* osd_mkdir */

#ifndef osd_rmdir
void osd_rmdir(const char *dir);
#endif /* osd_rmdir */

#ifndef osd_rmfile
void osd_rmfile(const char *filepath);
#endif /* osd_rmfile */

#ifndef osd_copyfile
void osd_copyfile(const char *destfile, const char *srcfile);
#endif /* osd_copyfile */

#ifndef osd_getcurdir
void osd_getcurdir(char *buffer, size_t buffer_len);
#endif /* osd_getcurdir */

#ifndef osd_getcurdir
void osd_setcurdir(const char *dir);
#endif /* osd_getcurdir */



/* -----------------------------------------------------------------------
 * memset16
 *
 * 16 bit memset
 * ----------------------------------------------------------------------- */
#ifndef memset16
void *memset16 (void *dest, int value, size_t size);
#endif /* memset16 */


/* -----------------------------------------------------------------------
 * CRC stuff
 * ----------------------------------------------------------------------- */
unsigned short ccitt_crc16(unsigned short crc, const unsigned char *buffer, size_t buffer_len);
unsigned short ccitt_crc16_one( unsigned short crc, const unsigned char data );

/* -----------------------------------------------------------------------
 * Miscellaneous
 * ----------------------------------------------------------------------- */

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR	'/'
#endif

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))



/* miscellaneous functions */
char *stripspace(const char *src);
char *strip_extension(const char *filename);
int compute_log2(int val);
int findextension(const char *extensions, const char *ext);
int hexdigit(char c);



/* Endian macros */
#define FLIPENDIAN_INT16(x)	(((((UINT16) (x)) >> 8) | ((x) << 8)) & 0xffff) 
#define FLIPENDIAN_INT32(x)	((((UINT32) (x)) << 24) | (((UINT32) (x)) >> 24) | \
	(( ((UINT32) (x)) & 0x0000ff00) << 8) | (( ((UINT32) (x)) & 0x00ff0000) >> 8))
#define FLIPENDIAN_INT64(x)	\
	(												\
		(((((UINT64) (x)) >> 56) & ((UINT64) 0xFF)) <<  0)	|	\
		(((((UINT64) (x)) >> 48) & ((UINT64) 0xFF)) <<  8)	|	\
		(((((UINT64) (x)) >> 40) & ((UINT64) 0xFF)) << 16)	|	\
		(((((UINT64) (x)) >> 32) & ((UINT64) 0xFF)) << 24)	|	\
		(((((UINT64) (x)) >> 24) & ((UINT64) 0xFF)) << 32)	|	\
		(((((UINT64) (x)) >> 16) & ((UINT64) 0xFF)) << 40)	|	\
		(((((UINT64) (x)) >>  8) & ((UINT64) 0xFF)) << 48)	|	\
		(((((UINT64) (x)) >>  0) & ((UINT64) 0xFF)) << 56)		\
	)

#ifdef LSB_FIRST
#define BIG_ENDIANIZE_INT16(x)		(FLIPENDIAN_INT16(x))
#define BIG_ENDIANIZE_INT32(x)		(FLIPENDIAN_INT32(x))
#define BIG_ENDIANIZE_INT64(x)		(FLIPENDIAN_INT64(x))
#define LITTLE_ENDIANIZE_INT16(x)	(x)
#define LITTLE_ENDIANIZE_INT32(x)	(x)
#define LITTLE_ENDIANIZE_INT64(x)	(x)
#else
#define BIG_ENDIANIZE_INT16(x)		(x)
#define BIG_ENDIANIZE_INT32(x)		(x)
#define BIG_ENDIANIZE_INT64(x)		(x)
#define LITTLE_ENDIANIZE_INT16(x)	(FLIPENDIAN_INT16(x))
#define LITTLE_ENDIANIZE_INT32(x)	(FLIPENDIAN_INT32(x))
#define LITTLE_ENDIANIZE_INT64(x)	(FLIPENDIAN_INT64(x))
#endif /* LSB_FIRST */

#endif /* UTILS_H */
