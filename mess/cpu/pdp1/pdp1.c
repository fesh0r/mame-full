/*
 * Note: Original Java source written by:
 *
 * Barry Silverman mailto:barry@disus.com or mailto:bss@media.mit.edu
 * Vadim Gerasimov mailto:vadim@media.mit.edu
 * (not much was changed, only the IOT stuff and the 64 bit integer shifts)
 *
 * Note: I removed the IOT function to be external, it is
 * set at emulation initiation, the IOT function is part of
 * the machine emulation...
 *
 *
 * for the runnable java applet go to:
 * http://lcs.www.media.mit.edu/groups/el/projects/spacewar/
 *
 * for a complete html version of the pdp1 handbook go to:
 * http://www.dbit.com/~greeng3/pdp1/index.html
 *
 * there is another java simulator (by the same people) which runs the
 * original pdp1 LISP interpreter, go to:
 * http://lcs.www.media.mit.edu/groups/el/projects/pdp1
 *
 * and finally, there is a nice article about SPACEWAR!, go to:
 * http://ars-www.uchicago.edu/~eric/lore/spacewar/spacewar.html
 *
 * some extra documentation is available on spies:
 * http://www.spies.com/~aek/pdf/dec/pdp1/
 * The file "F17_PDP1Maint.pdf" explains operation procedures and much of the internals of pdp-1.
 * The file "F25_PDP1_IO.pdf" has interesting information on the I/O system, too.
 *
 * following is an extract from the handbook:
 *
 * INTRODUCTION
 *
 * The Programmed Data Processor (PDP-1) is a high speed, solid state digital computer designed to
 * operate with many types of input-output devices with no internal machine changes. It is a single
 * address, single instruction, stored program computer with powerful program features. Five-megacycle
 * circuits, a magnetic core memory and fully parallel processing make possible a computation rate of
 * 100,000 additions per second. The PDP-1 is unusually versatile. It is easy to install, operate and
 * maintain. Conventional 110-volt power is used, neither air conditioning nor floor reinforcement is
 * necessary, and preventive maintenance is provided for by built-in marginal checking circuits.
 *
 * PDP-1 circuits are based on the designs of DEC's highly successful and reliable System Modules.
 * Flip-flops and most switches use saturating transistors. Primary active elements are
 * Micro-Alloy-Diffused transistors.
 *
 * The entire computer occupies only 17 square feet of floor space. It consists of four equipment frames,
 * one of which is used as the operating station.
 *
 * CENTRAL PROCESSOR
 *
 * The Central Processor contains the control, arithmetic and memory addressing elements, and the memory
 * buffer register. The word length is 18 binary digits. Instructions are performed in multiples of the
 * memory cycle time of five microseconds. Add, subtract, deposit, and load, for example, are two-cycle
 * instructions requiring 10 microseconds. Multiplication requires and average of 20 microseconds.
 * Program features include: single address instructions, multiple step indirect addressing and logical
 * arithmetic commands. Console features include: flip-flop indicators grouped for convenient octal
 * reading, six program flags for automatic setting and computer sensing, and six sense switches for
 * manual setting and computer sensing.
 *
 * MEMORY SYSTEM
 *
 * The coincident-current, magnetic core memory of a standard PDP-1 holds 4096 words of 18 bits each.
 * Memory capacity may be readily expanded, in increments of 4096 words, to a maximum of 65,536 words.
 * The read-rewrite time of the memory is five microseconds, the basic computer rate. Driving currents
 * are automatically adjusted to compensate for temperature variations between 50 and 110 degrees
 * Fahrenheit. The core memory storage may be supplemented by up to 24 magnetic tape transports.
 *
 * INPUT-OUTPUT
 *
 * PDP-1 is designed to operate a variety of buffered input-output devices. Standard equipment consistes
 * of a perforated tape reader with a read speed of 400 lines per second, and alphanuermic typewriter for
 * on-line operation in both input and output, and a perforated tape punch (alphanumeric or binary) with
 * a speed of 63 lines per second. A variety of optional equipment is available, including the following:
 *
 *     Precision CRT Display Type 30
 *     Ultra-Precision CRT Display Type 31
 *     Symbol Generator Type 33
 *     Light Pen Type 32
 *     Oscilloscope Display Type 34
 *     Card Punch Control Type 40-1
 *     Card Reader and Control Type 421
 *     Magnetic Tape Transport Type 50
 *     Programmed Magnetic Tape Control Type 51
 *     Automatic Magnetic Tape Control Type 52
 *     Automatic Magnetic Tape Control Type 510
 *     Parallel Drum Type 23
 *     Automatic Line Printer and Control Type 64
 *     18-bit Real Time Clock
 *     18-bit Output Relay Buffer Type 140
 *     Multiplexed A-D Converter Type 138/139
 *
 * All in-out operations are performed through the In-Out Register or through the high speed input-output
 * channels.
 *
 * The PDP-1 is also available with the optional Sequence Break System. This is a multi-channel priority
 * interrupt feature which permits concurrent operation of several in-out devices. A one-channel Sequence
 * Break System is included in the standard PDP-1. Optional Sequence Break Systems consist of 16, 32, 64,
 * 128, and 256 channels.
 *
 * ...
 *
 * BASIC INSTRUCTIONS
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #  EXPLANATION                                     (usec)
 * ------------------------------------------------------------------------------
 * add Y        40      Add C(Y) to C(AC)                                 10
 * and Y        02      Logical AND C(Y) with C(AC)                       10
 * cal Y        16      Equals jda 100                                    10
 * dac Y        24      Deposit C(AC) in Y                                10
 * dap Y        26      Deposit contents of address part of AC in Y       10
 * dio Y        32      Deposit C(IO) in Y                                10
 * dip Y        30      Deposit contents of instruction part of AC in Y   10
 * div Y        56      Divide                                          40 max
 * dzm Y        34      Deposit zero in Y                                 10
 * idx Y        44      Index (add one) C(Y), leave in Y & AC             10
 * ior Y        04      Inclusive OR C(Y) with C(AC)                      10
 * iot Y        72      In-out transfer, see below
 * isp Y        46      Index and skip if result is positive              10
 * jda Y        17      Equals dac Y and jsp Y+1                          10
 * jmp Y        60      Take next instruction from Y                      5
 * jsp Y        62      Jump to Y and save program counter in AC          5
 * lac Y        20      Load the AC with C(Y)                             10
 * law N        70      Load the AC with the number N                     5
 * law-N        71      Load the AC with the number -N                    5
 * lio Y        22      Load IO with C(Y)                                 10
 * mul Y        54      Multiply                                        25 max
 * opr          76      Operate, see below                                5
 * sad Y        50      Skip next instruction if C(AC) <> C(Y)            10
 * sas Y        52      Skip next instruction if C(AC) = C(Y)             10
 * sft          66      Shift, see below                                  5
 * skp          64      Skip, see below                                   5
 * sub Y        42      Subtract C(Y) from C(AC)                          10
 * xct Y        10      Execute instruction in Y                          5+
 * xor Y        06      Exclusive OR C(Y) with C(AC)                      10
 *
 * OPERATE GROUP
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                    (usec)
 * ------------------------------------------------------------------------------
 * cla        760200     Clear AC                                         5
 * clf        76000f     Clear selected Program Flag (f = flag #)         5
 * cli        764000     Clear IO                                         5
 * cma        761000     Complement AC                                    5
 * hlt        760400     Halt                                             5
 * lap        760100     Load AC with Program Counter                     5
 * lat        762200     Load AC from Test Word switches                  5
 * nop        760000     No operation                                     5
 * stf        76001f     Set selected Program Flag                        5
 *
 * IN-OUT TRANSFER GROUP
 *
 * PERFORATED TAPE READER
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rpa        720001     Read Perforated Tape Alphanumeric
 * rpb        720002     Read Perforated Tape Binary
 * rrb        720030     Read Reader Buffer
 *
 * PERFORATED TAPE PUNCH
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * ppa        720005     Punch Perforated Tape Alphanumeric
 * ppb        720006     Punch Perforated Tape Binary
 *
 * ALPHANUMERIC ON-LINE TYPEWRITER
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * tyo        720003     Type Out
 * tyi        720004     Type In
 *
 * SEQUENCE BREAK SYSTEM TYPE 120
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * esm        720055     Enter Sequence Break Mode
 * lsm        720054     Leave Sequence Break Mode
 * cbs        720056     Clear Sequence Break System
 * dsc        72kn50     Deactivate Sequence Break Channel
 * asc        72kn51     Activate Sequence Break Channel
 * isb        72kn52     Initiate Sequence Break
 * cac        720053     Clear All Channels
 *
 * HIGH SPEED DATA CONTROL TYPE 131
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * swc        72x046     Set Word Counter
 * sia        720346     Set Location Counter
 * sdf        720146     Stop Data Flow
 * rlc        720366     Read Location Counter
 * shr        720446     Set High Speed Channel Request
 *
 * PRECISION CRT DISPLAY TYPE 30
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * dpy        720007     Display One Point
 *
 * SYMBOL GENERATOR TYPE 33
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * gpl        722027     Generator Plot Left
 * gpr        720027     Generator Plot Right
 * glf        722026     Load Format
 * gsp        720026     Space
 * sdb        722007     Load Buffer, No Intensity
 *
 * ULTRA-PRECISION CRT DISPLAY TYPE 31
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * dpp        720407     Display One Point on Ultra Precision CRT
 *
 * CARD PUNCH CONTROL TYPE 40-1
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * lag        720044     Load a Group
 * pac        720043     Punch a Card
 *
 * CARD READER TYPE 421
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rac        720041     Read Card Alpha
 * rbc        720042     Read Card Binary
 * rcc        720032     Read Card Column
 *
 * PROGRAMMED MAGNETIC TAPE CONTROL TYPE 51
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * msm        720073     Select Mode
 * mcs        720034     Check Status
 * mcb        720070     Clear Buffer
 * mwc        720071     Write a Character
 * mrc        720072     Read Character
 *
 * AUTOMATIC MAGNETIC TAPE CONTROL TYPE 52
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * muf        72ue76     Tape Unit and FinalT
 * mic        72ue75     Initial and Command
 * mrf        72u067     Reset Final
 * mri        72ug66     Reset Initial
 * mes        72u035     Examine States
 * mel        72u036     Examine Location
 * inr        72ur67     Initiate a High Speed Channel Request
 * ccr        72s067     Clear Command Register
 *
 * AUTOMATIC MAGNETIC TAPE CONTROL TYPE 510
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * sfc        720072     Skip if Tape Control Free
 * rsr        720172     Read State Register
 * crf        720272     Clear End-of-Record Flip-Flop
 * cpm        720472     Clear Proceed Mode
 * dur        72xx70     Load Density, Unit, Rewind
 * mtf        73xx71     Load Tape Function Register
 * cgo        720073     Clear Go
 *
 * MULTIPLEXED A-D CONVERTER TYPE 138/139
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rcb        720031     Read Converter Buffer
 * cad        720040     Convert a Voltage
 * scv        72mm47     Select Multiplexer (1 of 64 Channels)
 * icv        720060     Index Multiplexer
 *
 * AUTOMATIC LINE PRINTER TYPE 64
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * clrbuf     722045     Clear Buffer
 * lpb        720045     Load Printer Buffer
 * pas        721x45     Print and Space
 *
 * SKIP GROUP
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                    (usec)
 * ------------------------------------------------------------------------------
 * sma        640400     Dkip on minus AC                                 5
 * spa        640200     Skip on plus AC                                  5
 * spi        642000     Skip on plus IO                                  5
 * sza        640100     Skip on ZERO (+0) AC                             5
 * szf        6400f      Skip on ZERO flag                                5
 * szo        641000     Skip on ZERO overflow (and clear overflow)       5
 * szs        6400s0     Skip on ZERO sense switch                        5
 *
 * SHIFT/ROTATE GROUP
 *
 *                                                                      OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                      (usec)
 * ------------------------------------------------------------------------------
 *   ral        661      Rotate AC left                                     5
 *   rar        671      Rotate AC right                                    5
 *   rcl        663      Rotate Combined AC & IO left                       5
 *   rcr        673      Rotate Combined AC & IO right                      5
 *   ril        662      Rotate IO left                                     5
 *   rir        672      Rotate IO right                                    5
 *   sal        665      Shift AC left                                      5
 *   sar        675      Shift AC right                                     5
 *   scl        667      Shift Combined AC & IO left                        5
 *   scr        677      Shift Combined AC & IO right                       5
 *   sil        666      Shift IO left                                      5
 *   sir        676      Shift IO right                                     5
 */


/*
	TODO:
	* suspend processor when I/O device not ready during iot instructions and read-in mode
	* support sequence break
	* support memory extension and other extensions as time permits
*/


#include <stdio.h>
#include <stdlib.h>
#include "driver.h"
#include "mamedbg.h"
#include "pdp1.h"


/* Layout of the registers in the debugger */
static UINT8 pdp1_reg_layout[] =
{
	PDP1_PC, PDP1_IR, PDP1_MB, PDP1_MA, PDP1_AC, PDP1_IO, PDP1_PF, -1,
	PDP1_TW, PDP1_TA, PDP1_SS, PDP1_SNGL_STEP, PDP1_SNGL_INST, -1,
	PDP1_RUN, PDP1_CYC, PDP1_DEFER, PDP1_OV, PDP1_RIM, PDP1_EXD, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 pdp1_win_layout[] =
{
	 0,  0, 80,  4, /* register window (top rows) */
	 0,  5, 24, 17, /* disassembler window (left colums) */
	25,  5, 55,  8, /* memory #1 window (right, upper middle) */
	25, 14, 55,  8, /* memory #2 window (right, lower middle) */
	0,	23, 80,  1, /* command line window (bottom rows) */
};

static void execute_instruction(void);
static int intern_iot (int *io, int md);
static void pulse_start_clear(void);


/* PDP1 Registers */
typedef struct
{
	/* processor registers */
	UINT32 pc;		/* program counter (12, 15 or 16 bits) */
	int ir;			/* basic operation code of current instruction (5 bits) */
	int mb;			/* memory buffer (used for holding the current instruction only) (18 bits) */
	int ma;			/* memory address (12, 15 or 16 bits) */
	int ac;			/* accumulator (18 bits) */
	int io;			/* i/o register (18 bits) */
	int pf;			/* program flag register (6 bits) */

	/* operator panel switches */
	int ta;			/* current state of the 12 or 16 address switches */
	int tw;			/* current state of the 18 test word switches */
	int ss;			/* current state of the 6 sense switches on the operator panel (6 bits) */
	unsigned int sngl_step : 1;	/* stop every memory cycle */
	unsigned int sngl_inst : 1;	/* stop every instruction */
	unsigned int extend_sw : 1;	/* extend switch (loaded into the extend flip-flop on start/read-in) */

	/* processor state flip-flops */
	unsigned int run : 1;		/* processor is running */
	unsigned int cycle : 1;		/* processor is in the midst of an instruction */
	unsigned int defer : 1;		/* processor is handling deferred (i.e. indirect) addressing */
	unsigned int ov;			/* overflow flip-flop */
	unsigned int rim : 1;		/* processor is in read-in mode */
#if 0
	unsigned int sbm : 1;		/* processor is in sequence break mode (i.e. interrupts are enabled) */
	unsigned int irq_state : 1;	/* mirrors the state of the interrupt pin */
	unsigned int b2: 1;			/* interrupt pending */
	unsigned int b4: 1;			/* interrupt in progress */
#endif
	unsigned int exd : 1;		/* extend mode: processor is in extend mode */
	unsigned int exc : 1;		/* extend-mode cycle: current instruction cycle is done in extend mode */
#if 0
	unsigned int i_o_halt : 1;	/* processor is executing an Input-Output Transfer wait */
#endif

	/* callback for the iot instruction */
	int (*extern_iot)(int *, int);
	/* read a byte from the perforated tape reader */
	int (*read_binary_word)(UINT32 *reply);

	/* 0: no extend support, 1: extend with 15-bit address, 2: extend with 16-bit address */
	int extend_support;

	int extended_address_mask;	/* 07777 with no extend support, 077777 or 0177777 with extend support */
	int address_extension_mask;	/* 00000 with no extend support, 070000 or 0170000 with extend support */
}
pdp1_Regs;


static pdp1_Regs pdp1;

#define PC		pdp1.pc
#define IR		pdp1.ir
#define MB		pdp1.mb
#define MA		pdp1.ma
#define AC		pdp1.ac
#define IO		pdp1.io
#define OV		pdp1.ov
#define EXD		pdp1.exd
/* note that we start counting flags/sense switches at 1, therefore n is in [1,6] */
#define FLAGS	pdp1.pf
#define READFLAG(n)	((pdp1.pf >> (6-(n))) & 1)
#define WRITEFLAG(n, data)	(pdp1.pf = (pdp1.pf & ~(1 << (6-(n)))) | (((data) & 1) << (6-(n))))
#define SENSE_SW	pdp1.ss
#define READSENSE(n)	((pdp1.ss >> (6-(n))) & 1)
#define WRITESENSE(n, data)	(pdp1.ss = (pdp1.ss & ~(1 << (6-(n)))) | (((data) & 1) << (6-(n))))

#define EXTENDED_ADDRESS_MASK	pdp1.extended_address_mask
#define ADDRESS_EXTENSION_MASK	pdp1.address_extension_mask
#define BASE_ADDRESS_MASK		0007777

#define INCREMENT_PC	(PC = (PC & ADDRESS_EXTENSION_MASK) | ((PC+1) & BASE_ADDRESS_MASK))
#define INCREMENT_MA	(MA = (MA & ADDRESS_EXTENSION_MASK) | ((MA+1) & BASE_ADDRESS_MASK))

/* public globals */
signed int pdp1_ICount;



/*
	interrupts are called "sequence break", and they do exist.

	we do not emulate them, which is sad
*/
void pdp1_set_irq_line (int irqline, int state)
{
}

void pdp1_set_irq_callback (int (*callback) (int irqline))
{
}


void pdp1_init(void)
{
	/* nothing to do */
}

void pdp1_reset (void *untyped_param)
{
	pdp1_reset_param_t *param = untyped_param;

	memset (&pdp1, 0, sizeof (pdp1));

	/* set up params and callbacks */
	pdp1.read_binary_word = (param) ? param->read_binary_word : NULL;
	pdp1.extern_iot = (param && param->extern_iot)
									? param->extern_iot
									: intern_iot;
	pdp1.extend_support = (param) ? param->extend_support : 0;

	switch (pdp1.extend_support)
	{
	default:
		pdp1.extend_support = 0;
	case 0:		/* no extension */
		pdp1.extended_address_mask = 07777;
		pdp1.address_extension_mask = 00000;
		break;
	case 1:		/* 15-bit extension */
		pdp1.extended_address_mask = 077777;
		pdp1.address_extension_mask = 070000;
		break;
	case 2:		/* 16-bit extension */
		pdp1.extended_address_mask = 0177777;
		pdp1.address_extension_mask = 0170000;
		break;
	}
}

void pdp1_exit (void)
{
	/* nothing to do */
}


unsigned pdp1_get_context (void *dst)
{
	if (dst)
		*(pdp1_Regs *) dst = pdp1;
	return sizeof (pdp1_Regs);
}

void pdp1_set_context (void *src)
{
	if (src)
		pdp1 = *(pdp1_Regs *) src;
}


unsigned pdp1_get_reg (int regnum)
{
	switch (regnum)
	{
	case REG_PC:
	case PDP1_PC: return PC;
	case PDP1_IR: return IR;
	case PDP1_MB: return MB;
	case PDP1_MA: return MA;
	case PDP1_AC: return AC;
	case PDP1_IO: return IO;
	case PDP1_OV: return OV;
	case PDP1_PF:  return FLAGS;
	case PDP1_PF1: return READFLAG(1);
	case PDP1_PF2: return READFLAG(2);
	case PDP1_PF3: return READFLAG(3);
	case PDP1_PF4: return READFLAG(4);
	case PDP1_PF5: return READFLAG(5);
	case PDP1_PF6: return READFLAG(6);
	case PDP1_TA: return pdp1.ta;
	case PDP1_TW: return pdp1.tw;
	case PDP1_SS:  return SENSE_SW;
	case PDP1_SS1: return READSENSE(1);
	case PDP1_SS2: return READSENSE(2);
	case PDP1_SS3: return READSENSE(3);
	case PDP1_SS4: return READSENSE(4);
	case PDP1_SS5: return READSENSE(5);
	case PDP1_SS6: return READSENSE(6);
	case PDP1_SNGL_STEP: return pdp1.sngl_step;
	case PDP1_SNGL_INST: return pdp1.sngl_inst;
	case PDP1_EXTEND_SW: return pdp1.extend_sw;
	case PDP1_RUN: return pdp1.run;
	case PDP1_CYC: return pdp1.cycle;
	case PDP1_DEFER: return pdp1.defer;
	case PDP1_RIM: return pdp1.rim;
	case PDP1_EXD: return EXD;
	case REG_SP:  return 0;
	}
	return 0;
}

void pdp1_set_reg (int regnum, unsigned val)
{
	switch (regnum)
	{
	case REG_PC:
	case PDP1_PC: PC = val & EXTENDED_ADDRESS_MASK; break;
	case PDP1_IR: IR = val & 037; /* weird idea */ break;
	case PDP1_MB: MB = val & 0777777; break;
	case PDP1_MA: MA = val & EXTENDED_ADDRESS_MASK; break;
	case PDP1_AC: AC = val & 0777777; break;
	case PDP1_IO: IO = val & 0777777; break;
	case PDP1_OV: OV = val ? 1 : 0; break;
	case PDP1_PF:  FLAGS = val & 077; break;
	case PDP1_PF1: WRITEFLAG(1, val ? 1 : 0); break;
	case PDP1_PF2: WRITEFLAG(2, val ? 1 : 0); break;
	case PDP1_PF3: WRITEFLAG(3, val ? 1 : 0); break;
	case PDP1_PF4: WRITEFLAG(4, val ? 1 : 0); break;
	case PDP1_PF5: WRITEFLAG(5, val ? 1 : 0); break;
	case PDP1_PF6: WRITEFLAG(6, val ? 1 : 0); break;
	case PDP1_TA: pdp1.ta = val & 0177777 /*07777 with simpler control panel*/; break;
	case PDP1_TW: pdp1.tw = val & 0777777; break;
	case PDP1_SS:  SENSE_SW = val & 077; break;
	case PDP1_SS1: WRITESENSE(1, val ? 1 : 0); break;
	case PDP1_SS2: WRITESENSE(2, val ? 1 : 0); break;
	case PDP1_SS3: WRITESENSE(3, val ? 1 : 0); break;
	case PDP1_SS4: WRITESENSE(4, val ? 1 : 0); break;
	case PDP1_SS5: WRITESENSE(5, val ? 1 : 0); break;
	case PDP1_SS6: WRITESENSE(6, val ? 1 : 0); break;
	case PDP1_SNGL_STEP: pdp1.sngl_step = val ? 1 : 0; break;
	case PDP1_SNGL_INST: pdp1.sngl_inst = val ? 1 : 0; break;
	case PDP1_EXTEND_SW: pdp1.extend_sw = val ? 1 : 0; break;
	case PDP1_RUN: pdp1.run = val ? 1 : 0; break;
	case PDP1_CYC: logerror("pdp1_set_reg to cycle flip-flop ignored\n");/* no way!*/ break;
	case PDP1_DEFER: logerror("pdp1_set_reg to defer flip-flop ignored\n");/* no way!*/ break;
	case PDP1_RIM: pdp1.rim = val ? 1 : 0; break;
	case PDP1_EXD: EXD = (pdp1.extend_support && val) ? 1 : 0; break;
	case REG_SP:  break;
	case PDP1_START_CLEAR: pulse_start_clear(); break;
	}
}


/*
	flags:
	* 1 for each instruction which supports indirect addressing (memory reference instructions,
	  except cal and jda, and with the addition of jmp and jsp)
	* 2 for memory reference instructions
*/
static const char instruction_kind[32] =
{
/*		and	ior	xor	xct			cal/jda */
	0,	3,	3,	3,	3,	0,	0,	2,
/*	lac	lio	dac	dap	dip	dio	dzm		*/
	3,	3,	3,	3,	3,	3,	3,	0,
/*	add	sub	idx	isp	sad	sas	mus	dis	*/
	3,	3,	3,	3,	3,	3,	3,	3,
/*	jmp	jsp	skp	sft	law	iot		opr	*/
	1,	1,	0,	0,	0,	0,	0,	0
};


/* execute instructions on this CPU until icount expires */
int pdp1_execute (int cycles)
{
	pdp1_ICount = cycles;

	do
	{
		CALL_MAME_DEBUG;

		if ((! pdp1.run) && (! pdp1.rim))
			pdp1_ICount = 0;	/* if processor is stopped, just burn cycles */
		else if (pdp1.rim)
		{
			if (! pdp1.read_binary_word)
				pdp1.rim = 0;	/* what else can we do ??? */
			else
			{
				UINT32 data18;

				/* read first word as instruction */
				(void)(*pdp1.read_binary_word)(&data18);
				IO = data18;						/* data is transferred to IO register */
				MB = IO;
				IR = MB >> 13;		/* basic opcode */
				if (IR == JMP)		/* jmp instruction ? */
				{
					PC = (MA & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);
					pdp1.rim = 0;	/* exit read-in mode */
					pdp1.run = 1;
				}
				else if ((IR == DIO) || (IR == DAC))	/* dio or dac instruction ? */
				{	/* there is a discrepancy: the pdp1 handbook tells that only dio should be used,
					but the lisp tape uses the dac instruction instead */
					/* Yet maintainance manual p. 6-25 states clearly that the data is located
					in IO and transfered to MB, so DAC is likely to be a mistake. */
					MA = (PC & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);

					/* read second word as data */
					(void)(*pdp1.read_binary_word)(&data18);
					IO = data18;						/* data is transferred to IO register */
					MB = IO;
					WRITE_PDP_18BIT(MA, MB);
				}
				else
				{
					/* what the heck? */
					logerror("It seems this tape should not be operated in read-in mode\n");
					pdp1.rim = 0;		/* exit read-in mode (right???) */
				}

				pdp1_ICount -= 1000;	/* ***HACK*** */
			}
		}
		else
		{
			/* no instruction in progress: time to fetch a new instruction, I guess */
			if (! pdp1.cycle)
			{
				MB = READ_PDP_18BIT(MA = PC);
				INCREMENT_PC;
				IR = MB >> 13;		/* basic opcode */

				if ((instruction_kind[IR] & 1) && (MB & 010000))
				{
					pdp1.defer = 1;
					pdp1.cycle = 1;			/* instruction shall be executed later */
				}
				else if (instruction_kind[IR] & 2)
					pdp1.cycle = 1;			/* instruction shall be executed later */
				else
					execute_instruction();	/* execute instruction at once */

				pdp1_ICount -= 5;
			}
			else if (pdp1.defer)
			{	/* defer cycle : handle indirect addressing */
				MA = (PC & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);

				MB = READ_PDP_18BIT(MA);

				/* determinate new value of pdp1.defer */
				if (EXD)
				{
					pdp1.defer = 0;
					pdp1.exc = 1;
				}
				else
					pdp1.defer = (MB & 010000) ? 1 : 0;

				/* execute JMP and JSP immediately if applicable */
				if ((! pdp1.defer) && (! instruction_kind[IR] & 2))
				{
					execute_instruction();	/* execute instruction at once */
					/*pdp1.cycle = 0;*/
					pdp1.exc = 0;
				}

				pdp1_ICount -= 5;
			}
			else
			{	/* memory reference instruction in cycle 1 */
				if (pdp1.exc)
				{
					MA = MB & EXTENDED_ADDRESS_MASK;
					pdp1.exc = 0;
				}
				else
					MA = (PC & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);

				execute_instruction();	/* execute instruction */

				pdp1_ICount -= 5;
			}

			if ((pdp1.sngl_step) || ((pdp1.sngl_inst) && (! pdp1.cycle)))
				pdp1.run = 0;
		}
	}
	while (pdp1_ICount > 0);

	return cycles - pdp1_ICount;
}

unsigned pdp1_dasm (char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return dasmpdp1 (buffer, pc);
#else
	sprintf (buffer, "0%06o", READ_PDP_18BIT (pc));
	return 1;
#endif
}


/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *pdp1_info (void *context, int regnum)
{
	static char buffer[16][47 + 1];
	static int which = 0;
	pdp1_Regs *r = context;

	which = (which + 1) % 16;
	buffer[which][0] = '\0';
	if (!context)
		r = &pdp1;

	switch (regnum)
	{
	case CPU_INFO_REG + PDP1_PC: sprintf (buffer[which], "PC:0%06o", r->pc); break;
	case CPU_INFO_REG + PDP1_IR: sprintf (buffer[which], "IR:0%02o", r->ir); break;
	case CPU_INFO_REG + PDP1_MB: sprintf (buffer[which], "MB:0%06o", r->mb);  break;
	case CPU_INFO_REG + PDP1_MA: sprintf (buffer[which], "MA:0%06o", r->ma);  break;
	case CPU_INFO_REG + PDP1_AC: sprintf (buffer[which], "AC:0%06o", r->ac); break;
	case CPU_INFO_REG + PDP1_IO: sprintf (buffer[which], "IO:0%06o", r->io); break;
	case CPU_INFO_REG + PDP1_OV: sprintf (buffer[which], "OV:%X", r->ov); break;
	case CPU_INFO_REG + PDP1_PF:  sprintf (buffer[which], "FLAGS:0%02o", r->pf);  break;
	case CPU_INFO_REG + PDP1_PF1: sprintf (buffer[which], "FLAG1:%X", (r->pf >> 5) & 1); break;
	case CPU_INFO_REG + PDP1_PF2: sprintf (buffer[which], "FLAG2:%X", (r->pf >> 4) & 1); break;
	case CPU_INFO_REG + PDP1_PF3: sprintf (buffer[which], "FLAG3:%X", (r->pf >> 3) & 1); break;
	case CPU_INFO_REG + PDP1_PF4: sprintf (buffer[which], "FLAG4:%X", (r->pf >> 2) & 1); break;
	case CPU_INFO_REG + PDP1_PF5: sprintf (buffer[which], "FLAG5:%X", (r->pf >> 1) & 1); break;
	case CPU_INFO_REG + PDP1_TA: sprintf (buffer[which], "TA:0%06o", r->ta);  break;
	case CPU_INFO_REG + PDP1_TW: sprintf (buffer[which], "TW:0%06o", r->tw); break;
	case CPU_INFO_REG + PDP1_PF6: sprintf (buffer[which], "FLAG6:%X", r->pf & 1); break;
	case CPU_INFO_REG + PDP1_SS:  sprintf (buffer[which], "SS:0%02o", r->ss);  break;
	case CPU_INFO_REG + PDP1_SS1: sprintf (buffer[which], "SENSE1:%X", (r->ss >> 5) & 1); break;
	case CPU_INFO_REG + PDP1_SS2: sprintf (buffer[which], "SENSE2:%X", (r->ss >> 4) & 1); break;
	case CPU_INFO_REG + PDP1_SS3: sprintf (buffer[which], "SENSE3:%X", (r->ss >> 3) & 1); break;
	case CPU_INFO_REG + PDP1_SS4: sprintf (buffer[which], "SENSE4:%X", (r->ss >> 2) & 1); break;
	case CPU_INFO_REG + PDP1_SS5: sprintf (buffer[which], "SENSE5:%X", (r->ss >> 1) & 1); break;
	case CPU_INFO_REG + PDP1_SS6: sprintf (buffer[which], "SENSE6:%X", r->ss & 1); break;
	case CPU_INFO_REG + PDP1_SNGL_STEP: sprintf (buffer[which], "SNGLSTEP:%X", r->sngl_step); break;
	case CPU_INFO_REG + PDP1_SNGL_INST: sprintf (buffer[which], "SNGLINST:%X", r->sngl_inst); break;
	case CPU_INFO_REG + PDP1_EXTEND_SW: sprintf (buffer[which], "EXS:%X", r->extend_sw); break;
	case CPU_INFO_REG + PDP1_RUN: sprintf (buffer[which], "RUN:%X", r->run); break;
	case CPU_INFO_REG + PDP1_CYC: sprintf (buffer[which], "CYC:%X", r->cycle); break;
	case CPU_INFO_REG + PDP1_DEFER: sprintf (buffer[which], "DF:%X", r->defer); break;
	case CPU_INFO_REG + PDP1_RIM: sprintf (buffer[which], "RIM:%X", r->rim); break;
	case CPU_INFO_REG + PDP1_EXD: sprintf (buffer[which], "EXD:%X", r->exd); break;
    case CPU_INFO_FLAGS:
		sprintf (buffer[which], "%c%c%c%c%c%c-%c%c%c%c%c%c",
				 (r->pf & 040) ? '1' : '.',
				 (r->pf & 020) ? '2' : '.',
				 (r->pf & 010) ? '3' : '.',
				 (r->pf & 004) ? '4' : '.',
				 (r->pf & 002) ? '5' : '.',
				 (r->pf & 001) ? '6' : '.',
				 (r->ss & 040) ? '1' : '.',
				 (r->ss & 020) ? '2' : '.',
				 (r->ss & 010) ? '3' : '.',
				 (r->ss & 004) ? '4' : '.',
				 (r->ss & 002) ? '5' : '.',
				 (r->ss & 001) ? '6' : '.');
		break;
	case CPU_INFO_NAME: return "PDP1";
	case CPU_INFO_FAMILY: return "DEC PDP-1";
	case CPU_INFO_VERSION: return "2.0";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return
			"Brian Silverman (original Java Source)\n"
			"Vadim Gerasimov (original Java Source)\n"
			"Chris Salomon (MESS driver)\n"
			"Raphael Nabet (MESS driver)\n";
	case CPU_INFO_REG_LAYOUT: return (const char *) pdp1_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char *) pdp1_win_layout;
	}
	return buffer[which];
}


/* execute one instruction */
static void execute_instruction(void)
{
	switch (IR)
	{
	case AND:		/* Logical And */
		AC &= (MB = READ_PDP_18BIT(MA));
		break;
	case IOR:		/* Inclusive Or */
		AC |= (MB = READ_PDP_18BIT(MA));
		break;
	case XOR:		/* Exclusive Or */
		AC ^= (MB = READ_PDP_18BIT(MA));
		break;
	case XCT:		/* Execute */
		MB = READ_PDP_18BIT(MA);
		IR = MB >> 13;		/* basic opcode */
		if ((instruction_kind[IR] & 1) && (MB & 010000))
		{
			pdp1.defer = 1;
			/*pdp1.cycle = 1;*/			/* instruction shall be executed later */
			goto no_fetch;			/* fall through to next instruction */
		}
		else if (instruction_kind[IR] & 2)
		{
			/*pdp1.cycle = 1;*/			/* instruction shall be executed later */
			goto no_fetch;			/* fall through to next instruction */
		}
		else
			execute_instruction();	/* execute instruction at once */
		break;
	case CALJDA:	/* Call subroutine and Jump and Deposit Accumulator instructions */
		if (MB & 010000)
			/* JDA */
			MA = (PC & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);
		else
			/* CAL: equivalent to JDA 100 */
			MA = 0100;	/* or is it "(PC & ADDRESS_EXTENSION_MASK) | 0100" ??? */

		WRITE_PDP_18BIT(MA, (MB = AC));
		INCREMENT_MA;
		AC = (OV << 17) | (EXD << 16) | PC;
		PC = MA;
		break;
	case LAC:		/* Load Accumulator */
		AC = (MB = READ_PDP_18BIT(MA));
		break;
	case LIO:		/* Load i/o register */
		IO = (MB = READ_PDP_18BIT(MA));
		break;
	case DAC:		/* Deposit Accumulator */
		WRITE_PDP_18BIT(MA, (MB = AC));
		break;
	case DAP:		/* Deposit Address Part */
		WRITE_PDP_18BIT(MA, (MB = ((READ_PDP_18BIT(MA) & 0770000) | (AC & 07777))));
		break;
	case DIP:		/* Deposit Instruction Part */
		WRITE_PDP_18BIT(MA, (MB = ((READ_PDP_18BIT(MA) & 07777) | (AC & 0770000))));
		break;
	case DIO:		/* Deposit I/O Register */
		WRITE_PDP_18BIT(MA, (MB = IO));
		break;
	case DZM:		/* Deposit Zero in Memory */
		WRITE_PDP_18BIT(MA, (MB = 0));
		break;
	case ADD:		/* Add */
	{
		int new_ov;
		AC = AC + (MB = READ_PDP_18BIT(MA));
		OV |= new_ov = AC >> 18;
		AC = (AC + new_ov) & 0777777;
		if (AC == 0777777)
			AC = 0;
		break;
	}
	case SUB:		/* Subtract */
		{
			int diffsigns;

			MB = READ_PDP_18BIT(MA);
			diffsigns = ((AC >> 17) ^ (MB >> 17)) == 1;
			AC = AC + (MB ^ 0777777);
			AC = (AC + (AC >> 18)) & 0777777;
			if (AC == 0777777)
				AC = 0;
			if (diffsigns && ((MB >> 17) == (AC >> 17)))
				OV = 1;
			break;
		}
	case IDX:		/* Index */
		AC = READ_PDP_18BIT(MA) + 1;
		if (AC == 0777777)
			AC = 0;
		WRITE_PDP_18BIT(MA, (MB = AC));
		break;
	case ISP:		/* Index and Skip if Positive */
		AC = READ_PDP_18BIT(MA) + 1;
		if (AC == 0777777)
			AC = 0;
		WRITE_PDP_18BIT(MA, (MB = AC));
		if ((AC & 0400000) == 0)
			INCREMENT_PC;
		break;
	case SAD:		/* Skip if Accumulator and Y differ */
		if (AC != (MB = READ_PDP_18BIT(MA)))
			INCREMENT_PC;
		break;
	case SAS:		/* Skip if Accumulator and Y are the same */
		if (AC == (MB = READ_PDP_18BIT(MA)))
			INCREMENT_PC;
		break;
	case MUS:		/* Multiply Step */
		if ((IO & 1) == 1)
		{
			AC = AC + (MB = READ_PDP_18BIT(MA));
			AC = (AC + (AC >> 18)) & 0777777;
			if (AC == 0777777)
				AC = 0;
		}
		IO = (IO >> 1 | AC << 17) & 0777777;
		AC >>= 1;
		break;
	case DIS:		/* Divide Step */
		{
			int acl;

			acl = AC >> 17;
			AC = (AC << 1 | IO >> 17) & 0777777;
			IO = ((IO << 1 | acl) & 0777777) ^ 1;
			MB = READ_PDP_18BIT(MA);
			if (IO & 1)
			{
				AC += (MB ^ 0777777);
			}
			else
			{
				AC += MB + 1;
			}
			AC = (AC + (AC >> 18)) & 0777777;
			if (AC == 0777777)
				AC = 0;
			break;
		}
	case JMP:		/* Jump */
		if (pdp1.exc)
			PC = MB & EXTENDED_ADDRESS_MASK;
		else
			PC = (MA & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);
		break;
	case JSP:		/* Jump and Save Program Counter */
		AC = (OV << 17) | (EXD << 16) | PC;
		if (pdp1.exc)
			PC = MB & EXTENDED_ADDRESS_MASK;
		else
			PC = (MA & ADDRESS_EXTENSION_MASK) | (MB & BASE_ADDRESS_MASK);
		break;
	case SKP:		/* Skip Instruction Group */
		{
			int mb = MB;
			int cond = ((mb & 0100) && (AC == 0))	/* ZERO Accumulator */
				|| ((mb & 0200) && (AC >> 17 == 0))	/* Plus Accumulator */
				|| ((mb & 0400) && (AC >> 17 == 1))	/* Minus Accumulator */
				|| ((mb & 01000) && (OV == 0))		/* ZERO Overflow */
				|| ((mb & 02000) && (IO >> 17 == 0))	/* Plus I/O Register */
				|| (((mb & 7) != 0) && (((mb & 7) == 7) ? ! FLAGS : ! READFLAG(mb & 7)))	/* ZERO Flag (deleted by mistake in PDP-1 handbook) */
				|| (((mb & 070) != 0) && (((mb & 070) == 070) ? ! SENSE_SW : ! READSENSE((mb & 070) >> 3)));	/* ZERO Switch */

			if (! (mb & 010000))
			{
				if (cond)
					INCREMENT_PC;
			}
			else
			{
				if (!cond)
					INCREMENT_PC;
			}
			if (mb & 01000)
				OV = 0;
			break;
		}
	case SFT:		/* Shift Instruction Group */
		{
			/* Bit 5 specifies direction of shift, Bit 6 specifies the character of the shift
			(arithmetic or logical), Bits 7 and 8 enable the registers (01 = AC, 10 = IO,
			and 11 = both) and Bits 9 through 17 specify the number of steps. */
			int mb = MB;
			int nshift = 0;
			int mask = mb & 0777;

			while (mask != 0)
			{
				nshift += mask & 1;
				mask >>= 1;
			}
			switch ((mb >> 9) & 017)
			{
				int i;

			case 1:		/* ral rotate accumulator left */
				for (i = 0; i < nshift; i++)
					AC = (AC << 1 | AC >> 17) & 0777777;
				break;
			case 2:		/* ril rotate i/o register left */
				for (i = 0; i < nshift; i++)
					IO = (IO << 1 | IO >> 17) & 0777777;
				break;
			case 3:		/* rcl rotate AC and IO left */
				for (i = 0; i < nshift; i++)
				{
					int tmp = AC;

					AC = (AC << 1 | IO >> 17) & 0777777;
					IO = (IO << 1 | tmp >> 17) & 0777777;
				}
				break;
			case 5:		/* sal shift accumulator left */
				for (i = 0; i < nshift; i++)
					AC = ((AC << 1 | AC >> 17) & 0377777) + (AC & 0400000);
				break;
			case 6:		/* sil shift i/o register left */
				for (i = 0; i < nshift; i++)
					IO = ((IO << 1 | IO >> 17) & 0377777) + (IO & 0400000);
				break;
			case 7:		/* scl shift AC and IO left */
				for (i = 0; i < nshift; i++)
				{
					int tmp = AC;

					AC = ((AC << 1 | IO >> 17) & 0377777) + (AC & 0400000);		/* shouldn't that be IO?, no it is the sign! */
					IO = (IO << 1 | tmp >> 17) & 0777777;
				}
				break;
			case 9:		/* rar rotate accumulator right */
				for (i = 0; i < nshift; i++)
					AC = (AC >> 1 | AC << 17) & 0777777;
				break;
			case 10:	/* rir rotate i/o register right */
				for (i = 0; i < nshift; i++)
					IO = (IO >> 1 | IO << 17) & 0777777;
				break;
			case 11:	/* rcr rotate AC and IO right */
				for (i = 0; i < nshift; i++)
				{
					int tmp = AC;

					AC = (AC >> 1 | IO << 17) & 0777777;
					IO = (IO >> 1 | tmp << 17) & 0777777;
				}
				break;
			case 13:	/* sar shift accumulator right */
				for (i = 0; i < nshift; i++)
					AC = (AC >> 1) + (AC & 0400000);
				break;
			case 14:	/* sir shift i/o register right */
				for (i = 0; i < nshift; i++)
					IO = (IO >> 1) + (IO & 0400000);
				break;
			case 15:	/* scr shift AC and IO right */
				for (i = 0; i < nshift; i++)
				{
					int tmp = AC;

					AC = (AC >> 1) + (AC & 0400000);	/* shouldn't that be IO, no it is the sign */
					IO = (IO >> 1 | tmp << 17) & 0777777;
				}
				break;
			default:
				logerror("Undefined shift: 0%06o at 0%06o\n", mb, PC - 1);
				break;
			}
			break;
		}
	case LAW:		/* Load Accumulator with N */
		AC = MB & 07777;
		if (MB & 010000)
			AC ^= 0777777;
		break;
	case IOT:		/* In-Out Transfer Instruction Group */
		/*
			The variations within this group of instructions perform all the in-out control
			and information transfer functions.  If Bit 5 (normally the Indirect Address bit)
			is a ONE, the computer will enter a special waiting state until the completion pulse
			from the activated device has returned.  When this device delivers its completion,
			the computer will resume operation of the instruction sequence. 

			The computer may be interrupted from the special waiting state to serve a sequence
			break request or a high speed channel request. 

			Most in-out operations require a known minimum time before completion.  This time
			may be utilized for programming.  The appropriate In-Out Transfer can be given with
			no in-out wait (Bit 5 a ZERO and Bit 6 a ONE).  The instruction sequence then
			continues.  This sequence must include an iot instruction 730000 which performs
			nothing but the in-out wait. The computer will then enter the special waiting state
			until the device returns the in-out restart pulse.  If the device has already
			returned the completion pulse before the instruction 730000, the computer will
			proceed immediately. 

			Bit 6 determines whether a completion pulse will or will not be received from
			the in-out device.  When it is different than Bit 5, a completion pulse will be
			received.  When it is the same as Bit 5, a completion pulse will not be received. 

			In addition to the control function of Bits 5 and 6, Bits 7 through 11 are also
			used as control bits serving to extend greatly the power of the iot instructions.
			For example, Bits 12 through 17, which are used to designate a class of input or
			output devices such as typewriters, may be further defined by Bits 7 through 11
			as referring to Typewriter 1, 2, 3, etc.  In several of the optional in-out devices,
			in particular the magnetic tape, Bits 7 through 11 specify particular functions
			such as forward, backward etc.  If a large number of specialized devices are to
			be attached, these bits may be used to further the in-out transfer instruction
			to perform totally distinct functions. 
		*/
		if (MB == 10000)
			pdp1_ICount -= pdp1.extern_iot (&IO, MB);
		else
			(void) pdp1.extern_iot (&IO, MB);
		break;
	case OPR:		/* Operate Instruction Group */
		{
			int mb = MB;
			int nflag;

			if (mb & 00200)		/* clear AC */
				AC = 0;
			if (mb & 04000)		/* clear I/O register */
				IO = 0;
			if (mb & 02000)		/* load Accumulator from Test Word */
				AC |= pdp1.tw;
			if (mb & 00100)		/* load Accumulator with Program Counter */
				AC |= (OV << 17) | (EXD << 16) | PC;
			nflag = mb & 7;
			if (nflag)
			{
				if (nflag == 7)
					FLAGS = (mb & 010) ? 077 : 000;
				else
					WRITEFLAG(nflag, (mb & 010) ? 1 : 0);
			}
			if (mb & 01000)		/* Complement AC */
				AC ^= 0777777;
			if (mb & 00400)		/* Halt */
			{
				logerror("PDP1 Program executed HALT: at 0%06o\n", PC - 1);
				pdp1.run = 0;
			}
			break;
		}
	default:
		logerror("Illegal instruction: 0%06o at 0%06o\n", MB, PC - 1);
		/* let us stop the CPU, like a real pdp-1 */
		pdp1.run = 0;

		break;
	}
	pdp1.cycle = 0;
no_fetch:
	;
}

static int intern_iot (int *io, int mb)
{
	logerror("No external IOT function given (IO=0%06o) -> EXIT(1) invoked in PDP1\\PDP1.C\n", *io);
	exit (1);
	return 1;
}


static void pulse_start_clear(void)
{
	/* processor registers */
	PC = 0;			/* according to maintainance manual p. 6-17 */
	IR = 0;			/* according to maintainance manual p. 6-13 */
	/*MB = 0;*/		/* ??? */
	/*MA = 0;*/		/* ??? */
	/*AC = 0;*/		/* ??? */
	/*IO = 0;*/		/* ??? */
	/*PF = 0;*/		/* ??? */

	/* processor state flip-flops */
	pdp1.run = 0;		/* ??? */
	pdp1.cycle = 0;		/* mere guess */
	pdp1.defer = 0;		/* mere guess */
	/*pdp1.ov = 0;*/	/* ??? */
	pdp1.rim = 0;		/* ??? */
#if 0
	pdp1.sbm = 0;		/* ??? */
	pdp1.b2 = 0;		/* ??? */
	pdp1.b4 = 0;		/* ??? */
#endif
	EXD = 0;			/* according to maintainance manual p. 8-16 */
	pdp1.exc = 0;		/* according to maintainance manual p. 8-16 */
#if 0
	pdp1.i_o_halt = 0;	/* mere guess */
#endif
}
