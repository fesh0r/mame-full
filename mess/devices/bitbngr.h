#ifndef BITBNGR_H
#define BITBNGR_H

#include "devices/printer.h"

struct bitbanger_config
{
	/* filter function; returns non-zero if input accepted */
	int (*filter)(mess_image *img, const int *pulses, int total_pulses, int total_duration);
	double pulse_threshhold;			/* the maximum duration pulse that we will consider */
	double pulse_tolerance;				/* deviation tolerance for pulses */
	int minimum_pulses;					/* the minimum amount of pulses before we start analyzing */
	int maximum_pulses;					/* the maximum amount of pulses that we will track */
	int begin_pulse_value;				/* the begining value of a pulse */
	int initial_value;					/* the initial value of the bitbanger line */
};

#define IO_BITBANGER IO_PRINTER

extern void bitbanger_specify(struct IODevice *iodev, int count, const struct bitbanger_config *config);

#define CONFIG_DEVICE_BITBANGER(count, config)			\
	if (cfg->device_num-- == 0)							\
	{													\
		static struct IODevice iodev;					\
		bitbanger_specify(&iodev, (count), (config));	\
		cfg->dev = &iodev;								\
	}													\

#endif /* BITBNGR_H */
