#include "driver.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void pc_runtime_loader_init(void);
# else
	extern void pc_runtime_loader_init(void);
# endif
#endif

struct _PC_SETUP;
extern struct _PC_SETUP pc_setup_at[], pc_setup_t1000hx[];
void pc_init_setup(struct _PC_SETUP *setup);


extern void init_pccga(void);
extern void init_pcmda(void);
void init_europc(void);
void init_bondwell(void);
extern void init_pc200(void);
extern void init_pc1512(void);
extern void init_pc1640(void);
extern void init_pc_vga(void);

extern void pc_mda_init_machine(void);
extern void pc_cga_init_machine(void);
extern void pc_aga_init_machine(void);
extern void pc_vga_init_machine(void);

void init_pc_common(void);
void pc_cga_init(void);
void pc_mda_init(void);
void pc_vga_init(void);

void pc_keyboard(void);
UINT8 pc_keyb_read(void);
void pc_keyb_set_clock(int on);

extern WRITE_HANDLER ( pc_COM1_w );
extern READ_HANDLER ( pc_COM1_r );
extern WRITE_HANDLER ( pc_COM2_w );
extern READ_HANDLER ( pc_COM2_r );
extern WRITE_HANDLER ( pc_COM3_w );
extern READ_HANDLER ( pc_COM3_r );
extern WRITE_HANDLER ( pc_COM4_w );
extern READ_HANDLER ( pc_COM4_r );

extern int  pc_cga_frame_interrupt(void);
extern int  pc_aga_frame_interrupt(void);
extern int  pc_mda_frame_interrupt(void);
extern int  pc_vga_frame_interrupt(void);

/* from sndhrdw/pc.c */
extern int  pc_sh_init(const char *name);
extern int  pc_sh_custom_start(const struct MachineSound *driver);
extern void pc_sh_custom_update(void);
extern void pc_sh_stop(void);
extern void pc_sh_update(int param, INT16 *buff, int length);
extern void pc_sh_speaker(int mode);
void pc_sh_speaker_change_clock(double pc_clock);

/* from machine/pc_fdc.c */
void pc_fdc_setup(void);

