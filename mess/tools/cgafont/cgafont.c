/* CGAFONT.C by John Elliott, March 2003.
   This program is public domain, please copy!
   Note: Must be compiled by a large-model compiler (I used Pacific C).
*/

#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <time.h>

unsigned char textscr[1024];
unsigned char gfxscr[16384];
unsigned char fontbuf[2048];

typedef unsigned char far *PBYTE;
PBYTE cdata;
PBYTE graftabl;
PBYTE *int1f;


void sleep(int secs)
{
	long t, t1;

	time(&t);
	while (secs)
	{
		time(&t1);
		if (t != t1)
		{
			--secs;
			t = t1;
		}
	}
}

void setcurpos(int row, int col)
{
	union REGS rg;

	rg.x.ax = 0x200;
	rg.x.bx = 0x000;
	rg.h.dl = col;
	rg.h.dh = row;
	int86(0x10, &rg, &rg);
}

void drawch(int row, int col, unsigned char c)
{
	union REGS rg;

	rg.x.ax = 0x200;
	rg.x.bx = 0x000;
	rg.h.dl = col;
	rg.h.dh = row;
	int86(0x10, &rg, &rg);

	rg.h.ah = 9;
	rg.h.al = c;
	rg.x.bx = 15;
	rg.x.cx = 1;
	int86(0x10, &rg, &rg);
}

void drawstr(int r, int c, char *s)
{
	while (*s)
	{
		drawch(r,c,*s);
		++s;
		++c;
		if (c == 40)
		{
			c = 0;
			++r;
		}
	}
}

void mkdisp(int gfx)
{
	int r,c,n;
	unsigned char ch = 0;
	unsigned char mk = gfx ? 0x80 : 0;

	*int1f = fontbuf;
	for (r=c=n=0; n < 256; n++)
	{
		drawch(r,c,ch | mk);
		++c;
		if (c == 40) { c=0; ++r; }
		++ch;
		if (n == 127) *int1f = (fontbuf + 1024);
	}

}
/* Using CGA mode 6, get the current ROS+GRAFTABL font combination */

void getfont()
{
	int n;
	unsigned char *pch = fontbuf;
	unsigned char *scr = MK_FP(0xB800, 0);

	for (n = 0; n < 256; n++)
	{
		drawch(0, 0, n);
		pch[0] = scr[0x0000];
		pch[1] = scr[0x2000];
		pch[2] = scr[0x0050];
		pch[3] = scr[0x2050];
		pch[4] = scr[0x00A0];
		pch[5] = scr[0x20A0];
		pch[6] = scr[0x00F0];
		pch[7] = scr[0x20F0];
		pch += 8;
	}
}


void setmode(char m)
{
	union REGS rg;

	rg.x.ax = m;
	int86(0x10, &rg, &rg);
}



void putscr(unsigned char *buf, int len)
{
	void *scr = MK_FP(0xB800, 0);
	memcpy(scr, buf, len);
}


void getscr(unsigned char *buf, int len)
{
	void *scr = MK_FP(0xB800, 0);
	memcpy(buf, scr, len);
}

void dotest(int slow)
{
	setmode(0);
	mkdisp(0);
	getscr(textscr, 1024);
	setmode(5);
	mkdisp(1);
	getscr(gfxscr, 16384);
	while (1)
	{
		setmode(0);
		putscr(textscr, 1024);
		if (slow) sleep(1);
		if (kbhit()) break;
		setmode(5);
		putscr(gfxscr, 16384);
		if (kbhit()) break;
		if (slow) sleep(1);
	}
	getch();	/* Swallow the kbhit() */
	setmode(0);
}


void draw_editor()
{
	int n;

	setmode(0);
	drawstr(0,0, "Character editor");
	drawch(2,  2,0xC9);
	drawch(2, 11,0xBB);
	drawch(11, 2,0xC8);
	drawch(11,11,0xBC);
	for (n=0;n<8;n++)
	{
		drawch(2,  3+n,0xCD);
		drawch(11, 3+n,0xCD);
		drawch(3+n,2,  0xC7);
		drawch(3+n,11, 0xC7);
	}
	drawstr(12,0,"Cursors and SPACE to edit");
	drawstr(13,0, "S/T to test with ROM font, ESC to exit");
}

void draw_edchar(int nc)
{
	int r,c;
	unsigned char mask;

	drawch(8, 20, nc);

	for (r=0;r<8;r++)
	{
		mask = 0x80;
		for (c = 0; c < 8; c++)
		{
			if (cdata[r] & mask) drawch(3+r,3+c,10);
			else		     drawch(3+r,3+c,7);
			mask = mask >> 1;
		}
	}
}



void edit_char(unsigned nc)
{
	static int cx=0,cy=0, mask = 0x80;

	cdata = fontbuf + 8 * nc;
	draw_editor();
	draw_edchar(nc);
	while (1)
	{
		setcurpos(3+cy,3+cx);
		switch(getch())
		{
			case '\r': case '\n':
			case 27:		return;
			case ' ' : cdata[cy] ^= mask;
				   if (cdata[cy] & mask) drawch(3+cy,3+cx,10);
				   else		         drawch(3+cy,3+cx,7);
				   break;
			case 'T': case 't': dotest(0); draw_editor(); draw_edchar(nc); break;
			case 'S': case 's': dotest(1); draw_editor(); draw_edchar(nc); break;
			case 0: switch(getch())
			{
				case 0x4B: if (cx)   { --cx; mask = mask << 1; } break;
				case 0x4D: if (cx<7) { ++cx; mask = mask >> 1; } break;
				case 0x48: if (cy)   --cy; break;
				case 0x50: if (cy<7) ++cy; break;
			}
		}
	}
}

void draw_chooser()
{
	setmode(0);
	drawstr(0, 0, "Choose character to edit");
		/*	1...5...10...15...20...25...30...35...40 */
	drawstr(10, 0, "Cursor keys to select, ENTER to edit");
	drawstr(11, 0, "W to write font dump to disc");
	drawstr(12, 0, "S/T to test with ROM font, ESC to exit");
}

void wrfont()
{
	int okay = 1;

	FILE *fp = fopen("cgafont.bin", "rb");
	if (fp)
	{
		char c;

		fclose(fp);
		drawstr(14, 0, "CGAFONT.BIN exists -- delete (Y/N)?");
		do
		{
			c = getch();
			if (c == 'N' || c == 'n')
			{
				drawstr(14, 0, "                                        ");
				return;
			}
		} while (c != 'y' && c != 'Y');
		drawstr(14, 0, "                                        ");
		remove("cgafont.bin");
	}
	fp = fopen("cgafont.bin", "wb");
	if (fp)
	{
		okay = (fwrite(fontbuf, 1, 2048, fp) == 2048);
		fclose(fp);
	}
	else okay = 0;
	if (okay) drawstr(14, 0, "CGAFONT.BIN has been saved.[press SPACE]");
	else	  drawstr(14, 0, "Save failed.               [press SPACE]");
	getch();
	drawstr(14, 0,           "                                        ");
}


int choose_char()
{
	static unsigned int nchar = 0;
	char buf[8];

	draw_chooser();
	while (1)
	{
		sprintf(buf, "%03d: %c", nchar, nchar);
        	drawstr(5, 5, buf);
		switch(getch())
		{
			case 'T': case 't': dotest(0); draw_chooser(); break;
			case 'S': case 's': dotest(1); draw_chooser(); break;
			case 'W': case 'w': wrfont(); break;
			case 27: return -1;
			case '\n': case '\r':
				return nchar;
			case 0: switch(getch())
				{
					case 0x4D: if (nchar < 255) ++nchar; break;
					case 0x4B: if (nchar > 0) --nchar; break;
				}
		}
	}
	return -1;
}


int main(int argc, char **argv)
{
	int nc;

	int1f = MK_FP(0, 0x7C);
	setmode(5);
	graftabl = *int1f;
	setmode(6);
	getfont();
	setmode(0);

	while ((nc = choose_char()) != -1)
	{
		edit_char(nc);
	}

	setmode(5);
	*int1f = graftabl;
	setmode(2);

	return 0;
}
