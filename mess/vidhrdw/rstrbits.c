#include <assert.h>
#include "includes/rstrbits.h"
#include "mame.h"
#include "driver.h"

#define myMIN(a, b) ((a) < (b) ? (a) : (b))
#define myMAX(a, b) ((a) > (b) ? (a) : (b))

#define NEW_GFX	1

#if !NEW_GFX
/* -------------------------------------------------------------------------
 * These functions were taken out of the CoCo/M6847 support, currently what
 * raster_bits() does is looks at the args, and calls one of these
 * functions.  The problem is that this leaves many holes in raster_bits()'s
 * functionality, but as time goes on, these holes will probably be filled
 * ------------------------------------------------------------------------- */

/*
 * Note that 'sizex' is in bytes, and 'sizey' is in pixels
 */
static void blitgraphics2(struct osd_bitmap *bitmap, struct rasterbits_clip *clip,
	UINT8 *vrambase, int vrampos, int offset, int loopbackpos, int loopbackadj,
	int vramsize, UINT8 *db, UINT16 *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int fg, bg;
	int p, b;
	int pixtop, pixbottom;
	UINT8 *vidram;

	if (metapalette) {
		bg = Machine->pens[metapalette[0]];
		fg = Machine->pens[metapalette[1]];
	}
	else {
		bg = Machine->pens[0];
		fg = Machine->pens[1];
	}

	vidram = vrambase + vrampos;

	if (offset) {
		vidram += offset;
		if (db)
			db += offset;
	}

	for (y = 0; y < sizey; y++) {
		pixtop = basey + y * scaley;
		pixbottom = pixtop + scaley - 1;
		pixtop = myMAX(pixtop, clip->ybegin);
		pixbottom = myMIN(pixbottom, clip->yend);

		if (pixbottom >= pixtop) {
			for (x = 0; x < sizex; x++) {
				/* Check loopback; this is used to support $FF9F in the CoCo 3 */
				if (x == loopbackpos) {
					vidram += loopbackadj;
					if (db)
						db += loopbackadj;
				}

				if (!db || *db) {
					for (b = 0; b < 8; b++) {
						p = ((*vidram) & (1<<(7-b))) ? fg : bg;
						plot_box(bitmap, (x * 8 + b) * scalex + basex, pixtop, scalex, (pixbottom - pixtop + 1), p);
					}
					if (db)
						*(db++) = 0;
				}
				else {
					db++;
				}
				vidram++;
			}
		}
		else {
			vidram += sizex + loopbackadj;
			if (db)
				db += sizex + loopbackadj;
		}

		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics4(struct osd_bitmap *bitmap, struct rasterbits_clip *clip,
	UINT8 *vrambase, int vrampos, int offset, int loopbackpos, int loopbackadj,
	int vramsize, UINT8 *db, UINT16 *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int c[4];
	int p, b;
	int crunlen, crunc =0, thisx =0;
	int pixtop, pixbottom;
	UINT8 *vidram;

	if (metapalette) {
		c[0] = Machine->pens[metapalette[0]];
		c[1] = Machine->pens[metapalette[1]];
		c[2] = Machine->pens[metapalette[2]];
		c[3] = Machine->pens[metapalette[3]];
	}
	else {
		c[0] = Machine->pens[0];
		c[1] = Machine->pens[1];
		c[2] = Machine->pens[2];
		c[3] = Machine->pens[3];
	}

	vidram = vrambase + vrampos;

	if (offset) {
		vidram += offset;
		if (db)
			db += offset;
	}

	for (y = 0; y < sizey; y++) {
		crunlen = 0;

		pixtop = basey + y * scaley;
		pixbottom = pixtop + scaley - 1;
		pixtop = myMAX(pixtop, clip->ybegin);
		pixbottom = myMIN(pixbottom, clip->yend);

		if (pixbottom >= pixtop) {
			for (x = 0; x < sizex; x++) {
				/* Check loopback; this is used to support $FF9F in the CoCo 3 */
				if (x == loopbackpos) {
					vidram += loopbackadj;
					if (db)
						db += loopbackadj;
				}

				if (!db || *db) {
					for (b = 0; b < 4; b++) {
						p = (((*vidram) >> (6-(2*b)))) & 3;

						if (crunlen == 0) {
							thisx = (x * 4 + b) * scalex + basex;
							crunlen = 1;
							crunc = p;
						}
						else if (crunc == p) {
							crunlen++;
						}
						else {
							plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), c[crunc]);
							thisx += scalex * crunlen;
							crunlen = 1;
							crunc = p;
						}
					}
					if (db)
						*(db++) = 0;
				}
				else {
					if (crunlen) {
						plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), c[crunc]);
						crunlen = 0;
					}
					db++;
				}
				vidram++;
			}
			if (crunlen) {
				plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), c[crunc]);
				crunlen = 0;
			}
		}
		else {
			vidram += sizex + loopbackadj;
			if (db)
				db += sizex + loopbackadj;
		}

		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics4artifact(struct osd_bitmap *bitmap, struct rasterbits_clip *clip,
	UINT8 *vrambase,
	int vrampos, int offset, int loopbackpos, int loopbackadj,
	int vramsize, UINT8 *db, UINT16 *metapalette, int sizex,
	int sizey, int basex, int basey, int scalex, int scaley)
{
	/* Arifacting isn't truely the same resolution as PMODE 3
	 * it has a bias to the higher resolution.  We need to
	 * emulate this because some things are unreadable if we
	 * say that its just like PMODE 3 with different colors
	 */

	/* This is the blur correction table. For any given pixel,
	 * you can take color of that pixel, the color to the left
	 * and right, and compute what the two resulting pixels
	 * should look like.  The table is accessed like this:
	 *
	 *		artifactcorrection[pix(x-1,y)*16+pix(x,y)*4+pix(x+1,y)]
	 *
	 * This gives you a pair of pixels to display pix(x,y) as
	 *
	 * Special thanks to Derek Snider for coming up with the
	 * basis for the values in this table
	 */
	static int artifactcorrection[64][2] = {
		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 1}, {0, 1}, {0, 3}, {0, 3},
		{2, 0}, {2, 0}, {2, 2}, {2, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{1, 1}, {1, 1}, {1, 3}, {1, 3},
		{3, 0}, {3, 0}, {3, 2}, {3, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 1}, {0, 1}, {0, 3}, {0, 3},
		{2, 0}, {2, 0}, {2, 2}, {2, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{1, 1}, {1, 1}, {1, 3}, {1, 3},
		{3, 0}, {3, 0}, {3, 2}, {3, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3}
	};

	int x, y;
	int c[4];
	int b;
	int c1, c2;
	int crunc =0, crunlen;
	int drunlen;
	int thisx;
	int pixtop, pixbottom;
	UINT8 *vidram;
	UINT32 w;
	int *blur;

	c[0] = Machine->pens[metapalette[0]];
	c[1] = Machine->pens[metapalette[1]];
	c[2] = Machine->pens[metapalette[2]];
	c[3] = Machine->pens[metapalette[3]];

	vidram = vrambase + vrampos;

	if (offset) {
		vidram += offset;
		if (db)
			db += offset;
	}

	/* NYI - Note! We are not supporting loopback yet!!! */

	for (y = 0; y < sizey; y++) {
		pixtop = basey + y * scaley;
		pixbottom = pixtop + scaley - 1;
		pixtop = myMAX(pixtop, clip->ybegin);
		pixbottom = myMIN(pixbottom, clip->yend);

		if (pixbottom >= pixtop) {
			x = 0;
			while(x < sizex) {
				if (!db || db[0] || ((x < (sizex-1)) && db[1])) {
					/* We are in a run; calculate the size of the run */
					if (db) {
						drunlen = 1;
						while((x + drunlen < (sizex-1)) && db[drunlen])
							drunlen++;
						if (x + drunlen < (sizex-1))
							drunlen++;
					}
					else {
						drunlen = sizex;
					}

					thisx = (x * 8) * scalex + basex;

					/* Artifacting causes pixels to appear differently depending on
					 * their neighbors.  Thus we use 'w' to hold all bits in the
					 * vicinity:
					 *
					 * bits 23-16: previous byte
					 * bits 15-08: current byte
					 * bits 07-00: next byte
					 *
					 * As we plot the run, we shift 'w' left, and bring in new data
					 */
					w = (((UINT32) vidram[0]) << 8);
					if (x == 0)
						w |= (w << 2) & 0x030000;
					else
						w |= ((UINT32) vidram[-1]) << 16;

					crunlen = 0;
					if (db) {
						memset(db, 0, drunlen);
						db += drunlen;
					}
					x += drunlen;

					while(drunlen--) {
						/* Bring new data into 'w' */
						if (drunlen || (x < sizex))
							w |= (UINT32) vidram[1];
						else
							w |= (w >> 2) & 0x0000e0;

						for (b = 0; b < 4; b++) {
							blur = artifactcorrection[(w & 0x3f000) >> 12];
							c1 = c[blur[0]];
							c2 = c[blur[1]];
							w <<= 2;

							if (crunlen == 0) {
								crunlen = 1;
								crunc = c1;
							}
							else if (crunc == c1) {
								crunlen++;
							}
							else {
								plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), crunc);
								thisx += scalex * crunlen;
								crunlen = 1;
								crunc = c1;
							}
							if (crunc == c2) {
								crunlen++;
							}
							else {
								plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), crunc);
								thisx += scalex * crunlen;
								crunlen = 1;
								crunc = c2;
							}
						}
						vidram++;
					}
					plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), crunc);
				}
				else {
					if (db)
						db++;
					vidram++;
					x++;
				}
			}
		}
		else {
			vidram += sizex + loopbackadj;
			if (db)
				db += sizex + loopbackadj;
		}

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics16(struct osd_bitmap *bitmap, struct rasterbits_clip *clip,
	UINT8 *vrambase,
	int vrampos, int offset, int loopbackpos, int loopbackadj,
	int vramsize, UINT8 *db, int sizex, int sizey, int basex,
	int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int thisx = 0;
	int pixtop, pixbottom;
	int crunlen, crunc =0;
	UINT8 *vidram;
	UINT8 b;

	vidram = vrambase + vrampos;

	if (offset) {
		vidram += offset;
		if (db)
			db += offset;
	}

	for (y = 0; y < sizey; y++) {
		crunlen = 0;

		pixtop = basey + y * scaley;
		pixbottom = pixtop + scaley - 1;
		pixtop = myMAX(pixtop, clip->ybegin);
		pixbottom = myMIN(pixbottom, clip->yend);

		if (pixbottom >= pixtop) {
			for (x = 0; x < sizex; x++) {
				/* Check loopback; this is used to support $FF9F in the CoCo 3 */
				if (x == loopbackpos) {
					vidram += loopbackadj;
					if (db)
						db += loopbackadj;
				}

				if (!db || *db) {
					b = *vidram;

					if (crunlen == 0) {
						thisx = (x * 2) * scalex + basex;
						crunlen = 1;
						crunc = b >> 4;
					}
					else if (crunc == (b >> 4)) {
						crunlen++;
					}
					else {
						plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), Machine->pens[crunc]);
						thisx += scalex * crunlen;
						crunlen = 1;
						crunc = b >> 4;
					}

					if (crunc == (b & 15)) {
						crunlen++;
					}
					else {
						plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), Machine->pens[crunc]);
						thisx += scalex * crunlen;
						crunlen = 1;
						crunc = b & 15;
					}

					if (db)
						*(db++) = 0;
				}
				else {
					db++;

					if (crunlen > 0) {
						plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), Machine->pens[crunc]);
						crunlen = 0;
					}
				}
				vidram++;
			}

			if (crunlen > 0) {
				plot_box(bitmap, thisx, pixtop, scalex * crunlen, (pixbottom - pixtop + 1), Machine->pens[crunc]);
				crunlen = 0;
			}
		}
		else {
			vidram += sizex + loopbackadj;
			if (db)
				db += sizex + loopbackadj;
		}

		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}
#endif /* !NEW_GFX */

#if NEW_GFX
/* -------------------------------------------------------------------------
 * New raster_graphic call
 * ------------------------------------------------------------------------- */

static int isrowdirty(UINT8 *db, int rowbytes)
{
	int i;

	if (!db)
		return TRUE;

	for (i = 0; i < rowbytes; i++) {
		if (db[i]) {
			memset(&db[i], 0, rowbytes - i);
			return TRUE;
		}
	}
	return FALSE;
}

static void build_scanline4_4(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;

	for (i = 0; i < length; i++) {
		c = *(vram++);
		scanline[0] = scanline[1] = scanline[2] = scanline[3] = pens[(c & 0xf0) >> 4];
		scanline[4] = scanline[5] = scanline[6] = scanline[7] = pens[(c & 0x0f) >> 0];
		scanline += 8;
	}
}

static void build_scanline4_2(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;

	for (i = 0; i < length; i++) {
		c = *(vram++);
		scanline[0] = scanline[1] = pens[(c & 0xf0) >> 4];
		scanline[2] = scanline[3] = pens[(c & 0x0f) >> 0];
		scanline += 4;
	}
}

static void build_scanline4_1(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;

	for (i = 0; i < length; i++) {
		c = *(vram++);
		scanline[0] = pens[(c & 0xf0) >> 4];
		scanline[1] = pens[(c & 0x0f) >> 0];
		scanline += 2;
	}
}

static void build_scanline4(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;

	for (i = 0; i < length; i++) {
		c = *(vram++);
		memset(scanline, pens[(c & 0xf0) >> 4], scale);	scanline += scale;
		memset(scanline, pens[(c & 0x0f) >> 0], scale);	scanline += scale;
	}
}

static void build_scanline2(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;

	for (i = 0; i < length; i++) {
		c = *(vram++);
		memset(scanline, pens[(c & 0xc0) >> 6], scale);	scanline += scale;
		memset(scanline, pens[(c & 0x30) >> 4], scale);	scanline += scale;
		memset(scanline, pens[(c & 0x0c) >> 2], scale);	scanline += scale;
		memset(scanline, pens[(c & 0x03) >> 0], scale);	scanline += scale;
	}
}

static void build_scanline2a(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	/* Arifacting isn't truely the same resolution as PMODE 3
	 * it has a bias to the higher resolution.  We need to
	 * emulate this because some things are unreadable if we
	 * say that its just like PMODE 3 with different colors
	 */

	/* This is the blur correction table. For any given pixel,
	 * you can take color of that pixel, the color to the left
	 * and right, and compute what the two resulting pixels
	 * should look like.  The table is accessed like this:
	 *
	 *		artifactcorrection[pix(x-1,y)*16+pix(x,y)*4+pix(x+1,y)]
	 *
	 * This gives you a pair of pixels to display pix(x,y) as
	 *
	 * Special thanks to Derek Snider for coming up with the
	 * basis for the values in this table
	 */
	static int artifactcorrection[64][2] = {
		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 1}, {0, 1}, {0, 3}, {0, 3},
		{2, 0}, {2, 0}, {2, 2}, {2, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{1, 1}, {1, 1}, {1, 3}, {1, 3},
		{3, 0}, {3, 0}, {3, 2}, {3, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 1}, {0, 1}, {0, 3}, {0, 3},
		{2, 0}, {2, 0}, {2, 2}, {2, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3},

		{0, 0}, {0, 0}, {0, 0}, {0, 0},
		{1, 1}, {1, 1}, {1, 3}, {1, 3},
		{3, 0}, {3, 0}, {3, 2}, {3, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3}
	};

	/* Artifacting causes pixels to appear differently depending on
	 * their neighbors.  Thus we use 'w' to hold all bits in the
	 * vicinity:
	 *
	 * bits 23-16: previous byte
	 * bits 15-08: current byte
	 * bits 07-00: next byte
	 *
	 * As we plot the run, we shift 'w' left, and bring in new data
	 *
	 * Given this map - these bit fields are used for the four pixels:
	 *
	 * pixel 0: bits 17-12
	 * pixel 1: bits 15-10
	 * pixel 2: bits 13-08
	 * pixel 3: bits 11-06
	 *
	 * Note that we assume that on either sides of the scanline, the 'data'
	 * would be 0xff.  This is always true, unless if we were affected by
	 * loopback (fortunately the way the CoCo driver works now, this will
	 * never happen
	 */

	UINT32 w;
	int i;
	int *b;

	w = ((UINT32) vram[0]) | 0xff00;

	for (i = 0; i < length; i++) {
		w <<= 8;
		w |= (i+1) == length ? 0xff : *(++vram);

		/* We have a switch statement to test for common cases; solid colors */
		switch(w & 0x03ffc0) {
		case 0x000000:
			/* Solid 0 */
			memset(scanline, pens[0], scale * 8);
			scanline += scale * 8;
			break;

		case 0x015540:
			/* Solid 1 */
			memset(scanline, pens[1], scale * 8);
			scanline += scale * 8;
			break;

		case 0x02aa80:
			/* Solid 2 */
			memset(scanline, pens[2], scale * 8);
			scanline += scale * 8;
			break;

		case 0x03ffc0:
			/* Solid 3 */
			memset(scanline, pens[3], scale * 8);
			scanline += scale * 8;
			break;

		default:
			b = artifactcorrection[(w >> 12) & 0x3f];
			memset(scanline, pens[b[0]], scale);	scanline += scale;
			memset(scanline, pens[b[1]], scale);	scanline += scale;
			b = artifactcorrection[(w >> 10) & 0x3f];
			memset(scanline, pens[b[0]], scale);	scanline += scale;
			memset(scanline, pens[b[1]], scale);	scanline += scale;
			b = artifactcorrection[(w >>  8) & 0x3f];
			memset(scanline, pens[b[0]], scale);	scanline += scale;
			memset(scanline, pens[b[1]], scale);	scanline += scale;
			b = artifactcorrection[(w >>  6) & 0x3f];
			memset(scanline, pens[b[0]], scale);	scanline += scale;
			memset(scanline, pens[b[1]], scale);	scanline += scale;
			break;
		}
	}
}

static void build_scanline1(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
{
	UINT8 c;
	int i;
	UINT8 fg, bg;

	bg = pens[0];
	fg = pens[1];

	for (i = 0; i < length; i++) {
		c = *(vram++);
		memset(scanline, (c & 0x80) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x40) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x20) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x10) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x08) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x04) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x02) ? fg : bg, scale);	scanline += scale;
		memset(scanline, (c & 0x01) ? fg : bg, scale);	scanline += scale;
	}
}

static void raster_graphics(struct osd_bitmap *bitmap, struct rasterbits_source *src,
	struct rasterbits_videomode *mode, struct rasterbits_clip *clip,
	int scalex, int scaley, int basex, int basey)
{
	UINT16 artifactpens[4];
	UINT16 *pens;
	UINT16 *mappedpens = NULL;
	UINT8 *vram;
	UINT8 *vramtop;
	UINT8 *scanline = NULL;
	UINT8 *db;
	int pixtop, pixbottom;
	int loopbackpos;
	int visualbytes;
	int y, r, i;
	void (*build_scanline)(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens);

	scanline = malloc(mode->width * scalex);
	if (!scanline)
		goto done; /* PANIC */

	vram = src->videoram + src->position;
	vramtop = vram + (src->size - (src->position % src->size));

	pens = mode->pens;
	db = src->db;
	visualbytes = mode->width * mode->depth / 8;

	/* Do we need loopback? */
	if ((mode->flags & RASTERBITS_FLAG_WRAPINROW) && ((mode->width * mode->depth / 8 + mode->offset) > mode->wrapbytesperrow)) {
		loopbackpos = mode->wrapbytesperrow - mode->offset;
	}
	else {
		loopbackpos = -1;
	}

	switch(mode->depth) {
	case 4:
		assert(!(mode->flags & RASTERBITS_FLAG_ARTIFACT));
		switch(scalex) {
		case 4:
			build_scanline = build_scanline4_4;
			break;
		case 2:
			build_scanline = build_scanline4_2;
			break;
		case 1:
			build_scanline = build_scanline4_1;
			break;
		default:
			build_scanline = build_scanline4;
			break;
		}
		break;

	case 2:
		assert(!(mode->flags & RASTERBITS_FLAG_ARTIFACT));
		build_scanline = build_scanline2;
		break;

	case 1:
		if (mode->flags & RASTERBITS_FLAG_ARTIFACT) {
			artifactpens[0] = pens ? pens[0] : 0;
			artifactpens[3] = pens ? pens[1] : 1;
			mode->u.artifact(artifactpens);
			build_scanline = build_scanline2a;
			pens = artifactpens;
		}
		else {
			build_scanline = build_scanline1;
		}
		break;

	default:
		assert(0);
		break;
	}

	/* We need to choose the proper pens mapping */
	if (pens) {
		int num_colors;

		num_colors = 1 << mode->depth;
		if (mode->flags & RASTERBITS_FLAG_ARTIFACT)
			num_colors *= 2;

		mappedpens = malloc(num_colors * sizeof(UINT16));
		if (!mappedpens)
			goto done; /* PANIC */

		for (i = 0; i < num_colors; i++)
			mappedpens[i] = Machine->pens[pens[i]];
		pens = mappedpens;
	}
	else {
		/* Use default */
		pens = Machine->pens;
	}

	for (y = 0; y < mode->height; y++) {
		pixtop = basey + y * scaley;
		pixbottom = pixtop + scaley - 1;
		pixtop = myMAX(pixtop, clip->ybegin);
		pixbottom = myMIN(pixbottom, clip->yend);
		if ((pixbottom >= pixtop) && isrowdirty(db, mode->bytesperrow)) {
			/* We have to draw this scanline */

			if (loopbackpos >= 0) {
				build_scanline(scanline,               vram + mode->offset, loopbackpos,               scalex, pens);
				build_scanline(scanline + loopbackpos, vram,                visualbytes - loopbackpos, scalex, pens);
			}
			else {
				build_scanline(scanline,               vram + mode->offset, visualbytes,               scalex, pens);
			}

			/* We have the scanline, now draw it */
			for (r = pixtop; r <= pixbottom; r++) {
				draw_scanline8(bitmap, basex, r, mode->width * scalex, scanline, NULL, -1);
			}

			mark_dirty(basex, pixtop, basex + mode->width * scalex - 1, pixbottom);
		}
		vram += mode->bytesperrow;
		if (db)
			db += mode->bytesperrow;
	}

done:
	if (scanline)
		free(scanline);
	if (mappedpens)
		free(mappedpens);
}
#endif /* NEW_GFX */

/* -------------------------------------------------------------------------
 * One of the few modern cals yet
 * ------------------------------------------------------------------------- */

#define COUNTDIRTYCHARS 0

static void raster_text(struct osd_bitmap *bitmap, struct rasterbits_source *src,
	struct rasterbits_videomode *mode, struct rasterbits_clip *clip,
	int scalex, int scaley, int basex, int basey)
{
	int x, y, i;
	int xi, yi;
	int chartop, charbottom, charleft;
	int bytesperchar;
	UINT8 *vram;
	UINT8 *vramtop;
	UINT8 *db;
	UINT8 b;
	UINT8 *thechar;
	UINT8 *remappedchar;
	int fg, bg, fgc, bgc, attr;
	int additionalrowbytes;
	int underlinepos;
	int loopbackpos, loopbackadj;
	int c[16];

#if COUNTDIRTYCHARS
	int dirtychars = 0;
#endif

	assert(src->videoram);
	assert((mode->depth == 8) || (mode->depth == 16));
	assert(scalex >= 8);

	if (mode->u.text.fontheight != scaley) {
		/* If the scale and the font height don't match, we may have to remap */
		remappedchar = malloc(scaley);
		if (!remappedchar)
			return;
		memset(remappedchar, 0, scaley);
	}
	else {
		remappedchar = NULL;
	}

	bytesperchar = mode->depth / 8;
	db = src->db;
	fg = 1;
	bg = 0;
	attr = 0;

	/* Build up our pen table; save some work later */
	for (i = 0; i < (sizeof(c) / sizeof(c[0])); i++) {
		if (mode->pens)
			c[i] = Machine->pens[mode->pens[i]];
		else
			c[i] = Machine->pens[i];
	}

	vram = src->videoram + src->position;
	vramtop = vram + (src->size - (src->position % src->size));
	additionalrowbytes = mode->bytesperrow - (mode->width * bytesperchar);

	charbottom = basey - 1;

	if (mode->offset) {
		vram += mode->offset;
		if (db)
			db += mode->offset;
	}

	/* Do we need loopback? */
	if ((mode->flags & RASTERBITS_FLAG_WRAPINROW) && ((mode->width * bytesperchar + mode->offset) > mode->wrapbytesperrow)) {
		loopbackpos = (mode->wrapbytesperrow - mode->offset + (bytesperchar - 1)) / bytesperchar;
		loopbackadj = -mode->wrapbytesperrow;
		additionalrowbytes += mode->wrapbytesperrow;
	}
	else {
		loopbackpos = -1;
		loopbackadj = 0;
	}

	for (y = 0; y < mode->height; y++) {
		chartop = basey + y * scaley;
		charbottom = chartop + scaley - 1;

		chartop = myMAX(chartop, clip->ybegin);
		charbottom = myMIN(charbottom, clip->yend);
		if (charbottom >= chartop) {
			for (x = 0; x < mode->width; x++) {
				if (!db || db[0] || ((bytesperchar == 2) && db[1])) {
					thechar = mode->u.text.mapper(vram, mode->u.text.mapper_param, &fg, &bg, &attr);
drawchar:
#if COUNTDIRTYCHARS
					dirtychars++;
#endif
					if ((mode->flags & RASTERBITS_FLAG_BLINKING) && (attr & RASTERBITS_CHARATTR_BLINKING))
						thechar = NULL;

					charleft = x * scalex + basex;

					if (thechar) {
						/* We're a bonafide character */
						fgc = c[fg];
						bgc = c[bg];

						if (remappedchar) {
							/* We have to remap */
							if (mode->flags & RASTERBITS_FLAG_TEXTMODULO) {
								for (yi = 0; yi < scaley; yi++) {
									remappedchar[yi] = thechar[(y * scaley + yi) % mode->u.text.fontheight];
								}
							}
							else {
								memcpy(remappedchar, thechar, myMIN(scaley, mode->u.text.fontheight));
							}
							thechar = remappedchar;
						}

						/* Calculate what line gets underlined, if appropriate */
						if (attr & RASTERBITS_CHARATTR_UNDERLINE)
							underlinepos = mode->u.text.underlinepos + basey + y * scaley;
						else
							underlinepos = -1;

						for (yi = chartop; yi <= charbottom; yi++) {
							if (yi == underlinepos)
								b = 0xff;
							else
								b = thechar[yi - (basey + y * scaley)];

							switch(b) {
							case 0x00:
								plot_box(bitmap, charleft, yi, scalex, 1, bgc);
								break;
							case 0xff:
								plot_box(bitmap, charleft, yi, scalex, 1, fgc);
								break;
							default:
								for (xi = 0; xi < scalex; xi+=(scalex/8)) {
									plot_box(bitmap, xi + charleft, yi, scalex / 8, 1, (b & 0x80) ? fgc : bgc);
									b <<= 1;
								}
								break;
							}
						}
					}
					else {
						/* We're a space */
						plot_box(bitmap, charleft, chartop, scalex, (charbottom - chartop + 1), c[bg]);
					}

					/* Make us 'undirty', if appropriate */
					if (db) {
						db[0] = '\0';
						if (bytesperchar == 2)
							db[1] = '\0';
					}
				}
				else if (mode->flags & RASTERBITS_FLAG_BLINKNOW) {
					thechar = mode->u.text.mapper(vram, mode->u.text.mapper_param, &fg, &bg, &attr);
					if (attr & RASTERBITS_CHARATTR_BLINKING)
						goto drawchar;
				}
				vram += bytesperchar;
				if (db)
					db += bytesperchar;

				/* Check loopback; this is used to support $FF9F in the CoCo 3 */
				if (x == loopbackpos) {
					vram += loopbackadj;
					if (db)
						db += loopbackadj;
				}
			}
		}
		else {
			vram += mode->width * bytesperchar;
			if (db)
				db += mode->width * bytesperchar;
		}

		vram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;
		if (vram >= vramtop)
			vram -= src->size;
	}

#if COUNTDIRTYCHARS
	logerror("%i / %i chars dirty\n", dirtychars, mode->height * mode->width);
#endif

	if (remappedchar)
		free(remappedchar);
}

/* -------------------------------------------------------------------------
 * Now here is the actual raster_bits() call!  Note the asserts that raise
 * errors when the holes are walked into
 * ------------------------------------------------------------------------- */

void raster_bits(struct osd_bitmap *bitmap, struct rasterbits_source *src, struct rasterbits_videomode *mode,
	struct rasterbits_frame *frame, struct rasterbits_clip *clip)
{
	int scalex, scaley;
	int basex, basey;
	int bitmapwidth;
	int bitmapheight;
	int drawingbody;
#if !NEW_GFX
	UINT16 artifactpalette[4];
	int loopbackpos, loopbackadj;
	int additionalrowbytes;
#endif
	struct rasterbits_clip myclip;

	assert(bitmap);
	assert(src);
	assert(mode);
	assert(frame);

	/* Get bitmap height and width; accounting for rotation/flipping */
	if (Machine->orientation & ORIENTATION_SWAP_XY) {
		bitmapwidth = bitmap->height;
		bitmapheight = bitmap->width;
	}
	else {
		bitmapwidth = bitmap->width;
		bitmapheight = bitmap->height;
	}

	/* Sanity check clipping */
	if (clip) {
		myclip = *clip;
		if (myclip.ybegin < 0)
			myclip.ybegin = 0;
		else if (myclip.ybegin >= bitmapheight)
			return;

		if (myclip.yend < 0)
			return;
		else if (myclip.yend >= bitmapheight)
			myclip.yend = bitmapheight - 1;

		if (myclip.ybegin > myclip.yend)
			return;
	}
	else {
		myclip.ybegin = 0;
		myclip.yend = bitmapheight - 1;
	}

	/* Compute base and scaling */
	scalex = (frame->width + mode->width - 1) / mode->width;
	scaley = (frame->height + mode->height - 1) / mode->height;
	basex = (bitmapwidth - frame->width) / 2;
	if (frame->total_scanlines == -1)
		basey = (bitmapheight - frame->height) / 2;
	else
		basey = (bitmapheight - frame->total_scanlines) / 2 + frame->top_scanline;

	assert(scalex > 0);
	assert(scaley > 0);
	assert(basex >= 0);
	assert(basey >= 0);

	drawingbody = (myclip.ybegin < (basey + frame->height)) && (myclip.yend >= basey);

	/* Render the border */
	if (frame->border_pen != -1) {
		/* ------------------------------
		 * The border is drawn in 4 zones
		 *
		 *
		 *   +---------------------+
		 *   |          1          |
		 *   +-----+---------+-----+
		 *   |     |         |     | 
		 *   |     |         |     | 
		 *   |  3  |         |  4  | 
		 *   |     |         |     | 
		 *   |     |         |     | 
		 *   +-----+---------+-----+
		 *   |          2          |
		 *   +---------------------+
		 *
		 * ------------------------------ */
		struct rectangle r;
		if (myclip.ybegin < basey) {
			/* Zone 1 */
			r.min_x = 0;
			r.min_y = myMAX(0, myclip.ybegin);
			r.max_x = bitmapwidth - 1;
			r.max_y = myMIN(basey - 1, myclip.yend);
			fillbitmap(bitmap, frame->border_pen, &r);
		}
		if (myclip.yend >= basey + frame->height) {
			/* Zone 2 */
			r.min_x = 0;
			r.min_y = myMAX(basey + frame->height, myclip.ybegin);
			r.max_x = bitmapwidth - 1;
			r.max_y = myMIN(bitmapheight - 1, myclip.yend);
			fillbitmap(bitmap, frame->border_pen, &r);
		}
		if (drawingbody) {
			/* Zone 3 */
			r.min_x = 0;
			r.min_y = myMAX(basey, myclip.ybegin);
			r.max_x = basex - 1;
			r.max_y = myMIN(basey + frame->height - 1, myclip.yend);
			fillbitmap(bitmap, frame->border_pen, &r);
			/* Zone 4 */
			r.min_x = basex + frame->width;
			r.max_x = bitmapwidth - 1;
			fillbitmap(bitmap, frame->border_pen, &r);
		}
	}

	if (drawingbody) {
		/* Clip to content */
		if (myclip.ybegin < basey)
			myclip.ybegin = basey;
		if (myclip.yend >= (basey + frame->height))
			myclip.yend = basey + frame->height - 1;

		/* Render the content */
#if NEW_GFX
		((mode->flags & RASTERBITS_FLAG_TEXT) ? raster_text : raster_graphics)(bitmap, src, mode, &myclip, scalex, scaley, basex, basey);
#else /* !NEW_GFX */
		switch(mode->flags & RASTERBITS_FLAG_TEXT) {
		case 0:
			/* Graphics */
			if (mode->flags & RASTERBITS_FLAG_ARTIFACT)
				assert(mode->depth == 1);

			additionalrowbytes = mode->bytesperrow - (mode->width * mode->depth / 8);

			/* Do we need loopback? */
			if ((mode->flags & RASTERBITS_FLAG_WRAPINROW) && ((mode->width * mode->depth / 8 + mode->offset) > mode->wrapbytesperrow)) {
				loopbackpos = mode->wrapbytesperrow - mode->offset;
				loopbackadj = -mode->wrapbytesperrow;
				additionalrowbytes += mode->wrapbytesperrow;
			}
			else {
				loopbackpos = -1;
				loopbackadj = 0;
			}

			switch(mode->depth) {
			case 1:
				if (mode->flags & RASTERBITS_FLAG_ARTIFACT) {
					assert(mode->bytesperrow == (mode->width / 8));
					assert(mode->u.artifact);
					assert(mode->pens);
					artifactpalette[0] = mode->pens[0];
					artifactpalette[3] = mode->pens[1];
					mode->u.artifact(artifactpalette);
					blitgraphics4artifact(bitmap, &myclip, src->videoram, src->position, mode->offset, loopbackpos, loopbackadj, src->size, src->db, artifactpalette,
						mode->width / 8, mode->height, basex, basey, scalex, scaley);
				}
				else {
					blitgraphics2(bitmap, &myclip, src->videoram, src->position, mode->offset, loopbackpos, loopbackadj, src->size, src->db, mode->pens,
						mode->width / 8, mode->height, basex, basey, scalex, scaley, additionalrowbytes);
				}
				break;

			case 2:
				blitgraphics4(bitmap, &myclip, src->videoram, src->position, mode->offset, loopbackpos, loopbackadj, src->size, src->db, mode->pens,
					mode->width / 4, mode->height, basex, basey, scalex, scaley, additionalrowbytes);
				break;

			case 4:
				assert(!mode->pens);
				blitgraphics16(bitmap, &myclip, src->videoram, src->position, mode->offset, loopbackpos, loopbackadj, src->size, src->db,
					mode->width / 2, mode->height, basex, basey, scalex, scaley, additionalrowbytes);
				break;

			default:
				assert(0);
				break;
			};
			break;

		case RASTERBITS_FLAG_TEXT:
			/* Text */
			raster_text(bitmap, src, mode, &myclip, scalex, scaley, basex, basey);
			break;
		};
#endif /* NEW_GFX */
	}
}

