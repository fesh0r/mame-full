#ifndef __INTV_H
#define __INTV_H

/* in vidhrdw/intv.c */
extern VIDEO_START( intv );
extern VIDEO_UPDATE( intv );
extern VIDEO_START( intvkbd );
extern VIDEO_UPDATE( intvkbd );

/* in machine/intv.c */

/*  for the console alone... */
extern UINT8 intv_gramdirty;
extern UINT8 intv_gram[];
extern UINT8 intv_gramdirtybytes[];
extern UINT16 intv_ram16[];

extern DRIVER_INIT( intv );

int intv_cart_init(int id);
int intv_cart_load(mess_image *img, mame_file *fp, int open_mode);

extern MACHINE_INIT( intv );
extern INTERRUPT_GEN( intv_interrupt );

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_empty_r );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

void stic_screenrefresh(void);

READ_HANDLER( intv_right_control_r );
READ_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */
extern int intvkbd_text_blanked;

extern DRIVER_INIT( intvkbd );
int intvkbd_cart_load (mess_image *img, mame_file *fp, int open_mode);

READ16_HANDLER ( intvkbd_dualport16_r );
WRITE16_HANDLER ( intvkbd_dualport16_w );
READ_HANDLER ( intvkbd_dualport8_lsb_r );
WRITE_HANDLER ( intvkbd_dualport8_lsb_w );
READ_HANDLER ( intvkbd_dualport8_msb_r );
WRITE_HANDLER ( intvkbd_dualport8_msb_w );

READ_HANDLER ( intvkbd_tms9927_r );
WRITE_HANDLER ( intvkbd_tms9927_w );

/* in sndhrdw/intv.c */
READ16_HANDLER( AY8914_directread_port_0_lsb_r );
WRITE16_HANDLER( AY8914_directwrite_port_0_lsb_w );

#endif

