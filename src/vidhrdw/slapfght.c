/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *slapfight_videoram;
unsigned char *slapfight_colorram;
size_t slapfight_videoram_size;
unsigned char *slapfight_scrollx_lo,*slapfight_scrollx_hi,*slapfight_scrolly;
static int flipscreen;

static struct tilemap *pf1_tilemap,*fix_tilemap;


WRITE_HANDLER( slapfight_flipscreen_w )
{
	if (offset==0) flipscreen=1; /* Port 0x2 is flipscreen */
	else flipscreen=0; /* Port 0x3 is normal */
}

WRITE_HANDLER( slapfight_fixram_w )
{
	slapfight_videoram[offset]=data;
	tilemap_mark_tile_dirty(fix_tilemap,offset);
}

WRITE_HANDLER( slapfight_fixcol_w )
{
	slapfight_colorram[offset]=data;	
	tilemap_mark_tile_dirty(fix_tilemap,offset);
}

WRITE_HANDLER( slapfight_videoram_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE_HANDLER( slapfight_colorram_w )
{
	colorram[offset]=data;
	tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

static void get_pf1_tile_info(int tile_index)
{
	int tile,color;

	tile=videoram[tile_index] + ((colorram[tile_index] & 0x0f) << 8);
	color=(colorram[tile_index] & 0xf0) >> 4;

	SET_TILE_INFO(1,tile,color)
}

static void get_fix_tile_info(int tile_index)
{
	int tile,color;

	tile=slapfight_videoram[tile_index] + ((slapfight_colorram[tile_index] & 0x03) << 8);
	color=(slapfight_colorram[tile_index] & 0xfc) >> 2;

	SET_TILE_INFO(0,tile,color)
}

int slapfight_vh_start (void)
{
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	fix_tilemap = tilemap_create(get_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!pf1_tilemap || !fix_tilemap)
		return 1;

	fix_tilemap->transparent_pen=0;

	return 0;
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Slapfight has three 256x4 palette PROMs (one per gun) all colours for all
  outputs are mapped to the palette directly.

  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void slapfight_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void slapfight_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	if (flipscreen) {
		tilemap_set_scrollx( fix_tilemap,0,296);
		tilemap_set_scrollx( pf1_tilemap,0,(*slapfight_scrollx_lo + 256 * *slapfight_scrollx_hi)+296 );
		tilemap_set_scrolly( pf1_tilemap,0, (*slapfight_scrolly)+15 );
		tilemap_set_scrolly( fix_tilemap,0, -1 ); /* Glitch in Tiger Heli otherwise */
	}
	else {
		tilemap_set_scrollx( fix_tilemap,0,0);
		tilemap_set_scrollx( pf1_tilemap,0,(*slapfight_scrollx_lo + 256 * *slapfight_scrollx_hi) );
		tilemap_set_scrolly( pf1_tilemap,0, (*slapfight_scrolly)-1 );
		tilemap_set_scrolly( fix_tilemap,0, -1 ); /* Glitch in Tiger Heli otherwise */
	}

	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);
	tilemap_draw(bitmap,pf1_tilemap,0);

	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (flipscreen)
			drawgfx(bitmap,Machine->gfx[2],
				buffered_spriteram[offs] + ((buffered_spriteram[offs+2] & 0xc0) << 2),
				(buffered_spriteram[offs+2] & 0x1e) >>1,
				1,1,
				288-(buffered_spriteram[offs+1] + ((buffered_spriteram[offs+2] & 0x01) << 8)) +18,240-buffered_spriteram[offs+3],
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		else
			drawgfx(bitmap,Machine->gfx[2],
				buffered_spriteram[offs] + ((buffered_spriteram[offs+2] & 0xc0) << 2),
				(buffered_spriteram[offs+2] & 0x1e) >>1,
				0,0,
				(buffered_spriteram[offs+1] + ((buffered_spriteram[offs+2] & 0x01) << 8)) - 13,buffered_spriteram[offs+3],
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

	tilemap_draw(bitmap,fix_tilemap,0);
}
