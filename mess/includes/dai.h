#ifndef DAI_H
#define DAI_H

#define DAI_DEBUG	1

/* machine/dai.c */
MACHINE_INIT( dai );
READ_HANDLER( dai_io_discrete_devices_r );
WRITE_HANDLER( dai_io_discrete_devices_w );

/* vidhrdw/dai.c */
extern unsigned char dai_palette[16*3];
extern unsigned short dai_colortable[1][16];
VIDEO_START( dai );
VIDEO_UPDATE( dai );
PALETTE_INIT( dai );
void dai_update_palette (UINT8);

#endif /* DAI_H */
