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
#define KC85_4_SCREEN_HEIGHT (32*8)

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
/*
Hi!

Sorry for the late answer. I'm not at the university anymore so I'm
not reading the mail there very often. The address still works but
i've registered an address at gmx.de that I can access much more
easily from home: Torsten.Paul@gmx.de

> From: "Kev Thacker" <kev@distdevs.co.uk>
> Date: Mon, 6 Mar 2000 11:23:12 -0000
>
> I have started a KC85/4 emulation for the MESS project. mess.emuverse.com
>
Yes, I already noticed this. Currently I don't have the time to follow
the MAME/MESS developement but I'm reading emulator news at retrogames.com
so atleast I can read about new interesting things.

> I used a lot of info that I found from looking at the source code to your
> emulator,
>
The naming of the roms looked a bit familar too ;-).

> and reading the docs from another emulator. I am lucky because in MESS the
> Z80 PIO, Z80 CTC,
> and Z80 are already written, so all I needed to write was the code to link
> these together.
>
Yup, the CTC is quite a complex thing to emulate. Fortunately the more
difficult PIO modes are not used by the KC.

> I hope that you will be able to help me so I can add keyboard
> emulation to it.
>
Oh yes the keyboard caused me a lot of trouble too. I still don't
understand it completely.
A first dirty but working keyboard support is quite easy because
of the good and modular system rom. Most programs use the supplied
routines to read keyboard input so patching the right memory address
with the keycode that is originally supplied by the interrupt
routines for keyboard input will work. So if you first want to
have a simple start to get other things to work you can try this.

    * write keycode (ascii) to address IX+0dh (0x1fd)
    * set bit 0 at address IX+08h (0x1f8) - simply writing 01h
      worked for me but some other bits are used too
      this bit is reset if the key is read by the system

> From the schematics I see that the keyboard is linked to the CTC and PIO,
> but I don't understand it fully.
> Please can you tell me more about how the keyboard is scanned?
>
The full emulation is quite tricky and I still have some programs that
behave odd. (A good hint for a correct keyboard emulation is Digger ;-).

Ok, now the technical things:

The keyboard of the KC is driven by a remote control circuit that is
originally designed for infrared remote control. This one was named
U807 and I learned there should be a similar chip called SAB 3021
available but I never found the specs on the web. The SAB 3021 was
produced by Valvo which doesn't exist anymore (bought by Phillips
if I remember correctly). If you have more luck finding the specs
I'm still interested.
There also was a complementary chip for the recieving side but that
was not used in the KC unfortunately. They choosed to measure the
pulses sent by the U807 via PIO and CTC.

Anyway here is what I know about the protocol:

The U807 sends impulses with equal length. The information is
given by the time between two impulses. Short time means bit is 1
long time mean bit is 0. The impulses are modulated by a 62.5 kHz
Signal but that's not interesting for the emulator.
The timing comes from a base clock of 4 MHz that is divided
multiple times:

4 MHz / 64 - 62.5 kHz -> used for filtered amplification
62.5 kHz / 64 - 976 Hz -> 1.024 ms
976 / 7 - 140 Hz -> 7.2 ms
976 / 5 - 195 Hz -> 5.1 ms

short - 5.12 ms - 1 bit
long - 7.168 ms - 0 bit

       +-+     +-+       +-+     +-+
       | |     | |       | |     | |
    +--+ +-----+ +-------+ +-----+ +--....

         |     | |---0---| |--1--|
            ^
            |
        Startbit = shift key

The keyboard can have 64 keys with an additional key that is used by
the KC as shift key. This additional key is directly connected to the
chip and changes the startbit. All other keys are arranged in a matrix.

The chip sends full words of 7 bits (including the startbit) at least
twice for each keypress with a spacing of 14 * 1.024 ms. If the key
is still pressed the double word is repeated after 19 * 1.024 ms.

The impulses trigger the pio interrupt line for channel B that
triggers the time measurement by the CTC channel 3.

That's all for today :-). I'm sure I've forgotten some things so
feel free to mail again...

ciao,
  Torsten.


--
Torsten.Paul@gmx.de      True wealth is measured not by what you
//     accumulate, but by what you pass
__________________ooO_(+ +)_Ooo__________ on to others. (Larry Wall)
U
*/

/* the following is a temporary key emulation. This works by poking OS
variables. Thankyou to Torsten Paul for the details.

  The actual key scanning uses a special transmission protocol where
  pulses are measured by the CTC and PIO */

/* load image */
int kc_load(char *filename, int addr)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_ROM,OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				int i;

				/* read whole file */
				osd_fread(file, data, datasize);

				for (i=0; i<datasize; i++)
				{
					cpu_writemem16(addr+i, data[i]);
				}

				/* close file */
				osd_fclose(file);

				free(data);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}





#define KC85_4_KEYCODE_QUEUE_LENGTH 256
static unsigned char kc85_4_keycodes[KC85_4_KEYCODE_QUEUE_LENGTH];
static int kc85_4_keycodes_head;
static int kc85_4_keycodes_tail;

static void kc85_4_keyboard_update(int);


/* initialise keyboard queue */
static void kc85_4_keyboard_init(void)
{
	/* head and tail of list is at beginning */
	kc85_4_keycodes_head = kc85_4_keycodes_tail = 0;

	/* 50hz is just a arbitrary value */
	timer_pulse(TIME_IN_HZ(50), 0, kc85_4_keyboard_update);
}


/* add a key to the queue */
static void kc85_4_queue_key(int code)
{
	/* add a key */
	kc85_4_keycodes[kc85_4_keycodes_tail] = code;
	/* update next insert position */
	kc85_4_keycodes_tail = (kc85_4_keycodes_tail + 1) % KC85_4_KEYCODE_QUEUE_LENGTH;
}

/* 1 = true, to send keycode, 0 = false, do not send keycode */
static int kc85_4_can_send_keycode(void)
{
	/* temporary until correct hardware key emulation has been done */
	/* check if os has read key yet */

	int flag;

	flag = cpu_readmem16(0x01f8);
	flag = flag & 0x01;

	if (flag==0)
		return 1;

	return 0;
}

static void kc85_4_send_keycode(int code)
{
	int flag;

	/* temporary until correct hardware key emulation has been done */

	/* write code into OS variables */
	cpu_writemem16(0x01fd, code);

	/* indicate char is ready in OS variables */
	flag = cpu_readmem16(0x01f8);
	flag |= 0x01;
	cpu_writemem16(0x01f8, flag);
}

#define KC85_4_KEYBOARD_NUM_LINES	9

/* this table will eventually hold the actual keycodes */
//static int kc85_4_keycode_translation[64];


/* previous state of keyboard - currently used to detect if a key has been pressed/released  since last scan */
static int kc85_4_previous_keyboard[KC85_4_KEYBOARD_NUM_LINES];

/* update keyboard */
void	kc85_4_keyboard_update(int dummy)
{
	int i;

	/* scan all lines */
	for (i=0; i<8; i++)
	{
		int b;
		int keyboard_line_data;
		int changed_keys;
		int mask = 0x080;

		/* read input port */
		keyboard_line_data = readinputport(i);
		/* identify keys that have changed */
		changed_keys = keyboard_line_data ^ kc85_4_previous_keyboard[i];
		/* store input port for next time */
		kc85_4_previous_keyboard[i] = keyboard_line_data;

		/* scan through each bit */
		for (b=0; b<8; b++)
		{
			/* key changed? */
			if (changed_keys & mask)
			{
				/* yes */

				/* new state is pressed? */
				if ((keyboard_line_data & mask)!=0)
				{
					/* pressed */
					int ascii_code;
					int fake_code;

					/* generate fake code */
					fake_code = (i<<3) | (7-b);

					if ((fake_code>=0) && (fake_code<=26))
					{
						/* shift? */
						if ((readinputport(8) & 0x03)!=0)
						{
							ascii_code = fake_code + 'A';
							/* lookup actual kc85/4 keycode - and add to queue */
							kc85_4_queue_key(ascii_code);

						}
						else
						{
							ascii_code = fake_code + 'a';
							/* lookup actual kc85/4 keycode - and add to queue */
							kc85_4_queue_key(ascii_code);
						}
					}
					else if ((fake_code>=27) && (fake_code<=(27+9)))
					{
						if (fake_code==27)
						{
							if ((readinputport(8) & 0x03)!=0)
							{
								ascii_code = '@';
							}
							else
							{
								ascii_code = '0';
							}
						}
						else
						{

							if ((readinputport(8) & 0x03)!=0)
							{
								ascii_code = fake_code - 27 + '!';
							}
							else
							{
								ascii_code = fake_code + '0' - 27;
							}
						}
						/* lookup actual kc85/4 keycode - and add to queue */
						kc85_4_queue_key(ascii_code);
					}
					else if (fake_code == ((6*8) + 4))
					{
						kc85_4_queue_key(13);
					}
					else if (fake_code == ((5*8) + 5))
					{
						kc85_4_queue_key(127);
					}
					else if (fake_code == ((6*8) + 5))
					{
						kc85_4_queue_key(32);
					}
					else if (fake_code == ((6*8) + 6))
					{
						if ((readinputport(8) & 0x03)!=0)
						{
							ascii_code = '<';
						}
						else
						{
							ascii_code = ',';
						}
						kc85_4_queue_key(ascii_code);
					}
					else if (fake_code == ((6*8) + 7))
					{
						if ((readinputport(8) & 0x03)!=0)
						{
							ascii_code = '>';
						}
						else
						{
							ascii_code = '.';
						}
						kc85_4_queue_key(ascii_code);
					}
					else if (fake_code == ((7*8) + 1))
					{
						if ((readinputport(8) & 0x03)!=0)
						{
							ascii_code = ':';
						}
						else
						{
							ascii_code = '*';
						}

						kc85_4_queue_key(ascii_code);
					}


				//	/* lookup actual kc85/4 keycode - and add to queue */
				//	kc85_4_queue_key(ascii_code);


				}
			}

			mask = mask>>1;
		}
	}


	/* transmit a new keycode? */
	if (kc85_4_can_send_keycode())
	{

		/* keycode available? */
		if (kc85_4_keycodes_head!=kc85_4_keycodes_tail)
		{
			int code;

			/* get code */
			code = kc85_4_keycodes[kc85_4_keycodes_head];
			/* update start of list */
			kc85_4_keycodes_head = (kc85_4_keycodes_head + 1) % KC85_4_KEYCODE_QUEUE_LENGTH;

			/* send the code */
			kc85_4_send_keycode(code);

		}
	}
}


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

	kc_load("bd_0400.prg", 0x0400);
    kc_load("bd_4000.scr", 0x4000);


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
//z80ctc_trg_w(0, 3, 0,keyboard_data);
//z80ctc_trg_w(0, 3, 1,keyboard_data);
//z80ctc_trg_w(0, 3, 2,keyboard_data);
//z80ctc_trg_w(0, 3, 3,keyboard_data);

//keyboard_data^=0x0ff;
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

	kc85_4_keyboard_init();


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



/* this is a fake keyboard layout. The keys are converted into codes which are transmitted to the kc85/4 */
INPUT_PORTS_START( kc85_4 )
        PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
        PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M",KEYCODE_M, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
        PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U",KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
        PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 @", KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
        PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 *", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (", KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 )", KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F3", KEYCODE_F3, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE)
		PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F5", KEYCODE_F5, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F6", KEYCODE_F6, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "BRK", KEYCODE_F7, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STOP", KEYCODE_F8, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "INS", KEYCODE_F9, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_F10, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CLR", KEYCODE_F11, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "HOME", KEYCODE_F12, IP_JOY_NONE)
		PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR UP", KEYCODE_UP, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR DOWN", KEYCODE_DOWN, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR LEFT", KEYCODE_LEFT, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
		PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN/ENTER", KEYCODE_ENTER, IP_JOY_NONE)
		PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)
		PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

		PORT_START
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": *", KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- = ", KEYCODE_EQUALS, IP_JOY_NONE)

		PORT_START
		/* has a single shift key. Mapped here to left and right shift */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
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

