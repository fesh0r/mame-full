/* register names for apexc_get_reg apexc_set_reg */
enum
{
	APEXC_CR =1,	/* control register */
	APEXC_A,		/* acumulator */
	APEXC_R,		/* register */
	APEXC_ML,		/* memory location */
	APEXC_WS,		/* working store */
	APEXC_FLAGS		/* various flags */
};

extern int apexc_ICount;

void apexc_reset(void *param);
void apexc_exit(void);
unsigned apexc_get_context(void *dst);
void apexc_set_context(void *src);
unsigned apexc_get_pc(void);
void apexc_set_pc(unsigned val);
unsigned apexc_get_sp(void);
void apexc_set_sp(unsigned val);
void apexc_set_nmi_line(int state);
void apexc_set_irq_line(int irqline, int state);
void apexc_set_irq_callback(int (*callback)(int irqline));
unsigned apexc_get_reg(int regnum);
void apexc_set_reg(int regnum, unsigned val);
const char *apexc_info(void *context, int regnum);
unsigned apexc_dasm(char *buffer, unsigned pc);
int apexc_execute(int cycles);

#ifdef MAME_DEBUG
unsigned DasmAPEXC(char *buffer, unsigned pc);
#endif
