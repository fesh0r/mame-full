/*

Lord of Gun
IGS, 1994

PCB Layout
----------

IGSPCB NO. T0076
--------------------------------------------------------
| YM3014           62256      IGS008  IGS006   IGST003 |
| YM3812      6295 62256                       IGST002 |
|       3.57945MHz 62256                       IGST001 |
|                  62256                               |
|6116 LORDGUN.100                              IGSB003 |
|     Z80               62256                  IGSB002 |
|LORDGUN.90                                    IGSB001 |
|J    PAL              6116                            |
|A    PAL              6116                       6116 |
|M                          IGS003                6116 |
|M   68000P10 PAL                                 6116 |
|A                          PAL     PAL           6116 |
|                           PAL     6116               |
|                           PAL     6116        IGS007 |
|                           PAL     6116         20MHz |
|       DSW1(4)                     6116 PAL           |
|             62256    62256          IGSA001 IGSA004  |
|      8255          LORDGUN.10       IGSA002 IGSA005  |
|93C46 8255          LORDGUN.4        IGSA003 IGSA006  |
--------------------------------------------------------

HW Notes:
      68k clock: 10.000MHz
      Z80 clock: 5.000MHz
          VSync: 60Hz
          HSync: 15.15kHz
   YM3812 clock: 3.57945MHz
 OKI 6295 clock: 5.000MHz
  OKI 6295 pin7: HI

  All frequencies are checked with my frequency counter (i.e. they are not guessed)

  IGST* are 8M devices
  IGSA* and IGSB* are 16M devices
  LORDGUN.90 is 27C512
  LORDGUN.100 \
  LORDGUN.10  | 27C040
  LORDGUN.4   /


Emulation Notes:

The program roms are slightly encrypted

protection?

ends up going wrong and writing to rom areas etc.

*/

#include "driver.h"
#include "machine/random.h"

VIDEO_START(lordgun)
{
	return 0;
}

VIDEO_UPDATE(lordgun)
{

}

READ16_HANDLER( lordgun_check_r )
{
	return 0x01;
}

READ16_HANDLER( lordgun_random_r )
{
//	return mame_rand();
	return 0x1234;

}

static ADDRESS_MAP_START( lordgun_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x300000, 0x314fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x318000, 0x319fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x31c000, 0x31c7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x400000, 0x4007ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x500000, 0x500fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x506000, 0x506001) AM_READ(lordgun_random_r) // protection device?
	AM_RANGE(0x506004, 0x506005) AM_READ(lordgun_random_r) // protection device?
	AM_RANGE(0x508000, 0x508001) AM_READ(lordgun_check_r)  // protection device?
	AM_RANGE(0x508002, 0x508003) AM_READ(lordgun_random_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lordgun_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x300000, 0x314fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x318000, 0x319fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x31c000, 0x31c7ff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x400000, 0x4007ff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x500000, 0x500fff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x502000, 0x502001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502200, 0x502201) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502400, 0x502401) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502600, 0x502601) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502800, 0x502801) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502a00, 0x502a01) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502c00, 0x502c01) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x502e00, 0x502e01) AM_WRITE(MWA16_NOP)

	AM_RANGE(0x503000, 0x503001) AM_WRITE(MWA16_NOP) // ?
	AM_RANGE(0x506004, 0x506005) AM_WRITE(MWA16_NOP) // protection device?

ADDRESS_MAP_END

INPUT_PORTS_START( lordgun )
INPUT_PORTS_END

static struct GfxLayout lordgun_16x16x6_layout =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(0,3)+0, RGN_FRAC(0,3)+8, RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+8, RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+8, },
	{ 0,1,2,3,4,5,6,7, 256,257,258,259,260,261,262,263 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*32
};

static struct GfxLayout lordgun_8x8x6_layout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(0,3)+0, RGN_FRAC(0,3)+8, RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+8, RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+8,},
	{ 0,1,2,3,4,5,6,7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lordgun_8x8x6_layout,  0x0, 2  },
	{ REGION_GFX2, 0, &lordgun_16x16x6_layout,  0x0, 2  },
	{ REGION_GFX3, 0, &lordgun_16x16x6_layout,  0x0, 2  }, /* later part of this is pretty strange, might just be unused tiles */

	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( lordgun )
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(lordgun_readmem,lordgun_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
//	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)
	/* z80 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(lordgun)
	MDRV_VIDEO_UPDATE(lordgun)
MACHINE_DRIVER_END


DRIVER_INIT( lordgun )
{

	int i;
	data16_t *src = (data16_t *) (memory_region(REGION_CPU1));

	int rom_size = 0x100000;

	for(i=0; i<rom_size/2; i++) {
		data16_t x = src[i];

		if((i & 0x0120) == 0x0100 || (i & 0x0a00) == 0x0800)
			x ^= 0x0010;

		src[i] = x;
	}
}

ROM_START( lordgun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "lordgun.4",  0x00001, 0x80000, CRC(a1a61254) SHA1(b0c5aa656024cfb9be28a11061656159e7b72d00) )
	ROM_LOAD16_BYTE( "lordgun.10", 0x00000, 0x80000, CRC(acda77ef) SHA1(7cd8580419e2f62a3b5a1e4a6020a3ef978ff1e8) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) /* Z80 */
	ROM_LOAD( "lordgun.90",  0x00000, 0x10000, CRC(d59b5e28) SHA1(36696058684d69306f463ed543c8b0195bafa21e) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "lordgun.100", 0x00000, 0x80000, CRC(b4e0fa07) SHA1(f5f33fe3f3a124f4737751fda3ea409fceeec0be) )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )
	ROM_LOAD( "igst001.108", 0x000000, 0x100000, CRC(36dd96f3) SHA1(4e70eb807160e7ed1b19d7f38df3a38021f42d9b) )
	ROM_LOAD( "igst002.114", 0x100000, 0x100000, CRC(816a7665) SHA1(f2f2624ab262c957f84c657cfc432d14c61b19e8) )
	ROM_LOAD( "igst003.119", 0x200000, 0x100000, CRC(cbfee543) SHA1(6fad8ef8d683f709f6ff2b16319447516c372fc8) )

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD( "igsa001.14", 0x000000, 0x200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) )
	ROM_LOAD( "igsa004.13", 0x200000, 0x200000, CRC(52687264) SHA1(28444cf6b5662054e283992857e0827a2ca15b83) )
	ROM_LOAD( "igsa002.9",  0x400000, 0x200000, CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) )
	ROM_LOAD( "igsa005.8",  0x600000, 0x200000, CRC(e32e79e3) SHA1(419f9b501e5a37d763ece9322271e61035b50217) )
	ROM_LOAD( "igsa003.3",  0x800000, 0x200000, CRC(649e48d9) SHA1(ce346154024cf13f3e40000ceeb4c2003cd35894) )
	ROM_LOAD( "igsa006.2",  0xa00000, 0x200000, CRC(39288eb6) SHA1(54d157f0e151f6665f4288b4d09bd65571005132) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD( "igsb001.82", 0x000000, 0x200000, CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) )
	ROM_LOAD( "igsb002.91", 0x200000, 0x200000, CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) )
	ROM_LOAD( "igsb003.97", 0x400000, 0x200000, CRC(6cbf21ac) SHA1(ad25090a00f291aa48929ffa01347cc53e0051f8) )
ROM_END

GAMEX( 1994, lordgun, 0, lordgun, lordgun, lordgun, ROT0, "IGS", "Lord of Gun", GAME_NOT_WORKING | GAME_NO_SOUND  )
