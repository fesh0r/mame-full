/* machine/ti85.c */

extern UINT8 ti85_LCD_memory_base;
extern UINT8 ti85_LCD_contrast;
extern UINT8 ti85_LCD_status;
extern UINT8 ti85_timer_interrupt_mask;
extern void ti81_init_machine (void);
extern void ti81_stop_machine (void);
extern void ti85_init_machine (void);
extern void ti85_stop_machine (void);
extern void ti86_init_machine (void);
extern void ti86_stop_machine (void);
extern int ti85_load_snap (int);
extern void ti85_exit_snap (int);
extern int ti85_serial_init (int);
extern void ti85_serial_exit (int);
extern void ti81_nvram_handler (void *, int);
extern void ti85_nvram_handler (void *, int);
extern void ti86_nvram_handler (void *, int);
extern WRITE_HANDLER( ti81_port_0007_w);
extern READ_HANDLER( ti85_port_0000_r);
extern READ_HANDLER( ti85_port_0001_r);
extern READ_HANDLER( ti85_port_0002_r);
extern READ_HANDLER( ti85_port_0003_r);
extern READ_HANDLER( ti85_port_0004_r);
extern READ_HANDLER( ti85_port_0005_r);
extern READ_HANDLER( ti85_port_0006_r);
extern READ_HANDLER( ti85_port_0007_r);
extern READ_HANDLER( ti86_port_0005_r);
extern READ_HANDLER( ti86_port_0006_r);
extern WRITE_HANDLER( ti85_port_0000_w);
extern WRITE_HANDLER( ti85_port_0001_w);
extern WRITE_HANDLER( ti85_port_0002_w);
extern WRITE_HANDLER( ti85_port_0003_w);
extern WRITE_HANDLER( ti85_port_0004_w);
extern WRITE_HANDLER( ti85_port_0005_w);
extern WRITE_HANDLER( ti85_port_0006_w);
extern WRITE_HANDLER( ti85_port_0007_w);
extern WRITE_HANDLER( ti86_port_0005_w);
extern WRITE_HANDLER( ti86_port_0006_w);

/* vidhrdw/ti85.c */
extern VIDEO_START( ti85 );
extern VIDEO_UPDATE( ti85 );
extern unsigned char ti85_palette[32*7][3];
extern unsigned short ti85_colortable[32][7];
extern void ti85_init_palette (unsigned char *, unsigned short *, const unsigned char *);
												
