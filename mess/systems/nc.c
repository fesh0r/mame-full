/******************************************************************************

        nc.c

        NC100/NC150/NC200 Notepad computer

        system driver


		Thankyou to:
			Cliff Lawson, Russell Marks and Tim Surtel

        Documentation:

		NC100:
			NC100 I/O Specification by Cliff Lawson,
			NC100EM by Russell Marks
		NC200:
			Dissassembly of the NC200 ROM + e-mail
			exchange with Russell Marks


		NC100:

        Hardware:
            - Z80 CPU, 6mhz
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
			- mc146818 real time clock?

        TODO:
           - find out what the unused key bits are for
		   (checked all unused bits on nc200! - do not seem to have any use!)
           - complete serial (xmodem protocol!)
		   - overlay would be nice!
		   - finish NC200 disc drive emulation
		   - finish pcmcia card emulation
		   - add NC150 driver - ROM needed!!! 

		Interrupt system of NC100:

		The IRQ mask is used to control the interrupt sources that can interrupt.
		
		The IRQ status can be read to determine which devices are interrupting.
		Some devices, e.g. serial, cannot be cleared by writing to the irq status
		register. These can only be cleared by performing an operation on the 
		device (e.g. reading a data register).

		Self Test:

		- requires memory save and real time clock save to be working!
		(i.e. for MESS nc100 driver, nc100.nv can be created)
		- turn off nc (use NMI button)
		- reset+FUNCTION+SYMBOL must be pressed together.

		Note: NC200 Self test does not test disc hardware :(

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
/* for serial data transfers */
#include "includes/serial.h"
/* uncomment for verbose debugging information */
//#define VERBOSE


#include "includes/centroni.h"
#include "printer.h"

static void nc_printer_update(int);

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
NC100:
        bit 7: memory card present 0 = yes, 1 = no
        bit 6: memory card write protected 1 = yes, 0 = no
        bit 5: input voltage = 1, if >= to 4 volts
        bit 4: mem card battery: 0 = battery low
        bit 3: alkaline batteries. 0 if >=3.2 volts
        bit 2: lithium battery 0 if >= 2.7 volts
        bit 1: parallel interface busy (0 if busy)
        bit 0: parallel interface ack (1 if ack)
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
IRQ/MASK
bits 7: not used
bit 6: NC100: not used. NC200: RTC??
bit 5: NC100: not used. NC200: FDC interrupt
bits 4: Not used
Bit 3: Key scan interrupt (10ms)
Bit 2: NC100: ACK from parallel interface NC200: serial interrupt
Bit 1: Tx Ready
Bit 0: NC100: Rx Ready NC200: ACK from parallel interface
*/
static int nc_irq_mask;
static int nc_irq_status;



/* latched interrupts are interrupts that cannot be cleared by writing to the irq
mask. latched interrupts can only be cleared by accessing the interrupting
device e.g. serial chip, fdc */
static int nc_irq_latch;
/* this is a mask of irqs that are latched, and it is different for nc100 and
nc200 */
static int nc_irq_latch_mask;
static int nc_sound_channel_periods[2];

static unsigned char previous_on_off_button_state;

static void nc_update_interrupts(void)
{
		nc_irq_status &= ~nc_irq_latch_mask;
		nc_irq_status |= nc_irq_latch;

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

        memory_set_bankhandler_r(bank+1, 0, nc_bankhandler_r[bank]);

        switch (mem_type)
        {
                /* ROM */
                case 0:
                {
                   unsigned char *addr;

                   mem_bank = mem_bank & nc_membank_rom_mask;

                   addr = (memory_region(REGION_CPU1)+0x010000) + (mem_bank<<14);

                   cpu_setbank(bank+1, addr);

                   memory_set_bankhandler_w(bank+5, 0, MWA_NOP);
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

                   memory_set_bankhandler_w(bank+5, 0, nc_bankhandler_w[bank]);
#ifdef VERBOSE
                   logerror("BANK %d: RAM\n",bank);
#endif
                }
                break;

                /* card RAM */
                case 2:
                {
                   /* card connected? */
                   if (((nc_card_battery_status & (1<<7))==0) && (nc_card_ram!=NULL))
                   {

                           unsigned char *addr;

                           addr = nc_card_ram + (mem_bank<<14);

                           cpu_setbank(bank+1, addr);
                           cpu_setbank(bank+5, addr);

                           memory_set_bankhandler_w(bank+5, 0, nc_bankhandler_w[bank]);
#ifdef VERBOSE
                           logerror("BANK %d: CARD-RAM\n",bank);
#endif
				   }
                    else
                    {
                        /* if no card connected, then writes fail */
                           memory_set_bankhandler_r(bank+1, 0, MRA_NOP);
                           memory_set_bankhandler_w(bank+1, 0, MWA_NOP);
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
	nc_irq_latch &= ~(1<<1);

	if (state)
	{
		nc_irq_latch |= (1<<1);
	}

	nc_update_interrupts();
}

static void nc100_rxrdy_callback(int state)
{
	nc_irq_latch &= ~(1<<0);

	if (state)
	{
		nc_irq_latch |= (1<<0);
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


/* assumption. nc200 uses the same uart chip. The rxrdy and txrdy are combined
together with a or to generate a single interrupt */
static UINT8 nc200_uart_interrupt_irq;

static void nc200_refresh_uart_interrupt(void)
{
	nc_irq_latch &=~(1<<2);
	if ((nc200_uart_interrupt_irq & 0x03)!=0)
	{
		nc_irq_latch |= (1<<2);
	}
}

static void nc200_txrdy_callback(int state)
{
	nc200_uart_interrupt_irq &=~(1<<0);

	if (state)
	{
		nc200_uart_interrupt_irq |=(1<<0);
	}

	nc200_refresh_uart_interrupt();
}

static void nc200_rxrdy_callback(int state)
{
	nc200_uart_interrupt_irq &=~(1<<1);

	if (state)
	{
		nc200_uart_interrupt_irq |=(1<<1);
	}

	nc200_refresh_uart_interrupt();
}

static struct msm8251_interface nc200_uart_interface=
{
	nc200_rxrdy_callback,
	NULL,
	nc200_txrdy_callback,
};


/* NC100 printer emulation */
/* port 0x040 (write only) = 8-bit printer data */
/* port 0x030 bit 6 = printer strobe */

/* same for nc100 and nc200 */
static WRITE_HANDLER(nc_printer_data_w)
{
#ifdef VERBOSE
	logerror("printer write %02x\n",data);
#endif
	centronics_write_data(0,data);
}

/* same for nc100 and nc200 */
static void	nc_printer_update(int port0x030)
{
	int handshake = 0;

	if (port0x030 & (1<<6))
	{
		handshake = CENTRONICS_STROBE;
	}
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(0, handshake, CENTRONICS_STROBE);
}


static void nc100_printer_handshake_in(int number, int data, int mask)
{
	nc_irq_status &= ~(1<<2);

	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
			nc_irq_status|=(1<<2);
		}
	}
	/* trigger an int if the irq is set */
	nc_update_interrupts();
}

static void nc200_printer_handshake_in(int number, int data, int mask)
{
	nc_irq_status &= ~(1<<0);

	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
			nc_irq_status|=(1<<0);
		}
	}
	/* trigger an int if the irq is set */
	nc_update_interrupts();
}


static CENTRONICS_CONFIG nc100_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		nc100_printer_handshake_in
	},
};


static CENTRONICS_CONFIG nc200_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		nc200_printer_handshake_in
	},
};

static void *file;

/* restore a block of memory from the nvram file */
static void nc_common_restore_memory_from_stream(void)
{
	if (!file)
		return;

    if (nc_memory!=NULL)
    {
		unsigned long stored_size;
		unsigned long restore_size;
		
#ifdef VERBOSE
		logerror("restoring nc memory\n");
#endif
		/* get size of memory data stored */
		osd_fread(file, &stored_size, sizeof(unsigned long));

		if (stored_size>nc_memory_size)
		{
			restore_size = nc_memory_size;
		}
		else
		{
			restore_size = stored_size;
		}
		/* read as much as will fit into memory */
		osd_fread(file, nc_memory, restore_size);
		/* seek over remaining data */    
		osd_fseek(file, SEEK_CUR,stored_size - restore_size);
	}
}

/* store a block of memory to the nvram file */
static void nc_common_store_memory_to_stream(void)
{
	if (!file)
		return;

    if (nc_memory!=NULL)
    {
#ifdef VERBOSE
		logerror("storing nc memory\n");
#endif
		/* write size of memory data */
		osd_fwrite(file, &nc_memory_size, sizeof(unsigned long));

		/* write data block */
		osd_fwrite(file, nc_memory, nc_memory_size);
    }
}

static void nc_common_open_stream_for_reading(void)
{
	char filename[13];

	sprintf(filename,"%s.nv", Machine->gamedrv->name);

	file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, OSD_FOPEN_READ);
}

static void nc_common_open_stream_for_writing(void)
{
    char filename[13];

    sprintf(filename,"%s.nv", Machine->gamedrv->name);

    file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, OSD_FOPEN_WRITE);
}


static void	nc_common_close_stream(void)
{
	if (file)
	{
		osd_fclose(file);
	}
}


void nc_common_init_machine(void)
{
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
        
		nc_irq_latch = 0;
		nc_irq_latch_mask = 0;

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

		nc_uart_control = 0x0ff;
}

void nc100_init_machine(void)
{
    nc_type = NC_TYPE_1xx;

    nc_memory_size = 64*1024;

    /* 256k of rom */
    nc_membank_rom_mask = 0x0f;

    nc_memory = (unsigned char *)malloc(nc_memory_size);
    nc_membank_internal_ram_mask = 3;

    nc_membank_card_ram_mask = 0x03f;

    nc_common_init_machine();

	tc8521_init(&nc100_tc8521_interface);

	msm8251_init(&nc100_uart_interface);

	centronics_config(0, nc100_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

	nc_common_open_stream_for_reading();
	tc8521_load_stream(file);

	nc_common_restore_memory_from_stream();

	nc_common_close_stream();

	/* serial */
	nc_irq_latch_mask = (1<<0) | (1<<1);

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
        nc_irq_latch &=~(1<<5);

        if (state)
        {
                nc_irq_latch |=(1<<5);
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

	    /* 512k of rom */
		nc_membank_rom_mask = 0x1f;


        nc_memory_size = 128*1024;
        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 7;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();

        nec765_init(&nc200_nec765_interface, NEC765A);
        floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);
        floppy_drive_set_motor_state(0,1);
        floppy_drive_set_ready_state(0,1,0);

		mc146818_init(MC146818_STANDARD);


		nc_card_battery_status = 0;

		nc200_uart_interrupt_irq = 0;
		msm8251_init(&nc200_uart_interface);

		centronics_config(0, nc200_cent_config);
		/* assumption: select is tied low */
		centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

		nc_common_open_stream_for_reading();
		if (file)
		{
			mc146818_load_stream(file);
		}
		nc_common_restore_memory_from_stream();
		nc_common_close_stream();

		/* fdc, serial */
		nc_irq_latch_mask = (1<<5) | (1<<2);
}

void nc_common_shutdown_machine(void)
{
	logerror("shutdown machine");

	msm8251_stop();

    if (nc_memory!=NULL)
    {
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
	nc_common_open_stream_for_writing();
	tc8521_save_stream(file);
	nc_common_store_memory_to_stream();
	nc_common_close_stream();

	nc_common_shutdown_machine();
    tc8521_stop();
}


void	nc200_shutdown_machine(void)
{
	nc_common_open_stream_for_writing();
	if (file)
	{
		mc146818_save_stream(file);
	}
	nc_common_store_memory_to_stream();
	nc_common_close_stream();

	nc_common_shutdown_machine();
	mc146818_close();
}


MEMORY_READ_START( readmem_nc )
        {0x00000, 0x03fff, MRA_BANK1},
        {0x04000, 0x07fff, MRA_BANK2},
        {0x08000, 0x0bfff, MRA_BANK3},
        {0x0c000, 0x0ffff, MRA_BANK4},
MEMORY_END


MEMORY_WRITE_START( writemem_nc )
        {0x00000, 0x03fff, MWA_BANK5},
        {0x04000, 0x07fff, MWA_BANK6},
        {0x08000, 0x0bfff, MWA_BANK7},
        {0x0c000, 0x0ffff, MWA_BANK8},
MEMORY_END


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
	int printer_handshake;

	/* bit 1: printer busy */
	/* bit 2: printer acknowledge */
	nc_card_battery_status &=~3;
	

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

	printer_handshake = centronics_read_handshake(0);

	nc_card_battery_status |=(1<<1);

	/* if printer is not online, it is busy */
	if ((printer_handshake & CENTRONICS_ONLINE)!=0)
	{
		nc_card_battery_status &=~(1<<1);
	}

	if (printer_handshake & CENTRONICS_ACKNOWLEDGE)
	{
		nc_card_battery_status |=(1<<0);
	}

    return nc_card_battery_status;
}


/* port &80:

  bit 0: Parallel interface BUSY
 */

static unsigned char nc200_printer_status;

READ_HANDLER(nc200_printer_status_r)
{
	int printer_handshake;
	
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

	printer_handshake = centronics_read_handshake(0);

	nc200_printer_status |=(1<<0);

	/* if printer is not online, it is busy */
	if ((printer_handshake & CENTRONICS_ONLINE)!=0)
	{
		nc200_printer_status &=~(1<<0);
	}

    return nc200_printer_status;
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
	/* update printer state */
	nc_printer_update(data);

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

PORT_READ_START( readport_nc )
        {0x010, 0x013, nc_memory_management_r},
        {0x0a0, 0x0a0, nc_card_battery_status_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
		{0x0c0, 0x0c0, msm8251_data_r},
		{0x0c1, 0x0c1, msm8251_status_r},
        {0x0d0, 0x0df, tc8521_r},
PORT_END

PORT_WRITE_START( writeport_nc )
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
		{0x030, 0x030, nc_uart_control_w},
		{0x040, 0x040, nc_printer_data_w},
        {0x060, 0x060, nc_irq_mask_w},
        {0x070, 0x070, nc_poweroff_control_w},
        {0x090, 0x090, nc_irq_status_w},
		{0x0c0, 0x0c0, msm8251_data_w},
		{0x0c1, 0x0c1, msm8251_control_w},
        {0x0d0, 0x0df, tc8521_w},
        {0x050, 0x053, nc_sound_w},
PORT_END


PORT_READ_START( readport_nc200 )
        {0x010, 0x013, nc_memory_management_r},
		{0x080, 0x080, nc200_printer_status_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
		{0x0c0, 0x0c0, msm8251_data_r},
		{0x0c1, 0x0c1, msm8251_status_r},
		{0x0d0, 0x0d1, mc146818_port_r },
        {0x0e0, 0x0e0, nec765_status_r},
        {0x0e1, 0x0e1, nec765_data_r},
PORT_END

PORT_WRITE_START( writeport_nc200 )
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
		{0x040, 0x040, nc_printer_data_w},
		{0x030, 0x030, nc_uart_control_w},
        {0x060, 0x060, nc_irq_mask_w},
        {0x090, 0x090, nc_irq_status_w},
		{0x0c0, 0x0c0, msm8251_data_w},
		{0x0c1, 0x0c1, msm8251_control_w},
		{0x0d0, 0x0d1, mc146818_port_w },
        {0x050, 0x053, nc_sound_w},
        {0x0e1, 0x0e1, nec765_data_w},
PORT_END



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
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "YELLOW/FUNCTION", KEYCODE_RALT, IP_JOY_NONE) 
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
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MENU", KEYCODE_PGUP, IP_JOY_NONE)
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
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
	    PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT/RED", KEYCODE_LEFT, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
		PORT_BIT (0x080, 0x00, IPT_UNUSED)
	    /* 1 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "YELLOW/FUNCTION", KEYCODE_RALT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESCAPE/STOP", KEYCODE_ESC, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BIT (0x040, 0x00, IPT_UNUSED)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (", KEYCODE_9, IP_JOY_NONE)
        /* 2 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SYMBOL", KEYCODE_HOME, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^", KEYCODE_6, IP_JOY_NONE)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
	    /* 3 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"", KEYCODE_2, IP_JOY_NONE)
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
        /* 4 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)
        PORT_BITX(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 &", KEYCODE_7, IP_JOY_NONE)
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
		PORT_BIT (0x001, 0x00, IPT_UNUSED)
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
        PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MENU", KEYCODE_PGUP, IP_JOY_NONE)
        PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
        PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
        /* 8 */
        PORT_START
        PORT_BIT (0x001, 0x00, IPT_UNUSED)
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
                        6000000, /* clock: See Note Above */
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
        640/*NC_SCREEN_WIDTH*/, /* screen width */
        480/*NC_SCREEN_HEIGHT*/,  /* screen height */
        {0, (640/*NC_SCREEN_WIDTH*/ - 1), 0, (480/*NC_SCREEN_HEIGHT*/ - 1)},        /* rectangle: visible_area */
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
                        6000000, /* clock: See Note Above */
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
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1,0)
        ROM_LOAD("nc100.rom", 0x010000, 0x040000, 0x0849884f9)
ROM_END

ROM_START(nc100a)
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1,0)
        ROM_LOAD("nc100a.rom", 0x010000, 0x040000, 0x0a699eca3)
ROM_END

ROM_START(nc200)
        ROM_REGION(((64*1024)+(512*1024)), REGION_CPU1,0)
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
		{
                IO_SERIAL,           /* type */
                1,                     /* count */
                "txt\0",               /* file extensions */
				IO_RESET_NONE,			/* reset if file changed */
                NULL,   /* id */
                nc_serial_init, /* load */
                serial_device_exit, /* exit */
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
	IO_PRINTER_PORT(1,"\0"),
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
      {
                IO_SERIAL,           /* type */
                1,                     /* count */
                "txt\0",               /* file extensions */
				IO_RESET_NONE,			/* reset if file changed */
                NULL,   /* id */
                nc_serial_init, /* load */
                serial_device_exit, /* exit */
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
	IO_PRINTER_PORT(1,"\0"),
	{IO_END}
};


#define io_nc100a io_nc100

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 1992, nc100,   0,                nc100,  nc100,      0,       "Amstrad plc", "NC100 Rom version v1.09")
COMP( 1992, nc100a,  nc100,                nc100, nc100,      0,   "Amstrad plc","NC100 Rom version v1.00")
COMP( 1993, nc200,   0,                nc200, nc200,      0,   "Amstrad plc", "NC200")
