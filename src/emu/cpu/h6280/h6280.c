/*****************************************************************************

    h6280.c - Portable HuC6280 emulator

    Copyright (c) 1999, 2000 Bryan McPhail, mish@tendril.co.uk

    This source code is based (with permission!) on the 6502 emulator by
    Juergen Buchmueller.  It is released as part of the Mame emulator project.
    Let me know if you intend to use this code in any other project.


    NOTICE:

    This code is around 99% complete!  Several things are unimplemented,
    some due to lack of time, some due to lack of documentation, mainly
    due to lack of programs using these features.

    csh, csl opcodes are not supported.
    set opcode and T flag behaviour are not supported.

    I am unsure if instructions like SBC take an extra cycle when used in
    decimal mode.  I am unsure if flag B is set upon execution of rti.

    Cycle counts should be quite accurate, illegal instructions are assumed
    to take two cycles.


    Changelog, version 1.02:
        JMP + indirect X (0x7c) opcode fixed.
        SMB + RMB opcodes fixed in disassembler.
        change_pc function calls removed.
        TSB & TRB now set flags properly.
        BIT opcode altered.

    Changelog, version 1.03:
        Swapped IRQ mask for IRQ1 & IRQ2 (thanks Yasuhiro)

    Changelog, version 1.04, 28/9/99-22/10/99:
        Adjusted RTI (thanks Karl)
        TST opcodes fixed in disassembler (missing break statements in a case!).
        TST behaviour fixed.
        SMB/RMB/BBS/BBR fixed in disassembler.

    Changelog, version 1.05, 8/12/99-16/12/99:
        Added CAB's timer implementation (note: irq ack & timer reload are changed).
        Fixed STA IDX.
        Fixed B flag setting on BRK.
        Assumed CSH & CSL to take 2 cycles each.

        Todo:  Performance could be improved by precalculating timer fire position.

    Changelog, version 1.06, 4/5/00 - last opcode bug found?
        JMP indirect was doing a EAL++; instead of EAD++; - Obviously causing
        a corrupt read when L = 0xff!  This fixes Bloody Wolf and Trio The Punch!

    Changelog, version 1.07, 3/9/00:
        Changed timer to be single shot - fixes Crude Buster music in level 1.

    Changelog, version 1.08, 8/11/05: (Charles MacDonald)

        Changed timer implementation, no longer single shot and reading the
        timer registers returns the count only. Fixes the following:
        - Mesopotamia: Music tempo & in-game timer
        - Dragon Saber: DDA effects
        - Magical Chase: Music tempo and speed regulation
        - Cadash: Allows the first level to start
        - Turrican: Allows the game to start

        Changed PLX and PLY to set NZ flags. Fixes:
        - Afterburner: Graphics unpacking
        - Aoi Blink: Collision detection with background

        Fixed the decimal version of ADC/SBC to *not* update the V flag,
        only the binary ones do.

        Fixed B flag handling so it is always set outside of an interrupt;
        even after being set by PLP and RTI.

        Fixed P state after reset to set I and B, leaving T, D cleared and
        NVZC randomized (cleared in this case).

        Fixed interrupt processing order (Timer has highest priority followed
        by IRQ1 and finally IRQ2).

    Changelog, version 1.08, 1/07/06: (Rob Bohms)

        Added emulation of the T flag, fixes PCE Ankuku Densetsu title screen

******************************************************************************/
#include "debugger.h"
#include "h6280.h"

extern FILE * errorlog;

static int 	h6280_ICount = 0;

/****************************************************************************
 * The 6280 registers.
 ****************************************************************************/
typedef struct
{
	PAIR  ppc;			/* previous program counter */
    PAIR  pc;           /* program counter */
    PAIR  sp;           /* stack pointer (always 100 - 1FF) */
    PAIR  zp;           /* zero page address */
    PAIR  ea;           /* effective address */
    UINT8 a;            /* Accumulator */
    UINT8 x;            /* X index register */
    UINT8 y;            /* Y index register */
    UINT8 p;            /* Processor status */
    UINT8 mmr[8];       /* Hu6280 memory mapper registers */
    UINT8 irq_mask;     /* interrupt enable/disable */
    UINT8 timer_status; /* timer status */
	UINT8 timer_ack;	/* timer acknowledge */
    INT32 timer_value;    /* timer interrupt */
    INT32 timer_load;		/* reload value */
	INT32 extra_cycles;	/* cycles used taking an interrupt */
    UINT8 nmi_state;
    UINT8 irq_state[3];
    int (*irq_callback)(int irqline);

#if LAZY_FLAGS
    INT32 NZ;             /* last value (lazy N and Z flag) */
#endif
	UINT8 io_buffer;	/* last value written to the PSG, timer, and interrupt pages */
}   h6280_Regs;

static  h6280_Regs  h6280;

#ifdef  MAME_DEBUG /* Need some public segmentation registers for debugger */
UINT8	H6280_debug_mmr[8];
#endif

static void set_irq_line(int irqline, int state);

/* include the macros */
#include "h6280ops.h"

/* include the opcode macros, functions and function pointer tables */
#include "tblh6280.c"

/*****************************************************************************/
static void h6280_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	state_save_register_item("h6280", index, h6280.ppc.w.l);
	state_save_register_item("h6280", index, h6280.pc.w.l);
	state_save_register_item("h6280", index, h6280.sp.w.l);
	state_save_register_item("h6280", index, h6280.zp.w.l);
	state_save_register_item("h6280", index, h6280.ea.w.l);
	state_save_register_item("h6280", index, h6280.a);
	state_save_register_item("h6280", index, h6280.x);
	state_save_register_item("h6280", index, h6280.y);
	state_save_register_item("h6280", index, h6280.p);
	state_save_register_item_array("h6280", index, h6280.mmr);
	state_save_register_item("h6280", index, h6280.irq_mask);
	state_save_register_item("h6280", index, h6280.timer_status);
	state_save_register_item("h6280", index, h6280.timer_ack);
	state_save_register_item("h6280", index, h6280.timer_value);
	state_save_register_item("h6280", index, h6280.timer_load);
	state_save_register_item("h6280", index, h6280.extra_cycles);
	state_save_register_item("h6280", index, h6280.nmi_state);
	state_save_register_item("h6280", index, h6280.irq_state[0]);
	state_save_register_item("h6280", index, h6280.irq_state[1]);
	state_save_register_item("h6280", index, h6280.irq_state[2]);

	#if LAZY_FLAGS
	state_save_register_item("h6280", index, h6280.NZ);
	#endif
	state_save_register_item("h6280", index, h6280.io_buffer);

	h6280.irq_callback = irqcallback;
}

static void h6280_reset(void)
{
	int (*save_irqcallback)(int);
	int i;

	/* wipe out the h6280 structure */
	save_irqcallback = h6280.irq_callback;
	memset(&h6280, 0, sizeof(h6280_Regs));
	h6280.irq_callback = save_irqcallback;

	/* set I and B flags */
	P = _fI | _fB;

    /* stack starts at 0x01ff */
	h6280.sp.d = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(H6280_RESET_VEC);
	PCH = RDMEM((H6280_RESET_VEC+1));
	CHANGE_PC;

	/* timer off by default */
	h6280.timer_status=0;

    /* clear pending interrupts */
	for (i = 0; i < 3; i++)
		h6280.irq_state[i] = CLEAR_LINE;
}

static void h6280_exit(void)
{
	/* nothing */
}

static int h6280_execute(int cycles)
{
	int in,lastcycle,deltacycle;
	h6280_ICount = cycles;

	/* Subtract cycles used for taking an interrupt */
	h6280_ICount -= h6280.extra_cycles;
	h6280.extra_cycles = 0;
	lastcycle = h6280_ICount;

	/* Execute instructions */
	do
    {
    	if ((h6280.ppc.w.l ^ h6280.pc.w.l) & 0xe000)
    		CHANGE_PC;
		h6280.ppc = h6280.pc;

#ifdef  MAME_DEBUG
	 	{
			if (Machine->debug_mode)
			{
				/* Copy the segmentation registers for debugger to use */
				int i;
				for (i=0; i<8; i++)
					H6280_debug_mmr[i]=h6280.mmr[i];

				mame_debug_hook();
			}
		}
#endif

		/* Execute 1 instruction */
		in=RDOP();
		PCW++;
		insnh6280[in]();

		/* Check internal timer */
		if(h6280.timer_status)
		{
			deltacycle = lastcycle - h6280_ICount;
			h6280.timer_value -= deltacycle;
			if(h6280.timer_value<=0)
			{
				h6280.timer_value=h6280.timer_load;
				set_irq_line(2,ASSERT_LINE);
			}
		}
		lastcycle = h6280_ICount;

		/* If PC has not changed we are stuck in a tight loop, may as well finish */
		if( h6280.pc.d == h6280.ppc.d )
		{
			if (h6280_ICount > 0) h6280_ICount=0;
			h6280.extra_cycles = 0;
			return cycles;
		}

	} while (h6280_ICount > 0);

	/* Subtract cycles used for taking an interrupt */
	h6280_ICount -= h6280.extra_cycles;
	h6280.extra_cycles = 0;

	return cycles - h6280_ICount;
}

static void h6280_get_context (void *dst)
{
	if( dst )
		*(h6280_Regs*)dst = h6280;
}

static void h6280_set_context (void *src)
{
	if( src )
		h6280 = *(h6280_Regs*)src;
	CHANGE_PC;
}


/*****************************************************************************/

static void set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (h6280.nmi_state == state) return;
		h6280.nmi_state = state;
		if (state != CLEAR_LINE)
	    {
			DO_INTERRUPT(H6280_NMI_VEC);
		}
	}
	else if (irqline < 3)
	{
	    h6280.irq_state[irqline] = state;

		/* If line is cleared, just exit */
		if (state == CLEAR_LINE) return;

		/* Check if interrupts are enabled and the IRQ mask is clear */
		CHECK_IRQ_LINES;
	}
}



/*****************************************************************************/

READ8_HANDLER( H6280_irq_status_r )
{
	int status;

	switch (offset&3)
	{
	default:return h6280.io_buffer;break;
	case 3:
		{
			status=0;
			if(h6280.irq_state[1]!=CLEAR_LINE) status|=1; /* IRQ 2 */
			if(h6280.irq_state[0]!=CLEAR_LINE) status|=2; /* IRQ 1 */
			if(h6280.irq_state[2]!=CLEAR_LINE) status|=4; /* TIMER */
			return status|(h6280.io_buffer&(~H6280_IRQ_MASK));break;
		}
	case 2: return h6280.irq_mask|(h6280.io_buffer&(~H6280_IRQ_MASK));break;
	}
}

WRITE8_HANDLER( H6280_irq_status_w )
{
	h6280.io_buffer=data;
	switch (offset&3)
	{
		default:h6280.io_buffer=data;break;
		case 2: /* Write irq mask */
			h6280.irq_mask=data&0x7;
			CHECK_IRQ_LINES;
			break;

		case 3: /* Timer irq ack */
			set_irq_line(2,CLEAR_LINE);
			break;
	}
}

READ8_HANDLER( H6280_timer_r )
{
	/* only returns countdown */
	return ((h6280.timer_value/1024)&0x7F)|(h6280.io_buffer&0x80);
}

WRITE8_HANDLER( H6280_timer_w )
{
	h6280.io_buffer=data;
	switch (offset) {
		case 0: /* Counter preload */
			h6280.timer_load=h6280.timer_value=((data&127)+1)*1024;
			return;

		case 1: /* Counter enable */
			if(data&1)
			{	/* stop -> start causes reload */
				if(h6280.timer_status==0) h6280.timer_value=h6280.timer_load;
			}
			h6280.timer_status=data&1;
			return;
	}
}

static int h6280_translate(int space, offs_t *addr)
{
	if (space == ADDRESS_SPACE_PROGRAM)
		*addr = TRANSLATED(*addr);
	return 1;
}

UINT8 h6280io_get_buffer()
{
	return h6280.io_buffer;
}
void h6280io_set_buffer(UINT8 data)
{
	h6280.io_buffer=data;
}


/*****************************************************************************/

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void h6280_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:			set_irq_line(0, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 1:			set_irq_line(1, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + 2:			set_irq_line(2, info->i);					break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + H6280_PC:		PCW = info->i;								break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + H6280_S:		S = info->i;								break;
		case CPUINFO_INT_REGISTER + H6280_P:		P = info->i;								break;
		case CPUINFO_INT_REGISTER + H6280_A:		A = info->i;								break;
		case CPUINFO_INT_REGISTER + H6280_X:		X = info->i;								break;
		case CPUINFO_INT_REGISTER + H6280_Y:		Y = info->i;								break;
		case CPUINFO_INT_REGISTER + H6280_IRQ_MASK: h6280.irq_mask = info->i; CHECK_IRQ_LINES;	break;
		case CPUINFO_INT_REGISTER + H6280_TIMER_STATE: h6280.timer_status = info->i; 			break;
		case CPUINFO_INT_REGISTER + H6280_NMI_STATE: set_irq_line( INPUT_LINE_NMI, info->i ); 	break;
		case CPUINFO_INT_REGISTER + H6280_IRQ1_STATE: set_irq_line( 0, info->i ); 				break;
		case CPUINFO_INT_REGISTER + H6280_IRQ2_STATE: set_irq_line( 1, info->i ); 				break;
		case CPUINFO_INT_REGISTER + H6280_IRQT_STATE: set_irq_line( 2, info->i ); 				break;
#ifdef MAME_DEBUG
		case CPUINFO_INT_REGISTER + H6280_M1:		h6280.mmr[0] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M2:		h6280.mmr[1] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M3:		h6280.mmr[2] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M4:		h6280.mmr[3] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M5:		h6280.mmr[4] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M6:		h6280.mmr[5] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M7:		h6280.mmr[6] = info->i;						break;
		case CPUINFO_INT_REGISTER + H6280_M8:		h6280.mmr[7] = info->i;						break;
#endif
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void h6280_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(h6280);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 3;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 7;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 2;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 17 + 6*65536;					break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 21;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 2;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = h6280.irq_state[0];			break;
		case CPUINFO_INT_INPUT_STATE + 1:				info->i = h6280.irq_state[1];			break;
		case CPUINFO_INT_INPUT_STATE + 2:				info->i = h6280.irq_state[2];			break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = h6280.nmi_state; 				break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = h6280.ppc.d;					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + H6280_PC:			info->i = PCD;							break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + H6280_S:			info->i = S;							break;
		case CPUINFO_INT_REGISTER + H6280_P:			info->i = P;							break;
		case CPUINFO_INT_REGISTER + H6280_A:			info->i = A;							break;
		case CPUINFO_INT_REGISTER + H6280_X:			info->i = X;							break;
		case CPUINFO_INT_REGISTER + H6280_Y:			info->i = Y;							break;
		case CPUINFO_INT_REGISTER + H6280_IRQ_MASK:		info->i = h6280.irq_mask;				break;
		case CPUINFO_INT_REGISTER + H6280_TIMER_STATE:	info->i = h6280.timer_status;			break;
		case CPUINFO_INT_REGISTER + H6280_NMI_STATE:	info->i = h6280.nmi_state;				break;
		case CPUINFO_INT_REGISTER + H6280_IRQ1_STATE:	info->i = h6280.irq_state[0];			break;
		case CPUINFO_INT_REGISTER + H6280_IRQ2_STATE:	info->i = h6280.irq_state[1];			break;
		case CPUINFO_INT_REGISTER + H6280_IRQT_STATE:	info->i = h6280.irq_state[2];			break;
#ifdef MAME_DEBUG
		case CPUINFO_INT_REGISTER + H6280_M1:			info->i = h6280.mmr[0];					break;
		case CPUINFO_INT_REGISTER + H6280_M2:			info->i = h6280.mmr[1];					break;
		case CPUINFO_INT_REGISTER + H6280_M3:			info->i = h6280.mmr[2];					break;
		case CPUINFO_INT_REGISTER + H6280_M4:			info->i = h6280.mmr[3];					break;
		case CPUINFO_INT_REGISTER + H6280_M5:			info->i = h6280.mmr[4];					break;
		case CPUINFO_INT_REGISTER + H6280_M6:			info->i = h6280.mmr[5];					break;
		case CPUINFO_INT_REGISTER + H6280_M7:			info->i = h6280.mmr[6];					break;
		case CPUINFO_INT_REGISTER + H6280_M8:			info->i = h6280.mmr[7];					break;
#endif

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = h6280_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = h6280_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = h6280_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = h6280_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = h6280_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = h6280_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = h6280_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = h6280_dasm;			break;
#endif /* MAME_DEBUG */
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &h6280_ICount;			break;
		case CPUINFO_PTR_TRANSLATE:						info->translate = h6280_translate;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "HuC6280");				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "Hudsonsoft 6280");		break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.07");				break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright (c) 1999, 2000 Bryan McPhail, mish@tendril.co.uk"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s, "%c%c%c%c%c%c%c%c",
				h6280.p & 0x80 ? 'N':'.',
				h6280.p & 0x40 ? 'V':'.',
				h6280.p & 0x20 ? 'R':'.',
				h6280.p & 0x10 ? 'B':'.',
				h6280.p & 0x08 ? 'D':'.',
				h6280.p & 0x04 ? 'I':'.',
				h6280.p & 0x02 ? 'Z':'.',
				h6280.p & 0x01 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + H6280_PC:			sprintf(info->s, "PC:%04X", h6280.pc.d); break;
        case CPUINFO_STR_REGISTER + H6280_S:			sprintf(info->s, "S:%02X", h6280.sp.b.l); break;
        case CPUINFO_STR_REGISTER + H6280_P:			sprintf(info->s, "P:%02X", h6280.p); break;
        case CPUINFO_STR_REGISTER + H6280_A:			sprintf(info->s, "A:%02X", h6280.a); break;
		case CPUINFO_STR_REGISTER + H6280_X:			sprintf(info->s, "X:%02X", h6280.x); break;
		case CPUINFO_STR_REGISTER + H6280_Y:			sprintf(info->s, "Y:%02X", h6280.y); break;
		case CPUINFO_STR_REGISTER + H6280_IRQ_MASK:		sprintf(info->s, "IM:%02X", h6280.irq_mask); break;
		case CPUINFO_STR_REGISTER + H6280_TIMER_STATE:	sprintf(info->s, "TMR:%02X", h6280.timer_status); break;
		case CPUINFO_STR_REGISTER + H6280_NMI_STATE:	sprintf(info->s, "NMI:%X", h6280.nmi_state); break;
		case CPUINFO_STR_REGISTER + H6280_IRQ1_STATE:	sprintf(info->s, "IRQ1:%X", h6280.irq_state[0]); break;
		case CPUINFO_STR_REGISTER + H6280_IRQ2_STATE:	sprintf(info->s, "IRQ2:%X", h6280.irq_state[1]); break;
		case CPUINFO_STR_REGISTER + H6280_IRQT_STATE:	sprintf(info->s, "IRQT:%X", h6280.irq_state[2]); break;
#ifdef MAME_DEBUG
		case CPUINFO_STR_REGISTER + H6280_M1:			sprintf(info->s, "M1:%02X", h6280.mmr[0]); break;
		case CPUINFO_STR_REGISTER + H6280_M2:			sprintf(info->s, "M2:%02X", h6280.mmr[1]); break;
		case CPUINFO_STR_REGISTER + H6280_M3:			sprintf(info->s, "M3:%02X", h6280.mmr[2]); break;
		case CPUINFO_STR_REGISTER + H6280_M4:			sprintf(info->s, "M4:%02X", h6280.mmr[3]); break;
		case CPUINFO_STR_REGISTER + H6280_M5:			sprintf(info->s, "M5:%02X", h6280.mmr[4]); break;
		case CPUINFO_STR_REGISTER + H6280_M6:			sprintf(info->s, "M6:%02X", h6280.mmr[5]); break;
		case CPUINFO_STR_REGISTER + H6280_M7:			sprintf(info->s, "M7:%02X", h6280.mmr[6]); break;
		case CPUINFO_STR_REGISTER + H6280_M8:			sprintf(info->s, "M8:%02X", h6280.mmr[7]); break;
#endif
	}
}
