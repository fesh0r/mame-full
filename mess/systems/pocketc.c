/******************************************************************************
 sharp pocket computers
 pc1401/pc1403
 PeT mess@utanet.at May 2000
******************************************************************************/

#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1251.h"
#include "includes/pc1350.h"
#include "includes/pc1403.h"


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
	{ 0x7000, 0x77ff, pc1401_lcd_read },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1401_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
/*	{ 0x2000, 0x3fff, MWA_RAM }, // done in pc1401_machine_init */
	{ 0x4000, 0x47ff, MWA_RAM },
/*	{ 0x5000, 0x57ff, ? }, */
	{ 0x6000, 0x67ff, pc1401_lcd_write },
	{ 0x7000, 0x77ff, pc1401_lcd_write },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pc1251_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
//	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xcbff, MRA_ROM },
	{ 0xf800, 0xf8ff, pc1251_lcd_read },
MEMORY_END

static MEMORY_WRITE_START( pc1251_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_ROM },
//	{ 0xa000, 0xcbff, MWA_ROM }, // c600 b800 b000 a000 tested
	{ 0xf800, 0xf8ff, pc1251_lcd_write },
MEMORY_END

static MEMORY_READ_START( pc1350_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x7eff, pc1350_lcd_read },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1350_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM }, /*ram card 16k */
	{ 0x4000, 0x5fff, MWA_RAM }, /*ram card 16k oder 8k */
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7000, 0x7eff, pc1350_lcd_write },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pc1403_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
{ 0x3000, 0x30bf, pc1403_lcd_read },    
{ 0x3800, 0x3fff, pc1403_asic_read },    
{ 0x4000, 0x7fff, MRA_BANK1 },
{ 0xe000,0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( pc1403_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
{ 0x3000, 0x30bf, pc1403_lcd_write },    
{ 0x3800, 0x3fff, pc1403_asic_write },    
{ 0xe000,0xffff, MWA_RAM },
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


#define PC1401_HELPER1 \
	PORT_START \
	PORT_BITX (0x80, 0x00, IPT_DIPSWITCH_NAME|IPF_TOGGLE, \
			   "Power",CODE_DEFAULT, CODE_NONE) \
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING( 0x00, DEF_STR( On ) ) \
	DIPS_HELPER( 0x40, "CAL", KEYCODE_F1, CODE_NONE) \
	DIPS_HELPER( 0x20, "BASIC", KEYCODE_F2, CODE_NONE) \
	DIPS_HELPER( 0x10, "BRK   ON", KEYCODE_F4, CODE_NONE) \
	DIPS_HELPER( 0x08, "DEF", KEYCODE_LALT, KEYCODE_RALT) \
	DIPS_HELPER( 0x04, "down", KEYCODE_DOWN, CODE_NONE) \
	DIPS_HELPER( 0x02, "up", KEYCODE_UP, CODE_NONE) \
	DIPS_HELPER( 0x01, "left  DEL", KEYCODE_LEFT, CODE_NONE) \
	PORT_START \
	DIPS_HELPER( 0x80, "right INS", KEYCODE_RIGHT, CODE_NONE)

#define PC1401_HELPER2 \
	DIPS_HELPER( 0x20, "SHIFT", KEYCODE_LSHIFT, KEYCODE_RSHIFT) \
	DIPS_HELPER( 0x10, "Q     !", KEYCODE_Q, CODE_NONE)\
	DIPS_HELPER( 0x08, "W     \"", KEYCODE_W, CODE_NONE)\
	DIPS_HELPER( 0x04, "E     #", KEYCODE_E, CODE_NONE)\
	DIPS_HELPER( 0x02, "R     $", KEYCODE_R, CODE_NONE)\
	DIPS_HELPER( 0x01, "T     %", KEYCODE_T, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "Y     &", KEYCODE_Y, CODE_NONE)\
	DIPS_HELPER( 0x40, "U     ?", KEYCODE_U, CODE_NONE)\
	DIPS_HELPER( 0x20, "I     At", KEYCODE_I, CODE_NONE)\
	DIPS_HELPER( 0x10, "O     :", KEYCODE_O, CODE_NONE)\
	DIPS_HELPER( 0x08, "P     ;", KEYCODE_P, CODE_NONE)\
	DIPS_HELPER( 0x04, "A     INPUT", KEYCODE_A, CODE_NONE)\
	DIPS_HELPER( 0x02, "S     IF", KEYCODE_S, CODE_NONE)\
	DIPS_HELPER( 0x01, "D     THEN", KEYCODE_D, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "F     GOTO", KEYCODE_F, CODE_NONE)\
	DIPS_HELPER( 0x40, "G     FOR", KEYCODE_G, CODE_NONE)\
	DIPS_HELPER( 0x20, "H     TO", KEYCODE_H, CODE_NONE)\
	DIPS_HELPER( 0x10, "J     STEP", KEYCODE_J, CODE_NONE)\
	DIPS_HELPER( 0x08, "K     NEXT", KEYCODE_K, CODE_NONE)\
	DIPS_HELPER( 0x04, "L     LIST", KEYCODE_L, CODE_NONE)\
	DIPS_HELPER( 0x02, ",     RUN", KEYCODE_COMMA, CODE_NONE)\
	DIPS_HELPER( 0x01, "Z     PRINT", KEYCODE_Z, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "X     USING", KEYCODE_X, CODE_NONE)\
	DIPS_HELPER( 0x40, "C     GOSUB", KEYCODE_C, CODE_NONE)\
	DIPS_HELPER( 0x20, "V     RETURN", KEYCODE_V, CODE_NONE)\
	DIPS_HELPER( 0x10, "B     DIM", KEYCODE_B, CODE_NONE)\
	DIPS_HELPER( 0x08, "N     END", KEYCODE_N, CODE_NONE)\
	DIPS_HELPER( 0x04, "M     CSAVE", KEYCODE_M, CODE_NONE)\
	DIPS_HELPER( 0x02, "SPC   CLOAD", KEYCODE_SPACE, CODE_NONE)\
	DIPS_HELPER( 0x01, "ENTER N<>NP", KEYCODE_ENTER, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "HYP   ARCHYP", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x40, "SIN   SIN^-1", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x20, "COS   COS^-1", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x10, "TAN   TAN^-1", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x08, "F<>E  TAB", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x04, "C-CE  CA", KEYCODE_ESC, CODE_NONE)\
	DIPS_HELPER( 0x02, "->HEX ->DEC", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x01, "->DEG ->D.MS", CODE_DEFAULT, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "LN    e^x    E", CODE_DEFAULT, CODE_NONE)\
 	DIPS_HELPER( 0x40, "LOG   10^x   F", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x20, "1/x   ->re", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x10, "chnge STAT", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x08, "EXP   Pi     A", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x04, "y^x   xROOTy B", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x02, "sqrt  3root  C", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x01, "sqr   tri%   D", CODE_DEFAULT, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "(     ->xy", KEYCODE_8, CODE_NONE)\
	DIPS_HELPER( 0x40, ")     n!", KEYCODE_9, CODE_NONE)\
	DIPS_HELPER( 0x20, "7     y mean", KEYCODE_7_PAD, CODE_NONE)\
	DIPS_HELPER( 0x10, "8     Sy", KEYCODE_8_PAD, CODE_NONE)\
	DIPS_HELPER( 0x08, "9     sigmay", KEYCODE_9_PAD, CODE_NONE)\
	DIPS_HELPER( 0x04, "/", KEYCODE_SLASH_PAD, CODE_NONE)\
	DIPS_HELPER( 0x02, "X->M  Sum y  Sum y^2", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x01, "4     x mean", KEYCODE_4_PAD, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "5     Sx", KEYCODE_5_PAD, CODE_NONE)\
	DIPS_HELPER( 0x40, "6     sigmax", KEYCODE_6_PAD, CODE_NONE)\
	DIPS_HELPER( 0x20, "*     <", KEYCODE_ASTERISK, CODE_NONE)\
	DIPS_HELPER( 0x10, "RM    (x,y)", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x08, "1     r", KEYCODE_1_PAD, CODE_NONE)\
	DIPS_HELPER( 0x04, "2     a", KEYCODE_2_PAD, CODE_NONE)\
	DIPS_HELPER( 0x02, "3     b", KEYCODE_3_PAD, CODE_NONE)\
	DIPS_HELPER( 0x01, "-     >", KEYCODE_MINUS_PAD, CODE_NONE)\
	PORT_START\
	DIPS_HELPER( 0x80, "M+    DATA   CD", CODE_DEFAULT, CODE_NONE)\
	DIPS_HELPER( 0x40, "0     x'", KEYCODE_0_PAD, CODE_NONE)\
	DIPS_HELPER( 0x20, "+/-   y'", KEYCODE_NUMLOCK, CODE_NONE)\
	DIPS_HELPER( 0x10, ".     DRG", KEYCODE_DEL_PAD, CODE_NONE)\
	DIPS_HELPER( 0x08, "+     ^", KEYCODE_PLUS_PAD, CODE_NONE)\
	DIPS_HELPER( 0x04, "=", KEYCODE_ENTER_PAD, CODE_NONE)\
	DIPS_HELPER( 0x02, "Reset", KEYCODE_F3, CODE_NONE)

INPUT_PORTS_START( pc1401 )
    PC1401_HELPER1
	PORT_BIT ( 0x40, 0x0,	 IPT_UNUSED )
    PC1401_HELPER2
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

INPUT_PORTS_START( pc1403 )
    PC1401_HELPER1
	DIPS_HELPER( 0x40, "SML", KEYCODE_CAPSLOCK, CODE_NONE)
    PC1401_HELPER2
	PORT_START
    PORT_DIPNAME   ( 0x80, 0x80, "RAM")
	PORT_DIPSETTING( 0x00, "PC1403(8KB)" )
	PORT_DIPSETTING( 0x80, "PC1403H(32KB)" )

    // normally no contrast control!
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

struct DACinterface pocketc_sound_interface =
{
        1,
        {25}
};

static SC61860_CONFIG config={
    pc1401_reset, pc1401_brk, NULL,
    pc1401_ina, pc1401_outa,
    pc1401_inb, pc1401_outb,
    pc1401_outc
};

static MACHINE_DRIVER_START( pc1401 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SC61860, 192000)        /* 7.8336 Mhz */
	MDRV_CPU_MEMORY(pc1401_readmem,pc1401_writemem)
	MDRV_CPU_CONFIG(config)

	MDRV_FRAMES_PER_SECOND(20)	/* very early and slow lcd */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( pc1401 )
	MDRV_MACHINE_STOP( pc1401 )

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x3 pixel)
	   it seams to have 3/4 ratio in the real pc1401 */
    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(594, 273)
	MDRV_VISIBLE_AREA(0, 594-1, 0, 273-1)
//	MDRV_SCREEN_SIZE(640, 273)
//	MDRV_VISIBLE_AREA(0, 640-1, 0, 273-1)
	MDRV_GFXDECODE( pc1401_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(248 + 32768)
	MDRV_COLORTABLE_LENGTH( 16 )
	MDRV_PALETTE_INIT( pocketc )

	MDRV_VIDEO_START( pocketc )
	MDRV_VIDEO_UPDATE( pc1401 )

	/* sound hardware */
	/*MDRV_SOUND_ADD(DAC, pocketc_sound_interface)*/
MACHINE_DRIVER_END


static SC61860_CONFIG pc1251_config={
    NULL, pc1251_brk, NULL,
    pc1251_ina, pc1251_outa,
    pc1251_inb, pc1251_outb,
    pc1251_outc
};

static MACHINE_DRIVER_START( pc1251 )
	MDRV_IMPORT_FROM( pc1401 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( pc1251_readmem,pc1251_writemem )
	MDRV_CPU_CONFIG( pc1251_config )

	MDRV_MACHINE_INIT( pc1251 )
	MDRV_MACHINE_STOP( pc1251 )

	/* video hardware */
	MDRV_SCREEN_SIZE(608, 300)
	MDRV_VISIBLE_AREA(0, 608-1, 0, 300-1)
//	MDRV_SCREEN_SIZE(640, 334)
//	MDRV_VISIBLE_AREA(0, 640-1, 0, 334-1)
	MDRV_GFXDECODE( pc1251_gfxdecodeinfo )

	MDRV_VIDEO_UPDATE( pc1251 )
MACHINE_DRIVER_END


static SC61860_CONFIG pc1350_config={
    NULL, pc1350_brk,NULL,
    pc1350_ina, pc1350_outa,
    pc1350_inb, pc1350_outb,
    pc1350_outc
};

static MACHINE_DRIVER_START( pc1350 )
	MDRV_IMPORT_FROM( pc1401 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( pc1350_readmem,pc1350_writemem )
	MDRV_CPU_CONFIG( pc1350_config )

	MDRV_MACHINE_INIT( pc1350 )
	MDRV_MACHINE_STOP( pc1350 )

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x2 pixel) */
	/* video hardware */
	MDRV_SCREEN_SIZE(640, 252)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 252-1)
//	MDRV_SCREEN_SIZE(640, 255)
//	MDRV_VISIBLE_AREA(0, 640-1, 0, 255-1)
	MDRV_GFXDECODE( pc1350_gfxdecodeinfo )

	MDRV_VIDEO_UPDATE( pc1350 )
MACHINE_DRIVER_END


static SC61860_CONFIG pc1403_config={
    NULL, pc1403_brk, NULL,
    pc1403_ina, pc1403_outa,
    NULL,NULL,
    pc1403_outc
};

static MACHINE_DRIVER_START( pc1403 )
	MDRV_IMPORT_FROM( pc1401 )
	MDRV_CPU_REPLACE( "main", SC61860, 256000 )
	MDRV_CPU_MEMORY( pc1403_readmem,pc1403_writemem )
	MDRV_CPU_CONFIG( pc1403_config )

	MDRV_MACHINE_INIT( pc1403 )
	MDRV_MACHINE_STOP( pc1403 )

	/*
	   aim: show sharp with keyboard
	   resolution depends on the dots of the lcd
	   (lcd dot displayed as 2x2 pixel) */
	/* video hardware */
	MDRV_SCREEN_SIZE(848, 320)
	MDRV_VISIBLE_AREA(0, 848-1, 0, 320-1)
//	MDRV_SCREEN_SIZE(848, 361)
//	MDRV_VISIBLE_AREA(0, 848-1, 0, 361-1)

	MDRV_VIDEO_START( pc1403 )
	MDRV_VIDEO_UPDATE( pc1403 )
MACHINE_DRIVER_END


ROM_START(pc1401)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* SC61860A08 5H 13LD cpu with integrated rom*/
	ROM_LOAD("sc61860.a08", 0x0000, 0x2000, CRC(44bee438))
/* 5S1 SC613256 D30
   or SC43536LD 5G 13 (LCD chip?) */
	ROM_LOAD("sc613256.d30", 0x8000, 0x8000, CRC(69b9d587))
	ROM_REGION(0x80,REGION_GFX1,0)
ROM_END

#define rom_pc1402 rom_pc1401

ROM_START(pc1251)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* sc61860a13 6c 13 ld */
	ROM_LOAD("cpu1251.rom", 0x0000, 0x2000, CRC(f7287aca))
	ROM_LOAD("bas1251.rom", 0x4000, 0x4000, CRC(93ecb629))
	ROM_REGION(0x80,REGION_GFX1,0)
ROM_END

#define rom_trs80pc3 rom_pc1251

ROM_START(pc1350)
	ROM_REGION(0x10000,REGION_CPU1,0)
	/* sc61860a13 6c 13 ld */
	ROM_LOAD("cpu.rom", 0x0000, 0x2000, CRC(79a924bc))
	ROM_LOAD("basic.rom", 0x8000, 0x8000, CRC(158b28e2))
	ROM_REGION(0x100,REGION_GFX1,0)
ROM_END

ROM_START(pc1403)
	ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("introm.bin", 0x0000, 0x2000, CRC(588c500b ))
	ROM_REGION(0x10000,REGION_USER1,0)
    ROM_LOAD("extrom08.bin", 0x0000, 0x4000, CRC(1fa65140 ))
    ROM_LOAD("extrom09.bin", 0x4000, 0x4000, CRC(4a7da6ab ))
    ROM_LOAD("extrom0a.bin", 0x8000, 0x4000, CRC(9925174f ))
    ROM_LOAD("extrom0b.bin", 0xc000, 0x4000, CRC(fa5df9ec ))
	ROM_REGION(0x100,REGION_GFX1,0)
ROM_END

#define rom_pc1403h rom_pc1403

// disk drive support
#define io_pc1403 io_pc1401
#define io_pc1403h io_pc1403

SYSTEM_CONFIG_START(pocketc)
//	CONFIG_DEVICE_CASSETTE(1, "", mycas_init)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */

/* cpu sc43177, sc43178 (4bit!)
   pc 1211
   clone tandy trs80 pocket computer
   pc1246/pc1247
*/

/* cpu lh5801
   pc1500 
   clone tandy pc2 look into systems/pc1500.c 
   pc1600
*/

/*    YEAR  NAME		PARENT	COMPAT	MACHINE			INPUT		INIT	CONFIG		COMPANY		FULLNAME */
/* cpu sc61860 */
COMPX( 1982, pc1251,	0, 		0,		pc1251,	pc1251,	pc1251,	pocketc,	"Sharp",  "Pocket Computer 1251", GAME_NOT_WORKING)
COMPX( 198?, trs80pc3,	pc1251,	0,		pc1251,	pc1251,	pc1251,	pocketc,	"Tandy",  "TRS80 PC-3", GAME_ALIAS|GAME_NOT_WORKING)

/* pc1261/pc1262 */
COMPX( 1984, pc1350,	0, 		0,		pc1350,	pc1350,	pc1350,	pocketc,	"Sharp",  "Pocket Computer 1350", GAME_NOT_WORKING)
COMPX( 1983, pc1401,	0, 		0,		pc1401,	pc1401,	pc1401,	pocketc,	"Sharp",  "Pocket Computer 1401", GAME_NOT_WORKING)
COMPX( 1984, pc1402,	pc1401,	0,		pc1401,	pc1401,	pc1401,	pocketc,	"Sharp",  "Pocket Computer 1402", GAME_ALIAS|GAME_NOT_WORKING)

/* 72kb rom, 32kb ram, cpu? pc1360 */
COMPX( 198?, pc1403,	0,		0,		pc1403,	pc1403,	pc1403,	pocketc,	"Sharp", "Pocket Computer 1403", GAME_NOT_WORKING)
COMPX( 198?, pc1403h,	pc1403,	0,		pc1403,	pc1403,	pc1403,	pocketc,	"Sharp", "Pocket Computer 1403H", GAME_ALIAS|GAME_NOT_WORKING)
