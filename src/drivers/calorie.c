/*

Calorie-Kun
Sega, 1986

PCB Layout
----------

Top PCB


837-6077  171-5381
|--------------------------------------------|
|                    Z80             YM2149  |
|                                            |
|                    2016            YM2149  |
|                                            |
|10079    10077      10075           YM2149  |
|    10078    10076                          |
|                                           -|
|                                          |
|                    2114                   -|
|                                           1|
|                    2114                   8|
|                                           W|
|                    2114                   A|
|                                           Y|
|                                           -|
|                                          |
|                                    DSW2(8)-|
|           10082   10081   10080            |
|2016                                DSW1(8) |
|--------------------------------------------|

Notes:
      Z80 clock   : 3.000MHz
      YM2149 clock: 1.500MHz
      VSync       : 60Hz
      2016        : 2K x8 SRAM
      2114        : 1K x4 SRAM



Bottom PCB


837-6076  171-5380
|--------------------------------------------|
|                            12MHz           |
|                                            |
|                                            |
|                                            |
|                                            |
|                                            |
|                                            |
| 10074                               10071  |
|               NEC                          |
| 10073      D317-0004   4MHz         10070  |
|                                            |
| 10072                               10069  |
|                                            |
| 2016         2114      6148                |
|                                            |
| 2016         2114      6148                |
|                                            |
|                        6148                |
|                        6148                |
|--------------------------------------------|
Notes:
      317-0004 clock: 4.000MHz
      2016          : 2K x8 SRAM
      2114          : 1K x4 SRAM
      6148          : 1K x4 SRAM




*/

#include "driver.h"


static data8_t*calorie_video;
static data8_t*calorie_video2;
static data8_t*calorie_sprites;


VIDEO_START(calorie)
{
	return 0;
}

/* change to tilemap */
VIDEO_UPDATE( calorie)
{
	int x,y,counter;
	int bg_base=0x0;

	data8_t *src    = memory_region       ( REGION_USER1 );

	fillbitmap(bitmap, get_black_pen(), cliprect);


	if (calorie_video2[0] & 0x10)
	{
		bg_base = (calorie_video2[0]&0x0f)*0x200;

		counter = 0;
		for (y=0;y<16;y++)
		{
			for (x=0;x<16;x++)
			{

				int tileno=(src[bg_base+counter]) | (((src[bg_base+counter+0x100])&0x10)<<4);
				int flipx=((src[bg_base+counter+0x100])&0x40);

				drawgfx(bitmap,Machine->gfx[2],tileno,0,flipx,0,x*16,y*16,cliprect,TRANSPARENCY_NONE,0);
				counter++;

			}
		}
	}

	counter=0;
	for (y=0;y<32;y++)
	{
		for (x=0;x<32;x++)
		{
			int tileno=(((calorie_video[counter+0x400]&0xf0)<<4)|(calorie_video[counter]))&0x7ff;

			drawgfx(bitmap,Machine->gfx[1],tileno,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			counter++;
		}
	}

	for (x=0;x<0x400;x+=4)
	{
		int xpos;
		int ypos;
		int tileno;
		int colr;

		tileno = calorie_sprites[x+0];
		colr = 0xff-calorie_sprites[x+1];
		ypos = 0xff-calorie_sprites[x+2]-16;
		xpos = calorie_sprites[x+3];

		drawgfx(bitmap,Machine->gfx[0],tileno,0,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);


	}



}




static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xcfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xdfff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xcfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(MWA8_RAM) AM_BASE(&calorie_video)
	AM_RANGE(0xd800, 0xdbff) AM_WRITE(MWA8_RAM) AM_BASE(&calorie_sprites )/* sprites? */
	AM_RANGE(0xdc00, 0xddff) AM_WRITE(paletteram_xxxxBBBBGGGGRRRR_w) AM_BASE(&paletteram) /* palette? */
	AM_RANGE(0xde00, 0xdfff) AM_WRITE(MWA8_RAM) AM_BASE(&calorie_video2) /* ? */
ADDRESS_MAP_END


INPUT_PORTS_START( calorieb )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )
INPUT_PORTS_END



static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(1,3),RGN_FRAC(2,3) },
	{ 0, 1,2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(1,3),RGN_FRAC(2,3) },
	{ 0, 1,2, 3, 4, 5, 6, 7, 8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	16*8+0*8,16*8+1*8,16*8+2*8,16*8+3*8,16*8+4*8,16*8+5*8,16*8+6*8,16*8+7*8, },
	8*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles16x16_layout, 0, 16 },
	{ -1 }
};



static MACHINE_DRIVER_START( calorieb )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(calorie)
	MDRV_VIDEO_UPDATE(calorie)
MACHINE_DRIVER_END


ROM_START( calorie )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "epr10072.1j", 0x10000, 0x04000, CRC(ade792c1) SHA1(6ea5afb00a87037d502c17adda7e4060d12680d7) )
	ROM_LOAD( "epr10073.1k", 0x14000, 0x04000, CRC(b53e109f) SHA1(a41c5affe917232e7adf40d7c15cff778b197e90) )

	ROM_LOAD( "epr10074.1m", 0x18000, 0x04000, CRC(a08da685) SHA1(69f9cfebc771312dbb1726350c2d9e9e8c46353f) )
	ROM_RELOAD(0x08000,0x4000)

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "epr10075.4d", 0x0000, 0x04000, CRC(ca547036) SHA1(584a65482f2b92a4c08c37560450d6db68a56c7b) )

	ROM_REGION( 0x10000, REGION_USER1, 0 ) /* tilemaps? */
	ROM_LOAD( "epr10079.8d", 0x0000, 0x02000, CRC(3c61a42c) SHA1(68ea6b5d2f3c6a9e5308c08dde20424f20021a73) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 ) /* sprites */
	ROM_LOAD( "epr10069.7j",  0x0000, 0x04000, CRC(c0c3deaf) SHA1(8bf2e2146b794a330a079dd080f0586500964b1a) )
	ROM_LOAD( "epr10070.7k", 0x4000, 0x04000, CRC(97f35a23) SHA1(869553a334e1b3ba900a8b9c9eaf25fbc6ab31dd) )
	ROM_LOAD( "epr10071.7m", 0x8000, 0x04000, CRC(5f55527a) SHA1(ec1ba8f95ac47a0c783e117ef4af6fe0ab5925b5) )

	ROM_REGION( 0x6000, REGION_GFX2, 0 ) /* tiles  */
	ROM_LOAD( "epr10082.5r", 0x0000, 0x02000, CRC(5984ea44) SHA1(010011b5b8dfa593c6fc7d2366f8cf82fcc8c978) )
	ROM_LOAD( "epr10081.4r", 0x2000, 0x02000, CRC(e2d45dd8) SHA1(5e11089680b574ea4cbf64510e51b0a945f79174) )
	ROM_LOAD( "epr10080.3r", 0x4000, 0x02000, CRC(42edfcfe) SHA1(feba7b1daffcad24d4c24f55ab5466f8cebf31ad) )

	ROM_REGION( 0xc000, REGION_GFX3, 0 ) /* tiles */
	ROM_LOAD( "epr10078.7d", 0x0000, 0x04000, CRC(5b8eecce) SHA1(e7eee82081939b361edcbb9587b072b4b9a162f9) )
	ROM_LOAD( "epr10077.6d", 0x4000, 0x04000, CRC(01bcb609) SHA1(5d01fa75f214d34483284aaaef985ab92a606505) )
	ROM_LOAD( "epr10076.5d", 0x8000, 0x04000, CRC(b1529782) SHA1(8e0e92aae4c8dd8720414372aa767054cc316a0f) )
ROM_END

ROM_START( calorieb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "12.bin", 0x10000, 0x04000, CRC(cf5fa69e) SHA1(520d5652e93a672a1fc147295fbd63b873967885) )
	ROM_CONTINUE(0x00000,0x4000)
	ROM_LOAD( "13.bin", 0x14000, 0x04000, CRC(52e7263f) SHA1(4d684c9e3f08ddb18b0b3b982aba82d3c809a633) )
	ROM_CONTINUE(0x04000,0x4000)
	ROM_LOAD( "epr10074.1m", 0x18000, 0x04000, CRC(a08da685) SHA1(69f9cfebc771312dbb1726350c2d9e9e8c46353f) )
	ROM_RELOAD(0x08000,0x4000)

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "epr10075.4d", 0x0000, 0x04000, CRC(ca547036) SHA1(584a65482f2b92a4c08c37560450d6db68a56c7b) )

	ROM_REGION( 0x10000, REGION_USER1, 0 ) /* tilemaps? */
	ROM_LOAD( "epr10079.8d", 0x0000, 0x02000, CRC(3c61a42c) SHA1(68ea6b5d2f3c6a9e5308c08dde20424f20021a73) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 ) /* sprites */
	ROM_LOAD( "epr10069.7j",  0x0000, 0x04000, CRC(c0c3deaf) SHA1(8bf2e2146b794a330a079dd080f0586500964b1a) )
	ROM_LOAD( "epr10070.7k", 0x4000, 0x04000, CRC(97f35a23) SHA1(869553a334e1b3ba900a8b9c9eaf25fbc6ab31dd) )
	ROM_LOAD( "epr10071.7m", 0x8000, 0x04000, CRC(5f55527a) SHA1(ec1ba8f95ac47a0c783e117ef4af6fe0ab5925b5) )

	ROM_REGION( 0x6000, REGION_GFX2, 0 ) /* tiles  */
	ROM_LOAD( "epr10082.5r", 0x0000, 0x02000, CRC(5984ea44) SHA1(010011b5b8dfa593c6fc7d2366f8cf82fcc8c978) )
	ROM_LOAD( "epr10081.4r", 0x2000, 0x02000, CRC(e2d45dd8) SHA1(5e11089680b574ea4cbf64510e51b0a945f79174) )
	ROM_LOAD( "epr10080.3r", 0x4000, 0x02000, CRC(42edfcfe) SHA1(feba7b1daffcad24d4c24f55ab5466f8cebf31ad) )

	ROM_REGION( 0xc000, REGION_GFX3, 0 ) /* tiles */
	ROM_LOAD( "epr10078.7d", 0x0000, 0x04000, CRC(5b8eecce) SHA1(e7eee82081939b361edcbb9587b072b4b9a162f9) )
	ROM_LOAD( "epr10077.6d", 0x4000, 0x04000, CRC(01bcb609) SHA1(5d01fa75f214d34483284aaaef985ab92a606505) )
	ROM_LOAD( "epr10076.5d", 0x8000, 0x04000, CRC(b1529782) SHA1(8e0e92aae4c8dd8720414372aa767054cc316a0f) )
ROM_END

static DRIVER_INIT( bootleg )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;

	memory_set_opcode_base(0,rom+diff);
}
GAMEX( 1986, calorie, 0, calorieb, calorieb, bootleg, ROT0, "Sega", "Calorie Kun vs Moguranian",GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1986, calorieb, calorie, calorieb, calorieb, bootleg, ROT0, "bootleg", "Calorie Kun vs Moguranian (bootleg)",GAME_NO_SOUND|GAME_NOT_WORKING )
