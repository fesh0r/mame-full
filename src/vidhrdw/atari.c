/******************************************************************************
	Atari 400/800

	Video handler

	Juergen Buchmueller, June 1998
******************************************************************************/

#include "driver.h"
#include "m6502/m6502.h"
#include "machine/atari.h"
#include "vidhrdw/atari.h"

#define VERBOSE 0

/* flag for displaying television artifacts in ANTIC mode F (15) */
static	int color_artifacts = 0;

/*************************************************************************
 * The priority tables tell which playfield, player or missile colors
 * have precedence about the others, depending on the contents of the
 * "prior" register. There are 64 possible priority selections.
 * The table is here to make it easier to build the 'illegal' priority
 * combinations that produce black or 'ILL' color.
 *************************************************************************/

/*************************************************************************
 * calculate player/missile priorities (GTIA prior at $D00D)
 * prior   color priorities in descending order
 * ------------------------------------------------------------------
 * bit 0   PL0	  PL1	 PL2	PL3    PF0	  PF1	 PF2	PF3/P4 BK
 *		   all players in front of all playfield colors
 * bit 1   PL0	  PL1	 PF0	PF1    PF2	  PF3/P4 PL2	PL3    BK
 *		   pl 0+1 in front of pf 0-3 in front of pl 2+3
 * bit 2   PF0    PF1    PF2    PF3/P4 PL0    PL1    PL2    PL3    BK
 *		   all playfield colors in front of all players
 * bit 3   PF0    PF1    PL0    PL1    PL2    PL3    PF2    PF3/P4 BK
 *		   pf 0+1 in front of all players in front of pf 2+3
 * bit 4   missiles colors are PF3 (P4)
 *		   missiles have the same priority as pf3
 * bit 5   PL0+PL1 and PL2+PL3 bits xored
 *		   00: playfield, 01: PL0/2, 10: PL1/3 11: black (EOR)
 * bit 7+6 CTIA mod (00) or GTIA mode 1 to 3 (01, 10, 11)
 *************************************************************************/

/* player/missile #4 color is equal to playfield #3 */
#define PM4 PF3

/* bit masks for players and missiles */
#define P0 0x01
#define P1 0x02
#define P2 0x04
#define P3 0x08
#define M0 0x10
#define M1 0x20
#define M2 0x40
#define M3 0x80

/************************************************************************
 * Contents of the following table:
 *
 * PL0 -PL3  are the player/missile colors 0 to 3
 * P000-P011 are the 4 available color clocks for playfield color 0
 * P100-P111 are the 4 available color clocks for playfield color 1
 * P200-P211 are the 4 available color clocks for playfield color 2
 * P300-P311 are the 4 available color clocks for playfield color 3
 * ILL       is some undefined color. On my 800XL it looked light yellow ;)
 *
 * Each line holds the 8 bitmasks and resulting colors for player and
 * missile number 0 to 3 in their fixed priority order.
 * The 8 lines per block are for the 8 available playfield colors.
 * Yes, 8 colors because the text modes 2,3 and graphics mode F can
 * be combined with players. The result is the players color with
 * luminance of the modes foreground (ie. colpf1).
 * Any combination of players/missiles (256) is checked for the highest
 * priority player or missile and the resulting color is stored into
 * antic.prio_table. The second part (20-3F) contains the resulting
 * color values for the EOR mode, which is derived from the *visible*
 * player/missile colors calculated for the first part (00-1F).
 * The priorities of combining priority bits (which games use!) are:
 ************************************************************************/
static	byte	_pm_colors[32][8*2*8] = {
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 00
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 01
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 02
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2,   0,P2,   0,M3,   0,P3,   0,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2,   0,P2,   0,M3,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 03
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 04
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 05
		M0, ILL,P0, ILL,M1, ILL,P1, ILL,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 06
		M0,   0,P0, ILL,M1,   0,P1, ILL,M2,   0,P2,   0,M3,   0,P3,   0,
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 07
		M0, ILL,P0, ILL,M1, ILL,P1, ILL,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 08
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 09
		M0, ILL,P0, ILL,M1, ILL,P1, ILL,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0A
		M0, ILL,P0, ILL,M1, ILL,P1, ILL,M2,   0,P2,   0,M3,   0,P3,   0,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0B
		M0, ILL,P0, ILL,M1, ILL,P1, ILL,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0C
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0D
		M0,   0,P0,   0,M1,   0,P1,   0,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0E
		M0,   0,P0,   0,M1,   0,P1,   0,M2,   0,P2,   0,M3,   0,P3,   0,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		M0, PL0,P0, PL0,M1, PL1,P1, PL1,M2, PL2,P2, PL2,M3, PL3,P3, PL3,  // 0F
		M0,   0,P0,   0,M1,   0,P1,   0,M2, PL2,P2, PL2,M3, PL3,P3, PL3,
		M0,   0,P0,   0,M1,   0,P1,   0,M2, ILL,P2, ILL,M3, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,P0,P000,M1,P100,P1,P100,M2,P200,P2,P200,M3,P300,P3,P300,
		M0,P001,P0,P001,M1,P101,P1,P101,M2,P201,P2,P201,M3,P301,P3,P301,
		M0,P010,P0,P010,M1,P110,P1,P110,M2,P210,P2,P210,M3,P310,P3,P310,
		M0,P011,P0,P011,M1,P111,P1,P111,M2,P211,P2,P211,M3,P311,P3,P311
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 10
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 11
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2, PL2,P3, PL3,  // 12
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,   0,P3,   0,
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,M0,P400,M1,P400,M2,P400,M3,P400,P2,P200,P3,P300,
		P0,P001,P1,P101,M0,P401,M1,P401,M2,P401,M3,P401,P2,P201,P3,P301,
		P0,P010,P1,P110,M0,P410,M1,P410,M2,P410,M3,P410,P2,P210,P3,P310,
		P0,P011,P1,P111,M0,P411,M1,P411,M2,P411,M3,P411,P2,P211,P3,P311
	},
	{
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2, PL2,P3, PL3,  // 13
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2, PL2,P3, PL3,
		P0, PL0,P1, PL1,M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,M0,P400,M1,P400,M2,P400,M3,P400,P2,P200,P3,P300,
		P0,P001,P1,P101,M0,P401,M1,P401,M2,P401,M3,P401,P2,P201,P3,P301,
		P0,P010,P1,P110,M0,P410,M1,P410,M2,P410,M3,P410,P2,P210,P3,P310,
		P0,P011,P1,P111,M0,P411,M1,P411,M2,P411,M3,P411,P2,P211,P3,P311
	},
	{
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0, PL0,P1, PL1,P2, PL2,P3, PL3,  // 14
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0,   0,P1,   0,P2,   0,P3,   0,
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0,   0,P1,   0,P2,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P400,M1,P400,M2,P400,M3,P400,P0,P000,P1,P100,P2,P200,P3,P300,
		M0,P401,M1,P401,M2,P401,M3,P401,P0,P001,P1,P101,P2,P201,P3,P301,
		M0,P410,M1,P410,M2,P410,M3,P410,P0,P010,P1,P110,P2,P210,P3,P310,
		M0,P411,M1,P411,M2,P411,M3,P411,P0,P011,P1,P111,P2,P211,P3,P311
	},
	{
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0, PL0,P1, PL1, PL2,P3, PL3,  // 15
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0, ILL,P1, ILL, PL2,P3, PL3,
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0,	 0,P1,	 0, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0, 0,	 0, 0,	 0,   0, 0,   0,
		M0,P000,M1,P100,M2,P200,M3,P300,P2,P0,P000,P1,P100,P200,P3,P300,
		M0,P001,M1,P101,M2,P201,M3,P301,P2,P0,P001,P1,P101,P201,P3,P301,
		M0,P010,M1,P110,M2,P210,M3,P310,P2,P0,P010,P1,P110,P210,P3,P310,
		M0,P011,M1,P111,M2,P211,M3,P311,P2,P0,P011,P1,P111,P211,P3,P311
	},
	{
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0, PL0,P1, PL1,P2, PL2,P3, PL3,  // 16
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0, ILL,P1, ILL,P2,   0,P3,   0,
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P0,   0,P1,   0,P2,   0,P3,   0,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		M0,P000,M1,P100,M2,P200,M3,P300,P0,P000,P1,P100,P2,P200,P3,P300,
		M0,P001,M1,P101,M2,P201,M3,P301,P0,P001,P1,P101,P2,P201,P3,P301,
		M0,P010,M1,P110,M2,P210,M3,P310,P0,P010,P1,P110,P2,P210,P3,P310,
		M0,P011,M1,P111,M2,P211,M3,P311,P0,P011,P1,P111,P2,P211,P3,P311
	},
	{
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0, PL0,P1, PL1, PL2,P3, PL3,  // 17
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0, ILL,P1, ILL, PL2,P3, PL3,
		M0, PM4,M1, PM4,M2, PM4,M3, PM4,P2,P0,	 0,P1,	 0, ILL,P3, ILL,
		 0,   0, 0,   0, 0,   0, 0,   0, 0, 0,	 0, 0,	 0,   0, 0,   0,
		M0,P000,M1,P100,M2,P200,M3,P300,P2,P0,P000,P1,P100,P200,P3,P300,
		M0,P001,M1,P101,M2,P201,M3,P301,P2,P0,P001,P1,P101,P201,P3,P301,
		M0,P010,M1,P110,M2,P210,M3,P310,P2,P0,P010,P1,P110,P210,P3,P310,
		M0,P011,M1,P111,M2,P211,M3,P311,P2,P0,P011,P1,P111,P211,P3,P311
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 18
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 19
		P0, ILL,P1, ILL,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P000,M1,P100,M2,P200,M3,P300,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P001,M1,P101,M2,P201,M3,P301,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P010,M1,P110,M2,P210,M3,P310,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P011,M1,P111,M2,P211,M3,P311
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1A
		P0, ILL,P1, ILL,P2,   0,P3,   0,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1B
		P0, ILL,P1, ILL,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0, PL0,P1, PL1,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1C
		P0,   0,P1,   0,P2,   0,P3,   0,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0,   0,P1,   0,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1D
		P0,   0,P1,   0,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0,   0,P1,   0,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1E
		P0,   0,P1,   0,P2,   0,P3,   0,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0,   0,P1,   0,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	},
	{
		P0, PL0,P1, PL1,P2, PL2,P3, PL3,M0, PM4,M1, PM4,M2, PM4,M3, PM4,  // 1F
		P0,   0,P1,   0,P2,   0,P3,   0,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		P0,   0,P1,   0,P2, ILL,P3, ILL,M0, PM4,M1, PM4,M2, PM4,M3, PM4,
		 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0,
		P0,P000,P1,P100,P2,P200,P3,P300,M0,P400,M1,P400,M2,P400,M3,P400,
		P0,P001,P1,P101,P2,P201,P3,P301,M0,P401,M1,P401,M2,P401,M3,P401,
		P0,P010,P1,P110,P2,P210,P3,P310,M0,P410,M1,P410,M2,P410,M3,P410,
		P0,P011,P1,P111,P2,P211,P3,P311,M0,P411,M1,P411,M2,P411,M3,P411
	}
};

static  void    prio_init(void)
{
int i, j, pm, p, c;
byte * prio;

	/* 32 priority bits combinations */
	for (i = 0; i < 32; i++)
	{
		/* 8 playfield colors */
		for (j = 0; j < 8; j++)
		{
			prio = &_pm_colors[i][j*16];
			/* 256 player/missile combinations to build */
			for (pm = 0; pm < 256; pm++)
			{
				c = PFD; /* assume playfield color */
				for (p = 0; (c == PFD) && (p < 16); p += 2)
				{
					if (((prio[p] & pm) == prio[p]) && (prio[p+1]))
						c = prio[p+1];
				}
				antic.prio_table[i][(j << 8) + pm] = c;
				if ((c==PL0 || c==P000 || c==P001 || c==P010 || c==P011) &&
					(pm & (P0+P1))==(P0+P1))
					c = EOR;
				if ((c==PL2 || c==P200 || c==P201 || c==P210 || c==P211) &&
					(pm & (P2+P3))==(P2+P3))
					c = EOR;
				antic.prio_table[32 + i][(j << 8) + pm] = c;
			}
		}
	}
}

static	void	cclk_init(void)
{
static byte _pf_21[4] =   {T00,T01,T10,T11};
static byte _pf_1b[4] =   {G00,G01,G10,G11};
static byte _pf_210b[4] = {PBK,PF0,PF1,PF2};
static byte _pf_310b[4] = {PBK,PF0,PF1,PF3};
int i;
byte * dst;
    for (i = 0; i < 256; i++)
    {
        /****** text mode (2,3) **********/
		dst = (byte *)&antic.pf_21[0x000+i];
		*dst++ = _pf_21[(i>>6)&3];
		*dst++ = _pf_21[(i>>4)&3];
		*dst++ = _pf_21[(i>>2)&3];
		*dst++ = _pf_21[(i>>0)&3];

        /****** 4 color text (4,5) with pf2, D, E **********/
		dst = (byte *)&antic.pf_x10b[0x000+i];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>0)&3];
		dst = (byte *)&antic.pf_x10b[0x100+i];
		*dst++ = _pf_310b[(i>>6)&3];
		*dst++ = _pf_310b[(i>>4)&3];
		*dst++ = _pf_310b[(i>>2)&3];
		*dst++ = _pf_310b[(i>>0)&3];

        /****** pf0 color text (6,7), 9, B, C **********/
		dst = (byte *)&antic.pf_3210b2[0x000+i*2];
		*dst++ = (i&0x80)?PF0:PBK;
		*dst++ = (i&0x40)?PF0:PBK;
		*dst++ = (i&0x20)?PF0:PBK;
		*dst++ = (i&0x10)?PF0:PBK;
		*dst++ = (i&0x08)?PF0:PBK;
		*dst++ = (i&0x04)?PF0:PBK;
		*dst++ = (i&0x02)?PF0:PBK;
		*dst++ = (i&0x01)?PF0:PBK;

        /****** pf1 color text (6,7), 9, B, C **********/
		dst = (byte *)&antic.pf_3210b2[0x200+i*2];
		*dst++ = (i&0x80)?PF1:PBK;
		*dst++ = (i&0x40)?PF1:PBK;
		*dst++ = (i&0x20)?PF1:PBK;
		*dst++ = (i&0x10)?PF1:PBK;
		*dst++ = (i&0x08)?PF1:PBK;
		*dst++ = (i&0x04)?PF1:PBK;
		*dst++ = (i&0x02)?PF1:PBK;
		*dst++ = (i&0x01)?PF1:PBK;

        /****** pf2 color text (6,7), 9, B, C **********/
		dst = (byte *)&antic.pf_3210b2[0x400+i*2];
		*dst++ = (i&0x80)?PF2:PBK;
		*dst++ = (i&0x40)?PF2:PBK;
		*dst++ = (i&0x20)?PF2:PBK;
		*dst++ = (i&0x10)?PF2:PBK;
		*dst++ = (i&0x08)?PF2:PBK;
		*dst++ = (i&0x04)?PF2:PBK;
		*dst++ = (i&0x02)?PF2:PBK;
		*dst++ = (i&0x01)?PF2:PBK;

        /****** pf3 color text (6,7), 9, B, C **********/
		dst = (byte *)&antic.pf_3210b2[0x600+i*2];
		*dst++ = (i&0x80)?PF3:PBK;
		*dst++ = (i&0x40)?PF3:PBK;
		*dst++ = (i&0x20)?PF3:PBK;
		*dst++ = (i&0x10)?PF3:PBK;
		*dst++ = (i&0x08)?PF3:PBK;
		*dst++ = (i&0x04)?PF3:PBK;
		*dst++ = (i&0x02)?PF3:PBK;
		*dst++ = (i&0x01)?PF3:PBK;

        /****** 4 color graphics 4 cclks (8) **********/
		dst = (byte *)&antic.pf_210b4[i*4];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>0)&3];
		*dst++ = _pf_210b[(i>>0)&3];
		*dst++ = _pf_210b[(i>>0)&3];
		*dst++ = _pf_210b[(i>>0)&3];

        /****** 4 color graphics 2 cclks (A) **********/
		dst = (byte *)&antic.pf_210b2[i*2];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>6)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>4)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>2)&3];
		*dst++ = _pf_210b[(i>>0)&3];
		*dst++ = _pf_210b[(i>>0)&3];

        /****** high resolution graphics (F) **********/
		dst = (byte *)&antic.pf_1b[i];
		*dst++ = _pf_1b[(i>>6)&3];
		*dst++ = _pf_1b[(i>>4)&3];
		*dst++ = _pf_1b[(i>>2)&3];
		*dst++ = _pf_1b[(i>>0)&3];

        /****** gtia mode 1 **********/
		dst = (byte *)&antic.pf_gtia1[i];
		*dst++ = GT1+((i>>4)&15);
		*dst++ = GT1+((i>>4)&15);
		*dst++ = GT1+(i&15);
		*dst++ = GT1+(i&15);

        /****** gtia mode 2 **********/
		dst = (byte *)&antic.pf_gtia2[i];
		*dst++ = GT2+((i>>4)&15);
		*dst++ = GT2+((i>>4)&15);
		*dst++ = GT2+(i&15);
		*dst++ = GT2+(i&15);

        /****** gtia mode 3 **********/
		dst = (byte *)&antic.pf_gtia3[i];
		*dst++ = GT3+((i>>4)&15);
		*dst++ = GT3+((i>>4)&15);
		*dst++ = GT3+(i&15);
		*dst++ = GT3+(i&15);

    }

	for (i = 0; i < 256; i++)
	{
		/* used colors in text modes 2,3 */
		antic.uc_21[i] = (i) ? PF2 | PF1 : PF2;

		/* used colors in text modes 4,5 and graphics modes D,E */
        switch (i & 0x03)
		{
			case 0x01: antic.uc_x10b[0x000+i] |= PF0; antic.uc_x10b[0x100+i] |= PF0; break;
			case 0x02: antic.uc_x10b[0x000+i] |= PF1; antic.uc_x10b[0x100+i] |= PF1; break;
			case 0x03: antic.uc_x10b[0x000+i] |= PF2; antic.uc_x10b[0x100+i] |= PF3; break;
		}
		switch (i & 0x0c)
		{
			case 0x04: antic.uc_x10b[0x000+i] |= PF0; antic.uc_x10b[0x100+i] |= PF0; break;
			case 0x08: antic.uc_x10b[0x000+i] |= PF1; antic.uc_x10b[0x100+i] |= PF1; break;
			case 0x0c: antic.uc_x10b[0x000+i] |= PF2; antic.uc_x10b[0x100+i] |= PF3; break;
        }
		switch (i & 0x30)
		{
			case 0x10: antic.uc_x10b[0x000+i] |= PF0; antic.uc_x10b[0x100+i] |= PF0; break;
			case 0x20: antic.uc_x10b[0x000+i] |= PF1; antic.uc_x10b[0x100+i] |= PF1; break;
			case 0x30: antic.uc_x10b[0x000+i] |= PF2; antic.uc_x10b[0x100+i] |= PF3; break;
		}
		switch (i & 0xc0)
		{
			case 0x40: antic.uc_x10b[0x000+i] |= PF0; antic.uc_x10b[0x100+i] |= PF0; break;
			case 0x80: antic.uc_x10b[0x000+i] |= PF1; antic.uc_x10b[0x100+i] |= PF1; break;
			case 0xc0: antic.uc_x10b[0x000+i] |= PF2; antic.uc_x10b[0x100+i] |= PF3; break;
        }

		/* used colors in text modes 6,7 and graphics modes 9,B,C */
		if (i)
		{
			antic.uc_3210b2[0x000+i*2] |= PF0;
			antic.uc_3210b2[0x200+i*2] |= PF1;
			antic.uc_3210b2[0x400+i*2] |= PF2;
			antic.uc_3210b2[0x600+i*2] |= PF3;
        }

		/* used colors in graphics mode 8 */
        switch (i & 0x03)
		{
			case 0x01: antic.uc_210b4[i*4] |= PF0; break;
			case 0x02: antic.uc_210b4[i*4] |= PF1; break;
			case 0x03: antic.uc_210b4[i*4] |= PF2; break;
		}
		switch (i & 0x0c)
		{
			case 0x04: antic.uc_210b4[i*4] |= PF0; break;
			case 0x08: antic.uc_210b4[i*4] |= PF1; break;
			case 0x0c: antic.uc_210b4[i*4] |= PF2; break;
        }
		switch (i & 0x30)
		{
			case 0x10: antic.uc_210b4[i*4] |= PF0; break;
			case 0x20: antic.uc_210b4[i*4] |= PF1; break;
			case 0x30: antic.uc_210b4[i*4] |= PF2; break;
		}
		switch (i & 0xc0)
		{
			case 0x40: antic.uc_210b4[i*4] |= PF0; break;
			case 0x80: antic.uc_210b4[i*4] |= PF1; break;
			case 0xc0: antic.uc_210b4[i*4] |= PF2; break;
        }

		/* used colors in graphics mode A */
        switch (i & 0x03)
		{
			case 0x01: antic.uc_210b2[i*2] |= PF0; break;
			case 0x02: antic.uc_210b2[i*2] |= PF1; break;
			case 0x03: antic.uc_210b2[i*2] |= PF2; break;
		}
		switch (i & 0x0c)
		{
			case 0x04: antic.uc_210b2[i*2] |= PF0; break;
			case 0x08: antic.uc_210b2[i*2] |= PF1; break;
			case 0x0c: antic.uc_210b2[i*2] |= PF2; break;
        }
		switch (i & 0x30)
		{
			case 0x10: antic.uc_210b2[i*2] |= PF0; break;
			case 0x20: antic.uc_210b2[i*2] |= PF1; break;
			case 0x30: antic.uc_210b2[i*2] |= PF2; break;
		}
		switch (i & 0xc0)
		{
			case 0x40: antic.uc_210b2[i*2] |= PF0; break;
			case 0x80: antic.uc_210b2[i*2] |= PF1; break;
			case 0xc0: antic.uc_210b2[i*2] |= PF2; break;
        }

		/* used colors in graphics mode F */
		if (i)
			antic.uc_1b[i] |= PF1;

		/* used colors in GTIA graphics modes */
		/* GTIA 1 is 16 different luminances with hue of colbk */
		antic.uc_g1[i] = 0x00;
		/* GTIA 2 is all 9 colors (8..15 is colbk) */
        switch (i & 0x0f)
		{
			case 0x00: antic.uc_g2[i] = 0x10; break;
			case 0x01: antic.uc_g2[i] = 0x20; break;
			case 0x02: antic.uc_g2[i] = 0x40; break;
			case 0x03: antic.uc_g2[i] = 0x80; break;
			case 0x04: antic.uc_g2[i] = 0x01; break;
			case 0x05: antic.uc_g2[i] = 0x02; break;
			case 0x06: antic.uc_g2[i] = 0x04; break;
			case 0x07: antic.uc_g2[i] = 0x08; break;
			default:   antic.uc_g2[i] = 0x00;
        }

        /* GTIA 3 is 16 different hues with luminance of colbk */
		antic.uc_g3[i] = 0x00;
    }
}

int     atari_vh_start(void)
{
int i;

#if VERBOSE
    if (errorlog) fprintf(errorlog, "atari antic_vh_start\n");
#endif
    memset(&antic, 0, sizeof(antic));
	memset(&gtia, 0, sizeof(gtia));

	antic.cclk_expand = malloc(21 * 256 * sizeof(dword));
	if (!antic.cclk_expand)
		return 1;

	antic.pf_21 	  = &antic.cclk_expand[ 0 * 256];
	antic.pf_x10b	  = &antic.cclk_expand[ 1 * 256];
	antic.pf_3210b2   = &antic.cclk_expand[ 3 * 256];
	antic.pf_210b4	  = &antic.cclk_expand[11 * 256];
	antic.pf_210b2	  = &antic.cclk_expand[15 * 256];
	antic.pf_1b 	  = &antic.cclk_expand[17 * 256];
	antic.pf_gtia1	  = &antic.cclk_expand[18 * 256];
	antic.pf_gtia2	  = &antic.cclk_expand[19 * 256];
	antic.pf_gtia3	  = &antic.cclk_expand[20 * 256];

	antic.used_colors = malloc(21 * 256 * sizeof(byte));
	if (!antic.used_colors)
	{
		free(antic.cclk_expand);
		return 1;
	}
	memset(antic.used_colors, 0, 21 * 256 * sizeof(byte));

	antic.uc_21 	  = &antic.used_colors[ 0 * 256];
	antic.uc_x10b	  = &antic.used_colors[ 1 * 256];
	antic.uc_3210b2   = &antic.used_colors[ 3 * 256];
	antic.uc_210b4	  = &antic.used_colors[11 * 256];
	antic.uc_210b2	  = &antic.used_colors[15 * 256];
	antic.uc_1b 	  = &antic.used_colors[17 * 256];
	antic.uc_g1 	  = &antic.used_colors[18 * 256];
	antic.uc_g2 	  = &antic.used_colors[19 * 256];
	antic.uc_g3 	  = &antic.used_colors[20 * 256];

	/* reset the ANTIC color tables */
    for (i = 0; i < 256; i ++)
        antic.color_lookup[i] = (Machine->pens[0] << 8) + Machine->pens[0];

#if VERBOSE
    if (errorlog) fprintf(errorlog, "atari cclk_init\n");
#endif
    cclk_init();

	for (i = 0; i < 64; i++)
    {
		antic.prio_table[i] = malloc(8*256);
		if (!antic.prio_table[i])
		{
			while (--i >= 0)
				free(antic.prio_table[i]);
			return 1;
		}
    }

#if VERBOSE
    if (errorlog) fprintf(errorlog, "atari prio_init\n");
#endif
    prio_init();

	for (i = 0; i < TOTAL_LINES; i++)
    {
		antic.video[i] = malloc(sizeof(VIDEO));
		if (!antic.video[i])
        {
            while (--i >= 0)
				free(antic.video[i]);
            return 1;
        }
		memset(antic.video[i], 0, sizeof(VIDEO));
    }

    return 0;
}

void	atari_vh_stop(void)
{
int i;
	for (i = 0; i < 64; i++)
	{
		if (antic.prio_table[i])
		{
			free(antic.prio_table[i]);
			antic.prio_table[i] = 0;
		}
	}
	for (i = 0; i < TOTAL_LINES; i++)
	{
		if (antic.video[i])
		{
			free(antic.video[i]);
			antic.video[i] = 0;
		}
	}

	if (antic.cclk_expand)
	{
		free(antic.cclk_expand);
		antic.cclk_expand = 0;
	}
}

void	atari_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* Television Artifacts mode changed ? */
	if (color_artifacts != (input_port_0_r(0) & 0x40))
	{
		color_artifacts = input_port_0_r(0) & 0x40;
		full_refresh = 1;
	}

#if OPTIMIZE
    if (full_refresh)
	{
		int y;
		for (y = 0; y < TOTAL_LINES; y++)
			antic.video[y]->dirty = 1;
		fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
    }
#endif
	full_refresh = 0;
}

static  renderer_function antic_renderer = antic_mode_0_xx;

static	void artifacts_gfx(byte * src, word * dst, int width)
{
int x;
byte n, bits = 0;
byte b = gtia.w.colbk & 0xf0;
byte c = gtia.w.colpf1 & 0x0f;
byte *d = (byte *)dst;
byte _A = Machine->colortable[((b+0x30)&0xf0)+c];
byte _B = Machine->colortable[((b+0x70)&0xf0)+c];
byte _C = Machine->colortable[b+c];
byte _D = Machine->colortable[gtia.w.colbk];
	for (x = 0; x < width * 4; x++)
	{
		n = *src++;
		bits <<= 2;
		switch (n)
		{
			case G00:
				break;
			case G01:
				bits |= 1;
				break;
			case G10:
				bits |= 2;
				break;
			case G11:
				bits |= 3;
				break;
			default:
                *d++ = antic.color_lookup[n];
				*d++ = antic.color_lookup[n];
				continue;
		}
		switch ((bits >> 1) & 7)
		{
			case 0: /* 0 0 0 */
			case 1: /* 0 0 1 */
			case 4: /* 1 0 0 */
				*d++ = _D;
				break;
			case 3: /* 0 1 1 */
			case 6: /* 1 1 0 */
			case 7: /* 1 1 1 */
				*d++ = _C;
				break;
			case 2: /* 0 1 0 */
				*d++ = _B;
				break;
			case 5: /* 1 0 1 */
				*d++ = _A;
				break;
		}
		switch (bits & 7)
		{
			case 0: /* 0 0 0 */
			case 1: /* 0 0 1 */
			case 4: /* 1 0 0 */
				*d++ = _D;
				break;
			case 3: /* 0 1 1 */
			case 6: /* 1 1 0 */
			case 7: /* 1 1 1 */
				*d++ = _C;
				break;
			case 2: /* 0 1 0 */
				*d++ = _A;
				break;
			case 5: /* 1 0 1 */
				*d++ = _B;
				break;
        }
    }
}

static	void artifacts_txt(byte * src, word * dst, int width)
{
int x;
byte n, bits = 0;
byte b = gtia.w.colpf2 & 0xf0;
byte c = gtia.w.colpf1 & 0x0f;
byte *d = (byte *)dst;
byte _A = Machine->colortable[((b+0x30)&0xf0)+c];
byte _B = Machine->colortable[((b+0x70)&0xf0)+c];
byte _C = Machine->colortable[b+c];
byte _D = Machine->colortable[gtia.w.colpf2];
	for (x = 0; x < width * 4; x++)
	{
		n = *src++;
		bits <<= 2;
		switch (n)
		{
			case T00:
				break;
			case T01:
				bits |= 1;
				break;
			case T10:
				bits |= 2;
				break;
			case T11:
				bits |= 3;
				break;
			default:
				*d++ = antic.color_lookup[n];
				*d++ = antic.color_lookup[n];
                continue;
        }
		switch ((bits >> 1) & 7)
		{
			case 0: /* 0 0 0 */
			case 1: /* 0 0 1 */
			case 4: /* 1 0 0 */
				*d++ = _D;
				break;
			case 3: /* 0 1 1 */
			case 6: /* 1 1 0 */
			case 7: /* 1 1 1 */
				*d++ = _C;
				break;
			case 2: /* 0 1 0 */
				*d++ = _A;
				break;
			case 5: /* 1 0 1 */
				*d++ = _B;
				break;
		}
		switch (bits & 7)
		{
			case 0:/* 0 0 0 */
			case 1:/* 0 0 1 */
			case 4:/* 1 0 0 */
				*d++ = _D;
				break;
			case 3: /* 0 1 1 */
			case 6: /* 1 1 0 */
			case 7: /* 1 1 1 */
				*d++ = _C;
				break;
			case 2: /* 0 1 0 */
				*d++ = _B;
				break;
			case 5: /* 1 0 1 */
				*d++ = _A;
				break;
        }
    }
}

static  void antic_linerefresh(VIDEO *video)
{
int x, y, width;
byte *src;
word *dst;
#if SHOW_EVERY_SCREEN_BIT
	src = &antic.cclock[PMOFFSET-antic.hscrol_old];
	y = antic.scanline;
    dst = (word *)&Machine->scrbitmap->line[y][0];
    width = HWIDTH;
#else
    if (antic.scanline < VDATA_START || antic.scanline >= VDATA_END)
        return;
    src = &antic.cclock[PMOFFSET-antic.hscrol_old+BUF_OFFS0*4];
	y = antic.scanline - VDATA_START;
    dst = (word *)&Machine->scrbitmap->line[y][BUF_OFFS0*8];
	width = HCHARS;
#endif
#if OPTIMIZE_VISIBLE
	memmove(&Machine->scrbitmap->line[y][0],
			&Machine->scrbitmap->line[y][1],
			BUF_OFFS0*8-1);
#endif
#if OPTIMIZE
    if (video->dirty == 0)
	{
#if OPTIMIZE_VISIBLE
		Machine->scrbitmap->line[y][BUF_OFFS0*8-1] = Machine->colortable[0];
#endif
        return;
	}
#endif
#if OPTIMIZE_VISIBLE
	Machine->scrbitmap->line[y][BUF_OFFS0*8-1] = Machine->colortable[15];
#endif

	if (color_artifacts)
	{
		switch (antic.cmd & 0x0f)
		{
			case 2: case 3:
				artifacts_txt(src, dst, width);
				break;
			case 15:
				artifacts_gfx(src, dst, width);
				break;
			default:
				for (x = 0; x < width; x++)
				{
					*dst++ = antic.color_lookup[*src++];
					*dst++ = antic.color_lookup[*src++];
					*dst++ = antic.color_lookup[*src++];
					*dst++ = antic.color_lookup[*src++];
				}
        }
	}
	else
	{
		for (x = 0; x < width; x++)
		{
			*dst++ = antic.color_lookup[*src++];
			*dst++ = antic.color_lookup[*src++];
			*dst++ = antic.color_lookup[*src++];
			*dst++ = antic.color_lookup[*src++];
        }
	}
    osd_mark_dirty(
		Machine->drv->visible_area.min_x,
		Machine->drv->visible_area.max_x,
		y, y, 1);
#if OPTIMIZE
    video->dirty = 0;
#endif
}

#if VERBOSE
static int cycle(void)
{
	return cycles_currently_ran();
}
#endif

static void after(int cycles, VIDEO * video, void (*function)(VIDEO *video))
{
double duration = 64.0e-6 * cycles / CYCLES_PER_LINE;
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d after %3d (%5.1f us)\n",
		antic.scanline, cycles, duration * 1.0e6);
#endif
    timer_set(duration, (int)video, (void(*)(int)) function);
}

static void antic_issue_dli(VIDEO * video)
{
    cpu_cause_interrupt(0, M6502_INT_NMI);
}

static void antic_handle_dli(VIDEO * video)
{
    antic.r.nmist &= ~DLI_NMI;
    if (antic.modelines == 1)
	{
		/* display list interrupt requested ? */
		if (antic.cmd & ANTIC_DLI)
		{
            if (antic.w.nmien & DLI_NMI)
			{
#if VERBOSE
				if (errorlog) fprintf(errorlog, "ANTIC #%3d cause DLI NMI @ cycle #%d\n", antic.scanline, cycle());
#endif
				antic.r.nmist |= DLI_NMI;
				after(CYCLES_DLI_NMI, video, antic_issue_dli);
			}
		}
    }
}

static  renderer_function renderer[2][19][5] = {
	/*	 no playfield	 narrow 		 normal 		 wide		  */
	{
		{antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx},
		{antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx},
		{antic_mode_0_xx,antic_mode_2_32,antic_mode_2_40,antic_mode_2_48},
		{antic_mode_0_xx,antic_mode_3_32,antic_mode_3_40,antic_mode_3_48},
		{antic_mode_0_xx,antic_mode_4_32,antic_mode_4_40,antic_mode_4_48},
		{antic_mode_0_xx,antic_mode_5_32,antic_mode_5_40,antic_mode_5_48},
		{antic_mode_0_xx,antic_mode_6_32,antic_mode_6_40,antic_mode_6_48},
		{antic_mode_0_xx,antic_mode_7_32,antic_mode_7_40,antic_mode_7_48},
		{antic_mode_0_xx,antic_mode_8_32,antic_mode_8_40,antic_mode_8_48},
		{antic_mode_0_xx,antic_mode_9_32,antic_mode_9_40,antic_mode_9_48},
		{antic_mode_0_xx,antic_mode_a_32,antic_mode_a_40,antic_mode_a_48},
		{antic_mode_0_xx,antic_mode_b_32,antic_mode_b_40,antic_mode_b_48},
		{antic_mode_0_xx,antic_mode_c_32,antic_mode_c_40,antic_mode_c_48},
		{antic_mode_0_xx,antic_mode_d_32,antic_mode_d_40,antic_mode_d_48},
		{antic_mode_0_xx,antic_mode_e_32,antic_mode_e_40,antic_mode_e_48},
		{antic_mode_0_xx,antic_mode_f_32,antic_mode_f_40,antic_mode_f_48},
		{antic_mode_0_xx, gtia_mode_1_32, gtia_mode_1_40, gtia_mode_1_48},
		{antic_mode_0_xx, gtia_mode_2_32, gtia_mode_2_40, gtia_mode_2_48},
		{antic_mode_0_xx, gtia_mode_3_32, gtia_mode_3_40, gtia_mode_3_48},
	},
	/*	 with hscrol enabled playfield width is +32 color clocks	  */
	/*	 no playfield	 narrow->normal  normal->wide	 wide->wide   */
	{
		{antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx},
		{antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx,antic_mode_0_xx},
		{antic_mode_0_xx,antic_mode_2_40,antic_mode_2_48,antic_mode_2_48},
		{antic_mode_0_xx,antic_mode_3_40,antic_mode_3_48,antic_mode_3_48},
		{antic_mode_0_xx,antic_mode_4_40,antic_mode_4_48,antic_mode_4_48},
		{antic_mode_0_xx,antic_mode_5_40,antic_mode_5_48,antic_mode_5_48},
		{antic_mode_0_xx,antic_mode_6_40,antic_mode_6_48,antic_mode_6_48},
		{antic_mode_0_xx,antic_mode_7_40,antic_mode_7_48,antic_mode_7_48},
		{antic_mode_0_xx,antic_mode_8_40,antic_mode_8_48,antic_mode_8_48},
		{antic_mode_0_xx,antic_mode_9_40,antic_mode_9_48,antic_mode_9_48},
		{antic_mode_0_xx,antic_mode_a_40,antic_mode_a_48,antic_mode_a_48},
		{antic_mode_0_xx,antic_mode_b_40,antic_mode_b_48,antic_mode_b_48},
		{antic_mode_0_xx,antic_mode_c_40,antic_mode_c_48,antic_mode_c_48},
		{antic_mode_0_xx,antic_mode_d_40,antic_mode_d_48,antic_mode_d_48},
		{antic_mode_0_xx,antic_mode_e_40,antic_mode_e_48,antic_mode_e_48},
		{antic_mode_0_xx,antic_mode_f_40,antic_mode_f_48,antic_mode_f_48},
		{antic_mode_0_xx, gtia_mode_1_40, gtia_mode_1_48, gtia_mode_1_48},
		{antic_mode_0_xx, gtia_mode_2_40, gtia_mode_2_48, gtia_mode_2_48},
		{antic_mode_0_xx, gtia_mode_3_40, gtia_mode_3_48, gtia_mode_3_48},
	}
};

static	void LMS(int new_cmd)
{
	/**************************************************************
	 * if the LMS bit (load memory scan) of the current display
	 * list command is set, load the video source address from the
	 * following two bytes and split it up into video page/offset
	 **************************************************************/
	if (new_cmd & ANTIC_LMS)
	{
	int addr;
		addr = RDANTIC();
		antic.doffs = ++antic.doffs & DOFFS;
		addr += RDANTIC() << 8;
		antic.doffs = ++antic.doffs & DOFFS;
		antic.vpage = addr & VPAGE;
		antic.voffs = addr & VOFFS;
#if VERBOSE
		if (errorlog) fprintf(errorlog, "ANTIC #%3d LMS $%04x\n", antic.scanline, addr);
#endif
		/* steal two more clock cycles from the cpu */
		antic.steal_cycles += 2;
	}
}
/*****************************************************************************
 *
 *	Antic Scan Line DMA
 *	This is called once per scanline from Atari Interrupt
 *	If the ANTIC DMA is active (DMA_ANTIC) and the scanline not inside
 *	the VBL range (VBL_START - TOTAL_LINES or 0 - VBL_END)
 *	check if all mode lines of the previous ANTIC command were done and
 *	if so, read a new command and set up the renderer function
 *	Also transport player/missile data to the grafp and grafm registers
 *	of the GTIA if enabled (DMA_PLAYER or DMA_MISSILE)
 *
 *****************************************************************************/
static	void	antic_scanline_dma(VIDEO *video)
{
    if ((antic.w.dmactl & DMA_ANTIC) &&
		antic.scanline >= VBL_END &&
		antic.scanline < VBL_START)
	{
        if (antic.scanline == VBL_END)
            antic.r.nmist &= ~VBL_NMI;
        if (antic.modelines <= 0)
		{
		int h = 0, w = antic.w.dmactl & 3;
		byte vscrol_subtract = 0;
		byte new_cmd;
			/* steal at least one clock cycle from the cpu */
			antic.steal_cycles += 1;
			new_cmd = RDANTIC();
			antic.doffs = ++antic.doffs & DOFFS;
#if VERBOSE
			if (errorlog) fprintf(errorlog, "ANTIC #%3d CMD $%02x\n", antic.scanline, new_cmd);
#endif
			/* command 1 .. 15 ? */
			if (new_cmd & ANTIC_MODE)
			{
				antic.w.chbasl = 0;
                /* vscroll mode changed ? */
				if ((antic.cmd ^ new_cmd) & ANTIC_VSCR)
				{
					/* activate now ? */
					if (new_cmd & ANTIC_VSCR)
					{
						antic.vscrol_old =
						vscrol_subtract =
						antic.w.chbasl = antic.w.vscrol;
					}
					else
					{
						vscrol_subtract = ~antic.vscrol_old;
					}
				}
				/* does this command have hscrol enabled ? */
				if (new_cmd & ANTIC_HSCR)
				{
					h = 1;
					antic.hscrol_old = antic.w.hscrol;
				}
				else
				{
					antic.hscrol_old = 0;
                }
            }
			/* set the mode renderer function pointer */
			antic_renderer = renderer[h][new_cmd & ANTIC_MODE][w];

			switch (new_cmd & ANTIC_MODE)
			{
				case 0:
					/* generate 1 .. 8 empty lines */
					antic.modelines = ((new_cmd >> 4) & 7) + 1;
					/* did the last antic command have vscrol enabled ? */
					if (antic.cmd & ANTIC_VSCR)
					{
						/* case generate old vscrol value times additional lines */
						antic.modelines += antic.vscrol_old;
					}
					/* leave only bit 7 as antic command */
					new_cmd &= ANTIC_DLI;
                    break;
                case 1:
					/* ANTIC jump with DLI: issue interrupt immediately */
					if (new_cmd & ANTIC_DLI)
					{
						/* remove the DLI bit */
						new_cmd &= ~ANTIC_DLI;
						/* display list interrupt enabled ? */
						if (antic.w.nmien & DLI_NMI)
						{
#if VERBOSE
							if (errorlog) fprintf(errorlog, "ANTIC #%3d DLI NMI\n", antic.scanline);
#endif
							antic.r.nmist |= DLI_NMI;
							after(CYCLES_DLI_NMI, video, antic_issue_dli);
						}
                    }
                    /* load memory scan bit set ? */
					if (new_cmd & ANTIC_LMS)
					{
						/* produce empty scanlines until vblank start */
						antic.modelines = VBL_START + 1 - antic.scanline;
						if (antic.modelines < 0)
							antic.modelines = TOTAL_LINES - antic.scanline;
#if VERBOSE
						if (errorlog) fprintf(errorlog, "ANTIC #%3d JVB ", antic.scanline);
#endif
                    }
					else
					{
						/* produce a single empty scanline */
						antic.modelines = 1;
#if VERBOSE
						if (errorlog) fprintf(errorlog, "ANTIC #%3d JMP ", antic.scanline);
#endif
                    }
					/* fetch new display list and video address */
                    {
					int addr;
						addr = RDANTIC();
						antic.doffs = ++antic.doffs & DOFFS;
                        addr += RDANTIC() << 8;
						antic.dpage = addr & DPAGE;
						antic.doffs = addr & DOFFS;
                    }
#if VERBOSE
					if (errorlog) fprintf(errorlog, "$%04x\n", antic.dpage + antic.doffs);
#endif
                    break;
                case  2:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if (antic.w.chactl & 4)
						antic.w.chbasl = antic.modelines - 1;
                    break;
                case  3:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 10 - (vscrol_subtract & 9);
					if (antic.w.chactl & 4)
                        antic.w.chbasl = antic.modelines - 1;
                    break;
                case  4:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if (antic.w.chactl & 4)
						antic.w.chbasl = antic.modelines - 1;
                    break;
                case  5:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 16 - (vscrol_subtract & 15);
					if (antic.w.chactl & 4)
						antic.w.chbasl = antic.modelines - 1;
                    break;
                case  6:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfe) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if (antic.w.chactl & 4)
						antic.w.chbasl = antic.modelines - 1;
                    break;
                case  7:
					LMS(new_cmd);
                    antic.chbase = (antic.w.chbash & 0xfe) << 8;
					antic.modelines = 16 - (vscrol_subtract & 15);
					if (antic.w.chactl & 4)
						antic.w.chbasl = antic.modelines - 1;
                    break;
                case  8:
					LMS(new_cmd);
                    antic.modelines = 8 - (vscrol_subtract & 7);
                    break;
				case  9:
					LMS(new_cmd);
                    antic.modelines = 4 - (vscrol_subtract & 3);
                    break;
				case 10:
					LMS(new_cmd);
                    antic.modelines = 4 - (vscrol_subtract & 3);
                    break;
				case 11:
					LMS(new_cmd);
                    antic.modelines = 2 - (vscrol_subtract & 1);
                    break;
				case 12:
					LMS(new_cmd);
                    antic.modelines = 1;
                    break;
				case 13:
					LMS(new_cmd);
                    antic.modelines = 2 - (vscrol_subtract & 1);
                    break;
				case 14:
					LMS(new_cmd);
                    antic.modelines = 1;
                    break;
				case 15:
					LMS(new_cmd);
                    /* bits 6+7 of the priority select register determine */
					/* if newer GTIA or plain graphics modes are used	  */
					switch (gtia.w.prior >> 6)
					{
						case 0: break;
						case 1: antic_renderer = renderer[h][16][w];  break;
						case 2: antic_renderer = renderer[h][17][w];  break;
						case 3: antic_renderer = renderer[h][18][w];  break;
					}
					antic.modelines = 1;
                    break;
			}
			/* set new (current) antic command */
			antic.cmd = new_cmd;
        }
    }
	else
	{
		antic.cmd = 0x00;
		antic_renderer = antic_mode_0_xx;
	}

    /* if player/missile graphics is enabled */
	if (antic.scanline < 256 && (antic.w.dmactl & (DMA_PLAYER|DMA_MISSILE)))
	{
		/* new player/missile graphics data for every scanline ? */
		if (antic.w.dmactl & DMA_PM_DBLLINE)
		{
			/* transport missile data to GTIA ? */
            if (antic.w.dmactl & DMA_MISSILE)
			{
				antic.steal_cycles += 1;
				MWA_GTIA(0x11, RDPMGFXD(3*256));
			}
            /* transport player data to GTIA ? */
			if (antic.w.dmactl & DMA_PLAYER)
            {
				antic.steal_cycles += 4;
                MWA_GTIA(0x0d, RDPMGFXD(4*256));
				MWA_GTIA(0x0e, RDPMGFXD(5*256));
				MWA_GTIA(0x0f, RDPMGFXD(6*256));
				MWA_GTIA(0x10, RDPMGFXD(7*256));
            }
        }
		else
        /* new player/missile graphics data only every other scanline */
		if ((antic.scanline & 1) == 0)		/* even line ? */
		{
            /* transport missile data to GTIA ? */
            if (antic.w.dmactl & DMA_MISSILE)
			{
				antic.steal_cycles += 1;
                MWA_GTIA(0x11, RDPMGFXS(3*128));
			}
            /* transport player data to GTIA ? */
			if (antic.w.dmactl & DMA_PLAYER)
			{
				antic.steal_cycles += 4;
                MWA_GTIA(0x0d, RDPMGFXS(4*128));
				MWA_GTIA(0x0e, RDPMGFXS(5*128));
				MWA_GTIA(0x0f, RDPMGFXS(6*128));
				MWA_GTIA(0x10, RDPMGFXS(7*128));
			}
        }
	}
}


/*****************************************************************************
 *
 *	Atari Scan Line Done
 *	This is called once per scanline by a interrupt issued in the
 *	atari_steal_cycles_done function.
 *  Refresh the actual display (translate color clocks to host pixels)
 *	Release the CPU if it was halted by an access to WSYNC (D01A)
 *	Increment the scan line and wrap around at TOTAL_LINES
 *
 *****************************************************************************/
void atari_scanline_done(VIDEO *video)
{
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d @cycle #%d scanline done\n", antic.scanline, cycle());
#endif
    if (antic.w.wsync)
	{
#if VERBOSE
		if (errorlog) fprintf(errorlog, "ANTIC #%3d wsync release @ cycle #%d\n", antic.scanline, cycle());
#endif
		/* release the CPU if it was actually waiting for HSYNC */
		cpu_trigger(TRIGGER_HSYNC);
		/* and turn off the 'wait for hsync' flag */
        antic.w.wsync = 0;
    }

	/* refresh the display (translate color clocks to pixels) */
    antic_linerefresh(video);

    /* increment the scanline */
	if (++antic.scanline == TOTAL_LINES)
    {
        /* and return to the top if it was done */
        antic.scanline = 0;
        antic.modelines = 0;
        /* count frames gone since last write to hitclr */
		gtia.h.hitclr_frames++;
    }
}

/*****************************************************************************
 *
 *	Atari Steal Cycles
 *	This is called once per scanline by a interrupt issued in the
 *	atari_scanline_render function. Set a new timer for the HSYNC
 *	position and release the CPU; but hold it again immediately until
 *	TRIGGER_HSYNC if WSYNC (D01A) was accessed
 *
 *****************************************************************************/
void atari_steal_cycles_done(VIDEO *video)
{
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d @cycle #%d steal_cycles_done\n", antic.scanline, cycle());
#endif
    /* release the CPU again */
    cpu_trigger(TRIGGER_STEAL);
	/* but hold the CPU again for TRIGGER_HSYNC if wsync was accessed */
	if (antic.w.wsync)
	{
		timer_holdcpu_trigger(0, TRIGGER_HSYNC);
#if VERBOSE
		if (errorlog) fprintf(errorlog, "ANTIC #%3d steal cycles: wsync active @ cycle #%d\n", antic.scanline, cycle());
#endif
    }
#if VERBOSE
	else
	if (errorlog) fprintf(errorlog, "ANTIC #%3d steal cycles: to go #%d @ cycle #%d\n", antic.scanline, CYCLES_HSYNC - antic.steal_cycles, cycle());
#endif
	/* set a new timer for the HSYNC position */
    after(CYCLES_HSYNC - antic.steal_cycles, video, atari_scanline_done);
	/* reset the steal cycles count to the cycles used for refresh */
    antic.steal_cycles = 0;
}

/*****************************************************************************
 *
 *	Atari Scan Line Render
 *	This is called once per scanline by a interrupt issued in the
 *	atari_scanline_dma function:
 *	handle display list interrupt on the last scan line of a ANTIC mode
 *	ANTIC render the actual mode line
 *	GTIA render player / missile graphics and update collisions
 *	hold CPU for cycles stolen by ANTICs DMA accesses and set an
 *	interrupt after antic.steal_cycles to release the CPU again
 *
 *****************************************************************************/
void atari_scanline_render(VIDEO *video)
{
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d @cycle #%d render\n", antic.scanline, cycle());
#endif

	antic_handle_dli(video);

    (*antic_renderer)(video);

#if OPTIMIZE
	/* Do we need to recalculate p/m graphics and collisions too ? */
    if (video->dirty)
	{
        gtia_render(video);
		/* copy collisions registers to the per scanline buffer */
		*(dword*)&video->gtia_r.m0pf = *(dword*)&gtia.r.m0pf;
		*(dword*)&video->gtia_r.p0pf = *(dword*)&gtia.r.p0pf;
		*(dword*)&video->gtia_r.m0pl = *(dword*)&gtia.r.m0pl;
		*(dword*)&video->gtia_r.p0pl = *(dword*)&gtia.r.p0pl;
    }
	else
	{
		/* if hitclr was written less than two freames ago */
		if (gtia.h.hitclr_frames < 2)
		{
			/* mask in scanline buffer to the collisions registers */
			*(dword*)&gtia.r.m0pf |= *(dword*)&video->gtia_r.m0pf;
			*(dword*)&gtia.r.p0pf |= *(dword*)&video->gtia_r.p0pf;
			*(dword*)&gtia.r.m0pl |= *(dword*)&video->gtia_r.m0pl;
			*(dword*)&gtia.r.p0pl |= *(dword*)&video->gtia_r.p0pl;
		}
    }
#else
	gtia_render(video);
#endif

	antic.steal_cycles += CYCLES_REFRESH;
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d steal %d cycles \n", antic.scanline, antic.steal_cycles);
#endif
    timer_holdcpu_trigger(0, TRIGGER_STEAL);
	after(antic.steal_cycles, video, atari_steal_cycles_done);
}

/*****************************************************************************
 *
 *  Atari Interrupt Dispatcher
 *	This is called once per scanline and handles:
 *	vertical blank interrupt
 *	ANTIC dma to possibly access the next display list command
 *
 *****************************************************************************/
int  atari_interrupt(void)
{
VIDEO * video = antic.video[antic.scanline];
#if VERBOSE
	if (errorlog) fprintf(errorlog, "ANTIC #%3d @cycle #%d interrupt\n", antic.scanline, cycle());
#endif

    if (antic.scanline < VBL_START)
	{
		antic_scanline_dma(video);
		after(CYCLES_HSTART, video, atari_scanline_render);
    }
	else
	if (antic.scanline == VBL_START)
	{
		if (!(gtia.w.gractl & GTIA_TRIGGER))
			gtia.r.but0 = gtia.r.but1 = gtia.r.but2 = gtia.r.but3 = 1;
		gtia.r.but0 &= (input_port_3_r(0) >> 0) & 1;
		gtia.r.but1 &= (input_port_3_r(0) >> 1) & 1;
		gtia.r.but2 &= (input_port_3_r(0) >> 2) & 1;
		gtia.r.but3 &= (input_port_3_r(0) >> 3) & 1;

		/* It would be really nice to have osd_ callbacks :( */
		switch (Machine->gamedrv->name[0])
		{
			case 'a':   /* running atari 400/800 emulation */
				atari800_handle_keyboard();
				break;
			case 'v':   /* running 5200 emulation */
				vcs5200_handle_keypads();
				break;
		}

		/* do nothing new for the rest of the frame */
		antic.modelines = TOTAL_LINES - VBL_START;
		antic_renderer = antic_mode_0_xx;

		/* if the CPU want's to be interrupted at vertical blank... */
		if (antic.w.nmien & VBL_NMI)
		{
#if VERBOSE
			if (errorlog) fprintf(errorlog, "ANTIC #%3d cause VBL NMI\n", antic.scanline);
#endif
			/* set the VBL NMI status bit */
			antic.r.nmist |= VBL_NMI;
			cpu_cause_interrupt(0, M6502_INT_NMI);
		}
		atari_scanline_done(video);
    }
	else
	{
		atari_scanline_done(video);
	}
	return M6502_INT_NONE;
}

