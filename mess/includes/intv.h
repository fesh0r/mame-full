#ifndef __INTV_H
#define __INTV_H

/* in vidhrdw/intv.c */
int intv_vh_start(void);
void intv_vh_stop(void);
void intv_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

int intvkbd_vh_start(void);
void intvkbd_vh_stop(void);
void intvkbd_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

/* in machine/intv.c */

/*  for the console alone... */
extern UINT8 intv_gram[];
extern UINT8 intv_gramdirty[];
extern UINT16 intv_ram16[];

void init_intv(void);
int intv_load_rom (int id);

void intv_machine_init(void);
int intv_interrupt(void);

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_empty_r );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

void stic_screenrefresh();

READ_HANDLER( intv_right_control_r );
READ_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */
extern int intvkbd_text_blanked;

void init_intvkbd(void);
int intvkbd_load_rom (int id);

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

