#include <stdio.h>

#include "driver.h"

#include "includes/pdp1.h"
#include "cpu/pdp1/pdp1.h"


int *pdp1_memory;

static int reader_buffer = 0;
static int typewriter_buffer = 0;
static int typewriter_buffer_status = 0;



/*
	driver init function

	Set up the pdp1_memory pointer, and generate font data.
*/
void init_pdp1(void)
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[pdp1_fontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	};

	/* set up memory regions */
	pdp1_memory = (int *) memory_region(REGION_CPU1); /* really bad! */

	/* set up our font */
	dst = memory_region(REGION_GFX1);

	memcpy(dst, fontdata6x8, pdp1_fontdata_size);

}


static OPBASE_HANDLER(setOPbasefunc)
{
	/* just to get rid of the warnings */
	return -1;
}

void pdp1_init_machine(void)
{
	memory_set_opbase_handler(0,setOPbasefunc);
}

READ18_HANDLER(pdp1_read_mem)
{
	return pdp1_memory ? pdp1_memory[offset] : 0;
}

WRITE18_HANDLER(pdp1_write_mem)
{
	if (pdp1_memory)
		pdp1_memory[offset]=data;
}

/*
	perforated tape handling
*/
typedef struct tape
{
	void *fd;
} tape;

tape pdp1_tapes[2];

/*
	Open a perforated tape image
*/
int pdp1_tape_init(int id)
{
	tape *t = &pdp1_tapes[id];

	/* open file */
	/* unit 0 is read-only, unit 1 is write-only */
	t->fd = image_fopen(IO_PUNCHTAPE, id, OSD_FILETYPE_IMAGE,
							(id==0) ? OSD_FOPEN_READ : OSD_FOPEN_WRITE);

	return INIT_PASS;
}

/*
	Close a perforated tape image
*/
void pdp1_tape_exit(int id)
{
	tape *t = &pdp1_tapes[id];

	if (t->fd)
		osd_fclose(t->fd);
}

/*
	Read a byte from perforated tape
*/
static int tape_read(UINT8 *reply)
{
	if (pdp1_tapes[0].fd && (osd_fread(pdp1_tapes[0].fd, reply, 1) == 1))
		return 0;	/* unit OK */
	else
	{
		*reply = 0;	/* might be the actual result */
		return 1;	/* unit not ready */
	}
}

/*
	Write a byte to perforated tape
*/
static void tape_write(UINT8 data)
{
	if (pdp1_tapes[1].fd)
		osd_fwrite(pdp1_tapes[1].fd, & data, 1);
}

/*
	Read a 18-bit word in binary format from tape
*/
int pdp1_tape_read_binary(UINT32 *reply)
{
	int not_ready;
	int i;
	UINT32 data18;
	UINT8 data;


	data18 = 0;
	for (i=0; i<3; i++)
	{
		while ((! (not_ready = tape_read(& data))) && (! (data & 0x80)))
			;

		if (not_ready)
			return 1;			/* aargh ! */

		data18 = (data18 << 6) | (data & 077);
	}

	* reply = data18;

	return 0;
}


/*
	Teletyper handling
*/

typedef struct teletyper
{
	void *fd;
} teletyper;

teletyper pdp1_teletyper;

/*
	Open a file for teletyper output
*/
int pdp1_teletyper_init(int id)
{
	/* open file */
	pdp1_teletyper.fd = image_fopen(IO_PRINTER, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_WRITE);

	return INIT_PASS;
}

/*
	Close teletyper output file
*/
void pdp1_teletyper_exit(int id)
{
	if (pdp1_teletyper.fd)
		osd_fclose(pdp1_teletyper.fd);
}

/*
	Write a character to teletyper
*/
static void teletyper_write(UINT8 data)
{
	logerror("typewriter output %o\n", data);
	if (pdp1_teletyper.fd)
		osd_fwrite(pdp1_teletyper.fd, & data, 1);
}

/* these are the key-bits specified in driver\pdp1.c */
#define FIRE_PLAYER2              128
#define THRUST_PLAYER2            64
#define ROTATE_RIGHT_PLAYER2      32
#define ROTATE_LEFT_PLAYER2       16
#define FIRE_PLAYER1              8
#define THRUST_PLAYER1            4
#define ROTATE_RIGHT_PLAYER1      2
#define ROTATE_LEFT_PLAYER1       1

/* time in micro seconds, (*io_register) */
int pdp1_iot(int *io, int md)
{
	int etime /*=0*/;

	switch (md & 077)
	{
	case 00:
	{
		/* Waiting for output to finnish */
		logerror("Warning, IOT instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));
		etime = 0;
		break;
	}

	/* perforated tape reader */
	case 01: /* RPA */
	{
		/*
		 * Read Perforated Tape, Alphanumeric
		 * rpa Address 0001
		 *
		 * This instruction reads one line of tape (all eight Channels) and transfers the resulting 8-bit code to
		 * the Reader Buffer. If bits 5 and 6 of the rpa function are both zero (720001), the contents of the
		 * Reader Buffer must be transferred to the IO Register by executing the rrb instruction. When the Reader
		 * Buffer has information ready to be transferred to the IO Register, Status Register Bit 1 is set to
		 * one. If bits 5 and 6 are different (730001 or 724001) the 8-bit code read from tape is automatically
		 * transferred to the IO Register via the Reader Buffer and appears as follows:
		 *
		 * IO Bits        10 11 12 13 14 15 16 17
		 * TAPE CHANNELS  8  7  6  5  4  3  2  1
		 */
		UINT8 read_byte;
		logerror("Warning, RPA instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));

		etime = 10;	/* approximately 1/400s (actually, it is 1/400s after last character) */

		(void)tape_read(& read_byte);

		reader_buffer = read_byte;
		if (read_byte == 0)
		{
			cpu_set_reg(PDP1_F1,0);
			break;
		}
		else
			cpu_set_reg(PDP1_F1,1);

		if (!((md>>11)&3))
		{
			;/* work thru rrb */
		}

		if (((md>>11)&1) != ((md>>12)&1))
		{
			/* code to IO */
			*io = reader_buffer;
		}
		break;
	}
	case 02: /* RPB */
	{
		/*
		 * Read Perforated Tape, Binary rpb
		 * Address 0002
		 *
		 * The instruction reads three lines of tape (six Channels per line) and assembles
		 * the resulting 18-bit word in the Reader Buffer.  For a line to be recognized by
		 * this instruction Channel 8 must be punched (lines with Channel 8 not punched will
		 * be skipped over).  Channel 7 is ignored.  The instruction sub 5137, for example,
		 * appears on tape and is assembled by rpb as follows:
		 *
		 * Channel 8 7 6 5 4 | 3 2 1
		 * Line 1  X   X     |   X
		 * Line 2  X   X   X |     X
		 * Line 3  X     X X | X X X
		 * Reader Buffer 100 010 101 001 011 111
		 *
		 * (Vertical dashed line indicates sprocket holes and the symbols "X" indicate holes
		 * punched in tape). 
		 *
		 * If bits 5 and 6 of the rpb instruction are both zero (720002), the contents of
		 * the Reader Buffer must be transferred to the IO Register by executing
		 * a rrb instruction.  When the Reader Buffer has information ready to be transferred
		 * to the IO Register, Status Register Bit 1 is set to one.  If bits 5 and 6 are
		 * different (730002 or 724002) the 18-bit word read from tape is automatically
		 * transferred to the IO Register via the Reader Buffer. 
		 */
		UINT32 read_word;
		int not_ready;
		logerror("Warning, RPB instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));

		etime = 10;	/* approximately 1/400s*3 (actually, it is 1/400s after last character) */

		not_ready = pdp1_tape_read_binary(& read_word);

		reader_buffer = read_word;
		if (not_ready)
		{
			cpu_set_reg(PDP1_F1,0);
			break;
		}
		else
			cpu_set_reg(PDP1_F1,1);

		if (!((md>>11)&3))
		{
			;/* work thru rrb */
		}

		if (((md>>11)&1) != ((md>>12)&1))
		{
			/* code to IO */
			*io = reader_buffer;
		}
		break;
	}
	case 030: /* RRB */
	{
		logerror("Warning, RRB instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n",*io, md, cpu_get_reg(PDP1_PC));
		etime = 5;
		cpu_set_reg(PDP1_F1,0);
		*io = reader_buffer;
		break;
	}

	/* perforated tape punch */
	case 05: /* PPA */
	{	/*
		 * Punch Perforated Tape, Alphanumeric
		 * ppa Address 0005
		 *
		 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
		 * Bit 17 conditions Hole 1. Bit 16 conditions Hole 2, etc. Bit 10 conditions Hole 8
		 */
		tape_write(*io & 0377);
		etime = 15875;	/* 1/63 second */
		break;
	}
	case 06: /* PPB */
	{	/*
		 * Punch Perforated Tape, Binary
		 * ppb Addres 0006 
		 *
		 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
		 * Bit 5 conditions Hole 1. Bit 4 conditions Hole 2, etc. Bit 0 conditions Hole 6.
		 * Hole 7 is left blank. Hole 8 is always punched in this mode. 
		 */
		tape_write((*io >> 12) | 0200);
		etime = 15875;	/* 1/63 second */
		break;
	}

	/* alphanumeric on-line typewriter */
	case 03: /* TYO */
	{
		logerror("Warning, TYO instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));

		teletyper_write((*io) & 077);
		etime = 105000;	/* approx. 1/9.5 second */
		break;
	}
	case 04: /* TYI */
	{
		/* When a typewriter key is struck, the code for the struck key is placed in the
		 * typewriter buffer, Program Flag 1 is set, and the type-in status bit is set to
		 * one. A program designed to accept typed-in data would periodically check
		 * Program Flag 1, and if found to be set, an In-Out Transfer Instruction with
		 * address 4 could be executed for the information to be transferred to the
		 * In-Out Register. This In-Out Transfer should not use the optional in-out wait.
		 * The information contained in the typewriter buffer is then transferred to the
		 * right six bits of the In-Out Register. The tyi instruction automatically
		 * clears the In-Out Register before transferring the information and also clears
		 * the type-in status bit.
		 */

		logerror("Warning, TYI instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));

		etime = 0; /* To be confirmed */
		*io = typewriter_buffer;
		if (! typewriter_buffer_status)
			*io |= 0100;
		else
			typewriter_buffer_status = 0;

		break;
	}

	/* CRT display */
	case 07: /* DPY */
	{
		/*
		 * PRECISION CRT DISPLAY (TYPE 30)
		 *
		 * This sixteen-inch cathode ray tube display is intended to be used as an on-line output device for the
		 * PDP-1. It is useful for high speed presentation of graphs, diagrams, drawings, and alphanumerical
		 * information. The unit is solid state, self-powered and completely buffered. It has magnetic focus and
		 * deflection.
		 *
		 * Display characteristics are as follows:
		 *
		 *     Random point plotting
		 *     Accuracy of points +/-3 per cent of raster size
		 *     Raster size 9.25 by 9.25 inches
		 *     1024 by 1024 addressable locations
		 *     Fixed origin at center of CRT
		 *     Ones complement binary arithmetic
		 *     Plots 20,000 points per second
		 *
		 * Resolution is such that 512 points along each axis are discernable on the face of the tube.
		 *
		 * One instruction is added to the PDP-1 with the installation of this display:
		 *
		 * Display One Point On CRT
		 * dpy Address 0007
		 *
		 * This instruction clears the light pen status bit and displays one point using bits 0 through 9 of the
		 * AC to represent the (signed) X coordinate of the point and bits 0 through 9 of the IO as the (signed)
		 * Y coordinate.
		 *
		 * Many variations of the Type 30 Display are available. Some of these include hardware for line and
		 * curve generation.
		 */
		int x;
		int y;
		etime = 45;
		x = ((cpu_get_reg(PDP1_AC)+0400000)&0777777) >> 8;
		y = ((cpu_get_reg(PDP1_IO)+0400000)&0777777) >> 8;
		pdp1_plot(x,y);
		break;
	}

	/* Spacewar! controllers */
	case 011: /* IOT 011 (undocumented?), Input... */
	{
		int key_state = readinputport(pdp1_spacewar_controllers);
		etime = 0;	/* value unknown */
		*io = 0;
		if (key_state&FIRE_PLAYER2)         *io |= 040000;
		if (key_state&THRUST_PLAYER2)       *io |= 0100000;
		if (key_state&ROTATE_LEFT_PLAYER2)  *io |= 0200000;
		if (key_state&ROTATE_RIGHT_PLAYER2) *io |= 0400000;
		if (key_state&FIRE_PLAYER1)         *io |= 01;
		if (key_state&THRUST_PLAYER1)       *io |= 02;
		if (key_state&ROTATE_LEFT_PLAYER1)  *io |= 04;
		if (key_state&ROTATE_RIGHT_PLAYER1) *io |= 010;
		break;
	}

	/* check status */
	case 033: /* CKS */
	{
		logerror("Warning, CKS instruction not fully emulated: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));
		etime = 0;
		break;
	}

	default:
	{
		logerror("Not supported IOT command: io=0%06o, mb=0%06o, pc=0%06o\n", *io, md, cpu_get_reg(PDP1_PC));
		etime = 0;
		break;
	}
	}

	return etime;
}


/*
	keyboard handlers
*/

int pdp1_keyboard(void)
{
	int i;
	int j;

	int typewriter_keys[4];

	static int old_typewriter_keys[4];

	int typewriter_transitions;


	for (i=0; i<4; i++)
		typewriter_keys[i] = readinputport(pdp1_typewriter + i);

	for (i=0; i<4; i++)
	{
		typewriter_transitions = typewriter_keys[i] & (~ old_typewriter_keys[i]);
		if (typewriter_transitions)
		{
			for (j=0; (((typewriter_transitions >> j) & 1) == 0) /*&& (j<16)*/; j++)
				;
			typewriter_buffer = (i << 4) + j;
			typewriter_buffer_status = 1;
			cpu_set_reg(PDP1_F1, 1);
			break;
		}
	}

	for (i=0; i<4; i++)
		old_typewriter_keys[i] = typewriter_keys[i];

	return 0;
}

static int extend_switch;
int pdp1_ta;
int pdp1_tw;

/*
	Not a real interrupt - just handle keyboard input
*/
int pdp1_interrupt(void)
{
	int control_keys;
	int tw_keys;
	int ta_keys;

	static int old_control_keys;
	static int old_tw_keys;
	static int old_ta_keys;

	int control_transitions;
	int tw_transitions;
	int ta_transitions;


	/* update display */
	pdp1_screen_update();


	cpu_set_reg(PDP1_S, readinputport(pdp1_sense_switches));

	/* read new state of control keys */
	control_keys = readinputport(pdp1_control_switches);

	if (control_keys & pdp1_control)
	{
		/* compute transitions */
		control_transitions = control_keys & (~ old_control_keys);

		if (control_transitions & pdp1_extend)
		{
			extend_switch = ! extend_switch;
		}
		if (control_transitions & pdp1_start_nobrk)
		{
			/*cpunum_reset(0);*/
			/*cpunum_set_reg(0, PDP1_EXTEND, extend_switch);*/
			/*cpunum_set_reg(0, PDP1_SBM, 0);*/
			cpunum_set_reg(0, PDP1_OV, 0);
			cpunum_set_reg(0, PDP1_PC, pdp1_ta);
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_start_brk)
		{
			/*cpunum_reset(0);*/
			/*cpunum_set_reg(0, PDP1_EXTEND, extend_switch);*/
			/*cpunum_set_reg(0, PDP1_SBM, 1);*/
			cpunum_set_reg(0, PDP1_OV, 0);
			cpunum_set_reg(0, PDP1_PC, pdp1_ta);
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_stop)
		{
			cpunum_set_reg(0, PDP1_RUN, 0);
			cpunum_set_reg(0, PDP1_RIM, 0);	/* bug : we stop after reading an even-numbered word
											(i.e. data), whereas a real pdp-1 stops after reading
											an odd-numbered word (i.e. dio instruciton) */
		}
		if (control_transitions & pdp1_continue)
		{
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_examine)
		{
			int value;
			/*cpunum_reset(0);*/
			cpunum_set_reg(0, PDP1_PC, pdp1_ta);
			cpunum_set_reg(0, PDP1_MA, pdp1_ta);
			value = READ_PDP_18BIT(pdp1_ta);
			cpunum_set_reg(0, PDP1_MB, value);
			cpunum_set_reg(0, PDP1_AC, value);
		}
		if (control_transitions & pdp1_deposit)
		{
			/*cpunum_reset(0);*/
			cpunum_set_reg(0, PDP1_PC, pdp1_ta);
			cpunum_set_reg(0, PDP1_MA, pdp1_ta);
			cpunum_set_reg(0, PDP1_AC, pdp1_tw);
			cpunum_set_reg(0, PDP1_MB, pdp1_tw);
			WRITE_PDP_18BIT(pdp1_ta, pdp1_tw);
		}
		if (control_transitions & pdp1_read_in)
		{	/* set cpu to read instruction from perforated tape */
			/*cpunum_reset(0);*/
			/*cpunum_set_reg(0, PDP1_EXTEND, extend_switch);*/
			/*cpunum_set_reg(0, PDP1_OV, 0);*/		/* right??? */
			cpunum_set_reg(0, PDP1_RUN, 0);		/* right??? */
			cpunum_set_reg(0, PDP1_RIM, 1);
		}
		if (control_transitions & pdp1_reader)
		{
		}
		if (control_transitions & pdp1_tape_feed)
		{
		}
		if (control_transitions & pdp1_single_step)
		{
			/*cpunum_set_reg(0, PDP1_SNGLSTEP, ! cpunum_get_reg(0, PDP1_SNGLSTEP));*/
		}
		if (control_transitions & pdp1_single_inst)
		{
			/*cpunum_set_reg(0, PDP1_SNGLINST, ! cpunum_get_reg(0, PDP1_SNGLINST));*/
		}

		/* remember new state of control keys */
		old_control_keys = control_keys;


		tw_keys = (readinputport(pdp1_tw_switches_MSB) << 16) | readinputport(pdp1_tw_switches_LSB);

		/* compute transitions */
		tw_transitions = tw_keys & (~ old_tw_keys);

		pdp1_tw ^= tw_transitions;

		/* remember new state of test word keys */
		old_tw_keys = tw_keys;


		ta_keys = readinputport(pdp1_ta_switches);

		/* compute transitions */
		ta_transitions = ta_keys & (~ old_ta_keys);

		pdp1_ta ^= ta_transitions;

		/* remember new state of test word keys */
		old_ta_keys = ta_keys;

	}
	else
	{
		old_control_keys = 0;
		old_tw_keys = 0;
		old_ta_keys = 0;

		pdp1_keyboard();
	}

	return ignore_interrupt();
}

int pdp1_get_test_word(void)
{
	return pdp1_tw;
}

