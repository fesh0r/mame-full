#include <windows.h>
#include <gx.h>
#include "mamece.h"
#include "osdepend.h"

const short cKeyUp      = VK_UP;
const short cKeyDown    = VK_DOWN;
const short cKeyLeft    = VK_LEFT;
const short cKeyRight   = VK_RIGHT;
const short cKeyA       = VK_SPACE;
const short cKeyB       = 'Z';
const short cKeyC       = 'X';
const short cKeyStart   = 'C';

int gx_open_input(void)
{
	return GXOpenInput();
}

int gx_close_input(void)
{
	return GXCloseInput();
}

void gx_get_default_keys(struct gx_keylist *keylist)
{
	GXKeyList keys;
	keys = GXGetDefaultKeys(GX_NORMALKEYS);
	keylist->vkUp = keys.vkUp;
	keylist->vkDown = keys.vkDown;
	keylist->vkLeft = keys.vkLeft;
	keylist->vkRight = keys.vkRight;
	keylist->vkA = keys.vkA;
	keylist->vkB = keys.vkB;
	keylist->vkC = keys.vkC;
	keylist->vkStart = keys.vkStart;
}

int gx_open_display(HWND hWnd)
{
	return GXOpenDisplay(hWnd, GX_FULLSCREEN);
}

int gx_close_display(void)
{
	return GXCloseDisplay();
}

void *gx_begin_draw(void)
{
	return GXBeginDraw();
}

int gx_end_draw(void)
{
	return GXEndDraw();
}

void gx_get_display_properties(struct gx_display_properties *properties)
{
	GXDisplayProperties gxproperties;
	
	gxproperties = GXGetDisplayProperties();

	properties->cxWidth = gxproperties.cxWidth;
	properties->cyHeight = gxproperties.cyHeight;
	properties->cbxPitch = gxproperties.cbxPitch;
	properties->cbyPitch = gxproperties.cbyPitch;
	properties->cBPP = gxproperties.cBPP;
	properties->ffFormat = gxproperties.ffFormat;
}

// --------------------------------------------------------------------------
// Blit code - speed very critical! templates out the wazoo!!
// --------------------------------------------------------------------------

#pragma optimize("agptb", on)

inline UINT16 rgb_part(UINT16 value, int size, int base)
{
	return (value >> (8 - size)) << base;
}

inline UINT16 rgb_value(const RGBQUAD *quad, int rbits, int gbits, int bbits)
{
	return rgb_part(quad->rgbRed, rbits, gbits + bbits) | rgb_part(quad->rgbGreen, gbits, bbits) | rgb_part(quad->rgbBlue, bbits, 0);
}

template<int rbits, int gbits, int bbits>
class blend16_functor {
public:
	void blend(UINT16 *pix, UINT16 newpix)
	{
		UINT16 p = *pix;
		if (p != newpix) {
			UINT16 mask = ((1 << (rbits + gbits + bbits)) - 1);
			mask &= ~(1 << 0);
			mask &= ~(1 << (bbits));
			mask &= ~(1 << (bbits + gbits));
			*pix = ((p & mask) >> 1) + ((newpix & mask) >> 1);
		}
	}
};

class blend888_functor {
public:
	void blend(RGBTRIPLE *pix, RGBTRIPLE newpix)
	{
		pix->rgbtRed = (pix->rgbtRed) / 2 + (newpix.rgbtRed / 2);
		pix->rgbtGreen = (pix->rgbtGreen) / 2 + (newpix.rgbtGreen / 2);
		pix->rgbtBlue = (pix->rgbtBlue) / 2 + (newpix.rgbtBlue / 2);
	}
};

class blendnull_functor {
public:
	void blend(RGBTRIPLE *pix, RGBTRIPLE newpix)
	{
		// Dummy
	}

	void blend(UINT16 *pix, UINT16 newpix)
	{
		// Dummy
	}
};

// The template that does the dirty work --- hyper optimized!!!
template<class pixtyp, class dummy, int do_skipx, int do_skipy, class blend_functor>
void __fastcall blit(pixtyp *pixels, long cbxPitch, long cbyPitch, int width, int height, void **linep,
	int skipx_mask, int skipy_mask, blend_functor blend, dummy *d)
{
	UINT8 *pvBits;
	UINT16 *line;
	int x, y;
	pixtyp ent;

	pvBits = (UINT8 *) pixels;

	// Do the dirty work
	y = height;
	while(y--) {
		pvBits += cbyPitch;

		if (do_skipy && !(y & skipy_mask)) {
			// Skipped
			pvBits += cbxPitch * width;
		}
		else {
			line = (UINT16 *) *(linep++);
			x = width;
			while(x--) {
				ent = *((const pixtyp *) line);
				if (do_skipx && !(x & skipx_mask)) {
					// Skipped
					blend.blend((pixtyp *) pvBits, ent);
				}
				else {
					pvBits += cbxPitch;
					*((pixtyp *) pvBits) = ent;
				}
				line++;
			}
		}
	}

	// Schwag code to work around bugs in MSFT's compiler
	memset(d, sizeof(*d), sizeof(*d));
}

template<class pixtyp, class blend_functor>
void __fastcall blit2(pixtyp *pixels, long cbxPitch, long cbyPitch, int width, int height, void **linep,
	int skipx_mask, int skipy_mask, int do_blend, blend_functor blend)
{
	union {
		char c;
		short s;
		int i;
		long l;
		double d;
	} u;
	blendnull_functor nullfunctor;

	if (skipx_mask) {
		if (skipy_mask) {
			if (do_blend)
				blit<pixtyp, char, TRUE, TRUE, blend_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, blend, &u.c);
			else
				blit<pixtyp, char, TRUE, TRUE, blendnull_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, nullfunctor, &u.c);
		}
		else {
			if (do_blend)
				blit<pixtyp, short, TRUE, FALSE, blend_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, blend, &u.s);
			else
				blit<pixtyp, short, TRUE, FALSE, blendnull_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, nullfunctor, &u.s);
		}
	}
	else {
		if (skipy_mask) {
			if (do_blend)
				blit<pixtyp, long, FALSE, TRUE, blend_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, blend, &u.l);
			else
				blit<pixtyp, long, FALSE, TRUE, blendnull_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
					skipx_mask, skipy_mask, nullfunctor, &u.l);
		}
		else {
			blit<pixtyp, double, FALSE, FALSE, blendnull_functor>(pixels, cbxPitch, cbyPitch, width, height, linep,
				skipx_mask, skipy_mask, nullfunctor, &u.d);
		}
	}
}


// The uber-function that performs blits
void gx_blit(struct osd_bitmap *bitmap, int update, const RGBQUAD *palette, int palette_len)
{
	BYTE mappedpalette[256 * 3];
	BYTE *pvBits;
	GXDisplayProperties prop;
	const RGBQUAD *quad;
	int i;
	void **linep;
	int width, height;
	long cbxPitch, cbyPitch;
	int skipx_mask, skipy_mask;
	int do_blend;

	// Get the surface
	prop = GXGetDisplayProperties();
	pvBits = (BYTE *) GXBeginDraw();

	// Set up instance variables
	width = bitmap->width;
	height = bitmap->height;
	linep = bitmap->line;
	cbxPitch = prop.cbxPitch;
	cbyPitch = prop.cbyPitch - (cbxPitch * 240);
	pvBits -= cbxPitch;
	pvBits -= cbyPitch;

	skipx_mask = 0x0f;
	skipy_mask = 1;
	do_blend = 0;

	if (prop.ffFormat & kfDirect)
	{
		if (prop.ffFormat & kfDirect555)
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((UINT16 *) mappedpalette)[i] = rgb_value(quad, 5, 5, 5);
			}
			blit2((UINT16 *) pvBits, cbxPitch, cbyPitch, width, height, linep, skipx_mask, skipy_mask, do_blend, blend16_functor<5,5,5>());
		}
		else if (prop.ffFormat & kfDirect565) 
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((UINT16 *) mappedpalette)[i] = rgb_value(quad, 5, 6, 5);
			}
			blit2((UINT16 *) pvBits, cbxPitch, cbyPitch, width, height, linep, skipx_mask, skipy_mask, do_blend, blend16_functor<5,6,5>());
		}
		else if (prop.ffFormat & kfDirect888)
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((RGBTRIPLE *) mappedpalette)[i].rgbtRed = quad->rgbRed;
				((RGBTRIPLE *) mappedpalette)[i].rgbtGreen = quad->rgbGreen;
				((RGBTRIPLE *) mappedpalette)[i].rgbtBlue = quad->rgbBlue;
			}
			blit2((RGBTRIPLE *) pvBits, cbxPitch, cbyPitch, width, height, linep, skipx_mask, skipy_mask, do_blend, blend888_functor());
		}
	}
	else if (prop.ffFormat & kfPalette)
	{
		// ???
	}
	else
	{
		// Bad?
	}

	GXEndDraw();
}
