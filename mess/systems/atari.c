/******************************************************************************
    Atari 400/800

    MESS Driver

	Juergen Buchmueller, June 1998
******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/atari.h"
#include "devices/cartslot.h"
#include "inputx.h"

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

static ADDRESS_MAP_START(a400_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x9fff) AM_RAM									/* optional RAM */
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(atari_pia_r, atari_pia_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(a800_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
    AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(atari_pia_r, atari_pia_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(a800xl_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x4fff) AM_RAM
	AM_RANGE(0x5000, 0x57ff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x5800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
    AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(atari_pia_r, atari_pia_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
ADDRESS_MAP_END


static ADDRESS_MAP_START(a5200_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd400, 0xd5ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xe800, 0xe8ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END



#define JOYSTICK_DELTA			10
#define JOYSTICK_SENSITIVITY	200

INPUT_PORTS_START( a800 )

    PORT_START  /* IN0 console keys & switch settings */
	PORT_CONFNAME(0x80, 0x80, "Cursor keys act as" )
    PORT_CONFSETTING(0x80, "Joystick")
    PORT_CONFSETTING(0x00, "Cursor")
	PORT_CONFNAME(0x40, 0x00, "Television Artifacts" )
	PORT_CONFSETTING(0x00, DEF_STR( Off ))
    PORT_CONFSETTING(0x40, DEF_STR( On ))
	PORT_BIT (0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD | IPF_RESETCPU, "CONS.3: Reset",  KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "CONS.2: Option", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "CONS.1: Select", KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "CONS.0: Start",  KEYCODE_F1, IP_JOY_NONE )

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

	PORT_START /* IN4 keyboard #1 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",       IP_KEY_NONE,       IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Escape", KEYCODE_ESC,       IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",    KEYCODE_1,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"",   KEYCODE_2,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",    KEYCODE_3,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",    KEYCODE_4,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",    KEYCODE_5,         IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^",    KEYCODE_6,         IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 &",    KEYCODE_7,         IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *",    KEYCODE_8,         IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (",    KEYCODE_9,         IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )",    KEYCODE_0,         IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- _",    KEYCODE_MINUS,     IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= +",    KEYCODE_EQUALS,    IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backsp", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab",    KEYCODE_TAB,       IP_JOY_NONE)

	PORT_START /* IN5 keyboard #2 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "q Q",    KEYCODE_Q,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "w W",    KEYCODE_W,         IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "e E",    KEYCODE_E,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "r R",    KEYCODE_R,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "t T",    KEYCODE_T,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "y Y",    KEYCODE_Y,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "u U",    KEYCODE_U,         IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "i I",    KEYCODE_I,         IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "o O",    KEYCODE_O,         IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "p P",    KEYCODE_P,         IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",    KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",    KEYCODE_CLOSEBRACE,IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter",  KEYCODE_ENTER,     IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "a A",    KEYCODE_A,         IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "s S",    KEYCODE_S,         IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "d D",    KEYCODE_D,         IP_JOY_NONE)

	PORT_START /* IN6 keyboard #3 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "f F",    KEYCODE_F,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "g G",    KEYCODE_G,         IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "h H",    KEYCODE_H,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "j J",    KEYCODE_J,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "k K",    KEYCODE_K,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "l L",    KEYCODE_L,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; :",    KEYCODE_COLON,     IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ \\",   KEYCODE_QUOTE,     IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "* ^",    KEYCODE_TILDE,     IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",       IP_KEY_NONE,       IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "z Z",    KEYCODE_Z,         IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x X",    KEYCODE_X,         IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "c C",    KEYCODE_C,         IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "v V",    KEYCODE_V,         IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "b B",    KEYCODE_B,         IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "n N",    KEYCODE_N,         IP_JOY_NONE)

	PORT_START /* IN7 keyboard #4 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "m M",    KEYCODE_M,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", [",    KEYCODE_COMMA,     IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". ]",    KEYCODE_STOP,      IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",    KEYCODE_SLASH,     IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ |",   KEYCODE_BACKSLASH2,IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Atari",  KEYCODE_LALT,      IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space",  KEYCODE_SPACE,     IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Caps",   KEYCODE_CAPSLOCK,  IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Clear",  KEYCODE_HOME,      IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Insert", KEYCODE_INSERT,    IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Delete", KEYCODE_DEL,       IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Break",  KEYCODE_PGUP,      IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Left)", KEYCODE_LEFT,      IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Right)",KEYCODE_RIGHT,     IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Up)",   KEYCODE_UP,        IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Down)", KEYCODE_DOWN,      IP_JOY_NONE)

	PORT_START /* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

    PORT_START /* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER3, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER4, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER5 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER6 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER7 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER8 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)
INPUT_PORTS_END

INPUT_PORTS_START( a800xl )

    PORT_START  /* IN0 console keys & switch settings */
	PORT_CONFNAME(0x80, 0x80, "Cursor keys act as" )
    PORT_CONFSETTING(0x80, "Joystick")
    PORT_CONFSETTING(0x00, "Cursor")
	PORT_CONFNAME(0x40, 0x00, "Television Artifacts" )
	PORT_CONFSETTING(0x00, DEF_STR( Off ))
    PORT_CONFSETTING(0x40, DEF_STR( On ))
	PORT_BIT (0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD | IPF_RESETCPU, "CONS.3: Reset",  KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "CONS.2: Option", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "CONS.1: Select", KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "CONS.0: Start",  KEYCODE_F1, IP_JOY_NONE )

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

	PORT_START /* IN4 keyboard #1 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",       IP_KEY_NONE,       IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Escape", KEYCODE_ESC,       IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",    KEYCODE_1,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"",   KEYCODE_2,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",    KEYCODE_3,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",    KEYCODE_4,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",    KEYCODE_5,         IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^",    KEYCODE_6,         IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 &",    KEYCODE_7,         IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *",    KEYCODE_8,         IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (",    KEYCODE_9,         IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )",    KEYCODE_0,         IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- _",    KEYCODE_MINUS,     IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= +",    KEYCODE_EQUALS,    IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backsp", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab",    KEYCODE_TAB,       IP_JOY_NONE)

	PORT_START /* IN5 keyboard #2 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "q Q",    KEYCODE_Q,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "w W",    KEYCODE_W,         IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "e E",    KEYCODE_E,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "r R",    KEYCODE_R,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "t T",    KEYCODE_T,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "y Y",    KEYCODE_Y,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "u U",    KEYCODE_U,         IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "i I",    KEYCODE_I,         IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "o O",    KEYCODE_O,         IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "p P",    KEYCODE_P,         IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",    KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",    KEYCODE_CLOSEBRACE,IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter",  KEYCODE_ENTER,     IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "a A",    KEYCODE_A,         IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "s S",    KEYCODE_S,         IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "d D",    KEYCODE_D,         IP_JOY_NONE)

	PORT_START /* IN6 keyboard #3 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "f F",    KEYCODE_F,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "g G",    KEYCODE_G,         IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "h H",    KEYCODE_H,         IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "j J",    KEYCODE_J,         IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "k K",    KEYCODE_K,         IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "l L",    KEYCODE_L,         IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; :",    KEYCODE_COLON,     IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+ \\",   KEYCODE_QUOTE,     IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "* ^",    KEYCODE_TILDE,     IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",       IP_KEY_NONE,       IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "z Z",    KEYCODE_Z,         IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x X",    KEYCODE_X,         IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "c C",    KEYCODE_C,         IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "v V",    KEYCODE_V,         IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "b B",    KEYCODE_B,         IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "n N",    KEYCODE_N,         IP_JOY_NONE)

	PORT_START /* IN7 keyboard #4 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "m M",    KEYCODE_M,         IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", [",    KEYCODE_COMMA,     IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". ]",    KEYCODE_STOP,      IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",    KEYCODE_SLASH,     IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ |",   KEYCODE_BACKSLASH2,IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Atari",  KEYCODE_LALT,      IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space",  KEYCODE_SPACE,     IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Caps",   KEYCODE_CAPSLOCK,  IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Clear",  KEYCODE_HOME,      IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Insert", KEYCODE_INSERT,    IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Delete", KEYCODE_DEL,       IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Break",  KEYCODE_PGUP,      IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Left)", KEYCODE_LEFT,      IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Right)",KEYCODE_RIGHT,     IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Up)",   KEYCODE_UP,        IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Down)", KEYCODE_DOWN,      IP_JOY_NONE)

    PORT_START /* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

    PORT_START /* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER3, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER4, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER5 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER6 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER7 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START /* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x74, IPT_PADDLE | IPF_REVERSE /* | IPF_PLAYER8 */, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)
INPUT_PORTS_END

INPUT_PORTS_START( a5200 )

	PORT_START	/* IN0 switch settings */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_CONFNAME(0x40, 0x40, "Television Artifacts" )
    PORT_CONFSETTING(0x00, DEF_STR( Off ))
    PORT_CONFSETTING(0x40, DEF_STR( On ))
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN1 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN2 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN3 lower/upper buttons */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT(0x04, 0x04, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT(0x08, 0x08, IPT_BUTTON1 | IPF_PLAYER4)
	PORT_BIT(0x10, 0x10, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT(0x40, 0x40, IPT_BUTTON2 | IPF_PLAYER3)
    PORT_BIT(0x80, 0x80, IPT_BUTTON2 | IPF_PLAYER4)

	/* KBCODE						   */
	/* Key	   bits    Keypad code	   */
	/* -------------------			   */
	/* none    0000    $FF			   */
	/* #	   0001    $0B			   */
	/* 0	   0010    $00			   */
	/* *	   0011    $0A			   */
	/* Reset   0100    $0E			   */
	/* 9	   0101    $09			   */
	/* 8	   0110    $08			   */
	/* 7	   0111    $07			   */
	/* Pause   1000    $0D			   */
	/* 6	   1001    $06			   */
	/* 5	   1010    $05			   */
	/* 4	   1011    $04			   */
	/* Start   1100    $0C			   */
	/* 3	   1101    $03			   */
	/* 2	   1110    $02			   */
	/* 1	   1111    $01			   */

    PORT_START  /* IN4 keypad */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Break)",KEYCODE_PAUSE,     IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[#]",    KEYCODE_ENTER_PAD, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[0]",    KEYCODE_0_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[*]",    KEYCODE_PLUS_PAD,  IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Reset",  KEYCODE_F3,        IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[9]",    KEYCODE_9_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[8]",    KEYCODE_8_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[7]",    KEYCODE_7_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Pause",  KEYCODE_F2,        IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[6]",    KEYCODE_6_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[5]",    KEYCODE_5_PAD,     IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[4]",    KEYCODE_4_PAD,     IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Start",  KEYCODE_F1,        IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[3]",    KEYCODE_3_PAD,     IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[2]",    KEYCODE_2_PAD,     IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[1]",    KEYCODE_1_PAD,     IP_JOY_NONE)

	PORT_START	/* IN5 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN6 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* IN7 unused */
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

    PORT_START  /* IN8 analog in #1 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN9 analog in #2 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN10 analog in #3 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN11 analog in #4 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN12 analog in #5 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_X | IPF_PLAYER3 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN13 analog in #6 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_Y | IPF_PLAYER3 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN14 analog in #7 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_X | IPF_PLAYER4 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

	PORT_START	/* IN15 analog in #8 */
	PORT_ANALOG(0xff, 0x72, IPT_AD_STICK_Y | IPF_PLAYER4 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x00, 0xe4)

INPUT_PORTS_END


static UINT8 atari_palette[256*3] =
{
	/* Grey */
    0x00,0x00,0x00, 0x1c,0x1c,0x1c, 0x39,0x39,0x39, 0x59,0x59,0x59,
	0x79,0x79,0x79, 0x92,0x92,0x92, 0xab,0xab,0xab, 0xbc,0xbc,0xbc,
	0xcd,0xcd,0xcd, 0xd9,0xd9,0xd9, 0xe6,0xe6,0xe6, 0xec,0xec,0xec,
	0xf2,0xf2,0xf2, 0xf8,0xf8,0xf8, 0xff,0xff,0xff, 0xff,0xff,0xff,
	/* Gold */
    0x39,0x17,0x01, 0x5e,0x23,0x04, 0x83,0x30,0x08, 0xa5,0x47,0x16,
	0xc8,0x5f,0x24, 0xe3,0x78,0x20, 0xff,0x91,0x1d, 0xff,0xab,0x1d,
	0xff,0xc5,0x1d, 0xff,0xce,0x34, 0xff,0xd8,0x4c, 0xff,0xe6,0x51,
	0xff,0xf4,0x56, 0xff,0xf9,0x77, 0xff,0xff,0x98, 0xff,0xff,0x98,
	/* Orange */
    0x45,0x19,0x04, 0x72,0x1e,0x11, 0x9f,0x24,0x1e, 0xb3,0x3a,0x20,
	0xc8,0x51,0x22, 0xe3,0x69,0x20, 0xff,0x81,0x1e, 0xff,0x8c,0x25,
	0xff,0x98,0x2c, 0xff,0xae,0x38, 0xff,0xc5,0x45, 0xff,0xc5,0x59,
	0xff,0xc6,0x6d, 0xff,0xd5,0x87, 0xff,0xe4,0xa1, 0xff,0xe4,0xa1,
	/* Red-Orange */
    0x4a,0x17,0x04, 0x7e,0x1a,0x0d, 0xb2,0x1d,0x17, 0xc8,0x21,0x19,
	0xdf,0x25,0x1c, 0xec,0x3b,0x38, 0xfa,0x52,0x55, 0xfc,0x61,0x61,
	0xff,0x70,0x6e, 0xff,0x7f,0x7e, 0xff,0x8f,0x8f, 0xff,0x9d,0x9e,
	0xff,0xab,0xad, 0xff,0xb9,0xbd, 0xff,0xc7,0xce, 0xff,0xc7,0xce,
	/* Pink */
    0x05,0x05,0x68, 0x3b,0x13,0x6d, 0x71,0x22,0x72, 0x8b,0x2a,0x8c,
	0xa5,0x32,0xa6, 0xb9,0x38,0xba, 0xcd,0x3e,0xcf, 0xdb,0x47,0xdd,
	0xea,0x51,0xeb, 0xf4,0x5f,0xf5, 0xfe,0x6d,0xff, 0xfe,0x7a,0xfd,
	0xff,0x87,0xfb, 0xff,0x95,0xfd, 0xff,0xa4,0xff, 0xff,0xa4,0xff,
	/* Purple */
    0x28,0x04,0x79, 0x40,0x09,0x84, 0x59,0x0f,0x90, 0x70,0x24,0x9d,
	0x88,0x39,0xaa, 0xa4,0x41,0xc3, 0xc0,0x4a,0xdc, 0xd0,0x54,0xed,
	0xe0,0x5e,0xff, 0xe9,0x6d,0xff, 0xf2,0x7c,0xff, 0xf8,0x8a,0xff,
	0xff,0x98,0xff, 0xfe,0xa1,0xff, 0xfe,0xab,0xff, 0xfe,0xab,0xff,
	/* Purple-Blue */
    0x35,0x08,0x8a, 0x42,0x0a,0xad, 0x50,0x0c,0xd0, 0x64,0x28,0xd0,
	0x79,0x45,0xd0, 0x8d,0x4b,0xd4, 0xa2,0x51,0xd9, 0xb0,0x58,0xec,
	0xbe,0x60,0xff, 0xc5,0x6b,0xff, 0xcc,0x77,0xff, 0xd1,0x83,0xff,
	0xd7,0x90,0xff, 0xdb,0x9d,0xff, 0xdf,0xaa,0xff, 0xdf,0xaa,0xff,
	/* Blue 1 */
    0x05,0x1e,0x81, 0x06,0x26,0xa5, 0x08,0x2f,0xca, 0x26,0x3d,0xd4,
	0x44,0x4c,0xde, 0x4f,0x5a,0xee, 0x5a,0x68,0xff, 0x65,0x75,0xff,
	0x71,0x83,0xff, 0x80,0x91,0xff, 0x90,0xa0,0xff, 0x97,0xa9,0xff,
	0x9f,0xb2,0xff, 0xaf,0xbe,0xff, 0xc0,0xcb,0xff, 0xc0,0xcb,0xff,
	/* Blue 2 */
    0x0c,0x04,0x8b, 0x22,0x18,0xa0, 0x38,0x2d,0xb5, 0x48,0x3e,0xc7,
	0x58,0x4f,0xda, 0x61,0x59,0xec, 0x6b,0x64,0xff, 0x7a,0x74,0xff,
	0x8a,0x84,0xff, 0x91,0x8e,0xff, 0x99,0x98,0xff, 0xa5,0xa3,0xff,
	0xb1,0xae,0xff, 0xb8,0xb8,0xff, 0xc0,0xc2,0xff, 0xc0,0xc2,0xff,
	/* Light-Blue */
    0x1d,0x29,0x5a, 0x1d,0x38,0x76, 0x1d,0x48,0x92, 0x1c,0x5c,0xac,
	0x1c,0x71,0xc6, 0x32,0x86,0xcf, 0x48,0x9b,0xd9, 0x4e,0xa8,0xec,
	0x55,0xb6,0xff, 0x70,0xc7,0xff, 0x8c,0xd8,0xff, 0x93,0xdb,0xff,
	0x9b,0xdf,0xff, 0xaf,0xe4,0xff, 0xc3,0xe9,0xff, 0xc3,0xe9,0xff,
	/* Turquoise */
    0x2f,0x43,0x02, 0x39,0x52,0x02, 0x44,0x61,0x03, 0x41,0x7a,0x12,
	0x3e,0x94,0x21, 0x4a,0x9f,0x2e, 0x57,0xab,0x3b, 0x5c,0xbd,0x55,
	0x61,0xd0,0x70, 0x69,0xe2,0x7a, 0x72,0xf5,0x84, 0x7c,0xfa,0x8d,
	0x87,0xff,0x97, 0x9a,0xff,0xa6, 0xad,0xff,0xb6, 0xad,0xff,0xb6,
	/* Green-Blue */
    0x0a,0x41,0x08, 0x0d,0x54,0x0a, 0x10,0x68,0x0d, 0x13,0x7d,0x0f,
    0x16,0x92,0x12, 0x19,0xa5,0x14, 0x1c,0xb9,0x17, 0x1e,0xc9,0x19,
	0x21,0xd9,0x1b, 0x47,0xe4,0x2d, 0x6e,0xf0,0x40, 0x78,0xf7,0x4d,
	0x83,0xff,0x5b, 0x9a,0xff,0x7a, 0xb2,0xff,0x9a, 0xb2,0xff,0x9a,
	/* Green */
    0x04,0x41,0x0b, 0x05,0x53,0x0e, 0x06,0x66,0x11, 0x07,0x77,0x14,
	0x08,0x88,0x17, 0x09,0x9b,0x1a, 0x0b,0xaf,0x1d, 0x48,0xc4,0x1f,
	0x86,0xd9,0x22, 0x8f,0xe9,0x24, 0x99,0xf9,0x27, 0xa8,0xfc,0x41,
	0xb7,0xff,0x5b, 0xc9,0xff,0x6e, 0xdc,0xff,0x81, 0xdc,0xff,0x81,
	/* Yellow-Green */
    0x02,0x35,0x0f, 0x07,0x3f,0x15, 0x0c,0x4a,0x1c, 0x2d,0x5f,0x1e,
	0x4f,0x74,0x20, 0x59,0x83,0x24, 0x64,0x92,0x28, 0x82,0xa1,0x2e,
	0xa1,0xb0,0x34, 0xa9,0xc1,0x3a, 0xb2,0xd2,0x41, 0xc4,0xd9,0x45,
	0xd6,0xe1,0x49, 0xe4,0xf0,0x4e, 0xf2,0xff,0x53, 0xf2,0xff,0x53,
	/* Orange-Green */
    0x26,0x30,0x01, 0x24,0x38,0x03, 0x23,0x40,0x05, 0x51,0x54,0x1b,
	0x80,0x69,0x31, 0x97,0x81,0x35, 0xaf,0x99,0x3a, 0xc2,0xa7,0x3e,
	0xd5,0xb5,0x43, 0xdb,0xc0,0x3d, 0xe1,0xcb,0x38, 0xe2,0xd8,0x36,
	0xe3,0xe5,0x34, 0xef,0xf2,0x58, 0xfb,0xff,0x7d, 0xfb,0xff,0x7d,
	/* Light-Orange */
    0x40,0x1a,0x02, 0x58,0x1f,0x05, 0x70,0x24,0x08, 0x8d,0x3a,0x13,
	0xab,0x51,0x1f, 0xb5,0x64,0x27, 0xbf,0x77,0x30, 0xd0,0x85,0x3a,
	0xe1,0x93,0x44, 0xed,0xa0,0x4e, 0xf9,0xad,0x58, 0xfc,0xb7,0x5c,
	0xff,0xc1,0x60, 0xff,0xc6,0x71, 0xff,0xcb,0x83, 0xff,0xcb,0x83
};

static unsigned short atari_colortable[] =
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

/* Initialise the palette */
static PALETTE_INIT( atari )
{
	palette_set_colors(0, atari_palette, sizeof(atari_palette) / 3);
	memcpy(colortable,atari_colortable,sizeof(atari_colortable));
}


static struct POKEYinterface pokey_interface = {
	1,
	FREQ_17_EXACT,
    { 100 },
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


static MACHINE_DRIVER_START( atari_common_nodac )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6510, FREQ_17_EXACT)
	MDRV_VBLANK_DURATION(1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_VISIBLE_AREA(MIN_X, MAX_X, MIN_Y, MAX_Y)
	MDRV_PALETTE_LENGTH(sizeof(atari_palette) / sizeof(atari_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(atari_colortable) / sizeof(atari_colortable[0]))
	MDRV_PALETTE_INIT(atari)

	MDRV_VIDEO_START(atari)
	MDRV_VIDEO_UPDATE(atari)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atari_common )
	MDRV_IMPORT_FROM( atari_common_nodac )
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a400 )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a400_mem, 0)
	MDRV_CPU_VBLANK_INT(a400_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_INIT( a400 )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a400pal )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a400_mem, 0)
	MDRV_CPU_VBLANK_INT(a400_interrupt, TOTAL_LINES_50HZ)

	MDRV_MACHINE_INIT( a400 )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_50HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_50HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800 )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800_mem, 0)
	MDRV_CPU_VBLANK_INT(a800_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_INIT( a800 )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800pal )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800_mem, 0)
	MDRV_CPU_VBLANK_INT(a800_interrupt, TOTAL_LINES_50HZ)

	MDRV_MACHINE_INIT( a800 )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_50HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_50HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800xl )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800xl_mem, 0)
	MDRV_CPU_VBLANK_INT(a800xl_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_INIT( a800xl )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a5200 )
	MDRV_IMPORT_FROM( atari_common_nodac )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a5200_mem, 0)
	MDRV_CPU_VBLANK_INT(a5200_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_INIT( a5200 )
	MDRV_FRAMES_PER_SECOND(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


ROM_START(a400)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari400.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a400pal)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari400.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari800.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800pal)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari800.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800xl)
	ROM_REGION(0x18000,REGION_CPU1,0) /* 64K for the CPU + 16K + 2 * 8K for cartridges */
	ROM_LOAD("basic.rom",   0x10000, 0x2000, CRC(7d684184) SHA1(3693c9cb9bf3b41bae1150f7a8264992468fc8c0))
	ROM_LOAD("atarixl.rom", 0x14000, 0x4000, CRC(1f9cd270) SHA1(ae4f523ba08b6fd59f3cae515a2b2410bbd98f55))
ROM_END

ROM_START(a5200)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 16K for cartridges */
	ROM_LOAD("5200.rom", 0xf800, 0x0800, CRC(4248d3e3) SHA1(6ad7a1e8c9fad486fbec9498cb48bf5bc3adc530))
ROM_END

SYSTEM_CONFIG_START(atari)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 4, "atr\0dsk\0xfd\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE_OR_READ,
		NULL, NULL, device_load_a800_floppy, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(a400)
	CONFIG_IMPORT_FROM(atari)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "rom\0bin\0", NULL, NULL, device_load_a800_cart, device_unload_a800_cart, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(a800)
	CONFIG_IMPORT_FROM(atari)
	CONFIG_DEVICE_CARTSLOT_OPT(2, "rom\0bin\0", NULL, NULL, device_load_a800_cart, device_unload_a800_cart, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(a5200)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "rom\0bin\0a52\0", NULL, NULL, device_load_a5200_cart, device_unload_a5200_cart, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT    COMPAT	MACHINE		INPUT    INIT	CONFIG	COMPANY   FULLNAME */
COMP ( 1979, a400,	   0,		 0,		a400,		a800,	 0, 	a400,	"Atari",  "Atari 400 (NTSC)" )
COMP ( 1979, a400pal,  a400,	 0,		a400pal,	a800,	 0, 	a400,	"Atari",  "Atari 400 (PAL)" )
COMP ( 1979, a800,	   0,		 0,		a800,		a800,	 0, 	a800,	"Atari",  "Atari 800 (NTSC)" )
COMP ( 1979, a800pal,  a800,	 0,		a800pal,	a800,	 0,		a800,	"Atari",  "Atari 800 (PAL)" )
COMPX( 1983, a800xl,   a800,	 0,		a800xl,		a800xl,	 0, 	a800,	"Atari",  "Atari 800XL", GAME_NOT_WORKING )
CONS ( 1982, a5200,    0,		 0,		a5200,		a5200,	 0, 	a5200,	"Atari",  "Atari 5200")
