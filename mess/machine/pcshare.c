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
#include <assert.h>
#include "driver.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"

#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/dma8237.h"
#include "includes/uart8250.h"
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_aga.h"

#include "includes/pc_mouse.h"
#include "includes/pckeybrd.h"
#include "includes/pc_fdc_h.h"

#include "includes/pclpt.h"
#include "includes/centroni.h"

#include "devices/pc_hdc.h"
#include "includes/nec765.h"

#include "includes/pcshare.h"
#include "mscommon.h"

#define VERBOSE_DBG 0       /* general debug messages */
#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

#define VERBOSE_JOY 0		/* JOY (joystick port) */

#if VERBOSE_JOY
#define LOG(LEVEL,N,M,A)  \
#define JOY_LOG(N,M,A) \
	if(VERBOSE_JOY>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define JOY_LOG(n,m,a)
#endif

#define FDC_DMA 2

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

static PC_LPT_CONFIG lpt_config[3]={
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	}
};

static CENTRONICS_CONFIG cent_config[3]={
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	}
};

void init_pc_common(UINT32 flags)
{
	/* MESS managed RAM */
	if (mess_ram)
		cpu_setbank(10, mess_ram);

	/* PIT */
	pit8253_config(0, &pc_pit8253_config);

	/* FDC hardware */
	pc_fdc_setup();

	/* com hardware */
	uart8250_init(0, com_interface);
	uart8250_reset(0);
	uart8250_init(1, com_interface+1);
	uart8250_reset(1);
	uart8250_init(2, com_interface+2);
	uart8250_reset(2);
	uart8250_init(3, com_interface+3);
	uart8250_reset(3);

	pc_lpt_config(0, lpt_config);
	centronics_config(0, cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(1, lpt_config+1);
	centronics_config(1, cent_config+1);
	pc_lpt_set_device(1, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(2, lpt_config+2);
	centronics_config(2, cent_config+2);
	pc_lpt_set_device(2, &CENTRONICS_PRINTER_DEVICE);

	/* serial mouse */
	pc_mouse_set_protocol(TYPE_MICROSOFT_MOUSE);
	pc_mouse_set_input_base(12);
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* PC-XT keyboard */
	if (flags & PCCOMMON_KEYBOARD_AT)
		at_keyboard_init(AT_KEYBOARD_TYPE_AT);
	else
		at_keyboard_init(AT_KEYBOARD_TYPE_PC);
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);


	/* DMA */
	if (flags & PCCOMMON_DMA8237_AT)
	{
		static DMA8237_CONFIG at_dma = { DMA8237_AT };
		dma8237_config(dma8237 + 0, &at_dma);
		dma8237_config(dma8237 + 1, &at_dma);
	}
	else
	{
		static DMA8237_CONFIG pc_dma = { DMA8237_PC };
		dma8237_config(dma8237 + 0, &pc_dma);
	}
	dma8237_reset(dma8237);
}

void pc_mda_init(void)
{
	/* Get this out of the way of possibly big character ROMs */
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb0000, 0xbffff, MRA8_RAM );
	install_mem_write_handler(0, 0xb0000, 0xbffff, pc_video_videoram_w );
	videoram = memory_region(REGION_CPU1)+0xb0000;
	videoram_size = 0x10000;

	install_port_read_handler(0, 0x3b0, 0x3bf, pc_MDA_r );
	install_port_write_handler(0, 0x3b0, 0x3bf, pc_MDA_w );
}

void pc_cga_init(void)
{
	/* Get this out of the way of possibly big character ROMs */
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/* Changed video RAM size to full 32k, for cards which support the
	 * Plantronics chipset */
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA8_RAM );
	install_mem_write_handler(0, 0xb8000, 0xbffff, pc_video_videoram_w );
	videoram = memory_region(REGION_CPU1)+0xb8000;
	videoram_size = 0x4000;

	install_port_read_handler(0, 0x3d0, 0x3df, pc_CGA_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc_CGA_w );
}

void pc_vga_init(void)
{
	install_mem_read_handler(0, 0xa0000, 0xaffff, MRA_BANK1 );
	install_mem_read_handler(0, 0xb0000, 0xb7fff, MRA_BANK2 );
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA_BANK3 );
	install_mem_read_handler(0, 0xc0000, 0xc7fff, MRA8_ROM );

	install_mem_write_handler(0, 0xa0000, 0xaffff, MWA_BANK1 );
	install_mem_write_handler(0, 0xb0000, 0xb7fff, MWA_BANK2 );
	install_mem_write_handler(0, 0xb8000, 0xbffff, MWA_BANK3 );
	install_mem_write_handler(0, 0xc0000, 0xc7fff, MWA8_ROM );

	install_port_read_handler(0, 0x3b0, 0x3bf, vga_port_03b0_r );
	install_port_read_handler(0, 0x3c0, 0x3cf, vga_port_03c0_r );
	install_port_read_handler(0, 0x3d0, 0x3df, vga_port_03d0_r );

	install_port_write_handler(0, 0x3b0, 0x3bf, vga_port_03b0_w );
	install_port_write_handler(0, 0x3c0, 0x3cf, vga_port_03c0_w );
	install_port_write_handler(0, 0x3d0, 0x3df, vga_port_03d0_w );
}

/***********************************/
/* PC interface to PC COM hardware */
/* Done this way because PCW16 also has PC-com hardware but it
is connected in a different way */

static int pc_COM_r(int n, int offset)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_r",("COM%d $%02x: disabled\n", n+1, 0x0ff));
		return 0x0ff;
    }

	return uart8250_r(n, offset);
}

static void pc_COM_w(int n, int offset, int data)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_w",("COM%d $%02x: disabled\n", n+1, data));
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

/*
   keyboard seams to permanently sent data clocked by the mainboard
   clock line low for longer means "resync", keyboard sends 0xaa as answer
   will become automatically 0x00 after a while
*/
static struct {
	UINT8 data;
	int on;
} pc_keyb= { 0 };

UINT8 pc_keyb_read(void)
{
	return pc_keyb.data;
}

static void pc_keyb_timer(int param)
{
	at_keyboard_reset();
	pc_keyboard();
}

void pc_keyb_set_clock(int on)
{
	if (!pc_keyb.on && on)
		timer_set(1/200.0, 0, pc_keyb_timer);

	pc_keyb.on = on;
}

void pc_keyb_clear(void)
{
	pc_keyb.data = 0;
}

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

//	if( !pic8259_0_irq_pending(1) && ((pc_port[0x61]&0xc0)==0xc0) ) // amstrad problems
	if( !pic8259_0_irq_pending(1) && pc_keyb.on)
	{
		if ( (data=at_keyboard_read())!=-1) {
			pc_keyb.data = data;
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_keyb.data));
			pic8259_0_issue_irq(1);
		}
	}
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
		delta = (int)( 256 * 1000 * (new_time - JOY_time) );
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
