/*
	SNUG SGCPU (a.k.a. 99/4p) system (preliminary)

	SNUG 99/4p ("peripheral box", since the system is a card to be inserted in
	the peripheral box, instead of a self contained console), a.k.a. SGCPU
	("Second Generation CPU", which was originally the name used in TI
	documentation to refer to either (or both) TI99/5 and TI99/8 projects),
	is a reimplementation of the old ti99/4a console.

	It was designed by Michael Becker for the hardware part and Harald Glaab for
	the software part.  It has no relationship with TI.

	The card is architectured around a 16-bit bus (vs. an 8-bit bus in every other
	TI99 system).  It includes 64kb of ROM, including a GPL interpreter, an internal
	DSR ROM which contains system-specific code, and part of the TI extended Basic
	interpreter, and up to 1Mbyte of RAM.  It still includes a 16-bit to 8-bit multiplexer
	in order to support extension cards designed for TI99/4a, but it can support 16-bit
	cards, too.  It does not include GROMs, video or sound: instead, it relies on the HSGPL
	and EVPC cards to do the job.

	TODO:
	* write support for HSGPL, as it is actually required.
	* finish and test support for EVPC
*/

#include "driver.h"
#include "vidhrdw/v9938.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "devices/basicdsk.h"

static MEMORY_READ16_START (readmem)

	{ 0x0000, 0x1fff, MRA16_BANK1 },		/*system ROM*/
	{ 0x2000, 0x2fff, MRA16_BANK3 },		/*lower 8kb of RAM extension: AMS bank 2*/
	{ 0x3000, 0x3fff, MRA16_BANK4 },		/*lower 8kb of RAM extension: AMS bank 3*/
	{ 0x4000, 0x5fff, ti99_4p_rw_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_rw_cartmem },	/*cartridge memory*/
	{ 0x8000, 0x83ff, MRA16_BANK2 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_rw_null8bits },	/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_rw_rv38 },		/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_rw_null8bits },	/*vdp write*/
	{ 0x9000, 0x93ff, ti99_rw_null8bits },	/*speech read - installed dynamically*/
	{ 0x9400, 0x97ff, ti99_rw_null8bits },	/*speech write*/
	{ 0x9800, 0x9bff, ti99_rw_rgpl },		/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_rw_null8bits },	/*GPL write*/
	{ 0xa000, 0xafff, MRA16_BANK5 },		/*upper 24kb of RAM extension: AMS bank 10*/
	{ 0xb000, 0xbfff, MRA16_BANK6 },		/*upper 24kb of RAM extension: AMS bank 11*/
	{ 0xc000, 0xcfff, MRA16_BANK7 },		/*upper 24kb of RAM extension: AMS bank 12*/
	{ 0xd000, 0xdfff, MRA16_BANK8 },		/*upper 24kb of RAM extension: AMS bank 13*/
	{ 0xe000, 0xefff, MRA16_BANK9 },		/*upper 24kb of RAM extension: AMS bank 14*/
	{ 0xf000, 0xffff, MRA16_BANK10 },		/*upper 24kb of RAM extension: AMS bank 15*/

MEMORY_END

static MEMORY_WRITE16_START (writemem)

	{ 0x0000, 0x1fff, MWA16_BANK1 },		/*system ROM*/
	{ 0x2000, 0x2fff, MWA16_BANK3 },		/*lower 8kb of RAM extension: AMS bank 2*/
	{ 0x3000, 0x3fff, MWA16_BANK4 },		/*lower 8kb of RAM extension: AMS bank 3*/
	{ 0x4000, 0x5fff, ti99_4p_ww_expansion },	/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_ww_cartmem },	/*cartridge memory (some carts include RAM or a pager chip)*/
	{ 0x8000, 0x83ff, MWA16_BANK2 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_ww_wsnd },		/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_ww_null8bits },	/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_ww_wv38 },		/*vdp write*/
	{ 0x9000, 0x93ff, ti99_ww_null8bits },	/*speech read*/
	{ 0x9400, 0x97ff, ti99_ww_null8bits },	/*speech write - installed dynamically*/
	{ 0x9800, 0x9bff, ti99_ww_null8bits },	/*GPL read*/
	{ 0x9c00, 0x9fff, ti99_ww_wgpl },		/*GPL write*/
	{ 0xa000, 0xafff, MWA16_BANK5 },		/*upper 24kb of RAM extension: AMS bank 10*/
	{ 0xb000, 0xbfff, MWA16_BANK6 },		/*upper 24kb of RAM extension: AMS bank 11*/
	{ 0xc000, 0xcfff, MWA16_BANK7 },		/*upper 24kb of RAM extension: AMS bank 12*/
	{ 0xd000, 0xdfff, MWA16_BANK8 },		/*upper 24kb of RAM extension: AMS bank 13*/
	{ 0xe000, 0xefff, MWA16_BANK9 },		/*upper 24kb of RAM extension: AMS bank 14*/
	{ 0xf000, 0xffff, MWA16_BANK10 },		/*upper 24kb of RAM extension: AMS bank 15*/

MEMORY_END

static PORT_WRITE16_START(writecru)

	{0x0000<<1, 0x01ff<<1, tms9901_0_CRU_write16},
	{0x0200<<1, 0x0fff<<1, ti99_4p_expansion_CRU_w},

PORT_END

static PORT_READ16_START(readcru)

	{0x0000<<1, 0x003f<<1, tms9901_0_CRU_read16},
	{0x0040<<1, 0x01ff<<1, ti99_4p_expansion_CRU_r},

PORT_END


/*
	Input ports, used by machine code for TI keyboard and joystick emulation.

	Since the keyboard microcontroller is not emulated, we use the TI99/4a 48-key keyboard,
	plus two optional joysticks.
*/

INPUT_PORTS_START(ti99_4p)

	PORT_START	/* config */
		PORT_BITX( config_speech_mask << config_speech_bit, 1 << config_speech_bit, IPT_DIPSWITCH_NAME, "Speech synthesis", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_BITX( config_fdc_mask << config_fdc_bit, fdc_kind_BwG << config_fdc_bit, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
		PORT_BITX( config_rs232_mask << config_rs232_bit, 1 << config_rs232_bit, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_BITX(0x88, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_RCONTROL, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
		/* TI99/4a has a second shift key which maps the same */
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_RALT, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

	PORT_START	/* col 1 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

	PORT_START	/* col 2 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 # ERASE", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)

	PORT_START	/* col 3 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)

	PORT_START	/* col 4 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)

	PORT_START	/* col 5 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A |", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

	PORT_START	/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

	PORT_START	/* one more port for Alpha line */
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END


static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }		/* end of array */
};

/*
	TMS9919 (SN76489? SN76496?) sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. connected to the TMS9918A CPUCLK pin... */
	{ 75 }			/* Volume.  I don't know the best value. */
};

static struct TMS5220interface tms5220interface =
{
	680000L,					/* 640kHz -> 8kHz output */
	50,							/* Volume.  I don't know the best value. */
	NULL,						/* no IRQ callback */
#if 1
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
#endif
};

/*
	we use a DAC to emulate "audio gate", even thought
	a) there was no DAC in an actual TI99
	b) this is a 2-level output (whereas a DAC provides a 256-level output...)
*/
static struct DACinterface aux_sound_intf =
{
	1,				/* total number of DACs */
	{
		20			/* volume for audio gate*/
	}
};

/*
	2 tape units
*/
static struct Wave_interface tape_input_intf =
{
	2,
	{
		20,			/* Volume for CS1 */
		20			/* Volume for CS2 */
	}
};



/*
	machine description.
*/
static MACHINE_DRIVER_START(ti99_4p_60hz)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readcru, writecru)
	MDRV_CPU_VBLANK_INT(ti99_4ev_hblank_interrupt, 263)	/* 262.5 in 60Hz, 312.5 in 50Hz */
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)	/* or 50Hz */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(512+32, (212+16)*2)
	MDRV_VISIBLE_AREA(0, 512+32 - 1, 0, (212+16)*2 - 1)

	/*MDRV_GFXDECODE(NULL)*/
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(v9938)
	MDRV_VIDEO_START(ti99_4ev)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(v9938)

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(DAC, aux_sound_intf)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END


ROM_START(ti99_4p)
	/*CPU memory space*/
	ROM_REGION16_BE(region_cpu1_len_4p, REGION_CPU1, 0)
	ROM_LOAD16_BYTE("sgcpu_hb.bin", 0x0000, 0x8000, 0xaa100730) /* system ROMs */
	ROM_LOAD16_BYTE("sgcpu_lb.bin", 0x0001, 0x8000, 0x2a5dc818) /* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("grom0.grm", 0x0000, /*0x6000*/0x2000, 0xc56b86a5) /* system GROMs */

	/* Used for disk DSR */
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD("disk.bin", 0x0000, 0x2000, 0x8f7df93f) /* disk DSR ROM */
	ROM_LOAD("evpcdsr.bin", 0x2000, 0x10000, 0xa062b75d) /* evpc DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x58b155f7) /* system speech ROM */
ROM_END

SYSTEM_CONFIG_START(ti99_4p)
	CONFIG_DEVICE_CASSETTE			(2,	"",			ti99_cassette_init)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(3,	"dsk\0",	ti99_floppy_init)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT   MACHINE		ÊINPUT	  INIT	   CONFIG	COMPANY		FULLNAME */
COMP( 1996, ti99_4p,  0,	   ti99_4p_60hz, ti99_4p, ti99_4p, ti99_4p,	"snug",		"TI99/4P (60 Hz only)" )
