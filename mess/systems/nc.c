/******************************************************************************

        nc.c

        NC100/NC150/NC200 Notepad computer 


        system driver

        Documentation:
        NC100 I/O Specification by Cliff Lawson,
        NC100EM by Russell Marks


        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/nc.h"

static char nc_memory_config[4];
unsigned long nc_display_memory_start;

static int nc_membank_rom_mask;
static int nc_membank_internal_ram_mask;
static int nc_membank_card_ram_mask;
static UINT8 nc_poweroff_control;

/* internal ram */
unsigned char    *nc_memory;
/* card ram */
unsigned char    *nc_card_ram;


/*
bits 7-4: Not used
Bit 3: Key scan interrupt (10ms)
Bit 2: ACK from parallel interface
Bit 1: Tx Ready
Bit 0: Rx Ready
*/
static int nc_irq_mask;
static int nc_irq_status;

static void nc_update_interrupts(void)
{
        /* any ints set and they are not masked? */
        if (((nc_irq_status & nc_irq_mask) & 0x03)!=0)
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
                   unsigned char *addr;

                   addr = nc_card_ram + (mem_bank<<14);

                   cpu_setbank(bank+1, addr);
                   cpu_setbank(bank+5, addr);

                   cpu_setbankhandler_w(bank+5, nc_bankhandler_w[bank]);

                   logerror("BANK %d: CARD-RAM\r\n",bank);

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

void nc_common_init_machine(void)
{
        nc_display_memory_start = 0;

        nc_memory_config[0] = 0;
        nc_memory_config[1] = 0;
        nc_memory_config[2] = 0;
        nc_memory_config[3] = 0;

        /* ints are masked */
        nc_irq_mask = 0;
        /* no ints wanting servicing */
        nc_irq_status = 0;

        /* 256k of rom */
        nc_membank_rom_mask = 0x0f;

        /* at reset set to 1 */
        nc_poweroff_control = 1;

        nc_refresh_memory_config();

        nc_card_ram = (unsigned char *)malloc(1024*1024);
}

void nc100_init_machine(void)
{
        nc_memory = (unsigned char *)malloc(64*1024);
        nc_membank_internal_ram_mask = 3;

        nc_membank_card_ram_mask = 0x03f;

        nc_common_init_machine();
}

#if 0
void nc150_init_machine(void)
{
        nc_memory = (unsigned char *)malloc(128*1024);
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
        if (nc_memory!=NULL)
        {
                free(nc_memory);
                nc_memory = NULL;
        }

        if (nc_card_ram!=NULL)
        {
                free(nc_card_ram);
                nc_card_ram = NULL;
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
        nc_irq_mask = data;

        nc_update_interrupts();
}

WRITE_HANDLER(nc_irq_status_w)
{
        nc_irq_status = data;

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

        nc_display_memory_start = (data & 0x0f0)<<12;
}

WRITE_HANDLER(nc_poweroff_control_w)
{
        /* bits 7-1: not used */
        /* bit 0: 1 = no effect, 0 = power off */
        nc_poweroff_control = data;
}

READ_HANDLER(nc_key_data_in_r)
{
        return readinputport(offset);

}

static struct IOReadPort readport_nc[] =
{
        {0x010, 0x013, nc_memory_management_r},
        {0x0b0, 0x0b9, nc_key_data_in_r},
        {0x090, 0x090, nc_irq_status_r},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_nc[] =
{
        {0x000, 0x000, nc_display_memory_start_w},
        {0x010, 0x013, nc_memory_management_w},
        {0x060, 0x060, nc_irq_mask_w},
        {0x070, 0x070, nc_poweroff_control_w},
        {0x090, 0x090, nc_irq_status_w},
        {-1}                                                       /* end of table */
        
};

INPUT_PORTS_START(nc100)
INPUT_PORTS_END

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
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(nc100)
        ROM_REGION(((64*1024)+(256*1024)), REGION_CPU1)
        ROM_LOAD("nc100.rom", 0x010000, 0x020000, 0x0000)
ROM_END

static const struct IODevice io_nc100[] =
{

	{IO_END}
};


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 19??, nc100,   0,                nc100,  nc100,      0,       "Amstrad plc", "NC100")

