#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "mame.h"
#include "bitbngr.h"
#include "printer.h"

struct bitbanger_info
{
	const struct bitbanger_config *config;
	double last_pulse_time;
	double *pulses;
	int *factored_pulses;
	int recorded_pulses;
	int value;
	void *timeout_timer;
	int over_threshhold;
};

static struct bitbanger_info *bitbangers[MAX_PRINTER];

int bitbanger_init(int id, const struct bitbanger_config *config)
{
	struct bitbanger_info *bi;

	bi = (struct bitbanger_info *) auto_malloc(sizeof(struct bitbanger_info));
	if (!bi)
		return INIT_FAIL;

	bi->pulses = (double *) auto_malloc(config->maximum_pulses * sizeof(double));
	if (!bi->pulses)
		return INIT_FAIL;

	bi->factored_pulses = (int *) auto_malloc(config->maximum_pulses * sizeof(int));
	if (!bi->factored_pulses)
		return INIT_FAIL;

	bi->config = config;
	bi->last_pulse_time = 0.0;
	bi->recorded_pulses = 0;
	bi->value = config->initial_value;
	bi->timeout_timer = NULL;
	bi->over_threshhold = 1;

	bitbangers[id] = bi;
	return printer_init(id);
}

static void bitbanger_analyze(int id, struct bitbanger_info *bi)
{
	int i, factor, total_duration;
	double smallest_pulse;
	double d;

	/* compute the smallest pulse */
	smallest_pulse = bi->config->pulse_threshhold;
	for (i = 0; i < bi->recorded_pulses; i++)
		if (smallest_pulse > bi->pulses[i])
			smallest_pulse = bi->pulses[i];

	/* figure out what factor the smallest pulse was */
	factor = 0;
	do
	{
		factor++;
		total_duration = 0;

		for (i = 0; i < bi->recorded_pulses; i++)
		{
			d = bi->pulses[i] / smallest_pulse * factor + bi->config->pulse_tolerance;
			if ((i < (bi->recorded_pulses-1)) && (d - floor(d)) >= (bi->config->pulse_tolerance * 2))
			{
				i = -1;
				break;
			}
			bi->factored_pulses[i] = (int) d;
			total_duration += (int) d;
		}
	}
	while((i == -1) && (factor < bi->config->maximum_pulses));
	if (i == -1)
		return;

	/* filter the output */
	if (bi->config->filter(id, bi->factored_pulses, bi->recorded_pulses, total_duration))
		bi->recorded_pulses = 0;
}

static void bitbanger_addpulse(int id, struct bitbanger_info *bi, double pulse_width)
{
	/* exceeded total countable pulses? */
	if (bi->recorded_pulses == bi->config->maximum_pulses)
		memmove(bi->pulses, bi->pulses + 1, (--bi->recorded_pulses) * sizeof(double));

	/* record the pulse */
	bi->pulses[bi->recorded_pulses++] = pulse_width;

	/* analyze, if necessary */
	if (bi->recorded_pulses >= bi->config->minimum_pulses)
		bitbanger_analyze(id, bi);
}

static void bitbanger_overthreshhold(int id)
{
	struct bitbanger_info *bi = bitbangers[id];
	bitbanger_addpulse(id, bi, timer_get_time() - bi->last_pulse_time);
	bi->over_threshhold = 1;
	bi->recorded_pulses = 0;
	bi->timeout_timer = NULL;
}

void bitbanger_output(int id, int value)
{
	struct bitbanger_info *bi = bitbangers[id];
	double current_time;
	double pulse_width;

	/* normalize input */
	value = value ? 1 : 0;

	/* only meaningful if we change */
	if (bi->value != value)
	{
		current_time = timer_get_time();
		pulse_width = current_time - bi->last_pulse_time;

		assert(pulse_width > 0);

		if (!bi->over_threshhold && ((bi->recorded_pulses > 0) || (bi->value == bi->config->initial_value)))
			bitbanger_addpulse(id, bi, pulse_width);

		/* update state */
		bi->value = value;
		bi->last_pulse_time = current_time;
		bi->over_threshhold = 0;

		/* update timeout timer */
		/* remove timeout timer, if need be */
		if (bi->timeout_timer)
			timer_reset(bi->timeout_timer, bi->config->pulse_threshhold);
		else
			bi->timeout_timer = timer_set(bi->config->pulse_threshhold, id, bitbanger_overthreshhold);
	}
}



