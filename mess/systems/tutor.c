/*
	Experimental Tomy Tutor driver

	This computer is known as Tomy Tutor in US, and as Grandstand Tutor in UK.
	It was initially released in Japan in 1982 or 1983 under the name of Pyuuta
	(Pi-yu-u-ta, with a Kanji for the "ta").  The Japanese versions are
	different from the English-language versions, as they have different ROMs
	with Japanese messages and support for the katakana syllabus.  There are at
	least 4 versions:
	* original Pyuuta (1982 or 1983) with title screens in Japanese but no
	  Basic
	* Pyuuta Jr. (1983?) which is a console with a simplified keyboard
	* Tomy/Grandstand Tutor (circa October 1983?) with title screens in English
	  and integrated Basic
	* Pyuuta Mk. 2 (1984?) with a better-looking keyboard and integrated Basic

	The Tomy Tutor features a TMS9995 CPU @10.7MHz (which includes a
	timer/counter and 256 bytes of 16-bit RAM), 48kb of ROM (32kb on early
	models that did not have the BASIC interpreter), a tms9918a/9929a VDP (or
	equivalent?) with 16kb of VRAM, and a sn76496 sound generator (or
	equivalent?).  There is a tape interface, a 56-key keyboard, an interface
	for two joysticks, a cartridge port and an extension port.  The OS design
	does not seem to be particularly expandable (I don't see any hook for
	additional DSRs), but there were prototypes for a parallel port (emulated)
	and a speech synthesizer unit (not emulated).


	The Tutor appears to be related to Texas Instruments' TI99 series.

	The general architecture is relatively close to the ti99/4(a): arguably,
	the Tutor does not include any GROM (it uses regular CPU ROMs for GPL
	code), and its memory map is quite different due to the fact it was
	designed with a tms9995 in mind (vs. a tms9985 for ti99/4), but, apart from
	that, it has a similar architecture with only 256 bytes of CPU RAM and 16kb
	of VRAM.

	While the OS is not derived directly from the TI99/4(a) OS, there are
	disturbing similarities: the Japanese title screen is virtually identical
	to the TI-99 title screen.  Moreover, the Tutor BASIC seems to be be
	derived from TI Extended BASIC, as both BASIC uses similar tokens and
	syntax, and are partially written in GPL (there is therefore a GPL
	interpreter in Tutor ROMs, although the Tutor GPL is incompatible with TI
	GPL, does not seem to be used by any program other than Tutor Basic, and it
	seems unlikely that the original Pyuuta had this GPL interpreter in ROMs).

	It appears that TI has sold the licence of the TI BASIC to Tomy, probably
	after it terminated its TI99 series.  It is not impossible that the entire
	Tutor concept is derived from a TI project under licence: this machine
	looks like a crossbreed of the TI99/2 and the TI99/4 (or /4a, /4b, /5), and
	it could either have been an early version of the TI99/2 project with a
	tms9918a/99289a VDP instead of the DMA video controller, or a "TI99/3" that
	would have closed the gap between the extremely low-end TI99/2 and the
	relatively mid-range TI99/5.


	Raphael Nabet, 2003


TODO :
	* debug the tape interface (Saved tapes sound OK, both Verify and Load
	  recognize the tape as a Tomy tape, but the data seems to be corrupted and
	  we end with a read error.)
	* guess which device is located at the >e600 base
	* find info about other Tutor variants


	Interrupts:

	Interrupt levels 1 (external interrupt 1) and 2 (error interrupt) do not
	seem to be used: triggering these seems to cause a soft reset.  XOPs are
	not used at all: the ROM area where these vectors should be defined is used
	by a ROM branch table.
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "vidhrdw/tms9928a.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"


/* mapper state */
static char cartridge_enable;

/* tape interface state */
static void tape_interrupt_handler(int dummy);

static char tape_interrupt_enable;
static void *tape_interrupt_timer;

/* parallel interface state */
static mame_file *printer_fp;
static UINT8 printer_data;
static char printer_strobe;

enum
{
	basic_base = 0x8000,
	cartridge_base = 0xe000
};


static void machine_init_tutor(void)
{
	cartridge_enable = 0;

	tape_interrupt_enable = 0;
	tape_interrupt_timer = timer_alloc(tape_interrupt_handler);

	printer_data = 0;
	printer_strobe = 0;
}

static void tutor_vblank_interrupt(void)
{
	/* No vblank interrupt? */
	TMS9928A_interrupt();
}

/*
	Keyboard:

	Keyboard ports are located at CRU logical address >ec00->ec7e (CRU physical
	address >7600->763f).  There is one bit per key (bit >00 for keycode >00,
	bit >01 for keycode >01, etc.), each bit is set to one when the key is
	down.

	Joystick:

	Joystick ports seem to overlap keyboard ports, i.e. some CRU bits are
	mapped to both a keyboard key and a joystick switch.
*/

static READ_HANDLER(read_keyboard)
{
	return readinputport(offset);
}

static DEVICE_LOAD(tutor_cart)
{
	mame_fread(file, memory_region(REGION_CPU1) + cartridge_base, 0x6000);

	return INIT_PASS;
}

static DEVICE_UNLOAD(tutor_cart)
{
	memset(memory_region(REGION_CPU1) + cartridge_base, 0x6000, 0);
}

/*
	Cartridge mapping:

	Cartridges share the >8000 address base with BASIC.  A write to @>e10c
	disables the BASIC ROM and enable the cartridge.  A write to @>e108
	disables the cartridge and enables the BASIC ROM.

	In order to be recognized by the system ROM, a cartridge should start with
	>55, >66 or >aa.  This may may correspond to three different ROM header
	versions (I am not sure).

	Cartridge may also define a boot ROM at base >0000 (see below).
*/

static READ_HANDLER(tutor_mapper_r)
{
	int reply;

	switch (offset)
	{
	case 0x10:
		/* return 0x42 if we have an cartridge with an alternate boot ROM */
		reply = 0;
		break;

	default:
		logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		reply = 0;
		break;
	}

	return reply;
}

static WRITE_HANDLER(tutor_mapper_w)
{
	switch (offset)
	{
	case 0x00:
		/* disable system ROM, enable alternate boot ROM in cartridge */
		break;

	case 0x08:
		/* disable cartridge ROM, enable BASIC ROM at base >8000 */
		cartridge_enable = 0;
		cpu_setbank(1, memory_region(REGION_CPU1) + basic_base);
		break;

	case 0x0c:
		/* enable cartridge ROM, disable BASIC ROM at base >8000 */
		cartridge_enable = 1;
		cpu_setbank(1, memory_region(REGION_CPU1) + cartridge_base);
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

/*
	Cassette interface:

	The cassette interface uses several ports in the >e000 range.

	Writing to *CPU* address @>ee00 will set the tape output to 0.  Writing to
	*CPU* address @>ee20 will set the tape output to 1.

	Tape input level can be read from *CRU* logical address >ed00 (CRU physical
	address >7680).

	Writing to @>ee40 enables tape interrupts; writing to @>ee60 disables tape
	interrupts.  Tape interrupts are level-4 interrupt that occur when the tape
	input level is high(?).

	There are other output ports: @>ee80, @>eea0, @>eec0 & @>eee0.  I don't
	know their exact meaning.
*/

static void tape_interrupt_handler(int dummy)
{
	//assert(tape_interrupt_enable);
	cpu_set_irq_line(0, 1, (device_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0) ? ASSERT_LINE : CLEAR_LINE);
}

/* CRU handler */
static READ_HANDLER(tutor_cassette_r)
{
	return (device_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0) ? 1 : 0;
}

/* memory handler */
static WRITE_HANDLER(tutor_cassette_w)
{
	if (offset & /*0x1f*/0x1e)
		logerror("unknown port in %s %d\n", __FILE__, __LINE__);

	if ((offset & 0x1f) == 0)
	{
		data = (offset & 0x20) != 0;

		switch ((offset >> 6) & 3)
		{
		case 0:
			/* data out */
			device_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data) ? 32767 : -32767);
			break;
		case 1:
			/* interrupt control??? */
			//logerror("ignoring write of %d to cassette port 1\n", data);
			if (tape_interrupt_enable != ! data)
			{
				tape_interrupt_enable = ! data;
				if (tape_interrupt_enable)
					timer_adjust(tape_interrupt_timer, /*TIME_IN_HZ(44100)*/0., 0, TIME_IN_HZ(44100));
				else
				{
					timer_adjust(tape_interrupt_timer, TIME_NEVER, 0, 0.);
					cpu_set_irq_line(0, 1, CLEAR_LINE);
				}
			}
			break;
		case 2:
			/* ??? */
			logerror("ignoring write of %d to cassette port 2\n", data);
			break;
		case 3:
			/* ??? */
			logerror("ignoring write of %d to cassette port 3\n", data);
			break;
		}
	}
}

static DEVICE_LOAD(tutor_printer)
{
	printer_fp = file;

	return INIT_PASS;
}

static DEVICE_UNLOAD(tutor_printer)
{
	printer_fp = NULL;
}

/* memory handlers */
static READ_HANDLER(tutor_printer_r)
{
	int reply;

	switch (offset)
	{
	case 0x20:
		/* busy */
		reply = (printer_fp) ? 0xff : 0x00;
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		reply = 0;
		break;
	}

	return reply;
}

static WRITE_HANDLER(tutor_printer_w)
{
	switch (offset)
	{
	case 0x10:
		/* data */
		printer_data = data;
		break;

	case 0x40:
		/* strobe */
		if (data && ! printer_strobe)
		{
			/* strobe is asserted: output data */
			if (printer_fp)
				mame_fwrite(printer_fp, & printer_data, 1);
		}
		printer_strobe = data != 0;
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

/*
	Memory map summary:

	@>0000-@>7fff: system ROM (can be paged out, see above).
	@>8000-@>bfff: basic ROM (can be paged out, see above).
	@>c000-@>dfff: free for future expansion? Used by 24kb cartridges?

	@>e000(r/w): VDP data
	@>e002(r/w): VDP register

	@>e100(w): enable cart and disable system ROM at base >0000??? (the system will only link to such a ROM if @>e110 is >42???)
	@>e108(w): disable cart and enable BASIC ROM at base >8000?
	@>e10c(w): enable cart and disable BASIC ROM at base >8000?
	@>e110(r): cartridges should return >42 if they have a ROM at base >0000 and they want the Tutor to boot from this ROM (with a blwp@>0000)???

	@>e200(w): sound write

	@>e600(r): handshake in from whatever device???
	@>e610(w): ???
	@>e620(w): ???
	@>e680(w): handshake out to this device???

	@>e810(w): parallel port data bus
	@>e820(r): parallel port busy input
	@>e840(w): parallel port strobe output

	@>ee00-@>eee0(w): tape interface (see above)

	@>f000-@>f0fb: tms9995 internal RAM 1
	@>fffa-@>fffb: tms9995 internal decrementer
	@>f000-@>f0fb: tms9995 internal RAM 2
*/

/*static WRITE_HANDLER(test_w)
{
	switch (offset)
	{
	default:
		logerror("unmapped write %d %d\n", offset, data);
		break;
	}
}*/

static ADDRESS_MAP_START(tutor_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x7fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)	/*system ROM*/
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_ROM)	/*BASIC ROM & cartridge ROM*/
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)	/*free for expansion, or cartridge ROM?*/

	AM_RANGE(0xe000, 0xe000) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)	/*VDP data*/
	AM_RANGE(0xe002, 0xe002) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)/*VDP status*/
	AM_RANGE(0xe100, 0xe1ff) AM_READWRITE(tutor_mapper_r, tutor_mapper_w)	/*cartridge mapper*/
	AM_RANGE(0xe200, 0xe200) AM_READWRITE(MRA8_NOP, SN76496_0_w)			/*sound chip*/
	AM_RANGE(0xe800, 0xe8ff) AM_READWRITE(tutor_printer_r, tutor_printer_w)	/*printer*/
	AM_RANGE(0xee00, 0xeeff) AM_READWRITE(MRA8_NOP, tutor_cassette_w)		/*cassette interface*/

	AM_RANGE(0xf000, 0xffff) AM_READWRITE(MRA8_NOP, MWA8_NOP)	/*free for expansion (and internal processor RAM)*/

ADDRESS_MAP_END

/*
	CRU map summary:

	>1ee0->1efe: tms9995 flag register
	>1fda: tms9995 MID flag

	>ec00->ec7e(r): keyboard interface
	>ed00(r): tape input
*/

static ADDRESS_MAP_START(tutor_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0xec0, 0xec7) AM_READ(read_keyboard)			/*keyboard interface*/
	AM_RANGE(0xed0, 0xed0) AM_READ(tutor_cassette_r)		/*cassette interface*/

ADDRESS_MAP_END

/*static ADDRESS_MAP_START(tutor_writecru, ADDRESS_SPACE_IO, 8)


ADDRESS_MAP_END*/



/* tutor keyboard: 56 keys */
INPUT_PORTS_START(tutor)

	PORT_START    /* col 0 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",  KEYCODE_1,     IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"", KEYCODE_2,     IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q",    KEYCODE_Q,     IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W",    KEYCODE_W,     IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A",    KEYCODE_A,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S",    KEYCODE_S,     IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z",    KEYCODE_Z,     IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X",    KEYCODE_X,     IP_JOY_NONE)

	PORT_START    /* col 1 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",  KEYCODE_3,     IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",  KEYCODE_4,     IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E",    KEYCODE_E,     IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R",    KEYCODE_R,     IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D",    KEYCODE_D,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F",    KEYCODE_F,     IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C",    KEYCODE_C,     IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V",    KEYCODE_V,     IP_JOY_NONE)

	PORT_START    /* col 2 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",  KEYCODE_5,     IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &",  KEYCODE_6,     IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T",    KEYCODE_T,     IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y",    KEYCODE_Y,     IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G",    KEYCODE_G,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H",    KEYCODE_H,     IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B",    KEYCODE_B,     IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N",    KEYCODE_N,     IP_JOY_NONE)

	PORT_START    /* col 3 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 \\", KEYCODE_7,     IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (",  KEYCODE_8,     IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 )",  KEYCODE_9,     IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U",    KEYCODE_U,     IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I",    KEYCODE_I,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J",    KEYCODE_J,     IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K",    KEYCODE_K,     IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M",    KEYCODE_M,     IP_JOY_NONE)

	PORT_START    /* col 4 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 =",  KEYCODE_0,     IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- |",  KEYCODE_MINUS, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O",    KEYCODE_O,     IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P",    KEYCODE_P,     IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L",    KEYCODE_L,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; +",  KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",  KEYCODE_COMMA, IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",  KEYCODE_STOP,  IP_JOY_NONE)

		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1)

	PORT_START    /* col 5 */
		/* Unused? */
		PORT_BITX(0x03, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ ^",  KEYCODE_EQUALS,    IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "_ @",  KEYCODE_BACKSLASH, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": *",  KEYCODE_QUOTE,     IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",  KEYCODE_OPENBRACE, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",  KEYCODE_SLASH,     IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",  KEYCODE_CLOSEBRACE,IP_JOY_NONE)

		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)

	PORT_START    /* col 6 */
		/* Unused? */
		PORT_BITX(0x21, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LOCK", KEYCODE_CAPSLOCK,IP_JOY_NONE)
		/* only one shift key located on the left, but we support both for
		emulation to be friendlier */
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT",KEYCODE_LSHIFT,KEYCODE_RSHIFT)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MON",  KEYCODE_F1,    IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RT",   KEYCODE_ENTER, IP_JOY_NONE)

		PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MOD",  KEYCODE_F2,    IP_JOY_NONE)
		PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(space)",KEYCODE_SPACE, IP_JOY_NONE)

	PORT_START    /* col 7 */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(left)", KEYCODE_LEFT,    IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(up)",   KEYCODE_UP, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(down)", KEYCODE_DOWN,  IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(right)",KEYCODE_RIGHT,  IP_JOY_NONE)

		/* Unused? */
		PORT_BITX(0xf0, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)


INPUT_PORTS_END


static struct tms9995reset_param tutor_processor_config =
{
#if 0
	REGION_CPU1,/* region for processor RAM */
	0xf000,     /* offset : this area is unused in our region, and matches the processor address */
	0xf0fc,		/* offset for the LOAD vector */
	1,          /* use fast IDLE */
#endif
	1,			/* enable automatic wait state generation */
	NULL		/* no IDLE callback */
};

static const TMS9928a_interface tms9929a_interface =
{
	TMS9929A,
	0x4000,
	/*tms9901_set_int2*/NULL
};

/*
	SN76489 or SN76496 sound chip(?)
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. connected to the TMS9918A CPUCLK pin? */
	{ 75 }			/* Volume.  I don't know the best value. */
};

/*
	1 tape units
*/
static struct Wave_interface tape_input_intf =
{
	1,
	{
		20			/* Volume */
	}
};

static MACHINE_DRIVER_START(tutor)

	/* basic machine hardware */
	/* TMS9995 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10700000)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(tutor_processor_config)
	MDRV_CPU_PROGRAM_MAP(tutor_memmap, 0)
	MDRV_CPU_IO_MAP(tutor_readcru, /*tutor_writecru*/0)
	MDRV_CPU_VBLANK_INT(tutor_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( tutor )
	/*MDRV_MACHINE_STOP( tutor )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9929a_interface )

	/* sound */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(tutor)
	/*CPU memory space*/
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("tutor1.bin", 0x0000, 0x8000, CRC(702c38ba))      /* system ROM */
	ROM_LOAD("tutor2.bin", 0x8000, 0x4000, CRC(05f228f5))      /* BASIC ROM */
ROM_END

SYSTEM_CONFIG_START(tutor)

	/* cartridge port is not emulated */
	CONFIG_DEVICE_CARTSLOT_OPT(1,	"",	NULL,	NULL,	device_load_tutor_cart,	device_unload_tutor_cart,	NULL,	NULL)
	CONFIG_DEVICE_CASSETTE	(1,		NULL)
	CONFIG_DEVICE_LEGACY	(IO_PARALLEL,	1, "\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_WRITE,	NULL,	NULL,	device_load_tutor_printer,	device_unload_tutor_printer,		NULL)

SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT		COMPAT	MACHINE		INPUT	INIT	CONFIG		COMPANY		FULLNAME */
COMP(	1983?,	tutor,	0,			0,		tutor,		tutor,	NULL,	tutor,		"Tomy",		"Tomy Tutor" )
