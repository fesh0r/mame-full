/* machine/apple1.c */

extern void apple1_init_machine (void);
extern void apple1_stop_machine (void);
extern int apple1_interrupt (void);
extern READ_HANDLER( apple1_pia0_kbdin );
extern READ_HANDLER( apple1_pia0_dsprdy );
extern READ_HANDLER( apple1_pia0_kbdrdy );
extern WRITE_HANDLER( apple1_pia0_dspout );

/* vidhrdw/apple1.c */

extern int apple1_vh_start (void);
extern void apple1_vh_stop (void);
extern void apple1_vh_screenrefresh (struct osd_bitmap *bitmap,
														int full_refresh);
extern void apple1_vh_dsp_w (int data);
extern void apple1_vh_dsp_clr (void);

/* systems/apple1.c */

extern struct GfxLayout apple1_charlayout;
