/*********************************************************************

	artworkx.h

	MESS specific artwork code

*********************************************************************/

#ifndef ARTWORKX_H
#define ARTWORKX_H

#include "fileio.h"
#include "palette.h"
#include "artwork.h"

/***************************************************************************

	Globals

***************************************************************************/

extern struct artwork_callbacks mess_artwork_callbacks;

/***************************************************************************

	Type definitions

***************************************************************************/

struct inputform_customization
{
	UINT32 ipt;
	int x, y;
	int width, height;
};

/***************************************************************************

	Prototypes

***************************************************************************/

void artwork_use_device_art(mess_image *img, const char *defaultartfile);

void artwork_get_inputscreen_customizations(struct png_info *png,
	struct inputform_customization *customizations, int length);

#endif /* ARTWORKX_H */
