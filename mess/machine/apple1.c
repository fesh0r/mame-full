/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  The Apple I used a Motorola 6820 PIA for its keyboard and display
  I/O.  The keyboard was mapped to PIA port A, and the display to port
  B.

  Port A, the keyboard, was an input port connected to a standard
  ASCII-encoded keyboard.  The high bit of the port was tied to +5V.
  The keyboard strobe signal was connected to the PIA's CA1 control
  input so that the keyboard could signal each keypress to the PIA.
  The processor could check for a keypress by testing the IRQA1 flag
  in the Port A Control Register and then reading the character value
  from Port A.

  The keyboard connector also had two special lines, RESET and CLEAR
  SCREEN, which were meant to be connected to pushbutton switches on
  the keyboard.  RESET was tied to the reset inputs for the CPU and
  PIA; it allowed the user to stop a program and return control to the
  Monitor.  CLEAR SCREEN was directly tied to the video hardware and
  would clear the display.

  Port B, the display, was an output port which accepted 7-bit ASCII
  characters from the PIA and wrote them on the display.  The details
  of this are described in vidhrdw/apple1.c.  Control line CB2 served
  as an output signal to inform the display of a new character.  (CB2
  was also connected to line 7 of port B, which was configured as an
  input, so that the CPU could more easily check the status of the
  write.)  The CB1 control input signaled the PIA when the display had
  finished writing the character and could accept a new one.

  MAME models the 6821 instead of the earlier 6820 used in the Apple
  I, but there is no difference in functionality between the two
  chips; the 6821 simply has a better ability to drive electrical
  loads.

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6502/m6502.h"
#include "inptport.h"
#include "includes/apple1.h"
#include "image.h"

static void apple1_kbd_poll(int dummy);
static void apple1_kbd_strobe_end(int dummy);

static READ8_HANDLER( apple1_pia0_kbdin );
static WRITE8_HANDLER( apple1_pia0_dspout );
static WRITE8_HANDLER( apple1_pia0_dsp_write_signal );

static void apple1_dsp_ready_start(int dummy);
static void apple1_dsp_ready_end(int dummy);

/*****************************************************************************
**	Structures
*****************************************************************************/

/* Handlers for the PIA.  The inputs for Port B, CA1, and CB1 are
   handled by writing to them at the moment when their values change,
   rather than updating them when they are read; thus they don't need
   handler functions. */

struct pia6821_interface apple1_pia0 =
{
	apple1_pia0_kbdin,				/* Port A input (keyboard) */
	0,								/* Port B input (display status) */
	0,								/* CA1 input (key pressed) */
	0,								/* CB1 input (display ready) */
	0,								/* CA2 not used as input */
	0,								/* CB2 not used as input */
	0,								/* Port A not used as output */
	apple1_pia0_dspout,				/* Port B output (display) */
	0,								/* CA2 not used as output */
	apple1_pia0_dsp_write_signal,	/* CB2 output (display write) */
	0,								/* IRQA not connected */
	0								/* IRQB not connected */
};

/* Use the same keyboard mapping as on a modern keyboard.  This is not
   the same as the keyboard mapping of the actual teletype-style
   keyboards used with the Apple I, but it's less likely to cause
   confusion for people who haven't memorized that layout.

   The Backspace key is mapped to the '_' (underscore) character
   because the Apple I ROM Monitor used "back-arrow" to erase
   characters, rather than backspace, and back-arrow is an earlier
   form of the underscore. */

#define ESCAPE	'\x1b'

static UINT8 apple1_unshifted_keymap[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '-', '=', '[', ']', ';', '\'',
	',', '.', '/', '\\', 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', '\r', '_',
	' ', ESCAPE
};

static UINT8 apple1_shifted_keymap[] =
{
	')', '!', '@', '#', '$', '%', '^', '&',
	'*', '(', '_', '+', '[', ']', ':', '"',
	'<', '>', '?', '\\', 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', '\r', '_',
	' ', ESCAPE
};

/* Control key mappings, like the other mappings, conform to a modern
   keyboard where possible.  Note that the Apple I ROM Monitor ignores
   most control characters. */

static UINT8 apple1_control_keymap[] =
{
	'0', '1', '\x00', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
	'8', '9', '\x1f', '=', '\x1b', '\x1d', ';', '\'',
	',', '.', '/', '\x1c', '\x01', '\x02', '\x03', '\x04',
	'\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c',
	'\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14',
	'\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\r', '_',
	'\x00', ESCAPE
};

static int apple1_kbd_data = 0;


/*****************************************************************************
**	driver init
*****************************************************************************/

DRIVER_INIT( apple1 )
{
	/* install RAM handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, MWA8_BANK1);
	cpu_setbank(1, mess_ram);

	pia_config(0, PIA_8BIT | PIA_AUTOSENSE, &apple1_pia0);

	/* Poll the keyboard input ports periodically.  These include both
	   ordinary keys and the RESET and CLEAR SCREEN pushbutton
	   switches.  We can't handle these switches in a VBLANK_INT or
	   PERIODIC_INT because both switches need to be monitored even
	   while the CPU is suspended during RESET; VBLANK_INT and
	   PERIODIC_INT callbacks aren't run while the CPU is in this
	   state.

	   A 120-Hz poll rate seems to be fast enough to ensure no
	   keystrokes are missed. */
	timer_pulse(TIME_IN_HZ(120), 0, apple1_kbd_poll);
}

/*****************************************************************************
**	machine init:  actions to perform on cold boot
*****************************************************************************/

MACHINE_INIT( apple1 )
{
	/* On cold boot, reset the PIA and the display hardware. */
	pia_reset();
	apple1_vh_dsp_clr();
}

/*****************************************************************************
**	apple1_verify_header
*****************************************************************************/
static int apple1_verify_header (UINT8 *data)
{
	/* Verify the format for the snapshot */
	if ((data[0] == 'L') &&
		(data[1] == 'O') &&
		(data[2] == 'A') &&
		(data[3] == 'D') &&
		(data[4] == ':') &&
		(data[7] == 'D') &&
		(data[8] == 'A') &&
		(data[9] == 'T') &&
		(data[10]== 'A') &&
		(data[11]== ':'))
	{
		return(IMAGE_VERIFY_PASS);
	}
	else
	{
		return(IMAGE_VERIFY_FAIL);
	}
}


/*****************************************************************************
**	apple1_load_snap
**	Format of the binary SnapShot Image is:
**	[ LOAD:xxyyDATA:zzzzzzzzzzzzz....]
**	Where xxyy is the hex value to load the Data zzzzzzz to
**
*****************************************************************************/
SNAPSHOT_LOAD(apple1)
{
	UINT8 *memptr;
	UINT8 snapdata[0x1000];
	UINT16 starting_offset = 0x0000;

	/* Load the specified Snapshot */

	/* Read the snapshot data into a temporary array */
	if (mame_fread(fp, snapdata, 0x1000) != 0x1000)
		return INIT_FAIL;

	/* Verify the snapshot header */
	if (apple1_verify_header(snapdata) == IMAGE_VERIFY_FAIL)
	{
		logerror("Apple1 - Snapshot Header is in incorrect format - needs to be LOAD:xxyyDATA:\n");
		return INIT_FAIL;
	}

	/* Extract the starting offset to load the snapshot to! */
	starting_offset = (snapdata[5] << 8) | (snapdata[6]);
	logerror("Apple1 - LoadAddress is 0x%04x\n", starting_offset);

	/* Point to the region where the snapshot will be loaded to */
	memptr = memory_region(REGION_CPU1) + starting_offset;

	/* Copy the Actual Data into Memory Space */
	memcpy(memptr, &snapdata[12], 0x1000);

	return INIT_PASS;
}


/*****************************************************************************
**	apple1_kbd_poll
**
**	Keyboard polling handles both ordinary keys and the special RESET
**	and CLEAR SCREEN switches.
**
**	For ordinary keys, this implements 2-key rollover to reduce the
**	chance of missed keypresses.  If we press a key and then press a
**	second key while the first hasn't been completely released, as
**	might happen during rapid typing, only the second key is
**	registered; the first key is ignored.
**
**	If multiple newly-pressed keys are found, the one closest to the
**	end of the input ports list is counted; the others are ignored.
*****************************************************************************/
static void apple1_kbd_poll(int dummy)
{
	int port, bit;
	int key_pressed;
	UINT32 shiftkeys, ctrlkeys;

	/* This holds the values of all the input ports for ordinary keys
	   seen during the last scan. */
	static UINT32 kbd_last_scan[] = { 0, 0, 0, 0 };

	static int reset_flag = 0;

	/* First we check the RESET and CLEAR SCREEN pushbutton switches. */

	/* The RESET switch resets the CPU and the 6820 PIA. */
	if (readinputport(5) & 0x0001)
	{
		if (!reset_flag) {
			reset_flag = 1;
			/* using PULSE_LINE does not allow us to press and hold key */
			cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
			pia_reset();
		}
	}
	else if (reset_flag) {
		/* RESET released--allow the processor to continue. */
		reset_flag = 0;
		cpunum_set_input_line(0, INPUT_LINE_RESET, CLEAR_LINE);
	}

	/* The CLEAR SCREEN switch clears the video hardware. */
	if (readinputport(5) & 0x0002)
	{
		if (!apple1_vh_clrscrn_pressed)
		{
			/* Ignore further video writes, and clear the screen. */
			apple1_vh_clrscrn_pressed = 1;
			apple1_vh_dsp_clr();
		}
	}
	else if (apple1_vh_clrscrn_pressed)
	{
		/* CLEAR SCREEN released--pay attention to video writes again. */
		apple1_vh_clrscrn_pressed = 0;
	}

	/* Now we scan all the input ports for ordinary keys, recording
	   new keypresses while ignoring keys that were already pressed in
	   the last scan. */

	apple1_kbd_data = 0;
	key_pressed = 0;

	/* The keyboard strobe line should always be low when a scan starts. */
	pia_set_input_ca1(0, 0);

	shiftkeys = readinputport(4) & 0x0003;
	ctrlkeys = readinputport(4) & 0x000c;

	for (port = 0; port < 4; port++)
	{
		UINT32 portval = readinputport(port);
		UINT32 newkeys = portval & ~(kbd_last_scan[port]);

		if (newkeys)
		{
			key_pressed = 1;
			for (bit = 0; bit < 16; bit++) {
				if (newkeys & 1)
				{
					apple1_kbd_data = (ctrlkeys)
					  ? apple1_control_keymap[port*16 + bit]
					  : (shiftkeys)
					  ? apple1_shifted_keymap[port*16 + bit]
					  : apple1_unshifted_keymap[port*16 + bit];
				}
				newkeys >>= 1;
			}
		}
		kbd_last_scan[port] = portval;
	}

	if (key_pressed)
	{
		/* The keyboard will pulse its strobe line when a key is
		   pressed.  A 10-usec pulse is typical. */
		pia_set_input_ca1(0, 1);
		timer_set(TIME_IN_USEC(10), 0, apple1_kbd_strobe_end);
	}
}

static void apple1_kbd_strobe_end(int dummy)
{
	/* End of the keyboard strobe pulse. */
	pia_set_input_ca1(0, 0);
}


/*****************************************************************************
**	READ/WRITE HANDLERS
*****************************************************************************/
static READ8_HANDLER( apple1_pia0_kbdin )
{
	/* Bit 7 of the keyboard input is permanently wired high.  This is
	   what the ROM Monitor software expects. */
	return apple1_kbd_data | 0x80;
}

static WRITE8_HANDLER( apple1_pia0_dspout )
{
	/* Send an ASCII character to the video hardware. */
	apple1_vh_dsp_w(data);
}

static WRITE8_HANDLER( apple1_pia0_dsp_write_signal )
{
	/* PIA output CB2 is inverted to become the DA signal, used to
	   signal a display write to the video hardware. */

	/* DA is directly connected to PIA input PB7, so the processor can
	   read bit 7 of port B to test whether the display has completed
	   a write. */
	pia_set_input_b(0, (!data) << 7);

	/* Once DA is asserted, the display will wait until it can perform
	   the write, when the cursor position is about to be refreshed.
	   Only then will it assert \RDA to signal readiness for another
	   write.  Thus the write delay depends on the cursor position and
	   where the display is in the refresh cycle. */
	if (!data)
		timer_set(apple1_vh_dsp_time_to_ready(), 0, apple1_dsp_ready_start);
}

static void apple1_dsp_ready_start(int dummy)
{
	/* When the display asserts \RDA to signal it is ready, it
	   triggers a 74123 one-shot to send a 3.5-usec low pulse to PIA
	   input CB1.  The end of this pulse will tell the PIA that the
	   display is ready for another write. */
	pia_set_input_cb1(0, 0);
	timer_set(TIME_IN_USEC(3.5), 0, apple1_dsp_ready_end);
}

static void apple1_dsp_ready_end(int dummy)
{
	/* The one-shot pulse has ended; return CB1 to high, so we can do
	   another display write. */
	pia_set_input_cb1(0, 1);
}
