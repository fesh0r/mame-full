// MAME headers
#include "osdepend.h"
#include "sysdep/misc.h"

/* return the current number of cycles, or some other high-resolution timer */
cycles_t osd_cycles(void)
{
	return uclock();
}

/* return the number of cycles per second */
cycles_t osd_cycles_per_second(void)
{
	return UCLOCKS_PER_SEC;
}
