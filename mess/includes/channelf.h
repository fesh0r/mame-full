
/* in vidhrdw/channelf.c */
extern int channelf_val_reg;
extern int channelf_row_reg;
extern int channelf_col_reg;

void channelf_init_palette(unsigned char *sys_palette,
                           unsigned short *sys_colortable,
                           const unsigned char *color_prom);
int channelf_vh_start(void);
void channelf_vh_stop(void);
void channelf_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

/* in sndhrdw/channelf.c */
void channelf_sound_w(int);

int channelf_sh_custom_start(const struct MachineSound* driver);
void channelf_sh_stop(void);
void channelf_sh_custom_update(void);

