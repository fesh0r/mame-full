/***************************************************************************

    machine/pc.c

    Functions to emulate general aspects of the machine
    (RAM, ROM, interrupts, I/O ports)

    The information herein is heavily based on
    'Ralph Browns Interrupt List'
    Release 52, Last Change 20oct96

***************************************************************************/

#include "emu.h"
#include "includes/pc.h"

#include "machine/i8255.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/pc_turbo.h"

#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_t1t.h"

#include "machine/pit8253.h"

#include "machine/pckeybrd.h"
#include "machine/pc_lpt.h"
#include "machine/pc_fdc.h"
#include "machine/upd765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "machine/pcshare.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"

#include "machine/8237dma.h"
#include "machine/wd17xx.h"

#include "machine/ram.h"

#include "imagedev/cassette.h"
#include "machine/pit8253.h"
#include "machine/pcshare.h"
#include "coreutil.h"

#define VERBOSE_PIO 0	/* PIO (keyboard controller) */

#define PIO_LOG(N,M,A) \
	do { \
		if(VERBOSE_PIO>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",machine.time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

#define VERBOSE_DBG 0       /* general debug messages */

#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_DBG>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",machine.time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

/*
 * EC-1841 memory controller.  The machine can hold four memory boards;
 * each board has a control register, its address is set by a DIP switch
 * on the board itself.
 *
 * Only one board should be enabled for read, and one for write.
 * Normally, this is the same board.
 *
 * Each board is divided into 4 banks, internally numbererd 0..3.
 * POST tests each board on startup, and an error (indicated by
 * I/O CH CK bus signal) causes it to disable failing bank(s) by writing
 * 'reconfiguration code' (inverted number of failing memory bank) to
 * the register.

 * bit 1-0  'reconfiguration code'
 * bit 2    enable read access
 * bit 3    enable write access
 */

READ8_MEMBER(pc_state::ec1841_memboard_r)
{
	pc_state *st = space.machine().driver_data<pc_state>();
	return st->m_memboard[(offset % 4)];
}

WRITE8_MEMBER(pc_state::ec1841_memboard_w)
{
	pc_state *st = space.machine().driver_data<pc_state>();
	address_space *program = space.machine().device("maincpu")->memory().space(AS_PROGRAM);
	running_machine &machine = space.machine();
	UINT8 current;

	DBG_LOG(1,"ec1841_memboard_w",("(%d) <- %02X at %s\n", offset, data, machine.describe_context()));

	// for now, handle only board 0
	if (offset > 0) {
		st->m_memboard[offset] = data;
		return;
	}

	current = st->m_memboard[offset];

	if (BIT(current, 2) && !BIT(data, 2)) {
		// disable read access
		program->unmap_read(0, 0x7ffff);
		DBG_LOG(1,"ec1841_memboard_w",("unmap_read(%d)\n", offset));
	}

	if (BIT(current, 3) && !BIT(data, 3)) {
		// disable write access
		program->unmap_write(0, 0x7ffff);
		DBG_LOG(1,"ec1841_memboard_w",("unmap_write(%d)\n", offset));
	}

	if (!BIT(current, 2) && BIT(data, 2)) {
		// enable read access
		program->install_read_bank(0, 0x7ffff, "bank10");
		DBG_LOG(1,"ec1841_memboard_w",("map_read(%d)\n", offset));
	}

	if (!BIT(current, 3) && BIT(data, 3)) {
		// enable write access
		program->install_write_bank(0, 0x7ffff, "bank10");
		DBG_LOG(1,"ec1841_memboard_w",("map_write(%d)\n", offset));
	}

	st->m_memboard[offset] = data;
}

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

READ8_MEMBER(pc_state::pc_page_r)
{
	return 0xFF;
}


WRITE8_MEMBER(pc_state::pc_page_w)
{
	switch(offset % 4)
	{
	case 1:
		m_dma_offset[0][2] = data;
		break;
	case 2:
		m_dma_offset[0][3] = data;
		break;
	case 3:
		m_dma_offset[0][0] = m_dma_offset[0][1] = data;
		break;
	}
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	device_set_input_line(st->m_maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}


READ8_MEMBER(pc_state::pc_dma_read_byte)
{
	UINT8 result;
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0x0F0000;

	result = space.read_byte( page_offset + offset);
	return result;
}


WRITE8_MEMBER(pc_state::pc_dma_write_byte)
{
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0x0F0000;

	space.write_byte( page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( pc_dma8237_fdc_dack_r )
{
	return pc_fdc_dack_r(device->machine());
}


static READ8_DEVICE_HANDLER( pc_dma8237_hdc_dack_r )
{
	return 0xff;
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_fdc_dack_w )
{
	pc_fdc_dack_w( device->machine(), data );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_hdc_dack_w )
{
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_0_dack_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	st->m_u73_q2 = 0;
	i8237_dreq0_w( st->m_dma8237, st->m_u73_q2 );
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma8237_out_eop )
{
	pc_fdc_set_tc_state( device->machine(), state == ASSERT_LINE ? 0 : 1 );
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	pc_state *st = device->machine().driver_data<pc_state>();

	if (!state) st->m_dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( ibm5150_dma8237_config )
{
	DEVCB_DEVICE_LINE("dma8237", pc_dma_hrq_changed),
	DEVCB_DEVICE_LINE("dma8237", pc_dma8237_out_eop),
	DEVCB_DRIVER_MEMBER(pc_state, pc_dma_read_byte),
	DEVCB_DRIVER_MEMBER(pc_state, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_HANDLER("dma8237", pc_dma8237_fdc_dack_r), DEVCB_DEVICE_HANDLER("dma8237", pc_dma8237_hdc_dack_r) },
	{ DEVCB_DEVICE_HANDLER("dma8237", pc_dma8237_0_dack_w), DEVCB_NULL, DEVCB_DEVICE_HANDLER("dma8237", pc_dma8237_fdc_dack_w), DEVCB_DEVICE_HANDLER("dma8237", pc_dma8237_hdc_dack_w) },
	{ DEVCB_DEVICE_LINE("dma8237", pc_dack0_w), DEVCB_DEVICE_LINE("dma8237", pc_dack1_w), DEVCB_DEVICE_LINE("dma8237", pc_dack2_w), DEVCB_DEVICE_LINE("dma8237", pc_dack3_w) }
};


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

const struct pic8259_interface ibm5150_pic8259_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


/*************************************************************
 *
 * PCJR pic8259 configuration
 *
 * Part of the PCJR CRT POST test at address F0452/F0454 writes
 * to the PIC enabling an IRQ which is then immediately fired,
 * however it is expected that the actual IRQ is taken one
 * instruction later (the irq bit is reset by the instruction
 * at F0454). Delaying taking of an IRQ by one instruction for
 * all cases breaks floppy emulation. This seems to be a really
 * tight corner case. For now we delay the IRQ by one instruction
 * only for the PCJR and only when it's inside the POST checks.
 *
 *************************************************************/

static emu_timer	*pc_int_delay_timer;

static TIMER_CALLBACK( pcjr_delayed_pic8259_irq )
{
    device_set_input_line(machine.firstcpu, 0, param ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( pcjr_pic8259_set_int_line )
{
	if ( cpu_get_reg( device->machine().firstcpu, STATE_GENPC ) == 0xF0454 )
	{
		pc_int_delay_timer->adjust( device->machine().firstcpu->cycles_to_attotime(1), state );
	}
	else
	{
		device_set_input_line(device->machine().firstcpu, 0, state ? ASSERT_LINE : CLEAR_LINE);
	}
}

const struct pic8259_interface pcjr_pic8259_config =
{
	DEVCB_LINE(pcjr_pic8259_set_int_line),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


/*************************************************************************
 *
 *      PC Speaker related
 *
 *************************************************************************/
UINT8 pc_speaker_get_spk(running_machine &machine)
{
	pc_state *st = machine.driver_data<pc_state>();
	return st->m_pc_spkrdata & st->m_pc_input;
}


void pc_speaker_set_spkrdata(running_machine &machine, UINT8 data)
{
	device_t *speaker = machine.device(SPEAKER_TAG);
	pc_state *st = machine.driver_data<pc_state>();
	st->m_pc_spkrdata = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


void pc_speaker_set_input(running_machine &machine, UINT8 data)
{
	device_t *speaker = machine.device(SPEAKER_TAG);
	pc_state *st = machine.driver_data<pc_state>();
	st->m_pc_input = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


/*************************************************************
 *
 * pit8253 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( ibm5150_pit8253_out1_changed )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	/* Trigger DMA channel #0 */
	if ( st->m_out1 == 0 && state == 1 && st->m_u73_q2 == 0 )
	{
		st->m_u73_q2 = 1;
		i8237_dreq0_w( st->m_dma8237, st->m_u73_q2 );
	}
	st->m_out1 = state;
}


static WRITE_LINE_DEVICE_HANDLER( ibm5150_pit8253_out2_changed )
{
	pc_speaker_set_input( device->machine(), state );
}


const struct pit8253_config ibm5150_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out1_changed)
		}, {
			XTAL_14_31818MHz/12,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out2_changed)
		}
	}
};


/*
  On the PC Jr the input for clock 1 seems to be selectable
  based on bit 4(/5?) written to output port A0h. This is not
  supported yet.
 */

const struct pit8253_config pcjr_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,              /* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			XTAL_14_31818MHz/12,              /* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out2_changed)
		}
	}
};

/* MC1502 uses single XTAL for everything -- incl. CGA? check */

const struct pit8253_config mc1502_pit8253_config =
{
	{
		{
			XTAL_16MHz/12,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_16MHz/12,              /* serial port */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			XTAL_16MHz/12,              /* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(ibm5150_pit8253_out2_changed)
		}
	}
};

/**********************************************************
 *
 * COM hardware
 *
 **********************************************************/

/* called when a interrupt is set/cleared from com hardware */
static WRITE_LINE_DEVICE_HANDLER( pc_com_interrupt_1 )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	pic8259_ir4_w(st->m_pic8259, state);
}

static WRITE_LINE_DEVICE_HANDLER( pc_com_interrupt_2 )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	pic8259_ir3_w(st->m_pic8259, state);
}

const ins8250_interface ibm5150_com_interface[4]=
{
	{
		DEVCB_DEVICE_LINE_MEMBER("serport0", serial_port_device, tx),
		DEVCB_DEVICE_LINE_MEMBER("serport0", rs232_port_device, dtr_w),
		DEVCB_DEVICE_LINE_MEMBER("serport0", rs232_port_device, rts_w),
		DEVCB_LINE(pc_com_interrupt_1),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("serport1", serial_port_device, tx),
		DEVCB_DEVICE_LINE_MEMBER("serport1", rs232_port_device, dtr_w),
		DEVCB_DEVICE_LINE_MEMBER("serport1", rs232_port_device, rts_w),
		DEVCB_LINE(pc_com_interrupt_2),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("serport2", serial_port_device, tx),
		DEVCB_DEVICE_LINE_MEMBER("serport2", rs232_port_device, dtr_w),
		DEVCB_DEVICE_LINE_MEMBER("serport2", rs232_port_device, rts_w),
		DEVCB_LINE(pc_com_interrupt_1),
		DEVCB_NULL,
		DEVCB_NULL
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("serport3", serial_port_device, tx),
		DEVCB_DEVICE_LINE_MEMBER("serport3", rs232_port_device, dtr_w),
		DEVCB_DEVICE_LINE_MEMBER("serport3", rs232_port_device, rts_w),
		DEVCB_LINE(pc_com_interrupt_2),
		DEVCB_NULL,
		DEVCB_NULL
	}
};

const rs232_port_interface ibm5150_serport_config[4] =
{
	{
		DEVCB_DEVICE_LINE_MEMBER("ins8250_0", ins8250_uart_device, rx_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_0", ins8250_uart_device, dcd_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_0", ins8250_uart_device, dsr_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_0", ins8250_uart_device, ri_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_0", ins8250_uart_device, cts_w)
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("ins8250_1", ins8250_uart_device, rx_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_1", ins8250_uart_device, dcd_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_1", ins8250_uart_device, dsr_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_1", ins8250_uart_device, ri_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_1", ins8250_uart_device, cts_w)
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("ins8250_2", ins8250_uart_device, rx_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_2", ins8250_uart_device, dcd_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_2", ins8250_uart_device, dsr_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_2", ins8250_uart_device, ri_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_2", ins8250_uart_device, cts_w)
	},
	{
		DEVCB_DEVICE_LINE_MEMBER("ins8250_3", ins8250_uart_device, rx_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_3", ins8250_uart_device, dcd_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_3", ins8250_uart_device, dsr_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_3", ins8250_uart_device, ri_w),
		DEVCB_DEVICE_LINE_MEMBER("ins8250_3", ins8250_uart_device, cts_w)
	}
};

/**********************************************************
 *
 * NMI handling
 *
 **********************************************************/

static UINT8	nmi_enabled;

WRITE8_MEMBER(pc_state::pc_nmi_enable_w)
{
	logerror( "%08X: changing NMI state to %s\n", cpu_get_pc(&space.device()), data & 0x80 ? "enabled" : "disabled" );

	nmi_enabled = data & 0x80;
}

/*************************************************************
 *
 * PCJR NMI and raw keybaord handling
 *
 * raw signals on the keyboard cable:
 * ---_-b0b1b2b3b4b5b6b7pa----------------------
 *    | | | | | | | | | | |
 *    | | | | | | | | | | *--- 11 stop bits ( -- = 1 stop bit )
 *    | | | | | | | | | *----- parity bit ( 0 = _-, 1 = -_ )
 *    | | | | | | | | *------- bit 7 ( 0 = _-, 1 = -_ )
 *    | | | | | | | *--------- bit 6 ( 0 = _-, 1 = -_ )
 *    | | | | | | *----------- bit 5 ( 0 = _-, 1 = -_ )
 *    | | | | | *------------- bit 4 ( 0 = _-, 1 = -_ )
 *    | | | | *--------------- bit 3 ( 0 = _-, 1 = -_ )
 *    | | | *----------------- bit 2 ( 0 = _-, 1 = -_ )
 *    | | *------------------- bit 1 ( 0 = _-, 1 = -_ )
 *    | *--------------------- bit 0 ( 0 = _-, 1 = -_ )
 *    *----------------------- start bit (always _- )
 *
 * An entire bit lasts for 440 uSec, half bit time is 220 uSec.
 * Transferring an entire byte takes 21 x 440uSec. The extra
 * time of the stop bits is to allow the CPU to do other things
 * besides decoding keyboard signals.
 *
 * These signals get inverted before going to the PCJR
 * handling hardware. The sequence for the start then
 * becomes:
 *
 * __-_b0b1.....
 *   |
 *   *---- on the 0->1 transition of the start bit a keyboard
 *         latch signal is set to 1 and an NMI is generated
 *         when enabled.
 *         The keyboard latch is reset by reading from the
 *         NMI enable port (A0h).
 *
 *************************************************************/

static struct {
	UINT8		transferring;
	UINT8		latch;
	UINT32		raw_keyb_data;
	int			signal_count;
	emu_timer	*keyb_signal_timer;
} pcjr_keyb;


READ8_MEMBER(pc_state::pcjr_nmi_enable_r)
{
	pcjr_keyb.latch = 0;

	return nmi_enabled;
}


static TIMER_CALLBACK( pcjr_keyb_signal_callback )
{
	pcjr_keyb.raw_keyb_data = pcjr_keyb.raw_keyb_data >> 1;
	pcjr_keyb.signal_count--;

	if ( pcjr_keyb.signal_count <= 0 )
	{
		pcjr_keyb.keyb_signal_timer->adjust( attotime::never, 0, attotime::never );
		pcjr_keyb.transferring = 0;
	}
}


static void pcjr_set_keyb_int(running_machine &machine, int state)
{
	pc_state *st = machine.driver_data<pc_state>();
	if ( state )
	{
		UINT8	data = pc_keyb_read();
		UINT8	parity = 0;
		int		i;

		/* Calculate the raw data */
		for( i = 0; i < 8; i++ )
		{
			if ( ( 1 << i ) & data )
			{
				parity ^= 1;
			}
		}
		pcjr_keyb.raw_keyb_data = 0;
		pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | ( parity ? 1 : 2 );
		for( i = 0; i < 8; i++ )
		{
			pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | ( ( data & 0x80 ) ? 1 : 2 );
			data <<= 1;
		}
		/* Insert start bit */
		pcjr_keyb.raw_keyb_data = ( pcjr_keyb.raw_keyb_data << 2 ) | 1;
		pcjr_keyb.signal_count = 20 + 22;

		/* we are now transferring a byte of keyboard data */
		pcjr_keyb.transferring = 1;

		/* Set timer */
		pcjr_keyb.keyb_signal_timer->adjust( attotime::from_usec(220), 0, attotime::from_usec(220) );

		/* Trigger NMI */
		if ( ! pcjr_keyb.latch )
		{
			pcjr_keyb.latch = 1;
			if ( nmi_enabled & 0x80 )
			{
				device_set_input_line( st->m_pit8253->machine().firstcpu, INPUT_LINE_NMI, PULSE_LINE );
			}
		}
	}
}


static void pcjr_keyb_init(running_machine &machine)
{
	pcjr_keyb.transferring = 0;
	pcjr_keyb.latch = 0;
	pcjr_keyb.raw_keyb_data = 0;
	pc_keyb_set_clock( 1 );
}



/**********************************************************
 *
 * PPI8255 interface
 *
 *
 * PORT A (input)
 *
 * Directly attached to shift register which stores data
 * received from the keyboard.
 *
 * PORT B (output)
 * 0 - PB0 - TIM2GATESPK - Enable/disable counting on timer 2 of the 8253
 * 1 - PB1 - SPKRDATA    - Speaker data
 * 2 - PB2 -             - Enable receiving data from the keyboard when keyboard is not locked.
 * 3 - PB3 -             - Dipsswitch set selector
 * 4 - PB4 - ENBRAMPCK   - Enable ram parity check
 * 5 - PB5 - ENABLEI/OCK - Enable expansion I/O check
 * 6 - PB6 -             - Connected to keyboard clock signal
 *                         0 = ignore keyboard signals
 *                         1 = accept keyboard signals
 * 7 - PB7 -             - Clear/disable shift register and IRQ1 line
 *                         0 = normal operation
 *                         1 = clear and disable shift register and clear IRQ1 flip flop
 *
 * PORT C
 * 0 - PC0 -         - Dipswitch 0/4 SW1
 * 1 - PC1 -         - Dipswitch 1/5 SW1
 * 2 - PC2 -         - Dipswitch 2/6 SW1
 * 3 - PC3 -         - Dipswitch 3/7 SW1
 * 4 - PC4 - SPK     - Speaker/cassette data
 * 5 - PC5 - I/OCHCK - Expansion I/O check result
 * 6 - PC6 - T/C2OUT - Output of 8253 timer 2
 * 7 - PC7 - PCK     - Parity check result
 *
 * IBM5150 SW1:
 * 0   - OFF - One or more floppy drives
 *       ON  - Diskless operation
 * 1   - OFF - 8087 present
 *       ON  - No 8087 present
 * 2+3 - Used to determine on board memory configuration
 *       OFF OFF - 64KB
 *       ON  OFF - 48KB
 *       OFF ON  - 32KB
 *       ON  ON  - 16KB
 * 4+5 - Used to select display
 *       OFF OFF - Monochrome
 *       ON  OFF - CGA, 80 column
 *       OFF ON  - CGA, 40 column
 *       ON  ON  - EGA/VGA display
 * 6+7 - Used to select number of disk drives
 *       OFF OFF - four disk drives
 *       ON  OFF - three disk drives
 *       OFF ON  - two disk drives
 *       ON  ON  - one disk drive
 *
 **********************************************************/

WRITE_LINE_MEMBER( pc_state::keyboard_clock_w )
{
	if ( m_ppi_clock_signal != state )
	{
		if ( m_ppi_keyb_clock && m_ppi_shift_enable )
		{
			m_ppi_clock_signal = state;
			if ( ! m_ppi_keyboard_clear )
			{
				/* Data is clocked in on a high->low transition */
				if ( ! state )
				{
					UINT8   trigger_irq = m_ppi_shift_register & 0x01;

					m_ppi_shift_register = ( m_ppi_shift_register >> 1 ) | ( m_ppi_data_signal << 7 );
					if ( trigger_irq )
					{
						pic8259_ir1_w(m_pic8259, 1);
						m_ppi_shift_enable = 0;
						m_ppi_clock_signal = 0;
						m_pc_kbdc->clock_write_from_mb(m_ppi_clock_signal);
					}
				}
			}
		}
	}
}


WRITE_LINE_MEMBER( pc_state::keyboard_data_w )
{
	m_ppi_data_signal = state;
}


static READ8_DEVICE_HANDLER (ibm5160_ppi_porta_r)
{
	int data = 0xFF;
	running_machine &machine = device->machine();
	pc_state *st = device->machine().driver_data<pc_state>();

	/* KB port A */
	if (st->m_ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = device->machine().root_device().ioport("DSW0")->read();
	}
	else
	{
		data = st->m_ppi_shift_register;
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}


static READ8_DEVICE_HANDLER ( ibm5160_ppi_portc_r )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	int timer2_output = pit8253_get_output( st->m_pit8253, 2 );
	int data=0xff;
	running_machine &machine = device->machine();

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
//  if (pc_port[0x61] & 0x08)
	if (st->m_ppi_portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data & 0xf0) | ((device->machine().root_device().ioport("DSW0")->read() >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data & 0xf0) | (device->machine().root_device().ioport("DSW0")->read() & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	if ( st->m_ppi_portb & 0x01 )
	{
		data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );

	return data;
}


static WRITE8_DEVICE_HANDLER( ibm5160_ppi_portb_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();

	/* PPI controller port B*/
	st->m_ppi_portb = data;
	st->m_ppi_portc_switch_high = data & 0x08;
	st->m_ppi_keyboard_clear = data & 0x80;
	st->m_ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->m_pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine(), data & 0x02 );

	st->m_ppi_clock_signal = ( st->m_ppi_keyb_clock ) ? 1 : 0;
	st->m_pc_kbdc->clock_write_from_mb(st->m_ppi_clock_signal);

	/* If PB7 is set clear the shift register and reset the IRQ line */
	if ( st->m_ppi_keyboard_clear )
	{
		pic8259_ir1_w(st->m_pic8259, 0);
		st->m_ppi_shift_register = 0;
		st->m_ppi_shift_enable = 1;
	}
}


I8255_INTERFACE( ibm5160_ppi8255_interface )
{
	DEVCB_HANDLER(ibm5160_ppi_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(ibm5160_ppi_portb_w),
	DEVCB_HANDLER(ibm5160_ppi_portc_r),
	DEVCB_NULL
};


static READ8_DEVICE_HANDLER (pc_ppi_porta_r)
{
	int data = 0xFF;
	running_machine &machine = device->machine();
	pc_state *st = device->machine().driver_data<pc_state>();

	/* KB port A */
	if (st->m_ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = device->machine().root_device().ioport("DSW0")->read();
	}
	else
	{
		data = pc_keyb_read();
	}
	PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
	return data;
}


static WRITE8_DEVICE_HANDLER( pc_ppi_portb_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	/* PPI controller port B*/
	st->m_ppi_portb = data;
	st->m_ppi_portc_switch_high = data & 0x08;
	st->m_ppi_keyboard_clear = data & 0x80;
	st->m_ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->m_pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine(), data & 0x02 );
	pc_keyb_set_clock( st->m_ppi_keyb_clock );

	if ( st->m_ppi_keyboard_clear )
		pc_keyb_clear();
}


I8255_INTERFACE( pc_ppi8255_interface )
{
	DEVCB_HANDLER(pc_ppi_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pc_ppi_portb_w),
	DEVCB_HANDLER(ibm5160_ppi_portc_r),
	DEVCB_NULL
};


static struct {
	UINT8		pulsing;
	UINT8		latch;		/* keyboard scan code */
	UINT16		mask;		/* input lines */
	emu_timer	*keyb_signal_timer;
} mc1502_keyb;


/* check if any keys are pressed, raise IRQ1 if so */

static TIMER_CALLBACK( mc1502_keyb_signal_callback )
{
	pc_state *st = machine.driver_data<pc_state>();
	UINT8 key = 0;

	key |= machine.root_device().ioport("Y1")->read();
	key |= machine.root_device().ioport("Y2")->read();
	key |= machine.root_device().ioport("Y3")->read();
	key |= machine.root_device().ioport("Y4")->read();
	key |= machine.root_device().ioport("Y5")->read();
	key |= machine.root_device().ioport("Y6")->read();
	key |= machine.root_device().ioport("Y7")->read();
	key |= machine.root_device().ioport("Y8")->read();
	key |= machine.root_device().ioport("Y9")->read();
	key |= machine.root_device().ioport("Y10")->read();
	key |= machine.root_device().ioport("Y11")->read();
	key |= machine.root_device().ioport("Y12")->read();
	DBG_LOG(1,"mc1502_k_s_c",("= %02X (%d) %s\n", key, mc1502_keyb.pulsing,
		(key || mc1502_keyb.pulsing) ? " will IRQ" : ""));

	/*
       If a key is pressed and we're not pulsing yet, start pulsing the IRQ1;
       keep pulsing while any key is pressed, and pulse one time after all keys
       are released.
     */
	if (key) {
		if (mc1502_keyb.pulsing < 2) {
			mc1502_keyb.pulsing += 2;
		}
	}

	if (mc1502_keyb.pulsing) {
		pic8259_ir1_w(st->m_pic8259, (mc1502_keyb.pulsing & 1));
		mc1502_keyb.pulsing--;
	}
}

static READ8_DEVICE_HANDLER ( mc1502_ppi_porta_r )
{
	running_machine &machine = device->machine();

	DBG_LOG(1,"mc1502_ppi_porta_r",("= %02X\n", mc1502_keyb.latch));
	return mc1502_keyb.latch;
}

static WRITE8_DEVICE_HANDLER ( mc1502_ppi_porta_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	running_machine &machine = device->machine();

	DBG_LOG(1,"mc1502_ppi_porta_w",("( %02X )\n", data));
	mc1502_keyb.latch = data;
	if (mc1502_keyb.pulsing)
		mc1502_keyb.pulsing--;
	pic8259_ir1_w(st->m_pic8259, 0);
}

static WRITE8_DEVICE_HANDLER ( mc1502_ppi_portb_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	running_machine &machine = device->machine();

	DBG_LOG(2,"mc1502_ppi_portb_w",("( %02X )\n", data));
	st->m_ppi_portb = data;
	pit8253_gate2_w(device->machine().device("pit8253"), BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine(), data & 0x02 );
}

static READ8_DEVICE_HANDLER ( mc1502_ppi_portc_r )
{
	running_machine &machine = device->machine();
	pc_state *st = device->machine().driver_data<pc_state>();
	int timer2_output = pit8253_get_output( device->machine().device("pit8253"), 2 );
	int data = 0xff;
	double tap_val = (device->machine().device<cassette_image_device>(CASSETTE_TAG)->input());

//  0x80 -- serial RxD
//  0x40 -- CASS IN, also loops back T2OUT (gated by CASWR)
	data = ( data & ~0x40 ) | ( tap_val < 0 ? 0x40 : 0x00 ) | ( (BIT(st->m_ppi_portb, 7) && timer2_output) ? 0x40 : 0x00 );
//  0x20 -- T2OUT
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );
//  0x10 -- SNDOUT
	data = ( data & ~0x10 ) | ( (BIT(st->m_ppi_portb, 1) && timer2_output) ? 0x10 : 0x00 );

	DBG_LOG(2,"mc1502_ppi_portc_r",("= %02X (tap_val %f t2out %d) at %s\n",
		data, tap_val, timer2_output, machine.describe_context()));
	return data;
}

static READ8_DEVICE_HANDLER ( mc1502_kppi_porta_r )
{
	running_machine &machine = device->machine();
	UINT8 key = 0;

	if (mc1502_keyb.mask & 0x0001) { key |= machine.root_device().ioport("Y1")->read(); }
	if (mc1502_keyb.mask & 0x0002) { key |= machine.root_device().ioport("Y2")->read(); }
	if (mc1502_keyb.mask & 0x0004) { key |= machine.root_device().ioport("Y3")->read(); }
	if (mc1502_keyb.mask & 0x0008) { key |= machine.root_device().ioport("Y4")->read(); }
	if (mc1502_keyb.mask & 0x0010) { key |= machine.root_device().ioport("Y5")->read(); }
	if (mc1502_keyb.mask & 0x0020) { key |= machine.root_device().ioport("Y6")->read(); }
	if (mc1502_keyb.mask & 0x0040) { key |= machine.root_device().ioport("Y7")->read(); }
	if (mc1502_keyb.mask & 0x0080) { key |= machine.root_device().ioport("Y8")->read(); }
	if (mc1502_keyb.mask & 0x0100) { key |= machine.root_device().ioport("Y9")->read(); }
	if (mc1502_keyb.mask & 0x0200) { key |= machine.root_device().ioport("Y10")->read(); }
	if (mc1502_keyb.mask & 0x0400) { key |= machine.root_device().ioport("Y11")->read(); }
	if (mc1502_keyb.mask & 0x0800) { key |= machine.root_device().ioport("Y12")->read(); }
	key ^= 0xff;
	DBG_LOG(2,"mc1502_kppi_porta_r",("= %02X\n", key));
	return key;
}

static WRITE8_DEVICE_HANDLER ( mc1502_kppi_portb_w )
{
	running_machine &machine = device->machine();

	mc1502_keyb.mask &= ~255;
	mc1502_keyb.mask |= data ^ 255;
	if (!BIT(data, 0))
		mc1502_keyb.mask |= 1 << 11;
	else
		mc1502_keyb.mask &= ~(1 << 11);
	DBG_LOG(2,"mc1502_kppi_portb_w",("( %02X -> %04X )\n", data, mc1502_keyb.mask));
}

static WRITE8_DEVICE_HANDLER ( mc1502_kppi_portc_w )
{
	running_machine &machine = device->machine();

	mc1502_keyb.mask &= ~(7 << 8);
	mc1502_keyb.mask |= ((data ^ 7) & 7) << 8;
	DBG_LOG(2,"mc1502_kppi_portc_w",("( %02X -> %04X )\n", data, mc1502_keyb.mask));
}


static WRITE8_DEVICE_HANDLER ( pcjr_ppi_portb_w )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	/* KB controller port B */
	st->m_ppi_portb = data;
	st->m_ppi_portc_switch_high = data & 0x08;
	pit8253_gate2_w(device->machine().device("pit8253"), BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine(), data & 0x02 );

	device->machine().device<cassette_image_device>(CASSETTE_TAG)->change_state(( data & 0x08 ) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
}


/*
 * On a PCJR none of the port A bits are connected.
 */
static READ8_DEVICE_HANDLER (pcjr_ppi_porta_r )
{
	int data;
	running_machine &machine = device->machine();

	data = 0xff;
	PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
	return data;
}


/*
 * Port C connections on a PCJR (notes from schematics):
 * PC0 - KYBD LATCH
 * PC1 - MODEM CD INSTALLED
 * PC2 - DISKETTE CD INSTALLED
 * PC3 - ATR CD IN
 * PC4 - cassette audio
 * PC5 - OUT2 from 8253
 * PC6 - KYBD IN
 * PC7 - (keyboard) CABLE CONNECTED
 */
static READ8_DEVICE_HANDLER ( pcjr_ppi_portc_r )
{
	pc_state *st = device->machine().driver_data<pc_state>();
	int timer2_output = pit8253_get_output( device->machine().device("pit8253"), 2 );
	int data=0xff;

	data&=~0x80;
	data &= ~0x04;		/* floppy drive installed */
	if ( device->machine().device<ram_device>(RAM_TAG)->size() > 64 * 1024 )	/* more than 64KB ram installed */
		data &= ~0x08;
	data = ( data & ~0x01 ) | ( pcjr_keyb.latch ? 0x01: 0x00 );
	if ( ! ( st->m_ppi_portb & 0x08 ) )
	{
		double tap_val = (device->machine().device<cassette_image_device>(CASSETTE_TAG)->input());

		if ( tap_val < 0 )
		{
			data &= ~0x10;
		}
		else
		{
			data |= 0x10;
		}
	}
	else
	{
		if ( st->m_ppi_portb & 0x01 )
		{
			data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
		}
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );
	data = ( data & ~0x40 ) | ( ( pcjr_keyb.raw_keyb_data & 0x01 ) ? 0x40 : 0x00 );

	return data;
}


I8255_INTERFACE( pcjr_ppi8255_interface )
{
	DEVCB_HANDLER(pcjr_ppi_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pcjr_ppi_portb_w),
	DEVCB_HANDLER(pcjr_ppi_portc_r),
	DEVCB_NULL
};

I8255_INTERFACE( mc1502_ppi8255_interface )
{
	DEVCB_HANDLER(mc1502_ppi_porta_r),
	DEVCB_HANDLER(mc1502_ppi_porta_w),
	DEVCB_NULL,
	DEVCB_HANDLER(mc1502_ppi_portb_w),
	DEVCB_HANDLER(mc1502_ppi_portc_r),
	DEVCB_NULL
};

I8255_INTERFACE( mc1502_ppi8255_interface_2 )
{
	DEVCB_HANDLER(mc1502_kppi_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(mc1502_kppi_portb_w),
	DEVCB_NULL,
	DEVCB_HANDLER(mc1502_kppi_portc_w)
};


/**********************************************************
 *
 * NEC uPD765 floppy interface
 *
 **********************************************************/

static void pc_fdc_interrupt(running_machine &machine, int state)
{
	pc_state *st = machine.driver_data<pc_state>();
	if (st->m_pic8259)
	{
		pic8259_ir6_w(st->m_pic8259, state);
	}
}

static void pc_fdc_dma_drq(running_machine &machine, int state)
{
	pc_state *st = machine.driver_data<pc_state>();
	i8237_dreq2_w( st->m_dma8237, state);
}

static device_t * pc_get_device(running_machine &machine )
{
	return machine.device("upd765");
}

static const struct pc_fdc_interface fdc_interface_nc =
{
	pc_fdc_interrupt,
	pc_fdc_dma_drq,
	NULL,
	pc_get_device
};


static const struct pc_fdc_interface pcjr_fdc_interface_nc =
{
	pc_fdc_interrupt,
	NULL,
	NULL,
	pc_get_device
};


static void pc_set_irq_line(running_machine &machine,int irq, int state)
{
	pc_state *st = machine.driver_data<pc_state>();

	switch (irq)
	{
	case 0: pic8259_ir0_w(st->m_pic8259, state); break;
	case 1: pic8259_ir1_w(st->m_pic8259, state); break;
	case 2: pic8259_ir2_w(st->m_pic8259, state); break;
	case 3: pic8259_ir3_w(st->m_pic8259, state); break;
	case 4: pic8259_ir4_w(st->m_pic8259, state); break;
	case 5: pic8259_ir5_w(st->m_pic8259, state); break;
	case 6: pic8259_ir6_w(st->m_pic8259, state); break;
	case 7: pic8259_ir7_w(st->m_pic8259, state); break;
	}
}

static void pc_set_keyb_int(running_machine &machine, int state)
{
	pc_set_irq_line( machine, 1, state );
}

/*
 * MC1502 uses a FD1793 clone instead of uPD765
 */

READ8_DEVICE_HANDLER( mc1502_wd17xx_aux_r )
{
	UINT8 data;

	data = 0;

	return data;
}

WRITE8_DEVICE_HANDLER( mc1502_wd17xx_aux_w )
{
	// master reset
	wd17xx_mr_w(device, BIT(data, 0));

	// SIDE ONE
	wd17xx_set_side(device, BIT(data, 1));

	// bits 2, 3 -- motor on (drive 0, 1)

	// DRIVE SEL
	wd17xx_set_drive(device, BIT(data, 4));
}

/*
 * Accesses to this port block (halt the CPU until DRQ, INTRQ or MOTOR ON)
 */
READ8_DEVICE_HANDLER( mc1502_wd17xx_drq_r )
{
	UINT8 data;
	UINT64 newpc;

	data = wd17xx_drq_r(device);
	if (!data && !wd17xx_intrq_r(device)) {
		/* fake cpu halt by resetting PC one insn back */
		newpc = cpu_get_reg( device->machine().firstcpu, STATE_GENPC );
		cpu_set_reg( device->machine().firstcpu, STATE_GENPC, newpc - 1 );
	}

	return data;
}

READ8_DEVICE_HANDLER( mc1502_wd17xx_motor_r )
{
	UINT8 data;

	/* fake motor being always on */
	data = 1;

	return data;
}


/**********************************************************
 *
 * Initialization code
 *
 **********************************************************/

void mess_init_pc_common(running_machine &machine, UINT32 flags, void (*set_keyb_int_func)(running_machine &, int), void (*set_hdc_int_func)(running_machine &,int,int))
{
	pc_state *state = machine.driver_data<pc_state>();
	if ( set_keyb_int_func != NULL )
		init_pc_common(machine, flags, set_keyb_int_func);

	/* MESS managed RAM */
	if ( machine.device<ram_device>(RAM_TAG)->pointer() )
		state->membank( "bank10" )->set_base( machine.device<ram_device>(RAM_TAG)->pointer() );
}


DRIVER_INIT_MEMBER(pc_state,ibm5150)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_rtc_init(machine());
}


DRIVER_INIT_MEMBER(pc_state,pccga)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_rtc_init(machine());
}


DRIVER_INIT_MEMBER(pc_state,bondwell)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, NULL, pc_set_irq_line);
	pc_turbo_setup(machine(), machine().firstcpu, "DSW2", 0x02, 4.77/12, 1);
}

DRIVER_INIT_MEMBER(pc_state,pcmda)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT_MEMBER(pc_state,europc)
{
	UINT8 *gfx = &machine().root_device().memregion("gfx1")->base()[0x8000];
	UINT8 *rom = &machine().root_device().memregion("maincpu")->base()[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
      fix century rom bios bug !
      if year <79 month (and not CENTURY) is loaded with 0x20
    */
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	europc_rtc_init(machine());
//  europc_rtc_set_time(machine());
}

DRIVER_INIT_MEMBER(pc_state,t1000hx)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_turbo_setup(machine(), machine().firstcpu, "DSW2", 0x02, 4.77/12, 1);
}

DRIVER_INIT_MEMBER(pc_state,pc200)
{
	UINT8 *gfx = &machine().root_device().memregion("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT_MEMBER(pc_state,ppc512)
{
	UINT8 *gfx = &machine().root_device().memregion("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}
DRIVER_INIT_MEMBER(pc_state,pc1512)
{
	UINT8 *gfx = &machine().root_device().memregion("gfx1")->base()[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}


DRIVER_INIT_MEMBER(pc_state,pcjr)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pcjr_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT_MEMBER(pc_state,mc1502)
{
	mess_init_pc_common(machine(), 0, NULL, pc_set_irq_line);
}

static READ8_HANDLER( input_port_0_r ) { return space->machine().root_device().ioport("IN0")->read(); }

DRIVER_INIT_MEMBER(pc_state,pc1640)
{
	address_space *io_space = machine().firstcpu->memory().space( AS_IO );

	io_space->install_legacy_read_handler(0x278, 0x27b, FUNC(pc1640_port278_r), 0xffff);
	io_space->install_legacy_read_handler(0x4278, 0x427b, FUNC(pc1640_port4278_r), 0xffff);

	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT_MEMBER(pc_state,pc_vga)
{
	mess_init_pc_common(machine(), PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	pc_vga_init(machine(), ::input_port_0_r, NULL);
	pc_vga_io_init(machine(), machine().device("maincpu")->memory().space(AS_PROGRAM), 0xa0000, machine().device("maincpu")->memory().space(AS_IO), 0x0000);
}

static IRQ_CALLBACK(pc_irq_callback)
{
	pc_state *st = device->machine().driver_data<pc_state>();
	return pic8259_acknowledge( st->m_pic8259 );
}


MACHINE_START( pc )
{
	pc_state *st = machine.driver_data<pc_state>();

	st->m_pic8259 = machine.device("pic8259");
	st->m_dma8237 = machine.device("dma8237");
	st->m_pit8253 = machine.device("pit8253");
	pc_fdc_init( machine, &fdc_interface_nc );
}


MACHINE_RESET( pc )
{
	device_t *speaker = machine.device(SPEAKER_TAG);
	pc_state *st = machine.driver_data<pc_state>();
	st->m_maincpu = machine.device("maincpu" );
	device_set_irq_callback(st->m_maincpu, pc_irq_callback);

	st->m_u73_q2 = 0;
	st->m_out1 = 0;
	st->m_pc_spkrdata = 0;
	st->m_pc_input = 1;
	st->m_dma_channel = 0;
	memset(st->m_dma_offset,0,sizeof(st->m_dma_offset));
	st->m_ppi_portc_switch_high = 0;
	st->m_ppi_speaker = 0;
	st->m_ppi_keyboard_clear = 0;
	st->m_ppi_keyb_clock = 0;
	st->m_ppi_portb = 0;
	st->m_ppi_clock_signal = 0;
	st->m_ppi_data_signal = 0;
	st->m_ppi_shift_register = 0;
	st->m_ppi_shift_enable = 0;

	speaker_level_w( speaker, 0 );
}


MACHINE_START( mc1502 )
{
	pc_state *st = machine.driver_data<pc_state>();
	st->m_maincpu = machine.device("maincpu" );
	device_set_irq_callback(st->m_maincpu, pc_irq_callback);

	st->m_pic8259 = machine.device("pic8259");
	st->m_dma8237 = NULL;
	st->m_pit8253 = machine.device("pit8253");

	/*
           Keyboard polling circuit holds IRQ1 high until a key is
           pressed, then it starts a timer that pulses IRQ1 low each
           40ms (check) for 20ms (check) until all keys are released.
           Last pulse causes BIOS to write a 'break' scancode into port 60h.
     */
	pic8259_ir1_w(st->m_pic8259, 1);
	memset(&mc1502_keyb, 0, sizeof(mc1502_keyb));
	mc1502_keyb.keyb_signal_timer = machine.scheduler().timer_alloc(FUNC(mc1502_keyb_signal_callback));
	mc1502_keyb.keyb_signal_timer->adjust( attotime::from_msec(20), 0, attotime::from_msec(20) );
}


MACHINE_START( pcjr )
{
	pc_state *st = machine.driver_data<pc_state>();
	pc_fdc_init( machine, &pcjr_fdc_interface_nc );
	pcjr_keyb.keyb_signal_timer = machine.scheduler().timer_alloc(FUNC(pcjr_keyb_signal_callback));
	pc_int_delay_timer = machine.scheduler().timer_alloc(FUNC(pcjr_delayed_pic8259_irq));
	st->m_maincpu = machine.device("maincpu" );
	device_set_irq_callback(st->m_maincpu, pc_irq_callback);

	st->m_pic8259 = machine.device("pic8259");
	st->m_dma8237 = NULL;
	st->m_pit8253 = machine.device("pit8253");
}


MACHINE_RESET( pcjr )
{
	device_t *speaker = machine.device(SPEAKER_TAG);
	pc_state *st = machine.driver_data<pc_state>();
	st->m_u73_q2 = 0;
	st->m_out1 = 0;
	st->m_pc_spkrdata = 0;
	st->m_pc_input = 1;
	st->m_dma_channel = 0;
	memset(st->m_memboard,0xc,sizeof(st->m_memboard));	// check
	memset(st->m_dma_offset,0,sizeof(st->m_dma_offset));
	st->m_ppi_portc_switch_high = 0;
	st->m_ppi_speaker = 0;
	st->m_ppi_keyboard_clear = 0;
	st->m_ppi_keyb_clock = 0;
	st->m_ppi_portb = 0;
	st->m_ppi_clock_signal = 0;
	st->m_ppi_data_signal = 0;
	st->m_ppi_shift_register = 0;
	st->m_ppi_shift_enable = 0;
	speaker_level_w( speaker, 0 );

	pcjr_keyb_init(machine);
}


DEVICE_IMAGE_LOAD( pcjr_cartridge )
{
	UINT32	address;
	UINT32	size;

	address = ( ! strcmp( ":cart2", image.device().tag() ) ) ? 0xd0000 : 0xe0000;

	if ( image.software_entry() )
	{
		UINT8 *cart = image.get_software_region( "rom" );

		size = image.get_software_region_length("rom" );

		memcpy( image.device().machine().root_device().memregion("maincpu")->base() + address, cart, size );
	}
	else
	{
		UINT8	header[0x200];

		unsigned image_size = image.length();

		/* Check for supported image sizes */
		switch( image_size )
		{
		case 0x2200:
		case 0x4200:
		case 0x8200:
		case 0x10200:
			break;
		default:
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Invalid rom file size" );
			return IMAGE_INIT_FAIL;
		}

		/* Read and verify the header */
		if ( 512 != image.fread( header, 512 ) )
		{
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Unable to read header" );
			return IMAGE_INIT_FAIL;
		}

		/* Read the cartridge contents */
		if ( ( image_size - 0x200 ) != image.fread(image.device().machine().root_device().memregion("maincpu")->base() + address, image_size - 0x200 ) )
		{
			image.seterror(IMAGE_ERROR_UNSUPPORTED, "Unable to read cartridge contents" );
			return IMAGE_INIT_FAIL;
		}
	}

	return IMAGE_INIT_PASS;
}


/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/

TIMER_DEVICE_CALLBACK( pc_frame_interrupt )
{
	int scanline = param;

	if((scanline % 64) == 0)
		pc_keyboard();
}

TIMER_DEVICE_CALLBACK( pc_vga_frame_interrupt )
{
	int scanline = param;

	if((scanline % 64) == 0)
	{
		//vga_timer();
		pc_keyboard();
	}
}

TIMER_DEVICE_CALLBACK( pcjr_frame_interrupt )
{
	int scanline = param;

	if((scanline % 64) == 0 &&  pcjr_keyb.transferring == 0 )
		pc_keyboard();
}

/*
ibm xt bios
-----------

fe0ac
fe10c
fe12b: hangs after reset
fe15e
fe19c
fe2c6
 graphics adapter sync signals
fe332
 search roms
fe35d
 pic test
fe38f
fe3c6
fe3e6
 301 written
 expect 0xaa after reset, send when lines activated in short time
 (keyboard polling in frame interrupt is not quick enough now)
fe424
fe448
 i/o expansion test
fe49c
 memory test
fe500
 harddisk bios used
fe55c
 disk booting

 f0bef
  f0b85
   f096d
    disk related
    feca0
     prueft kanal 0 address register (memory refresh!?)

ibm pc bios
-----------
fe104
fe165
fe1b4
fe205
fe23f
fe256
fe363
fe382 expansion test
fe3c4
 memory test
fe43b
fe443 call fe643 keyboard test
fe4a1 call ff979 tape!!! test
*/


// damned old checkit doesn't test at standard addresses
// will do more when I have a program supporting it
static struct {
	int data[0x18];
	emu_timer *timer;
} pc_rtc;

static TIMER_CALLBACK(pc_rtc_timer)
{
	int year;
	if (++pc_rtc.data[2]>=60) {
		pc_rtc.data[2]=0;
		if (++pc_rtc.data[3]>=60) {
			pc_rtc.data[3]=0;
			if (++pc_rtc.data[4]>=24) {
				pc_rtc.data[4]=0;
				pc_rtc.data[5]=(pc_rtc.data[5]%7)+1;
				year=pc_rtc.data[9]+2000;
				if (++pc_rtc.data[6]>=gregorian_days_in_month(pc_rtc.data[7], year)) {
					pc_rtc.data[6]=1;
					if (++pc_rtc.data[7]>12) {
						pc_rtc.data[7]=1;
						pc_rtc.data[9]=(pc_rtc.data[9]+1)%100;
					}
				}
			}
		}
	}
}

void pc_rtc_init(running_machine &machine)
{
	memset(&pc_rtc,0,sizeof(pc_rtc));
	pc_rtc.timer = machine.scheduler().timer_alloc(FUNC(pc_rtc_timer));
	pc_rtc.timer->adjust(attotime::zero, 0, attotime(1,0));
}

READ8_MEMBER(pc_state::pc_rtc_r)
{
	int data;
	switch (offset) {
	default:
		data=pc_rtc.data[offset];
	}
	logerror( "rtc read %.2x %.2x\n", offset, data);
	return data;
}

WRITE8_MEMBER(pc_state::pc_rtc_w)
{
	logerror( "rtc write %.2x %.2x\n", offset, data);
	switch(offset) {
	default:
		pc_rtc.data[offset]=data;
	}
}

/*************************************************************************
 *
 *      EXP
 *      expansion port
 *
 *************************************************************************/

// I even don't know what it is!
static struct {
	/*
      reg 0 ram behaviour if in
      reg 3 write 1 to enable it
      reg 4 ram behaviour ???
      reg 5,6 (5 hi, 6 lowbyte) ???
    */
	/* selftest in ibmpc, ibmxt */
	UINT8 reg[8];
} pc_expansion={ { 0,0,0,0,0,0,1 } };

WRITE8_MEMBER(pc_state::pc_EXP_w)
{
	//DBG_LOG(1,"EXP_unit_w",("%.2x $%02x\n", offset, data));
	switch (offset) {
	case 4:
		pc_expansion.reg[4]=pc_expansion.reg[5]=pc_expansion.reg[6]=data;
		break;
	default:
		pc_expansion.reg[offset] = data;
	}
}

READ8_MEMBER(pc_state::pc_EXP_r)
{
    int data;
	UINT16 a;
	switch (offset) {
	case 6:
		data = pc_expansion.reg[offset];
		a=(pc_expansion.reg[5]<<8)|pc_expansion.reg[6];
		a<<=1;
		pc_expansion.reg[5]=a>>8;
		pc_expansion.reg[6]=a&0xff;
		break;
	default:
		data = pc_expansion.reg[offset];
	}
    //DBG_LOG(1,"EXP_unit_r",("%.2x $%02x\n", offset, data));
	return data;
}



