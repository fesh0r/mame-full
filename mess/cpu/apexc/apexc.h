/* register names for apexc_get_reg & apexc_set_reg */
enum
{
	APEXC_CR =1,	/* control register */
	APEXC_A,		/* acumulator */
	APEXC_R,		/* register */
	APEXC_ML,		/* memory location */
	APEXC_WS,		/* working store */
	APEXC_STATE,	/* whether CPU is running */

	APEXC_ML_FULL	/* read-only pseudo-register for exclusive use by the control panel code
					in the apexc driver : enables it to get the complete address computed
					from the contents of ML and WS */
};

extern int apexc_ICount;

void apexc_init(void);
void apexc_reset(void *param);
void apexc_exit(void);
unsigned apexc_get_context(void *dst);
void apexc_set_context(void *src);
void apexc_set_irq_line(int irqline, int state);
void apexc_set_irq_callback(int (*callback)(int irqline));
unsigned apexc_get_reg(int regnum);
void apexc_set_reg(int regnum, unsigned val);
const char *apexc_info(void *context, int regnum);
unsigned apexc_dasm(char *buffer, unsigned pc);
int apexc_execute(int cycles);

#ifndef SUPPORT_ODD_WORD_SIZES
#define apexc_readmem(address)	cpu_readmem24bedw_dword((address)<<2)
#define apexc_writemem(address, data)	cpu_writemem24bedw_dword((address)<<2, (data))
/* eewww ! - Fortunately, there is no memory mapped I/O, so we can simulate masked write
without danger */
#define apexc_writemem_masked(address, data, mask)										\
	apexc_writemem((address), (apexc_readmem(address) & ~(mask)) | ((data) & (mask)))
#else
#define apexc_readmem(address)	cpu_readmem13_32(address)
#define apexc_writemem(address, data)	cpu_writemem13_32((address), (data))
#define apexc_writemem_masked(address, data, mask)	cpu_writemem13_32masked((address), (data), (mask))
#endif

#ifdef MAME_DEBUG
unsigned DasmAPEXC(char *buffer, unsigned pc);
#endif

#define apexc_readop(address)	apexc_readmem(address)

