/*
	APE(X)C CPU emulation

	By Raphael Nabet

	APE(X)C (All Purpose Electronic X-ray Computer) was a computer built by Andrew D. Booth
	and others for the Birkbeck College, in London, which was used to compute cristal
	structure using X-ray interferometry.  It was one of the APEC series of computer, which
	were simple electronic computers built in the early 1950s.

	Generals specs :
	* 32-bit data word size (10-bit addresses) : uses fixed-point, 2's complement arithmetic
	* one accumulator (A) and one register (R)
	* memory is composed of 256 circular magnetic tracks of 32 words : only 32-tracks can
	be accessed at a time, and the rotation rate is 3750rpm
	* two I/O units : tape reader and card puncher.  A teletyper was designed to read
	punched cards and print data.
	* machine code has 15 instructions (!), including add, substract, shift, multiply (!),
	test and branch, input and punch.  A so-called vector mode allow to repeat the same
	operation 32 times with 32 successive memory locations.  Note the lack of bitwise
	and/or/xor (!) .
	* .06 MIPS (I feel this is a lot for the time), although memory access time make this
	figure fairly theorical (Booth&Booth give 600us as a typical delay for an addition)
	* there is no indirect addressing whatever, although dynamic modification of opcodes (!)
	allow to simulate it...

	References :
	* Andrew D. Booth & Kathleen H. V. Booth : Automatic Digital Calculators, 2nd edition
	(Buttersworth Scientific Publications, 1956)
	* Kathleen H. V. Booth : Programming for an Automatic Digital Calculator
	(Buttersworth Scientific Publications, 1958)
*/

/*
	Conventions :
	Bits are numbered in big-endian endian, starting with 1 (!) : bit #1 is the MSB,
	and bit #32 is the LSB
*/

/*
	Format of a machine instruction :
bits:		1-5			6-10		11-15		16-20		21-25		26-31		32
field:		X address	X address	Y address	Y address	Function	C6			Vector
			(track)		(location)	(track)		(location)

	Meaning of fields :
	X : address of an operand, or immediate, or meaningless, depending on Function
		(When X is meaningless, it should be a duplicate of y - don't ask why)
	Y : address of the next instruction
	Function : code for the actual instruction executed
	C6 : immediate value used by shift operations
	Vector : repeat operation 32 times (with 32 consecutive locations on the X track given
		by the revelant field)

	Function code :
	#	Mnemonic	C6		Description

	0	Stop

	2	I(y)				Input

	4	P(y)				Punch

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
	>= is actually one single character.  Last, "1-n" and "n-32" are the actual sequences
	"1 *DASH* (number n)" and 	"(number n) *DASH* 32" (these are NOT formauls with
	substract signs).
*/

typedef struct
{
	UINT32 a;	/* accumulator */
	UINT32 r;	/* register */
	UINT32 cr;	/* control register (i.e. instruction register) */
} apexc_regs;

static apexc_regs apexc;

static UINT32 word_read(int address)
{
	
}

static void word_write(int address, UINT32 data)
{
}

static void execute(void)
{
	int x, y, function, c6, vector;

	x = (apexc.cr >> 22) & 0x3FF;
	y = (apexc.cr >> 12) & 0x3FF;
	function = (apexc.cr >> 7) & 0x1F;
	c6 = (apexc.cr >> 1) & 0x3F;
	vector = apexc.cr & 1;

	#define INSTRUCTION_END apexc.cr = word_read(y);
	#define DELAY(n)	;

	switch (function & 0x17)	/* the "& 0x17" is a mere guess */
	{
	case 0:
		/* stop */
		/* ... */
		break;

	case 2:
		break;

	case 4:
		break;

	case 6:
		break;

	case 8:
		break;

	case 10:
		break;

	case 12:
		/* unused function code.  We assume this results into a NOP, for lack of any
		specific info... */
		INSTRUCTION_END
		break;

	case 14:
		/* X_n(x) */

		/* Yes, this code is inefficient, but this is the way the APEXC does it ;-) */
		for (; c6 != 0; c6 = (c6+1) & 0x3f)
		{
			//if 
		}

		INSTRUCTION_END
		break;

	case 16:
		/* +c(x) */
		apexc.a = + word_read(x);
		INSTRUCTION_END
		break;

	case 18:
		/* -c(x) */
		apexc.a = - word_read(x);
		INSTRUCTION_END
		break;

	case 20:
		/* +(x) */
		apexc.a += word_read(x);
		INSTRUCTION_END
		break;

	case 22:
		/* -(x) */
		apexc.a -= word_read(x);
		INSTRUCTION_END
		break;

	case 24:
		/* T(x) */
		apexc.r = word_read(x);
		INSTRUCTION_END
		break;

	case 26:
		/* R_(1-n)(x) & R_(n-32)(x) */

		break;

	case 28:
		/* A_(1-n)(x) & A_(n-32)(x) */

		break;

	case 30:
		/* S(x) */

		break;
	}
}
