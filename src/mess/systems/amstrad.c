
#include "driver.h"
#include "mess/systems/i8255.h"
#include "mess/vidhrdw/hd6845s.h"
#include "mess/machine/amstrad.h"
#include "mess/vidhrdw/amstrad.h"

/* if we're doing it properly, then these are the correct values
for the full-area visible on a amstrad monitor. 50 CRTC chars in X,
35 CRTC chars in Y, with 8 lines per CRTC char */
#define AMSTRAD_SCREEN_WIDTH	(50*16)
#define AMSTRAD_SCREEN_HEIGHT	(35*8)

/* Machine name is "Amstrad" */
#define AMSTRAD_MACHINE_NAME ((0x07)<<1)
/* 50hz screen refresh */
#define AMSTRAD_SCREEN_REFRESH (1<<4)

/* pointers to each of the 16k banks in read/write mode */
static int AmstradCPC_SelectedKeyboardLine = 0;

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

unsigned char Amstrad_8255PortC;
unsigned char Amstrad_8255PortA;


/* Amstrad Sound Hardware */
int     amstrad_sh_control_port_r(int offset);
int     amstrad_sh_data_port_r(int offset);
void    amstrad_sh_control_port_w(int offset, int data);
void    amstrad_sh_data_port_w(int offset, int data);


static void HandlePSG(void)
{
        switch ((Amstrad_8255PortC>>6) & 0x03)
        {
                default:
                        break;

                // PSG Read selected
                case 1:
                {
                        i8255_set_porta(amstrad_sh_data_port_r(0));
                }
                break;

                // PSG Write data
                case 2:
                {
                      amstrad_sh_data_port_w(0, Amstrad_8255PortA);

                }
                break;

                // write index register
                case 3:
                {
                        amstrad_sh_control_port_w(0, Amstrad_8255PortA);
                }
                break;
          }
}



/* call this when a read from port a is done */
int	amstrad_ppi_porta_r(void)
{
        HandlePSG();

        return 0x0ff;
}

int	amstrad_ppi_portb_r(void)
{
        i8255_set_portb(AMSTRAD_MACHINE_NAME);

        return 0x0ff;
}

int	amstrad_ppi_portc_r(void)
{
        return 0x0ff;
}



void	amstrad_ppi_porta_w(int Data)
{
        Amstrad_8255PortA = Data;

        HandlePSG();
}

void	amstrad_ppi_portb_w(int Data)
{
}

void	amstrad_ppi_portc_w(int Data)
{
        Amstrad_8255PortC = Data;
        AmstradCPC_SelectedKeyboardLine = Data & 0x0f;

        HandlePSG();
}

static I8255_INTERFACE amstrad_i8255_interface=
{
	amstrad_ppi_porta_r,
	amstrad_ppi_portb_r,
	amstrad_ppi_portc_r,
	amstrad_ppi_porta_w,
	amstrad_ppi_portb_w,
	amstrad_ppi_portc_w
};


/* pointers to current ram configuration selected for banks */
static unsigned char * AmstradCPC_RamBanks[4];

/* base of all ram allocated - 128k */
unsigned char *Amstrad_Memory;

/* current selected upper rom */
static unsigned char *Amstrad_UpperRom;

/* bit 0,1 = mode, 2 = if zero, os rom is enabled, otherwise
disabled, 3 = if zero, upper rom is enabled, otherwise disabled */
unsigned char AmstradCPC_GA_RomConfiguration;

unsigned char AmstradCPC_PenColours[18];


/* 16 colours, + 1 for border */
unsigned short amstrad_colour_table[32]=
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,
	29,30,31
};

static int RamConfigurations[8*4]=
{
	0,1,2,3,	/* config 0 */
	0,1,2,7,	/* config 1 */
	4,5,6,7,	/* config 2 */
	0,3,2,7,	/* config 3 */
	0,4,2,3,	/* config 4 */
	0,5,2,3,	/* config 5 */
	0,6,2,3,	/* config 6 */
	0,7,2,3		/* config 7 */
};

/* ram configuration */
static unsigned char AmstradCPC_GA_RamConfiguration;
/* selected pen */
static unsigned char AmstradCPC_GA_PenSelected;



static unsigned char AmstradCPC_ReadKeyboard(void)
{
	if (AmstradCPC_SelectedKeyboardLine>9)
		return 0x0ff;

	return readinputport(AmstradCPC_SelectedKeyboardLine);
}


/* simplified ram configuration - e.g. only correct for 128k machines */
void	AmstradCPC_GA_SetRamConfiguration(void)
{
	int ConfigurationIndex = AmstradCPC_GA_RamConfiguration & 0x07;
	int BankIndex;
	unsigned char *BankAddr;

	BankIndex  = RamConfigurations[(ConfigurationIndex<<2)];
	BankAddr = Amstrad_Memory + (BankIndex<<14);

	AmstradCPC_RamBanks[0] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex<<2)+1];
	BankAddr = Amstrad_Memory + (BankIndex<<14);

	AmstradCPC_RamBanks[1] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex<<2)+2];
	BankAddr = Amstrad_Memory + (BankIndex<<14);

	AmstradCPC_RamBanks[2] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex<<2)+3];
	BankAddr = Amstrad_Memory + (BankIndex<<14);

	AmstradCPC_RamBanks[3] = BankAddr;
}

void	AmstradCPC_GA_Write(int Data)
{
#ifdef AMSTRAD_DEBUG
	printf("GA Write: %02x\r\n",Data);
#endif

	switch ((Data & 0x0c0)>>6)
	{
		case 0:
		{
			/* pen  selection */
			AmstradCPC_GA_PenSelected = Data;
		}
		return ;

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
				PenIndex = AmstradCPC_GA_PenSelected &0x0f;
			}

			AmstradCPC_PenColours[PenIndex] = Data & 0x01f;
		}
		return ;

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


	/* the following is used for banked memory read/writes and for setting up
	opcode and opcode argument reads */
	{
		unsigned char *BankBase;

		/* bank 0 - 0x0000..0x03fff */
		if ((AmstradCPC_GA_RomConfiguration & 0x04)==0)
		{
                        BankBase = &Machine->memory_region[0][0x010000];
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
		if ((AmstradCPC_GA_RomConfiguration & 0x08)==0)
		{
			BankBase = Amstrad_UpperRom;
		}
		else
		{
			BankBase = AmstradCPC_RamBanks[3];
		}
                cpu_setbank(4, BankBase);
          }


}

/* very simplified version of setting upper rom - since
we are not going to have additional roms, this is the best
way */
void	AmstradCPC_SetUpperRom(int Data)
{
	/* low byte of port holds the rom index */
	if ((Data & 0x0ff)==7)
	{
		/* select dos rom */
                Amstrad_UpperRom = &Machine->memory_region[0][0x018000];
	}
	else
	{
		/* select basic rom */
                Amstrad_UpperRom = &Machine->memory_region[0][0x014000];
	}
}

/* write Ram functions */
void	AmstradCPC_WriteMem0(int Offset, int Data)
{
	AmstradCPC_RamBanks[0][Offset & 0x03fff] = Data;
}

void	AmstradCPC_WriteMem1(int Offset, int Data)
{
	AmstradCPC_RamBanks[1][Offset & 0x03fff] = Data;
}

void	AmstradCPC_WriteMem2(int Offset, int Data)
{
	AmstradCPC_RamBanks[2][Offset & 0x03fff] = Data;
}

void	AmstradCPC_WriteMem3(int Offset, int Data)
{
	AmstradCPC_RamBanks[3][Offset & 0x03fff] = Data;
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

	if ((Offset & 0x04000)==0)
	{
		/* CRTC */
		unsigned int		Index;

		Index = (Offset & 0x0300)>>8;

		if (Index==3)
		{
			// CRTC Read register
			data = hd6845s_register_r();
		}
	}

	if ((Offset & 0x0800)==0)
	{
		/* 8255 PPI */

		unsigned int		Index;

		Index = (Offset & 0x0300)>>8;

		switch (Index)
		{
			case 0:
			{
				/* write port A */
				data = i8255_porta_r();
			}
			break;

			case 1:
			{
				/* write port B */
				data = i8255_portb_r();
				data = data | 1;
			}
			break;

			case 2:
			{
				/* write port C */
				data = i8255_portc_r();
			}
			break;

			default:
				break;
		}
	}

	if ((Offset & 0x0400)==0)
	{
		if ((Offset & 0x080)==0)
		{
			unsigned int		Index;

			Index = ((Offset & 0x0100)>>(8-1)) | (Offset & 0x01);

			switch (Index)
			{
				case 2:
				{
		//			data = FDC765A_statusreg_r();
				}
				break;

				case 3:
				{
		//			data = FDC765A_datareg_r();
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
void	AmstradCPC_WritePortHandler(int Offset, int Data)
{
#ifdef AMSTRAD_DEBUG
	printf("Write port Offs: %04x Data: %04x\r\n",Offset, Data);
#endif
	if ((Offset & 0x0c000)==0x04000)
	{
		/* GA */
		AmstradCPC_GA_Write(Data);
	}

	if ((Offset & 0x04000)==0)
	{
		/* CRTC */
		unsigned int		Index;

		Index = (Offset & 0x0300)>>8;

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

		//AmstradCPC_CRTC_Write[Index](Data);
	}

	if ((Offset & 0x02000)==0)
	{
		AmstradCPC_SetUpperRom(Data);
	}

	if ((Offset & 0x0800)==0)
	{
		unsigned int		Index;

		Index = (Offset & 0x0300)>>8;

		switch (Index)
		{
			case 0:
			{
				/* write port A */
				i8255_porta_w(Data);
			}
			break;

			case 1:
			{
				/* write port B */
				i8255_portb_w(Data);
			}
			break;

			case 2:
			{
				/* write port C */
				i8255_portc_w(Data);

			}
			break;

			case 3:
			{
				/* write control */
				i8255_control_port_w(Data);
			}
			break;
		}
	}
/*
	if ((Offset & 0x0400)==0)
	{
		if ((Offset & 0x080)==0)
		{
			unsigned int		Index;

			Index = ((Offset & 0x0100)>>(8-1)) | (Offset & 0x01);

			switch (Index)
			{
				case 0:
				{
//					FDD_SetMotor(Data);
				}
				break;

				case 3:
				{
//					FDC765A_datareg_w();
				}
				break;

				default:
					break;
			}
		}
	}
*/
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

int amstrad_timer_interrupt(void)
{
	return ignore_interrupt();
	//return 0;
}

int amstrad_frame_interrupt(void)
{
	return 0;
}
#if 0
int	amstrad_opbase_override(int pc)
{
	/* get memory address for opcode and opcode argument reads */
	OP_RAM = OP_ROM = (unsigned long)AmstradCPC_ReadBanks[(pc>>14)] - (unsigned long)(pc<<14));

	return -1;
}
#endif

/* sets up for a machine reset */
void	Amstrad_Reset(void)
{
	/* enable lower rom (OS rom) */
	AmstradCPC_GA_Write(0x089);

	/* set ram config 0 */
	AmstradCPC_GA_Write(0x0c0);

        i8255_init(&amstrad_i8255_interface);

	/* cpu opcode read and cpu opcode argument reads go via OP_RAM and OP_ROM.
	The following code sets these up depending on which memory bank we are reading from.
	The cpu opcode handling doesn't go through the banked mechanism! */
//	cpu_setOPbaseoverride(amstrad_opbase_override);
}



/* amstrad has 27 colours, 3 levels of R,G and B. The other colours
are copies of existing ones in the palette */

unsigned char	amstrad_palette[32*3]=
{
	0x080,0x080,0x080,	/* white */
	0x080,0x080,0x080,	/* white */
	0x000,0x0ff,0x080,	/* sea green */
	0x0ff,0x0ff,0x080,	/* pastel yellow */
	0x000,0x000,0x080,	/* blue */
	0x0ff,0x000,0x080,	/* purple */
	0x000,0x080,0x080,	/* cyan */
	0x0ff,0x080,0x080,	/* pink */
	0x0ff,0x000,0x080,	/* purple */
	0x0ff,0x0ff,0x080,	/* pastel yellow */
	0x0ff,0x0ff,0x000,	/* bright yellow */
	0x0ff,0x0ff,0x0ff,	/* bright white */
	0x0ff,0x000,0x000,	/* bright red */
	0x0ff,0x000,0x0ff,	/* bright magenta */
	0x0ff,0x080,0x000,	/* orange */
	0x0ff,0x080,0x0ff,	/* pastel magenta */
	0x000,0x000,0x080,	/* blue */
	0x000,0x0ff,0x080,	/* sea green */
	0x000,0x0ff,0x000,	/* bright green */
	0x000,0x0ff,0x0ff,	/* bright cyan */
	0x000,0x000,0x000,	/* black */
	0x000,0x000,0x0ff,	/* bright blue */
	0x000,0x080,0x000,	/* green */
	0x000,0x080,0x0ff,	/* sky blue */
	0x080,0x000,0x080,	/* magenta */
	0x080,0x0ff,0x080,	/* pastel green */
	0x080,0x0ff,0x080,	/* lime */
	0x080,0x0ff,0x0ff,	/* pastel cyan */
	0x080,0x000,0x000,	/* Red */
	0x080,0x000,0x0ff,	/* mauve */
	0x080,0x080,0x000,	/* yellow */
	0x080,0x080,0x0ff,	/* pastel blue */
};

/* Memory is banked in 16k blocks. The ROM can
be paged into bank 0 and bank 3. */
static struct MemoryReadAddress readmem_amstrad[] =
{
    { 0x00000, 0x03fff, MRA_BANK1},
        { 0x04000, 0x07fff, MRA_BANK2},
        { 0x08000, 0x0bfff, MRA_BANK3},
        { 0x0c000, 0x0ffff, MRA_BANK4},
        { 0x010000, 0x013fff, MRA_ROM}, /* OS */
        { 0x014000, 0x017fff, MRA_ROM}, /* BASIC */
        { 0x018000, 0x01bfff, MRA_ROM}, /* AMSDOS */
        { -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem_amstrad[] =
{
          { 0x00000, 0x03fff, AmstradCPC_WriteMem0 },
         { 0x04000, 0x07fff, AmstradCPC_WriteMem1 },
           {0x08000,0x0bfff, AmstradCPC_WriteMem2 },
           {0x0c000,0x0ffff, AmstradCPC_WriteMem3 },
        { -1 }  /* end of table */
};

/* I've handled the I/O ports in this way, because the ports
are not fully decoded by the CPC h/w. Doing it this way means
I can decode it myself and a lot of  software should work */
static struct IOReadPort readport_amstrad[] =
{
	{ 0x0000, 0x0ffff, AmstradCPC_ReadPortHandler},
	{ -1 } /* end of table */
};

static struct IOWritePort writeport_amstrad[] =
{
	{ 0x0000, 0x0ffff, AmstradCPC_WritePortHandler},
	{ -1 } /* end of table */
};

/* read PSG port A */
int	amstrad_psg_porta_read(int offset)
{
	/* read cpc keyboard */
	return AmstradCPC_ReadKeyboard();
}


static struct AY8910interface amstrad_ay_interface =
{
	1,	/* 1 chips */
	1000000,	/* 1.0 MHz  */
	{ 0x20ff},
	AY8910_DEFAULT_GAIN,/* ???????? */
	{ amstrad_psg_porta_read },
	{ 0 },
	{ 0 },
	{ 0 }
};

INPUT_PORTS_START(amstrad_input_ports)
	/* keyboard row 0 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "Cursor Up", KEYCODE_UP,IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "Cursor Right", KEYCODE_RIGHT,IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "Cursor Down", KEYCODE_DOWN,IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "F9", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "F6", KEYCODE_6_PAD,IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "F3", KEYCODE_3_PAD,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "Small Enter", KEYCODE_ENTER_PAD, IP_JOY_NONE)
	//PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "F.", KEYCODE_STOP_PAD, IP_JOY_NONE)
        	/* keyboard line 1 */
	PORT_START
	PORT_BITX(0x001,IP_ACTIVE_LOW, IPT_UNKNOWN, "Cursor Left", KEYCODE_LEFT,IP_JOY_NONE)
	//PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "Copy", KEYCODE_ALT,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "F7", KEYCODE_7_PAD,IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "F8", KEYCODE_8_PAD,IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "F5", KEYCODE_5_PAD,IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "F1", KEYCODE_1_PAD,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "F2", KEYCODE_2_PAD,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "F0", KEYCODE_0_PAD,IP_JOY_NONE)

	/* keyboard row 2 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "CLR", KEYCODE_DEL,IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "[", KEYCODE_CLOSEBRACE,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "RETURN", KEYCODE_ENTER,IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "]", KEYCODE_TILDE,IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "F4", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_LSHIFT,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "SLASH",IP_KEY_NONE,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN,"CTRL",KEYCODE_LCONTROL,IP_JOY_NONE)

	/* keyboard row 3 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "^",KEYCODE_EQUALS,IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "=",KEYCODE_MINUS,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN,"[",KEYCODE_OPENBRACE,IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "P",KEYCODE_P,IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN,";",KEYCODE_COLON,IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN,":",KEYCODE_QUOTE,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "/",KEYCODE_SLASH,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, ".",KEYCODE_STOP,IP_JOY_NONE)

	/* keyboard line 4 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "0",KEYCODE_0,IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "9",KEYCODE_9,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "O",KEYCODE_O,IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "I",KEYCODE_I,IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "L",KEYCODE_L,IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "K",KEYCODE_K,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "M",KEYCODE_M,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, ",",KEYCODE_COMMA, IP_JOY_NONE)


	/* keyboard line 5 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "8",KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "7",KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "U",KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "Y",KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "H",KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "J",KEYCODE_J,IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "N",KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "SPACE",KEYCODE_SPACE,IP_JOY_NONE)


	/* keyboard line 6 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "6",KEYCODE_6, IPT_JOYSTICK_UP)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "5",KEYCODE_5, IPT_JOYSTICK_DOWN)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "R",KEYCODE_R, IPT_JOYSTICK_LEFT)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "T",KEYCODE_T, IPT_JOYSTICK_RIGHT)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "G",KEYCODE_G, IPT_BUTTON1)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "F",KEYCODE_F, IPT_BUTTON2)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "B",KEYCODE_B, IPT_BUTTON3)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "V",KEYCODE_V, IP_JOY_NONE)

	/* keyboard line 7 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "4",KEYCODE_4,IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "3",KEYCODE_3,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "E",KEYCODE_E,IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "W",KEYCODE_W,IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "S",KEYCODE_S,IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "D",KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "C",KEYCODE_C,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "X",KEYCODE_X,IP_JOY_NONE)

	/* keyboard line 8 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "1",KEYCODE_1,IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "2",KEYCODE_2,IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "ESC",KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN, "Q",KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "TAB",KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "A",KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "CAPS LOCK",KEYCODE_CAPSLOCK,IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "Z",KEYCODE_Z, IP_JOY_NONE)

	/* keyboard line 9 */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_JOYSTICK_UP)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_JOYSTICK_DOWN)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_JOYSTICK_LEFT)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_UNKNOWN,"",IP_KEY_NONE, IPT_JOYSTICK_RIGHT)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_BUTTON1)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_BUTTON2)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_UNKNOWN, "",IP_KEY_NONE,IPT_BUTTON3)
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_UNKNOWN, "DEL",KEYCODE_BACKSPACE,IP_JOY_NONE)

INPUT_PORTS_END

/* actual clock to CPU is 4Mhz, but it is slowed by memory
accessess. A HALT is used every 4 CPU clock cycles. This gives
an effective speed of 3.8Mhz. */

static struct MachineDriver amstrad_machine_driver =
{
        /* basic machine hardware */
        {
			/* MachineCPU */
                {
                        CPU_Z80 | CPU_16BIT_PORT,	/* type */
                        3800000,    /* clock: See Note Above */
                        0,		/* memory region */
                        readmem_amstrad,	/* MemoryReadAddress */
				writemem_amstrad, /* MemoryWriteAddress */
                        readport_amstrad, /* IOReadPort */
				writeport_amstrad, /* IOWritePort */
                        amstrad_frame_interrupt, /* VBlank
Interrupt */
				1,				/* vblanks per frame */
                        amstrad_timer_interrupt, /* timer interrupt
*/
				300				/* timers
per second */
                },
        },
        50,						/* frames per second */
	  DEFAULT_60HZ_VBLANK_DURATION,       /* vblank duration */
        1,						/* cpu slices per frame */
        amstrad_init_machine,			/* init machine */
	NULL,
	/* video hardware */
        AMSTRAD_SCREEN_WIDTH,             /* screen width */
        AMSTRAD_SCREEN_HEIGHT,            /* screen height */
        { 0,(AMSTRAD_SCREEN_WIDTH-1),0,(AMSTRAD_SCREEN_HEIGHT-1)},   /* rectangle: visible_area */
        0, /*amstrad_gfxdecodeinfo*/                    /* graphics
decode info */
        32, 						/* total colours
*/
	  32,                                  	/* color table len */
        0,                                      /* convert color prom */

        VIDEO_TYPE_RASTER, /* video attributes */
        0,							/* MachineLayer */
        amstrad_vh_start,
        amstrad_vh_stop,
        amstrad_vh_screenrefresh,

        /* sound hardware */
        0,							/* sh init */
	  0,							/* sh start */
        0,							/* sh stop */
        0,							/* sh update */
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
ROM_START(amstrad_rom)
        /* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
        ROM_REGION(0x01c000)
        /* load the os to offset 0x01000 from memory base */
        ROM_LOAD("cpc6128.rom",0x10000,0x8000, 0x9e827fe1)
        ROM_LOAD("cpcados.rom",0x18000,0x4000, 0x1fe22ecd)
ROM_END

ROM_START(kccompact_rom)
        ROM_REGION(0x01c000)
        ROM_LOAD("kccos.rom", 0x10000,0x04000, 0x7f9ab3f7)
        ROM_LOAD("kccbas.rom",0x14000,0x04000, 0xca6af63d)
ROM_END

/* amstrad cpc 6128 game driver */
struct GameDriver amstrad_driver =
{
	__FILE__,
	0,
	"amstrad",
        "Amstrad CPC6128",			/* description */
	"1985",
	"Amstrad plc",
	"Kevin Thacker [MESS driver]", /* credits */
    GAME_COMPUTER,
	&amstrad_machine_driver,		/* MachineDriver */
	0,
        amstrad_rom,
        amstrad_rom_load,
	amstrad_rom_id,         /* load rom_file images */
	0,                      /* number of ROM slots */
	2,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	0,                      /* number of cassette drives supported */
        0,                      /* rom decoder */
        0,                      /* opcode decoder */
        0,                      /* pointer to sample names */
        0,                      /* sound_prom */

        amstrad_input_ports,

        0,                      /* color_prom */
        amstrad_palette,          /* color palette */
        amstrad_colour_table,       /* color lookup table */

        ORIENTATION_DEFAULT,    /* orientation */

        0,                      /* hiscore load */
        0                       /* hiscore save */
};

/* amstrad cpc 6128 game driver */
struct GameDriver kccomp_driver =
{
	__FILE__,
	0,
	"kccomp",
        "KC Compact",			/* description */
        "19??",
        "Veb Mikroelektronik <<wilhelm pieck>> mulhausen",
	"Kevin Thacker [MESS driver]", /* credits */
    GAME_COMPUTER,
	&amstrad_machine_driver,		/* MachineDriver */
	0,
        kccompact_rom,
        amstrad_rom_load,
	amstrad_rom_id,         /* load rom_file images */
	0,                      /* number of ROM slots */
	2,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	0,                      /* number of cassette drives supported */
        0,                      /* rom decoder */
        0,                      /* opcode decoder */
        0,                      /* pointer to sample names */
        0,                      /* sound_prom */

        amstrad_input_ports,

        0,                      /* color_prom */
        amstrad_palette,          /* color palette */
        amstrad_colour_table,       /* color lookup table */

        ORIENTATION_DEFAULT,    /* orientation */

        0,                      /* hiscore load */
        0                       /* hiscore save */
};
