/* TOSHIBA TC8521 REAL TIME CLOCK */

#include "driver.h"

READ_HANDLER(tc8521_r);
WRITE_HANDLER(tc8521_w);

struct tc8521_interface
{
        /* tc8521 causes an interrupt */
      void (*interrupt_1hz_callback)(int);
      void (*interrupt_16hz_callback)(int);
};

void tc8521_init(struct tc8521_interface *);
void tc8521_stop(void);

