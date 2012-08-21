/******************************************************************************
*
*  Pacific Educational Systems 'PES' Speech box
*  Part number VPU-1 V1
*  By Kevin 'kevtris' Horton and Jonathan Gevaryahu AKA Lord Nightmare
*
*  RE work done by Kevin Horton and Jonathan Gevaryahu
*
*  DONE:
*  compiles correctly
*  rom loads correctly
*  interface with tms5220 is done
*  rts and cts bits are stored in struct
*  serial is attached to terminal
*
*  TODO:
*  serial receive clear should happen after delay of one cpu cycle, not ASSERT and then CLEAR immediately after
*  figure out how to attach serial to external socket
*
***********************************************************************
This is almost as simple as hardware gets from the digital side:
Hardware consists of:
10.245Mhz xtal
80c31 cpu/mcu
27c64 rom (holds less than 256 bytes of code)
unpopulated 6164 sram, which isn't used
TSP5220C speech chip (aka tms5220c)
mc145406 RS232 driver/receiver (+-24v to 0-5v converter for serial i/o)
74hc573b1 octal tri-state D-latch (part of bus interface for ROM)
74hc74b1 dual d-flipflop with set/reset, positive edge trigger (?)
74hc02b1 quad 2-input NOR gate (wired up to decode address 0, and data 0 and 1 to produce /RS and /WS)
mc14520b dual 4-bit binary up counter (used as a chopper for the analog filter)
Big messy analog section to allow voice output to be filtered extensively by a 4th order filter

Address map:
80C31 ADDR:
  0000-1FFF: ROM
  2000-3FFF: open bus (would be ram)
  4000-ffff: open bus
80C31 IO:
  00 W: d0 and d1 are the /RS and /WS bits
  port 1.x: tms5220 bus
  port 2.x: unused
  port 3.0: RxD serial receive
  port 3.1: TxD serial send
  port 3.2: read, from /INT on tms5220c
  port 3.3: read, from /READY on tms5220c
  port 3.4: read, from the serial RTS line
  port 3.5: read/write, to the serial CTS line, inverted (effectively /CTS)
  port 3.6: write, /WR (general) and /WE (pin 27) for unpopulated 6164 SRAM
  port 3.7: write, /RD (general) and /OE (pin 22) for unpopulated 6164 SRAM

*/
#define CPU_CLOCK		XTAL_10_245MHz

#undef DEBUG_FIFO
#undef DEBUG_SERIAL_CB
#undef DEBUG_PORTS

/* Core includes */
#include "emu.h"
#include "includes/pes.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/tms5220.h"
#include "machine/terminal.h"

/* Devices */
static WRITE8_DEVICE_HANDLER( pes_kbd_input )
{
	pes_state *state = device->machine().driver_data<pes_state>();
#ifdef DEBUG_FIFO
	fprintf(stderr,"keyboard input: %c, ", data);
#endif
	// if fifo is full (head ptr = tail ptr-1), do not increment the head ptr and do not store the data
	if (((state->m_infifo_tail_ptr-1)&0x1F) == state->m_infifo_head_ptr)
	{
		logerror("infifo was full, write ignored!\n");
#ifdef DEBUG_FIFO
		fprintf(stderr,"infifo was full, write ignored!\n");
#endif
		return;
	}
	state->m_infifo[state->m_infifo_head_ptr] = data;
	state->m_infifo_head_ptr++;
	state->m_infifo_head_ptr&=0x1F;
#ifdef DEBUG_FIFO
	fprintf(stderr,"kb input fifo fullness: %d\n",(state->m_infifo_head_ptr-state->m_infifo_tail_ptr)&0x1F);
#endif
	// todo: following two should be set so clear happens after one cpu cycle
	cputag_set_input_line(device->machine(), "maincpu", MCS51_RX_LINE, ASSERT_LINE);
	cputag_set_input_line(device->machine(), "maincpu", MCS51_RX_LINE, CLEAR_LINE);
}

static GENERIC_TERMINAL_INTERFACE( pes_terminal_intf )
{
	DEVCB_HANDLER(pes_kbd_input)
};

/* Helper Functions */
static int data_to_i8031(device_t *device)
{
	pes_state *state = device->machine().driver_data<pes_state>();
	UINT8 data;
	data = state->m_infifo[state->m_infifo_tail_ptr];
	// if fifo is empty (tail ptr == head ptr), do not increment the tail ptr, otherwise do.
	if (state->m_infifo_tail_ptr != state->m_infifo_head_ptr) state->m_infifo_tail_ptr++;
	state->m_infifo_tail_ptr&=0x1F;
#ifdef DEBUG_SERIAL_CB
	fprintf(stderr,"callback: input to i8031/pes from pc/terminal: %02X\n",data);
#endif
	return data;
}

static void data_from_i8031(device_t *device, int data)
{
	pes_state *state = device->machine().driver_data<pes_state>();
	state->m_terminal->write(*device->machine().memory().first_space(),0,data);
#ifdef DEBUG_SERIAL_CB
	fprintf(stderr,"callback: output from i8031/pes to pc/terminal: %02X\n",data);
#endif
}

/* Port Handlers */
WRITE8_MEMBER( pes_state::rsws_w )
{
	pes_state *state = machine().driver_data<pes_state>();
	m_wsstate = data&0x1; // /ws is bit 0
	m_rsstate = (data&0x2)>>1; // /rs is bit 1
#ifdef DEBUG_PORTS
	logerror("port0 write: RSWS states updated: /RS: %d, /WS: %d\n", m_rsstate, m_wsstate);
#endif
	tms5220_rsq_w(state->m_speech, m_rsstate);
	tms5220_wsq_w(state->m_speech, m_wsstate);
}

WRITE8_MEMBER( pes_state::port1_w )
{
	pes_state *state = machine().driver_data<pes_state>();
#ifdef DEBUG_PORTS
	logerror("port1 write: tms5220 data written: %02X\n", data);
#endif
	tms5220_data_w(state->m_speech, 0, data);

}

READ8_MEMBER( pes_state::port1_r )
{
	UINT8 data = 0xFF;
	pes_state *state = machine().driver_data<pes_state>();
	data = tms5220_status_r(state->m_speech, 0);
#ifdef DEBUG_PORTS
	logerror("port1 read: tms5220 data read: 0x%02X\n", data);
#endif
	return data;
}

/*#define P3_RXD (((state->m_port3_data)&(1<<0))>>0)
#define P3_TXD (((state->m_port3_data)&(1<<1))>>1)
#define P3_INT (((state->m_port3_data)&(1<<2))>>2)
#define P3_RDY (((state->m_port3_data)&(1<<3))>>3)
#define P3_RTS (((state->m_port3_data)&(1<<4))>>4)
#define P3_CTS (((state->m_port3_data)&(1<<5))>>5)
#define P3_WR (((state->m_port3_data)&(1<<6))>>6)
#define P3_RD (((state->m_port3_data)&(1<<7))>>7)
*/
WRITE8_MEMBER( pes_state::port3_w )
{
	m_port3_state = data;
#ifdef DEBUG_PORTS
	logerror("port3 write: control data written: %02X; ", data);
	logerror("RXD: %d; ", BIT(data,0));
	logerror("TXD: %d; ", BIT(data,1));
	logerror("/INT: %d; ", BIT(data,2));
	logerror("/RDY: %d; ", BIT(data,3));
	logerror("RTS: %d; ", BIT(data,4));
	logerror("CTS: %d; ", BIT(data,5));
	logerror("WR: %d; ", BIT(data,6));
	logerror("RD: %d;\n", BIT(data,7));
#endif
	// todo: poke serial handler here somehow?
}

READ8_MEMBER( pes_state::port3_r )
{
	UINT8 data = m_port3_state & 0xE3; // return last written state with rts, /rdy and /int masked out
	pes_state *state = machine().driver_data<pes_state>();
	// check rts state; if virtual fifo is nonzero, rts is set, otherwise it is cleared
	if (state->m_infifo_tail_ptr != state->m_infifo_head_ptr)
	{
		data |= 0x10; // set RTS bit
	}
	data |= (tms5220_intq_r(state->m_speech)<<2);
	data |= (tms5220_readyq_r(state->m_speech)<<3);
#ifdef DEBUG_PORTS
	logerror("port3 read: returning 0x%02X: ", data);
	logerror("RXD: %d; ", BIT(data,0));
	logerror("TXD: %d; ", BIT(data,1));
	logerror("/INT: %d; ", BIT(data,2));
	logerror("/RDY: %d; ", BIT(data,3));
	logerror("RTS: %d; ", BIT(data,4));
	logerror("CTS: %d; ", BIT(data,5));
	logerror("WR: %d; ", BIT(data,6));
	logerror("RD: %d;\n", BIT(data,7));
#endif
	return data;
}


/* Reset */
void pes_state::machine_reset()
{
	// clear fifos (TODO: memset would work better here...)
	int i;
	for (i=0; i<32; i++) m_infifo[i] = 0;
	m_infifo_tail_ptr = m_infifo_head_ptr = 0;
	m_wsstate = 1;
	m_rsstate = 1;

	m_port3_state = 0; // reset the openbus state of port 3
	//cputag_set_input_line(machine, "maincpu", INPUT_LINE_RESET, ASSERT_LINE); // this causes debugger to fail badly if included
	devtag_reset(machine(), "tms5220"); // reset the 5220
}

/******************************************************************************
 Driver Init and Timer Callbacks
******************************************************************************/
/*static TIMER_CALLBACK( serial_read_cb )
{
    machine.scheduler().timer_set(attotime::from_hz(10000), FUNC(outfifo_read_cb));
}*/

DRIVER_INIT_MEMBER(pes_state,pes)
{
	i8051_set_serial_tx_callback(machine().device("maincpu"), data_from_i8031);
	i8051_set_serial_rx_callback(machine().device("maincpu"), data_to_i8031);
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(i80c31_mem, AS_PROGRAM, 8, pes_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM /* 27C64 ROM */
	// AM_RANGE(0x2000, 0x3fff) AM_RAM /* 6164 8k SRAM, not populated */
ADDRESS_MAP_END

static ADDRESS_MAP_START(i80c31_io, AS_IO, 8, pes_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0, 0x0) AM_WRITE(rsws_w) /* /WS(0) and /RS(1) */
	AM_RANGE(0x1, 0x1) AM_READWRITE(port1_r, port1_w) /* tms5220 reads and writes */
	AM_RANGE(0x3, 0x3) AM_READWRITE(port3_r, port3_w) /* writes and reads from port 3, see top of file */
ADDRESS_MAP_END

/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( pes )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static MACHINE_CONFIG_START( pes, pes_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80C31, CPU_CLOCK)
	MCFG_CPU_PROGRAM_MAP(i80c31_mem)
	MCFG_CPU_IO_MAP(i80c31_io)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("tms5220", TMS5220C, 720000) /* 720Khz clock, 10khz output */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,pes_terminal_intf)
MACHINE_CONFIG_END

/******************************************************************************
 ROM Definitions
******************************************************************************/
ROM_START( pes )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_DEFAULT_BIOS("kevbios")
	ROM_SYSTEM_BIOS( 0, "orig", "PES box with original firmware v2.5")
	ROMX_LOAD( "vpu_2-5.bin",   0x0000, 0x2000, CRC(B27CFDF7) SHA1(C52ACF9C080823DE5EF26AC55ABE168AD53A7D38), ROM_BIOS(1)) // original firmware, rather buggy, 4800bps serial, buggy RTS/CTS flow control, no buffer
	ROM_SYSTEM_BIOS( 1, "kevbios", "PES box with kevtris' rewritten firmware")
	ROMX_LOAD( "pes.bin",   0x0000, 0x2000, CRC(22C1C4EC) SHA1(042E139CD0CF6FFAFCD88904F1636C6FA1B38F25), ROM_BIOS(2)) // rewritten firmware by kevtris, 4800bps serial, RTS/CTS plus XON/XOFF flow control, 64 byte buffer
ROM_END

/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY                        FULLNAME            FLAGS */
COMP( 1987, pes,    0,      0,      pes,        pes, pes_state,    pes, "Pacific Educational Systems", "VPU-01 Speech box", GAME_NOT_WORKING )
