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

extern MACHINE_INIT( pc_mda );
extern MACHINE_INIT( pc_cga );
extern MACHINE_INIT( pc_t1t );
extern MACHINE_INIT( pc_aga );
extern MACHINE_INIT( pc_vga );

extern void pc_cga_frame_interrupt(void);
extern void pc_mda_frame_interrupt(void);
extern void tandy1000_frame_interrupt (void);
extern void pc_aga_frame_interrupt(void);
extern void pc_vga_frame_interrupt(void);

