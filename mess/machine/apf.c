/***********************************************************************

	apf.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

#include "driver.h"
#include "includes/apf.h"
#include "cassette.h"

int apf_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void apf_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


