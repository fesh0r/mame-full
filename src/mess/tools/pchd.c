#include <string.h>
#include "osdepend.h"
#include "imgtool.h"

/* Preliminary PC Hard Drive Image driver.  Based on Mad Hatchet's mkhdimg utility */

static int pchd_create(STREAM *f, const geometry_options *options);

static geometry_ranges pchd_geometry = { {1,1}, {1024,16} };

IMAGEMODULE(pchd,
	"PC Hard Drive image",						/* human readable name */
	"img",										/* file extension */
	IMAGE_USES_CYLINDERS | IMAGE_USES_HEADS,	/* flags */
	NULL,										/* crcfile */
	NULL,										/* crc system name */
	&pchd_geometry,								/* geometry ranges */
	NULL,										/* init function */
	NULL,										/* exit function */
	NULL,										/* info function */
	NULL,										/* begin enumeration */
	NULL,										/* enumerate next */
	NULL,										/* close enumeration */
	NULL,										/* free space on image */
	NULL,										/* read file */
	NULL,										/* write file */
	NULL,										/* delete file */
	pchd_create,								/* create image */
	NULL										/* extract function */
)

#define SECTORS     17
#define MAGIC       0xaa55
#define ECC 		11
#define CONTROL 	5

static int pchd_create(STREAM *f, const geometry_options *options)
{
	size_t s;
	unsigned char buffer[512];

	memset(buffer, 0, sizeof(buffer));
    /* fill in the drive geometry information */
	buffer[0x1ad] = options->cylinders & 0xff;           /* cylinders */
	buffer[0x1ae] = (options->cylinders >> 8) & 3;
	buffer[0x1af] = options->heads;						/* heads */
	buffer[0x1b0] = (options->cylinders+1) & 0xff;		/* write precompensation */
	buffer[0x1b1] = ((options->cylinders+1) >> 8) & 3;
	buffer[0x1b2] = (options->cylinders+1) & 0xff;		/* reduced write current */
	buffer[0x1b3] = ((options->cylinders+1) >> 8) & 3;
	buffer[0x1b4] = ECC;						/* ECC length */
	buffer[0x1b5] = CONTROL;					/* control (step rate) */
	buffer[0x1b6] = options->cylinders & 0xff;			/* parking cylinder */
	buffer[0x1b7] = (options->cylinders >> 8) & 3;
	buffer[0x1b8] = 0x00;						/* no idea */
	buffer[0x1b9] = 0x00;
	buffer[0x1ba] = 0x00;
	buffer[0x1bb] = 0x00;
	buffer[0x1bc] = 0x00;
	buffer[0x1bd] = SECTORS;					/* some non zero value */
	buffer[0x1fe] = MAGIC & 0xff;
	buffer[0x1ff] = (MAGIC >> 8) & 0xff;

	if (stream_write(f, buffer, sizeof(buffer)) != sizeof(buffer))
		return IMGTOOLERR_WRITEERROR;

	/* write F6 patterns throughout the image */
	s = options->cylinders * options->heads * SECTORS * sizeof(buffer);
	if (stream_fill(f, 0xf6, s) != s)
		return IMGTOOLERR_WRITEERROR;

	return 0;
}


