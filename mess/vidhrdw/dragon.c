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
#include "includes/rstrbits.h"
#include "includes/rstrtrck.h"

static int coco3_hires;
static int coco3_gimevhreg[8];
static int coco3_borderred, coco3_bordergreen, coco3_borderblue;
static int sam_videomode;
static int coco3_blinkstatus;

#define MAX_HIRES_VRAM	57600

#define LOG_PALETTE	0
#define LOG_GIME	0
#define LOG_VIDEO	0

/* -------------------------------------------------- */

static void coco3_getvideoinfo(int full_refresh, struct rasterbits_source *rs,
	struct rasterbits_videomode *rvm, struct rasterbits_frame *rf);

static const struct rastertrack_info coco3_ri = {
	262,
	240,
	coco3_getvideoinfo
};

/* --------------------------------------------------
 * CoCo 1/2 Stuff
 * -------------------------------------------------- */

void dragon_charproc(UINT8 c)
{
	int inv;

	inv = (c & 0x40) ? 1 : 0;

	m6847_as_w(0,	(c & 0x80) || (m6847_intext_r(0)));
	m6847_inv_w(0,	inv);
}

static void coco2b_charproc(UINT8 c)
{
	int inv;
	int gm0;

	gm0 = m6847_gm0_r(0);

	if (gm0 && (c < 0x20)) {
		/* A lowercase char */
		inv = 1;
	}
	else {
		/* something else */
		inv = (c & 0x40) ? 1 : 0;
	}

	m6847_as_w(0, (c & 0x80) || (!gm0 && m6847_intext_r(0)));
	m6847_inv_w(0, inv);
}

static WRITE_HANDLER( coco_m6847_hs_w )
{
	pia_0_ca1_w(0, data);
}

static WRITE_HANDLER( coco_m6847_fs_w )
{
	pia_0_cb1_w(0, data);
}

static int internal_dragon_vh_start(int m6847_version, void (*charproc)(UINT8))
{
	struct m6847_init_params p;

	memset(&p, '\0', sizeof(p));
	p.version = m6847_version;
	p.artifactdipswitch = 12;
	p.ram = memory_region(REGION_CPU1);
	p.ramsize = 0x10000;
	p.charproc = charproc;
	p.hs_func = coco_m6847_hs_w;
	p.fs_func = coco_m6847_fs_w;
	p.callback_delay = (TIME_IN_HZ(894886) * 2);

	if (m6847_vh_start(&p))
		return 1;

	sam_videomode = 0;
	return 0;
}

int dragon_vh_start(void)
{
	return internal_dragon_vh_start(M6847_VERSION_ORIGINAL, dragon_charproc);
}

int coco2b_vh_start(void)
{
	return internal_dragon_vh_start(M6847_VERSION_M6847T1, coco2b_charproc);
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

	static int sammode2rowheight[] = {
		12,	/* 0 */
		3,	/* 1 */
		3,	/* 2 */
		2,	/* 3 */
		2,	/* 4 */
		1,	/* 5 */
		1,	/* 6 */
		1	/* 7 */
	};

	if (offset & 0x01)
		sam_videomode |= 0x01 << (offset/2);
	else
		sam_videomode &= ~(0x01 << (offset/2));

	m6847_set_row_height(sammode2rowheight[sam_videomode]);
}

/* --------------------------------------------------
 * CoCo 3 Stuff
 * -------------------------------------------------- */

int coco3_vh_start(void)
{
    int i;
	struct m6847_init_params p;

	memset(&p, '\0', sizeof(p));
	p.version = M6847_VERSION_M6847T1;
	p.artifactdipswitch = 12;
	p.ram = memory_region(REGION_CPU1);
	p.ramsize = 0x10000;
	p.charproc = coco2b_charproc;

	if (internal_m6847_vh_start(&p, MAX_HIRES_VRAM)) {
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

	coco3_hires = coco3_blinkstatus = 0;
	coco3_borderred = coco3_bordergreen = coco3_borderblue = -1;

	rastertrack_init(&coco3_ri);
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
	int r, g, b;

	if ((readinputport(12) & 0x08) == 0) {
		/* CMP colors
		 *
		 * These colors are of the format IICCCC, where II is the intensity and
		 * CCCC is the base color.  There is some weirdness because intensity
		 * is often different for each base color.
		 *
		 * The following CoCo BASIC program was used to approximate composite
		 * colors.  (Program by SockMaster)
		 *
		 *	10 POKE65497,0:DIMR(63),G(63),B(63):WIDTH80:PALETTE0,0:PALETTE8,54:CLS1
		 *	20 SAT=92:CON=53:BRI=-16:L(0)=0:L(1)=47:L(2)=120:L(3)=255
		 *	30 W=.4195456981879*1.01:A=W*9.2:S=A+W*5:D=S+W*5:P=0:FORH=0TO3:P=P+1
		 *	40 BRI=BRI+CON:FORG=1TO15:R(P)=COS(A)*SAT+BRI
		 *	50 G(P)=(COS(S)*SAT)*.50+BRI:B(P)=(COS(D)*SAT)*1.9+BRI:P=P+1
		 *	55 A=A+W:S=S+W:D=D+W:NEXT:R(P-16)=L(H):G(P-16)=L(H):B(P-16)=L(H)
		 *	60 NEXT:R(63)=R(48):G(63)=G(48):B(63)=B(48)
		 *	70 FORH=0TO63STEP1:R=INT(R(H)):G=INT(G(H)):B=INT(B(H)):IFR<0THENR=0
		 *	80 IFG<0THENG=0
		 *	90 IFB<0THENB=0
		 *	91 IFR>255THENR=255
		 *	92 IFG>255THENG=255
		 *	93 IFB>255THENB=255
		 *	100 PRINTRIGHT$(STR$(H),2);" $";:R=R+256:G=G+256:B=B+256
		 *	110 PRINTRIGHT$(HEX$(R),2);",$";RIGHT$(HEX$(G),2);",$";RIGHT$(HEX$(B),2)
		 *	115 IF(H AND15)=15 THENIFINKEY$=""THEN115ELSEPRINT
		 *	120 NEXT
		 */
		static const UINT8 cmp2rgb[] = {
			0x00, 0x00, 0x00,	/* 0 */
			0x00, 0x4E, 0x00,	/* 1 */
			0x00, 0x47, 0x00,	/* 2 */
			0x04, 0x37, 0x00,	/* 3 */
			0x33, 0x22, 0x00,	/* 4 */
			0x5D, 0x0A, 0x00,	/* 5 */
			0x7B, 0x00, 0x00,	/* 6 */
			0x85, 0x00, 0x20,	/* 7 */
			0x7C, 0x00, 0x72,	/* 8 */
			0x60, 0x00, 0xB4,	/* 9 */
			0x37, 0x00, 0xD8,	/* 10 */
			0x08, 0x07, 0xD9,	/* 11 */
			0x00, 0x1F, 0xB7,	/* 12 */
			0x00, 0x35, 0x77,	/* 13 */
			0x00, 0x46, 0x26,	/* 14 */
			0x00, 0x4E, 0x00,	/* 15 */
			0x2F, 0x2F, 0x2F,	/* 16 */
			0x00, 0x83, 0x00,	/* 17 */
			0x18, 0x7D, 0x00,	/* 18 */
			0x40, 0x6E, 0x00,	/* 19 */
			0x6B, 0x5B, 0x00,	/* 20 */
			0x91, 0x45, 0x00,	/* 21 */
			0xAC, 0x32, 0x0E,	/* 22 */
			0xB5, 0x25, 0x5A,	/* 23 */
			0xAD, 0x20, 0xA4,	/* 24 */
			0x94, 0x24, 0xE0,	/* 25 */
			0x6E, 0x30, 0xFF,	/* 26 */
			0x43, 0x43, 0xFF,	/* 27 */
			0x1B, 0x58, 0xE2,	/* 28 */
			0x00, 0x6C, 0xA8,	/* 29 */
			0x00, 0x7B, 0x5E,	/* 30 */
			0x00, 0x83, 0x12,	/* 31 */
			0x78, 0x78, 0x78,	/* 32 */
			0x3E, 0xB8, 0x31,	/* 33 */
			0x58, 0xB2, 0x01,	/* 34 */
			0x7C, 0xA5, 0x00,	/* 35 */
			0xA2, 0x94, 0x00,	/* 36 */
			0xC5, 0x80, 0x15,	/* 37 */
			0xDD, 0x6F, 0x4E,	/* 38 */
			0xE5, 0x63, 0x93,	/* 39 */
			0xDE, 0x5F, 0xD6,	/* 40 */
			0xC7, 0x62, 0xFF,	/* 41 */
			0xA5, 0x6D, 0xFF,	/* 42 */
			0x7F, 0x7E, 0xFF,	/* 43 */
			0x5B, 0x91, 0xFF,	/* 44 */
			0x40, 0xA4, 0xDA,	/* 45 */
			0x32, 0xB1, 0x97,	/* 46 */
			0x35, 0xB8, 0x52,	/* 47 */
			0xFF, 0xFF, 0xFF,	/* 48 */
			0x81, 0xED, 0x75,	/* 49 */
			0x98, 0xE8, 0x4A,	/* 50 */
			0xB8, 0xDD, 0x36,	/* 51 */
			0xDA, 0xCD, 0x3D,	/* 52 */
			0xF8, 0xBC, 0x5C,	/* 53 */
			0xFF, 0xAC, 0x8F,	/* 54 */
			0xFF, 0xA2, 0xCC,	/* 55 */
			0xFF, 0x9E, 0xFF,	/* 56 */
			0xFA, 0xA1, 0xFF,	/* 57 */
			0xDD, 0xAB, 0xFF,	/* 58 */
			0xBA, 0xBA, 0xFF,	/* 59 */
			0x9A, 0xCB, 0xFF,	/* 60 */
			0x82, 0xDB, 0xFF,	/* 61 */
			0x76, 0xE7, 0xD0,	/* 62 */
			0xFF, 0xFF, 0xFF	/* 63 */
		};
		color &= 0x3f;
		r = cmp2rgb[color * 3 + 0];
		g = cmp2rgb[color * 3 + 1];
		b = cmp2rgb[color * 3 + 2];

		if (coco3_gimevhreg[0] & 0x10) {
			/* We are on a composite monitor/TV and the monochrome phase invert
			 * flag is on in the GIME.  This means we have to average out all
			 * colors
			 */
			r = g = b = (r + g + b) / 3;
		}
	}
	else {
		/* RGB colors
		 *
		 * These colors are of the format RGBRGB, where the first 3 bits
		 * are more significant than the last three bits
		 */
		r = (((color >> 4) & 2) | ((color >> 2) & 1)) * 0x55;
		g = (((color >> 3) & 2) | ((color >> 1) & 1)) * 0x55;
		b = (((color >> 2) & 2) | ((color >> 0) & 1)) * 0x55;
	}
	*red = r;
	*green = g;
	*blue = b;
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

int coco3_calculate_rows(int *bordertop, int *borderbottom)
{
	int rows = 0;
	int t = 0;
	int b = 0;

	switch((coco3_gimevhreg[1] & 0x60) >>5) {
	case 0:
		rows = 192;
		t = 43;
		b = 28;
		break;
	case 1:
		rows = 199;
		t = 41;
		b = 23;
		break;
	case 2:
		rows = 0;	/* NYI - This is "zero/infinite" lines, according to Sock Master */
		t = 132;
		b = 131;
		break;
	case 3:
		rows = 225;
		t = 26;
		b = 12;
		break;
	}

	if (bordertop)
		*bordertop = t;
	if (borderbottom)
		*borderbottom = b;
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

	return (indexx == 7) ? coco3_calculate_rows(NULL, NULL) : hires_linesperrow[indexx];
}

static int coco3_hires_vidbase(void)
{
	return (((coco3_gimevhreg[5] * 0x800) + (coco3_gimevhreg[6] * 8)));

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
		rows = coco3_calculate_rows(NULL, NULL) / coco3_hires_linesperrow();
		logerror(" @ %dx%d\n", cols, rows);

		vidbase = coco3_hires_vidbase();
		bytesperrow = (coco3_gimevhreg[7] & 0x80) ? 256 : visualbytesperrow;
		logerror("CoCo3 HiRes Video: Occupies memory %05x-%05x\n", vidbase, (vidbase + (rows * bytesperrow) - 1) & 0x7ffff);
		break;
	}
}
#endif

static UINT8 *coco3_textmapper_noattr(UINT8 *mem, int param, int *fg, int *bg, int *attr)
{
	UINT8 *result;
	UINT8 *RAM;
	int b;
	
	RAM = (UINT8 *) param;
	b = (*mem) & 0x7f;
	if (b < 32) {
		/* Characters 0-31 are at $FA10 - $FB0F */
		result = &RAM[0x80000 + 0xfa10 - 0x8000 + (b * 8)];
	}
	else {
		/* Characters 32-127 are at $F09D - $F39C */
		result = &RAM[0x80000 + 0xf09d - 0x8000 + ((b - 32) * 8)];
	}
	return result;
}

static UINT8 *coco3_textmapper_attr(UINT8 *mem, int param, int *fg, int *bg, int *attr)
{
	int b;
	int a;

	b = mem[1];
	*bg = b & 0x07;
	*fg = 8 + ((b & 0x38) >> 3);

	a = 0;
	if (b & 0x80)
		a |= RASTERBITS_CHARATTR_BLINKING;
	if (b & 0x40)
		a |= RASTERBITS_CHARATTR_UNDERLINE;
	*attr = a;
	
	return coco3_textmapper_noattr(mem, param, NULL, NULL, NULL);
}

/*
 * All models of the CoCo has 262 scan lines.  However, we pretend that it has
 * 240 so that the emulation fits on a 640x480 screen
 */
void coco3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	rastertrack_sync();
	rastertrack_refresh(bitmap, full_refresh);
}

static void coco3_getvideoinfo(int full_refresh, struct rasterbits_source *rs,
	struct rasterbits_videomode *rvm, struct rasterbits_frame *rf)
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

	/* Did the user change between CMP and RGB? */
	cmprgb = readinputport(12) & 0x08;
	if (cmprgb != old_cmprgb) {
		old_cmprgb = cmprgb;

		/* Reset all colors and border */
		coco3_borderred = -1;	/* force border to redraw */
		for (i = 0; i < 16; i++)
			coco3_vh_palette_change_color(i, paletteram[i]);
	}

	if (coco3_hires) {
		static int last_blink;
		int linesperrow, rows = 0;
		int borderred, bordergreen, borderblue;
		int visualbytesperrow;
		int bordertop, borderbottom;

		rows = coco3_calculate_rows(&bordertop, &borderbottom);
		linesperrow = coco3_hires_linesperrow();

		/* check border */
		coco3_compute_color(coco3_gimevhreg[2] & 0x3f, &borderred, &bordergreen, &borderblue);
		full_refresh += coco3_vh_setborder(borderred, bordergreen, borderblue);

		/* check palette recalc */
		if (palette_recalc() || full_refresh) {
			full_refresh = 1;

#if LOG_VIDEO
			log_video();
#endif
		}

		/*
		 * TODO - We should support the case where there is a rounding
		 * error when rows is divided by linesperrow
		 */

		rs->videoram = RAM;
		rs->size = 0x80000;
		rs->position = coco3_hires_vidbase();
		rs->db = full_refresh ? NULL : dirtybuffer;
		rvm->height = (rows + linesperrow - 1) / linesperrow;
		rvm->metapalette = NULL;
		rvm->flags = (coco3_gimevhreg[0] & 0x80) ? RASTERBITS_FLAG_GRAPHICS : RASTERBITS_FLAG_TEXT;
		if (coco3_gimevhreg[7]) {
			rvm->flags |= RASTERBITS_FLAG_WRAPINROW;
			rvm->offset = ((coco3_gimevhreg[7] & 0x7f) * 2);
			rvm->wrapbytesperrow = 256;
		}
		else {
			rvm->offset = 0;
		}
		rf->width = (coco3_gimevhreg[1] & 0x04) ? 640 : 512;
		rf->height = rows;
		rf->border_pen = full_refresh ? Machine->pens[16] : -1;
		rf->total_scanlines = 263;
		rf->top_scanline = bordertop;

		if (coco3_gimevhreg[0] & 0x80) {
			/* Graphics */
			switch(coco3_gimevhreg[1] & 3) {
			case 0:
				/* Two colors */
				rvm->depth = 1;
				break;
			case 1:
				/* Four colors */
				rvm->depth = 2;
				break;
			case 2:
				/* Sixteen colors */
				rvm->depth = 4;
				break;
			case 3:
				/* Blank screen */
				/* TODO - Draw a blank screen! */
				rvm->depth = 4;
				break;
			}
			visualbytesperrow = 16 << ((coco3_gimevhreg[1] & 0x18) >> 3);
		}
		else {
			/* Text */
			visualbytesperrow = (coco3_gimevhreg[1] & 0x10) ? 64 : 32;

			if (coco3_gimevhreg[1] & 1) {
				/* With attributes */
				rvm->depth = 16;
				rvm->u.text.mapper = coco3_textmapper_attr;
				visualbytesperrow *= 2;

				if (coco3_blinkstatus != last_blink)
					rvm->flags |= RASTERBITS_FLAG_BLINKNOW;
				if (coco3_blinkstatus)
					rvm->flags |= RASTERBITS_FLAG_BLINKING;
				last_blink = coco3_blinkstatus;				
			}
			else {
				/* Without attributes */
				rvm->depth = 8;
				rvm->u.text.mapper = coco3_textmapper_noattr;
			}
			rvm->u.text.mapper_param = (int) RAM;
			rvm->u.text.fontheight = 8;

			/* To quote SockMaster:
			 *
			 * The underline attribute will light up the bottom scan line of the character
			 * if the lines are set to 8 or 9.  Not appear at all when less, or appear on
			 * the 2nd to bottom scan line if set higher than 9.  Further exception being
			 * the $x7 setting where the whole screen is filled with only one line of data
			 * - but it's glitched - the line repeats over and over again every 16 scan
			 * lines..  Nobody will use this mode, but that's what happens if you want to
			 * make things really authentic :)
			 *
			 * NPW Note: The '$x7' mode is not yet implemented
			 */
			if (linesperrow < 8)
				rvm->u.text.underlinepos = -1;
			else if (linesperrow < 10)
				rvm->u.text.underlinepos = linesperrow - 1;
			else
				rvm->u.text.underlinepos = linesperrow - 2;
		}

		if (coco3_gimevhreg[1] & 0x04)
			visualbytesperrow |= (visualbytesperrow / 4);

		rvm->width = visualbytesperrow * 8 / rvm->depth;
		rvm->bytesperrow = (coco3_gimevhreg[7] & 0x80) ? 256 : visualbytesperrow;

		if (full_refresh)
			memset(dirtybuffer, 0, ((rows + linesperrow - 1) / linesperrow) * rvm->bytesperrow);
	}
	else {
		int bordercolor = 0, borderred, bordergreen, borderblue;

		switch(m6847_get_bordercolor()) {
		case M6847_BORDERCOLOR_BLACK:
			bordercolor = 0;
			break;

		case M6847_BORDERCOLOR_GREEN:
			bordercolor = 18;
			break;

		case M6847_BORDERCOLOR_WHITE:
			bordercolor = 63;
			break;

		case M6847_BORDERCOLOR_ORANGE:
			bordercolor = 38;
			break;
		}

		coco3_compute_color(bordercolor, &borderred, &bordergreen, &borderblue);

		full_refresh += coco3_vh_setborder(borderred, bordergreen, borderblue);
		if (palette_recalc())
			full_refresh = 1;

		internal_m6847_vh_screenrefresh(rs, rvm, rf,
			full_refresh, coco3_metapalette,
			&RAM[coco3_lores_vidbase()],
			1, (full_refresh ? 16 : -1), 2,
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
			rastertrack_touchvideomode();
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
			rastertrack_touchvideomode();
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
			rastertrack_touchvideomode();
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
			rastertrack_touchvideomode();
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
		rastertrack_touchvideomode();
	}
}

