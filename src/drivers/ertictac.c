/*********************************************************************
 Erotictac/Tactic [Sisteme 1992]
 (title depends on "Sexy Views" DIP)

 driver by
  Tomasz Slanina
  Steve Ellenoff

TODO:
 - sound
 - get rid of dirty hack in DRIVER_INIT (bug in the core?)
 - is screen double buffered ?
 - flashing text in test mode
 - timer ? ($3200014)
 - proper interrupt timing
 - fix freezes in test mode (depends on int/frame)
   cpu sits in a loop and waiting for change
   at $7c -  but interrupts are disabled.
 - speed - gameplay is way too fast - it depends on int/frame
   (see above). 8 is fixing above bug
   (imo $7c should be modifed only once/frame, but it's dangerous )

**********************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/arm/arm.h"
#include "machine/random.h"

static data32_t *ertictac_mainram;
static data32_t *vram;
static data32_t int_flag;

static READ32_HANDLER(vram_r)
{
	return vram[offset];
}

static WRITE32_HANDLER(vram_w)
{
	int x=(offset%80)<<2;
	int y=(offset/80)&0xff	;
	COMBINE_DATA(&vram[offset]);
	plot_pixel(tmpbitmap,x++,y,vram[offset]&0xff);
	plot_pixel(tmpbitmap,x++,y,(vram[offset]>>8)&0xff);
	plot_pixel(tmpbitmap,x++,y,(vram[offset]>>16)&0xff);
	plot_pixel(tmpbitmap,x,y,(vram[offset]>>24)&0xff);
}

static READ32_HANDLER(random_num_r)
{
	return mame_rand();
}

static READ32_HANDLER(int_r)
{
	if(!int_flag)
		return 0x20; //vblank ?
	if(!int_flag==1)
		return 2; //timer ?
	return mame_rand()&(~0x22);
}

static READ32_HANDLER(int2_r)
{
	if(int_flag>3)
		return 8;
	return mame_rand()&(~8);
}


data32_t unk34tab[256];
static WRITE32_HANDLER(serial_w)
{
	unk34tab[data>>24]=data&0xffffff; //the upper 8 (or 6) bits are some kind of index
}

static ADDRESS_MAP_START( ertictac_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE (&ertictac_mainram)
	AM_RANGE(0x01f00000, 0x01fd7fff) AM_RAM /* unknown */
	AM_RANGE(0x01fd8000, 0x01ffffff) AM_READWRITE(vram_r, vram_w) AM_BASE (&vram)

	AM_RANGE(0x03200000, 0x03200003) AM_WRITENOP AM_READ(random_num_r)
	AM_RANGE(0x03200010, 0x03200013) AM_READ(int2_r)
	AM_RANGE(0x03200014, 0x03200017) AM_WRITENOP AM_READ(random_num_r)  //timer ?
	AM_RANGE(0x03200018, 0x0320001b) AM_NOP
	AM_RANGE(0x03200024, 0x03200027) AM_READ(int_r)
	AM_RANGE(0x03200028, 0x0320002b) AM_NOP
	AM_RANGE(0x03200038, 0x0320003b) AM_NOP
	AM_RANGE(0x03340000, 0x03340003) AM_NOP
	AM_RANGE(0x03340010, 0x03340013) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x03340014, 0x03340017) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x03340018, 0x0334001b) AM_READ(input_port_1_dword_r)

	AM_RANGE(0x033c0004, 0x033c0007) AM_READ(input_port_3_dword_r)
	AM_RANGE(0x033c0008, 0x033c000b) AM_READ(input_port_4_dword_r)
	AM_RANGE(0x033c0010, 0x033c0013) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x033c0014, 0x033c0017) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x033c0018, 0x033c001b) AM_READ(input_port_1_dword_r)

	AM_RANGE(0x03400000, 0x03400003) AM_WRITE(serial_w)

	AM_RANGE(0x03600000, 0x03600003) AM_NOP //unmapped program memory dword write to 03600000 = 03600000 & FFFFFFFF
	AM_RANGE(0x03605000, 0x03605003) AM_NOP //unmapped program memory dword write to 03605000 = 03605000 & FFFFFFFF

	AM_RANGE(0x03800000, 0x038fffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

INPUT_PORTS_START( ertictac )
	PORT_START_TAG("IN 0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN 1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)

	PORT_START_TAG("IN 2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sound" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "Test Mode" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x30, "Game Timing" )
	PORT_DIPSETTING(    0x30, "Normal Game" )
	PORT_DIPSETTING(    0x20, "3.0 Minutes" )
	PORT_DIPSETTING(    0x10, "2.5 Minutes" )
	PORT_DIPSETTING(    0x00, "2.0 Minutes" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x05, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_DIPNAME( 0x0a, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_4C ) )

	PORT_DIPNAME( 0x10, 0x00, "Sexy Views" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

static MACHINE_INIT( ertictac )
{
	ertictac_mainram[0]=0xeae00007; //reset vector
	int_flag=0;
}

static INTERRUPT_GEN( ertictac_interrupt )
{
	int_flag=cpu_getiloops();
	cpunum_set_input_line(0, ARM_IRQ_LINE, HOLD_LINE);
}

PALETTE_INIT(ertictac)
{
	int c;

	for (c = 0; c < 256; c++)
	{
		int r,g,b,i;

		i = c & 0x03;
		r = ((c & 0x04) >> 0) | ((c & 0x10) >> 1) | i;
		g = ((c & 0x20) >> 3) | ((c & 0x40) >> 3) | i;
		b = ((c & 0x08) >> 1) | ((c & 0x80) >> 4) | i;

		palette_set_color(c, r * 0x11, g * 0x11, b * 0x11);
	}
}


static MACHINE_DRIVER_START( ertictac )

	MDRV_CPU_ADD(ARM, 12000000) /* guess */
	MDRV_CPU_PROGRAM_MAP(ertictac_map,0)
	MDRV_CPU_VBLANK_INT(ertictac_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ertictac)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(ertictac)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

ROM_START( ertictac )
	ROM_REGION(0x100000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "01", 0x00000, 0x10000, CRC(8dce677c) SHA1(9f12b1febe796038caa1ecad1d17864dc546cfd8) )
	ROM_LOAD32_BYTE( "02", 0x00001, 0x10000, CRC(7c5c916c) SHA1(d4ed5fc3a7b27253551e7d9176ed9e6513092e60) )
	ROM_LOAD32_BYTE( "03", 0x00002, 0x10000, CRC(edca5ac6) SHA1(f6c4b8030f3c1c93922c5f7232f2159e0471b93a) )
	ROM_LOAD32_BYTE( "04", 0x00003, 0x10000, CRC(959be81c) SHA1(3e8eaacf8809863fd712ad605d23fdda4e904aee) )
	ROM_LOAD32_BYTE( "05", 0x40000, 0x10000, CRC(d08a6c89) SHA1(17b0f5fb2094126146b09d69c91bf413737b0c9e) )
	ROM_LOAD32_BYTE( "06", 0x40001, 0x10000, CRC(d727bcd8) SHA1(4627f8d4d6e6f323445b3ffcfc0d9c699a9a4f89) )
	ROM_LOAD32_BYTE( "07", 0x40002, 0x10000, CRC(23b75de2) SHA1(e18f5339ca2dd362298784ff3e5479d780d823f8) )
	ROM_LOAD32_BYTE( "08", 0x40003, 0x10000, CRC(9448ccdf) SHA1(75fe3c31625f8ba1eedd806b52993e92b1f585b6) )
	ROM_LOAD32_BYTE( "09", 0x80000, 0x10000, CRC(2bfb312e) SHA1(af7a4a1926c9c3d0b3ad41a4729a17383581c070) )
	ROM_LOAD32_BYTE( "10", 0x80001, 0x10000, CRC(e7a05477) SHA1(0ec6f94a35b87e1e4529dea194fce1fde9a5b0ad) )
	ROM_LOAD32_BYTE( "11", 0x80002, 0x10000, CRC(3722591c) SHA1(e0c4075bc4b1c90a6decba3005a1f8298bd61bd1) )
	ROM_LOAD32_BYTE( "12", 0x80003, 0x10000, CRC(fe022b7e) SHA1(056f7283bc54eff555fd1db91410c020fd905063) )
	ROM_LOAD32_BYTE( "13", 0xc0000, 0x10000, CRC(83550842) SHA1(0fee78dbf13ba970e0544c48f8939550f9347822) )
	ROM_LOAD32_BYTE( "14", 0xc0001, 0x10000, CRC(3029567c) SHA1(6d49bea3a3f6f11f4182a602d37b53f1f896c154) )
	ROM_LOAD32_BYTE( "15", 0xc0002, 0x10000, CRC(500997ab) SHA1(028c7b3ca03141e5b596ab1e2ab98d0ccd9bf93a) )
	ROM_LOAD32_BYTE( "16", 0xc0003, 0x10000, CRC(70a8d136) SHA1(50b11f5701ed5b79a5d59c9a3c7d5b7528e66a4d) )
ROM_END

static DRIVER_INIT( ertictac )
{
	((data32_t *)memory_region(REGION_USER1))[0x55]=0;// patched TSTS r11,r15,lsl #32  @ $3800154
}

GAMEX( 1992, ertictac, 0, ertictac, ertictac, ertictac, ROT0, "Sisteme", "Erotictac/Tactic" ,GAME_WRONG_COLORS|GAME_NO_SOUND)
