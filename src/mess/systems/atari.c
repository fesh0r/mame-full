/******************************************************************************
    Atari 400/800

    MESS Driver

	Juergen Buchmueller, June 1998
******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/atari.h"
#include "mess/vidhrdw/atari.h"

/******************************************************************************
    Atari 800 memory map (preliminary)

    ***************** read access *******************
    range     short   description
	0000-9FFF RAM	  main memory
	A000-BFFF RAM/ROM RAM or (banked) ROM cartridges
    C000-CFFF ROM     unused or monitor ROM

    ********* GTIA    ********************************
    D000      m0pf    missile 0 playfield collisions
    D001      m1pf    missile 1 playfield collisions
    D002      m2pf    missile 2 playfield collisions
    D003      m3pf    missile 3 playfield collisions
    D004      p0pf    player 0 playfield collisions
    D005      p1pf    player 1 playfield collisions
    D006      p2pf    player 2 playfield collisions
    D007      p3pf    player 3 playfield collisions
    D008      m0pl    missile 0 player collisions
    D009      m1pl    missile 1 player collisions
    D00A      m2pl    missile 2 player collisions
    D00B      m3pl    missile 3 player collisions
    D00C      p0pl    player 0 player collisions
    D00D      p1pl    player 1 player collisions
    D00E      p2pl    player 2 player collisions
    D00F      p3pl    player 3 player collisions
    D010      but0    button stick 0
    D011      but1    button stick 1
    D012      but2    button stick 2
    D013      but3    button stick 3
	D014	  xff	  unused
	D015	  xff	  unused
	D016	  xff	  unused
	D017	  xff	  unused
	D018	  xff	  unused
	D019	  xff	  unused
	D01A	  xff	  unused
	D01B	  xff	  unused
	D01C	  xff	  unused
	D01D	  xff	  unused
	D01E	  xff	  unused
    D01F      cons    console keys
    D020-D0FF repeated 7 times

    D100-D1FF xff

	********* POKEY   ********************************
    D200      pot0    paddle 0
    D201      pot1    paddle 1
    D202      pot2    paddle 2
    D203      pot3    paddle 3
    D204      pot4    paddle 4
    D205      pot5    paddle 5
    D206      pot6    paddle 6
    D207      pot7    paddle 7
    D208      potb    all paddles
    D209      kbcode  keyboard scan code
    D20A      random  random number generator
	D20B	  xff	  unused
	D20C	  xff	  unused
    D20D      serin   serial input
    D20E      irqst   IRQ status
    D20F      skstat  sk status
    D210-D2FF repeated 15 times

	********* PIO	  ********************************
    D300      porta   read pio port A
    D301      portb   read pio port B
    D302      pactl   read pio port A control
    D303      pbctl   read pio port B control
    D304-D3FF repeated 63 times

	********* ANTIC   ********************************
	D400	  xff	  unused
	D401	  xff	  unused
	D402	  xff	  unused
	D403	  xff	  unused
	D404	  xff	  unused
	D405	  xff	  unused
	D406	  xff	  unused
	D407	  xff	  unused
	D408	  xff	  unused
	D409	  xff	  unused
	D40A	  xff	  unused
    D40B      vcount  vertical (scanline) counter
    D40C      penh    light pen horizontal pos
    D40D      penv    light pen vertical pos
	D40E	  xff	  unused
    D40F      nmist   NMI status

    D500-D7FF xff     unused memory

    D800-DFFF ROM     floating point ROM
    E000-FFFF ROM     bios ROM

    ***************** write access *******************
    range     short   description
	0000-9FFF RAM	  main memory
	A000-BFFF RAM/ROM RAM or (banked) ROM
	C000-CFFF ROM	  unused or monitor ROM

    ********* GTIA    ********************************
    D000      hposp0  player 0 horz position
    D001      hposp1  player 1 horz position
    D002      hposp2  player 2 horz position
    D003      hposp3  player 3 horz position
    D004      hposm0  missile 0 horz position
    D005      hposm1  missile 0 horz position
    D006      hposm2  missile 0 horz position
    D007      hposm3  missile 0 horz position
    D008      sizep0  size player 0
    D009      sizep1  size player 0
    D00A      sizep2  size player 0
    D00B      sizep3  size player 0
    D00C      sizem   size missiles
    D00D      grafp0  graphics data for player 0
    D00E      grafp1  graphics data for player 1
    D00F      grafp2  graphics data for player 2
    D010      grafp3  graphics data for player 3
    D011      grafm   graphics data for missiles
    D012      colpm0  color for player/missile 0
    D013      colpm1  color for player/missile 1
    D014      colpm2  color for player/missile 2
    D015      colpm3  color for player/missile 3
    D016      colpf0  color 0 playfield
    D017      colpf1  color 1 playfield
    D018      colpf2  color 2 playfield
    D019      colpf3  color 3 playfield
    D01A      colbk   background playfield
    D01B      prior   priority select
    D01C      vdelay  delay until vertical retrace
    D01D      gractl  graphics control
    D01E      hitclr  clear collisions
    D01F      wcons   write console (speaker)
    D020-D0FF repeated 7 times

	D100-D1FF xff	  unused

	********* POKEY   ********************************
    D200      audf1   frequency audio chan #1
	D201	  audc1   control audio chan #1
	D202	  audf2   frequency audio chan #2
	D203	  audc2   control audio chan #2
	D204	  audf3   frequency audio chan #3
	D205	  audc3   control audio chan #3
	D206	  audf4   frequency audio chan #4
	D207	  audc4   control audio chan #4
    D208      audctl  audio control
    D209      stimer  start timer
    D20A      skres   sk reset
    D20B      potgo   start pot AD conversion
	D20C	  xff	  unused
    D20D      serout  serial output
    D20E      irqen   IRQ enable
    D20F      skctl   sk control
    D210-D2FF repeated 15 times

	********* PIO	  ********************************
    D300      porta   write pio port A (output or mask)
	D301	  portb   write pio port B (output or mask)
    D302      pactl   write pio port A control
    D303      pbctl   write pio port B control
	D304-D3FF		  repeated

	********* ANTIC   ********************************
    D400      dmactl  write DMA control
    D401      chactl  write character control
    D402      dlistl  write display list lo
    D403      dlisth  write display list hi
    D404      hscrol  write horz scroll
    D405      vscrol  write vert scroll
	D406	  xff	  unused
    D407      pmbash  player/missile base addr hi
	D408	  xff	  unused
    D409      chbash  character generator base addr hi
    D40A      wsync   wait for hsync
	D40B	  xff	  unused
	D40C	  xff	  unused
	D40D	  xff	  unused
	D40E	  nmien   NMI enable
    D40F      nmires  NMI reset

    D500-D7FF xff     unused memory

	D800-DFFF ROM	  floating point ROM
	E000-FFFF ROM	  BIOS ROM
******************************************************************************/

static struct MemoryReadAddress readmem_a800[] =
{
	{ 0x0000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xd0ff, MRA_GTIA },
	{ 0xd100, 0xd1ff, MRA_NOP },
	{ 0xd200, 0xd2ff, pokey1_r },
    { 0xd300, 0xd3ff, MRA_PIA },
	{ 0xd400, 0xd4ff, MRA_ANTIC },
	{ 0xd500, 0xd7ff, MRA_NOP },
	{ 0xd800, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress writemem_a800[] =
{
	{ 0x0000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_BANK2 },
	{ 0xc000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd0ff, MWA_GTIA },
	{ 0xd100, 0xd1ff, MWA_NOP },
	{ 0xd200, 0xd2ff, pokey1_w },
    { 0xd300, 0xd3ff, MWA_PIA },
	{ 0xd400, 0xd4ff, MWA_ANTIC },
	{ 0xd500, 0xd7ff, MWA_NOP },
	{ 0xd800, 0xffff, MWA_ROM },
    {-1}
};

static struct MemoryReadAddress readmem_a800xl[] =
{
	{ 0x0000, 0x4fff, MRA_RAM },
	{ 0x5000, 0x57ff, MRA_BANK1 },
	{ 0x5800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xcfff, MRA_BANK3 },
	{ 0xd000, 0xd0ff, MRA_GTIA },
	{ 0xd100, 0xd1ff, MRA_NOP },
	{ 0xd200, 0xd2ff, pokey1_r },
    { 0xd300, 0xd3ff, MRA_PIA },
	{ 0xd400, 0xd4ff, MRA_ANTIC },
	{ 0xd500, 0xd7ff, MRA_NOP },
	{ 0xd800, 0xffff, MRA_BANK4 },
    {-1}
};

static struct MemoryWriteAddress writemem_a800xl[] =
{
	{ 0x0000, 0x4fff, MWA_RAM },
	{ 0x5000, 0x57ff, MWA_BANK1 },
	{ 0x5800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_BANK2 },
	{ 0xc000, 0xcfff, MWA_BANK3 },
	{ 0xd000, 0xd0ff, MWA_GTIA },
	{ 0xd100, 0xd1ff, MWA_NOP },
	{ 0xd200, 0xd2ff, pokey1_w },
    { 0xd300, 0xd3ff, MWA_PIA },
	{ 0xd400, 0xd4ff, MWA_ANTIC },
	{ 0xd500, 0xd7ff, MWA_NOP },
	{ 0xd800, 0xffff, MWA_BANK4 },
    {-1}
};

static struct MemoryReadAddress readmem_5200[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc0ff, MRA_GTIA },
	{ 0xd400, 0xd4ff, MRA_ANTIC },
	{ 0xe800, 0xe8ff, pokey1_r },
    { 0xf800, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress writemem_5200[] =
{
	{ 0x0000, 0x3fff, MWA_RAM },
	{ 0x4000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_GTIA },
	{ 0xd400, 0xd4ff, MWA_ANTIC },
	{ 0xe800, 0xe8ff, pokey1_w },
    { 0xf800, 0xffff, MWA_ROM },
    {-1}
};

INPUT_PORTS_START( input_ports_a800 )

    PORT_START  /* IN0 console keys & switch settings */
	PORT_BITX(0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Cursor keys act as",   KEYCODE_F4, IP_JOY_NONE )
    PORT_DIPSETTING(0x80, "Joystick")
    PORT_DIPSETTING(0x00, "Cursor")
	PORT_BITX(0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Television Artifacts", KEYCODE_F6, IP_JOY_NONE )
    PORT_DIPSETTING(0x40, "On")
	PORT_DIPSETTING(0x00, "Off")
	PORT_BITX(0x20, 0x00, IPT_DIPSWITCH_NAME,			   "Turn/Swap floppy images", KEYCODE_F7, IP_JOY_NONE )
    PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "cons.3: Reset",  KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "cons.2: Option", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "cons.1: Select", KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "cons.0: Start",  KEYCODE_F1, IP_JOY_NONE )

	PORT_START	/* IN1 digital joystick #1 + #2 (PIA port A) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP	| IPF_PLAYER1)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN	| IPF_PLAYER1)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT	| IPF_PLAYER1)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT | IPF_PLAYER1)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP	| IPF_PLAYER2)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN	| IPF_PLAYER2)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT	| IPF_PLAYER2)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)

	PORT_START	/* IN2 digital joystick #3 + #4 (PIA port B) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP	| IPF_PLAYER3)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN	| IPF_PLAYER3)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT	| IPF_PLAYER3)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT | IPF_PLAYER3)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP	| IPF_PLAYER4)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN	| IPF_PLAYER4)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT	| IPF_PLAYER4)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT | IPF_PLAYER4)

	PORT_START	/* IN3 digital joystick buttons (GTIA button bits) */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT(0x04, 0x04, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT(0x08, 0x08, IPT_BUTTON1 | IPF_PLAYER4)
	PORT_BIT(0x10, 0x10, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT(0x40, 0x40, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT(0x80, 0x80, IPT_BUTTON2 | IPF_PLAYER4)

	PORT_START /* IN4 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN5 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN6 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN7 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER1, 40, 8, 0x3f, 0x10, 0xf0)

    PORT_START /* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER2, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER3, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER4, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER5 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER6 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER7 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER8 */, 40, 8, 0x3f, 0x10, 0xf0)
INPUT_PORTS_END

INPUT_PORTS_START( input_ports_a800xl )

    PORT_START  /* IN0 console keys & switch settings */
	PORT_BITX(0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Cursor keys act as",   KEYCODE_F4, IP_JOY_NONE )
    PORT_DIPSETTING(0x80, "Joystick")
    PORT_DIPSETTING(0x00, "Cursor")
	PORT_BITX(0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Television Artifacts", KEYCODE_F6, IP_JOY_NONE )
    PORT_DIPSETTING(0x40, "On")
	PORT_DIPSETTING(0x00, "Off")
	PORT_BITX(0x20, 0x00, IPT_DIPSWITCH_NAME,			   "Turn/Swap floppy images", KEYCODE_F7, IP_JOY_NONE )
    PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "cons.3: Reset",  KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "cons.2: Option", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "cons.1: Select", KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "cons.0: Start",  KEYCODE_F1, IP_JOY_NONE )

	PORT_START	/* IN1 digital joystick #1 + #2 (PIA port A) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP	| IPF_PLAYER1)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN	| IPF_PLAYER1)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT	| IPF_PLAYER1)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT | IPF_PLAYER1)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP	| IPF_PLAYER2)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN	| IPF_PLAYER2)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT	| IPF_PLAYER2)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)

	PORT_START	/* IN2 digital joystick #3 + #4 (PIA port B) */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)

	PORT_START	/* IN3 digital joystick buttons (GTIA button bits) */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT(0x04, 0x04, IPT_UNUSED )
	PORT_BIT(0x08, 0x08, IPT_UNUSED )
	PORT_BIT(0x10, 0x10, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT(0x40, 0x40, IPT_UNUSED )
	PORT_BIT(0x80, 0x80, IPT_UNUSED )

	PORT_START /* IN4 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN5 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN6 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN7 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER1, 40, 8, 0x3f, 0x10, 0xf0)

    PORT_START /* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER2, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER3, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER4, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER5 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER6 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER7 */, 40, 8, 0x3f, 0x10, 0xf0)

	PORT_START /* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x80, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER8 */, 40, 8, 0x3f, 0x10, 0xf0)
INPUT_PORTS_END

/* !!! this is partially wrong !!! */
INPUT_PORTS_START( input_ports_5200 )

	PORT_START	/* IN0 switch settings */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BITX(0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Television Artifacts", KEYCODE_F6, IP_JOY_NONE )
    PORT_DIPSETTING(0x40, "On")
    PORT_DIPSETTING(0x00, "Off")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN1 keypad # 1 */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN2 keypad # 2 */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN3 keypad # 3 */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1 | IPF_PLAYER1)
    PORT_BIT(0x02, 0x02, IPT_BUTTON1 | IPF_PLAYER2)
    PORT_BIT(0x04, 0x04, IPT_BUTTON1 | IPF_PLAYER3)
    PORT_BIT(0x08, 0x08, IPT_BUTTON1 | IPF_PLAYER4)
    PORT_BIT(0x10, 0x10, IPT_BUTTON2 | IPF_PLAYER1)
    PORT_BIT(0x20, 0x20, IPT_BUTTON2 | IPF_PLAYER2)
    PORT_BIT(0x40, 0x40, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT(0x80, 0x80, IPT_BUTTON2 | IPF_PLAYER4)

	PORT_START	/* IN4 keypad # 4 */
	PORT_BIT(0x01, 0x01, IPT_BUTTON3 | IPF_PLAYER1)
    PORT_BIT(0x02, 0x02, IPT_BUTTON3 | IPF_PLAYER2)
    PORT_BIT(0x04, 0x04, IPT_BUTTON3 | IPF_PLAYER3)
    PORT_BIT(0x08, 0x08, IPT_BUTTON3 | IPF_PLAYER4)
    PORT_BIT(0x10, 0x10, IPT_BUTTON4 | IPF_PLAYER1)
    PORT_BIT(0x20, 0x20, IPT_BUTTON4 | IPF_PLAYER2)
    PORT_BIT(0x40, 0x40, IPT_BUTTON4 | IPF_PLAYER3)
    PORT_BIT(0x80, 0x80, IPT_BUTTON4 | IPF_PLAYER4)

	PORT_START	/* IN5 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN6 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN7 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER3, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER3, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER4, 40, 12, 0x3f, 0x10, 0xf0)

	PORT_START	/* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER4, 40, 12, 0x3f, 0x10, 0xf0)
INPUT_PORTS_END

#define GTIA_256COLORS	0

#if GTIA_256COLORS
#define RANGE(r0,g0,b0,r1,g1,b1) \
	 ( 0*(r1-r0)/15+r0)*4, ( 0*(g1-g0)/15+g0)*4, ( 0*(b1-b0)/15+b0)*4, \
	 ( 1*(r1-r0)/15+r0)*4, ( 1*(g1-g0)/15+g0)*4, ( 1*(b1-b0)/15+b0)*4, \
	 ( 2*(r1-r0)/15+r0)*4, ( 2*(g1-g0)/15+g0)*4, ( 2*(b1-b0)/15+b0)*4, \
	 ( 3*(r1-r0)/15+r0)*4, ( 3*(g1-g0)/15+g0)*4, ( 3*(b1-b0)/15+b0)*4, \
	 ( 4*(r1-r0)/15+r0)*4, ( 4*(g1-g0)/15+g0)*4, ( 4*(b1-b0)/15+b0)*4, \
	 ( 5*(r1-r0)/15+r0)*4, ( 5*(g1-g0)/15+g0)*4, ( 5*(b1-b0)/15+b0)*4, \
	 ( 6*(r1-r0)/15+r0)*4, ( 6*(g1-g0)/15+g0)*4, ( 6*(b1-b0)/15+b0)*4, \
	 ( 7*(r1-r0)/15+r0)*4, ( 7*(g1-g0)/15+g0)*4, ( 7*(b1-b0)/15+b0)*4, \
	 ( 8*(r1-r0)/15+r0)*4, ( 8*(g1-g0)/15+g0)*4, ( 8*(b1-b0)/15+b0)*4, \
	 ( 9*(r1-r0)/15+r0)*4, ( 9*(g1-g0)/15+g0)*4, ( 9*(b1-b0)/15+b0)*4, \
	 (10*(r1-r0)/15+r0)*4, (10*(g1-g0)/15+g0)*4, (10*(b1-b0)/15+b0)*4, \
	 (11*(r1-r0)/15+r0)*4, (11*(g1-g0)/15+g0)*4, (11*(b1-b0)/15+b0)*4, \
	 (12*(r1-r0)/15+r0)*4, (12*(g1-g0)/15+g0)*4, (12*(b1-b0)/15+b0)*4, \
	 (13*(r1-r0)/15+r0)*4, (13*(g1-g0)/15+g0)*4, (13*(b1-b0)/15+b0)*4, \
	 (14*(r1-r0)/15+r0)*4, (14*(g1-g0)/15+g0)*4, (14*(b1-b0)/15+b0)*4, \
	 (15*(r1-r0)/15+r0)*4, (15*(g1-g0)/15+g0)*4, (15*(b1-b0)/15+b0)*4
#else
#define RANGE(r0,g0,b0,r1,g1,b1) \
	 (0*(r1-r0)/7+r0)*4, (0*(g1-g0)/7+g0)*4, (0*(b1-b0)/7+b0)*4, \
	 (1*(r1-r0)/7+r0)*4, (1*(g1-g0)/7+g0)*4, (1*(b1-b0)/7+b0)*4, \
	 (2*(r1-r0)/7+r0)*4, (2*(g1-g0)/7+g0)*4, (2*(b1-b0)/7+b0)*4, \
	 (3*(r1-r0)/7+r0)*4, (3*(g1-g0)/7+g0)*4, (3*(b1-b0)/7+b0)*4, \
	 (4*(r1-r0)/7+r0)*4, (4*(g1-g0)/7+g0)*4, (4*(b1-b0)/7+b0)*4, \
	 (5*(r1-r0)/7+r0)*4, (5*(g1-g0)/7+g0)*4, (5*(b1-b0)/7+b0)*4, \
	 (6*(r1-r0)/7+r0)*4, (6*(g1-g0)/7+g0)*4, (6*(b1-b0)/7+b0)*4, \
     (7*(r1-r0)/7+r0)*4, (7*(g1-g0)/7+g0)*4, (7*(b1-b0)/7+b0)*4
#endif

static unsigned char palette[] =
{
	RANGE( 0, 0, 0, 63,63,63), /* gray		   */
	RANGE( 4, 0, 0, 63,55, 0), /* gold		   */
	RANGE(12, 0, 0, 63,43, 0), /* orange	   */
	RANGE(18, 0, 0, 63,23,23), /* red orange   */
	RANGE(15, 0,24, 63,39,55), /* pink		   */
	RANGE(10, 0, 8, 35,35,63), /* purple	   */
	RANGE(10, 0,32, 39,39,63), /* violet	   */
	RANGE( 0, 0,32, 24,39,63), /* blue1 	   */
	RANGE( 0, 4,32, 31,43,59), /* blue2 	   */
	RANGE( 0, 6,18,  0,47,63), /* light blue   */
	RANGE( 0, 8, 8, 15,63,43), /* light cyan   */
	RANGE( 0,10, 0, 39,63,13), /* cyan		   */
	RANGE( 0,10, 0,  0,63, 0), /* green 	   */
	RANGE(16,24, 0, 31,63, 0), /* light green  */
	RANGE( 4, 4, 0, 63,63, 0), /* green orange */
	RANGE( 8, 4, 0, 63,43, 0)  /* light orange */
};

#if GTIA_256COLORS
static unsigned short colortable[] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};
#else
static unsigned short colortable[] =
{
	0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,
	0x08,0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0b,0x0c,0x0c,0x0d,0x0d,0x0e,0x0e,0x0f,0x0f,
	0x10,0x10,0x11,0x11,0x12,0x12,0x13,0x13,0x14,0x14,0x15,0x15,0x16,0x16,0x17,0x17,
	0x18,0x18,0x19,0x19,0x1a,0x1a,0x1b,0x1b,0x1c,0x1c,0x1d,0x1d,0x1e,0x1e,0x1f,0x1f,
	0x20,0x20,0x21,0x21,0x22,0x22,0x23,0x23,0x24,0x24,0x25,0x25,0x26,0x26,0x27,0x27,
	0x28,0x28,0x29,0x29,0x2a,0x2a,0x2b,0x2b,0x2c,0x2c,0x2d,0x2d,0x2e,0x2e,0x2f,0x2f,
	0x30,0x30,0x31,0x31,0x32,0x32,0x33,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,
	0x38,0x38,0x39,0x39,0x3a,0x3a,0x3b,0x3b,0x3c,0x3c,0x3d,0x3d,0x3e,0x3e,0x3f,0x3f,
	0x40,0x40,0x41,0x41,0x42,0x42,0x43,0x43,0x44,0x44,0x45,0x45,0x46,0x46,0x47,0x47,
	0x48,0x48,0x49,0x49,0x4a,0x4a,0x4b,0x4b,0x4c,0x4c,0x4d,0x4d,0x4e,0x4e,0x4f,0x4f,
	0x50,0x50,0x51,0x51,0x52,0x52,0x53,0x53,0x54,0x54,0x55,0x55,0x56,0x56,0x57,0x57,
	0x58,0x58,0x59,0x59,0x5a,0x5a,0x5b,0x5b,0x5c,0x5c,0x5d,0x5d,0x5e,0x5e,0x5f,0x5f,
	0x60,0x60,0x61,0x61,0x62,0x62,0x63,0x63,0x64,0x64,0x65,0x65,0x66,0x66,0x67,0x67,
	0x68,0x68,0x69,0x69,0x6a,0x6a,0x6b,0x6b,0x6c,0x6c,0x6d,0x6d,0x6e,0x6e,0x6f,0x6f,
	0x70,0x70,0x71,0x71,0x72,0x72,0x73,0x73,0x74,0x74,0x75,0x75,0x76,0x76,0x77,0x77,
	0x78,0x78,0x79,0x79,0x7a,0x7a,0x7b,0x7b,0x7c,0x7c,0x7d,0x7d,0x7e,0x7e,0x7f,0x7f
};
#endif

static struct POKEYinterface pokey_interface = {
	1,
	CPU_APPROX,
	{ 100 },
	POKEY_DEFAULT_GAIN,
	USE_CLIP,
	{ input_port_8_r, },
	{ input_port_9_r, },
	{ input_port_10_r, },
	{ input_port_11_r, },
	{ input_port_12_r, },
	{ input_port_13_r, },
	{ input_port_14_r, },
	{ input_port_15_r, },
	{ 0, },
	{ atari_serin_r, },
	{ atari_serout_w, },
	{ atari_interrupt_cb, },
};

static struct DACinterface dac_interface =
{
	1,					/* number of DACs */
	{ 50 }				/* volume */
};

static struct MachineDriver a800_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6510,
			CPU_APPROX,
			0,
			readmem_a800,writemem_a800,0,0,
			a800_interrupt, TOTAL_LINES    /* every scanline */
        }
	},
	/* frames per second, VBL handled by atari_interrupt */
	FRAME_RATE, 0,
    1,
	a800_init_machine,
	a800_close_floppy,

	/* video hardware */
#ifdef SHOW_EVERY_SCREEN_BIT
	HWIDTH*8, TOTAL_LINES, { 0, HWIDTH*8-1, 0, VBL_START-1},
#else
	HWIDTH*8, TOTAL_LINES, { BUF_OFFS0*8, HCHARS*8-1, VBL_END, VBL_START - 1},
#endif
    0,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	0,

	VIDEO_TYPE_RASTER,
	0,
	atari_vh_start,
	atari_vh_stop,
	atari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		},
        {
			SOUND_DAC,
			&dac_interface
		}
    }
};

static struct MachineDriver a800xl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6510,
			CPU_APPROX,
			0,
			readmem_a800xl,writemem_a800xl,0,0,
			a800xl_interrupt, TOTAL_LINES	/* every scanline */
        }
	},
	/* frames per second, VBL handled by atari_interrupt */
	FRAME_RATE, 0,
    1,
	a800xl_init_machine,
	a800_close_floppy,

	/* video hardware */
#ifdef SHOW_EVERY_SCREEN_BIT
	HWIDTH*8, TOTAL_LINES, { 0, HWIDTH*8-1, 0, VBL_START-1},
#else
	HWIDTH*8, TOTAL_LINES, { BUF_OFFS0*8, HCHARS*8-1, VBL_END, VBL_START - 1},
#endif
    0,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	0,

	VIDEO_TYPE_RASTER,
	0,
	atari_vh_start,
	atari_vh_stop,
	atari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		},
        {
			SOUND_DAC,
			&dac_interface
		}
    }
};

static struct MachineDriver a5200_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6510,
			CPU_APPROX,
            0,
			readmem_5200,writemem_5200,0,0,
			a5200_interrupt, TOTAL_LINES	  /* every scanline */
        }
	},
	/* frames per second, VBL handled by atari_interrupt */
	FRAME_RATE, 0,
    1,
	a5200_init_machine,
	0,	/* stop_machine */

	/* video hardware */
#ifdef SHOW_EVERY_SCREEN_BIT
	HWIDTH*8, TOTAL_LINES, { 0, HWIDTH*8-1, 0, VBL_START-1},
#else
	HWIDTH*8, TOTAL_LINES, { BUF_OFFS0*8, HCHARS*8-1, VBL_END, VBL_START - 1},
#endif
    0,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	0,

	VIDEO_TYPE_RASTER,
	0,
	atari_vh_start,
	atari_vh_stop,
	atari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
    }
};

struct GameDriver a800_driver =
{
	__FILE__,
	0,
	"a800",
	"Atari 800",
	"19??",
	"Atari",
	"Juergen Buchmueller",
	GAME_COMPUTER,
	&a800_machine_driver,
	0,

	0,	/* ROM_LOAD structures */
	a800_load_rom,
	a800_id_rom,
	1,	/* number of ROM slots */
	4,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports_a800,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0,
};

struct GameDriver a800xl_driver =
{
	__FILE__,
	&a800_driver,
	"a800xl",
	"Atari 800-XL",
	"19??",
	"Atari",
	"Juergen Buchmueller",
	GAME_NOT_WORKING | GAME_COMPUTER,
	&a800xl_machine_driver,
	0,

	0,	/* ROM_LOAD structures */
	a800xl_load_rom,
	a800xl_id_rom,
	1,	/* number of ROM slots */
	4,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports_a800xl,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0,
};



struct GameDriver a5200_driver =
{
	__FILE__,
	0,
	"a5200",
	"Atari 5200",
	"19??",
	"Atari",
	"Juergen Buchmueller",
	GAME_NOT_WORKING,
	&a5200_machine_driver,
	0,

	0,	/* ROM_LOAD structures */
    a5200_load_rom,
	a5200_id_rom,
	1,	/* number of ROM slots */
	4,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports_5200,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0,
};


