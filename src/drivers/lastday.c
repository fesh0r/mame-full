/***************************************************************************

The Last Day

driver by Nicola Salmoria

TODO:
- sprite/fg priority is not understood (tanks, boats should pass below bridges)
- port A of both of the YM2203 is constantly read and stored in memory -
  function unknown
- video driver is not optimized at all
- when you insert a coin, the deo sprites continue to move in the background.
  Maybe the whole background and sprites are supposed to be disabled.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *lastday_fgvideoram,*lastday_bgscroll,*lastday_fgscroll;

WRITE_HANDLER( lastday_ctrl_w );
void lastday_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static WRITE_HANDLER( lastday_bankswitch_w )
{
 	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x07) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc010, 0xc010, input_port_0_r },
	{ 0xc011, 0xc011, input_port_1_r },
	{ 0xc012, 0xc012, input_port_2_r },
	{ 0xc013, 0xc013, input_port_3_r },	/* DSWA */
	{ 0xc014, 0xc014, input_port_4_r },	/* DSWB */
	{ 0xc800, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc004, MWA_RAM, &lastday_bgscroll },	/* 0 and 1 scroll x; others */
	{ 0xc008, 0xc00c, MWA_RAM, &lastday_fgscroll },	/*                always 0? */
	{ 0xc010, 0xc010, lastday_ctrl_w },	/* coin counter, flip screen */
	{ 0xc011, 0xc011, lastday_bankswitch_w },
	{ 0xc012, 0xc012, soundlatch_w },
	{ 0xc800, 0xcfff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0xd000, 0xdfff, MWA_RAM, &lastday_fgvideoram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf001, 0xf001, YM2203_read_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ 0xf003, 0xf003, YM2203_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( lastday )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )	/* maybe, but I'm not sure */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc2, 0xc2, DEF_STR( Coin_B ) )
//	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x82, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "Every 200000" )
	PORT_DIPSETTING(    0x20, "Every 240000" )
	PORT_DIPSETTING(    0x10, "280000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Speed" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x40, "High" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout =
{
	32,32,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*16+0, 32*16+1, 32*16+2, 32*16+3, 32*16+8+0, 32*16+8+1, 32*16+8+2, 32*16+8+3,
			2*32*16+0, 2*32*16+1, 2*32*16+2, 2*32*16+3, 2*32*16+8+0, 2*32*16+8+1, 2*32*16+8+2, 2*32*16+8+3,
			3*32*16+0, 3*32*16+1, 3*32*16+2, 3*32*16+3, 3*32*16+8+0, 3*32*16+8+1, 3*32*16+8+2, 3*32*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX2, 0, &tilelayout,   768, 16 },
	{ REGION_GFX3, 0, &tilelayout,   512, 16 },
	{ REGION_GFX4, 0, &spritelayout, 256, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER( unk_r )
{
	return 0;
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3500000,	/* ?????? */
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ unk_r, unk_r },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static struct MachineDriver machine_driver_lastday =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ??? */
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* ??? */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	lastday_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};


ROM_START( lastday )
	ROM_REGION( 0x30000, REGION_CPU1 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "lday3.bin",    0x00000, 0x10000, 0xa06dfb1e )
	ROM_RELOAD(               0x10000, 0x10000 )				/* banked at 0x8000-0xbfff */
	ROM_LOAD( "lday4.bin",    0x20000, 0x10000, 0x70961ea6 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2 )	/* sound */
	ROM_LOAD( "lday1.bin",    0x0000, 0x8000, 0xdd4316fd )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x8000, REGION_GFX1 | REGIONFLAG_DISPOSE )	/* chars */
	ROM_LOAD( "lday2.bin",    0x0000, 0x8000, 0x83eb572c )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x80000, REGION_GFX2 | REGIONFLAG_DISPOSE )	/* tiles */
	ROM_LOAD( "lday6.bin",    0x00000, 0x20000, 0x1054361d )
	ROM_LOAD( "lday7.bin",    0x20000, 0x20000, 0x6e57a888 )
	ROM_LOAD( "lday9.bin",    0x40000, 0x20000, 0x6952ef4d )
	ROM_LOAD( "lday10.bin",   0x60000, 0x20000, 0xa5548dca )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )	/* tiles */
	ROM_LOAD( "lday12.bin",   0x00000, 0x20000, 0x992bc4af )
	ROM_LOAD( "lday14.bin",   0x20000, 0x20000, 0xa79abc85 )

	ROM_REGION( 0x40000, REGION_GFX4 | REGIONFLAG_DISPOSE )	/* sprites */
	ROM_LOAD( "lday16.bin",   0x00000, 0x20000, 0xdf503504 )
	ROM_LOAD( "lday15.bin",   0x20000, 0x20000, 0xcd990442 )

	ROM_REGION( 0x20000, REGION_GFX5 )	/* background tilemaps */
	ROM_LOAD( "lday8.bin",    0x00000, 0x10000, 0x92402b9a )
	ROM_LOAD( "lday5.bin",    0x10000, 0x10000, 0x4789bae8 )

	ROM_REGION( 0x20000, REGION_GFX6 )	/* fg tilemaps */
	ROM_LOAD( "lday13.bin",   0x00000, 0x10000, 0x6bdbd887 )
	ROM_LOAD( "lday11.bin",   0x10000, 0x10000, 0x04b961de )
ROM_END

ROM_START( lastdaya )
	ROM_REGION( 0x30000, REGION_CPU1 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "lday3.bin",    0x00000, 0x10000, 0xa06dfb1e )
	ROM_RELOAD(               0x10000, 0x10000 )				/* banked at 0x8000-0xbfff */
	ROM_LOAD( "lday4.bin",    0x20000, 0x10000, 0x70961ea6 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2 )	/* sound */
	ROM_LOAD( "e1",           0x0000, 0x8000, 0xce96e106 )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x8000, REGION_GFX1 | REGIONFLAG_DISPOSE )	/* chars */
	ROM_LOAD( "lday2.bin",    0x0000, 0x8000, 0x83eb572c )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x80000, REGION_GFX2 | REGIONFLAG_DISPOSE )	/* tiles */
	ROM_LOAD( "e6",           0x00000, 0x20000, 0x7623c443 )
	ROM_LOAD( "lday7.bin",    0x20000, 0x20000, 0x6e57a888 )
	ROM_LOAD( "e9",           0x40000, 0x20000, 0x717f6a0e )
	ROM_LOAD( "lday10.bin",   0x60000, 0x20000, 0xa5548dca )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )	/* tiles */
	ROM_LOAD( "lday12.bin",   0x00000, 0x20000, 0x992bc4af )
	ROM_LOAD( "lday14.bin",   0x20000, 0x20000, 0xa79abc85 )

	ROM_REGION( 0x40000, REGION_GFX4 | REGIONFLAG_DISPOSE )	/* sprites */
	ROM_LOAD( "lday16.bin",   0x00000, 0x20000, 0xdf503504 )
	ROM_LOAD( "lday15.bin",   0x20000, 0x20000, 0xcd990442 )

	ROM_REGION( 0x20000, REGION_GFX5 )	/* bg tilemaps */
	ROM_LOAD( "e8",           0x00000, 0x10000, 0xa7b8250b )
	ROM_LOAD( "e5",           0x10000, 0x10000, 0x5f801410 )

	ROM_REGION( 0x20000, REGION_GFX6 )	/* fg tilemaps */
	ROM_LOAD( "lday13.bin",   0x00000, 0x10000, 0x6bdbd887 )
	ROM_LOAD( "lday11.bin",   0x10000, 0x10000, 0x04b961de )
ROM_END


/* The differences between the two sets are only in the sound program and graphics.
   The main program is the same. */
GAME( 1990, lastday,  0,       lastday, lastday, 0, ROT270_16BIT, "Dooyong", "The Last Day (set 1)" )
GAME( 1990, lastdaya, lastday, lastday, lastday, 0, ROT270_16BIT, "Dooyong", "The Last Day (set 2)" )
