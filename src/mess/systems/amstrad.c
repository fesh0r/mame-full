/******************************************************************************

    amstrad.c
	system driver

	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "machine/8255ppi.h"
//#include "mess/systems/i8255.h"
#include "mess/vidhrdw/hd6845s.h"
#include "mess/machine/amstrad.h"
#include "mess/vidhrdw/amstrad.h"
#include "mess/machine/nec765.h"


/* On the Amstrad, any part of the 64k memory can be access by the video
hardware (GA and CRTC - the CRTC specifies the memory address to access,
and the GA fetches 2 bytes of data for each 1us cycle.

The Z80 must also access the same ram.

To maintain the screen display, the Z80 is halted on each memory access.

The result is that timing for opcodes, appears to fall into a nice pattern,
where the time for each opcode can be measured in NOP cycles. NOP cycles is
the name I give to the time taken for one NOP command to execute.

This happens to be 1us.

From measurement, there are 64 NOPs per line, with 312 lines per screen.
This gives a total of 19968 NOPs per frame. */

/* number of us cycles per frame (measured) */
#define AMSTRAD_US_PER_FRAME    19968
#define AMSTRAD_FPS             50
#define AMSTRAD_T_STATES_PER_US 4

/* These are the measured visible screen dimensions in CRTC characters.
50 CRTC chars in X, 35 CRTC chars in Y (8 lines per char assumed) */
#define AMSTRAD_SCREEN_WIDTH	(50*16)
#define AMSTRAD_SCREEN_HEIGHT	(35*8)

/* machine name is defined in bits 3,2,1.
Names are: Isp, Triumph, Saisho, Solavox, Awa, Schneider, Orion, Amstrad.
Name is set by a link on the PCB

Bits for this port:
7: Cassette read data
6: Printer busy
5: /Expansion Port signal
4: Screen Refresh
3..1: Machine name
0: VSYNC state
*/

/* psg access operation:
0 = inactive, 1 = read register data, 2 = write register data,
3 = write register index */
static int amstrad_psg_operation;

/* data present on input of ppi, and data written to ppi output */
static int ppi_port_inputs[3];
static int ppi_port_outputs[3];

/* keyboard line 0-9 */
static int amstrad_keyboard_line;
static int crtc_vsync_output;

static void update_psg(void)
{
	switch (amstrad_psg_operation)
	{
		/* inactive */
	default:
		break;

		/* psg read data register */
	case 1:
		{
			ppi_port_inputs[0] = AY8910_read_port_0_r(0);
		}
		break;

		/* psg write data */
	case 2:
		{
			AY8910_write_port_0_w(0, ppi_port_outputs[0]);
		}
		break;

		/* write index register */
	case 3:
		{
			AY8910_control_port_0_w(0, ppi_port_outputs[0]);
		}
		break;
	}
}



/* ppi port a read */
int amstrad_ppi_porta_r(int chip)
{
	update_psg();

	return ppi_port_inputs[0];
}


/* ppi port b read */
int amstrad_ppi_portb_r(int chip)
{
        return ((ppi_port_inputs[1] & 0x0fe) | crtc_vsync_output);
}

void amstrad_ppi_porta_w(int chip, int Data)
{
	ppi_port_outputs[0] = Data;

	update_psg();
}

void amstrad_ppi_portc_w(int chip, int Data)
{
	ppi_port_outputs[2] = Data;

	amstrad_psg_operation = (Data >> 6) & 0x03;
	amstrad_keyboard_line = (Data & 0x0f);

	update_psg();
}

static ppi8255_interface amstrad_ppi8255_interface =
{
	1,
	amstrad_ppi_porta_r,
	amstrad_ppi_portb_r,
	NULL,
	amstrad_ppi_porta_w,
	NULL,
	amstrad_ppi_portc_w
};

extern void amstrad_seek_callback(int, int);
extern void amstrad_get_id_callback(int, nec765_id *, int, int);
extern void amstrad_get_sector_ptr_callback(int, int, int, char **);
extern int amstrad_get_sectors_per_track(int, int);


static nec765_interface amstrad_nec765_interface =
{
	amstrad_seek_callback,
	amstrad_get_sectors_per_track,
	amstrad_get_id_callback,
	amstrad_get_sector_ptr_callback,
};

/* pointers to current ram configuration selected for banks */
static unsigned char *AmstradCPC_RamBanks[4];

/* base of all ram allocated - 128k */
unsigned char *Amstrad_Memory;

/* current selected upper rom */
static unsigned char *Amstrad_UpperRom;

/* bit 0,1 = mode, 2 = if zero, os rom is enabled, otherwise
disabled, 3 = if zero, upper rom is enabled, otherwise disabled */
unsigned char AmstradCPC_GA_RomConfiguration;

short AmstradCPC_PenColours[18];


/* 16 colours, + 1 for border */
unsigned short amstrad_colour_table[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
	29, 30, 31
};

static int RamConfigurations[8 * 4] =
{
	0, 1, 2, 3,						   /* config 0 */
	0, 1, 2, 7,						   /* config 1 */
	4, 5, 6, 7,						   /* config 2 */
	0, 3, 2, 7,						   /* config 3 */
	0, 4, 2, 3,						   /* config 4 */
	0, 5, 2, 3,						   /* config 5 */
	0, 6, 2, 3,						   /* config 6 */
	0, 7, 2, 3						   /* config 7 */
};

/* ram configuration */
static unsigned char AmstradCPC_GA_RamConfiguration;

/* selected pen */
static unsigned char AmstradCPC_GA_PenSelected;



static unsigned char AmstradCPC_ReadKeyboard(void)
{
	if (amstrad_keyboard_line > 9)
		return 0x0ff;

	return readinputport(amstrad_keyboard_line);
}

void Amstrad_RethinkMemory(void)
{
	/* the following is used for banked memory read/writes and for setting up
	 * opcode and opcode argument reads */
	{
		unsigned char *BankBase;

		/* bank 0 - 0x0000..0x03fff */
		if ((AmstradCPC_GA_RomConfiguration & 0x04) == 0)
		{
			BankBase = &memory_region(REGION_CPU1)[0x010000];
		}
		else
		{
			BankBase = AmstradCPC_RamBanks[0];
		}
		/* set bank address for MRA_BANK1 */
		cpu_setbank(1, BankBase);


		/* bank 1 - 0x04000..0x07fff */
		cpu_setbank(2, AmstradCPC_RamBanks[1]);

		/* bank 2 - 0x08000..0x0bfff */
		cpu_setbank(3, AmstradCPC_RamBanks[2]);

		/* bank 3 - 0x0c000..0x0ffff */
		if ((AmstradCPC_GA_RomConfiguration & 0x08) == 0)
		{
			BankBase = Amstrad_UpperRom;
		}
		else
		{
			BankBase = AmstradCPC_RamBanks[3];
		}
		cpu_setbank(4, BankBase);

		cpu_setbank(5, AmstradCPC_RamBanks[0]);
		cpu_setbank(6, AmstradCPC_RamBanks[1]);
		cpu_setbank(7, AmstradCPC_RamBanks[2]);
		cpu_setbank(8, AmstradCPC_RamBanks[3]);
	}
}




/* simplified ram configuration - e.g. only correct for 128k machines */
void AmstradCPC_GA_SetRamConfiguration(void)
{
	int ConfigurationIndex = AmstradCPC_GA_RamConfiguration & 0x07;
	int BankIndex;
	unsigned char *BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2)];
	BankAddr = Amstrad_Memory + (BankIndex << 14);

	AmstradCPC_RamBanks[0] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 1];
	BankAddr = Amstrad_Memory + (BankIndex << 14);

	AmstradCPC_RamBanks[1] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 2];
	BankAddr = Amstrad_Memory + (BankIndex << 14);

	AmstradCPC_RamBanks[2] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 3];
	BankAddr = Amstrad_Memory + (BankIndex << 14);

	AmstradCPC_RamBanks[3] = BankAddr;
}

void AmstradCPC_GA_Write(int Data)
{
#ifdef AMSTRAD_DEBUG
	printf("GA Write: %02x\r\n", Data);
#endif

	switch ((Data & 0x0c0) >> 6)
	{
	case 0:
		{
			/* pen  selection */
			AmstradCPC_GA_PenSelected = Data;
		}
		return;

	case 1:
		{
			int PenIndex;

			/* colour selection */
			if (AmstradCPC_GA_PenSelected & 0x010)
			{
				/* specify border colour */
				PenIndex = 16;
			}
			else
			{
				PenIndex = AmstradCPC_GA_PenSelected & 0x0f;
			}

			AmstradCPC_PenColours[PenIndex] = Data & 0x01f;
		}
		return;

	case 2:
		{
			AmstradCPC_GA_RomConfiguration = Data;
		}
		break;

	case 3:
		{
			AmstradCPC_GA_RamConfiguration = Data;

			AmstradCPC_GA_SetRamConfiguration();
		}
		break;
	}


	Amstrad_RethinkMemory();
}

/* very simplified version of setting upper rom - since
we are not going to have additional roms, this is the best
way */
void AmstradCPC_SetUpperRom(int Data)
{
	/* low byte of port holds the rom index */
	if ((Data & 0x0ff) == 7)
	{
		/* select dos rom */
		Amstrad_UpperRom = &memory_region(REGION_CPU1)[0x018000];
	}
	else
	{
		/* select basic rom */
		Amstrad_UpperRom = &memory_region(REGION_CPU1)[0x014000];
	}

	Amstrad_RethinkMemory();
}

/*
Port decoding:

  Bit 15 = 0, Bit 14 = 1: Access Gate Array (W)
  Bit 14 = 0: Access CRTC (R/W)
  Bit 13 = 0: Select upper rom (W)
  Bit 12 = 0: Printer (W)
  Bit 11 = 0: PPI (8255) (R/W)
  Bit 10 = 0: Expansion.

  Bit 10 = 0, Bit 7 = 0: uPD 765A FDC
 */


/* port handler */
int AmstradCPC_ReadPortHandler(int Offset)
{
	unsigned char data = 0x0ff;

	if ((Offset & 0x04000) == 0)
	{
		/* CRTC */
		unsigned int Index;

		Index = (Offset & 0x0300) >> 8;

		if (Index == 3)
		{
			// CRTC Read register
			data = hd6845s_register_r();
		}
	}

	if ((Offset & 0x0800) == 0)
	{
		/* 8255 PPI */

		unsigned int Index;

		Index = (Offset & 0x0300) >> 8;

		data = ppi8255_r(0, Index);
	}

	if ((Offset & 0x0400) == 0)
	{
		if ((Offset & 0x080) == 0)
		{
			unsigned int Index;

			Index = ((Offset & 0x0100) >> (8 - 1)) | (Offset & 0x01);

			switch (Index)
			{
			case 2:
				{
					/* read status */
					data = nec765_status_r();
				}
				break;

			case 3:
				{
					/* read data register */
					data = nec765_data_r();
				}
				break;

			default:
				break;
			}
		}
	}


	return data;

}


/* Offset handler for write */
void AmstradCPC_WritePortHandler(int Offset, int Data)
{
#ifdef AMSTRAD_DEBUG
	printf("Write port Offs: %04x Data: %04x\r\n", Offset, Data);
#endif
	if ((Offset & 0x0c000) == 0x04000)
	{
		/* GA */
		AmstradCPC_GA_Write(Data);
	}

	if ((Offset & 0x04000) == 0)
	{
		/* CRTC */
		unsigned int Index;

		Index = (Offset & 0x0300) >> 8;

		switch (Index)
		{
		case 0:
			{
				/* register select */
				hd6845s_index_w(Data);
			}
			break;

		case 1:
			{
				/* write data */
				hd6845s_register_w(Data);
			}
			break;

		default:
			break;
		}
	}

	if ((Offset & 0x02000) == 0)
	{
		AmstradCPC_SetUpperRom(Data);
	}

	if ((Offset & 0x0800) == 0)
	{
		unsigned int Index;

		Index = (Offset & 0x0300) >> 8;

		ppi8255_w(0, Index, Data);
	}

	if ((Offset & 0x0400) == 0)
	{
		if ((Offset & 0x080) == 0)
		{
			unsigned int Index;

			Index = ((Offset & 0x0100) >> (8 - 1)) | (Offset & 0x01);

			switch (Index)
			{
			case 0:
				{
					/* fdc motor on */
//                  FDD_SetMotor(Data);
				}
				break;

			case 3:
				{
					nec765_data_w(Data);
				}
				break;

			default:
				break;
			}
		}
	}
}

/* The Amstrad interrupts are not really 300hz. The gate array
counts the CRTC HSYNC pulses. (It has a internal 6-bit counter).
When the counter in the gate array reaches 52, a interrupt is signalled.
The counter is reset and the count starts again. If the Z80 has ints
enabled, the int will be executed, otherwise the int can be held.
As soon as the z80 acknowledges the int, this is recognised by the
gate array and the top bit of this counter is reset.

  This counter is also reset two scanlines into the VSYNC signal

Interrupts are therefore never closer than 32 lines. For this
driver, for now, we will assume that ints do occur every 300Hz. */

static int count = 6;

int amstrad_timer_interrupt(void)
{
	count--;

	if (count < 0)
	{
		crtc_vsync_output = 1;
		count = 6;
	}
	else
	{
		crtc_vsync_output = 0;
	}


	cpu_set_irq_line(0, 0, HOLD_LINE);

	return ignore_interrupt();
	//return 0;
}

int amstrad_frame_interrupt(void)
{
	return 0;
}

int int_counter = 0;


int amstrad_interrupt(void)
{
	/* update graphics display */
	amstrad_update_scanline();

	int_counter++;

	if (int_counter == 52)
	{
		int_counter = 0;
		cpu_set_irq_line(0, 0, HOLD_LINE);
	}


	return ignore_interrupt();

}

/*void amstrad_timer_callback(int timer_id)
{


}
  */



//        timer_set(amstrad_time_to_int, 0, amstrad_timer_callback);


void amstrad_common_init(void)
{
	/* set all colours to black */
	int i;

	for (i = 0; i < 17; i++)
	{
		AmstradCPC_PenColours[i] = 0x014;
	}

	ppi8255_init(&amstrad_ppi8255_interface);

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
	cpu_setbankhandler_r(3, MRA_BANK3);
	cpu_setbankhandler_r(4, MRA_BANK4);

	cpu_setbankhandler_w(5, MWA_BANK5);
	cpu_setbankhandler_w(6, MWA_BANK6);
	cpu_setbankhandler_w(7, MWA_BANK7);
	cpu_setbankhandler_w(8, MWA_BANK8);

	cpu_0_irq_line_vector_w(0, 0x0ff);

	nec765_init(&amstrad_nec765_interface);
}

void amstrad_init_machine(void)
{
        amstrad_common_init();

        amstrad_setup_machine();

        /* bits 1,2,3 are connected to links on the PCB, these
        define the machine name.

        000 = Isp
        001 = Triumph
        010 = Saisho
        011 = Solavox
        100 = Awa
        101 = Schneider
        110 = Orion
        111 = Amstrad

        bit 4 is connected to a link on the PCB used to define screen
        refresh rate. 1 = 50Hz, 0 = 60Hz */

        ppi_port_inputs[1] = readinputport(10) & 0x01e;

}

void    kccomp_init_machine(void)
{
        amstrad_common_init();

        amstrad_setup_machine();

        /* bit 1 = /TEST. When 0, KC compact will enter data transfer
        sequence, where another system using the expansion port signals
        DATA2,DATA1, /STROBE and DATA7 can transfer 256 bytes of program.
        When the program has been transfered, it will be executed. This
        is not supported in the driver */
        /* bit 3,4 are tied to +5V, bit 2 is tied to 0V */
        ppi_port_inputs[1] = (1<<4) | (1<<3) | 2;
}


/* sets up for a machine reset */
void Amstrad_Reset(void)
{
	/* enable lower rom (OS rom) */
	AmstradCPC_GA_Write(0x089);

	/* set ram config 0 */
	AmstradCPC_GA_Write(0x0c0);
}



/* amstrad has 27 colours, 3 levels of R,G and B. The other colours
are copies of existing ones in the palette */

unsigned char amstrad_palette[32 * 3] =
{
	0x080, 0x080, 0x080,			   /* white */
	0x080, 0x080, 0x080,			   /* white */
	0x000, 0x0ff, 0x080,			   /* sea green */
	0x0ff, 0x0ff, 0x080,			   /* pastel yellow */
	0x000, 0x000, 0x080,			   /* blue */
	0x0ff, 0x000, 0x080,			   /* purple */
	0x000, 0x080, 0x080,			   /* cyan */
	0x0ff, 0x080, 0x080,			   /* pink */
	0x0ff, 0x000, 0x080,			   /* purple */
	0x0ff, 0x0ff, 0x080,			   /* pastel yellow */
	0x0ff, 0x0ff, 0x000,			   /* bright yellow */
	0x0ff, 0x0ff, 0x0ff,			   /* bright white */
	0x0ff, 0x000, 0x000,			   /* bright red */
	0x0ff, 0x000, 0x0ff,			   /* bright magenta */
	0x0ff, 0x080, 0x000,			   /* orange */
	0x0ff, 0x080, 0x0ff,			   /* pastel magenta */
	0x000, 0x000, 0x080,			   /* blue */
	0x000, 0x0ff, 0x080,			   /* sea green */
	0x000, 0x0ff, 0x000,			   /* bright green */
	0x000, 0x0ff, 0x0ff,			   /* bright cyan */
	0x000, 0x000, 0x000,			   /* black */
	0x000, 0x000, 0x0ff,			   /* bright blue */
	0x000, 0x080, 0x000,			   /* green */
	0x000, 0x080, 0x0ff,			   /* sky blue */
	0x080, 0x000, 0x080,			   /* magenta */
	0x080, 0x0ff, 0x080,			   /* pastel green */
	0x080, 0x0ff, 0x080,			   /* lime */
	0x080, 0x0ff, 0x0ff,			   /* pastel cyan */
	0x080, 0x000, 0x000,			   /* Red */
	0x080, 0x000, 0x0ff,			   /* mauve */
	0x080, 0x080, 0x000,			   /* yellow */
	0x080, 0x080, 0x0ff,			   /* pastel blue */
};


/* Initialise the palette */
static void amstrad_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy(sys_palette, amstrad_palette, sizeof (amstrad_palette));
	memcpy(sys_colortable, amstrad_colour_table, sizeof (amstrad_colour_table));
}


/* Memory is banked in 16k blocks. The ROM can
be paged into bank 0 and bank 3. */
static struct MemoryReadAddress readmem_amstrad[] =
{
	{0x00000, 0x03fff, MRA_BANK1},
	{0x04000, 0x07fff, MRA_BANK2},
	{0x08000, 0x0bfff, MRA_BANK3},
	{0x0c000, 0x0ffff, MRA_BANK4},
	{0x010000, 0x013fff, MRA_ROM},	   /* OS */
	{0x014000, 0x017fff, MRA_ROM},	   /* BASIC */
	{0x018000, 0x01bfff, MRA_ROM},	   /* AMSDOS */
	{-1}							   /* end of table */
};


static struct MemoryWriteAddress writemem_amstrad[] =
{
	{0x00000, 0x03fff, MWA_BANK5},
	{0x04000, 0x07fff, MWA_BANK6},
	{0x08000, 0x0bfff, MWA_BANK7},
	{0x0c000, 0x0ffff, MWA_BANK8},
	{-1}							   /* end of table */
};

#if 0
static struct GfxLayout amstrad_mode1_gfxlayout =
{
	/* width, height */
	8, 1,							   /* 16 pixels wide, 1 pixel tall */
	256,							   /* number of graphics patterns */
	2,								   /* 2 bits per pixel */
	{0, 4},							   /* bitplanes offset */
	{0, 0, 1, 1, 2, 2, 3, 3},		   /* x offsets to pixels */
	{0, 0, 0, 0, 0, 0, 0, 0},		   /* y offsets to lines */
	1								   /* number of bytes per char */
};

static struct GfxDecodeInfo amstrad_gfxdecodeinfo[] =
{
	/* memory region, start of graphics to decode, gfxlayout, color_codes_start, total_color_codes */
	{REGION_GFX1, 0, &amstrad_mode1_gfxlayout, 0, 32},
	{-1}
};

#endif

/* I've handled the I/O ports in this way, because the ports
are not fully decoded by the CPC h/w. Doing it this way means
I can decode it myself and a lot of  software should work */
static struct IOReadPort readport_amstrad[] =
{
	{0x0000, 0x0ffff, AmstradCPC_ReadPortHandler},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_amstrad[] =
{
	{0x0000, 0x0ffff, AmstradCPC_WritePortHandler},
	{-1}							   /* end of table */
};

/* read PSG port A */
int amstrad_psg_porta_read(int offset)
{
	/* read cpc keyboard */
	return AmstradCPC_ReadKeyboard();
}


static struct AY8910interface amstrad_ay_interface =
{
	1,								   /* 1 chips */
	1000000,						   /* 1.0 MHz  */
	{25, 25},
	{amstrad_psg_porta_read},
	{0},
	{0},
	{0}
};

#define KEYBOARD_PORTS \
        /* keyboard row 0 */ \
        PORT_START \
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor Up", KEYCODE_UP, IP_JOY_NONE) \
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor Right", KEYCODE_RIGHT, IP_JOY_NONE) \
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor Down", KEYCODE_DOWN, IP_JOY_NONE) \
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F9", KEYCODE_9_PAD, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6", KEYCODE_6_PAD, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3", KEYCODE_3_PAD, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "Small Enter", KEYCODE_ENTER_PAD, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F.", KEYCODE_DEL_PAD, IP_JOY_NONE) \
\
    /* keyboard line 1 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor Left", KEYCODE_LEFT, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "Copy", KEYCODE_LALT, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "F7", KEYCODE_7_PAD, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "F8", KEYCODE_8_PAD, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5", KEYCODE_5_PAD, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_1_PAD, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_2_PAD, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0", KEYCODE_0_PAD, IP_JOY_NONE) \
\
        /* keyboard row 2 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLR", KEYCODE_DEL, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_CLOSEBRACE, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_TILDE, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4", KEYCODE_4_PAD, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "SLASH", IP_KEY_NONE, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE) \
\
        /* keyboard row 3 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "^", KEYCODE_EQUALS, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_MINUS, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) \
\
        /* keyboard line 4 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) \
\
        /* keyboard line 5 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE) \
\
        /* keyboard line 6 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "6/JOYSTICK 1 UP", KEYCODE_6, IPT_JOYSTICK_UP) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "5/JOYSTICK 1 DOWN", KEYCODE_5, IPT_JOYSTICK_DOWN) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "R/JOYSTICK 1 LEFT", KEYCODE_R, IPT_JOYSTICK_LEFT) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "T/JOYSTICK 1 RIGHT", KEYCODE_T, IPT_JOYSTICK_RIGHT) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "G/JOYSTICK 1 FIRE 1", KEYCODE_G, IPT_BUTTON1) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "F/JOYSTICK 1 FIRE 2", KEYCODE_F, IPT_BUTTON2) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "B/JOYSTICK 1 FIRE 3", KEYCODE_B, IPT_BUTTON3) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) \
\
        /* keyboard line 7 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE) \
\
        /* keyboard line 8 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC", KEYCODE_ESC, IP_JOY_NONE) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE) \
\
        /* keyboard line 9 */ \
        PORT_START \
        PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 UP", IP_KEY_NONE, IPT_JOYSTICK_UP) \
        PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 DOWN", IP_KEY_NONE, IPT_JOYSTICK_DOWN) \
        PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 LEFT", IP_KEY_NONE, IPT_JOYSTICK_LEFT) \
        PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 RIGHT", IP_KEY_NONE, IPT_JOYSTICK_RIGHT) \
        PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 FIRE 1", IP_KEY_NONE, IPT_BUTTON1) \
        PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 FIRE 2", IP_KEY_NONE, IPT_BUTTON2) \
        PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 FIRE 3", IP_KEY_NONE, IPT_BUTTON3) \
        PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE) \



INPUT_PORTS_START(amstrad)

        KEYBOARD_PORTS

	/* the following are defined as dipswitches, but are in fact solder links on the
	 * curcuit board. The links are open or closed when the PCB is made, and are set depending on which country
	 * the Amstrad system was to go to */

	PORT_START
	PORT_BITX(0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Machine Name (bit 0)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0, DEF_STR( Off) )
	PORT_DIPSETTING(0x02, DEF_STR( On) )
	PORT_BITX(0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Machine Name (bit 1)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0, DEF_STR( Off) )
	PORT_DIPSETTING(0x04, DEF_STR( On) )
	PORT_BITX(0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Machine Name (bit 2)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0, DEF_STR( Off) )
	PORT_DIPSETTING(0x08, DEF_STR( On) )
	PORT_BITX(0x010, 0x010, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "TV Refresh Rate", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00, "60hz")
	PORT_DIPSETTING(0x010, "50hz")

INPUT_PORTS_END

INPUT_PORTS_START(kccomp)
        KEYBOARD_PORTS
INPUT_PORTS_END

/* actual clock to CPU is 4Mhz, but it is slowed by memory
accessess. A HALT is used every 4 CPU clock cycles. This gives
an effective speed of 3.8Mhz. */

static struct MachineDriver machine_driver_amstrad =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80 | CPU_16BIT_PORT,  /* type */
                        (AMSTRAD_US_PER_FRAME*AMSTRAD_T_STATES_PER_US*AMSTRAD_FPS),                                   /* clock: See Note Above */
			readmem_amstrad,		   /* MemoryReadAddress */
			writemem_amstrad,		   /* MemoryWriteAddress */
			readport_amstrad,		   /* IOReadPort */
			writeport_amstrad,		   /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
									    * Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
			amstrad_timer_interrupt, 300,	/* every scanline */
		},
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	amstrad_init_machine,			   /* init machine */
	amstrad_shutdown_machine,
	/* video hardware */
	AMSTRAD_SCREEN_WIDTH,			   /* screen width */
	AMSTRAD_SCREEN_HEIGHT,			   /* screen height */
	{0, (AMSTRAD_SCREEN_WIDTH - 1), 0, (AMSTRAD_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo,              *//* graphics
									    * decode info */
	32,								   /* total colours
									    */
	32,								   /* color table len */
	amstrad_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	amstrad_vh_start,
	amstrad_vh_stop,
	amstrad_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
			/* MachineSound */
		{
			SOUND_AY8910,
			&amstrad_ay_interface
		}
	}
};

static struct MachineDriver machine_driver_kccomp =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80 | CPU_16BIT_PORT,  /* type */
                        (AMSTRAD_US_PER_FRAME*AMSTRAD_T_STATES_PER_US*AMSTRAD_FPS),                                   /* clock: See Note Above */
			readmem_amstrad,		   /* MemoryReadAddress */
			writemem_amstrad,		   /* MemoryWriteAddress */
			readport_amstrad,		   /* IOReadPort */
			writeport_amstrad,		   /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
									    * Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
			amstrad_timer_interrupt, 300,	/* every scanline */
		},
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        kccomp_init_machine,                      /* init machine */
	amstrad_shutdown_machine,
	/* video hardware */
	AMSTRAD_SCREEN_WIDTH,			   /* screen width */
	AMSTRAD_SCREEN_HEIGHT,			   /* screen height */
	{0, (AMSTRAD_SCREEN_WIDTH - 1), 0, (AMSTRAD_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo,              *//* graphics
									    * decode info */
	32,								   /* total colours
									    */
	32,								   /* color table len */
	amstrad_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	amstrad_vh_start,
	amstrad_vh_stop,
	amstrad_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
			/* MachineSound */
		{
			SOUND_AY8910,
			&amstrad_ay_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* cpc6128.rom contains OS in first 16k, BASIC in second 16k */
/* cpcados.rom contains Amstrad DOS */

/* I am loading the roms outside of the Z80 memory area, because they
are banked. */
ROM_START(cpc6128)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc6128.rom", 0x10000, 0x8000, 0x9e827fe1)
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, 0x1fe22ecd)

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END

ROM_START(cpc464)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1)
	/* load the os to offset 0x01000 from memory base */
        ROM_LOAD("cpc464.rom", 0x10000, 0x8000, 0x040852f25)
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, 0x1fe22ecd)

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END

ROM_START(cpc664)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1)
	/* load the os to offset 0x01000 from memory base */
        ROM_LOAD("cpc664.rom", 0x10000, 0x8000, 0x09AB5A036)
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, 0x1fe22ecd)

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END


ROM_START(kccomp)
	ROM_REGION(0x01c000, REGION_CPU1)
	ROM_LOAD("kccos.rom", 0x10000, 0x04000, 0x7f9ab3f7)
	ROM_LOAD("kccbas.rom", 0x14000, 0x04000, 0xca6af63d)

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0c00, REGION_GFX1) */
ROM_END

static const struct IODevice io_cpc6128[] =
{
	{
		IO_CARTSLOT,				/* type */
		1,							/* count */
		"sna\0",					/* file extensions */
		NULL,						/* private */
		amstrad_snapshot_id,		/* id */
		amstrad_snapshot_load,		/* init */
		amstrad_snapshot_exit,		/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
		NULL,						/* status */
		NULL,						/* seek */
		NULL,						/* input */
		NULL,						/* output */
		NULL,						/* input_chunk */
		NULL						/* output_chunk */
	},
	{
		IO_FLOPPY,					/* type */
		2,							/* count */
		"dsk\0",					/* file extensions */
		NULL,						/* private */
		amstrad_floppy_id,			/* id */
		amstrad_floppy_load,		/* init */
		amstrad_floppy_exit,		/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
		NULL,						/* status */
		NULL,						/* seek */
		NULL,						/* input */
		NULL,						/* output */
		NULL,						/* input_chunk */
		NULL						/* output_chunk */
	},
	{IO_END}
};

#define io_kccomp io_cpc6128
#define io_cpc464 io_cpc6128
#define io_cpc664 io_cpc6128

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 1984, cpc464,   0,		amstrad,  amstrad,	0,	 "Amstrad plc", "Amstrad/Schneider CPC464")
COMP( 1985, cpc664,   0,		amstrad,  amstrad,	0,	 "Amstrad plc", "Amstrad/Schneider CPC664")
COMP( 1985, cpc6128,  0,		amstrad,  amstrad,	0,	 "Amstrad plc", "Amstrad/Schneider CPC6128")
COMP( 19??, kccomp,   cpc6128,	kccomp,   kccomp,	0,	 "VEB Mikroelektronik", "KC Compact")

