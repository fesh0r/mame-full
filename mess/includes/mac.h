/* from machine/mac.c */
extern MACHINE_INIT( mac );
extern void init_mac512ke( void );
extern void init_macplus( void );
extern INTERRUPT_GEN( mac_interrupt );
extern READ16_HANDLER ( mac_via_r );
extern WRITE16_HANDLER ( mac_via_w );
extern READ16_HANDLER ( mac_autovector_r );
extern WRITE16_HANDLER ( mac_autovector_w );
extern READ16_HANDLER ( mac_iwm_r );
extern WRITE16_HANDLER ( mac_iwm_w );
extern READ16_HANDLER ( mac_scc_r );
extern WRITE16_HANDLER ( mac_scc_w );
extern READ16_HANDLER ( macplus_scsi_r );
extern WRITE16_HANDLER ( macplus_scsi_w );
extern DEVICE_LOAD(mac_floppy);
extern DEVICE_UNLOAD(mac_floppy);
extern NVRAM_HANDLER( mac );
extern void mac_scc_mouse_irq( int x, int y );

extern int mac_ram_size;
extern UINT8 *mac_ram_ptr;

/* from vidhrdw/mac.c */
extern VIDEO_START( mac );
extern VIDEO_UPDATE( mac );

extern void mac_set_screen_buffer( int buffer );

/* from sndhrdw/mac.c */
extern int mac_sh_start( const struct MachineSound *msound );
extern void mac_sh_stop( void );
extern void mac_sh_update( void );

extern void mac_enable_sound( int on );
extern void mac_set_sound_buffer( int buffer );
extern void mac_set_volume( int volume );

void mac_sh_data_w(int index_loc);
