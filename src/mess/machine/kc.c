/******************************************************************************

    kc.c
	system driver

	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80fmly.h"
#include "cpu\z80\z80.h"
#include "mess/vidhrdw/kc.h"

#define KC85_4_SCREEN_WIDTH 320
#define KC85_4_SCREEN_HEIGHT 200

#define KC85_4_SCREEN_PIXEL_RAM_SIZE 0x04000
#define KC85_4_SCREEN_COLOUR_RAM_SIZE 0x04000

static void kc85_4_update_0x0c000(void);
static void kc85_4_update_0x0e000(void);
static void kc85_4_update_0x08000(void);
//static void kc85_4_update_0x04000(void);

unsigned char *kc85_ram;

unsigned char *kc85_4_display_video_ram;
unsigned char *kc85_4_video_ram;

static int kc85_4_pio_data[4];

/* port 0x084/0x085:

bit 7: RAF3
bit 6: RAF2
bit 5: RAF1
bit 4: RAF0
bit 3: FPIX. high resolution
bit 2: BLA1 .access screen
bit 1: BLA0 .pixel/color
bit 0: BILD .display screen 0 or 1
*/

/* port 0x086/0x087:

bit 7: ROCC
bit 6: ROF1
bit 5: ROF0
bit 4-2 are not connected
bit 1: WRITE PROTECT RAM 4
bit 0: ACCESS RAM 4
*/

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */



static int kc85_84_data;
static int kc85_86_data;

//static int opbase_reset_done = 0;

OPBASE_HANDLER( kc85_opbaseoverride )
{
        //if (!opbase_reset_done)
        //{
        //    opbase_reset_done = 1;

            cpu_setOPbaseoverride(0,0);

            cpu_set_reg(Z80_PC, 0x0f000);

            return (cpu_get_pc() & 0x0ffff);

        //}
        //
        //return PC;
}


static READ_HANDLER ( kc85_4_pio_data_r )
{

        return z80pio_d_r(0,offset);
}

static READ_HANDLER ( kc85_4_pio_control_r )
{

        return z80pio_c_r(0,offset);
}


static WRITE_HANDLER ( kc85_4_pio_data_w )
{

   {
   int PC = cpu_get_pc();

        logerror( "PIO W: PC: %04x O %02x D %02x\r\n",PC, offset, data);
   }

   kc85_4_pio_data[offset] = data;

   z80pio_d_w(0, offset, data);


   if (offset==0)
   {
           kc85_4_update_0x0c000();
           kc85_4_update_0x0e000();
   }

   //if (offset==1)
   //{
        kc85_4_update_0x08000();
   //}
}

static WRITE_HANDLER ( kc85_4_pio_control_w )
{
   z80pio_c_w(0, offset, data);
}


static READ_HANDLER ( kc85_4_ctc_r )
{
	return z80ctc_0_r(offset);
}

static WRITE_HANDLER ( kc85_4_ctc_w )
{
        logerror("CTC W: %02x\r\n",data);

        z80ctc_0_w(offset,data);
}

static WRITE_HANDLER ( kc85_4_84_w )
{
        logerror("0x084 W: %02x\r\n",data);

        kc85_84_data = data;

        {
                /* calculate address of video ram to display */
                unsigned char *video_ram;

                video_ram = kc85_4_video_ram;

                if (data & 0x01)
                {
                        video_ram +=
                                   (KC85_4_SCREEN_PIXEL_RAM_SIZE +
                                   KC85_4_SCREEN_COLOUR_RAM_SIZE);
                }

                kc85_4_display_video_ram = video_ram;
        }

        kc85_4_update_0x08000();

}

static READ_HANDLER ( kc85_4_84_r )
{
	return kc85_84_data;
}

/* port 0x086:

bit 7: CAOS ROM C
bit 6:
bit 5:
bit 4:
bit 3:
bit 2:
bit 1: write protect ram 4
bit 0: ram 4
*/

static void kc85_4_update_0x08000(void)
{
        unsigned char *ram_page;

        if (kc85_4_pio_data[0] & 4)
        {
                /* IRM enabled - has priority over RAM8 enabled */
                logerror("IRM enabled\r\n");

                /* base address: screen 0 pixel data */
                ram_page = kc85_4_video_ram;


                {

                        if (kc85_84_data & 0x04)
                        {
                                logerror("access screen 1\r\n");
                        }
                        else
                        {
                                logerror("access screen 0\r\n");
                        }

                        if (kc85_84_data & 0x02)
                        {
                                logerror("access colour\r\n");
                        }
                        else
                        {
                                logerror("access pixel\r\n");
                        }
               }


                if (kc85_84_data & 0x04)
                {
                        /* access screen 1 */
                        ram_page += KC85_4_SCREEN_PIXEL_RAM_SIZE +
                                KC85_4_SCREEN_COLOUR_RAM_SIZE;
                }

                if (kc85_84_data & 0x02)
                {
                        /* access colour information of selected screen */
                        ram_page += KC85_4_SCREEN_PIXEL_RAM_SIZE;
                }


               cpu_setbank(3, ram_page);


               cpu_setbankhandler_r(3, MRA_BANK3);
               cpu_setbankhandler_w(3, MWA_BANK3);


        }
        else
        if (kc85_4_pio_data[1] & 0x020)
        {
                /* RAM8 ACCESS */
                logerror("RAM8 enabled\r\n");

                ram_page = kc85_ram + 0x08000;

                cpu_setbank(3, ram_page);


                cpu_setbankhandler_r(3, MRA_BANK3);

                /* write protect RAM8 ? */
                if (kc85_4_pio_data[1] & 0x040)
                {
                        cpu_setbankhandler_w(3,MWA_NOP);
                }
                else
                {
                        cpu_setbankhandler_w(3, MWA_BANK3);
                }

        }
        else
        {
                cpu_setbankhandler_r(3, MRA_NOP);
                cpu_setbankhandler_w(3, MWA_NOP);
        }
}

static WRITE_HANDLER ( kc85_4_86_w )
{
        logerror("0x086 W: %02x\r\n",data);

	kc85_86_data = data;

        kc85_4_update_0x0c000();
}

static READ_HANDLER ( kc85_4_86_r )
{
	return kc85_86_data;
}

static void kc85_4_pio_interrupt(int state)
{
        cpu_cause_interrupt(0, Z80_VECTOR(0, state));

}

static void kc85_4_ctc_interrupt(int state)
{
        cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

static z80pio_interface kc85_4_pio_intf =
{
	1,					/* number of PIOs to emulate */
	{ kc85_4_pio_interrupt },	/* callback when change interrupt status */
	{ 0 },				/* portA ready active callback (do not support yet)*/
	{ 0 }				/* portB ready active callback (do not support yet)*/
};

int keyboard_data = 0;

static WRITE_HANDLER ( kc85_4_zc2_callback )
{
z80ctc_trg_w(0, 3, 0,keyboard_data);
z80ctc_trg_w(0, 3, 1,keyboard_data);
z80ctc_trg_w(0, 3, 2,keyboard_data);
z80ctc_trg_w(0, 3, 3,keyboard_data);

keyboard_data^=0x0ff;
}


#define KC85_4_CTC_CLOCK		4000000
static z80ctc_interface	kc85_4_ctc_intf =
{
	1,
	{KC85_4_CTC_CLOCK},
	{0},
        {kc85_4_ctc_interrupt},
	{0},
	{0},
    {kc85_4_zc2_callback}
};

static Z80_DaisyChain kc85_4_daisy_chain[] =
{
        {z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};

/* update memory address 0x0c000-0x0e000 */
static void kc85_4_update_0x0c000(void)
{
        if (kc85_86_data & 0x080)
	{
		/* CAOS rom takes priority */

                logerror("CAOS rom 0x0c000\r\n");

                cpu_setbank(1,memory_region(REGION_CPU1) + 0x012000);
     //           cpu_setbankhandler_r(1,MRA_BANK1);
	}
	else
	if (kc85_4_pio_data[0] & 0x080)
	{
		/* BASIC takes next priority */
                logerror("BASIC rom 0x0c000\r\n");

                cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
       //         cpu_setbankhandler_r(1, MRA_BANK1);
	}
	else
	{
                logerror("No roms 0x0c000\r\n");

      //          cpu_setbankhandler_r(1, MRA_NOP);
	}
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_4_update_0x0e000(void)
{
	if (kc85_4_pio_data[0] & 0x01)
	{
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */

                logerror("CAOS rom 0x0e000\r\n");

		/* read will access the rom */
                cpu_setbank(2,memory_region(REGION_CPU1) + 0x014000);
       //         cpu_setbankhandler_r(2, MRA_BANK2);
	}
	else
	{
                logerror("no rom 0x0e000\r\n");

		/* enable empty space memory range 0x0e000-0x0ffff */
       //         cpu_setbankhandler_r(2, MRA_NOP);
	}
}


void    kc85_4_reset(void)
{
	z80ctc_reset(0);
	z80pio_reset(0);


        /* enable CAOS rom in range 0x0e000-0x0ffff */
        kc85_4_pio_data[0] = 1;
        kc85_4_update_0x0e000();


        /* this is temporary. Normally when a Z80 is reset, it will
        execute address 0. It appears the KC85/4 pages in the rom
        at address 0x0000-0x01000 which has a single jump in it,
        can't see yet where it disables it later!!!! so for now
        here will be a override */
        cpu_setOPbaseoverride(0, kc85_opbaseoverride);

}

void kc85_4_init_machine(void)
{
	z80pio_init(&kc85_4_pio_intf);
	z80ctc_init(&kc85_4_ctc_intf);

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_w(3, MWA_BANK3);

        kc85_ram = malloc(64*1024);
		if (!kc85_ram) return;

        kc85_4_video_ram = malloc(
        (KC85_4_SCREEN_COLOUR_RAM_SIZE*2) +
        (KC85_4_SCREEN_PIXEL_RAM_SIZE*2));
		if (!kc85_4_video_ram) return;

        kc85_4_display_video_ram = kc85_4_video_ram;

	kc85_4_reset();
}

void kc85_4_shutdown_machine(void)
{
}


static struct IOReadPort readport_kc85_4[] =
{
	{0x084, 0x084, kc85_4_84_r},
	{0x085, 0x085, kc85_4_84_r},
	{0x086, 0x086, kc85_4_86_r},
	{0x087, 0x087, kc85_4_86_r},
        {0x088, 0x089, kc85_4_pio_data_r},
        {0x08a, 0x08b, kc85_4_pio_control_r},
	{0x08c, 0x08f, kc85_4_ctc_r},
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_kc85_4[] =
{
        /* D05 decodes io ports on schematic */

        /* D08,D09 on schematic handle these ports */
	{0x084, 0x084, kc85_4_84_w},
	{0x085, 0x085, kc85_4_84_w},
	{0x086, 0x086, kc85_4_86_w},
	{0x087, 0x087, kc85_4_86_w},

        /* D06 on schematic handle these ports */
        {0x088, 0x089, kc85_4_pio_data_w},
        {0x08a, 0x08b, kc85_4_pio_control_w},

        /* D07 on schematic handle these ports */
        {0x08c, 0x08f, kc85_4_ctc_w },
	{-1}							   /* end of table */
};

#define KC85_4_PALETTE_SIZE 24

static unsigned short kc85_4_colour_table[KC85_4_PALETTE_SIZE] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23
};

static unsigned char kc85_4_palette[KC85_4_PALETTE_SIZE * 3] =
{
		 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xd0,
		 0xd0, 0x00, 0x00,
		 0xd0, 0x00, 0xd0,
		 0x00, 0xd0, 0x00,
		 0x00, 0xd0, 0xd0,
		 0xd0, 0x00, 0x00,
		 0xd0, 0xd0, 0xd0,

		 0x00, 0x00, 0x00,
		 0x60, 0x00, 0xa0,
		 0xa0, 0x60, 0x00,
		 0xa0, 0x00, 0x60,
		 0x00, 0xa0, 0x60,
		 0x00, 0x60, 0xa0,
		 0x30, 0xa0, 0x30,
		 0xd0, 0xd0, 0xd0,

		 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xa0,
		 0xa0, 0x00, 0x00,
		 0xa0, 0x00, 0xa0,
		 0x00, 0xa0, 0x00,
		 0x00, 0xa0, 0xa0,
		 0xa0, 0xa0, 0x00,
		 0xa0, 0xa0, 0xa0

};

/* Initialise the palette */
static void kc85_4_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy(sys_palette, kc85_4_palette, sizeof (kc85_4_palette));
	memcpy(sys_colortable, kc85_4_colour_table, sizeof (kc85_4_colour_table));
}

static struct MemoryReadAddress readmem_kc85_4[] =
{
        {0x00000, 0x07fff, MRA_RAM},

        {0x08000, 0x0a7ff, MRA_BANK3},
        {0x0a800, 0x0bfff, MRA_RAM},
        {0x0c000, 0x0dfff, MRA_BANK1},
	{0x0e000, 0x0ffff, MRA_BANK2},
	{-1}							   /* end of table */
};

static struct MemoryWriteAddress writemem_kc85_4[] =
{
        {0x00000, 0x07fff, MWA_RAM},
        {0x08000, 0x0a7ff, MWA_BANK3},
        {0x0a800, 0x0bfff, MWA_RAM},
        {0x0c000, 0x0dfff, MWA_NOP},
        {0x0e000, 0x0ffff, MWA_NOP},
        {-1}							   /* end of table */
};




INPUT_PORTS_START( kc85_4 )
        PORT_START
INPUT_PORTS_END



static struct MachineDriver machine_driver_kc85_4 =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80,  /* type */
			4000000,
			readmem_kc85_4,		   /* MemoryReadAddress */
			writemem_kc85_4,		   /* MemoryWriteAddress */
			readport_kc85_4,		   /* IOReadPort */
			writeport_kc85_4,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
                        kc85_4_daisy_chain
                },
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	kc85_4_init_machine,			   /* init machine */
	kc85_4_shutdown_machine,
	/* video hardware */
	KC85_4_SCREEN_WIDTH,			   /* screen width */
	KC85_4_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_4_SCREEN_WIDTH - 1), 0, (KC85_4_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_4_PALETTE_SIZE,								   /* total colours
									    */
	KC85_4_PALETTE_SIZE,								   /* color table len */
	kc85_4_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	kc85_4_vh_start,
	kc85_4_vh_stop,
	kc85_4_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
};




ROM_START(kc85_4)
	ROM_REGION(0x016000, REGION_CPU1)

        ROM_LOAD("basic_c0.rom", 0x10000, 0x2000, 0x0dfe34b08)
        ROM_LOAD("caos__c0.rom", 0x12000, 0x1000, 0x057d9ab02)
        ROM_LOAD("caos__e0.rom", 0x14000, 0x2000, 0x0d64cd50b)
ROM_END

static const struct IODevice io_kc85_4[] =
{
       {IO_END}
};



/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 19??, kc85_4,   0,     kc85_4,  kc85_4,        0,                "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)

