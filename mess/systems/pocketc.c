/******************************************************************************
 sharp pocket computers
 pc1401/pc1403
 Peter.Trauner@jk.uni-linz.ac.at May 2000
******************************************************************************/

#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1251.h"
#include "includes/pc1350.h"


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

static MEMORY_READ_START( pc1401_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x47ff, MRA_RAM },
/*	{ 0x5000, 0x57ff, ? }, */
	{ 0x6000, 0x67ff, pc1401_lcd_read },
//	{ 0x6800, 0x685f, sc61860_read_internal },
	{ 0x7000, 0x77ff, pc1401_lcd_read },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1401_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
/*	{ 0x2000, 0x3fff, MWA_RAM }, // done in pc1401_machine_init */
	{ 0x4000, 0x47ff, MWA_RAM },
/*	{ 0x5000, 0x57ff, ? }, */
	{ 0x6000, 0x67ff, pc1401_lcd_write },
//	{ 0x6800, 0x685f, sc61860_write_internal },
	{ 0x7000, 0x77ff, pc1401_lcd_write },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pc1251_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
//	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xcbff, MRA_ROM },
	{ 0xf800, 0xf8ff, pc1251_lcd_read },
//	{ 0xff00, 0xff5f, sc61860_read_internal },
MEMORY_END

static MEMORY_WRITE_START( pc1251_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_ROM },
//	{ 0xa000, 0xcbff, MWA_ROM }, // c600 b800 b000 a000 tested
	{ 0xf800, 0xf8ff, pc1251_lcd_write },
//	{ 0xff00, 0xff5f, sc61860_write_internal },
MEMORY_END

static MEMORY_READ_START( pc1350_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x7eff, pc1350_lcd_read },
//	{ 0x7f00, 0x7f5f, sc61860_read_internal },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1350_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM }, /*ram card 16k */
	{ 0x4000, 0x5fff, MWA_RAM }, /*ram card 16k oder 8k */
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7000, 0x7eff, pc1350_lcd_write },
//	{ 0x7f00, 0x7f5f, sc61860_write_internal },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

#if 0
static MEMORY_READ_START( pc1421_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x3800, 0x47ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1421_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x47ff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pc1260_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1260_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_RAM }, /* 1261 */
	{ 0x5800, 0x67ff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },

	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END
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
	DIPS_HELPER( 0x10, "BRK   ON", KEYCODE_F4, CODE_NONE)
	DIPS_HELPER( 0x08, "DEF", KEYCODE_LALT, KEYCODE_RALT)
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
    PORT_DIPNAME   ( 7, 2, "Contrast")
	PORT_DIPSETTING( 0, "0/Low" )
	PORT_DIPSETTING( 1, "1" )
	PORT_DIPSETTING( 2, "2" )
	PORT_DIPSETTING( 3, "3" )
	PORT_DIPSETTING( 4, "4" )
	PORT_DIPSETTING( 5, "5" )
	PORT_DIPSETTING( 6, "6" )
	PORT_DIPSETTING( 7, "7/High" )
INPUT_PORTS_END

INPUT_PORTS_START( pc1251 )
	PORT_START
    PORT_DIPNAME   ( 7, 0, "Mode")
	PORT_DIPSETTING( 4, DEF_STR(Off) )
	PORT_DIPSETTING( 0, "On/RUN" )
	PORT_DIPSETTING( 2, "On/PRO" )
	PORT_DIPSETTING( 1, "On/RSV" )
	DIPS_HELPER( 0x100, "DEF", KEYCODE_LALT, KEYCODE_RALT)
	DIPS_HELPER( 0x80, "SHIFT", KEYCODE_LSHIFT, KEYCODE_RSHIFT)
	DIPS_HELPER( 0x40, "DOWN  (", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x20, "UP    )", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "LEFT  DEL", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x08, "RIGHT INS", KEYCODE_RIGHT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "BRK   ON", KEYCODE_F4, CODE_NONE)
	DIPS_HELPER( 0x40, "Q     !", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x20, "W     \"", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x10, "E     #", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x08, "R     $", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x04, "T     %", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x02, "Y     &", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x01, "U     ?", KEYCODE_U, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "I     :", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x40, "O     ,", KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x20, "P     ;", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x10, "A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x08, "S", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x04, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x02, "F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x01, "G", KEYCODE_G, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "H", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x40, "J", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x20, "K", KEYCODE_K, CODE_NONE)
	DIPS_HELPER( 0x10, "L", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x08, "=", KEYCODE_QUOTE, CODE_NONE)
	DIPS_HELPER( 0x04, "Z", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x02, "X", KEYCODE_X, CODE_NONE)
	DIPS_HELPER( 0x01, "C", KEYCODE_C, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "V", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x40, "B", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x20, "N", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x10, "M", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x08, "      SPC", KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x04, "ENTER N<>NP", KEYCODE_ENTER, KEYCODE_ENTER_PAD)
	DIPS_HELPER( 0x02, "7", KEYCODE_7, KEYCODE_7_PAD)
	DIPS_HELPER( 0x01, "8", KEYCODE_8, KEYCODE_8_PAD)
	PORT_START
	DIPS_HELPER( 0x80, "9", KEYCODE_9, KEYCODE_9_PAD)
	DIPS_HELPER( 0x40, "CL    CA", KEYCODE_ESC, CODE_NONE)
	DIPS_HELPER( 0x20, "4", KEYCODE_4, KEYCODE_4_PAD)
	DIPS_HELPER( 0x10, "5", KEYCODE_5, KEYCODE_5_PAD)
	DIPS_HELPER( 0x08, "6", KEYCODE_6, KEYCODE_6_PAD)
	DIPS_HELPER( 0x04, "/     ^", KEYCODE_SLASH, KEYCODE_SLASH_PAD)
	DIPS_HELPER( 0x02, "1", KEYCODE_1, KEYCODE_1_PAD)
	DIPS_HELPER( 0x01, "2", KEYCODE_2, KEYCODE_2_PAD)
	PORT_START
 	DIPS_HELPER( 0x80, "3     @", KEYCODE_3, KEYCODE_3_PAD)
	DIPS_HELPER( 0x40, "*     <", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x20, "0     Pi", KEYCODE_0, KEYCODE_0_PAD)
	DIPS_HELPER( 0x10, ".     SquareRoot", KEYCODE_STOP, KEYCODE_DEL_PAD)
	DIPS_HELPER( 0x08, "+     Exp", KEYCODE_PLUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "-     >", KEYCODE_MINUS, KEYCODE_MINUS_PAD)
	PORT_START
#if 0
    PORT_DIPNAME   ( 0xc0, 0xc0, "RAM")
	PORT_DIPSETTING( 0x00, "1KB" )
	PORT_DIPSETTING( 0x40, "3.5KB" )
	PORT_DIPSETTING( 0x80, "6KB" )
	PORT_DIPSETTING( 0xc0, "11KB" )
#endif
    PORT_DIPNAME   ( 7, 2, "Contrast")
	PORT_DIPSETTING( 0, "0/Low" )
	PORT_DIPSETTING( 1, "1" )
	PORT_DIPSETTING( 2, "2" )
	PORT_DIPSETTING( 3, "3" )
	PORT_DIPSETTING( 4, "4" )
	PORT_DIPSETTING( 5, "5" )
	PORT_DIPSETTING( 6, "6" )
	PORT_DIPSETTING( 7, "7/High" )
INPUT_PORTS_END

INPUT_PORTS_START( pc1350 )
	PORT_START
    PORT_BITX (0x80, 0x00, IPT_DIPSWITCH_NAME|IPF_TOGGLE,
			   "Power",CODE_DEFAULT, CODE_NONE)
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	DIPS_HELPER( 0x40, "DOWN", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x20, "UP", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "MODE", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x08, "CLS   CA", KEYCODE_ESC, CODE_NONE)
	DIPS_HELPER( 0x04, "LEFT", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "RIGHT", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x01, "DEL", KEYCODE_DEL, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "INS", KEYCODE_INSERT, CODE_NONE)
	DIPS_HELPER( 0x40, "BRK   ON", KEYCODE_F4, CODE_NONE)
	DIPS_HELPER( 0x20, "SHIFT", KEYCODE_LSHIFT, CODE_NONE)
	DIPS_HELPER( 0x10, "7", KEYCODE_7, KEYCODE_7_PAD)
	DIPS_HELPER( 0x08, "8", KEYCODE_8, KEYCODE_8_PAD)
	DIPS_HELPER( 0x04, "9", KEYCODE_9, KEYCODE_9_PAD)
	DIPS_HELPER( 0x02, "(     <", KEYCODE_OPENBRACE, CODE_NONE)
	DIPS_HELPER( 0x01, ")     >", KEYCODE_CLOSEBRACE, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "4", KEYCODE_4, KEYCODE_4_PAD)
	DIPS_HELPER( 0x40, "5", KEYCODE_5, KEYCODE_5_PAD)
	DIPS_HELPER( 0x20, "6", KEYCODE_6, KEYCODE_6_PAD)
	DIPS_HELPER( 0x10, "/", KEYCODE_SLASH, KEYCODE_SLASH_PAD)
	DIPS_HELPER( 0x08, ":", KEYCODE_COLON, CODE_NONE)
	DIPS_HELPER( 0x04, "1", KEYCODE_1, KEYCODE_1_PAD)
	DIPS_HELPER( 0x02, "2", KEYCODE_2, KEYCODE_2_PAD)
	DIPS_HELPER( 0x01, "3", KEYCODE_3, KEYCODE_3_PAD)
	PORT_START
	DIPS_HELPER( 0x80, "*", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x40, ";", KEYCODE_QUOTE, CODE_NONE)
	DIPS_HELPER( 0x20, "0", KEYCODE_0, KEYCODE_0_PAD)
	DIPS_HELPER( 0x10, ".", KEYCODE_STOP, CODE_NONE)
	DIPS_HELPER( 0x08, "+", CODE_NONE, KEYCODE_PLUS_PAD)
	DIPS_HELPER( 0x04, "-     ^", KEYCODE_MINUS, KEYCODE_MINUS_PAD)
	DIPS_HELPER( 0x02, ",", KEYCODE_COMMA, CODE_NONE)
	DIPS_HELPER( 0x01, "SHIFT", KEYCODE_LSHIFT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Q     !", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x40, "W     \"", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x20, "E     #", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x10, "R     $", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x08, "T     %", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x04, "Y     &", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x02, "U     ?", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x01, "I     Pi",KEYCODE_I, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "O     Squareroot", KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x40, "P     Alpha", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x20, "DEF", KEYCODE_LALT, KEYCODE_RALT)
	DIPS_HELPER( 0x10, "A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x08, "S", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x04, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x02, "F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x01, "G", KEYCODE_G, CODE_NONE)
	PORT_START
 	DIPS_HELPER( 0x80, "H", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x40, "J", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x20, "K", KEYCODE_K, CODE_NONE)
	DIPS_HELPER( 0x10, "L", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x08, "=", KEYCODE_EQUALS, CODE_NONE)
	DIPS_HELPER( 0x04, "SML", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
	DIPS_HELPER( 0x02, "Z", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x01, "X", KEYCODE_X, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x40, "V", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x20, "B", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x10, "N", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x08, "M", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x04, "SPC", KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x02, "ENTER P<->NP", KEYCODE_ENTER, KEYCODE_ENTER_PAD)
	PORT_START
    PORT_DIPNAME   ( 0xc0, 0x80, "RAM")
	PORT_DIPSETTING( 0x00, "4KB" )
	PORT_DIPSETTING( 0x40, "12KB" )
	PORT_DIPSETTING( 0x80, "20KB" )
    PORT_DIPNAME   ( 7, 2, "Contrast")
	PORT_DIPSETTING( 0, "0/Low" )
	PORT_DIPSETTING( 1, "1" )
	PORT_DIPSETTING( 2, "2" )
	PORT_DIPSETTING( 3, "3" )
	PORT_DIPSETTING( 4, "4" )
	PORT_DIPSETTING( 5, "5" )
	PORT_DIPSETTING( 6, "6" )
	PORT_DIPSETTING( 7, "7/High" )
INPUT_PORTS_END

static struct GfxLayout pc1401_charlayout =
{
        2,21,
        128,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,0 },
        /* y offsets */
        {
			7, 7, 7,
			6, 6, 6,
			5, 5, 5,
			4, 4, 4,
			3, 3, 3,
			2, 2, 2,
			1, 1, 1
        },
        1*8
};

static struct GfxLayout pc1251_charlayout =
{
        3,21,
        128,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0, },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,0,0 },
        /* y offsets */
        {
			7, 7, 7,
			6, 6, 6,
			5, 5, 5,
			4, 4, 4,
			3, 3, 3,
			2, 2, 2,
			1, 1, 1
        },
        1*8
};

static struct GfxLayout pc1350_charlayout =
{
        2,16,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0 },
        /* y offsets */
        {
			7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0
        },
        1*8
};

static struct GfxDecodeInfo pc1401_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &pc1401_charlayout,                     0, 8 },
    { -1 } /* end of array */
};

static struct GfxDecodeInfo pc1251_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &pc1251_charlayout,                     0, 8 },
    { -1 } /* end of array */
};

static struct GfxDecodeInfo pc1350_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &pc1350_charlayout,                     0, 8 },
    { -1 } /* end of array */
};

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

struct DACinterface pocketc_sound_interface =
{
        1,
        {25}
};

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
	20, DEFAULT_60HZ_VBLANK_DURATION, // very early and slow lcd
	1,				/* single CPU */
	pc1401_machine_init,
	pc1401_machine_stop,

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x3 pixel)
	   it seams to have 3/4 ratio in the real pc1401 */

	594, 273, { 0, 594 - 1, 0, 273 - 1},
//	640, 273, { 0, 640 - 1, 0, 273 - 1},
	pc1401_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pocketc_palette) / sizeof (pocketc_palette[0]) ,
	sizeof (pocketc_colortable) / sizeof(pocketc_colortable[0][0]),
	pocketc_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER| VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
    pocketc_vh_start,
	pocketc_vh_stop,
	pc1401_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
//		{SOUND_DAC, &pocketc_sound_interface},
        { 0 }
    }
};

static SC61860_CONFIG pc1251_config={
	NULL, pc1251_brk,
	pc1251_ina, pc1251_outa,
	pc1251_inb, pc1251_outb,
	pc1251_outc
};

static struct MachineDriver machine_driver_pc1251 =
{
	/* basic machine hardware */
	{
		{
			CPU_SC61860,
			192000,
			pc1251_readmem,pc1251_writemem,0,0,
			pocketc_frame_int, 1,
			0,0,
			&pc1251_config
        }
	},
	/* frames per second, VBL duration */
	20, DEFAULT_60HZ_VBLANK_DURATION, // very early and slow lcd
	1,				/* single CPU */
	pc1251_machine_init,
	pc1251_machine_stop,

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 3x3 pixel) */

	608, 300, { 0, 608 - 1, 0, 300 - 1},
//	640, 334, { 0, 640 - 1, 0, 334 - 1},
	pc1251_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pocketc_palette) / sizeof (pocketc_palette[0]) ,
	sizeof (pocketc_colortable) / sizeof(pocketc_colortable[0][0]),
	pocketc_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER| VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
    pocketc_vh_start,
	pocketc_vh_stop,
	pc1251_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
        { 0 }
    }
};

static SC61860_CONFIG pc1350_config={
	NULL, pc1350_brk,
	pc1350_ina, pc1350_outa,
	pc1350_inb, pc1350_outb,
	pc1350_outc
};

static struct MachineDriver machine_driver_pc1350 =
{
	/* basic machine hardware */
	{
		{
			CPU_SC61860,
			192000,
			pc1350_readmem,pc1350_writemem,0,0,
			pocketc_frame_int, 1,
			0,0,
			&pc1350_config
        }
	},
	/* frames per second, VBL duration */
	20, DEFAULT_60HZ_VBLANK_DURATION, // very early and slow lcd
	1,				/* single CPU */
	pc1350_machine_init,
	pc1350_machine_stop,

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x2 pixel) */

	640, 252, { 0, 640 - 1, 0, 252 - 1},
//	640, 255, { 0, 640 - 1, 0, 255 - 1},
	pc1350_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pocketc_palette) / sizeof (pocketc_palette[0]) ,
	sizeof (pocketc_colortable) / sizeof(pocketc_colortable[0][0]),
	pocketc_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER| VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
    pocketc_vh_start,
	pocketc_vh_stop,
	pc1350_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
        { 0 }
    }
};

ROM_START(pc1401)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* SC61860A08 5H 13LD cpu with integrated rom*/
	ROM_LOAD("sc61860.a08", 0x0000, 0x2000, 0x44bee438)
/* 5S1 SC613256 D30
   or SC43536LD 5G 13 (LCD chip?) */
	ROM_LOAD("sc613256.d30", 0x8000, 0x8000, 0x69b9d587)
	ROM_REGION(0x80,REGION_GFX1,0)
ROM_END

#define rom_pc1402 rom_pc1401

ROM_START(pc1251)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* sc61860a13 6c 13 ld */
	ROM_LOAD("cpu1251.rom", 0x0000, 0x2000, 0xf7287aca)
	ROM_LOAD("bas1251.rom", 0x4000, 0x4000, 0x93ecb629)
	ROM_REGION(0x80,REGION_GFX1,0)
ROM_END

ROM_START(pc1350)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* sc61860a13 6c 13 ld */
	ROM_LOAD("cpu.rom", 0x0000, 0x2000, 0x79a924bc)
	ROM_LOAD("basic.rom", 0x8000, 0x8000, 0x158b28e2)
	ROM_REGION(0x100,REGION_GFX1,0)
ROM_END


static const struct IODevice io_pc1401[] = {
//	IO_CASSETTE_WAVE(1,"wav\0",mycas_id,mycas_init,mycas_exit),
    { IO_END }
};

#define io_pc1402 io_pc1401
#define io_pc1350 io_pc1401
#define io_pc1251 io_pc1401

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */
COMPX( 198?, pc1251,	  0, 		pc1251,  pc1251, 	pc1251,	  "Sharp",  "Pocket Computer 1251", GAME_NOT_WORKING)
COMPX( 198?, pc1350,	  0, 		pc1350,  pc1350, 	pc1350,	  "Sharp",  "Pocket Computer 1350", GAME_NOT_WORKING)
COMPX( 198?, pc1401,	  0, 		pc1401,  pc1401, 	pc1401,	  "Sharp",  "Pocket Computer 1401", GAME_NOT_WORKING)
COMPX( 198?, pc1402,	  pc1401, 	pc1401,  pc1401, 	pc1401,	  "Sharp",  "Pocket Computer 1402", GAME_ALIAS|GAME_NOT_WORKING)

#ifdef RUNTIME_LOADER
extern void pocketc_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"pc1251")==0) drivers[i]=&driver_pc1251;
		if ( strcmp(drivers[i]->name,"pc1401")==0) drivers[i]=&driver_pc1401;
		if ( strcmp(drivers[i]->name,"pc1402")==0) drivers[i]=&driver_pc1402;
		if ( strcmp(drivers[i]->name,"pc1350")==0) drivers[i]=&driver_pc1350;
	}
}
#endif

