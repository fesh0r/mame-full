#include "driver.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void pc_runtime_loader_init(void);
# else
	extern void pc_runtime_loader_init(void);
# endif
#endif

extern void init_pccga(void);
extern void init_pcmda(void);
void init_europc(void);
void init_bondwell(void);
extern void init_pc200(void);
extern void init_pc1512(void);
extern void init_pc1640(void);
extern void init_pc_vga(void);
extern void init_t1000hx(void);

extern void pc_mda_init_machine(void);
extern void pc_cga_init_machine(void);
extern void pc_t1t_init_machine(void);
extern void pc_aga_init_machine(void);
extern void pc_vga_init_machine(void);

extern int  pc_cga_frame_interrupt(void);
extern int  pc_mda_frame_interrupt(void);
int tandy1000_frame_interrupt (void);
extern int  pc_aga_frame_interrupt(void);
extern int  pc_vga_frame_interrupt(void);

