/***************************************************************************
TRS80 memory map

CPU #1:
0000-2fff ROM                     R   D0-D7
3000-37ff ROM on Model III        R   D0-D7
          unused on Model I
37e0-37e3 floppy motor            W   D0-D3
          or floppy head select   W   D3
37e4-37eb printer / RS232??       R/W D0-D7
37ec-37ef FDC WD179x              R/W D0-D7
37ec      command                 W   D0-D7
37ec      status                  R   D0-D7
37ed      track                   R/W D0-D7
37ee      sector                  R/W D0-D7
37ef      data                    R/W D0-D7
3800-38ff keyboard matrix         R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM               R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "drivers/trs80.h"

#define FW  TRS80_FONT_W
#define FH  TRS80_FONT_H

extern int  trs80_rom_load(void);
extern int  trs80_rom_id(const char *name, const char * gamename);

extern int  trs80_vh_start(void);
extern void trs80_vh_stop(void);
extern void trs80_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void trs80_sh_sound_init(const char * gamename);

extern void trs80_init_machine(void);

extern void trs80_port_ff_w(int offset, int data);
extern int  trs80_port_ff_r(int offset);
extern int  trs80_port_xx_r(int offset);

extern int  trs80_timer_interrupt(void);
extern int  trs80_frame_interrupt(void);

extern int  trs80_irq_status_r(int offset);
extern void trs80_irq_mask_w(int offset, int data);

extern int  trs80_printer_r(int offset);
extern void trs80_printer_w(int offset, int data);

extern int  trs80_status_r(int offset);
extern int  trs80_track_r(int offset);
extern int  trs80_sector_r(int offset);
extern int  trs80_data_r(int offset);

extern void trs80_command_w(int offset, int data);
extern void trs80_track_w(int offset, int data);
extern void trs80_sector_w(int offset, int data);
extern void trs80_data_w(int offset, int data);

extern void trs80_motor_w(int offset, int data);

extern int  trs80_keyboard_r(int offset);
extern int  trs80_videoram_r(int offset);
extern void trs80_videoram_w(int offset, int data);

static struct MemoryReadAddress readmem_model1[] =
{
        { 0x0000, 0x2fff, MRA_ROM },
        { 0x3000, 0x37df, MRA_NOP },
        { 0x37e0, 0x37e3, trs80_irq_status_r },
        { 0x30e4, 0x37e7, MRA_NOP },
        { 0x37e8, 0x37eb, trs80_printer_r },
        { 0x37ec, 0x37ec, trs80_status_r },
        { 0x37ed, 0x37ed, trs80_track_r },
        { 0x37ee, 0x37ee, trs80_sector_r },
        { 0x37ef, 0x37ef, trs80_data_r },
        { 0x37f0, 0x37ff, MRA_NOP },
        { 0x3800, 0x38ff, trs80_keyboard_r },
        { 0x3900, 0x3bff, MRA_NOP },
        { 0x3c00, 0x3fff, trs80_videoram_r, &videoram },
        { 0x4000, 0xffff, MRA_RAM },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_model1[] =
{
        { 0x0000, 0x2fff, MWA_ROM },
        { 0x3000, 0x37df, MWA_NOP },
        { 0x37e0, 0x37e3, trs80_motor_w },
        { 0x37e4, 0x37e7, MWA_NOP },
        { 0x37e8, 0x37eb, trs80_printer_w },
        { 0x37ec, 0x37ec, trs80_command_w },
        { 0x37ed, 0x37ed, trs80_track_w },
        { 0x37ee, 0x37ee, trs80_sector_w },
        { 0x37ef, 0x37ef, trs80_data_w },
        { 0x37f0, 0x37ff, MWA_NOP },
        { 0x3800, 0x3bff, MWA_NOP },
        { 0x3c00, 0x3fff, trs80_videoram_w, &videoram },
        { 0x4000, 0xffff, MWA_RAM },
        { -1 }  /* end of table */
};

static struct IOReadPort readport_model1[] =
{
        { 0x00, 0xfe, trs80_port_xx_r },
        { 0xff, 0xff, trs80_port_ff_r },
        { -1 }
};

static struct IOWritePort writeport_model1[] =
{
        { 0x00, 0xfe, IOWP_NOP },
        { 0xff, 0xff, trs80_port_ff_w },
        { -1 }
};

static struct MemoryReadAddress readmem_model3[] =
{
        { 0x0000, 0x37ff, MRA_ROM },
        { 0x3800, 0x38ff, trs80_keyboard_r },
        { 0x3c00, 0x3fff, trs80_videoram_r, &videoram },
        { 0x4000, 0xffff, MRA_RAM },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_model3[] =
{
        { 0x0000, 0x37ff, MWA_ROM },
        { 0x3800, 0x38ff, MWA_NOP },
        { 0x3c00, 0x3fff, trs80_videoram_w, &videoram },
        { 0x4000, 0xffff, MWA_RAM },
        { -1 }  /* end of table */
};

static struct IOReadPort readport_model3[] =
{
        { 0xe0, 0xe3, trs80_irq_status_r },
        { 0xf0, 0xf0, trs80_status_r },
        { 0xf1, 0xf1, trs80_track_r },
        { 0xf2, 0xf2, trs80_sector_r },
        { 0xf3, 0xf3, trs80_data_r },
        { 0xff, 0xff, trs80_port_ff_r },
        { -1 }
};

static struct IOWritePort writeport_model3[] =
{
        { 0xe0, 0xe3, trs80_irq_mask_w },
        { 0xf0, 0xf0, trs80_command_w },
        { 0xf1, 0xf1, trs80_track_w },
        { 0xf2, 0xf2, trs80_sector_w },
        { 0xf3, 0xf3, trs80_data_w },
        { 0xf4, 0xf4, trs80_motor_w },
        { 0xff, 0xff, trs80_port_ff_w },
        { -1 }
};

/**************************************************************************
   w/o SHIFT                             with SHIFT
   +-------------------------------+     +-------------------------------+
   | 0   1   2   3   4   5   6   7 |     | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |   |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
NB: row 7 contains some originally unused bits
    only the shift bit was there in the TRS80
***************************************************************************/

INPUT_PORTS_START( trs80_input_ports )
        PORT_START /* IN0 */
        PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy Disc Drives",   IP_KEY_NONE, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(    0x80, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Video RAM",            IP_KEY_NONE, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(    0x40, "7 bit" )
        PORT_DIPSETTING(    0x00, "8 bit" )
        PORT_BITX(    0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Virtual Tape",         OSD_KEY_F2,  IP_JOY_NONE, 0 )
        PORT_DIPSETTING(    0x20, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Scanlines",            OSD_KEY_F3,  IP_JOY_NONE, 0 )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_BITX(    0x07, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Colors",               OSD_KEY_F4,  IP_JOY_NONE, 0 )
        PORT_DIPSETTING(    0x00, "green/black" )
        PORT_DIPSETTING(    0x01, "black/green" )
        PORT_DIPSETTING(    0x02, "white/black" )
        PORT_DIPSETTING(    0x03, "black/white" )
        PORT_DIPSETTING(    0x04, "yellow/blue" )
        PORT_DIPSETTING(    0x05, "white/blue"  )
        PORT_DIPSETTING(    0x06, "white/gray"  )
        PORT_DIPSETTING(    0x07, "gray/white"  )

        PORT_START /* KEY ROW 0 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "0.0: @   ",      OSD_KEY_ASTERISK,    IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "0.1: A  a",      OSD_KEY_A,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "0.2: B  b",      OSD_KEY_B,           IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "0.3: C  c",      OSD_KEY_C,           IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "0.4: D  d",      OSD_KEY_D,           IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "0.5: E  e",      OSD_KEY_E,           IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "0.6: F  f",      OSD_KEY_F,           IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "0.7: G  g",      OSD_KEY_G,           IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 1 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "1.0: H  h",      OSD_KEY_H,           IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "1.1: I  i",      OSD_KEY_I,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "1.2: J  j",      OSD_KEY_J,           IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "1.3: K  k",      OSD_KEY_K,           IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "1.4: L  l",      OSD_KEY_L,           IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "1.5: M  m",      OSD_KEY_M,           IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "1.6: N  n",      OSD_KEY_N,           IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "1.7: O  o",      OSD_KEY_O,           IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 2 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "2.0: P  p",      OSD_KEY_P,           IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "2.1: Q  q",      OSD_KEY_Q,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "2.2: R  r",      OSD_KEY_R,           IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "2.3: S  s",      OSD_KEY_S,           IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "2.4: T  t",      OSD_KEY_T,           IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "2.5: U  u",      OSD_KEY_U,           IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "2.6: V  v",      OSD_KEY_V,           IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "2.7: W  w",      OSD_KEY_W,           IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 3 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "3.0: X  x",      OSD_KEY_X,           IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "3.1: Y  y",      OSD_KEY_Y,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "3.2: Z  z",      OSD_KEY_Z,           IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "3.3: [  {",      OSD_KEY_OPENBRACE,   IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "3.4: \\  |",     OSD_KEY_F1,          IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "3.5: ]  }",      OSD_KEY_CLOSEBRACE,  IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "3.6: ^  ~",      OSD_KEY_TILDE,       IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "3.7: _   ",      OSD_KEY_EQUALS,      IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 4 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "4.0: 0   ",      OSD_KEY_0,           IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "4.1: 1  !",      OSD_KEY_1,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "4.2: 2  \"",     OSD_KEY_2,           IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "4.3: 3  #",      OSD_KEY_3,           IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "4.4: 4  $",      OSD_KEY_4,           IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "4.5: 5  %",      OSD_KEY_5,           IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "4.6: 6  &",      OSD_KEY_6,           IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "4.7: 7  '",      OSD_KEY_7,           IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 5 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "5.0: 8  (",      OSD_KEY_8,           IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "5.1: 9  )",      OSD_KEY_9,           IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "5.2: :  *",      OSD_KEY_COLON,       IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "5.3: ;  +",      OSD_KEY_QUOTE,       IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "5.4: ,  <",      OSD_KEY_COMMA,       IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "5.5: -  =",      OSD_KEY_MINUS,       IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "5.6: .  >",      OSD_KEY_STOP,        IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "5.7: /  ?",      OSD_KEY_SLASH,       IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 6 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "6.0: ENTER",  OSD_KEY_ENTER,       IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "6.1: CLEAR",  OSD_KEY_HOME,        IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "6.2: BREAK",  OSD_KEY_END,         IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "6.3: UP",     OSD_KEY_UP,          IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "6.4: DOWN",   OSD_KEY_DOWN,        IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "6.5: LEFT",   OSD_KEY_LEFT,        IP_JOY_NONE, 0)
        /* backspace should do the same as cursor left */
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "6.5: (BSP)",  OSD_KEY_BACKSPACE,   IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "6.6: RIGHT",  OSD_KEY_RIGHT,       IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "6.7: SPACE",  OSD_KEY_SPACE,       IP_JOY_NONE, 0)

        PORT_START /* KEY ROW 7 */
        PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "7.0: SHIFT",  OSD_KEY_LSHIFT,      IP_JOY_NONE, 0)
        PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "7.1: (ALT)",  OSD_KEY_ALT,         IP_JOY_NONE, 0)
        PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "7.2: (PGUP)", OSD_KEY_PGUP,        IP_JOY_NONE, 0)
        PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "7.3: (PGDN)", OSD_KEY_PGDN,        IP_JOY_NONE, 0)
        PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "7.4: (INS)",  OSD_KEY_INSERT,      IP_JOY_NONE, 0)
        PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "7.5: (DEL)",  OSD_KEY_DEL,         IP_JOY_NONE, 0)
        PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "7.6: (CTRL)", OSD_KEY_LCONTROL,    IP_JOY_NONE, 0)
        PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "7.7: (ALTGR)",OSD_KEY_ALTGR,       IP_JOY_NONE, 0)

INPUT_PORTS_END

/* The character layout of the TRS-80 Model 1 font is 6x12 pixels
   It though had a 4x3 aspect ratio, but with 64x6 / 16x12 we would have 2x1.
   So we decided to scale the font up x2 horizontal and x3 vertical.
   This leads to 12x36 characters and since MAME (and therefore MESS)
   supports a maximum of 32x32 pixels per graphics element, we needed to
   split up the characters :( Now there is a upper and lower half with
   12x18 pixels for each character.
   The double width characters are now 24x36 and also split into upper/lower.
*/

static struct GfxLayout trs80_charlayout_normal_width_upper =
{
        FW*2,FH*3/2,            /* FW*2 x FH*3 characters, upper half */
        256,                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets double width: reuse each bit once */
        { 0, 0,
          1, 1,
          2, 2,
          3, 3,
          4, 4,
          5, 5,
          6, 6,
          7, 7},
        /* y offsets triple height: use each line three times */
        {  0*8,  0*8,  0*8,
           1*8,  1*8,  1*8,
           2*8,  2*8,  2*8,
           3*8,  3*8,  3*8,
           4*8,  4*8,  4*8,
           5*8,  5*8,  5*8,
           6*8,  6*8,  6*8,
           7*8,  7*8,  7*8,
           8*8,  8*8,  8*8,
           9*8,  9*8,  9*8},
        8*FH           /* every char takes FH bytes */
};

static struct GfxLayout trs80_charlayout_normal_width_lower =
{
        FW*2,FH*3/2,            /* FW*2 x FH*3 characters, upper half */
        256,                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets double width: reuse each bit once */
        { 0, 0,
          1, 1,
          2, 2,
          3, 3,
          4, 4,
          5, 5,
          6, 6,
          7, 7},
        /* y offsets triple height: use each line three times */
        {  (FH/2+0)*8,  (FH/2+0)*8,  (FH/2+0)*8,
           (FH/2+1)*8,  (FH/2+1)*8,  (FH/2+1)*8,
           (FH/2+2)*8,  (FH/2+2)*8,  (FH/2+2)*8,
           (FH/2+3)*8,  (FH/2+3)*8,  (FH/2+3)*8,
           (FH/2+4)*8,  (FH/2+4)*8,  (FH/2+4)*8,
           (FH/2+5)*8,  (FH/2+5)*8,  (FH/2+5)*8,
           (FH/2+6)*8,  (FH/2+6)*8,  (FH/2+6)*8,
           (FH/2+7)*8,  (FH/2+7)*8,  (FH/2+7)*8,
           (FH/2+8)*8,  (FH/2+8)*8,  (FH/2+8)*8,
           (FH/2+9)*8,  (FH/2+9)*8,  (FH/2+9)*8},
        8*FH           /* every char takes FH bytes */
};

static struct GfxLayout trs80_charlayout_double_width_upper =
{
        FW*4,FH*3/2,   /* FW*2 x FH*3 characters, upper half */
        256,           /* 256 characters */
        1,             /* 1 bits per pixel */
        { 0 },         /* no bitplanes; 1 bit per pixel */
        /* x offsets quad width: use each bit four times */
        { 0, 0, 0, 0,  
          1, 1, 1, 1,
          2, 2, 2, 2,
          3, 3, 3, 3,
          4, 4, 4, 4,
          5, 5, 5, 5,
          6, 6, 6, 6,
          7, 7, 7, 7},
        /* y offsets triple height: use each line three times */
        {  0*8,  0*8,  0*8,
           1*8,  1*8,  1*8,
           2*8,  2*8,  2*8,
           3*8,  3*8,  3*8,
           4*8,  4*8,  4*8,
           5*8,  5*8,  5*8,
           6*8,  6*8,  6*8,
           7*8,  7*8,  7*8,
           8*8,  8*8,  8*8,
           9*8,  9*8,  9*8},
        8*FH           /* every char takes FH bytes */
};

static struct GfxLayout trs80_charlayout_double_width_lower =
{
        FW*4,FH*3/2,   /* FW*2 x FH*3 characters, upper half */
        256,           /* 256 characters */
        1,             /* 1 bits per pixel */
        { 0 },         /* no bitplanes; 1 bit per pixel */
        /* x offsets quad width: reuse each bit three times */
        { 0, 0, 0, 0,  
          1, 1, 1, 1,
          2, 2, 2, 2,
          3, 3, 3, 3,
          4, 4, 4, 4,
          5, 5, 5, 5,
          6, 6, 6, 6,
          7, 7, 7, 7},
        /* y offsets triple height: use each line three times */
        {  (FH/2+0)*8,  (FH/2+0)*8,  (FH/2+0)*8,
           (FH/2+1)*8,  (FH/2+1)*8,  (FH/2+1)*8,
           (FH/2+2)*8,  (FH/2+2)*8,  (FH/2+2)*8,
           (FH/2+3)*8,  (FH/2+3)*8,  (FH/2+3)*8,
           (FH/2+4)*8,  (FH/2+4)*8,  (FH/2+4)*8,
           (FH/2+5)*8,  (FH/2+5)*8,  (FH/2+5)*8,
           (FH/2+6)*8,  (FH/2+6)*8,  (FH/2+6)*8,
           (FH/2+7)*8,  (FH/2+7)*8,  (FH/2+7)*8,
           (FH/2+8)*8,  (FH/2+8)*8,  (FH/2+8)*8,
           (FH/2+9)*8,  (FH/2+9)*8,  (FH/2+9)*8},
        8*FH           /* every char takes FH bytes */
};

static struct GfxDecodeInfo trs80_gfxdecodeinfo[] =
{
        { 1, 0, &trs80_charlayout_normal_width_upper, 0, 8 },
        { 1, 0, &trs80_charlayout_normal_width_lower, 0, 8 },
        { 1, 0, &trs80_charlayout_double_width_upper, 0, 8 },
        { 1, 0, &trs80_charlayout_double_width_lower, 0, 8 },
        { -1 } /* end of array */
};

/* some colors for the DOS frontend and selectable fore-/background */
static unsigned char trs80_palette[9*3] = {
          0,  0,  0,
        255,  0,  0,
          0,255,  0,
        255,255,  0,
          0,  0,255,
        255,  0,255,
          0,255,255,
        255,255,255,
         63, 63, 63,
};

static short trs80_colortable[8*2] = {
        0,2,    /* green on black */
        2,0,    /* black on green */
        0,7,    /* white on black */
        7,0,    /* black on white */
        4,3,    /* yellow on blue */
        4,7,    /* white on blue */
        8,7,    /* white on gray */
        7,8,    /* gray on white */
};

static struct DACinterface trs80_DAC_interface =
{
    1,          /* number of DACs */
    22450,      /* DAC sampling rate: CPU clock / 80 */
    {255, },    /* volume */
    {0, }       /* filter rate */
};

static struct MachineDriver machine_driver_model1 =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        1796000,        /* 1.796 Mhz */
                        0,
                        readmem_model1,writemem_model1,
                        readport_model1,writeport_model1,
                        trs80_frame_interrupt,1,
                        trs80_timer_interrupt,40
                },
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        trs80_init_machine,

        /* video hardware */
        64*FW*2,                                /* screen width */
        16*FH*3,                                /* screen height */
        { 0*FW*2,64*FW*2-1,0*FH*3,16*FH*3-1},   /* visible_area */
        trs80_gfxdecodeinfo,                    /* graphics decode info */
        9, 16,                                  /* colors used for the characters */
        0,                                      /* convert color prom */

        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
        0,
        trs80_vh_start,
        trs80_vh_stop,
        trs80_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_DAC,
                        &trs80_DAC_interface
                }
        }
};

static struct MachineDriver machine_driver_model3 =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        1796000,        /* 1.796 Mhz */
                        0,
                        readmem_model3,writemem_model3,
                        readport_model3,writeport_model3,
                        trs80_frame_interrupt,2,
                        trs80_timer_interrupt,40
                },
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        trs80_init_machine,

        /* video hardware */
        64*FW*2,                                /* screen width */
        16*FH*3,                                /* screen height */
        { 0*FW*2,64*FW*2-1,0*FH*3,16*FH*3-1},   /* visible_area */
        trs80_gfxdecodeinfo,                    /* graphics decode info */
        9, 16,                                  /* colors used for the characters */
        0,                                      /* convert color prom */

        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
        0,
        trs80_vh_start,
        trs80_vh_stop,
        trs80_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_DAC,
                        &trs80_DAC_interface
                }
        }
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver trs80_driver =
{
        "Tandy TRS-80 Model I",
        "trs80",
        "Juergen Buchmueller [MESS driver]",
        &machine_driver_model1,

        trs80_rom_load,         /* load rom_file images */
        trs80_rom_id,           /* identify rom images */
	1,                      /* number of ROM slots - in this case, a CMD binary */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
        0,                      /* rom decoder */
        0,                      /* opcode decoder */
        0,                      /* pointer to sample names */
        0,                      /* sound_prom */

        trs80_input_ports,

        0,                      /* color_prom */
        trs80_palette,          /* color palette */
        trs80_colortable,       /* color lookup table */

        ORIENTATION_DEFAULT,    /* orientation */

        0,                      /* hiscore load */
        0                       /* hiscore save */
};

struct GameDriver trs80_m3_driver =
{
        "Tandy TRS-80 Model III",
        "trs80_m3",
        "Juergen Buchmueller [MESS driver]",
        &machine_driver_model3,

        trs80_rom_load,         /* load rom_file images */
        trs80_rom_id,           /* identify rom images */
	1,                      /* number of ROM slots - in this case, a CMD binary */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
        0,                      /* rom decoder */
        0,                      /* opcode decoder */
        0,                      /* pointer to sample names */
        0,                      /* sound_prom */

        trs80_input_ports,

        0,                      /* color_prom */
        trs80_palette,          /* color palette */
        trs80_colortable,       /* color lookup table */

        ORIENTATION_DEFAULT,    /* orientation */

        0,                      /* hiscore load */
        0                       /* hiscore save */
};

