/***************************************************************************

  spectrum.c

  Functions to emulate the video hardware of the ZX Spectrum.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *spectrum_characterram;
unsigned char *spectrum_colorram;
unsigned char *charsdirty;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
int spectrum_vh_start(void) {

	spectrum_characterram = malloc(0x1800);
	if (!spectrum_characterram) {
		return 1;
	}

	spectrum_colorram = malloc(0x300);
	if (!spectrum_colorram) {
		free(spectrum_characterram);
		return 1;
	}

	charsdirty = malloc(0x300);
	if (!charsdirty) {
		free(spectrum_colorram);
		free(spectrum_characterram);
		return 1;
	}
	memset(charsdirty,1,0x300);
	return 0;
}

void    spectrum_vh_stop(void) {
	free(spectrum_characterram);
	free(spectrum_colorram);
	free(charsdirty);
}

void spectrum_characterram_w(int offset, int data) {
	spectrum_characterram[offset] = data;
	charsdirty[(offset&0xff)*(offset>>9)] = 1;
}

int spectrum_characterram_r(int offset) {
	return(spectrum_characterram[offset]);
}


void spectrum_colorram_w(int offset,int data) {
	spectrum_colorram[offset] = data;
	charsdirty[offset] = 1;
}

int spectrum_colorram_r(int offset) {
	return(spectrum_colorram[offset]);
}


/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void spectrum_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh) {
	int count;

	if (full_refresh)
		memset(charsdirty,1,0x300);

	for (count=0;count<32*8;count++) {
		if (charsdirty[count]) {
			decodechar( Machine->gfx[0],count,spectrum_characterram,
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}

		if (charsdirty[count+256]) {
			decodechar( Machine->gfx[1],count,&spectrum_characterram[0x800],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}

		if (charsdirty[count+512]) {
			decodechar( Machine->gfx[2],count,&spectrum_characterram[0x1000],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );
		}
	}

	for (count=0;count<32*8;count++) {
		int sx=count%32;
		int sy=count/32;
		unsigned char color;

		if (charsdirty[count]) {
			color=spectrum_colorram[count];
			drawgfx(bitmap,Machine->gfx[0],
				count,
				color,
				0,0,
				sx*8,sy*8,
				0,TRANSPARENCY_NONE,0);
			charsdirty[count] = 0;
		}

		if (charsdirty[count+256]) {
			color=spectrum_colorram[count+0x100];
			drawgfx(bitmap,Machine->gfx[1],
				count,
				color,
				0,0,
				sx*8,(sy+8)*8,
				0,TRANSPARENCY_NONE,0);
			charsdirty[count+256] = 0;
		}

		if (charsdirty[count+512]) {
			color=spectrum_colorram[count+0x200];
			drawgfx(bitmap,Machine->gfx[2],
				count,
				color,
				0,0,
				sx*8,(sy+16)*8,
				0,TRANSPARENCY_NONE,0);
			charsdirty[count+512] = 0;
		}
	}
}

