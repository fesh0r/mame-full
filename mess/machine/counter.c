#include <stdlib.h>
#include "timer.h"
#include "machine/counter.h"

typedef struct {
	void *timer;
	double period;
	void (*callback)(int);
	int param;
} counter_t;

static void counter_callback(int param)
{
	counter_t *counter;

	counter = (counter_t *) param;
	counter->callback(param);
	free(counter);
}

static void counter_maketimer(counter_t *counter, unsigned long value)
{
	void *newtimer;
	
	newtimer = timer_set(counter->period * value - fmod(timer_get_time(), counter->period), (int) counter, counter_callback);
	if (newtimer) {
		if (counter->timer)
			timer_remove(counter->timer);
		counter->timer = newtimer;
	}
}

void *counter_set(unsigned long value, double period, int param, void(*callback)(int))
{
	counter_t *counter;

	counter = malloc(sizeof(counter_t));
	if (!counter)
		return NULL;

	counter->timer = NULL;
	counter->callback = callback;
	counter->param = param;
	counter->period = period;
	counter_maketimer(counter, value);

	return (void *) counter;
}

unsigned long counter_get_value(void *which)
{
	counter_t *counter;

	counter = (counter_t *) which;
	return (unsigned long) (timer_timeleft(counter->timer) / counter->period + 0.5);
}

void counter_set_value(void *which, unsigned long value)
{
	counter_t *counter;

	counter = (counter_t *) which;
	counter_maketimer(counter, value);
}

void counter_set_period(void *which, double period)
{
	counter_t *counter;
	unsigned long value;

	counter = (counter_t *) which;
	if (counter->period != period) {
		value = counter_get_value(which);
		counter->period = period;
		counter_maketimer(counter, value);
	}
}

void counter_remove(void *which)
{
	counter_t *counter;

	counter = (counter_t *) which;
	timer_remove(counter->timer);
	free(counter);
}

