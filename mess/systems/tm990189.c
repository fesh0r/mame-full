/*
	Experimental tm990/189 ("University Module") driver.

	The tm990/189 is a simple board built around a tms9980 at 2.0 MHz.
	The board features:
	* a calculator-like alphanumeric keyboard, a 10-digit 8-segment display,
	  a sound buzzer and 4 status LEDs
	* a 4kb ROM socket (0x3000-0x3fff), and a 2kb ROM socket (0x0800-0x0fff)
	* 1kb of RAM expandable to 2kb (0x0000-0x03ff and 0x0400-0x07ff)
	* a tms9901 controlling a custom parallel I/O port (available for
	  expansion)
	* an optional on-board serial interface (either TTY or RS232): TI ROMs
	  support a terminal connected to this port
	* an optional tape interface
	* an optional bus extension port for adding additionnal custom devices (TI
	  sold a video controller expansion built around a tms9918, which was
	  supported by University Basic)

	One tms9901 is set up so that it can trigger interrupts on the tms9980.

	TI sold two ROM sets for this machine: a monitor and assembler ("UNIBUG",
	packaged as one 4kb EPROM) and a Basic interpreter ("University BASIC",
	packaged as a 4kb and a 2kb EPROM).  Users could burn and install custom
	ROM sets, too.

	The board was sold to university to learn microprocessor system design.
	A few hobbyists may have bought one of these, too.  This board can be
	used as a cheap TMS9980 development kit (much cheaper than a full TI 990/4
	development system):  you can connect peripherals to the extension bus and
	write the associated software under UNIBUG.  Of course, such a board would
	probably have been much more interesting if TI had managed to design a
	working 9900-based microcontroller design.

	Raphael Nabet 2003
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/tms9901.h"
#include "vidhrdw/generic.h"

static int load_state;
static int ic_state;

static int digitsel;
static int segment;

static void *displayena_timer;
#define displayena_duration TIME_IN_MSEC(4.5)	/* Can anyone confirm this? 74LS123 connected to C=0.1uF and R=100kOhm */
static UINT8 segment_state[10];
static UINT8 old_segment_state[10];
static UINT8 LED_state;

enum
{
	tm990_189_palette_size = 3
};
unsigned char tm990_189_palette[tm990_189_palette_size*3] =
{
	0x00,0x00,0x00,	/* black */
	0xFF,0x33,0x00	/* red for LEDs */
};

static void hold_load(void);

static void usr9901_interrupt_callback(int intreq, int ic);
static void sys9901_interrupt_callback(int intreq, int ic);

static void usr9901_led_w(int offset, int data);

static int sys9901_r0(int offset);
static void sys9901_digitsel_w(int offset, int data);
static void sys9901_segment_w(int offset, int data);
static void sys9901_dsplytrgr_w(int offset, int data);
static void sys9901_shiftlight_w(int offset, int data);
static void sys9901_spkrdrive_w(int offset, int data);
static void sys9901_tapewdata_w(int offset, int data);


/* user tms9901 setup */
static const tms9901reset_param usr9901reset_param =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INT3 | TMS9901_INT4 | TMS9901_INT5 | TMS9901_INT6,	/* only input pins whose state is always known */

	{	/* read handlers */
		NULL
		/*usr9901_r0,
		usr9901_r1,
		usr9901_r2,
		usr9901_r3*/
	},

	{	/* write handlers */
		usr9901_led_w,
		usr9901_led_w,
		usr9901_led_w,
		usr9901_led_w/*,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w*/
	},

	/* interrupt handler */
	usr9901_interrupt_callback,

	/* clock rate = 2MHz */
	2000000.
};

/* system tms9901 setup */
static const tms9901reset_param sys9901reset_param =
{
	0,	/* only input pins whose state is always known */

	{	/* read handlers */
		sys9901_r0,
		NULL,
		NULL,
		NULL
	},

	{	/* write handlers */
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_dsplytrgr_w,
		sys9901_shiftlight_w,
		sys9901_spkrdrive_w,
		sys9901_tapewdata_w
	},

	/* interrupt handler */
	sys9901_interrupt_callback,

	/* clock rate = 2MHz */
	2000000.
};

static void machine_init_tm990_189(void)
{
	displayena_timer = timer_alloc(NULL);

	hold_load();

	tms9901_init(0, &usr9901reset_param);
	tms9901_init(1, &sys9901reset_param);
	tms9901_reset(0);
	tms9901_reset(1);
}


/*
	tm990_189 video emulation.

	Has an integrated 10 digit 8-segment display.
	Supports EIA and TTY terminals, and an optional 9918 controller.
*/

static void palette_init_tm990_189(unsigned short *colortable, const unsigned char *dummy)
{
	palette_set_colors(0, tm990_189_palette, tm990_189_palette_size);

	/*memcpy(colortable, & tm990_189_colortable, sizeof(tm990_189_colortable));*/
}

static int video_start_tm990_189(void)
{
	return video_start_generic_bitmapped();
}

static VIDEO_EOF( tm990_189 )
{
	int i;

	for (i=0; i<10; i++)
		segment_state[digitsel] = 0;
}

INLINE void draw_digit(void)
{
	segment_state[digitsel] |= segment;
}

static VIDEO_UPDATE( tm990_189 )
{
	int i, j;
	static const struct{ int x, y; int w, h; } bounds[8] =
	{
		{ 1, 1, 5, 1 },
		{ 6, 2, 1, 5 },
		{ 6, 8, 1, 5 },
		{ 1,13, 5, 1 },
		{ 0, 8, 1, 5 },
		{ 0, 2, 1, 5 },
		{ 1, 7, 5, 1 },
		{ 8,12, 2, 2 },
	};

	for (i=0; i<10; i++)
	{
		old_segment_state[i] |= segment_state[i];

		for (j=0; j<8; j++)
			plot_box(tmpbitmap, i*16 + bounds[j].x, bounds[j].y, bounds[j].w, bounds[j].h,
						Machine->pens[(old_segment_state[i] & (1 << j)) ? 1 : 0]);

		old_segment_state[i] = segment_state[i];
	}

	for (i=0; i<4; i++)
	{
		plot_box(tmpbitmap, 48+4 - i*16, 16+4, 8, 8,
					Machine->pens[(LED_state & (1 << i)) ? 1 : 0]);
	}

	plot_box(tmpbitmap, 128+4, 16+4, 8, 8,
				Machine->pens[(LED_state & 0x10) ? 1 : 0]);

	plot_box(tmpbitmap, 96+4, 16+4, 8, 8,
				Machine->pens[(LED_state & 0x20) ? 1 : 0]);

	plot_box(tmpbitmap, 80+4, 16+4, 8, 8,
				Machine->pens[(LED_state & 0x40) ? 1 : 0]);


	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

/*
	tms9901 code
*/

static void field_interrupts(void)
{
	if (load_state)
		cpu_set_irq_line_and_vector(0, 0, ASSERT_LINE, 2);
	else
		cpu_set_irq_line_and_vector(0, 0, ASSERT_LINE, ic_state);
}

/*
	hold and debounce load line (emulation is inaccurate)
*/

static void clear_load(int dummy)
{
	load_state = FALSE;
	field_interrupts();
}

static void hold_load(void)
{
	load_state = TRUE;
	field_interrupts();
	timer_set(TIME_IN_MSEC(100), 0, clear_load);
}

static void usr9901_interrupt_callback(int intreq, int ic)
{
	ic_state = ic & 7;

	if (!load_state)
		field_interrupts();
}

static void usr9901_led_w(int offset, int data)
{
	if (data)
		LED_state |= (1 << offset);
	else
		LED_state &= ~(1 << offset);
}

static void sys9901_interrupt_callback(int intreq, int ic)
{
	(void)ic;

	tms9901_set_single_int(0, 5, intreq);
}

static int sys9901_r0(int offset)
{
	int reply = 0x00;

	/* keyboard read */
	if (digitsel < 9)
		reply |= readinputport(digitsel) << 1;

	/* tape input */
	/*if (...)
		reply |= 0x40*/

	return reply;
}

static void sys9901_digitsel_w(int offset, int data)
{
	if (data)
		digitsel |= 1 << offset;
	else
		digitsel &= ~ (1 << offset);
}

static void sys9901_segment_w(int offset, int data)
{
	if (data)
		segment &= ~ (1 << (offset-4));
	else
	{
		segment |= 1 << (offset-4);
		if ((timer_timeleft(displayena_timer) >= 0.) && (digitsel < 10))
			draw_digit();
	}
}

static void sys9901_dsplytrgr_w(int offset, int data)
{
	if ((!data) && (digitsel < 10))
	{
		timer_reset(displayena_timer, displayena_duration);
		draw_digit();
	}
}

static void sys9901_shiftlight_w(int offset, int data)
{
	if (data)
		LED_state |= 0x10;
	else
		LED_state &= ~0x10;
}

static void sys9901_spkrdrive_w(int offset, int data)
{
	speaker_level_w(0, data);
}

static void sys9901_tapewdata_w(int offset, int data)
{
	/*...*/
}

/*
	Memory map:

	0x0000-0x03ff: 1kb RAM (or ROM???)
	0x0400-0x07ff: 1kb onboard expansion RAM or ROM
	0x0800-0x0bff: 1kb onboard expansion RAM or ROM
	0x0c00-0x0fff: 1kb onboard expansion RAM or ROM
	0x1000-0x2fff: for offboard expansion
		Only known card: Color Video Board
			0x1000-0x17ff (R): Video board ROM 1
			0x1800-0x1fff (R): Video board ROM 2
			0x2000-0x27ff (R): tms9918 read port
				(address & 2) == 0: data port (bogus)
				(address & 2) == 1: status register (bogus)
			0x2800-0x2fff (R): joystick port
				d2: joy 2 switch
				d3: joy 2 Y (length of pulse after retrigger is proportional to axis position)
				d4: joy 2 X (length of pulse after retrigger is proportional to axis position)
				d2: joy 1 switch
				d3: joy 1 Y (length of pulse after retrigger is proportional to axis position)
				d4: joy 1 X (length of pulse after retrigger is proportional to axis position)
			0x1801-0x1fff ((address & 1) == 1) (W): joystick port retrigger
			0x2801-0x2fff ((address & 1) == 1) (W): tms9918 write port
				(address & 2) == 0: data port
				(address & 2) == 1: control register
	0x3000-0x3fff: 4kb onboard ROM
*/

static MEMORY_READ_START (tm990_189_readmem)

	{ 0x0000, 0x07ff, MRA_RAM },		/* RAM */
	{ 0x0800, 0x0fff, MRA_ROM },		/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */
	{ 0x1000, 0x2fff, MRA_NOP },		/* reserved for expansion (RAM and/or tms9918 video controller) */
	{ 0x3000, 0x3fff, MRA_ROM },		/* main ROM - unibug or university basic */

MEMORY_END

static MEMORY_WRITE_START (tm990_189_writemem)

	{ 0x0000, 0x07ff, MWA_RAM },		/* RAM */
	{ 0x0800, 0x0fff, MWA_ROM },		/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */
	{ 0x1000, 0x2fff, MWA_NOP },		/* reserved for expansion (RAM and/or tms9918 video controller) */
	{ 0x3000, 0x3fff, MWA_ROM },		/* main ROM - unibug or university basic */

MEMORY_END


/*
	CRU map

	The board features one tms9901 for keyboard and sound I/O, another tms9901
	to drive the parallel port and a few LEDs and handle tms9980 interrupts,
	and one optional tms9902 for serial I/O.

	* bits >000->1ff (R12=>000->3fe): first TMS9901: parallel port, a few LEDs,
		interrupts
		- CRU bits 1-5: UINT1* through UINT5* (jumper connectable to parallel
			port or COMINT from tms9902)
		- CRU bit 6: KBINT (interrupt request from second tms9901)
		- CRU bits 7-15: mirrors P15-P7
		- CRU bits 16-31: P0-P15 (parallel port)
			(bits 16, 17, 18 and 19 control LEDs numbered 1, 2, 3 and 4, too)
	* bits >200->3ff (R12=>400->7fe): second TMS9901: display, keyboard and
		sound
		- CRU bits 1-5: KB1*-KB5* (inputs from current keyboard row(?))
		- CRU bit 6: RDATA (tape in)
		- CRU bits 7-15: mirrors P15-P7
		- CRU bits 16-19: DIGITSELA-DIGITSELD (select current display digit and
			keyboard row(?))
		- CRU bits 20-27: SEGMENTA*-SEGMENTG* and SEGMENTP* (drives digit
			segments)
		- CRU bit 28: DSPLYTRGR* (will generate a pulse to drive the digit
			segments)
		- bit 29: SHIFTLIGHT (controls shift light)
		- bit 30: SPKRDRIVE (controls buzzer)
		- bit 31: WDATA (tape out)
	* bits >400->5ff (R12=>800->bfe): optional TMS9902?
	* bits >600->7ff (R12=>c00->ffe): reserved for expansion
	* write 0 to bits >1000->17ff: IDLE: flash IDLE LED
	* write 0 to bits >1800->1fff: RSET: not connected, but it is decoded and
		hackers can connect any device they like to this pin
	* write 1 to bits >0800->0fff: CKON: light FWD LED and activates
		DECKCONTROL* signal (part of tape interface)
	* write 1 to bits >1000->17ff: CKOF: light off FWD LED and deactivates
		DECKCONTROL* signal (part of tape interface)
	* write 1 to bits >1800->1fff: LREX: trigger load interrupt

	Keyboard map: see input ports

	Display segment designation:
		   a
		  ___
		 |   |
		f|   |b
		 |___|
		 | g |
		e|   |c
		 |___|  .p
		   d
*/

static WRITE_HANDLER(ext_instr_decode)
{
	int ext_op_ID;

	ext_op_ID = (offset >> 11) & 0x3;
	if (data)
		ext_op_ID |= 4;
	switch (ext_op_ID)
	{
	case 2: /* IDLE: handle elsewhere */
		break;

	case 3: /* RSET: not used in default board */
		break;

	case 5: /* CKON: set DECKCONTROL */
		LED_state |= 0x20;
		/* ... */
		break;

	case 6: /* CKOF: clear DECKCONTROL */
		LED_state &= ~0x20;
		/* ... */
		break;

	case 7: /* LREX: trigger LOAD */
		hold_load();
		break;
	}
}

static void idle_callback(int state)
{
	if (state)
		LED_state |= 0x40;
	else
		LED_state &= ~0x40;
}

static PORT_WRITE_START ( tm990_189_writeport )


	{ 0x000, 0x1ff, tms9901_0_CRU_write },	/* user I/O tms9901 */
	{ 0x200, 0x3ff, tms9901_1_CRU_write },	/* system I/O tms9901 */
	{ 0x400, 0x5ff, MWA_NOP },				/* optional tms9902 is not implemented */

	{0x0800,0x1fff, ext_instr_decode },		/* external instruction decoding (IDLE, RSET, CKON, CKOF, LREX) */

PORT_END

static PORT_READ_START ( tm990_189_readport )

	{ 0x00, 0x3f, tms9901_0_CRU_read },		/* user I/O tms9901 */
	{ 0x40, 0x6f, tms9901_1_CRU_read },		/* system I/O tms9901 */
	{ 0x80, 0xcf, MRA_NOP },				/* optional tms9902 is not implemented */

PORT_END

static struct Speaker_interface tm990_189_sound_interface =
{
	1,
	{ 100 },
	{ 0 },
	{ NULL }
};

static tms9980areset_param reset_params =
{
	idle_callback
};

static MACHINE_DRIVER_START(tm990_189)

	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MDRV_CPU_ADD(TMS9980, 2000000)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(reset_params)
	MDRV_CPU_MEMORY(tm990_189_readmem, tm990_189_writemem)
	MDRV_CPU_PORTS(tm990_189_readport, tm990_189_writeport)
	/*MDRV_CPU_VBLANK_INT(tm990_189_interrupt, 1)*/
	/*MDRV_CPU_PERIODIC_INT(NULL, 0)*/

	/* video hardware - we emulate a 8-segment LED display */
	MDRV_FRAMES_PER_SECOND(75)
	MDRV_VBLANK_DURATION(/*DEFAULT_REAL_60HZ_VBLANK_DURATION*/0)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( tm990_189 )
	/*MDRV_MACHINE_STOP( tm990_189 )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(750, 532)
	MDRV_VISIBLE_AREA(0, 750-1, 0, 532-1)

	/*MDRV_GFXDECODE(tm990_189_gfxdecodeinfo)*/
	MDRV_PALETTE_LENGTH(tm990_189_palette_size)
	MDRV_COLORTABLE_LENGTH(/*tm990_189_colortable_size*/0)

	MDRV_PALETTE_INIT(tm990_189)
	MDRV_VIDEO_START(tm990_189)
	/*MDRV_VIDEO_STOP(tm990_189)*/
	MDRV_VIDEO_EOF(tm990_189)
	MDRV_VIDEO_UPDATE(tm990_189)

	MDRV_SOUND_ATTRIBUTES(0)
	/* one two-level buzzer */
	MDRV_SOUND_ADD(SPEAKER, tm990_189_sound_interface)

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(tm990189)
	/*CPU memory space*/
	ROM_REGION(0x4000, REGION_CPU1,0)

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b))		
	/*ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744))*/	/* older, partial dump of university BASIC */

ROM_END

INPUT_PORTS_START(tm990_189)

	/* 45-key calculator-like alphanumeric keyboard... */

	PORT_START    /* row 0 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift",   KEYCODE_LSHIFT, KEYCODE_RSHIFT)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Sp *",    KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Ret '",   KEYCODE_ENTER, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "$ =",     KEYCODE_STOP, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <",     KEYCODE_COMMA, IP_JOY_NONE)

	PORT_START    /* row 1 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "+ (",     KEYCODE_OPENBRACE, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "- )",     KEYCODE_CLOSEBRACE, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "@ /",     KEYCODE_MINUS, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "> %",     KEYCODE_EQUALS, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 ^",     KEYCODE_0, IP_JOY_NONE)
		
	PORT_START    /* row 2 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 .",     KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 ;",     KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 :",     KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 ?",     KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 !",     KEYCODE_5, IP_JOY_NONE)
		
	PORT_START    /* row 3 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 _",     KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 \"",    KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 #",     KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (ESC)", KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A (SOH)", KEYCODE_A, IP_JOY_NONE)

	PORT_START    /* row 4 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "B (STH)", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "C (ETX)", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D (EOT)", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E (ENQ)", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "F (ACK)", KEYCODE_F, IP_JOY_NONE)

	PORT_START    /* row 5 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G (BEL)", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "H (BS)",  KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I (HT)",  KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J (LF)",  KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "K (VT)",  KEYCODE_K, IP_JOY_NONE)

	PORT_START    /* row 6 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "L (FF)",  KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "M (DEL)", KEYCODE_M, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "N (SO)",  KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "O (SI)",  KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P (DLE)", KEYCODE_P, IP_JOY_NONE)

	PORT_START    /* row 7 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q (DC1)", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "R (DC2)", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "S (DC3)", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "T (DC4)", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "U (NAK)", KEYCODE_U, IP_JOY_NONE)

	PORT_START    /* row 9 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "V <-D",   KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W (ETB)", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X (CAN)", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y (EM)",  KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z ->D",   KEYCODE_Z, IP_JOY_NONE)

INPUT_PORTS_END

SYSTEM_CONFIG_START(tm990_189)
	/* a tape interface and a rs232 interface... */
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1979,	tm990189,		0,		0,		tm990_189,	tm990_189,	0,			tm990_189,	"Texas Instruments",	"TM 990/189 (and 189-1) University Board microcomputer" )
