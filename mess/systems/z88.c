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

struct blink_hw blink;

static void blink_reset(void)
{
	memset(&blink, 0, sizeof(struct blink_hw));

	/* rams is cleared on reset */
	blink.com &=~(1<<2);
	blink.sbf = 0;
	blink.z88_state = Z88_AWAKE;

}

/* load image */
static void z88_dump_ram(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, "z88.bin", FILETYPE_NVRAM,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;
		for (i=0; i<65536; i++)
		{
			char data;

			data = cpu_readmem16(i);

			mame_fwrite(file, &data, 1);

		}

		/* close file */
		mame_fclose(file);
	}

	file = mame_fopen(Machine->gamedrv->name, "z88b.bin", FILETYPE_NVRAM,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;

/*		for (i=0; i<(8*16384); i++)
		{
			char data;

			data = memory_region(REGION_CPU1)[0x010000+i];

			mame_fwrite(file, &data, 1);
		}

		for (i=0; i<((32-8)*16384); i++)
		{
			char data;

			data = 0;
			
			mame_fwrite(file, &data, 1);
		}
*/
		for (i=0; i<(2048*1024); i++)
		{
			char data;

			data = z88_memory[i];

			mame_fwrite(file, &data, 1);
		}

		/* close file */
		mame_fclose(file);
	}


}


static void z88_interrupt_refresh(void)
{
	/* ints enabled? */
	if ((blink.ints & INT_GINT)!=0)
	{
		/* yes */

		/* other ints - except timer */
		if (
			(((blink.ints & blink.sta) & 0x0fc)!=0) ||
			(((blink.ints>>1) & blink.sta & STA_TIME)!=0)
			)
		{
			logerror("set int\n");
			cpu_set_irq_line(0,0,HOLD_LINE);
			return;
		}
	}
	
	logerror("clear int\n");
	cpu_set_irq_line(0,0,CLEAR_LINE);
}

static void z88_update_rtc_interrupt(void)
{
	blink.sta &=~STA_TIME;

	/* time interrupt enabled? */
	if (blink.ints & INT_TIME)
	{
		/* yes */

		/* any ints occured? */
		if ((blink.tsta & 0x07)!=0)
		{
			/* yes, set time int */
			blink.sta |= STA_TIME;
		}
	}
}



static void z88_rtc_timer_callback(int dummy)
{
	int refresh_ints = 0;

	if ((readinputport(0) & 0x040)==0)
	{
		z88_dump_ram();
	}

	/* is z88 in snooze state? */
	if (blink.z88_state == Z88_SNOOZE)
	{
		int i;
		unsigned char data = 0x0ff;

		/* any key pressed will wake up z88 */
		for (i=0; i<8; i++)
		{
			data &= readinputport(i);
		}

		/* if any key is pressed, then one or more bits will be 0 */
		if (data!=0x0ff)
		{
			logerror("Z88 wake up from snooze!\n");

			/* wake up z88 */
			blink.z88_state = Z88_AWAKE;
			/* column has gone low in snooze/coma */
			blink.sta |= STA_KEY;

			cpu_trigger(Z88_SNOOZE_TRIGGER);
		
			z88_interrupt_refresh();
		}
	}



	/* hold clock at reset? - in this mode it doesn't update */
	if ((blink.com & (1<<4))==0)
	{
		/* update 5 millisecond counter */
		blink.tim[0]++;

		/* tick */
		if ((blink.tim[0]%10)==0)
		{
			/* set tick int has occured */
			blink.tsta |= RTC_TICK_INT;
			refresh_ints = 1;
		}

		if (blink.tim[0]==200)
		{
			blink.tim[0] = 0;

			/* set seconds int has occured */
			blink.tsta |= RTC_SEC_INT;
			refresh_ints = 1;

			blink.tim[1]++;

			if (blink.tim[1]==60)
			{
				/* set minutes int has occured */
				blink.tsta |=RTC_MIN_INT;
				refresh_ints = 1;
				
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

	if (refresh_ints)
	{
		z88_update_rtc_interrupt();

		/* refresh */
		z88_interrupt_refresh();
	}
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

	/* bank index for read's for use with cpu_setbank, etc */
	read_bank = 2+index1;
	/* bank index for write's for use with cpu_setbank, etc */
	write_bank = 7+index1;

	logerror("%02x: offset %04x\n",index1, blink.mem[index1]<<14);

    /* ram? */
    if (blink.mem[index1]>=0x020)
    {
       block = blink.mem[index1]-0x020;

	   if (block>=128)
	   {
		   
     //  logerror("%02x: unavailable ram\n",index1);

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

			 //  logerror("%02x: ram\n",index1);

			memory_set_bankhandler_r(read_bank, 0, z88_read_handler[index1+1]);
			memory_set_bankhandler_w(write_bank, 0, z88_write_handler[index1+1]);

			addr = z88_memory + (block<<14);

			if (index1==0)
			{
				memory_set_bankhandler_r(read_bank-1, 0, z88_read_handler[index1]);
				memory_set_bankhandler_w(write_bank-1, 0, z88_write_handler[index1]);

				cpu_setbank(read_bank, addr+0x02000);
				cpu_setbank(write_bank, addr+0x02000);
				cpu_setbank(read_bank-1, addr);
				cpu_setbank(write_bank-1, addr);
			}
			else
			{
				cpu_setbank(read_bank, addr);
				cpu_setbank(write_bank, addr);
			}
		}
	}
    else
    {
       block = blink.mem[index1];

	   block = block & 0x07;

       /* in rom area, but rom not present */
       if (block>=8)
       {
	//	   logerror("%02x: unmapped rom\n",index1);

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
	//	   logerror("%02x: rom\n",index1);

			addr = memory_region(REGION_CPU1) + 0x010000;
			memory_set_bankhandler_r(read_bank, 0, z88_read_handler[index1+1]);
			memory_set_bankhandler_w(write_bank, 0, MWA_NOP);

			addr = addr + (block<<14);

			if (index1==0)
			{
				memory_set_bankhandler_r(read_bank-1, 0, z88_read_handler[index1]);
				memory_set_bankhandler_w(write_bank-1, 0, MWA_NOP);

				cpu_setbank(read_bank-1, addr);
				cpu_setbank(read_bank, addr+0x02000);
			}
			else
			{
				cpu_setbank(read_bank, addr);
			}
       }
    }

	if (index1==0)
	{
		/* override setting for lower 8k of bank 0 */

		/* enable rom? */
		if ((blink.com & (1<<2))==0)
		{
			/* yes */
			addr = memory_region(REGION_CPU1) + 0x010000;

			cpu_setbank(1, addr);
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(6, 0, MWA_NOP);

			logerror("lower 8k is rom\n");
		}
		else
		{
			/* ram bank 20 */
			addr = z88_memory;

			cpu_setbank(1, addr);
			cpu_setbank(6, addr);
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(6, 0, MWA_BANK6);
			
			logerror("lower 8k is ram\n");
		}
	}
}

static MACHINE_INIT( z88 )
{
	z88_memory = auto_malloc(2048*1024);

	if (z88_memory)
		memset(z88_memory, 0x0ff, 2048*1024);

	timer_pulse(TIME_IN_MSEC(5), 0, z88_rtc_timer_callback);

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

	z88_refresh_memory_bank(0);
	z88_refresh_memory_bank(1);
	z88_refresh_memory_bank(2);
	z88_refresh_memory_bank(3);
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
#if 0
unsigned long blink_pb_offset(int num_bits, unsigned long addr_written, int shift)
{
	unsigned long offset;
	unsigned long h,b;

	h = addr_written & 0x01f;
	b = addr_written & 0x01fe0;

	offset = ((b<<2) | h)<<shift;

	logerror("offset: %04x\n",offset);
   //     unsigned long offset;
//
  //      offset = addr_written & ((unsigned long)0x0ffffffff>>(16-num_bits));
//
  //      offset = (
    //                    ((addr_written & 0x01fe0)<<2) |
      //                  (addr_written & 0x01f)
       //                 );
      //  offset = offset<<shift;

        return offset;
}
#endif
static void blink_pb_w(int offset, int data, int reg_index)
{
    unsigned short addr_written = (offset & 0x0ff00) | (data & 0x0ff);

	logerror("reg_index: %02x addr: %04x\n",reg_index,addr_written);

    switch (reg_index)
	{
	
		/* 1c000 */
	
	case 0x00:
		{
/**/            blink.pb[0] = addr_written;
            blink.lores0 = ((addr_written & 0x01f)<<9) | ((addr_written & 0x01fe0)<<9);	// blink_pb_offset(-1, addr_written, 9);
            logerror("lores0 %08x\n",blink.lores0);
		}
		break;

		case 0x01:
		{
            blink.pb[1] = addr_written;
            blink.lores1 = ((addr_written & 0x01f)<<12) | ((addr_written & 0x01fe0)<<12);	//blink_pb_offset(-1, addr_written, 12);
            logerror("lores1 %08x\n",blink.lores1);
		}
		break;

		case 0x02:
		{
            blink.pb[2] = addr_written;
/**/            blink.hires0 = ((addr_written & 0x01f)<<13) | ((addr_written & 0x01fe0)<<13);	//blink_pb_offset(-1, addr_written, 13);
            logerror("hires0 %08x\n", blink.hires0);
		}
		break;


		case 0x03:
		{
            blink.pb[3] = addr_written;
            blink.hires1 = ((addr_written & 0x01f)<<11) | ((addr_written & 0x01fe0)<<11);	//blink_pb_offset(-1, addr_written, 11);

            logerror("hires1 %08x\n", blink.hires1);
		}
		break;

		case 0x04:
		{
            blink.sbr = addr_written;

			blink.sbf = ((addr_written & 0x01f)<<11) | ((addr_written & 0x01fe0)<<11);
            logerror("%08x\n", blink.sbf);

		}
		break;

		default:
			break;
	}
}


/* segment register write */
static WRITE_HANDLER(blink_srx_w)
{
	blink.mem[offset] = data;

	z88_refresh_memory_bank(offset);
}
/*
 00b0 00
blink w: 00d0 00
blink w: 00d1 00
blink w: 00d2 00
blink w: 00d3 00
blink w: 01b5 01
blink w: 01b4 01
blink w: 03b1 03
blink w: 03b6 03
*/

static WRITE_HANDLER(z88_port_w)
{
	unsigned char port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x070:
		case 0x071:
		case 0x072:
		case 0x073:
		case 0x074:
			blink_pb_w(offset, data, port & 0x0f);
			return;


		/* write rtc interrupt acknowledge */
		case 0x0b4:
		{
		    logerror("tack w: %02x\n", data);

			/* set acknowledge */
			blink.tack = data & 0x07;
			/* clear ints that have occured */
			blink.tsta &= ~blink.tack;

			/* refresh ints */
			z88_update_rtc_interrupt();
			z88_interrupt_refresh();
		}
		return;

		/* write rtc interrupt mask */
		case 0x0b5:
		{
		    logerror("tmk w: %02x\n", data);

			/* set new int mask */
			blink.tmk = data & 0x07;

			/* refresh ints */
			z88_update_rtc_interrupt();
			z88_interrupt_refresh();
		}
		return;

		case 0x0b0:
		{
			UINT8 changed_bits;

		    logerror("com w: %02x\n", data);

			changed_bits = blink.com^data;
			blink.com = data;

			/* reset clock? */
			if ((data & (1<<4))!=0)
			{
				blink.tim[0] = (blink.tim[1] = (blink.tim[2] = (blink.tim[3] = (blink.tim[4] = 0))));
			}

			/* SBIT controls speaker direct? */
			if ((data & (1<<7))==0)
			{
			   /* yes */

			   /* speaker controlled by SBIT */
			   if ((changed_bits & (1<<6))!=0)
			   {
				   speaker_level_w(0, (data>>6) & 0x01);
			   }
			}
			else
			{
			   /* speaker under control of continuous tone,
			   or txd */


			}

			if ((changed_bits & (1<<2))!=0)
			{
				z88_refresh_memory_bank(0);
			}
		}
		return;

		case 0x0b1:
		{
			/* set int enables */
		    logerror("int w: %02x\n", data);

			blink.ints = data;
			z88_update_rtc_interrupt();
			z88_interrupt_refresh();
		}
		return;


		case 0x0b6:
		{
		    logerror("ack w: %02x\n", data);

			/* acknowledge ints */
			blink.ack = data & ((1<<6) | (1<<5) | (1<<3) | (1<<2));

			blink.ints &= ~blink.ack;
			z88_update_rtc_interrupt();
			z88_interrupt_refresh();
		}
		return;



		case 0x0d0:
		case 0x0d1:
		case 0x0d2:
		case 0x0d3:
			blink_srx_w(port & 0x03, data);
			return;
	}


    logerror("blink w: %04x %02x\n", offset, data);

}

static READ_HANDLER(z88_port_r)
{
	unsigned char port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x0b1:
			blink.sta &=~(1<<1);
			logerror("sta r: %02x\n",blink.sta);
			return blink.sta;


		case 0x0b2:
		{
			unsigned char data = 0x0ff;

			int lines;

			lines = offset>>8;

			/* if set, reading the keyboard will put z88 into snooze */
			if ((blink.ints & INT_KWAIT)!=0)
			{
				blink.z88_state = Z88_SNOOZE;
				/* spin cycles until rtc timer */
				cpu_spinuntil_trigger (Z88_SNOOZE_TRIGGER);

				logerror("z88 entering snooze!\n");
			}


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

			logerror("lines: %02x\n",lines);
			logerror("key r: %02x\n",data);
			return data;
		}

		/* read real time clock status */
		case 0x0b5:
			blink.tsta &=~0x07;
			logerror("tsta r: %02x\n",blink.tsta);
			return blink.tsta;

		/* read real time clock counters */
		case 0x0d0:
		    logerror("tim0 r: %02x\n", blink.tim[0]);
			return blink.tim[0] & 0x0ff;
		case 0x0d1:
			logerror("tim1 r: %02x\n", blink.tim[1]);
			return blink.tim[1] & 0x03f;
		case 0x0d2:
			logerror("tim2 r: %02x\n", blink.tim[2]);
			return blink.tim[2] & 0x0ff;
		case 0x0d3:
			logerror("tim3 r: %02x\n", blink.tim[3]);
			return blink.tim[3] & 0x0ff;
		case 0x0d4:
			logerror("tim4 r: %02x\n", blink.tim[4]);
			return blink.tim[4] & 0x01f;

		default:
			break;

	}

	logerror("blink r: %04x \n", offset);


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
	/* 0 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"8", KEYCODE_8, IP_JOY_NONE)
	/* 1 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"I", KEYCODE_I, IP_JOY_NONE)
	/* 2 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"O", KEYCODE_O, IP_JOY_NONE)
	/* 3 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"9", KEYCODE_9, IP_JOY_NONE)
	/* 4 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"LEFT", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"2", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"P", KEYCODE_P, IP_JOY_NONE)
	/* 5 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"SPACE", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"0", KEYCODE_0, IP_JOY_NONE)
	/* 6 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"HELP", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"LEFT SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"TAB", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"DIA", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"MENU", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"'", KEYCODE_4_PAD, IP_JOY_NONE) 
	/* 7 */
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"SQR", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"ESC", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"INDEX", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"£", KEYCODE_TILDE, IP_JOY_NONE)

INPUT_PORTS_END

static MACHINE_DRIVER_START( z88 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3276800)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem_z88, writemem_z88)
	MDRV_CPU_PORTS(readport_z88, writeport_z88)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( z88 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(Z88_SCREEN_WIDTH, 480)
	MDRV_VISIBLE_AREA(0, (Z88_SCREEN_WIDTH - 1), 0, (480 - 1))
	MDRV_PALETTE_LENGTH(Z88_NUM_COLOURS)
	MDRV_COLORTABLE_LENGTH(Z88_NUM_COLOURS)
	MDRV_PALETTE_INIT( z88 )

	MDRV_VIDEO_EOF( z88 )
	MDRV_VIDEO_UPDATE( z88 )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, z88_speaker_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(z88)
    ROM_REGION(((64*1024)+(128*1024)), REGION_CPU1,0)
    ROM_LOAD("z88v400.rom", 0x010000, 0x020000, CRC(1356d440))
ROM_END

/*	   YEAR	    NAME	PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG	COMPANY					FULLNAME */
COMPX( 1988,	z88,	0,		0,		z88,		z88,		0,		NULL,	"Cambridge Computers",	"Z88",GAME_NOT_WORKING)

