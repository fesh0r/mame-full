/* Video hardware for CoCo/Dragon family
 *
 * See src/mess/machine/dragon.c for references
 *
 * TODO:
 *		- Implement "burst phase invert" (it switches the artifact colors)
 *		- Figure out what Bit 3 of $FF98 is and implement it
 *		- Support mid-frame video register modification, and maybe get a
 *		  SockMaster demo working...
 *		- Learn more about the "mystery" CoCo 3 video modes
 *		- Support the VSC register
 *
 * My future plan to support mid-frame video register modification is to keep
 * a queue that shows the status of the video registers in relation between
 * different scanlines.  Thus if no modifications are made, there is only a
 * one entry queue, and perfomance is minimally affected.  The big innacuracy
 * is that it doesn't save the status of memory, but JK informs me that this
 * will have a minimal effect on software though.
 *
 * Also, JK informed me of what GIME registers would need to be saved in the
 * queue.  I quote his email to me:
 *
 *		Here are the things that get changed mid-frame:
 *			-palette registers ($ffb0-$ffbf)
 *			-horizontal resolution (switches between 256 and 320 pixels, $ff99)
 *			-horizontal scroll position (bits 0-6 $ff9f)
 *			-horizontal virtual screen (bit 7 $ff9f)
 *			-pixel height (bits 0-2 $ff98)
 *			-border color ($ff9a)
 *		On the positive side, you don't have to worry about registers
 *		$ff9d/ff9e being changed mid-frame.  Even if they are changed
 *		mid-frame, they have no effect on the displayed image.  The video
 *		address only gets latched at the top of the frame.
 *
 * Also, take note that the CoCo family have 262 scan lines.  In these drivers
 * we only display 240 of them.
 */
#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/m6847.h"
#include "vidhrdw/generic.h"
#include "includes/dragon.h"

static int coco3_hires;
static int coco3_gimevhreg[8];
static int coco3_borderred, coco3_bordergreen, coco3_borderblue;
static int sam_videomode;
static int coco3_somethingdirty;
static int coco3_blinkstatus;
static struct GfxElement *coco3font;

#define MAX_HIRES_VRAM	57600

#define LOG_PALETTE	0
#define LOG_GIME	0
#define LOG_VIDEO	0

/* --------------------------------------------------
 * CoCo 1/2 Stuff
 * -------------------------------------------------- */

static void dragon_vblank(void)
{
	/* clear vblank */
	pia_0_cb1_w (0, 0);
}

int dragon_vh_start(void)
{
	if (m6847_vh_start())
		return 1;

	m6847_set_vram(memory_region(REGION_CPU1), 0xffff);
	m6847_set_vblank_proc(dragon_vblank);
	m6847_set_artifact_dipswitch(12);
	sam_videomode = 0;
	return 0;
}

WRITE_HANDLER(coco_ram_w)
{
	UINT8 *mem = memory_region(REGION_CPU1) + offset;

	if (*mem != data) {
		m6847_touch_vram(offset);
		*mem = data;
	}
}

WRITE_HANDLER(dragon_sam_display_offset)
{
	UINT16 d_offset = m6847_get_video_offset();

	if (offset & 0x01)
		d_offset |= 0x01 << (offset/2 + 9);
	else
		d_offset &= ~(0x01 << (offset/2 + 9));

	m6847_set_video_offset(d_offset);
}

WRITE_HANDLER(dragon_sam_vdg_mode)
{
	/* SAM Video modes:
	 *
	 * 000	Text
	 * 001	G1C/G1R
	 * 010	G2C
	 * 011	G2R
	 * 100	G3C
	 * 101	G3R
	 * 110	G4C/G4R
	 * 111	Reserved/Invalid
	 */

	static int sammode2vmode[] = {
		M6847_MODE_TEXT,	/* 0 */
		M6847_MODE_G1C,		/* 1 */
		M6847_MODE_G2C,		/* 2 */
		M6847_MODE_G2R,		/* 3 */
		M6847_MODE_G3C,		/* 4 */
		M6847_MODE_G3R,		/* 5 */
		M6847_MODE_G4R,		/* 6 */
		M6847_MODE_G4R		/* 7 */
	};

	if (offset & 0x01)
		sam_videomode |= 0x01 << (offset/2);
	else
		sam_videomode &= ~(0x01 << (offset/2));

	m6847_set_vmode(sammode2vmode[sam_videomode]);
}

/* --------------------------------------------------
 * CoCo 3 Stuff
 * -------------------------------------------------- */

struct GfxElement *build_coco3_font(void)
{
	static unsigned short colortable[2];
	static struct GfxLayout fontlayout8x8 =
	{
		8,8,	/* 8*8 characters */
		128+1,	 /* 128 characters + 1 "underline" */
		1,	/* 1 bit per pixel */
		{ 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, },	/* straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
		8*8	/* every char takes 12 consecutive bytes */
	};

	unsigned char buf[129 * 8];
	struct GfxElement *font;
	UINT8 *mem;

	mem = memory_region(REGION_CPU1);

	/* The font info for the CoCo 3 hi res text modes happens to be
	 * same as the font info used for the HPRINT command, which is in
	 * the CoCo 3 ROM, so we get it from there
	 */

	/* Characters 0-31 are at $FA10 - $FB0F */
	memcpy(buf, &mem[0x80000 + 0xfa10 - 0x8000], 32 * 8);

	/* Characters 32-127 are at $F09D - $F39C */
	memcpy(&buf[8*32], &mem[0x80000 + 0xf09d - 0x8000], 96 * 8);

	/* The underline character is stored as "Character 128" */
	buf[8 * 128 + 0] = 0;
	buf[8 * 128 + 1] = 0;
	buf[8 * 128 + 2] = 0;
	buf[8 * 128 + 3] = 0;
	buf[8 * 128 + 4] = 0;
	buf[8 * 128 + 5] = 0;
	buf[8 * 128 + 6] = 0;
	buf[8 * 128 + 7] = 0xff;

	font = decodegfx(buf,&fontlayout8x8);
	if (font)
	{
		/* colortable will be set at run time */
		memset(colortable,0,sizeof(colortable));
		font->colortable = colortable;
		font->total_colors = 2;
	}
	return font;
}

static void drawgfx_wf(struct osd_bitmap *dest,const struct GfxElement *gfx,
	unsigned int code,int sx,int sy,
	const struct rectangle *clip,int transparency,int transparent_color, int wf)
{
	if (wf == 1) {
		drawgfx(dest, gfx, code, 0, 0, 0, sx, sy, clip, transparency, transparent_color);
	}
	else {
		/* drawgfxzoom doesnt directly take TRANSPARENCY_NONE */
		if (transparency == TRANSPARENCY_NONE) {
			transparency = TRANSPARENCY_PEN;
			transparent_color = -1;
		}
		drawgfxzoom(dest, gfx, code, 0, 0, 0, sx, sy, clip, transparency, transparent_color, (1<<16) * wf, 1<<16);
	}
}

int coco3_vh_start(void)
{
    int i;

	if (internal_m6847_vh_start(MAX_HIRES_VRAM)) {
		paletteram = NULL;
		return 1;
	}

	m6847_set_vram(memory_region(REGION_CPU1) + 0x70000, 0xffff);
	m6847_set_artifact_dipswitch(12);

	if ((coco3font = build_coco3_font())==NULL) {
		paletteram = NULL;
		return 1;
	}

	paletteram = malloc(16 * sizeof(int));
	if (!paletteram)
		return 1;

	memset(paletteram, 0, 16 * sizeof(int));
	for (i = 0; i < 16; i++)
		palette_change_color(i, 0, 0, 0);

	for (i = 0; i < (sizeof(coco3_gimevhreg) / sizeof(coco3_gimevhreg[0])); i++)
		coco3_gimevhreg[i] = 0;

	coco3_hires = coco3_somethingdirty = coco3_blinkstatus = 0;
	coco3_borderred = coco3_bordergreen = coco3_borderblue = -1;
	return 0;
}

void coco3_vh_stop(void)
{
	m6847_vh_stop();
	if (paletteram) {
		free(paletteram);
		paletteram = NULL;
	}
}

static void coco3_compute_color(int color, int *red, int *green, int *blue)
{
	if ((readinputport(12) & 0x08) == 0) {
		/* CMP colors
		 *
		 * These colors are of the format IICCCC, where II is the intensity and
		 * CCCC is the base color.  There is some weirdness because intensity
		 * is often different for each base color.  Thus a table is used here
		 * to map to RGB colors
		 *
		 * This table was based on a color table the "Super Extended Color Basic
		 * Unravelled" (http://www.giftmarket.org/unravelled/unravelled.shtml)
		 * that mapped RGB to CMP, then I compared it with the palette that the
		 * CoCo III uses on startup for CMP. Where they conflicted, the CoCo III
		 * startup palette took precedence. Where neither of them helped, I guessed
		 *
		 * The fault of this table is that some colors directly map to each other.
		 * I do not like this, so if anyone can come up with a better formula, I'm
		 * all ears.
		 */
		static const UINT8 cmp2rgb[] = {
			0,	/*  0 - Black */
			21,	/*  1 - Magenta tint green */
			2,	/*  2 - Low intensity green */
			20,	/*  3 - Red tint green */
			49,	/*  4 - Blue tint yellow */
			6,	/*  5 - Low intensity brown */
			35,	/*  6 - Cyan tint red */
			36,	/*  7 - Full intensity red*/
			33,	/*  8 - Blue tint red */
			27,	/*  9 - Full intensity cyan */
			14,	/* 10 - Brown tint blue */
			9,	/* 11 - Full intensity blue */
			1,	/* 12 - Low intensity blue */
			10,	/* 13 - Green tint blue */
			3,	/* 14 - Low intensity cyan */
			28,	/* 15 - Red tint cyan */
			7,	/* 16 - Low intensity white */
			17,	/* 17 - Blue tint green */
			18,	/* 18 - Full intensity green */
			22,	/* 19 - Brown tint green */
			48,	/* 20 - Medium intensity yellow */
			34,	/* 21 - Light Orange */
			34,	/* 22 - Light Orange */
			32,	/* 23 - Medium intensity red */
			37,	/* 24 - Magenta tint red */
			40,	/* 25 - Medium intensity magenta */
			42,	/* 26 - Green tint magenta */
			13,	/* 27 - Magenta tint blue */
			8,	/* 28 - Medium intensity blue */
			11,	/* 29 - Cyan tint blue */
			24,	/* 30 - Medium intensity cyan */
			45,	/* 31 - Full intensity magenta */
			56,	/* 32 - Medium intensity white */
			19,	/* 33 - Cyan tint green */
			16,	/* 34 - Medium intensity green */
			50,	/* 35 - Green tint yellow */
			54,	/* 36 - Full intensity yellow */
			52,	/* 37 - Red tint yellow */
			38,	/* 38 - Brown tint red */
			36,	/* 39 - Full intensity red */
			46,	/* 40 - Brown tint magenta */
			45,	/* 41 - Full intensity magenta */
			41,	/* 42 - Blue tint magenta */
			15,	/* 43 - Faded blue */
			9,	/* 44 - Full intensity blue */
			25,	/* 45 - Blue tint cyan  */
			27,	/* 46 - Full intensity cyan */
			30,	/* 47 - Brown tint cyan */
			63,	/* 48 - White */
			58,	/* 49 - Light green */
			58,	/* 50 - Light green */
			51,	/* 51 - Cyan tint yellow */
			55,	/* 52 - Faded yellow */
			53,	/* 53 - Magenta tint yellow */
			39,	/* 54 - Faded red */
			60,	/* 55 - Light red */
			47,	/* 56 - Faded magenta */
			61,	/* 57 - Light magenta */
			43,	/* 58 - Cyan tint magenta */
			57,	/* 59 - Light blue */
			29,	/* 60 - Magenta tint cyan */
			31,	/* 61 - Faded cyan */
			59,	/* 62 - Light cyan */
			63,	/* 63 - White */
		};
		color = cmp2rgb[color & 63];
	}

	/* RGB colors
	 *
	 * These colors are of the format RGBRGB, where the first 3 bits
	 * are more significant than the last three bits
	 */
	*red = (((color >> 4) & 2) | ((color >> 2) & 1)) * 0x55;
	*green = (((color >> 3) & 2) | ((color >> 1) & 1)) * 0x55;
	*blue = (((color >> 2) & 2) | ((color >> 0) & 1)) * 0x55;

	if (!input_port_11_r(0) && (coco3_gimevhreg[0] & 0x10)) {
		/* We are on a composite monitor/TV and the monochrome phase invert
		 * flag is on in the GIME.  This means we have to average out all
		 * colors
		 */
		*red = *green = *blue = (*red + *green + *blue) / 3;
	}
}

static void coco3_vh_palette_change_color(int color, int data)
{
	int red, green, blue;
	coco3_compute_color(data, &red, &green, &blue);
	palette_change_color(color, red, green, blue);
}

static void coco3_vh_palette_recompute(void)
{
	int i;
	for (i = 0; i < 16; i++)
		coco3_vh_palette_change_color(i, paletteram[i]);
}

static void coco3_vh_drawborder(struct osd_bitmap *bitmap, int screenx, int screeny)
{
	internal_m6847_drawborder(bitmap, screenx, screeny, 16);
}

static int coco3_vh_setborder(int red, int green, int blue)
{
	int full_refresh = 0;

	if ((coco3_borderred != red) || (coco3_bordergreen != green) || (coco3_borderblue != blue)) {
		coco3_borderred = red;
		coco3_bordergreen = green;
		coco3_borderblue = blue;
		palette_change_color(16, red, green, blue);

		full_refresh = palette_recalc() ? 1 : 0;
	}
	return full_refresh;
}

void coco3_vh_blink(void)
{
	coco3_blinkstatus = !coco3_blinkstatus;
	coco3_somethingdirty = 1;
}

WRITE_HANDLER(coco3_palette_w)
{
	paletteram[offset] = data;
	coco3_vh_palette_change_color(offset, data);

#if LOG_PALETTE
	logerror("CoCo3 Palette: %i <== $%02x\n", offset, data);
#endif
}

static void coco3_artifact(int *artifactcolors)
{
	int c1, c2, r1, r2, g1, g2, b1, b2;
	static int oldc1, oldc2;

	c1 = paletteram[artifactcolors[0]];
	c2 = paletteram[artifactcolors[3]];

	/* Have the colors actually changed? */
	if ((oldc1 != c1) || (oldc2 != c2)) {
		coco3_compute_color(c1, &r1, &g1, &b1);
		coco3_compute_color(c2, &r2, &g2, &b2);
		palette_change_color(17, r2, (g1+g2)/2, b1);
		palette_change_color(18, r1, (g1+g2)/2, b2);
	}
}

static void coco3_artifact_red(int *artifactcolors)
{
	coco3_artifact(artifactcolors);
	artifactcolors[2] = 18;
	artifactcolors[1] = 17;
}

static void coco3_artifact_blue(int *artifactcolors)
{
	coco3_artifact(artifactcolors);
	artifactcolors[1] = 18;
	artifactcolors[2] = 17;
}

static int coco3_calculate_rows(void)
{
	int rows=0;
	switch((coco3_gimevhreg[1] & 0x60) >>5) {
	case 0:
		rows = 192;
		break;
	case 1:
		rows = 200;
		break;
	case 2:
		rows = 210;
		break;
	case 3:
		rows = 225;
		break;
	}
	return rows;
}

static int coco3_hires_linesperrow(void)
{
	/* TODO - This variable is only used in graphics mode.  This should probably
	 * be used in text mode as well.
	 *
	 * There are several conflicting sources of information on what this should
	 * return.  For example, Kevin Darling's reference implies this should be:
	 *
	 *		static int hires_linesperrow[] = {
	 *			1, 2, 3, 8, 9, 10, 12, 12
	 *		};
	 *
	 * For this I am going to go by Sock Master's GIME reference.  He has
	 * programmed far closer to the GIME chip then probably anyone.  In
	 * addition, running the Gate Crasher demo seems to validate his findings.
	 */

	static int hires_linesperrow[] = {
		1, 1, 2, 8, 9, 10, 11
	};

	int indexx;
	indexx = coco3_gimevhreg[0] & 7;

	return (indexx == 7) ? coco3_calculate_rows() : hires_linesperrow[indexx];
}

static int coco3_hires_vidbase(void)
{
	return (((coco3_gimevhreg[5] * 0x800) + (coco3_gimevhreg[6] * 8)) | ((coco3_gimevhreg[7] & 0x7f) * 2));

}

#define coco3_lores_vidbase	coco3_hires_vidbase

#if LOG_VIDEO
static void log_video(void)
{
	int rows, cols, visualbytesperrow, bytesperrow, vidbase;

	switch(coco3_gimevhreg[0] & 0x80) {
	case 0x00:	/* Text */
		logerror("CoCo3 HiRes Video: Text Mode\n");
		break;
	case 0x80:	/* Graphics */
		logerror("CoCo3 HiRes Video: Graphics Mode\n");

		visualbytesperrow = 16 << ((coco3_gimevhreg[1] & 0x18) >> 3);
		if (coco3_gimevhreg[1] & 0x04)
			visualbytesperrow |= (visualbytesperrow / 4);

		switch(coco3_gimevhreg[1] & 3) {
		case 0:
			cols = visualbytesperrow * 8;
			logerror("CoCo3 HiRes Video: 2 colors");
			break;
		case 1:
			cols = visualbytesperrow * 4;
			logerror("CoCo3 HiRes Video: 4 colors");
			break;
		case 2:
			cols = visualbytesperrow * 2;
			logerror("CoCo3 HiRes Video: 16 colors");
			break;
		default:
			logerror("CoCo3 HiRes Video: ??? colors\n");
			return;
		}
		rows = coco3_calculate_rows() / coco3_hires_linesperrow();
		logerror(" @ %dx%d\n", cols, rows);

		vidbase = coco3_hires_vidbase();
		bytesperrow = (coco3_gimevhreg[7] & 0x80) ? 256 : visualbytesperrow;
		logerror("CoCo3 HiRes Video: Occupies memory %05x-%05x\n", vidbase, (vidbase + (rows * bytesperrow) - 1) & 0x7ffff);
		break;
	}
}
#endif

/*
 * All models of the CoCo has 262 scan lines.  However, we pretend that it has
 * 240 so that the emulation fits on a 640x480 screen
 */
void coco3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	static int coco3_metapalette[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
	static artifactproc artifacts[] = {
		NULL,
		coco3_artifact_red,
		coco3_artifact_blue
	};
	static int old_cmprgb;
	int cmprgb, i;
	int use_mark_dirty;

	/* Did the user change between CMP and RGB? */
	cmprgb = readinputport(12) & 0x08;
	if (cmprgb != old_cmprgb) {
		old_cmprgb = cmprgb;

		/* Reset all colors and border */
		coco3_borderred = -1;	/* force border to redraw */
		for (i = 0; i < 16; i++)
			coco3_vh_palette_change_color(i, paletteram[i]);
	}

	/* clear vblank */
	coco3_vblank();

	if (coco3_hires) {
		static int last_blink;
		int blink_switch=0;
		int vidbase, bytesperrow, linesperrow, rows = 0, x, y, basex = 0, basey, wf = 0;
		int use_attr, charsperrow = 0, underlined = 0;
		int visualbytesperrow;
		int borderred, bordergreen, borderblue;
		UINT8 *vram, *db;
		UINT8 b;

		vidbase = coco3_hires_vidbase();
		if (coco3_gimevhreg[7] & 0x80) {
			bytesperrow = 256;
		}
		else {
			bytesperrow = 0;

		}

		rows = coco3_calculate_rows();
		linesperrow = coco3_hires_linesperrow();

		basey = (bitmap->height - rows) / 2;

		/* check border */
		coco3_compute_color(coco3_gimevhreg[2] & 0x3f, &borderred, &bordergreen, &borderblue);
		full_refresh += coco3_vh_setborder(borderred, bordergreen, borderblue);

		/* check palette recalc */
		if (palette_recalc() || full_refresh) {
			full_refresh = 1;
			coco3_somethingdirty = 1;

#if LOG_VIDEO
			log_video();
#endif
		}

		/* Draw border if appropriate */
		if (full_refresh)
			coco3_vh_drawborder(bitmap, coco3_gimevhreg[1] & 0x04 ? 640 : 512, rows * linesperrow);

		use_mark_dirty = 1;

		switch(coco3_gimevhreg[0] & 0x80) {
		case 0x00:	/* Text */
			if (full_refresh)
				memset(dirtybuffer, 1, MAX_HIRES_VRAM);

			use_attr = coco3_gimevhreg[1] & 1;
			if (use_attr) {
				/* Resolve blink */
				blink_switch = coco3_blinkstatus == last_blink;
				last_blink = coco3_blinkstatus;
			}
			else {
				/* Set colortable to be default */
				coco3font->colortable[0] = Machine->pens[0];
				coco3font->colortable[1] = Machine->pens[1];
				underlined = 0;
			}

			if (coco3_somethingdirty) {
				rows /= 8;
				switch(coco3_gimevhreg[1] & 0x14) {
				case 0x14:
					charsperrow = 80;
					basex = (bitmap->width - 640) / 2;
					wf = 1;
					break;
				case 0x10:
					charsperrow = 64;
					basex = (bitmap->width - 512) / 2;
					wf = 1;
					break;
				case 0x04:
					charsperrow = 40;
					basex = (bitmap->width - 640) / 2;
					wf = 2;
					break;
				case 0x00:
					charsperrow = 32;
					basex = (bitmap->width - 512) / 2;
					wf = 2;
					break;
				}
				if (!bytesperrow)
					bytesperrow = charsperrow * (use_attr + 1);

				for (y = 0; y < rows; y++) {
					i = y * bytesperrow;
					vram = &RAM[(vidbase + i) & 0x7FFFF];
					db = dirtybuffer + i;

					for (x = 0; x < charsperrow; x++) {
						if (db[0] || (use_attr && (db[1] || (blink_switch && (vram[1] & 0x80))))) {
							b = *vram & 0x7f;

							if (use_attr) {
								coco3font->colortable[0] = Machine->pens[vram[1] & 7];
								coco3font->colortable[1] = Machine->pens[8 + ((vram[1] >> 3) & 7)];

								/* Are we blinking? */
								if (coco3_blinkstatus && (vram[1] & 0x80)) {
									b = 0x20;
									underlined = 0;
								}
								else {
									/* Are we underlined? */
									underlined = vram[1] & 0x40;
								}
							}

							drawgfx_wf(bitmap, coco3font, b, x*8*wf+basex, y*8+basey, 0, TRANSPARENCY_NONE, 0, wf);
							if (underlined)
								drawgfx_wf(bitmap, coco3font, 128, x*8*wf+basex, y*8+basey, 0, TRANSPARENCY_PEN, 0, wf);
							if (use_mark_dirty)
								osd_mark_dirty(x*8*wf+basex, y*8+basey, (x+1)*8*wf-1+basex, y*8+7+basey);

							db[0] = 0;
							if (use_attr)
								db[1] = 0;
						}
						vram++;
						db++;
						vram += use_attr;
						db += use_attr;

						if (vram > &RAM[0x80000])
							vram -= 0x80000;
					}
				}
				coco3_somethingdirty = 0;
			}
			break;

		case 0x80:	/* Graphics */
			visualbytesperrow = 16 << ((coco3_gimevhreg[1] & 0x18) >> 3);
			switch(coco3_gimevhreg[1] & 3) {
			case 0:
				/* Two colors */
				wf = 64 / visualbytesperrow;
				break;
			case 1:
				/* Four colors */
				wf = 128 / visualbytesperrow;
				break;
			case 2:
			case 3:
				/* Sixteen colors */
				wf = 256 / visualbytesperrow;

				/* BUG - 'case 3' should actually show just a blank screen */
				break;
			}

			if (coco3_gimevhreg[1] & 0x04) {
				visualbytesperrow |= (visualbytesperrow / 4);
				basex = (bitmap->width - 640) / 2;
			}
			else {
				basex = (bitmap->width - 512) / 2;
			}

			if (!bytesperrow)
				bytesperrow = visualbytesperrow;

			/*
			 * TODO - We should support the case where there is a rounding
			 * error when rows is divided by linesperrow
			 */
			switch(coco3_gimevhreg[1] & 3) {
			case 0:
				/* Two colors */
				blitgraphics2(bitmap, RAM, vidbase, 0x80000,
					full_refresh ? NULL : dirtybuffer, NULL,
					visualbytesperrow, rows / linesperrow, basex, basey,
					wf, linesperrow, bytesperrow - visualbytesperrow);
				break;
			case 1:
				/* Four colors */
				blitgraphics4(bitmap, RAM, vidbase, 0x80000,
					full_refresh ? NULL : dirtybuffer, NULL,
					visualbytesperrow, rows / linesperrow, basex, basey,
					wf, linesperrow, bytesperrow - visualbytesperrow);
				break;
			case 2:
				/* Sixteen colors */
				blitgraphics16(bitmap, RAM, vidbase, 0x80000,
					full_refresh ? NULL : dirtybuffer,
					visualbytesperrow, rows / linesperrow, basex, basey, wf, linesperrow,
					bytesperrow - visualbytesperrow);
				break;
			case 3:
				/* Blank screen */
				/* TODO - Draw a blank screen! */
				break;
			}

			if (full_refresh)
				memset(dirtybuffer, 0, ((rows + linesperrow - 1) / linesperrow) * bytesperrow);
			break;
		}
	}
	else {
		int borderred, bordergreen, borderblue;

		m6847_get_bordercolor_rgb(&borderred, &bordergreen, &borderblue);
		full_refresh += coco3_vh_setborder(borderred, bordergreen, borderblue);
		if (palette_recalc())
			full_refresh = 1;
		if (full_refresh)
			coco3_vh_drawborder(bitmap, 512, 192);

		internal_m6847_vh_screenrefresh(bitmap, full_refresh, coco3_metapalette,
			&RAM[coco3_lores_vidbase()], m6847_get_video_offset(), 0x10000,
			TRUE, (bitmap->width - 512) / 2, (bitmap->height - 192) / 2, 2,
			artifacts[readinputport(12) & 3]);
	}
}

static void coco3_ram_w(int offset, int data, int block)
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	int vidbase;
	unsigned int vidbasediff;

	offset = coco3_mmu_translate(block, offset);

	if (RAM[offset] != data) {
		if (coco3_hires) {
			vidbase = coco3_hires_vidbase();
			vidbasediff = (unsigned int) (offset - vidbase) & 0x7ffff;
			if (vidbasediff < MAX_HIRES_VRAM) {
				dirtybuffer[vidbasediff] = 1;
				coco3_somethingdirty = 1;
			}
		}
		else {
			/* Apparently, lores video
			 */

			vidbase = coco3_lores_vidbase();

			if (offset >= vidbase)
				m6847_touch_vram(offset - vidbase);
		}

		RAM[offset] = data;
	}
}

void coco3_ram_b1_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 0);
}
void coco3_ram_b2_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 1);
}
void coco3_ram_b3_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 2);
}
void coco3_ram_b4_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 3);
}
void coco3_ram_b5_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 4);
}
void coco3_ram_b6_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 5);
}
void coco3_ram_b7_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 6);
}
void coco3_ram_b8_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 7);
}
void coco3_ram_b9_w (offs_t offset, data8_t data)
{
	coco3_ram_w(offset, data, 8);
}

READ_HANDLER(coco3_gimevh_r)
{
	return coco3_gimevhreg[offset];
}

WRITE_HANDLER(coco3_gimevh_w)
{
	int xorval;

#if LOG_GIME
	logerror("CoCo3 GIME: $%04x <== $%02x pc=$%04x\n", offset + 0xff98, data, cpu_get_pc());
#endif
	/* Features marked with '!' are not yet implemented */

	xorval = coco3_gimevhreg[offset] ^ data;
	coco3_gimevhreg[offset] = data;

	switch(offset) {
	case 0:
		/*	$FF98 Video Mode Register
		 *		  Bit 7 BP 0 = Text modes, 1 = Graphics modes
		 *		  Bit 6 Unused
		 *		! Bit 5 BPI Burst Phase Invert (Color Set)
		 *		  Bit 4 MOCH 1 = Monochrome on Composite
		 *		! Bit 3 H50 1 = 50 Hz power, 0 = 60 Hz power
		 *		  Bits 0-2 LPR Lines per row
		 */
		if (xorval & 0xB7) {
			coco3_borderred = -1;	/* force border to redraw */
			schedule_full_refresh();
#if LOG_GIME
			logerror("CoCo3 GIME: $ff98 forcing refresh\n");
#endif
		}
		if (xorval & 0x10) {
			coco3_vh_palette_recompute();
		}
		break;

	case 1:
		/*	$FF99 Video Resolution Register
		 *		  Bit 7 Undefined
		 *		  Bits 5-6 LPF Lines per Field (Number of Rows)
		 *		  Bits 2-4 HRES Horizontal Resolution
		 *		  Bits 0-1 CRES Color Resolution
		 */
		if (xorval) {
			coco3_borderred = -1;	/* force border to redraw */
			schedule_full_refresh();
		}
		break;

	case 2:
		/*	$FF9A Border Register
		 *		  Bits 6,7 Unused
		 *		  Bits 0-5 BRDR Border color
		 */
		break;

	case 4:
		/*	$FF9C Vertical Scroll Register
		 *		  Bits 4-7 Reserved
		 *		! Bits 0-3 VSC Vertical Scroll bits
		 */
		if (xorval)
			schedule_full_refresh();
		break;

	case 5:
	case 6:
	case 7:
		/*	$FF9D,$FF9E Vertical Offset Registers
		 *
		 *	$FF9F Horizontal Offset Register
		 *		  Bit 7 HVEN Horizontal Virtual Enable
		 *		  Bits 0-6 X0-X6 Horizontal Offset Address
		 *
		 *	According to JK, if an odd value is placed in $FF9E on the 1986
		 *	GIME, the GIME crashes
		 */
		if (xorval) {
			schedule_full_refresh();
#if LOG_GIME
			logerror("CoCo3 GIME: HiRes Video at $%05x\n", coco3_hires_vidbase());
#endif
		}
		break;
	}
}

void coco3_vh_sethires(int hires)
{
	if (hires != coco3_hires) {
		coco3_hires = hires;
#if LOG_GIME
		logerror("CoCo3 GIME: %s hires graphics/text\n", hires ? "Enabling" : "Disabling");
#endif
		schedule_full_refresh();
	}
}

