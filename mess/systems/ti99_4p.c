/*
	SNUG SGCPU (a.k.a. 99/4p) system (preliminary)

	This system is a reimplementation of the old ti99/4a console.  It is known
	both as the 99/4p ("peripheral box", since the system is a card to be
	inserted in the peripheral box, instead of a self contained console), and
	as the SGCPU ("Second Generation CPU", which was originally the name used
	in TI documentation to refer to either (or both) TI99/5 and TI99/8
	projects).

	The SGCPU was designed and built by the SNUG (System 99 Users Group),
	namely by Michael Becker for the hardware part and Harald Glaab for the
	software part.  It has no relationship with TI.

	The card is architectured around a 16-bit bus (vs. an 8-bit bus in every
	other TI99 system).  It includes 64kb of ROM, including a GPL interpreter,
	an internal DSR ROM which contains system-specific code, part of the TI
	extended Basic interpreter, and up to 1Mbyte of RAM.  It still includes a
	16-bit to 8-bit multiplexer in order to support extension cards designed
	for TI99/4a, but it can support 16-bit cards, too.  It does not include
	GROMs, video or sound: instead, it relies on the HSGPL and EVPC cards to
	do the job.

	TODO:
	* Test the system? Call Debug, Call XB16.
	* Implement MEM8 timings.
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/v9938.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "machine/99_ide.h"
#include "devices/basicdsk.h"

static MEMORY_READ16_START (readmem)

	{ 0x0000, 0x1fff, MRA16_BANK1 },		/*system ROM*/
	{ 0x2000, 0x2fff, MRA16_BANK3 },		/*lower 8kb of RAM extension: AMS bank 2*/
	{ 0x3000, 0x3fff, MRA16_BANK4 },		/*lower 8kb of RAM extension: AMS bank 3*/
	{ 0x4000, 0x5fff, ti99_4p_peb_r },		/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_4p_rw_cartmem },	/*cartridge space (internal or hsgpl)*/
	{ 0x8000, 0x83ff, MRA16_BANK2 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_rw_null8bits },	/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_rw_rv38 },		/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_rw_null8bits },	/*vdp write*/
	{ 0x9000, 0x93ff, ti99_rw_null8bits },	/*speech read - installed dynamically*/
	{ 0x9400, 0x97ff, ti99_rw_null8bits },	/*speech write*/
	{ 0x9800, 0x98ff, ti99_4p_rw_rgpl },	/*GPL read*/
	{ 0x9900, 0x9bff, MRA16_BANK11 },		/*extra RAM for debugger*/
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
	{ 0x4000, 0x5fff, ti99_4p_peb_w },		/*DSR ROM space*/
	{ 0x6000, 0x7fff, ti99_4p_ww_cartmem },	/*cartridge space (internal or hsgpl)*/
	{ 0x8000, 0x83ff, MWA16_BANK2 },		/*RAM PAD*/
	{ 0x8400, 0x87ff, ti99_ww_wsnd },		/*soundchip write*/
	{ 0x8800, 0x8bff, ti99_ww_null8bits },	/*vdp read*/
	{ 0x8C00, 0x8fff, ti99_ww_wv38 },		/*vdp write*/
	{ 0x9000, 0x93ff, ti99_ww_null8bits },	/*speech read*/
	{ 0x9400, 0x97ff, ti99_ww_null8bits },	/*speech write - installed dynamically*/
	{ 0x9800, 0x98ff, ti99_ww_null8bits },	/*GPL read*/
	{ 0x9900, 0x9bff, MWA16_BANK11 },		/*extra RAM for debugger*/
	{ 0x9c00, 0x9fff, ti99_4p_ww_wgpl },	/*GPL write*/
	{ 0xa000, 0xafff, MWA16_BANK5 },		/*upper 24kb of RAM extension: AMS bank 10*/
	{ 0xb000, 0xbfff, MWA16_BANK6 },		/*upper 24kb of RAM extension: AMS bank 11*/
	{ 0xc000, 0xcfff, MWA16_BANK7 },		/*upper 24kb of RAM extension: AMS bank 12*/
	{ 0xd000, 0xdfff, MWA16_BANK8 },		/*upper 24kb of RAM extension: AMS bank 13*/
	{ 0xe000, 0xefff, MWA16_BANK9 },		/*upper 24kb of RAM extension: AMS bank 14*/
	{ 0xf000, 0xffff, MWA16_BANK10 },		/*upper 24kb of RAM extension: AMS bank 15*/

MEMORY_END

static PORT_WRITE16_START(writecru)

	{0x0000<<1, (0x01ff<<1) + 1, tms9901_0_CRU_write16},
	{0x0200<<1, (0x0fff<<1) + 1, ti99_4p_peb_CRU_w},

PORT_END

static PORT_READ16_START(readcru)

	{0x0000<<1, (0x003f<<1) + 1, tms9901_0_CRU_read16},
	{0x0040<<1, (0x01ff<<1) + 1, ti99_4p_peb_CRU_r},

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
		PORT_BITX( config_fdc_mask << config_fdc_bit, fdc_kind_hfdc << config_fdc_bit, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments SD" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
			PORT_DIPSETTING( fdc_kind_hfdc << config_fdc_bit, "Myarc's HFDC" )
		/*PORT_BITX( config_ide_mask << config_ide_bit, 1 << config_ide_bit, IPT_DIPSWITCH_NAME, "Nouspickel's IDE card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )*/
		PORT_BITX( config_rs232_mask << config_rs232_bit, /*1 << config_rs232_bit*/0, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )

	PORT_START	/* col 0 */
		PORT_BITX(0x0088, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		/* The original control key is located on the left, but we accept the
		right control key as well */
		PORT_BITX(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE,		UCHAR_SHIFT_1)
		/* TI99/4a has a second shift key which maps the same */
		PORT_KEY1(0x0020, IP_ACTIVE_LOW, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE,		UCHAR_SHIFT_1)
		/* The original control key is located on the right, but we accept the
		left function key as well */
		PORT_KEY1(0x0010, IP_ACTIVE_LOW, "FCTN", KEYCODE_RALT, KEYCODE_LALT,		UCHAR_SHIFT_2)
		PORT_KEY1(0x0004, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, IP_JOY_NONE,		13)
		PORT_KEY1(0x0002, IP_ACTIVE_LOW, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE,		' ')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE,	'=',	'+')
				/* col 1 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE,		'x',	'X')
		PORT_KEY3(0x4000, IP_ACTIVE_LOW, "w W ~", KEYCODE_W, IP_JOY_NONE,			'w',	'W',	'~')
		PORT_KEY2(0x2000, IP_ACTIVE_LOW, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE,		's',	'S')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW, "2 @ INS", KEYCODE_2, IP_JOY_NONE,			'2',	'@')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "9 ( BACK", KEYCODE_9, IP_JOY_NONE,		'9',	'(')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "o O '", KEYCODE_O, IP_JOY_NONE,			'o',	'O',	'\'')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "l L", KEYCODE_L, IP_JOY_NONE,				'l',	'L')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, ". >", KEYCODE_STOP, IP_JOY_NONE,			'.',	'>')

	PORT_START	/* col 2 */
		PORT_KEY3(0x0080, IP_ACTIVE_LOW, "c C `", KEYCODE_C, IP_JOY_NONE,			'c',	'C',	'`')
		PORT_KEY2(0x0040, IP_ACTIVE_LOW, "e E (UP)", KEYCODE_E, IP_JOY_NONE,		'e',	'E')
		PORT_KEY2(0x0020, IP_ACTIVE_LOW, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE,		'd',	'D')
		PORT_KEY2(0x0010, IP_ACTIVE_LOW, "3 # ERASE", KEYCODE_3, IP_JOY_NONE,		'3',	'#')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "8 * REDO", KEYCODE_8, IP_JOY_NONE,		'8',	'*')
		PORT_KEY3(0x0004, IP_ACTIVE_LOW, "i I ?", KEYCODE_I, IP_JOY_NONE,			'i',	'I',	'?')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "k K", KEYCODE_K, IP_JOY_NONE,				'k',	'K')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, ", <", KEYCODE_COMMA, IP_JOY_NONE,			',',	'<')
				/* col 3 */
		PORT_KEY2(0x8000, IP_ACTIVE_LOW, "v V", KEYCODE_V, IP_JOY_NONE,				'v',	'V')
		PORT_KEY3(0x4000, IP_ACTIVE_LOW, "r R [", KEYCODE_R, IP_JOY_NONE,			'r',	'R',	'[')
		PORT_KEY3(0x2000, IP_ACTIVE_LOW, "f F {", KEYCODE_F, IP_JOY_NONE,			'f',	'F',	'{')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE,		'4',	'$')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "7 & AID", KEYCODE_7, IP_JOY_NONE,			'7',	'&')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "u U _", KEYCODE_U, IP_JOY_NONE,			'u',	'U',	'_')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "j J", KEYCODE_J, IP_JOY_NONE,				'j',	'J')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, "m M", KEYCODE_M, IP_JOY_NONE,				'm',	'M')

	PORT_START	/* col 4 */
		PORT_KEY2(0x0080, IP_ACTIVE_LOW, "b B", KEYCODE_B, IP_JOY_NONE,				'b',	'B')
		PORT_KEY3(0x0040, IP_ACTIVE_LOW, "t T ]", KEYCODE_T, IP_JOY_NONE,			't',	'T',	']')
		PORT_KEY3(0x0020, IP_ACTIVE_LOW, "g G }", KEYCODE_G, IP_JOY_NONE,			'g',	'G',	'}')
		PORT_KEY2(0x0010, IP_ACTIVE_LOW, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE,		'5',	'%')
		PORT_KEY2(0x0008, IP_ACTIVE_LOW, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE,		'6',	'^')
		PORT_KEY2(0x0004, IP_ACTIVE_LOW, "y Y", KEYCODE_Y, IP_JOY_NONE,				'y',	'Y')
		PORT_KEY2(0x0002, IP_ACTIVE_LOW, "h H", KEYCODE_H, IP_JOY_NONE,				'h',	'H')
		PORT_KEY2(0x0001, IP_ACTIVE_LOW, "n N", KEYCODE_N, IP_JOY_NONE,				'n',	'N')
				/* col 5 */
		PORT_KEY3(0x8000, IP_ACTIVE_LOW, "z Z \\", KEYCODE_Z, IP_JOY_NONE,			'z',	'Z',	'\\')
		PORT_KEY2(0x4000, IP_ACTIVE_LOW, "q Q", KEYCODE_Q, IP_JOY_NONE,				'q',	'Q')
		PORT_KEY3(0x2000, IP_ACTIVE_LOW, "a A |", KEYCODE_A, IP_JOY_NONE,			'a',	'A',	'|')
		PORT_KEY2(0x1000, IP_ACTIVE_LOW,  "1 ! DEL", KEYCODE_1, IP_JOY_NONE,		'1',	'!')
		PORT_KEY2(0x0800, IP_ACTIVE_LOW, "0 )", KEYCODE_0, IP_JOY_NONE,				'0',	')')
		PORT_KEY3(0x0400, IP_ACTIVE_LOW, "p P \"", KEYCODE_P, IP_JOY_NONE,			'p',	'P',	'\"')
		PORT_KEY2(0x0200, IP_ACTIVE_LOW, "; :", KEYCODE_COLON, IP_JOY_NONE,			';',	':')
		PORT_KEY2(0x0100, IP_ACTIVE_LOW, "/ -", KEYCODE_SLASH, IP_JOY_NONE,			'/',	'-')

	PORT_START	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BITX(0x00E0, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)
			/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BITX(0xE000, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

	PORT_START	/* one more port for Alpha line */
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END


static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }		/* end of array */
};

/*
	SN76496(?) sound chip parameters.
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
	ROM_LOAD16_BYTE("sgcpu_hb.bin", 0x0000, 0x8000, CRC(aa100730)) /* system ROMs */
	ROM_LOAD16_BYTE("sgcpu_lb.bin", 0x0001, 0x8000, CRC(2a5dc818)) /* system ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f)) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb)) /* TI rs232 DSR ROM */
	ROM_LOAD("evpcdsr.bin", offset_evpc_dsr, 0x10000, CRC(a062b75d)) /* evpc DSR ROM */

	/* HSGPL memory space */
	ROM_REGION(region_hsgpl_len, region_hsgpl, 0)

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7)) /* system speech ROM */
ROM_END

SYSTEM_CONFIG_START(ti99_4p)
	CONFIG_DEVICE_CASSETTE			(2, "",												device_load_ti99_cassette)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",										device_load_ti99_floppy)
	CONFIG_DEVICE_LEGACY			(IO_HARDDISK, 	4, "hd\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_OR_READ,device_init_ti99_hd, NULL, device_load_ti99_hd, device_unload_ti99_hd, NULL)
	CONFIG_DEVICE_LEGACY			(IO_PARALLEL,	1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_4_pio,	device_unload_ti99_4_pio,		NULL)
	CONFIG_DEVICE_LEGACY			(IO_SERIAL,		1, "",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_4_rs232,	device_unload_ti99_4_rs232,	NULL)
	/*CONFIG_DEVICE_LEGACY			(IO_QUICKLOAD,	1, "\0",	DEVICE_LOAD_RESETS_CPU,		OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_hsgpl,		device_unload_ti99_hsgpl,	NULL)*/
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT   COMPAT	MACHINE		ÊINPUT	  INIT	   CONFIG	COMPANY		FULLNAME */
COMP( 1996, ti99_4p,  0,	   0,		ti99_4p_60hz, ti99_4p, ti99_4p, ti99_4p,"snug",		"SGCPU (a.k.a. 99/4P)" )
