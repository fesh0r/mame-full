/*
 * msx_dsk.h : convert several bogus disk image formats to .dsk
 * (.ddi, .img, .msx). None of these have any right of existance!
 */

#include "msx_dsk.h"

int msx_dsk_convert (UINT8 *indata, int inlen, const char *fname,
        UINT8 **outdata, int *outlen)
    {
    int offset = 0, i;

    switch (inlen)
		{
		case (360*1024+1): /* 1DD .img file */
			if (indata[0] != 1) return MSX_IMAGE_ERROR;
			*outlen = inlen - 1;
			offset = 1;
			break;
		case (720*1024+1): /* 2DD .img file */
			if (indata[0] != 2) return MSX_IMAGE_ERROR;
			*outlen = inlen - 1;
			offset = 1;
			break;
		case (720*1024+0x1800): /* DiskDupe 5.12 image */
			*outlen = inlen - 0x1800;
			offset = 0;
			break;
		case (720*1024): /* .msx image */
			if (fname && !strcasecmp (fname + strlen (fname) - 4, ".msx") )
				{
				/* special case */
				*outlen = inlen;
    			*outdata = malloc (*outlen);
				if (!*outdata) return MSX_OUTOFMEMORY;

				for (i=0;i<80;i++)
					{
					/* side 1 */
					memcpy (*outdata + i * 0x2400, 
						indata + i * 0x1200, 0x1200);
					/* side 2 */
					memcpy (*outdata + i * 0x2400 + 0x1200, 
						indata + i * 0x1200 + 360*1024, 0x1200);
					}

				return 0;
				}
			/* fall through */
		default:
			return MSX_IMAGE_ERROR;
		}

    *outdata = malloc (*outlen);
	if (!*outdata) return MSX_OUTOFMEMORY;

   	memcpy (*outdata, indata + offset, *outlen);

	return 0;
	}
