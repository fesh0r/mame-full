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

#include "machine/pc_hdc.h"
#include "includes/nec765.h"

#include "includes/pcshare.h"
#include "mscommon.h"

#include "machine/8237dma.h"

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


static mame_timer *pc_keyboard_timer;

static void pc_keyb_timer(int param);

#define LOG_PORT80 0



/* ---------------------------------------------------------------------- */

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



static void pc_timer0_w(int state)
{
	if (state)
		pic8259_0_issue_irq(0);
}



/*
 * timer0	heartbeat IRQ
 * timer1	DRAM refresh (ignored)
 * timer2	PIO port C pin 4 and speaker polling
 */
static struct pit8253_config pc_pit8253_config =
{
	TYPE8253,
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w,
			NULL
		}, {
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};

static struct pit8253_config pc_pit8254_config =
{
	TYPE8254,
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w,
			NULL
		}, {
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
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



/*************************************************************************
 *
 *		PC DMA stuff
 *
 *************************************************************************/

static data8_t dma_offset[2][4];
static data8_t at_pages[0x10];
static offs_t pc_page_offset_mask;



READ8_HANDLER(pc_page_r)
{
	return 0xFF;
}



WRITE8_HANDLER(pc_page_w)
{
	switch(offset % 4) {
	case 1:
		dma_offset[0][2] = data;
		break;
	case 2:
		dma_offset[0][3] = data;
		break;
	case 3:
		dma_offset[0][0] = dma_offset[0][1] = data;
		break;
	}
}



READ8_HANDLER(at_page8_r)
{
	data8_t data = at_pages[offset % 0x10];

	switch(offset % 8) {
	case 1:
		data = dma_offset[(offset / 8) & 1][2];
		break;
	case 2:
		data = dma_offset[(offset / 8) & 1][3];
		break;
	case 3:
		data = dma_offset[(offset / 8) & 1][1];
		break;
	case 7:
		data = dma_offset[(offset / 8) & 1][0];
		break;
	}
	return data;
}



WRITE8_HANDLER(at_page8_w)
{
	at_pages[offset % 0x10] = data;

#if LOG_PORT80
	if (offset == 0)
		logerror(" at_page8_w(): Port 80h <== 0x%02x (PC=0x%08x)\n", data, activecpu_get_reg(REG_PC));
#endif /* LOG_PORT80 */

	switch(offset % 8) {
	case 1:
		dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}



READ32_HANDLER(at_page32_r)
{
	return read32_with_read8_handler(at_page8_r, offset, mem_mask);
}



WRITE32_HANDLER(at_page32_w)
{
	write32_with_write8_handler(at_page8_w, offset, data, mem_mask);
}



static data8_t pc_dma_read_byte(int channel, offs_t offset)
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& pc_page_offset_mask;
	return program_read_byte(page_offset + offset);
}



static void pc_dma_write_byte(int channel, offs_t offset, data8_t data)
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& pc_page_offset_mask;
	program_write_byte(page_offset + offset, data);
}



static struct dma8237_interface pc_dma =
{
	0,
	TIME_IN_USEC(1),

	pc_dma_read_byte,
	pc_dma_write_byte,

	{ 0, 0, pc_fdc_dack_r, pc_hdc_dack_r },
	{ 0, 0, pc_fdc_dack_w, pc_hdc_dack_w },
	pc_fdc_set_tc_state
};



/* ----------------------------------------------------------------------- */

void init_pc_common(UINT32 flags)
{
	/* MESS managed RAM */
	if (mess_ram)
		cpu_setbank(10, mess_ram);

	/* PIT */
	if (flags & PCCOMMON_TIMER_8254)
		pit8253_init(1, &pc_pit8254_config);
	else
		pit8253_init(1, &pc_pit8253_config);

	/* FDC/HDC hardware */
	pc_fdc_setup();
	pc_hdc_setup();

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

	/* PIC */
	pic8259_init(2);

	/* DMA */
	if (flags & PCCOMMON_DMA8237_AT)
	{
		dma8237_init(2);
		dma8237_config(0, &pc_dma);
		pc_page_offset_mask = 0xFF0000;
	}
	else
	{
		dma8237_init(1);
		dma8237_config(0, &pc_dma);
		pc_page_offset_mask = 0x0F0000;
	}

	pc_keyboard_timer = mame_timer_alloc(pc_keyb_timer);
}



void pc_mda_init(void)
{
	/* Get this out of the way of possibly big character ROMs */
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, MRA8_RAM );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc_video_videoram_w );
	videoram = memory_region(REGION_CPU1)+0xb0000;
	videoram_size = 0x10000;

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_w );
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

READ8_HANDLER(pc_COM1_r)  { return pc_COM_r(0, offset); }
READ8_HANDLER(pc_COM2_r)  { return pc_COM_r(1, offset); }
READ8_HANDLER(pc_COM3_r)  { return pc_COM_r(2, offset); }
READ8_HANDLER(pc_COM4_r)  { return pc_COM_r(3, offset); }
WRITE8_HANDLER(pc_COM1_w) { uart8250_w(0, offset, data); }
WRITE8_HANDLER(pc_COM2_w) { uart8250_w(1, offset, data); }
WRITE8_HANDLER(pc_COM3_w) { uart8250_w(2, offset, data); }
WRITE8_HANDLER(pc_COM4_w) { uart8250_w(3, offset, data); }

READ32_HANDLER(pc32_COM1_r)  { return read32_with_read8_handler(pc_COM1_r, offset, mem_mask); }
READ32_HANDLER(pc32_COM2_r)  { return read32_with_read8_handler(pc_COM2_r, offset, mem_mask); }
READ32_HANDLER(pc32_COM3_r)  { return read32_with_read8_handler(pc_COM3_r, offset, mem_mask); }
READ32_HANDLER(pc32_COM4_r)  { return read32_with_read8_handler(pc_COM4_r, offset, mem_mask); }
WRITE32_HANDLER(pc32_COM1_w) { write32_with_write8_handler(pc_COM1_w, offset, data, mem_mask); }
WRITE32_HANDLER(pc32_COM2_w) { write32_with_write8_handler(pc_COM2_w, offset, data, mem_mask); }
WRITE32_HANDLER(pc32_COM3_w) { write32_with_write8_handler(pc_COM3_w, offset, data, mem_mask); }
WRITE32_HANDLER(pc32_COM4_w) { write32_with_write8_handler(pc_COM4_w, offset, data, mem_mask); }



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
	mame_time keyb_delay = double_to_mame_time(1/200.0);

	on = on ? 1 : 0;

	if (pc_keyb.on != on)
	{
		if (on)
			mame_timer_adjust(pc_keyboard_timer, keyb_delay, 0, time_zero);
		else
			mame_timer_reset(pc_keyboard_timer, time_never);

		pc_keyb.on = on;
	}
}

void pc_keyb_clear(void)
{
	pc_keyb.data = 0;
}

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

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

WRITE8_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}

#if 0
#define JOY_VALUE_TO_TIME(v) (24.2e-6+11e-9*(100000.0/256)*v)
 READ8_HANDLER ( pc_JOY_r )
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
 READ8_HANDLER ( pc_JOY_r )
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



/*************************************************************************
 *
 *		Turbo handling
 *
 *************************************************************************/

struct pc_turbo_info
{
	int cpunum;
	int port;
	int mask;
	int cur_val;
	double off_speed;
	double on_speed;
};



static void pc_turbo_callback(int param)
{
	struct pc_turbo_info *ti;
	int val;
	
	ti = (struct pc_turbo_info *) param;
	val = readinputport(ti->port) & ti->mask;

	if (val != ti->cur_val)
	{
		ti->cur_val = val;
		cpunum_set_clockscale(ti->cpunum, val ? ti->on_speed : ti->off_speed);
	}
}



int pc_turbo_setup(int cpunum, int port, int mask, double off_speed, double on_speed)
{
	struct pc_turbo_info *ti;

	ti = auto_malloc(sizeof(struct pc_turbo_info));
	if (!ti)
		return 1;

	ti->cpunum = cpunum;
	ti->port = port;
	ti->mask = mask;
	ti->cur_val = -1;
	ti->off_speed = off_speed;
	ti->on_speed = on_speed;
	timer_pulse(TIME_IN_SEC(0.1), (int) ti, pc_turbo_callback);
	return 0;
}


INPUT_PORTS_START( pc_joystick_none )
	PORT_START      /* IN15 */
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START      /* IN16 */
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START      /* IN17 */
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START      /* IN18 */
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START      /* IN19 */
    PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
INPUT_PORTS_END



INPUT_PORTS_START( pc_joystick )
	PORT_START	/* IN15 */
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED ) 
	PORT_BIT( 0x0010, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 1 Button 1")
	PORT_BIT( 0x0020, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 1 Button 2")
	PORT_BIT( 0x0040, 0x0000, IPT_BUTTON1) PORT_NAME("Joystick 2 Button 1") PORT_CODE(JOYCODE_2_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x0080, 0x0000, IPT_BUTTON2) PORT_NAME("Joystick 2 Button 2") PORT_CODE(JOYCODE_2_BUTTON2) PORT_PLAYER(2)
		
	PORT_START	/* IN16 */
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_REVERSE PORT_CENTER
		
	PORT_START /* IN17 */
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_REVERSE PORT_CENTER
		
	PORT_START	/* IN18 */
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_2_LEFT) PORT_CODE_INC(JOYCODE_2_RIGHT) PORT_PLAYER(2) PORT_REVERSE PORT_CENTER
		
	PORT_START /* IN19 */
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(JOYCODE_2_UP) PORT_CODE_INC(JOYCODE_2_DOWN) PORT_PLAYER(2) PORT_REVERSE PORT_CENTER
INPUT_PORTS_END
