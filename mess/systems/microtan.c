/******************************************************************************
 *  Microtan 65
 *
 *  system driver
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 *  Microtan65 memory map
 *
 *  range     short     description
 *  0000-01FF SYSRAM    system ram
 *                      0000-003f variables
 *                      0040-00ff basic
 *                      0100-01ff stack
 *  0200-03ff VIDEORAM  display
 *  0400-afff RAM       main memory
 *  bc00-bc01 AY8912-0  sound chip #0
 *  bc02-bc03 AY8912-1  sound chip #1
 *  bc04      SPACEINV  space invasion sound (?)
 *  bfc0-bfcf VIA6522-0 VIA 6522 #0
 *  bfd0-bfd3 SIO       serial i/o
 *  bfe0-bfef VIA6522-1 VIA 6522 #0
 *  bff0      GFX_KBD   R: chunky graphics on W: reset KBD interrupt
 *  bff1      NMI       W: start delayed NMI
 *  bff2      HEX       W: hex. keypad column
 *  bff3      KBD_GFX   R: ASCII KBD / hex. keypad row W: chunky graphics off
 *  c000-e7ff BASIC     BASIC Interpreter ROM
 *  f000-f7ff XBUG      XBUG ROM
 *  f800-ffff TANBUG    TANBUG ROM
 *
 *****************************************************************************/

#include "includes/microtan.h"
#include "devices/cassette.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

static ADDRESS_MAP_START( readmem_mtan , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x03ff) AM_READ( MRA8_RAM )
/*  { 0x0400, 0x1fff, MRA8_RAM },    */  /* TANEX 7K RAM */
/*  { 0x2000, 0xbbff, MRA8_RAM },    */  /* TANRAM 40K RAM */
    AM_RANGE( 0xbc00, 0xbc00) AM_READ( MRA8_NOP )
    AM_RANGE( 0xbc01, 0xbc01) AM_READ( AY8910_read_port_0_r )
    AM_RANGE( 0xbc02, 0xbc02) AM_READ( MRA8_NOP )
    AM_RANGE( 0xbc03, 0xbc03) AM_READ( AY8910_read_port_1_r )
    AM_RANGE( 0xbfc0, 0xbfcf) AM_READ( microtan_via_0_r )
    AM_RANGE( 0xbfd0, 0xbfd3) AM_READ( microtan_sio_r )
    AM_RANGE( 0xbfe0, 0xbfef) AM_READ( microtan_via_1_r )
    AM_RANGE( 0xbff0, 0xbfff) AM_READ( microtan_bffx_r )
    AM_RANGE( 0xc000, 0xe7ff) AM_READ( MRA8_ROM )
    AM_RANGE( 0xf000, 0xffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_mtan , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x01ff) AM_WRITE( MWA8_RAM )
    AM_RANGE( 0x0200, 0x03ff) AM_WRITE( microtan_videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size )
/*  { 0x0400, 0x1fff, MRA8_RAM },    */  /* TANEX 7K RAM */
/*  { 0x2000, 0xbbff, MRA8_RAM },    */  /* TANRAM 40K RAM */
    AM_RANGE( 0xbc00, 0xbc00) AM_WRITE( AY8910_control_port_0_w )
    AM_RANGE( 0xbc01, 0xbc01) AM_WRITE( AY8910_write_port_0_w )
    AM_RANGE( 0xbc02, 0xbc02) AM_WRITE( AY8910_control_port_1_w )
    AM_RANGE( 0xbc03, 0xbc03) AM_WRITE( AY8910_write_port_1_w )
    AM_RANGE( 0xbc04, 0xbc04) AM_WRITE( microtan_sound_w )
    AM_RANGE( 0xbfc0, 0xbfcf) AM_WRITE( microtan_via_0_w )
    AM_RANGE( 0xbfd0, 0xbfd3) AM_WRITE( microtan_sio_w )
    AM_RANGE( 0xbfe0, 0xbfef) AM_WRITE( microtan_via_1_w )
    AM_RANGE( 0xbff0, 0xbfff) AM_WRITE( microtan_bffx_w )
    AM_RANGE( 0xc000, 0xe7ff) AM_WRITE( MWA8_ROM )
    AM_RANGE( 0xf000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

INPUT_PORTS_START( microtan )
    PORT_START /* DIP switches */
    PORT_DIPNAME(0x03, 0x00, "Memory size")
    PORT_DIPSETTING(0x00, "1K" )
    PORT_DIPSETTING(0x01, "1K + 7K TANEX" )
    PORT_DIPSETTING(0x02, "1K + 7K TANEX + 40K TANRAM" )
    PORT_BIT(0xfe, 0xfe, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1  !") PORT_CODE(KEYCODE_1)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2  \"") PORT_CODE(KEYCODE_2)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3  #") PORT_CODE(KEYCODE_3)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4  $") PORT_CODE(KEYCODE_4)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5  %") PORT_CODE(KEYCODE_5)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6  &") PORT_CODE(KEYCODE_6)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7  '") PORT_CODE(KEYCODE_7)

    PORT_START /* KEY ROW 1 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("8  *") PORT_CODE(KEYCODE_8)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("9  (") PORT_CODE(KEYCODE_9)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0  )") PORT_CODE(KEYCODE_0)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("-  _") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("=  +") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("`  ~") PORT_CODE(KEYCODE_TILDE)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("BACK SPACE") PORT_CODE(KEYCODE_BACKSPACE)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)

    PORT_START /* KEY ROW 2 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("q  Q") PORT_CODE(KEYCODE_Q)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("w  W") PORT_CODE(KEYCODE_W)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("e  E") PORT_CODE(KEYCODE_E)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("r  R") PORT_CODE(KEYCODE_R)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("t  T") PORT_CODE(KEYCODE_T)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("y  Y") PORT_CODE(KEYCODE_Y)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("u  U") PORT_CODE(KEYCODE_U)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("i  I") PORT_CODE(KEYCODE_I)

    PORT_START /* KEY ROW 3 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("o  O") PORT_CODE(KEYCODE_O)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("p  P") PORT_CODE(KEYCODE_P)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("[  {") PORT_CODE(KEYCODE_OPENBRACE)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE

    PORT_START /* KEY ROW 4 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("a  A") PORT_CODE(KEYCODE_A)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("s  S") PORT_CODE(KEYCODE_S)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("d  D") PORT_CODE(KEYCODE_D)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("f  F") PORT_CODE(KEYCODE_F)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("g  G") PORT_CODE(KEYCODE_G)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("h  H") PORT_CODE(KEYCODE_H)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("j  J") PORT_CODE(KEYCODE_J)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("k  K") PORT_CODE(KEYCODE_K)

    PORT_START /* KEY ROW 5 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("l  L") PORT_CODE(KEYCODE_L)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME(";  :") PORT_CODE(KEYCODE_COLON)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("'  \"") PORT_CODE(KEYCODE_QUOTE)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\\  |") PORT_CODE(KEYCODE_ASTERISK)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (L)") PORT_CODE(KEYCODE_LSHIFT)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("z  Z") PORT_CODE(KEYCODE_Z)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("x  X") PORT_CODE(KEYCODE_X)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("c  C") PORT_CODE(KEYCODE_C)

    PORT_START /* KEY ROW 6 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("v  V") PORT_CODE(KEYCODE_V)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("b  B") PORT_CODE(KEYCODE_B)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("n  N") PORT_CODE(KEYCODE_N)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("m  M") PORT_CODE(KEYCODE_M)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME(",  <") PORT_CODE(KEYCODE_COMMA)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(".  >") PORT_CODE(KEYCODE_STOP)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("/  ?") PORT_CODE(KEYCODE_SLASH)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (R)") PORT_CODE(KEYCODE_RSHIFT)

    PORT_START /* KEY ROW 7 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("LINE FEED")
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("- (KP)") PORT_CODE(KEYCODE_MINUS_PAD)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME(", (KP)") PORT_CODE(KEYCODE_PLUS_PAD)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("ENTER (KP)") PORT_CODE(KEYCODE_ENTER_PAD)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(". (KP)") PORT_CODE(KEYCODE_DEL_PAD)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0 (KP)") PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1 (KP)") PORT_CODE(KEYCODE_1_PAD)

    PORT_START /* KEY ROW 8 */
    PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2 (KP)") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3 (KP)") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4 (KP)") PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5 (KP)") PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6 (KP)") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7 (KP)") PORT_CODE(KEYCODE_7_PAD)
    PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("8 (KP)") PORT_CODE(KEYCODE_8_PAD)
    PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("9 (KP)") PORT_CODE(KEYCODE_9_PAD)

    PORT_START /* VIA #1 PORT A */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1                   )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2                   )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY
INPUT_PORTS_END

static struct GfxLayout char_layout =
{
    8, 16,      /* 8 x 16 graphics */
    128,        /* 128 codes */
    1,          /* 1 bit per pixel */
    { 0 },      /* no bitplanes */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
      8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
    8 * 16      /* code takes 8 times 16 bits */
};

static struct GfxLayout chunky_layout =
{
    8, 16,      /* 8 x 16 graphics */
    256,        /* 256 codes */
    1,          /* 1 bit per pixel */
    { 0 },      /* no bitplanes */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
      8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
    8 * 16      /* code takes 8 times 16 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { REGION_GFX1, 0, &char_layout, 0, 1 },
    { REGION_GFX2, 0, &chunky_layout, 0, 1 },
	{-1}
};   /* end of array */

static struct Wave_interface wave_interface = {
    1,
    { 25 }
};

static struct DACinterface dac_interface =
{
    1,          /* number of DACs */
    { 100 }     /* volume */
};

static struct AY8910interface ay8910_interface =
{
    2,  /* 2 chips */
    1000000,    /* 1 MHz ?? */
    { 50, 50 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};

static MACHINE_DRIVER_START( microtan )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 750000)        /* 750 kHz */
	MDRV_CPU_PROGRAM_MAP(readmem_mtan,writemem_mtan)
	MDRV_CPU_VBLANK_INT(microtan_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( microtan )

    /* video hardware - include overscan */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 16*16)
	MDRV_VISIBLE_AREA(0*8, 32*8 - 1, 0*16, 16*16 - 1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT( microtan )

	MDRV_VIDEO_START( microtan )
	MDRV_VIDEO_UPDATE( microtan )

	/* sound hardware */
	MDRV_SOUND_ADD(WAVE, wave_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

ROM_START(microtan)
    ROM_REGION(0x10000,REGION_CPU1,0)
        ROM_LOAD("tanex_j2.rom", 0xc000, 0x1000, CRC(3e09d384))
        ROM_LOAD("tanex_h2.rom", 0xd000, 0x1000, CRC(75105113))
        ROM_LOAD("tanex_d3.rom", 0xe000, 0x0800, CRC(ee6e8412))
        ROM_LOAD("tanex_e2.rom", 0xe800, 0x0800, CRC(bd87fd34))
        ROM_LOAD("tanex_g2.rom", 0xf000, 0x0800, CRC(9fd233ee))
        ROM_LOAD("tanbug_2.rom", 0xf800, 0x0400, CRC(7e215313))
        ROM_LOAD("tanbug.rom",   0xfc00, 0x0400, CRC(c8221d9e))
    ROM_REGION(0x00800,REGION_GFX1,0)
        ROM_LOAD("charset.rom",  0x0000, 0x0800, CRC(3b3c5360))
    ROM_REGION(0x01000,REGION_GFX2,0)
        /* initialized in init_microtan */
ROM_END

SYSTEM_CONFIG_START(microtan)
	CONFIG_DEVICE_CASSETTE(1,		NULL)
	CONFIG_DEVICE_SNAPSHOT_DELAY(	"m65\0", microtan, 0.5)
	CONFIG_DEVICE_QUICKLOAD_DELAY(	"hex\0", microtan_hexfile, 0.5)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT      CONFIG    COMPANY      FULLNAME */
COMP( 1979, microtan, 0,		0,		microtan, microtan, microtan, microtan, "Tangerine", "Microtan 65" )


