/***************************************************************************

	Video hardware for CoCo/Dragon family

	driver by Nathan Woods

	See mess/machine/dragon.c for references

	TODO:
		- Implement "burst phase invert" (it switches the artifact colors)
		- Figure out what Bit 3 of $FF98 is and implement it
		- Learn more about the "mystery" CoCo 3 video modes
		- Support the VSC register


	Mid frame raster effects (source John Kowalski)
		Here are the things that get changed mid-frame:
 			-palette registers ($ffb0-$ffbf)
 			-horizontal resolution (switches between 256 and 320 pixels, $ff99)
			-horizontal scroll position (bits 0-6 $ff9f)
			-horizontal virtual screen (bit 7 $ff9f)
			-pixel height (bits 0-2 $ff98)
			-border color ($ff9a)
		On the positive side, you don't have to worry about registers
		$ff9d/ff9e being changed mid-frame.  Even if they are changed
		mid-frame, they have no effect on the displayed image.  The video
		address only gets latched at the top of the frame.
	
***************************************************************************/


#include <assert.h>
#include <math.h>

#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/m6847.h"
#include "vidhrdw/generic.h"
#include "includes/dragon.h"



/*************************************
 *
 *	Global variables
 *
 *************************************/

int coco3_gimevhreg[8];



/*************************************
 *
 *	Local variables
 *
 *************************************/

static int coco3_hires;
static int coco3_blinkstatus;
static int coco3_vidbase;



/*************************************
 *
 *	Parameters
 *
 *************************************/

#define MAX_HIRES_VRAM	57600

#ifdef MAME_DEBUG
#define LOG_BORDER	0
#define LOG_PALETTE	0
#define LOG_GIME	0
#define LOG_VIDEO	0
#define LOG_MISC	0
#else /* !MAME_DEBUG */
#define LOG_BORDER	0
#define LOG_PALETTE	0
#define LOG_GIME	0
#define LOG_VIDEO	0
#define LOG_MISC	0
#endif /* MAME_DEBUG */

static int coco3_palette_recalc(int force);

/* --------------------------------------------------
 * CoCo 1/2 Stuff
 * -------------------------------------------------- */

void dragon_charproc(UINT8 c);
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

static int internal_video_start_coco(int m6847_version, void (*charproc)(UINT8))
{
	struct m6847_init_params p;

	m6847_vh_normalparams(&p);
	p.version = m6847_version;
	p.artifactdipswitch = COCO_DIP_ARTIFACTING;
	p.ram = mess_ram;
	p.ramsize = mess_ram_size;
	p.charproc = charproc;
	p.hs_func = coco_m6847_hs_w;
	p.fs_func = coco_m6847_fs_w;
	p.callback_delay = (TIME_IN_HZ(894886) * 2);

	if (video_start_m6847(&p))
		return 1;
	return 0;
}

VIDEO_START( dragon )
{
	return internal_video_start_coco(M6847_VERSION_ORIGINAL_PAL, dragon_charproc );
}

VIDEO_START( coco )
{
	return internal_video_start_coco(M6847_VERSION_ORIGINAL_NTSC, dragon_charproc );
}

VIDEO_START( coco2b )
{
	return internal_video_start_coco(M6847_VERSION_M6847T1_NTSC, coco2b_charproc);
}

WRITE8_HANDLER(coco_ram_w)
{
	if (offset < mess_ram_size) {
		if (mess_ram[offset] != data) {
			m6847_touch_vram(offset);
			mess_ram[offset] = data;
		}
	}
}

/* --------------------------------------------------
 * CoCo 3 Stuff
 * -------------------------------------------------- */

static void coco3_calc_vidbase(int ff9d_mask, int ff9e_mask)
{
	coco3_vidbase = (
		((coco3_gimevhreg[3] & 0x0f)		* 0x80000) +
		((coco3_gimevhreg[5] & ff9d_mask)	* 0x800) +
		((coco3_gimevhreg[6] & ff9e_mask)	* 8)
		) % mess_ram_size;
}

static void coco3_frame_callback(struct videomap_framecallback_info *info)
{
	int border_top, rows;

	rows = coco3_calculate_rows(&border_top, NULL);

	if (coco3_hires)
	{
		/* hires CoCo 3 specific video modes */
		coco3_calc_vidbase(0xff, 0xff);

		info->video_base = coco3_vidbase;
		info->bordertop_scanlines = border_top;
		info->visible_scanlines = rows;

		if (coco3_gimevhreg[7] & 0x80)
		{
			/* we have horizontal scrolling */
			info->pitch = 256;
		}
		else
		{
			if (coco3_gimevhreg[0] & 0x80)
			{
				/* graphics */
				info->pitch = 16 << ((coco3_gimevhreg[1] & 0x18) >> 3);
			}
			else
			{
				/* text */
				info->pitch = (coco3_gimevhreg[1] & 0x10) ? 64 : 32;
				if (coco3_gimevhreg[1] & 1)
					info->pitch *= 2;
			}
			if (coco3_gimevhreg[1] & 0x04)
				info->pitch |= info->pitch / 4;
		}
	}
	else
	{
		/* legacy CoCo 1/2 video modes */

		/* some FF9D/E bits are dropped in lo-res mode (Source: John Kowalski) */
		coco3_calc_vidbase(0xe0, 0x3f);

		/* call back door into m6847 emulation */
		internal_m6847_frame_callback(info, coco3_vidbase, border_top, rows);
	}
}

static UINT16 coco3_metapalette[16];

static UINT8 coco3_lores_charproc(UINT32 c, UINT16 *charpalette, int row)
{
	return internal_m6847_charproc(c, charpalette, coco3_metapalette, row, 1);
}

static UINT8 internal_coco3_hires_charproc(UINT32 c, int row)
{
	const UINT8 *ROM;
	const UINT8 *resultptr;
	UINT8 result;

	if (row >= 8)
	{
		result = 0;
	}
	else
	{
		/* subtracting here so that we can get an offset that looks like a real CoCo address */
		ROM = memory_region(REGION_CPU1) - 0x8000;

		c &= 0x7f;
		if (c < 32)
			resultptr = &ROM[0xfa10 + (c * 8)];	/* characters 0-31 are at $FA10 - $FB0F */
		else
			resultptr = &ROM[0xf09d + ((c - 32) * 8)];	/* characters 32-127 are at $F09D - $F39C */
		result = resultptr[row];
	}
	return result;
}

static int coco3_hires_linesperrow(void);

static UINT8 coco3_hires_charproc_withattr(UINT32 c, UINT16 *charpalette, int row)
{
	int linesperrow;
	int underlinepos = -1;

	/* foreground and background */
	charpalette[0] = paletteram[((c >>  8) & 0x07) + 0];
	charpalette[1] = paletteram[((c >> 11) & 0x07) + 8];

	/* blink? */
	if (c & 0x8000)
	{
		if (!coco3_blinkstatus)
			return 0x00;
	}

	/* underline? */
	if (c & 0x4000)
	{
		/* to quote SockMaster:
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
		linesperrow = coco3_hires_linesperrow();
		if (linesperrow >= 8)
		{
			if (linesperrow >= 10)
				underlinepos = linesperrow - 2;
			else
				underlinepos = linesperrow - 1;
			if (underlinepos == row)
				return 0xff;
		}
	}
	return internal_coco3_hires_charproc(c, row);
}

static UINT8 coco3_hires_charproc_withoutattr(UINT32 c, UINT16 *charpalette, int row)
{
	charpalette[0] = paletteram[0];
	charpalette[1] = paletteram[1];
	return internal_coco3_hires_charproc(c, row);
}

static void coco3_compute_color(int color, int *red, int *green, int *blue);

static void coco3_getcolorrgb(int color, UINT8 *red, UINT8 *green, UINT8 *blue)
{
	int r, g, b;
	coco3_compute_color(color, &r, &g, &b);
	*red = (UINT8) r;
	*green = (UINT8) g;
	*blue = (UINT8) b;
}

static int coco3_setup_dynamic_artifact_palette(int artifact_mode, UINT8 *bgcolor, UINT8 *fgcolor)
{
	coco3_getcolorrgb(paletteram[(artifact_mode & 2) ? 10 : 8], &bgcolor[0], &bgcolor[1], &bgcolor[2]);
	coco3_getcolorrgb(paletteram[(artifact_mode & 2) ? 11 : 9], &fgcolor[0], &fgcolor[1], &fgcolor[2]);
	return 64;
}

static UINT16 coco3_calculate_artifact_color(UINT16 metacolor, int artifact_mode)
{
	UINT16 result;
	switch(metacolor)
	{
	case 0:
		result = paletteram[(artifact_mode & 2) ? 10 : 8];
		break;
	case 15:
		result = paletteram[(artifact_mode & 2) ? 11 : 9];
		break;
	default:
		result = (metacolor - 1) + 64;
		break;					
	}
	return result;
}

static struct internal_m6847_linecallback_interface coco3_linecallback_interface =
{
	2,
	coco3_lores_charproc,
	coco3_calculate_artifact_color,
	coco3_setup_dynamic_artifact_palette
};

static void coco3_line_callback(struct videomap_linecallback_info *info)
{
	int i;

	if (coco3_hires)
	{
		/* new CoCo 3 video modes */
		info->visible_columns = (coco3_gimevhreg[1] & 0x04) ? 640 : 512;
		info->scanlines_per_row = coco3_hires_linesperrow();
		info->borderleft_columns = (Machine->drv->screen_width - info->visible_columns) / 2;
		info->border_value = 0xff;
		info->offset = (coco3_gimevhreg[7] & 0x7f) * 2;
		info->offset_wrap = 256;

		if (coco3_gimevhreg[0] & 0x80)
		{
			/* graphics */
			info->grid_depth = 1 << (coco3_gimevhreg[1] & 3);
			info->grid_width = 1 << (((coco3_gimevhreg[1] >> 3) & 0x03) - (coco3_gimevhreg[1] & 0x03) + 7);
			info->flags = VIDEOMAP_FLAGS_USEPALETTERAM;
		}
		else
		{
			/* text */
			info->grid_depth = 8 << (coco3_gimevhreg[1] & 1);
			info->grid_width = (coco3_gimevhreg[1] & 0x10) ? 64 : 32;
			info->charproc = (coco3_gimevhreg[1] & 1) ? coco3_hires_charproc_withattr : coco3_hires_charproc_withoutattr;
		}
		if (coco3_gimevhreg[1] & 0x04)
			info->grid_width |= info->grid_width / 4;
	}
	else
	{
		internal_m6847_line_callback(info, coco3_metapalette, &coco3_linecallback_interface);
	}

	/* Now translate the pens */
	for (i = 0; i < (sizeof(coco3_metapalette) / sizeof(coco3_metapalette[0])); i++)
		coco3_metapalette[i] = paletteram[i];
}

static UINT16 coco3_get_border_color_callback(void)
{
	int bordercolor = 0;

	if (coco3_hires)
	{
		bordercolor = coco3_gimevhreg[2] & 0x3f;
	}
	else
	{
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
	}
	return bordercolor;
}

static struct videomap_interface coco3_videomap_interface =
{
	VIDEOMAP_FLAGS_MEMORY8 | VIDEOMAP_FLAGS_BUFFERVIDEO,
	coco3_frame_callback,
	coco3_line_callback,
	coco3_get_border_color_callback
};

VIDEO_START( coco3 )
{
	struct m6847_init_params p;

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_M6847T1_NTSC;
	p.artifactdipswitch = COCO_DIP_ARTIFACTING;
	p.ram = mess_ram;
	p.ramsize = mess_ram_size;
	p.charproc = coco2b_charproc;
	p.hs_func = coco3_m6847_hs_w;
	p.fs_func = coco3_m6847_fs_w;

	/* initialize palette RAM */
	paletteram = auto_malloc(16 * sizeof(int));
	if (!paletteram)
		return 1;
	memset(paletteram, 0, 16 * sizeof(int));

	if (internal_video_start_m6847(&p, &coco3_videomap_interface, MAX_HIRES_VRAM)) {
		paletteram = NULL;
		return 1;
	}

	coco3_vh_reset();
	return 0;
}

void coco3_vh_reset(void)
{
	int i;
	for (i = 0; i < (sizeof(coco3_gimevhreg) / sizeof(coco3_gimevhreg[0])); i++)
		coco3_gimevhreg[i] = 0;

	coco3_hires = coco3_blinkstatus = 0;
	coco3_palette_recalc(1);
}

static void get_composite_color(int color, int *r, int *g, int *b)
{
	/* CMP colors
	 *
	 * These colors are of the format IICCCC, where II is the intensity and
	 * CCCC is the base color.  There is some weirdness because intensity
	 * is often different for each base color.
	 *
	 * The code below is based on an algorithm specified in the following
	 * CoCo BASIC program was used to approximate composite colors.
	 * (Program by SockMaster):
	 * 
	 * 10 POKE65497,0:DIMR(63),G(63),B(63):WIDTH80:PALETTE0,0:PALETTE8,54:CLS1
	 * 20 SAT=92:CON=70:BRI=-50:L(0)=0:L(1)=47:L(2)=120:L(3)=255
	 * 30 W=.4195456981879*1.01:A=W*9.2:S=A+W*5:D=S+W*5:P=0:FORH=0TO3:P=P+1
	 * 40 BRI=BRI+CON:FORG=1TO15:R(P)=COS(A)*SAT+BRI
	 * 50 G(P)=(COS(S)*SAT)*1+BRI:B(P)=(COS(D)*SAT)*1+BRI:P=P+1
	 * 55 A=A+W:S=S+W:D=D+W:NEXT:R(P-16)=L(H):G(P-16)=L(H):B(P-16)=L(H)
	 * 60 NEXT:R(63)=R(48):G(63)=G(48):B(63)=B(48)
	 * 70 FORH=0TO63STEP1:R=INT(R(H)):G=INT(G(H)):B=INT(B(H)):IFR<0THENR=0
	 * 80 IFG<0THENG=0
	 * 90 IFB<0THENB=0
	 * 91 IFR>255THENR=255
	 * 92 IFG>255THENG=255
	 * 93 IFB>255THENB=255
	 * 100 PRINTRIGHT$(STR$(H),2);" $";:R=R+256:G=G+256:B=B+256
	 * 110 PRINTRIGHT$(HEX$(R),2);",$";RIGHT$(HEX$(G),2);",$";RIGHT$(HEX$(B),2)
	 * 115 IF(H AND15)=15 THENIFINKEY$=""THEN115ELSEPRINT
	 * 120 NEXT
	 *
	 *	At one point, we used a different SockMaster program, but the colors
	 *	produced were too dark for people's taste
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

	double saturation, brightness, contrast;
	int offset;
	double w;

	switch(color) {
	case 0:
		*r = *g = *b = 0;
		break;

	case 16:
		*r = *g = *b = 47;
		break;

	case 32:
		*r = *g = *b = 120;
		break;

	case 48:
	case 63:
		*r = *g = *b = 255;
		break;

	default:
		w = .4195456981879*1.01;
		contrast = 70;
		saturation = 92;
		brightness = -50;
		brightness += ((color / 16) + 1) * contrast;
		offset = (color % 16) - 1 + (color / 16)*15;
		*r = cos(w*(offset +  9.2)) * saturation + brightness;
		*g = cos(w*(offset + 14.2)) * saturation + brightness;
		*b = cos(w*(offset + 19.2)) * saturation + brightness;

		if (*r < 0)
			*r = 0;
		else if (*r > 255)
			*r = 255;

		if (*g < 0)
			*g = 0;
		else if (*g > 255)
			*g = 255;

		if (*b < 0)
			*b = 0;
		else if (*b > 255)
			*b = 255;
		break;
	}
}

static void coco3_compute_color(int color, int *red, int *green, int *blue)
{
	if ((readinputport(COCO3_DIP_MONITORTYPE) & COCO3_DIP_MONITORTYPE_MASK) == 0)
	{
		color &= 0x3f;
		get_composite_color(color, red, green, blue);

		if (coco3_gimevhreg[0] & 0x10)
		{
			/* We are on a composite monitor/TV and the monochrome phase invert
			 * flag is on in the GIME.  This means we have to average out all
			 * colors
			 */
			*red = *green = *blue = (*red + *green + *blue) / 3;
		}
	}
	else {
		/* RGB colors
		 *
		 * These colors are of the format RGBRGB, where the first 3 bits
		 * are more significant than the last three bits
		 */
		*red = (((color >> 4) & 2) | ((color >> 2) & 1)) * 0x55;
		*green = (((color >> 3) & 2) | ((color >> 1) & 1)) * 0x55;
		*blue = (((color >> 2) & 2) | ((color >> 0) & 1)) * 0x55;
	}
}

static int coco3_palette_recalc(int force)
{
	int flag;
	int i, r, g, b;   
	static int lastflag;

	flag = readinputport(COCO3_DIP_MONITORTYPE) & (COCO3_DIP_MONITORTYPE_MASK | ((coco3_gimevhreg[0] & 0x10) << 16));
	if (force || (flag != lastflag)) {
		lastflag = flag;

		for (i = 0; i < 64; i++) {   
			coco3_compute_color(i, &r, &g, &b);   
			palette_set_color(i, r, g, b);   
		}   
		return 1;
	}
	return 0;
}

void coco3_vh_blink(void)
{
	coco3_blinkstatus = !coco3_blinkstatus;
	if (coco3_hires && ((coco3_gimevhreg[0] & 0x80) == 0))
	{
		videomap_invalidate_frameinfo();
		schedule_full_refresh();
	}
}

WRITE8_HANDLER(coco3_palette_w)
{
	videomap_invalidate_lineinfo();

	data &= 0x3f;
	paletteram[offset] = data;

#if LOG_PALETTE
	logerror("CoCo3 Palette: %i <== $%02x\n", offset, data);
#endif
}

int coco3_calculate_rows(int *bordertop, int *borderbottom)
{
	int rows = 0;
	int t = 0;
	int b = 0;


	switch((coco3_gimevhreg[1] & 0x60) >> 5) {
	case 0:
		rows = 192;
		t = coco3_vidvars.bordertop_192;
		break;
	case 1:
		rows = 199;
		t = coco3_vidvars.bordertop_199;
		break;
	case 2:
		rows = 0;	/* NYI - This is "zero/infinite" lines, according to Sock Master */
		t = coco3_vidvars.bordertop_0;
		break;
	case 3:
		rows = 225;
		t = coco3_vidvars.bordertop_225;
		break;
	}

	b = 263 - rows - b;

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

		vidbase = coco3_vidbase;
		bytesperrow = (coco3_gimevhreg[7] & 0x80) ? 256 : visualbytesperrow;
		logerror("CoCo3 HiRes Video: Occupies memory %05x-%05x\n", vidbase, (vidbase + (rows * bytesperrow) - 1) & 0x7ffff);
		break;
	}
}
#endif

/*
 * All models of the CoCo has 262.5 scan lines.  However, we pretend that it has
 * 240 so that the emulation fits on a 640x480 screen
 */
VIDEO_UPDATE( coco3 )
{
	coco3_palette_recalc(0);
	internal_video_update_m6847(screen, bitmap, cliprect, do_skip);
}

static void coco3_ram_w(int offset, int data, int block)
{
	int vidbase;
	unsigned int vidbasediff;

	offset = coco3_mmu_translate(block, offset);
	if (offset & 0x80000000)
		return;

	if ((offset >= 0) && (mess_ram[offset] != data)) {
		if (coco3_hires) {
			vidbase = coco3_vidbase;
			vidbasediff = (unsigned int) (offset - vidbase) & 0x7ffff;
			if (vidbasediff < MAX_HIRES_VRAM) {
				dirtybuffer[vidbasediff] = 1;
			}
		}
		else {
			/* Apparently, lores video
			 */
			if (offset >= coco3_vidbase)
				m6847_touch_vram(offset - coco3_vidbase);
		}

		mess_ram[offset] = data;
	}
}

void coco3_ram_b1_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 0);
}
void coco3_ram_b2_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 1);
}
void coco3_ram_b3_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 2);
}
void coco3_ram_b4_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 3);
}
void coco3_ram_b5_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 4);
}
void coco3_ram_b6_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 5);
}
void coco3_ram_b7_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 6);
}
void coco3_ram_b8_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 7);
}
void coco3_ram_b9_w (offs_t offset, UINT8 data)
{
	coco3_ram_w(offset, data, 8);
}

 READ8_HANDLER(coco3_gimevh_r)
{
	return coco3_gimevhreg[offset];
}



/* write handler for GIME registers $FF98-$FF9F */
WRITE8_HANDLER(coco3_gimevh_w)
{
	int xorval;

#if LOG_GIME
	logerror("CoCo3 GIME: $%04x <== $%02x pc=$%04x scanline=%i\n", offset + 0xff98, data, activecpu_get_pc(), cpu_getscanline());
#endif
	/* Features marked with '!' are not yet implemented */

	xorval = coco3_gimevhreg[offset] ^ data;
	if (!xorval)
		return;

	switch(offset) {
	case 0:
	case 1:
	case 4:
	case 7:
		videomap_invalidate_lineinfo();
		break;
	};

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
		if (xorval & 0xB7)
		{
			videomap_invalidate_frameinfo();
			videomap_invalidate_lineinfo();
#if LOG_GIME
			logerror("CoCo3 GIME: $ff98 forcing refresh\n");
#endif
		}
		break;

	case 1:
		/*	$FF99 Video Resolution Register
		 *		  Bit 7 Undefined
		 *		  Bits 5-6 LPF Lines per Field (Number of Rows)
		 *		  Bits 2-4 HRES Horizontal Resolution
		 *		  Bits 0-1 CRES Color Resolution
		 */
		videomap_invalidate_frameinfo();
		videomap_invalidate_lineinfo();
		break;

	case 2:
		/*	$FF9A Border Register
		 *		  Bits 6,7 Unused
		 *		  Bits 0-5 BRDR Border color
		 */
#if LOG_BORDER
		logerror("CoCo3 GIME: Writing $%02x into border; scanline=%i\n", data, cpu_getscanline());
#endif
		if (coco3_hires)
			videomap_invalidate_border();
		break;

	case 4:
		/*	$FF9C Vertical Scroll Register
		 *		  Bits 4-7 Reserved
		 *		! Bits 0-3 VSC Vertical Scroll bits
		 */
		videomap_invalidate_frameinfo();
		break;

	case 3:
	case 5:
	case 6:
		/*	$FF9B,$FF9D,$FF9E Vertical Offset Registers
		 *
		 *	According to JK, if an odd value is placed in $FF9E on the 1986
		 *	GIME, the GIME crashes
		 *
		 *  Also, $FF9[B|D|E] are latched at the top of each screen, so
		 *  we schedule a refresh, not touch the video mode
		 *
		 *  The reason that $FF9B is not mentioned in offical documentation
		 *  is because it is only meaninful in CoCo 3's with the 2mb upgrade
		 */
		videomap_invalidate_frameinfo();
		break;

	case 7:
		/*
		 *	$FF9F Horizontal Offset Register
		 *		  Bit 7 HVEN Horizontal Virtual Enable
		 *		  Bits 0-6 X0-X6 Horizontal Offset Address
		 *
		 *  Unline $FF9D-E, this value can be modified mid frame
		 */
		videomap_invalidate_lineinfo();
		if (xorval & 0x80)
			videomap_invalidate_frameinfo();
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
		videomap_invalidate_frameinfo();
		videomap_invalidate_lineinfo();
		videomap_invalidate_border();
	}
}

