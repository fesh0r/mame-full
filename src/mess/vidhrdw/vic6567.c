/***************************************************************************

	Video Interface Chip 6567 (6566 6569 and some other)

***************************************************************************/
/* mos videochips
  vic (6560 NTSC, 6561 PAL)
  used in commodore vic20
  
  vic II
   6566 NTSC
    no dram refresh?
   6567 NTSC
   6569 PAL-B
   6572 PAL-N
   6573 PAL-M
   8562 NTSC
   8565 PAL
  used in commodore c64
  complete different to vic

  ted 
   7360/8360 (NTSC-M, PAL-B by same chip ?)
   8365 PAL-N
   8366 PAL-M
  used in c16 c116 c232 c264 plus4 c364
  based on the vic II
  but no sprites and not register compatible
  includes timers, input port, sound generators
  memory interface, dram refresh, clock generation

  vic IIe
   8564 NTSC-M
   8566 PAL-B
   8569 PAL-N
  used in commodore c128
  vic II with some additional features
   3 programmable output pins k0 k1 k2
     
  vic III
   4567
  used in commodore c65 prototype
  vic II compatible (mode only?) 
  several additional features
   different resolutions, more colors, ...
   (maybe like in the amiga graphic chip docu)

  vdc
   8563
   8568 (composite video and composite sync)
  second graphic chip in c128
  complete different to above chips
*/

#include <math.h>
#include <stdio.h>

#include "osd_cpu.h"
#include "driver.h"
#include "vidhrdw/generic.h"


#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/c64.h"

#include "mess/machine/vc20tape.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/cia6526.h"

#include "vic6567.h"

/*#define GFX */

#define VREFRESHINLINES 28

#define VIC2_YPOS 50
#define RASTERLINE_2_C64(a) (a)
#define C64_2_RASTERLINE(a) (a)
#define XPOS 8
#define YPOS 8

/* lightpen delivers values from internal counters
 * they do not start with the visual area or frame area */
#define VIC2_MAME_XPOS 0
#define VIC2_MAME_YPOS 0
#define VIC6567_X_BEGIN 38
#define VIC6567_Y_BEGIN -6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6569_X_BEGIN 38
#define VIC6569_Y_BEGIN -6
#define VIC2_X_BEGIN (vic2_pal?VIC6569_X_BEGIN:VIC6567_X_BEGIN)
#define VIC2_Y_BEGIN (vic2_pal?VIC6569_Y_BEGIN:VIC6567_Y_BEGIN)
#define VIC2_X_VALUE ((LIGHTPEN_X_VALUE+VIC2_X_BEGIN \
                          +VIC2_MAME_XPOS)/2)
#define VIC2_Y_VALUE ((LIGHTPEN_Y_VALUE+VIC2_Y_BEGIN \
                          +VIC2_MAME_YPOS))

#define VIC2E_K0_LEVEL (vic2[0x2f]&1)
#define VIC2E_K1_LEVEL (vic2[0x2f]&2)
#define VIC2E_K2_LEVEL (vic2[0x2f]&4)
#define VIC3_P5_LEVEL (vic2[0x30]&0x20)

	 /*#define VIC2E_TEST (vic2[0x30]&2) */
#define DOUBLE_CLOCK (vic2[0x30]&1)

/* sprites 0 .. 7 */
#define SPRITE_BASE_X_SIZE 24
#define SPRITE_BASE_Y_SIZE 21
#define SPRITEON(nr) (vic2[0x15]&(1<<nr))
#define SPRITE_Y_EXPAND(nr) (vic2[0x17]&(1<<nr))
#define SPRITE_Y_SIZE(nr) (SPRITE_Y_EXPAND(nr)?2*21:21)
#define SPRITE_X_EXPAND(nr) (vic2[0x1d]&(1<<nr))
#define SPRITE_X_SIZE(nr) (SPRITE_X_EXPAND(nr)?2*24:24)
#define SPRITE_X_POS(nr) ( (vic2[nr*2]|(vic2[0x10]&(1<<nr)?0x100:0))-25+XPOS )
#define SPRITE_Y_POS(nr) (vic2[1+2*nr]-50+YPOS)
#define SPRITE_MULTICOLOR(nr) (vic2[0x1c]&(1<<nr))
#define SPRITE_PRIORITY(nr) (vic2[0x1b]&(1<<nr))
#define SPRITE_MULTICOLOR1 (vic2[0x25]&0xf)
#define SPRITE_MULTICOLOR2 (vic2[0x26]&0xf)
#define SPRITE_COLOR(nr) (vic2[0x27+nr]&0xf)
#define SPRITE_ADDR(nr) (videoaddr+0x3f8+nr)
#define SPRITE_BG_COLLISION(nr) (vic2[0x1f]&(1<<nr))
#define SPRITE_COLLISION(nr) (vic2[0x1e]&(1<<nr))
#define SPRITE_SET_BG_COLLISION(nr) (vic2[0x1f]|=(1<<nr))
#define SPRITE_SET_COLLISION(nr) (vic2[0x1e]|=(1<<nr))

#define SCREENON (vic2[0x11]&0x10)
#define VERTICALPOS (vic2[0x11]&7)
#define HORICONTALPOS (vic2[0x16]&7)
#define ECMON (vic2[0x11]&0x40)
#define HIRESON (vic2[0x11]&0x20)
#define MULTICOLORON (vic2[0x16]&0x10)
#define LINES25 (vic2[0x11]&8)		   /* else 24 Lines */
#define LINES (LINES25?25:24)
#define YSIZE (LINES*8)
#define COLUMNS40 (vic2[0x16]&8)	   /* else 38 Columns */
#define COLUMNS (COLUMNS40?40:38)
#define XSIZE (COLUMNS*8)

#define VIDEOADDR ( (vic2[0x18]&0xf0)<<(10-4) )
#define CHARGENADDR ((vic2[0x18]&0xe)<<10)

#define RASTERLINE ( ((vic2[0x11]&0x80)<<1)|vic2[0x12])

#define BACKGROUNDCOLOR (vic2[0x21]&0xf)
#define MULTICOLOR1 (vic2[0x22]&0xf)
#define MULTICOLOR2 (vic2[0x23]&0xf)
#define FOREGROUNDCOLOR (vic2[0x24]&0xf)
#define FRAMECOLOR (vic2[0x20]&0xf)

unsigned char vic2_palette[] =
{
/* black, white, red, cyan */
/* purple, green, blue, yellow */
/* orange, brown, light red, dark gray, */
/* medium gray, light green, light blue, light gray */
/* taken from the vice emulator */
	0x00, 0x00, 0x00, 0xfd, 0xfe, 0xfc, 0xbe, 0x1a, 0x24, 0x30, 0xe6, 0xc6,
	0xb4, 0x1a, 0xe2, 0x1f, 0xd2, 0x1e, 0x21, 0x1b, 0xae, 0xdf, 0xf6, 0x0a,
	0xb8, 0x41, 0x04, 0x6a, 0x33, 0x04, 0xfe, 0x4a, 0x57, 0x42, 0x45, 0x40,
	0x70, 0x74, 0x6f, 0x59, 0xfe, 0x59, 0x5f, 0x53, 0xfe, 0xa4, 0xa7, 0xa2
};

UINT8 vic2[0x40] =
{0};

bool vic2_pal;
bool vic2e=false;			   			   /* version with some port lines */
bool vic3=false;			
static void (*port_changed)(int)=NULL;

static int lines;
static void *lightpentimer;
static int (*vic_dma_read) (int);
static int (*vic_dma_read_color) (int);
static void (*vic_interrupt) (int);

static struct
{
	int repeat;						   /* y expand, line once drawn */
	int line;						   /* 0 not painting, else painting */

	/* buffer for currently painted line */
	int paintedline[8];
	UINT8 bitmap[8][SPRITE_BASE_X_SIZE * 2 / 8];
}
sprites[8] =
{
	{
		0
	}
};

static int chargenaddr, videoaddr;

/* convert multicolor byte to background/foreground for sprite collision */
static UINT8 foreground[256];
static UINT16 expandx[256];
static UINT16 expandx_multi[256];

/* converts sprite multicolor info to info for background collision checking */
static UINT8 multi_collision[256];

/* background/foreground for sprite collision */
static UINT8 *vic2_screen[216], vic2_shift[216];

struct osd_bitmap *vic2_bitmap;		   /* Machine->scrbitmap for speedup */

static int vic2_getforeground (register int y, register int x)
{
	return ((vic2_screen[y][x >> 3] << 8)
			| (vic2_screen[y][(x >> 3) + 1])) >> (8 - (x & 7));
}

static int vic2_getforeground16 (register int y, register int x)
{
	return ((vic2_screen[y][x >> 3] << 16)
			| (vic2_screen[y][(x >> 3) + 1] << 8)
			| (vic2_screen[y][(x >> 3) + 2])) >> (8 - (x & 7));
}

#if 0
static void vic2_setforeground (int y, int x, int value)
{
	vic2_screen[y][x >> 3] = value;
}
#endif

static int x_begin, x_end;
static int y_begin, y_end;

static UINT16 c64_bitmap[2], bitmapmulti[4], mono[2], multi[4], ecmcolor[2], colors[4], spritemulti[4];

static int lastline = 0, rasterline = 0;
static void vic2_drawlines (int first, int last);

static void vic2_init (int (*dma_read) (int), int (*dma_read_color) (int),
					   void (*irq) (int))
{
	lines = VIC2_LINES;
	chargenaddr = videoaddr = 0;
	vic_dma_read = dma_read;
	vic_dma_read_color = dma_read_color;
	vic_interrupt = irq;
	rasterline = 0;
}

void vic6567_init (int chip_vic2e, int pal,
				   int (*dma_read) (int), int (*dma_read_color) (int),
				   void (*irq) (int))
{
	vic2e = chip_vic2e;
	vic2_pal = pal;
	vic2_init (dma_read, dma_read_color, irq);
}

extern void vic4567_init (int pal, int (*dma_read) (int),
						  int (*dma_read_color) (int), void (*irq) (int),
						  void (*param_port_changed)(int))
{
	vic3=true;
	vic2_pal = pal;
	port_changed = param_port_changed;
	vic2_init (dma_read, dma_read_color, irq);
}


int vic2e_k0_r (void)
{
	return VIC2E_K0_LEVEL;
}
int vic2e_k1_r (void)
{
	return VIC2E_K1_LEVEL;
}
int vic2e_k2_r (void)
{
	return VIC2E_K2_LEVEL;
}
int vic3_p5_r (void)
{
	return VIC3_P5_LEVEL;
}

static void vic2_set_interrupt (int mask)
{
	if (((vic2[0x19] ^ mask) & vic2[0x1a] & 0xf))
	{
		if (!(vic2[0x19] & 0x80))
		{
			DBG_LOG (1, "vic2", (errorlog, "irq start %.2x\n", mask));
			vic2[0x19] |= 0x80;
			vic_interrupt (1);
		}
	}
	vic2[0x19] |= mask;
}

static void vic2_clear_interrupt (int mask)
{
	vic2[0x19] &= ~mask;
	if ((vic2[0x19] & 0x80) && !(vic2[0x19] & vic2[0x1a] & 0xf))
	{
		DBG_LOG (1, "vic2", (errorlog, "irq end %.2x\n", mask));
		vic2[0x19] &= ~0x80;
		vic_interrupt (0);
	}
}

void vic2_lightpen_write (int level)
{
	/* calculate current position, write it and raise interrupt */
}

static void vic2_timer_timeout (int which)
{
	DBG_LOG (3, "vic2 ", (errorlog, "timer %d timeout\n", which));
	switch (which)
	{
	case 1:						   /* light pen */
		/* and diode must recognize light */
		if (1)
		{
			vic2[0x13] = VIC2_X_VALUE;
			vic2[0x14] = VIC2_Y_VALUE;
		}
		vic2_set_interrupt (8);
		break;
	}
}

int vic2_frame_interrupt (void)
{
	return ignore_interrupt ();
}

void vic2_port_w (int offset, int data)
{
	DBG_LOG (2, "vic write", (errorlog, "%.2x:%.2x\n", offset, data));
	offset &= 0x3f;
	switch (offset)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
	case 0xb:
	case 0xd:
	case 0xf:
		/* sprite y positions */
	case 0:
	case 2:
	case 4:
	case 6:
	case 8:
	case 0xa:
	case 0xc:
	case 0xe:
	case 0x10:						   /* sprite x positions */
	case 0x17:						   /* sprite y size */
	case 0x1d:						   /* sprite x size */
	case 0x1b:						   /* sprite background priority */
	case 0x1c:						   /* sprite multicolor mode select */
	case 0x27:
	case 0x28:						   /* sprite colors */
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
		if (vic2[offset] != data)
		{
			vic2[offset] = data;
			vic2_drawlines (lastline, rasterline);
		}
		break;
	case 0x25:
		if (vic2[offset] != data)
		{
			vic2[offset] = data;
			spritemulti[1] = Machine->pens[SPRITE_MULTICOLOR1];
			vic2_drawlines (lastline, rasterline);
		}
		break;
	case 0x26:
		if (vic2[offset] != data)
		{
			vic2[offset] = data;
			spritemulti[3] = Machine->pens[SPRITE_MULTICOLOR2];
			vic2_drawlines (lastline, rasterline);
		}
		break;
	case 0x19:
		vic2_clear_interrupt (data & 0xf);
		break;
	case 0x1a:						   /* irq mask */
		vic2[offset] = data;
		break;
	case 0x11:
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			if (LINES25)
			{
				y_begin = 0;
				y_end = y_begin + 200;
			}
			else
			{
				y_begin = 4;
				y_end = y_begin + 192;
			}
		}
		break;
	case 0x12:
		if (data != vic2[offset])
		{
			vic2[offset] = data;
		}
		break;
	case 0x16:
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			if (COLUMNS40)
			{
				x_begin = 0;
				x_end = x_begin + 320;
			}
			else
			{
				x_begin = HORICONTALPOS;
				x_end = x_begin + 320;
			}
		}
		break;
	case 0x18:
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			videoaddr = VIDEOADDR;
			chargenaddr = CHARGENADDR;
		}
		break;
	case 0x21:						   /* backgroundcolor */
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			mono[0] = bitmapmulti[0] = multi[0] = colors[0] =
				Machine->pens[BACKGROUNDCOLOR];
		}
		break;
	case 0x22:						   /* background color 1 */
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			multi[1] = colors[1] = Machine->pens[MULTICOLOR1];
		}
		break;
	case 0x23:						   /* background color 2 */
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			multi[2] = colors[2] = Machine->pens[MULTICOLOR2];
		}
		break;
	case 0x24:						   /* background color 3 */
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
			colors[3] = Machine->pens[FOREGROUNDCOLOR];
		}
		break;
	case 0x20:						   /* framecolor */
		if (vic2[offset] != data)
		{
			vic2_drawlines (lastline, rasterline);
			vic2[offset] = data;
		}
		break;
	case 0x2f:
		DBG_LOG (1, "vic write", (errorlog, "%.2x:%.2x\n", offset, data));
		if (vic2e || vic3)
		{
			vic2[offset] = data;
		}
		break;
	case 0x30:
		DBG_LOG (1, "vic write", (errorlog, "%.2x:%.2x\n", offset, data));
		if (vic2e || vic3)
		{
			vic2[offset] = data;
		}
		if (vic3 && (port_changed!=NULL)) {
			port_changed(data);
		}
		break;
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:						   /* not used */
		break;
	default:
		vic2[offset] = data;
		break;
	}
}

int vic2_port_r (int offset)
{
	int val = 0;

	offset &= 0x3f;
	switch (offset)
	{
	case 0x11:
		val = (vic2[offset] & ~0x80) | ((rasterline & 0x100) >> 1);
		vic2_drawlines (lastline, rasterline);
		break;
	case 0x12:
		val = rasterline & 0xff;
		vic2_drawlines (lastline, rasterline);
		break;
	case 0x16:
		val = vic2[offset] | 0xc0;
		break;
	case 0x18:
		val = vic2[offset] | 1;
		break;
	case 0x19:						   /* interrupt flag register */
		/*    vic2_clear_interrupt(0xf); */
		val = vic2[offset] | 0x70;
		break;
	case 0x1a:
		val = vic2[offset] | 0xf0;
		break;
	case 0x1e:						   /* sprite to sprite collision detect */
		val = vic2[offset];
		vic2[offset] = 0;
		vic2_clear_interrupt (4);
		break;
	case 0x1f:						   /* sprite to background collision detect */
		val = vic2[offset];
		vic2[offset] = 0;
		vic2_clear_interrupt (2);
		break;
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
		val = vic2[offset] | 0xf0;
		break;
	case 0x2f:
	case 0x30:
		if (vic2e || vic3) {
			val = vic2[offset];
			val |= 0xce;
		} else
			val = 0xff;
		break;
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:						   /* not used */
		val = 0xff;
		break;
	default:
		val = vic2[offset];
	}
	if ((offset != 0x11) && (offset != 0x12))
		DBG_LOG (2, "vic read", (errorlog, "%.2x:%.2x\n", offset, val));
	return val;
}

int vic2_vh_start (void)
{
	int i;

	vic2_bitmap = Machine->scrbitmap;

	vic2_screen[0] = malloc (sizeof (UINT8) * 216 * 336 / 8);

	if (!vic2_screen[0])
		return 1;
	for (i = 1; i < 216; i++)
		vic2_screen[i] = vic2_screen[i - 1] + 336 / 8;

	for (i = 0; i < 256; i++)
	{
		foreground[i] = 0;
		if ((i & 3) > 1)
			foreground[i] |= 0x3;
		if ((i & 0xc) > 0x4)
			foreground[i] |= 0xc;
		if ((i & 0x30) > 0x10)
			foreground[i] |= 0x30;
		if ((i & 0xc0) > 0x40)
			foreground[i] |= 0xc0;
	}
	for (i = 0; i < 256; i++)
	{
		expandx[i] = 0;
		if (i & 1)
			expandx[i] |= 3;
		if (i & 2)
			expandx[i] |= 0xc;
		if (i & 4)
			expandx[i] |= 0x30;
		if (i & 8)
			expandx[i] |= 0xc0;
		if (i & 0x10)
			expandx[i] |= 0x300;
		if (i & 0x20)
			expandx[i] |= 0xc00;
		if (i & 0x40)
			expandx[i] |= 0x3000;
		if (i & 0x80)
			expandx[i] |= 0xc000;
	}
	for (i = 0; i < 256; i++)
	{
		expandx_multi[i] = 0;
		if (i & 1)
			expandx_multi[i] |= 5;
		if (i & 3)
			expandx_multi[i] |= 0xa;
		if (i & 4)
			expandx_multi[i] |= 0x50;
		if (i & 8)
			expandx_multi[i] |= 0xa0;
		if (i & 0x10)
			expandx_multi[i] |= 0x500;
		if (i & 0x20)
			expandx_multi[i] |= 0xa00;
		if (i & 0x40)
			expandx_multi[i] |= 0x5000;
		if (i & 0x80)
			expandx_multi[i] |= 0xa000;
	}
	for (i = 0; i < 256; i++)
	{
		multi_collision[i] = 0;
		if (i & 3)
			multi_collision[i] |= 3;
		if (i & 0xc)
			multi_collision[i] |= 0xc;
		if (i & 0x30)
			multi_collision[i] |= 0x30;
		if (i & 0xc0)
			multi_collision[i] |= 0xc0;
	}
	return 0;
}

void vic2_vh_stop (void)
{
	free (vic2_screen[0]);
}

static void vic2_draw_character (int ybegin, int yend, int ch,
								 int yoff, int xoff, UINT16 *color)
{
	int y, code;

	if (Machine->color_depth == 8)
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = code;
			vic2_bitmap->line[y + yoff][xoff] = color[code >> 7];
			vic2_bitmap->line[y + yoff][1 + xoff] = color[(code >> 6) & 1];
			vic2_bitmap->line[y + yoff][2 + xoff] = color[(code >> 5) & 1];
			vic2_bitmap->line[y + yoff][3 + xoff] = color[(code >> 4) & 1];
			vic2_bitmap->line[y + yoff][4 + xoff] = color[(code >> 3) & 1];
			vic2_bitmap->line[y + yoff][5 + xoff] = color[(code >> 2) & 1];
			vic2_bitmap->line[y + yoff][6 + xoff] = color[(code >> 1) & 1];
			vic2_bitmap->line[y + yoff][7 + xoff] = color[code & 1];
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = code;
			*((short *) vic2_bitmap->line[y + yoff] + xoff) = color[code >> 7];
			*((short *) vic2_bitmap->line[y + yoff] + 1 + xoff) = color[(code >> 6) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 2 + xoff) = color[(code >> 5) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 3 + xoff) = color[(code >> 4) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 4 + xoff) = color[(code >> 3) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 5 + xoff) = color[(code >> 2) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 6 + xoff) = color[(code >> 1) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 7 + xoff) = color[code & 1];
		}
	}
}

static void vic2_draw_character_multi (int ybegin, int yend, int ch,
									   int yoff, int xoff)
{
	int y, code;

	if (Machine->color_depth == 8)
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = foreground[code];
			vic2_bitmap->line[y + yoff][xoff] =
				vic2_bitmap->line[y + yoff][xoff + 1] = multi[code >> 6];
			vic2_bitmap->line[y + yoff][xoff + 2] =
				vic2_bitmap->line[y + yoff][xoff + 3] = multi[(code >> 4) & 3];
			vic2_bitmap->line[y + yoff][xoff + 4] =
				vic2_bitmap->line[y + yoff][xoff + 5] = multi[(code >> 2) & 3];
			vic2_bitmap->line[y + yoff][xoff + 6] =
				vic2_bitmap->line[y + yoff][xoff + 7] = multi[code & 3];
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = foreground[code];
			*((short *) vic2_bitmap->line[y + yoff] + xoff) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 1) = multi[code >> 6];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 2) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 3) = multi[(code >> 4) & 3];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 4) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 5) = multi[(code >> 2) & 3];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 6) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 7) = multi[code & 3];
		}
	}
}

static void vic2_draw_bitmap (int ybegin, int yend,
							  int ch, int yoff, int xoff)
{
	int y, code;

	if (Machine->color_depth == 8)
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = code;
			vic2_bitmap->line[y + yoff][xoff] = c64_bitmap[code >> 7];
			vic2_bitmap->line[y + yoff][1 + xoff] = c64_bitmap[(code >> 6) & 1];
			vic2_bitmap->line[y + yoff][2 + xoff] = c64_bitmap[(code >> 5) & 1];
			vic2_bitmap->line[y + yoff][3 + xoff] = c64_bitmap[(code >> 4) & 1];
			vic2_bitmap->line[y + yoff][4 + xoff] = c64_bitmap[(code >> 3) & 1];
			vic2_bitmap->line[y + yoff][5 + xoff] = c64_bitmap[(code >> 2) & 1];
			vic2_bitmap->line[y + yoff][6 + xoff] = c64_bitmap[(code >> 1) & 1];
			vic2_bitmap->line[y + yoff][7 + xoff] = c64_bitmap[code & 1];
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = code;
			*((short *) vic2_bitmap->line[y + yoff] + xoff) = c64_bitmap[code >> 7];
			*((short *) vic2_bitmap->line[y + yoff] + 1 + xoff) = c64_bitmap[(code >> 6) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 2 + xoff) = c64_bitmap[(code >> 5) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 3 + xoff) = c64_bitmap[(code >> 4) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 4 + xoff) = c64_bitmap[(code >> 3) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 5 + xoff) = c64_bitmap[(code >> 2) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 6 + xoff) = c64_bitmap[(code >> 1) & 1];
			*((short *) vic2_bitmap->line[y + yoff] + 7 + xoff) = c64_bitmap[code & 1];
		}
	}
}

static void vic2_draw_bitmap_multi (int ybegin, int yend,
									int ch, int yoff, int xoff)
{
	int y, code;

	if (Machine->color_depth == 8)
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = foreground[code];
			vic2_bitmap->line[y + yoff][xoff] =
				vic2_bitmap->line[y + yoff][xoff + 1] = bitmapmulti[code >> 6];
			vic2_bitmap->line[y + yoff][xoff + 2] =
				vic2_bitmap->line[y + yoff][xoff + 3] = bitmapmulti[(code >> 4) & 3];
			vic2_bitmap->line[y + yoff][xoff + 4] =
				vic2_bitmap->line[y + yoff][xoff + 5] = bitmapmulti[(code >> 2) & 3];
			vic2_bitmap->line[y + yoff][xoff + 6] =
				vic2_bitmap->line[y + yoff][xoff + 7] = bitmapmulti[code & 3];
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			code = vic_dma_read (chargenaddr + ch * 8 + y);
			vic2_screen[y + yoff][xoff >> 3] = foreground[code];
			*((short *) vic2_bitmap->line[y + yoff] + xoff) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 1) = bitmapmulti[code >> 6];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 2) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 3) = bitmapmulti[(code >> 4) & 3];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 4) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 5) = bitmapmulti[(code >> 2) & 3];
			*((short *) vic2_bitmap->line[y + yoff] + xoff + 6) =
				*((short *) vic2_bitmap->line[y + yoff] + xoff + 7) = bitmapmulti[code & 3];
		}
	}
}

static void vic2_draw_sprite_code_multi (int y, int xbegin,
										 int code, int prior)
{
	register int x, mask, shift;

	if ((y < YPOS) || (y >= 208) || (xbegin <= 1) || (xbegin >= 328))
		return;
	if (Machine->color_depth == 8)
	{
		for (x = 0, mask = 0xc0, shift = 6; x < 8; x += 2, mask >>= 2, shift -= 2)
		{
			if (code & mask)
			{
				switch ((prior & mask) >> shift)
				{
				case 1:
					vic2_bitmap->line[y][xbegin + x + 1] =
						spritemulti[(code >> shift) & 3];
					break;
				case 2:
					vic2_bitmap->line[y][xbegin + x] =
						spritemulti[(code >> shift) & 3];
					break;
				case 3:
					vic2_bitmap->line[y][xbegin + x] =
						vic2_bitmap->line[y][xbegin + x + 1] =
						spritemulti[(code >> shift) & 3];
					break;
				}
			}
		}
	}
	else
	{
		for (x = 0, mask = 0xc0, shift = 6; x < 8; x += 2, mask >>= 2, shift -= 2)
		{
			if (code & mask)
			{
				switch ((prior & mask) >> shift)
				{
				case 1:
					((short *) vic2_bitmap->line[y])[xbegin + x + 1] =
						spritemulti[(code >> shift) & 3];
					break;
				case 2:
					((short *) vic2_bitmap->line[y])[xbegin + x] =
						spritemulti[(code >> shift) & 3];
					break;
				case 3:
					((short *) vic2_bitmap->line[y])[xbegin + x] =
						((short *) vic2_bitmap->line[y])[xbegin + x + 1] =
						spritemulti[(code >> shift) & 3];
					break;
				}
			}
		}
	}
}

static void vic2_draw_sprite_code (int y, int xbegin, int code, int color)
{
	register int mask, x;

	if ((y < YPOS) || (y >= 208) || (xbegin <= 1) || (xbegin >= 328))
		return;
	if (Machine->color_depth == 8)
	{
		for (x = 0, mask = 0x80; x < 8; x++, mask >>= 1)
		{
			if (code & mask)
			{
				vic2_bitmap->line[y][xbegin + x] = color;
			}
		}
	}
	else
	{
		for (x = 0, mask = 0x80; x < 8; x++, mask >>= 1)
		{
			if (code & mask)
			{
				((short *) vic2_bitmap->line[y])[xbegin + x] = color;
			}
		}
	}
}

static void vic2_sprite_collision (int nr, int y, int x, int mask)
{
	int i, value, xdiff;

	for (i = 7; i > nr; i--)
	{
		if (!SPRITEON (i)
			|| !sprites[i].paintedline[y]
			|| (SPRITE_COLLISION (i) && SPRITE_COLLISION (nr)))
			continue;
		if ((x + 7 < SPRITE_X_POS (i))
			|| (x >= SPRITE_X_POS (i) + SPRITE_X_SIZE (i)))
			continue;
		xdiff = x - SPRITE_X_POS (i);
		if ((x & 7) == (SPRITE_X_POS (i) & 7))
			value = sprites[i].bitmap[y][xdiff >> 3];
		else if (xdiff < 0)
			value = sprites[i].bitmap[y][0] >> (xdiff + 8);
		else {
			UINT8 *vp = sprites[nr].bitmap[y]+(xdiff>>3);
			value = (vp[1] | (*vp << 8)) >> (8 - x) & 0xff;
		}
		if (value & mask)
		{
			SPRITE_SET_COLLISION (i);
			SPRITE_SET_COLLISION (nr);
			vic2_set_interrupt (4);
		}
	}
}

static void vic2_draw_sprite_multi (int nr, int yoff, int ybegin, int yend)
{
	int y, i, prior, addr, xbegin, collision;
	int value, value2, value3 = 0, bg, color[2];

	xbegin = SPRITE_X_POS (nr);
	addr = vic_dma_read (SPRITE_ADDR (nr)) << 6;
	spritemulti[2] = Machine->pens[SPRITE_COLOR (nr)];
	prior = SPRITE_PRIORITY (nr);
	collision = SPRITE_BG_COLLISION (nr);
	color[0] = Machine->pens[0];
	color[1] = Machine->pens[1];

	if (SPRITE_X_EXPAND (nr))
	{
		for (y = ybegin; y <= yend; y++)
		{
			sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = expandx_multi[bg = vic_dma_read (addr + sprites[nr].line * 3 + i)];   
#if !defined(LSB_FIRST) || defined(ALIGN_SHORTS)
				{       
					UINT8 *vp = sprites[nr].bitmap[y] + 2*i;
					value2=expandx[multi_collision[bg]];
					*vp = value2 & 0xff;
					*(vp + 1) = value2 >> 8;
				}
#else
				value2 = expandx[multi_collision[bg]];
				sprites[nr].bitmap[y][i*2] = value2>>8;
				sprites[nr].bitmap[y][i*2+1] = value2&0xff;
#endif
				vic2_sprite_collision (nr, y, xbegin + i * 16, value2 >> 8);
				vic2_sprite_collision (nr, y, xbegin + i * 16 + 8, value2 & 0xff);
				if (prior || !collision)
				{
					value3 = vic2_getforeground16 (yoff + y, xbegin + i * 16);
				}
				if (!collision && (value2 & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt (2);
				}
				if (prior)
				{
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 16, value >> 8,
												 (value3 >> 8) ^ 0xff);
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 16 + 8, value & 0xff,
												 (value3 & 0xff) ^ 0xff);
				}
				else
				{
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 16, value >> 8, 0xff);
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 16 + 8, value & 0xff, 0xff);
				}
			}
			if (SPRITE_Y_EXPAND (nr))
			{
				if (sprites[nr].repeat)
				{
					sprites[nr].line++;
					sprites[nr].repeat = 0;
				}
				else
					sprites[nr].repeat = 1;
			}
			else
			{
				sprites[nr].line++;
			}
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic_dma_read (addr + sprites[nr].line * 3 + i);
				sprites[nr].bitmap[y][i] =
					value2 = multi_collision[value];
				vic2_sprite_collision (nr, y, xbegin + i * 8, value2);
				if (prior || !collision)
				{
					value3 = vic2_getforeground (yoff + y, xbegin + i * 8);
				}
				if (!collision && (value2 & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt (2);
				}
				if (prior)
				{
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 8, value, value3 ^ 0xff);
				}
				else
				{
					vic2_draw_sprite_code_multi (yoff + y, xbegin + i * 8, value, 0xff);
				}
			}
			if (SPRITE_Y_EXPAND (nr))
			{
				if (sprites[nr].repeat)
				{
					sprites[nr].line++;
					sprites[nr].repeat = 0;
				}
				else
					sprites[nr].repeat = 1;
			}
			else
			{
				sprites[nr].line++;
			}
		}
	}
}

static void vic2_draw_sprite (int nr, int yoff, int ybegin, int yend)
{
	int y, i, addr, xbegin, color, prior, collision;
	int value, value3 = 0;

	xbegin = SPRITE_X_POS (nr);
	addr = vic_dma_read (SPRITE_ADDR (nr)) << 6;
	color = Machine->pens[SPRITE_COLOR (nr)];
	prior = SPRITE_PRIORITY (nr);
	collision = SPRITE_BG_COLLISION (nr);

	if (SPRITE_X_EXPAND (nr))
	{
		for (y = ybegin; y <= yend; y++)
		{
			sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = expandx[vic_dma_read (addr + sprites[nr].line * 3 + i)];
#if !defined(LSB_FIRST) || defined(ALIGN_SHORTS)
				{
					UINT8 *vp = sprites[nr].bitmap[y] + 2*i;
					*vp = value & 0xff;
					*(vp + 1) = value >> 8;
				}
#else
				sprites[nr].bitmap[y][i*2] = value>>8;
				sprites[nr].bitmap[y][i*2+1] = value&0xff;
				//((UINT16 *) sprites[nr].bitmap[y])[i] = value;
#endif
				vic2_sprite_collision (nr, y, xbegin + i * 16, value >> 8);
				vic2_sprite_collision (nr, y, xbegin + i * 16 + 8, value & 0xff);
				if (prior || !collision)
					value3 = vic2_getforeground16 (yoff + y, xbegin + i * 16);
				if (!collision && (value & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt (2);
				}
				if (prior)
					value &= ~value3;
				vic2_draw_sprite_code (yoff + y, xbegin + i * 16, value >> 8, color);
				vic2_draw_sprite_code (yoff + y, xbegin + i * 16 + 8, value & 0xff, color);
			}
			if (SPRITE_Y_EXPAND (nr))
			{
				if (sprites[nr].repeat)
				{
					sprites[nr].line++;
					sprites[nr].repeat = 0;
				}
				else
					sprites[nr].repeat = 1;
			}
			else
			{
				sprites[nr].line++;
			}
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic_dma_read (addr + sprites[nr].line * 3 + i);
				sprites[nr].bitmap[y][i] = value;
				vic2_sprite_collision (nr, y, xbegin + i * 8, value);
				if (prior || !collision)
					value3 = vic2_getforeground (yoff + y, xbegin + i * 8);
				if (!collision && (value & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt (2);
				}
				if (prior)
					value &= ~value3;
				vic2_draw_sprite_code (yoff + y, xbegin + i * 8, value, color);
			}
			if (SPRITE_Y_EXPAND (nr))
			{
				if (sprites[nr].repeat)
				{
					sprites[nr].line++;
					sprites[nr].repeat = 0;
				}
				else
					sprites[nr].repeat = 1;
			}
			else
			{
				sprites[nr].line++;
			}
		}
	}
}

static void vic3_drawlines (int first, int last)
{
	int line, vline, end;
	int attr, ch, ecm;
	int syend;
	int offs, yoff, xoff, ybegin, yend, xbegin, xend;
	int x_end2;
	int i, j;

	if (first == last)
		return;
	lastline = last;
	if (osd_skip_this_frame ())
		return;

	/* top part of display not rastered */
	first -= VIC2_YPOS - YPOS;
	last -= VIC2_YPOS - YPOS;
	if ((first >= last) || (last <= 0))
	{
		for (i = 0; i < 8; i++)
			sprites[i].repeat = sprites[i].line = 0;
		return;
	}
	if (first < 0)
		first = 0;

	if (!SCREENON)
	{
		if (Machine->color_depth == 8)
		{
			for (line = first; (line < last) && (line < vic2_bitmap->height); line++)
				memset (vic2_bitmap->line[line], Machine->pens[0], vic2_bitmap->width);
		}
		else
		{
			for (line = first; (line < last) && (line < vic2_bitmap->height); line++)
				memset16 (vic2_bitmap->line[line], Machine->pens[0], vic2_bitmap->width);
		}
		return;
	}

	if (COLUMNS40)
		xbegin = XPOS, xend = xbegin + 640;
	else
		xbegin = XPOS + 7, xend = xbegin + 624;

	if (last < y_begin)
		end = last;
	else
		end = y_begin + YPOS;
	if (Machine->color_depth == 8)
	{
		for (line = first; line < end; line++)
			memset (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					vic2_bitmap->width);
	}
	else
	{
		for (line = first; line < end; line++)
			memset16 (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					  vic2_bitmap->width);
	}
	if (LINES25)
	{
		vline = line - y_begin - YPOS;
	}
	else
	{
		vline = line - y_begin - YPOS + 8 - VERTICALPOS;
	}
	if (last < y_end + YPOS)
		end = last;
	else
		end = y_end + YPOS;
	x_end2=x_end*2;
	for (; line < end; vline = (vline + 8) & ~7, line = line + 1 + yend - ybegin)
	{
		offs = (vline >> 3) * 80;
		ybegin = vline & 7;
		yoff = line - ybegin;
		yend = (yoff + 7 < end) ? 7 : (end - yoff - 1);
		/* rendering 39 characters */
		/* left and right borders are overwritten later */
		vic2_shift[line] = HORICONTALPOS;

		for (xoff = x_begin + XPOS; xoff < x_end2 + XPOS; xoff += 8, offs++)
		{
			ch = vic_dma_read (videoaddr + offs);
			attr = vic_dma_read_color (videoaddr + offs);
			if (HIRESON)
			{
				bitmapmulti[1] = c64_bitmap[1] = Machine->pens[ch >> 4];
				bitmapmulti[2] = c64_bitmap[0] = Machine->pens[ch & 0xf];
				if (MULTICOLORON)
				{
					bitmapmulti[3] = Machine->pens[attr];
					vic2_draw_bitmap_multi (ybegin, yend, offs, yoff, xoff);
				}
				else
				{
					vic2_draw_bitmap (ybegin, yend, offs, yoff, xoff);
				}
			}
			else if (ECMON)
			{
				ecm = ch >> 6;
				ecmcolor[0] = colors[ecm];
				ecmcolor[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch & ~0xC0, yoff, xoff, ecmcolor);
			}
			else if (MULTICOLORON && (attr & 8))
			{
				multi[3] = Machine->pens[attr & 7];
				vic2_draw_character_multi (ybegin, yend, ch, yoff, xoff);
			}
			else
			{
				mono[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch, yoff, xoff, mono);
			}
		}
		/* sprite priority, sprite overwrites lowerprior pixels */
		for (i = 7; i >= 0; i--)
		{
			if (sprites[i].line || sprites[i].repeat)
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if ((21 - sprites[i].line) * 2 - sprites[i].repeat < yend - ybegin + 1)
						syend = ybegin + (21 - sprites[i].line) * 2 - sprites[i].repeat - 1;
				}
				else
				{
					if (sprites[i].line + yend - ybegin + 1 > 20)
						syend = ybegin + 20 - sprites[i].line;
				}
				if (yoff + syend > YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, ybegin, syend);
				else
					vic2_draw_sprite (i, yoff, ybegin, syend);
				if ((syend != yend) || (sprites[i].line > 20))
				{
					sprites[i].line = sprites[i].repeat = 0;
					for (j = syend; j <= yend; j++)
						sprites[i].paintedline[j] = 0;
				}
			}
			else if (SPRITEON (i) && (yoff + ybegin <= SPRITE_Y_POS (i))
					 && (yoff + yend >= SPRITE_Y_POS (i)))
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if (21 * 2 < yend - ybegin + 1)
						syend = ybegin + 21 * 2 - 1;
				}
				else
				{
					if (yend - ybegin + 1 > 21)
						syend = ybegin + 21 - 1;
				}
				if (yoff + syend >= YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				for (j = 0; j < SPRITE_Y_POS (i) - yoff; j++)
					sprites[i].paintedline[j] = 0;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				else
					vic2_draw_sprite (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				if ((syend != yend) || (sprites[i].line > 20))
				{
					for (j = syend; j <= yend; j++)
						sprites[i].paintedline[j] = 0;
					sprites[i].line = sprites[i].repeat = 0;
				}
			}
			else
			{
				memset (sprites[i].paintedline, 0, sizeof (sprites[i].paintedline));
			}
		}
		if (Machine->color_depth == 8)
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset (vic2_bitmap->line[yoff + i], Machine->pens[FRAMECOLOR], xbegin);
				memset (vic2_bitmap->line[yoff + i] + xend, Machine->pens[FRAMECOLOR],
						vic2_bitmap->width - xend);
			}
		}
		else
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset16 (vic2_bitmap->line[yoff + i], Machine->pens[FRAMECOLOR],
						  xbegin);
				memset16 ((short *) vic2_bitmap->line[yoff + i] + xend,
						  Machine->pens[FRAMECOLOR], vic2_bitmap->width - xend);
			}
		}
	}
	if (last < vic2_bitmap->height)
		end = last;
	else
		end = vic2_bitmap->height;
	if (Machine->color_depth == 8)
	{
		for (; line < end; line++)
			memset (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					vic2_bitmap->width);
	}
	else
	{
		for (; line < end; line++)
			memset16 (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					  vic2_bitmap->width);
	}
}

static void vic2_drawlines (int first, int last)
{
	int line, vline, end;
	int attr, ch, ecm;
	int syend;
	int offs, yoff, xoff, ybegin, yend, xbegin, xend;
	int i, j;

	/* temporary allowing vic3 displaying 80 columns */
	if (vic3) {
		vic3_drawlines(first,last);
		return;
	}

	if (first == last)
		return;
	lastline = last;
	if (osd_skip_this_frame ())
		return;

	/* top part of display not rastered */
	first -= VIC2_YPOS - YPOS;
	last -= VIC2_YPOS - YPOS;
	if ((first >= last) || (last <= 0))
	{
		for (i = 0; i < 8; i++)
			sprites[i].repeat = sprites[i].line = 0;
		return;
	}
	if (first < 0)
		first = 0;

	if (!SCREENON)
	{
		if (Machine->color_depth == 8)
		{
			for (line = first; (line < last) && (line < vic2_bitmap->height); line++)
				memset (vic2_bitmap->line[line], Machine->pens[0], vic2_bitmap->width);
		}
		else
		{
			for (line = first; (line < last) && (line < vic2_bitmap->height); line++)
				memset16 (vic2_bitmap->line[line], Machine->pens[0], vic2_bitmap->width);
		}
		return;
	}

	if (COLUMNS40)
		xbegin = XPOS, xend = xbegin + 320;
	else
		xbegin = XPOS + 7, xend = xbegin + 304;

	if (last < y_begin)
		end = last;
	else
		end = y_begin + YPOS;
	if (Machine->color_depth == 8)
	{
		for (line = first; line < end; line++)
			memset (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					vic2_bitmap->width);
	}
	else
	{
		for (line = first; line < end; line++)
			memset16 (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					  vic2_bitmap->width);
	}
	if (LINES25)
	{
		vline = line - y_begin - YPOS;
	}
	else
	{
		vline = line - y_begin - YPOS + 8 - VERTICALPOS;
	}
	if (last < y_end + YPOS)
		end = last;
	else
		end = y_end + YPOS;
	for (; line < end; vline = (vline + 8) & ~7, line = line + 1 + yend - ybegin)
	{
		offs = (vline >> 3) * 40;
		ybegin = vline & 7;
		yoff = line - ybegin;
		yend = (yoff + 7 < end) ? 7 : (end - yoff - 1);
		/* rendering 39 characters */
		/* left and right borders are overwritten later */
		vic2_shift[line] = HORICONTALPOS;

		for (xoff = x_begin + XPOS; xoff < x_end + XPOS; xoff += 8, offs++)
		{
			ch = vic_dma_read (videoaddr + offs);
			attr = vic_dma_read_color (videoaddr + offs);
			if (HIRESON)
			{
				bitmapmulti[1] = c64_bitmap[1] = Machine->pens[ch >> 4];
				bitmapmulti[2] = c64_bitmap[0] = Machine->pens[ch & 0xf];
				if (MULTICOLORON)
				{
					bitmapmulti[3] = Machine->pens[attr];
					vic2_draw_bitmap_multi (ybegin, yend, offs, yoff, xoff);
				}
				else
				{
					vic2_draw_bitmap (ybegin, yend, offs, yoff, xoff);
				}
			}
			else if (ECMON)
			{
				ecm = ch >> 6;
				ecmcolor[0] = colors[ecm];
				ecmcolor[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch & ~0xC0, yoff, xoff, ecmcolor);
			}
			else if (MULTICOLORON && (attr & 8))
			{
				multi[3] = Machine->pens[attr & 7];
				vic2_draw_character_multi (ybegin, yend, ch, yoff, xoff);
			}
			else
			{
				mono[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch, yoff, xoff, mono);
			}
		}
		/* sprite priority, sprite overwrites lowerprior pixels */
		for (i = 7; i >= 0; i--)
		{
			if (sprites[i].line || sprites[i].repeat)
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if ((21 - sprites[i].line) * 2 - sprites[i].repeat < yend - ybegin + 1)
						syend = ybegin + (21 - sprites[i].line) * 2 - sprites[i].repeat - 1;
				}
				else
				{
					if (sprites[i].line + yend - ybegin + 1 > 20)
						syend = ybegin + 20 - sprites[i].line;
				}
				if (yoff + syend > YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, ybegin, syend);
				else
					vic2_draw_sprite (i, yoff, ybegin, syend);
				if ((syend != yend) || (sprites[i].line > 20))
				{
					sprites[i].line = sprites[i].repeat = 0;
					for (j = syend; j <= yend; j++)
						sprites[i].paintedline[j] = 0;
				}
			}
			else if (SPRITEON (i) && (yoff + ybegin <= SPRITE_Y_POS (i))
					 && (yoff + yend >= SPRITE_Y_POS (i)))
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if (21 * 2 < yend - ybegin + 1)
						syend = ybegin + 21 * 2 - 1;
				}
				else
				{
					if (yend - ybegin + 1 > 21)
						syend = ybegin + 21 - 1;
				}
				if (yoff + syend >= YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				for (j = 0; j < SPRITE_Y_POS (i) - yoff; j++)
					sprites[i].paintedline[j] = 0;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				else
					vic2_draw_sprite (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				if ((syend != yend) || (sprites[i].line > 20))
				{
					for (j = syend; j <= yend; j++)
						sprites[i].paintedline[j] = 0;
					sprites[i].line = sprites[i].repeat = 0;
				}
			}
			else
			{
				memset (sprites[i].paintedline, 0, sizeof (sprites[i].paintedline));
			}
		}
		if (Machine->color_depth == 8)
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset (vic2_bitmap->line[yoff + i], Machine->pens[FRAMECOLOR], xbegin);
				memset (vic2_bitmap->line[yoff + i] + xend, Machine->pens[FRAMECOLOR],
						vic2_bitmap->width - xend);
			}
		}
		else
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset16 (vic2_bitmap->line[yoff + i], Machine->pens[FRAMECOLOR],
						  xbegin);
				memset16 ((short *) vic2_bitmap->line[yoff + i] + xend,
						  Machine->pens[FRAMECOLOR], vic2_bitmap->width - xend);
			}
		}
	}
	if (last < vic2_bitmap->height)
		end = last;
	else
		end = vic2_bitmap->height;
	if (Machine->color_depth == 8)
	{
		for (; line < end; line++)
			memset (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					vic2_bitmap->width);
	}
	else
	{
		for (; line < end; line++)
			memset16 (vic2_bitmap->line[line], Machine->pens[FRAMECOLOR],
					  vic2_bitmap->width);
	}
}

static void vic2_draw_text (struct osd_bitmap *bitmap, char *text, int *y)
{
	int x, x0, y2, width = (Machine->gamedrv->drv->visible_area.max_x -
							Machine->gamedrv->drv->visible_area.min_x) / Machine->uifont->width;

	if (text[0] != 0)
	{
		x = strlen (text);
		*y -= Machine->uifont->height * ((x + width - 1) / width);
		y2 = *y + Machine->uifont->height;
		x = 0;
		while (text[x])
		{
			for (x0 = Machine->gamedrv->drv->visible_area.min_x;
				 text[x] && (x0 < Machine->gamedrv->drv->visible_area.max_x -
							 Machine->uifont->width);
				 x++, x0 += Machine->uifont->width)
			{
				drawgfx (bitmap, Machine->uifont, text[x], 0, 0, 0, x0, y2, 0,
						 TRANSPARENCY_NONE, 0);
			}
			y2 += Machine->uifont->height;
		}
	}
}

int vic2_raster_irq (void)
{
	int i, y;

	rasterline++;
	if (rasterline >= lines)
	{
		rasterline = 0;
		vic2_drawlines (lastline, lines);
		for (i = 0; i < 8; i++)
			sprites[i].repeat = sprites[i].line = 0;
		lastline = 0;
		if (LIGHTPEN_BUTTON)
		{
			double time = 0.0;

			/* lightpen timer starten */
			lightpentimer = timer_set (time, 1, vic2_timer_timeout);
		}
		{
			char text[70];

			y = Machine->gamedrv->drv->visible_area.max_y + 1 - Machine->uifont->height;

#if 1
			cia6526_status (text, sizeof (text));
			vic2_draw_text (Machine->scrbitmap, text, &y);

			c64_status (text, sizeof (text));
			vic2_draw_text (Machine->scrbitmap, text, &y);
#endif
#if VERBOSE_DBG
			snprintf (text, sizeof (text), "vic chargen:%.4x video:%.4x %s %s %s",
					  chargenaddr, videoaddr, HIRESON ? "hires" : "",
					  ECMON ? "ecm" : "", MULTICOLORON ? "multi" : "");
			vic2_draw_text (Machine->scrbitmap, text, &y);
#if 0
			snprintf (text, sizeof (text), "on:%.2x multi:%.2x sp:%.2x bg:%.2x",
					  vic2[0x15], vic2[0x1c], vic2[0x1e], vic2[0x1f]);
			vic2_draw_text (Machine->scrbitmap, text, &y);
			snprintf (text, sizeof (text), "enable:%.2x occured:%.2x",
					  vic2[0x1a], vic2[0x19]);
			vic2_draw_text (Machine->scrbitmap, text, &y);
#endif
#endif

			vc20_tape_status (text, sizeof (text));
			vic2_draw_text (Machine->scrbitmap, text, &y);
#ifdef VC1541
			vc1541_drive_status (text, sizeof (text));
#else
			cbm_drive_0_status (text, sizeof (text));
#endif
			vic2_draw_text (Machine->scrbitmap, text, &y);

			cbm_drive_1_status (text, sizeof (text));
			vic2_draw_text (Machine->scrbitmap, text, &y);
		}
	}
	if (rasterline == C64_2_RASTERLINE (RASTERLINE))
	{
		vic2_drawlines (lastline, rasterline);
		vic2_set_interrupt (1);
	}
	return 0;
}

void vic2_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
}
