/*
	Experimental exelvision driver

	Raphael Nabet, 2004

	Exelvision was a French company that designed and sold two computers:
	* EXL 100 (1984)
	* EXELTEL (1986), which is mostly compatible with EXL 100, but has an
	  integrated V23b modem and 5 built-in programs.  Two custom variants of
	  the EXELTEL were designed for chemist's shops and car dealers: they were
	  bundled with application-specific business software, bar code reader,
	  etc.
	These computer were mostly sold in France and in Europe (Spain); there was
	an Arabic version, too.

	Exelvision was founded by former TI employees, which is why their designs
	use TI components and have architectural reminiscences of the primitive
	TI-99/4 design (both computers are built around a microcontroller, have
	little CPU RAM and must therefore store program data in VRAM, and feature
	I/R keyboard and joysticks)

Specs:
	* main CPU is a variant of tms7020 (exl100) or tms7040 (exeltel).  AFAIK,
	  the only difference compared to a stock tms7020/7040 is the SWAP R0
	  instruction is replaced by a custom microcoded LVDP instruction that
	  reads a byte from the VDP VRAM read port; it seems that the first 6 bytes
	  of internal ROM (0xF000-0xF005 on an exeltel) are missing, too.
	* in addition to the internal 128-byte RAM and 2kb (exl100) or 4kb
	  (exeltel) ROM, there are 2kb of CPU RAM and 64(?)kb (exeltel only?) of
	  CPU ROM.
	* I/O is controlled by a tms7041 (exl100) or tms7042 (exeltel) or a variant
	  thereof.  Communication with the main CPU is done through some custom
	  interface (I think), details are still to be worked out.
	* video: tms3556 VDP with 32kb of VRAM (expandable to 64kb), attached to
	  the main CPU.
	* sound: tms5220 speech synthesizer with speech ROM, attached to the I/O
	  CPU
	* keyboard and joystick: an I/R interface controlled by the I/O CPU enables
	  to use a keyboard and two joysticks
	* mass storage: tape interface controlled by the I/O CPU

STATUS:
	* EXL 100 cannot be emulated because the ROMs are not dumped
	* EXELTEL stops early in the boot process and displays a red error screen,
	  presumably because the I/O processor is not emulated

TODO:
	* everything
*/

#include "driver.h"
#include "cpu/tms7000/tms7000.h"
#include "vidhrdw/tms3556.h"
/*#include "devices/cartslot.h"
#include "devices/cassette.h"*/


/*
	video initialization
*/
static int video_start_exelv(void)
{
	return tms3556_init(/*0x8000*/0x10000);	/* tms3556 with 32 kb of video RAM */
}

static void machine_init_exelv(void)
{
	tms3556_reset();
}

static void exelv_hblank_interrupt(void)
{
	tms3556_interrupt();
}

/*static DEVICE_LOAD(exelv_cart)
{
	return INIT_PASS;
}

static DEVICE_UNLOAD(exelv_cart)
{
}*/

/*
	I/O CPU protocol (WIP):

	I do not have a dump of the I/O CPU ROMs.

	* I/O CPU seems to be reset by port B bit >01 high.  This results into a
	  "I/O cpu initialized" (>08) code being posted in the mailbox.
	* port B bit >02 seems to be some acknowledgement of sorts.  When the I/O
	  CPU sees this line asserted, it asserts port A bit >01.
	* I/O CPU asserts main CPU INT1 line when ready to send data; data can be
	  read by the main CPU on the mailbox port (P48).  The data is a function
	  code optionally followed by several bytes of data.  Function codes are:
		>00: unused
		>01: joystick 0 receive
		>02: joystick 1 receive
		>03: speech buffer start
		>04: speech buffer end
		>05: serial
		>06: unused
		>07: introduction screen (logo) (EXL 100 only? what about EXELTEL?)
		>08: I/O cpu initialized
		>09: I/O cpu serial interface ready
		>0a: I/O cpu serial interface not ready
		>0b: screen switched off
		>0c: speech buffer start (EXELTEL only?)
		>0d: speech ROM or I/O cpu CRC check (EXELTEL only?)
			data byte #1: expected CRC MSB
			data byte #2: expected CRC LSB
			data byte #3: data length - 1 MSB
			data byte #4: data length - 1 LSB
			data bytes #5 through (data length + 5): data on which effective
				CRC is computed
		>0e: mailbox test, country code read (EXELTEL only?)
		>0f: speech ROM read (data repeat) (EXELTEL only?)
	* The main CPU sends data to the I/O CPU through the mailbox port (P48).
	  The data byte is a function code; some function codes ask for extra data
	  bytes, which are sent through the mailbox port as well.  Function codes
	  are:
		>00: I/O CPU reset
		>01: NOP (EXELTEL only?)
		>02: read joystick 0 current value
		>03: read joystick 1 current value
		>04: test serial interface availability
		>05: transmit a byte to serial interface
		>06: initialization of serial interface
		>07: read contents of speech ROM (EXELTEL only?)
		>08: reset speech synthesizer
		>09: start speech synthesizer
		>0a: synthesizer data
		>0b: standard generator request
		>0c: I/O CPU CRC (EXELTEL only?)
		>0d: send exelvision logo (EXL 100 only?), start speech ROM sound (EXELTEL only?)
		>0e: data for speech on ROM (EXELTEL only?)
		>0f: do not decode joystick 0 keys (EXELTEL only?)
		>10: do not decode joystick 1 keys (EXELTEL only?)
		>11: decode joystick 0 keys (EXELTEL only?)
		>12: decode joystick 1 keys (EXELTEL only?)
		>13: mailbox test: echo sent data (EXELTEL only?)
		>14: enter sleep mode (EXELTEL only?)
		>15: read country code in speech ROM (EXELTEL only?)
		>16: position I/O CPU DSR without initialization (EXELTEL only?)
		>17: handle speech ROM sound with address (EXELTEL only?)
*/

static int io_reset;
static int io_ack;
static int mailbox_out;

static void set_io_reset(int state)
{
	if (state != io_reset)
	{
		io_reset = state;
		if (io_reset)
		{
			/* reset I/O state */
			/*...*/
			/* clear mailbox and post I/O cpu intialized message instead */
			mailbox_out = 0x08;
			cpunum_set_input_line(0, TMS7000_IRQ1_LINE, PULSE_LINE);
		}
	}
}

static void set_io_ack(int state)
{
	if (state != io_ack)
	{
		io_ack = state;
		/* ??? */
	}
}

static READ8_HANDLER(mailbox_r)
{
	int reply = mailbox_out;

	/* clear mailbox */
	mailbox_out = 0x00;

	/* see if there are other messages to post */
	/*...*/

	return reply;
}

static WRITE8_HANDLER(mailbox_w)
{
	switch (data)
	{
	default:
		logerror("unemulated I/O command %d", (int) data);
		break;
	}
}

static READ8_HANDLER(exelv_porta_r)
{
	return io_ack ? 1 : 0;
}

static WRITE8_HANDLER(exelv_portb_w)
{
	set_io_reset((data & 0x1) != 0);
	set_io_ack((data & 0x2) != 0);
}

/*
	Main CPU memory map summary:

	@>0000-@>007f: tms7020/tms7040 internal RAM
	@>0080-@>00ff: reserved
	@>0100-@>010b: tms7020/tms7040 internal I/O ports
		@>104 (P4): port A
		@>106 (P6): port B
			bit >04: page select bit 0 (LSBit)
	@>010c-@>01ff: external I/O ports?
		@>012d (P45): tms3556 control write port???
		@>012e (P46): tms3556 VRAM write port???
		@>0130 (P48): I/O CPU communication port R/W ("mailbox")
		@>0138 (P56): read sets page select bit 1, write clears it???
		@>0139 (P57): read sets page select bit 2 (MSBit), write clears it???
		@>0140 (P64)
			bit >40: enable page select bit 1 and 2 (MSBits)
	@>0200-@>7fff: system ROM? (two pages?) + cartridge ROMs? (one or two pages?)
	@>8000-@>bfff: free for expansion?
	@>c000-@>c7ff: CPU RAM?
	@>c800-@>efff: free for expansion?
	@>f000-@>f7ff: tms7040 internal ROM
	@>f800-@>ffff: tms7020/tms7040 internal ROM
*/

static ADDRESS_MAP_START(exelv_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x007f) AM_READWRITE(tms7000_internal_r, tms7000_internal_w)/* tms7020 internal RAM */
	AM_RANGE(0x0080, 0x00ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/* reserved */
	AM_RANGE(0x0100, 0x010b) AM_READWRITE(tms70x0_pf_r, tms70x0_pf_w)/* tms7020 internal I/O ports */
	//AM_RANGE(0x010c, 0x01ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/* external I/O ports */
	AM_RANGE(0x012d, 0x0012d) AM_READWRITE(MRA8_NOP, tms3556_reg_w)
	AM_RANGE(0x012e, 0x0012e) AM_READWRITE(MRA8_NOP, tms3556_vram_w)
	AM_RANGE(0x0130, 0x00130) AM_READWRITE(mailbox_r, mailbox_w)
	AM_RANGE(0x0200, 0x7fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)		/* system ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/* CPU RAM */
	AM_RANGE(0xc800, /*0xf7ff*/0xefff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(/*0xf800*/0xf000, 0xffff) AM_READWRITE(MRA8_ROM, MWA8_ROM)/* tms7020 internal ROM */

ADDRESS_MAP_END

static ADDRESS_MAP_START(exelv_portmap, ADDRESS_SPACE_IO, 8)

	AM_RANGE(TMS7000_PORTA, TMS7000_PORTA) AM_READ(exelv_porta_r)
	AM_RANGE(TMS7000_PORTB, TMS7000_PORTB) AM_WRITE(exelv_portb_w)

ADDRESS_MAP_END


/* keyboard: ??? */
static INPUT_PORTS_START(exelv)

	PORT_START

INPUT_PORTS_END


static MACHINE_DRIVER_START(exelv)

	/* basic machine hardware */
	/* TMS7020 CPU @ 4.91(?) MHz */
	MDRV_CPU_ADD(TMS7000_EXL, 4910000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_PROGRAM_MAP(exelv_memmap, 0)
	MDRV_CPU_IO_MAP(exelv_portmap, 0)
	MDRV_CPU_VBLANK_INT(exelv_hblank_interrupt, 363)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( exelv )
	/*MDRV_MACHINE_STOP( exelv )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_VIDEO_START(exelv)
	MDRV_TMS3556

	/* sound */
	MDRV_SOUND_ATTRIBUTES(0)
	/*MDRV_SOUND_ADD(TMS5220, tms5220interface)*/

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(exeltel)
	/*CPU memory space*/
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("exeltel14.bin", 0x0000, 0x8000, CRC(52a80dd4))      /* system ROM */
	ROM_LOAD("exeltelin.bin", 0xf006, 0x0ffa, CRC(c12f24b5))      /* internal ROM */
ROM_END

SYSTEM_CONFIG_START(exelv)

	/* cartridge port is not emulated */

SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT		COMPAT	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
/*COMP(	1984,	exl100,	0,			0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exl 100" )*/
COMP(	1986,	exeltel,0/*exl100*/,0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exeltel" )
