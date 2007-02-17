/***************************************************************************

    Atari Crystal Castles hardware

    driver by Pat Lawrence

    Games supported:
        * Crystal Castles (1983) [3 sets]

    Known issues:
        * none at this time

****************************************************************************

    Horizontal sync chain:

        A J/K flip flop @ 8L counts the 1H line, and cascades into a
        4-bit binary counter @ 7M, which counts the 2H,4H,8H,16H lines.
        This counter cascades into a 4-bit BCD decade counter @ 7N
        which counts the 32H,64H,128H,HBLANK lines. The counter system
        rolls over after counting to 320.

        Pixel clock = 5MHz
        HBLANK ends at H = 0
        HBLANK begins at H = 256
        HSYNC begins at H = 304
        HSYNC ends at H = 320
        HTOTAL = 320

    Vertical sync chain:

        The HBLANK signal clocks a 4-bit binary counter @ 7P, which counts
        the 1V,2V,4V,8V lines. This counter cascades into a second 4-bit
        binary counter @ 7R which counts the 16V,32V,64V,128V lines. The
        counter system rolls over after counting to 256.

        VBLANK and VSYNC signals are controlled by a PROM at 8J. The
        standard PROM maps as follows:

        VBLANK ends at V = 24
        VBLANK begins at V = 0
        VSYNC begins at V = 4
        VSYNC ends at V = 7
        VTOTAL = 256

    Interrupts:

        /IRQ clocked by IRQCK signal from the PROM at 8J. The standard
        PROM has a rising edge at V = 0,64,128,192.

****************************************************************************

    Crystal Castles memory map.

     Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
              1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
              5 4 3 2 1 0                      W
    -------------------------------------------------------------------------------
    0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
    0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
    0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
    0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
    0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
    8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
    8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
    -------------------------------------------------------------------------------
    8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
                                          0 0 R/W D D D D D D D D  MOB Picture
                                          0 1 R/W D D D D D D D D  MOB Vertical
                                          1 0 R/W D D D D D D D D  MOB Priority
                                          1 1 R/W D D D D D D D D  MOB Horizontal
    -------------------------------------------------------------------------------
    9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
    9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
    9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
    9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
    9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                               R                D  COIN R
                                               R              D    COIN L
                                               R            D      COIN AUX
                                               R          D        SLAM
                                               R        D          SELF TEST
                                               R      D            VBLANK
                                               R    D              JMP1
                                               R  D                JMP2
    -------------------------------------------------------------------------------
    9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
    9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
    9A08                                                    D D D  Option SW
                                                          D        SPARE
                                                        D          SPARE
                                                      D            SPARE
    9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
    -------------------------------------------------------------------------------
    9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
    9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
    9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
    9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
              1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
    9E80                                0 0 0  W                D  Trak Ball Light P1
    9E81                                0 0 1  W                D  Trak Ball Light P2
    9E82                                0 1 0  W                D  Store Low
    9E83                                0 1 1  W                D  Store High
    9E84                                1 0 0  W                D  Spare
    9E85                                1 0 1  W                D  Coin Counter R
    9E86                                1 1 0  W                D  Coin Counter L
    9E87                                1 1 1  W                D  BANK0-BANK1
              1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
    9F00                                0 0 0  W          D        ^AX
    9F01                                0 0 1  W          D        ^AY
    9F02                                0 1 0  W          D        ^XINC
    9F03                                0 1 1  W          D        ^YINC
    9F04                                1 0 0  W          D        PLAYER2 (flip screen)
    9F05                                1 0 1  W          D        ^SIRE
    9F06                                1 1 0  W          D        BOTHRAM
    9F07                                1 1 1  W          D        BUF1/^BUF2 (sprite bank)
    9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
    A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

***************************************************************************/

#include "driver.h"
#include "sound/pokey.h"
#include "ccastles.h"


#define MASTER_CLOCK	(10000000)

#define PIXEL_CLOCK		(MASTER_CLOCK/2)
#define HTOTAL			(320)
#define VTOTAL			(256)



/*************************************
 *
 *  Globals
 *
 *************************************/

static const UINT8 *syncprom;
static mame_timer *irq_timer;

static UINT8 irq_state;
static UINT8 *nvram_stage;
static UINT8 nvram_store[2];

int ccastles_vblank_start;
int ccastles_vblank_end;



/*************************************
 *
 *  VBLANK and IRQ generation
 *
 *************************************/

INLINE void schedule_next_irq(int curscanline)
{
	/* scan for a rising edge on the IRQCK signal */
	for (curscanline++; ; curscanline = (curscanline + 1) & 0xff)
		if ((syncprom[(curscanline - 1) & 0xff] & 8) == 0 && (syncprom[curscanline] & 8) != 0)
			break;

	/* next one at the start of this scanline */
	mame_timer_adjust(irq_timer, video_screen_get_time_until_pos(0, curscanline, 0), curscanline, time_zero);
}


static void clock_irq(int param)
{
	/* assert the IRQ if not already asserted */
	if (!irq_state)
	{
		cpunum_set_input_line(0, 0, ASSERT_LINE);
		irq_state = 1;
	}

	/* force an update now */
	video_screen_update_partial(0, video_screen_get_vpos(0));

	/* find the next edge */
	schedule_next_irq(param);
}


static UINT32 get_vblank(void *param)
{
	int scanline = video_screen_get_vpos(0);
	return syncprom[scanline & 0xff] & 1;
}



/*************************************
 *
 *  Machine setup
 *
 *************************************/

static MACHINE_START( ccastles )
{
	rectangle visarea;

	/* initialize globals */
	syncprom = memory_region(REGION_PROMS) + 0x000;

	/* find the start of VBLANK in the SYNC PROM */
	for (ccastles_vblank_start = 0; ccastles_vblank_start < 256; ccastles_vblank_start++)
		if ((syncprom[(ccastles_vblank_start - 1) & 0xff] & 1) == 0 && (syncprom[ccastles_vblank_start] & 1) != 0)
			break;
	if (ccastles_vblank_start == 0)
		ccastles_vblank_start = 256;

	/* find the end of VBLANK in the SYNC PROM */
	for (ccastles_vblank_end = 0; ccastles_vblank_end < 256; ccastles_vblank_end++)
		if ((syncprom[(ccastles_vblank_end - 1) & 0xff] & 1) != 0 && (syncprom[ccastles_vblank_end] & 1) == 0)
			break;

	/* can't handle the wrapping case */
	assert(ccastles_vblank_end < ccastles_vblank_start);

	/* reconfigure the visible area to match */
	visarea.min_x = 0;
	visarea.max_x = 255;
	visarea.min_y = ccastles_vblank_end;
	visarea.max_y = ccastles_vblank_start - 1;
	video_screen_configure(0, 320, 256, &visarea, (float)PIXEL_CLOCK / (float)VTOTAL / (float)HTOTAL);

	/* configure the ROM banking */
	memory_configure_bank(1, 0, 2, memory_region(REGION_CPU1) + 0xa000, 0x6000);

	/* create a timer for IRQs and set up the first callback */
	irq_timer = timer_alloc(clock_irq);
	irq_state = 0;
	schedule_next_irq(0);

	/* allocate backing memory for the NVRAM */
	generic_nvram = auto_malloc(generic_nvram_size);

	/* setup for save states */
	state_save_register_global(irq_state);
	state_save_register_global_array(nvram_store);
	state_save_register_global_pointer(generic_nvram, generic_nvram_size);

	return 0;
}


static MACHINE_RESET( ccastles )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	irq_state = 0;
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE8_HANDLER( irq_ack_w )
{
	if (irq_state)
	{
		cpunum_set_input_line(0, 0, CLEAR_LINE);
		irq_state = 0;
	}
}


static WRITE8_HANDLER( led_w )
{
	set_led_status(offset, ~data & 1);
}


static WRITE8_HANDLER( ccounter_w )
{
	coin_counter_w(offset, data & 1);
}


static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1, data & 1);
}


static READ8_HANDLER( leta_r )
{
	return readinputport(2 + offset);
}



/*************************************
 *
 *  NVRAM handling
 *
 *************************************/

static NVRAM_HANDLER( ccastles )
{
	if (read_or_write)
	{
		/* on power down, the EAROM is implicitly stored */
		memcpy(generic_nvram, nvram_stage, generic_nvram_size);
		mame_fwrite(file, generic_nvram, generic_nvram_size);
	}
	else if (file)
		mame_fread(file, generic_nvram, generic_nvram_size);
	else
		memset(generic_nvram, 0, generic_nvram_size);
}


static WRITE8_HANDLER( nvram_recall_w )
{
	memcpy(nvram_stage, generic_nvram, generic_nvram_size);
}


static WRITE8_HANDLER( nvram_store_w )
{
	nvram_store[offset] = data & 1;
	if (!nvram_store[0] && nvram_store[1])
		memcpy(generic_nvram, nvram_stage, generic_nvram_size);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

/* complete memory map derived from schematics */
static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0001) AM_WRITE(ccastles_bitmode_addr_w)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(ccastles_bitmode_r, ccastles_bitmode_w)
	AM_RANGE(0x0000, 0x7fff) AM_READWRITE(MRA8_RAM, ccastles_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x8e00, 0x8fff) AM_BASE(&spriteram)
	AM_RANGE(0x9000, 0x90ff) AM_MIRROR(0x0300) AM_RAM AM_BASE(&nvram_stage) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9400, 0x9403) AM_MIRROR(0x01fc) AM_READ(leta_r)
	AM_RANGE(0x9600, 0x97ff) AM_READ(input_port_0_r)
	AM_RANGE(0x9800, 0x980f) AM_MIRROR(0x01f0) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x9a00, 0x9a0f) AM_MIRROR(0x01f0) AM_READWRITE(pokey2_r, pokey2_w)
	AM_RANGE(0x9c00, 0x9c7f) AM_WRITE(nvram_recall_w)
	AM_RANGE(0x9c80, 0x9cff) AM_WRITE(ccastles_hscroll_w)
	AM_RANGE(0x9d00, 0x9d7f) AM_WRITE(ccastles_vscroll_w)
	AM_RANGE(0x9d80, 0x9dff) AM_WRITE(irq_ack_w)
	AM_RANGE(0x9e00, 0x9e7f) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x9e80, 0x9e81) AM_MIRROR(0x0078) AM_WRITE(led_w)
	AM_RANGE(0x9e82, 0x9e83) AM_MIRROR(0x0078) AM_WRITE(nvram_store_w)
	AM_RANGE(0x9e85, 0x9e86) AM_MIRROR(0x0078) AM_WRITE(ccounter_w)
	AM_RANGE(0x9e87, 0x9e87) AM_MIRROR(0x0078) AM_WRITE(bankswitch_w)
	AM_RANGE(0x9f00, 0x9f07) AM_MIRROR(0x0078) AM_WRITE(ccastles_video_control_w)
	AM_RANGE(0x9f80, 0x9fbf) AM_MIRROR(0x0040) AM_WRITE(ccastles_paletteram_w)
	AM_RANGE(0xa000, 0xdfff) AM_ROMBANK(1)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( ccastles )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_vblank, 0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )					/* 1p Jump, non-cocktail start1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)		/* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("LETA0")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START_TAG("LETA1")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30)

	PORT_START_TAG("LETA2")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_Y ) PORT_COCKTAIL PORT_SENSITIVITY(10) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START_TAG("LETA3")
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_X ) PORT_COCKTAIL PORT_SENSITIVITY(10) PORT_KEYDELTA(30)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

/* technically, this is 4bpp graphics, but the top bit is thrown away during
    processing to make room for the priority bit in the sprite buffers */
static const gfx_layout ccastles_spritelayout =
{
	8,16,
	RGN_FRAC(1,2),
	3,
	{ 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &ccastles_spritelayout,  0, 2 },
	{ -1 }
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	{ 0 },
	input_port_1_r
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ccastles )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, MASTER_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_MACHINE_START(ccastles)
	MDRV_MACHINE_RESET(ccastles)
	MDRV_NVRAM_HANDLER(ccastles)
	MDRV_WATCHDOG_VBLANK_INIT(8)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE((float)PIXEL_CLOCK / (float)VTOTAL / (float)HTOTAL)
	MDRV_SCREEN_SIZE(HTOTAL, VTOTAL)
	MDRV_SCREEN_VBLANK_TIME(0)			/* VBLANK is handled manually */
	MDRV_SCREEN_VISIBLE_AREA(0, 255, 0, 231)

	MDRV_VIDEO_START(ccastles)
	MDRV_VIDEO_UPDATE(ccastles)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, MASTER_CLOCK/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(POKEY, MASTER_CLOCK/8)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( ccastles )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-403.1k", 0x0a000, 0x2000, CRC(81471ae5) SHA1(8ec13b48119ecf8fe85207403c0a0de5240cded4) )
	ROM_LOAD( "136022-404.1l", 0x0c000, 0x2000, CRC(820daf29) SHA1(a2cff00e9ddce201344692b75038431e4241fedd) )
	ROM_LOAD( "136022-405.1n", 0x0e000, 0x2000, CRC(4befc296) SHA1(2e789a32903808014e9d5f3021d7eff57c3e2212) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastleg )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-303.1k", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
	ROM_LOAD( "136022-304.1l", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
	ROM_LOAD( "136022-112.1n", 0x0e000, 0x2000, CRC(69b8d906) SHA1(b71251a4402eedf97b6ed5798403823739991d3e) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastlep )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-303.1k", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
	ROM_LOAD( "136022-304.1l", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
	ROM_LOAD( "136022-113.1n", 0x0e000, 0x2000, CRC(b833936e) SHA1(c063989107acb82ac963342d6328c7e459160d2a) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastlef )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-303.1k", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
	ROM_LOAD( "136022-304.1l", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
	ROM_LOAD( "136022-114.1n", 0x0e000, 0x2000, CRC(8585b4d1) SHA1(e2054dba64cc210a0790fe32a98d8c35c1389bf5) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastle3 )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-303.1k", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
	ROM_LOAD( "136022-304.1l", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
	ROM_LOAD( "136022-305.1n", 0x0e000, 0x2000, CRC(9418cf8a) SHA1(1f835db94270e4a16e721b2ac355fb7e7c052285) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastle2 )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-203.1k", 0x0a000, 0x2000, CRC(348a96f0) SHA1(76de7bf6a01ccb15a4fe7333c1209f623a2e0d1b) )
	ROM_LOAD( "136022-204.1l", 0x0c000, 0x2000, CRC(d48d8c1f) SHA1(8744182a3e2096419de63e341feb77dd8a8bcb34) )
	ROM_LOAD( "136022-205.1n", 0x0e000, 0x2000, CRC(0e4883cc) SHA1(a96abbf654e087409a90c1686d9dd553bd08c14e) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END


ROM_START( ccastle1 )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "136022-103.1k", 0x0a000, 0x2000, CRC(9d10e314) SHA1(3474ae0f0617c1dc9aaa02ca2a912a72d57eba73) )
	ROM_LOAD( "136022-104.1l", 0x0c000, 0x2000, CRC(fe2647a4) SHA1(532b236043449b35bd444fff63a7e083d0e2d8c8) )
	ROM_LOAD( "136022-105.1n", 0x0e000, 0x2000, CRC(5a13af07) SHA1(d4314a4344aac4a794d9014943591fee2e9bf13b) )
	ROM_LOAD( "136022-102.1h", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )
	ROM_LOAD( "136022-101.1f", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136022-106.8d", 0x0000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
	ROM_LOAD( "136022-107.8b", 0x2000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s129-136022-108.7k",  0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) )
	ROM_LOAD( "82s129-136022-109.6l",  0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) )
	ROM_LOAD( "82s129-136022-110.11l", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) )
	ROM_LOAD( "82s129-136022-111.10k", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, ccastles, 0,        ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 4)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastleg, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3, German)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastlep, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3, Spanish)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastlef, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3, French)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastle3, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastle2, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 2)", GAME_SUPPORTS_SAVE )
GAME( 1983, ccastle1, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 1)", GAME_SUPPORTS_SAVE )
