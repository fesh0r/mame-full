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
#include "includes/trs80.h"
#include "devices/basicdsk.h"

#define FW	TRS80_FONT_W
#define FH	TRS80_FONT_H

MEMORY_READ_START( readmem_level1 )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x3800, 0x38ff, trs80_keyboard_r },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( writemem_level1 )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x3c00, 0x3fff, trs80_videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x7fff, MWA_RAM },
MEMORY_END

PORT_READ_START( readport_level1 )
	{ 0xff, 0xfe, trs80_port_xx_r },
	{ 0xff, 0xff, trs80_port_ff_r },
PORT_END

PORT_WRITE_START( writeport_level1 )
	{ 0xff, 0xff, trs80_port_ff_w },
PORT_END

MEMORY_READ_START( readmem_model1 )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x37df, MRA_NOP },
	{ 0x37e0, 0x37e3, trs80_irq_status_r },
	{ 0x30e4, 0x37e7, MRA_NOP },
	{ 0x37e8, 0x37eb, trs80_printer_r },
	{ 0x37ec, 0x37ec, wd179x_status_r },
	{ 0x37ed, 0x37ed, wd179x_track_r },
	{ 0x37ee, 0x37ee, wd179x_sector_r },
	{ 0x37ef, 0x37ef, wd179x_data_r },
	{ 0x37f0, 0x37ff, MRA_NOP },
	{ 0x3800, 0x38ff, trs80_keyboard_r },
	{ 0x3900, 0x3bff, MRA_NOP },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( writemem_model1 )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x37df, MWA_NOP },
	{ 0x37e0, 0x37e3, trs80_motor_w },
	{ 0x37e4, 0x37e7, MWA_NOP },
	{ 0x37e8, 0x37eb, trs80_printer_w },
	{ 0x37ec, 0x37ec, wd179x_command_w },
	{ 0x37ed, 0x37ed, wd179x_track_w },
	{ 0x37ee, 0x37ee, wd179x_sector_w },
	{ 0x37ef, 0x37ef, wd179x_data_w },
	{ 0x37f0, 0x37ff, MWA_NOP },
	{ 0x3800, 0x3bff, MWA_NOP },
	{ 0x3c00, 0x3fff, trs80_videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0xffff, MWA_RAM },
MEMORY_END

PORT_READ_START( readport_model1 )
	{ 0xff, 0xfe, trs80_port_xx_r },
	{ 0xff, 0xff, trs80_port_ff_r },
PORT_END

PORT_WRITE_START( writeport_model1 )
	{ 0xff, 0xff, trs80_port_ff_w },
PORT_END

MEMORY_READ_START( readmem_model3 )
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x3800, 0x38ff, trs80_keyboard_r },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( writemem_model3 )
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x3800, 0x38ff, MWA_NOP },
	{ 0x3c00, 0x3fff, trs80_videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0xffff, MWA_RAM },
MEMORY_END

PORT_READ_START( readport_model3 )
	{ 0xe0, 0xe3, trs80_irq_status_r },
	{ 0xf0, 0xf0, wd179x_status_r },
	{ 0xf1, 0xf1, wd179x_track_r },
	{ 0xf2, 0xf2, wd179x_sector_r },
	{ 0xf3, 0xf3, wd179x_data_r },
	{ 0xff, 0xff, trs80_port_ff_r },
PORT_END

PORT_WRITE_START( writeport_model3 )
	{ 0xe0, 0xe3, trs80_irq_mask_w },
	{ 0xe4, 0xe4, trs80_motor_w },
	{ 0xf0, 0xf0, wd179x_command_w },
	{ 0xf1, 0xf1, wd179x_track_w },
	{ 0xf2, 0xf2, wd179x_sector_w },
	{ 0xf3, 0xf3, wd179x_data_w },
	{ 0xff, 0xff, trs80_port_ff_w },
PORT_END

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
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "0.0: @   ",   KEYCODE_ASTERISK,    IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "0.1: A  a",   KEYCODE_A,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0.2: B  b",   KEYCODE_B,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "0.3: C  c",   KEYCODE_C,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "0.4: D  d",   KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "0.5: E  e",   KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0.6: F  f",   KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "0.7: G  g",   KEYCODE_G,           IP_JOY_NONE )

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "1.0: H  h",   KEYCODE_H,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1.1: I  i",   KEYCODE_I,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "1.2: J  j",   KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "1.3: K  k",   KEYCODE_K,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "1.4: L  l",   KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "1.5: M  m",   KEYCODE_M,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "1.6: N  n",   KEYCODE_N,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1.7: O  o",   KEYCODE_O,           IP_JOY_NONE )

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2.0: P  p",   KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "2.1: Q  q",   KEYCODE_Q,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2.2: R  r",   KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "2.3: S  s",   KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "2.4: T  t",   KEYCODE_T,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "2.5: U  u",   KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "2.6: V  v",   KEYCODE_V,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "2.7: W  w",   KEYCODE_W,           IP_JOY_NONE )

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "3.0: X  x",   KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3.1: Y  y",   KEYCODE_Y,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "3.2: Z  z",   KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3.3: [  {",   KEYCODE_OPENBRACE,   IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "3.4: \\  |",  KEYCODE_F1,          IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "3.5: ]  }",   KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "3.6: ^  ~",   KEYCODE_TILDE,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "3.7: _   ",   KEYCODE_EQUALS,      IP_JOY_NONE )

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "4.0: 0   ",   KEYCODE_0,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "4.1: 1  !",   KEYCODE_1,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4.2: 2  \"",  KEYCODE_2,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "4.3: 3  #",   KEYCODE_3,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4.4: 4  $",   KEYCODE_4,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "4.5: 5  %",   KEYCODE_5,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "4.6: 6  &",   KEYCODE_6,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "4.7: 7  '",   KEYCODE_7,           IP_JOY_NONE )

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "5.0: 8  (",   KEYCODE_8,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "5.1: 9  )",   KEYCODE_9,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "5.2: :  *",   KEYCODE_COLON,       IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5.3: ;  +",   KEYCODE_QUOTE,       IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "5.4: ,  <",   KEYCODE_COMMA,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5.5: -  =",   KEYCODE_MINUS,       IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "5.6: .  >",   KEYCODE_STOP,        IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "5.7: /  ?",   KEYCODE_SLASH,       IP_JOY_NONE )

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "6.0: ENTER",  KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "6.1: CLEAR",  KEYCODE_HOME,        IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "6.2: BREAK",  KEYCODE_END,         IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "6.3: UP",     KEYCODE_UP,          IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6.4: DOWN",   KEYCODE_DOWN,        IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "6.5: LEFT",   KEYCODE_LEFT,        IP_JOY_NONE )
	/* backspace should do the same as cursor left */
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "6.5: (BSP)",  KEYCODE_BACKSPACE,   IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6.6: RIGHT",  KEYCODE_RIGHT,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "6.7: SPACE",  KEYCODE_SPACE,       IP_JOY_NONE )

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "7.0: SHIFT",  KEYCODE_LSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "7.1: (ALT)",  KEYCODE_LALT,        IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "7.2: (PGUP)", KEYCODE_PGUP,        IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "7.3: (PGDN)", KEYCODE_PGDN,        IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "7.4: (INS)",  KEYCODE_INSERT,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7.5: (DEL)",  KEYCODE_DEL,         IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "7.6: (CTRL)", KEYCODE_LCONTROL,    IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7.7: (ALTGR)",KEYCODE_RALT,        IP_JOY_NONE )

INPUT_PORTS_END

static struct GfxLayout trs80_charlayout_normal_width =
{
	FW,FH,			/* 6 x 12 characters */
	256,			/* 256 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5 },
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
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static struct GfxDecodeInfo trs80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &trs80_charlayout_normal_width, 0, 4 },
	{ REGION_GFX1, 0, &trs80_charlayout_double_width, 0, 4 },
MEMORY_END	 /* end of array */

static unsigned char trs80_palette[] =
{
   0x00,0x00,0x00,
   0xff,0xff,0xff
};

static unsigned short trs80_colortable[] =
{
	0,1 	/* green on black */
};



/* Initialise the palette */
static PALETTE_INIT( trs80 )
{
	palette_set_colors(0, trs80_palette, sizeof(trs80_palette)/3);
	memcpy(colortable,trs80_colortable,sizeof(trs80_colortable));
}

static INT16 speaker_levels[3] = {0.0*32767,0.46*32767,0.85*32767};

static struct Speaker_interface speaker_interface =
{
	1,					/* one speaker */
	{ 100 },			/* mixing levels */
	{ 3 },				/* optional: number of different levels */
	{ speaker_levels }	/* optional: level lookup table */
};


static MACHINE_DRIVER_START( level1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 1796000)        /* 1.796 Mhz */
	MDRV_CPU_MEMORY(readmem_level1,writemem_level1)
	MDRV_CPU_PORTS(readport_level1,writeport_level1)
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(trs80_timer_interrupt, 40)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( trs80 )
	MDRV_MACHINE_STOP( trs80 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*FW, 16*FH)
	MDRV_VISIBLE_AREA(0*FW,64*FW-1,0*FH,16*FH-1)
	MDRV_GFXDECODE( trs80_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(trs80_palette)/sizeof(trs80_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(sizeof(trs80_colortable)/sizeof(trs80_colortable[0]))
	MDRV_PALETTE_INIT( trs80 )

	MDRV_VIDEO_START( trs80 )
	MDRV_VIDEO_UPDATE( trs80 )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model1 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( readmem_model1, writemem_model1 )
	MDRV_CPU_PORTS( readport_model1,writeport_model1 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model3 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( readmem_model3, writemem_model3 )
	MDRV_CPU_PORTS( readport_model3,writeport_model3 )
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 2)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("level1.rom",  0x0000, 0x1000, CRC(70d06dff))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(trs80l2)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80.z33",   0x0000, 0x1000, CRC(83dbbbe2))
	ROM_LOAD("trs80.z34",   0x1000, 0x1000, CRC(05818718))
	ROM_LOAD("trs80.zl2",   0x2000, 0x1000, CRC(306e5d66))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(trs80l2a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80alt.z33",0x0000, 0x1000, CRC(be46faf5))
	ROM_LOAD("trs80alt.z34",0x1000, 0x1000, CRC(6c791c2d))
	ROM_LOAD("trs80alt.zl2",0x2000, 0x1000, CRC(55b3ad13))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(sys80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("sys80rom.1",  0x0000, 0x1000, CRC(8f5214de))
	ROM_LOAD("sys80rom.2",  0x1000, 0x1000, CRC(46e88fbf))
	ROM_LOAD("trs80.zl2",  0x2000, 0x1000, CRC(306e5d66))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END

ROM_START(lnw80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("lnw_a.bin",  0x0000, 0x0800, CRC(e09f7e91))
	ROM_LOAD("lnw_a1.bin", 0x0800, 0x0800, CRC(ac297d99))
	ROM_LOAD("lnw_b.bin",  0x1000, 0x0800, CRC(c4303568))
	ROM_LOAD("lnw_b1.bin", 0x1800, 0x0800, CRC(3a5ea239))
	ROM_LOAD("lnw_c.bin",  0x2000, 0x0800, CRC(2ba025d7))
	ROM_LOAD("lnw_c1.bin", 0x2800, 0x0800, CRC(ed547445))

	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("lnw_chr.bin",0x0800, 0x0800, CRC(c89b27df))
ROM_END

ROM_START(trs80m3)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80m3.rom", 0x0000, 0x3800, NO_DUMP)

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END

SYSTEM_CONFIG_START(trs80)
	CONFIG_DEVICE_QUICKLOAD_DELAY(		"cmd\0", trs80_cmd,	0.5)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE,1,	"cas\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_trs80_cas, device_unload_trs80_cas, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(trs8012)
	CONFIG_IMPORT_FROM(trs80)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",	device_load_trs80_floppy)
SYSTEM_CONFIG_END

/*	   YEAR  NAME	   PARENT	 COMPAT	MACHINE   INPUT	 INIT	   CONFIG	COMPANY	 FULLNAME */
COMP ( 1977, trs80,    0,		 0,		level1,   trs80,	 trs80,    trs80,	"Tandy Radio Shack",  "TRS-80 Model I (Level I Basic)" )
COMP ( 1978, trs80l2,  trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (Radio Shack Level II Basic)" )
COMP ( 1978, trs80l2a, trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (R/S L2 Basic)" )
COMP ( 1980, sys80,    trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"EACA Computers Ltd.","System-80" )
COMPX( 1981, lnw80,    trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"LNW Research","LNW-80", GAME_NOT_WORKING )
COMPX( 19??, trs80m3,  trs80,	 0,		model3,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model III", GAME_NOT_WORKING )

