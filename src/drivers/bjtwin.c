/********************************************************************

Bombjack Twin

driver by Mirko Buffoni, Richard Bush


Memory layout:

000000-07FFFF	68k rom
080000-08000F	Inputs, DSW
080010-087FFF	command ram (Audio I/O)
088000-0887FF	15 bit palette ram (RRRRGGGGBBBBRGBx)
									4321^^^^^^^^0^^
									    4321|||| 0|
									    	4321  0

094000			background image number
09C000-09CFFF	Background ram	(8x8 tiles, 256x512)
09D000-09DFFF	mirror for the above
0F0000-0FFFFF	Work ram

----

084014			Flipscreen

0F8000-0F8FFF	Spriteram

This hardware can handle up to 256 sprites.
Each sprite entry occupy 16 bytes, which meaning is:

00-01:	!= 0 -> Sprite enabled
02   :	Hi nibble = composed sprite height (in GFX units)
		Lo nibble = composed sprite width (in GFX units)
06-07:	Sprite number
08-09:	Sprite x position	(lower 9 bits)
0C-0D:	Sprite y position	(lower 9 bits)
0E   :	Sprite color

----

Audio system:  2 x OKI M6295

084000	(RW)	OKI 1 data read/write
084010	(RW)	OKI 2 data read/write

084020	\
084022	|	Selects Bank address for voice 0-3	(OKI 1)
084024	|
084026	/

084028	\
08402a	|	Selects Bank address for voice 0-3	(OKI 2)
08402c	|
08402e	/

----

IRQ1 controls audio output and coin/joysticks reads
I haven't yet figured out why music stops so soon, but it's
surely due to interrupt timing (the hardware is so simple!).

IRQ4 controls DSW reads and vblank.
IRQ2 points to RTE (not used).

----

Test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 1 buttons in this sequence:
	2,2,2, 1,1,1, 2,2,2, 1,1,1
	The release date of this program will appear.

Some code has to be patched out for this to work (see below). The program
remaps button 2 and 3 to button 1, so you can't enter the above sequence.

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *bjtwin_txvideoram;
extern size_t bjtwin_txvideoram_size;


READ_HANDLER( bjtwin_txvideoram_r );
WRITE_HANDLER( bjtwin_txvideoram_w );
WRITE_HANDLER( bjtwin_paletteram_w );
WRITE_HANDLER( bjtwin_flipscreen_w );
WRITE_HANDLER( bjtwin_videocontrol_w );

int  bjtwin_vh_start(void);
void bjtwin_vh_stop(void);
void bjtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



WRITE_HANDLER( bjtwin_oki6295_bankswitch_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		switch (offset)
		{
			case 0x00:	OKIM6295_set_bank_base(0, 0, (data & 0x0f) * 0x10000);	break;
			case 0x02:	OKIM6295_set_bank_base(0, 1, (data & 0x0f) * 0x10000);	break;
			case 0x04:	OKIM6295_set_bank_base(0, 2, (data & 0x0f) * 0x10000);	break;
			case 0x06:	OKIM6295_set_bank_base(0, 3, (data & 0x0f) * 0x10000);	break;
			case 0x08:	OKIM6295_set_bank_base(1, 0, (data & 0x0f) * 0x10000);	break;
			case 0x0a:	OKIM6295_set_bank_base(1, 1, (data & 0x0f) * 0x10000);	break;
			case 0x0c:	OKIM6295_set_bank_base(1, 2, (data & 0x0f) * 0x10000);	break;
			case 0x0e:	OKIM6295_set_bank_base(1, 3, (data & 0x0f) * 0x10000);	break;
		}
	}
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x080001, input_port_0_r },
	{ 0x080002, 0x080003, input_port_1_r },
	{ 0x080008, 0x080009, input_port_2_r },
	{ 0x08000a, 0x08000b, input_port_3_r },
	{ 0x084000, 0x084001, OKIM6295_status_0_r },
	{ 0x084010, 0x084011, OKIM6295_status_1_r },
	{ 0x088000, 0x0887ff, paletteram_word_r },
	{ 0x09c000, 0x09cfff, bjtwin_txvideoram_r },
	{ 0x09d000, 0x09dfff, bjtwin_txvideoram_r },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MRA_BANK1 },
	{ 0x0f8000, 0x0f8fff, MRA_BANK2 },
	{ 0x0f9000, 0x0fffff, MRA_BANK3 },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080014, 0x080015, bjtwin_flipscreen_w },
	{ 0x084000, 0x084001, OKIM6295_data_0_w },
	{ 0x084010, 0x084011, OKIM6295_data_1_w },
	{ 0x084020, 0x08402f, bjtwin_oki6295_bankswitch_w },
	{ 0x088000, 0x0887ff, bjtwin_paletteram_w, &paletteram },
	{ 0x094000, 0x094001, bjtwin_videocontrol_w },
	{ 0x094002, 0x094003, MWA_NOP },	/* IRQ ack? */
	{ 0x09c000, 0x09cfff, bjtwin_txvideoram_w, &bjtwin_txvideoram, &bjtwin_txvideoram_size },
	{ 0x09d000, 0x09dfff, bjtwin_txvideoram_w },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MWA_BANK1 },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA_BANK2, &spriteram, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA_BANK3 },	/* Work RAM again */
	{ -1 }
};



INPUT_PORTS_START( bjtwin )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* shown in service mode, but no effect */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Starting level" )
	PORT_DIPSETTING(    0x08, "Germany" )
	PORT_DIPSETTING(    0x04, "Thailand" )
	PORT_DIPSETTING(    0x0c, "Nevada" )
	PORT_DIPSETTING(    0x0e, "Japan" )
	PORT_DIPSETTING(    0x06, "Korea" )
	PORT_DIPSETTING(    0x0a, "England" )
	PORT_DIPSETTING(    0x02, "Hong Kong" )
	PORT_DIPSETTING(    0x00, "China" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( nouryoku )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x03, 0x03, "Life Decrease Speed" )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static struct GfxDecodeInfo bjtwin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },	/* Chars */
	{ REGION_GFX2, 0, &charlayout,     0, 16 },	/* Tiles */
	{ REGION_GFX3, 0, &spritelayout, 256, 16 },	/* Sprites */
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	2,              					/* 2 chips */
	{ 16000000/4/165, 16000000/4/165 },	/* 24242Hz frequency? */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory region */
	{ 50, 50 }							/* volume */
};



static const struct MachineDriver machine_driver_bjtwin =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz? It's a P12, but xtals are 10MHz and 16MHz */
			readmem,writemem,0,0,
			m68_level4_irq,1,
			m68_level1_irq,112	/* ?? drives music */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	512, 256, { 0*8, 48*8-1, 2*8, 30*8-1 },
	bjtwin_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bjtwin_vh_start,
	bjtwin_vh_stop,
	bjtwin_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



ROM_START( bjtwin )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_EVEN( "bjt.77",  0x00000, 0x40000, 0x7830a465 )	/* 68000 code */
	ROM_LOAD_ODD ( "bjt.76",  0x00000, 0x40000, 0x7cd4e72a )	/* 68000 code */

	ROM_REGION( 0x010000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bjt.35",		0x000000, 0x010000, 0xaa13df7c )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bjt.32",		0x000000, 0x100000, 0x8a4f26d0 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "bjt.100",	0x000000, 0x100000, 0xbb06245d )	/* Sprites */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* 1Mb for ADPCM sounds - sound chip is OKIM6295 */
	ROM_LOAD( "bjt.130",    0x000000, 0x100000, 0x372d46dd )

	ROM_REGION( 0x100000, REGION_SOUND2 )	/* 1Mb for ADPCM sounds - sound chip is OKIM6295 */
	ROM_LOAD( "bjt.127",    0x000000, 0x100000, 0x8da67808 )
ROM_END

ROM_START( nouryoku )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_EVEN( "ic76.1",  0x00000, 0x40000, 0x26075988 )	/* 68000 code */
	ROM_LOAD_ODD ( "ic75.2",  0x00000, 0x40000, 0x75ab82cd )	/* 68000 code */

	ROM_REGION( 0x010000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ic35.3",		0x000000, 0x010000, 0x03d0c3b1 )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ic32.4",		0x000000, 0x200000, 0x88d454fd )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "ic100.5",	0x000000, 0x200000, 0x24d3e24e )	/* Sprites */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* 1Mb for ADPCM sounds - sound chip is OKIM6295 */
	ROM_LOAD( "ic30.6",     0x000000, 0x100000, 0xfeea34f4 )

	ROM_REGION( 0x100000, REGION_SOUND2 )	/* 1Mb for ADPCM sounds - sound chip is OKIM6295 */
	ROM_LOAD( "ic27.7",     0x000000, 0x100000, 0x8a69fded )
ROM_END



static unsigned char decode_byte(unsigned char src, unsigned char *bitp)
{
	unsigned char ret, i;

	ret = 0;
	for (i=0; i<8; i++)
		ret |= (((src >> bitp[i]) & 1) << (7-i));

	return ret;
}

unsigned long bjtwin_address_map_bg0(unsigned long addr)
{
   return ((addr&0x00004)>> 2) | ((addr&0x00800)>> 10) | ((addr&0x40000)>>16);
}


static unsigned short decode_word(unsigned short src, unsigned char *bitp)
{
	unsigned short ret, i;

	ret=0;
	for (i=0; i<16; i++)
		ret |= (((src >> bitp[i]) & 1) << (15-i));

	return ret;
}


unsigned long bjtwin_address_map_sprites(unsigned long addr)
{
   return ((addr&0x00010)>> 4) | ((addr&0x20000)>>16) | ((addr&0x100000)>>18);
}


void init_bjtwin(void)
{
	/* GFX are scrambled.  We decode them here.  (BIG Thanks to Antiriad for descrambling info) */
	unsigned char *rom;

	static unsigned char decode_data_bg[8][8] =
	{
		{0x3,0x0,0x7,0x2,0x5,0x1,0x4,0x6},
		{0x1,0x2,0x6,0x5,0x4,0x0,0x3,0x7},
		{0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0x7,0x6,0x5,0x0,0x1,0x4,0x3,0x2},
		{0x2,0x0,0x1,0x4,0x3,0x5,0x7,0x6},
		{0x5,0x3,0x7,0x0,0x4,0x6,0x2,0x1},
		{0x2,0x7,0x0,0x6,0x5,0x3,0x1,0x4},
		{0x3,0x4,0x7,0x6,0x2,0x0,0x5,0x1},
	};

	static unsigned char decode_data_sprite[8][16] =
	{
		{0x9,0x3,0x4,0x5,0x7,0x1,0xb,0x8,0x0,0xd,0x2,0xc,0xe,0x6,0xf,0xa},
		{0x1,0x3,0xc,0x4,0x0,0xf,0xb,0xa,0x8,0x5,0xe,0x6,0xd,0x2,0x7,0x9},
		{0xf,0xe,0xd,0xc,0xb,0xa,0x9,0x8,0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0xf,0xe,0xc,0x6,0xa,0xb,0x7,0x8,0x9,0x2,0x3,0x4,0x5,0xd,0x1,0x0},

		{0x1,0x6,0x2,0x5,0xf,0x7,0xb,0x9,0xa,0x3,0xd,0xe,0xc,0x4,0x0,0x8}, /* Haze 20/07/00 */
		{0x7,0x5,0xd,0xe,0xb,0xa,0x0,0x1,0x9,0x6,0xc,0x2,0x3,0x4,0x8,0xf}, /* Haze 20/07/00 */
		{0x0,0x5,0x6,0x3,0x9,0xb,0xa,0x7,0x1,0xd,0x2,0xe,0x4,0xc,0x8,0xf}, /* Antiriad, Corrected by Haze 20/07/00 */
		{0x9,0xc,0x4,0x2,0xf,0x0,0xb,0x8,0xa,0xd,0x3,0x6,0x5,0xe,0x1,0x7}, /* Antiriad, Corrected by Haze 20/07/00 */
	};


	int A,i;


	/* background */
	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_bg[bjtwin_address_map_bg0(A)]);
	}

	/* sprites */
	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A += 2)
	{
		unsigned short tmp = decode_word( rom[A+1]*256 + rom[A], decode_data_sprite[bjtwin_address_map_sprites(A)]);
		rom[A+1] = tmp >> 8;
		rom[A] = tmp & 0xff;
	}


	/*	Bombjack Twin uses different banks for each voice of each OKI chip
	 *	Each bank is 0x10000 bytes long, but 24 bit data stored in OKI rom header
	 *	is invalid.  So we void the highest bits of address here.
	 */

	rom = memory_region(REGION_SOUND1);	/* Process OKI 1 ROM */
	for (i=0; i < 0x10; i++)
	{
		for (A=0; A < 0x400; A += 8)
		{
			rom[i*0x10000+A] = 0;
			rom[i*0x10000+A+3] = 0;
		}
	}

	rom = memory_region(REGION_SOUND2);	/* Process OKI 2 ROM */
	for (i=0; i < 0x10; i++)
	{
		for (A=0; A < 0x400; A += 8)
		{
			rom[i*0x10000+A] = 0;
			rom[i*0x10000+A+3] = 0;
		}
	}


	/* Patch rom to enable test mode */

/*	008F54: 33F9 0008 0000 000F FFFC move.w  $80000.l, $ffffc.l
 *	008F5E: 3639 0008 0002           move.w  $80002.l, D3
 *	008F64: 3003                     move.w  D3, D0				\
 *	008F66: 3203                     move.w  D3, D1				|	This code remaps
 *	008F68: 0041 BFBF                ori.w   #-$4041, D1		|   buttons 2 and 3 to
 *	008F6C: E441                     asr.w   #2, D1				|   button 1, so
 *	008F6E: 0040 DFDF                ori.w   #-$2021, D0		|   you can't enter
 *	008F72: E240                     asr.w   #1, D0				|   service mode
 *	008F74: C640                     and.w   D0, D3				|
 *	008F76: C641                     and.w   D1, D3				/
 *	008F78: 33C3 000F FFFE           move.w  D3, $ffffe.l
 *	008F7E: 207C 000F 9000           movea.l #$f9000, A0
 */

//	rom = memory_region(REGION_CPU1);
//	WRITE_WORD(&rom[0x09172], 0x6006);	/* patch checksum error */
//	WRITE_WORD(&rom[0x08f74], 0x4e71);
}



GAME( 1993, bjtwin,   0, bjtwin, bjtwin,   bjtwin, ROT270, "NMK", "Bombjack Twin" )
GAMEX(1995, nouryoku, 0, bjtwin, nouryoku, bjtwin, ROT0, "Tecmo", "Nouryoku Koujou Iinkai", GAME_IMPERFECT_SOUND )
