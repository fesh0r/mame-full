/***************************************************************************

  machine.c


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "includes/exidy.h"
#include "devices/cassette.h"

DEVICE_LOAD( exidy_cassette )
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(image, file, &args);
}
