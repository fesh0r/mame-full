/*

	FUNVISION TECHNICAL INFO
	========================

	CHIPS
	=====

	2MHZ 6502A cpu
	2 x 2114 (1K RAM)
	9929 Video chip with 16kb vram
	76489 sound chip
	6821 PIA

	MEMORY MAP
	==========

	0000 - 0FFF     1K RAM repeated 4 times
	1000 - 1FFF     PIA (only 4 addresses)
	2000 - 2FFF     Read VDP chip (only occupies 2 addresses)
	3000 - 3FFF     Write VDP chip (only occupies 2 addresses)
	4000 - 7FFF     2nd cartridge ROM area
	8000 - BFFF     1st cartridge ROM area 
	C000 - FFFF     2K boot ROM repeated 8 times.

	Video
	=====

	Most code writes to $3000 and $3001 and reads from $2000 and $2001. Go read 
	a manual for the 9929/9918 for how it all works.

	Sound
	=====

	The 76489 is wired into port B of the PIA. You just write single bytes to it.
	Its a relatively slow access device so this is undoubtedly why they wired it to
	the PIA rather than straight to the data bus. I think they configure the PIA to
	automatically send a strobe signal when you write to $1002.

	Keyboard
	========

	The keyboard is the weirdest thing imaginable. Visually it consists of the two
	hand controllers slotted into the top of the console. The left joystick and
	24 keys on one side and the right joystick and another 24 keys on the other.
	The basic layout of the keys is:

	Left controller
	---------------

	 1     2     3     4     5     6
	ctrl   Q     W     E     R     T
	 <-    A     S     D     F     G
	shft   Z     X     C     V     B

	Right controller
	----------------

	 7     8     9     0     :     -
	 Y     U     I     O     P    Enter
	 H     J     K     L     ;     ->
	 N     M     ,     .     /    space

	The left controller is wired to the PIA pins pa0, pa1 and pb0-7
	The right controller is wired to the PIA pins pa2, pa3 and pb0-7

	The basic key scanning routine sets pa0-3 high then sends pa0 low, then pa1
	low and so forth and each time it reads back a value into pb0-7. You might ask
	the question 'How do they read 48 keys and two 8(16?) position joysticks using
	a 4 x 8 key matrix?'. Somehow when you press a key and the appropriate 
	strobe is low, two of the 8 input lines are sent low instead of 1. I have
	no idea how they do this. This allows them to read virtually all 24 keys on
	one controller by just sending one strobe low. The strobes go something like:

		PA? low          keys
		-------          ----
		PA3             right hand keys
		PA2             right joystick
		PA1             left hand keys
		PA0             left joystick

	An example of a key press is the 'Y' key. Its on the right controller so is
	scanned when PA3 is low. It will return 11111010 ($FA).

	There are some keys that don't follow the setup above. These are the '1', Left
	arrow, space and right arrow. They are all scanned as part of the corresponding
	joystick strobe. eg. '1' is detected by sending PA0 low and reading back a 
	11110011 ($F3). Also some keys are effectively the same as the fire buttons
	on the controllers. Left shift and control act like the fire buttons on the 
	left controller and right arrow and '-' act the same as the fire buttons on the
	right controller.

	---

	MESS Driver by Curt Coder
	Based on the FunnyMu emulator by ???

	---

	TODO:

	- inputs
	- only astro pinball & chopper rescue carts work

*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/tms9928a.h"
#include "machine/6821pia.h"
#include "sound/sn76496.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( fnvision_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_MIRROR(0x0c00)
//	AM_RANGE(0x1000, 0x1003) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x1002, 0x1002) AM_WRITE(SN76496_0_w)		// HACK
	AM_RANGE(0x1003, 0x1003) AM_READ(input_port_0_r)	// HACK
	AM_RANGE(0x2000, 0x2000) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_ROM AM_MIRROR(0x3800)
ADDRESS_MAP_END

INPUT_PORTS_START( fnvision )
    PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

/*
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "1", KEYCODE_1, CODE_NONE, '1' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "2", KEYCODE_2, CODE_NONE, '2' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "3", KEYCODE_3, CODE_NONE, '3' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "4", KEYCODE_4, CODE_NONE, '4' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "5", KEYCODE_5, CODE_NONE, '5' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "6", KEYCODE_6, CODE_NONE, '6' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "Q", KEYCODE_Q, CODE_NONE, 'Q' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "W", KEYCODE_W, CODE_NONE, 'W' )

	PORT_START	// IN
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "E", KEYCODE_E, CODE_NONE, 'E' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "R", KEYCODE_R, CODE_NONE, 'R' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "T", KEYCODE_T, CODE_NONE, 'T' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "A", KEYCODE_A, CODE_NONE, 'A' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "S", KEYCODE_S, CODE_NONE, 'S' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "D", KEYCODE_D, CODE_NONE, 'D' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "F", KEYCODE_F, CODE_NONE, 'F' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "G", KEYCODE_G, CODE_NONE, 'G' )

	PORT_START	// IN
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "Z", KEYCODE_Z, CODE_NONE, 'Z' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "X", KEYCODE_X, CODE_NONE, 'X' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "C", KEYCODE_C, CODE_NONE, 'C' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "V", KEYCODE_V, CODE_NONE, 'V' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "B", KEYCODE_B, CODE_NONE, 'B' )
	PORT_KEY0( 0x0, IP_ACTIVE_LOW, "CTRL", KEYCODE_LCONTROL, KEYCODE_RCONTROL )
	PORT_KEY0( 0x0, IP_ACTIVE_LOW, "(left)", KEYCODE_LEFT, CODE_NONE )
	PORT_KEY0( 0x0, IP_ACTIVE_LOW, "SHIFT", KEYCODE_LSHIFT, KEYCODE_RSHIFT )

	PORT_START	// IN
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START	// IN
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "7", KEYCODE_7, CODE_NONE, '7' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "8", KEYCODE_8, CODE_NONE, '8' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "9", KEYCODE_9, CODE_NONE, '9' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "0", KEYCODE_0, CODE_NONE, '0' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, ":", KEYCODE_OPENBRACE, CODE_NONE, ':' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "-", KEYCODE_MINUS, CODE_NONE, '-' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "Y", KEYCODE_Y, CODE_NONE, 'Y' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "U", KEYCODE_U, CODE_NONE, 'U' )

    PORT_START	// IN
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "I", KEYCODE_I, CODE_NONE, 'I' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "O", KEYCODE_O, CODE_NONE, 'O' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "P", KEYCODE_P, CODE_NONE, 'P' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "H", KEYCODE_H, CODE_NONE, 'H' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "J", KEYCODE_J, CODE_NONE, 'J' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "K", KEYCODE_K, CODE_NONE, 'K' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "L", KEYCODE_L, CODE_NONE, 'L' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, ";", KEYCODE_CLOSEBRACE, CODE_NONE, ';' )

    PORT_START	// IN
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "N", KEYCODE_N, CODE_NONE, 'N' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "M", KEYCODE_M, CODE_NONE, 'M' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, ",", KEYCODE_COMMA, CODE_NONE, ',' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, ".", KEYCODE_STOP, CODE_NONE, '.' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "/", KEYCODE_SLASH, CODE_NONE, '/' )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, CODE_NONE, '\r' )
	PORT_KEY0( 0x0, IP_ACTIVE_LOW, "(right)", KEYCODE_RIGHT, CODE_NONE )
	PORT_KEY1( 0x0, IP_ACTIVE_LOW, "(space)", KEYCODE_SPACE, CODE_NONE, ' ' )

    PORT_START	// IN
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)		PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)	PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)	PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)	PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1)			PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNUSED )
*/
INPUT_PORTS_END

static struct SN76496interface sn76496_interface =
{
    1,  			// 1 chip
    { 2000000 },	// ???
    { 100 }
};

static INTERRUPT_GEN( fnvision_int )
{
    TMS9928A_interrupt();
}

static void fnvision_vdp_interrupt(int state)
{
	static int last_state;

    // only if it goes up
	if (state && !last_state)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
	last_state = state;
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929,
	0x4000,
	fnvision_vdp_interrupt
};

static int pa;

static READ8_HANDLER( pia0_portb_r )
{
	switch (pa)
	{
	case 0:
		return readinputport(0);
	case 1:
		return readinputport(1);
	case 2:
		return readinputport(2);
	case 3:
		return readinputport(3);
	}

	return 0xff;
}

static WRITE8_HANDLER( pia0_porta_w )
{
	pa = ~data;
	logerror("%u",data);
}

static struct pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, pia0_portb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia0_porta_w, SN76496_0_w, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

static MACHINE_INIT( fnvision )
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_reset();
}

static MACHINE_DRIVER_START( fnvision )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 2000000)	// ???
	MDRV_CPU_PROGRAM_MAP(fnvision_map, 0)
	MDRV_CPU_VBLANK_INT(fnvision_int, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( fnvision )

    // video hardware
	MDRV_TMS9928A(&tms9928a_interface)

	// sound hardware
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

ROM_START( fnvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "funboot.rom", 0xc000, 0x0800, CRC(05602697) SHA1(c280b20c8074ba9abb4be4338b538361dfae517f) )
ROM_END

DEVICE_LOAD( fnvision_cart )
{
	/*

		Cartridge Image format
		======================

		- The first 16K is read into 8000 - BFFF. If the cart file is less than 16k
		then the cartridge is read into 8000, then replicated to fill up the 16k
		(eg. a 4k cartridge file is written to 8000 - 8FFF, then replicated at
		9000-9FFF, A000-AFFF and B000-BFFF).
		- The next 16k is read into 4000 - 7FFF. If this extra bit is less than 16k,
		then the data is replicated throughout 4000 - 7FFF.

		For example, an 18k cartridge dump has its first 16k read and written into
		memory at 8000-BFFF. The remaining 2K is written into 4000-47FF, then 
		replicated 8 times to appear at 4800, 5000, 5800, 6000, 6800, 7000 and 7800.

	*/

	if (file)
	{
		int size = mame_fread(file, memory_region(REGION_CPU1) + 0x8000, 0x4000);

		switch (size)
		{
		case 0x1000:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0x3000, MRA8_ROM);
			break;
		case 0x2000:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_ROM);
			break;
		case 0x4000:
			size = mame_fread(file, memory_region(REGION_CPU1) + 0x4000, 0x4000);

			switch (size)
			{
			case 0x0800:
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x47ff, 0, 0x3800, MRA8_ROM);
				break;
			case 0x1000:
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4fff, 0, 0x3000, MRA8_ROM);
				break;
			case 0x2000:
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_ROM);
				break;
			}
			break;
		}
	}

	return 0;
}

SYSTEM_CONFIG_START( fnvision )
	CONFIG_DEVICE_CARTSLOT_REQ( 1, "rom\0", NULL, NULL, device_load_fnvision_cart, NULL, NULL, NULL )
SYSTEM_CONFIG_END

//    YEAR	NAME	  PARENT COMPAT MACHINE INPUT INIT COMPANY FULLNAME
CONSX( 1981, fnvision, 0, 0, fnvision, fnvision, 0, fnvision, "Video Technology", "FunVision", GAME_NOT_WORKING )
