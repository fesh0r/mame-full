
extern unsigned char *astrocade_videoram;

int astrocade_load_rom(int id);
int astrocade_id_rom(int id);

void astrocade_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
READ_HANDLER ( astrocade_intercept_r );
WRITE_HANDLER ( astrocade_videoram_w );
WRITE_HANDLER ( astrocade_magic_expand_color_w );
WRITE_HANDLER ( astrocade_magic_control_w );
WRITE_HANDLER ( astrocade_magicram_w );

void astrocade_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
READ_HANDLER ( astrocade_video_retrace_r );
WRITE_HANDLER ( astrocade_vertical_blank_w );
WRITE_HANDLER ( astrocade_interrupt_enable_w );
WRITE_HANDLER ( astrocade_interrupt_w );
int  astrocade_interrupt(void);

WRITE_HANDLER ( astrocade_mode_w );

int  astrocade_vh_start(void);

WRITE_HANDLER ( astrocade_colour_register_w );
WRITE_HANDLER ( astrocade_colour_block_w );
WRITE_HANDLER ( astrocade_colour_split_w );

extern void AstrocadeCopyLine(int Line);