#include <stdlib.h>
#include "driver.h"

/* This code decodes VDK (Virtual DisK) format disks.  This is a disk image
 * format introduced in Paul Burgin's PC-Dragon emulator.  It is essentially
 * a basic disk with a header that specifies geometry information
 *
 * This module provides two calls - one to decode headers and one to encode
 * headers.  When a header is being decoded, you point it at the binary data
 * that comprises the header (must be VDK_HEADER_LEN bytes long) and it returns
 * the geometry info, including an offset.  When encoding a header, you pass in
 * geometry info and a buffer to receive the header, which is VDK_HEADER_LEN
 * bytes long
 */

#define VDK_HEADER_LEN	12

/* Returns INIT_PASS if successful, INIT_FAIL if not */
int cocovdk_decode_header(void *hdr, UINT8 *tracks, UINT8 *sides, UINT8 *sec_per_track, UINT16 *sector_length, size_t *offset);
int cocovdk_encode_header(void *hdr, UINT8 tracks, UINT8 sides, UINT8 sec_per_track, UINT16 sector_length);
