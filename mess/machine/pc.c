/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

	TODO:
	clean up (maybe split) the different pieces of hardware
	PIC, PIT, DMA... add support for LPT, COM (almost done)
	emulation of a serial mouse on a COM port (almost done)
	support for Game Controller port at 0x0201
	support for XT harddisk (under the way, see machine/pc_hdc.c)
	whatever 'hardware' comes to mind,
	maybe SoundBlaster? EGA? VGA?

***************************************************************************/
#include "driver.h"
#include "machine/8255ppi.h"

#include "mess/includes/pic8259.h"
#include "mess/includes/pit8253.h"
#include "mess/includes/mc146818.h"
#include "mess/includes/dma8237.h"
#include "mess/includes/uart8250.h"
#include "mess/includes/vga.h"
#include "mess/includes/pc_cga.h"
#include "mess/includes/pc_mda.h"

#include "mess/includes/pc_flopp.h"
#include "mess/includes/pc_mouse.h"
#include "mess/includes/pckeybrd.h"

#include "mess/includes/pc.h"

UINT8 pc_port[0x400];

#define FDC_DMA 2


void pc_fdc_setup(void);

/* called when a interrupt is set/cleared from com hardware */
static void pc_com_interrupt(int nr, int state)
{
	static const int irq[4]={4,3,4,3};
	/* issue COM1/3 IRQ4, COM2/4 IRQ3 */
	if (state)
	{
		pic8259_0_issue_irq(irq[nr]);
	}
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected(int n, int data)
{
	/* mouse connected to this port? */
	if (readinputport(3) & (0x80>>n))
		pc_mouse_handshake_in(n,data);
}

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
static uart8250_interface com_interface[4]=
{
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	}
};

/* static READ_HANDLER( pc_ppi_porta_r ); */
/* static READ_HANDLER( pc_ppi_portb_r ); */
static READ_HANDLER( pc_ppi_portc_r );
static WRITE_HANDLER( pc_ppi_porta_w );
/* static WRITE_HANDLER( pc_ppi_portb_w ); */
static WRITE_HANDLER( pc_ppi_portc_w );


/* PC-XT has a 8255 which is connected to keyboard and other
status information */
static ppi8255_interface pc_ppi8255_interface =
{
	1,
	pc_ppi_porta_r,
	pc_ppi_portb_r,
	pc_ppi_portc_r,
	pc_ppi_porta_w,
	pc_ppi_portb_w,
	pc_ppi_portc_w
};

static void pc_sh_speaker_change_clock(double clock)
{
	switch( pc_port[0x61] & 3 )
	{
		case 0: pc_sh_speaker(0); break;
		case 1: pc_sh_speaker(1); break;
		case 2: pc_sh_speaker(1); break;
		case 3: pc_sh_speaker(2); break;
    }
}

/*
 * timer0	heartbeat IRQ
 * timer1	DRAM refresh (ignored)
 * timer2	PIO port C pin 4 and speaker polling
 */
static PIT8253_CONFIG pc_pit8253_config={
	TYPE8253,
	{
		{
			4770000/4,				/* heartbeat IRQ */
			pic8259_0_issue_irq,
			NULL
		}, {
			4770000/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4770000/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};

void init_pc_common(void)
{
	pit8253_config(0,&pc_pit8253_config);
	/* FDC hardware */
	pc_fdc_setup();

	/* com hardware */
	uart8250_init(0,com_interface);
	uart8250_reset(0);
	uart8250_init(1,com_interface+1);
	uart8250_reset(1);
	uart8250_init(2,com_interface+2);
	uart8250_reset(2);
	uart8250_init(3,com_interface+3);
	uart8250_reset(3);

	/* serial mouse */
	pc_mouse_set_protocol(TYPE_MICROSOFT_MOUSE);
	pc_mouse_set_input_base(12);
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* PC-XT keyboard */
	ppi8255_init(&pc_ppi8255_interface);
	at_keyboard_init();
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);

	/* should be in init for DMA controller? */
	pc_DMA_status &= ~(0x10 << FDC_DMA);	/* reset DMA running flag */
	pc_DMA_status |= 0x01 << FDC_DMA;		/* set DMA terminal count flag */
}

void init_pc(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	init_pc_common();
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
}

void init_pc1512(void)
{
	init_pc();

	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
	mc146818_init(MC146818_IGNORE_CENTURY);
}

extern void init_pc1640(void)
{
	init_pc_common();
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);

	mc146818_init(MC146818_IGNORE_CENTURY);

	vga_init(input_port_0_r);
}

void init_pc_vga(void)
{
#if 0
        int i;
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

#if 0
        for (chksum=0, i=0;i<0x8000;i++) {
                chksum+=memory[i];
        }
        printf("checksum %.2x\n",chksum);
#endif

	init_pc_common();
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);

	vga_init(input_port_0_r);
}

extern void pc1640_close_machine(void)
{
	mc146818_close();
}

void pc_mda_init_machine(void)
{
	int i;

	pc_keyboard_init();

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < Machine->gfx[0]->height; y++ )
				Machine->gfx[0]->gfxdata[(i * Machine->gfx[0]->height + y) * Machine->gfx[0]->width + 8] = 0;
		}
	}
}

void pc_cga_init_machine(void)
{
	pc_keyboard_init();
}

void pc_vga_init_machine(void)
{
	pc_keyboard_init();
}

/*************************************
 *
 *		Port handlers.
 *
 *************************************/
int pc_harddisk_init(int id)
{
	pc_hdc_file[id] = image_fopen(IO_HARDDISK, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	return INIT_OK;
}

void pc_harddisk_exit(int id)
{
	if( pc_hdc_file[id] )
		osd_fclose(pc_hdc_file[id]);
    pc_hdc_file[id] = NULL;
}

/***********************************/
/* PC interface to PC COM hardware */
/* Done this way because PCW16 also has PC-com hardware but it
is connected in a different way */

int pc_COM_r(int n, int offset)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		COM_LOG(1,"COM_r",("COM%d $%02x: disabled\n", n+1, 0x0ff));
		return 0x0ff;
    }

	return uart8250_r(n, offset);
}

void pc_COM_w(int n, int offset, int data)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		COM_LOG(1,"COM_w",("COM%d $%02x: disabled\n", n+1, data));
		return;
    }

	uart8250_w(n,offset, data);
}

READ_HANDLER(pc_COM1_r)
{
	return pc_COM_r(0, offset);
}

READ_HANDLER(pc_COM2_r)
{
	return pc_COM_r(1, offset);
}

READ_HANDLER(pc_COM3_r)
{
	return pc_COM_r(2, offset);
}

READ_HANDLER(pc_COM4_r)
{
	return pc_COM_r(3, offset);
}


WRITE_HANDLER(pc_COM1_w)
{
	uart8250_w(0, offset,data);
}

WRITE_HANDLER(pc_COM2_w)
{
	uart8250_w(1, offset,data);
}

WRITE_HANDLER(pc_COM3_w)
{
	uart8250_w(2, offset,data);
}

WRITE_HANDLER(pc_COM4_w)
{
	uart8250_w(3, offset,data);
}


/*************************************************************************
 *
 *		PIO
 *		parallel input output
 *
 *************************************************************************/

READ_HANDLER( pc_ppi_porta_r )
{
	int data;

	/* KB port A */
    data = pc_port[0x60];
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}

READ_HANDLER( pc_ppi_portb_r )
{
	int data;

	/* KB port B */
	data = pc_port[0x61] /*& 0x03f*/;
	PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
	return data;
}

/* tandy1000hx
   bit 4 input eeprom data in
   bit 3 output turbo mode */
READ_HANDLER( pc_ppi_portc_r )
{
	int data=0xff;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
	if (pc_port[0x61] & 0x08)
	{
		/* read hi nibble of S2 */
		data = (data&0xf0)|((input_port_1_r(0) >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data&0xf0)|(input_port_1_r(0) & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	return data;
}

WRITE_HANDLER( pc_ppi_porta_w )
{
	/* KB controller port A */
	PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
	pc_port[0x60] = data;
}

WRITE_HANDLER( pc_ppi_portb_w )
{
	/* KB controller port B */
	PIO_LOG(1,"PIO_B_w",("$%02x\n", data));
	pc_port[0x61] = data;
	switch( data & 3 )
	{
		case 0: pc_sh_speaker(0); break;
		case 1: pc_sh_speaker(1); break;
		case 2: pc_sh_speaker(1); break;
		case 3: pc_sh_speaker(2); break;
	}
}

WRITE_HANDLER( pc_ppi_portc_w )
{
	/* KB controller port C */
	PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
	pc_port[0x62] = data;
}

/*************************************************************************
 *
 *		JOY
 *		joystick port
 *
 *************************************************************************/

static double JOY_time = 0.0;

WRITE_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}

#if 0
#define JOY_VALUE_TO_TIME(v) (24.2e-6+11e-9*(100000.0/256)*v)
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
#if 0
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
#endif
	{
		delta=new_time-JOY_time;
		if ( delta>JOY_VALUE_TO_TIME(input_port_16_r(0)) ) data &= ~0x01;
		if ( delta>JOY_VALUE_TO_TIME(input_port_17_r(0)) ) data &= ~0x02;
		if ( delta>JOY_VALUE_TO_TIME(input_port_18_r(0)) ) data &= ~0x04;
		if ( delta>JOY_VALUE_TO_TIME(input_port_19_r(0)) ) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input_port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#else
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
	{
		delta = 256 * 1000 * (new_time - JOY_time);
		if (input_port_16_r(0) < delta) data &= ~0x01;
		if (input_port_17_r(0) < delta) data &= ~0x02;
		if (input_port_18_r(0) < delta) data &= ~0x04;
		if (input_port_19_r(0) < delta) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input
						   _port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#endif



/*************************************************************************
 *
 *		HDC
 *		hard disk controller
 *
 *************************************************************************/
void pc_HDC_w(int chip, int offset, int data)
{
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file[chip<<1] )
		return;
	switch( offset )
	{
		case 0: pc_hdc_data_w(chip,data);	 break;
		case 1: pc_hdc_reset_w(chip,data);	 break;
		case 2: pc_hdc_select_w(chip,data);  break;
		case 3: pc_hdc_control_w(chip,data); break;
	}
}
WRITE_HANDLER ( pc_HDC1_w ) { pc_HDC_w(0, offset, data); }
WRITE_HANDLER ( pc_HDC2_w ) { pc_HDC_w(1, offset, data); }

int pc_HDC_r(int chip, int offset)
{
	int data = 0xff;
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file[chip<<1] )
		return data;
	switch( offset )
	{
		case 0: data = pc_hdc_data_r(chip); 	 break;
		case 1: data = pc_hdc_status_r(chip);	 break;
		case 2: data = pc_hdc_dipswitch_r(chip); break;
		case 3: break;
	}
	return data;
}
READ_HANDLER ( pc_HDC1_r ) { return pc_HDC_r(0, offset); }
READ_HANDLER ( pc_HDC2_r ) { return pc_HDC_r(1, offset); }

/*************************************************************************
 *
 *		EXP
 *		expansion port
 *
 *************************************************************************/
WRITE_HANDLER ( pc_EXP_w )
{
	DBG_LOG(1,"EXP_unit_w",("$%02x\n", data));
	pc_port[0x213] = data;
}

READ_HANDLER ( pc_EXP_r )
{
    int data = pc_port[0x213];
    DBG_LOG(1,"EXP_unit_r",("$%02x\n", data));
	return data;
}

/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

	if( !pic8259_0_irq_pending(1) )
	{
		if ( (data=at_keyboard_read())!=-1) {
			pc_port[0x60] = data;
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_port[0x60]));
			pic8259_0_issue_irq(1);
		}
	}
}

int pc_mda_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_mda_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}

int pc_cga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_cga_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}

int pc_vga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}
//	vga_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}
