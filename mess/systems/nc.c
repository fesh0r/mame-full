/******************************************************************************

        nc.c

        NC100/NC150 Notepad computer 


        system driver

        Documentation:
        NC100 I/O Specification by Cliff Lawson,
        NC100EM by Russell Marks

        Hardware:
                - Z80 CPU
                - memory powered by lithium batterys!
                - 2 channel tone (programmable frequency beep's)
                - LCD screen
                - laptop/portable computer
                - qwerty keyboard
                - serial/parallel connection
                - Amstrad custom ASIC chip

        TODO:
           - find out what the unused key bits are for
           - serial, parallel and loads more!!!
           - overlay would be nice!
        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/nc.h"
#include "includes/tc8521.h"
//#include "sound/beep.h"

static unsigned long nc_memory_size;

static char nc_memory_config[4];
unsigned long nc_display_memory_start;
static void *nc_keyboard_timer = NULL;
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
static int nc_card_battery_status;

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
bits 7-4: Not used
Bit 3: Key scan interrupt (10ms)
Bit 2: ACK from parallel interface
Bit 1: Tx Ready
Bit 0: Rx Ready
*/
static int nc_irq_mask;
static int nc_irq_status;
static int nc_sound_channel_periods[2];

static void nc_update_interrupts(void)
{
        /* any ints set and they are not masked? */
        if (
                (((nc_irq_status & nc_irq_mask) & 0x0f)!=0) 
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
        logerror("keyboard int\r\n");

        /* set int */
        nc_irq_status |= (1<<3);

        /* update ints */
        nc_update_interrupts();

        /* don't trigger again, but don't free it */
        timer_reset(nc_keyboard_timer, TIME_NEVER);
}

/* set int by index - see bit assignments above */
static void nc_set_int(int index1)
{
        nc_irq_status |= (1<<index1);

        /* update interrupt */
        nc_update_interrupts();
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

                   logerror("BANK %d: ROM %d\r\n",bank,mem_bank);

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

                   logerror("BANK %d: RAM\r\n",bank);

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
        
                           logerror("BANK %d: CARD-RAM\r\n",bank);
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
                        logerror("Invalid memory selection\r\n");
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

static struct tc8521_interface nc100_tc8521_interface=
{
  NULL,
  NULL
};

void nc_common_init_machine(void)
{
        
        void *file;

        nc_display_memory_start = 0;

        nc_memory_config[0] = 0;
        nc_memory_config[1] = 0;
        nc_memory_config[2] = 0;
        nc_memory_config[3] = 0;

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
        nc_card_battery_status = (1<<5) | (1<<4);

        nc_set_card_present_state(0);
        
        nc_card_ram = (unsigned char *)malloc(1024*1024);

        nc_keyboard_timer = timer_set(TIME_IN_MSEC(10), 0, nc_keyboard_timer_callback);

        /* 256k of rom */
        nc_membank_rom_mask = 0x0f;

        if (nc_memory!=NULL)
        {
                /* restore nc memory from file */
                file = osd_fopen(Machine->gamedrv->name, "nc100.nv", OSD_FILETYPE_MEMCARD, OSD_FOPEN_READ);
        
                if (file!=NULL)
                {
                   logerror("restoring nc100 memory\r\n");
                   osd_fread(file, nc_memory, nc_memory_size);
                   osd_fclose(file);
                }
        }

        tc8521_init(&nc100_tc8521_interface);
}

void nc100_init_machine(void)
{
        nc_memory_size = 64*1024;

        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 3;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();
}

#if 0
void nc150_init_machine(void)
{
        nc_memory = (unsigned char *)malloc(nc_memory_size);
        nc_membank_internal_ram_mask = 7;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();
}

void nc200_init_machine(void)
{
        nc_memory = (unsigned char *)malloc(128*1024);
        nc_membank_internal_ram_mask = 7;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();
}
#endif

void nc_shutdown_machine(void)
{
        
        tc8521_stop();

        if (nc_memory!=NULL)
        {
                /* write nc memory to file */
                void *file;

                file = osd_fopen(Machine->gamedrv->name, "nc100.nv", OSD_FILETYPE_MEMCARD, OSD_FOPEN_WRITE);

                if (file!=NULL)
                {
                   logerror("writing nc100 memory!\r\n");
                   osd_fwrite(file, nc_memory, nc_memory_size);
                   osd_fclose(file);
                }

                free(nc_memory);
                nc_memory = NULL;
        }

        if (nc_card_ram!=NULL)
        {
                free(nc_card_ram);
                nc_card_ram = NULL;
        }

        if (nc_keyboard_timer!=NULL)
        {
                timer_remove(nc_keyboard_timer);
                nc_keyboard_timer = NULL;
        }
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
        logerror("Memory management W: %02x %02x\r\n",offset,data);

        nc_memory_config[offset] = data;

        nc_refresh_memory_config();
}

WRITE_HANDLER(nc_irq_mask_w)
{
        logerror("irq mask w: %02x\r\n", data);

        nc_irq_mask = data;

        nc_update_interrupts();
}

WRITE_HANDLER(nc_irq_status_w)
{
        logerror("irq status w: %02x\r\n", data);

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

        logerror("disp memory w: %04x\r\n", nc_display_memory_start);

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
        logerror("sound w: %04x %02x\r\n", offset, data);

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

static struct IOReadPort readport_nc[] =
{
        {0x010, 0x013, nc_memory_management_r},
        {0x0a0, 0x0a0, nc_card_battery_status_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
        {0x0d0, 0x0df, tc8521_r},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_nc[] =
{
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
        {0x060, 0x060, nc_irq_mask_w},
        {0x070, 0x070, nc_poweroff_control_w},
        {0x090, 0x090, nc_irq_status_w},
        {0x0d0, 0x0df, tc8521_w},
        {0x050, 0x053, nc_sound_w},
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
        PORT_BITX(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESCAPE", KEYCODE_ESC, IP_JOY_NONE)
        PORT_BITX(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
        PORT_BIT (0x010, 0x00, IPT_UNUSED)
        PORT_BIT (0x020, 0x00, IPT_UNUSED)
        PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
        PORT_BIT (0x080, 0x00, IPT_UNUSED)
        /* 2 */
        PORT_START
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)
        PORT_BITX(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT", KEYCODE_RALT, IP_JOY_NONE)
        PORT_BIT (0x002, 0x00, IPT_UNUSED)
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

INPUT_PORTS_END

static struct beep_interface nc100_beep_interface =
{
        2
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
        nc_shutdown_machine,
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




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(nc100)
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1)
        ROM_LOAD("nc100.rom", 0x010000, 0x040000, 0x0849884f9)
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


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 19??, nc100,	  0,		nc100,	  nc100,	0,	 "Amstrad plc", "NC100")

