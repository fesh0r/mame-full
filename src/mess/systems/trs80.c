/***************************************************************************
TRS80 memory map

0000-2fff ROM					  R   D0-D7
3000-37ff ROM on Model III		  R   D0-D7
		  unused on Model I
37e0-37e3 floppy motor			  W   D0-D3
		  or floppy head select   W   D3
37e4-37eb printer / RS232?? 	  R/W D0-D7
37ec-37ef FDC WD179x			  R/W D0-D7
37ec	  command				  W   D0-D7
37ec	  status				  R   D0-D7
37ed	  track 				  R/W D0-D7
37ee	  sector				  R/W D0-D7
37ef	  data					  R/W D0-D7
3800-38ff keyboard matrix		  R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM 			  R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "mess/systems/trs80.h"

#define FW	TRS80_FONT_W
#define FH	TRS80_FONT_H

extern int trs80_cassette_init(int id);
extern int trs80_floppy_init(int id);
extern void trs80_floppy_exit(int id);
extern int trs80_rom_load(int id);
extern int trs80_rom_id(int id);

extern int trs80_vh_start(void);
extern void trs80_vh_stop(void);
extern void trs80_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern void trs80_sh_sound_init(const char * gamename);

extern void init_trs80(void);
extern void trs80_init_machine(void);
extern void trs80_shutdown_machine(void);

extern void trs80_port_ff_w(int offset, int data);
extern int trs80_port_ff_r(int offset);
extern int trs80_port_xx_r(int offset);

extern int trs80_timer_interrupt(void);
extern int trs80_frame_interrupt(void);

extern int trs80_irq_status_r(int offset);
extern void trs80_irq_mask_w(int offset, int data);

extern int trs80_printer_r(int offset);
extern void trs80_printer_w(int offset, int data);

extern int trs80_status_r(int offset);
extern int trs80_track_r(int offset);
extern int trs80_sector_r(int offset);
extern int trs80_data_r(int offset);

extern void trs80_command_w(int offset, int data);
extern void trs80_track_w(int offset, int data);
extern void trs80_sector_w(int offset, int data);
extern void trs80_data_w(int offset, int data);

extern void trs80_motor_w(int offset, int data);

extern int trs80_keyboard_r(int offset);
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
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
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
	{ 0x3c00, 0x3fff, trs80_videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
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
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_model3[] =
{
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x3800, 0x38ff, MWA_NOP },
	{ 0x3c00, 0x3fff, trs80_videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
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
   w/o SHIFT							 with SHIFT
   +-------------------------------+	 +-------------------------------+
   | 0	 1	 2	 3	 4	 5	 6	 7 |	 | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |	 |
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

INPUT_PORTS_START( trs80 )
	PORT_START /* IN0 */
	PORT_BITX(	  0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE,	"Floppy Disc Drives",   IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_BITX(	  0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE,	"Video RAM",            KEYCODE_F1,  IP_JOY_NONE )
	PORT_DIPSETTING(	0x40, "7 bit" )
	PORT_DIPSETTING(	0x00, "8 bit" )
	PORT_BITX(	  0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE,	"Virtual Tape",         KEYCODE_F2,  IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_BITX(	  0x10, 0x00, IPT_KEYBOARD | IPF_RESETCPU,		"Reset",                KEYCODE_F3,  IP_JOY_NONE )
	PORT_BITX(	  0x08, 0x00, IPT_KEYBOARD, 					"NMI",                  KEYCODE_F4,  IP_JOY_NONE )
	PORT_BITX(	  0x04, 0x00, IPT_KEYBOARD, 					"Tape start",           KEYCODE_F5,  IP_JOY_NONE )
	PORT_BITX(	  0x02, 0x00, IPT_KEYBOARD, 					"Tape stop",            KEYCODE_F6,  IP_JOY_NONE )
	PORT_BITX(	  0x01, 0x00, IPT_KEYBOARD, 					"Tape rewind",          KEYCODE_F7,  IP_JOY_NONE )

	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "0.0: @   ",   KEYCODE_ASTERISK,    IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "0.1: A  a",   KEYCODE_A,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "0.2: B  b",   KEYCODE_B,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "0.3: C  c",   KEYCODE_C,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "0.4: D  d",   KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "0.5: E  e",   KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "0.6: F  f",   KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "0.7: G  g",   KEYCODE_G,           IP_JOY_NONE )

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "1.0: H  h",   KEYCODE_H,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "1.1: I  i",   KEYCODE_I,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "1.2: J  j",   KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "1.3: K  k",   KEYCODE_K,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "1.4: L  l",   KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "1.5: M  m",   KEYCODE_M,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "1.6: N  n",   KEYCODE_N,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "1.7: O  o",   KEYCODE_O,           IP_JOY_NONE )

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "2.0: P  p",   KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "2.1: Q  q",   KEYCODE_Q,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "2.2: R  r",   KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "2.3: S  s",   KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "2.4: T  t",   KEYCODE_T,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "2.5: U  u",   KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "2.6: V  v",   KEYCODE_V,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "2.7: W  w",   KEYCODE_W,           IP_JOY_NONE )

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "3.0: X  x",   KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "3.1: Y  y",   KEYCODE_Y,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "3.2: Z  z",   KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "3.3: [  {",   KEYCODE_OPENBRACE,   IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "3.4: \\  |",  KEYCODE_F1,          IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "3.5: ]  }",   KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "3.6: ^  ~",   KEYCODE_TILDE,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "3.7: _   ",   KEYCODE_EQUALS,      IP_JOY_NONE )

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "4.0: 0   ",   KEYCODE_0,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "4.1: 1  !",   KEYCODE_1,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "4.2: 2  \"",  KEYCODE_2,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "4.3: 3  #",   KEYCODE_3,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "4.4: 4  $",   KEYCODE_4,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "4.5: 5  %",   KEYCODE_5,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "4.6: 6  &",   KEYCODE_6,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "4.7: 7  '",   KEYCODE_7,           IP_JOY_NONE )

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "5.0: 8  (",   KEYCODE_8,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "5.1: 9  )",   KEYCODE_9,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "5.2: :  *",   KEYCODE_COLON,       IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "5.3: ;  +",   KEYCODE_QUOTE,       IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "5.4: ,  <",   KEYCODE_COMMA,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "5.5: -  =",   KEYCODE_MINUS,       IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "5.6: .  >",   KEYCODE_STOP,        IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "5.7: /  ?",   KEYCODE_SLASH,       IP_JOY_NONE )

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "6.0: ENTER",  KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "6.1: CLEAR",  KEYCODE_HOME,        IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "6.2: BREAK",  KEYCODE_END,         IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "6.3: UP",     KEYCODE_UP,          IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "6.4: DOWN",   KEYCODE_DOWN,        IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "6.5: LEFT",   KEYCODE_LEFT,        IP_JOY_NONE )
	/* backspace should do the same as cursor left */
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "6.5: (BSP)",  KEYCODE_BACKSPACE,   IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "6.6: RIGHT",  KEYCODE_RIGHT,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "6.7: SPACE",  KEYCODE_SPACE,       IP_JOY_NONE )

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "7.0: SHIFT",  KEYCODE_LSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "7.1: (ALT)",  KEYCODE_LALT,        IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "7.2: (PGUP)", KEYCODE_PGUP,        IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "7.3: (PGDN)", KEYCODE_PGDN,        IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "7.4: (INS)",  KEYCODE_INSERT,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "7.5: (DEL)",  KEYCODE_DEL,         IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "7.6: (CTRL)", KEYCODE_LCONTROL,    IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "7.7: (ALTGR)",KEYCODE_RALT,        IP_JOY_NONE )

INPUT_PORTS_END

static struct GfxLayout trs80_charlayout_normal_width =
{
	FW,FH,					/* 6 x 12 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static struct GfxLayout trs80_charlayout_double_width =
{
	FW*2,FH,	   /* FW*2 x FH*3 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	/* x offsets double width: use each bit twice */
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static struct GfxDecodeInfo trs80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &trs80_charlayout_normal_width, 0, 4 },
	{ REGION_GFX1, 0, &trs80_charlayout_double_width, 0, 4 },
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
   0x00,0x00,0x00,
   0x00,0xff,0x00,
   0xff,0xff,0xff
};

static unsigned short colortable[] =
{
	0,1 	/* green on black */
};



/* Initialise the palette */
static void trs80_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}

static INT16 speaker_levels[3] = {0.0*32767,0.46*32767,0.85*32767};

static struct Speaker_interface speaker_interface =
{
	1,					/* one speaker */
	{ 100 },			/* mixing levels */
	{ 3 },				/* optional: number of different levels */
	{ speaker_levels }	/* optional: level lookup table */
};


static struct MachineDriver machine_driver_model1 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1796000,	/* 1.796 Mhz */
			readmem_model1,writemem_model1,
			readport_model1,writeport_model1,
			trs80_frame_interrupt,1,
			trs80_timer_interrupt,40
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	trs80_init_machine,
	trs80_shutdown_machine,

	/* video hardware */
	64*FW,									/* screen width */
	16*FH,									/* screen height */
	{ 0*FW,64*FW-1,0*FH,16*FH-1},			/* visible_area */
	trs80_gfxdecodeinfo,					/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,
	sizeof(colortable)/sizeof(colortable[0]),/* colors used for the characters */
	trs80_init_palette, 					/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	trs80_vh_start,
	trs80_vh_stop,
	trs80_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
			&speaker_interface
		}
	}
};

static struct MachineDriver machine_driver_model3 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1796000,		/* 1.796 Mhz */
			readmem_model3,writemem_model3,
			readport_model3,writeport_model3,
			trs80_frame_interrupt,2,
			trs80_timer_interrupt,40
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,		/* frames per second, vblank duration */
	1,
	trs80_init_machine,
	trs80_shutdown_machine,

	/* video hardware */
	64*FW,									/* screen width */
	16*FH,									/* screen height */
	{ 0*FW,64*FW-1,0*FH,16*FH-1},			/* visible_area */
	trs80_gfxdecodeinfo,					/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,
	sizeof(colortable)/sizeof(colortable[0]),/* colors used for the characters */
	trs80_init_palette, 					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	trs80_vh_start,
	trs80_vh_stop,
	trs80_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SPEAKER,
            &speaker_interface
        }
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("trs80.rom",   0x0000, 0x3000, 0xd6fd9041)

	ROM_REGION(0x0c00, REGION_GFX1)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, 0x0033f2b9)
ROM_END


ROM_START(sys80)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("sys80.rom",   0x0000, 0x3000, 0x2bfef8f7)

	ROM_REGION(0x0c00, REGION_GFX1)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, 0x0033f2b9)
ROM_END


ROM_START(trs80m3)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("trs80.rom",   0x0000, 0x3000, 0xd6fd9041)

	ROM_REGION(0x0c00, REGION_GFX1)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, 0x0033f2b9)
ROM_END



static const struct IODevice io_trs80[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"rom\0",            /* file extensions */
		NULL,				/* private */
		NULL,				/* id */
		trs80_rom_load, 	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{
		IO_CASSETTE,		/* type */
		1,					/* count */
		"cas\0cmd\0",       /* file extensions */
		NULL,				/* private */
		NULL,				/* id */
		trs80_cassette_init,/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"dsk\0",            /* file extensions */
		NULL,				/* private */
		NULL,				/* id */
		trs80_floppy_init,	/* init */
		trs80_floppy_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

#define io_sys80 io_trs80
#define io_trs80m3 io_trs80

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
COMP ( 1978, trs80,    0,		 model1,   trs80,	 trs80,    "Tandy Radio Shack",  "TRS-80 Model I (Level II Basic)" )
COMP ( 1980, sys80,    trs80,	 model1,   trs80,	 trs80,    "EACA Computers Ltd.",  "System-80" )
COMPX( 19??, trs80m3,  trs80,	 model3,   trs80,	 trs80,    "Tandy Radio Shack",  "TRS-80 Model III", GAME_NOT_WORKING )

