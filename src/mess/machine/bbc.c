/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

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

#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/6522via.h"
#include "mess/machine/wd179x.h"
#include "mess/machine/bbc.h"
#include "mess/vidhrdw/bbc.h"
#include "i8271.h"

const char *bbc_floppy_name[4] = {NULL,NULL,NULL,NULL};


static int b0_sound;
static int b1_speech_read;
static int b2_speech_write;
static int b3_keyboard;
static int b4_video0;
static int b5_video1;
static int b6_caps_lock_led;
static int b7_shift_lock_led;


static int via0_porta;


int column=0;

int bbcb_keyscan(void)
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
  			/* set the value of via0 ca2, by checking for any keys
    			 being pressed on the selected column */

			if ((readinputport(column)|0x01)!=0xff)
			{
				via_0_ca2_w(0,1);
			} else {
				via_0_ca2_w(0,0);
			};

		} else {
			via_0_ca2_w(0,0);
		};
	};
	return 0;
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
		};

		/* temp development fix to force start up into screen mode 0 */
		//if ((row==0) && ((column==7) || (column==8) || (column==9)))
		//	{ bit=1; };

		/* Normal keyboard result */
		if ((res&(1<<row))==0)
			{ bit=1; };

		if ((res|1)!=0xff)
		{
			via_0_ca2_w(0,1);
		} else {
			via_0_ca2_w(0,0);
		};

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

static void bbcb_via0_write_porta(int offset, int data)
{
	via0_porta=data;
	if (b0_sound==0)
	{
		SN76496_0_w(0,via0_porta);
	};
	if (b1_speech_read==0)
	{
		// Call Speech
	};
	if (b2_speech_write==0)
	{
		// Call Speech
	};
	if (b3_keyboard==0)
	{
		via0_porta=bbc_keyboard(via0_porta);
	};

}

static void bbcb_via0_write_portb(int offset, int data)
{
	int bit,value;
	bit=data & 0x07;
	value=(data>>3) &0x01;

	if (value) {
		switch (bit) {
		case 0:
			if (b0_sound==0) {
				b0_sound=1;
			};
			break;
		case 1:
			if (b1_speech_read==0) {
				b1_speech_read=1;
			};
			break;
		case 2:
			if (b2_speech_write==0) {
				b2_speech_write=1;
			};
			break;
		case 3:
			if (b3_keyboard==0) {
				b3_keyboard=1;
			};
			break;
		case 4:
			if (b4_video0==0) {
				b4_video0=1;
				setscreenstart(b4_video0,b5_video1);
			};
			break;
		case 5:
			if (b5_video1==0) {
				b5_video1=1;
				setscreenstart(b4_video0,b5_video1);
			};
			break;
		case 6:
			if (b6_caps_lock_led==0) {
				b6_caps_lock_led=1;
				/* call caps lock led update */
			};
			break;
		case 7:
			if (b7_shift_lock_led==0) {
				b7_shift_lock_led=1;
				/* call shift lock led update */
			};
			break;

	};
	} else {
		switch (bit) {
		case 0:
			if (b0_sound==1) {
				b0_sound=0;
				SN76496_0_w(0,via0_porta);
			};
			break;
		case 1:
			if (b1_speech_read==1) {
				b1_speech_read=0;
				via0_porta=0xff;
			};
			break;
		case 2:
			if (b2_speech_write==1) {
				b2_speech_write=0;
			};
			break;
		case 3:
			if (b3_keyboard==1) {
				b3_keyboard=0;
				/* *** call keyboard enabled *** */
				via0_porta=bbc_keyboard(via0_porta);
			};
			break;
		case 4:
			if (b4_video0==1) {
				b4_video0=0;
				setscreenstart(b4_video0,b5_video1);
			};
			break;
		case 5:
			if (b5_video1==1) {
				b5_video1=0;
				setscreenstart(b4_video0,b5_video1);
			};
			break;
		case 6:
			if (b6_caps_lock_led==1) {
				b6_caps_lock_led=0;
				/* call caps lock led update */
			};
			break;
		case 7:
			if (b7_shift_lock_led==1) {
				b7_shift_lock_led=0;
				/* call shift lock led update */
			};
			break;
		};
	};

}

static int bbcb_via0_read_porta(int offset)
{
  return via0_porta;
}

static int bbcb_via0_read_portb(int offset)
{
  return 0xff;
}

/* vertical sync pulse from video circuit */
static int bbcb_via0_read_ca1(int offset)
{
  return 0xf0;
}


/* joystick EOC (not emulated yet) */
static int bbcb_via0_read_cb1(int offset)
{
  return 1;
}


/* keyboard pressed detect */
static int bbcb_via0_read_ca2(int offset)
{
  return 0x01;
}


/* light pen strobe detect (not emulated yet) */
static int bbcb_via0_read_cb2(int offset)
{
  return 0x01;
}


/* this is wired as in input port so writing to this port would be bad */

static void bbcb_via0_write_ca2(int offset, int data)
{
  //if( errorlog ) fprintf(errorlog, "via0_write_ca2: $%02X\n", data);
}

/* this is wired as in input port so writing to this port would be bad */

static void bbcb_via0_write_cb2(int offset, int data)
{
  //if( errorlog ) fprintf(errorlog, "via0_write_cb2: $%02X\n", data);
}


static int via0_irq=0;
static int via1_irq=0;

static void bbc_via0_irq(int level)
{
  via0_irq=level;
  //if (errorlog) { fprintf(errorlog, "SYSTEM via irq %d %d %d\n",via0_irq,via1_irq,level); };
  cpu_set_irq_line(0, M6502_INT_IRQ, via0_irq|via1_irq);
}


static void bbc_via1_irq(int level)
{
  via1_irq=level;
  //if (errorlog) { fprintf(errorlog, "USER via irq %d %d %d\n",via0_irq,via1_irq,level); };
  cpu_set_irq_line(0, M6502_INT_IRQ, via0_irq|via1_irq);
}

static struct via6522_interface
bbcb_system_via= {
  bbcb_via0_read_porta,
  bbcb_via0_read_portb,
  bbcb_via0_read_ca1,
  bbcb_via0_read_cb1,
  bbcb_via0_read_ca2,
  bbcb_via0_read_cb2,
  bbcb_via0_write_porta,
  bbcb_via0_write_portb,
  bbcb_via0_write_ca2,
  bbcb_via0_write_cb2,
  bbc_via0_irq,
};


static struct via6522_interface
bbcb_user_via= {
  0,//via1_read_porta,
  0,//via1_read_portb,
  0,//via1_read_ca1,
  0,//via1_read_cb1,
  0,//via1_read_ca2,
  0,//via1_read_cb2,
  0,//via1_write_porta,
  0,//via1_write_portb,
  0,//via1_write_ca2,
  0,//via1_write_cb2,
  bbc_via1_irq,//via1_irq
};


static UINT8 first_fdc_access = 1;
static UINT8 motor_drive = 0;
static short motor_count = 0;
static UINT8 head[4]={0,};


static const char *floppy_name[4]={0,};


void	bbc_i8271_interrupt(int state)
{
	cpu_set_nmi_line(0, state);
}


static i8271_interface bbc_i8271_interface=
{
	//bbc_i8271_interrupt,
	NULL
};


void bbc_fdc_callback(int event)
{
	if (event==WD179X_IRQ_SET)
		cpu_set_irq_line(0,M6502_INT_NMI,1);
	//if (event==WD179X_IRQ_CLEAR)
		//cpu_set_irq_line(0,M6502_INT_NMI,0);
}

void init_machine_bbca(void)
{
	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);

	cpu_setbank(1,memory_region(REGION_CPU1));

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_reset();
	bbcb_IC32_initialise();

}

void init_machine_bbcb(void)
{
	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	via_reset();
	bbcb_IC32_initialise();

	if( floppy_name[0] )
		wd179x_init(1);
	else
		wd179x_init(0);
	first_fdc_access=1;

	i8271_init(&bbc_i8271_interface);
	i8271_reset();
}


void stop_machine_bbcb(void)
{
	wd179x_stop_drive();
}


/* load floppy */
int bbc_floppy_init(int id)
{
	/* load disk image */
    floppy_name[id]=device_filename(IO_FLOPPY,id);
    return 0;
}


void bbc_floppy_exit(int id)
{
	wd179x_stop_drive();
	floppy_name[id]=NULL;
	first_fdc_access=1;
}

void check_disc_status(void)
{
	if (motor_count && !--motor_count)
		wd179x_stop_drive();
}

void bbc_wd179x_status_w(int offset,int data)
{
	UINT8 drive = 255;
	void *file0;

	drive = 0;
	head[drive]=0;
	if (!floppy_name[drive])
		return;

	motor_drive=drive;
	motor_count=5*60;

	file0=wd179x_select_drive(drive,head[drive],bbc_fdc_callback,floppy_name[drive]);

	if (!file0)
		return;

	first_fdc_access=0;

	if (file0 == REAL_FDD)
		return;

	wd179x_set_geometry(drive,80,1,10,256,0,2,0);

	wd179x_select_drive(drive,head[drive],bbc_fdc_callback,floppy_name[drive]);
}

READ_HANDLER ( bbc_wd1770_read)
{
	int retval=0xff;
	logerror("wd177x read: $%02X\n", offset);
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

	return retval;
}

WRITE_HANDLER ( bbc_wd1770_write )
{
	logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		bbc_wd179x_status_w(offset, data);
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


READ_HANDLER(bbc_i8271_read)
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

WRITE_HANDLER(bbc_i8271_write)
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
