

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


typedef struct tms9901reset_param
{
	int supported_int_mask;	/* a bit for each input pin whose state is always notified to the TMS9901 core */
	int (*read_handlers[4])(int offset);
	void (*write_handlers[16])(int offset, int data);
} tms9901reset_param;


void tms9901_init(tms9901reset_param *param);
void tms9901_cleanup(void);

void tms9901_set_single_int(int pin_number, int state);

READ_HANDLER ( tms9901_CRU_read );
WRITE_HANDLER ( tms9901_CRU_write );

