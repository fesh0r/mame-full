
/* in vidhrdw/channelf.c */
extern int channelf_val_reg;
extern int channelf_row_reg;
extern int channelf_col_reg;

extern PALETTE_INIT( channelf );
extern VIDEO_START( channelf );
extern VIDEO_UPDATE( channelf );

/* in sndhrdw/channelf.c */
void channelf_sound_w(int);

int channelf_sh_custom_start(const struct MachineSound* driver);
void channelf_sh_stop(void);
void channelf_sh_custom_update(void);

