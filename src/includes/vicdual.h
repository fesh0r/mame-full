/*************************************************************************

	vicdual.h

*************************************************************************/

#define FROGS_USE_SAMPLES 1

/*----------- defined in sndhrdw/vicdual.c -----------*/

WRITE8_HANDLER( frogs_sh_port2_w );

#if FROGS_USE_SAMPLES

mame_timer *croak_timer;
void croak_callback(int param);

extern struct Samplesinterface frogs_samples_interface;

#else

extern struct discrete_sound_block frogs_discrete_interface[];

#endif
