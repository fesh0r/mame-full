
extern unsigned char *astrocade_videoram;

int astrocade_load_rom(mess_image *img, mame_file *fp, int open_mode);

extern PALETTE_INIT( astrocade );
extern READ_HANDLER( astrocade_intercept_r );
extern WRITE_HANDLER( astrocade_videoram_w );
extern WRITE_HANDLER( astrocade_magic_expand_color_w );
extern WRITE_HANDLER( astrocade_magic_control_w );
extern WRITE_HANDLER( astrocade_magicram_w );

extern READ_HANDLER( astrocade_video_retrace_r );
extern WRITE_HANDLER( astrocade_vertical_blank_w );
extern WRITE_HANDLER( astrocade_interrupt_enable_w );
extern WRITE_HANDLER( astrocade_interrupt_w );
extern INTERRUPT_GEN( astrocade_interrupt );

WRITE_HANDLER ( astrocade_mode_w );

WRITE_HANDLER ( astrocade_colour_register_w );
WRITE_HANDLER ( astrocade_colour_block_w );
WRITE_HANDLER ( astrocade_colour_split_w );

void astrocade_copy_line(int line);
