#include <windows.h>
#include <gx.h>
#include "mamece.h"

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

