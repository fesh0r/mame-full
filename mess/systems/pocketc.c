/******************************************************************************
 sharp pocket computers
 pc1401/pc1403
 Peter.Trauner@jk.uni-linz.ac.at May 2000
******************************************************************************/

#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"


/* pc1430 no peek poke operations! */

/* pc1280?? */

/* pc126x
   port
   1 ?
   2 +6v
   3 gnd
   4 f0
   5 f1
   6 load
   7 save
   8 ib7
   9 ib6
  10 ib5
  11 ib4 */

/* pc1350 other keyboard,
   f2 instead f0 at port
   port
   1 ?
   2 +6v
   3 gnd
   4 f2
   5 f1
   6 load
   7 save
   8 ib7
   9 ib6
  10 ib5
  11 ib4
*/

/* similar computers
   pc1260/1261
   pc1402/1403
   pc1421 */
/* pc140x
   a,b0..b5 keyboard matrix
   b0 off key
   c0 display on
   c1 counter reset
   c2 cpu halt
   c3 computer off
   c4 beeper frequency (1 4khz, 0 2khz), or (c5=0) membran pos1/pos2
   c5 beeper on
   c6 beeper steuerung

   port
   1 ?
   2 +6v
   3 gnd
   4 f0
   5 f1
   6 load
   7 save
   8 ib7
   9 ib6
  10 ?
  11 ?
*/

/* special keys
   red c-ce and reset; warm boot, program NOT lost*/

static UINT8 *pc1401_mem;

static struct MemoryReadAddress pc1401_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x47ff, MRA_RAM },
/*	{ 0x5000, 0x57ff, ? }, */
	{ 0x6000, 0x67ff, pc1401_lcd_read },
	{ 0x6800, 0x685f, sc61860_read_internal },
	{ 0x7000, 0x77ff, pc1401_lcd_read },
	{ 0x8000, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress pc1401_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM, &pc1401_mem },
/*	{ 0x2000, 0x3fff, MWA_RAM }, // done in pc1401_machine_init */
	{ 0x4000, 0x47ff, MWA_RAM },
/*	{ 0x5000, 0x57ff, ? }, */
	{ 0x6000, 0x67ff, pc1401_lcd_write },
	{ 0x6800, 0x685f, sc61860_write_internal },
	{ 0x7000, 0x77ff, pc1401_lcd_write },
	{ 0x8000, 0xffff, MWA_ROM },

    {-1}
};

#if 0
static struct MemoryReadAddress pc1421_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x3800, 0x47ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress pc1421_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x47ff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },

    {-1}
};

static struct MemoryReadAddress pc1260_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress pc1260_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_RAM }, /* 1261 */
	{ 0x5800, 0x67ff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },

	{ 0x8000, 0xffff, MWA_ROM },
    {-1}
};


static struct MemoryReadAddress pc1350_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress pc1350_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM }, /*ram card 16k */
	{ 0x4000, 0x5fff, MWA_RAM }, /*ram card 16k oder 8k */
	{ 0x6000, 0x6fff, MWA_RAM },

	{ 0x8000, 0xffff, MWA_ROM },
    {-1}
};
#endif

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( pc1401 )
	PORT_START
    PORT_BITX (0x80, 0x00, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
			   "Power",CODE_DEFAULT, CODE_NONE)
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	DIPS_HELPER( 0x40, "CAL", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x20, "BASIC", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x10, "BRK   ON", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x08, "DEF", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x04, "down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x02, "up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x01, "left  DEL", KEYCODE_LEFT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "right INS", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x40, "SHIFT", KEYCODE_LSHIFT, KEYCODE_RSHIFT)
	DIPS_HELPER( 0x20, "Q     !", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x10, "W     \"", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x08, "E     #", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x04, "R     $", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x02, "T     %", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x01, "Y     &", KEYCODE_Y, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "U     ?", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x40, "I     At", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x20, "O     :", KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x10, "P     ;", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x08, "A     INPUT", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x04, "S     IF", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x02, "D     THEN", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x01, "F     GOTO", KEYCODE_F, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "G     FOR", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x40, "H     TO", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x20, "J     STEP", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x10, "K     NEXT", KEYCODE_K, CODE_NONE)
	DIPS_HELPER( 0x08, "L     LIST", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x04, ",     RUN", KEYCODE_COMMA, CODE_NONE)
	DIPS_HELPER( 0x02, "Z     PRINT", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x01, "X     USING", KEYCODE_X, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "C     GOSUB", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x40, "V     RETURN", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x20, "B     DIM", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x10, "N     END", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x08, "M     CSAVE", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x04, "SPC   CLOAD", KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x02, "ENTER N<>NP", KEYCODE_ENTER, CODE_NONE)
	DIPS_HELPER( 0x01, "HYP   ARCHYP", CODE_DEFAULT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "SIN   SIN^-1", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x40, "COS   COS^-1", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x20, "TAN   TAN^-1", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x10, "F<>E  TAB", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x08, "C-CE  CA", KEYCODE_ESC, CODE_NONE)
	DIPS_HELPER( 0x04, "->HEX ->DEC", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x02, "->DEG ->D.MS", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x01, "LN    e^x    E", CODE_DEFAULT, CODE_NONE)
	PORT_START
 	DIPS_HELPER( 0x80, "LOG   10^x   F", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x40, "1/x   ->re", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x20, "chnge STAT", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x10, "EXP   Pi     A", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x08, "y^x   xROOTy B", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x04, "sqrt  3root  C", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x02, "sqr   tri%   D", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x01, "(     ->xy", KEYCODE_8, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, ")     n!", KEYCODE_9, CODE_NONE)
	DIPS_HELPER( 0x40, "7     y mean", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "8     Sy", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "9     sigmay", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "/", KEYCODE_SLASH_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "X->M  Sum y  Sum y^2", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x02, "4     x mean", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "5     Sx", KEYCODE_5_PAD, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "6     sigmax", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "*     <", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x20, "RM    (x,y)", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x10, "1     r", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "2     a", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "3     b", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "-     >", KEYCODE_MINUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "M+    DATA   CD", CODE_DEFAULT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "0     x'", KEYCODE_0_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "+/-   y'", KEYCODE_NUMLOCK, CODE_NONE)
	DIPS_HELPER( 0x20, ".     DRG", KEYCODE_DEL_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "+     ^", KEYCODE_PLUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "=", KEYCODE_ENTER_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Reset", KEYCODE_F3, CODE_NONE)
	PORT_START
    PORT_DIPNAME   ( 0xc0, 0x80, "RAM")
	PORT_DIPSETTING( 0x00, "2KB" )
	PORT_DIPSETTING( 0x40, "PC1401(4KB)" )
	PORT_DIPSETTING( 0x80, "PC1402(10KB)" )
INPUT_PORTS_END

static unsigned char pc1401_palette[] =
{
	0,0,0, /* black */
	0x10,0x10,0x10,
	0x20,0x20,0x20,
	0x30,0x30,0x30,
	0x40,0x40,0x40,
	0x50,0x50,0x50,
	0x60,0x60,0x60,
	0x70,0x70,0x70,
	0x80,0x80,0x80,
	0x90,0x90,0x90,
	0xa0,0xa0,0xa0,
	0xb0,0xb0,0xb0,
	0xc0,0xc0,0xc0,
	0xd0,0xd0,0xe0,
	0xe0,0xe0,0xe0,
	0xf0,0xf0,0xf0,
	0xff,0xff,0xff,
};

static unsigned short pc1401_colortable[] = {
	2, 10,
	5, 15
};

static struct GfxLayout pc1401_charlayout =
{
        2,14,
        128,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0 },
        /* y offsets */
        { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
        },
        1*8
};

static struct GfxDecodeInfo pc1401_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &pc1401_charlayout,                     0, 2 },
	{ REGION_GFX1, 0x0000, &pc1401_charlayout,                     2, 2 },
    { -1 } /* end of array */
};

static void pocketc_init_colors (unsigned char *sys_palette,
								 unsigned short *sys_colortable,
								 const unsigned char *color_prom)
{
	memcpy (sys_palette, pc1401_palette, sizeof (pc1401_palette));
	memcpy(sys_colortable,pc1401_colortable,sizeof(pc1401_colortable));
}

#if 0
static struct DACinterface dac_interface =
{
	1,			/* number of DACs */
	{ 100 } 	/* volume */
};
#endif

static int pocketc_frame_int(void)
{
	return 0;
}

static SC61860_CONFIG config={
	pc1401_reset, pc1401_brk,
	pc1401_ina, pc1401_outa,
	pc1401_inb, pc1401_outb,
	pc1401_outc
};

static struct MachineDriver machine_driver_pc1401 =
{
	/* basic machine hardware */
	{
		{
			CPU_SC61860,
			192000,
			pc1401_readmem,pc1401_writemem,0,0,
			pocketc_frame_int, 1,
			0,0,
			&config
        }
	},
	/* frames per second, VBL duration */
	64, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	pc1401_machine_init,
	pc1401_machine_stop,

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x2 pixel) */

	592, 252, { 0, 592 - 1, 0, 252 - 1},
	pc1401_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pc1401_palette) / sizeof (pc1401_palette[0]) / 3,
	sizeof (pc1401_colortable) / sizeof(pc1401_colortable[0]),
	pocketc_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER/* | VIDEO_SUPPORTS_DIRTY*/,	/* video flags */
	0,						/* obsolete */
    pocketc_vh_start,
	pocketc_vh_stop,
	pc1401_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
        { 0 }
    }
};

ROM_START(pc1401)
	ROM_REGION(0x10000,REGION_CPU1)
	/* SC61860A08 5H 13LD cpu with integrated rom*/
	ROM_LOAD("sc61860.a08", 0x0000, 0x2000, 0x44bee438)
/* 5S1 SC613256 D30
   or SC43536LD 5G 13 (LCD chip?) */
	ROM_LOAD("sc613256.d30", 0x8000, 0x8000, 0x69b9d587)
	ROM_REGION(0x80,REGION_GFX1)
ROM_END

static const struct IODevice io_pc1401[] = {
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */
COMPX( 198?, pc1401,	  0, 		pc1401,  pc1401, 	pc1401,	  "Sharp",  "Pocket Computer 1401/1402", GAME_NOT_WORKING)

