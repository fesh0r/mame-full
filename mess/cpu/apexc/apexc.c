/*
	APE(X)C CPU emulation

	By Raphael Nabet

	APE(X)C (All Purpose Electronic X-ray Computer) was a computer built by Andrew D. Booth
	and others for the Birkbeck College, in London, which was used to compute cristal
	structure using X-ray interferometry.  It was one of the APEC series of computer, which
	were simple electronic computers built in the early 1950s.

	References :
	* Andrew D. Booth & Kathleen H. V. Booth : Automatic Digital Calculators, 2nd edition
	(Buttersworth Scientific Publications, 1956)
	* Kathleen H. V. Booth : Programming for an Automatic Digital Calculator
	(Buttersworth Scientific Publications, 1958)
*/

/*
	Generals specs :
	* 32-bit data word size (10-bit addresses) : uses fixed-point, 2's complement arithmetic
	* CPU has one accumulator (A) and one register (R), plus one Control Register (this is
	  what we would call an "instruction register" nowadays).  No Program Counter, each
	  instruction contains the address of the next instruction (!).
	* memory is composed of 256 circular magnetic tracks of 32 words : only 32 tracks can
	  be accessed at a time (the 16 first ones, plus 16 others chosen by the programmer),
	  and the rotation rate is 3750rpm (62.5 rotations per second).
	* two I/O units : tape reader and tape puncher.  A teletyper was designed to read
	  specially-encoded punched tapes and print decoded text.
	* machine code has 15 instructions (!), including add, substract, shift, multiply (!),
	  test and branch, input and punch.  A so-called vector mode allow to repeat the same
	  operation 32 times with 32 successive memory locations.  Note the lack of bitwise
	  and/or/xor (!) .
	* 1 kIPS, although memory access times make this figure fairly theorical (drum rotation
	  time : 16ms, which would allow about 60IPS when no optimization is done)
	* there is no indirect addressing whatever, although dynamic modification of opcodes (!)
	  allow to simulate it...

	Conventions :
	Bits are numbered in big-endian order, starting with 1 (!) : bit #1 is the MSB,
	and bit #32 is the LSB

	References :
	* Andrew D. Booth & Kathleen H. V. Booth : Automatic Digital Calculators, 2nd edition
	(Buttersworth Scientific Publications, 1956)
	* Kathleen H. V. Booth : Programming for an Automatic Digital Calculator
	(Buttersworth Scientific Publications, 1958)
*/

/*
	Machine code:

	Format of a machine instruction :
bits:		1-5			6-10		11-15		16-20		21-25		26-31		32
field:		X address	X address	Y address	Y address	Function	C6			Vector
			(track)		(location)	(track)		(location)

	Meaning of fields :
	X : address of an operand, or immediate, or meaningless, depending on Function
		(When X is meaningless, it should be a duplicate of Y.  I guess this is because
		X is ALWAYS loaded into the memory address register, and if track # is different,
		we add unneeded track switch delays)
	Y : address of the next instruction
	Function : code for the actual instruction executed
	C6 : immediate value used by shift, multiply and store operations
	Vector : repeat operation 32 times (with 32 consecutive locations on the X track given
		by the revelant field)

	Function code :
	#	Mnemonic	C6		Description

	0	Stop

	2	I(y)				Input.  5 digits from tape are moved to the 5 MSBits of R (which
							must be cleared initially).

	4	P(y)				Punch.  5 MSBits of R are punched on tape.

	6	B<(x)>=(y)			Branch.  If A<0, next instruction is read from @x, whereas
							if A>=0, next instruction is read from @y

	8	l (y)		n		Shift left : the 64 bits of A and R are rotated left n times.
		 n

	10	r (y)		64-n	Shift right : the 64 bits of A and R are shifted right n times.
		 n					The sign bit of A is duplicated.

	14	X (x)(y)	33-n	Multiply the contents of *track* x by the last n digits of the
		 n					number in R, sending the 32 MSBs to A and 31 LSBs to R

	16	+c(x)(y)			A <- (x)

	18	-c(x)(y)			A <- -(x)

	20	+(x)(y)				A <- A+(x)

	22	-(x)(y)				A <- A-(x)

	24	T(x)(y)				R <- (x)

	26	R   (x)(y)	32+n	record first or last bits of R in (x).  The remaining bits of x
		 1-n				are unaffected, but the contents of R are filled with 0s or 1s
							according as the original contents were positive or negative.
		R    (x)(y)	n-1
		 n-32

	28	A   (x)(y)	32+n	Same as 26, except that source is A, and the contents of A are
		 1-n				not affected.

		A    (x)(y)	n-1
		 n-32

	30	S(x)(y)				Block Head switch.  This enables the block of heads specified
							in x to be loaded into the workign store.

	Note : Mnemonics use subscripts (!), which I tried to render the best I could.  Also,
	">=" is actually one single character.  Last, "1-n" and "n-32" in store mnemonics
	are the actual sequences "1 *DASH* <number n>" and "<number n> *DASH* 32"
	(these are NOT formulas with substract signs).
*/

/*
	memory interface :

	Data is exchanged on a 1-bit (!) data bus, 10-bit address bus.

	While the bus is 1-bit wide, read/write operation can only be take place on word
	(i.e. 32 bit) boundaries.  However, it is possible to store only the n first bits or
	n last bits of a word, leaving other bits in memory unaffected.

	The LSBits are transferred first, since it allows to perform bit-per-bit add and
	substract.  Otherwise, the CPU would need an additionnal register store the second
	operand, and the end result would be probably slower, since the operation could only
	take place after all the data has been transfered.

	Memory operations are synchronous with 2 clocks provided by the memory controller :
	* word clock : a pulse on each word boundary
	* bit clock : a pulse when a bit is present on the bus

	CPU operation is synchronous with these clocks, too.  For instance, AU does bit-per-bit
	addition and substraction with a memory operand, synchronously with bit clock,
	starting and stopping on word clock boundaries.  Same with a Fetch operation.

	There is a 10-bit memory location (i.e. address) register on the memory controller.
	It is loaded with the contents of X after when instruction fetch is complete, and
	with the contents of Y when instruction execution is complete, so that the next fetch
	can be executed correctly.
*/

#include "apexc.h"

typedef struct
{
	UINT32 a;	/* accumulator */
	UINT32 r;	/* register */
	UINT32 cr;	/* control register (i.e. instruction register) */
	int ml;		/* memory location (current track in working store, and requested
				word position within track) */
	int working_store;	/* current working store (group of 16 tracks) (1-15) */
	int current_word;	/* current word position within track (0-31) */

	int flags;	/* 2 flags : */
				/* running : flag implied by the existence of the stop instruction */
				/* tracing : I am not sure whether this flag existed, but it allows a cool
					feature for the control panel, and it kind of makes sense */
} apexc_regs;

/* masks for flags */
enum
{
	flag_running_bit = 0,
	flag_tracing_bit = 1,

	flag_running = 1<<flag_running_bit,
	flag_tracing = 1<<flag_tracing_bit
};

int apexc_ICount;

static apexc_regs apexc;

/* decrement ICount by n */
#define DELAY(n)	{apexc_ICount -= n; apexc.current_word = (apexc.current_word + n) & 0x1f;}

/* set/clear/get running and tracing flags flag */
#define set_running_flag apexc.flags |= flag_running
#define clear_running_flag apexc.flags &= ~flag_running
#define get_running_flag (apexc.flags & flag_running)

#define set_tracing_flag apexc.flags |= flag_tracing
#define clear_tracing_flag apexc.flags &= ~flag_tracing
#define get_tracing_flag (apexc.flags & flag_tracing)

/*
	word accessor functions

	take a 10-bit word address
		5 bits (MSBs) : track address within working store
		5 bits (LSBs) : word position within track

	take a mask : one bit is set for bit to read/write

	take a vector flag : if true, ignore word position

	memory latency delays should be taken into account
*/


/* memory access macros */

/* fortunately, there is no memory mapped I/O, so mask can be ignored for read */
#define cpu_readmem_masked(address, mask) cpu_readmem18bedw_dword(address << 2)

/* eewww ! */
#define cpu_writemem_masked(address, data, mask)										\
	cpu_writemem18bedw_dword(address << 2,												\
								(cpu_readmem18bedw_dword(address << 2) & ~mask) |		\
									(data & mask))

#define MASK_ALL 0xFFFFFFFFUL	/* mask which can be used for "normal" calls to word_read/word_write */


/* compute complete word address (i.e. translate a logical track address (i.e. within
working store) to an absolute track address) */
INLINE int effective_address(int address)
{
	if (address & 0x200)
	{
		address = (address & 0x1FF) | (apexc.working_store) << 9;
	}

	return address;
}

/* set memory location (i.e. address) register */
static void load_ml(int address)
{
	/* switch track if needed */
	if ((apexc.ml & 0x3E0) != (address & 0x3E0))	/* is old track different from new ? */
	{
		DELAY(6);	/* if yes, delay to allow for track switching */
	}

	apexc.ml = address;	/* save ml */
}

/* read word (or part of a word) */
static UINT32 word_read(UINT32 mask, int vector)
{
	int address;
	UINT32 result;

	/* compute absolute track address */
	address = effective_address(apexc.ml);

	if (vector)
	{
		/* ignore word position in x - use current position instead */
		address = (address & ~ 0x1f) | apexc.current_word;
	}
	else
	{
		/* wait for requested word to appear under the heads */
		DELAY(((address /*& 0x1f*/) - apexc.current_word) & 0x1f);
	}

	/* read 32 bits according to mask */
#if 0
	/* note that the APEXC reads LSBits first */
	result = 0;
	for (i=0; i<31; i++)
	{
		if (mask & (1 << i))
			result |= bit_read((address << 5) | i) << i;
	}
#else
	result = cpu_readmem_masked(address, mask);
#endif

	/* remember new head position */
#if 0	/* this code is actually equivalent to the code below */
	if (vector)
	{
		apexc.current_word = (apexc.current_word + 1) & 0x1f;
	}
	else
#endif
	{
		apexc.current_word = ((address /*& 0x1f*/) + 1) & 0x1f;
	}

	return result;
}

/* write word (or part of a word, according to mask) */
static void word_write(UINT32 data, UINT32 mask, int vector)
{
	int address;

	/* compute absolute track address */
	address = effective_address(apexc.ml);

	if (vector)
	{
		/* ignore word position in x - use current position instead */
		address = (address & ~ 0x1f) | apexc.current_word;
	}
	else
	{
		/* wait for requested word to appear under the heads */
		DELAY(((address /*& 0x1f*/) - apexc.current_word) & 0x1f);
	}

	/* write 32 bits according to mask */
#if 0
	/* note that the APEXC reads LSBits first */
	for (i=0; i<31; i++)
	{
		if (mask & (1 << i))
			bit_write((address << 5) | i, (data >> i) & 1);
	}
#else
	cpu_writemem_masked(address, data, mask);
#endif

	/* remember new head position */
#if 0	/* this code is actually equivalent to the code below */
	if (vector)
	{
		apexc.current_word = (apexc.current_word + 1) & 0x1f;
	}
	else
#endif
	{
		apexc.current_word = ((address /*& 0x1f*/) + 1) & 0x1f;
	}
}

/*
	I/O accessors

	no address is used, these functions just punch or read 5 bits
*/

static int papertape_read()
{
	return cpu_readport16bedw(0) & 0x1f;
}

static void papertape_punch(int data)
{
	cpu_writeport16bedw(0, data);
}



/*
	execute one instruction

	TODO :
	* implement stop
	* test

	NOTE :
	* I do not know whether we should fetch instructions at the beginning or the end of the
	instruction cycle.  Either solution is roughly equivalent to the other, but changes
	the control panel operation (and I know virtually nothing on the control panel).
	Currently, I fetch after the executing the instruction, so that the one may enter
	an instruction into the control register with the control panel, then execute it.
*/
static void execute(void)
{
	int x, y, function, c6, vector;
	int i = 0;

	x = (apexc.cr >> 22) & 0x3FF;
	y = (apexc.cr >> 12) & 0x3FF;
	function = (apexc.cr >> 7) & 0x1F;
	c6 = (apexc.cr >> 1) & 0x3F;
	vector = apexc.cr & 1;

	/* load ml with X */
	load_ml(x);

	do
	{
		switch (function & 0x1E)	/* the "& 0x1E" is a mere guess */
		{
		case 0:
			/* stop */

			clear_running_flag;

			/* BTW, I don't know whether stop loads y into ml or not (if it does not,
			we should do a "goto no_fetch") */
			break;

		case 2:
			/* I */
			/* I do not know whether the CPU does an OR or whatever, but since docs say that
			the 5 bits must be cleared initially, an OR kind of makes sense */
			apexc.r |= papertape_read() << 27;
			DELAY(32);
			break;

		case 4:
			/* P */
			papertape_punch((apexc.r >> 27) & 0x1f);
			DELAY(32);
			break;

		case 6:
			/* B<(x)>=(y) */
			/* I have no idea what we should do if the vector bit is set */
			if (apexc.a & 0x80000000UL)
			{
				apexc.cr = word_read(MASK_ALL, vector);	/* should it be 'vector' or '0' */
				goto no_fetch;
			}
			/* else, the instruction ends with a normal fetch */
			break;

		case 8:
			/* l_n */
			DELAY((c6 & 0x20) ? 1 : 2);	/* yes, if more than 32 shifts, it takes more time */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			while (c6 != 0)
			{
				int shifted_bit = 0;

				/* shift and increment c6 */
				shifted_bit = apexc.r & 1;
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a >>= 1;
				if (shifted_bit)
					apexc.a |= 0x80000000UL;

				c6 = (c6+1) & 0x3f;
			}

			break;

		case 10:
			/* r_n */
			DELAY((c6 & 0x20) ? 1 : 2);	/* yes, if more than 32 shifts, it takes more time */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			while (c6 != 0)
			{
				/* shift and increment c6 */
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a = ((INT32) apexc.a) >> 1;

				c6 = (c6+1) & 0x3f;
			}

			break;

		case 12:
			/* unused function code.  I assume this results into a NOP, for lack of any
			specific info... */

			break;

		case 14:
			/* X_n(x) */

			/* Yes, this code is inefficient, but this must be the way the APEXC does it ;-) */
			apexc.a = 0;
			while (1)
			{
				int shifted_bit = 0;

				/* note we read word at current word position */
				if (shifted_bit && ! (apexc.r & 1))
					apexc.a += word_read(MASK_ALL, 1);
				else if ((! shifted_bit) && (apexc.r & 1))
					apexc.a -= word_read(MASK_ALL, 1);
				else
					/* Even if we do not read anything, the loop still takes 1 cycle on
					the memory word clock. */
					/* Anyway, maybe we still read the data even if we do not use it. */
					DELAY(1);

				/* exit if c6 reached 32 ("c6 & 0x20 "is simpler to implement and
				essentially equivalent, so this is the most likely implementation) */
				if (c6 & 0x20)
					break;

				/* else increment c6 and  shift */
				c6 = (c6+1) & 0x3f;

				/* shift */
				shifted_bit = apexc.r & 1;
				apexc.r >>= 1;
				if (apexc.a & 1)
					apexc.r |= 0x80000000UL;
				apexc.a = ((INT32) apexc.a) >> 1;
			}

			//DELAY(32);	/* mmmh... we have already counted 32 wait states */
			/* actually, if (n < 32) (which is an untypical case), we do have 32 wait
			states.  Question is : do we really have 32 wait states if (n < 32), or is
			the timing table incomplete ? */
			break;

		case 16:
			/* +c(x) */
			apexc.a = + word_read(MASK_ALL, vector);
			break;

		case 18:
			/* -c(x) */
			apexc.a = - word_read(MASK_ALL, vector);
			break;

		case 20:
			/* +(x) */
			apexc.a += word_read(MASK_ALL, vector);
			break;

		case 22:
			/* -(x) */
			apexc.a -= word_read(MASK_ALL, vector);
			break;

		case 24:
			/* T(x) */
			apexc.r = word_read(MASK_ALL, vector);
			break;

		case 26:
			/* R_(1-n)(x) & R_(n-32)(x) */

			{
				UINT32 mask;

				if (c6 & 0x20)
					mask = 0xFFFFFFFFUL << (64 - c6);
				else
					mask = 0xFFFFFFFFUL >> c6;

				word_write(apexc.r, mask, vector);
			}

			apexc.r = (apexc.r & 0x80000000UL) ? 0xFFFFFFFFUL : 0;

			DELAY(1);	/* does this occur before or after store ? */
			break;

		case 28:
			/* A_(1-n)(x) & A_(n-32)(x) */

			{
				UINT32 mask;

				if (c6 & 0x20)
					mask = 0xFFFFFFFFUL << (64 - c6);
				else
					mask = 0xFFFFFFFFUL >> c6;

				word_write(apexc.a, mask, vector);
			}

			DELAY(1);	/* does this occur before or after store ? */
			break;

		case 30:
			/* S(x) */
			apexc.working_store = (x >> 5) & 0xf;	/* or is it (x >> 6) ? */
			DELAY(32);	/* no idea what the value is...  All I know is that it is much more
						than track switch (which is 6) */
			break;
		}
	} while (vector && (++i < 32));	/* iterate 32 times if vector bit is set */

	if (vector)
		DELAY(12);	/* does this occur before or after instruction execution ? */

	/* load ml with Y */
	load_ml(y);

	/* fetch current instruction into control register */
	apexc.cr = word_read(MASK_ALL, 0);

	/* entry point after a successful Branch (which alters the normal instruction sequence,
	in order not to load ml with Y) */
no_fetch:
	;
}




void apexc_reset(void *param)
{
	/* mmmh... */
	apexc.cr = 0;	/* first instruction executed is stop (is this right ?) */
	apexc.ml = 0;	/* no idea whether this is true or not */
	apexc.working_store = 1;	/* mere guess */
	apexc.current_word = 0;		/* well, we do have to start somewhere */
}

void apexc_exit(void)
{
}

unsigned apexc_get_context(void *dst)
{
	if (dst)
		* ((apexc_regs*) dst) = apexc;
	return sizeof(apexc_regs);
}

void apexc_set_context(void *src)
{
	if (src)
	{
		apexc = * ((apexc_regs*)src);
	}
}

/* no PC - return memory location register instead, this should be equivalent unless not
executed on instruction boundary */
unsigned apexc_get_pc(void)
{
	return apexc.ml;
}

void apexc_set_pc(unsigned val)
{
	apexc.ml = val;
}

/* no SP */
unsigned apexc_get_sp(void)
{
	return 0U;
}

void apexc_set_sp(unsigned val)
{
	(void) val;
}

/* no NMI line */
void apexc_set_nmi_line(int state)
{
	(void) state;
}

/* no IRQ line */
void apexc_set_irq_line(int irqline, int state)
{
	(void) irqline;
	(void) state;
}

void apexc_set_irq_callback(int (*callback)(int irqline))
{
	(void) callback;
}

unsigned apexc_get_reg(int regnum)
{
	switch (regnum)
	{
		case APEXC_CR:
			return apexc.cr;
		case APEXC_A:
			return apexc.a;
		case APEXC_R:
			return apexc.r;
		case APEXC_ML:
			return apexc.ml;
		case APEXC_WS:
			return apexc.working_store;
		case APEXC_FLAGS:
			return apexc.flags;
	}
	return 0;
}

void apexc_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case APEXC_CR:
			apexc.cr = val;
			break;
		case APEXC_A:
			apexc.a = val;
			break;
		case APEXC_R:
			apexc.r = val;
			break;
		case APEXC_ML:
			apexc.ml = val;
			break;
		case APEXC_WS:
			apexc.working_store = val;
			break;
		case APEXC_FLAGS:
			apexc.flags = val;
	}
}

const char *apexc_info(void *context, int regnum)
{
	static UINT8 apexc_reg_layout[] =
	{
		APEXC_CR, -1,
		APEXC_A, APEXC_R, -1,
		APEXC_ML, APEXC_WS, -1,
		APEXC_FLAGS, 0
	};

	/* OK, I have no idea what would be the best layout */
	static UINT8 apexc_win_layout[] =
	{
		48, 0,32,13,	/* register window (top right) */
		 0, 0,47,13,	/* disassembler window (top left) */
		 0,14,47, 8,	/* memory #1 window (left, middle) */
		48,14,32, 8,	/* memory #2 window (right, middle) */
		 0,23,80, 1 	/* command line window (bottom rows) */
	};

	static char buffer[16][47 + 1];
	static int which = 0;
	apexc_regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if (! context)
		r = &apexc;

	switch (regnum)
	{
	case CPU_INFO_REG + APEXC_CR:
		sprintf(buffer[which], "CR:%08X", r->cr);
		break;
	case CPU_INFO_REG + APEXC_A:
		sprintf(buffer[which], "A :%08X", r->a);
		break;
	case CPU_INFO_REG + APEXC_R: 
		sprintf(buffer[which], "R :%08X", r->r);
		break;
	case CPU_INFO_REG + APEXC_ML:
		sprintf(buffer[which], "ML:%03X", r->ml);
		break;
	case CPU_INFO_REG + APEXC_WS:
		sprintf(buffer[which], "WS:%01X", r->working_store);
		break;
	case CPU_INFO_REG + APEXC_FLAGS:
		sprintf(buffer[which], "flags:%01X", r->flags);
		break;
    case CPU_INFO_FLAGS:
    	/* no flags right now */
		sprintf(buffer[which], "%c%c", (r->flags & flag_tracing) ? 'T' : '.',
				(r->flags & flag_running) ? 'R' : 'S');
		break;
	case CPU_INFO_NAME:
		return "APEXC";
	case CPU_INFO_FAMILY:
		return "APEC";
	case CPU_INFO_VERSION:
		return "1.0";
	case CPU_INFO_FILE:
		return __FILE__;
	case CPU_INFO_CREDITS:
		return "Raphael Nabet";
	case CPU_INFO_REG_LAYOUT:
		return (const char *) apexc_reg_layout;
	case CPU_INFO_WIN_LAYOUT:
		return (const char *) apexc_win_layout;
	}
	return buffer[which];
}

unsigned apexc_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmAPEXC(buffer,pc);
#else
	sprintf(buffer, "$%08X", cpu_readop(pc));
	return 1;
#endif
}

int apexc_execute(int cycles)
{
	apexc_ICount = cycles;

	do
	{
		CALL_MAME_DEBUG;

		if (get_running_flag)
			execute();
		else
		{
			DELAY(apexc_ICount);	/* burn cycles once for all */
		}

		if (get_tracing_flag)
			clear_running_flag;
	} while (apexc_ICount > 0);

	return cycles - apexc_ICount;
}

