/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "ctype.h"
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "includes/wd179x.h"
#include "includes/bbc.h"
#include "includes/upd7002.h"
#include "includes/i8271.h"
#include "devices/basicdsk.h"
#include "image.h"

int startbank;

/*************************
memory handleing functions
*************************/

/* for the model A just address the 4 on board ROM sockets */
WRITE8_HANDLER ( page_selecta_w )
{
	cpu_setbank(3,memory_region(REGION_USER1)+((data&0x03)<<14));
}

/* for the model B address all 16 of the ROM sockets */
WRITE8_HANDLER ( page_selectb_w )
{
	cpu_setbank(3,memory_region(REGION_USER1)+((data&0x0f)<<14));
}


WRITE8_HANDLER ( memory_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}



/****************************************/
/* BBC B Plus memory handling function */
/****************************************/

static int pagedRAM=0;
static int vdusel=0;
static int rombankselect=0;
/* the model B plus addresses all 16 of the ROM sockets plus the extra 12K of ram at 0x8000
   and 20K of shadow ram at 0x3000 */
WRITE8_HANDLER ( page_selectbp_w )
{
	if ((offset&0x04)==0)
	{
		pagedRAM=(data&0x80)>>7;
		rombankselect=data&0x0f;

		if (pagedRAM)
		{
			/* if paged ram then set 8000 to afff to read from the ram 8000 to afff */
			cpu_setbank(3,memory_region(REGION_CPU1)+0x8000);
		} else {
			/* if paged rom then set the rom to be read from 8000 to afff */
			cpu_setbank(3,memory_region(REGION_USER1)+(rombankselect<<14));
		};

		/* set the rom to be read from b000 to bfff */
		cpu_setbank(4,memory_region(REGION_USER1)+(rombankselect<<14)+0x03000);
	}
	else
	{
		//the video display should now use this flag to display the shadow ram memory
		vdusel=(data&0x80)>>7;
		bbcbp_setvideoshadow(vdusel);
		//need to make the video display do a full screen refresh for the new memory area
	}
}


/* write to the normal memory from 0x0000 to 0x2fff
   the writes to this memory are just done the normal
   way */

WRITE8_HANDLER ( memorybp0_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}


/*  this function should return true if
	the instruction is in the VDU driver address ranged
	these are set when:
	PC is in the range c000 to dfff
	or if pagedRAM set and PC is in the range a000 to afff
*/
static int vdudriverset(void)
{
	int PC;
	PC=activecpu_get_pc(); // this needs to be set to the 6502 program counter
	return (((PC>=0xc000) && (PC<=0xdfff)) || ((pagedRAM) && ((PC>=0xa000) && (PC<=0xafff))));
}

/* the next two function handle reads and write to the shadow video ram area
   between 0x3000 and 0x7fff

   when vdusel is set high the video display uses the shadow ram memory
   the processor only reads and write to the shadow ram when vdusel is set
   and when the instruction being executed is stored in a set range of memory
   addresses known as the VDU driver instructions.
*/


 READ8_HANDLER ( memorybp1_r )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just read normal ram
		return memory_region(REGION_CPU1)[offset+0x3000];
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then read from shadow ram
			return memory_region(REGION_CPU1)[offset+0xb000];
		} else {
			// else read from normal ram
			return memory_region(REGION_CPU1)[offset+0x3000];
		}
	}
}

WRITE8_HANDLER ( memorybp1_w )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just write to normal ram
		memory_region(REGION_CPU1)[offset+0x3000]=data;
		vidmem[offset+0x3000]=1;
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then write to shadow ram
			memory_region(REGION_CPU1)[offset+0xb000]=data;
			vidmem[offset+0xb000]=1;
		} else {
			// else write to normal ram
			memory_region(REGION_CPU1)[offset+0x3000]=data;
			vidmem[offset+0x3000]=1;
		}
	}
}


/* if the pagedRAM is set write to RAM between 0x8000 to 0xafff
otherwise this area contains ROM so no write is required */
WRITE8_HANDLER ( memorybp3_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
}


/* the BBC B plus 128K had extra ram mapped in replacing the
rom bank 0,1,c and d.
The function memorybp3_128_w handles memory writes from 0x8000 to 0xafff
which could either be sideways ROM, paged RAM, or sideways RAM.
The function memorybp4_128_w handles memory writes from 0xb000 to 0xbfff
which could either be sideways ROM or sideways RAM */


static unsigned short bbc_b_plus_sideways_ram_banks[16]=
{
	1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0
};


WRITE8_HANDLER ( memorybp3_128_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
	else
	{
		if (bbc_b_plus_sideways_ram_banks[rombankselect])
		{
			memory_region(REGION_USER1)[offset+(rombankselect<<14)]=data;
		}
	}
}

WRITE8_HANDLER ( memorybp4_128_w )
{
	if (bbc_b_plus_sideways_ram_banks[rombankselect])
	{
		memory_region(REGION_USER1)[offset+(rombankselect<<14)+0x3000]=data;
	}
}





/** via irq status local store **/

static int via_system_irq=0;
static int via_user_irq=0;


/******************************************************************************

System VIA 6522

PA0-PA7
Port A forms a slow data bus to the keyboard sound and speech processors
PortA	Keyboard
D0	Pin 8
D1	Pin 9
D2	Pin 10
D3	Pin 11
D4	Pin 5
D5	Pin 6
D6	Pin 7
D7	Pin 12

PB0-PB2 outputs
---------------
These 3 outputs form the address to an 8 bit addressable latch.
(IC32 74LS259)


PB3 output
----------
This output holds the data to be written to the selected
addressable latch bit.


PB4 and PB5 inputs
These are the inputs from the joystick FIRE buttons. They are
normally at logic 1 with no button pressed and change to 0
when a button is pressed.
----------------------------------------------------------------
PB4 and PB5 are not needed in initial emulation
and should be set to logic high, should be mapped through to
joystick emulator later.


PB6 and PB7 inputs from the speech processor
PB6 is the speech processor 'ready' output and PB7 is from the
speech processor 'interrupt' output.
----------------------------------------------------------------
PB6 and PB7 are not needed in intial emulation
and should be set to logic high, should be mapped through to
speech emulator later.


CA1 input
This is the vertical sync input from the 6845. CA1 is set up to
interrupt the 6502 every 20ms (50Hz) as a vertical sync from
the video circuity is detected. The operation system changes
the flash colours on the display in this interrupt time so that
they maintain synchronisation with the rest of the picture.
----------------------------------------------------------------
This is required for a lot of time function within the machine
and must be triggered every 20ms. (Should check at some point
how this 20ms signal is made, and see if none standard shapped
screen modes change this time period.)


CB1 input
The CB1 input is the end of conversion (EOC) signal from the
7002 analogue to digital converter. It can be used to interrupt
the 6502 whenever a conversion is complete.
----------------------------------------------------------------
CB1 is not needed in the initial emulation
and should be set to logic high, should be mapped through to
joystick emulator later.


CA2 input
This input comes from the keyboard circuit, and is used to
generate an interrupt whenever a key is pressed. See the
keyboard circuit section for more details.
----------------------------------------------------------------
Is required to detect keyboard presses.


CB2 input
This is the light pen strobe signal (LPSTB) from the light pen.
If also connects to the 6845 video processor,
CB2 can be programmed to interrupt the processor whenever
a light pen strobe occurs.
----------------------------------------------------------------
CB2 is not needed in the initial emulation
and should be set to logic low, should be mapped through to
a light pen emulator later.

IRQ output
This connects to the IRQ line of the 6502


The addressable latch
This 8 bit addressable latch is operated from port B lines 0-3.
PB0-PB2 are set to the required address of the output bit to be set.
PB3 is set to the value which should be programmed at that bit.
The function of the 8 output bits from this latch are:-

B0 - Write Enable to the sound generator IC (Not needed initally)
B1 - READ select on the speech processor    (Not needed initally)
B2 - WRITE select on the speech processor   (Not needed initally)
B3 - Keyboard write enable	            ***NEEDED***
B4,B5 - these two outputs define the number to be added to the
start of screen address in hardware to control hardware scrolling:-
Mode	Size	Start of screen	 Number to add	B5   	B4
0,1,2	20K	&3000		 12K		1    	1
3	16K	&4000		 16K		0	0
4,5	10K	&5800 (or &1800) 22K		1	0
6	8K	&6000 (or &2000) 24K		0	1
B6 - Operates the CAPS lock LED  (Pin 17 keyboard connector)
B7 - Operates the SHIFT lock LED (Pin 16 keyboard connector)


******************************************************************************/

static int b0_sound;
static int b1_speech_read;
static int b2_speech_write;
static int b3_keyboard;
static int b4_video0;
static int b5_video1;
static int b6_caps_lock_led;
static int b7_shift_lock_led;


static int via_system_porta;


static int column=0;

INTERRUPT_GEN( bbcb_keyscan )
{
  	/* only do auto scan if keyboard is not enabled */
	if (b3_keyboard==1)
	{

  		/* KBD IC1 4 bit addressable counter */
  		/* KBD IC3 4 to 10 line decoder */
		/* keyboard not enabled so increment counter */
		column=(column+1)%16;
		if (column<10)
		{

  			/* KBD IC4 8 input NAND gate */
  			/* set the value of via_system ca2, by checking for any keys
    			 being pressed on the selected column */

			if ((readinputport(column)|0x01)!=0xff)
			{
				via_0_ca2_w(0,1);
			} else {
				via_0_ca2_w(0,0);
			}

		} else {
			via_0_ca2_w(0,0);
		}
	}
}

static int bbc_keyboard(int data)
{
	int bit;
	int row;
	int res;
	column=data & 0x0f;
	row=(data>>4) & 0x07;

	bit=0;
		if (column<10) {
			res=readinputport(column);
		} else {
			res=0xff;
		}

		/* temp development fix to force start up into screen mode 0 */
		//if ((row==0) && ((column==7) || (column==8) || (column==9)))
		//	{ bit=1; }

		/* Normal keyboard result */
		if ((res&(1<<row))==0)
			{ bit=1; }

		if ((res|1)!=0xff)
		{
			via_0_ca2_w(0,1);
		} else {
			via_0_ca2_w(0,0);
		}

	return (data & 0x7f) | (bit<<7);
}


static void bbcb_IC32_initialise(void)
{
	b0_sound=0x01;
	b1_speech_read=0x01;
	b2_speech_write=0x01;
	b3_keyboard=0x01;
	b4_video0=0x01;
	b5_video1=0x01;
	b6_caps_lock_led=0x01;
	b7_shift_lock_led=0x01;
}

static WRITE8_HANDLER( bbcb_via_system_write_porta )
{
	via_system_porta=data;
	if (b0_sound==0)
	{
		SN76496_0_w(0,via_system_porta);
	}
	if (b1_speech_read==0)
	{
		// Call Speech
	}
	if (b2_speech_write==0)
	{
		// Call Speech
	}
	if (b3_keyboard==0)
	{
		via_system_porta=bbc_keyboard(via_system_porta);
	}

}


static WRITE8_HANDLER( bbcb_via_system_write_portb )
{
	int bit,value;
	bit=data & 0x07;
	value=(data>>3) &0x01;

	if (value) {
		switch (bit) {
		case 0:
			if (b0_sound==0) {
				b0_sound=1;
			}
			break;
		case 1:
			if (b1_speech_read==0) {
				b1_speech_read=1;
			}
			break;
		case 2:
			if (b2_speech_write==0) {
				b2_speech_write=1;
			}
			break;
		case 3:
			if (b3_keyboard==0) {
				b3_keyboard=1;
			}
			break;
		case 4:
			if (b4_video0==0) {
				b4_video0=1;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 5:
			if (b5_video1==0) {
				b5_video1=1;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 6:
			if (b6_caps_lock_led==0) {
				b6_caps_lock_led=1;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (b7_shift_lock_led==0) {
				b7_shift_lock_led=1;
				/* call shift lock led update */
			}
			break;

	}
	} else {
		switch (bit) {
		case 0:
			if (b0_sound==1) {
				b0_sound=0;
				SN76496_0_w(0,via_system_porta);
			}
			break;
		case 1:
			if (b1_speech_read==1) {
				b1_speech_read=0;
				via_system_porta=0xff;
			}
			break;
		case 2:
			if (b2_speech_write==1) {
				b2_speech_write=0;
			}
			break;
		case 3:
			if (b3_keyboard==1) {
				b3_keyboard=0;
				/* *** call keyboard enabled *** */
				via_system_porta=bbc_keyboard(via_system_porta);
			}
			break;
		case 4:
			if (b4_video0==1) {
				b4_video0=0;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 5:
			if (b5_video1==1) {
				b5_video1=0;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 6:
			if (b6_caps_lock_led==1) {
				b6_caps_lock_led=0;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (b7_shift_lock_led==1) {
				b7_shift_lock_led=0;
				/* call shift lock led update */
			}
			break;
		}
	}

}

static  READ8_HANDLER( bbcb_via_system_read_porta )
{
  return via_system_porta;
}


// D4 of portb is joystick fire button 1
// D5 of portb is joystick fire button 2
static  READ8_HANDLER( bbcb_via_system_read_portb )
{
  return (0xcf | readinputport(16));
}

/* vertical sync pulse from video circuit */
static  READ8_HANDLER( bbcb_via_system_read_ca1 )
{
  return 0x01;
}


/* joystick EOC */
static  READ8_HANDLER( bbcb_via_system_read_cb1 )
{
  return uPD7002_EOC_r(0);
}


/* keyboard pressed detect */
static  READ8_HANDLER( bbcb_via_system_read_ca2 )
{
  return 0x01;
}


/* light pen strobe detect (not emulated) */
static  READ8_HANDLER( bbcb_via_system_read_cb2 )
{
  return 0x01;
}


/* this is wired as in input port so writing to this port would be bad */

static WRITE8_HANDLER( bbcb_via_system_write_ca2 )
{
  //if( errorlog ) fprintf(errorlog, "via_system_write_ca2: $%02X\n", data);
}

/* this is wired as in input port so writing to this port would be bad */

static WRITE8_HANDLER( bbcb_via_system_write_cb2 )
{
  //if( errorlog ) fprintf(errorlog, "via_system_write_cb2: $%02X\n", data);
}



static void bbc_via_system_irq(int level)
{
  via_system_irq=level;
//  logerror("SYSTEM via irq %d %d %d\n",via_system_irq,via_user_irq,level);
  cpu_set_irq_line(0, M6502_IRQ_LINE, via_system_irq|via_user_irq);
}


static struct via6522_interface
bbcb_system_via= {
  bbcb_via_system_read_porta,
  bbcb_via_system_read_portb,
  bbcb_via_system_read_ca1,
  bbcb_via_system_read_cb1,
  bbcb_via_system_read_ca2,
  bbcb_via_system_read_cb2,
  bbcb_via_system_write_porta,
  bbcb_via_system_write_portb,
  bbcb_via_system_write_ca2,
  bbcb_via_system_write_cb2,
  bbc_via_system_irq
};


/**********************************************************************
USER VIA
Port A output is buffered before being connected to the printer connector.
This means that they can only be operated as output lines.
CA1 is pulled high by a 4K7 resistor. CA1 normally acts as an acknowledge
line when a printer is used. CA2 is buffered so that it has become an open
collector output only. It usially acts as the printer strobe line.
***********************************************************************/

/* this code just sends the output of the printer port to the errorlog
file. I now need to look at the new printer code and see how I should
connect this code up to that */

int bbc_printer_porta;
int bbc_printer_ca1;
int bbc_printer_ca2;

/* USER VIA 6522 port A is buffered as an output through IC70 so
reading from this port will always return 0xff */
static  READ8_HANDLER( bbcb_via_user_read_porta )
{
	return 0xff;
}

/* USER VIA 6522 port B is connected to the BBC user port */
static  READ8_HANDLER( bbcb_via_user_read_portb )
{
	return 0xff;
}

static  READ8_HANDLER( bbcb_via_user_read_ca1 )
{
	return bbc_printer_ca1;
}

static  READ8_HANDLER( bbcb_via_user_read_ca2 )
{
	return 1;
}

static WRITE8_HANDLER( bbcb_via_user_write_porta )
{
	bbc_printer_porta=data;
}

static WRITE8_HANDLER( bbcb_via_user_write_ca2 )
{
	/* write value to printer on rising edge of ca2 */
	if ((bbc_printer_ca2==0) && (data==1))
	{
		logerror("print character $%02X\n",bbc_printer_porta);
	}
	bbc_printer_ca2=data;

	/* this is a very bad way of returning an acknowledge
	by just linking the strobe output into the acknowledge input */
	bbc_printer_ca1=data;
	via_1_ca1_w(0,data);
}

static void bbc_via_user_irq(int level)
{
  via_user_irq=level;
//  logerror("USER via irq %d %d %d\n",via_system_irq,via_user_irq,level);
  cpu_set_irq_line(0, M6502_IRQ_LINE, via_system_irq|via_user_irq);
}


static struct via6522_interface bbcb_user_via =
{
	bbcb_via_user_read_porta,//via_user_read_porta,
	bbcb_via_user_read_portb,//via_user_read_portb,
	bbcb_via_user_read_ca1,//via_user_read_ca1,
	0,//via_user_read_cb1,
	bbcb_via_user_read_ca2,//via_user_read_ca2,
	0,//via_user_read_cb2,
	bbcb_via_user_write_porta,//via_user_write_porta,
	0,//via_user_write_portb,
	bbcb_via_user_write_ca2,//via_user_write_ca2,
	0,//via_user_write_cb2,
	bbc_via_user_irq //via_user_irq
};


/**************************************
BBC Joystick Support
**************************************/

static int BBC_get_analogue_input(int channel_number)
{
	switch(channel_number)
	{
		case 0:
			return ((0xff-readinputport(17))<<8);
			break;
		case 1:
			return ((0xff-readinputport(18))<<8);
			break;
		case 2:
			return ((0xff-readinputport(19))<<8);
			break;
		case 3:
			return ((0xff-readinputport(20))<<8);
			break;
	}

	return 0;
}

static void BBC_uPD7002_EOC(int data)
{
	via_0_cb1_w(0,data);
}

static struct uPD7002_interface
BBC_uPD7002= {
	BBC_get_analogue_input,
	BBC_uPD7002_EOC
};

/**************************************
   load floppy disc
***************************************/

DEVICE_LOAD( bbc_floppy )
{
	if (device_load_basicdsk_floppy(image, file)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, 80, 1, 10, 256, 0, 0, FALSE);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

/**************************************
   i8271 disc control function
***************************************/

static int previous_i8271_int_state;

static void	bbc_i8271_interrupt(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}


static i8271_interface bbc_i8271_interface=
{
	bbc_i8271_interrupt,
    NULL
};


 READ8_HANDLER( bbc_i8271_read )
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			return i8271_r(offset);
		case 4:
			return i8271_data_r(offset);
		default:
			break;
	}

	return 0x0ff;
}

WRITE8_HANDLER( bbc_i8271_write )
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(offset, data);
			return;
		case 4:
			i8271_data_w(offset, data);
			return;
		default:
			break;
	}
}



/**************************************
   WD1770 disc control function
***************************************/

static int  drive_control;
/* bit 0: 1 = drq set, bit 1: 1 = irq set */
static int	bbc_wd177x_drq_irq_state;

static int previous_wd177x_int_state;

/*
   B/ B+ drive control:

        Bit       Meaning
        -----------------
        7,6       Not used.
         5        Reset drive controller chip. (0 = reset controller, 1 = no reset)
         4        Interrupt Enable (0 = enable int, 1 = disable int)
         3        Double density select (0 = double, 1 = single).
         2        Side select (0 = side 0, 1 = side 1).
         1        Drive select 1.
         0        Drive select 0.
*/

/*
density select
single density is as the 8271 disc format
double density is as the 8271 disc format but with 16 sectors per track

At some point we need to check the size of the disc image to work out if it is a single or double
density disc image
*/


static void bbc_wd177x_callback(int event)
{
	int state;
	/* wd177x_IRQ_SET and latch bit 4 (nmi_enable) are NAND'ED together
	   wd177x_DRQ_SET and latch bit 4 (nmi_enable) are NAND'ED together
	   the output of the above two NAND gates are then OR'ED together and sent to the 6502 NMI line.
		DRQ and IRQ are active low outputs from wd177x. We use wd177x_DRQ_SET for DRQ = 0,
		and wd177x_DRQ_CLR for DRQ = 1. Similarly wd177x_IRQ_SET for IRQ = 0 and wd177x_IRQ_CLR
		for IRQ = 1.

	  The above means that if IRQ or DRQ are set, a interrupt should be generated.
	  The nmi_enable decides if interrupts are actually triggered.
	  The nmi is edge triggered, and triggers on a +ve edge.
	*/

	/* update bbc_wd177x_drq_irq_state depending on event */
	switch (event)
	{
        case WD179X_DRQ_SET:
		{
			bbc_wd177x_drq_irq_state |= 1;
		}
		break;

		case WD179X_DRQ_CLR:
		{
			bbc_wd177x_drq_irq_state &= ~1;
		}
		break;

		case  WD179X_IRQ_SET:
		{
			bbc_wd177x_drq_irq_state |= (1<<1);
		}
		break;

		case WD179X_IRQ_CLR:
		{
			bbc_wd177x_drq_irq_state &= ~(1<<1);
		}
		break;
	}

	/* if drq or irq is set, and interrupt is enabled */
	if (((bbc_wd177x_drq_irq_state & 3)!=0) && ((drive_control & (1<<4))==0))
	{
		/* int trigger */
		state = 1;
	}
	else
	{
		/* do not trigger int */
		state = 0;
	}

	/* nmi is edge triggered, and triggers when the state goes from clear->set.
	Here we are checking this transition before triggering the nmi */
	if (state!=previous_wd177x_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	previous_wd177x_int_state = state;

}


static void bbc_wd177x_status_w(int offset,int data)
{
	int drive;
	int density;

	drive_control = data;

	drive = 0;
	if ((data & 0x03)==1)
	{
		drive = 0;
	}

	if ((data & 0x03)==2)
	{
		drive = 1;
	}

	/* set drive */
	wd179x_set_drive(drive);
	/* set side */
	wd179x_set_side((data>>2) & 0x01);

    if ((data & (1<<3))!=0)
	{
		/* low-density */
		density = DEN_FM_HI;
	}
	else
	{
		/* double density */
		density = DEN_MFM_LO;
	}

	wd179x_set_density(density);
}



 READ8_HANDLER ( bbc_wd1770_read )
{
	int retval=0xff;
	switch (offset)
	{
	case 4:
		retval=wd179x_status_r(offset);
		break;
	case 5:
		retval=wd179x_track_r(offset);
		break;
	case 6:
		retval=wd179x_sector_r(offset);
		break;
	case 7:
		retval=wd179x_data_r(offset);
		break;
	default:
		break;
	}
	//logerror("wd177x read: $%02X  $%02X\n", offset,retval);

	return retval;
}

WRITE8_HANDLER ( bbc_wd1770_write )
{
	//logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		bbc_wd177x_status_w(offset, data);
		break;
	case 4:
		wd179x_command_w(offset, data);
		break;
	case 5:
		wd179x_track_w(offset, data);
		break;
	case 6:
		wd179x_sector_w(offset, data);
		break;
	case 7:
		wd179x_data_w(offset, data);
		break;
	default:
		break;
	}
}


/**************************************
   BBC B Rom loading functions
***************************************/
DEVICE_LOAD( bbcb_cart )
{
	UINT8 *mem = memory_region (REGION_USER1);
	int size, read_;
	int addr = 0;

	size = mame_fsize (file);

    addr = 0x8000 + (0x4000 * image_index_in_device(image));


	logerror("loading rom %s at %.4x size:%.4x\n",image_filename(image), addr, size);


	switch (size) {
	case 0x2000:
		read_ = mame_fread (file, mem + addr, size);
		read_ = mame_fread (file, mem + addr + 0x2000, size);
		break;
	case 0x4000:
		read_ = mame_fread (file, mem + addr, size);
		break;
	default:
		read_ = 0;
		logerror("bad rom file size of %.4x\n",size);
		break;
	}

	if (read_ != size)
		return 1;
	return 0;
}




/**************************************
   Machine Initialisation functions
***************************************/

MACHINE_INIT( bbca )
{
	cpu_setbank(1,memory_region(REGION_CPU1));         /* bank 1 repeat of the 16K ram from 4000 to 7fff */
	cpu_setbank(2,memory_region(REGION_USER1)+0x10000); /* bank 2 points at the OS rom  from c000 to ffff */
	cpu_setbank(3,memory_region(REGION_USER1));         /* bank 3 is the paged ROMs     from 8000 to bfff */

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_reset();

	bbcb_IC32_initialise();
}

MACHINE_INIT( bbcb )
{
	cpu_setbank(2,memory_region(REGION_USER1)+0x40000);/* bank 2 points at the OS rom  from c000 to ffff */
	cpu_setbank(3,memory_region(REGION_USER1));        /* bank 3 is the paged ROMs     from 8000 to bfff */

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);

	i8271_init(&bbc_i8271_interface);
	i8271_reset();
}

MACHINE_INIT( bbcb1770 )
{
	cpu_setbank(2,memory_region(REGION_USER1)+0x40000);  /* bank 2 points at the OS rom  from c000 to ffff */
	cpu_setbank(3,memory_region(REGION_USER1));          /* bank 3 is the paged ROMs     from 8000 to bfff */

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);

	previous_wd177x_int_state=1;
    wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
    wd179x_reset();
}

MACHINE_INIT( bbcbp )
{
	cpu_setbank(1,memory_region(REGION_CPU1)+0x03000); /* BANK 1 points at the screen  from 3000 to 7fff */
	cpu_setbank(2,memory_region(REGION_USER1)+0x40000); /* bank 2 points at the OS rom  from c000 to ffff */
	cpu_setbank(3,memory_region(REGION_USER1));         /* bank 3 is paged ROM or RAM   from 8000 to afff */
	cpu_setbank(4,memory_region(REGION_USER1)+0x03000); /* bank 4 is the paged ROMs     from b000 to bfff */

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);

	previous_wd177x_int_state=1;
    wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
    wd179x_reset();
}

MACHINE_INIT( bbcb6502 )
{
	cpu_setbank(2,memory_region(REGION_USER1)+0x40000);  /* bank 2 points at the OS rom  from c000 to ffff */
	cpu_setbank(3,memory_region(REGION_USER1));          /* bank 3 is the paged ROMs     from 8000 to bfff */

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);

	previous_wd177x_int_state=1;
    wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
    wd179x_reset();

	/* second processor init */

	startbank=1;
	cpu_setbank(5,memory_region(REGION_CPU2)+0x10000);
}

