/******************************************************************************

        nc.c

        NC100/NC150/NC200 Notepad computer 

        system driver


        Documentation:
        
		NC100:
			NC100 I/O Specification by Cliff Lawson,
			NC100EM by Russell Marks
		NC200:
			Dissassembly of the NC200 ROM + e-mail
			exchange with Russell Marks


		NC100:

        Hardware:
            - Z80 CPU
            - memory powered by lithium batterys!
            - 2 channel tone (programmable frequency beep's)
            - LCD screen
            - laptop/portable computer
            - qwerty keyboard
            - serial/parallel connection
            - Amstrad custom ASIC chip
			- tc8521 real time clock
			- intel 8251 compatible uart


		NC200:

        Hardware:
			- Z80 CPU
			- Intel 8251 compatible uart
            - nec765 compatible floppy disc controller
			- mc146818 real time clock

        TODO:
           - find out what the unused key bits are for
           - serial, parallel and loads more!!!
           - overlay would be nice!

		Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/nc.h"
/* for NC100 real time clock */
#include "includes/tc8521.h"
/* for NC100 uart */
#include "includes/msm8251.h"
/* for NC200 real time clock */
#include "includes/mc146818.h"
/* for NC200 disk drive interface */
#include "includes/nec765.h"
/* for NC200 disk image */
#include "includes/pc_flopp.h"

/* uncomment for verbose debugging information */
//#define VERBOSE

static unsigned long nc_memory_size;
UINT8 nc_type;

static char nc_memory_config[4];
unsigned long nc_display_memory_start;
static void *nc_keyboard_timer = NULL;
static void *dummy_timer = NULL;

static int nc_membank_rom_mask;
static int nc_membank_internal_ram_mask;
static int nc_membank_card_ram_mask;
/*
        bit 7: memory card present 0 = yes, 1 = no
        bit 6: memory card write protected 1 = yes, 0 = no
        bit 5: input voltage = 1, if >= to 4 volts
        bit 4: mem card battery: 0 = battery low
        bit 3: alkaline batteries. 0 if >=3.2 volts
        bit 2: lithium battery 0 if >= 2.7 volts
        bit 1: parallel interface busy (0 if busy)
        bit 0: parallel interface ack (1 if ack);
*/
static int nc_card_battery_status=0x0c0;

static UINT8 nc_poweroff_control;

void nc_set_card_present_state(int state)
{
        if (state)
        {
                /* if bit 7 = 0, card is present */
                nc_card_battery_status &= ~(1<<7);
        }
        else
        {
                nc_card_battery_status |= (1<<7);
        }
}

void    nc_set_card_write_protect_state(int state)
{
        if (state)
        {
                /* bit 6 = 1 if card is write protected */
                nc_card_battery_status |= (1<<6);
        }
        else
        {
                nc_card_battery_status &= ~(1<<6);
        }
}




/* internal ram */
unsigned char    *nc_memory;
/* card ram */
extern unsigned char    *nc_card_ram;



/* 
  bit 7     select card register 1=common, 0=attribute
        bit 6     parallel interface Strobe signal
        bit 5     Not Used
        bit 4     uPD4711 line driver, 1=off, 0=on
        bit 3     UART clock and reset, 1=off, 0=on

        bits 2-0  set the baud rate as follows

                000 = 150
                001 = 300
                010 = 600
                011 = 1200
                100 = 2400
                101 = 4800
                110 = 9600
                111 = 19200
*/

unsigned char nc_uart_control;


/*
NC100:
bits 7: not used
bits 5: NC100: not used. NC200: FDC interrupt
bits 4: Not used
Bit 3: Key scan interrupt (10ms)
Bit 2: ACK from parallel interface
Bit 1: Tx Ready
Bit 0: Rx Ready
*/
static int nc_irq_mask;
static int nc_irq_status;
static int nc_sound_channel_periods[2];

static unsigned char previous_on_off_button_state;

static void nc_update_interrupts(void)
{
        /* any ints set and they are not masked? */
        if (
                (((nc_irq_status & nc_irq_mask) & 0x3f)!=0) 
                )
        {
                /* set int */
                cpu_set_irq_line(0,0, HOLD_LINE);
        }
        else
        {
                /* clear int */
                cpu_set_irq_line(0,0, CLEAR_LINE);
        }
}

static void nc_keyboard_timer_callback(int dummy)
{
#ifdef VERBOSE
		logerror("keyboard int\n");
#endif

        /* set int */
        nc_irq_status |= (1<<3);

        /* update ints */
        nc_update_interrupts();

        /* don't trigger again, but don't free it */
        timer_reset(nc_keyboard_timer, TIME_NEVER);
}

static void dummy_timer_callback(int dummy)
{
    unsigned char on_off_button_state;

    on_off_button_state = readinputport(10) & 0x01;

    if (on_off_button_state^previous_on_off_button_state)
    {
        if (on_off_button_state)
        {
#ifdef VERBOSE
            logerror("nmi triggered\n");
#endif
            cpu_set_nmi_line(0, PULSE_LINE);
        }
    }

    previous_on_off_button_state = on_off_button_state;
}



static mem_read_handler nc_bankhandler_r[]={
MRA_BANK1, MRA_BANK2, MRA_BANK3, MRA_BANK4};

static mem_write_handler nc_bankhandler_w[]={
MWA_BANK5, MWA_BANK6, MWA_BANK7, MWA_BANK8};



static void nc_refresh_memory_bank_config(int bank)
{
        int mem_type;
        int mem_bank;

        mem_type = (nc_memory_config[bank]>>6) & 0x03;
        mem_bank = nc_memory_config[bank] & 0x03f;

        cpu_setbankhandler_r(bank+1, nc_bankhandler_r[bank]);

        switch (mem_type)
        {
                /* ROM */
                case 0:
                {
                   unsigned char *addr;

                   mem_bank = mem_bank & nc_membank_rom_mask;

                   addr = (memory_region(REGION_CPU1)+0x010000) + (mem_bank<<14);

                   cpu_setbank(bank+1, addr);

                   cpu_setbankhandler_w(bank+5, MWA_NOP);
#ifdef VERBOSE
                   logerror("BANK %d: ROM %d\n",bank,mem_bank);
#endif
                }
                break;

                /* internal RAM */
                case 1:
                {
                   unsigned char *addr;

                   mem_bank = mem_bank & nc_membank_internal_ram_mask;

                   addr = nc_memory + (mem_bank<<14);

                   cpu_setbank(bank+1, addr);
                   cpu_setbank(bank+5, addr);

                   cpu_setbankhandler_w(bank+5, nc_bankhandler_w[bank]);
#ifdef VERBOSE
                   logerror("BANK %d: RAM\n",bank);
#endif
                }
                break;

                /* card RAM */
                case 2:
                {
                   /* card connected? */
                   if ((nc_card_battery_status & (1<<7))==0)
                   {
        
                           unsigned char *addr;
        
                           addr = nc_card_ram + (mem_bank<<14);
        
                           cpu_setbank(bank+1, addr);
                           cpu_setbank(bank+5, addr);
        
                           cpu_setbankhandler_w(bank+5, nc_bankhandler_w[bank]);
#ifdef VERBOSE        
                           logerror("BANK %d: CARD-RAM\n",bank);
#endif
				   }
                    else
                    {
                        /* if no card connected, then writes fail */
                           cpu_setbankhandler_r(bank+1, MRA_NOP);
                           cpu_setbankhandler_w(bank+1, MWA_NOP);
                    }
                }
                break;

                /* ?? */
                default:
                case 3:
                {
#ifdef VERBOSE
					logerror("Invalid memory selection\n");
#endif
                }
                break;

                
        }


}


static void nc_refresh_memory_config(void)
{
        nc_refresh_memory_bank_config(0);
        nc_refresh_memory_bank_config(1);
        nc_refresh_memory_bank_config(2);
        nc_refresh_memory_bank_config(3);
}



static int previous_alarm_state;

void	nc_tc8521_alarm_callback(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */
	
	if (state!=previous_alarm_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	previous_alarm_state = state;
}

static void nc100_txrdy_callback(int state)
{
	nc_irq_status &= ~(1<<1);

	if (state)
	{
		nc_irq_status |= (1<<1);
	}

	nc_update_interrupts();
}

static void nc100_rxrdy_callback(int state)
{
	nc_irq_status &= ~(1<<0);

	if (state)
	{
		nc_irq_status |= (1<<0);
	}

	nc_update_interrupts();
}


static struct tc8521_interface nc100_tc8521_interface=
{
  nc_tc8521_alarm_callback,
};

static struct msm8251_interface nc100_uart_interface=
{
	nc100_txrdy_callback,
	NULL,
	nc100_rxrdy_callback
};


void nc_common_init_machine(void)
{
        
        void *file;

        nc_display_memory_start = 0;

        nc_memory_config[0] = 0;
        nc_memory_config[1] = 0;
        nc_memory_config[2] = 0;
        nc_memory_config[3] = 0;

        previous_on_off_button_state = readinputport(10) & 0x01;

        /* ints are masked */
        nc_irq_mask = 0;
        /* no ints wanting servicing */
        nc_irq_status = 0;
        /* at reset set to 0x0ffff */
        nc_sound_channel_periods[0] = (nc_sound_channel_periods[1] = 0x0ffff);

        /* at reset set to 1 */
        nc_poweroff_control = 1;

        nc_refresh_memory_config();
                                            
        /* enough power - see bit assignments where
        nc card battery status is defined */
        /* keep card status bits in case card has been inserted and
        the machine is then reset! */
        nc_card_battery_status &= ~((1<<7) | (1<<6));

        nc_card_battery_status |= (1<<5) | (1<<4);

/*        nc_set_card_present_state(0); */
        
        nc_keyboard_timer = timer_set(TIME_IN_MSEC(10), 0, nc_keyboard_timer_callback);

        dummy_timer = timer_pulse(TIME_IN_HZ(50), 0, dummy_timer_callback);

        /* 256k of rom */
        nc_membank_rom_mask = 0x0f;

        if (nc_memory!=NULL)
        {
                char filename[13];

                sprintf(filename,"%s.nv", Machine->gamedrv->name);

                /* restore nc memory from file */
                file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, OSD_FOPEN_READ);
        
                if (file!=NULL)
                {
#ifdef VERBOSE
					logerror("restoring nc memory\n");
#endif
					osd_fread(file, nc_memory, nc_memory_size);
                   osd_fclose(file);
                }
        }

		nc_uart_control = 0x0ff;


		msm8251_init(&nc100_uart_interface);
}

void nc100_init_machine(void)
{
        nc_type = NC_TYPE_1xx;

        nc_memory_size = 64*1024;

        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 3;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();

	    tc8521_init(&nc100_tc8521_interface);
}

#if 0
void nc150_init_machine(void)
{
        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 7;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();
}
#endif

static void nc200_fdc_interrupt(int state)
{
        nc_irq_status &=~(1<<5);

        if (state)
        {
                nc_irq_status |=(1<<5);
        }

        nc_update_interrupts();
}

static struct nec765_interface nc200_nec765_interface=
{
        nc200_fdc_interrupt,
        NULL,
};

void nc200_init_machine(void)
{
        nc_type = NC_TYPE_200;

        nc_memory_size = 128*1024;
        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 7;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();

        floppy_drives_init();
        nec765_init(&nc200_nec765_interface, NEC765A);
        floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);
        floppy_drive_set_motor_state(0,1);
        floppy_drive_set_ready_state(0,1,0);

		mc146818_init(MC146818_STANDARD);
}

void nc_common_shutdown_machine(void)
{
        
		msm8251_stop();

        if (nc_memory!=NULL)
        {
                /* write nc memory to file */
                void *file;
                char filename[13];

                sprintf(filename,"%s.nv", Machine->gamedrv->name);

                file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, OSD_FOPEN_WRITE);

                if (file!=NULL)
                {
#ifdef VERBOSE
					logerror("writing nc memory!\n");
#endif
					osd_fwrite(file, nc_memory, nc_memory_size);
                   osd_fclose(file);
                }

                free(nc_memory);
                nc_memory = NULL;
        }


        if (nc_keyboard_timer!=NULL)
        {
                timer_remove(nc_keyboard_timer);
                nc_keyboard_timer = NULL;
        }

        if (dummy_timer!=NULL)
        {
                timer_remove(dummy_timer);
                dummy_timer = NULL;
        }
}

void	nc100_shutdown_machine(void)
{
	nc_common_shutdown_machine();
    tc8521_stop();
}


void	nc200_shutdown_machine(void)
{
	nc_common_shutdown_machine();
	mc146818_close();
}


static struct MemoryReadAddress readmem_nc[] =
{
        {0x00000, 0x03fff, MRA_BANK1},
        {0x04000, 0x07fff, MRA_BANK2},
        {0x08000, 0x0bfff, MRA_BANK3},
        {0x0c000, 0x0ffff, MRA_BANK4},
	{-1}							   /* end of table */
};


static struct MemoryWriteAddress writemem_nc[] =
{
        {0x00000, 0x03fff, MWA_BANK5},
        {0x04000, 0x07fff, MWA_BANK6},
        {0x08000, 0x0bfff, MWA_BANK7},
        {0x0c000, 0x0ffff, MWA_BANK8},
	{-1}							   /* end of table */
};


READ_HANDLER(nc_memory_management_r)
{
        return nc_memory_config[offset];
}

WRITE_HANDLER(nc_memory_management_w)
{
#ifdef VERBOSE
	logerror("Memory management W: %02x %02x\n",offset,data);
#endif
        nc_memory_config[offset] = data;

        nc_refresh_memory_config();
}

WRITE_HANDLER(nc_irq_mask_w)
{
#ifdef VERBOSE
	logerror("irq mask w: %02x\n", data);
#endif

        nc_irq_mask = data;

        nc_update_interrupts();
}

WRITE_HANDLER(nc_irq_status_w)
{
#ifdef VERBOSE
	logerror("irq status w: %02x\n", data);
#endif
        data = data^0x0ff;
        if (
                /* clearing keyboard int? */
                ((data & (1<<3))!=0) &&
                /* keyboard int request? */
                ((nc_irq_status & (1<<3))!=0)
           )
        {
           if (nc_keyboard_timer!=NULL)
           {
                timer_remove(nc_keyboard_timer);
                nc_keyboard_timer = NULL;
           }

           /* set timer to occur again */
           nc_keyboard_timer = timer_set(TIME_IN_MSEC(10), 0, nc_keyboard_timer_callback);

        }

        nc_irq_status &=~data;
         
        nc_update_interrupts();
}

READ_HANDLER(nc_irq_status_r)
{
        return ~nc_irq_status;
}

WRITE_HANDLER(nc_display_memory_start_w)
{
        /* bit 7: A15 */
        /* bit 6: A14 */
        /* bit 5: A13 */
        /* bit 4: A12 */
        /* bit 3-0: not used */

        nc_display_memory_start = (data & 0x0f0)<<(12-4);

#ifdef VERBOSE
        logerror("disp memory w: %04x\n", nc_display_memory_start);
#endif

}

WRITE_HANDLER(nc_poweroff_control_w)
{
        /* bits 7-1: not used */
        /* bit 0: 1 = no effect, 0 = power off */
        nc_poweroff_control = data;
}

READ_HANDLER(nc_key_data_in_r)
{
        if (offset==9)
        {
           nc_irq_status &= (1<<3);

           if (nc_keyboard_timer!=NULL)
           {
                timer_remove(nc_keyboard_timer);
                nc_keyboard_timer = NULL;
           }

           /* set timer to occur again */
           nc_keyboard_timer = timer_set(TIME_IN_MSEC(10), 0, nc_keyboard_timer_callback);

           nc_update_interrupts();
        }

        return readinputport(offset);

}

READ_HANDLER(nc_card_battery_status_r)
{
        return nc_card_battery_status;
}


static void nc_sound_update(int channel)
{
        int on;
        int frequency;
        int period;

        period = nc_sound_channel_periods[channel];

        /* if top bit is 0, sound is on */
        on = ((period & (1<<15))==0);

        /* calculate frequency from period */
        frequency = (int)(1000000.0f/((float)((period & 0x07fff)<<1) * 1.6276f));

        /* set state */
        beep_set_state(channel, on);
        /* set frequency */
        beep_set_frequency(channel, frequency);
}

WRITE_HANDLER(nc_sound_w)
{
#ifdef VERBOSE
	logerror("sound w: %04x %02x\n", offset, data);
#endif
        switch (offset)
        {
                case 0x0:
                {
                   /* update period value */
                   nc_sound_channel_periods[0]  =
                        (nc_sound_channel_periods[0] & 0x0ff00) | (data & 0x0ff);

                   nc_sound_update(0);
                }
                break;

                case 0x01:
                {
                   nc_sound_channel_periods[0] =
                        (nc_sound_channel_periods[0] & 0x0ff) | ((data & 0x0ff)<<8);

                   nc_sound_update(0);
                }
                break;

                case 0x02:
                {
                   /* update period value */
                   nc_sound_channel_periods[1]  =
                        (nc_sound_channel_periods[1] & 0x0ff00) | (data & 0x0ff);

                   nc_sound_update(1);
                }
                break;

                case 0x03:
                {
                   nc_sound_channel_periods[1] =
                        (nc_sound_channel_periods[1] & 0x0ff) | ((data & 0x0ff)<<8);

                   nc_sound_update(1);
                }
                break;

                default:
                 break;
          }
}

static unsigned long baud_rate_table[]=
{
	150,
    300,
    600,
    1200,
    2400,
    4800,
    9600,
    19200
};

WRITE_HANDLER(nc_uart_control_w)
{

	/* on/off changed state? */
	if (((nc_uart_control ^ data) & (1<<3))!=0)
	{
		/* changed uart from off to on */
		if ((data & (1<<3))==0)
		{
			msm8251_reset();
		}
	}
	
	nc_uart_control = data;

	msm8251_set_baud_rate(baud_rate_table[(data & 0x03)]);

}


static struct IOReadPort readport_nc[] =
{
        {0x010, 0x013, nc_memory_management_r},
        {0x0a0, 0x0a0, nc_card_battery_status_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
		{0x0c0, 0x0c0, msm8251_data_r},
		{0x0c1, 0x0c1, msm8251_status_r},
        {0x0d0, 0x0df, tc8521_r},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_nc[] =
{
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
		{0x030, 0x030, nc_uart_control_w},
        {0x060, 0x060, nc_irq_mask_w},
        {0x070, 0x070, nc_poweroff_control_w},
        {0x090, 0x090, nc_irq_status_w},
		{0x0c0, 0x0c0, msm8251_data_w},
		{0x0c1, 0x0c1, msm8251_control_w},
        {0x0d0, 0x0df, tc8521_w},
        {0x050, 0x053, nc_sound_w},
        {-1}                                                       /* end of table */
        
};

static struct IOReadPort readport_nc200[] =
{
        {0x010, 0x013, nc_memory_management_r},
//        {0x0a0, 0x0a0, nc_card_battery_status_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
		{0x0d0, 0x0d1, mc146818_port_r },
        {0x0e0, 0x0e0, nec765_status_r},
        {0x0e1, 0x0e1, nec765_data_r},
        {-1}							   /* end of table */
};

static struct IOWritePort writeport_nc200[] =
{
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
        {0x060, 0x060, nc_irq_mask_w},
  //      {0x070, 0x070, nc_poweroff_control_w},
        {0x090, 0x090, nc_irq_status_w},
		{0x0d0, 0x0d1, mc146818_port_w },
        {0x050, 0x053, nc_sound_w},
        {0x0e1, 0x0e1, nec765_data_w},
        {-1}                                                       /* end of table */
        
};


INPUT_PORTS_START(nc100)
        /* 0 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
        PORT_BIT (0x004, 0x00, IPT_UNUSED)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT/RED", KEYCODE_LEFT, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 1 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "YELLOW/FUNCTION", KEYCODE_INSERT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESCAPE/STOP", KEYCODE_ESC, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 2 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_RALT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SYMBOL", KEYCODE_HOME, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 3 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \" ", KEYCODE_2, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
        /* 4 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
        PORT_BIT (0x002, 0x00, IPT_UNUSED)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
        /* 5 */
        PORT_START
        PORT_BIT (0x001, 0x00, IPT_UNUSED)
        PORT_BIT (0x002, 0x00, IPT_UNUSED)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
        /* 6 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^", KEYCODE_6, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN/BLUE", KEYCODE_DOWN, IP_JOY_NONE)
        PORT_BIT (0x004, 0x00, IPT_UNUSED)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT/GREEN", KEYCODE_RIGHT, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "#", KEYCODE_TILDE, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "?", KEYCODE_SLASH, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
        /* 7 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ =", KEYCODE_EQUALS,IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 & ", KEYCODE_7, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ |", KEYCODE_BACKSLASH, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
        /* 8 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- _", KEYCODE_MINUS, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "} ]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "{ [", KEYCODE_OPENBRACE, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
        /* 9 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (", KEYCODE_9, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": ;", KEYCODE_COLON, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP,IP_JOY_NONE)

        /* these are not part of the nc100 keyboard */ 
        /* extra */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ON BUTTON", KEYCODE_END, IP_JOY_NONE)

INPUT_PORTS_END

INPUT_PORTS_START(nc200)
        /* 0 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
        PORT_BIT (0x004, 0x00, IPT_UNUSED)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT/RED", KEYCODE_LEFT, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 1 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "YELLOW/FUNCTION", KEYCODE_INSERT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESCAPE/STOP", KEYCODE_ESC, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 2 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_RALT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SYMBOL", KEYCODE_HOME, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 3 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \" ", KEYCODE_2, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
        /* 4 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
        PORT_BIT (0x002, 0x00, IPT_UNUSED)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
        /* 5 */
        PORT_START
        PORT_BIT (0x001, 0x00, IPT_UNUSED)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *", KEYCODE_4, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
        /* 6 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^", KEYCODE_6, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN/BLUE", KEYCODE_DOWN, IP_JOY_NONE)
        PORT_BIT (0x004, 0x00, IPT_UNUSED)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT/GREEN", KEYCODE_RIGHT, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "#", KEYCODE_TILDE, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "?", KEYCODE_SLASH, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
        /* 7 */
        PORT_START
        PORT_BIT (0x001, 0x00, IPT_UNUSED)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ = ", KEYCODE_EQUALS, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ |", KEYCODE_BACKSLASH, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
        /* 8 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- _", KEYCODE_MINUS, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "} ]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "{ [", KEYCODE_OPENBRACE, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
        /* 9 */
        PORT_START
        PORT_BIT (0x001, 0x00, IPT_UNUSED)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": ;", KEYCODE_COLON, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP,IP_JOY_NONE)

        /* not part of the nc200 keyboard */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ON BUTTON", KEYCODE_END, IP_JOY_NONE)

INPUT_PORTS_END


static struct beep_interface nc100_beep_interface =
{
	2,
	{50,50}
};

static struct MachineDriver machine_driver_nc100 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
                        CPU_Z80 ,  /* type */
                        4000000, /* clock: See Note Above */
                        readmem_nc,                   /* MemoryReadAddress */
                        writemem_nc,                  /* MemoryWriteAddress */
                        readport_nc,                  /* IOReadPort */
                        writeport_nc,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        nc100_init_machine,                      /* init machine */
        nc100_shutdown_machine,
	/* video hardware */
        NC_SCREEN_WIDTH, /* screen width */
        NC_SCREEN_HEIGHT,  /* screen height */
        {0, (NC_SCREEN_WIDTH - 1), 0, (NC_SCREEN_HEIGHT - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        NC_NUM_COLOURS,                                                        /* total colours */
        NC_NUM_COLOURS,                                                        /* color table len */
        nc_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        nc_vh_start,
        nc_vh_stop,
        nc_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
        {
                {
                   SOUND_BEEP,
                   &nc100_beep_interface
                }
        }
};



static struct MachineDriver machine_driver_nc200 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
                        CPU_Z80 ,  /* type */
                        4000000, /* clock: See Note Above */
                        readmem_nc,                   /* MemoryReadAddress */
                        writemem_nc,                  /* MemoryWriteAddress */
                        readport_nc200,                  /* IOReadPort */
                        writeport_nc200,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        nc200_init_machine,                      /* init machine */
        nc200_shutdown_machine,
	/* video hardware */
        NC200_SCREEN_WIDTH, /* screen width */
        NC200_SCREEN_HEIGHT,  /* screen height */
        {0, (NC200_SCREEN_WIDTH - 1), 0, (NC200_SCREEN_HEIGHT - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        NC200_NUM_COLOURS,                                                        /* total colours */
        NC200_NUM_COLOURS,                                                        /* color table len */
        nc_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        nc_vh_start,
        nc_vh_stop,
        nc_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
        {
                {
                   SOUND_BEEP,
                   &nc100_beep_interface
                }
        }
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(nc100)
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1)
        ROM_LOAD("nc100.rom", 0x010000, 0x040000, 0x0849884f9)
ROM_END

ROM_START(nc100a)
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1)
        ROM_LOAD("nc100a.rom", 0x010000, 0x040000, 0x0a699eca3)
ROM_END

ROM_START(nc200)
        ROM_REGION(((64*1024)+(512*1024)), REGION_CPU1)
        ROM_LOAD("nc200.rom", 0x010000, 0x080000, 0x0bb8180e7)
ROM_END


static const struct IODevice io_nc100[] =
{
        {
                IO_CARTSLOT,           /* type */
                1,                     /* count */
                "crd\0card\0",               /* file extensions */
				IO_RESET_NONE,			/* reset if file changed */
                nc_pcmcia_card_id,   /* id */
                nc_pcmcia_card_load, /* load */
                nc_pcmcia_card_exit, /* exit */
                NULL,                   /* info */
                NULL,                   /* open */
                NULL,                   /* close */
                NULL,                   /* status */
                NULL,                   /* seek */
                NULL,                   /* tell */
                NULL,                   /* input */
                NULL,                   /* output */
                NULL,                   /* input chunk */
                NULL,                   /* output chunk */
        },
	{IO_END}
};

static const struct IODevice io_nc200[] =
{
        {
                IO_CARTSLOT,           /* type */
                1,                     /* count */
                "crd\0card\0",               /* file extensions */
				IO_RESET_NONE,			/* reset if file changed */
                nc_pcmcia_card_id,   /* id */
                nc_pcmcia_card_load, /* load */
                nc_pcmcia_card_exit, /* exit */
                NULL,                   /* info */
                NULL,                   /* open */
                NULL,                   /* close */
                NULL,                   /* status */
                NULL,                   /* seek */
                NULL,                   /* tell */
                NULL,                   /* input */
                NULL,                   /* output */
                NULL,                   /* input chunk */
                NULL,                   /* output chunk */
        },
        {
                IO_FLOPPY,
                1,
                "dsk\0",
                IO_RESET_NONE,
                NULL,
                pc_floppy_init,
                pc_floppy_exit,
                NULL,                   /* info */
                NULL,                   /* open */
                NULL,                   /* close */
                floppy_status,                   /* status */
                NULL,                   /* seek */
                NULL,                   /* tell */
                NULL,                   /* input */
                NULL,                   /* output */
                NULL,                   /* input chunk */
                NULL,                   /* output chunk */
        },

	{IO_END}
};


#define io_nc100a io_nc100

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 1992, nc100,   0,                nc100,  nc100,      0,       "Amstrad plc", "NC100 Rom version v1.09")
COMP( 1992, nc100a,  0,                nc100, nc100,      0,   "Amstrad plc","NC100 Rom version v1.00") 
COMP( 1993, nc200,   0,                nc200, nc200,      0,   "Amstrad plc", "NC200")
