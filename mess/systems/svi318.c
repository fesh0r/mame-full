/*
** svi318.c : driver for Spectravideo SVI-318 and SVI-328
**
** Sean Young, 2000
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "includes/svi318.h"
#include "includes/wd179x.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cartslot.h"


static MEMORY_READ_START (readmem)
    { 0x0000, 0x7fff, MRA_BANK1 },
    { 0x8000, 0xbfff, MRA_BANK2 },
    { 0xc000, 0xffff, MRA_BANK3 },
MEMORY_END

static MEMORY_WRITE_START( writemem )
    { 0x0000, 0x7fff, svi318_writemem0 },
    { 0x8000, 0xffff, svi318_writemem1 },
MEMORY_END

static READ_HANDLER (svi318_null_r)
	{
	return 0xff;
	}

static PORT_READ_START (readport)
    { 0x00, 0x11, svi318_null_r },
    { 0x12, 0x12, svi318_printer_r },
    { 0x13, 0x2f, svi318_null_r },
#ifdef SVI_DISK
	{ 0x30, 0x30, wd179x_status_r },
	{ 0x31, 0x31, wd179x_track_r },
	{ 0x32, 0x32, wd179x_sector_r },
	{ 0x33, 0x33, wd179x_data_r },
	{ 0x34, 0x34, svi318_fdc_status_r },
#endif
    { 0x35, 0x83, svi318_null_r },
    { 0x84, 0x84, TMS9928A_vram_r },
    { 0x85, 0x85, TMS9928A_register_r },
    { 0x86, 0x8f, svi318_null_r },
	{ 0x90, 0x90, AY8910_read_port_0_r },
    { 0x91, 0x95, svi318_null_r },
    { 0x98, 0x9a, svi318_ppi_r },
    { 0x9b, 0xff, svi318_null_r },
PORT_END

static PORT_WRITE_START (writeport)
    { 0x10, 0x11, svi318_printer_w },
#ifdef SVI_DISK
	{ 0x30, 0x30, wd179x_command_w },
	{ 0x31, 0x31, wd179x_track_w },
	{ 0x32, 0x32, wd179x_sector_w },
	{ 0x33, 0x33, wd179x_data_w },
	{ 0x34, 0x34, fdc_disk_motor_w },
	{ 0x38, 0x38, fdc_density_side_w },
#endif
    { 0x80, 0x80, TMS9928A_vram_w },
    { 0x81, 0x81, TMS9928A_register_w },
	{ 0x88, 0x88, AY8910_control_port_0_w },
 	{ 0x8c, 0x8c, AY8910_write_port_0_w },
    { 0x96, 0x97, svi318_ppi_w },
PORT_END

/*

From: "Tomas Karlsson" <tomas.k@home.se>

http://www.hut.fi/~mstuomel/328/svikoko.gif

Keyboard status table

     Bit#:|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
          |     |     |     |     |     |     |     |     |
Line:     |     |     |     |     |     |     |     |     |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  0       | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  1       | "/" | "." | "=" | "," | "'" | ":" | "9" | "8" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  2       | "G" | "F" | "E" | "D" | "C" | "B" | "A" | "-" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  3       | "O" | "N" | "M" | "L" | "K" | "J" | "I" | "H" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  4       | "W" | "V" | "U" | "T" | "S" | "R" | "Q" | "P" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  5       | UP  | BS  | "]" | "\" | "[" | "Z" | "Y" | "X" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  6       |LEFT |ENTER|STOP | ESC |RGRAP|LGRAP|CTRL |SHIFT|
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  7       |DOWN | INS | CLS | F5  | F4  | F3  | F2  | F1  |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  8       |RIGHT|     |PRINT| SEL |CAPS | DEL | TAB |SPACE|
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
  9*      | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
 ---------+-----+-----+-----+-----+-----+-----+-----+-----|
 10*      | "," | "." | "/" | "*" | "-" | "+" | "9" | "8" |
 ----------------------------------------------------------

* Numcerical keypad (SVI-328 only)

*/

#define SVI_318_KEYS	\
 PORT_START /* 0 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @", KEYCODE_2, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^", KEYCODE_6, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 &", KEYCODE_7, IP_JOY_NONE)  \
  \
 PORT_START /* 1 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 *", KEYCODE_8, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (", KEYCODE_9, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ": ;", KEYCODE_COLON, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "' \"", KEYCODE_QUOTE, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "= +", KEYCODE_EQUALS, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE)  \
  \
 PORT_START /* 2 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "- _", KEYCODE_MINUS, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C", KEYCODE_C, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D", KEYCODE_D, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E", KEYCODE_E, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F", KEYCODE_F, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G", KEYCODE_G, IP_JOY_NONE)  \
  \
 PORT_START /* 3 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I", KEYCODE_I, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O", KEYCODE_O, IP_JOY_NONE)  \
  \
 PORT_START /* 4 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P", KEYCODE_P, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R", KEYCODE_R, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S", KEYCODE_S, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T", KEYCODE_T, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U", KEYCODE_U, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W", KEYCODE_W, IP_JOY_NONE)  \
  \
 PORT_START /* 5 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X", KEYCODE_X, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z", KEYCODE_Z, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "[ {", KEYCODE_OPENBRACE, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\ ~", KEYCODE_BACKSLASH, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "] }", KEYCODE_CLOSEBRACE, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)  \
  \
 PORT_START /* 6 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT GRAPH", KEYCODE_PGUP, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT GRAPH", KEYCODE_PGDN, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC", KEYCODE_ESC, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "STOP", KEYCODE_END, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE)  \
  \
 PORT_START /* 7 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3", KEYCODE_F3, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5", KEYCODE_F5, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLS", KEYCODE_HOME, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "INS", KEYCODE_INSERT, IP_JOY_NONE)  \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE)  \
  \
 PORT_START /* 8 */  \
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)  \
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)  \
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE)  \
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE)  \
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SELECT", KEYCODE_PAUSE, IP_JOY_NONE)  \
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "PRINT", KEYCODE_PRTSCR, IP_JOY_NONE)  \
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)   \
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)


INPUT_PORTS_START (svi318)

SVI_318_KEYS

 PORT_START /* 9 */
  PORT_BITX (0xff, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

 PORT_START /* 10 */
  PORT_BITX (0xff, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

 PORT_START /* 11 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)

 PORT_START /* 12 */
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)

 PORT_START /* 13 */
  PORT_DIPNAME( 0x20, 0x20, "Enforce 4 sprites/line")
  PORT_DIPSETTING( 0, DEF_STR( No ) )
  PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
  PORT_DIPNAME( 0x01, 0x00, "Bank 21 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x01, DEF_STR( On ) )
  PORT_DIPNAME( 0x02, 0x00, "Bank 22 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x02, DEF_STR( On ) )
  PORT_DIPNAME( 0x04, 0x00, "Bank 31 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x04, DEF_STR( On ) )
  PORT_DIPNAME( 0x08, 0x00, "Bank 32 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x08, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START (svi328)

SVI_318_KEYS

 PORT_START /* 9 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 0", KEYCODE_0_PAD, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 1", KEYCODE_1_PAD, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 2", KEYCODE_2_PAD, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 3", KEYCODE_3_PAD, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 4", KEYCODE_4_PAD, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 5", KEYCODE_5_PAD, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 6", KEYCODE_6_PAD, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 7", KEYCODE_7_PAD, IP_JOY_NONE)

 PORT_START /* 10 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 8", KEYCODE_8_PAD, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM 9", KEYCODE_9_PAD, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM +", KEYCODE_PLUS_PAD, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM -", KEYCODE_MINUS_PAD, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM *", KEYCODE_ASTERISK, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM /", KEYCODE_SLASH_PAD, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM .", KEYCODE_LALT, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUM ,", KEYCODE_RALT, IP_JOY_NONE)

 PORT_START /* 11 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)

 PORT_START /* 12 */
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)

 PORT_START /* 13 */
  PORT_DIPNAME( 0x20, 0x20, "Enforce 4 sprites/line")
  PORT_DIPSETTING( 0, DEF_STR( No ) )
  PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
  PORT_DIPNAME( 0x01, 0x00, "Bank 21 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x01, DEF_STR( On ) )
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
  PORT_DIPNAME( 0x04, 0x00, "Bank 31 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x04, DEF_STR( On ) )
  PORT_DIPNAME( 0x08, 0x00, "Bank 32 RAM")
  PORT_DIPSETTING( 0, DEF_STR( Off ) )
  PORT_DIPSETTING( 0x08, DEF_STR( On ) )

INPUT_PORTS_END

static struct AY8910interface ay8910_interface =
{
    1,  /* 1 chip */
    1789773,    /* 1.7897725 MHz */
    { 75 },
    { svi318_psg_port_a_r },
    { NULL },
    { NULL },
    { svi318_psg_port_b_w }
};

static struct DACinterface dac_interface =
{
    1,
    { 25 }
};

static struct Wave_interface wave_interface = {
    1,              /* number of waves */
    { 25 }           /* mixing levels */
};

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	svi318_vdp_interrupt
};

static MACHINE_DRIVER_START( svi318 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3579545)        /* 3.579545 Mhz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(svi318_interrupt,1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( svi318 )
	MDRV_MACHINE_STOP( svi318 )

    /* video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (svi318)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi100.rom", 0x0000, 0x8000, CRC(98d48655))
ROM_END

ROM_START (svi328)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi110.rom", 0x0000, 0x8000, CRC(709904e9))
ROM_END

ROM_START (svi328a)
    ROM_REGION (0x10000, REGION_CPU1,0)
    ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6))
ROM_END

SYSTEM_CONFIG_START(svi318)
	CONFIG_DEVICE_PRINTER			(1)
	CONFIG_DEVICE_CASSETTE			(1,	"cas\0",	device_load_svi318_cassette)
	CONFIG_DEVICE_CARTSLOT_OPT		(1,	"rom\0",	NULL, NULL, device_load_svi318_cart, device_unload_svi318_cart, NULL, NULL)
#ifdef SVI_DISK
	CONFIG_DEVICE_FLOPPY_BASICDSK	(2,	"dsk\0",	device_load_svi318_floppy)
#endif
SYSTEM_CONFIG_END

/*   YEAR	NAME		PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY FULLNAME */
COMP(1983,	svi318,		0,		0,		svi318,	svi318,	svi318, svi318,	"Spectravideo", "SVI-318" )
COMP(1983,	svi328,		svi318,	0,		svi318,	svi328,	svi318, svi318,	"Spectravideo", "SVI-328" )
COMP(1983,	svi328a,	svi318,	0,		svi318,	svi328,	svi318, svi318,	"Spectravideo", "SVI-328 (BASIC 1.11)" )
