#ifndef KIM1_H
#define KIM1_H

/* from mess/machine/kim1.c */
DRIVER_INIT( kim1 );
MACHINE_INIT( kim1 );

DEVICE_LOAD( kim1_cassette );

#define CONFIG_DEVICE_KIM1_CASSETTE \
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "kim\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_READ, NULL, NULL, device_load_kim1_cassette, NULL, NULL)

INTERRUPT_GEN( kim1_interrupt );

 READ8_HANDLER ( m6530_003_r );
 READ8_HANDLER ( m6530_002_r );
 READ8_HANDLER ( kim1_mirror_r );

WRITE8_HANDLER ( m6530_003_w );
WRITE8_HANDLER ( m6530_002_w );
WRITE8_HANDLER ( kim1_mirror_w );

/* from mess/vidhrdw/kim1.c */
PALETTE_INIT( kim1 );
VIDEO_START( kim1 );
VIDEO_UPDATE( kim1 );

#endif /* KIM1_H */
