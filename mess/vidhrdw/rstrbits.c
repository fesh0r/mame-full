#include <assert.h>
#include "includes/rstrbits.h"
#include "mame.h"
#include "driver.h"

#define myMIN(a, b) ((a) < (b) ? (a) : (b))
#define myMAX(a, b) ((a) > (b) ? (a) : (b))

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

static void build_scanline1a(UINT8 *scanline, UINT8 *vram, int length, int scale, UINT16 *pens)
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
#if 0
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
#endif

	/* The following is an 'extra artifacting' table, designed to take into
	 * account two pixels before and after (as opposed to one).  Data courtesy
	 * of John Kowalski
	 */
#if 1
	static int artifactcorrection[64][2] = {
		{ 0,  0}, { 0,  0}, { 0,  8}, { 0,  3},
		{ 5,  7}, { 5,  7}, { 1,  2}, { 1, 11},
		{10,  8}, {10, 14}, {10,  9}, {10,  9},
		{ 4,  4}, { 4, 15}, {12, 12}, {12, 15},

		{ 5, 13}, { 5, 13}, {13,  0}, {13,  3},
		{ 6,  6}, { 6,  6}, { 6, 15}, { 6, 11},
		{ 2,  1}, { 2,  1}, {15,  9}, {15,  9},
		{11, 11}, {11, 11}, {15, 15}, {15, 15},

		{14,  0}, {14,  0}, {14,  8}, {14,  3},
		{ 0,  7}, { 0,  7}, { 1,  2}, { 1, 11},
		{ 9,  8}, { 9, 14}, { 9,  9}, { 9,  9},
		{15,  4}, {15, 15}, {12, 12}, {12, 15},

		{ 3, 13}, { 3, 13}, { 3,  0}, { 3,  3},
		{ 6,  6}, { 6,  6}, { 6, 15}, { 6, 11},
		{12,  1}, {12,  1}, {12,  9}, {12,  9},
		{15, 11}, {15, 11}, {15, 15}, {15, 15}
	};
#endif

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
		case 0x015540:
		case 0x02aa80:
		case 0x03ffc0:
			b = artifactcorrection[(w >> 12) & 0x3f];
			memset(scanline, pens[b[0]], scale * 8);
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

static void mix_colors(UINT8 *dest, const double *val, const UINT8 *c0, const UINT8 *c1, int reverse)
{
	double v;
	double rval[3];
	int i;

	if (reverse) {
		rval[0] = val[2];
		rval[1] = val[1];
		rval[2] = val[0];
		val = rval;
	}

	for (i = 0; i < 3; i++) {
		v = (c0[i] * (1.0 - val[i])) + (c1[i] * val[i]);
		dest[i] = (UINT8) (v + 0.5);
	}
}

static void map_artifact_palette(UINT16 c0, UINT16 c1, const struct rasterbits_artifacting *artifact, UINT16 *artifactpens)
{
	int i, j;
	int totalcolors, palettebase;
	const double *table;
	UINT8 myrgb[3];
	UINT8 rgb0[3];
	UINT8 rgb1[3];

	totalcolors = artifact->numfactors + 2;
	table = artifact->colorfactors;
	palettebase = (artifact->flags & RASTERBITS_ARTIFACT_DYNAMICPALETTE) ? artifact->u.dynamicpalettebase : -1;

	artifactpens[0] = c0;
	artifactpens[totalcolors-1] = c1;

	if (artifact->getcolorrgb) {
		artifact->getcolorrgb(c0, &rgb0[0], &rgb0[1], &rgb0[2]);
		artifact->getcolorrgb(c1, &rgb1[0], &rgb1[1], &rgb1[2]);
	}
	else {
		assert(palettebase < 0);
		memcpy(rgb0, &artifact->u.staticpalette[c0 * 3], 3);
		memcpy(rgb1, &artifact->u.staticpalette[c1 * 3], 3);
	}

  	for (i = 1; i < (totalcolors-1); i++) {
		mix_colors(myrgb, &table[(i-1)*3], rgb0, rgb1, artifact->flags & RASTERBITS_ARTIFACT_REVERSE);

		if (palettebase < 0) {
			/* Search palette for matching color; start at the end of table because thats where we put stuff */
			for (j = Machine->drv->total_colors-1; j >= 0; j--) {
				if (!memcmp(myrgb, &artifact->u.staticpalette[j * 3], 3))
					break;
			}

			/* We had better have found a valid color */
			assert((j >= 0) && (j < Machine->drv->total_colors));

			artifactpens[i] = j;
		}
		else {
			/* We can use palette_change_color */
			palette_change_color(palettebase + i - 1, myrgb[0], myrgb[1], myrgb[2]);
			artifactpens[i] = palettebase + i - 1;
		}
	}

}

void setup_artifact_palette(UINT8 *destpalette, int destcolor, UINT16 c0, UINT16 c1,
	const double *colorfactors, int numfactors, int reverse)
{
	int i;
	const UINT8 *rgb0;
	const UINT8 *rgb1;

	rgb0 = &destpalette[c0 * 3];
	rgb1 = &destpalette[c1 * 3];
	destpalette += (destcolor * 3);

	for (i = 0; i < numfactors; i++) {
		mix_colors(&destpalette[i * 3], &colorfactors[i * 3], rgb0, rgb1, reverse);
	}
}

static void raster_graphics(struct osd_bitmap *bitmap, struct rasterbits_source *src,
	struct rasterbits_videomode *mode, struct rasterbits_clip *clip,
	int scalex, int scaley, int basex, int basey)
{
	UINT16 artifactpens[16];
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
	void (*build_scanline)(UINT8 *, UINT8 *, int , int , UINT16 *) = NULL;

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
			map_artifact_palette(pens ? pens[0] : 0, pens ? pens[1] : 1, &mode->u.artifact, artifactpens);
			build_scanline = build_scanline1a;
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

		if (mode->flags & RASTERBITS_FLAG_ARTIFACT)
			num_colors = mode->u.artifact.numfactors + 2;
		else
			num_colors = 1 << mode->depth;

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
		((mode->flags & RASTERBITS_FLAG_TEXT) ? raster_text : raster_graphics)(bitmap, src, mode, &myclip, scalex, scaley, basex, basey);
	}
}


