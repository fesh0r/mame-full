/******************************************************************************

        avigo.c

        TI "Avigo" PDA


        system driver

        Documentation:
                Hans B Pufal
                Avigo simulator


        MEMORY MAP:
                0x0000-0x03fff: flash 0 block 0
                0x4000-0x07fff: flash x block y

                0xc000-0x0ffff: ram block 0

		Hardware:
			- Z80 CPU
            - 16c500c UART  
			-  28f008sa flash-file memory x 3 (3mb)
			- 128k ram
			- stylus pen
			- touch-pad screen
        TODO:
                Dissassemble the rom a bit and find out exactly
                how memory paging works!

        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/avigo.h"
#include "includes/28f008sa.h"
#include "includes/tc8521.h"
#include "includes/uart8250.h"

static UINT8 avigo_key_line;
/* 
	bit 7:						?? high priority. When it occurs, clear this bit.
	bit 6: pen int				pen or power
	bit 5:						port 1d,1f
	bit 4: timer 2 int
	bit 3: uart int				port 35,30
	bit 2: keyboard int			;; check bit 5 of port 1,
	bit 1: timer 1 int			(cleared in nmi, and then set again)

*/
static UINT8 avigo_irq;
/* 128k ram */
unsigned char *avigo_memory;

/* bit 3 = speaker state */
static UINT8 avigo_speaker_data;

/* bits 0-5 define bank index */
static unsigned long avigo_ram_bank_l;
static unsigned long avigo_ram_bank_h;
/* bits 0-5 define bank index */
static unsigned long avigo_rom_bank_l;
static unsigned long avigo_rom_bank_h;
static unsigned long avigo_ad_control_status;
static  int avigo_flash_at_0x4000;
static  int avigo_flash_at_0x8000;

/* memory 0x0000-0x03fff */
static READ_HANDLER(avigo_flash_0x0000_read_handler)
{

        int flash_offset = offset;
        
	return flash_bank_handler_r(0, flash_offset);
}

/* memory 0x04000-0x07fff */
static READ_HANDLER(avigo_flash_0x4000_read_handler)
{

        int flash_offset = (avigo_rom_bank_l<<14) | offset;

        return flash_bank_handler_r(avigo_flash_at_0x4000, flash_offset);
}

/* memory 0x0000-0x03fff */
static WRITE_HANDLER(avigo_flash_0x0000_write_handler)
{

	int flash_offset = offset;
        
	flash_bank_handler_w(0, flash_offset, data);
}

/* memory 0x04000-0x07fff */
static WRITE_HANDLER(avigo_flash_0x4000_write_handler)
{

        int flash_offset = (avigo_rom_bank_l<<14) | offset;
       
        flash_bank_handler_w(avigo_flash_at_0x4000, flash_offset, data);
}

/* memory 0x08000-0x0bfff */
static READ_HANDLER(avigo_flash_0x8000_read_handler)
{

        int flash_offset = (avigo_ram_bank_l<<14) | offset;

        return flash_bank_handler_r(avigo_flash_at_0x8000, flash_offset);
}

/* memory 0x08000-0x0bfff */
static WRITE_HANDLER(avigo_flash_0x8000_write_handler)
{

        int flash_offset = (avigo_ram_bank_l<<14) | offset;

        flash_bank_handler_w(avigo_flash_at_0x8000, flash_offset, data);
}


static READ_HANDLER(avigo_ram_0xc000_read_handler)
{
	return avigo_memory[offset];
}

static WRITE_HANDLER(avigo_ram_0xc000_write_handler)
{
	avigo_memory[offset] = data;
}


static void avigo_refresh_ints(void)
{
	if (avigo_irq!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}

#if 0
static void avigo_1hz_int(int state)
{
        logerror("1hz int\r\n");

        avigo_irq |=(1<<1);

        avigo_refresh_ints();


}

static void avigo_16hz_int(int state)
{
        logerror("16hz int\r\n");

        avigo_irq |=(1<<4);

        avigo_refresh_ints();

}
#endif

static struct tc8521_interface avigo_tc8521_interface = 
{
	NULL,	//avigo_alarm_int
};

static void avigo_refresh_memory(void)
{
        unsigned char *addr;

        switch (avigo_rom_bank_h)
        {
		  case 0x03:
          {
              avigo_flash_at_0x4000 = 1;
          }
          break;

          case 0x05:
          {
              avigo_flash_at_0x4000 = 2;
          }
          break;

          default:
              avigo_flash_at_0x4000 = 0;
              break;
        }

        addr = (unsigned char *)flash_get_base(avigo_flash_at_0x4000);
        addr = addr + (avigo_rom_bank_l<<14);
        cpu_setbank(2, addr);
        cpu_setbank(6, addr);

        cpu_setbankhandler_r(2, avigo_flash_0x4000_read_handler);
        cpu_setbankhandler_w(6, avigo_flash_0x4000_write_handler);

        switch (avigo_ram_bank_h)
        {
                /* screen */
                case 0x06:
                {
                        cpu_setbankhandler_w(7, avigo_vid_memory_w);
                        cpu_setbankhandler_r(3, avigo_vid_memory_r);
                }
                break;

                /* ram */
                case 0x01:
                {
                        addr = avigo_memory + ((avigo_ram_bank_l & 0x07)<<14);

                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        cpu_setbankhandler_w(7, MWA_BANK7);
                        cpu_setbankhandler_r(3, MRA_BANK3);
                }
                break;

                case 0x03:
                {
                        avigo_flash_at_0x8000 = 1;


                        addr = (unsigned char *)flash_get_base(avigo_flash_at_0x8000);
                        addr = addr + (avigo_ram_bank_l<<14);
                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        cpu_setbankhandler_r(3, avigo_flash_0x8000_read_handler);
                        cpu_setbankhandler_w(7, avigo_flash_0x8000_write_handler);



                }
                break;

                case 0x07:
                {
                        avigo_flash_at_0x8000 = 0;


                        addr = (unsigned char *)flash_get_base(avigo_flash_at_0x8000);
                        addr = addr + (avigo_ram_bank_l<<14);
                        cpu_setbank(3, addr);
                        cpu_setbank(7, addr);

                        cpu_setbankhandler_r(3, avigo_flash_0x8000_read_handler);
                        cpu_setbankhandler_w(7, avigo_flash_0x8000_write_handler);


                }
                break;

                default:
                  break;
        }

}



static void avigo_com_interrupt(int irq_num, int state)
{
        logerror("com int\r\n");

	avigo_irq &= ~(1<<3);

	if (state)
	{
		avigo_irq |= (1<<3);
	}

	avigo_refresh_ints();
}



static uart8250_interface avigo_com_interface[1]=
{
	{
		TYPE16550,
		1843200,
		avigo_com_interrupt,
		NULL,
		NULL
	},
};     


void avigo_init_machine(void)
{
        unsigned char *addr;

	/* initialise flash memory */
	flash_init(0);
	flash_restore(0, "avigof1.nv");
	flash_reset(0);

        flash_init(1);
        flash_restore(1, "avigof2.nv");
        flash_reset(1);

        flash_init(2);
        flash_restore(2, "avigof3.nv");
        flash_reset(2);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);

        cpu_setbankhandler_w(5, MWA_BANK5);
        cpu_setbankhandler_w(6, MWA_BANK6);
        cpu_setbankhandler_w(7, MWA_BANK7);
        cpu_setbankhandler_w(8, MWA_BANK8);

        avigo_irq = 0;
        avigo_rom_bank_l = 0;
        avigo_rom_bank_h = 0;
        avigo_ram_bank_l = 0;
        avigo_ram_bank_h = 0;
        avigo_flash_at_0x4000 = 0;
        avigo_flash_at_0x8000 = 0;

        tc8521_init(&avigo_tc8521_interface);

        uart8250_init(0, avigo_com_interface);
        uart8250_reset(0);

	/* allocate memory */
	avigo_memory = (unsigned char *)malloc(128*1024);

        if (avigo_memory!=NULL)
        {
                memset(avigo_memory, 0, 128*1024);
        }

        addr = (unsigned char *)flash_get_base(0);
        cpu_setbank(1, addr);
        cpu_setbank(5, addr);

        /* initialise fixed settings */
	cpu_setbankhandler_r(1, avigo_flash_0x0000_read_handler);
	cpu_setbankhandler_w(5, avigo_flash_0x0000_write_handler);

        addr = avigo_memory;
        cpu_setbank(4, addr);
        cpu_setbank(8, addr);

	cpu_setbankhandler_r(4, avigo_ram_0xc000_read_handler);
        cpu_setbankhandler_w(8, avigo_ram_0xc000_write_handler);


	/* 0x08000 is specially banked! */

        avigo_refresh_memory();
}

void avigo_shutdown_machine(void)
{
	/* store and free flash memory */
	flash_store(0, "avigof1.nv");
	flash_finish(0);

        flash_store(1, "avigof2.nv");
        flash_finish(1);

        flash_store(2, "avigof3.nv");
        flash_finish(2);

        tc8521_stop();

	/* free memory */
	if (avigo_memory!=NULL)
    {
            free(avigo_memory);
            avigo_memory = NULL;
    }

}


MEMORY_READ_START( readmem_avigo )
        {0x00000, 0x03fff, MRA_BANK1},
        {0x04000, 0x07fff, MRA_BANK2},
        {0x08000, 0x0bfff, MRA_BANK3},
        {0x0c000, 0x0ffff, MRA_BANK4},
MEMORY_END


MEMORY_WRITE_START( writemem_avigo )
        {0x00000, 0x03fff, MWA_BANK5},
        {0x04000, 0x07fff, MWA_BANK6},
        {0x08000, 0x0bfff, MWA_BANK7},
        {0x0c000, 0x0ffff, MWA_BANK8},
MEMORY_END







READ_HANDLER(avigo_key_data_read_r)
{
	UINT8 data;

	data = 0x007;

	if (avigo_key_line & 0x01)
	{
		data &= readinputport(0);
	}

	if (avigo_key_line & 0x02)
	{
		data &= readinputport(1);
	}

	if (avigo_key_line & 0x04)
	{
		data &= readinputport(2);

	}

	/* bit 3 must be set, otherwise there is an infinite loop in startup */
	data |= (1<<3);

	return data;
}


/* set key line(s) to read */
/* bit 0 set for line 0, bit 1 set for line 1, bit 2 set for line 2 */
WRITE_HANDLER(avigo_set_key_line_w)
{
	/* 5, 101, read back 3 */
	avigo_key_line = data;
}

READ_HANDLER(avigo_irq_r)
{
	return avigo_irq;
}

WRITE_HANDLER(avigo_irq_w)
{
        avigo_irq &= ~data;

	avigo_refresh_ints();
}

READ_HANDLER(avigo_rom_bank_l_r)
{
	return avigo_rom_bank_l;
}

READ_HANDLER(avigo_rom_bank_h_r)
{
	return avigo_rom_bank_h;
}

READ_HANDLER(avigo_ram_bank_l_r)
{
	return avigo_ram_bank_l;
}

READ_HANDLER(avigo_ram_bank_h_r)
{
	return avigo_ram_bank_h;
}



WRITE_HANDLER(avigo_rom_bank_l_w)
{
	logerror("rom bank l w: %04x\r\n", data);

        avigo_rom_bank_l = data & 0x03f;

        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_rom_bank_h_w)
{
	logerror("rom bank h w: %04x\r\n", data);


        /* 000 = flash 0
           001 = ram select
           011 = flash 1 (rom at ram - block 1 select)
           101 = flash 2
           110 = screen select?
           111 = flash 0 (rom at ram?)

       
        */
	avigo_rom_bank_h = data;


        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_ram_bank_l_w)
{
	logerror("ram bank l w: %04x\r\n", data);

        avigo_ram_bank_l = data & 0x03f;

        avigo_refresh_memory();
}

WRITE_HANDLER(avigo_ram_bank_h_w)
{
	logerror("ram bank h w: %04x\r\n", data);

	avigo_ram_bank_h = data;
                     
        avigo_refresh_memory();
}

READ_HANDLER(avigo_ad_control_status_r)
{
	logerror("avigo ad control read\n");

        return avigo_ad_control_status;
}

WRITE_HANDLER(avigo_ad_control_status_w)
{
	logerror("avigo ad control w %02x\n",data);

        avigo_ad_control_status = data | 1;
}

READ_HANDLER(avigo_ad_data_r)
{
	logerror("avigo ad read\n");

	return 0x00;	//ff;

//        switch (avigo_ad_control_status & 0x0fe)
  //      {
    //       case 0x020:
      //      return 0x0fd;
     //      case 0x060:
     //       return 0x0fd;
     // /     default:
     //      break;
     //   }
//
  //      return 0;


}


WRITE_HANDLER(avigo_speaker_w)
{
	UINT8 previous_speaker;

	previous_speaker = avigo_speaker_data;
	avigo_speaker_data = data;

	/* changed state? */
        if (((data^avigo_speaker_data) & (1<<3))!=0)
	{
		/* DAC output state */
		speaker_level_w(0,(data>>3) & 0x01);
	}
}

READ_HANDLER(avigo_unmapped_r)
{
	logerror("read unmapped port\n",offset);
	
	return 0x0ff;
}

/* port 0x04:

  bit 7: ??? if set, does a write 0x00 to 0x02e */

  /* port 0x029:
	port 0x02e */
READ_HANDLER(avigo_04_r)
{
	return 0x07f;
}


PORT_READ_START( readport_avigo )
	{0x000, 0x000, avigo_unmapped_r},
    {0x001, 0x001, avigo_key_data_read_r},
	{0x002, 0x002, avigo_unmapped_r},
	{0x003, 0x003, avigo_irq_r},
	{0x004, 0x004, avigo_04_r},
	{0x005, 0x005, avigo_rom_bank_l_r},
	{0x006, 0x006, avigo_rom_bank_h_r},
	{0x007, 0x007, avigo_ram_bank_l_r},
	{0x008, 0x008, avigo_ram_bank_h_r},
    {0x009, 0x009, avigo_ad_control_status_r},
	{0x00a, 0x00f, avigo_unmapped_r},
	{0x010, 0x01f, tc8521_r},
	{0x020, 0x02c, avigo_unmapped_r},
    {0x02d, 0x02d, avigo_ad_data_r},
	{0x02e, 0x02f, avigo_unmapped_r},
	{0x030, 0x037, uart8250_0_r},
	{0x038, 0x0ff, avigo_unmapped_r},
PORT_END

PORT_WRITE_START( writeport_avigo )
	{0x001, 0x001, avigo_set_key_line_w},
	{0x003, 0x003, avigo_irq_w},
	{0x005, 0x005, avigo_rom_bank_l_w},
	{0x006, 0x006, avigo_rom_bank_h_w},
	{0x007, 0x007, avigo_ram_bank_l_w},
	{0x008, 0x008, avigo_ram_bank_h_w},
    {0x009, 0x009, avigo_ad_control_status_w},
   	{0x010, 0x01f, tc8521_w},
	{0x028, 0x028, avigo_speaker_w},
	{0x030, 0x037, uart8250_0_w},
PORT_END
        

INPUT_PORTS_START(avigo)
	PORT_START
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "PAGE UP", KEYCODE_PGUP, IP_JOY_NONE)
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "PAGE DOWN", KEYCODE_PGDN, IP_JOY_NONE)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LIGHT", KEYCODE_L, IP_JOY_NONE)
        PORT_BIT (0x0f7, 0xf7, IPT_UNUSED)
	
	PORT_START
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "TO DO", KEYCODE_T, IP_JOY_NONE)
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "ADDRESS", KEYCODE_A, IP_JOY_NONE)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SCHEDULE", KEYCODE_S, IP_JOY_NONE)
        PORT_BIT (0x0f7, 0xf7, IPT_UNUSED)

	PORT_START
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "MEMO", KEYCODE_M, IP_JOY_NONE)
        PORT_BIT (0x0fe, 0xfe, IPT_UNUSED)
INPUT_PORTS_END

static struct Speaker_interface avigo_speaker_interface=
{
 1,
 {50},
};

static struct MachineDriver machine_driver_avigo =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
                        CPU_Z80 ,  /* type */
                        4000000, /* clock: See Note Above */
                        readmem_avigo,                   /* MemoryReadAddress */
                        writemem_avigo,                  /* MemoryWriteAddress */
                        readport_avigo,                  /* IOReadPort */
                        writeport_avigo,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        avigo_init_machine,                      /* init machine */
        avigo_shutdown_machine,
	/* video hardware */
        AVIGO_SCREEN_WIDTH, /* screen width */
        AVIGO_SCREEN_HEIGHT,  /* screen height */
        {0, (AVIGO_SCREEN_WIDTH - 1), 0, (AVIGO_SCREEN_HEIGHT - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        AVIGO_NUM_COLOURS,                                                        /* total colours */
        AVIGO_NUM_COLOURS,                                                        /* color table len */
        avigo_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        avigo_vh_start,
        avigo_vh_stop,
        avigo_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
		{
                SOUND_SPEAKER,
                &avigo_speaker_interface
        },
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(avigo)
/*        ROM_REGION(((64*1024)+(1024*1024)+), REGION_CPU1)
        ROM_LOAD("avigo.rom", 0x010000, 0x020000, 0x0000)
*/
ROM_END

static const struct IODevice io_avigo[] =
{

	{IO_END}
};


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 1997, avigo,   0,                avigo,  avigo,      0,       "Texas Instruments", "avigo")

