/***************************************************************************

  machine.c


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "includes/exidy.h"
#include "cassette.h"

int exidy_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void exidy_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}

