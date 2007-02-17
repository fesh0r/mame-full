/*

  Limenko POWERSYSTEM2

  driver by Pierpaolo Prazzoli and Tomasz Slanina

  POWERSYSTEM2 General specs:
  - Cartridge Based System
  - Hyperstone E1-32XN CPU
  - QDSP QS1000 Sound Hardware

  Games Supported:
  - Legend of Heroes
  - Super Bubble 2003 (2 sets)

  Known Games Not Dumped:
  - Dynamite Bomber
  - Happy Hunter (shooting themed prize game)

  To Do:
  - QDSP QS1000 sound core
  - Dump QS1003 QDSP wavetable rom
  - Legend of Heroes link up, 2 cabinets can be linked for a 4 player game

*/

#include "driver.h"
#include "machine/eeprom.h"

static tilemap *bg_tilemap, *md_tilemap, *fg_tilemap;
static UINT32 *bg_videoram, *md_videoram, *fg_videoram, *limenko_videoreg;

static UINT32 *mainram;

/*****************************************************************************************************
  MISC FUNCTIONS
*****************************************************************************************************/

static READ32_HANDLER( port2_r )
{
	return (EEPROM_read_bit() << 23) | (readinputport(2) & ~(1<<23));
}

static WRITE32_HANDLER( eeprom_w )
{
	// data & 0x80000 -> video disabled?

	EEPROM_write_bit(data & 0x40000);
	EEPROM_set_cs_line((data & 0x10000) ? CLEAR_LINE : ASSERT_LINE );
	EEPROM_set_clock_line((data & 0x20000) ? ASSERT_LINE : CLEAR_LINE );
}

static WRITE32_HANDLER( limenko_coincounter_w )
{
	coin_counter_w(0,data & 0x10000);
}

static WRITE32_HANDLER( limenko_paletteram_w )
{
	UINT16 paldata;
	COMBINE_DATA(&paletteram32[offset]);

	if(ACCESSING_LSW32)
	{
		paldata = paletteram32[offset] & 0x7fff;
		palette_set_color(Machine, offset * 2 + 1, pal5bit(paldata >> 0), pal5bit(paldata >> 5), pal5bit(paldata >> 10));
	}

	if(ACCESSING_MSW32)
	{
		paldata = (paletteram32[offset] >> 16) & 0x7fff;
		palette_set_color(Machine, offset * 2 + 0, pal5bit(paldata >> 0), pal5bit(paldata >> 5), pal5bit(paldata >> 10));
	}
}

static WRITE32_HANDLER( bg_videoram_w )
{
	UINT32 oldword = bg_videoram[offset];
	COMBINE_DATA(&bg_videoram[offset]);
	if (oldword != bg_videoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE32_HANDLER( md_videoram_w )
{
	UINT32 oldword = md_videoram[offset];
	COMBINE_DATA(&md_videoram[offset]);
	if (oldword != md_videoram[offset])
		tilemap_mark_tile_dirty(md_tilemap,offset);
}

static WRITE32_HANDLER( fg_videoram_w )
{
	UINT32 oldword = fg_videoram[offset];
	COMBINE_DATA(&fg_videoram[offset]);
	if (oldword != fg_videoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

/*****************************************************************************************************
  MEMORY MAPS
*****************************************************************************************************/

static ADDRESS_MAP_START( limenko_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_BASE(&mainram)
	AM_RANGE(0x40000000, 0x403fffff) AM_ROM AM_REGION(REGION_USER2,0)
	AM_RANGE(0x80000000, 0x80007fff) AM_RAM AM_WRITE(fg_videoram_w) AM_BASE(&fg_videoram)
	AM_RANGE(0x80008000, 0x8000ffff) AM_RAM AM_WRITE(md_videoram_w) AM_BASE(&md_videoram)
	AM_RANGE(0x80010000, 0x80017fff) AM_RAM AM_WRITE(bg_videoram_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x80018000, 0x80018fff) AM_RAM AM_BASE(&spriteram32)
	AM_RANGE(0x80019000, 0x80019fff) AM_RAM AM_BASE(&spriteram32_2)
	AM_RANGE(0x8001c000, 0x8001dfff) AM_RAM AM_WRITE(limenko_paletteram_w) AM_BASE(&paletteram32)
	AM_RANGE(0x8001e000, 0x8001ebff) AM_RAM // ? not used
	AM_RANGE(0x8001ffec, 0x8001ffff) AM_RAM AM_BASE(&limenko_videoreg)
	AM_RANGE(0x8003e000, 0x8003e003) AM_WRITENOP // video reg? background pen?
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( limenko_io_map, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0000, 0x0003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x0800, 0x0803) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x1000, 0x1003) AM_READ(port2_r)
	AM_RANGE(0x4000, 0x4003) AM_WRITE(limenko_coincounter_w)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(eeprom_w)
	AM_RANGE(0x5000, 0x5003) AM_WRITENOP // sound latch
ADDRESS_MAP_END


/*****************************************************************************************************
  VIDEO HARDWARE EMULATION
*****************************************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int tile  = bg_videoram[tile_index] & 0x7ffff;
	int color = (bg_videoram[tile_index]>>28) & 0xf;
	SET_TILE_INFO(0,tile,color,0)
}

static void get_md_tile_info(int tile_index)
{
	int tile  = md_videoram[tile_index] & 0x7ffff;
	int color = (md_videoram[tile_index]>>28) & 0xf;
	SET_TILE_INFO(0,tile,color,0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile  = fg_videoram[tile_index] & 0x7ffff;
	int color = (fg_videoram[tile_index]>>28) & 0xf;
	SET_TILE_INFO(0,tile,color,0)
}

// sprites aren't tile based (except for 8x8 ones)
static void limenko_draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int i;
	int sprites_on_screen = (limenko_videoreg[0] & 0x1ff0000) >> 16;

	unsigned char *base_gfx	= memory_region(REGION_GFX1);
	unsigned char *gfx_max	= base_gfx + memory_region_length(REGION_GFX1);

	unsigned char *gfxdata;
	gfx_element gfx;

	for(i = 0; i <= sprites_on_screen*2; i += 2)
	{
		int x, width, flipx, y, height, flipy, code, color, pri;

		x = ((spriteram32[i] & 0x1ff0000) >> 16) - 1;
		width = (((spriteram32[i] & 0xe000000) >> 25) + 1) * 8;
		flipx = spriteram32[i] & 0x10000000;
		y = (spriteram32[i] & 0x1ff) + 1;
		height = (((spriteram32[i] & 0xe00) >> 9) + 1) * 8;
		flipy = spriteram32[i] & 0x1000;
		code = spriteram32[i + 1] & 0x7ffff;
		color = (spriteram32[i + 1] & 0xf0000000) >> 28;

		if(spriteram32[i + 1] & 0x04000000)
			pri = 0xfffe; // below fg
		else
			pri = 0; // above everything

		// spriteram32[i] & 0x80000000 it's always set -> sprite enabled?

		gfxdata	= base_gfx + (64*8/8) * code;

		/* prepare GfxElement on the fly */
		gfx.width = width;
		gfx.height = height;
		gfx.total_elements = 1;
		gfx.color_granularity = 256;
		gfx.colortable = Machine->remapped_colortable;
		gfx.total_colors = 0x10;
		gfx.pen_usage = NULL;
		gfx.gfxdata = gfxdata;
		gfx.line_modulo = width;
		gfx.char_modulo = 0;	/* doesn't matter */
		gfx.flags = 0;

		/* Bounds checking */
		if ( (gfxdata + width * height - 1) >= gfx_max )
			continue;

		pdrawgfx(bitmap,&gfx,
				 0,
				 color,
				 flipx,flipy,
				 x,y,
				 cliprect,TRANSPARENCY_PEN,0,pri);

		// wrap around y
		pdrawgfx(bitmap,&gfx,
				 0,
				 color,
				 flipx,flipy,
				 x,y - 512,
				 cliprect,TRANSPARENCY_PEN,0,pri);

		// wrap around x
		pdrawgfx(bitmap,&gfx,
				 0,
				 color,
				 flipx,flipy,
				 x - 512,y,
				 cliprect,TRANSPARENCY_PEN,0,pri);

		// wrap around x and y
		pdrawgfx(bitmap,&gfx,
				 0,
				 color,
				 flipx,flipy,
				 x - 512,y - 512,
				 cliprect,TRANSPARENCY_PEN,0,pri);
	}
}

VIDEO_START( limenko )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,128,64);
	md_tilemap = tilemap_create(get_md_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);

	tilemap_set_transparent_pen(md_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_UPDATE( limenko )
{
	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_set_enable(bg_tilemap, limenko_videoreg[0] & 4);
	tilemap_set_enable(md_tilemap, limenko_videoreg[0] & 2);
	tilemap_set_enable(fg_tilemap, limenko_videoreg[0] & 1);

	tilemap_set_scrolly(bg_tilemap, 0, limenko_videoreg[3] & 0xffff);
	tilemap_set_scrolly(md_tilemap, 0, limenko_videoreg[2] & 0xffff);
	tilemap_set_scrolly(fg_tilemap, 0, limenko_videoreg[1] & 0xffff);

	tilemap_set_scrollx(bg_tilemap, 0, (limenko_videoreg[3] & 0xffff0000) >> 16);
	tilemap_set_scrollx(md_tilemap, 0, (limenko_videoreg[2] & 0xffff0000) >> 16);
	tilemap_set_scrollx(fg_tilemap, 0, (limenko_videoreg[1] & 0xffff0000) >> 16);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,md_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,1);

	if(limenko_videoreg[0] & 8)
		limenko_draw_sprites(bitmap, cliprect);

	return 0;
}

/*****************************************************************************************************
  INPUT PORTS
*****************************************************************************************************/

INPUT_PORTS_START( legendoh )
	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(3)
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(3)
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(3)
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(4)
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(4)
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(4)
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x00200000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_SPECIAL ) //security bit
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_SPECIAL ) //eeprom
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_DIPNAME( 0x20000000, 0x00000000, "Sound Enable" )
	PORT_DIPSETTING(          0x20000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_SPECIAL ) //changes spriteram location, pointing to spriteram32_2
	PORT_BIT( 0x4000ffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( sb2003 )
	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00ffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00ffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x00200000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_SPECIAL ) //security bit
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_SPECIAL ) //eeprom
	PORT_DIPNAME( 0x20000000, 0x00000000, "Sound Enable" )
	PORT_DIPSETTING(          0x20000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_SPECIAL ) //changes spriteram location, pointing to spriteram32_2
	PORT_BIT( 0x5f10ffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*****************************************************************************************************
  GRAPHICS DECODES
*****************************************************************************************************/


static gfx_layout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24,32,40,48,56 },
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7 },
	64*8,
};

static gfx_decode limenko_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 16 }, /* tiles */
	{ -1 } /* end of array */
};


/*****************************************************************************************************
  MACHINE DRIVERS
*****************************************************************************************************/


static MACHINE_DRIVER_START( limenko )
	MDRV_CPU_ADD(E132XT, 80000000)	//E132XN!
	MDRV_CPU_PROGRAM_MAP(limenko_map,0)
	MDRV_CPU_IO_MAP(limenko_io_map,0)
	MDRV_CPU_VBLANK_INT(irq5_line_hold, 2)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 383, 0, 239)

	MDRV_GFXDECODE(limenko_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(limenko)
	MDRV_VIDEO_UPDATE(limenko)

	/* sound hardware */
MACHINE_DRIVER_END


/*****************************************************************************************************
  ROM LOADING
*****************************************************************************************************/

ROM_START( sb2003 ) /* No specific Country/Region */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP( "sb2003_05.u6", 0x00000000, 0x200000, CRC(8aec4554) SHA1(57a12b142eb7bf08dd1e78d3c79222001bbaa636) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASEFF )
	// u5 empty

	ROM_REGION( 0x220000, REGION_CPU2, 0 ) /* sound cpu + data */
	ROM_LOAD( "07.u16", 0x000000, 0x020000, CRC(78acc607) SHA1(30a1aed40d45233dce88c6114989c71aa0f99ff7) )
	// u17 empty
	ROM_LOAD( "06.u18", 0x020000, 0x200000, CRC(b6ad0d32) SHA1(33e73963ea25e131801dc11f25be6ab18bef03ed) )
	// u19 empty

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "01.u1", 0x000000, 0x200000, CRC(d2c7091a) SHA1(deff050eb0aee89f60d5ad13053e4f1bd4d35961) )
	ROM_LOAD32_BYTE( "02.u2", 0x000001, 0x200000, CRC(a0734195) SHA1(8947f351434e2f750c4bdf936238815baaeb8402) )
	ROM_LOAD32_BYTE( "03.u3", 0x000002, 0x200000, CRC(0f020280) SHA1(2c10baec8dbb201ee5e1c4c9d6b962e2ed02df7d) )
	ROM_LOAD32_BYTE( "04.u4", 0x000003, 0x200000, CRC(fc2222b9) SHA1(c7ee8cffbbee1673a9f107f3f163d029c3900230) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* QDSP wavetable rom */
	ROM_LOAD( "qs1003",  0x000000, 0x200000, NO_DUMP )

	// u20 (S-ROM) empty
ROM_END

ROM_START( sb2003a ) /* Asia Region */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP( "sb2003a_05.u6", 0x000000, 0x200000, CRC(265e45a7) SHA1(b9c8b63aa89c08f3d9d404621e301b122f85389a) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASEFF )
	// u5 empty

	ROM_REGION( 0x220000, REGION_CPU2, 0 ) /* sound cpu + data */
	ROM_LOAD( "07.u16", 0x000000, 0x020000, CRC(78acc607) SHA1(30a1aed40d45233dce88c6114989c71aa0f99ff7) )
	// u17 empty
	ROM_LOAD( "06.u18", 0x020000, 0x200000, CRC(b6ad0d32) SHA1(33e73963ea25e131801dc11f25be6ab18bef03ed) )
	// u19 empty

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "01.u1", 0x000000, 0x200000, CRC(d2c7091a) SHA1(deff050eb0aee89f60d5ad13053e4f1bd4d35961) )
	ROM_LOAD32_BYTE( "02.u2", 0x000001, 0x200000, CRC(a0734195) SHA1(8947f351434e2f750c4bdf936238815baaeb8402) )
	ROM_LOAD32_BYTE( "03.u3", 0x000002, 0x200000, CRC(0f020280) SHA1(2c10baec8dbb201ee5e1c4c9d6b962e2ed02df7d) )
	ROM_LOAD32_BYTE( "04.u4", 0x000003, 0x200000, CRC(fc2222b9) SHA1(c7ee8cffbbee1673a9f107f3f163d029c3900230) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* QDSP wavetable rom */
	ROM_LOAD( "qs1003",  0x000000, 0x200000, NO_DUMP )

	// u20 (S-ROM) empty
ROM_END

/*

Legend Of Heroes
Limenko, 2000

This game runs on a cartridge-based system with Hyperstone E1-32XN CPU and
QDSP QS1000 sound hardware.

PCB Layout
----------

LIMENKO MAIN BOARD SYSTEM
MODEL : LMSYS
REV : LM-003B
SEL : B3-06-00
|-----------------------------------------------------------|
|                                                           |
||-|                 IS61C256    |--------| IS41C16256   |-||
|| |                             |SYS     |              | ||
|| |           |------|          |L2D_HYP |              | ||
|| | QS1003    |QS1000|          |VER1.0  |              | ||
|| |           |      |24kHz     |--------| IC41C16256   | ||
|| |           |------|                                  | ||
|| |                             32MHz     20MHz         | ||
|| |                                                     | ||
|| |              PAL                                    | ||
|| |DA1311                       |--------| IS41C16105   | ||
|| |               IS61C6416     |E1-32XN |              | ||
||-|                             |        |              |-||
|  TL084                         |        |                 |
|                                |--------| IC41C16105      |
|                                                           |
|  TL082         93C46                               PWR_LED|
|                                                    RUN_LED|
|VOL                                                        |
| KIA6280                                           RESET_SW|
|                                                    TEST_SW|
|                                                           |
|---|          JAMMA            |------|    22-WAY      |---|
    |---------------------------|      |----------------|


ROM Board
---------

REV : LMSYS_D
SEL : D2-09-00
|-----------------------------------------------------------|
|   +&*SYS_ROM7              SOU_ROM2      SOU_PRG          |
||-|+&*SYS_ROM8                                          |-||
|| |  +SYS_ROM6              SOU_ROM1                    | ||
|| |  +SYS_ROM5                           +CG_ROM10      | ||
|| |  &SYS_ROM1              CG_ROM12    +*CG_ROM11      | ||
|| |                                                     | ||
|| |                                      +CG_ROM20      | ||
|| |  &SYS_ROM2              CG_ROM22    +*CG_ROM21      | ||
|| |                                                     | ||
|| |                                      +CG_ROM30      | ||
|| |  &SYS_ROM3              CG_ROM32    +*CG_ROM31      | ||
|| |                                                     | ||
||-|                                      +CG_ROM40      |-||
|      SYS_ROM4              CG_ROM42    +*CG_ROM41         |
|-----------------------------------------------------------|
Notes:
      * - These ROMs located on the other side of the PCB
      + - These ROMs surface mounted, type MX29F1610 16MBit SOP44
      & - These locations not populated

Link up 2 cabinets, up to 4 players can play at a time as a team

*/

ROM_START( legendoh )
	ROM_REGION32_BE( 0x200000, REGION_USER1, ROMREGION_ERASEFF ) /* Hyperstone CPU Code */
	/* sys_rom1 empty */
	/* sys_rom2 empty */
	/* sys_rom3 empty */
	ROM_LOAD16_WORD_SWAP( "01.sys_rom4", 0x180000, 0x80000, CRC(49b4a91f) SHA1(21619e8cd0b2fba8c2e08158497575a1760f52c5) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "sys_rom6", 0x000000, 0x200000, CRC(5c13d467) SHA1(ed07b7e1b22293e256787ab079d00c2fb070bf4f) )
	ROM_LOAD16_WORD_SWAP( "sys_rom5", 0x200000, 0x200000, CRC(19dc8d23) SHA1(433687c6aa24b9456436eecb1dcb57814af3009d) )
	/* sys_rom8 empty */
	/* sys_rom7 empty */

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "cg_rom10",     0x0000000, 0x200000, CRC(93a48489) SHA1(a14157d31b4e9c8eb7ebe1b2f1b707ec8c8561a0) )
	ROM_LOAD32_BYTE( "cg_rom20",     0x0000001, 0x200000, CRC(1a6c0258) SHA1(ac7c3b8c2fdfb542103032144a30293d44759fd1) )
	ROM_LOAD32_BYTE( "cg_rom30",     0x0000002, 0x200000, CRC(a0559ef4) SHA1(6622f7107b374c9da816b9814fe93347e7422190) )
	ROM_LOAD32_BYTE( "cg_rom40",     0x0000003, 0x200000, CRC(a607b2b5) SHA1(9a6b867d6a777cbc910b98d505367819e0c20077) )
	ROM_LOAD32_BYTE( "cg_rom11",     0x0800000, 0x200000, CRC(a9fd5a50) SHA1(d15fc4d1697c1505aa98979af09bcfbbc2521145) )
	ROM_LOAD32_BYTE( "cg_rom21",     0x0800001, 0x200000, CRC(b05cdeb2) SHA1(43115146496ee3a820278ffc0b5f0325d6af6335) )
	ROM_LOAD32_BYTE( "cg_rom31",     0x0800002, 0x200000, CRC(a9a0d386) SHA1(501af14ea1af70be4862172701af4850750d3f36) )
	ROM_LOAD32_BYTE( "cg_rom41",     0x0800003, 0x200000, CRC(1c014f45) SHA1(a76246e90b41cc892575f3a3dc26d8d674e3fc3a) )
	ROM_LOAD32_BYTE( "02.cg_rom12",  0x1000000, 0x080000, CRC(8b2e8cbc) SHA1(6ed6db843e27d715e473752dd3853a28bb81a368) )
	ROM_LOAD32_BYTE( "03.cg_rom22",  0x1000001, 0x080000, CRC(a35960c8) SHA1(86914701930512cae81d1ad892d482264f80f695) )
	ROM_LOAD32_BYTE( "04.cg_rom32",  0x1000002, 0x080000, CRC(3f486cab) SHA1(6507d4bb9b4aa7d43f1026e932c82629d4fa44dd) )
	ROM_LOAD32_BYTE( "05.cg_rom42",  0x1000003, 0x080000, CRC(5d807bec) SHA1(c72c77ed0478f705018519cf68a54d22524d05fd) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sounds */
	ROM_LOAD( "sou_prg.06",   0x000000, 0x80000, CRC(bfafe7aa) SHA1(3e65869fe0970bafb59a0225642834042fdedfa6) )
	ROM_LOAD( "sou_rom.07",   0x000000, 0x80000, CRC(4c6eb6d2) SHA1(58bced7bd944e03b0e3dfe1107c01819a33b2b31) )
	ROM_LOAD( "sou_rom.08",   0x000000, 0x80000, CRC(42c32dd5) SHA1(4702771288ba40119de63feb67eed85667235d81) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* QDSP wavetable rom */
	ROM_LOAD( "qs1003",  0x000000, 0x200000, NO_DUMP )
ROM_END


static int irq_active(void)
{
	UINT32 FCR = activecpu_get_reg(27);
	if( !(FCR&(1<<28)) ) // int 1 (irq 5)
		return 1;
	else
		return 0;
}

static READ32_HANDLER( legendoh_speedup_r )
{
	if(activecpu_get_pc() == 0x23e32)
	{
		if(irq_active())
			cpu_spinuntil_int();
		else
			activecpu_eat_cycles(50);
	}

	return mainram[0x32ab0/4];
}

static READ32_HANDLER( sb2003_speedup_r )
{
	if(activecpu_get_pc() == 0x26da4)
	{
		if(irq_active())
			cpu_spinuntil_int();
		else
			activecpu_eat_cycles(50);
	}

	return mainram[0x135800/4];
}

DRIVER_INIT( legendoh )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x32ab0, 0x32ab3, 0, 0, legendoh_speedup_r );
}

DRIVER_INIT( sb2003 )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x135800, 0x135803, 0, 0, sb2003_speedup_r );
}

GAME( 2000, legendoh, 0,      limenko, legendoh, legendoh, ROT0, "Limenko", "Legend of Heroes",          GAME_NO_SOUND )
GAME( 2003, sb2003,   0,      limenko, sb2003,   sb2003,   ROT0, "Limenko", "Super Bubble 2003 (World)", GAME_NO_SOUND )
GAME( 2003, sb2003a,  sb2003, limenko, sb2003,   sb2003,   ROT0, "Limenko", "Super Bubble 2003 (Asia)",  GAME_NO_SOUND )
