/******************************************************************************

	amstrad.c
	system driver

		Amstrad Hardware:
			- 8255 connected to AY-3-8912 sound generator,
				keyboard, cassette, printer, crtc vsync output,
				and some PCB links
			- 6845 (either HD6845S, UM6845R or M6845) crtc graphics display
			  controller
			- NEC765 floppy disc controller (CPC664,CPC6128)
			- Z80 CPU running at 4Mhz (slowed by wait states on memory
			  access)
			- custom ASIC "Gate Array" controlling rom paging, ram paging,
				current display mode and colour palette

	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"

#include "includes/centroni.h"
#include "machine/8255ppi.h"	/* for 8255 ppi */
#include "cpu/z80/z80.h"		/* for cycle tables */
#include "vidhrdw/m6845.h"		/* CRTC display */
#include "includes/amstrad.h"
#include "includes/nec765.h"	/* for floppy disc controller */
#include "devices/dsk.h"		/* for CPCEMU style disk images */
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "devices/cassette.h"

#ifdef AMSTRAD_VIDEO_EVENT_LIST
/* for event list */
#include "eventlst.h"
#endif

//#define AMSTRAD_DEBUG

/* the hardware allows selection of 256 ROMs. Rom 0 is usually BASIC and Rom 7 is AMSDOS */
/* With the CPC hardware, if a expansion ROM is not connected, BASIC rom will be selected instead */
static unsigned char *Amstrad_ROM_Table[256];


/*-------------------------------------------*/
/* MULTIFACE */
static void multiface_rethink_memory(void);
static WRITE_HANDLER(multiface_io_write);
static void multiface_init(void);
static void multiface_stop(void);
static int multiface_hardware_enabled(void);
static void multiface_reset(void);

/*-------------------------------------------*/
static void amstrad_clear_top_bit_of_int_counter(void);


/* cycles at end of last frame */
static unsigned long amstrad_cycles_at_frame_end = 0;
/* cycle count of last write */
static unsigned long amstrad_cycles_last_write = 0;
static unsigned long time_delta_fraction = 0;

static void amstrad_update_video(void)
{
	int current_time;
	int time_delta;

	/* current cycles */
	current_time = TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()) + amstrad_cycles_at_frame_end;
	/* time between last write and this write */
	time_delta = current_time - amstrad_cycles_last_write + time_delta_fraction;
	/* The timing used to be spot on, but now it can give odd cycles, hopefully
	this will compensate for that! */
	time_delta_fraction = time_delta & 0x03;
	time_delta = time_delta>>2;

	/* set new previous write */
	amstrad_cycles_last_write = current_time;

	if (time_delta!=0)
	{
		amstrad_vh_execute_crtc_cycles(time_delta);
	}
}

static VIDEO_EOF( amstrad )
{
	if ((readinputport(11) & 0x02)!=0)
	{
			multiface_stop();
	}
#ifndef AMSTRAD_VIDEO_EVENT_LIST
	amstrad_update_video();
	// update cycle count
	amstrad_cycles_at_frame_end += TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod());
#endif
}

/* psg access operation:
0 = inactive, 1 = read register data, 2 = write register data,
3 = write register index */
static int amstrad_psg_operation;

/* data present on input of ppi, and data written to ppi output */
static int ppi_port_inputs[3];
static int ppi_port_outputs[3];

/* keyboard line 0-9 */
static int amstrad_keyboard_line;
/*static int crtc_vsync_output;*/

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
static READ_HANDLER ( amstrad_ppi_porta_r )
{
	update_psg();
	
	return ppi_port_inputs[0];
}


/* 
Amstrad hardware:


8255 PPI port A (connected to AY-3-8912 databus).

8255 PPI port B input:
 Bit 7 = Cassette tape input
 bit 6 = printer busy/online
 bit 5 = /exp signal on expansion port of CPC
 bit 4 = 50/60hz (link on PCB. For this MESS driver I have used the dipswitch feature)
 bit 3,2,1 = PCB links to define manufacturer name. For this MESS driver I have used the dipswitch feature.
 Bit 0 = VSYNC from 6845 CRTC 
 
Manufacturer names are: Isp, Triumph, Saisho, Solavox, Awa, Schneider, Orion, Amstrad.
On real CPC name is set using a link on the PCB.

8255 PPI port C output:
 bit 7,6 = AY-3-8912 PSG operation
 bit 5 = cassette write data
 bit 4 = Cassette motor control
 bit 3-0 =	keyboard line

I/O Port decoding:

  Bit 15 = 0, Bit 14 = 1: Gate Array (W)
  Bit 15 = 0, RAM management PAL
  Bit 14 = 0: 6845 CRTC (R/W)
  Bit 13 = 0: Select upper rom (W)
  Bit 12 = 0: Printer (W)
  Bit 11 = 0: 8255 PPI (R/W)
  Bit 10 = 0: Expansion.

  Bit 10 = 0, Bit 7 = 0: uPD 765A FDC

Gate Array:

bit 7, bit 6 = function 
	00 = pen select/clut entry index, 
	01 = set pen colour/clut entry colour, 
	10 = display mode, upper/lower rom enable/disable, interrupt control


function 00:
	bit 4..0 = pen index/clut entry index

	if bit 4 = 1, border is selected, bits 3..0 is ignored
	if bit 4 = 0, bit 3..0 define pen index

function 01:
	bit 4..0 = hardware colour id

function 10:
	bit 4 = reset top-bit of interrupt counter
	bit 3 = 0 = upper rom is enabled, visible in &c000-&ffff range
	bit 2 = 0 = lower rom is enabled, visible in &0000-&3fff range
	bit 1,0 = display mode

Ram Expansion/PAL in CPC6128 (accessed at same I/O address' as Gate Array)

function 11:
	(actually part of ram expansion or PAL in CPC6128)
	bit 3..0 define ram configuration code.
	bit 6..4 define 64k block to use
*/


static READ_HANDLER (amstrad_ppi_portb_r)
{
	int data;

#ifndef AMSTRAD_VIDEO_EVENT_LIST
		amstrad_update_video();
#endif
	data = 0x0;

	/* cassette read */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.03)
		data |=0x080;

	/* printer busy */
	if (device_status(image_from_devtype_and_index(IO_PRINTER, 0), 0)==0 )
		data |=0x040;

	/* vsync state from CRTC */
	data |= amstrad_vsync;

	/* manufacturer name and 50hz/60hz state, defined by links on PCB */
	data |= ppi_port_inputs[1] & 0x01e;

	return data;
}

static WRITE_HANDLER ( amstrad_ppi_porta_w )
{
	ppi_port_outputs[0] = data;

	update_psg();
}


/* previous value */
static int previous_ppi_portc_w;

static WRITE_HANDLER ( amstrad_ppi_portc_w )
{
	int changed_data;

	previous_ppi_portc_w = ppi_port_outputs[2];
	ppi_port_outputs[2] = data;

	changed_data = previous_ppi_portc_w^data;

	/* cassette motor changed state */
	if ((changed_data & (1<<4))!=0)
	{
		/* cassette motor control */
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
			CASSETTE_MASK_MOTOR);
	}

	/* cassette write data changed state */
	if ((changed_data & (1<<5))!=0)
	{
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x20) ? -1.0 : +1.0);
	}

	/* psg operation */
	amstrad_psg_operation = (data >> 6) & 0x03;
	/* keyboard line */
	amstrad_keyboard_line = (data & 0x0f);

	update_psg();
}

static ppi8255_interface amstrad_ppi8255_interface =
{
	1,
	{amstrad_ppi_porta_r},
	{amstrad_ppi_portb_r},
	{NULL},
	{amstrad_ppi_porta_w},
	{NULL},
	{amstrad_ppi_portc_w}
};

/* Amstrad NEC765 interface doesn't use interrupts or DMA! */
static nec765_interface amstrad_nec765_interface =
{
	NULL,
	NULL
};

/* pointers to current ram configuration selected for banks */
static unsigned char *AmstradCPC_RamBanks[4];

/* current selected upper rom */
static unsigned char *Amstrad_UpperRom;

/* bit 0,1 = mode, 2 = if zero, os rom is enabled, otherwise
disabled, 3 = if zero, upper rom is enabled, otherwise disabled */
unsigned char AmstradCPC_GA_RomConfiguration;

static short AmstradCPC_PenColours[18];

static int RamConfigurations[8 * 4] =
{
	0, 1, 2, 3, 					   /* config 0 */
	0, 1, 2, 7, 					   /* config 1 */
	4, 5, 6, 7, 					   /* config 2 */
	0, 3, 2, 7, 					   /* config 3 */
	0, 4, 2, 3, 					   /* config 4 */
	0, 5, 2, 3, 					   /* config 5 */
	0, 6, 2, 3, 					   /* config 6 */
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
		cpu_setbank(2, BankBase+0x02000);


		/* bank 1 - 0x04000..0x07fff */
		cpu_setbank(3, AmstradCPC_RamBanks[1]);
		cpu_setbank(4, AmstradCPC_RamBanks[1]+0x02000);

		/* bank 2 - 0x08000..0x0bfff */
		cpu_setbank(5, AmstradCPC_RamBanks[2]);
		cpu_setbank(6, AmstradCPC_RamBanks[2]+0x02000);

		/* bank 3 - 0x0c000..0x0ffff */
		if ((AmstradCPC_GA_RomConfiguration & 0x08) == 0)
		{
			BankBase = Amstrad_UpperRom;
		}
		else
		{
			BankBase = AmstradCPC_RamBanks[3];
		}
		cpu_setbank(7, BankBase);
		cpu_setbank(8, BankBase+0x02000);

		cpu_setbank(9, AmstradCPC_RamBanks[0]);
		cpu_setbank(10, AmstradCPC_RamBanks[0]+0x02000);
		cpu_setbank(11, AmstradCPC_RamBanks[1]);
		cpu_setbank(12, AmstradCPC_RamBanks[1]+0x02000);
		cpu_setbank(13, AmstradCPC_RamBanks[2]);
		cpu_setbank(14, AmstradCPC_RamBanks[2]+0x02000);
		cpu_setbank(15, AmstradCPC_RamBanks[3]);
		cpu_setbank(16, AmstradCPC_RamBanks[3]+0x02000);

		/* multiface hardware enabled? */
		if (multiface_hardware_enabled())
		{
			multiface_rethink_memory();
		}
	}
}



/* simplified ram configuration - e.g. only correct for 128k machines */
static void AmstradCPC_GA_SetRamConfiguration(void)
{
	int ConfigurationIndex = AmstradCPC_GA_RamConfiguration & 0x07;
	int BankIndex;
	unsigned char *BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2)];
	BankAddr = mess_ram + (BankIndex << 14);

	AmstradCPC_RamBanks[0] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 1];
	BankAddr = mess_ram + (BankIndex << 14);

	AmstradCPC_RamBanks[1] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 2];
	BankAddr = mess_ram + (BankIndex << 14);

	AmstradCPC_RamBanks[2] = BankAddr;

	BankIndex = RamConfigurations[(ConfigurationIndex << 2) + 3];
	BankAddr = mess_ram + (BankIndex << 14);

	AmstradCPC_RamBanks[3] = BankAddr;
}

 

void AmstradCPC_GA_Write(int Data)
{
	switch ((Data & 0x0c0) >> 6)
	{
		case 0:
		{
			/* pen	selection */
			AmstradCPC_GA_PenSelected = Data;
		}
		return;

	case 1:
		{
			int PreviousColour;
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


			PreviousColour = AmstradCPC_PenColours[PenIndex];

			AmstradCPC_PenColours[PenIndex] = Data & 0x01f;

			/* colour changed? */
			if (PreviousColour!=AmstradCPC_PenColours[PenIndex])
			{

#ifdef AMSTRAD_VIDEO_EVENT_LIST
			   EventList_AddItemOffset((EVENT_LIST_CODE_GA_COLOUR<<6) | PenIndex, AmstradCPC_PenColours[PenIndex], TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#else
			   amstrad_update_video();
			   amstrad_vh_update_colour(PenIndex, AmstradCPC_PenColours[PenIndex]);
#endif
			}

		}
		return;

	case 2:
		{
			int Previous_GA_RomConfiguration = AmstradCPC_GA_RomConfiguration;

			AmstradCPC_GA_RomConfiguration = Data;

			/* if bit is set, clear top bit of interrupt counter */
			if ((Data & (1<<4))!=0)
			{
#ifndef AMSTRAD_VIDEO_EVENT_LIST
				amstrad_update_video();
#endif
				amstrad_clear_top_bit_of_int_counter();
			}

			/* mode change? */
			if (((Data^Previous_GA_RomConfiguration) & 0x03)!=0)
			{
#ifdef AMSTRAD_VIDEO_EVENT_LIST
			EventList_AddItemOffset((EVENT_LIST_CODE_GA_MODE<<6) , Data & 0x03, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#else
				amstrad_update_video();
				amstrad_vh_update_mode(Data & 0x03);
#endif
			}
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

void AmstradCPC_PALWrite(int data)
{
	if ((data & 0x0c0)==0x0c0)
	{

		AmstradCPC_GA_RamConfiguration = data;

		AmstradCPC_GA_SetRamConfiguration();

		Amstrad_RethinkMemory();
	}
}

void AmstradCPC_SetUpperRom(int Data)
{
	Amstrad_UpperRom = Amstrad_ROM_Table[Data & 0x0ff];

	logerror("upper rom %02x\n",Data);


	Amstrad_RethinkMemory();
}



/* port handler */
static READ_HANDLER ( AmstradCPC_ReadPortHandler )
{
	unsigned char data = 0x0ff;

	if ((offset & 0x04000) == 0)
	{
		/* CRTC */
		unsigned int Index;

		Index = (offset & 0x0300) >> 8;

		if (Index == 3)
		{
#ifndef AMSTRAD_VIDEO_EVENT_LIST
			amstrad_update_video();
#endif
			/* CRTC Read register */
			data =	crtc6845_register_r(0);
		}
	}

	if ((offset & 0x0800) == 0)
	{
		/* 8255 PPI */

		unsigned int Index;

		Index = (offset & 0x0300) >> 8;

		data = ppi8255_r(0, Index);
	}

	if ((offset & 0x0400) == 0)
	{
		if ((offset & 0x080) == 0)
		{
			unsigned int Index;

			Index = ((offset & 0x0100) >> (8 - 1)) | (offset & 0x01);

			switch (Index)
			{
			case 2:
				{
					/* read status */
					data = nec765_status_r(0);
				}
				break;

			case 3:
				{
					/* read data register */
					data = nec765_data_r(0);
				}
				break;

			default:
				break;
			}
		}
	}


	return data;

}

static unsigned char previous_printer_data_byte;

/* Offset handler for write */
static WRITE_HANDLER ( AmstradCPC_WritePortHandler )
{
	if ((offset & 0x0c000) == 0x04000)
	{
		/* GA */
		AmstradCPC_GA_Write(data);
	}

	if ((offset & 0x0c000) == 0x00000)
	{
		/* GA */
		AmstradCPC_PALWrite(data);
	}

	if ((offset & 0x04000) == 0)
	{
		/* CRTC */
		unsigned int Index;

		Index = (offset & 0x0300) >> 8;

		switch (Index)
		{
		case 0:
			{
#ifdef AMSTRAD_VIDEO_EVENT_LIST
				EventList_AddItemOffset((EVENT_LIST_CODE_CRTC_INDEX_WRITE<<6), data, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
#endif

				///* register select */
								crtc6845_address_w(0,data);
			}
			break;

		case 1:
			{
#if 0
				int current_time;
								int time_delta;
								int cur_time;

								/* current time */
								current_time = TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod());
								cur_time = current_time;

								if (previous_crtc_write_time>current_time)
								{
									cur_time += cpu_getfperiod();
								}

								/* time between last write and this write */
								time_delta = (cur_time - previous_crtc_write_time)>>2;

								/* set new previous write */
								previous_crtc_write_time = current_time;

#ifdef AMSTRAD_VIDEO_EVENT_LIST
				/* crtc register write */
				{

					EventList_AddItemOffset((EVENT_LIST_CODE_CRTC_WRITE<<6), data, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
				}
#endif
								/* recalc time */
								crtc6845_recalc(0, time_delta);

#else
								  amstrad_update_video();
#endif

							  /* write data */
							  crtc6845_register_w(0,data);

			}
			break;

		default:
			break;
		}
	}


	if ((offset & 0x01000)==0)
	{
		/* on CPC, write to printer through LS chip */
		/* the amstrad is crippled with a 7-bit port :( */
		/* bit 7 of the data is the printer /strobe */

		/* strobe state changed? */
		if (((previous_printer_data_byte^data) & 0x080)!=0)
		{
			/* check for only one transition */
			if ((data & 0x080)==0)
			{
				/* output data to printer */
				device_output(image_from_devtype_and_index(IO_PRINTER, 0), data & 0x07f);
			}
		}
		previous_printer_data_byte = data;
	}

	if ((offset & 0x02000) == 0)
	{
		AmstradCPC_SetUpperRom(data);
	}

	if ((offset & 0x0800) == 0)
	{
		unsigned int Index;

		Index = (offset & 0x0300) >> 8;

		ppi8255_w(0, Index, data);
	}

	if ((offset & 0x0400) == 0)
	{
		if ((offset & 0x080) == 0)
		{
			unsigned int Index;

			Index = ((offset & 0x0100) >> (8 - 1)) | (offset & 0x01);

			switch (Index)
			{
			case 0:
				{
					/* fdc motor on */
					floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), data & 0x01);
					floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), data & 0x01);
					floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
					floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
				}
				break;

			case 3:
				{
					nec765_data_w(0,data);
				}
				break;

			default:
				break;
			}
		}
	}

	multiface_io_write(offset,data);
}

/******************************************************************************************
	Multiface emulation
  ****************************************************************************************/



static unsigned char *multiface_ram;
static unsigned long multiface_flags;

/* stop button has been pressed */
#define MULTIFACE_STOP_BUTTON_PRESSED	0x0001
/* ram/rom is paged into memory space */
#define MULTIFACE_RAM_ROM_ENABLED		0x0002
/* when visible OUT commands are performed! */
#define MULTIFACE_VISIBLE				0x0004

/* multiface traps calls to 0x0065 when it is active.
This address has a RET and so executes no code.

It is believed that it is used to make multiface invisible to programs */

/*#define MULTIFACE_0065_TOGGLE 				  0x0008*/


/* used to setup computer if a snapshot was specified */
static OPBASE_HANDLER( amstrad_multiface_opbaseoverride )
{
		int pc;

		pc = activecpu_get_pc();

		/* there are two places where CALL &0065 can be found
		in the multiface rom. At this address there is a RET.

		To disable the multiface from being detected, the multiface
		stop button must be pressed, then the program that was stopped
		must be returned to. When this is done, the multiface cannot
		be detected and the out operations to page the multiface
		ram/rom into the address space will not work! */

		/* I assume that the hardware in the multiface detects
		the PC set to 0x065 and uses this to enable/disable the multiface
		*/

		/* I also use this to allow the stop button to be pressed again */
		if (pc==0x0164)
		{
			/* first call? */
			multiface_flags |= MULTIFACE_VISIBLE;
		}
		else if (pc==0x0c98)
		{
		  /* second call */

		  /* no longer visible */
		  multiface_flags &= ~(MULTIFACE_VISIBLE|MULTIFACE_STOP_BUTTON_PRESSED);

		 /* clear op base override */
				memory_set_opbase_handler(0,0);
		}

		return pc;
}

void multiface_init(void)
{
	/* after a reset the multiface is visible */
	multiface_flags = MULTIFACE_VISIBLE;

	/* allocate ram */
	multiface_ram = (unsigned char *)auto_malloc(8192);
}

/* call when a system reset is done */
void multiface_reset(void)
{
		/* stop button not pressed and ram/rom disabled */
		multiface_flags &= ~(MULTIFACE_STOP_BUTTON_PRESSED |
						MULTIFACE_RAM_ROM_ENABLED);
		/* as on the real hardware the multiface is visible after
		a reset! */
		multiface_flags |= MULTIFACE_VISIBLE;
}

int multiface_hardware_enabled(void)
{
		if (multiface_ram!=NULL)
		{
				if ((readinputport(11) & 0x01)!=0)
				{
						return 1;
				}
		}

		return 0;
}

/* simulate the stop button has been pressed */
void	multiface_stop(void)
{
	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

	/* if stop button not already pressed, do press action */
	/* pressing stop button while multiface is running has no effect */
	if ((multiface_flags & MULTIFACE_STOP_BUTTON_PRESSED)==0)
	{
		/* initialise 0065 toggle */
				/*multiface_flags &= ~MULTIFACE_0065_TOGGLE;*/

		multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;

		/* stop button has been pressed, furthur pressess will not issue a NMI */
		multiface_flags |= MULTIFACE_STOP_BUTTON_PRESSED;

		AmstradCPC_GA_RomConfiguration &=~0x04;

		/* page rom into memory */
		multiface_rethink_memory();

		/* pulse the nmi line */
		cpu_set_nmi_line(0, PULSE_LINE);

		/* initialise 0065 override to monitor calls to 0065 */
		memory_set_opbase_handler(0,amstrad_multiface_opbaseoverride);
	}

}

static void multiface_rethink_memory(void)
{
		unsigned char *multiface_rom;

	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

		multiface_rom = &memory_region(REGION_CPU1)[0x01C000];

	if (
		((multiface_flags & MULTIFACE_RAM_ROM_ENABLED)!=0) &&
		((AmstradCPC_GA_RomConfiguration & 0x04) == 0)
		)
	{

		/* set bank addressess */
		cpu_setbank(1, multiface_rom);
		cpu_setbank(2, multiface_ram);
		cpu_setbank(9, multiface_rom);
		cpu_setbank(10, multiface_ram);
	}
}

/* any io writes are passed through here */
static WRITE_HANDLER(multiface_io_write)
{
	/* multiface hardware enabled? */
		if (!multiface_hardware_enabled())
		return;

		/* visible? */
	if (multiface_flags & MULTIFACE_VISIBLE)
	{
		if (offset==0x0fee8)
		{
			multiface_flags |= MULTIFACE_RAM_ROM_ENABLED;
			Amstrad_RethinkMemory();
		}

		if (offset==0x0feea)
		{
			multiface_flags &= ~MULTIFACE_RAM_ROM_ENABLED;
			Amstrad_RethinkMemory();
		}
	}

	/* update multiface ram with data */
		/* these are decoded fully! */
		switch ((offset>>8) & 0x0ff)
		{
				/* gate array */
				case 0x07f:
				{
						switch (data & 0x0c0)
						{
								/* pen index */
								case 0x00:
								{
									multiface_ram[0x01fcf] = data;

								}
								break;

								/* pen colour */
								case 0x040:
								{
									int pen_index;

									pen_index = multiface_ram[0x01fcf] & 0x0f;

									if (multiface_ram[0x01fcf] & 0x010)
									{

										multiface_ram[0x01fdf + pen_index] = data;
									}
									else
									{
										multiface_ram[0x01f90 + pen_index] = data & 0x01f;
									}

								}
								break;

								/* rom/mode selection */
								case 0x080:
								{

									multiface_ram[0x01fef] = data;

								}
								break;

								/* ram configuration */
								case 0x0c0:
								{

									multiface_ram[0x01fff] = data;

								}
								break;

								default:
								  break;

						}

				}
				break;


				/* crtc register index */
				case 0x0bc:
				{
						multiface_ram[0x01cff] = data;
				}
				break;

				/* crtc register write */
				case 0x0bd:
				{
						int reg_index;

						reg_index = multiface_ram[0x01cff] & 0x0f;

						multiface_ram[0x01db0 + reg_index] = data;
				}
				break;


				/* 8255 ppi control */
				case 0x0f7:
				{
				  multiface_ram[0x017ff] = data;

				}
				break;

				/* rom select */
				case 0x0df:
				{
				   multiface_ram[0x01aac] = data;
				}
				break;

				default:
				   break;

		 }

}


/*

  Interrupt system:

  - The gate array counts CRTC HSYNC pulses.
	(It has a internal 6-bit counter).

  - When the counter in the gate array reaches 52, a interrupt is generated.
	The counter is reset to zero and the count starts again.

  - The interrupt is held until the Z80 acknowledges it.

  - When the interrupt is acknowledge, and is detected by the Gate Array,
  the top bit of this counter is reset to 0.
  This prevents the next interrupt from occuring closer than 32 lines.

  - The counter is also reset, 2 HSYNCS after the VSYNC has begun.
	This ensures it is synchronised.
*/

/* int counter */
static int amstrad_52_divider;
/* reset int counter 2 scans into vsync */
static int amstrad_52_divider_vsync_reset;

void amstrad_interrupt_timer_trigger_reset_by_vsync(void)
{
	amstrad_52_divider_vsync_reset = 2;
}

void amstrad_interrupt_timer_update(void)
{
	/* update counter */
	amstrad_52_divider++;

	/* vsync synchronisation active? */
	if (amstrad_52_divider_vsync_reset!=0)
	{
		amstrad_52_divider_vsync_reset--;

		if (amstrad_52_divider_vsync_reset==0)
		{
			/* if counter is <32 or equal to 52 trigger an int. If it isn't then
				the previous int is closer than 32 scan-lines to this
			position */
			if (((amstrad_52_divider & (1<<5))==0) || (amstrad_52_divider==52))
			{

				cpu_set_irq_line(0,0, HOLD_LINE);
			}

			/* reset counter */
			amstrad_52_divider = 0;
			return;
		}
	}

	/* counter==52? */
	if (amstrad_52_divider == 52)
	{
		/* reset and trigger int */
		amstrad_52_divider = 0;
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
}

static void	amstrad_interrupt_timer_callback(int dummy)
{
#ifndef AMSTRAD_VIDEO_EVENT_LIST
	amstrad_update_video();
#else
	amstrad_interrupt_timer_update();
#endif
}

static void amstrad_clear_top_bit_of_int_counter(void)
{
	/* clear bit 5 of counter - next int will not be closer than
	32 lines */
	amstrad_52_divider &=31;
}

/* called when cpu acknowledges int */
static int 	amstrad_cpu_acknowledge_int(int cpu)
{
	amstrad_clear_top_bit_of_int_counter();

	cpu_set_irq_line(0,0, CLEAR_LINE);

	return 0x0ff;
}

#define US_TO_CPU_CYCLES(x) ((x)<<2)

/* the following timings have been measured! */
static UINT8 amstrad_cycle_table_op[256]=
{
	US_TO_CPU_CYCLES(1),	/* NOP */
	US_TO_CPU_CYCLES(3),	/* LD BC,nnnn */
	US_TO_CPU_CYCLES(2),	/* LD (BC),A */
	US_TO_CPU_CYCLES(2),	/* INC BC */
	US_TO_CPU_CYCLES(1),	/* INC B */
	US_TO_CPU_CYCLES(1),	/* DEC B */
	US_TO_CPU_CYCLES(2),	/* LD B,n */
	US_TO_CPU_CYCLES(1),	/* RLCA */
	US_TO_CPU_CYCLES(1),	/* EX AF,AF' */
	US_TO_CPU_CYCLES(3),	/* ADD HL,BC */
	US_TO_CPU_CYCLES(2),	/* LD A,(BC) */
	US_TO_CPU_CYCLES(2),	/* DEC BC */
	US_TO_CPU_CYCLES(1),	/* INC C */
	US_TO_CPU_CYCLES(1),	/* DEC C */
	US_TO_CPU_CYCLES(2),	/* LD C,n */
	US_TO_CPU_CYCLES(1),	/* RRCA */
		US_TO_CPU_CYCLES(3),	/*		DJNZ  4 taken, 3 not taken */
	US_TO_CPU_CYCLES(3),	/* LD DE,nnnn */
	US_TO_CPU_CYCLES(2),	/* LD (DE),A */
	US_TO_CPU_CYCLES(2),	/* INC DE */
	US_TO_CPU_CYCLES(1),	/* INC D */
	US_TO_CPU_CYCLES(1),	/* DEC D */
	US_TO_CPU_CYCLES(2),	/* LD D,n */
	US_TO_CPU_CYCLES(1),	/* RLA */
	US_TO_CPU_CYCLES(3),	/* JR */
	US_TO_CPU_CYCLES(3),	/* ADD HL,DE */
	US_TO_CPU_CYCLES(2),	/* LD A,(DE) */
	US_TO_CPU_CYCLES(2),	/* DEC DE */
	US_TO_CPU_CYCLES(1),	/* INC E */
	US_TO_CPU_CYCLES(1),	/* DEC E */
	US_TO_CPU_CYCLES(2),	/* LD E,n */
	US_TO_CPU_CYCLES(1),	/* RRA */
		US_TO_CPU_CYCLES(2),	/* JR 3 if taken, 2 if not taken */
	US_TO_CPU_CYCLES(3),	/* LD HL,nnnn */
	US_TO_CPU_CYCLES(5),	/* LD (nnnn),HL */
	US_TO_CPU_CYCLES(2),	/* INC HL */
	US_TO_CPU_CYCLES(1),	/* INC H */
	US_TO_CPU_CYCLES(1),	/* DEC H */
	US_TO_CPU_CYCLES(2),	/* LD H,n */
	US_TO_CPU_CYCLES(1),	/* DAA */
		US_TO_CPU_CYCLES(2),	/*		 JR Z,3 if taken, 2 if not taken */
	US_TO_CPU_CYCLES(3),	/* ADD HL,HL */
	US_TO_CPU_CYCLES(5),	/* LD HL,(nnnn) */
	US_TO_CPU_CYCLES(2),	/* DEC HL */
	US_TO_CPU_CYCLES(1),	/* INC H */
	US_TO_CPU_CYCLES(1),	/* DEC H */
	US_TO_CPU_CYCLES(2),	/* LD H,n */
	US_TO_CPU_CYCLES(1),	/* CPL */
		US_TO_CPU_CYCLES(2),	/* JR NZ,3 if taken, 2 if not taken */
	US_TO_CPU_CYCLES(3),	/* LD SP,nnnn */
	US_TO_CPU_CYCLES(4),	/* LD (nnnn), A */
	US_TO_CPU_CYCLES(2),	/* INC SP */
	US_TO_CPU_CYCLES(2),	/* INC (HL) */
	US_TO_CPU_CYCLES(2),	/* DEC (HL) */
	US_TO_CPU_CYCLES(2),	/* LD (HL),n */
	US_TO_CPU_CYCLES(1),	/* SCF */
		US_TO_CPU_CYCLES(2),			/* JR NC, 3 if taken, 2 if not taken */
	US_TO_CPU_CYCLES(3),	/* ADD HL,SP */
	US_TO_CPU_CYCLES(4),	/* LD A,(nnnn) */
	US_TO_CPU_CYCLES(2),	/* DEC SP */
	US_TO_CPU_CYCLES(1),	/* INC A*/
	US_TO_CPU_CYCLES(1),	/* DEC A */
	US_TO_CPU_CYCLES(2),	/* LD A,n */
	US_TO_CPU_CYCLES(1),	/* CCF */
	US_TO_CPU_CYCLES(1),	/* LD B,B */
	US_TO_CPU_CYCLES(1),	/* LD B,C */
	US_TO_CPU_CYCLES(1),	/* LD B,D */
	US_TO_CPU_CYCLES(1),	/* LD B,E */
	US_TO_CPU_CYCLES(1),	/* LD B,H */
	US_TO_CPU_CYCLES(1),	/* LD B,L */
	US_TO_CPU_CYCLES(2),	/* LD B,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD B,A */
	US_TO_CPU_CYCLES(1),	/* LD C,B */
	US_TO_CPU_CYCLES(1),	/* LD C,C */
	US_TO_CPU_CYCLES(1),	/* LD C,D */
	US_TO_CPU_CYCLES(1),	/* LD C,E */
	US_TO_CPU_CYCLES(1),	/* LD C,H */
	US_TO_CPU_CYCLES(1),	/* LD C,L */
	US_TO_CPU_CYCLES(2),	/* LD C,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD C,A */
	US_TO_CPU_CYCLES(1),	/* LD D,B */
	US_TO_CPU_CYCLES(1),	/* LD D,C */
	US_TO_CPU_CYCLES(1),	/* LD D,D */
	US_TO_CPU_CYCLES(1),	/* LD D,E */
	US_TO_CPU_CYCLES(1),	/* LD D,H */
	US_TO_CPU_CYCLES(1),	/* LD D,L */
	US_TO_CPU_CYCLES(2),	/* LD D,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD D,A */
	US_TO_CPU_CYCLES(1),	/* LD E,B */
	US_TO_CPU_CYCLES(1),	/* LD E,C */
	US_TO_CPU_CYCLES(1),	/* LD E,D */
	US_TO_CPU_CYCLES(1),	/* LD E,E */
	US_TO_CPU_CYCLES(1),	/* LD E,H */
	US_TO_CPU_CYCLES(1),	/* LD E,L */
	US_TO_CPU_CYCLES(2),	/* LD E,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD E,A */
	US_TO_CPU_CYCLES(1),	/* LD H,B */
	US_TO_CPU_CYCLES(1),	/* LD H,C */
	US_TO_CPU_CYCLES(1),	/* LD H,D */
	US_TO_CPU_CYCLES(1),	/* LD H,E */
	US_TO_CPU_CYCLES(1),	/* LD H,H */
	US_TO_CPU_CYCLES(1),	/* LD H,L */
	US_TO_CPU_CYCLES(2),	/* LD H,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD H,A */
	US_TO_CPU_CYCLES(1),	/* LD L,B */
	US_TO_CPU_CYCLES(1),	/* LD L,C */
	US_TO_CPU_CYCLES(1),	/* LD L,D */
	US_TO_CPU_CYCLES(1),	/* LD L,E */
	US_TO_CPU_CYCLES(1),	/* LD L,H */
	US_TO_CPU_CYCLES(1),	/* LD L,L */
	US_TO_CPU_CYCLES(2),	/* LD L,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD L,A */
	US_TO_CPU_CYCLES(2),	/* LD (HL), B */
	US_TO_CPU_CYCLES(2),	/* LD (HL), C */
	US_TO_CPU_CYCLES(2),	/* LD (HL), D */
	US_TO_CPU_CYCLES(2),	/* LD (HL), E */
	US_TO_CPU_CYCLES(2),	/* LD (HL), H */
	US_TO_CPU_CYCLES(2),	/* LD (HL), L */
	US_TO_CPU_CYCLES(1),	/* HALT */
	US_TO_CPU_CYCLES(2),	/* LD (HL), A */
	US_TO_CPU_CYCLES(1),	/* LD A,B */
	US_TO_CPU_CYCLES(1),	/* LD A,C */
	US_TO_CPU_CYCLES(1),	/* LD A,D */
	US_TO_CPU_CYCLES(1),	/* LD A,E */
	US_TO_CPU_CYCLES(1),	/* LD A,H */
	US_TO_CPU_CYCLES(1),	/* LD A,L */
	US_TO_CPU_CYCLES(2),	/* LD A,(HL) */
	US_TO_CPU_CYCLES(1),	/* LD A,A */
	US_TO_CPU_CYCLES(1),	/* ADD A,B */
	US_TO_CPU_CYCLES(1),	/* ADD A,C */
	US_TO_CPU_CYCLES(1),	/* ADD A,D */
	US_TO_CPU_CYCLES(1),	/* ADD A,E */
	US_TO_CPU_CYCLES(1),	/* ADD A,H */
	US_TO_CPU_CYCLES(1),	/* ADD A,L */
	US_TO_CPU_CYCLES(2),	/* ADD A,(HL) */
	US_TO_CPU_CYCLES(1),	/* ADD A,A */
	US_TO_CPU_CYCLES(1),	/* ADC A,B */
	US_TO_CPU_CYCLES(1),	/* ADC A,C */
	US_TO_CPU_CYCLES(1),	/* ADC A,D */
	US_TO_CPU_CYCLES(1),	/* ADC A,E */
	US_TO_CPU_CYCLES(1),	/* ADC A,H */
	US_TO_CPU_CYCLES(1),	/* ADC A,L */
	US_TO_CPU_CYCLES(2),	/* ADC A,(HL) */
	US_TO_CPU_CYCLES(1),	/* ADC A,A */
	US_TO_CPU_CYCLES(1),	/* SUB A,B */
	US_TO_CPU_CYCLES(1),	/* SUB A,C */
	US_TO_CPU_CYCLES(1),	/* SUB A,D */
	US_TO_CPU_CYCLES(1),	/* SUB A,E */
	US_TO_CPU_CYCLES(1),	/* SUB A,H */
	US_TO_CPU_CYCLES(1),	/* SUB A,L */
	US_TO_CPU_CYCLES(2),	/* SUB A,(HL) */
	US_TO_CPU_CYCLES(1),	/* SUB A,A */
	US_TO_CPU_CYCLES(1),	/* SBC A,B */
	US_TO_CPU_CYCLES(1),	/* SBC A,C */
	US_TO_CPU_CYCLES(1),	/* SBC A,D */
	US_TO_CPU_CYCLES(1),	/* SBC A,E */
	US_TO_CPU_CYCLES(1),	/* SBC A,H */
	US_TO_CPU_CYCLES(1),	/* SBC A,L */
	US_TO_CPU_CYCLES(2),	/* SBC A,(HL) */
	US_TO_CPU_CYCLES(1),	/* SBC A,A */
	US_TO_CPU_CYCLES(1),	/* AND A,B */
	US_TO_CPU_CYCLES(1),	/* AND A,C */
	US_TO_CPU_CYCLES(1),	/* AND A,D */
	US_TO_CPU_CYCLES(1),	/* AND A,E */
	US_TO_CPU_CYCLES(1),	/* AND A,H */
	US_TO_CPU_CYCLES(1),	/* AND A,L */
	US_TO_CPU_CYCLES(2),	/* AND A,(HL) */
	US_TO_CPU_CYCLES(1),	/* AND A,A */
	US_TO_CPU_CYCLES(1),	/* XOR A,B */
	US_TO_CPU_CYCLES(1),	/* XOR A,C */
	US_TO_CPU_CYCLES(1),	/* XOR A,D */
	US_TO_CPU_CYCLES(1),	/* XOR A,E */
	US_TO_CPU_CYCLES(1),	/* XOR A,H */
	US_TO_CPU_CYCLES(1),	/* XOR A,L */
	US_TO_CPU_CYCLES(2),	/* XOR A,(HL) */
	US_TO_CPU_CYCLES(1),	/* XOR A,A */
	US_TO_CPU_CYCLES(1),	/* OR A,B */
	US_TO_CPU_CYCLES(1),	/* OR A,C */
	US_TO_CPU_CYCLES(1),	/* OR A,D */
	US_TO_CPU_CYCLES(1),	/* OR A,E */
	US_TO_CPU_CYCLES(1),	/* OR A,H */
	US_TO_CPU_CYCLES(1),	/* OR A,L */
	US_TO_CPU_CYCLES(2),	/* OR A,(HL) */
	US_TO_CPU_CYCLES(1),	/* OR A,A */
	US_TO_CPU_CYCLES(1),	/* CP A,B */
	US_TO_CPU_CYCLES(1),	/* CP A,C */
	US_TO_CPU_CYCLES(1),	/* CP A,D */
	US_TO_CPU_CYCLES(1),	/* CP A,E */
	US_TO_CPU_CYCLES(1),	/* CP A,H */
	US_TO_CPU_CYCLES(1),	/* CP A,L */
	US_TO_CPU_CYCLES(2),	/* CP A,(HL) */
	US_TO_CPU_CYCLES(1),	/* CP A,A */
		US_TO_CPU_CYCLES(2),	/* RET NZ 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(3),	/* POP BC */
	US_TO_CPU_CYCLES(3),	/* JP NZ, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(3),	/* JP  */
		US_TO_CPU_CYCLES(3),	/* CALL NZ 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(4),	/* PUSH BC */
	US_TO_CPU_CYCLES(2),	/* ADD A,n */
	US_TO_CPU_CYCLES(4),	/* RST 0 */
		US_TO_CPU_CYCLES(2),	/* RET Z 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(3),	/* RET	*/
	US_TO_CPU_CYCLES(3),	/* JP NZ, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* cb prefix */
		US_TO_CPU_CYCLES(3),	/* CALL NZ 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(5),	/* CALL */
	US_TO_CPU_CYCLES(2),	/* ADC A,n */
	US_TO_CPU_CYCLES(4),	/* RST 8 */
		US_TO_CPU_CYCLES(2),	/* RET NC 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(3),	/* POP DE */
	US_TO_CPU_CYCLES(3),	/* JP NC, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(3),	/* OUT (n), A */
		US_TO_CPU_CYCLES(3),	/* CALL NC 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(4),	/* PUSH DE */
	US_TO_CPU_CYCLES(2),	/* SUB A,n */
	US_TO_CPU_CYCLES(4),	/* RST 10 */
		US_TO_CPU_CYCLES(2),	/* RET C 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(1),	/* EXX */
	US_TO_CPU_CYCLES(3),	/* JP C, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(3),	/* IN A,(n) */
		US_TO_CPU_CYCLES(3),	/* CALL C 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* DD prefix */
	US_TO_CPU_CYCLES(2),	/* SBC A,n */
	US_TO_CPU_CYCLES(4),	/* RST 18 */
		US_TO_CPU_CYCLES(2),	/* RET PO 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(3),	/* POP HL */
	US_TO_CPU_CYCLES(3),	/* JP PO, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(6),	/* EX SP, HL */
		US_TO_CPU_CYCLES(3),	/* CALL PO 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(4),	/* PUSH HL */
	US_TO_CPU_CYCLES(2),	/* AND A,n */
	US_TO_CPU_CYCLES(4),	/* RST 20 */
		US_TO_CPU_CYCLES(2),	/* RET PE 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(1),	/* JP (HL) */
	US_TO_CPU_CYCLES(3),	/* JP PE, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* EX DE,HL */
		US_TO_CPU_CYCLES(3),	/* CALL PE 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* ED prefix */
	US_TO_CPU_CYCLES(2),	/* XOR A,n */
	US_TO_CPU_CYCLES(4),	/* RST 28 */
		US_TO_CPU_CYCLES(2),	/* RET P 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(3),	/* POP AF */
	US_TO_CPU_CYCLES(3),	/* JP P, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* DI */
		US_TO_CPU_CYCLES(3),	/* CALL P 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(4),	/* PUSH AF */
	US_TO_CPU_CYCLES(2),	/* OR A,n */
	US_TO_CPU_CYCLES(4),	/* RST 30 */
		US_TO_CPU_CYCLES(2),	/* RET M 4 taken, 2 not taken */
	US_TO_CPU_CYCLES(2),	/* LD SP,HL */
	US_TO_CPU_CYCLES(3),	/* JP M, 3 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* EI */
		US_TO_CPU_CYCLES(3),	/* CALL M 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(1),	/* FD prefix */
	US_TO_CPU_CYCLES(2),	/* CP A,n */
	US_TO_CPU_CYCLES(4),	/* RST 38 */

};

static UINT8 amstrad_cycle_table_cb[256]=
{
		/* RLC */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* RRC */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* RL */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* RR */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* SLA */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* SRA */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* SLL */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* SRL */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* BIT */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(3), US_TO_CPU_CYCLES(2),

		/* RES */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

		/* SET */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(4), US_TO_CPU_CYCLES(2),

};


static UINT8 amstrad_cycle_table_ed[256]=
{
		/* 0x00-0x03f */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

		US_TO_CPU_CYCLES(4), /* IN B,(C) */
		US_TO_CPU_CYCLES(4), /* OUT (C), B*/
		US_TO_CPU_CYCLES(4), /* SBC HL,BC */
		US_TO_CPU_CYCLES(6), /* LD (nnnn), bc */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETN */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(3), /* LD I,A */

		US_TO_CPU_CYCLES(4), /* IN C,(C) */
		US_TO_CPU_CYCLES(4), /* OUT (C), C*/
		US_TO_CPU_CYCLES(4), /* ADC HL,BC */
		US_TO_CPU_CYCLES(6), /* LD bc,(nnnn) */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETI */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(3), /* LD R,A */

		US_TO_CPU_CYCLES(4), /* IN D,(C) */
		US_TO_CPU_CYCLES(4), /* OUT (C), D*/
		US_TO_CPU_CYCLES(4), /* SBC HL,DE*/
		US_TO_CPU_CYCLES(6), /* LD (nnnn), DE */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETN */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(3), /* LD A,I */

		US_TO_CPU_CYCLES(4), /* IN E,(C)*/
		US_TO_CPU_CYCLES(4), /* OUT (C), E*/
		US_TO_CPU_CYCLES(4), /* ADC HL,DE */
		US_TO_CPU_CYCLES(6), /* LD de, (nnnn) */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETI */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(3), /* LD A,R */


		US_TO_CPU_CYCLES(4), /* IN H,(C)*/
		US_TO_CPU_CYCLES(4), /* OUT (C),H */
		US_TO_CPU_CYCLES(4), /* SBC HL,HL */
		US_TO_CPU_CYCLES(6), /* LD (nnnn), HL */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETN */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(5), /* RRD */

		US_TO_CPU_CYCLES(4), /* IN L,(C) */
		US_TO_CPU_CYCLES(4), /* OUT (C),L */
		US_TO_CPU_CYCLES(4), /* ADC HL,HL */
		US_TO_CPU_CYCLES(6), /* LD hl, (nnnn) */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETI */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(5), /* RLD */

		US_TO_CPU_CYCLES(4), /* IN X,(C)*/
		US_TO_CPU_CYCLES(4), /* OUT (C), 0 */
		US_TO_CPU_CYCLES(4), /* SBC HL,SP */
		US_TO_CPU_CYCLES(6), /* LD (nnnn), sp */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETN */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(2), /*  */

		US_TO_CPU_CYCLES(4), /* IN A,(C) */
		US_TO_CPU_CYCLES(4), /* OUT (C), A */
		US_TO_CPU_CYCLES(4), /* ADC HL,SP */
		US_TO_CPU_CYCLES(6), /* LD sp.(nnnn) */
		US_TO_CPU_CYCLES(2), /* NEG */
		US_TO_CPU_CYCLES(4), /* RETI */
		US_TO_CPU_CYCLES(2), /* IM */
		US_TO_CPU_CYCLES(2), /*  */

		/* 0x080-0x09f */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

		US_TO_CPU_CYCLES(5), /* LDI */
		US_TO_CPU_CYCLES(5), /* CPI */
		US_TO_CPU_CYCLES(5), /* INI */
		US_TO_CPU_CYCLES(5), /* OUTI */

		/* 0x0a4-0x0a7 */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

		US_TO_CPU_CYCLES(5), /* LDD */
		US_TO_CPU_CYCLES(5), /* CPD */
		US_TO_CPU_CYCLES(5), /* IND */
		US_TO_CPU_CYCLES(5), /* OUTD */

		/* 0x0ac-0x0af */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

		US_TO_CPU_CYCLES(5), /* LDIR */
		US_TO_CPU_CYCLES(5), /* CPIR */
		US_TO_CPU_CYCLES(5), /* INIR */
		US_TO_CPU_CYCLES(5), /* OTIR */

		/* 0x0b4-0x0b7 */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

		US_TO_CPU_CYCLES(5), /* LDDR */
		US_TO_CPU_CYCLES(5), /* CPDR */
		US_TO_CPU_CYCLES(5), /* INDR */
		US_TO_CPU_CYCLES(5), /* OTDR */

		/* 0x0c0-0x0ff */
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),
		US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2), US_TO_CPU_CYCLES(2),

};


static UINT8 amstrad_cycle_table_xy[256]=
{
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only  */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* ADD xy,BC */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* ADD xy,DE */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* LD xy,nnnn */
	US_TO_CPU_CYCLES(6),	/* LD (nnnn),xy */
	US_TO_CPU_CYCLES(3),	/* INC xy */
	US_TO_CPU_CYCLES(2),	/* INC hxy */
	US_TO_CPU_CYCLES(2),	/* DEC hxy */
	US_TO_CPU_CYCLES(3),	/* LD hxy,n */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* ADD xy,xy */
	US_TO_CPU_CYCLES(6),	/* LD HL,(nnnn) */
	US_TO_CPU_CYCLES(3),	/* DEC xy */
	US_TO_CPU_CYCLES(2),	/* INC lxy */
	US_TO_CPU_CYCLES(2),	/* DEC lxy */
	US_TO_CPU_CYCLES(3),	/* LD lxy,n */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(6),	/* INC (xy) */
	US_TO_CPU_CYCLES(6),	/* DEC (xy) */
	US_TO_CPU_CYCLES(6),	/* LD (xy),n */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* ADD xy,SP */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD B,hxy */
	US_TO_CPU_CYCLES(2),	/* LD B,lxy */
	US_TO_CPU_CYCLES(5),	/* LD B,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD C,hxy */
	US_TO_CPU_CYCLES(2),	/* LD C,lxy */
	US_TO_CPU_CYCLES(5),	/* LD C,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD D,hxy */
	US_TO_CPU_CYCLES(2),	/* LD D,lxy */
	US_TO_CPU_CYCLES(5),	/* LD D,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD E,hxy */
	US_TO_CPU_CYCLES(2),	/* LD E,lxy */
	US_TO_CPU_CYCLES(5),	/* LD E,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD hxy,B */
	US_TO_CPU_CYCLES(2),	/* LD hxy,C */
	US_TO_CPU_CYCLES(2),	/* LD hxy,D */
	US_TO_CPU_CYCLES(2),	/* LD hxy,E */
	US_TO_CPU_CYCLES(2),	/* LD hxy,hxy */
	US_TO_CPU_CYCLES(2),	/* LD hxy,lxy */
	US_TO_CPU_CYCLES(5),	/* LD H,(xy) */
	US_TO_CPU_CYCLES(2),	/* LD hxy,A */
	US_TO_CPU_CYCLES(2),	/* LD lxy,B */
	US_TO_CPU_CYCLES(2),	/* LD lxy,C */
	US_TO_CPU_CYCLES(2),	/* LD lxy,D */
	US_TO_CPU_CYCLES(2),	/* LD lxy,E */
	US_TO_CPU_CYCLES(2),	/* LD lxy,H */
	US_TO_CPU_CYCLES(2),	/* LD lxy,L */
	US_TO_CPU_CYCLES(5),	/* LD l,(HL) */
	US_TO_CPU_CYCLES(2),	/* LD lxy,A */
	US_TO_CPU_CYCLES(5),	/* LD (xy), B */
	US_TO_CPU_CYCLES(5),	/* LD (xy), C */
	US_TO_CPU_CYCLES(5),	/* LD (xy), D */
	US_TO_CPU_CYCLES(5),	/* LD (xy), E */
	US_TO_CPU_CYCLES(5),	/* LD (xy), H */
	US_TO_CPU_CYCLES(5),	/* LD (xy), L */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(5),	/* LD (xy), A */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* LD A,hxy */
	US_TO_CPU_CYCLES(2),	/* LD A,lxy */
	US_TO_CPU_CYCLES(5),	/* LD A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* ADD A,hxy */
	US_TO_CPU_CYCLES(2),	/* ADD A,lxy */
	US_TO_CPU_CYCLES(5),	/* ADD A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* ADC A,hxy */
	US_TO_CPU_CYCLES(2),	/* ADC A,lxy */
	US_TO_CPU_CYCLES(5),	/* ADC A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* SUB A,hxy */
	US_TO_CPU_CYCLES(2),	/* SUB A,lxy */
	US_TO_CPU_CYCLES(5),	/* SUB A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* SBC A,hxy */
	US_TO_CPU_CYCLES(2),	/* SBC A,lxy */
	US_TO_CPU_CYCLES(5),	/* SBC A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* AND A,hxy */
	US_TO_CPU_CYCLES(2),	/* AND A,lxy */
	US_TO_CPU_CYCLES(5),	/* AND A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* XOR A,hxy */
	US_TO_CPU_CYCLES(2),	/* XOR A,lxy */
	US_TO_CPU_CYCLES(5),	/* XOR A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* OR A,hxy */
	US_TO_CPU_CYCLES(2),	/* OR A,lxy */
	US_TO_CPU_CYCLES(5),	/* OR A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* CP A,hxy */
	US_TO_CPU_CYCLES(2),	/* CP A,lxy */
	US_TO_CPU_CYCLES(5),	/* CP A,(xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* CB prefix */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* DD prefix */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(4),	/* POP xy */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(7),	/* EX SP, (xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(5),	/* PUSH xy */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* JP (xy) */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* ED prefix */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(3),	/* LD SP,HL */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(2),	/* FD prefix */
	US_TO_CPU_CYCLES(1),	/* illegal - time for prefix only */
	US_TO_CPU_CYCLES(1) 	/* illegal - time for prefix only */
};

static UINT8 amstrad_cycle_table_xycb[256]=
{
		/* 0x00-0x03f */
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),

		/* 0x040-0x07f */
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),
		US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6), US_TO_CPU_CYCLES(6),

		/* 0x080-0x0ff */
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
		US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7), US_TO_CPU_CYCLES(7),
};

static UINT8 amstrad_cycle_table_ex[256]=
{

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(4-3),	/*	DJNZ  4 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(3-2),	/* JR 3 if taken, 2 if not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(3-2),	/*	 JR Z,3 if taken, 2 if not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(3-2),	/* JR NZ,3 if taken, 2 if not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(3-2),		/* JR NC, 3 if taken, 2 if not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		/* a0 */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

		US_TO_CPU_CYCLES(5-5),	  /* LDIR */
		US_TO_CPU_CYCLES(5-5),	  /* CPIR */
		US_TO_CPU_CYCLES(5-5),	  /* INIR */
		US_TO_CPU_CYCLES(5-5),	  /* OTIR */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(5-5),	  /* LDDR */
		US_TO_CPU_CYCLES(5-5),	  /* CPDR */
		US_TO_CPU_CYCLES(5-5),	  /* INDR */
		US_TO_CPU_CYCLES(5-5),	  /* OTDR */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */

	US_TO_CPU_CYCLES(4-2),	/* RET NZ 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL NZ 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 0 */
	US_TO_CPU_CYCLES(4-2),	/* RET Z 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL NZ 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 8 */
	US_TO_CPU_CYCLES(4-2),	/* RET NC 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL NC 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 10 */
	US_TO_CPU_CYCLES(4-2),	/* RET C 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL C 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(1),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 18 */
	US_TO_CPU_CYCLES(4-2),	/* RET PO 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL PO 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 20 */
	US_TO_CPU_CYCLES(4-2),	/* RET PE 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL PE 5 taken, 3 not taken */
	US_TO_CPU_CYCLES(0),	/* ED prefix */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(1),	/* RST 28 */
	US_TO_CPU_CYCLES(4-2),	/* RET P 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL P 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(1),	/* RST 30 */
	US_TO_CPU_CYCLES(4-2),	/* RET M 4 taken, 2 not taken */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(5-3),	/* CALL M 5 taken, 3 not taken */
		US_TO_CPU_CYCLES(1),	/* FD prefix */
		US_TO_CPU_CYCLES(0),	/* not used */
	US_TO_CPU_CYCLES(1),	/* RST 38 */
};

static const void *previous_op_table;
static const void *previous_cb_table;
static const void *previous_ed_table;
static const void *previous_xy_table;
static const void *previous_xycb_table;
static const void *previous_ex_table;


static void amstrad_common_init(void)
{

	/* set all colours to black */
	int i;

	for (i = 0; i < 17; i++)
	{
		AmstradCPC_PenColours[i] = 0x014;
	}

	ppi8255_init(&amstrad_ppi8255_interface);

	AmstradCPC_GA_RomConfiguration = 0;
	timer_pulse(TIME_IN_USEC(64), 0,amstrad_interrupt_timer_callback);

	amstrad_52_divider = 0;
	amstrad_52_divider_vsync_reset = 0;

	install_mem_read_handler(0, 0x0000, 0x1fff, MRA8_BANK1);
	install_mem_read_handler(0, 0x2000, 0x3fff, MRA8_BANK2);
	install_mem_read_handler(0, 0x4000, 0x5fff, MRA8_BANK3);
	install_mem_read_handler(0, 0x6000, 0x7fff, MRA8_BANK4);
	install_mem_read_handler(0, 0x8000, 0x9fff, MRA8_BANK5);
	install_mem_read_handler(0, 0xa000, 0xbfff, MRA8_BANK6);
	install_mem_read_handler(0, 0xc000, 0xdfff, MRA8_BANK7);
	install_mem_read_handler(0, 0xe000, 0xffff, MRA8_BANK8);

	install_mem_write_handler(0, 0x0000, 0x1fff, MWA8_BANK9);
	install_mem_write_handler(0, 0x2000, 0x3fff, MWA8_BANK10);
	install_mem_write_handler(0, 0x4000, 0x5fff, MWA8_BANK11);
	install_mem_write_handler(0, 0x6000, 0x7fff, MWA8_BANK12);
	install_mem_write_handler(0, 0x8000, 0x9fff, MWA8_BANK13);
	install_mem_write_handler(0, 0xa000, 0xbfff, MWA8_BANK14);
	install_mem_write_handler(0, 0xc000, 0xdfff, MWA8_BANK15);
	install_mem_write_handler(0, 0xe000, 0xffff, MWA8_BANK16);

	amstrad_cycles_at_frame_end = 0;
	amstrad_cycles_last_write = 0;
	time_delta_fraction = 0;

	cpu_irq_line_vector_w(0, 0,0x0ff);

	nec765_init(&amstrad_nec765_interface,NEC765A/*?*/);

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0),  FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 1),  FLOPPY_DRIVE_SS_40);

	/* Juergen is a cool dude! */
	cpu_set_irq_callback(0, amstrad_cpu_acknowledge_int);

	/* The opcode timing in the Amstrad is different to the opcode
	timing in the core for the Z80 CPU.

	The Amstrad hardware issues a HALT for each memory fetch.
	This has the effect of stretching the timing for Z80 opcodes,
	so that they are all multiple of 4 T states long. All opcode
	timings are a multiple of 1us in length. */

	previous_op_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_op);
	previous_cb_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_cb);
	previous_ed_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ed);
	previous_xy_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xy);
	previous_xycb_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xycb);
	previous_ex_table = cpunum_get_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ex);

	/* Using the cool code Juergen has provided, I will override
	the timing tables with the values for the amstrad */
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_op, amstrad_cycle_table_op);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_cb, amstrad_cycle_table_cb);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ed, amstrad_cycle_table_ed);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xy, amstrad_cycle_table_xy);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xycb, amstrad_cycle_table_xycb);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ex, amstrad_cycle_table_ex);
}

static MACHINE_STOP( amstrad )
{
	/* restore previous tables */
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_op, (void *) previous_op_table);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_cb, (void *) previous_cb_table);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ed, (void *) previous_ed_table);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xy, (void *) previous_xy_table);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_xycb, (void *) previous_xycb_table);
	cpunum_set_info_ptr(0,CPUINFO_PTR_Z80_CYCLE_TABLE+Z80_TABLE_ex, (void *) previous_ex_table);

	cpu_set_irq_callback(0, NULL);

}

static MACHINE_INIT( amstrad )
{
	int i;
	unsigned char machine_name_and_refresh_rate;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &memory_region(REGION_CPU1)[0x014000];
	}
	
	Amstrad_ROM_Table[7] = &memory_region(REGION_CPU1)[0x018000];

	amstrad_common_init();

	amstrad_setup_machine();

	machine_name_and_refresh_rate = readinputport(10);
	ppi_port_inputs[1] = ((machine_name_and_refresh_rate & 0x07)<<1) | (machine_name_and_refresh_rate & 0x010);

	multiface_init();
}

static MACHINE_INIT( kccomp )
{
	int i;

	for (i=0; i<256; i++)
	{
		Amstrad_ROM_Table[i] = &memory_region(REGION_CPU1)[0x014000];
	}

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

	multiface_reset();
}

/* Memory is banked in 16k blocks. However, the multiface
pages the memory in 8k blocks! The ROM can
be paged into bank 0 and bank 3. */
static ADDRESS_MAP_START(readmem_amstrad, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x01fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x02000, 0x03fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x04000, 0x05fff) AM_READ(MRA8_BANK3)
	AM_RANGE(0x06000, 0x07fff) AM_READ(MRA8_BANK4)
	AM_RANGE(0x08000, 0x09fff) AM_READ(MRA8_BANK5)
	AM_RANGE(0x0a000, 0x0bfff) AM_READ(MRA8_BANK6)
	AM_RANGE(0x0c000, 0x0dfff) AM_READ(MRA8_BANK7)
	AM_RANGE(0x0e000, 0x0ffff) AM_READ(MRA8_BANK8)
ADDRESS_MAP_END

static ADDRESS_MAP_START(writemem_amstrad, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x01fff) AM_WRITE(MWA8_BANK9)
	AM_RANGE(0x02000, 0x03fff) AM_WRITE(MWA8_BANK10)
	AM_RANGE(0x04000, 0x05fff) AM_WRITE(MWA8_BANK11)
	AM_RANGE(0x06000, 0x07fff) AM_WRITE(MWA8_BANK12)
	AM_RANGE(0x08000, 0x09fff) AM_WRITE(MWA8_BANK13)
	AM_RANGE(0x0a000, 0x0bfff) AM_WRITE(MWA8_BANK14)
	AM_RANGE(0x0c000, 0x0dfff) AM_WRITE(MWA8_BANK15)
	AM_RANGE(0x0e000, 0x0ffff) AM_WRITE(MWA8_BANK16)
ADDRESS_MAP_END

/* I've handled the I/O ports in this way, because the ports
are not fully decoded by the CPC h/w. Doing it this way means
I can decode it myself and a lot of  software should work */
static ADDRESS_MAP_START(readport_amstrad, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_READ(AmstradCPC_ReadPortHandler)
ADDRESS_MAP_END

static ADDRESS_MAP_START(writeport_amstrad, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_WRITE(AmstradCPC_WritePortHandler)
ADDRESS_MAP_END

/* read PSG port A */
static READ_HANDLER ( amstrad_psg_porta_read )
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
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\x18", KEYCODE_UP, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "\x1a", KEYCODE_RIGHT, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "\x19", KEYCODE_DOWN, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 9", KEYCODE_9_PAD, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 6", KEYCODE_6_PAD, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 3", KEYCODE_3_PAD, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad Enter", KEYCODE_ENTER_PAD, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad .", KEYCODE_DEL_PAD, IP_JOY_NONE) \
\
	/* keyboard line 1 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\x1b", KEYCODE_LEFT, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Copy", KEYCODE_LALT, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 7", KEYCODE_7_PAD, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 8", KEYCODE_8_PAD, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 5", KEYCODE_5_PAD, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 1", KEYCODE_1_PAD, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 2", KEYCODE_2_PAD, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 0", KEYCODE_0_PAD, IP_JOY_NONE) \
\
	/* keyboard row 2 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Clr", KEYCODE_HOME, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "@ |", KEYCODE_ASTERISK, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keypad 4", KEYCODE_4_PAD, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left Shift", KEYCODE_LSHIFT, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right Shift", KEYCODE_RSHIFT, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Ctrl", KEYCODE_LCONTROL, IP_JOY_NONE) \
\
	/* keyboard row 3 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "^ \xa3", KEYCODE_SLASH_PAD, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "- =", KEYCODE_MINUS, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P", KEYCODE_P, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "; +", KEYCODE_QUOTE, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ": *", KEYCODE_COLON, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE) \
\
	/* keyboard line 4 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 _", KEYCODE_0, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 )", KEYCODE_9, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O", KEYCODE_O, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I", KEYCODE_I, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE) \
\
	/* keyboard line 5 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 (", KEYCODE_8, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 '", KEYCODE_7, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U", KEYCODE_U, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE) \
\
	/* keyboard line 6 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 &", KEYCODE_6, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R", KEYCODE_R, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T", KEYCODE_T, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G", KEYCODE_G, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F", KEYCODE_F, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE) \
\
	/* keyboard line 7 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E", KEYCODE_E, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W", KEYCODE_W, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S", KEYCODE_S, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D", KEYCODE_D, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C", KEYCODE_C, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X", KEYCODE_X, IP_JOY_NONE) \
\
	/* keyboard line 8 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 \"", KEYCODE_2, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Esc", KEYCODE_ESC, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z", KEYCODE_Z, IP_JOY_NONE) \
\
	/* keyboard line 9 */ \
	PORT_START \
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1) \
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1) \
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1) \
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1) \
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1) \
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1) \
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1) \
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Del", KEYCODE_BACKSPACE, IP_JOY_NONE) \


#define MULTIFACE_PORTS \
	PORT_START \
	PORT_BITX(0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Multiface Hardware", IP_KEY_NONE, IP_JOY_NONE) \
	PORT_DIPSETTING(0x00, DEF_STR( Off) ) \
	PORT_DIPSETTING(0x01, DEF_STR( On) ) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Multiface Stop", KEYCODE_F1, IP_JOY_NONE) \






/* Steph 2000-10-27	I remapped the 'Machine Name' Dip Switches (easier to understand) */

INPUT_PORTS_START(amstrad)

	KEYBOARD_PORTS

	/* the following are defined as dipswitches, but are in fact solder links on the
	 * curcuit board. The links are open or closed when the PCB is made, and are set depending on which country
	 * the Amstrad system was to go to */
	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Manufacturer Name" )
	PORT_DIPSETTING(    0x00, "Isp" )
	PORT_DIPSETTING(    0x01, "Triumph" )
	PORT_DIPSETTING(    0x02, "Saisho" )
	PORT_DIPSETTING(    0x03, "Solavox" )
	PORT_DIPSETTING(    0x04, "Awa" )
	PORT_DIPSETTING(    0x05, "Schneider" )
	PORT_DIPSETTING(    0x06, "Orion" )
	PORT_DIPSETTING(    0x07, "Amstrad" )

	/* Steph's comment/question :
		I don't understand why there is a IPF_TOGGLE here ...
		Couldn't we use a standard PORT_DIPNAME instead ?
		PORT_DIPNAME( 0x10, 0x10, "TV Refresh Rate" ) */

	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "TV Refresh Rate", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(    0x00, "60 Hz" )
	PORT_DIPSETTING(    0x10, "50 Hz" )

	MULTIFACE_PORTS

INPUT_PORTS_END

INPUT_PORTS_START(kccomp)
		KEYBOARD_PORTS
INPUT_PORTS_END

static struct Wave_interface wave_interface = {
	1,		/* 1 cassette recorder */
	{ 50 }	/* mixing levels in percent */
};

/* actual clock to CPU is 4Mhz, but it is slowed by memory
accessess. A HALT is used for every memory access by the CPU.
This stretches the timing for opcodes, and gives an effective
speed of 3.8Mhz */

/* Info about structures below:

	The Amstrad has a CPU running at 4Mhz, slowed with wait states.
	I have measured 19968 NOP instructions per frame, which gives,
	50.08 fps as the tv refresh rate.

  There are 312 lines on a PAL screen, giving 64us per line.

	There is only 50us visible per line, and 35*8 lines visible on the
	screen.

  This is the reason why the displayed area is not the same as
  the visible area.
 */

static MACHINE_DRIVER_START( amstrad )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        /*((AMSTRAD_US_PER_FRAME*AMSTRAD_FPS)*4)*/ /* clock: See Note Above */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_PROGRAM_MAP(readmem_amstrad,writemem_amstrad)
	MDRV_CPU_IO_MAP(readport_amstrad,writeport_amstrad)
	MDRV_FRAMES_PER_SECOND(50)	/*50.08*/
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( amstrad )
	MDRV_MACHINE_STOP( amstrad )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(AMSTRAD_MONITOR_SCREEN_WIDTH, AMSTRAD_MONITOR_SCREEN_HEIGHT)
	MDRV_VISIBLE_AREA(0, (AMSTRAD_SCREEN_WIDTH - 1), 0, (AMSTRAD_SCREEN_HEIGHT - 1))
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32)
	MDRV_PALETTE_INIT( amstrad_cpc )

	MDRV_VIDEO_EOF( amstrad )
	MDRV_VIDEO_START( amstrad )
	MDRV_VIDEO_UPDATE( amstrad )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, amstrad_ay_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kccomp )
	MDRV_IMPORT_FROM( amstrad )
	MDRV_MACHINE_INIT( kccomp )
	MDRV_SCREEN_SIZE(AMSTRAD_SCREEN_WIDTH, AMSTRAD_SCREEN_HEIGHT)
	MDRV_PALETTE_INIT( kccomp )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cpcplus )
	MDRV_IMPORT_FROM( amstrad )
	MDRV_FRAMES_PER_SECOND( 50.08 )
	MDRV_SCREEN_SIZE(AMSTRAD_SCREEN_WIDTH, AMSTRAD_SCREEN_HEIGHT)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(4096)
	MDRV_PALETTE_INIT( amstrad_plus )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* cpc6128.rom contains OS in first 16k, BASIC in second 16k */
/* cpcados.rom contains Amstrad DOS */

/* I am loading the roms outside of the Z80 memory area, because they
are banked. */
ROM_START(cpc6128)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x020000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc6128.rom", 0x10000, 0x8000, CRC(9e827fe1))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd))

	/* optional Multiface hardware */
		ROM_LOAD_OPTIONAL("multface.rom", 0x01c000, 0x02000, CRC(f36086de))

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END

ROM_START(cpc464)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc464.rom", 0x10000, 0x8000, CRC(40852f25))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd))

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END

ROM_START(cpc664)
	/* this defines the total memory size - 64k ram, 16k OS, 16k BASIC, 16k DOS */
	ROM_REGION(0x01c000, REGION_CPU1,0)
	/* load the os to offset 0x01000 from memory base */
	ROM_LOAD("cpc664.rom", 0x10000, 0x8000, CRC(9AB5A036))
	ROM_LOAD("cpcados.rom", 0x18000, 0x4000, CRC(1fe22ecd))

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0100,REGION_GFX1) */
ROM_END


ROM_START(kccomp)
	ROM_REGION(0x018000, REGION_CPU1,0)
	ROM_LOAD("kccos.rom", 0x10000, 0x04000, CRC(7f9ab3f7))
	ROM_LOAD("kccbas.rom", 0x14000, 0x04000, CRC(ca6af63d))
	ROM_REGION(0x018000+0x0800, REGION_PROMS, 0 )
	ROM_LOAD("farben.rom", 0x018000, 0x0800, CRC(a50fa3cf))

	/* fake region - required by graphics decode structure */
	/*ROM_REGION(0x0c00, REGION_GFX1) */
ROM_END


/* this system must have a cartridge installed to run */
ROM_START(cpc6128p)
	ROM_REGION(0, REGION_CPU1,0)
ROM_END


/* this system must have a cartridge installed to run */
ROM_START(cpc464p)
	ROM_REGION(0, REGION_CPU1,0)
ROM_END

SYSTEM_CONFIG_START(cpc6128)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE_LEGACY_DSK(2)
	CONFIG_DEVICE_CASSETTE(1, NULL)
	CONFIG_DEVICE_PRINTER(1)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(cpcplus)
	CONFIG_IMPORT_FROM(cpc6128)
	CONFIG_DEVICE_CARTSLOT_REQ(1,	"cpr\0", NULL, NULL, device_load_amstrad_plus_cartridge, NULL, NULL, NULL)
	CONFIG_DEVICE_SNAPSHOT(			"sna\0", amstrad)
SYSTEM_CONFIG_END

/*      YEAR  NAME    PARENT	COMPAT  MACHINE    INPUT    INIT    CONFIG,  COMPANY               FULLNAME */
COMP( 1984, cpc464,   0,		0,		amstrad,  amstrad,	0,		cpc6128, "Amstrad plc", "Amstrad/Schneider CPC464")
COMP( 1985, cpc664,   cpc464,	0,		amstrad,  amstrad,	0,	    cpc6128, "Amstrad plc", "Amstrad/Schneider CPC664")
COMP( 1985, cpc6128,  cpc464,	0,		amstrad,  amstrad,	0,	    cpc6128, "Amstrad plc", "Amstrad/Schneider CPC6128")
COMP( 1990, cpc464p,  0,		0,		cpcplus,  amstrad,	0,	    cpcplus, "Amstrad plc", "Amstrad 464plus")
COMP( 1990, cpc6128p, 0,		0,		cpcplus,  amstrad,	0,	    cpcplus, "Amstrad plc", "Amstrad 6128plus")
COMP( 1989, kccomp,   cpc464,	0,		kccomp,   kccomp,	0,	    cpc6128, "VEB Mikroelektronik", "KC Compact")

