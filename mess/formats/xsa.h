/*
 * extracts .xsa disk images
 *
 * .xsa is "XelaSoft Archive", written to compress disk images. It also
 * runs on a real MSX, which has the advantage that you can store an entire
 * disk image on another disk (since it is compressed). 
 *
 * These files are reasonably popular. The archive can only contain one  
 * file; not necessarily a disk image. gzip and zip are way more efficient
 * and widely accepted so writing is not supported. The only advantage
 * of this format is that there are compressors / decompressors available 
 * for the MSX.
 * 
 * The code is stolen from Alex Wulms' xsd program:
 * 
 * 	http://web.inter.nl.net/users/A.P.Wulms/
 */

#include "driver.h"

/*
 * extracts the actual data from the .xsa file (in memory)
 * 
 */
int msx_xsa_extract (UINT8 *xsadata, int xsalen, UINT8 **dskdata, int *dsklen);

/*
 * returns the size of the decompressed file, and the filename (which
 * is malloc()ed so should be free()ed).
 */
int msx_xsa_id (UINT8 *casdata, int caslen, char **filename, int *dsklen);

