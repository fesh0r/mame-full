#include <assert.h>
#include "includes/rstrbits.h"
#include "mame.h"
#include "driver.h"

/* -------------------------------------------------------------------------
 * These functions were taken out of the CoCo/M6847 support, currently what
 * raster_bits() does is looks at the args, and calls one of these
 * functions.  The problem is that this leaves many holes in raster_bits()'s
 * functionality, but as time goes on, these holes will probably be filled
 * ------------------------------------------------------------------------- */

/*
 * Note that 'sizex' is in bytes, and 'sizey' is in pixels
 */
static void blitgraphics2(struct osd_bitmap *bitmap, UINT8 *vrambase, int vrampos,
	int vramsize, UINT8 *db, const int *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int fg, bg;
	int p, b;
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

	for (y = 0; y < sizey; y++) {
		for (x = 0; x < sizex; x++) {
			if (!db || *db) {
				for (b = 0; b < 8; b++) {
					p = ((*vidram) & (1<<(7-b))) ? fg : bg;
					plot_box(bitmap, (x * 8 + b) * scalex + basex, y * scaley + basey, scalex, scaley, p);
				}
				if (db)
					*(db++) = 0;
			}
			else {
				db++;
			}
			vidram++;
		}
		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics4(struct osd_bitmap *bitmap, UINT8 *vrambase, int vrampos,
	int vramsize, UINT8 *db, const int *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int c[4];
	int p, b;
	int crunlen, crunc =0, thisx =0, thisy;
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

	for (y = 0; y < sizey; y++) {
		crunlen = 0;
		thisy = y * scaley + basey;
		for (x = 0; x < sizex; x++) {
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
						plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, c[crunc]);
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
					plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, c[crunc]);
					crunlen = 0;
				}
				db++;
			}
			vidram++;
		}

		if (crunlen) {
			plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, c[crunc]);
			crunlen = 0;
		}

		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics4artifact(struct osd_bitmap *bitmap, UINT8 *vrambase,
	int vrampos, int vramsize, UINT8 *db, const int *metapalette, int sizex,
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
	 *		blurcorrection[pix(x-1,y)*16+pix(x,y)*4+pix(x+1,y)]
	 *
	 * This gives you a pair of pixels to display pix(x,y) as
	 *
	 * Special thanks to Derek Snider for coming up with the
	 * basis for the values in this table
	 */
	static int blurcorrection[64][2] = {
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
	int thisx, thisy;
	UINT8 *vidram;
	UINT32 w;
	int *blur;

	c[0] = Machine->pens[metapalette[0]];
	c[1] = Machine->pens[metapalette[1]];
	c[2] = Machine->pens[metapalette[2]];
	c[3] = Machine->pens[metapalette[3]];

	vidram = vrambase + vrampos;
	thisy = basey;

	for (y = 0; y < sizey; y++) {
		x = 0;
		while(x < sizex) {
			if (db[0] || ((x < (sizex-1)) && db[1])) {
				/* We are in a run; calculate the size of the run */
				drunlen = 1;
				while((x + drunlen < (sizex-1)) && db[drunlen])
					drunlen++;
				if (x + drunlen < (sizex-1))
					drunlen++;

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
				memset(db, 0, drunlen);
				db += drunlen;
				x += drunlen;

				while(drunlen--) {
					/* Bring new data into 'w' */
					if (drunlen || (x < sizex))
						w |= (UINT32) vidram[1];
					else
						w |= (w >> 2) & 0x0000e0;

					for (b = 0; b < 4; b++) {
						blur = blurcorrection[(w & 0x3f000) >> 12];
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
							plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, crunc);
							thisx += scalex * crunlen;
							crunlen = 1;
							crunc = c1;
						}
						if (crunc == c2) {
							crunlen++;
						}
						else {
							plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, crunc);
							thisx += scalex * crunlen;
							crunlen = 1;
							crunc = c2;
						}
					}
					vidram++;
				}
				plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, crunc);
			}
			else {
				db++;
				vidram++;
				x++;
			}
		}

		thisy += scaley;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

static void blitgraphics16(struct osd_bitmap *bitmap, UINT8 *vrambase,
	int vrampos, int vramsize, UINT8 *db, int sizex, int sizey, int basex,
	int basey, int scalex, int scaley, int additionalrowbytes)
{
	int x, y;
	int thisx =0, thisy;
	int crunlen, crunc =0;
	UINT8 *vidram;
	UINT8 b;

	vidram = vrambase + vrampos;
	thisy = basey;

	for (y = 0; y < sizey; y++) {
		crunlen = 0;
		for (x = 0; x < sizex; x++) {
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
					plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, Machine->pens[crunc]);
					thisx += scalex * crunlen;
					crunlen = 1;
					crunc = b >> 4;
				}

				if (crunc == (b & 15)) {
					crunlen++;
				}
				else {
					plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, Machine->pens[crunc]);
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
					plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, Machine->pens[crunc]);
					crunlen = 0;
				}
			}
			vidram++;
		}

		if (crunlen > 0) {
			plot_box(bitmap, thisx, thisy, scalex * crunlen, scaley, Machine->pens[crunc]);
			crunlen = 0;
		}

		thisy += scaley;

		vidram += additionalrowbytes;
		if (db)
			db += additionalrowbytes;

		/* Check to see if the video RAM has wrapped around */
		if (vidram > vrambase + vramsize)
			vidram -= vramsize;
	}
}

/* -------------------------------------------------------------------------
 * One of the few modern cals yet
 * ------------------------------------------------------------------------- */

#define COUNTDIRTYCHARS 0

static void raster_text(struct osd_bitmap *bitmap, struct rasterbits_source *src,
	struct rasterbits_videomode *mode, int scalex, int scaley, int basex, int basey)
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
	int fg, bg, fgc, bgc, attr;
	int additionalrowbytes;
	int c[16];

#if COUNTDIRTYCHARS
	int dirtychars = 0;
#endif

	assert((mode->depth == 8) || (mode->depth == 16));

	bytesperchar = mode->depth / 8;
	db = src->db;
	fg = 0;
	bg = 0;
	attr = 0;

	/* Build up our pen table; save some work later */
	for (i = 0; i < (sizeof(c) / sizeof(c[0])); i++) {
		if (mode->metapalette)
			c[i] = Machine->pens[mode->metapalette[i]];
		else
			c[i] = Machine->pens[i];
	}

	vram = src->videoram + src->position;
	vramtop = vram + (src->size - (src->position % src->size));
	additionalrowbytes = mode->bytesperrow - (mode->width * bytesperchar);

	charbottom = basey - 1;

	for (y = 0; y < mode->height; y++) {
		chartop = charbottom + 1;
		charbottom += scaley;

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
					for (yi = chartop; yi <= charbottom; yi++) {
						if ((attr & RASTERBITS_CHARATTR_UNDERLINE) && (yi == charbottom))
							b = 0xff;
						else
							b = thechar[(yi-basey) % mode->u.text.modulo];

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

	assert(!clip);
	assert(bitmap);
	assert(src);
	assert(mode);
	assert(frame);

	if (Machine->orientation & ORIENTATION_SWAP_XY) {
		bitmapwidth = bitmap->height;
		bitmapheight = bitmap->width;
	}
	else {
		bitmapwidth = bitmap->width;
		bitmapheight = bitmap->height;
	}


	scalex = frame->width / mode->width;
	scaley = frame->height / mode->height;
	basex = (bitmapwidth - frame->width) / 2;
	basey = (bitmapheight - frame->height) / 2;

	assert(scalex > 0);
	assert(scaley > 0);
	assert(basex >= 0);
	assert(basey >= 0);

	/* Render the border */
	if (frame->border_pen != -1) {
		struct rectangle r;
		r.min_x = 0;
		r.min_y = 0;
		r.max_x = bitmapwidth - 1;
		r.max_y = basey - 1;
		fillbitmap(bitmap, frame->border_pen, &r);
		r.min_x = 0;
		r.min_y = basey + frame->height;
		r.max_x = bitmapwidth - 1;
		r.max_y = bitmapheight - 1;
		fillbitmap(bitmap, frame->border_pen, &r);
		r.min_x = 0;
		r.min_y = basey;
		r.max_x = basex - 1;
		r.max_y = basey + frame->height - 1;
		fillbitmap(bitmap, frame->border_pen, &r);
		r.min_x = basex + frame->width;
		r.min_y = basey;
		r.max_x = bitmapwidth - 1;
		r.max_y = basey + frame->height - 1;
		fillbitmap(bitmap, frame->border_pen, &r);
	}

	/* Render the content */
	switch(mode->flags & RASTERBITS_FLAG_TEXT) {
	case 0:
		/* Graphics */
		if (mode->flags & RASTERBITS_FLAG_ARTIFACT)
			assert(mode->depth == 1);

		switch(mode->depth) {
		case 1:
			if (mode->flags & RASTERBITS_FLAG_ARTIFACT) {
				assert(mode->bytesperrow == (mode->width / 8));
				assert(src->db);
				blitgraphics4artifact(bitmap, src->videoram, src->position, src->size, src->db, mode->metapalette,
					mode->width / 8, mode->height, basex, basey, scalex, scaley);
			}
			else {
				blitgraphics2(bitmap, src->videoram, src->position, src->size, src->db, mode->metapalette,
					mode->width / 8, mode->height, basex, basey, scalex, scaley, mode->bytesperrow - (mode->width / 8));
			}
			break;

		case 2:
			blitgraphics4(bitmap, src->videoram, src->position, src->size, src->db, mode->metapalette,
				mode->width / 4, mode->height, basex, basey, scalex, scaley, mode->bytesperrow - (mode->width / 4));
			break;

		case 4:
			assert(!mode->metapalette);
			blitgraphics16(bitmap, src->videoram, src->position, src->size, src->db,
				mode->width / 2, mode->height, basex, basey, scalex, scaley, mode->bytesperrow - (mode->width / 2));
			break;

		default:
			assert(0);
			break;
		};
		break;

	case RASTERBITS_FLAG_TEXT:
		/* Text */
		raster_text(bitmap, src, mode, scalex, scaley, basex, basey);
		break;
	};
}

