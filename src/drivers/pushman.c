/***************************************************************************

	Pushman							(c) 1990 Comad

	With 'Debug Mode' on button 2 advances a level, button 3 goes back.

	The microcontroller mainly controls the animation of the enemy robots,
	the communication between the 68000 and MCU is probably not emulated
	100% correct but it works.  Later levels (using the cheat mode) seem
	to have some corrupt tilemaps, I'm not sure if this is a driver bug
	or a game bug from using the cheat mode.	

	Text layer banking is wrong on the continue screen.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"

void pushman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( pushman_scroll_w );
WRITE_HANDLER( pushman_videoram_w );
READ_HANDLER( pushman_videoram_r );
int pushman_vh_start(void);

static UINT8 shared_ram[8];
static UINT16 latch,new_latch=0;

/******************************************************************************/

static WRITE_HANDLER( pushman_control_w )
{
	if (offset==2) soundlatch_w(0,(data>>8)&0xff);
}

static READ_HANDLER( pushman_control_r )
{
	switch (offset)
	{
		case 0: /* Player 1 & 2 controls */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Coins & start buttons */
			return (0xff + (readinputport(2) << 8));

		case 4: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
	}

	return 0xffff;
}

static READ_HANDLER( pushman_68705_r )
{
	if (offset==0)
		return latch;

	if (offset==6 && new_latch) { new_latch=0; return 0; }
	if (offset==6 && !new_latch) return 0xff;

	return (shared_ram[offset+1]<<8)+shared_ram[offset];
}

static WRITE_HANDLER( pushman_68705_w )
{
	shared_ram[offset]=data>>8;
	shared_ram[offset+1]=data&0xff;

	if (offset==2) {
		cpu_cause_interrupt(1,M68705_INT_IRQ);
		cpu_spin();
		new_latch=0;
	}
}

static READ_HANDLER( pushman_68000_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( pushman_68000_w )
{
	if (offset==2 && (shared_ram[2]&2)==0 && data&2) {
		latch=(shared_ram[1]<<8)|shared_ram[0];
		new_latch=1;
	}
	shared_ram[offset]=data;
}

/******************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x060000, 0x060007, pushman_68705_r },
	{ 0xfe0800, 0xfe17ff, MRA_BANK2 },
	{ 0xfe4000, 0xfe4007, pushman_control_r },
	{ 0xfec000, 0xfec7ff, pushman_videoram_r },
	{ 0xff8200, 0xff87ff, paletteram_word_r },
	{ 0xffc000, 0xffffff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x060000, 0x060007, pushman_68705_w },
	{ 0xfe0800, 0xfe17ff, MWA_BANK2, &spriteram },
	{ 0xfe4000, 0xfe4003, pushman_control_w },
	{ 0xfe8000, 0xfe8003, pushman_scroll_w },
	{ 0xfe800e, 0xfe800f, MWA_NOP }, /* ? */
	{ 0xfec000, 0xfec7ff, pushman_videoram_w, &videoram },
	{ 0xff8200, 0xff87ff, paletteram_xxxxRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xffc000, 0xffffff, MWA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress mcu_readmem[] =
{
	{ 0x0000, 0x0007, pushman_68000_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mcu_writemem[] =
{
	{ 0x0000, 0x0007, pushman_68000_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x0fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
};

/******************************************************************************/

INPUT_PORTS_START( pushman )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) 

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_VBLANK ) /* not sure, probably wrong */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) 

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, "Debug Mode" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Level Select" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
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

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	2048,
	4,
	{ 0x20000*8, 0x30000*8, 0x00000*8, 0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout tilelayout =
{
	32,32,
	512, 
	4,     
	{ 4, 0, 0x20000*8+4, 0x20000*8 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8
};

static struct GfxDecodeInfo pushman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &charlayout,      512, 16 },
	{ REGION_GFX2, 0x000000, &spritelayout,    256, 16 },
	{ REGION_GFX3, 0x000000, &tilelayout,        0, 16 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	2000000,
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static int pushman_interrupt(void)
{
	return 2;	/* VBL */
}

static UINT32 amask_m68705 = 0xfff;

static struct MachineDriver machine_driver_pushman =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000, 
			8000000,
			readmem,writemem,0,0,	
			pushman_interrupt,1
		},
		{
			CPU_M68705,
			400000,	/* No idea */
			mcu_readmem,mcu_writemem,0,0,
			ignore_interrupt,1,
			0,0,
			&amask_m68705
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			sound_readmem,sound_writemem,0,sound_writeport,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	60,					/* CPU interleave  */
	0,					/* Hardware initialization-function */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	pushman_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pushman_vh_start,
	0,
	pushman_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

/***************************************************************************/

ROM_START( pushman )
	ROM_REGION( 0x20000, REGION_CPU1 )
	ROM_LOAD_EVEN("12",   0x000000, 0x10000, 0x4251109d )
	ROM_LOAD_ODD ("11",   0x000000, 0x10000, 0x1167ed9f )
 
	ROM_REGION( 0x01000, REGION_CPU2 )
	ROM_LOAD("pushman.uc",0x00000, 0x01000, 0xd7916657 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD("13",        0x00000, 0x08000, 0xbc03827a )

	ROM_REGION( 0x10000, REGION_USER1 )
	ROM_LOAD("10",        0x00000, 0x08000, 0x5f9ae9a1 )

	ROM_REGION( 0x10000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD("01",        0x00000, 0x08000, 0x14497754 )

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD("02",        0x00000, 0x10000, 0x2cb2ac29 )
	ROM_LOAD("03",        0x10000, 0x10000, 0x8ab957c8 )
	ROM_LOAD("04",        0x20000, 0x10000, 0x16e5ce6b )
	ROM_LOAD("05",        0x30000, 0x10000, 0xb82140b8 )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD("06",        0x00000, 0x10000, 0xbd0f9025 )
	ROM_LOAD("08",        0x10000, 0x10000, 0x591bd5c0 )
	ROM_LOAD("07",        0x20000, 0x10000, 0x208cb197 )
	ROM_LOAD("09",        0x30000, 0x10000, 0x77ee8577 )
ROM_END

GAME( 1990, pushman, 0, pushman, pushman, 0, ROT0, "American Sammy Corporation / Comad", "Pushman" )
