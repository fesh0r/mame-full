/***************************************************************************

  machine.c


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "includes/exidy.h"
#include "devices/cassette.h"

int exidy_cassette_init(mess_image *img, mame_file *fp, int open_mode)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, fp, open_mode, &args);
}
