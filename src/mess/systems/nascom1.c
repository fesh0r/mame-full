/************************************************************************
Nascom 1 Memory map

	CPU: z80
		0000-03ff	ROM
		0400-07ff	ROM
		0800-0bff	RAM (Screen)
		0c00-ffff	RAM


	Interrupts:

	Ports:
		00			Keyboard
		01,02		UART


************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"

/* prototypes */

extern	void	nascom1_init_machine (void);
extern	void	nascom1_stop_machine (void);

extern	int		nascom1_vh_start (void);
extern	void	nascom1_vh_stop (void);
extern	void	nascom1_vh_screenrefresh (struct osd_bitmap *bitmap,
													int full_refresh);

/* structures */

static	struct
{
	UINT8	key_flags;
	UINT8	key_count;
} nascom1_keyb;

#define NASCOM1_KEY_RESET	0x02
#define NASCOM1_KEY_INCR	0x01

/* functions */

/* port i/o functions */

READ_HANDLER ( nascom1_port_00_r )

{

if (errorlog) fprintf (errorlog, "in (00). Count: %d.\n",
						nascom1_keyb.key_count);

return (readinputport (nascom1_keyb.key_count) | ~0x3f);

}

READ_HANDLER ( nascom1_port_01_r )

{

return (0xff);

}

READ_HANDLER ( nascom1_port_02_r )

{

return (0x00);

}

WRITE_HANDLER (	nascom1_port_00_w )

{

if (!(data & NASCOM1_KEY_RESET)) {
  if (nascom1_keyb.key_flags & NASCOM1_KEY_RESET) nascom1_keyb.key_count = 0;
} else nascom1_keyb.key_flags = NASCOM1_KEY_RESET;

if (!(data & NASCOM1_KEY_INCR)) {
  if (nascom1_keyb.key_flags & NASCOM1_KEY_INCR) nascom1_keyb.key_count++;
} else nascom1_keyb.key_flags = NASCOM1_KEY_INCR;

}

static	struct	IOReadPort	nascom1_readport[] =
{
	{ 0x00, 0x00, nascom1_port_00_r},
	{ 0x01, 0x01, nascom1_port_01_r},
	{ 0x02, 0x02, nascom1_port_02_r},
	{-1}
};

static	struct	IOWritePort	nascom1_writeport[] =
{
	{ 0x00, 0x00, nascom1_port_00_w},
	{-1}
};

/* Memory w/r functions */

static	struct	MemoryReadAddress	nascom1_readmem[] =
{
	{0x0000, 0x07ff, MRA_ROM},
	{0x0800, 0x0bff, videoram_r},
	{0x0c00, 0xffff, MRA_RAM},
	{-1}
};

static	struct	MemoryWriteAddress	nascom1_writemem[] =
{
	{0x0000, 0x07ff, MWA_ROM},
	{0x0800, 0x0bff, videoram_w, &videoram, &videoram_size},
	{0x0c00, 0xffff, MWA_RAM},
	{-1}
};

/* graphics output */

struct	GfxLayout	nascom1_charlayout =
{
	8, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

static	struct	GfxDecodeInfo	nascom1_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &nascom1_charlayout, 0, 2},
	{-1}
};

static	unsigned	char	nascom1_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff,
};

static	unsigned	short	nascom1_colortable[2 * 2] =
{
	1, 0,
	0, 1,
};

static	void	nascom1_init_palette (unsigned char *sys_palette,
			unsigned short *sys_colortable, const unsigned char *color_prom)

{

memcpy (sys_palette, nascom1_palette, sizeof (nascom1_palette));
memcpy (sys_colortable, nascom1_colortable, sizeof (nascom1_colortable));

}

/* Keyboard input */

INPUT_PORTS_START (nascom1)
	PORT_START							/* count = 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)

	PORT_START							/* count = 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START							/* count = 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START							/* count = 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)

	PORT_START							/* count = 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)

	PORT_START							/* count = 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START							/* count = 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)

	PORT_START							/* count = 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)

	PORT_START							/* count = 8 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)

INPUT_PORTS_END

/* Sound output */

/* Machine definition */

static	struct	MachineDriver	machine_driver_nascom1 =
{
	{
		{
			CPU_Z80,
			1000000,
			nascom1_readmem, nascom1_writemem,
			nascom1_readport, nascom1_writeport,
			interrupt, 1,
		},
	},
	50, 2500,
	1,
	nascom1_init_machine,
	nascom1_stop_machine,
	48 * 8,
	16 * 10,
	{ 0, 48 * 8 - 1, 0, 16 * 10 - 1},
	nascom1_gfxdecodeinfo,
	2, 4,
	nascom1_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	nascom1_vh_start,
	nascom1_vh_stop,
	nascom1_vh_screenrefresh,
	0, 0, 0, 0,
};

ROM_START(nascom1)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("nascom1.rom", 0x0000, 0x0800, 0x00000000)
	ROM_REGION(0x0500, REGION_GFX1)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0500, 0x00000000)
ROM_END

static	const	struct	IODevice	io_nascom1[] =
{
	{ IO_END }
};

/*		YEAR	NAME		PARENT	MACHINE		INPUT		INIT	COMPANY		FULLNAME */
COMP(	1978,	nascom1,	0,		nascom1,	nascom1,	0,		"Nascom",	"Nascom 1" )
