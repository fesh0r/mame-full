/* from machine/mac.c */
extern void mac_init_machine( void );
extern void init_mac512ke( void );
extern void init_macplus( void );
extern int mac_vblank_irq( void );
extern int mac_interrupt( void );
extern READ_HANDLER ( mac_via_r );
extern WRITE_HANDLER ( mac_via_w );
extern READ_HANDLER ( mac_autovector_r );
extern WRITE_HANDLER ( mac_autovector_w );
extern READ_HANDLER ( mac_iwm_r );
extern WRITE_HANDLER ( mac_iwm_w );
extern READ_HANDLER ( mac_scc_r );
extern WRITE_HANDLER ( mac_scc_w );
extern READ_HANDLER ( macplus_scsi_r );
extern WRITE_HANDLER ( macplus_scsi_w );
extern int mac_floppy_init(int id);
extern void mac_floppy_exit(int id);
extern void mac_nvram_handler(void *file, int read_or_write);
extern void mac_scc_mouse_irq( int x, int y );

extern int mac_ram_size;
extern UINT8 *mac_ram_ptr;

/* from vidhrdw/mac.c */
extern int mac_vh_start(void);
extern void mac_vh_stop(void);
extern void mac_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* from sndhrdw/mac.c */
extern int mac_sh_start( const struct MachineSound *msound );
extern void mac_sh_stop( void );
extern void mac_sh_update( void );

extern void mac_enable_sound( int on );
extern void mac_set_buffer( int buffer );
extern void mac_set_volume( int volume );

void mac_sh_data_w(int index_loc);


