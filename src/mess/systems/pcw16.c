/******************************************************************************

	pcw16.c
	system driver

	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "mess/vidhrdw/pcw16.h"

#define PCW16_NVRAM_SIZE	(2048*1024)

/* ram - up to 2mb */
unsigned char *pcw16_ram;
/* nvram - up to 2mb */
unsigned char *pcw16_nvram;

void	 *pcw16_timer;

extern int pcw16_video_control;
/* controls which bank of 2mb address space is paged into memory */
static int pcw16_banks[4];

unsigned char pcw16_keyboard_status;
unsigned char pcw16_keyboard_control;
unsigned char pcw16_keyboard_data_shift[2];
unsigned long pcw16_interrupt_counter;
static int pcw16_4_bit_port;
static int pcw16_fdc_int_code;
static int pcw16_system_status;

void pcw16_refresh_ints(void)
{
	/* any bits set excluding vsync */
	if ((pcw16_system_status & (~0x04))!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}


void pcw16_timer_callback(int dummy)
{
	/* do not increment past 15 */
	if (pcw16_interrupt_counter!=15)
	{
		pcw16_interrupt_counter++;
	}

	if (pcw16_interrupt_counter!=0)
	{
		pcw16_refresh_ints();
	}
}


/* read/write the nvram - holds cabinet and installed os */
void	pcw16_nvram_handler(void *file, int read_or_write)
{
	if ((file==0) && (read_or_write==0))
	{
		if (pcw16_nvram!=NULL)
		{
			/* fill in the default values */
			memset(pcw16_nvram,0,PCW16_NVRAM_SIZE);
		}
	}
	else
	{
		if (file!=0)
		{
			if (read_or_write == 0)
			{
				if (pcw16_nvram!=NULL)
				{
					osd_fread(file, pcw16_nvram, PCW16_NVRAM_SIZE);
				}

			}
			else
			{
				if (pcw16_nvram!=NULL)
				{
					osd_fwrite(file,pcw16_nvram,PCW16_NVRAM_SIZE);
				}
			}
		}
	}
}


static struct MemoryReadAddress readmem_pcw16[] =
{
	{0x0000, 0x03fff, MRA_BANK1},
	{0x4000, 0x07fff, MRA_BANK2},
	{0x8000, 0x0Bfff, MRA_BANK3},
	{0xC000, 0x0ffff, MRA_BANK4},
	{-1}							   /* end of table */
};

extern int pcw16_colour_palette[16];

WRITE_HANDLER(pcw16_palette_w)
{
	pcw16_colour_palette[offset & 0x0f] = data & 31;
}

static void pcw16_update_bank(int bank)
{
	unsigned char *ram_ptr = pcw16_ram;
	int bank_id = 0;
	int bank_offs = 0;

	/* get memory bank */
	bank_id = pcw16_banks[bank];

	if (bank_id<128)
	{
		bank_offs = 0;

		if (bank_id<4)
		{
			/* lower 4 banks are write protected. Use the rom
			loaded */
			ram_ptr = &memory_region(REGION_CPU1)[0x010000];
		}
		else
		{
			/* nvram */
			ram_ptr = pcw16_nvram;
		}
	}
	else
	{
		bank_offs = 128;
			/* dram */
			ram_ptr = pcw16_ram;
	}

	ram_ptr = ram_ptr + ((bank_id - bank_offs)<<14);
	cpu_setbank((bank+1), ram_ptr);
	cpu_setbank((bank+5), ram_ptr);

	/* selections 0-3 within the first 64k are write protected */
	if (bank_id<4)
	{
		cpu_setbankhandler_w((bank+5), MWA_NOP);
	}
	else
	{
		mem_write_handler mwa=0;

		switch (bank)
		{
			case 0:
			{
				mwa = MWA_BANK5;
			}
			break;

			case 1:
			{
				mwa = MWA_BANK6;
			}
			break;

			case 2:
			{
				mwa = MWA_BANK7;
			}
			break;
			case 3:
			{
				mwa = MWA_BANK8;
			}
			break;
		}

		cpu_setbankhandler_w((bank+5), mwa);
	}

}


/* update memory h/w */
static void pcw16_update_memory(void)
{
	pcw16_update_bank(0);
	pcw16_update_bank(1);
	pcw16_update_bank(2);
	pcw16_update_bank(3);

}

READ_HANDLER(pcw16_bankhw_r)
{
	logerror("bank r: %d \r\n", offset);

	return pcw16_banks[offset];
}

WRITE_HANDLER(pcw16_bankhw_w)
{
	logerror("bank w: %d block: %02x\r\n", offset, data);

	pcw16_banks[offset] = data;

	pcw16_update_memory();
}

WRITE_HANDLER(pcw16_video_control_w)
{
	logerror("video control w: %02x\r\n", data);

	pcw16_video_control = data;
}

READ_HANDLER(pcw16_keyboard_data_shift_r)
{
	return pcw16_keyboard_data_shift[0];
}

WRITE_HANDLER(pcw16_keyboard_data_shift_w)
{
	pcw16_keyboard_data_shift[1] = data;
}

READ_HANDLER(pcw16_keyboard_status_r)
{
	return pcw16_keyboard_status;
}

WRITE_HANDLER(pcw16_keyboard_control_w)
{
	pcw16_keyboard_control = data;
}


static struct MemoryWriteAddress writemem_pcw16[] =
{
	{0x00000, 0x03fff, MWA_BANK5},
	{0x04000, 0x07fff, MWA_BANK6},
	{0x08000, 0x0bfff, MWA_BANK7},
	{0x0c000, 0x0ffff, MWA_BANK8},
	{-1}							   /* end of table */
};

static unsigned char rtc_seconds;
static unsigned char rtc_minutes;
static unsigned char rtc_hours;
static unsigned char rtc_days_max;
static unsigned char rtc_days;
static unsigned char rtc_months;
static unsigned char rtc_years;
static unsigned char rtc_control;
static unsigned char rtc_256ths_seconds;

static int rtc_days_in_each_month[]=
{
	31,/* jan */
	28, /* feb */
	31, /* march */
	30, /* april */
	31, /* may */
	30, /* june */
	31, /* july */
	31, /* august */
	30, /* september */
	31, /* october */
	30, /* november */
	31	/* december */
};

static int rtc_days_in_february[] =
{
	29, 28, 28, 28
};

static void rtc_setup_max_days(void)
{
	/* february? */
	if (rtc_months == 2)
	{
		/* low two bits of year select number of days in february */
		rtc_days_max = rtc_days_in_february[rtc_years & 0x03];
	}
	else
	{
		rtc_days_max = (unsigned char)rtc_days_in_each_month;
	}
}

static void rtc_timer_callback(int dummy)
{
	int fraction_of_second;

	/* halt counter? */
	if ((rtc_control & 0x01)!=0)
	{
		/* no */

		/* increment 256th's of a second register */
		fraction_of_second = rtc_256ths_seconds+1;
		/* add bit 8 = overflow */
		rtc_seconds+=(fraction_of_second>>8);
		/* ensure counter is in range 0-255 */
		rtc_256ths_seconds = fraction_of_second & 0x0ff;
	}

	if (rtc_seconds>59)
	{
		rtc_seconds = 0;

		rtc_minutes++;

		if (rtc_minutes>59)
		{
			rtc_minutes = 0;

			rtc_hours++;

			if (rtc_hours>23)
			{
				rtc_hours = 0;

				rtc_days++;

				if (rtc_days>rtc_days_max)
				{
					rtc_days = 1;

					rtc_months++;

					if (rtc_months>12)
					{
						rtc_months = 1;

						/* 7 bit year counter */
						rtc_years = (rtc_years + 1) & 0x07f;

					}

					rtc_setup_max_days();
				}

			}


		}
	}
}

READ_HANDLER(rtc_year_invalid_r)
{
	/* year in lower 7 bits. RTC Invalid status is rtc_control bit 0
	inverted */
	return (rtc_years & 0x07f) | (((rtc_control & 0x01)<<7)^0x080);
}

READ_HANDLER(rtc_month_r)
{
	return rtc_months;
}

READ_HANDLER(rtc_days_r)
{
	return rtc_days;
}

READ_HANDLER(rtc_hours_r)
{
	return rtc_hours;
}

READ_HANDLER(rtc_minutes_r)
{
	return rtc_minutes;
}

READ_HANDLER(rtc_seconds_r)
{
	return rtc_seconds;
}

READ_HANDLER(rtc_256ths_seconds_r)
{
	return rtc_256ths_seconds;
}

WRITE_HANDLER(rtc_control_w)
{
	/* write control */
	rtc_control = data;
}

WRITE_HANDLER(rtc_seconds_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_seconds = data;
}

WRITE_HANDLER(rtc_minutes_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_minutes = data;
}

WRITE_HANDLER(rtc_hours_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;
}

WRITE_HANDLER(rtc_days_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_days = data;
}

WRITE_HANDLER(rtc_month_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_months = data;

	rtc_setup_max_days();
}


WRITE_HANDLER(rtc_year_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;

	rtc_setup_max_days();
}

static void pcw16_trigger_fdc_int(void)
{
	int state;

	state = pcw16_system_status & (1<<6);

	switch (pcw16_fdc_int_code)
	{
		/* nmi */
		case 0:
		{
			if (state)
			{
				cpu_set_nmi_line(0, ASSERT_LINE);
			}
			else
			{
				cpu_set_nmi_line(0, CLEAR_LINE);
			}
		}
		break;

		/* attach fdc to int */
		case 1:
		{
		}
		break;

		/* do not interrupt */
		default:
			break;
	}
}

READ_HANDLER(pcw16_system_status_r)
{
	logerror("system status r: \r\n");

	return pcw16_system_status | (readinputport(0) & 0x04);
}

READ_HANDLER(pcw16_timer_interrupt_counter_r)
{
	int data;

	data = pcw16_interrupt_counter;

	pcw16_interrupt_counter = 0;
	pcw16_refresh_ints();

	return data;
}


WRITE_HANDLER(pcw16_system_control_w)
{
	logerror("0x0f8: function: %d\r\n",data);

	/* lower 4 bits define function code */
	switch (data & 0x0f)
	{
		/* no effect */
		case 0x00:
		case 0x09:
		case 0x0a:
		case 0x0d:
		case 0x0e:
			break;

		/* system reset */
		case 0x01:
			break;

		/* connect IRQ6 input to /NMI */
		case 0x02:
		{
			pcw16_fdc_int_code = 0;
		}
		break;

		/* connect IRQ6 input to /INT */
		case 0x03:
		{
			pcw16_fdc_int_code = 1;
		}
		break;

		/* dis-connect IRQ6 input from /NMI and /INT */
		case 0x04:
		{
			pcw16_fdc_int_code = 2;
		}
		break;

		/* bleeper on */
		case 0x0b:
		{
		}
		break;

		/* bleeper off */
		case 0x0c:
		{
		}
		break;

		/* drive video outputs */
		case 0x07:
		{
		}
		break;

		/* float video outputs */
		case 0x08:
		{
		}
		break;

		/* set 4-bit output port to value X */
		case 0x0f:
		{
			pcw16_4_bit_port = data>>4;
		}
		break;
	}
}

static struct IOReadPort readport_pcw16[] =
{
	{0x0f0, 0x0f3, pcw16_bankhw_r},
		/*
	{0x0f4, 0x0f4, pcw16_keyboard_data_shift_r},
	{0x0f5, 0x0f5, pcw16_keyboard_status_r},
	{0x0f7, 0x0f7, pcw16_timer_interrupt_counter_r},
	{0x0f8, 0x0f8, pcw16_system_status_r},*/
	{0x0f9, 0x0f9, rtc_256ths_seconds_r},
	{0x0fa, 0x0fa, rtc_seconds_r},
	{0x0fb, 0x0fb, rtc_minutes_r},
	{0x0fc, 0x0fc, rtc_hours_r},
	{0x0fd, 0x0fd, rtc_days_r},
	{0x0fe, 0x0fe, rtc_month_r},
	{0x0ff, 0x0ff, rtc_year_invalid_r},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_pcw16[] =
{
	{0x0e0, 0x0ef, pcw16_palette_w},
	{0x0f0, 0x0f3, pcw16_bankhw_w},
	{0x0f7, 0x0f7, pcw16_video_control_w},
/*	{0x0f4, 0x0f4, pcw16_keyboard_data_shift_w},
	{0x0f5, 0x0f5, pcw16_keyboard_control_w},*/
	{0x0f8, 0x0f8, pcw16_system_control_w},
	{0x0f9, 0x0f9, rtc_control_w},
	{0x0fa, 0x0fa, rtc_seconds_w},
	{0x0fb, 0x0fb, rtc_minutes_w},
	{0x0fc, 0x0fc, rtc_hours_w},
	{0x0fd, 0x0fd, rtc_days_w},
	{0x0fe, 0x0fe, rtc_month_w},
	{0x0ff, 0x0ff, rtc_year_w},
	{-1}							   /* end of table */
};

void pcw16_init_machine(void)
{
	pcw16_ram = NULL;

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
	cpu_setbankhandler_r(3, MRA_BANK3);
	cpu_setbankhandler_r(4, MRA_BANK4);

	cpu_setbankhandler_w(5, MWA_BANK5);
	cpu_setbankhandler_w(6, MWA_BANK6);
	cpu_setbankhandler_w(7, MWA_BANK7);
	cpu_setbankhandler_w(8, MWA_BANK8);



	/* dram */
	pcw16_ram = malloc(2048*1024);
	/* nvram */
	pcw16_nvram = malloc(2048*1024);

	if (pcw16_nvram!=NULL)
	{
		pcw16_nvram[0] = 0x0ff;
		memset(pcw16_nvram, 0, 2048*1024);
	}

	pcw16_banks[0] = 0;
	pcw16_update_memory();

	pcw16_system_status = 0;
	pcw16_interrupt_counter = 0;
	pcw16_timer = timer_pulse(TIME_IN_MSEC(5.83), 0,pcw16_timer_callback);
}

void pcw16_shutdown_machine(void)
{
	if (pcw16_ram!=NULL)
	{
		free(pcw16_ram);
	}

	if (pcw16_nvram!=NULL)
	{
		free(pcw16_nvram);
	}

	if (pcw16_timer)
	{
		timer_remove(pcw16_timer);
	}

}

INPUT_PORTS_START(pcw16)
	PORT_START
	/* vblank */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
INPUT_PORTS_END

static struct MachineDriver machine_driver_pcw16 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			16000000,
			readmem_pcw16,		   /* MemoryReadAddress */
			writemem_pcw16,		   /* MemoryWriteAddress */
			readport_pcw16,		   /* IOReadPort */
			writeport_pcw16,		   /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
			0, 0,	/* every scanline */
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	pcw16_init_machine,			   /* init machine */
	pcw16_shutdown_machine,
	/* video hardware */
	PCW16_SCREEN_WIDTH,			   /* screen width */
	PCW16_SCREEN_HEIGHT,			   /* screen height */
	{0, (PCW16_SCREEN_WIDTH - 1), 0, (PCW16_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
	PCW16_NUM_COLOURS, 							   /* total colours */
	PCW16_NUM_COLOURS, 							   /* color table len */
	pcw16_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	pcw16_vh_start,
	pcw16_vh_stop,
	pcw16_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
		{
			0
		}
	},
	pcw16_nvram_handler
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* the lower 64k of the nram is write protected. This contains the boot
	rom. The boot rom is also on the OS rescue disc. Handy! */
ROM_START(pcw16)
	ROM_REGION((0x010000+524288), REGION_CPU1)
	ROM_LOAD("pcw045.sys",0x10000, 524288, 0x000000)
ROM_END

static const struct IODevice io_pcw16[] =
{
	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 1995, pcw16,   0,	pcw16,  pcw16,	0,	 "Amstrad plc", "PCW16")
