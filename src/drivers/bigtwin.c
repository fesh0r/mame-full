/***************************************************************************

Big Twins

driver by Nicola Salmoria

TODO:
- Sound is controlled by a pic16c57 whose ROM is missing.
- Unknown writes to 14 registers: scroll? In particular, 0x510006 changes
  during the game while the image is uncovered.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *bigtwin_bgvideoram,*bigtwin_videoram1,*bigtwin_videoram2;

int bigtwin_vh_start(void);
void bigtwin_vh_stop(void);
WRITE_HANDLER( bigtwin_paletteram_w );
WRITE_HANDLER( bigtwin_bgvideoram_w );
void bigtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static WRITE_HANDLER( coinctrl_w )
{
	if ((data & 0xff000000) == 0)
	{
		coin_counter_w(0,data & 0x0100);
		coin_counter_w(1,data & 0x0200);
	}
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x440000, 0x4403ff, MRA_BANK1 },
	{ 0x700010, 0x700011, input_port_0_r },
	{ 0x700012, 0x700013, input_port_1_r },
	{ 0x700014, 0x700015, input_port_2_r },
	{ 0x70001a, 0x70001b, input_port_3_r },
	{ 0x70001c, 0x70001d, input_port_4_r },
	{ 0xff0000, 0xffffff, MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x304000, 0x304001, MWA_NOP },	/* watchdog? irq ack? */
	{ 0x440000, 0x4403ff, MWA_BANK1, &spriteram, &spriteram_size },
	{ 0x500000, 0x5007ff, MWA_BANK2, &bigtwin_videoram1 },
{ 0x500800, 0x501fff, MWA_NOP },	/* unused RAM? */
	{ 0x502000, 0x503fff, MWA_BANK3, &bigtwin_videoram2 },
{ 0x504000, 0x50ffff, MWA_NOP },	/* unused RAM? */
{ 0x510000, 0x51000d, MWA_NOP },	/* scroll registers? */
	{ 0x600000, 0x67ffff, bigtwin_bgvideoram_w, &bigtwin_bgvideoram },
	{ 0x700016, 0x700017, coinctrl_w },
//	{ 0x70001e, 0x70001f, sound_command_w },
	{ 0x780000, 0x7807ff, bigtwin_paletteram_w, &paletteram },
//	{ 0xe00000, 0xe00001, ?? written on startup
	{ 0xff0000, 0xffffff, MWA_BANK4 },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( bigtwin )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Italian" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Censor Pictures" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* TODO: support coin mode 2 */
	PORT_DIPNAME( 0x1e, 0x1e, "Coinage Mode 1" )
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
#if 0
	PORT_DIPNAME( 0x06, 0x06, "Coin A Mode 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, "Coin B Mode 2" )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
#endif
	PORT_DIPNAME( 0x20, 0x20, "Minimum Credits to Start" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout charlayout8 =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			48*8+0, 48*8+1, 48*8+2, 48*8+3, 48*8+4, 48*8+5, 48*8+6, 48*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			72*8, 73*8, 74*8, 75*8, 76*8, 77*8, 78*8, 79*8 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000,  8 },	/* colors 0x000-0x07f */
	{ REGION_GFX1, 0, &charlayout8,  0x080,  8 },	/* colors 0x080-0x0ff */
	{ REGION_GFX2, 0, &spritelayout, 0x200, 16 },	/* colors 0x200-0x2ff */
	/* background bitmap uses colors 0x100-0x1ff */
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 8000 },			/* 8000Hz frequency? */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};



static struct MachineDriver machine_driver_bigtwin =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz ??? */
			readmem,writemem,0,0,
			m68_level2_irq,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bigtwin_vh_start,
	bigtwin_vh_stop,
	bigtwin_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bigtwin )
	ROM_REGION( 0x100000, REGION_CPU1 )	/* 68000 code */
	ROM_LOAD_EVEN( "bt_02.bin",    0x000000, 0x80000, 0xe6767f60 )
	ROM_LOAD_ODD ( "bt_03.bin",    0x000000, 0x80000, 0x5aba6990 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bt_04.bin",    0x00000, 0x40000, 0x6f628fbc )
	ROM_LOAD( "bt_05.bin",    0x40000, 0x40000, 0x6a9b1752 )
	ROM_LOAD( "bt_06.bin",    0x80000, 0x40000, 0x411cf852 )
	ROM_LOAD( "bt_07.bin",    0xc0000, 0x40000, 0x635c81fd )

	ROM_REGION( 0x80000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bt_08.bin",    0x00000, 0x20000, 0x2749644d )
	ROM_LOAD( "bt_09.bin",    0x20000, 0x20000, 0x1d1897af )
	ROM_LOAD( "bt_10.bin",    0x40000, 0x20000, 0x2a03432e )
	ROM_LOAD( "bt_11.bin",    0x60000, 0x20000, 0x2c980c4c )

	ROM_REGION( 0x40000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "bt_01.bin",    0x00000, 0x40000, 0xff6671dc )
ROM_END



GAMEX( 1995, bigtwin, 0, bigtwin, bigtwin, 0, ROT0_16BIT, "PlayMark", "Big Twin", GAME_NO_COCKTAIL | GAME_NO_SOUND )
