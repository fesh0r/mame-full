/***************************************************************************

  gb.c

  Driver file to handle emulation of the Nintendo Gameboy.
  By:

  Hans de Goede               1998
  Anthony Kruize              2002
  Wilbert Pol                 2004 (Megaduck/Cougar Boy)

  Todo list:
  Done entries kept for historical reasons, besides that it's nice to see
  what is already done instead of what has to be done.

Priority:  Todo:                                                  Done:
  2        Replace Marat's  vidhrdw/gb.c  by Playboy code           *
  2        Clean & speed up vidhrdw/gb.c                            *
  2        Replace Marat's  Z80gb/Z80gb.c by Playboy code           *
  2        Transform Playboys Z80gb.c to big case method            *
  2        Clean up Z80gb.c                                         *
  2        Fix / optimise halt instruction                          *
  2        Do correct lcd stat timing                               In Progress
  2        Generate lcd stat interrupts                             *
  2        Replace Marat's code in machine/gb.c by Playboy code     ?
  1        Check, and fix if needed flags bug which troubles ffa    ?
  1        Save/restore battery backed ram                          *
  1        Add sound                                                *
  0        Add supergb support                                      *
  0        Add palette editting, save & restore
  0        Add somekind of backdrop support
  0        Speedups if remotly possible

  2 = has to be done before first public release
  1 = should be added later on
  0 = bells and whistles


Timers
======

There seems to be some kind of selectable internal clock divider which is used to drive
the timer increments. This causes the first timer cycle to now always be a full cycle.
For instance in 1024 clock cycle mode, the first timer cycle could easily only take 400
clock cycles. The next timer cycle will take the full 1024 clock cycles though.

Writes to the DIV register seem to cause this internal clock divider/register to be
reset in such a way that the next stimulus cause a timer increment (in any mode).


Interrupts
==========

Taking an interrupt seems to take around 12(?) clock cycles.


Stat timing
===========

This timing table is accurate within 4 cycles:
           | stat = 2 | stat = 3 | stat = 0 |
No sprites |    80    |    172   |    204   |
1 sprite   |    80    |    182   |    194   |
2 sprites  |    80    |    192   |    184   |
3 sprites  |    80    |    202   |    174   |
4 sprites  |    80    |    212   |    164   |
5 sprites  |    80    |    222   |    154   |
6 sprites  |    80    |    232   |    144   |
7 sprites  |    80    |    242   |    134   |
8 sprites  |    80    |    252   |    124   |
9 sprites  |    80    |    262   |    114   |
10 sprites |    80    |    272   |    104   |

In other words, each sprite on a line makes stat 3 last 10 cycles longer.


For lines 1 - 143 when stat changes to 2 the line counter is incremented.

Line 153 is little odd timing wise. The line counter stays 153 for ~16-32 clock cycles
and is then rolls over to 0.

When the line counter is changed it gets checked against the lyc register.


Mappers used in the gameboy
===========================

MBC1 Mapper
===========

The MBC1 mapper has two modes: 2MB ROM/8KB RAM or 512KB ROM/32KB RAM.
This mode is selected by writing into the 6000-7FFF memory area:
0bXXXXXXXB - B=0 - 2MB ROM/8KB RAM mode
             B=1 - 512KB ROM/32KB RAM mode
The default behaviour is to be in 2MB ROM/8KB RAM mode.

Writing a value ( 0bXXXBBBBB ) into the 2000-3FFF memory area selects the
lower 5 bits of the ROM bank to select for the 4000-7FFF memory area. If a
value of 0bXXX00000 is written then this will autmatically be changed to
0bXXX00001 by the mbc chip.

Writing a value (0bXXXXXXBB ) into the 4000-5FFF memory area either selects
the RAM bank to use or bits 6 and 7 for the ROM bank to use for the 4000-7FFF
memory area. This behaviour depends on the memory moddel chosen.

The RAM sections are enabled by writing the value 0bXXX1010 into the 0000-1FFF
memory area. Writing any other value disables the RAM section.

Some unanswered cases:
#1 - Set mode 0
   - Set lower bank bits to 1F
   - Set high bank bits to 01  => bank #3F
   - Set mode 1
   - What ROM bank is now at 4000-7FFF, bank #1F or bank #3F?

#2 - Set mode 1
   - Set ram area #1
   - Set mode 0
   - What ram area is now at A000-BFFF, ram bank 00 or ram bank 01?


MBC2 Mapper
===========

The MBC2 mapper includes 512x4bits of builtin RAM.

0000-1FFF - Writing to this area enables (value 0bXXXX1010) or disables (any
            other value than 0bXXXX1010) the RAM. In order to perform this
            function bit 12 of the address must be reset, so usable areas are
            0000-00FF, 0200-02FF, 0400-04FF, 0600-06FF, ..., 1E00-1EFF.
2000-3FFF - Writing to this area selects the rom bank to appear at 4000-7FFF.
            Only bits 3-0 are used to select the bank number. If a value of
            0bXXXX0000 is written then this is automatically changed into
            0bXXXX0001 by the mapper.
            In order to perform the rom banking bit 12 of the address must be
            set, so usable areas are 2100-21FF, 2300-23FF, 2500-25FF, 2700-
            27FF, ..., 3F00-3FFF.

Some unanswered cases:
#1 - Set rom bank to 8 for a 4 bank rom image.
   - What rom bank appears at 4000-7FFF, bank #0 or bank #1 ?


MBC3 Mapper
===========

The MBC3 mapper cartridges can include a RTC chip.

0000-1FFF - Writing to this area enables (value 0x0A) or disables (0x00) the
            RAM and RTC registers.
2000-3FFF - Writing to this area selects the rom bank to appear at 4000-7FFF.
            Bits 6-0 are used  to select the bank number. If a value of
            0bX0000000 is written then this is autmatically changed into
            0bX0000001 by the mapper.
4000-5FFF - Writing to this area selects the RAM bank or the RTC register to
            read.
            XXXX00bb - Select RAM bank bb.
            XXXX1rrr - Select RTC register rrr. Accepted values for rrr are:
                       000 - Seconds (0x00-0x3B)
                       001 - Minutes (0x00-0x3B)
                       010 - Hours (0x00-0x17)
                       011 - Bits 7-0 of the day counter
                       100 - bit 0 - Bit 8 of the day counter
                             bit 6 - Halt RTC timer ( 0 = timer active, 1 = halted)
                             bit 7 - Day counter overflow flag
6000-7FFF - Writing 0x00 followed by 0x01 latches the RTC data. This latching
            method is used for reading the RTC registers.

Some unanswered cases:
#1 - Set rom bank to 8(/16/32/64) for a 4(/8/16/32) bank image.
   - What rom bank appears at 4000-7FFF, bank #0 or bank #1 ?


MBC4 Mapper
===========

Stauts: not supported yet.


MBC5 Mapper
===========

0000-1FFF - Writing to this area enables (0x0A) or disables (0x00) the RAM area.
2000-2FFF - Writing to this area updates bits 7-0 of the rom bank number to
            appear at 4000-7FFF.
3000-3FFF - Writing to this area updates bit 8 of the rom bank number to appear
            at 4000-7FFF.
4000-5FFF - Writing to this area select the RAM bank number to use. If the
            cartridge includes a Rumble Pack then bit 3 is used to control
            rumble motor (0 - disable motor, 1 - enable motor).


MBC7 Mapper (Used by Kirby's Tilt n' Tumble, Command Master)
===========

Status: Partial support (only ROM banking supported at the moment)

The MBC7 mapper has 0x0200(?) bytes of RAM built in.

0000-1FFF - Probably enable/disable RAM
            In order to use this area bit 12 of the address be set.
            Values written: 00, 0A
2000-2FFF - Writing to this area selects the ROM bank to appear at
            4000-7FFF.
            In order to use this area bit 12 of the address be set.
            Values written: 01, 07, 01, 1C
3000-3FFF - Unknown
            In order to use this area bit 12 of the address be set.
            Values written: 00
4000-4FFF - Unknown
            In order to use this area bit 12 of the address be set.
            Values written: 00, 40, 3F


TAMA5 Mapper (Used by Tamagotchi 3)
============

Status: partially supported.

The TAMA5 mapper includes a special RTC chip which communicates through the
RAM area (0xA000-0xBFFF); most notably addresses 0xA000 and 0xA001 seem to
be used. In this setup 0xA001 acts like a control register and 0xA000 like
a data register.

Accepted values by the TAMA5 control register:
0x00 - Writing to 0xA000 will set bits 3-0 for rom bank selection.
0x01 - Writing to 0xA000 will set bits (7-?)4 for rom bank selection.

0x04 - Bits 3-0 of the value to write
0x05 - Bits 4-7 of the value to write
0x06 - Address control hi
       bit 0 - Bit 4 for the address
       bit 3-1 - 000 - Write a byte to the 32 byte memory. The data to be
                       written must be set in registers 0x04 (lo nibble) and
                       0x05 (hi nibble).
               - 001 - Read a byte from the 32 byte memory. The data read
                       will be available in registers 0x0C (lo nibble) and
                       0x0D (hi nibble).
               - 010 - Unknown (occurs just after having started a game and
                       entered a date) (execution at address 1A19)
               - 011 - Unknown (not encountered yet)
               - 100 - Unknown (occurs during booting a game; appears to be
                       some kind of read command as it is followed by a read
                       of the 0x0C register) (execution at address 1B5B)
               - 101 - Unknown (not encountered yet)
               - 110 - Unknown (not encountered yet)
               - 111 - Unknown (not encountered yet)
0x07 - Address control lo
       bit 3-0 - bits 3-0 for the address

0x0A - After writing this the lowest 2 bits of A000 determine whether the
       TAMA5 chip is ready to accept the next command. If the lowest 2 bits
       hold the value 01 then the TAMA5 chip is ready for the next command.

0x0C - Reading from A000 will return bits 3-0 of the data
0x0D - Reading from A000 will return bits 7-4 of the data

0x04 - RTC controls? -> RTC/memory?
0x05 - Write time/memomry?
0x06 - RTC controls?
0x07 - RTC controls?

Unknown sequences:
During booting a game (1B5B:
04 <- 00, 06 <- 08, 07 <- 01, followed by read 0C
when value read from 0C equals 00 followed by the sequence:
04 <- 01, 06 <- 08, 07 <- 01, followed by read 0C
the value read from 0C is checked for non-zero, don't know the consequences for either
yet.

Initialization after starting a game:
At address 1A19:
06 <- 05, 07 <- 02, followed by read 0C, if != 0F => OK, otherwise do something.


HuC1 mapper
===========

Status: not supported yet.


HuC3 mapper
===========

Status: not supported yet.


***************************************************************************/
#include "driver.h"
#include "video/generic.h"
#include "cpu/z80gb/z80gb.h"
#include "includes/gb.h"
#include "devices/cartslot.h"
#include "rendlay.h"
#include "gb.lh"

/* Initial value of the cpu registers (hacks until we get bios dumps) */
static UINT16 sgb_cpu_regs[6] = { 0x01B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };    /* Super GameBoy                    */
static UINT16 mgb_cpu_regs[6] = { 0xFFB0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };	/* GameBoy Pocket / Super GameBoy 2 */
static UINT16 cgb_cpu_regs[6] = { 0x11B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };	/* GameBoy Color  / Gameboy Advance */
static UINT16 megaduck_cpu_regs[6] = { 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFE, 0x0000 };	/* Megaduck */

Z80GB_CONFIG dmg_cpu_reset = { NULL, gb_timer_callback };
Z80GB_CONFIG sgb_cpu_reset = { sgb_cpu_regs, gb_timer_callback };
Z80GB_CONFIG mgb_cpu_reset = { mgb_cpu_regs, gb_timer_callback };
Z80GB_CONFIG cgb_cpu_reset = { cgb_cpu_regs, gb_timer_callback };
Z80GB_CONFIG megaduck_cpu_reset = { megaduck_cpu_regs, gb_timer_callback };

static ADDRESS_MAP_START(gb_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x00ff) AM_ROMBANK(5)					/* BIOS or ROM */
	AM_RANGE(0x0100, 0x3fff) AM_ROMBANK(10)					/* ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)					/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_RAM AM_WRITE( gb_vram_w ) AM_BASE(&gb_vram)	/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK(2)					/* 8k switched RAM bank (cartridge) */
	AM_RANGE(0xc000, 0xfdff) AM_RAM						/* 8k low RAM, echo RAM */
	AM_RANGE(0xfe00, 0xfeff) AM_RAM AM_WRITE( gb_oam_w ) AM_BASE(&gb_oam)	/* OAM RAM */
	AM_RANGE(0xff00, 0xff0f) AM_READWRITE( gb_io_r, gb_io_w )		/* I/O */
	AM_RANGE(0xff10, 0xff26) AM_READWRITE( gb_sound_r, gb_sound_w )		/* sound registers */
	AM_RANGE(0xff27, 0xff2f) AM_NOP						/* unused */
	AM_RANGE(0xff30, 0xff3f) AM_READWRITE( gb_wave_r, gb_wave_w )		/* Wave ram */
	AM_RANGE(0xff40, 0xff7f) AM_READWRITE( gb_video_r, gb_io2_w)		/* Video controller & BIOS flip-flop */
	AM_RANGE(0xff80, 0xfffe) AM_RAM						/* High RAM */
	AM_RANGE(0xffff, 0xffff) AM_READWRITE( gb_ie_r, gb_ie_w )		/* Interrupt enable register */
ADDRESS_MAP_END

static ADDRESS_MAP_START(sgb_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_ROMBANK(5)					/* 16k fixed ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)					/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_RAM AM_WRITE( gb_vram_w ) AM_BASE(&gb_vram)	/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK(2)					/* 8k switched RAM bank (cartridge) */
	AM_RANGE(0xc000, 0xfdff) AM_RAM						/* 8k low RAM, echo RAM */
	AM_RANGE(0xfe00, 0xfeff) AM_RAM AM_WRITE( gb_oam_w ) AM_BASE(&gb_oam)	/* OAM RAM */
	AM_RANGE(0xff00, 0xff0f) AM_READWRITE( gb_io_r, sgb_io_w )		/* I/O */
	AM_RANGE(0xff10, 0xff26) AM_READWRITE( gb_sound_r, gb_sound_w )		/* sound registers */
	AM_RANGE(0xff27, 0xff2f) AM_NOP						/* unused */
	AM_RANGE(0xff30, 0xff3f) AM_READWRITE( gb_wave_r, gb_wave_w )		/* Wave RAM */
	AM_RANGE(0xff40, 0xff7f) AM_READWRITE( gb_video_r, gb_io2_w )		/* Video controller & BIOS flip-flop */
	AM_RANGE(0xff80, 0xfffe) AM_RAM						/* High RAM */
	AM_RANGE(0xffff, 0xffff) AM_READWRITE( gb_ie_r, gb_ie_w )		/* Interrupt enable register */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gbc_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_ROMBANK(5)					/* 16k fixed ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)					/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK(4) AM_WRITE( gbc_vram_w )		/* 8k switched VRAM bank */
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK(2)					/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xcfff) AM_RAM						/* 4k fixed RAM bank */
	AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(3)					/* 4k switched RAM bank */
	AM_RANGE(0xe000, 0xfdff) AM_RAM						/* echo RAM */
	AM_RANGE(0xfe00, 0xfeff) AM_RAM AM_WRITE( gb_oam_w ) AM_BASE(&gb_oam)	/* OAM RAM */
	AM_RANGE(0xff00, 0xff0f) AM_READWRITE( gb_io_r, gb_io_w )		/* I/O */
	AM_RANGE(0xff10, 0xff26) AM_READWRITE( gb_sound_r, gb_sound_w )		/* sound controller */
	AM_RANGE(0xff27, 0xff2f) AM_NOP						/* unused */
	AM_RANGE(0xff30, 0xff3f) AM_READWRITE( gb_wave_r, gb_wave_w )		/* Wave RAM */
	AM_RANGE(0xff40, 0xff7f) AM_READWRITE( gbc_io2_r, gbc_io2_w )		/* Other I/O and video controller */
	AM_RANGE(0xff80, 0xfffe) AM_RAM						/* high RAM */
	AM_RANGE(0xffff, 0xffff) AM_READWRITE( gb_ie_r, gb_ie_w )		/* Interrupt enable register */
ADDRESS_MAP_END

static ADDRESS_MAP_START(megaduck_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_ROMBANK(10)						/* 16k switched ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)						/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_RAM AM_WRITE( gb_vram_w ) AM_BASE(&gb_vram)		/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_NOP							/* unused? */
	AM_RANGE(0xc000, 0xfe9f) AM_RAM							/* 8k low RAM, echo RAM */
	AM_RANGE(0xfe00, 0xfeff) AM_RAM AM_WRITE( gb_oam_w ) AM_BASE(&gb_oam)		/* OAM RAM */
	AM_RANGE(0xff00, 0xff0f) AM_READWRITE( gb_io_r, gb_io_w )			/* I/O */
	AM_RANGE(0xff10, 0xff1f) AM_READWRITE( megaduck_video_r, megaduck_video_w )	/* video controller */
	AM_RANGE(0xff20, 0xff2f) AM_READWRITE( megaduck_sound_r1, megaduck_sound_w1)	/* sound controller pt1 */
	AM_RANGE(0xff30, 0xff3f) AM_READWRITE( gb_wave_r, gb_wave_w )			/* wave ram */
	AM_RANGE(0xff40, 0xff46) AM_READWRITE( megaduck_sound_r2, megaduck_sound_w2)	/* sound controller pt2 */
	AM_RANGE(0xff47, 0xff7f) AM_NOP							/* unused */
	AM_RANGE(0xff80, 0xfffe) AM_RAM							/* high RAM */
	AM_RANGE(0xffff, 0xffff) AM_READWRITE( gb_ie_r, gb_ie_w )			/* interrupt enable register */
ADDRESS_MAP_END

static gfx_decode gb_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

INPUT_PORTS_START( gameboy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left") 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right") 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up") 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down") 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Button A") 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("Button B") 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Start") 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("Select") 
INPUT_PORTS_END

static unsigned char palette[] =
{
/* Simple black and white palette */
/*	0xFF,0xFF,0xFF,
	0xB0,0xB0,0xB0,
	0x60,0x60,0x60,
	0x00,0x00,0x00 */

/* Possibly needs a little more green in it */
	0xFF,0xFB,0x87,		/* Background */
	0xB1,0xAE,0x4E,		/* Light */
	0x84,0x80,0x4E,		/* Medium */
	0x4E,0x4E,0x4E,		/* Dark */

/* Palette for GameBoy Pocket/Light */
	0xC4,0xCF,0xA1,		/* Background */
	0x8B,0x95,0x6D,		/* Light      */
	0x6B,0x73,0x53,		/* Medium     */
	0x41,0x41,0x41,		/* Dark       */
};

static unsigned char palette_megaduck[] = {
	0x6B, 0xA6, 0x4A, 0x43, 0x7A, 0x63, 0x25, 0x59, 0x55, 0x12, 0x42, 0x4C
};

/* Initialise the palette */
static PALETTE_INIT( gb )
{
	int ii;
	for( ii = 0; ii < 4; ii++)
	{
		palette_set_color(machine, ii, palette[ii*3+0], palette[ii*3+1], palette[ii*3+2]);
		colortable[ii] = ii;
	}
}

static PALETTE_INIT( gbp )
{
	int ii;
	for( ii = 0; ii < 4; ii++)
	{
		palette_set_color(machine, ii, palette[(ii + 4)*3+0], palette[(ii + 4)*3+1], palette[(ii + 4)*3+2]);
		colortable[ii] = ii;
	}
}

static PALETTE_INIT( sgb )
{
	int ii, r, g, b;

	for( ii = 0; ii < 32768; ii++ )
	{
		r = (ii & 0x1F) << 3;
		g = ((ii >> 5) & 0x1F) << 3;
		b = ((ii >> 10) & 0x1F) << 3;
		palette_set_color(machine,  ii, r, g, b );
	}

	/* Some default colours for non-SGB games */
	colortable[0] = 32767;
	colortable[1] = 21140;
	colortable[2] = 10570;
	colortable[3] = 0;
	/* The rest of the colortable can be black */
	for( ii = 4; ii < 8*16; ii++ )
		colortable[ii] = 0;
}

static PALETTE_INIT( gbc )
{
	int ii, r, g, b;

	for( ii = 0; ii < 32768; ii++ )
	{
		r = (ii & 0x1F) << 3;
		g = ((ii >> 5) & 0x1F) << 3;
		b = ((ii >> 10) & 0x1F) << 3;
		palette_set_color( machine, ii, r, g, b );
	}

	/* Background is initialised as white */
	for( ii = 0; ii < 8*4; ii++ )
		colortable[ii] = 32767;
	/* Sprites are supposed to be uninitialized, but we'll make them black */
	for( ii = 8*4; ii < 16*4; ii++ )
		colortable[ii] = 0;
}

static PALETTE_INIT( megaduck ) {
	int ii;
	for( ii = 0; ii < 4; ii++) {
		palette_set_color(machine, ii, palette_megaduck[ii*3+0], palette_megaduck[ii*3+1], palette_megaduck[ii*3+2]);
		colortable[ii] = ii;
	}
}

static struct CustomSound_interface gameboy_sound_interface =
{ gameboy_sh_start, 0, 0 };


static MACHINE_DRIVER_START( gameboy )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_PROGRAM_MAP(gb_map, 0)
	MDRV_CPU_CONFIG(dmg_cpu_reset)
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 1)	/* 1 dummy int each frame */

	MDRV_SCREEN_REFRESH_RATE(DMG_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( gb )
	MDRV_MACHINE_RESET( gb )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_DEFAULT_LAYOUT(layout_gb)
//	MDRV_SCREEN_SIZE(20*8, 18*8)
	MDRV_SCREEN_SIZE( 458, 154 )
	MDRV_SCREEN_VISIBLE_AREA(0*8, 20*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(gb)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(gameboy_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( supergb )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_REPLACE("main", Z80GB, 4295454)	/* 4.295454 Mhz */
	MDRV_CPU_PROGRAM_MAP(sgb_map, 0)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_CONFIG(sgb_cpu_reset)

	MDRV_MACHINE_RESET( sgb )

	MDRV_DEFAULT_LAYOUT(layout_horizont)	/* runs on a TV, not an LCD */
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(8*16)	/* 8 palettes of 16 colours */
	MDRV_PALETTE_INIT(sgb)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gbpocket )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_CONFIG(mgb_cpu_reset)
	MDRV_MACHINE_RESET( gbpocket )
	MDRV_PALETTE_INIT(gbp)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gbcolor )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP( gbc_map, 0 )
	MDRV_CPU_CONFIG(cgb_cpu_reset)

	MDRV_MACHINE_RESET(gbc)

	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(16*4)	/* 16 palettes of 4 colours */
	MDRV_PALETTE_INIT(gbc)
MACHINE_DRIVER_END

static void gameboy_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_gb_cart; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_gb_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "gb,gmb,cgb,gbc,sgb"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void gameboy_cartslot_getinfo_gb(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 0; break;

		default:										gameboy_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(gameboy)
	CONFIG_DEVICE(gameboy_cartslot_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(gameboy_gb)
	CONFIG_DEVICE(gameboy_cartslot_getinfo_gb)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(gb_cgb)
	CONFIG_DEVICE(gameboy_cartslot_getinfo)
	CONFIG_RAM_DEFAULT(2 * 8 * 1024 + 8 * 4 * 1024)	/* 2 pages of 8KB VRAM, 8 pages of 4KB RAM */
SYSTEM_CONFIG_END

static MACHINE_DRIVER_START( megaduck )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_PROGRAM_MAP( megaduck_map, 0 )
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 1)	/* 1 int each scanline ! */
	MDRV_CPU_CONFIG(megaduck_cpu_reset)

	MDRV_SCREEN_REFRESH_RATE(DMG_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( megaduck )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(20*8, 18*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 20*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(megaduck)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(gameboy_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static void megaduck_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_megaduck_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(megaduck)
	CONFIG_DEVICE(megaduck_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gameboy )
	ROM_REGION( 0x0100, REGION_CPU1, 0 )
	ROM_LOAD( "dmg_boot.bin", 0x0000, 0x0100, CRC(59c8598e) SHA1(4ed31ec6b0b175bb109c0eb5fd3d193da823339f) )
ROM_END

ROM_START( supergb )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
/*	ROM_LOAD( "sgb_boot.bin", 0x0000, 0x0100, NO_DUMP ) */
ROM_END

ROM_START( gbpocket )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
/*	ROM_LOAD( "gbp_boot.bin", 0x0000, 0x0100, NO_DUMP ) */
ROM_END

ROM_START( gbcolor )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
/*	ROM_LOAD( "gbc_boot.bin", 0x0000, 0x0100, NO_DUMP ) */
ROM_END


ROM_START( megaduck )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
ROM_END

/*    YEAR  NAME      PARENT   COMPAT	MACHINE   INPUT    INIT  CONFIG   COMPANY     FULLNAME */
CONS( 1990, gameboy,  0,       0,		gameboy,  gameboy, 0,    gameboy_gb, "Nintendo", "GameBoy"  , 0)
CONS( 1994, supergb,  0,       gameboy,	supergb,  gameboy, 0,    gameboy, "Nintendo", "Super GameBoy" , 0)
CONS( 1996, gbpocket, gameboy, 0,		gbpocket, gameboy, 0,    gameboy, "Nintendo", "GameBoy Pocket" , 0)
CONS( 1998, gbcolor,  0,       gameboy,	gbcolor,  gameboy, 0,    gb_cgb, "Nintendo", "GameBoy Color" , 0)

/* Sound is not 100% yet, it generates some sounds which could be ok. Since we're lacking a real
   system there's no way to verify. Same goes for the colors of the LCD. We are no using the default
   GameBoy green colors */
CONS( 1993, megaduck, 0,       0,       megaduck, gameboy, 0,    megaduck,"Creatronic/Videojet/Timlex/Cougar",  "MegaDuck/Cougar Boy" , 0)

