/*
	Sega System16 Hardware
	major cleanup in progress - still a lot to do!

	see vidhrdw/system16.c for more information

Changes:

04/28/04  Charles MacDonald
- Added MSM5205 sample playback to tturfbl.

03/17/04
- Added correctly dumped ROM set for eswat to replace the old one. Game is encrypted and unplayable.
- Moved Ace Attacker here from System 18 driver. Game is encrypted and unplayable.
- Added sound support for tturf, tturfu, tturfbl (no samples), fpointbl, fpointbj
- Fixed toryumon RAM test
- Cleaned up timscannr, toryumon drivers
03/11/04
- Cleaned up riotcity, aurail, altbeast, bayroute drivers
- Added missing coin control to sys16_coinctrl_w
- Removed 'extra' RAM in some drivers and replaced with sys16_tilebank_w

To do:
- tturf and tturfu have some invalid data written to the UPD7759. I think this causes the 'pop' sound when level 1 starts.
- tturfbl has some serious problems with tilemap scrolling
- bayroute has a bad sprite in the door of the level 1 boss
- altbeast writes to mirrored sprite RAM at $441000
- tturfu has bad single sprite frame when main character walks
- dduxbl has some 'stuck' sprites

Notes:
- toryumon RAM test accesses mirrored work RAM. Maybe there's a better way to support this than using AM_MASK.
- I separated the fpoint and fpointbl,fpointbj machine drivers as the latter two have different sound hardware,
  but the original does not. I think this creates a dependancy where fpointbl.zip needs flpoint.001 from fpointbj.zip,
  as fpointbl uses fpoint ROMs (it's parent), it's own, and the sound ROM from fpointbj.
  So add fpoint.001 to fpointbl.zip for it to work.
  I made fpointbl the parent of fpointbj so it would use the proper memory map for the sound hardware.
*/

/***************************************************************************/
/*
  ASTORMBL
          3. In the ending, the 3 heroes are floating into a half bubble. (see picture).
          Also colour problems during ending as well.
          4. In the later Shooting gallery stage (like inside the car shop and the factory (mission 3)),
		  there is some garbage graphics (sprite of death monsters that appear where they should not)

	working:
		Alex Kidd
		Alien Storm (bootleg)
		Alien Syndrome
		Altered Beast (Ver 1)
		Altered Beast (Ver 2)	(No Sound)
		Atomic Point			(No Sound)
		Aurail					(Speech quality sounds poor)
		Aurail (317-0168)
		Bay Route
		Body Slam
	    Dump Matsumoto (Japan, Body Slam)
		Dynamite Dux (bootleg)
		Enduro Racer (bootleg)
		Enduro Racer (custom bootleg)
		E-Swat (bootleg)
		Fantasy Zone (Old Ver.)
		Fantasy Zone (New Ver.)
		Flash Point  (bootleg)
		Golden Axe (Ver 1)
		Golden Axe (Ver 2)
		Hang-on
		Heavyweight Champ: some minor graphics glitches
		Major League: No game over.
		Moonwalker (bootleg): Music Speed varies
		Outrun (set 1)
		Outrun (set 2)
		Outrun (custom bootleg)
		Passing Shot (bootleg)
		Passing Shot (4 player bootleg)
		Quartet: Glitch on highscore list
		Quartet (Japan): Glitch on highscore list
		Quartet 2: Glitch on highscore list
		Riot City
		SDI
		Shadow Dancer
		Shadow Dancer (Japan)
		Shinobi
		Shinobi (Sys16A Bootleg?)
		Space Harrier
		Super Hangon (bootleg)
		Tetris (bootleg)
		Time Scanner
		Toryumon
		Tough Turf (Japan)			(No Sound)
		Tough Turf (US)				(No Sound)
		Tough Turf (bootleg)	(No Speech Roms)
		Wonderboy 3 - Monster Lair
		Wonderboy 3 - Monster Lair (bootleg)
		Wrestle War

	not really working:
		Shadow Dancer (bootleg)

	protected:
		Alex Kidd (jpn?)
		Alien Syndrome
		Alien Syndrome
		Alien Syndrome (Japan)
		Alien Storm
		Alien Storm (2 Player)
		Bay Route (317-0116)
		Bay Route (protected bootleg 1)
		Bay Route (protected bootleg 2)
		Enduro Racer
		E-Swat
		Flash Point
		Golden Axe (Ver 1 317-0121 Japan)
		Golden Axe (Ver 2 317-0110)
		Golden Axe (Ver 2 317-0122)
		Golden Axe (protected bootleg)
		Jyuohki (Japan, altered beast)
		Moonwalker (317-0158)
		Moonwalker (317-0159)
		Passing Shot (317-0080)
		Shinobi (Sys16B 317-0049)
		Shinobi (Sys16A 317-0050)
		SDI (Japan, old version)
		Super Hangon
		Tetris (Type A)
		Tetris (Type B 317-0092)
		Wonderboy 3 - Monster Lair (317-0089)

	protected (No driver):
		Ace Attacker
		Action Fighter
		Bloxeed
		Clutch Hitter
		Cotton (Japan)
		Cotton
		DD Crew
		Dunk Shot
		Excite League
		Laser Ghost
		MVP
		Ryukyu
		Super Leagu
		Turbo Outrun
		Turbo Outrun (Set 2)
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"
#include "cpu/m68000/m68000.h"
#include "machine/fd1094.h"

void fd1094_machine_init(void);
void fd1094_driver_init(void);

static void sys16_video_config(void (*update)(void), int sprxoffs, int *bank)
{
	static int bank_default[16] =
	{
		0x0,0x1,0x2,0x3,
		0x4,0x5,0x6,0x7,
		0x8,0x9,0xa,0xb,
		0xc,0xd,0xe,0xf
	};

	sys16_update_proc = update;
	sys16_sprxoffset = sprxoffs;
	sys16_obj_bank = bank ? bank : bank_default;
}

/***************************************************************************/

// 7751 emulation
WRITE8_HANDLER( sys16_7751_audio_8255_w );
 READ8_HANDLER( sys16_7751_audio_8255_r );
 READ8_HANDLER( sys16_7751_sh_rom_r );
 READ8_HANDLER( sys16_7751_sh_t1_r );
 READ8_HANDLER( sys16_7751_sh_command_r );
WRITE8_HANDLER( sys16_7751_sh_dac_w );
WRITE8_HANDLER( sys16_7751_sh_busy_w );
WRITE8_HANDLER( sys16_7751_sh_offset_a0_a3_w );
WRITE8_HANDLER( sys16_7751_sh_offset_a4_a7_w );
WRITE8_HANDLER( sys16_7751_sh_offset_a8_a11_w );
WRITE8_HANDLER( sys16_7751_sh_rom_select_w );

/***************************************************************************/

int sys16_wwfix=0, sys16_alienfix=0; //*

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB )
{
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

static INTERRUPT_GEN( sys16_interrupt )
{
	if(sys16_custom_irq) sys16_custom_irq();
	cpunum_set_input_line(0, 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}



/***************************************************************************/
/*
	Tough Turf (Datsu bootleg) sound emulation

	Memory map

	0000-7fff : ROM (fixed, tt014d68 0000-7fff)
	8000-bfff : ROM (banked)
	e000      : Bank control
	e800      : Sound command latch
	f000      : MSM5205 sample data buffer
	f800-ffff : Work RAM

	Interrupts

	IRQ = Read sound command from $E800
	NMI = Copy data from fixed/banked ROM to $F000

	Bank control values

	00 = tt014d68 8000-bfff
	01 = tt014d68 c000-ffff
	02 = tt0246ff 0000-3fff
	03 = tt0246ff 4000-7fff
	04 = tt0246ff 8000-bfff

	The sample sound codes in the sound test are OK, but in-game sample playback is bad.
	There seems to be more data in the high bits of the ROM bank control word which may be related.
*/

static int sample_buffer = 0;
static int sample_select = 0;

static WRITE8_HANDLER( tturfbl_msm5205_data_w )
{
	sample_buffer = data;
}

static void tturfbl_msm5205_callback(int data)
{
	MSM5205_data_w(0, (sample_buffer >> 4) & 0x0F);
	sample_buffer <<= 4;
	sample_select ^= 1;
	if(sample_select == 0)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static struct MSM5205interface tturfbl_msm5205_interface =
{
	1,
	220000, /* 220KHz */
	{ tturfbl_msm5205_callback },
	{ MSM5205_S48_4B},
	{ 80 }
};


UINT8 *tturfbl_soundbank_ptr = NULL;		/* Pointer to currently selected portion of ROM */

static READ8_HANDLER( tturfbl_soundbank_r )
{
	if(tturfbl_soundbank_ptr) return tturfbl_soundbank_ptr[offset & 0x3fff];
	return 0x80;
}

static WRITE8_HANDLER( tturfbl_soundbank_w )
{
	UINT8 *mem = memory_region(REGION_CPU2);

	switch(data)
	{
		case 0:
			tturfbl_soundbank_ptr = &mem[0x18000]; /* tt014d68 8000-bfff */
			break;
		case 1:
			tturfbl_soundbank_ptr = &mem[0x1C000]; /* tt014d68 c000-ffff */
			break;
		case 2:
			tturfbl_soundbank_ptr = &mem[0x20000]; /* tt0246ff 0000-3fff */
			break;
		case 3:
			tturfbl_soundbank_ptr = &mem[0x24000]; /* tt0246ff 4000-7fff */
			break;
		case 4:
			tturfbl_soundbank_ptr = &mem[0x28000]; /* tt0246ff 8000-bfff */
			break;
		case 8:
			tturfbl_soundbank_ptr = mem;
			break;
		default:
			tturfbl_soundbank_ptr = NULL;
			logerror("Invalid bank setting %02X (%04X)\n", data, activecpu_get_pc());
			break;
	}
}

static ADDRESS_MAP_START( tturfbl_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(tturfbl_soundbank_r)
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START(tturfbl_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_NOP) /* ROM bank */
	AM_RANGE(0xe000, 0xe000) AM_WRITE(tturfbl_soundbank_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(tturfbl_msm5205_data_w)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tturfbl_sound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x80, 0x80) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tturfbl_sound_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x80, 0x80) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

/*******************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
ADDRESS_MAP_END

// 7751 Sound
static ADDRESS_MAP_START( sound_readmem_7751, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport_7751, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
//    AM_RANGE(0x0e, 0x0e) AM_READ(sys16_7751_audio_8255_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport_7751, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(sys16_7751_audio_8255_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_7751, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_7751, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_7751, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(sys16_7751_sh_t1_r)
	AM_RANGE(I8039_p2, I8039_p2) AM_READ(sys16_7751_sh_command_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(sys16_7751_sh_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_7751, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(sys16_7751_sh_dac_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(sys16_7751_sh_busy_w)
	AM_RANGE(I8039_p4, I8039_p4) AM_WRITE(sys16_7751_sh_offset_a0_a3_w)
	AM_RANGE(I8039_p5, I8039_p5) AM_WRITE(sys16_7751_sh_offset_a4_a7_w)
	AM_RANGE(I8039_p6, I8039_p6) AM_WRITE(sys16_7751_sh_offset_a8_a11_w)
	AM_RANGE(I8039_p7, I8039_p7) AM_WRITE(sys16_7751_sh_rom_select_w)
ADDRESS_MAP_END

// 7759
static ADDRESS_MAP_START( sound_readmem_7759, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xdfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xe800, 0xe800) AM_READ(soundlatch_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END


static WRITE8_HANDLER( UPD7759_bank_w ) //*
{
	int offs, size = memory_region_length(REGION_CPU2) - 0x10000;

	UPD7759_reset_w(0, data & 0x40);
	if (sys16_alienfix && (data&0x30)==0x20) data-=2;
	offs = 0x10000 + (data * 0x4000) % size;
	cpu_setbank(1, memory_region(REGION_CPU2) + offs);
}


static ADDRESS_MAP_START( sound_writeport_7759, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(UPD7759_bank_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(UPD7759_0_port_w)
ADDRESS_MAP_END


static WRITE16_HANDLER( sound_command_w )
{
	if( ACCESSING_LSB )
{
		soundlatch_w( 0,data&0xff );
		cpunum_set_input_line( 1, 0, HOLD_LINE );
	}
}

static WRITE16_HANDLER( sound_command_nmi_w )
{
	if( ACCESSING_LSB )
{
		soundlatch_w( 0,data&0xff );
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

//static data16_t coinctrl;

static READ16_HANDLER( sys16_coinctrl_r )
{
	return coinctrl;
}

static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB )
{
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		set_led_status(1,coinctrl & 0x08);
		set_led_status(0,coinctrl & 0x04);
		coin_counter_w(1,coinctrl & 0x02);
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}

/***************************************************************************/

static MACHINE_DRIVER_START( system16 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	MDRV_VIDEO_START(system16)
	MDRV_VIDEO_UPDATE(system16)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("2151", YM2151, sys16_ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( system16_7759 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(sound_readmem_7759,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport_7759)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("7759", UPD7759, sys16_upd7759_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( system16_7751 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(sound_readmem_7751,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport_7751,sound_writeport_7751)

	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(readmem_7751,writemem_7751)
	MDRV_CPU_IO_MAP(readport_7751,writeport_7751)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, sys16_7751_dac_interface)
MACHINE_DRIVER_END

/***************************************************************************/

static WRITE16_HANDLER( sys16_tilebank_w )
{
	if(ACCESSING_LSB)
	{
		switch(offset & 1)
		{
			case 0:
				sys16_tile_bank0 = data & 0x0F;
				break;
			case 1:
				sys16_tile_bank1 = data & 0x0F;
				break;
		}
	}
}

static void set_tile_bank( int data )
{
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}

#if 0
static void set_tile_bank18( int data )
{
	sys16_tile_bank0 = data&0xf;
	sys16_tile_bank1 = (data>>4)&0xf;
}
#endif

static void set_fg_page( int data )
{
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data )
{
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg_page1( int data )
{
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data )
{
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

#if 0
static void set_fg2_page( int data )
{
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data )
{
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}
#endif


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/




/***************************************************************************/

static ADDRESS_MAP_START( alexkidd_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc40002, 0xc40005) AM_READ(MRA16_NOP)		//??
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc60000, 0xc60001) AM_READ(MRA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( alexkidd_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40005) AM_WRITE(MWA16_NOP)		//??
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void alexkidd_update_proc( void )
{
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( alexkidd )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbc;
	sys16_fgxoffset = sys16_bgxoffset = 7;
	sys16_bg_priority_mode=1;

	sys16_update_proc = alexkidd_update_proc;
}

static DRIVER_INIT( alexkidd )
{
	machine_init_sys16_onetime();
}
/***************************************************************************/

INPUT_PORTS_START( alexkidd )
	SYS16_JOY1_SWAPPEDBUTTONS
	SYS16_JOY2_SWAPPEDBUTTONS
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Continues ) )
	PORT_DIPSETTING(    0x01, "Only before level 5" )
	PORT_DIPSETTING(    0x00, "Unlimited" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "240 (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, "20000" )
	PORT_DIPSETTING(    0x10, "40000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Time Adjust" )
	PORT_DIPSETTING(    0x80, "70" )
	PORT_DIPSETTING(    0xc0, "60" )
	PORT_DIPSETTING(    0x40, "50" )
	PORT_DIPSETTING(    0x00, "40" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( alexkidd )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(alexkidd_readmem,alexkidd_writemem)

	MDRV_MACHINE_INIT(alexkidd)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( aliensyn_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aliensyn_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc00006, 0xc00007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc00020, 0xc0003f) AM_WRITE(MWA16_NOP) // config regs
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void aliensyn_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( aliensyn )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;

	sys16_bg_priority_mode=1;
	sys16_fg_priority_mode=1;

	sys16_update_proc = aliensyn_update_proc;

	sys16_alienfix = 1; //*
}

static DRIVER_INIT( aliensyn )
{
	machine_init_sys16_onetime();
	sys16_bg1_trans=1;
}

/***************************************************************************/

INPUT_PORTS_START( aliensyn )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "127 (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, "Timer" )
	PORT_DIPSETTING(    0x00, "120" )
	PORT_DIPSETTING(    0x10, "130" )
	PORT_DIPSETTING(    0x20, "140" )
	PORT_DIPSETTING(    0x30, "150" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
INPUT_PORTS_END

/****************************************************************************/

static MACHINE_DRIVER_START( aliensyn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(aliensyn_readmem,aliensyn_writemem)

	MDRV_MACHINE_INIT(aliensyn)
MACHINE_DRIVER_END


/***************************************************************************/


static ADDRESS_MAP_START( bayroute_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x503fff) AM_READ(SYS16_MRA16_WORKINGRAM)
	AM_RANGE(0x600000, 0x600fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x700000, 0x70ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x710000, 0x710fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x800000, 0x800fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x901002, 0x901003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0x901006, 0x901007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0x901000, 0x901001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0x902002, 0x902003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x902000, 0x902001) AM_READ(input_port_4_word_r) // dip2
ADDRESS_MAP_END

static ADDRESS_MAP_START( bayroute_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100003) AM_WRITE(MWA16_NOP) // tilebank control?
	AM_RANGE(0x500000, 0x503fff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
	AM_RANGE(0x600000, 0x600fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x700000, 0x70ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x710000, 0x710fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x800000, 0x800fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x900000, 0x900001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xff0006, 0xff0007) AM_WRITE(sound_command_w)
	AM_RANGE(0xff0020, 0xff003f) AM_WRITE(MWA16_NOP) // config regs
ADDRESS_MAP_END

/***************************************************************************/

static void bayroute_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( bayroute )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = bayroute_update_proc;
	sys16_spritesystem = sys16_sprite_shinobi;
}

static DRIVER_INIT( bayroute )
{
	machine_init_sys16_onetime();
}

static DRIVER_INIT( bayrouta )
{
	machine_init_sys16_onetime();
}

static DRIVER_INIT( bayrtbl1 )
{
	int i;
	machine_init_sys16_onetime();
	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}
/***************************************************************************/

INPUT_PORTS_START( bayroute )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "Unlimited (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "10000" )
	PORT_DIPSETTING(    0x20, "15000" )
	PORT_DIPSETTING(    0x10, "20000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xc0, "A" )
	PORT_DIPSETTING(    0x80, "B" )
	PORT_DIPSETTING(    0x40, "C" )
	PORT_DIPSETTING(    0x00, "D" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( bayroute )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bayroute_readmem,bayroute_writemem)

	MDRV_MACHINE_INIT(bayroute)
MACHINE_DRIVER_END

/***************************************************************************

   Body Slam

***************************************************************************/

static ADDRESS_MAP_START( bodyslam_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bodyslam_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void bodyslam_update_proc (void)
{
	sys16_fg_scrollx = sys16_textram[0x0ffa/2] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x0ff8/2] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x0f26/2] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x0f24/2] & 0x01ff;

	set_fg_page1( sys16_textram[0x0e9e/2] );
	set_bg_page1( sys16_textram[0x0e9c/2] );
}

static MACHINE_INIT( bodyslam )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbc;
	sys16_fgxoffset = sys16_bgxoffset = 7;
	sys16_bg_priority_mode = 2;
	sys16_bg_priority_value=0x0e00;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0x1f;
	sys16_textlayer_hi_min=0x20;
	sys16_textlayer_hi_max=0xff;

	sys16_update_proc = bodyslam_update_proc;
}

// I have no idea if this is needed, but I cannot find any code for the countdown
// timer in the code and this seems to work ok.
static void bodyslam_irq_timer(void)
{
	int flag=(*(UINT16 *)(&sys16_workingram[0x200/2]))>>8;
	int tick=(*(UINT16 *)(&sys16_workingram[0x200/2]))&0xff;
	int sec=(*(UINT16 *)(&sys16_workingram[0x202/2]))>>8;
	int min=(*(UINT16 *)(&sys16_workingram[0x202/2]))&0xff;

	if(tick == 0 && sec == 0 && min == 0)
		flag=1;
	else
	{
		if(tick==0)
		{
			tick=0x40;	// The game initialise this to 0x40
			if(sec==0)
			{
				sec=0x59;
				if(min==0)
				{
					flag=1;
					tick=sec=min=0;
				}
				else
					min--;
			}
			else
			{
				if((sec&0xf)==0)
				{
					sec-=0x10;
					sec|=9;
				}
				else
					sec--;

			}
		}
		else
			tick--;
	}
	sys16_workingram[0x200/2] = (flag<<8)+tick;
	sys16_workingram[0x202/2] = (sec<<8)+min;
}

static DRIVER_INIT( bodyslam )
{
	machine_init_sys16_onetime();
	sys16_bg1_trans=1;
	sys16_custom_irq=bodyslam_irq_timer;
}

/***************************************************************************/

INPUT_PORTS_START( bodyslam )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
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

/***************************************************************************/

static MACHINE_DRIVER_START( bodyslam )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bodyslam_readmem,bodyslam_writemem)

	MDRV_MACHINE_INIT(bodyslam)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( dduxbl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41004, 0xc41005) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dduxbl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x3f0000, 0x3fffff) AM_WRITE(sys16_tilebank_w)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc40006, 0xc40007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc46000, 0xc4603f) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0xfe0020, 0xfe003f) AM_WRITE(MWA16_NOP) // config regs
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void dduxbl_update_proc( void )
{
	sys16_fg_scrollx = (sys16_extraram2[0x0018/2] ^ 0xffff) & 0x01ff;
	sys16_bg_scrollx = (sys16_extraram2[0x0008/2] ^ 0xffff) & 0x01ff;
	sys16_fg_scrolly = sys16_extraram2[0x0010/2] & 0x00ff;
	sys16_bg_scrolly = sys16_extraram2[0x0000/2];

	{
		unsigned char lu = sys16_extraram2[0x0020/2] & 0xff;
		unsigned char ru = sys16_extraram2[0x0022/2] & 0xff;
		unsigned char ld = sys16_extraram2[0x0024/2] & 0xff;
		unsigned char rd = sys16_extraram2[0x0026/2] & 0xff;

		if (lu==4 && ld==4 && ru==5 && rd==5)
		{ // fix a bug in chicago round (un-tested in MAME)
			int vs=(*(UINT16 *)(&sys16_workingram[0x36ec]));
			sys16_bg_scrolly = vs & 0xff;
			sys16_fg_scrolly = vs & 0xff;
			if (vs >= 0x100)
			{
				lu=0x26; ru=0x37;
				ld=0x04; rd=0x15;
			} else {
				ld=0x26; rd=0x37;
				lu=0x04; ru=0x15;
			}
		}
		sys16_fg_page[0] = ld&0xf;
		sys16_fg_page[1] = rd&0xf;
		sys16_fg_page[2] = lu&0xf;
		sys16_fg_page[3] = ru&0xf;

		sys16_bg_page[0] = ld>>4;
		sys16_bg_page[1] = rd>>4;
		sys16_bg_page[2] = lu>>4;
		sys16_bg_page[3] = ru>>4;
	}
}

static MACHINE_INIT( dduxbl )
{
	sys16_patch_code( 0x1eb2e, 0x01 );
	sys16_patch_code( 0x1eb2f, 0x01 );
	sys16_patch_code( 0x1eb3c, 0x00 );
	sys16_patch_code( 0x1eb3d, 0x00 );
	sys16_patch_code( 0x23132, 0x01 );
	sys16_patch_code( 0x23133, 0x01 );
	sys16_patch_code( 0x23140, 0x00 );
	sys16_patch_code( 0x23141, 0x00 );
	sys16_patch_code( 0x24a9a, 0x01 );
	sys16_patch_code( 0x24a9b, 0x01 );
	sys16_patch_code( 0x24aa8, 0x00 );
	sys16_patch_code( 0x24aa9, 0x00 );
}

static DRIVER_INIT( dduxbl )
{
	static int bank[16] = { //*
		0,0,0,0,
		0,0,0,4,
		0,0,0,3,
		0,2,0,0
	};

	machine_init_sys16_onetime();
	sys16_video_config(dduxbl_update_proc, -0x48, bank);
}
/***************************************************************************/

INPUT_PORTS_START( ddux )
	SYS16_SERVICE
	SYS16_JOY1
	
	PORT_START

	SYS16_JOY2

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "150000" )
	PORT_DIPSETTING(    0x60, "200000" )
	PORT_DIPSETTING(    0x20, "300000" )
	PORT_DIPSETTING(    0x00, "400000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SYS16_COINAGE
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( dduxbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(dduxbl_readmem,dduxbl_writemem)

	MDRV_MACHINE_INIT(dduxbl)
MACHINE_DRIVER_END

/***************************************************************************/

/* in drivers/aburner.c */
WRITE16_HANDLER( math0_product_w );
READ16_HANDLER( math0_product_r );

/* compare chip */
static READ16_HANDLER( eswat_prot1_r )
{
	return 0x4e71;
}

static READ16_HANDLER( eswat_prot2_r )
{
	return 0x4000;
}

static ADDRESS_MAP_START( eswat_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x3e0000, 0x3e001f) AM_READWRITE(math0_product_r, math0_product_w)
	AM_RANGE(0x3e1006, 0x3e1007) AM_READ(eswat_prot2_r)
	AM_RANGE(0x3e100e, 0x3e100f) AM_READ(eswat_prot1_r)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x418fff) AM_READ(SYS16_MRA16_TEXTRAM) //*
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static int eswat_tilebank0;
static int eswat_tilebank1;

static WRITE16_HANDLER( eswat_tilebank0_w )
{
	if( ACCESSING_LSB )
{
		eswat_tilebank0 = data&0xff;
	}
}

static WRITE16_HANDLER( eswat_tilebank1_w )
{
	if( ACCESSING_LSB )
{
		eswat_tilebank1 = data&0xff;
	}
}

static ADDRESS_MAP_START( eswatbl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x3e2000, 0x3e2001) AM_WRITE(eswat_tilebank0_w)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x418fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc42006, 0xc42007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc80000, 0xc80001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END


/***************************************************************************/

static void eswatbl_update_proc( void )
{
	sys16_fg_scrollx = sys16_textram[0x8008/2] ^ 0xffff;
	sys16_bg_scrollx = sys16_textram[0x8018/2] ^ 0xffff;
	sys16_fg_scrolly = sys16_textram[0x8000/2];
	sys16_bg_scrolly = sys16_textram[0x8010/2];

	set_fg_page( sys16_textram[0x8020/2] );
	set_bg_page( sys16_textram[0x8028/2] );

	sys16_tile_bank1 = (sys16_textram[0x8030/2])&0xf;
	sys16_tile_bank0 = eswat_tilebank0;
}

static MACHINE_INIT( eswatbl )
{
	static int bank[] = {
		0,1,	4,5,
		8,9,	12,13,
		2,3,	6,7,
		10,11,	14,15
	};
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0x23c;

	sys16_patch_code( 0x3897, 0x11 );

	sys16_update_proc = eswatbl_update_proc;
}


static DRIVER_INIT( eswatbl )
{
	machine_init_sys16_onetime();
	sys16_rowscroll_scroll=0x8000;
	sys18_splittab_fg_x=&sys16_textram[0x0f80];
}

/***************************************************************************/

INPUT_PORTS_START( eswat )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Display Flip" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Time" )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( eswatbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(eswat_readmem,eswatbl_writemem)

	MDRV_MACHINE_INIT(eswatbl)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( fantzono_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc40000, 0xc40003) AM_READ(SYS16_MRA16_EXTRAM2)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fantzono_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40000, 0xc40003) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0xc60000, 0xc60003) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fantzone_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fantzone_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xc60000, 0xc60003) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void fantzone_update_proc( void )
{
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( fantzono )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_fantzone;
	sys16_sprxoffset = -0xbe;
//	sys16_fgxoffset = sys16_bgxoffset = 8;
	sys16_fg_priority_mode=3;				// fixes end of game priority
	sys16_fg_priority_value=0xd000;

	sys16_patch_code( 0x20e7, 0x16 );
	sys16_patch_code( 0x30ef, 0x16 );

	// solving Fantasy Zone scrolling bug
	sys16_patch_code(0x308f,0x00);

	// invincible
/*	sys16_patch_code(0x224e,0x4e);
	sys16_patch_code(0x224f,0x71);
	sys16_patch_code(0x2250,0x4e);
	sys16_patch_code(0x2251,0x71);

	sys16_patch_code(0x2666,0x4e);
	sys16_patch_code(0x2667,0x71);
	sys16_patch_code(0x2668,0x4e);
	sys16_patch_code(0x2669,0x71);

	sys16_patch_code(0x25c0,0x4e);
	sys16_patch_code(0x25c1,0x71);
	sys16_patch_code(0x25c2,0x4e);
	sys16_patch_code(0x25c3,0x71);
*/

	sys16_update_proc = fantzone_update_proc;
}

static MACHINE_INIT( fantzone )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_fantzone;
	sys16_sprxoffset = -0xbe;
	sys16_fg_priority_mode=3;				// fixes end of game priority
	sys16_fg_priority_value=0xd000;

	sys16_patch_code( 0x2135, 0x16 );
	sys16_patch_code( 0x3649, 0x16 );

	// hack? solving Fantasy Zone scrolling bug
	sys16_patch_code(0x35e9,0x00);

	sys16_update_proc = fantzone_update_proc;
}

static DRIVER_INIT( fantzone )
{
	machine_init_sys16_onetime();
}
/***************************************************************************/

INPUT_PORTS_START( fantzone )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "240 (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, "Extra Ship Cost" )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( fantzono )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(fantzono_readmem,fantzono_writemem)

	MDRV_MACHINE_INIT(fantzono)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fantzone )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(fantzone_readmem,fantzone_writemem)

	MDRV_MACHINE_INIT(fantzone)
MACHINE_DRIVER_END

/***************************************************************************/

static READ16_HANDLER( fp_io_service_dummy_r )
{
	int data = readinputport( 2 ) & 0xff;
	return (data << 8) + data;
}

static ADDRESS_MAP_START( fpointbl_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
//	AM_RANGE(0x02002e, 0x020049) AM_READ(fp_io_service_dummy_r)
	AM_RANGE(0x600006, 0x600007) AM_WRITE(sound_command_w)
	AM_RANGE(0x601000, 0x601001) AM_READ(input_port_0_word_r) // service
	AM_RANGE(0x601002, 0x601003) AM_READ(input_port_1_word_r) // player1
	AM_RANGE(0x601004, 0x601005) AM_READ(input_port_2_word_r) // player2
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0x400000, 0x40ffff) AM_READWRITE(SYS16_MRA16_TILERAM, SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_READWRITE(SYS16_MRA16_TEXTRAM, SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_READWRITE(SYS16_MRA16_SPRITERAM, SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
//	AM_RANGE(0x44302a, 0x44304d) AM_READ(fp_io_service_dummy_r)
	AM_RANGE(0x840000, 0x840fff) AM_READWRITE(SYS16_MRA16_PALETTERAM, SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
//	AM_RANGE(0xfe0006, 0xfe0007) AM_WRITE(sound_command_w) // original
//	AM_RANGE(0xfe003e, 0xfe003f) AM_READ(fp_io_service_dummy_r)
	AM_RANGE(0xffc000, 0xffffff) AM_READWRITE(SYS16_MRA16_WORKINGRAM, SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void fpoint_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( fpointbl )
{
	sys16_patch_code( 0x454, 0x33 );
	sys16_patch_code( 0x455, 0xf8 );
	sys16_patch_code( 0x456, 0xe0 );
	sys16_patch_code( 0x457, 0xe2 );
	sys16_patch_code( 0x8ce8, 0x16 );
	sys16_patch_code( 0x8ce9, 0x66 );
	sys16_patch_code( 0x17687, 0x00 );
	sys16_patch_code( 0x7bed, 0x04 );

	sys16_patch_code( 0x7ea8, 0x61 );
	sys16_patch_code( 0x7ea9, 0x00 );
	sys16_patch_code( 0x7eaa, 0x84 );
	sys16_patch_code( 0x7eab, 0x16 );
	sys16_patch_code( 0x2c0, 0xe7 );
	sys16_patch_code( 0x2c1, 0x48 );
	sys16_patch_code( 0x2c2, 0xe7 );
	sys16_patch_code( 0x2c3, 0x49 );
	sys16_patch_code( 0x2c4, 0x04 );
	sys16_patch_code( 0x2c5, 0x40 );
	sys16_patch_code( 0x2c6, 0x00 );
	sys16_patch_code( 0x2c7, 0x10 );
	sys16_patch_code( 0x2c8, 0x4e );
	sys16_patch_code( 0x2c9, 0x75 );

	sys16_update_proc = fpoint_update_proc;
}

static DRIVER_INIT( fpointbl )
{
	machine_init_sys16_onetime();
	sys16_video_config(fpoint_update_proc, -0xb8, NULL);
}
/***************************************************************************/

INPUT_PORTS_START( fpoint )
	SYS16_SERVICE

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, "Clear round allowed" ) /* Use button 3 */
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SYS16_COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( fpointbj )
	SYS16_SERVICE

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Cell Move Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	SYS16_COINAGE
INPUT_PORTS_END

/***************************************************************************/

/*
	Flash Point (Datsu bootlegs = fpointbl, fpointbj)
	Has sound latch at $E000 instead of I/O ports $C0-FF
*/
static ADDRESS_MAP_START( fpointbl_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xe000) AM_READ(soundlatch_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( fpointbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(fpointbl_map,0)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(fpointbl_sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(fpointbl)
MACHINE_DRIVER_END

/***************************************************************************/

static READ16_HANDLER( ga_io_players_r ) {
	return (readinputport(0) << 8) | readinputport(1);
}
static READ16_HANDLER( ga_io_service_r )
{
	return (input_port_2_word_r(0,0) << 8) | (sys16_workingram[0x2c96/2] & 0x00ff);
}

static ADDRESS_MAP_START( goldnaxe_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x140000, 0x140fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x1f0000, 0x1f0003) AM_READ(SYS16_MRA16_EXTRAM)
	AM_RANGE(0x200000, 0x200fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffecd0, 0xffecd1) AM_READ(ga_io_players_r)
	AM_RANGE(0xffec96, 0xffec97) AM_READ(ga_io_service_r)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static WRITE16_HANDLER( ga_sound_command_w )
{
	COMBINE_DATA( &sys16_workingram[(0xecfc-0xc000)/2] );
	if( ACCESSING_MSB )
{
		soundlatch_w( 0,data>>8 );
		cpunum_set_input_line( 1, 0, HOLD_LINE );
	}
}

static WRITE16_HANDLER( goldnaxe_prot_w )
{
	sys16_workingram[(0xecd8 - 0xc000)/2] = 0x048c;
	sys16_workingram[(0xecda - 0xc000)/2] = 0x159d;
	sys16_workingram[(0xecdc - 0xc000)/2] = 0x26ae;
	sys16_workingram[(0xecde - 0xc000)/2] = 0x37bf;
	COMBINE_DATA( &sys16_workingram[(0xec1c-0xc000)/2] );
}

static ADDRESS_MAP_START( goldnaxe_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x140000, 0x140fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x1f0000, 0x1f0003) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc43000, 0xc43001) AM_WRITE(MWA16_NOP) // ?
//	AM_RANGE(0xfe0006, 0xfe0007) AM_WRITE(MWA16_NOP) I think this is the real sound out
	AM_RANGE(0xffec1c, 0xffec1d) AM_WRITE(goldnaxe_prot_w)// how does this really work?
	AM_RANGE(0xffecfc, 0xffecfd) AM_WRITE(ga_sound_command_w)// probably just a buffer
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram) /* fails SCRATCH RAM test because of hacks */
//	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(MWA15_NOP) /* 0x400 bytes; battery backed up */
ADDRESS_MAP_END

/***************************************************************************/

static void goldnaxe_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];

	set_tile_bank( sys16_workingram[0x2c94/2] );
}

static MACHINE_INIT( goldnaxe )
{
	static int bank[16] = {
		0,1,4,5,
		8,9,0,0,
		2,3,6,7,
		10,11,0,0
	};
	sys16_obj_bank = bank;

// protection patch; no longer needed
//	sys16_patch_code( 0x3CB2, 0x60 );
//	sys16_patch_code( 0x3CB3, 0x1e );

	sys16_sprxoffset = -0xb8;
	sys16_update_proc = goldnaxe_update_proc;
}

static DRIVER_INIT( goldnaxe )
{
	machine_init_sys16_onetime();
}

static DRIVER_INIT( goldnabl )
{
	int i;

	machine_init_sys16_onetime();

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x60000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

INPUT_PORTS_START( goldnaxe )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Credits needed" )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue" )
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( goldnaxe )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(goldnaxe_readmem,goldnaxe_writemem)

	MDRV_MACHINE_INIT(goldnaxe)
MACHINE_DRIVER_END

/***************************************************************************/

// This version has somekind of hardware comparitor for collision detection,
// and a hardware multiplier.
static data16_t *ga_hardware_collision_data;
static WRITE16_HANDLER( ga_hardware_collision_w )
{
	static int bit=1;

	COMBINE_DATA( &ga_hardware_collision_data[offset] );
	if( offset==4/2 )
{
		if( ga_hardware_collision_data[2] <= ga_hardware_collision_data[0] &&
			ga_hardware_collision_data[2] >= ga_hardware_collision_data[1])
		{
			ga_hardware_collision_data[4] |=bit;
		}
		bit=bit<<1;
	}
	else if( offset==8/2 ) bit=1;
}

static READ16_HANDLER( ga_hardware_collision_r )
{
	return ga_hardware_collision_data[4];
}

static data16_t *ga_hardware_multiplier_data;
static WRITE16_HANDLER( ga_hardware_multiplier_w )
{
	COMBINE_DATA( &ga_hardware_multiplier_data[offset] );
}

static READ16_HANDLER( ga_hardware_multiplier_r )
{
	if(offset==6/2)
		return ga_hardware_multiplier_data[0] * ga_hardware_multiplier_data[1];
	else
		return ga_hardware_multiplier_data[offset];
}

static ADDRESS_MAP_START( goldnaxa_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x140000, 0x140fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x1e0008, 0x1e0009) AM_READ(ga_hardware_collision_r)
	AM_RANGE(0x1f0000, 0x1f0007) AM_READ(ga_hardware_multiplier_r)
	AM_RANGE(0x1f1008, 0x1f1009) AM_READ(ga_hardware_collision_r)
	AM_RANGE(0x1f2000, 0x1f2003) AM_READ(SYS16_MRA16_EXTRAM)
	AM_RANGE(0x200000, 0x200fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffecd0, 0xffecd1) AM_READ(ga_io_players_r)
	AM_RANGE(0xffec96, 0xffec97) AM_READ(ga_io_service_r)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( goldnaxa_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x140000, 0x140fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x1e0000, 0x1e0009) AM_WRITE(ga_hardware_collision_w) AM_BASE(&ga_hardware_collision_data)
	AM_RANGE(0x1f0000, 0x1f0003) AM_WRITE(ga_hardware_multiplier_w) AM_BASE(&ga_hardware_multiplier_data)
	AM_RANGE(0x1f1000, 0x1f1009) AM_WRITE(ga_hardware_collision_w)
	AM_RANGE(0x1f2000, 0x1f2003) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xffecfc, 0xffecfd) AM_WRITE(ga_sound_command_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void goldnaxa_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];

	set_tile_bank( sys16_workingram[0x2c94/2] );
}

static MACHINE_INIT( goldnaxa )
{
	static int bank[16] = {
		0,1,4,5,
		8,9,0,0,
		2,3,6,7,
		10,11,0,0
	};
	sys16_obj_bank = bank;
	//?
	sys16_patch_code( 0x3CA2, 0x60 );
	sys16_patch_code( 0x3CA3, 0x1e );
	sys16_sprxoffset = -0xb8;
	sys16_update_proc = goldnaxa_update_proc;
}

/***************************************************************************/

static MACHINE_DRIVER_START( goldnaxa )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(goldnaxa_readmem,goldnaxa_writemem)

	MDRV_MACHINE_INIT(goldnaxa)
MACHINE_DRIVER_END

/***************************************************************************/

static READ16_HANDLER( mjl_io_player1_r )
{
	data16_t data=input_port_0_r( offset ) & 0x80;

	if( sys16_extraram2[2/2] & 0x4 )
		data|=(input_port_5_r( offset ) & 0x3f) << 1;
	else
		data|=(input_port_6_r( offset ) & 0x3f) << 1;

	return data;
}

static READ16_HANDLER( mjl_io_service_r )
{
	data16_t data=input_port_2_r( offset ) & 0x3f;

	if(sys16_extraram2[2/2] & 0x4)
{
		data|=(input_port_5_r( offset ) & 0x40);
		data|=(input_port_7_r( offset ) & 0x40) << 1;
	}
	else {
		data|=(input_port_6_r( offset ) & 0x40);
		data|=(input_port_8_r( offset ) & 0x40) << 1;
	}

	return data;
}

static READ16_HANDLER( mjl_io_player2_r )
{
	data16_t data=input_port_1_r( offset ) & 0x80;
	if(sys16_extraram2[2/2] & 0x4)
		data|=(input_port_7_r( offset ) & 0x3f) << 1;
	else
		data|=(input_port_8_r( offset ) & 0x3f) << 1;
	return data;
}

static READ16_HANDLER( mjl_io_bat_r )
{
	int data1=input_port_0_r( offset );
	int data2=input_port_1_r( offset );
	int ret=0;

	// Hitting has 8 values, but for easy of playing, I've only added 3

	if(data1 &1) ret=0x00;
	else if(data1 &2) ret=0x03;
	else if(data1 &4) ret=0x07;
	else ret=0x0f;

	if(data2 &1) ret|=0x00;
	else if(data2 &2) ret|=0x30;
	else if(data2 &4) ret|=0x70;
	else ret|=0xf0;

	return ret;
}

static ADDRESS_MAP_START( mjleague_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)

	/* ?? these are just plain broken, they need 'extraram' but it isn't mapped! */
//	AM_RANGE(0xc40002, 0xc40003) AM_READ(sys16_coinctrl_r)
//	AM_RANGE(0xc41000, 0xc41001) AM_READ(mjl_io_service_r)
//	AM_RANGE(0xc41002, 0xc41003) AM_READ(mjl_io_player1_r)
//	AM_RANGE(0xc41006, 0xc41007) AM_READ(mjl_io_player2_r)
//	AM_RANGE(0xc41004, 0xc41005) AM_READ(mjl_io_bat_r)
//	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
//	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2

	AM_RANGE(0xc40002, 0xc40003) AM_READ(sys16_coinctrl_r)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2

	AM_RANGE(0xc60000, 0xc60001) AM_READ(MRA16_NOP) /* What is this? Watchdog? */

	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjleague_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void mjleague_update_proc( void )
{
	set_bg_page1( sys16_textram[0x746] );
	set_fg_page1( sys16_textram[0x747] );

	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( mjleague )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 7;

	// remove memory test because it fails.
	sys16_patch_code( 0xBD42, 0x66 );

	sys16_update_proc = mjleague_update_proc;
}

static DRIVER_INIT( mjleague )
{
	machine_init_sys16_onetime();
}

/***************************************************************************/

INPUT_PORTS_START( mjleague )

	PORT_START_TAG("IN0") /* player 1 button fake */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 )

	PORT_START_TAG("IN1") /* player 1 button fake */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")  /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Points" )
	PORT_DIPSETTING(    0x0c, "2000" )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x04, "5000" )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPNAME( 0x10, 0x10, "Team Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	//??? something to do with cocktail mode?
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN5")
	PORT_BIT( 0x7f, 0x40, IPT_TRACKBALL_Y ) PORT_MINMAX(0,127) PORT_SENSITIVITY(70) PORT_KEYDELTA(30)

	PORT_START_TAG("IN6")
	PORT_BIT( 0x7f, 0x40, IPT_TRACKBALL_X ) PORT_MINMAX(0,127) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START_TAG("IN7")
	PORT_BIT( 0x7f, 0x40, IPT_TRACKBALL_Y ) PORT_MINMAX(0,127) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_PLAYER(2)

	PORT_START_TAG("IN8")
	PORT_BIT( 0x7f, 0x40, IPT_TRACKBALL_X ) PORT_MINMAX(0,127) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_REVERSE PORT_PLAYER(2)

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( mjleague )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mjleague_readmem,mjleague_writemem)

	MDRV_MACHINE_INIT(mjleague)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( passsht_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41004, 0xc41005) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( passsht_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc42006, 0xc42007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

static int passht4b_io1_val;
static int passht4b_io2_val;
static int passht4b_io3_val;

static READ16_HANDLER( passht4b_service_r )
{
	data16_t val=input_port_2_word_r(offset,0);
	if(!(readinputport(0) & 0x40)) val&=0xef;
	if(!(readinputport(1) & 0x40)) val&=0xdf;
	if(!(readinputport(5) & 0x40)) val&=0xbf;
	if(!(readinputport(6) & 0x40)) val&=0x7f;

	passht4b_io3_val=(readinputport(0)<<4) | (readinputport(5)&0xf);
	passht4b_io2_val=(readinputport(1)<<4) | (readinputport(6)&0xf);

	passht4b_io1_val=0xff;

	// player 1 buttons
	if(!(readinputport(0) & 0x10)) passht4b_io1_val &=0xfe;
	if(!(readinputport(0) & 0x20)) passht4b_io1_val &=0xfd;
	if(!(readinputport(0) & 0x80)) passht4b_io1_val &=0xfc;

	// player 2 buttons
	if(!(readinputport(1) & 0x10)) passht4b_io1_val &=0xfb;
	if(!(readinputport(1) & 0x20)) passht4b_io1_val &=0xf7;
	if(!(readinputport(1) & 0x80)) passht4b_io1_val &=0xf3;

	// player 3 buttons
	if(!(readinputport(5) & 0x10)) passht4b_io1_val &=0xef;
	if(!(readinputport(5) & 0x20)) passht4b_io1_val &=0xdf;
	if(!(readinputport(5) & 0x80)) passht4b_io1_val &=0xcf;

	// player 4 buttons
	if(!(readinputport(6) & 0x10)) passht4b_io1_val &=0xbf;
	if(!(readinputport(6) & 0x20)) passht4b_io1_val &=0x7f;
	if(!(readinputport(6) & 0x80)) passht4b_io1_val &=0x3f;

	return val;
}

static READ16_HANDLER( passht4b_io1_r ) {	return passht4b_io1_val;}
static READ16_HANDLER( passht4b_io2_r ) {	return passht4b_io2_val;}
static READ16_HANDLER( passht4b_io3_r ) {	return passht4b_io3_val;}

static ADDRESS_MAP_START( passht4b_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41000, 0xc41001) AM_READ(passht4b_service_r)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(passht4b_io1_r)
	AM_RANGE(0xc41004, 0xc41005) AM_READ(passht4b_io2_r)
	AM_RANGE(0xc41006, 0xc41007) AM_READ(passht4b_io3_r)
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc43000, 0xc43001) AM_READ(input_port_0_word_r) // player1		// test mode only
	AM_RANGE(0xc43002, 0xc43003) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc43004, 0xc43005) AM_READ(input_port_5_word_r) // player3
	AM_RANGE(0xc43006, 0xc43007) AM_READ(input_port_6_word_r) // player4
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( passht4b_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc42006, 0xc42007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc4600a, 0xc4600b) AM_WRITE(sys16_coinctrl_w)	/* coin counter doesn't work */
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void passsht_update_proc( void )
{
	sys16_fg_scrollx = sys16_workingram[0x34be/2];
	sys16_bg_scrollx = sys16_workingram[0x34c2/2];
	sys16_fg_scrolly = sys16_workingram[0x34bc/2];
	sys16_bg_scrolly = sys16_workingram[0x34c0/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static void passht4b_update_proc( void )
{
	sys16_fg_scrollx = sys16_workingram[0x34ce/2];
	sys16_bg_scrollx = sys16_workingram[0x34d2/2];
	sys16_fg_scrolly = sys16_workingram[0x34cc/2];
	sys16_bg_scrolly = sys16_workingram[0x34d0/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static MACHINE_INIT( passsht )
{
	sys16_sprxoffset = -0x48;
	sys16_spritesystem = sys16_sprite_passshot;

	// fix name entry
	sys16_patch_code( 0x13a8,0xc0);

	sys16_update_proc = passsht_update_proc;
}

static MACHINE_INIT( passht4b )
{
	sys16_sprxoffset = -0xb8;
	sys16_spritesystem = sys16_sprite_passshot;

	// fix name entry
	sys16_patch_code( 0x138a,0xc0);

	sys16_update_proc = passht4b_update_proc;
}

static DRIVER_INIT( passsht )
{
	machine_init_sys16_onetime();
}

static DRIVER_INIT( passht4b )
{
	int i;

	machine_init_sys16_onetime();

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

INPUT_PORTS_START( passsht )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point" )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table" )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
INPUT_PORTS_END

INPUT_PORTS_START( passht4b )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point" )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table" )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)

	PORT_START_TAG("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( passsht )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(passsht_readmem,passsht_writemem)

	MDRV_MACHINE_INIT(passsht)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( passht4b )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(passht4b_readmem,passht4b_writemem)

	MDRV_MACHINE_INIT(passht4b)
MACHINE_DRIVER_END

/***************************************************************************/


static ADDRESS_MAP_START( quartet_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc40002, 0xc40003) AM_READ(sys16_coinctrl_r)
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_0_word_r) // p1
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_1_word_r) // p2
	AM_RANGE(0xc41004, 0xc41005) AM_READ(input_port_2_word_r) // p3
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_3_word_r) // p4
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_5_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( quartet_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void quartet_update_proc( void )
{
	sys16_fg_scrollx = sys16_workingram[0x0d14/2] & 0x01ff;
	sys16_bg_scrollx = sys16_workingram[0x0d18/2] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x0f24/2] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x0f26/2] & 0x01ff;

//	if(((*(UINT16 *)(&sys16_extraram[4])) & 0xff) == 1)
//		\=1;
//	else
		sys16_quartet_title_kludge=0;

	set_fg_page1( sys16_workingram[0x0d1c/2] );
	set_bg_page1( sys16_workingram[0x0d1e/2] );
}

static MACHINE_INIT( quartet )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbc;
	sys16_fgxoffset = sys16_bgxoffset = 7;

	sys16_update_proc = quartet_update_proc;
}

static DRIVER_INIT( quartet )
{
	machine_init_sys16_onetime();
}
/***************************************************************************/

INPUT_PORTS_START( quartet )
	// Player 1
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 1 coin 2 really */
	// Player 2
	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 2 coin 2 really */
	// Player 3
	PORT_START_TAG("P3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 3 coin 2 really */
	// Player 4
	PORT_START_TAG("P4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 4 coin 2 really */

	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, "Credit Power" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x06, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, "Coin During Game" )
	PORT_DIPSETTING(    0x20, "Power" )
	PORT_DIPSETTING(    0x00, "Credit" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( quartet )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(quartet_readmem,quartet_writemem)

	MDRV_MACHINE_INIT(quartet)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( quartet2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc40002, 0xc40003) AM_READ(sys16_coinctrl_r)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( quartet2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void quartet2_update_proc( void )
{
	sys16_fg_scrollx = sys16_workingram[0x0d14/2] & 0x01ff;
	sys16_bg_scrollx = sys16_workingram[0x0d18/2] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;

//let's fix this properly
//	if(((*(UINT16 *)(&sys16_extraram[4])) & 0xff) == 1)
//		sys16_quartet_title_kludge=1;
//	else
		sys16_quartet_title_kludge=0;

	set_fg_page1( sys16_workingram[0x0d1c/2] );
	set_bg_page1( sys16_workingram[0x0d1e/2] );
}

static MACHINE_INIT( quartet2 )
{
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbc;
	sys16_fgxoffset = sys16_bgxoffset = 7;

	sys16_update_proc = quartet2_update_proc;
}

static DRIVER_INIT( quartet2 )
{
	machine_init_sys16_onetime();
}
/***************************************************************************/

INPUT_PORTS_START( quartet2 )
	SYS16_JOY1_SWAPPEDBUTTONS
	SYS16_JOY2_SWAPPEDBUTTONS
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, "Credit Power" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x06, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( quartet2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(quartet2_readmem,quartet2_writemem)

	MDRV_MACHINE_INIT(quartet2)
MACHINE_DRIVER_END


/***************************************************************************/

static READ16_HANDLER( io_p1mousex_r )
{ return 0xff-input_port_5_r( offset ); }
static READ16_HANDLER( io_p1mousey_r )
{ return input_port_6_r( offset ); }

static READ16_HANDLER( io_p2mousex_r )
{ return input_port_7_r( offset ); }
static READ16_HANDLER( io_p2mousey_r )
{ return input_port_8_r( offset ); }

static ADDRESS_MAP_START( sdi_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x411fff) AM_READ(SYS16_MRA16_TEXTRAM) //*
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41004, 0xc41005) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42004, 0xc42005) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc43000, 0xc43001) AM_READ(io_p1mousex_r)
	AM_RANGE(0xc43004, 0xc43005) AM_READ(io_p1mousey_r)
	AM_RANGE(0xc43008, 0xc43009) AM_READ(io_p2mousex_r)
	AM_RANGE(0xc4300c, 0xc4300d) AM_READ(io_p2mousey_r)
//	AM_RANGE(0xc42000, 0xc42001) AM_READ(MRA16_NOP) /* What is this? */
	AM_RANGE(0xc60000, 0xc60001) AM_READ(MRA16_NOP) /* What is this? */
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sdi_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x02ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x123406, 0x123407) AM_WRITE(sound_command_w)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x411fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void sdi_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( sdi )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;

	sys16_patch_code( 0x102f2, 0x00 );
	sys16_patch_code( 0x102f3, 0x02 );

	sys16_update_proc = sdi_update_proc;
}

static DRIVER_INIT( sdi )
{
	machine_init_sys16_onetime();
	sys18_splittab_bg_x=&sys16_textram[0x0fc0];
	sys16_rowscroll_scroll=0xff00;
}

/***************************************************************************/

INPUT_PORTS_START( sdi )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT) PORT_8WAY PORT_PLAYER(2)

	SYS16_JOY2

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)

	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "240? (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "Every 50000" )
	PORT_DIPSETTING(    0xc0, "50000" )
	PORT_DIPSETTING(    0x40, "100000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START_TAG("IN5")				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_TRACKBALL_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(75) PORT_KEYDELTA(1)

	PORT_START_TAG("IN6")				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_TRACKBALL_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(75) PORT_KEYDELTA(1)

	PORT_START_TAG("IN7")				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_TRACKBALL_X  ) PORT_MINMAX(0,255) PORT_SENSITIVITY(75) PORT_KEYDELTA(1) PORT_PLAYER(2)

	PORT_START_TAG("IN8")				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_TRACKBALL_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(75) PORT_KEYDELTA(1) PORT_PLAYER(2)

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( sdi )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sdi_readmem,sdi_writemem)

	MDRV_MACHINE_INIT(sdi)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( shinobi_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc43000, 0xc43001) AM_READ(MRA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shinobi_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc43000, 0xc43001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xfe0006, 0xfe0007) AM_WRITE(sound_command_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void shinobi_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( shinobi )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = shinobi_update_proc;
	fd1094_machine_init();

}


static DRIVER_INIT( shinobi )
{
	machine_init_sys16_onetime();
}

/***************************************************************************/

INPUT_PORTS_START( shinobi )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "240 (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, "Enemy's Bullet Speed" )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( shinobi )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(shinobi_readmem,shinobi_writemem)

	MDRV_MACHINE_INIT(shinobi)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( shinobl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shinobl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w)
	AM_RANGE(0xc40002, 0xc40003) AM_WRITE(sys16_3d_coinctrl_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void shinobl_update_proc( void )
{
	set_bg_page( sys16_textram[0x74e] );
	set_fg_page( sys16_textram[0x74f] );
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
}

static MACHINE_INIT( shinobl )
{
	static int bank[] = {
		0,2,4,6,
		1,3,5,7
	};
	sys16_obj_bank = bank;
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbc;
	sys16_fgxoffset = sys16_bgxoffset = 7;
	sys16_tilebank_switch=0x2000;

	sys16_update_proc = shinobl_update_proc;
}



/***************************************************************************/

static MACHINE_DRIVER_START( shinobl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(shinobl_readmem,shinobl_writemem)

	MDRV_MACHINE_INIT(shinobl)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( tetris_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)

	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2

	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetris_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sound_command_nmi_w) // tetris
	AM_RANGE(0xfe0006, 0xfe0007) AM_WRITE(sound_command_w) // tetrisa + tetrisb
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/* bootleg has extra ram for regs? */

static ADDRESS_MAP_START( tetrisbl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x418000, 0x41803f) AM_READ(SYS16_MRA16_EXTRAM2)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc80000, 0xc80001) AM_READ(MRA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetrisbl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x418000, 0x41803f) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc42006, 0xc42007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc43034, 0xc43035) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xc80000, 0xc80001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/


static void tetris_s16a_update_proc( void )
{
	sys16_fg_scrolly = sys16_textram[0xf24/2];
	sys16_bg_scrolly = sys16_textram[0xf26/2];
	sys16_fg_scrollx = sys16_textram[0xff8/2];
	sys16_bg_scrollx = sys16_textram[0xffa/2];

	set_fg_page( sys16_textram[0xe9e/2] );
	set_bg_page( sys16_textram[0xe9c/2] );
}

static void tetris_bootleg_update_proc( void )
{
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];

	set_fg_page( sys16_extraram2[0x38/2] );
	set_bg_page( sys16_extraram2[0x28/2] );
}

static MACHINE_INIT( tetris_s16a )
{
	fd1094_machine_init();

	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xb8;
	sys16_tilebank_switch=0x2000;
	sys16_fgxoffset = sys16_bgxoffset = 8;

	sys16_update_proc = tetris_s16a_update_proc;
}

static MACHINE_INIT( tetrisbl )
{
//	sys16_patch_code( 0xba6, 0x4e );
//	sys16_patch_code( 0xba7, 0x71 );

	sys16_sprxoffset = -0x40;
	sys16_update_proc = tetris_bootleg_update_proc;
}

static DRIVER_INIT( tetris )
{
	machine_init_sys16_onetime();

	fd1094_driver_init();
}

static DRIVER_INIT( tetrisbl )
{
	machine_init_sys16_onetime();
}


/***************************************************************************/

INPUT_PORTS_START( tetris )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE /* unconfirmed */

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )	// from the code it looks like some kind of difficulty
	PORT_DIPSETTING(    0x0c, "A" )					// level, but all 4 levels points to the same place
	PORT_DIPSETTING(    0x08, "B" )					// so it doesn't actually change anything!!
	PORT_DIPSETTING(    0x04, "C" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( tetris_s16a )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tetris_readmem,tetris_writemem)

	MDRV_MACHINE_INIT(tetris_s16a)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tetrisbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tetrisbl_readmem,tetrisbl_writemem)

	MDRV_MACHINE_INIT(tetrisbl)
MACHINE_DRIVER_END

/***************************************************************************/

static READ16_HANDLER( tt_io_player1_r )
{ return input_port_0_r( offset ) << 8; }
static READ16_HANDLER( tt_io_player2_r )
{ return input_port_1_r( offset ) << 8; }
static READ16_HANDLER( tt_io_service_r )
{ return input_port_2_r( offset ) << 8; }

static ADDRESS_MAP_START( tturf_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x2001e6, 0x2001e7) AM_READ(tt_io_service_r)
	AM_RANGE(0x2001e8, 0x2001e9) AM_READ(tt_io_player1_r)
	AM_RANGE(0x2001ea, 0x2001eb) AM_READ(tt_io_player2_r)
	AM_RANGE(0x200000, 0x203fff) AM_READ(SYS16_MRA16_EXTRAM)
	AM_RANGE(0x300000, 0x300fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x500000, 0x500fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x602002, 0x602003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x602000, 0x602001) AM_READ(input_port_4_word_r) // dip2
ADDRESS_MAP_END

static ADDRESS_MAP_START( tturf_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
	AM_RANGE(0x300000, 0x300fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x500000, 0x500fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xff0020, 0xff003f) AM_WRITE(MWA16_NOP) // config regs
ADDRESS_MAP_END

/***************************************************************************/

/*
	This game has a MCU which does the following:
	- Get Z80 sound command out of work RAM and write to Z80 sound command register
	- Read input ports and store to work RAM

	The routine which stores the sound code in RAM looks like this:

	; D0 = sound command
	movem.l    d0-d1/a0, -(a7)
	lea        $2001d6, a0         ; base of 16-byte circular buffer
	move.w     $2001d4, d1         ; get buffer index
	move.b     d0, (a0, d1.w)      ; write sound command to buffer
	addq.w     #1, d1              ; next buffer index
	andi.w     #$000f, d1          ; wrap buffer index
	move.w     d1, $2001d4         ; save buffer index
	addq.w     #1, $2001d2         ; bump 'sound code written' flag
	movem.l    (a7)+, d0-d1/a0
	rts

	Most likely the MCU reads $2001D2 and copies the sound byte from $2001D6+$2001D4 to the sound command register.
	In tturfbl, a JSR is inserted over the first LEA instruction to a subroutine which copies D0 to the sound command
	register at $600007, and restores a0 to $2001D6 before returning.

	If the circular buffer is to prioritize sound requests, then this effect is lost in tturfbl. If it's just to
	be tricky, tturfbl handles it correctly.
*/

static WRITE16_HANDLER( tturfu_mcu_sound_trigger_w )
{
	COMBINE_DATA(&sys16_extraram[offset]);

	if(activecpu_get_pc() == 0x100E)
	{
		int code;

		if(ACCESSING_LSB)
			code = (data >> 0) & 0xFF;
		else
			code = (data >> 8) & 0xFF;

		soundlatch_w(0, code);
		cpunum_set_input_line(1, 0, HOLD_LINE);
	}
}
static WRITE16_HANDLER( tturf_mcu_sound_trigger_w )
{
	COMBINE_DATA(&sys16_extraram[offset]);

	if(activecpu_get_pc() == 0x104c)
	{
		int code;

		if(ACCESSING_LSB)
			code = (data >> 0) & 0xFF;
		else
			code = (data >> 8) & 0xFF;

		soundlatch_w(0, code);
		cpunum_set_input_line(1, 0, HOLD_LINE);
	}
}


static void tturf_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( tturf )
{
	static int bank[16] = { 0,0,1,0,2,0,3,0 };
	sys16_obj_bank = bank;
	sys16_update_proc = tturf_update_proc;

	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2001d6, 0x2001e5, 0, 0, tturf_mcu_sound_trigger_w );

}

static MACHINE_INIT( tturfu )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,0,
		0,0,0,1,
		0,2,3,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = tturf_update_proc;
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2001d6, 0x2001e5, 0, 0, tturfu_mcu_sound_trigger_w );
}

static DRIVER_INIT( tturf )
{
	machine_init_sys16_onetime();
}
/***************************************************************************/

INPUT_PORTS_START( tturf )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Continues ) )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "Unlimited" )
	PORT_DIPSETTING(    0x03, "Unlimited" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x30, 0x20, "Starting Energy" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x30, "8" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Bonus Energy" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
INPUT_PORTS_END


/***************************************************************************/

static ADDRESS_MAP_START( tturfbl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x2001e6, 0x2001e7) AM_READ(tt_io_service_r)
	AM_RANGE(0x2001e8, 0x2001e9) AM_READ(tt_io_player1_r)
	AM_RANGE(0x2001ea, 0x2001eb) AM_READ(tt_io_player2_r)
	AM_RANGE(0x200000, 0x203fff) AM_READ(SYS16_MRA16_EXTRAM)
	AM_RANGE(0x300000, 0x300fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x500000, 0x500fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0x601002, 0x601003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0x601004, 0x601005) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0x601000, 0x601001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0x602002, 0x602003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x602000, 0x602001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc46000, 0xc4601f) AM_READ(SYS16_MRA16_EXTRAM3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tturfbl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
	AM_RANGE(0x300000, 0x300fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x500000, 0x500fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0x600006, 0x600007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc44000, 0xc44001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xc46000, 0xc4601f) AM_WRITE(SYS16_MWA16_EXTRAM3) AM_BASE(&sys16_extraram3)
	AM_RANGE(0xff0020, 0xff003f) AM_WRITE(MWA16_NOP) // config regs
ADDRESS_MAP_END

/***************************************************************************/

static void tturfbl_update_proc( void )
{
	sys16_fg_scrollx = sys16_textram[0x74c] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x74d/2] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];


	{
		int data1,data2;

		data1 = sys16_textram[0x740];
		data2 = sys16_textram[0x741];

		sys16_fg_page[3] = data1>>12;
		sys16_bg_page[3] = (data1>>8)&0xf;
		sys16_fg_page[1] = (data1>>4)&0xf;
		sys16_bg_page[1] = data1&0xf;

		sys16_fg_page[2] = data2>>12;
		sys16_bg_page[2] = (data2>>8)&0xf;
		sys16_fg_page[0] = (data2>>4)&0xf;
		sys16_bg_page[0] = data2&0xf;
	}
}

static MACHINE_INIT( tturfbl )
{
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0x48;

	sys16_update_proc = tturfbl_update_proc;
}

static DRIVER_INIT( tturfbl )
{
	UINT8 *mem;
	int i;

	machine_init_sys16_onetime();

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;

	mem = memory_region(REGION_CPU2);
	memcpy(mem, mem+0x10000, 0x8000);

}
/***************************************************************************/
// sound ??
static MACHINE_DRIVER_START( tturfbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tturfbl_readmem,tturfbl_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(tturfbl_sound_readmem,tturfbl_sound_writemem)
	MDRV_CPU_IO_MAP(tturfbl_sound_readport,tturfbl_sound_writeport)

	MDRV_SOUND_REMOVE("7759")
	MDRV_SOUND_ADD_TAG("5205", MSM5205, tturfbl_msm5205_interface)

	MDRV_MACHINE_INIT(tturfbl)
MACHINE_DRIVER_END


/***************************************************************************/

static ADDRESS_MAP_START( wb3_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static WRITE16_HANDLER( wb3_sound_command_w )
{
	if( ACCESSING_MSB ) sound_command_w(offset,data>>8,0xff00); //*
}

static ADDRESS_MAP_START( wb3_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x3f0000, 0x3fffff) AM_WRITE(sys16_tilebank_w)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
 	AM_RANGE(0xdf0006, 0xdf0007) AM_WRITE(sound_command_w) // original encrypted
//	AM_RANGE(0xffc008, 0xffc009) AM_WRITE(wb3_sound_command_w) // protected set with mcu
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void wb3_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( wb3b )
{
	static int bank[16] = {
		2,0,
		1,0,
		3,0,
		0,3,
		0,0,
		0,2,
		0,1,
		0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = wb3_update_proc;
}

static DRIVER_INIT( wb3b )
{
	machine_init_sys16_onetime();

	/* mcu grabs sound command direct from ram! */
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xffc008, 0xffc009, 0, 0, wb3_sound_command_w );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xfe0006, 0xfe0007, 0, 0, MWA16_NOP );

}


/***************************************************************************/

INPUT_PORTS_START( wb3b )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )		//??
	PORT_DIPSETTING(    0x10, "5000/10000/18000/30000" )
	PORT_DIPSETTING(    0x00, "5000/15000/30000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Round Select" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )			// no collision though
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( wb3b )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(wb3_readmem,wb3_writemem)

	MDRV_MACHINE_INIT(wb3b)
MACHINE_DRIVER_END

/***************************************************************************/

static ADDRESS_MAP_START( wb3bbl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x840000, 0x840fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41004, 0xc41005) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc41000, 0xc41001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xc46000, 0xc4601f) AM_READ(SYS16_MRA16_EXTRAM3)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wb3bbl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x3f0000, 0x3fffff) AM_WRITE(sys16_tilebank_w)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x840000, 0x840fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc42006, 0xc42007) AM_WRITE(sound_command_w)
	AM_RANGE(0xc44000, 0xc44001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xc46000, 0xc4601f) AM_WRITE(SYS16_MWA16_EXTRAM3) AM_BASE(&sys16_extraram3)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void wb3bbl_update_proc( void )
{

	sys16_fg_scrollx = sys16_workingram[0xc030/2];
	sys16_bg_scrollx = sys16_workingram[0xc038/2];
	sys16_fg_scrolly = sys16_workingram[0xc032/2];
	sys16_bg_scrolly = sys16_workingram[0xc03c/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static MACHINE_INIT( wb3bbl )
{
	static int bank[16] = {
		2,0,
		1,0,
		3,0,
		0,3,
		0,0,
		0,2,
		0,1,
		0,0
	};
	sys16_obj_bank = bank;
#if 1
	sys16_patch_code( 0x17058, 0x4e );
	sys16_patch_code( 0x17059, 0xb9 );
	sys16_patch_code( 0x1705a, 0x00 );
	sys16_patch_code( 0x1705b, 0x00 );
	sys16_patch_code( 0x1705c, 0x09 );
	sys16_patch_code( 0x1705d, 0xdc );
	sys16_patch_code( 0x1705e, 0x4e );
	sys16_patch_code( 0x1705f, 0xf9 );
	sys16_patch_code( 0x17060, 0x00 );
	sys16_patch_code( 0x17061, 0x01 );
	sys16_patch_code( 0x17062, 0x70 );
	sys16_patch_code( 0x17063, 0xe0 );
	sys16_patch_code( 0x1a3a, 0x31 );
	sys16_patch_code( 0x1a3b, 0x7c );
	sys16_patch_code( 0x1a3c, 0x80 );
	sys16_patch_code( 0x1a3d, 0x00 );
	sys16_patch_code( 0x23df8, 0x14 );
	sys16_patch_code( 0x23df9, 0x41 );
	sys16_patch_code( 0x23dfa, 0x10 );
	sys16_patch_code( 0x23dfd, 0x14 );
	sys16_patch_code( 0x23dff, 0x1c );
#endif
	sys16_update_proc = wb3bbl_update_proc;
}

static DRIVER_INIT( wb3bbl )
{
	int i;

	machine_init_sys16_onetime();

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

static MACHINE_DRIVER_START( wb3bbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(wb3bbl_readmem,wb3bbl_writemem)

	MDRV_MACHINE_INIT(wb3bbl)
MACHINE_DRIVER_END

/***************************************************************************/

static READ16_HANDLER( ww_io_service_r )
{
	return input_port_2_word_r(offset,0) | (sys16_workingram[0x2082/2] & 0xff00);
}

static ADDRESS_MAP_START( wrestwar_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x110000, 0x111fff) AM_READ(SYS16_MRA16_TEXTRAM) //*
	AM_RANGE(0x200000, 0x200fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x300000, 0x300fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0x400000, 0x400003) AM_READ(SYS16_MRA16_EXTRAM)
	AM_RANGE(0xc41002, 0xc41003) AM_READ(input_port_0_word_r) // player1
	AM_RANGE(0xc41006, 0xc41007) AM_READ(input_port_1_word_r) // player2
	AM_RANGE(0xc42002, 0xc42003) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xc42000, 0xc42001) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xffe082, 0xffe083) AM_READ(ww_io_service_r)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wrestwar_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0bffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x110000, 0x111fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x300000, 0x300fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x400000, 0x400003) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(sys16_coinctrl_w)
	AM_RANGE(0xc43034, 0xc43035) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xffe08e, 0xffe08f) AM_WRITE(sound_command_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

/***************************************************************************/

static void wrestwar_update_proc( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];

	set_tile_bank( sys16_extraram[1] );
}

static MACHINE_INIT( wrestwar )
{
	sys16_bg_priority_mode=2;
	sys16_bg_priority_value=0x0a00;
	sys16_update_proc = wrestwar_update_proc;
	sys16_wwfix = 1; //*
}

static DRIVER_INIT( wrestwar )
{
	machine_init_sys16_onetime();
	sys16_bg1_trans=1;
	sys16_MaxShadowColors=16;
	sys18_splittab_bg_y=&sys16_textram[0x0f40];
	sys18_splittab_fg_y=&sys16_textram[0x0f00];
	sys16_rowscroll_scroll=0x8000;
}
/***************************************************************************/

/***************************************************************************/

static MACHINE_DRIVER_START( wrestwar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(wrestwar_readmem,wrestwar_writemem)

	MDRV_MACHINE_INIT(wrestwar)
MACHINE_DRIVER_END

/*****************************************************************************/
/* Dummy drivers for games that don't have a working clone and are protected */
/*****************************************************************************/

static ADDRESS_MAP_START( sys16_dummy_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(SYS16_MRA16_WORKINGRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sys16_dummy_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(SYS16_MWA16_WORKINGRAM) AM_BASE(&sys16_workingram)
ADDRESS_MAP_END

static MACHINE_INIT( sys16_dummy )
{
}

static DRIVER_INIT( s16dummy )
{
	machine_init_sys16_onetime();
}

INPUT_PORTS_START( s16dummy )
INPUT_PORTS_END

static MACHINE_DRIVER_START( s16dummy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sys16_dummy_readmem,sys16_dummy_writemem)

	MDRV_MACHINE_INIT(sys16_dummy)
MACHINE_DRIVER_END



/*****************************************************************************/


// sys16A
ROM_START( alexkidd )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10429.42", 0x000000, 0x10000, CRC(bdf49eca) SHA1(899bc2d346544e4a33de51b60e02ebf7ee82cea8) )
	ROM_LOAD16_BYTE( "epr10427.26", 0x000001, 0x10000, CRC(f6e3dd29) SHA1(bb94ebc062bb7c6c13b68579053b9cbe8b92417c) )
	ROM_LOAD16_BYTE( "epr10430.43", 0x020000, 0x10000, CRC(89e3439f) SHA1(7c751bb477584842d93fda6686b03e289140bd62) )
	ROM_LOAD16_BYTE( "epr10428.25", 0x020001, 0x10000, CRC(dbed3210) SHA1(1e2d22935a633641ff88967d67ec673ee25cbf55) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10431.95", 0x00000, 0x08000, CRC(a7962c39) SHA1(c816fc5d9f21b2ba32b9841e64b634bce7ea78c8) )
	ROM_LOAD( "10432.94", 0x08000, 0x08000, CRC(db8cd24e) SHA1(656d98844ad9ccaa68e3f501137dddd0a27d999d) )
	ROM_LOAD( "10433.93", 0x10000, 0x08000, CRC(e163c8c2) SHA1(ac54c5ecedca5b1a2c550de32687ca57c4d3a411) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10437.10", 0x000001, 0x8000, CRC(522f7618) SHA1(9a6bc857dfef1dd1b7bffa034523c1c4cd8b3f4c) )
	ROM_LOAD16_BYTE( "10441.11", 0x000000, 0x8000, CRC(74e3a35c) SHA1(26b980a0a3aee94ac38e0e0c7d305bb35a60d1c4) )
	ROM_LOAD16_BYTE( "10438.17", 0x010001, 0x8000, CRC(738a6362) SHA1(a3c5f10c263cb216d275875f6333484a1cca281b) )
	ROM_LOAD16_BYTE( "10442.18", 0x010000, 0x8000, CRC(86cb9c14) SHA1(42bd0ed985de61ff183eed0192257966caa01594) )
	ROM_LOAD16_BYTE( "10439.23", 0x020001, 0x8000, CRC(b391aca7) SHA1(ca9d80b67e5365f709f90a5342b5e3aa7c7126e1) )
	ROM_LOAD16_BYTE( "10443.24", 0x020000, 0x8000, CRC(95d32635) SHA1(788af2af1ae783128bcdc8cd44d17cd2f1542231) )
	ROM_LOAD16_BYTE( "10440.29", 0x030001, 0x8000, CRC(23939508) SHA1(68450a18fc7e35f5b0155632aa68cffd251be38c) )
	ROM_LOAD16_BYTE( "10444.30", 0x030000, 0x8000, CRC(82115823) SHA1(e4103003cda949bebe57815115a5028f4fe8e7d7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10434.12", 0x0000, 0x8000, CRC(77141cce) SHA1(6c5e83527f7e11a5ff5cc4fa75d55618a55e1a58) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* 7751 sound data (not used yet) */
	ROM_LOAD( "10435.1", 0x0000, 0x8000, CRC(ad89f6e3) SHA1(812a132142065b0fe13b5f0ac534b6d8830ba102) )
	ROM_LOAD( "10436.2", 0x8000, 0x8000, CRC(96c76613) SHA1(fe3e4e649fd2cb2453eec0c92015bd54b3b9a1b5) )
ROM_END

ROM_START( alexkida )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10447.43", 0x000000, 0x10000, CRC(29e87f71) SHA1(af980e55c02b3de1121c144fee23af74d24042ac) )
	ROM_LOAD16_BYTE( "10445.26", 0x000001, 0x10000, CRC(25ce5b6f) SHA1(dfec64df7e8d145d30740808bc94bdbbe667c4e8) )
	ROM_LOAD16_BYTE( "10448.42", 0x020000, 0x10000, CRC(05baedb5) SHA1(fc15989bf3d850170e4e018d74f18553f0268576) )
	ROM_LOAD16_BYTE( "10446.25", 0x020001, 0x10000, CRC(cd61d23c) SHA1(c235c4fef28556e9f2d07e815ad213c308e85598) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10431.95", 0x00000, 0x08000, CRC(a7962c39) SHA1(c816fc5d9f21b2ba32b9841e64b634bce7ea78c8) )
	ROM_LOAD( "10432.94", 0x08000, 0x08000, CRC(db8cd24e) SHA1(656d98844ad9ccaa68e3f501137dddd0a27d999d) )
	ROM_LOAD( "10433.93", 0x10000, 0x08000, CRC(e163c8c2) SHA1(ac54c5ecedca5b1a2c550de32687ca57c4d3a411) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10437.10", 0x000001, 0x8000, CRC(522f7618) SHA1(9a6bc857dfef1dd1b7bffa034523c1c4cd8b3f4c) )
	ROM_LOAD16_BYTE( "10441.11", 0x000000, 0x8000, CRC(74e3a35c) SHA1(26b980a0a3aee94ac38e0e0c7d305bb35a60d1c4) )
	ROM_LOAD16_BYTE( "10438.17", 0x010001, 0x8000, CRC(738a6362) SHA1(a3c5f10c263cb216d275875f6333484a1cca281b) )
	ROM_LOAD16_BYTE( "10442.18", 0x010000, 0x8000, CRC(86cb9c14) SHA1(42bd0ed985de61ff183eed0192257966caa01594) )
	ROM_LOAD16_BYTE( "10439.23", 0x020001, 0x8000, CRC(b391aca7) SHA1(ca9d80b67e5365f709f90a5342b5e3aa7c7126e1) )
	ROM_LOAD16_BYTE( "10443.24", 0x020000, 0x8000, CRC(95d32635) SHA1(788af2af1ae783128bcdc8cd44d17cd2f1542231) )
	ROM_LOAD16_BYTE( "10440.29", 0x030001, 0x8000, CRC(23939508) SHA1(68450a18fc7e35f5b0155632aa68cffd251be38c) )
	ROM_LOAD16_BYTE( "10444.30", 0x030000, 0x8000, CRC(82115823) SHA1(e4103003cda949bebe57815115a5028f4fe8e7d7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10434.12", 0x0000, 0x8000, CRC(77141cce) SHA1(6c5e83527f7e11a5ff5cc4fa75d55618a55e1a58) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "10435.1", 0x0000, 0x8000, CRC(ad89f6e3) SHA1(812a132142065b0fe13b5f0ac534b6d8830ba102) )
	ROM_LOAD( "10436.2", 0x8000, 0x8000, CRC(96c76613) SHA1(fe3e4e649fd2cb2453eec0c92015bd54b3b9a1b5) )
ROM_END

// sys16A - use a different sound chip?
ROM_START( aliensya )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code. I guessing the order a bit here */
	ROM_LOAD16_BYTE( "10808", 0x00000, 0x8000, CRC(e669929f) SHA1(b5ab41d6f31f0369f8c5f5eb6fc08e8c23312b96) )
	ROM_LOAD16_BYTE( "10806", 0x00001, 0x8000, CRC(9f7f8fdd) SHA1(819e9c491b7d23deaef646d37319c38e75827d68) )
	ROM_LOAD16_BYTE( "10809", 0x10000, 0x8000, CRC(9a424919) SHA1(a7be5d9bed329099df10ff5a0104cb832485bd0a) )
	ROM_LOAD16_BYTE( "10807", 0x10001, 0x8000, CRC(3d2c3530) SHA1(567ed45c84b1d3d92371c4ad33fdb28f68cf29a3) )
	ROM_LOAD16_BYTE( "10701", 0x20000, 0x8000, CRC(92171751) SHA1(69a282c01db7224f32386a6db25309e09e29a112) )
	ROM_LOAD16_BYTE( "10698", 0x20001, 0x8000, CRC(c1e4fdc0) SHA1(65817a9336f7887d2bf14485bdff8352c960d2ab) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10739", 0x00000, 0x10000, CRC(a29ec207) SHA1(c469d2689a7bdc2a59dfff56ce13d34e9fbac263) )
	ROM_LOAD( "10740", 0x10000, 0x10000, CRC(47f93015) SHA1(68247a6bffd1d4d1c450148dd46214d01ce1c668) )
	ROM_LOAD( "10741", 0x20000, 0x10000, CRC(4970739c) SHA1(5bdf4222209ec46e0015bfc0f90578dd9b30bdd1) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) //* sound CPU */
	ROM_LOAD( "10705", 0x00000, 0x8000, CRC(777b749e) SHA1(086b03100064a98228f95db7962b2671121c46ea) )
	ROM_LOAD( "10706", 0x10000, 0x8000, CRC(aa114acc) SHA1(81a2b3586ae90bc7fc55b82478ffe182ac49983e) )
	ROM_LOAD( "10707", 0x18000, 0x8000, CRC(800c1d82) SHA1(aac4123bd35f87da09264649f4cf8326b2ba3cb8) )
	ROM_LOAD( "10708", 0x20000, 0x8000, CRC(5921ef52) SHA1(eff9978361692e6e60a9c6caf5740dd6182cfe4a) )
ROM_END

ROM_START( aliensyj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* Custom 68000 code . I guessing the order a bit here */
	// Custom CPU 317-0033
	ROM_LOAD16_BYTE( "epr10699.43", 0x00000, 0x8000, CRC(3fd38d17) SHA1(538c1246121051a1af9ba2a4259eb1fe7e4952e1) )
	ROM_LOAD16_BYTE( "epr10696.26", 0x00001, 0x8000, CRC(d734f19f) SHA1(4a08c35084f7a9364ba0f058b9a9ffc30c8b5a78) )
	ROM_LOAD16_BYTE( "epr10700.42", 0x10000, 0x8000, CRC(3b04b252) SHA1(0e40e89e8feb7c98ee1da1c3fb3fe1d317c66842) )
	ROM_LOAD16_BYTE( "epr10697.25", 0x10001, 0x8000, CRC(f2bc123d) SHA1(7848529342495289e2d4f865767f3649cd85993b) )
	ROM_LOAD16_BYTE( "10701", 0x20000, 0x8000, CRC(92171751) SHA1(69a282c01db7224f32386a6db25309e09e29a112) )
	ROM_LOAD16_BYTE( "10698", 0x20001, 0x8000, CRC(c1e4fdc0) SHA1(65817a9336f7887d2bf14485bdff8352c960d2ab) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10739", 0x00000, 0x10000, CRC(a29ec207) SHA1(c469d2689a7bdc2a59dfff56ce13d34e9fbac263) )
	ROM_LOAD( "10740", 0x10000, 0x10000, CRC(47f93015) SHA1(68247a6bffd1d4d1c450148dd46214d01ce1c668) )
	ROM_LOAD( "10741", 0x20000, 0x10000, CRC(4970739c) SHA1(5bdf4222209ec46e0015bfc0f90578dd9b30bdd1) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) //* sound CPU */
	ROM_LOAD( "10705", 0x00000, 0x8000, CRC(777b749e) SHA1(086b03100064a98228f95db7962b2671121c46ea) )
	ROM_LOAD( "10706", 0x10000, 0x8000, CRC(aa114acc) SHA1(81a2b3586ae90bc7fc55b82478ffe182ac49983e) )
	ROM_LOAD( "10707", 0x18000, 0x8000, CRC(800c1d82) SHA1(aac4123bd35f87da09264649f4cf8326b2ba3cb8) )
	ROM_LOAD( "10708", 0x20000, 0x8000, CRC(5921ef52) SHA1(eff9978361692e6e60a9c6caf5740dd6182cfe4a) )
ROM_END


ROM_START( bayrtbl1 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "b4.bin", 0x000000, 0x10000, CRC(eb6646ae) SHA1(073bc0a3868e70785f44e497a949cd9e3b591a33) )
	ROM_LOAD16_BYTE( "b2.bin", 0x000001, 0x10000, CRC(ecd9cd0e) SHA1(177c38ca02c4e87d6adcae77ce4e9237938d23a9) )
	/* empty 0x20000-0x80000*/
	ROM_LOAD16_BYTE( "br.5a",  0x080000, 0x10000, CRC(9d6fd183) SHA1(5ae78d33c0e929886d84a25c0fbd62ab45dcbff4) )
	ROM_LOAD16_BYTE( "br.2a",  0x080001, 0x10000, CRC(5ca1e3d2) SHA1(51ce67ed0a0054f9c9c4ac56c5775716c44d74b1) )
	ROM_LOAD16_BYTE( "b8.bin", 0x0a0000, 0x10000, CRC(e7ca0331) SHA1(b255939576a84f4d266f31a7fde818e04ff35b24) )
	ROM_LOAD16_BYTE( "b6.bin", 0x0a0001, 0x10000, CRC(2bc748a6) SHA1(9ab760377fde24cecb703726ee3e59ee23d60a3a) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "bs16.bin", 0x00000, 0x10000, CRC(a8a5b310) SHA1(8883e1ed48a3e0f7b4c36d83579f93e84e28568c) )
	ROM_LOAD( "bs14.bin", 0x10000, 0x10000, CRC(6bc4d0a8) SHA1(90b9a61c7a140291d72554857ce26d54ebf03fc2) )
	ROM_LOAD( "bs12.bin", 0x20000, 0x10000, CRC(c1f967a6) SHA1(8eb6bbd9e17dc531830bc798b8485c8ea999e56e) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "br_obj0o.1b", 0x00001, 0x10000, CRC(098a5e82) SHA1(c5922f418773bc3629071e584457839d67a370e9) )
	ROM_LOAD16_BYTE( "br_obj0e.5b", 0x00000, 0x10000, CRC(85238af9) SHA1(39989a8d9b60c6d55272b5e2c213341a563dd993) )
	ROM_LOAD16_BYTE( "br_obj1o.2b", 0x20001, 0x10000, CRC(cc641da1) SHA1(28f8a6502702cb9e2cc7f3e98f6c5d201f462fa3) )
	ROM_LOAD16_BYTE( "br_obj1e.6b", 0x20000, 0x10000, CRC(d3123315) SHA1(16a87caed1cabb080d4f35935910b38797344ca5) )
	ROM_LOAD16_BYTE( "br_obj2o.3b", 0x40001, 0x10000, CRC(84efac1f) SHA1(41c43d70dc7ae7e361d6fa12c5790ea7ebf13ca8) )
	ROM_LOAD16_BYTE( "br_obj2e.7b", 0x40000, 0x10000, CRC(b73b12cb) SHA1(e8265ae90aabf1ee0522dbc6541a0f82fec97c7a) )
	ROM_LOAD16_BYTE( "br_obj3o.4b", 0x60001, 0x10000, CRC(a2e238ac) SHA1(c854774c0ffd1ccf6e46591a8fa3c80a4630e007) )
	ROM_LOAD16_BYTE( "bs7.bin",     0x60000, 0x10000, CRC(0c91abcc) SHA1(d25608f3cbacd1bd169f1a2247f007ac8bc8dda0) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12459.a10", 0x00000, 0x08000, CRC(3e1d29d0) SHA1(fe3d985983e5132e8a26a02a3f2d8d420cbf1a49) )
	ROM_LOAD( "mpr12460.a11", 0x10000, 0x20000, CRC(0bae570d) SHA1(05fa4a3405666342ab66e696a7344cca97569f19) )
	ROM_LOAD( "mpr12461.a12", 0x30000, 0x20000, CRC(b03b8b46) SHA1(b0283ac377d464f3d9374a992192ec6c515a3c2f) )
ROM_END

ROM_START( bayrtbl2 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "br_04", 0x000000, 0x10000, CRC(2e33ebfc) SHA1(f6b5a4bd28d302abd6b1e5a9ec6f2a8b57ff213e) )
	ROM_LOAD16_BYTE( "br_06", 0x000001, 0x10000, CRC(3db42313) SHA1(e1c874ebf83e1a458cefaa038fbe89a9804ca30d) )
	/* empty 0x20000-0x80000*/
	ROM_LOAD16_BYTE( "br_03", 0x080000, 0x20000, CRC(285d256b) SHA1(73eac0131d14f0d7fe2a06cb2e0e57dcf4779cf9) )
	ROM_LOAD16_BYTE( "br_05", 0x080001, 0x20000, CRC(552e6384) SHA1(2770b0c9d961671576e09ada2ebd7bb486f24547) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "br_15",    0x00000, 0x10000, CRC(050079a9) SHA1(4b356eddec2f500fb0dcc20af6b7aed2f9ef0c02) )
	ROM_LOAD( "br_16",    0x10000, 0x10000, CRC(fc371928) SHA1(b36866c95bdc440aae999a90ecf3bbaed11d4351) )
	ROM_LOAD( "bs12.bin", 0x20000, 0x10000, CRC(c1f967a6) SHA1(8eb6bbd9e17dc531830bc798b8485c8ea999e56e) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "br_11",       0x00001, 0x10000, CRC(65232905) SHA1(cb195a0ce8bff9d1d3e31678060b3aaccfefcd2d) )
	ROM_LOAD16_BYTE( "br_obj0e.5b", 0x00000, 0x10000, CRC(85238af9) SHA1(39989a8d9b60c6d55272b5e2c213341a563dd993) )
	ROM_LOAD16_BYTE( "br_obj1o.2b", 0x20001, 0x10000, CRC(cc641da1) SHA1(28f8a6502702cb9e2cc7f3e98f6c5d201f462fa3) )
	ROM_LOAD16_BYTE( "br_obj1e.6b", 0x20000, 0x10000, CRC(d3123315) SHA1(16a87caed1cabb080d4f35935910b38797344ca5) )
	ROM_LOAD16_BYTE( "br_obj2o.3b", 0x40001, 0x10000, CRC(84efac1f) SHA1(41c43d70dc7ae7e361d6fa12c5790ea7ebf13ca8) )
	ROM_LOAD16_BYTE( "br_09",       0x40000, 0x10000, CRC(05e9b840) SHA1(7cc1c9ac7b85f1e1bdb68215b5e83eae3ee5ba2a) )
	ROM_LOAD16_BYTE( "br_14",       0x60001, 0x10000, CRC(4c4a177b) SHA1(a9dfd7e56b0a21a0f7750d8ec4631901ad182609) )
	ROM_LOAD16_BYTE( "bs7.bin",     0x60000, 0x10000, CRC(0c91abcc) SHA1(d25608f3cbacd1bd169f1a2247f007ac8bc8dda0) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "br_01", 0x00000, 0x10000, CRC(b87156ec) SHA1(bdfef2ab5a4d3cac4077c92ce1ef4604b4c11cf8) )
	ROM_LOAD( "br_02", 0x10000, 0x10000, CRC(ef63991b) SHA1(4221741780f88c80b3213ddca949bee7d4c1469a) )
ROM_END

// pre16
ROM_START( bodyslam )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10066.b9", 0x000000, 0x8000, CRC(6cd53290) SHA1(68ef83ad99a26a507d9bc4cd715462169f4ac41f) )
	ROM_LOAD16_BYTE( "epr10063.b6", 0x000001, 0x8000, CRC(dd849a16) SHA1(b8cb9f2685a739698a3ed18f76617fd4ac9cb424) )
	ROM_LOAD16_BYTE( "epr10067.b10",0x010000, 0x8000, CRC(db22a5ce) SHA1(95c37d4913fa31d5edf02661681bc83deec731d9) )
	ROM_LOAD16_BYTE( "epr10064.b7", 0x010001, 0x8000, CRC(53d6b7e0) SHA1(00bfa1487479629f60e1cc1b98ced47e4cb07964) )
	ROM_LOAD16_BYTE( "epr10068.b11",0x020000, 0x8000, CRC(15ccc665) SHA1(b088a9bcb1499854794b2dbf4c689f3ae3ce2808) )
	ROM_LOAD16_BYTE( "epr10065.b8", 0x020001, 0x8000, CRC(0e5fa314) SHA1(44e36fde102ba6aef2c3b4374ddc21690f2fe527) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10321.c9",  0x00000, 0x8000, CRC(cd3e7cba) SHA1(4d3cfc7346c6e63e2221193601f949162d0e4f90) ) /* plane 1 */
	ROM_LOAD( "epr10322.c10", 0x08000, 0x8000, CRC(b53d3217) SHA1(baebf20925e9f8ab6660f041a24721716d5b7d92) ) /* plane 2 */
	ROM_LOAD( "epr10323.c11", 0x10000, 0x8000, CRC(915a3e61) SHA1(6504a8b26b7b4880971cd69ac2c8aae30dcfa18c) ) /* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr10012.c5",  0x00001, 0x08000, CRC(990824e8) SHA1(bd45f75d07cb4e17583c2d76050e5f819f4b7efe) )
	ROM_LOAD16_BYTE( "epr10016.b2",  0x00000, 0x08000, CRC(af5dc72f) SHA1(97bbb76940c702e642d8222dda71447b8f60b616) )
	ROM_LOAD16_BYTE( "epr10013.c6",  0x10001, 0x08000, CRC(9a0919c5) SHA1(e39e60c1e834b3b46bf2ef1c5952841bebe66ade) )
	ROM_LOAD16_BYTE( "epr10017.b3",  0x10000, 0x08000, CRC(62aafd95) SHA1(e1e3a95fd11cabf81f44ac2dd3f951d3094725e6) )
	ROM_LOAD16_BYTE( "epr10027.c7",  0x20001, 0x08000, CRC(3f1c57c7) SHA1(1336da8dc167a323f09534a2f62ae6f9c62290e4) )
	ROM_LOAD16_BYTE( "epr10028.b4",  0x20000, 0x08000, CRC(80d4946d) SHA1(d4c96a18ef6c2ac6bd9d153d8862a3af894642e8) )
	ROM_LOAD16_BYTE( "epr10015.c8",  0x30001, 0x08000, CRC(582d3b6a) SHA1(4f1d0060682e3fc1147082286e00e6a296a95da2) )
	ROM_LOAD16_BYTE( "epr10019.b5",  0x30000, 0x08000, CRC(e020c38b) SHA1(d13d38a64f2afa7df3cbccef2fe505a4421b73ad) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10026.b1", 0x00000, 0x8000, CRC(123b69b8) SHA1(c0614a8c822991e257f7218908247df278056de8) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr10029.c1", 0x00000, 0x8000, CRC(7e4aca83) SHA1(703486b96d493941ee87267e8363220a851f008e) )
	ROM_LOAD( "epr10030.c2", 0x08000, 0x8000, CRC(dcc1df0b) SHA1(a82a557fa48f4b3e1ab38f61b84d749cd417e80f) )
	ROM_LOAD( "epr10031.c3", 0x10000, 0x8000, CRC(ea3c4472) SHA1(ad8eac2d3d14fd6aba713f4d624861c17aabf757) )
	ROM_LOAD( "epr10032.c4", 0x18000, 0x8000, CRC(0aabebce) SHA1(fab12df8f4eab270be491c6c025d832c338e1e83) )
ROM_END

ROM_START( dumpmtmt )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7704a.bin", 0x000000, 0x8000, CRC(96de6c7b) SHA1(f23edf86c5044c151a8502957af7ca0de420d55e) )
	ROM_LOAD16_BYTE( "7701a.bin", 0x000001, 0x8000, CRC(786d1009) SHA1(c56ebd169c2792cde610a7130cffdc0363fca871) )
	ROM_LOAD16_BYTE( "7705a.bin", 0x010000, 0x8000, CRC(fc584391) SHA1(27238408fba2dda67f29094a6700b634b6fdaa58) )
	ROM_LOAD16_BYTE( "7702a.bin", 0x010001, 0x8000, CRC(2241a8fd) SHA1(d968ab57aa228dbb7ae6f17d7bf22991291e75ae) )
	ROM_LOAD16_BYTE( "7706a.bin", 0x020000, 0x8000, CRC(6bbcc9d0) SHA1(e8e0b85867f11eec6b280f3ad9e2746d3d97ab28) )
	ROM_LOAD16_BYTE( "7703a.bin", 0x020001, 0x8000, CRC(fcb0cd40) SHA1(999e107fe08fcb52729ddebc7714a85c47e748b1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7707a.bin",  0x00000, 0x8000, CRC(45318738) SHA1(6885347321aec8c4829a71e4518d1742f939ea9c) ) /* plane 1 */
	ROM_LOAD( "7708a.bin",  0x08000, 0x8000, CRC(411be9a4) SHA1(808a9c941d353f34c3491ca2cde984e73cc7a87d) ) /* plane 2 */
	ROM_LOAD( "7709a.bin",  0x10000, 0x8000, CRC(74ceb5a8) SHA1(93ed6bb4a3c820f3a7ee5e9b2c2ce35d2bed8529) ) /* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7715.bin",  	0x000001, 0x08000, CRC(bf47e040) SHA1(5aa1b9adaa2095844c10993402a0597bb5768efb) )
	ROM_LOAD16_BYTE( "7719.bin",  	0x000000, 0x08000, CRC(fa5c5d6c) SHA1(6cac5d3fd705d1365348d57a18bbeb1eb9e412b8) )
	ROM_LOAD16_BYTE( "epr10013.c6",	0x010001, 0x08000, CRC(9a0919c5) SHA1(e39e60c1e834b3b46bf2ef1c5952841bebe66ade) )	/* 7716 */
	ROM_LOAD16_BYTE( "epr10017.b3",	0x010000, 0x08000, CRC(62aafd95) SHA1(e1e3a95fd11cabf81f44ac2dd3f951d3094725e6) )	/* 7720 */
	ROM_LOAD16_BYTE( "7717.bin",  	0x020001, 0x08000, CRC(fa64c86d) SHA1(ada722dd6efbf466a719ee1fe34a36ce1ea20184) )
	ROM_LOAD16_BYTE( "7721.bin",  	0x020000, 0x08000, CRC(62a9143e) SHA1(28f0dc0329163f0a6505dd34a24a843b35118c5e) )
	ROM_LOAD16_BYTE( "epr10015.c8",	0x030001, 0x08000, CRC(582d3b6a) SHA1(4f1d0060682e3fc1147082286e00e6a296a95da2) )	/* 7718 */
	ROM_LOAD16_BYTE( "epr10019.b5",	0x030000, 0x08000, CRC(e020c38b) SHA1(d13d38a64f2afa7df3cbccef2fe505a4421b73ad) )	/* 7722 */

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7710a.bin", 0x00000, 0x8000, CRC(a19b8ba8) SHA1(21b628d4ecbe38a6d96a39ca4252ff1cb728343f) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "7711.bin", 0x00000, 0x8000, CRC(efa9aabd) SHA1(b0928313b98159b95f3a6784c6279924774b9253) )
	ROM_LOAD( "7712.bin", 0x08000, 0x8000, CRC(7bcd85cf) SHA1(9acba6998327e1074d7311a9b6d06da9baf69aa0) )
	ROM_LOAD( "7713.bin", 0x10000, 0x8000, CRC(33f292e7) SHA1(4358cd3922a0dcbf109d2d697c7b8c4e090c3d52) )
	ROM_LOAD( "7714.bin", 0x18000, 0x8000, CRC(8fd48c47) SHA1(1cba63a9e7e0b477683b7758d124f4949558ba7a) )
ROM_END

// sys16B

ROM_START( dduxbl )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "dduxb03.bin", 0x000000, 0x20000, CRC(e7526012) SHA1(a1798008bfa1ce9b87dc330f3817b1978052fcfd) )
	ROM_LOAD16_BYTE( "dduxb05.bin", 0x000001, 0x20000, CRC(459d1237) SHA1(55e9c0dc341c919d58cc789203642c397d7ac65e) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "dduxb02.bin", 0x080000, 0x20000, CRC(d8ed3132) SHA1(a9d5ad8f79fb635cc234a99fad398688a5f15926) )
	ROM_LOAD16_BYTE( "dduxb04.bin", 0x080001, 0x20000, CRC(30c6cb92) SHA1(2e17c74eeb37c9731fc2e365cc0114f7383c0106) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "dduxb14.bin", 0x00000, 0x10000, CRC(664bd135) SHA1(674b06e01c2c8f5b8057dd24d470330c3f140473) )
	ROM_LOAD( "dduxb15.bin", 0x10000, 0x10000, CRC(ce0d2b30) SHA1(e60521c46f1650c9bdc76f2ceb91a6d61aaa0a09) )
	ROM_LOAD( "dduxb16.bin", 0x20000, 0x10000, CRC(6de95434) SHA1(7bed2a0261cf6c2fbb3756633f05f0bb2173977c) )

	ROM_REGION( 0xa0000, REGION_GFX2, 0 ) //* sprites */
	ROM_LOAD16_BYTE( "dduxb10.bin", 0x00001, 0x010000, CRC(0be3aee5) SHA1(48fc779b7398abbb82cd0d0d28705ece75b3c4e3) )
	ROM_RELOAD( 0x20001, 0x010000 )
	ROM_LOAD16_BYTE( "dduxb06.bin", 0x00000, 0x010000, CRC(b0079e99) SHA1(9bb4d3fa804a3d05a6e06b45a1280d7064e96ac6) )
	ROM_RELOAD( 0x20000, 0x010000 )
	ROM_LOAD16_BYTE( "dduxb11.bin", 0x40001, 0x010000, CRC(cfb2af18) SHA1(1ad18f933a7b797f0364d1f4a6c8549351b4c9a6) )
	ROM_LOAD16_BYTE( "dduxb07.bin", 0x40000, 0x010000, CRC(0217369c) SHA1(b6ec2fa1279a27a602d79e1073c54193745ea816) )
	ROM_LOAD16_BYTE( "dduxb12.bin", 0x60001, 0x010000, CRC(28ce9b15) SHA1(1640df9c8f21893c0647ad2f4210c714a06e6f37) )
	ROM_LOAD16_BYTE( "dduxb08.bin", 0x60000, 0x010000, CRC(8844f336) SHA1(18c1baaad3bcc658d4a6d03de8c97378b5284e88) )
	ROM_LOAD16_BYTE( "dduxb13.bin", 0x80001, 0x010000, CRC(efe57759) SHA1(69b8969b20ab9480df2735bd2bcd527069196bd7) )
	ROM_LOAD16_BYTE( "dduxb09.bin", 0x80000, 0x010000, CRC(6b64f665) SHA1(df07fcf2bbec6fa78f89b95272762aebd6f3ec0e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "dduxb01.bin", 0x0000, 0x8000, CRC(0dbef0d7) SHA1(8b9afb2fcb946cec467b1e691c267194b503f841) )
ROM_END

// sys16B


ROM_START( eswatbl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "eswat_c.rom", 0x000000, 0x10000, CRC(1028cc81) SHA1(24b4cd182419a44f3d6afa1c4273353024eb278f) )
	ROM_LOAD16_BYTE( "eswat_f.rom", 0x000001, 0x10000, CRC(f7b2d388) SHA1(8131ba8f4fa01751b9993c3c6c218c9bd3adb328) )
	ROM_LOAD16_BYTE( "eswat_b.rom", 0x020000, 0x10000, CRC(87c6b1b5) SHA1(a9f29e29a9c0e3daf272dce263a5fd7866642c77) )
	ROM_LOAD16_BYTE( "eswat_e.rom", 0x020001, 0x10000, CRC(937ddf9a) SHA1(9fc73f93e9c4221a4dc778593edc02cb405b2f78) )
	ROM_LOAD16_BYTE( "eswat_a.rom", 0x040000, 0x08000, CRC(2af4fc62) SHA1(f7b1539a5ab9560bd49dfecf44699abccfb649be) )
	ROM_LOAD16_BYTE( "eswat_d.rom", 0x040001, 0x08000, CRC(b4751e19) SHA1(57c9687dc864c163d13dbb89057cd42684a199cd) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr12624.b11", 0x00000, 0x40000, CRC(375a5ec4) SHA1(42b9116bdc0e0a5b1dd667ac1856b4c2252829ba) ) // ic19
	ROM_LOAD( "mpr12625.b12", 0x40000, 0x40000, CRC(3b8c757e) SHA1(0b66e8446d059a12e47e2a6fe8f0a333245bb95c) ) // ic20
	ROM_LOAD( "mpr12626.b13", 0x80000, 0x40000, CRC(3efca25c) SHA1(0d866bf53a16b52719f73081e933f4db27d72ece) ) // ic21

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12618.b1", 0x000001, 0x40000, CRC(0d1530bf) SHA1(bb8626cd98761c1c20cee117d00315c85621ba6a) ) // ic9
	ROM_LOAD16_BYTE( "mpr12621.b4", 0x000000, 0x40000, CRC(18ff0799) SHA1(5417223378aef16ee2b4f438d1f8f11a23fe7265) ) // ic12
	ROM_LOAD16_BYTE( "mpr12619.b2", 0x080001, 0x40000, CRC(32069246) SHA1(4913009bc72bf4f8b171b14fe06457f5784cab15) ) // ic10
	ROM_LOAD16_BYTE( "mpr12622.b5", 0x080000, 0x40000, CRC(a3dfe436) SHA1(640ccc552114d403f35d441574d2f3e4f1d4a8f9) ) // ic13
	ROM_LOAD16_BYTE( "mpr12620.b3", 0x100001, 0x40000, CRC(f6b096e0) SHA1(695ad1adbdc29f4d614645867e16de038cf92709) ) // ic11
	ROM_LOAD16_BYTE( "mpr12623.b6", 0x100000, 0x40000, CRC(6773fef6) SHA1(91e646ea447be02254d060daf255d26afe0cc79e) ) // ic14

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12617.a13",  0x00000, 0x08000, CRC(7efecf23) SHA1(2b87af7cfaab5942a3f7b38c987fcba01d3475ab) ) // ic8
	ROM_LOAD( "mpr12616.a11", 0x10000, 0x40000, CRC(254347c2) SHA1(bf2d83a69a5be375c7e42e9f7d6e65c1095a354c) ) // ic6
ROM_END

// sys16A
ROM_START( fantzono )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7385.43", 0x000000, 0x8000, CRC(5cb64450) SHA1(5831405359975dd7d8c6614b20fd9b18a5d6410d) )
	ROM_LOAD16_BYTE( "7382.26", 0x000001, 0x8000, CRC(3fda7416) SHA1(91f34cc8afb4ad8bc783c31d25781a1359c44cfe) )
	ROM_LOAD16_BYTE( "7386.42", 0x010000, 0x8000, CRC(15810ace) SHA1(e61a258ab6601d359f6ad1f37a2b2801bf777d26) )
	ROM_LOAD16_BYTE( "7383.25", 0x010001, 0x8000, CRC(a001e10a) SHA1(04ebb012b10817db36997d0ee877104d512decf8) )
	ROM_LOAD16_BYTE( "7387.41", 0x020000, 0x8000, CRC(0acd335d) SHA1(f39566a2069eefa7682c57c6521ea7a328738d06) )
	ROM_LOAD16_BYTE( "7384.24", 0x020001, 0x8000, CRC(fd909341) SHA1(2f1e01eb7d7b330c9c0dd98e5f8ed4973f0e93fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7388.95", 0x00000, 0x08000, CRC(8eb02f6b) SHA1(80511b944b57541669010bd5a0ca52bc98eabd62) )
	ROM_LOAD( "7389.94", 0x08000, 0x08000, CRC(2f4f71b8) SHA1(ceb39e95cd43904b8e4f89c7227491e139fb3ca6) )
	ROM_LOAD( "7390.93", 0x10000, 0x08000, CRC(d90609c6) SHA1(4232f6ecb21f242c0c8d81e06b88bc742668609f) )

	ROM_REGION( 0x30000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7392.10", 0x00001, 0x8000, CRC(5bb7c8b6) SHA1(eaa0ed63ac4f66ee285757e842bdd7b005292600) )
	ROM_LOAD16_BYTE( "7396.11", 0x00000, 0x8000, CRC(74ae4b57) SHA1(1f24b1faea765994b85f0e7ac8e944c8da22103f) )
	ROM_LOAD16_BYTE( "7393.17", 0x10001, 0x8000, CRC(14fc7e82) SHA1(ca7caca989a3577dd30ad4f66b0fcce712a454ef) )
	ROM_LOAD16_BYTE( "7397.18", 0x10000, 0x8000, CRC(e05a1e25) SHA1(9691d9f0763b7483ee6912437902f22ab4b78a05) )
	ROM_LOAD16_BYTE( "7394.23", 0x20001, 0x8000, CRC(531ca13f) SHA1(19e68bc515f6021e1145cff4f3f0e083839ee8f3) )
	ROM_LOAD16_BYTE( "7398.24", 0x20000, 0x8000, CRC(68807b49) SHA1(0a189da8cdd2090e76d6d06c55b478abce60542d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7535.12", 0x0000, 0x8000, CRC(0cb2126a) SHA1(42b18a81bed58ef59eaad929007eef89ad273dbb) )
ROM_END

ROM_START( fantzone )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7385a.43", 0x000000, 0x8000, CRC(4091af42) SHA1(1d4fdd32db9f75e5ccaab5766a50249ad71a60af) )
	ROM_LOAD16_BYTE( "epr7382a.26", 0x000001, 0x8000, CRC(77d67bfd) SHA1(886ce4c2d779cedd81f85737ef55fce3c94baa18) )
	ROM_LOAD16_BYTE( "epr7386a.42", 0x010000, 0x8000, CRC(b0a67cd0) SHA1(2e2bf2b7306fc567f7d13f89977543b368c19027) )
	ROM_LOAD16_BYTE( "epr7383a.25", 0x010001, 0x8000, CRC(5f79b2a9) SHA1(de3125bbd0a126fc5a67ba3134cd3f4608ebdfce) )
	ROM_LOAD16_BYTE( "7387.41", 0x020000, 0x8000, CRC(0acd335d) SHA1(f39566a2069eefa7682c57c6521ea7a328738d06) )
	ROM_LOAD16_BYTE( "7384.24", 0x020001, 0x8000, CRC(fd909341) SHA1(2f1e01eb7d7b330c9c0dd98e5f8ed4973f0e93fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7388.95", 0x00000, 0x08000, CRC(8eb02f6b) SHA1(80511b944b57541669010bd5a0ca52bc98eabd62) )
	ROM_LOAD( "7389.94", 0x08000, 0x08000, CRC(2f4f71b8) SHA1(ceb39e95cd43904b8e4f89c7227491e139fb3ca6) )
	ROM_LOAD( "7390.93", 0x10000, 0x08000, CRC(d90609c6) SHA1(4232f6ecb21f242c0c8d81e06b88bc742668609f) )

	ROM_REGION( 0x30000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7392.10", 0x00001, 0x8000, CRC(5bb7c8b6) SHA1(eaa0ed63ac4f66ee285757e842bdd7b005292600) )
	ROM_LOAD16_BYTE( "7396.11", 0x00000, 0x8000, CRC(74ae4b57) SHA1(1f24b1faea765994b85f0e7ac8e944c8da22103f) )
	ROM_LOAD16_BYTE( "7393.17", 0x10001, 0x8000, CRC(14fc7e82) SHA1(ca7caca989a3577dd30ad4f66b0fcce712a454ef) )
	ROM_LOAD16_BYTE( "7397.18", 0x10000, 0x8000, CRC(e05a1e25) SHA1(9691d9f0763b7483ee6912437902f22ab4b78a05) )
	ROM_LOAD16_BYTE( "7394.23", 0x20001, 0x8000, CRC(531ca13f) SHA1(19e68bc515f6021e1145cff4f3f0e083839ee8f3) )
	ROM_LOAD16_BYTE( "7398.24", 0x20000, 0x8000, CRC(68807b49) SHA1(0a189da8cdd2090e76d6d06c55b478abce60542d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr7535a.12", 0x0000, 0x8000, CRC(bc1374fa) SHA1(ed2c87ae024dc251e175239f1bccc728fc096548) )
ROM_END

// sys16B


ROM_START( fpointbl )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "flpoint.003", 0x000000, 0x10000, CRC(4d6df514) SHA1(168aa1629ab7152ba1984605155406b236954a2c) )
	ROM_LOAD16_BYTE( "flpoint.002", 0x000001, 0x10000, CRC(4dff2ee8) SHA1(bd157d8c168d45e7490a05d5e1e901d9bdda9599) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "flpoint.006", 0x00000, 0x10000, CRC(c539727d) SHA1(56674effe1d273128dddd2ff9e02974ec10f3fff) )
	ROM_LOAD( "flpoint.005", 0x10000, 0x10000, CRC(82c0b8b0) SHA1(e1e2e721cb8ad53df33065582dc90edeba9c3cab) )
	ROM_LOAD( "flpoint.004", 0x20000, 0x10000, CRC(522426ae) SHA1(90fd0a19b30a8a61dc4cfa66a64115596333dcc6) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12596.bin", 0x00001, 0x010000, CRC(4a4041f3) SHA1(4c52b30223d8aa80ccdbb196098cb17e64ad6583) )
	ROM_LOAD16_BYTE( "12597.bin", 0x00000, 0x010000, CRC(6961e676) SHA1(7639d2da086b57a9a8d6100fdacf40d97d7c4772) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "flpoint.001", 0x0000, 0x8000, CRC(c5b8e0fe) SHA1(6cf8c67151d8604326fc6dbf976c0635b452a844) )	// bootleg rom doesn't work, but should be correct!
ROM_END

ROM_START( fpointbj )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "boot2.003", 0x000000, 0x10000, CRC(6c00d1b0) SHA1(fd0c47b8ca010a64d3ef91980f93854ebc98fbda) )
	ROM_LOAD16_BYTE( "boot2.002", 0x000001, 0x10000, CRC(c1fcd704) SHA1(697bef464e53fb9891ed15ee2d6210107b693b20) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT ) /* tiles */
	ROM_LOAD( "flpoint.006", 0x00000, 0x10000, CRC(c539727d) SHA1(56674effe1d273128dddd2ff9e02974ec10f3fff) )
	ROM_LOAD( "flpoint.005", 0x10000, 0x10000, CRC(82c0b8b0) SHA1(e1e2e721cb8ad53df33065582dc90edeba9c3cab) )
	ROM_LOAD( "flpoint.004", 0x20000, 0x10000, CRC(522426ae) SHA1(90fd0a19b30a8a61dc4cfa66a64115596333dcc6) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12596.bin", 0x00001, 0x010000, CRC(4a4041f3) SHA1(4c52b30223d8aa80ccdbb196098cb17e64ad6583) )
	ROM_LOAD16_BYTE( "12597.bin", 0x00000, 0x010000, CRC(6961e676) SHA1(7639d2da086b57a9a8d6100fdacf40d97d7c4772) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "flpoint.001", 0x0000, 0x8000, CRC(c5b8e0fe) SHA1(6cf8c67151d8604326fc6dbf976c0635b452a844) )	// bootleg rom doesn't work, but should be correct!

	/* stuff below isn't used but loaded because it was on the board .. */
	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.1",  0x0000, 0x0100, CRC(a7c22d96) SHA1(160deae8053b09c09328325246598b3518c7e20b) )
	ROM_LOAD( "82s123.2",  0x0100, 0x0020, CRC(58bcf8bd) SHA1(e4d3d179b08c0f3424a6bec0f15058fb1b56f8d8) )

	ROM_REGION( 0x800, REGION_USER2, 0 )
	ROM_LOAD( "d2716.rom",  0x0000, 0x0800, CRC(d7fd8ac4) SHA1(87e5f1c24350adab129ad79a1f68af402580f8f0) )
ROM_END

// sys16B
ROM_START( goldnaxe )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12523.a7", 0x00000, 0x20000, CRC(8e6128d7) SHA1(b8de216f4ca08815ca98d39a773024d191d21b4d) )
	ROM_LOAD16_BYTE( "epr12522.a5", 0x00001, 0x20000, CRC(b6c35160) SHA1(88015d0a486f56911360362c96a82f36a13de886) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr12521.a8", 0x80000, 0x20000, CRC(5001d713) SHA1(68cf3f48d6e440e5b800503a211adda02107d956) )
	ROM_LOAD16_BYTE( "epr12519.a6", 0x80001, 0x20000, CRC(4438ca8e) SHA1(0af53d64f06abf41f4c46540d28d5f008a4835a3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

ROM_START( goldnabl )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
// protected code
	ROM_LOAD16_BYTE( "ga6.a22", 0x00000, 0x10000, CRC(f95b459f) SHA1(dadf66d63454ed62fefa521d4fed249d28c63778) )
	ROM_LOAD16_BYTE( "ga4.a20", 0x00001, 0x10000, CRC(83eabdf5) SHA1(1effef966f513fbdec2026d535658e17ef7dea51) )
	ROM_LOAD16_BYTE( "ga11.a27",0x20000, 0x10000, CRC(f4ef9349) SHA1(3ffa335e74ffbc10f80387268da659643c566897) )
	ROM_LOAD16_BYTE( "ga8.a24", 0x20001, 0x10000, CRC(37a65839) SHA1(6e8055d91b840afd8526041d3752c0a55eaebe0c) )
	/* emtpy 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr12521.a8", 0x80000, 0x20000, CRC(5001d713) SHA1(68cf3f48d6e440e5b800503a211adda02107d956) )
	ROM_LOAD16_BYTE( "epr12519.a6", 0x80001, 0x20000, CRC(4438ca8e) SHA1(0af53d64f06abf41f4c46540d28d5f008a4835a3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ga33.b16", 0x00000, 0x10000, CRC(84587263) SHA1(3a88c8578a477a487a0a214a367042b9739f39eb) )
	ROM_LOAD( "ga32.b15", 0x10000, 0x10000, CRC(63d72388) SHA1(ba0a582b1daf3a1e316237efbad17fcc0381643f) )
	ROM_LOAD( "ga31.b14", 0x20000, 0x10000, CRC(f8b6ae4f) SHA1(55132c98955107e4b247992f7917a6ce588460a7) )
	ROM_LOAD( "ga30.b13", 0x30000, 0x10000, CRC(e29baf4f) SHA1(3761cb2217599fe3f2f860f9395930b96ec52f47) )
	ROM_LOAD( "ga29.b12", 0x40000, 0x10000, CRC(22f0667e) SHA1(2d11b2ce105a3db9c914942cace85aff17deded9) )
	ROM_LOAD( "ga28.b11", 0x50000, 0x10000, CRC(afb1a7e4) SHA1(726fded9db72a881128b43f449d2baf450131f63) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	/* wrong! */
	ROM_LOAD16_BYTE( "ga34.b17", 		0x000001, 0x10000, CRC(28ba70c8) SHA1(a6f33e1404928b6d1006943494646d6cfbd60a4b) )
	ROM_LOAD16_BYTE( "ga35.b18", 		0x010000, 0x10000, CRC(2ed96a26) SHA1(edcf915243e6f92d31cdfc53965438f6b6bff51d) )
	ROM_LOAD16_BYTE( "ga23.a14", 		0x020001, 0x10000, CRC(84dccc5b) SHA1(10263d98d663f1170c3203066f391075a1d64ff5) )
	ROM_LOAD16_BYTE( "ga18.a9",  		0x030000, 0x10000, CRC(de346006) SHA1(65aa489373b6d2cccbb024f13fc190a7cae86274) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 	0x040001, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "ga36.b19", 		0x080000, 0x10000, CRC(101d2fff) SHA1(1de1390c5f55f192491053c8aac31be3389aab2b) )
	ROM_LOAD16_BYTE( "ga37.b20", 		0x090001, 0x10000, CRC(677e64a6) SHA1(e3d0d31097017c6cb1a7f41292783f18ce13b41c) )
	ROM_LOAD16_BYTE( "ga19.a10", 		0x0a0000, 0x10000, CRC(11794d05) SHA1(eef52d7a644dbcc5f983222f163445a725286a32) )
	ROM_LOAD16_BYTE( "ga20.a11", 		0x0b0001, 0x10000, CRC(ad1c1c90) SHA1(155f17593cfab1a117bb755b1edd0c473d455f91) )
	ROM_LOAD16_BYTE( "mpr12381.b5",	0x0c0000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3",	0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6",	0x140000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

// sys16B
ROM_START( goldnaxa )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12545.a2", 0x00000, 0x40000, CRC(a97c4e4d) SHA1(41cda15ae56185725233db669d9f8c4a8c1eb1c3) )
	ROM_LOAD16_BYTE( "epr12544.a1", 0x00001, 0x40000, CRC(5e38f668) SHA1(3b15a9a30adde6e852c439c8e6e45875b66252cb) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

// pre16
ROM_START( mjleague )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7404.09b", 0x000000, 0x8000, CRC(ec1655b5) SHA1(5c1df364fa9733daa4478c5f88298089e4963c33) )
	ROM_LOAD16_BYTE( "epr-7401.06b", 0x000001, 0x8000, CRC(2befa5e0) SHA1(0a1681a4c7d62a5754ba6f3845436b4d08324246) )
	ROM_LOAD16_BYTE( "epr-7405.10b", 0x010000, 0x8000, CRC(7a4f4e38) SHA1(65a22097dd933e83f326bd64b3863915897780a6) )
	ROM_LOAD16_BYTE( "epr-7402.07b", 0x010001, 0x8000, CRC(b7bef762) SHA1(214450e0b094f99ef38dec2a3e5cbdb0b30e917d) )
	ROM_LOAD16_BYTE( "epra7406.11b", 0x020000, 0x8000, CRC(bb743639) SHA1(5d99638a79f02ce14374d3b1f3d9fbfc5c13c6e1) )
	ROM_LOAD16_BYTE( "epra7403.08b", 0x020001, 0x8000, CRC(d86250cf) SHA1(fb5dabb7b9b9fe0bbe93e28c60311c7b3256107a) )	// Fails memory test. Bad rom?

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr-7051.09a", 0x00000, 0x08000, CRC(10ca255a) SHA1(ccf58ffcac2f7fbdbfbdf32601a1b97f359cbd91) )
	ROM_LOAD( "epr-7052.10a", 0x08000, 0x08000, CRC(2550db0e) SHA1(28f8d68f43d26f12793fe295c205cc86adc4e96a) )
	ROM_LOAD( "epr-7053.11a", 0x10000, 0x08000, CRC(5bfea038) SHA1(01dc6e14cc7bba9f7930e68573c441fa2841f49a) )

	ROM_REGION( 0x50000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-7055.05a", 0x000001, 0x8000, CRC(1fb860bd) SHA1(4a4155d0352dfae9e402a2b2f1558ef17b1303b4) )
	ROM_LOAD16_BYTE( "epr-7059.02b", 0x000000, 0x8000, CRC(3d14091d) SHA1(36208415b2012b6e948fefa15b0f7041748066be) )
	ROM_LOAD16_BYTE( "epr-7056.06a", 0x010001, 0x8000, CRC(b35dd968) SHA1(e306b5e38acf583d7b2089302622ad25ae5564b0) )
	ROM_LOAD16_BYTE( "epr-7060.03b", 0x010000, 0x8000, CRC(61bb3757) SHA1(5c87cf23be22b84e3dae746527ca057d870d6397) )
	ROM_LOAD16_BYTE( "epr-7057.07a", 0x020001, 0x8000, CRC(3e5a2b6f) SHA1(d3dbafb4acb916e02c978a156008bd75ba122fb7) )
	ROM_LOAD16_BYTE( "epr-7061.04b", 0x020000, 0x8000, CRC(c808dad5) SHA1(9b65acc8dc23b16e56327298188d1a6ab48b2b5d) )
	ROM_LOAD16_BYTE( "epr-7058.08a", 0x030001, 0x8000, CRC(b543675f) SHA1(35ffc9295a8849a18fabe156fdbc9801ea2179cd) )
	ROM_LOAD16_BYTE( "epr-7062.05b", 0x030000, 0x8000, CRC(9168eb47) SHA1(daaa7836e627a0679e65373d8f20a9383ba4c905) )
//	ROM_LOAD16_BYTE( "epr-7055.05a", 0x040001, 0x8000, CRC(1fb860bd) SHA1(4a4155d0352dfae9e402a2b2f1558ef17b1303b4) ) loaded twice??
//	ROM_LOAD16_BYTE( "epr-7059.02b", 0x040000, 0x8000, CRC(3d14091d) SHA1(36208415b2012b6e948fefa15b0f7041748066be) ) loaded twice??

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "eprc7054.01b", 0x00000, 0x8000, CRC(4443b744) SHA1(73359a6e9d62b382dee47fea31b9e17eb26a0321) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr-7063.01a", 0x00000, 0x8000, CRC(45d8908a) SHA1(e61f81f953c1a744ded36fed3b55774e4747af29) )
	ROM_LOAD( "epr-7065.02a", 0x08000, 0x8000, CRC(8c8f8cff) SHA1(fca5a916a8b25800ee5e8771e2ced0ed9bd737f4) )
	ROM_LOAD( "epr-7064.03a", 0x10000, 0x8000, CRC(159f6636) SHA1(66fa3f3e95a6ef3d3ff4ded09c05ab1131d9fbbb) )
	ROM_LOAD( "epr-7066.04a", 0x18000, 0x8000, CRC(f5cfa91f) SHA1(c85d68cbcd03fe1436bed12235c033610acc11ee) )
ROM_END

ROM_START( passht4b )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "pas4p.3", 0x000000, 0x10000, CRC(2d8bc946) SHA1(35d3e529d4815543d9876fd0545c3d686467abaa) )
	ROM_LOAD16_BYTE( "pas4p.4", 0x000001, 0x10000, CRC(e759e831) SHA1(dd5727dc28010cb988e4951723171171eb645ce8) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "pas4p.11", 0x00000, 0x10000, CRC(da20fbc9) SHA1(21dc8143f4d1cebae4f86e83495fa84e5293ba48) )
	ROM_LOAD( "pas4p.12", 0x10000, 0x10000, CRC(bebb9211) SHA1(4f56048f6f70b63f74a4c0d64456213d36ce5b26) )
	ROM_LOAD( "pas4p.13", 0x20000, 0x10000, CRC(e37506c3) SHA1(e6fbf15d58f321a3d052fefbe5a1901e4a1734ae) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11862.b1",  0x00001, 0x10000, CRC(b6e94727) SHA1(0838e034f1f10d9cd1312c8c94b5c57387c0c271) )
	ROM_LOAD16_BYTE( "opr11865.b5",  0x00000, 0x10000, CRC(17e8d5d5) SHA1(ac1074b0a705be13c6e3391441e6cfec1d2b3f8a) )
	ROM_LOAD16_BYTE( "opr11863.b2",  0x20001, 0x10000, CRC(3e670098) SHA1(2cfc83f4294be30cd868738886ac546bd8489962) )
	ROM_LOAD16_BYTE( "opr11866.b6",  0x20000, 0x10000, CRC(50eb71cc) SHA1(463b4917ca19c7f4ad2c2845caa104d5e4a2dda3) )
	ROM_LOAD16_BYTE( "opr11864.b3",  0x40001, 0x10000, CRC(05733ca8) SHA1(1dbc7c99450ebe6a9fd8c0244fd3cb38b74984ef) )
	ROM_LOAD16_BYTE( "opr11867.b7",  0x40000, 0x10000, CRC(81e49697) SHA1(a70fa409e3555ad6c8f28930a7026fdf2deb8c65) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "pas4p.1",  0x00000, 0x08000, CRC(e60fb017) SHA1(21298036eab55c74427f1c2e3a9623d41bca4849) )
	ROM_LOAD( "pas4p.2",  0x10000, 0x10000, CRC(092e016e) SHA1(713638749efa9dce19c547b84308236110bc85fe) )
ROM_END

ROM_START( passshtb )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "pass3_2p.bin", 0x000000, 0x10000, CRC(26bb9299) SHA1(11bacf86dfdd8bcfbfb61f0ebc59890325c48adc) )
	ROM_LOAD16_BYTE( "pass4_2p.bin", 0x000001, 0x10000, CRC(06ac6d5d) SHA1(2dd71a8a956404326797de8beed7bca016c9919e) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr11854.b9",  0x00000, 0x10000, CRC(d31c0b6c) SHA1(610d04988da70c30300cc5614817eda9d2204f39) )
	ROM_LOAD( "opr11855.b10", 0x10000, 0x10000, CRC(b78762b4) SHA1(d594ef846bd7fed8da91a89906b39c1a2867a1fe) )
	ROM_LOAD( "opr11856.b11", 0x20000, 0x10000, CRC(ea49f666) SHA1(36ccd32cdcbb7fcc300628bb59c220ec3c324d82) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11862.b1",  0x00001, 0x10000, CRC(b6e94727) SHA1(0838e034f1f10d9cd1312c8c94b5c57387c0c271) )
	ROM_LOAD16_BYTE( "opr11865.b5",  0x00000, 0x10000, CRC(17e8d5d5) SHA1(ac1074b0a705be13c6e3391441e6cfec1d2b3f8a) )
	ROM_LOAD16_BYTE( "opr11863.b2",  0x20001, 0x10000, CRC(3e670098) SHA1(2cfc83f4294be30cd868738886ac546bd8489962) )
	ROM_LOAD16_BYTE( "opr11866.b6",  0x20000, 0x10000, CRC(50eb71cc) SHA1(463b4917ca19c7f4ad2c2845caa104d5e4a2dda3) )
	ROM_LOAD16_BYTE( "opr11864.b3",  0x40001, 0x10000, CRC(05733ca8) SHA1(1dbc7c99450ebe6a9fd8c0244fd3cb38b74984ef) )
	ROM_LOAD16_BYTE( "opr11867.b7",  0x40000, 0x10000, CRC(81e49697) SHA1(a70fa409e3555ad6c8f28930a7026fdf2deb8c65) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11857.a7",  0x00000, 0x08000, CRC(789edc06) SHA1(8c89c94e503513c287807d187de78a7fbd75a7cf) )
	ROM_LOAD( "epr11858.a8",  0x10000, 0x08000, CRC(08ab0018) SHA1(0685f80a7d403208c9cfffea3f2035324f3924fe) )
	ROM_LOAD( "epr11859.a9",  0x18000, 0x08000, CRC(8673e01b) SHA1(e79183ab30e683fdf61ced2e9dbe010567c324cb) )
	ROM_LOAD( "epr11860.a10", 0x20000, 0x08000, CRC(10263746) SHA1(1f981fb185c6a9795208ecdcfba36cf892a99ed5) )
	ROM_LOAD( "epr11861.a11", 0x28000, 0x08000, CRC(38b54a71) SHA1(68ec4ef5b115844214ff2213be1ce6678904fbd2) )
ROM_END

// pre16
ROM_START( quartet )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7458a.9b",  0x000000, 0x8000, CRC(42e7b23e) SHA1(9df3b1b915723f9a927ef03d80ae7983a8c91a21) )
	ROM_LOAD16_BYTE( "epr7455a.6b",  0x000001, 0x8000, CRC(01631ab2) SHA1(2d613d23fe79072f850ccc9020830dea54312b23) )
	ROM_LOAD16_BYTE( "epr7459a.10b", 0x010000, 0x8000, CRC(6b540637) SHA1(4b2e9ba06b80f8fb502310ab770805f8c6a47567) )
	ROM_LOAD16_BYTE( "epr7456a.7b",  0x010001, 0x8000, CRC(31ca583e) SHA1(8ade8f7e42ae3e171b138410374e4c090fdc4ecb) )
	ROM_LOAD16_BYTE( "epr7460.11b",  0x020000, 0x8000, CRC(a444ea13) SHA1(884ed22d606e3bd30d8401fe1750687e54674e82) )
	ROM_LOAD16_BYTE( "epr7457.8b",   0x020001, 0x8000, CRC(3b282c23) SHA1(95de41a97f50f6169887c6d9724d5c42a41bb264) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr7461.9c",  0x00000, 0x08000, CRC(f6af07f2) SHA1(546fabbda936d61a90d2395d033fd4d6bb0bc38a) )
	ROM_LOAD( "epr7462.10c", 0x08000, 0x08000, CRC(7914af28) SHA1(4bf59fe4a0b0aa5d4cc0b6f9375ffab3c96e8a2b) )
	ROM_LOAD( "epr7463.11c", 0x10000, 0x08000, CRC(827c5603) SHA1(8db3bd6eae5aeeb229e017471049ef5347974df5) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites  - the same as quartet 2 */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

ROM_START( quartetj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7458.43",  0x000000, 0x8000, CRC(0096499f) SHA1(dcf8e33513ce7c6660ea546c8e1c574fde629a22) )
	ROM_LOAD16_BYTE( "epr-7455.26",  0x000001, 0x8000, CRC(da934390) SHA1(d40eb65b6a36a4c1ebeadb76e47a61bd8b2e4b89) )
	ROM_LOAD16_BYTE( "epr-7459.42",  0x010000, 0x8000, CRC(d130cf61) SHA1(3a065f5c296b10b97c78d49aa285ae7afb16e881) )
	ROM_LOAD16_BYTE( "epr-7456.25",  0x010001, 0x8000, CRC(7847149f) SHA1(fc8ad669f2bc426cb7af78d92ea147cbd1e181af) )
	ROM_LOAD16_BYTE( "epr7460.11b",  0x020000, 0x8000, CRC(a444ea13) SHA1(884ed22d606e3bd30d8401fe1750687e54674e82) )
	ROM_LOAD16_BYTE( "epr7457.8b",   0x020001, 0x8000, CRC(3b282c23) SHA1(95de41a97f50f6169887c6d9724d5c42a41bb264) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr7461.9c",  0x00000, 0x08000, CRC(f6af07f2) SHA1(546fabbda936d61a90d2395d033fd4d6bb0bc38a) )
	ROM_LOAD( "epr7462.10c", 0x08000, 0x08000, CRC(7914af28) SHA1(4bf59fe4a0b0aa5d4cc0b6f9375ffab3c96e8a2b) )
	ROM_LOAD( "epr7463.11c", 0x10000, 0x08000, CRC(827c5603) SHA1(8db3bd6eae5aeeb229e017471049ef5347974df5) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites  - the same as quartet 2 */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

// pre16
ROM_START( quartet2 )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "quartet2.b9",  0x000000, 0x8000, CRC(67177cd8) SHA1(c4ea001dfbeeb29a09d597fb50d71f54e4e9572a) )
	ROM_LOAD16_BYTE( "quartet2.b6",  0x000001, 0x8000, CRC(50f50b08) SHA1(646c0d545150b95e5d8d47bf63360f7326add08f) )
	ROM_LOAD16_BYTE( "quartet2.b10", 0x010000, 0x8000, CRC(4273c3b7) SHA1(4cae221678a6d2b7806487becd4ba09b520f9fa0) )
	ROM_LOAD16_BYTE( "quartet2.b7",  0x010001, 0x8000, CRC(0aa337bb) SHA1(f31f8f294fccd866eadebfafee067bfae44b3184) )
	ROM_LOAD16_BYTE( "quartet2.b11", 0x020000, 0x8000, CRC(3a6a375d) SHA1(8ebea6b7f1208438b47e887b46cb569725c4042a) )
	ROM_LOAD16_BYTE( "quartet2.b8",  0x020001, 0x8000, CRC(d87b2ca2) SHA1(58adf0900e41036b1b78a931ab94b30ce601909d) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "quartet2.c9",  0x00000, 0x08000, CRC(547a6058) SHA1(5248e974c8d12183c996b1fc8fda09e8a4bf0d2d) )
	ROM_LOAD( "quartet2.c10", 0x08000, 0x08000, CRC(77ec901d) SHA1(b5961895473c16a8f4a111185cce48b05ab66885) )
	ROM_LOAD( "quartet2.c11", 0x10000, 0x08000, CRC(7e348cce) SHA1(82bba65280faaf3280208c85caef48ec8baeade8) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

ROM_START( quartt2j )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7728.43",  0x000000, 0x8000, CRC(56a8c88e) SHA1(33eaca5272f3588058952ca0b1fa298b89418e81) )
	ROM_LOAD16_BYTE( "epr-7725.26",  0x000001, 0x8000, CRC(ee15fcc9) SHA1(70d9755145245537f6aeb0d39abeda7811749b8c) )
	ROM_LOAD16_BYTE( "epr-7729.42",  0x010000, 0x8000, CRC(bc242123) SHA1(8e58dd89b70ba06d12437010a7375464647262f5) )
	ROM_LOAD16_BYTE( "epr-7726.25",  0x010001, 0x8000, CRC(9d1c48e7) SHA1(e11a358895c7809cdf7241ff9317c2b162e4040e) )
	ROM_LOAD16_BYTE( "quartet2.b11", 0x020000, 0x8000, CRC(3a6a375d) SHA1(8ebea6b7f1208438b47e887b46cb569725c4042a) )
	ROM_LOAD16_BYTE( "quartet2.b8",  0x020001, 0x8000, CRC(d87b2ca2) SHA1(58adf0900e41036b1b78a931ab94b30ce601909d) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "quartet2.c9",  0x00000, 0x08000, CRC(547a6058) SHA1(5248e974c8d12183c996b1fc8fda09e8a4bf0d2d) )
	ROM_LOAD( "quartet2.c10", 0x08000, 0x08000, CRC(77ec901d) SHA1(b5961895473c16a8f4a111185cce48b05ab66885) )
	ROM_LOAD( "quartet2.c11", 0x10000, 0x08000, CRC(7e348cce) SHA1(82bba65280faaf3280208c85caef48ec8baeade8) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

// sys16A
ROM_START( sdioj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	// Custom CPU 317-0027
	ROM_LOAD16_BYTE( "epr10970.43", 0x000000, 0x8000, CRC(b8fa4a2c) SHA1(06b448bbee0a2b2809d9af7a2a22c5847343c079) )
	ROM_LOAD16_BYTE( "epr10968.26", 0x000001, 0x8000, CRC(a3f97793) SHA1(0f924fae0d13b3387a0e5171482f6d413432ddb3) )
	ROM_LOAD16_BYTE( "epr10971.42", 0x010000, 0x8000, CRC(c44a0328) SHA1(3736bb83e728bb0e15ea58bc2a6c2fe66a1a4885) )
	ROM_LOAD16_BYTE( "epr10969.25", 0x010001, 0x8000, CRC(455d15bd) SHA1(be679ecb1687b0675614ad27973c20808ad53797) )
	ROM_LOAD16_BYTE( "epr10755.41", 0x020000, 0x8000, CRC(405e3969) SHA1(6d8c3bd06d35c971f7db005dffa2e83cae1378f8) )
	ROM_LOAD16_BYTE( "epr10752.24", 0x020001, 0x8000, CRC(77453740) SHA1(9032463e5e14c3c610c31e2eb6e2c962df9adf46) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10756.95", 0x00000, 0x10000, CRC(44d8a506) SHA1(363d49dcb65ac0093f3ed3b259b1bc45f0291e9d) )
	ROM_LOAD( "epr10757.94", 0x10000, 0x10000, CRC(497e1740) SHA1(95b166a9db46a27087e417c1b2cbb76bee2e64a7) )
	ROM_LOAD( "epr10758.93", 0x20000, 0x10000, CRC(61d61486) SHA1(d48ff87216947b78903cd98a10436babdf8b75a0) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "b1.rom", 0x00001, 0x10000, CRC(30e2c50a) SHA1(1fb9e69d4cb97fdcb0f98c2a7ede246aaa4ac382) )
	ROM_LOAD16_BYTE( "b5.rom", 0x00000, 0x10000, CRC(794e3e8b) SHA1(91ca1cb9aabf99adc8426feed4494a992afb8c4a) )
	ROM_LOAD16_BYTE( "b2.rom", 0x20001, 0x10000, CRC(6a8b3fd0) SHA1(a122d3cb0b3263714f026e57d85b0dbf6cb110d7) )
	ROM_LOAD16_BYTE( "b6.rom", 0x20000, 0x10000, CRC(602da5d5) SHA1(d32cdde7d86c4561e7bfa547d7d7995ce9a43c24) )
	ROM_LOAD16_BYTE( "b3.rom", 0x40001, 0x10000, CRC(b9de3aeb) SHA1(2f7a55a8377e831338a884f8962d6ab2757e8c9b) )
	ROM_LOAD16_BYTE( "b7.rom", 0x40000, 0x10000, CRC(0a73a057) SHA1(7f31124c67541a245e069e5b6aac59935d99a9a9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10759.12", 0x0000, 0x8000, CRC(d7f9649f) SHA1(ce4abe7dd7e33da048569d7817063345fab75ea7) )
ROM_END

// sys16B
ROM_START( shinobi )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "shinobi.a4", 0x00000, 0x10000, CRC(b930399d) SHA1(955ff2948e1990463631b0bc5c7f5275384236cc) )
	ROM_LOAD16_BYTE( "shinobi.a1", 0x00001, 0x10000, CRC(343f4c46) SHA1(2cf5d00462ad85ae9a2e16d59171c8ab85e10f49) )
	ROM_LOAD16_BYTE( "epr11283",   0x20000, 0x10000, CRC(9d46e707) SHA1(37ab25b3b37365c9f45837bfb6ec80652691dd4c) )
	ROM_LOAD16_BYTE( "epr11281",   0x20001, 0x10000, CRC(7961d07e) SHA1(38cbdab35f901532c0ad99ad0083513abd2ff182) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "shinobi.b9",  0x00000, 0x10000, CRC(5f62e163) SHA1(03f008745a1af84142ada647acf3601049f43ad5) )
	ROM_LOAD( "shinobi.b10", 0x10000, 0x10000, CRC(75f8fbc9) SHA1(29072edcd583af60ec66b4c8bb82b179a3751edf) )
	ROM_LOAD( "shinobi.b11", 0x20000, 0x10000, CRC(06508bb9) SHA1(57c9036123ec8e35d0275ab6eaff25a16aa203d4) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "shinobi.a7", 0x00000, 0x8000, CRC(2457a7cf) SHA1(ddfac640e442537acb015de8bb088659f5a217ee) )
	ROM_LOAD( "shinobi.a8", 0x10000, 0x8000, CRC(c8df8460) SHA1(0aeb41a493df155edb5f600f53ec43b798927dff) )
	ROM_LOAD( "shinobi.a9", 0x18000, 0x8000, CRC(e5a4cf30) SHA1(d1982da7a550c11ab2253f5d64ac6ab847da0a04) )
ROM_END

// sys16A
ROM_START( shinobia )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	// Custom CPU 317-0050
	ROM_LOAD16_BYTE( "epr11262.42", 0x000000, 0x10000, CRC(d4b8df12) SHA1(64bfa2dd8a3d99728d9eeb114887272d9590d0b8) )
	ROM_LOAD16_BYTE( "epr11260.27", 0x000001, 0x10000, CRC(2835c95d) SHA1(b5b42af265d3a16183e02d58b053ec2894072679) )
	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) )
	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11264.95", 0x00000, 0x10000, CRC(46627e7d) SHA1(66bb5b22a2100e7b9df303007a837bc2d52cf7ba) )
	ROM_LOAD( "epr11265.94", 0x10000, 0x10000, CRC(87d0f321) SHA1(885b38eaff2dcaeab4eeaa20cc8a2885d520abd6) )
	ROM_LOAD( "epr11266.93", 0x20000, 0x10000, CRC(efb4af87) SHA1(0b8a905023e1bc808fd2b1c3cfa3778cde79e659) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11267.12", 0x0000, 0x8000, CRC(dd50b745) SHA1(52e1977569d3713ad864d607170c9a61cd059a65) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr11268.1", 0x0000, 0x8000, CRC(6d7966da) SHA1(90f55a99f784c21d7c135e630f4e8b1d4d043d66) )
ROM_END


ROM_START( shinobl )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
// Star Bootleg
	ROM_LOAD16_BYTE( "b3",          0x000000, 0x10000, CRC(38e59646) SHA1(6a13015a93260ab99811b95950bb122eade01c27) )
	ROM_LOAD16_BYTE( "b1",          0x000001, 0x10000, CRC(8529d192) SHA1(202b912d20a2d82abe055b4a5e8c509ab7d69ff8) )
	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) )
	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) )

// Beta Bootleg
//	ROM_LOAD16_BYTE( "4",           0x000000, 0x10000, CRC(c178a39c) )
//	ROM_LOAD16_BYTE( "2",           0x000001, 0x10000, CRC(5ad8ebf2) )
//	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) )
//	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11264.95", 0x00000, 0x10000, CRC(46627e7d) SHA1(66bb5b22a2100e7b9df303007a837bc2d52cf7ba) )
	ROM_LOAD( "epr11265.94", 0x10000, 0x10000, CRC(87d0f321) SHA1(885b38eaff2dcaeab4eeaa20cc8a2885d520abd6) )
	ROM_LOAD( "epr11266.93", 0x20000, 0x10000, CRC(efb4af87) SHA1(0b8a905023e1bc808fd2b1c3cfa3778cde79e659) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
//	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )
	ROM_LOAD16_BYTE( "b17",         0x60000, 0x10000, CRC(0315cf42) SHA1(2d129171aece883cb9c2805f894b3867ec98332b) )	// Beta bootleg uses the rom above.

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11267.12", 0x0000, 0x8000, CRC(dd50b745) SHA1(52e1977569d3713ad864d607170c9a61cd059a65) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr11268.1", 0x0000, 0x8000, CRC(6d7966da) SHA1(90f55a99f784c21d7c135e630f4e8b1d4d043d66) )
ROM_END

// sys16B

// sys16A custom

/*

 Tetris
 Sega System 16A
 CPU: FD1094 (317-0093)

 Top board

 Pos.   Label       Part        Notes

 D4     EPR-12205   27C256      Z80 program
 D8     EPR-12200   27C256      68000 program
 C8     Unused                  68000 program
 B8     Unused                  68000 program
 D11    EPR-12201   27C256      68000 program
 C11    Unused                  68000 program
 B11    Unused                  68000 program
 C18    EPR-12204   27C512      Tile data
 D18    EPR-12203   27C512      Tile data
 E18    EPR-12202   27C512      Tile data

 Bottom board

 Pos.   Label       Part        Notes

 D3     EPR-12169   27C256      Sprite data
 D4     Unused                  Sprite data
 D5     Unused                  Sprite data
 D6     Unused                  Sprite data
 F3     EPR-12170   27C256      Sprite data
 F4     Unused                  Sprite data
 F5     Unused                  Sprite data
 F6     Unused                  Sprite data

*/

ROM_START( tetris )
	ROM_REGION( 0x040000, REGION_CPU1, ROMREGION_ERASEFF ) /* 68000 code */
	// Custom CPU 317-0093
	ROM_LOAD16_BYTE( "epr12201.rom", 0x000000, 0x8000, CRC(338e9b51) SHA1(f56a1124c963d4ad72a806b26f9aa906aaa37d2b) )
	ROM_LOAD16_BYTE( "epr12200.rom", 0x000001, 0x8000, CRC(fb058779) SHA1(0045985ea943ebc7e44bd95127c5e5212c2821e8) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )	/* decryption key */
	ROM_LOAD( "317-0093.key", 0x0000, 0x2000, CRC(e0064442) SHA1(cc70b1a2c66729c4540dabd6a24a5f5615beedcd) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12202.rom", 0x00000, 0x10000, CRC(2f7da741) SHA1(51a685673b4a57a13818eca65d122230f20bd9a0) )
	ROM_LOAD( "epr12203.rom", 0x10000, 0x10000, CRC(a6e58ec5) SHA1(5a6c43c989768270e0ab61cfaa5ef86d4607fe20) )
	ROM_LOAD( "epr12204.rom", 0x20000, 0x10000, CRC(0ae98e23) SHA1(f067b81b85f9e03a6373c7c53ff52d5395b8a985) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12169.b1", 0x0001, 0x8000, CRC(dacc6165) SHA1(87b1a7643e3630ff73b2b117752496e1ea5da23d) )
	ROM_LOAD16_BYTE( "epr12170.b5", 0x0000, 0x8000, CRC(87354e42) SHA1(e7fd55aee59b51d82cb9b619fbb815ad6839560c) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12205.rom", 0x0000, 0x8000, CRC(6695dc99) SHA1(08123aa24c302bc9243329384bd9c2545a4d50c3) )
ROM_END

// sys16B
ROM_START( tetrisbl )
	ROM_REGION( 0x040000, REGION_CPU1, ROMREGION_ERASEFF ) /* 68000 code */
	ROM_LOAD16_BYTE( "rom2.bin", 0x000000, 0x10000, CRC(4d165c38) SHA1(04706b1977ae18bd09bafaf8ea65f8e5f32e04b8) )
	ROM_LOAD16_BYTE( "rom1.bin", 0x000001, 0x10000, CRC(1e912131) SHA1(8f53504ac08942ee340489d84eab825e654d0a2c) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12165.b9",  0x00000, 0x10000, CRC(62640221) SHA1(c311d3847a981d0e1609f9b3d80481565d32d78c) )
	ROM_LOAD( "epr12166.b10", 0x10000, 0x10000, CRC(9abd183b) SHA1(621b017cb34973f9227be383e26b5cd41aea9422) )
	ROM_LOAD( "epr12167.b11", 0x20000, 0x10000, CRC(2495fd4e) SHA1(2db94ead9223a67238a97e724668076fc43e5534) )

	ROM_REGION( 0x020000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x00001, 0x10000, CRC(2fb38880) SHA1(0e1b601bbda78d1887951c1f7e752531c281bc83) )
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x00000, 0x10000, CRC(d6a02cba) SHA1(d80000f92e754e89c6ca7b7273feab448fc9a061) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12168.a7", 0x0000, 0x8000, CRC(bd9ba01b) SHA1(fafa7dc36cc057a50ae4cdf7a35f3594292336f4) )
ROM_END

// sys16B
ROM_START( tturfbl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "tt042197.rom", 0x00000, 0x10000, CRC(deee5af1) SHA1(0caba775021dc7e28ac6b7af8eac4f49d3102c83) )
	ROM_LOAD16_BYTE( "tt06c794.rom", 0x00001, 0x10000, CRC(90e6a95a) SHA1(014a0ae5cebcba9cc99e6ccde4ad5d938fab915c) )
	ROM_LOAD16_BYTE( "tt030be3.rom", 0x20000, 0x10000, CRC(100264a2) SHA1(d1ea4bf93f5472901ce95200f546ce9b58936aea) )
	ROM_LOAD16_BYTE( "tt05ef8a.rom", 0x20001, 0x10000, CRC(f787a948) SHA1(512b8cb2f5e9795171951e02c07cae957db41334) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "tt1574b3.rom", 0x00000, 0x10000, CRC(e9e630da) SHA1(e8471dedbb25475e4814d78b56f579fe9110461e) )
	ROM_LOAD( "tt16cf44.rom", 0x10000, 0x10000, CRC(4c467735) SHA1(8338b6605cbe2e076da0b3e3a47630409a79f002) )
	ROM_LOAD( "tt17d59e.rom", 0x20000, 0x10000, CRC(60c0f2fe) SHA1(3fea4ed757d47628f59ff940e40cb86b3b5b443b) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12279.1b", 0x00001, 0x10000, CRC(7a169fb1) SHA1(1ec6da0d2cfcf727e61f61c847fd8b975b64f944) )
	ROM_LOAD16_BYTE( "12283.5b", 0x00000, 0x10000, CRC(ae0fa085) SHA1(ae9af92d4dd0c8a0f064d24e647522b588fbd7f7) )
	ROM_LOAD16_BYTE( "12278.2b", 0x20001, 0x10000, CRC(961d06b7) SHA1(b1a9dea63785bfa2c0e7b931387b91dfcd27d79b) )
	ROM_LOAD16_BYTE( "12282.6b", 0x20000, 0x10000, CRC(e8671ee1) SHA1(a3732938c370f1936d867aae9c3d1e9bbfb57ede) )
	ROM_LOAD16_BYTE( "12277.3b", 0x40001, 0x10000, CRC(f16b6ba2) SHA1(00cc04c7b5aad82d51d2d252e1e57bcdc5e2c9e3) )
	ROM_LOAD16_BYTE( "12281.7b", 0x40000, 0x10000, CRC(1ef1077f) SHA1(8ce6fd7d32a20b93b3f91aaa43fe22720da7236f) )
	ROM_LOAD16_BYTE( "12276.4b", 0x60001, 0x10000, CRC(838bd71f) SHA1(82d9d127438f5e1906b1cf40bf3b4727f2ee5685) )
	ROM_LOAD16_BYTE( "12280.8b", 0x60000, 0x10000, CRC(639a57cb) SHA1(84fd8b96758d38f9e1ba1a3c2cf8099ec0452784) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) //* sound CPU */
	ROM_LOAD( "tt014d68.rom", 0x10000, 0x10000, CRC(d4aab1d9) SHA1(94885896d59da1ecabe2377a194fcf61eaae3765) )
	ROM_LOAD( "tt0246ff.rom", 0x20000, 0x10000, CRC(bb4bba8f) SHA1(b182a7e1d0425e93c2c1b93472aafd30a6af6907) )
ROM_END


// sys16B
ROM_START( wb3b )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12259.a7", 0x000000, 0x20000, CRC(54927c7e) SHA1(09a4c25b40aba2056c79b5c2e6e8cb7e6c05bc16) )
	ROM_LOAD16_BYTE( "epr12258.a5", 0x000001, 0x20000, CRC(01f5898c) SHA1(2422b4199ce5b63482f7fa1c790c90fc70a2b872) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12124.a14", 0x00000, 0x10000, CRC(dacefb6f) SHA1(789a5a99ad9419aee9da5397bcea34452ea8b4b3) )
	ROM_LOAD( "epr12125.a15", 0x10000, 0x10000, CRC(9fc36df7) SHA1(b39ccc687489e9781181197505fc78aa5cf7ea55) )
	ROM_LOAD( "epr12126.a16", 0x20000, 0x10000, CRC(a693fd94) SHA1(38e5446f41b6793a8e4134fdd92b02b86e3589f7) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12093.b4", 0x00001, 0x010000, CRC(4891e7bb) SHA1(1be04fcabe9bfa8cf746263a5bcca67902a021a0) )
	ROM_LOAD16_BYTE( "epr12097.b8", 0x00000, 0x010000, CRC(e645902c) SHA1(497cfcf6c25cc2e042e16dbcb1963d2223def15a) )
	ROM_LOAD16_BYTE( "epr12091.b2", 0x20001, 0x010000, CRC(8409a243) SHA1(bcbb9510a6499d8147543d6befa5a49f4ac055d9) )
	ROM_LOAD16_BYTE( "epr12095.b6", 0x20000, 0x010000, CRC(e774ec2c) SHA1(a4aa15ec7be5539a740ad02ff720458018dbc536) )
	ROM_LOAD16_BYTE( "epr12090.b1", 0x40001, 0x010000, CRC(aeeecfca) SHA1(496124b170a725ad863c741d4e021ab947511e4c) )
	ROM_LOAD16_BYTE( "epr12094.b5", 0x40000, 0x010000, CRC(615e4927) SHA1(d23f164973afa770714e284a77ddf10f18cc596b) )
	ROM_LOAD16_BYTE( "epr12092.b3", 0x60001, 0x010000, CRC(5c2f0d90) SHA1(e0fbc0f841e4607ad232931368b16e81440a75c4) )
	ROM_LOAD16_BYTE( "epr12096.b7", 0x60000, 0x010000, CRC(0cd59d6e) SHA1(caf754a461feffafcfe7bfc6e89da76c4db257c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12127.a10", 0x0000, 0x8000, CRC(0bb901bb) SHA1(c81b198df8e3b0ec568032c76addf0d1a1711194) )
ROM_END


// sys16B
ROM_START( wb3bbl )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "wb3_03", 0x000000, 0x10000, CRC(0019ab3b) SHA1(89d49a437690fa6e0c35bb9f1450042f89504714) )
	ROM_LOAD16_BYTE( "wb3_05", 0x000001, 0x10000, CRC(196e17ee) SHA1(71e4345b2c3d1612a3d424c9310fad1e23c8a9f7) )
	ROM_LOAD16_BYTE( "wb3_02", 0x020000, 0x10000, CRC(c87350cb) SHA1(55a8cb68d70b6060dd9a55e281e216ce3917ea5b) )
	ROM_LOAD16_BYTE( "wb3_04", 0x020001, 0x10000, CRC(565d5035) SHA1(e28a132f1a4ce9466945e231c54502178748af98) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "wb3_14", 0x00000, 0x10000, CRC(d3f20bca) SHA1(0a87f709f8e2a913473512ede408e2cbc535443f) )
	ROM_LOAD( "wb3_15", 0x10000, 0x10000, CRC(96ff9d52) SHA1(791a9da4860e0d42fba98f80a3c6725ad8c73e33) )
	ROM_LOAD( "wb3_16", 0x20000, 0x10000, CRC(afaf0d31) SHA1(d4309329a0a543250788146b63b27ff058c02fc3) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12093.b4", 0x000001, 0x010000, CRC(4891e7bb) SHA1(1be04fcabe9bfa8cf746263a5bcca67902a021a0) )
	ROM_LOAD16_BYTE( "epr12097.b8", 0x000000, 0x010000, CRC(e645902c) SHA1(497cfcf6c25cc2e042e16dbcb1963d2223def15a) )
	ROM_LOAD16_BYTE( "epr12091.b2", 0x020001, 0x010000, CRC(8409a243) SHA1(bcbb9510a6499d8147543d6befa5a49f4ac055d9) )
	ROM_LOAD16_BYTE( "epr12095.b6", 0x020000, 0x010000, CRC(e774ec2c) SHA1(a4aa15ec7be5539a740ad02ff720458018dbc536) )
	ROM_LOAD16_BYTE( "epr12090.b1", 0x040001, 0x010000, CRC(aeeecfca) SHA1(496124b170a725ad863c741d4e021ab947511e4c) )
	ROM_LOAD16_BYTE( "epr12094.b5", 0x040000, 0x010000, CRC(615e4927) SHA1(d23f164973afa770714e284a77ddf10f18cc596b) )
	ROM_LOAD16_BYTE( "epr12092.b3", 0x060001, 0x010000, CRC(5c2f0d90) SHA1(e0fbc0f841e4607ad232931368b16e81440a75c4) )
	ROM_LOAD16_BYTE( "epr12096.b7", 0x060000, 0x010000, CRC(0cd59d6e) SHA1(caf754a461feffafcfe7bfc6e89da76c4db257c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12127.a10", 0x0000, 0x8000, CRC(0bb901bb) SHA1(c81b198df8e3b0ec568032c76addf0d1a1711194) )
ROM_END

ROM_START( afighter )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	// Custom CPU 317-0018
	ROM_LOAD16_BYTE( "10348",0x00000,0x08000, CRC(e51e3012) SHA1(bb5522aacb55b5f04aa4cb7a642e202f0ddd7c84) )
	ROM_LOAD16_BYTE( "10349",0x00001,0x08000, CRC(4b434c37) SHA1(5f3afbdb9cdb0762e56b702a195274f30193b472) )
	ROM_LOAD16_BYTE( "10350",0x20000,0x08000, CRC(f2cd6b3f) SHA1(380f75b8c1696b388179641866cd1d23f78664e7) )
	ROM_LOAD16_BYTE( "10351",0x20001,0x08000, CRC(ede21d8d) SHA1(b3e3944d706c606fd01e00d9511f020ce9aec9f0) )
	ROM_LOAD16_BYTE( "10352",0x40000,0x08000, CRC(f8abb143) SHA1(97e78291c15bdf95fd35adca6b9e002480137b12) )
	ROM_LOAD16_BYTE( "10353",0x40001,0x08000, CRC(5a757dc9) SHA1(b0540844c8a09195f5d12312f8e27c334641d7b8) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10281",0x00000,0x10000, CRC(30e92cda) SHA1(36293a2a8a22dca5350571f19f3d5d04e1b27458) )
	ROM_LOAD( "10282",0x10000,0x10000, CRC(b67b8910) SHA1(f3f029a3e6547114cec28e5cf8fda65ef434c353) )
	ROM_LOAD( "10283",0x20000,0x10000, CRC(e7dbfd2d) SHA1(91bae3fbc4a3c612dc507eecfa8de1c2e1e7afee) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10285",0x00001,0x08000, CRC(98aa3d04) SHA1(1d26d17a72e55281e3444fee9c5af69ffb9e3c69) )
	ROM_LOAD16_BYTE( "10286",0x00000,0x08000, CRC(8da050cf) SHA1(c28e8968dbd9c110672581f4486f70d5f45df7f5) )
	ROM_LOAD16_BYTE( "10287",0x10001,0x08000, CRC(7989b74a) SHA1(a87acafe82b37a11d8f8b1f2ee4c9b2e1bb8161c) )
	ROM_LOAD16_BYTE( "10288",0x10000,0x08000, CRC(d3ce551a) SHA1(0ff2170d9ef89058273025dd8d5e1021094adef1) )
	ROM_LOAD16_BYTE( "10289",0x20001,0x08000, CRC(c59d1b98) SHA1(e232f2519234981c0e4ffecdd25c48083d9f93a8) )
	ROM_LOAD16_BYTE( "10290",0x20000,0x08000, CRC(39354223) SHA1(d8a73d3f7fc2d83d23bb7434f43bc8804f35cc16) )
	ROM_LOAD16_BYTE( "10291",0x30001,0x08000, CRC(6e4b245c) SHA1(1f8cecf7ea2d2dfa5ce18d7ee34b0da2cc40221e) )
	ROM_LOAD16_BYTE( "10292",0x30000,0x08000, CRC(cef289a3) SHA1(7ab817b6348c168f79be325fb3cc2cca14ee0f8e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10284",0x00000,0x8000, CRC(8ff09116) SHA1(8b99b6d2499897cfbd037a7e7cf5bc53bce8a63a) )
ROM_END


/* pre-System16 */
/*          rom       parent    machine   inp       init */
/* Alien Syndrome */
GAME( 1985, mjleague, 0,        mjleague, mjleague, mjleague, ROT270, "Sega",    "Major League" )
GAME( 1986, bodyslam, 0,        bodyslam, bodyslam, bodyslam, ROT0,   "Sega",    "Body Slam" )
GAME( 1986, dumpmtmt, bodyslam, bodyslam, bodyslam, bodyslam, ROT0,   "Sega",    "Dump Matsumoto (Japan)" )
GAME( 1986, quartet,  0,        quartet,  quartet,  quartet,  ROT0,   "Sega",    "Quartet" )
GAME( 1986, quartetj, quartet,  quartet,  quartet,  quartet,  ROT0,   "Sega",    "Quartet (Japan)" )
GAME( 1986, quartet2, quartet,  quartet2, quartet2, quartet2, ROT0,   "Sega",    "Quartet 2" )
GAME( 1986, quartt2j, quartet,  quartet2, quartet2, quartet2, ROT0,   "Sega",    "Quartet 2 (Japan)" )
GAMEX(1987, aliensya, aliensyn, aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (set 2)", GAME_NOT_WORKING )

/* System16A */
/*          rom       parent    machine   inp       init */
GAMEX(19??, afighter, 0,        s16dummy, s16dummy, s16dummy, ROT0,   "Sega",    "Action Fighter", GAME_NOT_WORKING )
GAMEX(1986, alexkidd, 0,        alexkidd, alexkidd, alexkidd, ROT0,   "Sega",    "Alex Kidd: The Lost Stars (set 1)", GAME_NOT_WORKING )
GAME( 1986, alexkida, alexkidd, alexkidd, alexkidd, alexkidd, ROT0,   "Sega",    "Alex Kidd: The Lost Stars (set 2)" )
GAME( 1986, fantzone, 0,        fantzone, fantzone, fantzone, ROT0,   "Sega",    "Fantasy Zone (Japan New Ver.)" )
GAME( 1986, fantzono, fantzone, fantzono, fantzone, fantzone, ROT0,   "Sega",    "Fantasy Zone (Old Ver.)" )
GAME( 1987, shinobi,  0,        shinobi,  shinobi,  shinobi,  ROT0,   "Sega",    "Shinobi (set 1)" )
GAMEX(1987, shinobia, shinobi,  shinobl,  shinobi,  shinobi,  ROT0,   "Sega",    "Shinobi (set 2)", GAME_NOT_WORKING )
GAME( 1987, shinobl,  shinobi,  shinobl,  shinobi,  shinobi,  ROT0,   "bootleg", "Shinobi (bootleg)" )
GAMEX(1987, sdioj,    sdi,      sdi,      sdi,      sdi,      ROT0,   "Sega",    "SDI - Strategic Defense Initiative (Japan)", GAME_NOT_WORKING )
GAME( 1988, tetris,   0,        tetris_s16a, tetris,   tetris,   ROT0,   "Sega",    "Tetris (Japan, System 16A, 317-0093)" )

/* System16B */
/*          rom       parent    machine   inp       init */
GAMEX(1987, aliensyj, aliensyn, aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (Japan)", GAME_NOT_WORKING )

GAMEX(1989, bayrtbl1, bayroute, bayroute, bayroute, bayrtbl1, ROT0,   "bootleg", "Bay Route (bootleg set 1)", GAME_NOT_WORKING )
GAMEX(1989, bayrtbl2, bayroute, bayroute, bayroute, bayrtbl1, ROT0,   "bootleg", "Bay Route (bootleg set 2)", GAME_NOT_WORKING )
GAME( 1989, dduxbl,   ddux,     dduxbl,   ddux,     dduxbl,   ROT0,   "bootleg", "Dynamite Dux (bootleg)" )
GAME( 1989, eswatbl,  eswat,    eswatbl,  eswat,    eswatbl,  ROT0,   "bootleg", "E-Swat - Cyber Police (bootleg)" )
GAME( 1989, fpointbl, fpoint,   fpointbl, fpoint,   fpointbl, ROT0,   "bootleg", "Flash Point (World, bootleg)" )
GAME( 1989, fpointbj, fpoint,   fpointbl, fpointbj, fpointbl, ROT0,   "bootleg", "Flash Point (Japan, bootleg)" )

GAME( 1989, goldnaxe, 0,        goldnaxe, goldnaxe, goldnaxe, ROT0,   "Sega",    "Golden Axe (Version 1)" )
GAMEX(1989, goldnabl, goldnaxe, goldnaxe, goldnaxe, goldnabl, ROT0,   "bootleg", "Golden Axe (bootleg)", GAME_NOT_WORKING )
GAME( 1989, goldnaxa, goldnaxe, goldnaxa, goldnaxe, goldnaxe, ROT0,   "Sega",    "Golden Axe (Version 2)" )

GAME( 1988, passshtb, passsht,  passsht,  passsht,  passsht,  ROT270, "bootleg", "Passing Shot (2 Players) (bootleg)" )
GAMEX(1988, passht4b, passsht,  passht4b, passht4b, passht4b, ROT270, "bootleg", "Passing Shot (4 Players) (bootleg)", GAME_NO_SOUND )
GAME( 1988, tetrisbl, tetris,   tetrisbl, tetris,   tetrisbl, ROT0,   "bootleg", "Tetris (bootleg)" )
GAMEX(1989, tturfbl,  tturf,    tturfbl,  tturf,    tturfbl,  ROT0,   "bootleg", "Tough Turf (bootleg)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
GAME( 1988, wb3b,     0,        wb3b,     wb3b,     wb3b,   ROT0,   "Sega / Westone", "Wonder Boy III - Monster Lair (System 16B, World, not encrypted)" ) //*
GAME( 1988, wb3bbl,   wb3b,     wb3bbl,   wb3b,     wb3bbl, ROT0,   "bootleg", "Wonder Boy III - Monster Lair (bootleg)" )
