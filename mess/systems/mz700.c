/******************************************************************************
 *	Sharp MZ700
 *
 *	system driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *	MZ700 memory map
 *
 *	0000-0FFF	1Z-013A ROM or RAM
 *	1000-CFFF	RAM
 *	D000-D7FF	videoram or RAM
 *	D800-DFFF	colorram or RAM
 *	E000-FFFF	memory mapped IO or RAM
 *
 *		xxx0	PPI8255 port A (output)
 *				bit 7	556RST (reset NE556)
 *				bit 6-4 unused
 *				bit 3-0 keyboard row demux (LS145)
 *
 *		xxx1	PPI8255 port B (input)
 *				bit 7-0 keyboard matrix code
 *
 *		xxx2	PPI8255 port C (input/output)
 *				bit 7 R -VBLANK input
 *				bit 6 R 556OUT (1.5Hz)
 *				bit 5 R RDATA from cassette
 *				bit 4 R MOTOR from cassette
 *				bit 3 W M-ON control
 *				bit 2 W INTMASK 1=enable 0=disabel clock interrupt
 *				bit 1 W WDATA to cassette
 *				bit 0 W unused
 *
 *		xxx3	PPI8255 control
 *
 *		xxx4	PIT8253 timer 0 (clock input 1,108800 MHz)
 *		xxx5	PIT8253 timer 1 (clock input 15,611 kHz)
 *		xxx6	PIT8253 timer 2 (clock input OUT1 1Hz (default))
 *		xxx7	PIT8253 control/status
 *
 *		xxx8	bit 7 R -HBLANK
 *				bit 6 R unused
 *				bit 5 R unused
 *				bit 4 R joystick JB2
 *				bit 3 R joystick JB1
 *				bit 2 R joystick JA2
 *				bit 1 R joystick JA1
 *				bit 0 R NE556 OUT (32Hz IC BJ)
 *					  W gate0 of PIT8253 (sound enable)
 *
 *	MZ800 memory map
 *
 *	0000-0FFF	ROM or RAM
 *	1000-1FFF	PCG ROM or RAM
 *	2000-7FFF	RAM
 *	8000-9FFF	videoram or RAM
 *	A000-BFFF	videoram or RAM
 *	C000-CFFF	PCG RAM or RAM
 *	D000-DFFF	videoram or RAM
 *	E000-FFFF	memory mapped IO or RAM
 *
 *****************************************************************************/

#include "includes/mz700.h"
#include "devices/cassette.h"
#include "formats/mz_cas.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

MEMORY_READ_START( readmem_mz700 )
	{ 0x00000, 0x00fff, MRA8_BANK1 },
	{ 0x01000, 0x0cfff, MRA8_RAM },
	{ 0x0d000, 0x0d7ff, MRA8_BANK6 },
	{ 0x0d800, 0x0dfff, MRA8_BANK7 },
	{ 0x0e000, 0x0ffff, MRA8_BANK8 },
#if 0 //mame37b9 traps
	{ 0x10000, 0x10fff, MRA8_ROM },
	{ 0x12000, 0x127ff, MRA8_RAM },
	{ 0x12800, 0x12fff, MRA8_RAM },
	{ 0x16000, 0x16fff, MRA8_RAM },
#endif
MEMORY_END

MEMORY_WRITE_START( writemem_mz700 )
	{ 0x00000, 0x00fff, MWA8_BANK1 },
	{ 0x01000, 0x0cfff, MWA8_RAM },
	{ 0x0d000, 0x0d7ff, MWA8_BANK6 },
	{ 0x0d800, 0x0dfff, MWA8_BANK7 },
	{ 0x0e000, 0x0ffff, MWA8_BANK8 },
#if 0
	{ 0x12000, 0x127ff, videoram_w, &videoram, &videoram_size },
	{ 0x12800, 0x12fff, colorram_w, &colorram },
	{ 0x16000, 0x16fff, pcgram_w },
#endif
MEMORY_END

PORT_READ_START( readport_mz700 )
PORT_END

PORT_WRITE_START( writeport_mz700 )
	{ 0xe0, 0xe6, mz700_bank_w },
PORT_END

MEMORY_READ_START( readmem_mz800 )
	{ 0x00000, 0x00fff, MRA8_BANK1 },
	{ 0x01000, 0x01fff, MRA8_BANK2 },
	{ 0x02000, 0x07fff, MRA8_RAM },
	{ 0x08000, 0x09fff, MRA8_BANK3 },
	{ 0x0a000, 0x0bfff, MRA8_BANK4 },
	{ 0x0c000, 0x0cfff, MRA8_BANK5 },
	{ 0x0d000, 0x0d7ff, MRA8_BANK6 },
	{ 0x0d800, 0x0dfff, MRA8_BANK7 },
	{ 0x0e000, 0x0ffff, MRA8_BANK8 },
#if 0
	{ 0x10000, 0x10fff, MRA8_ROM },
	{ 0x11000, 0x11fff, MRA8_ROM },
	{ 0x12000, 0x15fff, MRA8_RAM },
#endif
MEMORY_END

MEMORY_WRITE_START( writemem_mz800 )
	{ 0x00000, 0x00fff, MWA8_BANK1 },
	{ 0x01000, 0x01fff, MWA8_BANK2 },
	{ 0x02000, 0x07fff, MWA8_RAM },
	{ 0x08000, 0x09fff, MWA8_BANK3 },
	{ 0x0a000, 0x0bfff, MWA8_BANK4 },
	{ 0x0c000, 0x0cfff, MWA8_BANK5 },
	{ 0x0d000, 0x0d7ff, MWA8_BANK6 },
	{ 0x0d800, 0x0dfff, MWA8_BANK7 },
	{ 0x0e000, 0x0ffff, MWA8_BANK8 },
#if 0
	{ 0x10000, 0x10fff, MWA8_ROM },
	{ 0x11000, 0x11fff, MWA8_ROM },
    { 0x12000, 0x16fff, videoram_w, &videoram, &videoram_size },
	{ 0x12800, 0x12fff, colorram_w, &colorram },
#endif
MEMORY_END

PORT_READ_START( readport_mz800 )
	{ 0xce, 0xce, mz800_crtc_r },
	{ 0xd0, 0xd7, mz800_mmio_r },
	{ 0xe0, 0xe9, mz800_bank_r },
	{ 0xea, 0xea, mz800_ramdisk_r },
PORT_END

PORT_WRITE_START( writeport_mz800 )
	{ 0xcc, 0xcc, mz800_write_format_w },
	{ 0xcd, 0xcd, mz800_read_format_w },
	{ 0xce, 0xce, mz800_display_mode_w },
	{ 0xcf, 0xcf, mz800_scroll_border_w },
	{ 0xd0, 0xd7, mz800_mmio_w },
	{ 0xe0, 0xe9, mz800_bank_w },
	{ 0xea, 0xea, mz800_ramdisk_w },
	{ 0xeb, 0xeb, mz800_ramaddr_w },
	{ 0xf0, 0xf0, mz800_palette_w },
PORT_END

INPUT_PORTS_START( mz700 )
	PORT_START /* status */
	PORT_BIT(0x80, 0x80, IPT_VBLANK)
	PORT_BIT(0x7f, 0x00, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "CR",        KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, ":  *",      KEYCODE_COLON,       IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, ";  +",      KEYCODE_QUOTE,       IP_JOY_NONE )
	PORT_BIT (0x08, 0x08, IPT_UNUSED )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "Alpha",     KEYCODE_PGUP,        IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "~  ^",      KEYCODE_TILDE,       IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "Graph",     KEYCODE_PGDN,        IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "#  '",      KEYCODE_BACKSLASH2,  IP_JOY_NONE )

	PORT_START /* KEY ROW 1 */
	PORT_BIT (0x07, 0x07, IPT_UNUSED )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "] }",       KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "[ {",       KEYCODE_OPENBRACE,   IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "@  ",       KEYCODE_ASTERISK,    IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "z Z",       KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "y Y",       KEYCODE_Y,           IP_JOY_NONE )

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "x  X",      KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "w  W",      KEYCODE_W,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "v  V",      KEYCODE_V,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "u  U",      KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "t  T",      KEYCODE_T,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "s  S",      KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "r  R",      KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "q  Q",      KEYCODE_Q,           IP_JOY_NONE )

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "p  P",      KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "o  O",      KEYCODE_O,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "n  N",      KEYCODE_N,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "m  M",      KEYCODE_M,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "l  L",      KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "k  K",      KEYCODE_K,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "j  J",      KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "i  I",      KEYCODE_I,           IP_JOY_NONE )

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "h  H",      KEYCODE_H,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "g  G",      KEYCODE_G,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "f  F",      KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "e  E",      KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "d  D",      KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "c  C",      KEYCODE_C,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "b  B",      KEYCODE_B,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "a  A",      KEYCODE_A,           IP_JOY_NONE )

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "8  (",      KEYCODE_8,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "7  '",      KEYCODE_7,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "6  &",      KEYCODE_6,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "5  %",      KEYCODE_5,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "4  $",      KEYCODE_4,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "3  #",      KEYCODE_3,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "2  \"",     KEYCODE_2,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "1  !",      KEYCODE_1,           IP_JOY_NONE )

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, ".  <",      KEYCODE_STOP,        IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, ",  >",      KEYCODE_COMMA,       IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "9  )",      KEYCODE_9,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "0   ",      KEYCODE_0,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "Space",     KEYCODE_SPACE,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "-  =",      KEYCODE_EQUALS,      IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "Up-Arrow",  KEYCODE_ESC,         IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "\\  |",     KEYCODE_BACKSLASH,   IP_JOY_NONE )

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "/   ",      KEYCODE_SLASH,       IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "?   ",      KEYCODE_END,         IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "Left",      KEYCODE_LEFT,        IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "Right",     KEYCODE_RIGHT,       IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "Down",      KEYCODE_DOWN,        IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "Up",        KEYCODE_UP,          IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "Del",       KEYCODE_DEL,         IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE,   IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "Ins",       KEYCODE_INSERT,      IP_JOY_NONE )

	PORT_START /* KEY ROW 8 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "LShift",    KEYCODE_LSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "RShift",    KEYCODE_RSHIFT,      IP_JOY_NONE )
	PORT_BIT (0x3e, 0x3e, IPT_UNUSED)
    PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "LCtrl",     KEYCODE_LCONTROL,    IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "RCtrl",     KEYCODE_RCONTROL,    IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "Break",     KEYCODE_HOME,        IP_JOY_NONE )


	PORT_START /* KEY ROW 9 */
	PORT_BIT (0x07, 0x07, IPT_UNUSED)
    PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "F5",        KEYCODE_F5,          IP_JOY_NONE )
    PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "F4",        KEYCODE_F4,          IP_JOY_NONE )
    PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "F3",        KEYCODE_F3,          IP_JOY_NONE )
    PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "F2",        KEYCODE_F2,          IP_JOY_NONE )
    PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "F1",        KEYCODE_F1,          IP_JOY_NONE )

	PORT_START /* KEY ROW 10 */
	PORT_START /* joystick */
	PORT_BIT( 0x01, 0x00, IPT_UNUSED												  )
	PORT_BIT( 0x02, 0x00, IPT_JOYSTICK_UP	 | IPF_8WAY 							  )
	PORT_BIT( 0x04, 0x00, IPT_JOYSTICK_DOWN  | IPF_8WAY 							  )
	PORT_BIT( 0x08, 0x00, IPT_JOYSTICK_LEFT  | IPF_8WAY 							  )
	PORT_BIT( 0x10, 0x00, IPT_JOYSTICK_RIGHT | IPF_8WAY 							  )
INPUT_PORTS_END

static struct GfxLayout char_layout =
{
	8, 8,		/* 8 x 8 graphics */
	512,		/* 512 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout, 0, 256 },
MEMORY_END	 /* end of array */

static struct beep_interface mz700_beep_interface =
{
	1,
	{ 50 }
};


static struct Wave_interface wave_interface =
{
	1,
	{ 50 }
};
static MACHINE_DRIVER_START(mz700)

	/* basic machine hardware */
	/* Z80 CPU @ 3.5 MHz */
	MDRV_CPU_ADD(Z80, 3500000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_MEMORY(readmem_mz700, writemem_mz700)
	MDRV_CPU_PORTS(readport_mz700, writeport_mz700)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( mz700 )
	MDRV_MACHINE_STOP( mz700 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware - include overscan */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_VISIBLE_AREA(0*8, 40*8 - 1, 0*8, 25*8 - 1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*256)

	MDRV_PALETTE_INIT(mz700)
	MDRV_VIDEO_START(mz700)
	/*MDRV_VIDEO_EOF(mz700)*/
	MDRV_VIDEO_UPDATE(mz700)

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(BEEP, mz700_beep_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(mz800)

	/* basic machine hardware */
	/* Z80 CPU @ 3.5 MHz */
	MDRV_CPU_ADD(Z80, 3500000)
	MDRV_CPU_MEMORY(readmem_mz800, writemem_mz800)
	MDRV_CPU_PORTS(readport_mz800, writeport_mz800)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)

	MDRV_MACHINE_INIT( mz700 )
	MDRV_MACHINE_STOP( mz700 )

	/* video hardware - include overscan */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_VISIBLE_AREA(0*8, 40*8 - 1, 0*8, 25*8 - 1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*256)

	MDRV_PALETTE_INIT(mz700)
	MDRV_VIDEO_START(mz700)
	MDRV_VIDEO_UPDATE(mz700)

	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(BEEP, mz700_beep_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)

MACHINE_DRIVER_END



ROM_START(mz700)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("1z-013a.rom", 0x10000, 0x1000, CRC(4c6c6b7b))
	ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.int",0x00000, 0x1000, CRC(42b9e8fb))
ROM_END

ROM_START(mz700j)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("1z-013a.rom", 0x10000, 0x1000, CRC(4c6c6b7b))
	ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.jap",0x00000, 0x1000, CRC(425eedf5))
ROM_END

ROM_START(mz800)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("mz800h.rom",  0x10000, 0x2000, BAD_DUMP CRC(0c281675))
	ROM_REGION(0x10000,REGION_USER1,0)
		/* RAMDISK */
    ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.int",0x00000, 0x1000, CRC(42b9e8fb))
ROM_END

SYSTEM_CONFIG_START(mz700)
	CONFIG_DEVICE_CASSETTE(1, mz700_cassette_formats)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY      FULLNAME */
COMP( 1982, mz700,	  0,		0,		mz700,	  mz700,	mz700,	mz700,	"Sharp",     "MZ-700" )
COMP( 1982, mz700j,   mz700,	0,		mz700,	  mz700,	mz700,	mz700,	"Sharp",     "MZ-700 (Japan)" )
COMP( 1982, mz800,	  mz700,	0,		mz800,	  mz700,	mz800,	mz700,	"Sharp",     "MZ-800" )


