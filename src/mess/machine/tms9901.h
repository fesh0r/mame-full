

/* Masks for the interrupts levels available on TMS9901 */
#define TMS9901_INT1 0x0002
#define TMS9901_INT2 0x0004
#define TMS9901_INT3 0x0008		/* overriden by the timer interrupt */
#define TMS9901_INT4 0x0010
#define TMS9901_INT5 0x0020
#define TMS9901_INT6 0x0040
#define TMS9901_INT7 0x0080
#define TMS9901_INT8 0x0100
#define TMS9901_INT9 0x0200
#define TMS9901_INTA 0x0400
#define TMS9901_INTB 0x0800
#define TMS9901_INTC 0x1000
#define TMS9901_INTD 0x2000
#define TMS9901_INTE 0x4000
#define TMS9901_INTF 0x8000




void tms9901_init(void);
void tms9901_cleanup(void);

void tms9901_set_int2(int state);

int tms9901_CRU_read(int offset);
void tms9901_CRU_write(int offset, int data);






