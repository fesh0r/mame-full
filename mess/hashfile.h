/*********************************************************************

	hashfile.h

	Code for parsing hash info (*.hsi) files

*********************************************************************/

#ifndef HASHFILE_H
#define HASHFILE_H

#include "driver.h"
#include "sha1.h"


struct hash_info
{
	UINT32 crc32;
	const char *longname;
	const char *manufacturer;
	const char *year;
	const char *playable;
	const char *extrainfo;
};

typedef struct _hash_file hash_file;


/* Opens a hash file.  If is_preload is non-zero, the entire file is preloaded */
hash_file *hashfile_open(const char *sysname, int is_preload);

/* Closes a hash file and associated resources */
void hashfile_close(hash_file *hashfile);

/* Looks up information in a hash file */
const struct hash_info *hashfile_lookup(hash_file *hashfile,
	UINT32 crc, const UINT8 *sha1, const UINT8 *md5);

/* Performs a syntax check on a hash file */
int hashfile_verify(const char *sysname, void (*error_proc)(const char *message));


#endif /* HASHFILE_H */
