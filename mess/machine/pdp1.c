#include <stdio.h>

#include "driver.h"

#include "includes/pdp1.h"
#include "cpu/pdp1/pdp1.h"


int *pdp1_memory;

static int fio_dec = 0;
static int concise = 0;
static int case_state = 0;
static int reader_buffer = 0;



/*
	driver init function

	Just set up the pdp1_memory pointer.
*/
void init_pdp1(void)
{
	/* set up memory regions */
	pdp1_memory = (int *) memory_region(REGION_CPU1); /* really bad! */
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
	int etime=0;

	switch (cpu_get_reg(PDP1_Y) & 077)
	{
	case 00:
	{
		/* Waiting for output to finnish */
		etime=5;
		break;
	}

	/* perforated tape reader */
	case 01: /* RPA */
	{
		/*
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
		logerror("\nWarning, RPA instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,cpu_get_reg(PDP1_Y),cpu_get_reg(PDP1_PC));

		etime=10;	/* probably some more */
		/* somehow read a byte... */

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
			/**io=0;
			*io|=(reader_buffer&128)<<9;
			*io|=(reader_buffer&64)<<10;
			*io|=(reader_buffer&32)<<11;
			*io|=(reader_buffer&16)<<12;
			*io|=(reader_buffer&8)<<13;
			*io|=(reader_buffer&4)<<14;
			*io|=(reader_buffer&2)<<15;
			*io|=(reader_buffer&1)<<16;*/
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
		logerror("\nWarning, RPB instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,cpu_get_reg(PDP1_Y),cpu_get_reg(PDP1_PC));

		etime=10;	/* probably some more */
		/* somehow read a byte... */

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
		logerror("\nWarning, RRB instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,cpu_get_reg(PDP1_Y),cpu_get_reg(PDP1_PC));
		etime=5;
		cpu_set_reg(PDP1_F1,0);
		*io = reader_buffer;
		break;
	}

	/* perforated tape punch */
	case 05: /* PPA */
	{
		break;
	}
	case 06: /* PPA */
	{
		break;
	}

	/* alphanumeric on-line typewriter */
	case 03: /* TYO */
	{
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

		logerror("\nWarning, TYI instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,cpu_get_reg(PDP1_Y),cpu_get_reg(PDP1_PC));

		etime=10; /* probably heaps more */
		*io=0;
		*io=fio_dec&077;
		fio_dec=0;
		concise=0;

		break;
	}
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
		etime=10; /* probably some more */
		x=(cpu_get_reg(PDP1_AC)+0400000)&0777777;
		y=(cpu_get_reg(PDP1_IO)+0400000)&0777777;
		pdp1_plot(x,y);
		break;
	}
	case 011: /* IOT 011 (undocumented?), Input... */
	{
		int key_state=readinputport(0);
		etime=10; /* probably heaps more */
		*io=0;
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
	case 033: /* CKS */
	{
		etime=5;
	}
	default:
	{
		etime=5;
		logerror("\nNot supported IOT command: io=0%06o, y=0%06o, pc=0%06o",*io,cpu_get_reg(PDP1_Y),cpu_get_reg(PDP1_PC));
		break;
	}
	}

	return etime;
}


/*
	keyboard handlers
*/

typedef struct
{
	int     fio_dec;
	int     concise;
	int     lower_osd;
	int     upper_osd;
	char   *lower_text;
	char   *upper_text;
} key_trans_pdp1;

static key_trans_pdp1 kbd[] =
{
{  000,     0000, 0            , 0                ,"Tape Feed"                 ,""             },
{  256,     0100, KEYCODE_DEL  , KEYCODE_DEL      ,"Delete"                    ,""             },
{  000,     0200, KEYCODE_SPACE, KEYCODE_SPACE    ,"Space"                     ,""             },
{  001,     0001, KEYCODE_1    , KEYCODE_SLASH    ,"1"                         ,"\""           },
{  002,     0002, KEYCODE_2    , KEYCODE_QUOTE    ,"2"                         ,"'"            },
{  003,     0203, KEYCODE_3    , KEYCODE_TILDE    ,"3"                         ,"~"            },
{  004,     0004, KEYCODE_4    , 0                ,"4"                         ,"(implies)"    },
{  005,     0205, KEYCODE_5    , 0                ,"5"                         ,"(or)"         },
{  006,     0206, KEYCODE_6    , 0                ,"6"                         ,"(and)"        },
{  007,     0007, KEYCODE_7    , 0                ,"7"                         ,"<"            },
{  010,     0010, KEYCODE_8    , 0                ,"8"                         ,">"            },
{  011,     0211, KEYCODE_9    , KEYCODE_UP       ,"9"                         ,"(up arrow)"   },
{  256,     0013, KEYCODE_STOP , KEYCODE_STOP     ,"Stop Code"                 ,""             },
{  020,     0020, KEYCODE_0    , KEYCODE_RIGHT    ,"0"                         ,"(right arrow)"},
{  021,     0221, KEYCODE_SLASH, 0                ,"/"                         ,"?"            },
{  022,     0222, KEYCODE_S    , KEYCODE_S        ,"s"                         ,"S"            },
{  023,     0023, KEYCODE_T    , KEYCODE_T        ,"t"                         ,"T"            },
{  024,     0224, KEYCODE_U    , KEYCODE_U        ,"u"                         ,"U"            },
{  025,     0025, KEYCODE_V    , KEYCODE_V        ,"v"                         ,"V"            },
{  026,     0026, KEYCODE_W    , KEYCODE_W        ,"w"                         ,"W"            },
{  027,     0227, KEYCODE_X    , KEYCODE_X        ,"x"                         ,"X"            },
{  030,     0230, KEYCODE_Y    , KEYCODE_Y        ,"y"                         ,"Y"            },
{  031,     0031, KEYCODE_Z    , KEYCODE_Z        ,"z"                         ,"Z"            },
{  033,     0233, KEYCODE_COMMA, KEYCODE_EQUALS   ,","                         ,"="            },
{  034,      256, 0            , 0                ,"black"                     ,""             },
{  035,      256, 0            , 0                ,"red"                       ,""             },
{  036,     0236, KEYCODE_TAB  , 0                ,"tab"                       ,""             },
{  040,     0040, 0            , 0                ," (non-spacing middle dot)" ,"_"            },
{  041,     0241, KEYCODE_J    , KEYCODE_J        ,"j"                         ,"J"            },
{  042,     0242, KEYCODE_K    , KEYCODE_K        ,"k"                         ,"K"            },
{  043,     0043, KEYCODE_L    , KEYCODE_L        ,"l"                         ,"L"            },
{  044,     0244, KEYCODE_M    , KEYCODE_M        ,"m"                         ,"M"            },
{  045,     0045, KEYCODE_N    , KEYCODE_N        ,"n"                         ,"N"            },
{  046,     0046, KEYCODE_O    , KEYCODE_O        ,"o"                         ,"O"            },
{  047,     0247, KEYCODE_P    , KEYCODE_P        ,"p"                         ,"P"            },
{  050,     0250, KEYCODE_Q    , KEYCODE_Q        ,"q"                         ,"Q"            },
{  051,     0051, KEYCODE_R    , KEYCODE_R        ,"r"                         ,"R"            },
{  054,     0054, KEYCODE_MINUS, KEYCODE_PLUS_PAD ,"-"                         ,"+"            },
{  055,     0255, KEYCODE_CLOSEBRACE, 0           ,")"                         ,"]"            },
{  056,     0256, 0            , 0                ," (non-spacing overstrike)" ,"|"            },
{  057,     0057, KEYCODE_OPENBRACE, 0            ,"("                         ,"["            },
{  061,     0061, KEYCODE_A    , KEYCODE_A        ,"a"                         ,"A"            },
{  062,     0062, KEYCODE_B    , KEYCODE_B        ,"b"                         ,"B"            },
{  063,     0263, KEYCODE_C    , KEYCODE_C        ,"c"                         ,"C"            },
{  064,     0064, KEYCODE_D    , KEYCODE_D        ,"d"                         ,"D"            },
{  065,     0265, KEYCODE_E    , KEYCODE_E        ,"e"                         ,"E"            },
{  066,     0266, KEYCODE_F    , KEYCODE_F        ,"f"                         ,"F"            },
{  067,     0067, KEYCODE_G    , KEYCODE_G        ,"g"                         ,"G"            },
{  070,     0070, KEYCODE_H    , KEYCODE_H        ,"h"                         ,"H"            },
{  071,     0271, KEYCODE_I    , KEYCODE_I        ,"i"                         ,"I"            },
{  072,     0272, KEYCODE_LSHIFT, 0               ,"Lower Case"                ,""             },
{  073,     0073, KEYCODE_ASTERISK,0              ,".(multiply)"               ,""             },
{  074,     0274, KEYCODE_LSHIFT, 0               ,"Upper Case"                ,""             },
{  075,     0075, KEYCODE_BACKSPACE,0             ,"Backspace"                 ,""             },
{  077,     0277, KEYCODE_ENTER, 0                ,"Carriage Return"           ,""             },
{  257,     0000, 0            , 0                ,""                          ,""             }
};

/* return first found pressed key */
int pdp1_keyboard(void)
{
	int i=0;

	fio_dec=0;
	concise=0;
	if (osd_is_key_pressed(KEYCODE_LSHIFT))
	{
		if (case_state)
		{
			fio_dec=072;
			concise=0272;
		}
		else
		{
			fio_dec=074;
			concise=0274;
		}
		case_state=(case_state+1)%2;
		return 1;
	}

	while (kbd[i].fio_dec!=257)
	{
		if (case_state==0) /* lower case */
		{
			if ((kbd[i].lower_osd)&&(osd_is_key_pressed(kbd[i].lower_osd)))
			{
				fio_dec=kbd[i].fio_dec;
				concise=kbd[i].concise;
				return 1;
			}
		}
		else
		{
			if ((kbd[i].upper_osd)&&(osd_is_key_pressed(kbd[i].upper_osd)))
			{
				fio_dec=kbd[i].fio_dec;
				concise=kbd[i].concise;
				return 1;
			}
		}
		i++;
	}
	return 0;
}
