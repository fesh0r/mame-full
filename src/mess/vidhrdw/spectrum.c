/***************************************************************************

  spectrum.c

  Functions to emulate the video hardware of the ZX Spectrum.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* DJR 8/2/00 - Added support for FLASH 1 */

unsigned char *spectrum_characterram;
unsigned char *spectrum_colorram;
unsigned char *charsdirty;
static int frame_number;    /* Used for handling FLASH 1 */
static int flash_invert;

extern unsigned char *spectrum_128_screen_location;

extern unsigned char *ts2068_ram;
extern int ts2068_port_ff_data;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
int spectrum_vh_start(void)
{
        frame_number = 0;
        flash_invert = 0;
	spectrum_characterram = malloc(0x1800);
        if (!spectrum_characterram)
		return 1;

	spectrum_colorram = malloc(0x300);
        if (!spectrum_colorram)
        {
		free(spectrum_characterram);
		return 1;
	}

        charsdirty = malloc(0x300);
        if (!charsdirty)
        {
		free(spectrum_colorram);
		free(spectrum_characterram);
		return 1;
	}
	memset(charsdirty,1,0x300);
	return 0;
}

void    spectrum_vh_stop(void)
{
	free(spectrum_characterram);
	free(spectrum_colorram);
	free(charsdirty);
}

/* screen is stored as:
32 chars wide. first 0x100 bytes are top scan of lines 0 to 7 */

WRITE_HANDLER (spectrum_characterram_w)
{
	spectrum_characterram[offset] = data;

        charsdirty[((offset & 0x0f800)>>3) + (offset & 0x0ff)] = 1;
}

READ_HANDLER (spectrum_characterram_r)
{
	return(spectrum_characterram[offset]);
}


WRITE_HANDLER (spectrum_colorram_w)
{
	spectrum_colorram[offset] = data;
	charsdirty[offset] = 1;
}

READ_HANDLER (spectrum_colorram_r)
{
	return(spectrum_colorram[offset]);
}

/* return the color to be used inverting FLASHing colors if necessary */
INLINE unsigned char get_display_color (unsigned char color, int invert)
{
        if (invert && (color & 0x80))
                return (color & 0xc0) + ((color & 0x38) >> 3) + ((color & 0x07) << 3);
        else
                return color;
}

/* Code to change the FLASH status every 25 frames. Note this must be
   independent of frame skip etc. */
void spectrum_eof_callback(void)
{
        frame_number++;
        if (frame_number >= 25)
        {
                frame_number = 0;
                flash_invert = !flash_invert;
        }
}


/* Update FLASH status for ts2068. Assumes flash update every 1/2s. */
void ts2068_eof_callback(void)
{
        frame_number++;
        if (frame_number >= 30)
        {
                frame_number = 0;
                flash_invert = !flash_invert;
        }
}

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void spectrum_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh) {
	int count;
        static int last_invert = 0;

        if (full_refresh)
        {
		memset(charsdirty,1,0x300);
                last_invert = flash_invert;
        }
        else
        {
                /* Update all flashing characters when necessary */
                if (last_invert != flash_invert) {
                        for (count=0;count<0x300;count++)
                                if (spectrum_colorram[count] & 0x80)
                                        charsdirty[count] = 1;
                        last_invert = flash_invert;
                }
        }

        for (count=0;count<32*8;count++)
        {
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

        for (count=0;count<32*8;count++)
        {
		int sx=count%32;
		int sy=count/32;
		unsigned char color;

		if (charsdirty[count]) {
                        color=get_display_color(spectrum_colorram[count],
                                                flash_invert);
			drawgfx(bitmap,Machine->gfx[0],
				count,
				color,
				0,0,
				sx*8,sy*8,
				0,TRANSPARENCY_NONE,0);
			charsdirty[count] = 0;
		}

		if (charsdirty[count+256]) {
                        color=get_display_color(spectrum_colorram[count+0x100],
                                                flash_invert);
			drawgfx(bitmap,Machine->gfx[1],
				count,
				color,
				0,0,
				sx*8,(sy+8)*8,
				0,TRANSPARENCY_NONE,0);
			charsdirty[count+256] = 0;
		}

		if (charsdirty[count+512]) {
                        color=get_display_color(spectrum_colorram[count+0x200],
                                                flash_invert);
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


int spectrum_128_vh_start(void)
{
        return 0;
}

void    spectrum_128_vh_stop(void)
{
}

void spectrum_128_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh) {
	int count;

        /* for now 128K/+2/+3 will do a full-refresh */
        unsigned char *spectrum_128_colorram;
        unsigned char *spectrum_128_charram;

        spectrum_128_charram = spectrum_128_screen_location;
        spectrum_128_colorram = spectrum_128_screen_location + 0x01800;

        for (count=0;count<32*8;count++)
        {
                decodechar( Machine->gfx[0],count,spectrum_128_charram,
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );

                decodechar( Machine->gfx[1],count,&spectrum_128_charram[0x800],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );

                decodechar( Machine->gfx[2],count,&spectrum_128_charram[0x1000],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );
	}

        for (count=0;count<32*8;count++)
        {
		int sx=count%32;
		int sy=count/32;
		unsigned char color;

                color=get_display_color(spectrum_128_colorram[count],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[0],
				count,
				color,
				0,0,
				sx*8,sy*8,
				0,TRANSPARENCY_NONE,0);

                color=get_display_color(spectrum_128_colorram[count+0x100],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[1],
				count,
				color,
				0,0,
				sx*8,(sy+8)*8,
				0,TRANSPARENCY_NONE,0);

                color=get_display_color(spectrum_128_colorram[count+0x200],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[2],
				count,
				color,
				0,0,
				sx*8,(sy+16)*8,
				0,TRANSPARENCY_NONE,0);
	}
}

/*******************************************************************
 *
 *      Update the TS2068 display.
 *
 *      Port ff is used to set the display mode.
 *
 *      bits 2..0  Video Mode Select
 *      000 = Primary DFILE active   (at 0x4000-0x5aff)
 *      001 = Secondary DFILE active (at 0x6000-0x7aff)
 *      010 = Extended Colour Mode   (chars at 0x4000-0x57ff, colors 0x6000-0x7aff)
 *      110 = 64 column mode         (columns 0,2,4,...62 from DFILE 1
 *                                    columns 1,3,5,...63 from DFILE 2)
 *      other = unpredictable results
 *
 *      bits 5..3  64 column mode ink/paper selection
 *      000 = Black/White       100 = Green/Magenta
 *      001 = Blue/Yellow       101 = Cyan/Red
 *      010 = Red/Cyan          110 = Yellow/Blue
 *      011 = Magenta/Green     111 = White/Black
 *
 *******************************************************************/
void ts2068_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int count;

        /* TODO: Add support for Extended Color & 64 column modes */

        /* for now TS2068 will do a full-refresh */
        unsigned char *ts2068_colorram;
        unsigned char *ts2068_charram;

        if ((ts2068_port_ff_data & 7) == 1)
                /* Screen 6000-7aff */
                ts2068_charram = ts2068_ram + 0x2000;
        else
                /* Screen 4000-5aff */
                ts2068_charram = ts2068_ram;
        ts2068_colorram = ts2068_charram + 0x01800;

        for (count=0;count<32*8;count++)
        {
                decodechar( Machine->gfx[0],count,ts2068_charram,
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );

                decodechar( Machine->gfx[1],count,&ts2068_charram[0x800],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );

                decodechar( Machine->gfx[2],count,&ts2068_charram[0x1000],
				    Machine->drv->gfxdecodeinfo[0].gfxlayout );
	}

        for (count=0;count<32*8;count++)
        {
		int sx=count%32;
		int sy=count/32;
		unsigned char color;

                color=get_display_color(ts2068_colorram[count],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[0],
				count,
				color,
				0,0,
				sx*8,sy*8,
				0,TRANSPARENCY_NONE,0);

                color=get_display_color(ts2068_colorram[count+0x100],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[1],
				count,
				color,
				0,0,
				sx*8,(sy+8)*8,
				0,TRANSPARENCY_NONE,0);

                color=get_display_color(ts2068_colorram[count+0x200],
                                        flash_invert);
                drawgfx(bitmap,Machine->gfx[2],
				count,
				color,
				0,0,
				sx*8,(sy+16)*8,
				0,TRANSPARENCY_NONE,0);
	}
}
