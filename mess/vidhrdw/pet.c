/***************************************************************************

  commodore pet discrete video circuit

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/praster.h"
#include "includes/pet.h"

void pet_init (UINT8 *memory)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	/* inversion logic on board */
    for (i = 0; i < 0x400; i++) {
		gfx[0x800+i] = gfx[0x400+i];
		gfx[0x400+i] = gfx[i]^0xff;
		gfx[0xc00+i] = gfx[0x800+i]^0xff;
	}

	praster_2_init();

	raster2.memory.ram=memory;
	raster2.text.fontsize.y=8;
	raster2.raytube.screenpos.x=raster2.raytube.screenpos.y=0;
	raster2.mode=PRASTER_GFXTEXT;

	raster2.text.charsize.x=raster2.text.charsize.y=8;
	raster2.text.visiblesize.x=raster2.text.visiblesize.y=8;
	raster2.linediff=0;raster2.text.size.x=40;
	raster2.text.size.y=25;
	raster2.cursor.on=0;

	raster2.memory.mask=raster2.memory.videoram.mask=0xffff;
	raster2.memory.videoram.size=0x400;
}

