#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/pocketc.h"

/* pc140x
   16 5x7 with space between char
   6000 .. 6027, 6067.. 6040
  603c: 3 STAT
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 HYP, 4 PRO, 5 RUN, 6 CAL
  607c: 0 E, 1 M, 2 (), 3 RAD, 4 G, 5 DE, 6 PRINT */

/* pc1421
   16 5x7 with space between char
   6000 .. 6027, 6067.. 6040
  603c: 3 RUN
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 BGN, 4 STAT, 5 FIN, 6 PRINT
  607c: 0 E, 1 M, 2 BAL, 3 INT, 4 PRN, 5 Sum-Sign, 6 PRO */

/* PC126x
   24x2 5x7 space between char
   2000 .. 203b, 2800 .. 283b
   2040 .. 207b, 2840 .. 287b
  203d: 0 BUSY, 1 PRINT, 3 JAPAN, 4 SMALL, 5 SHIFT, 6 DEF
  207c: 1 DEF 1 RAD 2 GRAD 5 ERROR 6 FLAG */

/* pc1350
   24x4 5x8 no space between chars
   7000 .. 701d, 7200..721d, 7400 ..741d, 7600 ..761d, 7800 .. 781d
   7040 .. 705d, 7240..725d, 7440 ..745d, 7640 ..765d, 7840 .. 785d
   701e .. 703b, 721e..723b, 741e ..743b, 761e ..763b, 781e .. 783b
   705e .. 707b, 725e..727b, 745e ..747b, 765e ..767b, 785e .. 787b
   783c: 0 SHIFT 1 DEF 4 RUN 5 PRO 6 JAPAN 7 SML */

int pocketc_vh_start(void)
{
	return 0;
}

void pocketc_vh_stop(void)
{
}

static struct {
	UINT8 reg[0x100];
} pc1401_lcd;

READ_HANDLER(pc1401_lcd_read)
{
	offset&=0xff;
	return pc1401_lcd.reg[offset];
}

WRITE_HANDLER(pc1401_lcd_write)
{
	offset&=0xff;
	pc1401_lcd.reg[offset]=data;
}

typedef char *FIGURE[];
static const FIGURE
line={ "111","111e" },/* simple line */

	busy={ "11  1 1  11 1 1",
		   "1 1 1 1 1   1 1",
		   "11  1 1  1  1 1",
		   "1 1 1 1   1  1",
		   "11   1  11   1e"},
		def={ "11  111 111",
			  "1 1 1   1",
			  "1 1 111 11",
			  "1 1 1   1",
			  "11  111 1e" },
			shift={" 11 1 1 1 111 111",
				   "1   1 1 1 1    1",
				   " 1  111 1 11   1",
				   "  1 1 1 1 1    1",
				   "11  1 1 1 1    1e" },
				hyp={"1 1 1 1 11",
					 "1 1 1 1 1 1",
					 "111 1 1 11",
					 "1 1  1  1",
					 "1 1  1  1e" },
					de={"11  111",
						"1 1 1",
						"1 1 111",
						"1 1 1",
						"11  111e"},
						g={" 11",
						   "1",
						   "1 1",
						   "1 1",
						   " 11e" },
							rad={"11   1  11",
								 "1 1 1 1 1 1",
								 "11  111 1 1",
								 "1 1 1 1 1 1",
								 "1 1 1 1 11e"},
								braces={" 1 1",
										"1   1",
										"1   1",
										"1   1",
										" 1 1e"},
									m={"1   1",
									   "11 11",
									   "1 1 1",
									   "1   1",
									   "1   1e"},
										e={"111",
										   "1",
										   "111",
										   "1",
										   "111e" };

/* size in reality
 170x72.5 mm

 lcd 16 character 5x7 with 1 column between

 lcd upper left at 32.5x15mm

*/
static void pc1401_draw_special(struct osd_bitmap *bitmap,
								int x, int y, const FIGURE fig, int color)
{
	int i,j;
	color=color?Machine->pens[10]:Machine->pens[5];
	for (i=0;fig[i];i++,y++) {
		for (j=0;fig[i][j]!=0;j++) {
			switch(fig[i][j]) {
			case '1':
				bitmap->line[y][x+j]=color;
				break;
			case 'e': return;
			}
		}
	}
}

#define DOWN 52
#define RIGHT 113
void pc1401_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y, i, j;
	for (x=RIGHT,y=DOWN-13/*?*/,i=0; i<0x28;x+=2) {
		for (j=0; j<5;j++,i++,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1401_lcd.reg[i],0,
					x,y,x+2,y+14,
					0, TRANSPARENCY_NONE,0);
	}
	for (i=0x67; i>=0x40;x+=2) {
		for (j=0; j<5;j++,i--,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1401_lcd.reg[i],0,
					x,y,x+2,y+14,
					0, TRANSPARENCY_NONE,0);
	}
	pc1401_draw_special(bitmap,RIGHT+160,DOWN+20,line,pc1401_lcd.reg[0x3c]&8);
	pc1401_draw_special(bitmap,RIGHT+2,DOWN-10,busy,pc1401_lcd.reg[0x3d]&1);
	pc1401_draw_special(bitmap,RIGHT+20,DOWN-10,def,pc1401_lcd.reg[0x3d]&2);
	pc1401_draw_special(bitmap,RIGHT+45,DOWN-10,shift,pc1401_lcd.reg[0x3d]&4);
	pc1401_draw_special(bitmap,RIGHT+65,DOWN-10,hyp,pc1401_lcd.reg[0x3d]&8);
	pc1401_draw_special(bitmap,RIGHT+40,DOWN+20,line,pc1401_lcd.reg[0x3d]&0x10);
	pc1401_draw_special(bitmap,RIGHT+25,DOWN+20,line,pc1401_lcd.reg[0x3d]&0x20);
	pc1401_draw_special(bitmap,RIGHT+10,DOWN+20,line,pc1401_lcd.reg[0x3d]&0x40);
	pc1401_draw_special(bitmap,RIGHT+185,DOWN-10,e,pc1401_lcd.reg[0x7c]&1);
	pc1401_draw_special(bitmap,RIGHT+178,DOWN-10,m,pc1401_lcd.reg[0x7c]&2);
	pc1401_draw_special(bitmap,RIGHT+170,DOWN-10,braces,pc1401_lcd.reg[0x7c]&4);
	pc1401_draw_special(bitmap,RIGHT+150,DOWN-10,rad,pc1401_lcd.reg[0x7c]&8);
	pc1401_draw_special(bitmap,RIGHT+145,DOWN-10,g,pc1401_lcd.reg[0x7c]&0x10);
	pc1401_draw_special(bitmap,RIGHT+135,DOWN-10,de,pc1401_lcd.reg[0x7c]&0x20);
	pc1401_draw_special(bitmap,RIGHT+170,DOWN+20,line,pc1401_lcd.reg[0x7c]&0x40);

/*
  603c: 3 STAT
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 HYP, 4 PRO, 5 RUN, 6 CAL
  607c: 0 E, 1 M, 2 (), 3 RAD, 4 G, 5 DE, 6 PRINT
*/
}

