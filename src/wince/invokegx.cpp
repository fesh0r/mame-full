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

#define RGBPART(quad,member,base,size)	((((UINT16) ((quad)->member)) >> (8-(size))) << (base))

template<class palent>
palent blit(BYTE *pvBits, long cbxPitch, long cbyPitch, int width, int height, UINT8 **linep, const void *palette)
{
	UINT8 *line;
	int x, y;
	palent ent;

	// Schwag code to work around bugs in MSFT's compiler
	memset(&ent, sizeof(ent), sizeof(ent));

	// Do the dirty work
	y = height;
	while(y--) {
		line = *(linep++);
		pvBits += cbyPitch;
		x = width;
		while(x--) {
			*((palent *) pvBits) = ((const palent *) palette)[*line];
			line++;
			pvBits += cbxPitch;
		}
	}

	return ent;
}

void gx_blit(struct osd_bitmap *bitmap, int update, const RGBQUAD *palette, int palette_len)
{
	BYTE mappedpalette[256 * 3];
	BYTE *pvBits;
	GXDisplayProperties prop;
	const RGBQUAD *quad;
	int i;
	UINT8 **linep;
	int width, height;
	long cbxPitch, cbyPitch;

	// Get the surface
	prop = GXGetDisplayProperties();
	pvBits = (BYTE *) GXBeginDraw();

	// Set up instance variables
	width = bitmap->width;
	height = bitmap->height;
	linep = bitmap->line;
	cbxPitch = prop.cbxPitch;
	cbyPitch = prop.cbyPitch - (cbxPitch * bitmap->width);
	pvBits -= cbyPitch;

	if (prop.ffFormat & kfDirect)
	{
		if (prop.ffFormat & kfDirect555)
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((UINT16 *) mappedpalette)[i] = RGBPART(quad,rgbRed,10,5) | RGBPART(quad,rgbGreen,5,5) | RGBPART(quad,rgbBlue,0,5);
			}
			blit<UINT16>(pvBits, cbxPitch, cbyPitch, width, height, linep, mappedpalette);
		}
		else if (prop.ffFormat & kfDirect565) 
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((UINT16 *) mappedpalette)[i] = RGBPART(quad,rgbRed,11,5) | RGBPART(quad,rgbGreen,5,6) | RGBPART(quad,rgbBlue,0,5);
			}
			blit<UINT16>(pvBits, cbxPitch, cbyPitch, width, height, linep, mappedpalette);
		}
		else if (prop.ffFormat & kfDirect888)
		{
			for (i = 0; i < palette_len; i++) {
				quad = &palette[i];
				((RGBTRIPLE *) mappedpalette)[i].rgbtRed = quad->rgbRed;
				((RGBTRIPLE *) mappedpalette)[i].rgbtGreen = quad->rgbGreen;
				((RGBTRIPLE *) mappedpalette)[i].rgbtBlue = quad->rgbBlue;
			}
			blit<RGBTRIPLE>(pvBits, cbxPitch, cbyPitch, width, height, linep, mappedpalette);
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
