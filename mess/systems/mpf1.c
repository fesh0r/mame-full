/************************************************\
* Multitech Micro Professor 1                    *
*                                                *
*     CPU: Z80 @ 3.58MHz                         *
*     ROM: 4-kilobyte ROM monitor                *
*     RAM: 4 kilobytes                           *
*   Input: Hex keypad                            *
* Storage: Cassette tape                         *
*   Video: 6x 7-segment LED display              *
*   Sound: Beeper                                *
*                                                *
* TODO:                                          *
*    Cassette storage emulation                  *
*    Sound                                       *
*    BASIC ROM expansion                         *
*    I/O board                                   *
\************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "cpu/z80/z80.h"
#include "mscommon.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

const char leddisplay[] = {
	"  aaaaaaaaaaaa          \r"
	"   aaaaaaaaaa           \r"
	"f   aaaaaaaa   b        \r"
	"ff            bb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"fff          bbb        \r"
	"ff  gggggggg  bb        \r"
	"f  gggggggggg  b        \r"
	"  gggggggggggg          \r"
	"e  gggggggggg  c        \r"
	"ee  gggggggg  cc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"eee          ccc        \r"
	"ee            cc    hhhh\r"
	"e   dddddddd   c    hhhh\r"
	"   dddddddddd       hhhh\r"
	"  dddddddddddd      hhhh" };

static UINT32 leddigit[6];
static INT8 lednum;
static INT8 keycol;
static UINT8 kbdlatch;

static void logleds(void)
{
	for(lednum = 5; lednum >= 0; lednum--)
	{
		if( leddigit[lednum] & 0x08 ) logerror( " _ " );
		else logerror( "   " );
	}
	logerror( "\n" );
	for(lednum = 5; lednum >= 0; lednum--)
	{
		if( leddigit[lednum] & 0x04 ) logerror( "|" );
		else logerror( " " );
		if( leddigit[lednum] & 0x02 ) logerror( "_" );
		else logerror( " " );
		if( leddigit[lednum] & 0x10 ) logerror( "|" );
		else logerror( " " );
	}
	logerror( "\n" );
	for(lednum = 5; lednum >= 0; lednum--)
	{
		if( leddigit[lednum] & 0x01 ) logerror( "|" );
		else logerror( " " );
		if( leddigit[lednum] & 0x80 ) logerror( "_" );
		else logerror( " " );
		if( leddigit[lednum] & 0x20 ) logerror( "|" );
		else logerror( " " );
	}
	logerror( "\n" );
}



static PALETTE_INIT( mpf1 )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0xFF, 0xFF, 0xFF);
}



static VIDEO_START( mpf1 )
{
    videoram_size = 6 * 2 + 24;
    videoram = auto_malloc (videoram_size);
	if (!videoram)
        return 1;

	if (video_start_generic () != 0)
        return 1;

    return 0;
}



VIDEO_UPDATE( mpf1 )
{
    int x;

	fillbitmap(Machine->scrbitmap, get_black_pen(), NULL);

	for(x = 0; x < 6; x++)
	{
		draw_led(Machine->scrbitmap, leddisplay, leddigit[x], x * 32, 16);
	}
}



static ADDRESS_MAP_START( mpf1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpf1_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x03) AM_READ(ppi8255_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpf1_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x03) AM_WRITE(ppi8255_0_w)
ADDRESS_MAP_END



static READ_HANDLER( mpf1_porta_r )
{
	UINT8 retval;
	for(keycol = 0; keycol < 6; keycol++)
	{
		if( kbdlatch & 0x01 ) break;
		kbdlatch >>= 1;
	}
	verboselog( 0, "Key column for kbd read: %02x\n", keycol );
	if( keycol != 6 )
	{
		retval = readinputport(keycol);
	}
	else
	{
		retval = 0;
	}
	verboselog( 0, "PPI port A read: %02x\n", retval );
	return retval;
}



static READ_HANDLER( mpf1_portb_r )
{
	verboselog( 0, "PPI port B read\n" );
	return 0;
}



static READ_HANDLER( mpf1_portc_r )
{
	verboselog( 0, "PPI port C read\n" );
	return 0;
}



static WRITE_HANDLER( mpf1_porta_w )
{
	verboselog( 0, "PPI port A write: %02x\n", data );
}



static WRITE_HANDLER( mpf1_portb_w )
{
	/*	A	0x08
		B	0x10
		C	0x20
		D	0x80
		E	0x01
		F	0x04
		G	0x02
		H	0x40 */
	if( data )
	{
		data = ( (data & 0x08) >> 3 ) |
			   ( (data & 0x10) >> 3 ) |
		   	   ( (data & 0x20) >> 3 ) |
		       ( (data & 0x80) >> 4 ) |
		       ( (data & 0x01) << 4 ) |
		       ( (data & 0x04) << 3 ) |
		       ( (data & 0x02) << 5 ) |
		       ( (data & 0x40) << 1 );
		leddigit[lednum] = data;
	}
	verboselog( 1, "PPI port B (LED Data) write: %02x\n", data );
}



static WRITE_HANDLER( mpf1_portc_w )
{
	kbdlatch = ~data;
	if( data & 0x3F )
	{
		for(lednum = 0; lednum < 6; lednum++)
		{
			if( data & 0x01 ) break;
			data >>= 1;
		}
		if( lednum == 6 ) lednum = 0;
		lednum = 5 - lednum;
	}
	verboselog( 1, "PPI port C (LED/Kbd Col select) write: %02x\n", data );
}



static ppi8255_interface ppi8255_intf =
{
	1, 					/* 1 chip */
	{ mpf1_porta_r },	/* Port A read */
	{ mpf1_portb_r },	/* Port B read */
	{ mpf1_portc_r },	/* Port C read */
	{ mpf1_porta_w },	/* Port A write */
	{ mpf1_portb_w },	/* Port B write */
	{ mpf1_portc_w },	/* Port C write */
};



INPUT_PORTS_START( mpf1 )
	PORT_START			/* Column 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "3",        KEYCODE_3,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7",        KEYCODE_7,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",        KEYCODE_B,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",        KEYCODE_F,      IP_JOY_NONE )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* Column 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "2",        KEYCODE_2,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "6",        KEYCODE_6,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",        KEYCODE_A,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",        KEYCODE_E,      IP_JOY_NONE )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* Column 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1",        KEYCODE_1,      	IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "5",        KEYCODE_5,      	IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9",        KEYCODE_9,      	IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",        KEYCODE_D,      	IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "STP (F1)", KEYCODE_F1,     	IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T-R (F5)", KEYCODE_F5,     	IP_JOY_NONE )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* Column 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0",        KEYCODE_0,      	IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "4",        KEYCODE_4,      	IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8",        KEYCODE_8,      	IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",        KEYCODE_C,      	IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "GO  (F2)", KEYCODE_F2,     	IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T-W (F6)", KEYCODE_F6,     	IP_JOY_NONE )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* Column 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CBR",      KEYCODE_N,      	IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "PC",       KEYCODE_M,      	IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "REG",      KEYCODE_COMMA,  	IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "ADR",      KEYCODE_STOP,   	IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL",      KEYCODE_SLASH,  	IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "REL",      KEYCODE_RCONTROL,	IP_JOY_NONE )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* Column 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SBR",      KEYCODE_H,      	IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "--",       KEYCODE_J,      	IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "DATA",     KEYCODE_K,      	IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "+",        KEYCODE_L,      	IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "INS",      KEYCODE_COLON,  	IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "MOVE",     KEYCODE_QUOTE,   	IP_JOY_NONE )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START			/* User Key */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "USER KEY", KEYCODE_U,       	IP_JOY_NONE )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static MACHINE_INIT( mpf1 )
{
	ppi8255_init(&ppi8255_intf);
	for(lednum = 0; lednum < 6; lednum++)
		leddigit[lednum] = 0;
	lednum = 0;
}



static INTERRUPT_GEN( mpf1_interrupt )
{
	cpu_set_irq_line(0, 0, PULSE_LINE);
}



static MACHINE_DRIVER_START( mpf1 )
	MDRV_CPU_ADD(Z80,3580000)
	MDRV_CPU_PROGRAM_MAP(mpf1_map, 0)
	MDRV_CPU_IO_MAP(mpf1_readport,mpf1_writeport)
	MDRV_CPU_VBLANK_INT(mpf1_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( mpf1 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT( mpf1 )

	MDRV_VIDEO_START(mpf1)
	MDRV_VIDEO_UPDATE(mpf1)
MACHINE_DRIVER_END

ROM_START( mpf1 )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("mpf_u6.bin", 0x0000, 0x1000, CRC(b60249ce) SHA1(78e0e8874d1497fabfdd6378266d041175e3797f) )
ROM_END

SYSTEM_CONFIG_START(mpf1)
	CONFIG_RAM_DEFAULT(4 * 1024)
SYSTEM_CONFIG_END

COMP( 1979, mpf1, 0, 0, mpf1, mpf1, 0, mpf1, "Multitech", "Micro Professor 1" )

