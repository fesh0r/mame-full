/* vidhrdw/a2600.c */
extern int a2600_vh_start(void);
extern void a2600_vh_stop(void);
extern int a2600_scanline_interrupt(void);
extern void a2600_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);


/* machine/a2600.c */
extern READ_HANDLER  ( a2600_TIA_r );
extern WRITE_HANDLER ( a2600_TIA_w );
extern READ_HANDLER  ( a2600_riot_r );
extern WRITE_HANDLER ( a2600_riot_w );
extern READ_HANDLER  ( a2600_bs_r );

extern void a2600_init_machine(void);
extern void a2600_stop_machine(void);
extern int  a2600_id_rom (int id);
extern int  a2600_load_rom(int id);
extern READ_HANDLER ( a2600_ROM_r );

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void vcs_runtime_loader_init(void);
# else
	extern void vcs_runtime_loader_init(void);
# endif
#endif


