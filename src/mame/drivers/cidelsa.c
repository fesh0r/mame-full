#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "cpu/cop400/cop400.h"
#include "video/generic.h"
#include "video/cdp1869.h"
#include "sound/cdp1869.h"
#include "sound/ay8910.h"

/* CDP1802 Interface */

static int cdp1869_prd;
extern int cdp1869_pcb;

static void cidelsa_out_q(int level)
{
	cdp1869_pcb = level;
}

static int cidelsa_in_ef(void)
{
	/*
        EF1     CDP1869 _PRD
        EF2     Test
        EF3     Coin 2
        EF4     Coin 1
    */

	return (cdp1869_prd & 0x01) + (readinputportbytag("EF") & 0xfe);
}

static CDP1802_CONFIG cidelsa_cdp1802_config =
{
	NULL,
	cidelsa_out_q,
	cidelsa_in_ef
};

READ8_HANDLER ( cidelsa_input_port_0_r )
{
	return (readinputportbytag("IN0") & 0x7f) + (cdp1869_pcb ? 0x80 : 0x00);
}

/* Sound Interface */

static int draco_sound;
static int draco_g;

WRITE8_HANDLER ( draco_sound_bankswitch_w )
{
	memory_set_bank(1, (data & 0x08) >> 3);
}

WRITE8_HANDLER ( draco_sound_g_w )
{
    draco_g = data;
}

READ8_HANDLER ( draco_sound_in_r )
{
	return draco_sound & 0x07;
}

WRITE8_HANDLER ( draco_sound_ay8910_w )
{
	/*

     G1 G0  description

      0  0  inactive
      0  1  read from PSG
      1  0  write to PSG
      1  1  latch address

    */

	switch(draco_g)
	{
	case 0x02:
		AY8910_write_port_0_w(0, data);
		break;

	case 0x03:
		AY8910_control_port_0_w(0, data);
		break;
	}
}

WRITE8_HANDLER ( draco_ay8910_port_a_w )
{
	/*
      bit   description

        0   N/C
        1   N/C
        2   N/C
        3   N/C
        4   N/C
        5   N/C
        6   N/C
        7   N/C
    */
}

WRITE8_HANDLER ( draco_ay8910_port_b_w )
{
	/*
      bit   description

        0   RELE0
        1   RELE1
        2   sound output -> * -> 22K capacitor -> GND
        3   sound output -> * -> 220K capacitor -> GND
        4   5V -> 1K resistor -> * -> 10uF capacitor -> GND (volume pot voltage adjustment)
        5   N/C
        6   N/C
        7   N/C
    */
}

static struct AY8910interface ay8910_interface =
{
	0,
	0,
	draco_ay8910_port_a_w,
	draco_ay8910_port_b_w
};

/* Read/Write Handlers */

WRITE8_HANDLER ( destryer_out1_w )
{
	/*
      bit   description

        0
        1
        2
        3
        4
        5
        6
        7
    */
}

WRITE8_HANDLER ( altair_out1_w )
{
	/*
      bit   description

        0   S1 (CARTUCHO)
        1   S2 (CARTUCHO)
        2   S3 (CARTUCHO)
        3   LG1
        4   LG2
        5   LGF
        6   CONT. M2
        7   CONT. M1
    */

	set_led_status(0, data & 0x08); // 1P
	set_led_status(1, data & 0x10); // 2P
	set_led_status(2, data & 0x20); // FIRE
}

WRITE8_HANDLER ( draco_out1_w )
{
	/*
      bit   description

        0   3K9 -> Green signal
        1   820R -> Blue signal
        2   510R -> Red signal
        3   1K -> N/C
        4   N/C
        5   SONIDO A -> COP402 IN0
        6   SONIDO B -> COP402 IN1
        7   SONIDO C -> COP402 IN2
    */

    draco_sound = (data & 0xe0) >> 5;
}

/* Memory Maps */

// Destroyer

static ADDRESS_MAP_START( destryer_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x20ff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE(cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( destryea_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x3000, 0x30ff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE(cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( destryer_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cidelsa_input_port_0_r, destryer_out1_w)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

// Altair

static ADDRESS_MAP_START( altair_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x37ff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE(cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( altair_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cidelsa_input_port_0_r, altair_out1_w)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

// Draco

static ADDRESS_MAP_START( draco_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE(cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cidelsa_input_port_0_r, draco_out1_w)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x3ff) AM_ROMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(COP400_PORT_D, COP400_PORT_D) AM_WRITE(draco_sound_bankswitch_w)
	AM_RANGE(COP400_PORT_G, COP400_PORT_G) AM_WRITE(draco_sound_g_w)
	AM_RANGE(COP400_PORT_L, COP400_PORT_L) AM_READWRITE(AY8910_read_port_0_r, draco_sound_ay8910_w)
	AM_RANGE(COP400_PORT_IN, COP400_PORT_IN) AM_READ(draco_sound_in_r)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( destryer )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // CARTUCHO
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 ) // 1P
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 ) // 2P
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) // RG
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) // LF
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) // FR
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL ) // PCB from CDP1869

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Very Conserv" )
	PORT_DIPSETTING(    0x01, "Conserv" )
	PORT_DIPSETTING(    0x02, "Liberal" )
	PORT_DIPSETTING(    0x03, "Very Liberal" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, "14000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR(Coinage) )
	PORT_DIPSETTING(    0xc0, "Slot A: 1  Slot B: 2" )
	PORT_DIPSETTING(    0x80, "Slot A: 1.5  Slot B: 3" )
	PORT_DIPSETTING(    0x40, "Slot A: 2  Slot B: 4" )
	PORT_DIPSETTING(    0x00, "Slot A: 2.5  Slot B: 5" )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) // _PREDISPLAY from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) // M1
INPUT_PORTS_END

INPUT_PORTS_START( altair )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // CARTUCHO
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 ) // 1P
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 ) // 2P
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) // RG
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) // LF
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) // FR
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL ) // PCB from CDP1869

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Very Conserv" )
	PORT_DIPSETTING(    0x01, "Conserv" )
	PORT_DIPSETTING(    0x02, "Liberal" )
	PORT_DIPSETTING(    0x03, "Very Liberal" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, "14000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR(Coinage) )
	PORT_DIPSETTING(    0xc0, "Slot A: 1  Slot B: 2" )
	PORT_DIPSETTING(    0x80, "Slot A: 1.5  Slot B: 3" )
	PORT_DIPSETTING(    0x40, "Slot A: 2  Slot B: 4" )
	PORT_DIPSETTING(    0x00, "Slot A: 2.5  Slot B: 5" )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) // UP
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) // DN
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) // IN
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) // _PREDISPLAY from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) // M1
INPUT_PORTS_END

INPUT_PORTS_START( draco )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // CARTUCHO
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 ) // 1P
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 ) // 2P
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) // RG
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) // LF
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) // FR
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL ) // PCB from CDP1869

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Very Conserv" )
	PORT_DIPSETTING(    0x01, "Conserv" )
	PORT_DIPSETTING(    0x02, "Liberal" )
	PORT_DIPSETTING(    0x03, "Very Liberal" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, "14000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR(Coinage) )
	PORT_DIPSETTING(    0xc0, "Slot A: 1  Slot B: 2" )
	PORT_DIPSETTING(    0x80, "Slot A: 1.5  Slot B: 3" )
	PORT_DIPSETTING(    0x40, "Slot A: 2  Slot B: 4" )
	PORT_DIPSETTING(    0x00, "Slot A: 2.5  Slot B: 5" )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) // UP
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) // DN
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) // IN
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) // _PREDISPLAY from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) // M1
INPUT_PORTS_END

/* Interrupt Generators */

static INTERRUPT_GEN( draco_interrupt )
{
	int scanline = (CPD1870_TOTAL_SCANLINES_PAL - 1) - cpu_getiloops();

	switch (scanline)
	{
	case CPD1870_SCANLINE_PREDISPLAY_START_PAL:
		cdp1869_prd = 0;
		break;

	case CPD1870_SCANLINE_PREDISPLAY_END_PAL:
		cdp1869_prd = 1;
		break;
	}
}

static INTERRUPT_GEN( altair_interrupt )
{
	int scanline = (CPD1870_TOTAL_SCANLINES_PAL - 1) - cpu_getiloops();

	switch (scanline)
	{
	case CPD1870_SCANLINE_PREDISPLAY_START_PAL:
		cpunum_set_input_line(0, 0, CLEAR_LINE);
		cdp1869_prd = 1;
		break;

	case CPD1870_SCANLINE_PREDISPLAY_END_PAL:
		cpunum_set_input_line(0, 0, ASSERT_LINE);
		cdp1869_prd = 0;
		break;
	}
}

/* Machine Drivers */

static MACHINE_DRIVER_START( destryer )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000) // ???
	MDRV_CPU_PROGRAM_MAP(destryer_map, 0)
	MDRV_CPU_IO_MAP(destryer_io_map, 0)
	MDRV_CPU_CONFIG(cidelsa_cdp1802_config)
	MDRV_CPU_VBLANK_INT(altair_interrupt, CPD1870_TOTAL_SCANLINES_PAL)

	MDRV_SCREEN_REFRESH_RATE(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_SCREEN_VISIBLE_AREA(0, CDP1870_HBLANK_START-1, 0, CPD1870_SCANLINE_VBLANK_START_PAL-1)
	MDRV_PALETTE_LENGTH(8+64)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000) // ???
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( destryea )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000) // ???
	MDRV_CPU_PROGRAM_MAP(destryea_map, 0)
	MDRV_CPU_IO_MAP(destryer_io_map, 0)
	MDRV_CPU_CONFIG(cidelsa_cdp1802_config)
	MDRV_CPU_VBLANK_INT(altair_interrupt, CPD1870_TOTAL_SCANLINES_PAL)

	MDRV_SCREEN_REFRESH_RATE(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_SCREEN_VISIBLE_AREA(0, CDP1870_HBLANK_START-1, 0, CPD1870_SCANLINE_VBLANK_START_PAL-1)
	MDRV_PALETTE_LENGTH(8+64)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000) // ???
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( altair )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000)
	MDRV_CPU_PROGRAM_MAP(altair_map, 0)
	MDRV_CPU_IO_MAP(altair_io_map, 0)
	MDRV_CPU_CONFIG(cidelsa_cdp1802_config)
	MDRV_CPU_VBLANK_INT(altair_interrupt, CPD1870_TOTAL_SCANLINES_PAL)

	MDRV_SCREEN_REFRESH_RATE(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_SCREEN_VISIBLE_AREA(0, CDP1870_HBLANK_START-1, 0, CPD1870_SCANLINE_VBLANK_START_PAL-1)
	MDRV_PALETTE_LENGTH(8+64)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( draco )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000)
	MDRV_CPU_PROGRAM_MAP(draco_map, 0)
	MDRV_CPU_IO_MAP(draco_io_map, 0)
	MDRV_CPU_CONFIG(cidelsa_cdp1802_config)
	MDRV_CPU_VBLANK_INT(draco_interrupt, CPD1870_TOTAL_SCANLINES_PAL)

	MDRV_CPU_ADD(COP420, 2012160) // COP402N
	MDRV_CPU_PROGRAM_MAP(draco_sound_map, 0)
	MDRV_CPU_IO_MAP(draco_sound_io_map, 0)

	MDRV_SCREEN_REFRESH_RATE(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_SCREEN_VISIBLE_AREA(0, CDP1870_HBLANK_START-1, 0, CPD1870_SCANLINE_VBLANK_START_PAL-1)
	MDRV_PALETTE_LENGTH(8+64)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 2012160)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* Driver Initialization */

DRIVER_INIT( draco )
{
	UINT8 *ROM = memory_region(REGION_CPU2);
	memory_configure_bank(1, 0, 2, &ROM[0x000], 0x400);
}

/* ROMs */

ROM_START( destryer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "des a 2.ic4", 0x0000, 0x0800, CRC(63749870) SHA1(a8eee4509d7a52dcf33049de221d928da3632174) )
	ROM_LOAD( "des b 2.ic5", 0x0800, 0x0800, CRC(60604f40) SHA1(32ca95c5b38b0f4992e04d77123d217f143ae084) )
	ROM_LOAD( "des c 2.ic6", 0x1000, 0x0800, CRC(a7cdeb7b) SHA1(a5a7748967d4ca89fb09632e1f0130ef050dbd68) )
	ROM_LOAD( "des d 2.ic7", 0x1800, 0x0800, CRC(dbec0aea) SHA1(1d9d49009a45612ee79763781a004499313b823b) )
ROM_END

// this was destroyer2.rom in standalone emu..
ROM_START( destryea )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "destryea_1", 0x0000, 0x0800, CRC(421428e9) SHA1(0ac3a1e7f61125a1cd82145fa28cbc4b93505dc9) )
	ROM_LOAD( "destryea_2", 0x0800, 0x0800, CRC(55dc8145) SHA1(a0066d3f3ac0ae56273485b74af90eeffea5e64e) )
	ROM_LOAD( "destryea_3", 0x1000, 0x0800, CRC(5557bdf8) SHA1(37a9cbc5d25051d3bed7535c58aac937cd7c64e1) )
	ROM_LOAD( "destryea_4", 0x1800, 0x0800, CRC(608b779c) SHA1(8fd6cc376c507680777553090329cc66be42a934) )
ROM_END

ROM_START( altair )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "alt a 1.ic7",  0x0000, 0x0800, CRC(37c26c4e) SHA1(30df7efcf5bd12dafc1cb6e894fc18e7b76d3e61) )
	ROM_LOAD( "alt b 1.ic8",  0x0800, 0x0800, CRC(76b814a4) SHA1(e8ab1d1cbcef974d929ef8edd10008f60052a607) )
	ROM_LOAD( "alt c 1.ic9",  0x1000, 0x0800, CRC(2569ce44) SHA1(a09597d2f8f50fab9a09ed9a59c50a2bdcba47bb) )
	ROM_LOAD( "alt d 1.ic10", 0x1800, 0x0800, CRC(a25e6d11) SHA1(c197ff91bb9bdd04e88908259e4cde11b990e31d) )
	ROM_LOAD( "alt e 1.ic11", 0x2000, 0x0800, CRC(e497f23b) SHA1(6094e9873df7bd88c521ddc3fd63961024687243) )
	ROM_LOAD( "alt f 1.ic12", 0x2800, 0x0800, CRC(a06dd905) SHA1(c24ad9ff6d4e3b4e57fd75f946e8832fa00c2ea0) )
ROM_END

ROM_START( draco )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dra a 1.ic10", 0x0000, 0x0800, CRC(ca127984) SHA1(46721cf42b1c891f7c88bc063a2149dd3cefea74) )
	ROM_LOAD( "dra b 1.ic11", 0x0800, 0x0800, CRC(e4936e28) SHA1(ddbbf769994d32a6bce75312306468a89033f0aa) )
	ROM_LOAD( "dra c 1.ic12", 0x1000, 0x0800, CRC(94480f5d) SHA1(8f49ce0f086259371e999d097a502482c83c6e9e) )
	ROM_LOAD( "dra d 1.ic13", 0x1800, 0x0800, CRC(32075277) SHA1(2afaa92c91f554e3bdcfec6d94ef82df63032afb) )
	ROM_LOAD( "dra e 1.ic14", 0x2000, 0x0800, CRC(cce7872e) SHA1(c956eb994452bd8a27bbc6d0e6d103e87a4a3e6e) )
	ROM_LOAD( "dra f 1.ic15", 0x2800, 0x0800, CRC(e5927ec7) SHA1(42e0aabb6187bbb189648859fd5dddda43814526) )
	ROM_LOAD( "dra g 1.ic16", 0x3000, 0x0800, CRC(f28546c0) SHA1(daedf1d64f94358b15580d697dd77d3c977aa22c) )
	ROM_LOAD( "dra h 1.ic17", 0x3800, 0x0800, CRC(dce782ea) SHA1(f558096f43fb30337bc4a527169718326c265c2c) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "dra s 1.ic4",  0x0000, 0x0800, CRC(292a57f8) SHA1(b34a189394746d77c3ee669db24109ee945c3be7) )
ROM_END

/* Game Drivers */

GAME( 1980, destryer, 0, destryer, destryer, 0,     ROT90, "Cidelsa", "Destroyer (Cidelsa) (set 1)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND ) // ok, but bad colours, some bad gfx (large enemies etc.)
GAME( 1980, destryea, destryer, destryea, destryer, 0,     ROT90, "Cidelsa", "Destroyer (Cidelsa) (set 2)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND ) // ok, but bad colours, some bad gfx (large enemies etc.)
GAME( 1981, altair,   0, altair,   altair,   0,     ROT90, "Cidelsa", "Altair", GAME_NOT_WORKING | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND ) // some bullets are invisible
GAME( 1981, draco,    0, draco,    draco,    draco, ROT90, "Cidelsa", "Draco", GAME_NOT_WORKING | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND ) // crashes on game start
