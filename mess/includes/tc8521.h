/* TOSHIBA TC8521 REAL TIME CLOCK */

#include "driver.h"

READ_HANDLER(tc8521_r);
WRITE_HANDLER(tc8521_w);

struct tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(int);
};

void tc8521_init(struct tc8521_interface *);
void tc8521_stop(void);

