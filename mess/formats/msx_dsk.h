/*
 * msx_dsk.h : convert several bogus disk image formats to .dsk
 * (.ddi, .img, .msx). None of these have any right of existance!
 */

#include "driver.h"

int msx_dsk_convert (UINT8 *indata, int inlen, const char *fname, 
	UINT8 **outdata, int *outlen);
 
#define MSX_OK			(0)
#define MSX_IMAGE_ERROR (1)
#define MSX_OUTOFMEMORY (2)

#define MSX_MAX_IMAGE_DSK	(0x1800+720*1024)

