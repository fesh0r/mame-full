/******************************************************************************

        z88.c

        z88 Notepad computer


        system driver


        TODO:
        - speaker controlled by constant tone or txd


        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/z88.h"

unsigned char *z88_memory = NULL;
static void *z88_rtc_timer = NULL;


struct blink_hw blink;

static void blink_reset(void)
{
	/* rams is cleared on reset */
        blink.com &=~(1<<2);
        blink.sbf = 0;
}


static void z88_interrupt_refresh(void)
{

	/* ints enabled? */
	if ((blink.ints & (1<<0))!=0)
	{
		/* yes */

		if (
			/* rtc ints */
			(
			/* rtc can trigger int */
			(((blink.tsta & blink.tmk) & 0x07)!=0) &&
			/* rtc interrupts are enabled */
			((blink.ints & (1<<1))!=0)
			) ||
			(
			/* other ints */
			((blink.sta & blink.ints)!=0)
			)
			)

		{
			cpu_set_irq_line(0,0,HOLD_LINE);
                        return;
                }
	}

        cpu_set_irq_line(0,0,CLEAR_LINE);
}


static void z88_rtc_timer_callback(int dummy)
{
	/* hold clock at reset? - in this mode it doesn't update */
	if ((blink.com & (1<<4))==0)
	{

		blink.tim[0]++;

                if ((blink.tim[0]%10)==0)
                {
                  /* set tick int has occured */
                  blink.tsta |= (1<<0);
                }

		if (blink.tim[0]==200)
		{
			blink.tim[0] = 0;

                        /* set seconds int has occured */
                        blink.tsta |= (1<<1);

			blink.tim[1]++;

			if (blink.tim[1]==60)
			{
                                /* set minutes int has occured */
                                blink.tsta |=(1<<2);

				blink.tim[1]=0;

				blink.tim[2]++;

				if (blink.tim[2]==256)
				{
                                        blink.tim[2] = 0;

					blink.tim[3]++;

					if (blink.tim[3]==256)
					{
						blink.tim[3] = 0;

						blink.tim[4]++;

						if (blink.tim[4]==32)
						{
							blink.tim[4] = 0;
						}
					}
				}
			}
		}
	}

        z88_interrupt_refresh();
}

static mem_read_handler z88_read_handler[] =
{
 MRA_BANK1, MRA_BANK2, MRA_BANK3, MRA_BANK4, MRA_BANK5
};


static mem_write_handler z88_write_handler[] =
{
 MWA_BANK6, MWA_BANK7, MWA_BANK8, MWA_BANK9, MWA_BANK10
};

/* Assumption:

all banks can access the same memory blocks in the same way.
bank 0 is special. If a bit is set in the com register,
the lower 8k is replaced with the rom. Bank 0 has been split
into 2 8k chunks, and all other banks into 16k chunks.
I wanted to handle all banks in the code below, and this
explains why the extra checks are done */

static void z88_refresh_memory_bank(int index1)
{
        unsigned char *addr;
        unsigned long block;
        int read_bank,write_bank;

        /* bank index for read's for use with cpu_setbank, cpu_setbankhandler */
        read_bank = 2+index1;
        /* bank index for write's for use with cpu_setbank, cpu_setbankhandler */
        write_bank = 7+index1;

        /* ram? */
        if (blink.mem[index1] & 0x020)
        {
           block = blink.mem[index1] & (~0x020);

           logerror("ram selected\r\n");

           addr = z88_memory;
           memory_set_bankhandler_r(read_bank, 0, z88_read_handler[index1+1]);
           memory_set_bankhandler_w(write_bank, 0, z88_write_handler[index1+1]);

           if (index1==0)
           {
                memory_set_bankhandler_r(read_bank-1, 0, z88_read_handler[index1]);
                memory_set_bankhandler_w(write_bank-1, 0, z88_write_handler[index1]);
           }

           addr = addr + (block<<14);

           cpu_setbank(read_bank, addr);
           cpu_setbank(write_bank, addr);

           if (index1==0)
           {
                cpu_setbank(read_bank-1, addr+0x02000);
                cpu_setbank(write_bank-1, addr+0x02000);
           }
        }
        else
        {
           block = blink.mem[index1];

           /* in rom area, but rom not present */
           if (block & 8)
           {
                logerror("unmapped rom selected\r\n");

                memory_set_bankhandler_r(read_bank, 0, MRA_NOP);
                memory_set_bankhandler_w(write_bank, 0, MWA_NOP);

                if (index1==0)
                {
                        memory_set_bankhandler_r(read_bank-1, 0, MRA_NOP);
                        memory_set_bankhandler_w(write_bank-1, 0, MWA_NOP);
                }
           }
           else
           {
                logerror("rom selected\r\n");

                addr = memory_region(REGION_CPU1) + 0x010000;
                memory_set_bankhandler_r(read_bank, 0, z88_read_handler[index1+1]);
                memory_set_bankhandler_w(write_bank, 0, MWA_NOP);

                if (index1==0)
                {
                        memory_set_bankhandler_r(read_bank-1, 0, z88_read_handler[index1]);
                        memory_set_bankhandler_w(write_bank-1, 0, z88_write_handler[index1]);
                }

                addr = addr + (block<<14);
                cpu_setbank(read_bank, addr);
                cpu_setbank(write_bank, addr);

                if (index1==0)
                {
                        cpu_setbank(read_bank-1, addr+0x02000);
                        cpu_setbank(write_bank-1, addr+0x02000);
                }
           }
        }
}


static void z88_refresh_memory(void)
{
        /* setup bank 0 like all other banks would be */
        z88_refresh_memory_bank(0);

        /* override setting for lower 8k of bank 0 */

	/* enable rom? */
	if ((blink.com & (1<<2))==0)
	{
                unsigned char *addr;

		/* yes */
		addr = memory_region(REGION_CPU1) + 0x010000;

		cpu_setbank(1, addr);
                memory_set_bankhandler_r(1, 0, MRA_BANK1);
                memory_set_bankhandler_w(6, 0, MWA_NOP);
	}
        else
        {
                /* ram bank 20 */
                unsigned char *addr;

                addr = z88_memory;

                cpu_setbank(1, addr);
                cpu_setbank(6, addr);
                memory_set_bankhandler_r(1, 0, MRA_BANK1);
                memory_set_bankhandler_w(6, 0, MWA_BANK6);
        }

        z88_refresh_memory_bank(1);
        z88_refresh_memory_bank(2);
        z88_refresh_memory_bank(3);
}

void z88_init_machine(void)
{
        z88_memory = malloc(2048*1024); //512*1024);

	z88_rtc_timer = timer_pulse(TIME_IN_MSEC(5), 0, z88_rtc_timer_callback);

        blink_reset();

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);
	memory_set_bankhandler_r(5, 0, MRA_BANK5);

	memory_set_bankhandler_w(6, 0, MWA_BANK6);
	memory_set_bankhandler_w(7, 0, MWA_BANK7);
	memory_set_bankhandler_w(8, 0, MWA_BANK8);
	memory_set_bankhandler_w(9, 0, MWA_BANK9);
	memory_set_bankhandler_w(10, 0, MWA_BANK10);

	z88_refresh_memory();

}


void z88_shutdown_machine(void)
{
	if (z88_memory!=NULL)
	{
		free(z88_memory);
		z88_memory = NULL;
	}

	if (z88_rtc_timer!=NULL)
	{
		timer_remove(z88_rtc_timer);
		z88_rtc_timer = NULL;
	}

}


static MEMORY_READ_START (readmem_z88)
        {0x00000, 0x01fff, MRA_BANK1},
        {0x02000, 0x03fff, MRA_BANK2},
        {0x04000, 0x07fff, MRA_BANK3},
		{0x08000, 0x0bfff, MRA_BANK4},
        {0x0c000, 0x0ffff, MRA_BANK5},
MEMORY_END

static MEMORY_WRITE_START (writemem_z88)
        {0x00000, 0x01fff, MWA_BANK6},
		{0x02000, 0x03fff, MWA_BANK7},
        {0x04000, 0x07fff, MWA_BANK8},
        {0x08000, 0x0bfff, MWA_BANK9},
        {0x0c000, 0x0ffff, MWA_BANK10},
MEMORY_END

unsigned long blink_pb_offset(int num_bits, unsigned long addr_written, int shift)
{
        unsigned long offset;

        offset = addr_written & ((unsigned long)0x0ffffffff>>(16-num_bits));

        offset = (
                        ((addr_written & 0x01fe0)<<2) |
                        (addr_written & 0x01f)
                        );
        offset = offset<<shift;

        return offset;
}

static void blink_pb_w(int offset, int data, int reg_index)
{
        unsigned short addr_written = (offset & 0x0ff00) | (data & 0x0ff);

        switch (reg_index)
	{
		case 0x00:
		{
                        blink.pb[0] = addr_written;
                        blink.lores0 = blink_pb_offset(13, addr_written, 9);
                        logerror("lores0 %04x\r\n",blink.lores0);
                }
		break;

		case 0x01:
		{
                        blink.pb[1] = addr_written;
                        blink.lores1 = blink_pb_offset(10, addr_written, 11);
                        logerror("lores1 %04x\r\n",blink.lores1);
		}
		break;

		case 0x02:
		{
                        blink.pb[2] = addr_written;
                        blink.hires0 = blink_pb_offset(9, addr_written, 13);
                        logerror("hires0 %04x\r\n", blink.hires0);
                }
		break;


		case 0x03:
		{
                        blink.pb[3] = addr_written;
                        blink.hires1 = blink_pb_offset(11, addr_written, 10);

                        logerror("hires1 %04x\r\n", blink.hires1);
                }
                break;

		case 0x04:
		{
                        blink.sbr = addr_written;
                        blink.sbf = blink_pb_offset(11, addr_written, 10);
                        logerror("%04x\r\n", blink.sbf);

                }
		break;

		default:
			break;
	}
}


/* segment register write */
WRITE_HANDLER(blink_srx_w)
{
	blink.mem[offset] = data;

	z88_refresh_memory();
}

WRITE_HANDLER(z88_port_w)
{
	unsigned char port;

	port = offset & 0x0ff;

        logerror("blink w: %04x %02x\r\n", offset, data);


	switch (port)
	{
		case 0x070:
		case 0x071:
		case 0x072:
		case 0x073:
		case 0x074:
			blink_pb_w(offset, data, port-0x070);
			break;


		case 0x0b4:
		{
			/* rtc int acknowledge */

			/* set acknowledge */
			blink.tack = data & 0x07;
			/* clear ints that have occured */
			blink.tsta &= ~blink.tack;

			/* refresh ints */
			z88_interrupt_refresh();
		}
		break;

		case 0x0b5:
		{
			/* rtc int mask */

			/* set new int mask */
			blink.tmk = data & 0x07;

			/* refresh ints */
			z88_interrupt_refresh();
		}
		break;

		case 0x0b0:
		{
                        UINT8 previous_com;

                        previous_com = blink.com;

			blink.com = data;

			/* reset clock? */
			if ((data & (1<<4))!=0)
			{
				blink.tim[0] = (blink.tim[1] = (blink.tim[2] = (blink.tim[3] = (blink.tim[4] = 0))));
			}

                        /* SBIT controls speaker direct? */
                        if ((blink.com & (1<<7))==0)
                        {
                           /* yes */

                           /* speaker controlled by SBIT */
                           if (((previous_com^blink.com) & (1<<6))!=0)
                           {
                               speaker_level_w(0, (blink.com>>6) & 0x01);
                           }
                        }
                        else
                        {
                           /* speaker under control of continuous tone,
                           or txd */


                        }

			z88_refresh_memory();
		}
		break;

		case 0x0b1:
		{
			/* set int enables */

			blink.ints = data;

			z88_interrupt_refresh();
		}
		break;


		case 0x0b6:
		{
			/* acknowledge ints */
                        blink.ack = data & ((1<<6) | (1<<5) | (1<<3) | (1<<2));

			blink.ints &= ~blink.ack;

			z88_interrupt_refresh();
		}
		break;



		case 0x0d0:
		case 0x0d1:
		case 0x0d2:
		case 0x0d3:
			blink_srx_w(port-0x0d0, data);
			return;
	}
}

READ_HANDLER(z88_port_r)
{
	unsigned char port;

	port = offset & 0x0ff;

        logerror("blink r: %04x \r\n", offset);

	switch (port)
	{
		case 0x0b0:
		{
			return blink.com;
		}
		break;

		case 0x0b1:
		{
			return blink.sta & (~(1<<1));
		}
		break;


		case 0x0b2:
		{
			unsigned char data = 0x0ff;

			int lines;

			lines = offset>>8;

			if ((lines & 0x080)==0)
				data &=readinputport(7);

			if ((lines & 0x040)==0)
				data &=readinputport(6);

			if ((lines & 0x020)==0)
				data &=readinputport(5);

			if ((lines & 0x010)==0)
				data &=readinputport(4);

			if ((lines & 0x008)==0)
				data &=readinputport(3);

			if ((lines & 0x004)==0)
				data &=readinputport(2);

			if ((lines & 0x002)==0)
				data &=readinputport(1);

			if ((lines & 0x001)==0)
				data &=readinputport(0);

			return data;
		}
		break;


		/* read int status */
		case 0x0b5:
		{
			return blink.tsta;
		}
		break;


		case 0x0d0:
			return blink.tim[0] & 0x0ff;
		case 0x0d1:
			return blink.tim[1] & 0x03f;
		case 0x0d2:
			return blink.tim[2] & 0x0ff;
		case 0x0d3:
			return blink.tim[3] & 0x0ff;
		case 0x0d4:
			return blink.tim[4] & 0x01f;


		default:
			break;

	}


	return 0x0ff;
}


static PORT_READ_START (readport_z88)
	{0x0000, 0x0ffff, z88_port_r},
PORT_END

static PORT_WRITE_START (writeport_z88)
	{0x0000, 0x0ffff, z88_port_w},
PORT_END

static struct Speaker_interface z88_speaker_interface=
{
  1,
  {50}
};

/*
-------------------------------------------------------------------------
         | D7     D6      D5      D4      D3      D2      D1      D0
-------------------------------------------------------------------------
A15 (#7) | RSH    SQR     ESC     INDEX   CAPS    .       /       £
A14 (#6) | HELP   LSH     TAB     DIA     MENU    ,       ;       '
A13 (#5) | [      SPACE   1       Q       A       Z       L       0
A12 (#4) | ]      LFT     2       W       S       X       M       P
A11 (#3) | -      RGT     3       E       D       C       K       9
A10 (#2) | =      DWN     4       R       F       V       J       O
A9  (#1) | \      UP      5       T       G       B       U       I
A8  (#0) | DEL    ENTER   6       Y       H       N       7       8
-------------------------------------------------------------------------
*/

INPUT_PORTS_START(z88)
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"8", KEYCODE_8, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"I", KEYCODE_I, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"O", KEYCODE_O, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"9", KEYCODE_9, IP_JOY_NONE)


	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"9", KEYCODE_9, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"SPACE", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"0", KEYCODE_0, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"LEFT SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"TAB", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"DIA", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"MENU", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"'", KEYCODE_QUOTE, IP_JOY_NONE)


	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"SQR", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"ESC", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"INDEX", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"£", KEYCODE_TILDE, IP_JOY_NONE)

INPUT_PORTS_END

static struct MachineDriver machine_driver_z88 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
                        CPU_Z80 | CPU_16BIT_PORT ,  /* type */
                        3276800, /* clock */
                        readmem_z88,                   /* MemoryReadAddress */
                        writemem_z88,                  /* MemoryWriteAddress */
                        readport_z88,                  /* IOReadPort */
                        writeport_z88,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        z88_init_machine,                      /* init machine */
        z88_shutdown_machine,
	/* video hardware */
        Z88_SCREEN_WIDTH, /* screen width */
        480,  /* screen height */
        {0, (Z88_SCREEN_WIDTH - 1), 0, (480 - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        Z88_NUM_COLOURS,                                                        /* total colours */
        Z88_NUM_COLOURS,                                                        /* color table len */
        z88_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        z88_vh_start,
        z88_vh_stop,
        z88_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
        {
                {
                SOUND_SPEAKER,
                &z88_speaker_interface,
                }
        }

};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(z88)
        ROM_REGION(((64*1024)+(128*1024)), REGION_CPU1,0)
        ROM_LOAD("z88v400.rom", 0x010000, 0x020000, 0x1356d440)
ROM_END

static const struct IODevice io_z88[] =
{

	{IO_END}
};


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 19??, z88,   0,                z88,  z88,      0,       "Acorn", "z88")

