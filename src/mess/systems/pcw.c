/******************************************************************************

	pcw.c
	system driver

	Kevin Thacker [MESS driver]

	Thankyou to Jacob Nevins, Richard Fairhurst and Cliff Lawson,
	for their documentation that I used for the development of this
	driver.

 ******************************************************************************/
#include "driver.h"
#include "cpuintrf.h"
#include "mess/machine/nec765.h"
#include "mess/machine/dsk.h"
#include "mess/vidhrdw/pcw.h"

//#define VERBOSE
//#define PCW_DUMP_RAM

void pcw_fdc_interrupt(int);
void *pcw_int_timer = NULL;

unsigned char *pcw_ram;
unsigned int roller_ram_addr;
int 	pcw_boot;
int	pcw_system_status;
unsigned short roller_ram_offset;
int fdc_interrupt_code;
unsigned char pcw_vdu_video_control_register;
int pcw_interrupt_counter;
int pcw_ram_size = 2;

#ifdef PCW_DUMP_RAM
/* load image */
void pcw_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "pcwram.bin", OSD_FILETYPE_MEMCARD,OSD_FOPEN_WRITE);

	if (file)
	{
		int i;
		for (i=0; i<65536; i++)
		{
			char data;

			data = cpu_readmem16(i);

			osd_fwrite(file, &data, 1);

		}

		/* close file */
		osd_fclose(file);
	}
}
#endif

static void pcw_update_interrupt_counter(void)
{
	/* never increments past 15! */
	if (pcw_interrupt_counter==0x0f)
		return;

	/* increment count */
	pcw_interrupt_counter++;
}

static nec765_interface pcw_nec765_interface =
{
	dsk_seek_callback,
	dsk_get_sectors_per_track,
	dsk_get_id_callback,
	dsk_get_sector_ptr_callback,
	pcw_fdc_interrupt
};
void pcw_interrupt_handle(void)
{
	if (
		(pcw_interrupt_counter!=0) ||
		((fdc_interrupt_code==1) && ((pcw_system_status & (1<<5))!=0))
		)
	{
		cpu_set_irq_line(0,0,HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0,CLEAR_LINE);
	}
}




void pcw_timer_interrupt(int dummy)
{
//	if (fdc_interrupt_code!=1)
//	{
		pcw_update_interrupt_counter();

		pcw_interrupt_handle();
//
//
//		if (pcw_interrupt_counter!=0)
//		{
//			cpu_set_irq_line(0,0, HOLD_LINE);
//		}

//		/* vector */
//		return 0x0ff;
//	}
//
//	return ignore_interrupt();
}


int pcw_frame_interrupt(void)
{
	return ignore_interrupt();
}

void	pcw_trigger_int(void)
{
	int state;

	state = pcw_system_status & (1<<5);

	switch (fdc_interrupt_code)
	{
		/* attach fdc to nmi */
		case 0:
		{
			if (state)
			{
#ifdef VERBOSE
				logerror("asserting nmi\r\n");
#endif
				cpu_set_nmi_line(0, ASSERT_LINE);
			}
			else
			{
#ifdef VERBOSE
				logerror("clearing nmi\r\n");
#endif
				cpu_set_nmi_line(0, CLEAR_LINE);
			}
		}
		break;

		/* attach fdc to int */
		case 1:
		{

			pcw_interrupt_handle();

			//			if (state)
//			{
//				cpu_set_irq_line(0, 0, HOLD_LINE);
//			}
//			else
//			{
//				cpu_set_irq_line(0,0, CLEAR_LINE);
//			}
		}
		break;

		/* do not interrupt */
		default:
			break;
	}
}

void pcw_fdc_interrupt(int state)
{
	pcw_system_status &= ~(1<<5);

	if (state)
	{
		/* fdc interrupt occured */
		pcw_system_status |= (1<<5);
	}

	pcw_trigger_int();
}


/* Memory is banked in 16k blocks.

  The upper 16 bytes of block 3, contains the keyboard
  state. This is updated by the hardware.

  block 3 could be paged into any bank, and this explains the
  setup of the memory below.
 */
static struct MemoryReadAddress readmem_pcw[] =
{
	{0x0000, 0x03fef, MRA_BANK1},
	{0x3ff0, 0x03fff, MRA_BANK2},

	{0x4000, 0x07fef, MRA_BANK3},
	{0x7ff0, 0x07fff, MRA_BANK4},

	{0x8000, 0x0Bfef, MRA_BANK5},
	{0xbff0, 0x0bfff, MRA_BANK6},

	{0xC000, 0x0ffef, MRA_BANK7},
	{0xfff0, 0x0ffff, MRA_BANK8},

	{0x010000, 0x013fff, MRA_ROM},	   /* OS */
	{-1}							   /* end of table */
};


static struct MemoryWriteAddress writemem_pcw[] =
{
	{0x00000, 0x03fff, MWA_BANK9},
	{0x04000, 0x07fff, MWA_BANK10},
	{0x08000, 0x0bfff, MWA_BANK11},
	{0x0c000, 0x0ffff, MWA_BANK12},
	{-1}							   /* end of table */
};

unsigned long pcw_banks[4];
unsigned char pcw_bank_force = 0;

/* PCW keyboard is mapped into memory */
READ_HANDLER(pcw_keyboard_r)
{
	return readinputport(offset);
}

static void pcw_standard_ram_read(int block, int bank)
{
	cpu_setbank((block<<1)+1,pcw_ram+(bank<<14));
	cpu_setbank((block<<1)+2, pcw_ram+(bank<<14)+0x03ff0);

	/* bank 3 ? */
	if (bank==3)
	{
		/* when upper 16 bytes are accessed use keyboard read
		handler */
		cpu_setbankhandler_r((block<<1)+2, pcw_keyboard_r);
	}
	else
	{
		mem_read_handler mra=0;

		switch ((block<<1)+2)
		{
			case 2:
			{
				mra = MRA_BANK2;
			}
			break;

			case 4:
			{
				mra = MRA_BANK4;
			}
			break;
			case 6:
			{
				mra = MRA_BANK6;
			}
			break;
			case 8:
			{
				mra = MRA_BANK8;
			}
			break;
		}

		cpu_setbankhandler_r((block<<1)+2, mra);
	}
}

/* &F4 O  b7-b4: when set, force memory reads to access the same bank as
writes for &C000, &0000, &8000, and &4000 respectively */

static void pcw_update_mem(int block, int data)
{




	/* expansion ram select.
		if block is 0-7, selects internal ram instead for read/write
		*/
	if (data & 0x080)
	{
		int bank;

		/* same bank for reading and writing */
		bank = data & 0x7f;

		if ((bank & 0x078)==0)
		{
			/* expansion bank in range 0-7 - select standard ram for read/write */
			pcw_standard_ram_read(block, bank);
			/* standard ram write */
			cpu_setbank(block+9, pcw_ram+(bank<<14));
		}
		else
		{
			/* expansion bank not in range 0-7 */
			int ram_mask;

			switch (pcw_ram_size)
			{
				default:
				case 0:
				{
					ram_mask = 0x07;
				}
				break;

				case 1:
				{
					ram_mask = 0x0f;
				}
				break;

				case 2:
				{
					ram_mask = 0x01f;
				}
				break;
			}

			/* force into range */
			bank = bank & ram_mask;

			pcw_standard_ram_read(block, bank);

			cpu_setbank(block+9, pcw_ram+(bank<<14));
		}
	}
	else
	{
		/* specify a different bank for reading and writing */
		int write_bank;
		int read_bank;
		int mask=0;

		switch (block)
		{
			case 0:
			{
				mask = (1<<6);
			}
			break;

			case 1:
			{
				mask = (1<<4);
			}
			break;

			case 2:
			{
				mask = (1<<5);
			}
			break;

			case 3:
			{
				mask = (1<<7);
			}
			break;
		}

		if (pcw_bank_force & mask)
		{
			read_bank = data & 0x07;
		}
		else
		{
			read_bank = (data>>4) & 0x07;
		}

		pcw_standard_ram_read(block, read_bank);

		write_bank = data & 0x07;
		cpu_setbank(block+9, pcw_ram+(write_bank<<14));
	}

	/* if boot is active, page in fake ROM */
	if ((pcw_boot) && (block==0))
	{
		unsigned char *FakeROM;

		FakeROM = &memory_region(REGION_CPU1)[0x010000];

		cpu_setbank(1, FakeROM);
	}
}

/* from Jacob Nevins docs */
static int pcw_get_sys_status(void)
{
	return pcw_interrupt_counter | (readinputport(16) & (0x040 | 0x010)) | pcw_system_status;
}

READ_HANDLER(pcw_interrupt_counter_r)
{
	int data;

	/* from Jacob Nevins docs */

	/* get data */
	data = pcw_get_sys_status();
	/* clear int counter */
	pcw_interrupt_counter = 0;
	/* update interrupt */
	pcw_interrupt_handle();
	/* return data */
	return data;
}


WRITE_HANDLER(pcw_bank_select_w)
{
#ifdef VERBOSE
	logerror("BANK: %2x %x\r\n",offset, data);
#endif
	pcw_banks[offset] = data;

	pcw_update_mem(offset, data);
}

WRITE_HANDLER(pcw_bank_force_selection_w)
{
	pcw_bank_force = data;

	pcw_update_mem(0, pcw_banks[0]);
	pcw_update_mem(1, pcw_banks[1]);
	pcw_update_mem(2, pcw_banks[2]);
	pcw_update_mem(3, pcw_banks[3]);
}


WRITE_HANDLER(pcw_roller_ram_addr_w)
{
	/*
	Address of roller RAM. b7-5: bank (0-7). b4-1: address / 512. */

	roller_ram_addr = (((data>>5) & 0x07)<<14) |
							((data & 0x01f)<<9);
}

WRITE_HANDLER(pcw_pointer_table_top_scan_w)
{
	roller_ram_offset = data;
}

WRITE_HANDLER(pcw_vdu_video_control_register_w)
{
	pcw_vdu_video_control_register = data;
}

WRITE_HANDLER(pcw_system_control_w)
{
#ifdef VERBOSE
	logerror("SYSTEM CONTROL: %d\r\n",data);
#endif

	switch (data)
	{
		/* end bootstrap */
		case 0:
		{
			pcw_boot = 0;
			pcw_update_mem(0, pcw_banks[0]);
		}
		break;

		/* reboot */
		case 1:
		{
		}
		break;

		/* connect fdc interrupt to nmi */
		case 2:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 0;

			/* previously connected to INT? */
			if (fdc_previous_interrupt_code == 1)
			{
				/* yes */

				pcw_interrupt_handle();
			}

			pcw_trigger_int();

#ifdef PCW_DUMP_RAM
			/* load image */
			pcw_dump_ram();
#endif
		}
		break;


		/* connect fdc interrupt to interrupt */
		case 3:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			/* connect to INT */
			fdc_interrupt_code = 1;

			/* previously connected to NMI? */
			if (fdc_previous_interrupt_code == 0)
			{
				/* yes */

				/* clear nmi interrupt */
				cpu_set_nmi_line(0,CLEAR_LINE);
			}

			/* re-issue interrupt */
			pcw_interrupt_handle();
		}
		break;


		/* connect fdc interrupt to neither */
		case 4:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 2;

			/* previously connected to NMI or INT? */
			if ((fdc_previous_interrupt_code == 0) || (fdc_previous_interrupt_code == 1))
			{
				/* yes */

				/* Clear NMI */
				cpu_set_nmi_line(0, CLEAR_LINE);

			}

			pcw_interrupt_handle();
		}
		break;

		/* set fdc terminal count */
		case 5:
		{
			nec765_set_tc_state(1);
		}
		break;

		/* clear fdc terminal count */
		case 6:
		{
			nec765_set_tc_state(0);
		}
		break;

		/* screen on */
		case 7:
		{


		}
		break;

		/* screen off */
		case 8:
		{

		}
		break;

		/* disc motor on */
		case 9:
		{
			floppy_drive_set_motor_state(1);
		}
		break;

		/* disc motor off */
		case 10:
		{
			floppy_drive_set_motor_state(0);
		}
		break;

		/* beep on */
		case 11:
		{


		}
		break;

		/* beep off */
		case 12:
		{


		}
		break;

	}
}

READ_HANDLER(pcw_system_status_r)
{
	/* from Jacob Nevins docs */
	return pcw_get_sys_status();
}

READ_HANDLER(pcw_expansion_r)
{
	/* spectravideo joystick */
	if (offset == (0x0e0-0x080))
	{
		if (readinputport(16) & 0x020)
		{
			return readinputport(17);
		}
		else
		{
			return 0x0ff;
		}
	}

	/* kempston joystick */
	if (offset == (0x09f-0x080))
	{
		return readinputport(18);
	}

	return 0x0ff;
}

WRITE_HANDLER(pcw_expansion_w)
{
}

READ_HANDLER(pcw_fdc_r)
{
	/* from Jacob Nevins docs. FDC I/O is not fully decoded */
	if (offset & 1)
	{
		return nec765_data_r(0);
	}

	return nec765_status_r(0);
}

WRITE_HANDLER(pcw_fdc_w)
{
	if (offset & 1)
	{
		nec765_data_w(0,data);
	}
}

WRITE_HANDLER(pcw_printer_data_w)
{
}

WRITE_HANDLER(pcw_printer_command_w)
{
}

READ_HANDLER(pcw_printer_data_r)
{
	return 0x0ff;
}

READ_HANDLER(pcw_printer_status_r)
{
	return 0x0ff;
}


static struct IOReadPort readport_pcw[] =
{
	{0x000, 0x07f, pcw_fdc_r},
	{0x080, 0x0ef, pcw_expansion_r},
	{0x0f4, 0x0f4, pcw_interrupt_counter_r},
	{0x0f8, 0x0f8, pcw_system_status_r},
	{0x0fc, 0x0fc, pcw_printer_data_r},
	{0x0fd, 0x0fd, pcw_printer_status_r},
	{-1}							   /* end of table */
};


static struct IOWritePort writeport_pcw[] =
{
	{0x000, 0x07f, pcw_fdc_w},
	{0x080, 0x0ef, pcw_expansion_w},
	{0x0f0, 0x0f3, pcw_bank_select_w},
	{0x0f4, 0x0f4, pcw_bank_force_selection_w},
	{0x0f5, 0x0f5, pcw_roller_ram_addr_w},
	{0x0f6, 0x0f6, pcw_pointer_table_top_scan_w},
	{0x0f7, 0x0f7, pcw_vdu_video_control_register_w},
	{0x0f8, 0x0f8, pcw_system_control_w},
	{0x0fc, 0x0fc, pcw_printer_data_w},
	{0x0fd, 0x0fd, pcw_printer_command_w},
	{-1}							   /* end of table */
};

void pcw_init_machine(void)
{

	pcw_ram = NULL;

	pcw_ram = malloc(512*1024);

	pcw_boot = 1;

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
	cpu_setbankhandler_r(3, MRA_BANK3);
	cpu_setbankhandler_r(4, MRA_BANK4);
	cpu_setbankhandler_r(5, MRA_BANK5);
	cpu_setbankhandler_r(6, MRA_BANK6);
	cpu_setbankhandler_r(7, MRA_BANK7);
	cpu_setbankhandler_r(8, MRA_BANK8);

	cpu_setbankhandler_w(9, MWA_BANK9);
	cpu_setbankhandler_w(10, MWA_BANK10);
	cpu_setbankhandler_w(11, MWA_BANK11);
	cpu_setbankhandler_w(12, MWA_BANK12);


	cpu_0_irq_line_vector_w(0, 0x0ff);

	nec765_init(&pcw_nec765_interface);

	/* ram paging is actually undefined at power-on */
	pcw_banks[0] = 0;
	pcw_banks[1] = 1;
	pcw_banks[2] = 2;
	pcw_banks[3] = 3;

	pcw_update_mem(0, pcw_banks[0]);
	pcw_update_mem(1, pcw_banks[1]);
	pcw_update_mem(2, pcw_banks[2]);
	pcw_update_mem(3, pcw_banks[3]);

	/* lower 4 bits are interrupt counter */
	pcw_system_status = 0x000;
	pcw_system_status &= ~((1<<6) | (1<<5) | (1<<4));

	pcw_interrupt_counter = 0;

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);
	floppy_drive_set_geometry(1, FLOPPY_DRIVE_DS_80);

	roller_ram_offset = 0;

	pcw_int_timer = timer_pulse(TIME_IN_HZ(300), 0, pcw_timer_interrupt);
}


void pcw_shutdown_machine(void)
{
	if (pcw_ram!=NULL)
	{
		free(pcw_ram);
	}

	if (pcw_int_timer)
	{
		timer_remove(pcw_int_timer);
	}

}


/*
b7:   k2     k1     [+]    .      ,      space  V      X      Z      del<   alt
b6:   k3     k5     1/2    /      M      N      B      C      lock          k.
b5:   k6     k4     shift  ;      K      J      F      D      A             enter
b4:   k9     k8     k7     �      L      H      G      S      tab           f8
b3:   paste  copy   #      P      I      Y      T      W      Q             [-]
b2:   f2     cut    return [      O      U      R      E      stop          can
b1:   k0     ptr    ]      -      9      7      5      3      2             extra
b0:   f4     exit   del>   =      0      8      6      4      1             f6
      &3FF0  &3FF1  &3FF2  &3FF3  &3FF4  &3FF5  &3FF6  &3FF7  &3FF8  &3FF9  &3FFA
*/

INPUT_PORTS_START(pcw)
	/* keyboard "ports". These are poked automatically into the PCW address space */

	/* 0x03ff0 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K0", KEYCODE_0_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PASTE", KEYCODE_PGUP, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K9", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K6", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K3", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K2", KEYCODE_2_PAD, IP_JOY_NONE)

	/* 0x03ff1 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "EXIT", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PTR", KEYCODE_PGUP, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CUT", KEYCODE_INSERT, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "COPY", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K8", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K4", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K5", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K1", KEYCODE_1_PAD, IP_JOY_NONE)

	/* 0x03ff2 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL>", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "#", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K7", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
/*	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, " ", KEYCODE_NONE, IP_JOY_NONE)*/
	PORT_BIT (0x040, 0x000, IPT_UNUSED)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+", KEYCODE_PLUS_PAD, IP_JOY_NONE)

	/* 0x03ff3 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "�", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)

	/* 0x03ff4 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)

	/* 0x03ff5 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, " ", KEYCODE_SPACE, IP_JOY_NONE)

	/* 0x03ff6 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)

	/* 0x03ff7 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)

	/* 0x03ff8 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STOP", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LOCK", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)

	/* 0x03ff9 */
	PORT_START
	PORT_BIT(0x07f,0x00, IPT_UNUSED)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL<", KEYCODE_BACKSPACE, IP_JOY_NONE)

	/* 0x03ffa */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F6", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BIT (0x02, 0x00, IPT_UNUSED)
	PORT_BIT (0x04, 0x00, IPT_UNUSED)
/*	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, " ", KEYCODE_NONE, IP_JOY_NONE)*/
/*	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, " ", KEYCODE_NONE, IP_JOY_NONE)*/
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F8", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER_PAD, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K.", KEYCODE_DEL_PAD, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_RALT, IP_JOY_NONE)

	/* at this point the following reflect the above key combinations but in a incomplete
	way. No details available at this time */
	/* 0x03ffb */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03ffc */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03ffd */
	PORT_START
	PORT_BIT ( 0x03f, 0x000, IPT_UNUSED)
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BIT (0x080, 0x000, IPT_UNUSED)

	/* 0x03ffe */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03fff */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* from here on are the pretend dipswitches for machine config etc */
	PORT_START
	/* vblank */
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* frame rate option */
	PORT_BITX( 0x010, 0x010, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "50/60Hz Frame Rate Option", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "60hz")
	PORT_DIPSETTING(	0x10, "50Hz" )
	/* spectravideo joystick enabled */
	PORT_BITX( 0x020, 0x020, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Spetravideo Joystick Enabled", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No))
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )

	/* Spectravideo joystick */
	/* bit 7: 0
    6: 1
    5: 1
    4: 1 if in E position
    3: 1 if N
    2: 1 if W
    1: 1 if fire pressed
    0: 1 if S
	*/

	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOYSTICK DOWN", IP_KEY_NONE, JOYCODE_1_DOWN)
	PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOYSTICK FIRE", IP_KEY_NONE, JOYCODE_1_BUTTON1)
	PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOYSTICK LEFT", IP_KEY_NONE, JOYCODE_1_LEFT)
	PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOYSTICK UP", IP_KEY_NONE, JOYCODE_1_UP)
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOYSTICK RIGHT", IP_KEY_NONE, JOYCODE_1_RIGHT)
	PORT_BIT (0x020, 0x020, IPT_UNUSED)
	PORT_BIT (0x040, 0x040, IPT_UNUSED)
	PORT_BIT (0x080, 0x00, IPT_UNUSED)

INPUT_PORTS_END

static struct MachineDriver machine_driver_pcw =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			4000000,	/* clock supplied to chip, but in reality it is 3,4Mhz */
			readmem_pcw,		   /* MemoryReadAddress */
			writemem_pcw,		   /* MemoryWriteAddress */
			readport_pcw,		   /* IOReadPort */
			writeport_pcw,		   /* IOWritePort */
			0,
			1,
			0, 0,
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION /*DEFAULT_60HZ_VBLANK_DURATION*/,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	pcw_init_machine,			   /* init machine */
	pcw_shutdown_machine,
	/* video hardware */
	PCW_SCREEN_WIDTH,			   /* screen width */
	PCW_SCREEN_HEIGHT,			   /* screen height */
	{0, (PCW_SCREEN_WIDTH - 1), 0, (PCW_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
	PCW_NUM_COLOURS, 							   /* total colours */
	PCW_NUM_COLOURS, 							   /* color table len */
	pcw_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,				   /* video attributes */
	0,								   /* MachineLayer */
	pcw_vh_start,
	pcw_vh_stop,
	pcw_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* I am loading the roms outside of the Z80 memory area, because they
are banked. */
ROM_START(pcw)
	ROM_REGION(0x014000, REGION_CPU1)
	//ROM_LOAD("pcwboot.bin", 0x010000, 490, 0x0)
	ROM_LOAD("pcwboot.bin", 0x010000, 608, 0x0)
ROM_END

static const struct IODevice io_pcw[] =
{
	{
		IO_FLOPPY,					/* type */
		2,							/* count */
		"dsk\0",                    /* file extensions */
		NULL,						/* private */
		dsk_floppy_id,			/* id */
		dsk_floppy_load,		/* init */
		dsk_floppy_exit,		/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
		NULL,						/* status */
        NULL,                       /* seek */
		NULL,						/* tell */
        NULL,                       /* input */
		NULL,						/* output */
		NULL,						/* input_chunk */
		NULL						/* output_chunk */
	},
	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 198?, pcw,   0,	pcw,  pcw,	0,	 "Amstrad plc", "PCW")
