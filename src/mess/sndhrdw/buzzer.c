
/***************************************************************************

    sndhrdw/buzzer.c

	Functions to support the Jupiter Ace buzzer

****************************************************************************/
#include "driver.h"
#include "sound/streams.h"

static int buzzer_sh_channel = 0;
       int buzzer_sh_state = 0;
static int buzzer_sh_buzlen = 0;
static int buzzer_sh_buzptr = 0;
static INT8 *buzzer_sh_buzbuf = NULL;

void	buzzer_sh_timer (int param)

{

if (buzzer_sh_buzptr < buzzer_sh_buzlen) {
  if (buzzer_sh_state) {
	if (buzzer_sh_buzbuf[buzzer_sh_buzptr - 1] > 0) {
	  buzzer_sh_buzbuf[buzzer_sh_buzptr] =
						buzzer_sh_buzbuf[buzzer_sh_buzptr - 1] * 4;
	  if (buzzer_sh_buzbuf[buzzer_sh_buzptr] < 0)
	  					buzzer_sh_buzbuf[buzzer_sh_buzptr] = 127;
	} else buzzer_sh_buzbuf[buzzer_sh_buzptr] = 31;
  } else {
	buzzer_sh_buzbuf[buzzer_sh_buzptr] =
  			buzzer_sh_buzbuf[buzzer_sh_buzptr - 1] / 4;
  }
}
buzzer_sh_buzptr++;

}
void buzzer_sh_update (int param, INT16 *buffer, int length)

{

UINT8	lastsamp;

memset (buffer, 0, length);
if (buzzer_sh_buzptr >= length) {
  memcpy (buffer, buzzer_sh_buzbuf, length);
  lastsamp = buzzer_sh_buzbuf[length - 1];
} else {
  memcpy (buffer, buzzer_sh_buzbuf, buzzer_sh_buzptr - 1);
  lastsamp = buzzer_sh_buzbuf[buzzer_sh_buzptr - 1];
}

memset (buzzer_sh_buzbuf, 0, buzzer_sh_buzlen);
buzzer_sh_buzbuf[0] = lastsamp;
buzzer_sh_buzptr = 1;

}

int buzzer_sh_start (void)

{

fprintf (stdout, "buzzer_sh_start\n");
if (errorlog) fprintf (errorlog, "buzzer_sh_start\n");
if (!(buzzer_sh_buzbuf = malloc (buzzer_sh_buzlen =
	  (Machine->sample_rate / Machine->gamedrv->drv->frames_per_second * 2))))
																return (1);
timer_pulse (1.0 / (double)Machine->sample_rate, 0, buzzer_sh_timer);
buzzer_sh_state = 0;
buzzer_sh_buzptr = 1;
buzzer_sh_channel = stream_init ("Ace Buzzer", 50, Machine->sample_rate, 8, buzzer_sh_update);
return 0;

}

void buzzer_sh_stop (void)

{

if (errorlog) fprintf (errorlog, "buzzer_sh_stop\n");

}

