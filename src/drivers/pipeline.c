/*
PCB Layout
----------

|----------------------------------------------------------|
|Z8430   ROM2.U84                          ROM3.U32        |
|                                                          |
|                                          ROM4.U31        |
|YM2203  Z80                                               |
|                                          ROM5.U30        |
|        6116                                              |
|                                          6116            |
| 8255                                                     |
|                                          6116   7.3728MHz|
|       Y3014                                              |
|                                                82S129.U10|
|J       82S123.U79                                        |
|A                                               82S129.U9 |
|M                                                         |
|M             6116                                        |
|A                                               ROM6.U8   |
|          ROM1.U77                                        |
| VOL                                            ROM7.U7   |
|               Z80                                        |
| UPC1241H                                       ROM8.U6   |
|              8255                                        |
|                                  6116          ROM9.U5   |
|                                                          |
|           68705R3                6116          ROM10.U4  |
|                                                          |
|DSW1                              6116          ROM11.U3  |
|                                                          |
|    8255                          6116          ROM12.U2  |
|                                                          |
|DSW2                              6116          ROM13.U1  |
|----------------------------------------------------------|
Notes:
      Z80 clocks (both) - 3.6864MHz [7.3728/2]
      Z8430             - zILOG Z8430 z80 CTC Counter Timer Circuit, clock 3.6864MHz [7.3728/2] (DIP28)
      68705R3           - Motorola MC68705R3 Microcontroller, clock 3.6864MHz [7.3728/2] (DIP40)
      YM2203 clock      - 1.8432MHz [7.3728/4]
      6116              - 2K x8 SRAM (DIP24)
      8255              - NEC D8255AC-2 Programmable Peripheral Interface (DIP40)
      VSync             - 60.0Hz
      HSync             - 15.40kHz


*/

#include "driver.h"


VIDEO_START ( pipeline )
{
	return 0;
}

VIDEO_UPDATE ( pipeline)
{

}


INPUT_PORTS_START( pipeline )
INPUT_PORTS_END

static ADDRESS_MAP_START( cpu0_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb800, 0xb8ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu0_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb800, 0xb8ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x0fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x0fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static MACHINE_DRIVER_START( pipeline )
	/* basic machine hardware */

	MDRV_CPU_ADD(Z80, 7372800/2)
	MDRV_CPU_PROGRAM_MAP(cpu0_readmem,cpu0_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 7372800/2)
	MDRV_CPU_PROGRAM_MAP(cpu1_readmem,cpu1_writemem)
//	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(M68705, 7372800/2)
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(pipeline)
	MDRV_VIDEO_UPDATE(pipeline)
MACHINE_DRIVER_END


ROM_START( pipeline )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1.u77", 0x00000, 0x08000, CRC(6e928290) SHA1(e2c8c35c04fd8ce3ddd6ecec04b0193a248e4362) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "rom2.u84", 0x00000, 0x08000, CRC(e77c43b7) SHA1(8b04005bc448083a429ace3319fc7e168a61f2f9) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )
	ROM_LOAD( "68705r3.u74", 0x00000, 0x01000, CRC(0550e87f) SHA1(bc051720ee2b1b871c883ab37140380a7b616445) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "rom3.u32", 0x00000, 0x08000, CRC(d065ca46) SHA1(9fd8bb66735195d1cd20420096438abb5cb3fd54) )
	ROM_LOAD( "rom4.u31", 0x00000, 0x08000, CRC(6dc86355) SHA1(4b73e95726f7f244977634a5a152c90acb4ba89f) )
	ROM_LOAD( "rom5.u30", 0x00000, 0x08000, CRC(93f3f82a) SHA1(bc018370efc67a614ef6efa82526225f0db008ac) )

	ROM_REGION( 0x100000, REGION_USER2, 0 )
	ROM_LOAD( "rom6.u8",  0x00000, 0x20000,CRC(c34bee64) SHA1(2da2dc6e6615ccc2e3e7ca0ceb735a347923a728) )
	ROM_LOAD( "rom7.u7",  0x20000, 0x20000,CRC(23eb84dd) SHA1(ad1d359ba59087a5e786d262194cfe7db9bb0000) )
	ROM_LOAD( "rom8.u6",  0x40000, 0x20000,CRC(4e244c8a) SHA1(6808b2b195601e8041a12fff4b77e487efba015e) )
	ROM_LOAD( "rom9.u5",  0x60000, 0x20000,CRC(07d16ca1) SHA1(caecf98284236af0e1f77566a9c6950491d0902a) )
	ROM_LOAD( "rom10.u4", 0x80000, 0x20000,CRC(9892e465) SHA1(8789d169128bfc8de449bd617601a0f7fe1a19fb) )
	ROM_LOAD( "rom11.u3", 0xa0000, 0x20000,CRC(4e20e82d) SHA1(507b996ab5c8b8fb88f826d808f4b74dfc770db3) )
	ROM_LOAD( "rom12.u2", 0xc0000, 0x20000,CRC(8933c908) SHA1(9f04e454aacda479b6d6dd84dedbf56855f07fca) )
	ROM_LOAD( "rom13.u1", 0xe0000, 0x20000,CRC(611d7e01) SHA1(77e83c51b059f6d64009740d2e7e7ac1c8a6c7ec) )

	ROM_REGION( 0x10000, REGION_USER3, 0 )
	ROM_LOAD( "82s123.u79", 0x00000, 0x00020,CRC(6df3f972) SHA1(0096a7f7452b70cac6c0752cb62e24b643015b5c) )
	ROM_LOAD( "82s129.u10", 0x00000, 0x00100,CRC(e91a1f9e) SHA1(a293023ebe96a5438e89457a98d94beb6dad5418) )
	ROM_LOAD( "82s129.u9",  0x00000, 0x00100,CRC(1cc09f6f) SHA1(35c857e0f3df0dcceec963459978e779e94f76f6) )

ROM_END

GAMEX( 1990, pipeline, 0, pipeline, pipeline, 0, ROT0, "Daehyun Electronics", "Pipeline",GAME_NOT_WORKING|GAME_NO_SOUND )
