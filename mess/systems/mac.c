/*
 *	Mac Plus & 512ke emulation
 *
 *	Nate Woods
 *
 *
 *		0x000000 - 0x3fffff 	RAM/ROM (switches based on overlay)
 *		0x400000 - 0x4fffff 	ROM
 *		0x580000 - 0x5fffff 	5380 NCR/Symbios SCSI peripherals chip (Mac Plus only)
 *		0x600000 - 0x6fffff 	RAM
 *		0x800000 - 0x9fffff 	Zilog 8530 SCC (Serial Control Chip) Read
 *		0xa00000 - 0xbfffff 	Zilog 8530 SCC (Serial Control Chip) Write
 *		0xc00000 - 0xdfffff 	IWM (Integrated Woz Machine; floppy)
 *		0xe80000 - 0xefffff 	Rockwell 6522 VIA
 *		0xf00000 - 0xffffef 	??? (the ROM appears to be accessing here)
 *		0xfffff0 - 0xffffff 	Auto Vector
 *
 *
 *	Interrupts:
 *		M68K:
 *			Level 1 from VIA
 *			Level 2 from SCC
 *			Level 4 : Interrupt switch (not implemented)
 *
 *		VIA:
 *			CA1 from VBLANK
 *			CA2 from 1 Hz clock (RTC)
 *			CB1 from Keyboard Clock
 *			CB2 from Keyboard Data
 *			SR	from Keyboard Data Ready
 *
 *		SCC:
 *			PB_EXT	from mouse Y circuitry
 *			PA_EXT	from mouse X circuitry
 *
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6522via.h"
#include "includes/mac.h"
#include "videomap.h"


static MEMORY_READ16_START (mac512ke_readmem)

/*	{ 0x000000, 0x3fffff, MRA16_BANK1 },*/	/* ram/rom */
	{ 0x000000, 0x01ffff, /*MRA16_NOP*/MRA16_RAM }, /* ram/rom */
	{ 0x020000, 0x07ffff, /*MRA16_NOP*/MRA16_RAM }, /* ram/rom */
	{ 0x080000, 0x3fffff, /*MRA16_NOP*/MRA16_RAM }, /* ram/rom */
/*	{ 0x400000, 0x5fffff, MRA16_BANK3 },*/ /* rom */
/* for some reason, the system will not work without the next line */
	{ 0x400000, 0x41ffff, MRA16_BANK3 }, /* rom */
	{ 0x420000, 0x5fffff, /*MRA16_NOP*/MRA16_RAM }, /* rom */
	{ 0x600000, 0x6fffff, /*MRA16_NOP*/MRA16_RAM }, /* ram */
	{ 0x800000, 0x9fffff, mac_scc_r },
	{ 0xc00000, 0xdfffff, mac_iwm_r },
	{ 0xe80000, 0xefffff, mac_via_r },
	{ 0xfffff0, 0xffffff, mac_autovector_r },

MEMORY_END

static MEMORY_WRITE16_START (mac512ke_writemem)

/*	{ 0x000000, 0x3fffff, MWA16_BANK1 },*/ /* ram/rom */
	{ 0x000000, 0x01ffff, /*MWA16_NOP*/MWA16_RAM }, /* ram/rom */
	{ 0x020000, 0x07ffff, /*MWA16_NOP*/MWA16_RAM }, /* ram/rom */
	{ 0x080000, 0x3fffff, /*MWA16_NOP*/MWA16_RAM }, /* ram/rom */
	{ 0x400000, 0x5fffff, MWA16_ROM },	/* rom */

	{ 0x600000, 0x6fffff, /*MWA16_NOP*/MWA16_RAM }, /* ram */
	{ 0xa00000, 0xbfffff, mac_scc_w },
	{ 0xc00000, 0xdfffff, mac_iwm_w },
	{ 0xe80000, 0xefffff, mac_via_w },
	{ 0xfffff0, 0xffffff, mac_autovector_w },

MEMORY_END

static MEMORY_READ16_START (macplus_readmem)

/*	{ 0x000000, 0x3fffff, MRA16_BANK1 },*/	/* ram/rom */
	{ 0x400000, 0x41ffff, MRA16_BANK3 }, /* rom */
	{ 0x420000, 0x43ffff, MRA16_BANK4 }, /* rom - mirror */
	{ 0x440000, 0x45ffff, MRA16_BANK5 }, /* rom - mirror */
	{ 0x460000, 0x47ffff, MRA16_BANK6 }, /* rom - mirror */
	{ 0x480000, 0x49ffff, MRA16_BANK7 }, /* rom - mirror */
	{ 0x4a0000, 0x4bffff, MRA16_BANK8 }, /* rom - mirror */
	{ 0x4c0000, 0x4dffff, MRA16_BANK9 }, /* rom - mirror */
	{ 0x4e0000, 0x4fffff, MRA16_BANK10 }, /* rom - mirror */
	{ 0x580000, 0x5fffff, macplus_scsi_r },
/*	{ 0x600000, 0x6fffff, MRA16_BANK2 },*/	/* ram */
	{ 0x800000, 0x9fffff, mac_scc_r },
	{ 0xc00000, 0xdfffff, mac_iwm_r },
	{ 0xe80000, 0xefffff, mac_via_r },
	{ 0xfffff0, 0xffffff, mac_autovector_r },

MEMORY_END

static MEMORY_WRITE16_START (macplus_writemem)

/*	{ 0x000000, 0x3fffff, MWA16_BANK1 },*/ /* ram/rom */
	{ 0x400000, 0x4fffff, MWA16_ROM },
	{ 0x580000, 0x5fffff, macplus_scsi_w },
/*	{ 0x600000, 0x6fffff, MWA16_BANK2 },*/ /* ram */
	{ 0xa00000, 0xbfffff, mac_scc_w },
	{ 0xc00000, 0xdfffff, mac_iwm_w },
	{ 0xe80000, 0xefffff, mac_via_w },
	{ 0xfffff0, 0xffffff, mac_autovector_w },

MEMORY_END

static PALETTE_INIT( mac )
{
	palette_set_color(0, 0xff, 0xff, 0xff);
	palette_set_color(1, 0x00, 0x00, 0x00);
}

static struct CustomSound_interface custom_interface =
{
	mac_sh_start,
	mac_sh_stop,
	mac_sh_update
};

static MACHINE_DRIVER_START( mac512ke )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 7833600)        /* 7.8336 Mhz */
	MDRV_CPU_MEMORY(mac512ke_readmem,mac512ke_writemem)
	MDRV_CPU_VBLANK_INT(mac_interrupt, 370)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( mac )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(512, 342)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 342-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT(mac)

	MDRV_VIDEO_START(mac)
	MDRV_VIDEO_UPDATE(videomap)

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, custom_interface)

	MDRV_NVRAM_HANDLER(mac)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( macplus )
	MDRV_IMPORT_FROM( mac512ke )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( macplus_readmem,macplus_writemem )
MACHINE_DRIVER_END

INPUT_PORTS_START( macplus )
	PORT_START /* 0 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "mouse button", KEYCODE_NONE, IP_JOY_DEFAULT)
	/* Not yet implemented */

	PORT_START /* Mouse - X AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - Y AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	/* R Nabet 000531 : pseudo-input ports with keyboard layout */
	/* we only define US layout for keyboard - international layout is different ! */
	/* note : 16 bits at most per port ! */

	/* main keyboard pad */

	PORT_START	/* 3 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	/* extra key on ISO : */
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)

	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)

	PORT_START	/* 4 */

	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)

	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)

	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START	/* 5 */

	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)

	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)

	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)

	PORT_START	/* 6 */

	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "`", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)

	/* keyboard Enter : */
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	/* escape : */
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	/* ??? */
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Command", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Shift", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Option", KEYCODE_LALT, IP_JOY_NONE)
	/* Control : */
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	/* keypad pseudo-keycode */
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	/* ??? */
	PORT_BITX(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	/* keypad */

	PORT_START /* 7 */

	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". (KP)", KEYCODE_DEL_PAD, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "* (KP)", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ (KP)", KEYCODE_PLUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Clear (KP)", /*KEYCODE_NUMLOCK*/KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= (KP)", /*KEYCODE_OTHER*/KEYCODE_NUMLOCK, IP_JOY_NONE)
	PORT_BITX(0x0E00, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter (KP)", KEYCODE_ENTER_PAD, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ (KP)", KEYCODE_SLASH_PAD, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- (KP)", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	PORT_START /* 8 */

	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 (KP)", KEYCODE_0_PAD, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 (KP)", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 (KP)", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 (KP)", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 (KP)", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 (KP)", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 (KP)", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 (KP)", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (KP)", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (KP)", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

	/* Arrow keys */

	PORT_START /* 9 */

	PORT_BITX(0x0003, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Right Arrow", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Left Arrow", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down Arrow", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x1E00, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up Arrow", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0xC000, IP_ACTIVE_HIGH, IPT_UNUSED, DEF_STR( Unused ), KEYCODE_NONE, IP_JOY_NONE)

INPUT_PORTS_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

#if 1

ROM_START( mac512ke )
	ROM_REGION(0x420000,REGION_CPU1,0) /* for ram, etc */
	ROM_LOAD16_WORD( "macplus.rom",  0x400000, 0x20000, CRC(b2102e8e))
ROM_END

#else

#define rom_mac512ke rom_macplus

#endif

ROM_START( macplus )
	ROM_REGION(0x420000,REGION_CPU1,0) /* for ram, etc */
	ROM_LOAD16_WORD( "macplus.rom",  0x400000, 0x20000, CRC(b2102e8e))
ROM_END

SYSTEM_CONFIG_START(macplus)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2, "dsk\0img\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_OR_READ, NULL, NULL, mac_floppy_load, mac_floppy_unload, NULL)
	/* MacPlus should eventually support hard disks, possibly CD-ROMs, etc. */
SYSTEM_CONFIG_END

/*	   YEAR		NAME	  PARENT	COMPAT	MACHINE   INPUT		INIT		CONFIG		COMPANY				FULLNAME */
/*COMPX( 1984,	mac128k,  0, 		0,		mac128k,  macplus,	mac128k,	macplus,	"Apple Computer",	"Macintosh 128k",  0 )
COMPX( 1984,	mac512k,  mac128k,	0,		mac128k,  macplus,  mac512k,	macplus,	"Apple Computer",	"Macintosh 512k",  0 )*/
COMPX( 1986,	mac512ke, macplus,  0,		mac512ke, macplus,  mac512ke,	macplus,	"Apple Computer",	"Macintosh 512ke", 0 )
COMPX( 1986,	macplus,  0,		0,		macplus,  macplus,  macplus,	macplus,	"Apple Computer",	"Macintosh Plus",  0 )

#if 0

/* Early Mac2 driver - does not work at all, but enabled me to disassemble the ROMs */

static MEMORY_READ16_START (mac2_readmem)

	{ 0x00000000, 0x007fffff, MRA_RAM },	/* ram */
	{ 0x00800000, 0x008fffff, MRA_ROM },	/* rom */
	{ 0x00900000, 0x00ffffff, MRA_NOP },

MEMORY_END

static MEMORY_WRITE16_START (mac2_writemem)

	{ 0x00000000, 0x007fffff, MWA_RAM },	/* ram */
	{ 0x00800000, 0x008fffff, MWA_ROM },	/* rom */
	{ 0x00900000, 0x00ffffff, MWA_NOP },

MEMORY_END

static void mac2_init_machine( void )
{
	memset(memory_region(REGION_CPU1), 0, 0x800000);
}


static struct MachineDriver machine_driver_mac2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68020,
			16000000,			/* +/- 16 Mhz */
			mac2_readmem,mac2_writemem,0,0,
			0,0,
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	mac2_init_machine,
	0,

	/* video hardware */
	640, 480, /* screen width, screen height */
	{ 0, 640-1, 0, 480-1 }, 		/* visible_area */

	0,					/* graphics decode info */
	2, 2,						/* number of colors, colortable size */
	mac_init_palette,				/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	mac_vh_start,
	mac_vh_stop,
	mac_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{

	},

	mac_nvram_handler
};

//#define input_ports_mac2 NULL

INPUT_PORTS_START( mac2 )

INPUT_PORTS_END

ROM_START( mac2 )
	ROM_REGION(0x00900000,REGION_CPU1,0) /* for ram, etc */
	ROM_LOAD_WIDE( "256k.rom",  0x800000, 0x40000, CRC(00000000))
ROM_END

COMPX( 1987, mac2,	   0,		 mac2,	   mac2,	 0/*mac2*/,  "Apple Computer",    "Macintosh II",  GAME_NOT_WORKING )

#endif

