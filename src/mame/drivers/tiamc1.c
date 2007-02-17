/*****************************************************************************

  TIA-MC1 driver

  driver by Eugene Sandulenko
  special thanks to Shiru for his standalone emulator and documentation

  Games supported:
      * Konek-Gorbunok

  Other games known to exist on this hardware (interchangeable by the ROM swap):
      * Avtogonki
      * Billiard
      * Istrebitel'
      * Kot-Rybolov
      * Kotigoroshko
      * Ostrov Drakona
      * Ostrov Sokrovisch
      * Perehvatchik
      * Snezhnaja Koroleva
      * Zvezdnyj Rycar'

 ***************************************************************

  During bootup hold F2 to enter test mode.
  Also use F2 to switch the screens during the gameplay (feature of the original
    machine)

 ***************************************************************

  This is one of the last USSR-made arcades. Several games are known to exist on
  this hardware. It was created by the state company Terminal (Vinnitsa, Ukraine),
  which was later turned into EXTREMA-Ukraine company. These days it manufactures
  various gambling machines (http://www.extrema-ua.com).

 ***************************************************************

  TIA-MC1 arcade internals

  This arcade machine contains four PCBs:

    BEIA-100
      Main CPU board. Also contains address PROM, color DAC, input ports and sound

    BEIA-101
      Video board 1. Background generator, video tiles RAM and video sync schematics

    BEIA-102
      Video board 2. Sprite generator, video buffer RAM (hardware triple buffering)

    BEIA-103
      ROM banks, RAM. Contains ROMs and multiplexors


  BEIA-103 PCB Layout

    |----------------------------------|
    |                                  |
  |--| g1.d17   RU8A                   |
  |  |                                 |
  |  |                                 |
  |  | g2.d17   RU8A   IR13   a2.b07   |
  |  |                                 |
  |  |                                 |
  |  | g3.d17   RU8A   IR13   a3.g07  |--|
  |  |                                |  |
  |--|                                |  |
    |  g4.d17   RU8A    kp11          |  |
    |                                 |  |
    |                                 |  |
    |  g5.d17          IR13   a5.l07  |  |
  |--|                                |  |
  |  |           id7                  |  |
  |  | g6.d17          IR13   a6.r07  |--|
  |  |           id7                   |
  |  |                                 |
  |  | g7.d17                          |
  |  |           ll1                   |
  |  |                                 |
  |--|  ap6      le1                   |
    |                                  |
    |----------------------------------|

  Notes:

   g1.d17 \
   g2.d17 |
   g3.d17 |
   g4.d17 |
   g5.d17 |
   g6.d17 +- EPROM K573RF4A (2764 analog)
   g7.d17 |
   a2.b07 |
   a3.g07 |
   a5.l07 |
   a6.r07 /

   RU8A - KR537RU8A SRAM 2k x8 (TC5516 analog)
   IR13 - K155IR13 register (74198 analog)
   kp11 - K555KP11 selector/multiplexor (74LS257 analog)
   id7  - K555ID7 (74LS138 analog)
   ap6  - K555AP6 (74LS245 analog)
   ll1  - K555LL1 (74LS32 analog)
   le1  - K555LE1 (74LS02 analog)

 ***************************************************************

   TODO:
     - Use machine/pit8253.c in sound
     - Check sprites priorities on the real hardware
     - Check background scrolling on the real hardware
     - What charset control is used for?

*/

#include "driver.h"
#include "sound/custom.h"

/* routines defined in video */
extern PALETTE_INIT( tiamc1 );
extern VIDEO_START( tiamc1 );
extern VIDEO_UPDATE( tiamc1 );

extern WRITE8_HANDLER( tiamc1_palette_w );
extern WRITE8_HANDLER( tiamc1_videoram_w );
extern WRITE8_HANDLER( tiamc1_bankswitch_w );
extern WRITE8_HANDLER( tiamc1_sprite_x_w );
extern WRITE8_HANDLER( tiamc1_sprite_y_w );
extern WRITE8_HANDLER( tiamc1_sprite_a_w );
extern WRITE8_HANDLER( tiamc1_sprite_n_w );
extern WRITE8_HANDLER( tiamc1_bg_vshift_w );
extern WRITE8_HANDLER( tiamc1_bg_hshift_w );

extern UINT8 *tiamc1_charram, *tiamc1_tileram;
extern UINT8 *tiamc1_spriteram_x, *tiamc1_spriteram_y, *tiamc1_spriteram_n,
	*tiamc1_spriteram_a;

/* routines defined in audio */
extern void *tiamc1_sh_start(int clock, const struct CustomSound_interface *config);
extern WRITE8_HANDLER( tiamc1_timer0_w );
extern WRITE8_HANDLER( tiamc1_timer1_w );
extern WRITE8_HANDLER( tiamc1_timer1_gate_w );

UINT8 *video_ram;

DRIVER_INIT( tiamc1 )
{
	video_ram = auto_malloc(0x3040);
}

MACHINE_RESET( tiamc1 )
{
	video_ram = auto_malloc(0x3040);
	memset(video_ram, 0, 0x3040);

        tiamc1_charram = video_ram + 0x0800;     /* Ram is banked */
        tiamc1_tileram = video_ram + 0x0000;

	tiamc1_spriteram_y = video_ram + 0x3000;
	tiamc1_spriteram_x = video_ram + 0x3010;
	tiamc1_spriteram_n = video_ram + 0x3020;
	tiamc1_spriteram_a = video_ram + 0x3030;

	tiamc1_bankswitch_w(0, 0);

	state_save_register_global_pointer(video_ram, 0x3040);
}

WRITE8_HANDLER( tiamc1_control_w )
{
	coin_lockout_w(0, ~data & 0x02);
	coin_counter_w(0, data & 0x04);
}


static ADDRESS_MAP_START( tiamc1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xb000, 0xb7ff) AM_WRITE(tiamc1_videoram_w)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( tiamc1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x4f) AM_WRITE(tiamc1_sprite_y_w) /* sprites Y */
	AM_RANGE(0x50, 0x5f) AM_WRITE(tiamc1_sprite_x_w) /* sprites X */
	AM_RANGE(0x60, 0x6f) AM_WRITE(tiamc1_sprite_n_w) /* sprites # */
	AM_RANGE(0x70, 0x7f) AM_WRITE(tiamc1_sprite_a_w) /* sprites attributes */
	AM_RANGE(0xa0, 0xaf) AM_WRITE(tiamc1_palette_w)  /* color ram */
	AM_RANGE(0xbc, 0xbc) AM_WRITE(tiamc1_bg_hshift_w)/* background H scroll */
	AM_RANGE(0xbd, 0xbd) AM_WRITE(tiamc1_bg_vshift_w)/* background V scroll */
	AM_RANGE(0xbe, 0xbe) AM_WRITE(tiamc1_bankswitch_w) /* VRAM selector */
	AM_RANGE(0xbf, 0xbf) AM_WRITENOP                 /* charset control */
	AM_RANGE(0xc0, 0xc3) AM_WRITE(tiamc1_timer0_w)   /* timer 0 */
	AM_RANGE(0xd2, 0xd2) AM_WRITE(tiamc1_control_w)  /* coin counter and lockout */
	AM_RANGE(0xd3, 0xd3) AM_WRITENOP                 /* 8255 ctrl. Used for i/o ports */
	AM_RANGE(0xd4, 0xd7) AM_WRITE(tiamc1_timer1_w)   /* timer 1 */
	AM_RANGE(0xda, 0xda) AM_WRITE(tiamc1_timer1_gate_w) /* timer 1 gate control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xd0, 0xd0) AM_READ(input_port_0_r)
	AM_RANGE(0xd1, 0xd1) AM_READ(input_port_1_r)
	AM_RANGE(0xd2, 0xd2) AM_READ(input_port_2_r)
ADDRESS_MAP_END

INPUT_PORTS_START( tiamc1 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 0 JOYSTICK_RIGHT */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 2 JOYSTICK_RIGHT */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 3 JOYSTICK_RIGHT */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 0 JOYSTICK_LEFT */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 2 JOYSTICK_LEFT */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 3 JOYSTICK_LEFT */

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 0 JOYSTICK_UP */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 2 JOYSTICK_UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 3 JOYSTICK_UP */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 0 JOYSTICK_DOWN */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Player 2 JOYSTICK_DOWN */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )  /* OUT:coin lockout */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* OUT:game counter */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* RAZR ??? */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Player 1 Kick")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Player 1 Jump")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static const gfx_layout sprites16x16_layout =
{
	16,16,
	256,
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 256*16*8+0, 256*16*8+1, 256*16*8+2, 256*16*8+3, 256*16*8+4, 256*16*8+5, 256*16*8+6, 256*16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static const gfx_layout char_layout =
{
	8,8,
	256,
	4,
	{ 256*8*8*3, 256*8*8*2, 256*8*8*1, 256*8*8*0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ 0, 0x0000, &char_layout, 0, 16 },
	{ REGION_GFX1, 0x0000, &sprites16x16_layout, 0, 16 },
	{ -1 }
};

static struct CustomSound_interface tiamc1_custom_interface =
{
        tiamc1_sh_start
};


static MACHINE_DRIVER_START( tiamc1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080,16000000/9)		 /* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(tiamc1_readmem,tiamc1_writemem)
	MDRV_CPU_IO_MAP(tiamc1_readport,tiamc1_writeport)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(1600))
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_MACHINE_RESET(tiamc1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_PALETTE_INIT(tiamc1)
	MDRV_VIDEO_START(tiamc1)
	MDRV_VIDEO_UPDATE(tiamc1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("2 x 8253", CUSTOM, 16000000/9)
	MDRV_SOUND_CONFIG(tiamc1_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( konek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "g1.d17", 0x00000, 0x2000, CRC(f41d82c9) SHA1(63ac1be2ad58af0e5ef2d33e5c8d790769d80af9) )
	ROM_LOAD( "g2.d17", 0x02000, 0x2000, CRC(b44e7491) SHA1(ff4cb1d76a36f504d670a207ee25556c5faad435) )
	ROM_LOAD( "g3.d17", 0x04000, 0x2000, CRC(91301282) SHA1(cb448a1bb7a9c1768f870a8c062e37807431c9c7) )
	ROM_LOAD( "g4.d17", 0x06000, 0x2000, CRC(3ff0c20b) SHA1(3d999c05b3986149e569630779ed5581fc202842) )
	ROM_LOAD( "g5.d17", 0x08000, 0x2000, CRC(e3196d30) SHA1(a03d9f75926be9fcf5ee05df8b00fbf87361ea5b) )
	ROM_FILL( 0xa000, 0x2000, 0x00 ) /* g6.d17 is unpopulated */
	ROM_LOAD( "g7.d17", 0x0c000, 0x2000, CRC(fe4e9fdd) SHA1(2033585a6c53455d1dafee85cbb807d424ed231d) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "a2.b07", 0x00000, 0x2000, CRC(9eed06ee) SHA1(1b64a3f8fe3df4b4870315dbdf69bf60b1c272d0) )
	ROM_LOAD( "a3.g07", 0x02000, 0x2000, CRC(eeff9b77) SHA1(5dc66292a59f24277a8c2f38158a2e1d58f81338) )
	ROM_LOAD( "a5.l07", 0x04000, 0x2000, CRC(fff9e089) SHA1(f0d64dceaf72da785d55316bf8a7433faa09fabb) )
	ROM_LOAD( "a6.r07", 0x06000, 0x2000, CRC(092e8ee2) SHA1(6c4842e992c592b9f0663e039668f61a7b56700f) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */

ROM_END

GAME( 1988, konek, 0, tiamc1, tiamc1, 0, ROT0, "Terminal", "Konek-Gorbunok", GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
