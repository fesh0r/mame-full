#include <windows.h>
#include <gx.h>
#include "mamece.h"

#ifndef X86
#define HAS_GAPI 1
#endif /* X86 */

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
#if HAS_GAPI
	if (GXOpenInput() == 0)
		return 0;
#endif
	return 1;
}

int gx_close_input(void)
{
#if HAS_GAPI
	if (GXCloseInput() == 0)
		return 0;
#endif
	return 1;
}

void gx_get_default_keys(struct gx_keylist *keylist)
{
#if HAS_GAPI
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
#else
	int nOptions = GX_NORMALKEYS;

	switch(nOptions) {
	case GX_LANDSCAPEKEYS:
		keylist->vkUp    = cKeyLeft;
		keylist->vkDown  = cKeyRight;
		keylist->vkLeft  = cKeyDown;
		keylist->vkRight = cKeyUp;
		break;

	case GX_NORMALKEYS:
		keylist->vkUp    = cKeyUp;
		keylist->vkDown  = cKeyDown;
		keylist->vkLeft  = cKeyLeft;
		keylist->vkRight = cKeyRight;
		break;
	}

	keylist->vkA = cKeyA;
	keylist->vkB = cKeyB;
	keylist->vkC = cKeyC;
	keylist->vkStart = cKeyStart;
#endif
}

