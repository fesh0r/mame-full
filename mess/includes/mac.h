/*********************************************************************

	mac.h
	
	MESS Macintosh driver declarations

*********************************************************************/

#ifndef MESS_MAC_H
#define MESS_MAC_H

/* from machine/mac.c */
MACHINE_INIT( mac );

DRIVER_INIT(mac128k512k);
DRIVER_INIT(mac512ke);
DRIVER_INIT(macplus);

INTERRUPT_GEN( mac_interrupt );

READ16_HANDLER ( mac_via_r );
WRITE16_HANDLER ( mac_via_w );
READ16_HANDLER ( mac_autovector_r );
WRITE16_HANDLER ( mac_autovector_w );
READ16_HANDLER ( mac_iwm_r );
WRITE16_HANDLER ( mac_iwm_w );
READ16_HANDLER ( mac_scc_r );
WRITE16_HANDLER ( mac_scc_w );
READ16_HANDLER ( macplus_scsi_r );
WRITE16_HANDLER ( macplus_scsi_w );
NVRAM_HANDLER( mac );
void mac_scc_mouse_irq( int x, int y );

/* from vidhrdw/mac.c */
VIDEO_START( mac );
VIDEO_UPDATE( mac );
PALETTE_INIT( mac );

void mac_set_screen_buffer( int buffer );

/* from sndhrdw/mac.c */
int mac_sh_start( const struct MachineSound *msound );
void mac_sh_stop( void );
void mac_sh_update( void );

void mac_enable_sound( int on );
void mac_set_sound_buffer( int buffer );
void mac_set_volume( int volume );

void mac_sh_updatebuffer(void);

#endif /* MESS_MAC_H */

