
/* Sega System C/C2 Driver
   -----------------------
   Version 0.31 - September 2000

   Very Early Driver .. Lots and Lots and Lots (and even more than that) of issues :)

  Sega's C2 was used between 1989 and 1994, the hardware being very similar to that
  used by the Sega MegaDrive/Genesis Home Console Sega produced around the same time.

  Year  Game                  Developer         Versions Dumped  Board  Status		 Gen Ver Exists?
  1989  Bloxeed               Sega / Elorg      Eng		         C      Playable      n 
  1990  Borench               Sega              Eng              C2     Playable      n
  1990  Columns               Sega              Jpn              C      Playable      y
  1990  Columns II            Sega              Jpn              C      Playable      n
  1992  Tant-R                Sega              Jpn, JpnBL       C2     Partial       y
  1990  ThunderForce AC       Sega / Technosoft Eng, Jpn, EngBL  C2     Not Working   y (as ThunderForce 3)
  1992  Puyo Puyo             Sega / Compile    Jpn (2 Vers)     C2     Playable      y
  1994  Ichident-R            Sega              Jpn              C2     Not Working   y
  1994  PotoPoto              Sega              Jpn              C2     Playable      n
  1994  Puyo Puyo 2           Compile           Jpn              C2     Not Working   y
  1994  Stack Columns         Sega              Jpn              C2     Playable      n
  1994  Zunzunkyou No Yabou   Sega              Jpn              C2     Playable      n
  
   Notes: Eng indicates game is in the English Language, Most Likely a European / US Romset
          Jpn indicates the game plays in Japanese and is therefore likely a Japanes Romset

		  Bloxeed doesn't Read from the Protection Chip at all.  All of the other games do
		  and presently how the Protection Chip functions is unknown so the code is being
		  patched to ensure the games work.  The game boards apparently contain a PROM of
		  some kind which might need dumping for correct emulation of the protection chip...

		  I'm assuming System-C was the Board without the uPD7759 chip and System-C2 was the
		  version of the board with it, this could be completely wrong but it doesn't really
		  matter anyway.

		  Restructured some of this code to be more like the C2-Emu .. and am temp. using a
		  few bits of code from that (writes .. dma etc.. ) I really think I should write my
		  own..

		  Portability .. Ummm I think there might have to be some changes to make it work on
		  other machines .. I have no idea how to do this

		  Sound isn't emulated yet..

		  Interrupts (IRQ4.. etc) aren't yet accurate (actually we're just doing 1 irq6 a
		  frame at the moment which isn't ideal ...

		  Shadow / Hi-Light

		  Vertical 2 Cell Based Scrolling won't be 100% accurate on a line that uses a Line
		  Scroll value which isn't divisible by 8.. I've never seen a C2 game do this tho.

		  Window .. Flags .. HV Counter .. 
		  
		  Sprite Flipping needs doing ..

		  Dipswitches on all games (should be easy to put in as 99% of games tell you 'em in
		  their test modes.. just time consuming)
		  
		  Yes the rendering code is rather odd .. imo .. but using the tilemap was yielding
		  5 fps because of the way everything is generated in ram and potentially dirty ...

		  Plus more issues .. but spirit of open source projects might just get them
		  fixed up ;) .. and I've not finished working on this myself..
 

  Thanks: (in no particular order)  to any MameDev that helped me out .. (OG, Mish etc.)
		  Charles MacDonald for his C2Emu .. without it working out what were bugs in my code
		  and issues due to protection would have probably killed the driver long ago :p
		  Razoola & Antiriad .. for helping teach me some 68k ASM needed to work out just why
		  the games were crashing :)
		  Sega for producing some Fantastic Games...
		  and anyone else who knows they've contributed :)


.. First Driver .. I Can't Really Expect it to be perfect .. 

*/


/******************************************************************************/
/* drivers\segac2.c                                                           */
/******************************************************************************/

/*** Required *****************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

/*** Function Prototypes ******************************************************/

/* in drivers\segac2.c */
static READ_HANDLER ( segac2_paletteram_word_r );
static WRITE_HANDLER( segac2_paletteram_w );
static READ_HANDLER ( segac2_io_r );
static WRITE_HANDLER( segac2_io_w );
static READ_HANDLER ( segac2_prot_r );
static WRITE_HANDLER( segac2_prot_w );

/* in vidhrdw\segac2.c */
int segac2_vh_start(void);
void segac2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
READ_HANDLER ( segac2_vdp_r );
WRITE_HANDLER( segac2_vdp_w );

/*** Driver Variables *********************************************************/

char palbank, palscr, palspr;

char pba1, pba2; /* Game Specific, Set if the 'Accurate' Palette Banking System
                    works with a game.. Unused at the moment */

/*** Memory Maps **************************************************************/

static struct MemoryReadAddress segac2_readmem[] =
{
	{ 0x000000, 0x1fffff, MRA_ROM }, /* Main 68k Program Roms */
	{ 0x800000, 0x800001, segac2_prot_r }, /* The Protection Chip? */
	{ 0x840000, 0x84001F, segac2_io_r }, /* I/O (Controls etc.) */
/*  { 0x840100, 0x840107, YM3438_r }, */ /* Ym3438 Sound Chip Status Register */
	{ 0x8C0000, 0x8C03FF, segac2_paletteram_word_r }, /* Palette Ram */
	{ 0xC00000, 0xC0001F, segac2_vdp_r }, /* VDP Access */
	{ 0xFE0000, 0xFEFFFF, MRA_BANK1 }, /* Main Ram (Mirror?) */
	{ 0xFF0000, 0xFFFFFF, MRA_BANK1 }, /* Main Ram */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress segac2_writemem[] =
{
	{ 0x000000, 0x1fffff, MWA_ROM }, /* Main 68k Program Roms */
	{ 0x800000, 0x800001, segac2_prot_w }, /* The Protection Chip? */
	{ 0x840000, 0x84001F, segac2_io_w }, /* I/O Chip */
/*  { 0x840100, 0x840107, YM3438_w }, */ /* Ym3438 Sound Chip Status Register */
	{ 0x8C0000, 0x8C03FF, segac2_paletteram_w },  /* Palette Ram */
	{ 0xC00000, 0xC0001F, segac2_vdp_w }, /* VDP Access */
	{ 0xFE0000, 0xFEFFFF, MWA_BANK1 },  /* Main Ram (Mirror?) */
	{ 0xFF0000, 0xFFFFFF, MWA_BANK1 }, /* Main Ram */
	{ -1 }	/* end of table */
};

/*** Input Ports (Dipswitch, Controls etc.) ***********************************/

INPUT_PORTS_START( segac2 )
	PORT_START // 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )								// Coin Slot #1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )								// Coin Slot #2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 )							// Test Mode Button
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0,"SERVICE",IPT_BUTTON5,IP_JOY_NONE  )	// Service Button
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )								// Player 1 Start
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )								// Player 2 Start
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )							// No Use?
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )							// No Use?
	PORT_START // 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )							// Player 1 Button 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )							// Player 1 Button 2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )							// Player 1 Button 3
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )							// Player 1 Button 4
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )						// Player 1 DOWN
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )						// Player 1 UP
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )						// Player 1 RIGHT
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )						// Player 1 LEFT
	PORT_START // 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)				// Player 2 Button 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)				// Player 2 Button 2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)				// Player 2 Button 3
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2)				// Player 2 Button 4
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)			// Player 2 DOWN
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)			// Player 2 UP
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)		// Player 2 RIGHT
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)			// Player 2 LEFT
	PORT_START // 3
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )							// Coinage Side 1 #1
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )							// Coinage Side 1 #2
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )							// Coinage Side 1 #3
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )							// Coinage Side 1 #4
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )							// Coinage Side 2 #1
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )							// Coinage Side 2 #2
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )							// Coinage Side 2 #3
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )							// Coinage Side 2 #4
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START // 4
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )							// Game Options..
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

/*** Game Specific Initilization **********************************************/

static void init_segac2(void)
{
	pba1 = 0;
	pba2 = 0;
}

static void init_bloxeedc(void)
{
	/* Bloxeed doesn't read from the Protection Device, No Patches Required */
	pba1 = 1;
	pba2 = 1;
}

static void init_columns(void)
{
	/* Seems to Work Correctly With these Patches */
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x03572],0x6000);
	WRITE_WORD(&ROM[0x02FBA],0x6000);
	WRITE_WORD(&ROM[0x02700],0x6000);
	WRITE_WORD(&ROM[0x02906],0x6000);
	WRITE_WORD(&ROM[0x01EBA],0x6000);
	pba1 = 1;
	pba2 = 1;
}

static void init_columns2(void)
{
	/* Seems to Work Correctly With these Patches */
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x00E1E],0x6000);
	WRITE_WORD(&ROM[0x02FAC],0x6000);
	WRITE_WORD(&ROM[0x03712],0x6000);
	WRITE_WORD(&ROM[0x00FC8],0x6000);
	WRITE_WORD(&ROM[0x01FB6],0x6000);
	WRITE_WORD(&ROM[0x025B2],0x6000);
	WRITE_WORD(&ROM[0x027CA],0x6000);
	pba1 = 1;
	pba2 = 1;
}

static void init_potopoto(void)
{
	/* Seems to Work Correctly With these Patches .. Test mode Exit doesn't work (?)*/
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x03CAA],0x6000);
	pba1 = 1;
	pba2 = 1;
}

static void init_puyopuyo(void)
{
	/* Seems to Work Correctly With these Patches */
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x00C1C],0x6000);
	WRITE_WORD(&ROM[0x010F8],0x4E71);
	WRITE_WORD(&ROM[0x010FA],0x4E71);
	pba1 = 1;
	pba2 = 1;
}

static void init_stkclmns(void)
{
	/* Still Some Issues .. Hi-Score name can't be entered etc. */
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x0A514],0x4E71);
	WRITE_WORD(&ROM[0x0A516],0x4E71);
	WRITE_WORD(&ROM[0x0A518],0x4E71);
	WRITE_WORD(&ROM[0x03456],0x6000);
	WRITE_WORD(&ROM[0x0058A],0x6000);
	WRITE_WORD(&ROM[0x00E82],0x6000);
	WRITE_WORD(&ROM[0x0CA36],0x6000);
	WRITE_WORD(&ROM[0x147CE],0x6000);
	WRITE_WORD(&ROM[0x14B86],0x6000);
	WRITE_WORD(&ROM[0x16810],0x6000);
	pba1 = 1;
	pba2 = 1;
}

static void init_zunkyou(void)
{
	/* Seems to Work Correctly With these Patches */
	unsigned char *ROM = memory_region(REGION_CPU1);
	WRITE_WORD(&ROM[0x00AE8],0x6000);
	pba1 = 1;
	pba2 = 1;
}

/*** General Machine Related Functions / Handlers *****************************/

static int segac2_interrupt(void)
{
	return 6;
}

static READ_HANDLER( segac2_paletteram_word_r )
{
	offset += palbank * 0x400;
	return READ_WORD(&paletteram[offset]);
}

static WRITE_HANDLER( segac2_paletteram_w )
{
	int r,g,b,oldword,newword;
	offset += palbank * 0x400;
	oldword = READ_WORD (&paletteram[offset]);
	newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&paletteram[offset], newword);
	r = (data >> 0) & 0x0f;
	g = (data >> 4) & 0x0f;
	b = (data >> 8) & 0x0f;
	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;
	palette_change_color(offset/2,r,g,b);
}

static READ_HANDLER( segac2_io_r )
{
	switch (offset)
	{
		case 0x00: /* Player 1 Input */
			return (readinputport(1));
		case 0x02: /* Player 2 Input */
			return (readinputport(2));
		case 0x08: /* Control Panel */
			return (readinputport(0));
		case 0x0A: /* DSW #1 */
			return (readinputport(3));
		case 0x0C: /* DSW #2 */
			return (readinputport(4));
		case 0x10: return 'S';
		case 0x12: return 'E';
		case 0x14: return 'G';
		case 0x16: return 'A';
		default:
			logerror("PC:%06x: Unknown I/O Read Offset %02x\n",cpu_get_pc(),offset);			
			return (0xFF);
	}
	return (0xFF);
}

static WRITE_HANDLER( segac2_io_w )
{
	switch (offset)
	{
		case 0x0E:
		    if ( ( (data & 0xF0) != 0x10) &&  ( (data & 0xF0) != 0xF0) &&  ( (data & 0xF0) != 0xE0))  /* This also seems to be used for sample banking ... */
		    	palbank = data & 0x03;
			break;
		default:
			logerror("PC:%06x: Unknown I/O Write Offset %02x Data %04x\n",cpu_get_pc(),offset,data);			
			break;
	}
}

static READ_HANDLER( segac2_prot_r )
{
	return (0xFF);
}

static WRITE_HANDLER( segac2_prot_w )
{  /* The Protection Values are read from here .. but writing certainly seems to be colour banking */
	palscr = data & 0x03;
	palspr = (data >> 2) & 0x03;
}

/*** Machine Drivers **********************************************************/

static struct MachineDriver machine_driver_segac2 =
{
	{
		{
			CPU_M68000,
			10000000, 
			segac2_readmem, segac2_writemem,0,0,
			segac2_interrupt, 1 // 262 @ a later date ..
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	NULL,

	/* video hardware */
	336, 224, { 8, 336-9, 0, 224-1 },   
	NULL,   
	0x1000, 0x1000,
	NULL,  
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	segac2_vh_start,
	generic_vh_stop,
	segac2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{0}
	},

	NULL
};

/*** Rom Definitions **********************************************************/

/* System C Games */

ROM_START( bloxeedc ) /* Bloxeed (C System Version)  (c)1989 Sega / Elorg */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 512kb */
    ROM_LOAD_EVEN("12908.bin"   ,0x000000,0x020000, 0xfc77cb91 )
	ROM_LOAD_ODD ("12907.bin"   ,0x000000,0x020000, 0xe5fcbac6 )
	ROM_LOAD_EVEN("12993.bin"   ,0x040000,0x020000, 0x487bc8fc )
	ROM_LOAD_ODD ("12992.bin"   ,0x040000,0x020000, 0x19b0084c )
ROM_END

ROM_START( columns ) /* Columns (Jpn) (c)1990 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 256kb */
	ROM_LOAD_EVEN("epr13112.32" ,0x000000,0x020000, 0xbae6e53e )
	ROM_LOAD_ODD ("epr13111.31" ,0x000000,0x020000, 0xaa5ccd6d )
ROM_END

ROM_START( columns2 ) /* Columns II - The Voyage Through Time (Jpn)  (c)1990 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 256kb */
	ROM_LOAD_EVEN("epr13361.rom",0x000000,0x020000, 0xb54b5f12 )
	ROM_LOAD_ODD ("epr13360.rom",0x000000,0x020000, 0xa59b1d4f )
ROM_END

/* System C2 Games(?) */

ROM_START( borench ) /* Borench  (c)1990 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 512kb */
	ROM_LOAD_EVEN("ic32.bin"    ,0x000000,0x040000, 0x2c54457d )
	ROM_LOAD_ODD ("ic31.bin"    ,0x000000,0x040000, 0xb46445fc ) 
	ROM_REGION( 0x020000, REGION_SOUND1 ) /* 128kb for Samples */
	ROM_LOAD     ("ic4.bin"     ,0x000000,0x020000, 0x62b85e56 )
ROM_END

ROM_START( tfrceac ) /* ThunderForce AC  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 1536kb */
	ROM_LOAD_EVEN("ic32.bin",    0x000000,0x040000, 0x95ecf202 )
	ROM_LOAD_ODD ("ic31.bin",    0x000000,0x040000, 0xe63d7f1a )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD_EVEN("ic34.bin",    0x100000,0x040000, 0x29f23461 )
	ROM_LOAD_ODD ("ic33.bin",    0x100000,0x040000, 0x9e23734f )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("ic4.bin",     0x000000,0x040000, 0xe09961f6 )
ROM_END

ROM_START( tfrceacj ) /* ThunderForce AC (Jpn)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 1536kb */
	ROM_LOAD_EVEN("epr13657.32", 0x000000,0x040000, 0xa0f38ffd )
	ROM_LOAD_ODD ("epr13656.31", 0x000000,0x040000, 0xb9438d1e )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD_EVEN("ic34.bin",    0x100000,0x040000, 0x29f23461 )
	ROM_LOAD_ODD ("ic33.bin",    0x100000,0x040000, 0x9e23734f )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("ic4.bin",     0x000000,0x040000, 0xe09961f6 )
ROM_END

ROM_START( tfrceacb ) /* ThunderForce AC (Bootleg)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 1536kb */
	ROM_LOAD_EVEN("4.bin",       0x000000,0x040000, 0xeba059d3 )
	ROM_LOAD_ODD ("3.bin",       0x000000,0x040000, 0x3e5dc542 )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD_EVEN("ic34.bin",    0x100000,0x040000, 0x29f23461 )
	ROM_LOAD_ODD ("ic33.bin",    0x100000,0x040000, 0x9e23734f )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("1.bin",       0x000000,0x020000, 0x4e2ca65a )
	ROM_LOAD     ("2.bin",       0x020000,0x020000, 0x3a9be8ef )
ROM_END

ROM_START( tantr ) /* Tant-R (Puzzle & Action)  (c)1992 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 2048kb */
	ROM_LOAD_EVEN("epr15614.32", 0x000000,0x080000, 0x557782bc )
	ROM_LOAD_ODD ("epr15613.31", 0x000000,0x080000, 0x14bbb235 )
	ROM_LOAD_EVEN("mpr15616.34", 0x100000,0x080000, 0x17b80202 )
	ROM_LOAD_ODD ("mpr15615.33", 0x100000,0x080000, 0x36a88bd4 )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("epr15617.4",  0x000000,0x040000, 0x338324a1 )
ROM_END

ROM_START( tantrbl ) /* Tant-R (Puzzle & Action) (Bootleg)  (c)1992 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 2048kb */
	ROM_LOAD_EVEN("pa_e10.bin",  0x000000,0x080000, 0x6c3f711f )
	ROM_LOAD_ODD ("pa_f10.bin",  0x000000,0x080000, 0x75526786 )
	ROM_LOAD_EVEN("mpr15616.34", 0x100000,0x080000, 0x17b80202 )
	ROM_LOAD_ODD ("mpr15615.33", 0x100000,0x080000, 0x36a88bd4 )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("pa_e02.bin",  0x000000,0x020000, 0x4e85b2a3 )
	ROM_LOAD     ("pa_e03.bin",  0x020000,0x020000, 0x72918c58 )
ROM_END

ROM_START( puyopuyo	) /* Puyo Puyo  (c)1992 Sega / Compile */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /*1280kb */
	ROM_LOAD_EVEN("epr15036",    0x000000,0x020000, 0x5310ca1b )
	ROM_LOAD_ODD ("epr15035",    0x000000,0x020000, 0xbc62e400 )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD_EVEN("epr15038",    0x100000,0x020000, 0x3b9eea0c )
	ROM_LOAD_ODD ("epr15037",    0x100000,0x020000, 0xbe2f7974 )
	ROM_REGION( 0x020000, REGION_SOUND1 ) /* 128kb for Samples */
	ROM_LOAD     ("epr15034",    0x000000,0x020000, 0x5688213b )
ROM_END

ROM_START( puyopuya	) /* Puyo Puyo (Rev A)  (c)1992 Sega / Compile */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /*1280kb */
	ROM_LOAD_EVEN("ep15036a.32", 0x00000000,0x00020000, 0x61b35257 )
	ROM_LOAD_ODD ("ep15035a.31", 0x00000000,0x00020000, 0xdfebb6d9 )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD_EVEN("epr15038",    0x100000,0x020000, 0x3b9eea0c )
	ROM_LOAD_ODD ("epr15037",    0x100000,0x020000, 0xbe2f7974 )
	ROM_REGION( 0x020000, REGION_SOUND1 ) /* 128kb for Samples */
	ROM_LOAD     ("epr15034",    0x000000,0x020000, 0x5688213b )
ROM_END

ROM_START( ichident ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 2048kb */
	ROM_LOAD_EVEN("epr16886", 0x000000,0x080000, 0x38208e28 )
	ROM_LOAD_ODD ("epr16885", 0x000000,0x080000, 0x1ce4e837 )
	ROM_LOAD_EVEN("epr16888", 0x100000,0x080000, 0x85d73722 )
	ROM_LOAD_ODD ("epr16887", 0x100000,0x080000, 0xbc3bbf25 )
	ROM_REGION( 0x080000, REGION_SOUND1 ) /* 512kb for Samples */
	ROM_LOAD     ("epr16884", 0x000000,0x080000, 0xfd9dcdd6)
ROM_END

ROM_START( stkclmns ) /* Stack Columns  (c)1994 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 2048kb */
	ROM_LOAD_EVEN("epr16795.32", 0x000000,0x080000, 0xb478fd02 )
	ROM_LOAD_ODD ("epr16794.31", 0x000000,0x080000, 0x6d0e8c56 )
	ROM_LOAD_EVEN("mpr16797.34", 0x100000,0x080000, 0xb28e9bd5 )
	ROM_LOAD_ODD ("mpr16796.33", 0x100000,0x080000, 0xec7de52d )
	ROM_REGION( 0x020000, REGION_SOUND1 ) /* 128kb for Samples */
	ROM_LOAD     ("epr16793.4",  0x000000,0x020000, 0xebb2d057 )
ROM_END

ROM_START( puyopuy2 ) /* Puyo Puyo 2  (c)1994 Compile */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 1024kb */
	ROM_LOAD_EVEN("pp2.eve",     0x000000,0x080000, 0x1cad1149 )
	ROM_LOAD_ODD ("pp2.odd",     0x000000,0x080000, 0xbeecf96d )
	ROM_REGION( 0x080000, REGION_SOUND1 ) /* 512kb for Samples */
	ROM_LOAD     ("pp2.snd",     0x000000,0x080000, 0x020ff6ef )
ROM_END

ROM_START( potopoto ) /* Poto Poto  (c)1994 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 512kb */
	ROM_LOAD_EVEN("epr16662",    0x000000,0x040000, 0xbbd305d6 )
	ROM_LOAD_ODD ("epr16661",    0x000000,0x040000, 0x5a7d14f4 )
	ROM_REGION( 0x040000, REGION_SOUND1 ) /* 256kb for Samples */
	ROM_LOAD     ("epr16660",    0x000000,0x040000, 0x8251c61c )
ROM_END

ROM_START( zunkyou ) /* Zunzunkyou No Yabou  (c)1994 Sega */
	ROM_REGION( 0x1000000, REGION_CPU1 ) /* 2048kb */
	ROM_LOAD_EVEN("epr16812.32", 0x000000,0x080000, 0xeb088fb0 )
	ROM_LOAD_ODD ("epr16811.31", 0x000000,0x080000, 0x9ac7035b )
	ROM_LOAD_EVEN("epr16814.34", 0x100000,0x080000, 0x821b3b77 )
	ROM_LOAD_ODD ("epr16813.33", 0x100000,0x080000, 0x3cba9e8f )
	ROM_REGION( 0x080000, REGION_SOUND1 ) /* 512kb for Samples */
	ROM_LOAD     ("epr16810.4",  0x000000,0x080000, 0xd542f0fe )
ROM_END

/*** GameDrivers **********************************************************/

GAMEX(1989, bloxeedc, 0, segac2, segac2, bloxeedc, ROT0, "Sega / Elorg", "Bloxeed (C System)", GAME_NO_SOUND )
GAMEX(1990, columns, 0, segac2, segac2, columns, ROT0, "Sega", "Columns (Japan)", GAME_NO_SOUND )
GAMEX(1990, columns2, 0, segac2, segac2, columns2, ROT0, "Sega", "Columns II - The Voyage Through Time (Japan)", GAME_NO_SOUND )
GAMEX(1990, borench, 0, segac2, segac2, segac2, ROT0, "Sega", "Borench", GAME_NO_SOUND )
GAMEX(1990, tfrceac, 0, segac2, segac2, segac2, ROT0, "Sega / Technosoft", "ThunderForce AC", GAME_NOT_WORKING )
GAMEX(1990, tfrceacj, tfrceac, segac2, segac2, segac2, ROT0, "Sega / Technosoft", "ThunderForce AC (Japan)", GAME_NOT_WORKING )
GAMEX(1990, tfrceacb, tfrceac, segac2, segac2, segac2, ROT0, "Sega / Technosoft", "ThunderForce AC (Bootleg)", GAME_NOT_WORKING )
GAMEX(1992, tantr, 0, segac2, segac2, segac2, ROT0, "Sega", "Tant-R (Puzzle & Action) (Japan)", GAME_NOT_WORKING )
GAMEX(1992, tantrbl, tantr, segac2, segac2, segac2, ROT0, "Sega", "Tant-R (Puzzle & Action) (Japan) (Bootleg)", GAME_NOT_WORKING )
GAMEX(1992, puyopuyo, 0, segac2, segac2, puyopuyo, ROT0, "Sega / Compile", "Puyo Puyo (Japan)", GAME_NO_SOUND )
GAMEX(1992, puyopuya, puyopuyo, segac2, segac2, puyopuyo, ROT0, "Sega / Compile", "Puyo Puyo (Japan) (Rev A)", GAME_NO_SOUND )
GAMEX(1994, ichident, 0, segac2, segac2, segac2, ROT0, "Sega", "Ichident-R (Puzzle & Action 2) (Japan)", GAME_NOT_WORKING )
GAMEX(1994, stkclmns, 0, segac2, segac2, stkclmns, ROT0, "Sega", "Stack Columns (Japan)", GAME_NO_SOUND )
GAMEX(1994, puyopuy2, 0, segac2, segac2, segac2, ROT0, "Compile", "Puyo Puyo 2 Expert (Japan)", GAME_NOT_WORKING )
GAMEX(1994, potopoto, 0, segac2, segac2, potopoto, ROT0, "Sega", "Poto Poto (Japan)", GAME_NO_SOUND )
GAMEX(1994, zunkyou, 0, segac2, segac2, zunkyou, ROT0, "Sega", "Zunzunkyou No Yabou (Japan)", GAME_NO_SOUND )
