/***************************************************************************

	drivers/pc.c

	PC-XT memory map

	00000-9FFFF   RAM
	A0000-AFFFF   NOP		or videoram EGA/VGA
	B0000-B7FFF   videoram	MDA, page #0
	B8000-BFFFF   videoram	CGA and/or MDA page #1, T1T mapped RAM
	C0000-C7FFF   NOP		or ROM EGA/VGA
	C8000-C9FFF   ROM		XT HDC #1
	CA000-CBFFF   ROM		XT HDC #2
	D0000-EFFFF   NOP		or 'adapter RAM'
	F0000-FDFFF   NOP		or ROM Basic + other Extensions
	FE000-FFFFF   ROM

	TODO:
	think about a concept to split port_r/port_w for the
	different machines (specificially the Tandy1000/PCjr)
	It's now all handled by two single functions in machine/pc.c

***************************************************************************/
#include "mess/machine/pc.h"
#include "driver.h"
#include "sound/3812intf.h"

#define ym3812_StdClock 3579545

#define ADLIB	/* YM3812/OPL2 Chip */

static struct MemoryReadAddress mda_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xaffff, MRA_NOP },
	{ 0xb0000, 0xbffff, MRA_RAM },
	{ 0xc0000, 0xc7fff, MRA_NOP },
	{ 0xc8000, 0xcffff, MRA_ROM },
	{ 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mda_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xaffff, MWA_NOP },
	{ 0xb0000, 0xbffff, pc_mda_videoram_w, &videoram, &videoram_size },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xcffff, MWA_ROM },
	{ 0xd0000, 0xeffff, MWA_NOP },
	{ 0xf0000, 0xfffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort mda_readport[] =
{
	{ 0x0000, 0x000f, pc_DMA_r },
	{ 0x0020, 0x0021, pc_PIC_r },
	{ 0x0040, 0x0043, pc_PIT_r },
	{ 0x0060, 0x0063, pc_PIO_r },
	{ 0x0080, 0x0087, pc_DMA_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x0213, 0x0213, pc_EXP_r },
	{ 0x0278, 0x027b, pc_LPT3_r },
	{ 0x0378, 0x037b, pc_LPT2_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03bd, pc_LPT1_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_FDC_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x03b0, 0x03bf, pc_MDA_r },
    { -1 }
};

static struct IOWritePort mda_writeport[] =
{
	{ 0x0000, 0x000f, pc_DMA_w },
	{ 0x0020, 0x0021, pc_PIC_w },
	{ 0x0040, 0x0043, pc_PIT_w },
	{ 0x0060, 0x0063, pc_PIO_w },
	{ 0x0080, 0x0087, pc_DMA_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0278, 0x027b, pc_LPT3_w },
	{ 0x0378, 0x037b, pc_LPT2_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x03bc, 0x03bd, pc_LPT1_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_FDC_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x03b0, 0x03bf, pc_MDA_w },
    { -1 }
};

static struct MemoryReadAddress cga_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xaffff, MRA_NOP },
	{ 0xb0000, 0xb7fff, MRA_NOP },
	{ 0xb8000, 0xbffff, MRA_RAM },
	{ 0xc0000, 0xc7fff, MRA_NOP },
    { 0xc8000, 0xcffff, MRA_ROM },
    { 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cga_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xaffff, MWA_NOP },
	{ 0xb0000, 0xb7fff, MWA_NOP },
	{ 0xb8000, 0xbffff, pc_cga_videoram_w, &videoram, &videoram_size },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xcffff, MWA_ROM },
    { 0xd0000, 0xeffff, MWA_RAM },
	{ 0xf0000, 0xfffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort cga_readport[] =
{
	{ 0x0000, 0x000f, pc_DMA_r },
	{ 0x0020, 0x0021, pc_PIC_r },
	{ 0x0040, 0x0043, pc_PIT_r },
	{ 0x0060, 0x0063, pc_PIO_r },
	{ 0x0080, 0x0087, pc_DMA_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x0213, 0x0213, pc_EXP_r },
	{ 0x0278, 0x027b, pc_LPT3_r },
	{ 0x0378, 0x037b, pc_LPT2_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03bd, pc_LPT1_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_FDC_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x03d0, 0x03df, pc_CGA_r },
    { -1 }
};

static struct IOWritePort cga_writeport[] =
{
	{ 0x0000, 0x000f, pc_DMA_w },
	{ 0x0020, 0x0021, pc_PIC_w },
	{ 0x0040, 0x0043, pc_PIT_w },
	{ 0x0060, 0x0063, pc_PIO_w },
	{ 0x0080, 0x0087, pc_DMA_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0278, 0x027b, pc_LPT3_w },
	{ 0x0378, 0x037b, pc_LPT2_w },
	{ 0x03bc, 0x03bd, pc_LPT1_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_FDC_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x03d0, 0x03df, pc_CGA_w },
    { -1 }
};

static struct MemoryReadAddress t1t_readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xaffff, MRA_RAM },
	{ 0xb0000, 0xb7fff, MRA_NOP },
	{ 0xb8000, 0xbffff, pc_t1t_videoram_r },
	{ 0xc0000, 0xc7fff, MRA_NOP },
	{ 0xc8000, 0xc9fff, MRA_ROM },
	{ 0xca000, 0xcffff, MRA_NOP },
	{ 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress t1t_writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xaffff, MWA_RAM },
    { 0xb0000, 0xb7fff, MWA_NOP },
	{ 0xb8000, 0xbffff, pc_t1t_videoram_w },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xc9fff, MWA_ROM },
	{ 0xca000, 0xcffff, MWA_NOP },
	{ 0xd0000, 0xeffff, MWA_NOP },
	{ 0xf0000, 0xfffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort t1t_readport[] =
{
	{ 0x0000, 0x000f, pc_DMA_r },
	{ 0x0020, 0x0021, pc_PIC_r },
	{ 0x0040, 0x0043, pc_PIT_r },
	{ 0x0060, 0x0063, pc_PIO_r },
	{ 0x0080, 0x0087, pc_DMA_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x0213, 0x0213, pc_EXP_r },
	{ 0x0378, 0x037f, pc_t1t_p37x_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
    { 0x03bc, 0x03bd, pc_LPT1_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03f0, 0x03f7, pc_FDC_r },
	{ 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x03d0, 0x03df, pc_T1T_r },
    { -1 }
};

static struct IOWritePort t1t_writeport[] =
{
	{ 0x0000, 0x000f, pc_DMA_w },
	{ 0x0020, 0x0021, pc_PIC_w },
	{ 0x0040, 0x0043, pc_PIT_w },
	{ 0x0060, 0x0063, pc_PIO_w },
	{ 0x0080, 0x0087, pc_DMA_page_w },
	{ 0x00c0, 0x00c0, SN76496_0_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0378, 0x037f, pc_t1t_p37x_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
    { 0x03bc, 0x03bd, pc_LPT1_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x03f0, 0x03f7, pc_FDC_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x03d0, 0x03df, pc_T1T_w },
    { -1 }
};

static unsigned char palette[] = {
/*  normal colors */
    0x00,0x00,0x00, 0x00,0x00,0x7f, 0x00,0x7f,0x00, 0x00,0x7f,0x7f,
    0x7f,0x00,0x00, 0x7f,0x00,0x7f, 0x7f,0x7f,0x00, 0x7f,0x7f,0x7f,
/*  light colors */
    0x3f,0x3f,0x3f, 0x00,0x00,0xff, 0x00,0xff,0x00, 0x00,0xff,0xff,
    0xff,0x00,0x00, 0xff,0x00,0xff, 0xff,0xff,0x00, 0xff,0xff,0xff,
/*  black for 640x200 gfx */
    0x00,0x00,0x00
};

INPUT_PORTS_START( pc_mda )
	PORT_START /* IN0 */
	PORT_BIT ( 0x80, 0x80,	 IPT_VBLANK )
	PORT_BIT ( 0x7f, 0x7f,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x30, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x02, "yes" )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Any floppy drive installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x01, "yes" )

    PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x80, "yes" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x40, "yes" )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x20, "yes" )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x10, "yes" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x04, "yes" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x02, "yes" )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x01, "yes" )

	PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, "yes" )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_BIT( 0x03, 0x03,	IPT_UNUSED )

    PORT_START  /* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */

	PORT_START	/* IN5 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */

	PORT_START	/* IN6 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"` ~",          KEYCODE_TILDE,      IP_JOY_NONE ) /* `                           29  A9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"\\ |",         KEYCODE_BACKSLASH,  IP_JOY_NONE ) /* \                           2B  AB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */

	PORT_START	/* IN7 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP * (PrtScr)",KEYCODE_ASTERISK,   IP_JOY_NONE ) /* Keypad *  (PrtSc)           37  B7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */

	PORT_START	/* IN8 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"ScrLock",      KEYCODE_SCRLOCK,    IP_JOY_NONE ) /* Scroll Lock                 46  C6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP 7 (Home)",  KEYCODE_HOME,       IP_JOY_NONE ) /* Keypad 7  (Home)            47  C7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"KP 8 (Up)",    KEYCODE_UP,         IP_JOY_NONE ) /* Keypad 8  (Up arrow)        48  C8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_PGUP,       IP_JOY_NONE ) /* Keypad 9  (PgUp)            49  C9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"KP -",         KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* Keypad -                    4A  CA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"KP 4 (Left)",  KEYCODE_LEFT,       IP_JOY_NONE ) /* Keypad 4  (Left arrow)      4B  CB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"KP 6 (Right)", KEYCODE_RIGHT,      IP_JOY_NONE ) /* Keypad 6  (Right arrow)     4D  CD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"KP +",         KEYCODE_PLUS_PAD,   IP_JOY_NONE ) /* Keypad +                    4E  CE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_END,        IP_JOY_NONE ) /* Keypad 1  (End)             4F  CF */

	PORT_START	/* IN9 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"KP 2 (Down)",  KEYCODE_DOWN,       IP_JOY_NONE ) /* Keypad 2  (Down arrow)      50  D0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_PGDN,       IP_JOY_NONE ) /* Keypad 3  (PgDn)            51  D1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"KP 0 (Ins)",   KEYCODE_INSERT,     IP_JOY_NONE ) /* Keypad 0  (Ins)             52  D2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"KP . (Del)",   KEYCODE_DEL,        IP_JOY_NONE ) /* Keypad .  (Del)             53  D3 */

	PORT_START /* IN10 mouse X */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_X,30,2,0,0,640-1,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)

	PORT_START /* IN11 mouse Y */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_Y,30,2,0,0,480-1,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)

INPUT_PORTS_END

INPUT_PORTS_START( pc_cga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x02, "yes" )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x01, "yes" )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x80, "yes" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x40, "yes" )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x20, "yes" )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x10, "yes" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x04, "yes" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x02, "yes" )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
    PORT_DIPSETTING(    0x01, "yes" )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, "yes" )
    PORT_DIPSETTING(    0x00, "no" )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Fill odd scanlines", 0, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, "no" )
	PORT_DIPSETTING(	0x01, "yes" )

	PORT_START	/* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */

	PORT_START	/* IN5 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */

	PORT_START	/* IN6 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"` ~",          KEYCODE_TILDE,      IP_JOY_NONE ) /* `                           29  A9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"\\ |",         KEYCODE_BACKSLASH,  IP_JOY_NONE ) /* \                           2B  AB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */

	PORT_START	/* IN7 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP * (PrtScr)",KEYCODE_ASTERISK,   IP_JOY_NONE ) /* Keypad *  (PrtSc)           37  B7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */

	PORT_START	/* IN8 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"ScrLock",      KEYCODE_SCRLOCK,    IP_JOY_NONE ) /* Scroll Lock                 46  C6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP 7 (Home)",  KEYCODE_HOME,       IP_JOY_NONE ) /* Keypad 7  (Home)            47  C7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"KP 8 (Up)",    KEYCODE_UP,         IP_JOY_NONE ) /* Keypad 8  (Up arrow)        48  C8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_PGUP,       IP_JOY_NONE ) /* Keypad 9  (PgUp)            49  C9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"KP -",         KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* Keypad -                    4A  CA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"KP 4 (Left)",  KEYCODE_LEFT,       IP_JOY_NONE ) /* Keypad 4  (Left arrow)      4B  CB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"KP 6 (Right)", KEYCODE_RIGHT,      IP_JOY_NONE ) /* Keypad 6  (Right arrow)     4D  CD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"KP +",         KEYCODE_PLUS_PAD,   IP_JOY_NONE ) /* Keypad +                    4E  CE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_END,        IP_JOY_NONE ) /* Keypad 1  (End)             4F  CF */

	PORT_START	/* IN9 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"KP 2 (Down)",  KEYCODE_DOWN,       IP_JOY_NONE ) /* Keypad 2  (Down arrow)      50  D0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_PGDN,       IP_JOY_NONE ) /* Keypad 3  (PgDn)            51  D1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"KP 0 (Ins)",   KEYCODE_INSERT,     IP_JOY_NONE ) /* Keypad 0  (Ins)             52  D2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"KP . (Del)",   KEYCODE_DEL,        IP_JOY_NONE ) /* Keypad .  (Del)             53  D3 */

	PORT_START /* IN10 mouse X */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_X,30,2,0,0,640-1,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)

	PORT_START /* IN11 mouse Y */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_Y,30,2,0,0,480-1,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)

INPUT_PORTS_END

INPUT_PORTS_START( pc_t1t )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x02, "yes" )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x01, "yes" )

    PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x80, "yes" )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x40, "yes" )
	PORT_BIT ( 0x30, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_BIT ( 0x06, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no" )
    PORT_DIPSETTING(    0x01, "yes" )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, "yes" )
	PORT_DIPSETTING(	0x00, "no" )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", 0, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, "yes" )
    PORT_DIPSETTING(    0x00, "no" )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Fill odd scanlines", 0, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, "no" )
    PORT_DIPSETTING(    0x01, "yes" )

	PORT_START	/* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */

	PORT_START	/* IN5 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */

	PORT_START	/* IN6 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"` ~",          KEYCODE_TILDE,      IP_JOY_NONE ) /* `                           29  A9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"\\ |",         KEYCODE_BACKSLASH,  IP_JOY_NONE ) /* \                           2B  AB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */

	PORT_START	/* IN7 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP * (PrtScr)",KEYCODE_ASTERISK,   IP_JOY_NONE ) /* Keypad *  (PrtSc)           37  B7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */

	PORT_START	/* IN8 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"ScrLock",      KEYCODE_SCRLOCK,    IP_JOY_NONE ) /* Scroll Lock                 46  C6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP 7 (Home)",  KEYCODE_HOME,       IP_JOY_NONE ) /* Keypad 7  (Home)            47  C7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"KP 8 (Up)",    KEYCODE_UP,         IP_JOY_NONE ) /* Keypad 8  (Up arrow)        48  C8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_PGUP,       IP_JOY_NONE ) /* Keypad 9  (PgUp)            49  C9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"KP -",         KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* Keypad -                    4A  CA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"KP 4 (Left)",  KEYCODE_LEFT,       IP_JOY_NONE ) /* Keypad 4  (Left arrow)      4B  CB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"KP 6 (Right)", KEYCODE_RIGHT,      IP_JOY_NONE ) /* Keypad 6  (Right arrow)     4D  CD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"KP +",         KEYCODE_PLUS_PAD,   IP_JOY_NONE ) /* Keypad +                    4E  CE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_END,        IP_JOY_NONE ) /* Keypad 1  (End)             4F  CF */

	PORT_START	/* IN9 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"KP 2 (Down)",  KEYCODE_DOWN,       IP_JOY_NONE ) /* Keypad 2  (Down arrow)      50  D0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_PGDN,       IP_JOY_NONE ) /* Keypad 3  (PgDn)            51  D1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"KP 0 (Ins)",   KEYCODE_INSERT,     IP_JOY_NONE ) /* Keypad 0  (Ins)             52  D2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"KP . (Del)",   KEYCODE_DEL,        IP_JOY_NONE ) /* Keypad .  (Del)             53  D3 */

	PORT_START /* IN10 mouse X */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_X,30,2,0,0,640-1,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)

	PORT_START /* IN11 mouse Y */
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_Y,30,2,0,0,480-1,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)

INPUT_PORTS_END

static unsigned i86_address_mask = 0x000fffff;


static struct CustomSound_interface pc_sound_interface = {
	pc_sh_custom_start,
	pc_sh_stop,
	pc_sh_custom_update
};

#if defined(ADLIB)
// irq line not connected to pc on adlib cards (and compatibles)
static void irqhandler(int linestate) {}

static struct YM3812interface ym3812_interface = {
	1,
	ym3812_StdClock, // I hope this is the clock used on the original Adlib Sound card
	{255}, // volume adjustment in relation to speaker and tandy1000 sound neccessary
	{irqhandler}
};
#endif

static struct SN76496interface t1t_sound_interface = {
	1,
	{2386360},
	{255,}
};

static struct GfxLayout pc_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8,
	  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8 },
	8*8 					/* every char takes 8 bytes (upper half) */
};

static struct GfxLayout pc_mda_gfxlayout_1bpp =
{
	8,32,					/* 8 x 32 graphics */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bit planes */
    /* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets (we only use one byte to build the block) */
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	8						/* every code takes 1 byte */
};

static struct GfxDecodeInfo pc_mda_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pc_mda_charlayout,		0, 256 },
	{ 1, 0x1000, &pc_mda_gfxlayout_1bpp,256*2,	 1 },	/* 640x400x1 gfx */
    { -1 } /* end of array */
};

static unsigned short mda_colortable[] = {
     0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 2,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     0, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2, 0,10, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* the two colors for HGC graphics */
     0,10,
};


/* Initialise the mda palette */
static void mda_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,mda_colortable,sizeof(mda_colortable));
}


static struct MachineDriver mda_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			4772720,	/* 4,77 Mhz */
			mda_readmem,mda_writemem,
			mda_readport,mda_writeport,
			pc_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	pc_mda_init_machine,
	pc_shutdown_machine,

	/* video hardware */
	80*9,										/* screen width */
	25*14,										/* screen height */
	{ 0,80*9-1, 0,25*14-1 },					/* visible_area */
	pc_mda_gfxdecodeinfo,						/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(mda_colortable) / sizeof(mda_colortable[0]),
	mda_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	pc_mda_vh_start,
	pc_mda_vh_stop,
	pc_mda_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
	}
};

static struct GfxLayout CGA_charlayout =
{
	8,32,					/* 8 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static struct GfxLayout CGA_charlayout_dw =
{
	16,32,					/* 16 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	/* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
      4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static struct GfxLayout CGA_gfxlayout_1bpp =
{
    8,32,                   /* 8 x 32 graphics */
    256,                    /* 256 codes */
    1,                      /* 1 bit per pixel */
    { 0 },                  /* no bit planes */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static struct GfxLayout CGA_gfxlayout_2bpp =
{
	8,32,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
    { 0,0, 2,2, 4,4, 6,6  },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static unsigned short cga_colortable[] = {
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,
/* the color sets for 1bpp graphics mode */
	 0,16,
/* the color sets for 2bpp graphics mode */
     0, 2, 4, 6,  0,10,12,14,
     0, 3, 5, 7,  0,11,13,15,
};

static struct GfxDecodeInfo CGA_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x0000, &CGA_charlayout_dw,		  0, 256 },   /* double width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,   1 },   /* 640x400x1 gfx */
	{ 1, 0x1000, &CGA_gfxlayout_2bpp, 256*2+1*2,   4 },   /* 320x200x4 gfx */
    { -1 } /* end of array */
};

static struct GfxLayout t1t_gfxlayout_4bpp_160 =
{
	8,32,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,0,0,0,4,4,4,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	1*8 					/* every code takes 1 byte */
};

static struct GfxLayout t1t_gfxlayout_4bpp_320 =
{
	4,32,					/* 4 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,0,4,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	1*8 					/* every code takes 1 byte */
};

static struct GfxLayout t1t_gfxlayout_2bpp_640 =
{
	4,32,					/* 4 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
	{ 0,2,4,6 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static unsigned short t1t_colortable[] = {
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,
/* the color set for 1bpp graphics mode */
	 0,16,
/* the color sets for 2bpp graphics mode */
     0, 2, 4, 6,  0,10,12,14,
     0, 3, 5, 7,  0,11,13,15,
/* the color set for 4bpp graphics mode */
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
};

static struct GfxDecodeInfo t1t_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,		  0,			256 },	/* single width */
	{ 1, 0x0000, &CGA_charlayout_dw,	  0,			256 },	/* double width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,		  1 },	/* 640x400 1bpp gfx */
    { 1, 0x1000, &CGA_gfxlayout_2bpp,     256*2+1*2,      4 },  /* 320x200 2bpp gfx */
	{ 1, 0x1000, &t1t_gfxlayout_4bpp_160, 256*2+1*2+4*4, 16 },	/* 160x200 4bpp gfx */
	{ 1, 0x1000, &t1t_gfxlayout_4bpp_320, 256*2+1*2+4*4, 16 },	/* 320x200 4bpp gfx */
    { 1, 0x1000, &t1t_gfxlayout_2bpp_640, 256*2+1*2,      4 },  /* 640x200 2bpp gfx */
    { -1 } /* end of array */
};





/* Initialise the cga palette */
static void cga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
}
/* Initialise the t1t palette */
static void t1t_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,t1t_colortable,sizeof(t1t_colortable));
}


static struct MachineDriver cga_machine_driver =
{
    /* basic machine hardware */
    {
        {
            CPU_I86,
			4772720,	/* 4,77 Mhz */
			cga_readmem,cga_writemem,
			cga_readport,cga_writeport,
			pc_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_cga_init_machine,
    pc_shutdown_machine,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	pc_cga_vh_start,
	pc_cga_vh_stop,
	pc_cga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
	}
};

static struct MachineDriver t1t_machine_driver =
{
    /* basic machine hardware */
    {
        {
            CPU_I86,
			4772720,	/* 4,77 Mhz */
			t1t_readmem,t1t_writemem,
			t1t_readport,t1t_writeport,
			pc_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_t1t_init_machine,
    pc_shutdown_machine,

    /* video hardware */
	80*8,										/* screen width */
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	t1t_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(t1t_colortable) / sizeof(t1t_colortable[0]),
	t1t_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	pc_t1t_vh_start,
	pc_t1t_vh_stop,
	pc_t1t_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface }, // is this available on a Tandy ?
        	{ SOUND_SN76496, &t1t_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/



struct GameDriver pc_driver =
{
    __FILE__,
    0,
    "pc",
    "IBM PC/XT - parent driver",
	"1983",
    "IBM",
    "Juergen Buchmueller",
	0,
    &mda_machine_driver,
	0,

	NULL,						/* rom module */
	NULL,						/* load rom_file images */
	NULL,						/* identify rom images */
	0,						/* default file extensions */
    1,                          /* number of ROM slots */
    4,                          /* number of floppy drives supported */
    0,                          /* number of hard drives supported */
    1,                          /* number of cassette drives supported */
	NULL,						/* rom decoder */
	NULL,						/* opcode decoder */
	NULL,						/* pointer to sample names */
	NULL,						/* sound_prom */

	NULL,						/* input ports */

    0,                          /* color_prom */
    0,                          /* color palette */
    0,                          /* color lookup table */

    GAME_COMPUTER|ORIENTATION_DEFAULT,        /* orientation */

    0,                          /* hiscore load */
    0,                          /* hiscore save */
};

ROM_START(pc_mda)
	ROM_REGIONX(0x100000,REGION_CPU1)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)

	ROM_REGIONX(0x1100,REGION_GFX1)
	ROM_LOAD("mda.chr",     0x00000, 0x01000, 0xac1686f3)
ROM_END

struct GameDriver pcmda_driver =
{
	__FILE__,
	&pc_driver,
	"pcmda",
	"IBM PC/XT - MDA",
	"1983",
	"IBM",
	0,
	0,
	&mda_machine_driver,
	pc_init_driver,

	rom_pc_mda, 			/* rom module */
	pc_rom_load,			/* load rom_file images */
	pc_rom_id,				/* identify rom images */
	0,						/* default file extensions */
	1,						/* number of ROM slots */
	4,						/* number of floppy drives supported */
	2,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports_pc_mda, 	/* input ports */

	0,						/* color_prom */
	0,				/* color palette */
	0, 		/* color lookup table */

	GAME_COMPUTER|ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};


ROM_START(CGA)
	ROM_REGIONX(0x100000,REGION_CPU1)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)

	ROM_REGIONX(0x1100,REGION_GFX1)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

struct GameDriver pccga_driver =
{
	__FILE__,
	&pc_driver,
	"pccga",
	"IBM PC/XT - CGA",
	"1983",
	"IBM",
	0,
	0,
    &cga_machine_driver,
	pc_init_driver,

	rom_CGA,				/* rom module */
	pc_rom_load,			/* load rom_file images */
	pc_rom_id,				/* identify rom images */
	0,						/* default file extensions */
	1,						/* number of ROM slots */
	4,						/* number of floppy drives supported */
	2,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports_pc_cga,

	0,						/* color_prom */
	0,				/* color palette */
	0, 		/* color lookup table */

	GAME_COMPUTER|ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};

ROM_START(t1t)
	ROM_REGIONX(0x100000,REGION_CPU1)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)

	ROM_REGIONX(0x1100,REGION_GFX1)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

struct GameDriver tandy1t_driver =
{
	__FILE__,
	&pc_driver,
	"tandy1t",
	"Tandy 1000TX",
	"1987",
	"Tandy Radio Shack",
	0,
	0,
	&t1t_machine_driver,
	pc_init_driver,

	rom_t1t,				/* rom module */
	pc_rom_load,			/* load rom_file images */
	pc_rom_id,				/* identify rom images */
	0,						/* default file extensions */
	1,						/* number of ROM slots */
	4,						/* number of floppy drives supported */
	2,						/* number of hard drives supported */
	0,						/* number of cassette drives supported */
	0,						/* rom decoder */
    0,                      /* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports_pc_t1t,

	0,						/* color_prom */
	0,				/* color palette */
	0, 		/* color lookup table */

	GAME_COMPUTER|GAME_NOT_WORKING|GAME_IMPERFECT_COLORS|ORIENTATION_DEFAULT,	 /* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};



