/*****************************************************************************

	Irem M90 system games:

	Hasamu	
	Bomberman

	Uses M72 sound hardware.

	Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy!

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/irem_cpu.h"

void m72_init_sound(void);
void m72_ym2151_irq_handler(int irq);
WRITE_HANDLER( m72_sound_irq_ack_w );
WRITE_HANDLER( m72_sound_command_w );
WRITE_HANDLER( rtype2_sample_addr_w );
WRITE_HANDLER( m72_sample_w );
READ_HANDLER( m72_sample_r );

extern unsigned char *m90_video_data;

void m90_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void m90_bootleg_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( m90_video_control_w );
WRITE_HANDLER( m90_video_w );
int m90_vh_start(void);

static WRITE_HANDLER( m90_coincounter_w )
{
	if (offset==0) {
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		if (data&0xfe) logerror("Coin counter %02x\n",data);
	}
}

/***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x00000, 0x3ffff, MRA_ROM },
	{ 0x60000, 0x60fff, MRA_RAM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xd0000, 0xdffff, MRA_RAM },
	{ 0xe0000, 0xe03ff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x00000, 0x3ffff, MWA_ROM },
	{ 0xa0000, 0xa3fff, MWA_RAM },
	{ 0xd0000, 0xdffff, m90_video_w, &m90_video_data },
	{ 0xe0000, 0xe03ff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress bootleg_readmem[] =
{
	{ 0x00000, 0x3ffff, MRA_ROM },
	{ 0x60000, 0x60fff, MRA_RAM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xd0000, 0xdffff, MRA_RAM },
	{ 0xe0000, 0xe03ff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress bootleg_writemem[] =
{
	{ 0x00000, 0x3ffff, MWA_ROM },
	{ 0x6000e, 0x60fff, MWA_RAM, &spriteram },
	{ 0xa0000, 0xa3fff, MWA_RAM },
	//{ 0xd0000, 0xdffff, m90_bootleg_video_w, &m90_video_data },
	{ 0xe0000, 0xe03ff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r }, /* Player 1 */
	{ 0x01, 0x01, input_port_1_r }, /* Player 2 */
	{ 0x02, 0x02, input_port_2_r }, /* Coins */
	{ 0x03, 0x03, input_port_2_r }, /* Unused?  High byte of above */
	{ 0x04, 0x04, input_port_5_r }, /* Dip 1 */
	{ 0x05, 0x05, input_port_6_r }, /* Dip 2 */
	{ 0x06, 0x06, input_port_3_r }, /* Player 3 */
	{ 0x07, 0x07, input_port_4_r }, /* Player 4 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, m90_coincounter_w },
	{ 0x80, 0x8f, m90_video_control_w },
	{ -1 }	/* end of table */
};

/*****************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, soundlatch_r },
	{ 0x84, 0x84, m72_sample_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x81, rtype2_sample_addr_w },
	{ 0x82, 0x82, m72_sample_w },
	{ 0x83, 0x83, m72_sound_irq_ack_w },
	{ -1 }  /* end of table */
};

/*****************************************************************************/

INPUT_PORTS_START( m90 )
	PORT_START	
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 ) 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) //service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 ) 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )

	PORT_START	
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 ) 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 8*8 characters */
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout, 256, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(90,MIXER_PAN_LEFT,90,MIXER_PAN_RIGHT) },
	{ m72_ym2151_irq_handler },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,	/* 1 channel */
	{ 60 }
};

static int m90_interrupt(void)
{
	return 0x60/4;
}

static struct MachineDriver machine_driver_m90 =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/2,	/* 16 MHz */
			readmem,writemem,readport,writeport,
			m90_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_sound,

	/* video hardware */
//	512, 512, { 0, 511, 0, 511 },
	512, 512, { 80, 511-112, 128+8, 511-128-8 }, /* 320 x 240 */

	gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m90_vh_start,
	0,
	m90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_bootleg =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/2,	/* 16 MHz */
			bootleg_readmem,bootleg_writemem,readport,writeport,
			m90_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_sound,

	/* video hardware */
	320, 240, { 0, 319, 0, 239 },

	gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m90_vh_start,
	0,
	m90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

/***************************************************************************/

ROM_START( hasamu )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "hasc-p1.bin",    0x00000, 0x20000, 0xa7c72f8 )
	ROM_LOAD_V20_ODD ( "hasc-p0.bin",    0x00000, 0x20000, 0xbf137c3 )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "hasc-sp.bin",    0x0000, 0x10000, 0xe693c32f )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "hasc-c0.bin",    0x000000, 0x20000, 0x3c3613af )
	ROM_LOAD( "hasc-c1.bin",    0x020000, 0x20000, 0x0700d406 )
	ROM_LOAD( "hasc-c2.bin",    0x040000, 0x20000, 0x4c7c8bbc )
	ROM_LOAD( "hasc-c3.bin",    0x060000, 0x20000, 0x695d2019 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	/* No samples */
ROM_END

ROM_START( bombrman )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "bbm-p1.bin",    0x00000, 0x20000, 0x982bd166 )
	ROM_LOAD_V20_ODD ( "bbm-p0.bin",    0x00000, 0x20000, 0x0a20afcc )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.bin",    0x0000, 0x10000, 0x251090cd )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "db4-107.bin",   0x000000, 0x40000, 0x3c3613af )
	ROM_LOAD( "db5-106.bin",   0x040000, 0x40000, 0x0700d406 )
	ROM_LOAD( "db6-105.bin",   0x080000, 0x40000, 0x4c7c8bbc )
	ROM_LOAD( "db7-104.bin",   0x0c0000, 0x40000, 0x695d2019 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "bbm-v0.bin",    0x0000, 0x20000, 0x154803cc )
ROM_END

ROM_START( dynablsb )
	ROM_REGION( 0x100000, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "db2-26.bin",    0x00000, 0x20000, 0xa78c72f8 )
	ROM_LOAD_V20_ODD ( "db3-25.bin",    0x00000, 0x20000, 0xbf3137c3 )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
//	ROM_LOAD( "db1-17.bin",    0x0000, 0x10000, 0xe693c32f )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "db4-107.bin",    0x000000, 0x40000, 0x3c3613af )
	ROM_LOAD( "db5-106.bin",    0x040000, 0x40000, 0x0700d406 )
	ROM_LOAD( "db6-105.bin",    0x080000, 0x40000, 0x4c7c8bbc )
	ROM_LOAD( "db7-104.bin",    0x0c0000, 0x40000, 0x695d2019 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	/* Does this have a sample rom? */
ROM_END

static void init_m90(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x3fff0,0x10); /* Start vector */
}

static void init_hasamu(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x3fff0,0x10); /* Start vector */
	irem_cpu_decrypt(0,gunforce_decryption_table);
}

static void init_bombrman(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x3fff0,0x10); /* Start vector */
	irem_cpu_decrypt(0,bomberman_decryption_table);
}

GAME( 1991, hasamu,   0,        m90,     m90, hasamu,   ROT0, "Irem", "Hasamu (Japan)" )
GAME( 1992, bombrman, 0,        m90,     m90, bombrman, ROT0, "Irem (licensed from Hudson Soft)", "Bomberman" )
GAME( 1992, dynablsb, bombrman, bootleg, m90, m90,      ROT0, "bootleg", "Dynablaster (bootleg)" )
