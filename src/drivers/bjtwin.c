/********************************************************************

Hacha Mecha Fighter     NMK        68000 <unknown cpu> YM2203 2xOKIM6295
Macross                 Banpresto  68000 <unknown cpu> YM2203 2xOKIM6295
Macross II              Banpresto  68000 Z80           YM2203 2xOKIM6295
Bombjack Twin           NMK        68000               2xOKIM6295
Nouryoku Koujou Iinkai  Tecmo      68000               2xOKIM6295

driver by Mirko Buffoni, Richard Bush, Nicola Salmoria

The later games have an higher resolution (384x224 instead of 256x224)
but the hardware is pretty much the same. It's obvious that the higher
res is an afterthought, because the tilemap layout is weird (the left
8 screen columns have to be taken from the rightmost 8 columns of the
tilemap), and the games rely on mirror addresses to access the tilemap
sequentially.

TODO:
- Both hachamf and macross use an unknown (custom?) CPU to drive sound and
  for protection. In macross it's just checked on startup and easy to work
  around. hachamf is more complex, it seems that the two CPUs share some RAM
  (fe000-fefff), and the main CPU fetches pointers from that shared RAM to
  do important operations like reading the input ports. Some of them are
  easily deduced checking for similarities in macross and bjtwin; however
  another protection check involves (see the routine at 01429a) writing data
  to the fe100-fe1ff range, and then jumping to subroutines in that range (most
  likely function pointers since each one is only 0x10 bytes long), and heaven
  knows what those should do.
  On startup, hachamf does a RAM test, then copies some stuff and jumps to
  RAM at 0xfef00, where it sits in a loop. Maybe the RAM is shared with the
  sound CPU, and used as a protection. We patch around that by replacing the
  reset vector with the "real" one.
- Cocktail mode is supported, but tilemap.c has problems with asymmetrical
  visible areas.
- Macross2 is overflowing the palette, but this might be just a problem
  with tilemap.c.
- Macross2 dip switches (the ones currently listed match macross)
- Macross2 background is wrong in level 2 at the end of the vertical scroll.
  The tilemap layout is probably different from the one I used, the dimensions
  should be correct but the page order is likely different.
- Music timing in nouryoku is a little off.

----

IRQ1 controls audio output and coin/joysticks reads
I haven't yet figured out why music stops so soon, but it's
surely due to interrupt timing (the hardware is so simple!).

IRQ4 controls DSW reads and vblank.
IRQ2 points to RTE (not used).

----

hachamf test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 1 button 2 14 (!) times

bjtwin test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 1 buttons in this sequence:
	2,2,2, 1,1,1, 2,2,2, 1,1,1
	The release date of this program will appear.

Some code has to be patched out for this to work (see below). The program
remaps button 2 and 3 to button 1, so you can't enter the above sequence.

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *nmk_bgvideoram,*nmk_txvideoram;

READ_HANDLER( nmk_bgvideoram_r );
WRITE_HANDLER( nmk_bgvideoram_w );
READ_HANDLER( nmk_txvideoram_r );
WRITE_HANDLER( nmk_txvideoram_w );
WRITE_HANDLER( nmk_paletteram_w );
WRITE_HANDLER( nmk_scroll_w );
WRITE_HANDLER( nmk_flipscreen_w );
WRITE_HANDLER( nmk_tilebank_w );

int macross_vh_start(void);
int macross2_vh_start(void);
int bjtwin_vh_start(void);
void nmk_vh_stop(void);
void macross_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bjtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void nmk_eof_callback(void);


static unsigned char *ram;

static WRITE_HANDLER( macross_mcu_w )
{
//logerror("%04x: mcu_w %02x\n",cpu_get_pc(),data);
}

static READ_HANDLER( macross_mcu_r )
{
	static int respcount = 0;
	static int resp[] = {	0x82, 0xc7, 0x00,
							0x2c, 0x6c, 0x00,
							0x9f, 0xc7, 0x00,
							0x29, 0x69, 0x00,
							0x8b, 0xc7, 0x00 };
	int res;

	if (cpu_get_pc() == 0x0332) res = ram[0x0f6];
	else
	{
		res = resp[respcount++];
		if (respcount >= 15) respcount = 0;
	}

//logerror("%04x: mcu_r %02x\n",cpu_get_pc(),res);

	return res;
}

static WRITE_HANDLER( macross2_sound_command_w )
{
	if ((data & 0x00ff0000) == 0)
		soundlatch_w(0,data & 0xff);
}

static WRITE_HANDLER( macross2_sound_bank_w )
{
	const UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(9,rom + (data & 0x07) * 0x4000);
}

static WRITE_HANDLER( macross2_oki6295_bankswitch_w )
{
	/* The OKI6295 ROM space is divided in four banks, each one indepentently
	   controlled. The sample table at the beginning of the addressing space is
	   divided in four pages as well, banked together with the sample data. */
	#define TABLESIZE 0x100
	#define BANKSIZE 0x10000
	int chip = (offset & 4) >> 2;
	int banknum = offset & 3;
	unsigned char *rom = memory_region(REGION_SOUND1 + chip);
	int size = memory_region_length(REGION_SOUND1 + chip) - 0x40000;
	int bankaddr = (data * BANKSIZE) & (size-1);

	/* copy the samples */
	memcpy(rom + banknum * BANKSIZE,rom + 0x40000 + bankaddr,BANKSIZE);

	/* and also copy the samples address table */
	rom += banknum * TABLESIZE;
	memcpy(rom,rom + 0x40000 + bankaddr,TABLESIZE);
}

static WRITE_HANDLER( bjtwin_oki6295_bankswitch_w )
{
	if ((data & 0x00ff0000) == 0)
		macross2_oki6295_bankswitch_w(offset/2,data & 0xff);
}



static READ_HANDLER( hachamf_protection_hack_r )
{
	/* adresses for the input ports */
	static int pap[] = { 0x0008, 0x0000, 0x0008, 0x0002, 0x0008, 0x0008 };

	return pap[offset/2];
}

static struct MemoryReadAddress hachamf_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x080001, input_port_0_r },
	{ 0x080002, 0x080003, input_port_1_r },
	{ 0x080008, 0x080009, input_port_2_r },
	{ 0x088000, 0x0887ff, paletteram_word_r },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA_BANK1 },
	{ 0x0f8000, 0x0f8fff, MRA_BANK2 },
	{ 0x0fe000, 0x0fe00b, hachamf_protection_hack_r },
	{ 0x0f9000, 0x0fffff, MRA_BANK3 },
	{ -1 }
};

static struct MemoryWriteAddress hachamf_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x088000, 0x0887ff, nmk_paletteram_w, &paletteram },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA_BANK1 },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA_BANK2, &spriteram, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA_BANK3 },	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
	{ -1 }
};

static struct MemoryReadAddress macross_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x080001, input_port_0_r },
	{ 0x080002, 0x080003, input_port_1_r },
	{ 0x080008, 0x080009, input_port_2_r },
	{ 0x08000a, 0x08000b, input_port_3_r },
	{ 0x08000e, 0x08000f, macross_mcu_r },
	{ 0x088000, 0x0887ff, paletteram_word_r },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA_BANK1 },
	{ 0x0f8000, 0x0f8fff, MRA_BANK2 },
	{ 0x0f9000, 0x0fffff, MRA_BANK3 },
	{ -1 }
};

static struct MemoryWriteAddress macross_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA_NOP },	/* IRQ enable? */
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x08001e, 0x08001f, macross_mcu_w },
	{ 0x088000, 0x0887ff, nmk_paletteram_w, &paletteram },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA_BANK1 },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA_BANK2, &spriteram, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA_BANK3, &ram },	/* Work RAM again */
	{ -1 }
};

static struct MemoryReadAddress macross2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x100001, input_port_0_r },
	{ 0x100002, 0x100003, input_port_1_r },
	{ 0x100008, 0x100009, input_port_2_r },
	{ 0x10000a, 0x10000b, input_port_3_r },
	{ 0x10000e, 0x10000f, soundlatch2_r },	/* from Z80 */
	{ 0x120000, 0x1207ff, paletteram_word_r },
	{ 0x140000, 0x14ffff, nmk_bgvideoram_r },
	{ 0x170000, 0x170fff, nmk_txvideoram_r },
	{ 0x171000, 0x171fff, nmk_txvideoram_r },	/* mirror */
	{ 0x1f0000, 0x1f7fff, MRA_BANK1 },
	{ 0x1f8000, 0x1f8fff, MRA_BANK2 },
	{ 0x1f9000, 0x1fffff, MRA_BANK3 },
	{ -1 }
};

static struct MemoryWriteAddress macross2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100014, 0x100015, nmk_flipscreen_w },
	{ 0x100016, 0x100017, MWA_NOP },	/* IRQ eanble? */
	{ 0x100018, 0x100019, nmk_tilebank_w },
	{ 0x10001e, 0x10001f, macross2_sound_command_w },	/* to Z80 */
	{ 0x120000, 0x1207ff, nmk_paletteram_w, &paletteram },
	{ 0x130000, 0x130007, nmk_scroll_w },
	{ 0x130008, 0x1307ff, MWA_NOP },	/* 0 only? */
	{ 0x140000, 0x14ffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x170000, 0x170fff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x171000, 0x171fff, nmk_txvideoram_w },	/* mirror */
	{ 0x1f0000, 0x1f7fff, MWA_BANK1 },	/* Work RAM */
	{ 0x1f8000, 0x1f8fff, MWA_BANK2, &spriteram, &spriteram_size },
	{ 0x1f9000, 0x1fffff, MWA_BANK3, &ram },	/* Work RAM again */
	{ -1 }
};

static struct MemoryReadAddress macross2_sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK9 },
	{ 0xa000, 0xa000, MRA_NOP },	/* IRQ ack? watchdog? */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xf000, 0xf000, soundlatch_r },	/* from 68000 */
	{ -1 }
};

static struct MemoryWriteAddress macross2_sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe001, 0xe001, macross2_sound_bank_w },
	{ 0xf000, 0xf000, soundlatch2_w },	/* to 68000 */
	{ -1 }
};

static struct IOReadPort macross2_sound_readport[] =
{
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x80, 0x80, OKIM6295_status_0_r },
	{ 0x88, 0x88, OKIM6295_status_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort macross2_sound_writeport[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, OKIM6295_data_0_w },
	{ 0x88, 0x88, OKIM6295_data_1_w },
	{ 0x90, 0x97, macross2_oki6295_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress bjtwin_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x080001, input_port_0_r },
	{ 0x080002, 0x080003, input_port_1_r },
	{ 0x080008, 0x080009, input_port_2_r },
	{ 0x08000a, 0x08000b, input_port_3_r },
	{ 0x084000, 0x084001, OKIM6295_status_0_r },
	{ 0x084010, 0x084011, OKIM6295_status_1_r },
	{ 0x088000, 0x0887ff, paletteram_word_r },
	{ 0x09c000, 0x09cfff, nmk_bgvideoram_r },
	{ 0x09d000, 0x09dfff, nmk_bgvideoram_r },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MRA_BANK1 },
	{ 0x0f8000, 0x0f8fff, MRA_BANK2 },
	{ 0x0f9000, 0x0fffff, MRA_BANK3 },
	{ -1 }
};

static struct MemoryWriteAddress bjtwin_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x084000, 0x084001, OKIM6295_data_0_w },
	{ 0x084010, 0x084011, OKIM6295_data_1_w },
	{ 0x084020, 0x08402f, bjtwin_oki6295_bankswitch_w },
	{ 0x088000, 0x0887ff, nmk_paletteram_w, &paletteram },
	{ 0x094000, 0x094001, nmk_tilebank_w },
	{ 0x094002, 0x094003, MWA_NOP },	/* IRQ enable? */
	{ 0x09c000, 0x09cfff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09d000, 0x09dfff, nmk_bgvideoram_w },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MWA_BANK1 },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA_BANK2, &spriteram, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA_BANK3 },	/* Work RAM again */
	{ -1 }
};



INPUT_PORTS_START( hachamf )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* I think coin inputs come from */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* the second cpu */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( macross )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
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
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "Japanese" )
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

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

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

static struct GfxLayout tilelayout =
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

static struct GfxDecodeInfo macross_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo macross2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 32 },	/* color 0x100-0x2ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bjtwin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX2, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	2,              					/* 2 chips */
	{ 16000000/4/165, 16000000/4/165 },	/* 24242Hz frequency? */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory region */
	{ 50, 50 }							/* volume */
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	2000000,	/* 2 MHz ??? */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static const struct MachineDriver machine_driver_hachamf =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz ? */
			hachamf_readmem,hachamf_writemem,0,0,
			m68_level4_irq,1,
			m68_level1_irq,112	/* ???????? */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0*8, 32*8-1, 2*8, 30*8-1 },
	macross_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	nmk_eof_callback,
	macross_vh_start,
	nmk_vh_stop,
	macross_vh_screenrefresh,

	0,0,0,0,
	{
		/* there's also a YM2203 */
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static const struct MachineDriver machine_driver_macross =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz ? */
			macross_readmem,macross_writemem,0,0,
			m68_level4_irq,1,
			m68_level1_irq,112	/* ???????? */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0*8, 32*8-1, 2*8, 30*8-1 },
	macross_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	nmk_eof_callback,
	macross_vh_start,
	nmk_vh_stop,
	macross_vh_screenrefresh,

	0,0,0,0,
	{
		/* there's also a YM2203 */
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static const struct MachineDriver machine_driver_macross2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz ? */
			macross2_readmem,macross2_writemem,0,0,
			m68_level4_irq,1,
			m68_level1_irq,112	/* ???????? */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000, /* 4 MHz ? */
			macross2_sound_readmem,macross2_sound_writemem,macross2_sound_readport,macross2_sound_writeport,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	512, 256, { 0*8, 48*8-1, 2*8, 30*8-1 },
	macross2_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	nmk_eof_callback,
	macross2_vh_start,
	nmk_vh_stop,
	macross_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static const struct MachineDriver machine_driver_bjtwin =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz? It's a P12, but xtals are 10MHz and 16MHz */
			bjtwin_readmem,bjtwin_writemem,0,0,
			m68_level4_irq,1,
			m68_level1_irq,112	/* ?? drives music */
		}
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
	nmk_eof_callback,
	bjtwin_vh_start,
	nmk_vh_stop,
	bjtwin_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



ROM_START( hachamf )
	ROM_REGION( 0x40000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_EVEN( "hmf_07.rom",  0x00000, 0x20000, 0x9d847c31 )
	ROM_LOAD_ODD ( "hmf_06.rom",  0x00000, 0x20000, 0xde6408a0 )

	ROM_REGION( 0x10000, REGION_CPU2 )		/* unknown  - sound cpu ?????? */
	ROM_LOAD( "hmf_01.rom",  0x00000, 0x10000, 0x9e6f48fc )

	ROM_REGION( 0x020000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "hmf_05.rom",  0x000000, 0x020000, 0x29fb04a2 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "hmf_04.rom",  0x000000, 0x080000, 0x05a624e3 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "hmf_08.rom",  0x000000, 0x100000, 0x7fd0f556 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "hmf_02.rom",  0x000000, 0x080000, 0x3f1e67f2 )

	ROM_REGION( 0x080000, REGION_SOUND2 )	/* OKIM6295 samples */
	ROM_LOAD( "hmf_03.rom",  0x000000, 0x080000, 0xb25ed93b )
ROM_END

ROM_START( macross )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_WIDE_SWAP( "921a03",        0x00000, 0x80000, 0x33318d55 )

	ROM_REGION( 0x10000, REGION_CPU2 )		/* sound program (unknown CPU) */
	ROM_LOAD( "921a02",      0x00000, 0x10000, 0x77c082c7 )

	ROM_REGION( 0x020000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "921a01",      0x000000, 0x020000, 0xbbd8242d )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "921a04",      0x000000, 0x200000, 0x4002e4bb )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "921a07",      0x000000, 0x200000, 0x7d2bf112 )	/* Sprites */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "921a05",      0x000000, 0x080000, 0xd5a1eddd )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* OKIM6295 samples */
	ROM_LOAD( "921a06",      0x000000, 0x080000, 0x89461d0f )

	ROM_REGION( 0x0220, REGION_PROMS )
	ROM_LOAD( "921a08",      0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "921a09",      0x0100, 0x0100, 0x633ab1c9 )	/* unknown */
	ROM_LOAD( "921a10",      0x0200, 0x0020, 0x8371e42d )	/* unknown */
ROM_END

ROM_START( macross2 )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_WIDE_SWAP( "mcrs2j.3",      0x00000, 0x80000, 0x36a618fe )

	ROM_REGION( 0x30000, REGION_CPU2 )		/* Z80 code */
	ROM_LOAD( "mcrs2j.2",    0x00000, 0x20000, 0xb4aa8ac7 )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "mcrs2j.1",    0x000000, 0x020000, 0xc7417410 )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bp932an.a04", 0x000000, 0x200000, 0xc4d77ff0 )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "bp932an.a07", 0x000000, 0x200000, 0xaa1b21b9 )	/* Sprites */
	ROM_LOAD_GFX_SWAP( "bp932an.a08", 0x200000, 0x200000, 0x67eb2901 )

	ROM_REGION( 0x240000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a06", 0x040000, 0x200000, 0xef0ffec0 )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a05", 0x040000, 0x100000, 0xb5335abb )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS )
	ROM_LOAD( "mcrs2bpr.9",  0x0000, 0x0100, 0x435653a2 )	/* unknown */
	ROM_LOAD( "mcrs2bpr.10", 0x0100, 0x0100, 0xe6ead349 )	/* unknown */
ROM_END

ROM_START( bjtwin )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_EVEN( "bjt.77",  0x00000, 0x40000, 0x7830a465 )
	ROM_LOAD_ODD ( "bjt.76",  0x00000, 0x40000, 0x7cd4e72a )

	ROM_REGION( 0x010000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bjt.35",		0x000000, 0x010000, 0xaa13df7c )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bjt.32",		0x000000, 0x100000, 0x8a4f26d0 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "bjt.100",	0x000000, 0x100000, 0xbb06245d )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "bjt.130",    0x040000, 0x100000, 0x372d46dd )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2 )	/* OKIM6295 samples */
	ROM_LOAD( "bjt.127",    0x040000, 0x100000, 0x8da67808 )	/* all banked */
ROM_END

ROM_START( nouryoku )
	ROM_REGION( 0x80000, REGION_CPU1 )		/* 68000 code */
	ROM_LOAD_EVEN( "ic76.1",  0x00000, 0x40000, 0x26075988 )
	ROM_LOAD_ODD ( "ic75.2",  0x00000, 0x40000, 0x75ab82cd )

	ROM_REGION( 0x010000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ic35.3",		0x000000, 0x010000, 0x03d0c3b1 )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "ic32.4",		0x000000, 0x200000, 0x88d454fd )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD_GFX_SWAP( "ic100.5",	0x000000, 0x200000, 0x24d3e24e )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.6",     0x040000, 0x100000, 0xfeea34f4 )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.7",     0x040000, 0x100000, 0x8a69fded )	/* all banked */
ROM_END



static unsigned char decode_byte(unsigned char src, unsigned char *bitp)
{
	unsigned char ret, i;

	ret = 0;
	for (i=0; i<8; i++)
		ret |= (((src >> bitp[i]) & 1) << (7-i));

	return ret;
}

static unsigned long bjtwin_address_map_bg0(unsigned long addr)
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


static unsigned long bjtwin_address_map_sprites(unsigned long addr)
{
   return ((addr&0x00010)>> 4) | ((addr&0x20000)>>16) | ((addr&0x100000)>>18);
}


static void decode_gfx(void)
{
	/* GFX are scrambled.  We decode them here.  (BIG Thanks to Antiriad for descrambling info) */
	unsigned char *rom;
	int A;

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
}



static void init_nmk(void)
{
	decode_gfx();
}

static void init_hachamf(void)
{
	const UINT8 *rom = memory_region(REGION_CPU1);

	WRITE_WORD(&rom[0x0006], 0x7dc2);	/* replace reset vector with the "real" one */
}

static void init_bjtwin(void)
{
	init_nmk();

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



GAMEX( 1991, hachamf,  0, hachamf,  hachamf,  hachamf,  ROT0,   "NMK", "Hacha Mecha Fighter", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1992, macross,  0, macross,  macross,  nmk,      ROT270, "Banpresto", "Macross", GAME_NO_SOUND )
GAMEX( 1993, macross2, 0, macross2, macross,  0,        ROT0,   "Banpresto", "Macross II", GAME_NO_COCKTAIL )
GAMEX( 1993, bjtwin,   0, bjtwin,   bjtwin,   bjtwin,   ROT270, "NMK", "Bombjack Twin", GAME_NO_COCKTAIL )
GAMEX( 1995, nouryoku, 0, bjtwin,   nouryoku, nmk,      ROT0,   "Tecmo", "Nouryoku Koujou Iinkai", GAME_NO_COCKTAIL )
