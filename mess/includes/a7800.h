#ifndef A7800_H
#define A7800_H

/* vidhrdw/a7800.c */
VIDEO_START( a7800 );
VIDEO_UPDATE( a7800 );
void a7800_interrupt(void);
READ_HANDLER  ( a7800_MARIA_r);
WRITE_HANDLER ( a7800_MARIA_w );


/* machine/a7800.c */
extern int a7800_lines;
extern int a7800_ispal;

extern unsigned char *a7800_ram;
MACHINE_INIT( a7800 );
UINT32 a7800_partialcrc(const unsigned char *, size_t);

DEVICE_LOAD( a7800_cart );

READ_HANDLER  ( a7800_TIA_r );
WRITE_HANDLER ( a7800_TIA_w );
READ_HANDLER  ( a7800_RIOT_r );
WRITE_HANDLER ( a7800_RIOT_w );
WRITE_HANDLER ( a7800_RAM0_w );
WRITE_HANDLER ( a7800_cart_w );

DRIVER_INIT( a7800_ntsc );
DRIVER_INIT( a7800_pal );

#endif /* A7800_H */
