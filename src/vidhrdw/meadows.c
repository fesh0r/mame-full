/***************************************************************************
    meadows.c
	Video handler
	Dead Eye, Gypsy Juggler

	J. Buchmueller, June '98
****************************************************************************/

#include "vidhrdw/generic.h"
#include "artwork.h"

/* some constants to make life easier */
#define SCR_HORZ        32
#define SCR_VERT        30
#define CHR_HORZ        8
#define CHR_VERT        8
#define SPR_COUNT       4
#define SPR_HORZ        16
#define SPR_VERT        16
#define SPR_ADJUST_X    -18
#define SPR_ADJUST_Y    -14

static  int sprite_horz[SPR_COUNT];     /* x position */
static  int sprite_vert[SPR_COUNT];     /* y position */
static  int sprite_index[SPR_COUNT];    /* index 0x00..0x0f, prom 0x10, flip horz 0x20 */


/*************************************************************/
/* draw sprites		                                         */
/*************************************************************/
static void meadows_draw_sprites(struct mame_bitmap *bitmap)
{
	int 	i;
	for( i = 0; i < SPR_COUNT; i++ ) {
		int x, y, n, p, f;
		x = sprite_horz[i];
		y = sprite_vert[i];
		n = sprite_index[i] & 0x0f; 		/* bit #0 .. #3 select sprite */
//		p = (sprite_index[i] >> 4) & 1; 	/* bit #4 selects prom ??? */
		p = i;								/* that fixes it for now :-/ */
		f = sprite_index[i] >> 5;			/* bit #5 flip vertical flag */
		drawgfx(bitmap, Machine->gfx[p + 1],
			n, 0, f, 0, x, y,
			&Machine->visible_area,
			TRANSPARENCY_PEN,0);
	}
}


/*************************************************************/
/* Screen refresh											 */
/*************************************************************/
VIDEO_UPDATE( meadows )
{
	int 	i;

	if (get_vh_global_attribute_changed())
		memset(dirtybuffer,1,SCR_VERT * SCR_HORZ);

    /* the first two rows are invisible */
	for (i = 0; i < SCR_VERT * SCR_HORZ; i++)
	{
		if (dirtybuffer[i])
		{
			int x, y;
            dirtybuffer[i] = 0;

			x = (i % SCR_HORZ) * CHR_HORZ;
			y = (i / SCR_HORZ) * CHR_VERT;

			drawgfx(tmpbitmap, Machine->gfx[0],
				videoram[i] & 0x7f, 0, 0,0, x, y,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* now draw the sprites */
	meadows_draw_sprites(bitmap);
}

/*************************************************************/
/*                                                           */
/* Video RAM write                                           */
/*                                                           */
/*************************************************************/
WRITE_HANDLER( meadows_videoram_w )
{
    if (offset >= videoram_size)
        return;
	if (videoram[offset] == data)
		return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}


/*************************************************************/
/* write to the sprite registers                             */
/*************************************************************/
WRITE_HANDLER( meadows_sprite_w )
{
int     n = offset % SPR_COUNT;
	switch (offset)
	{
		case 0: /* sprite 0 horz */
		case 1: /* sprite 1 horz */
		case 2: /* sprite 2 horz */
		case 3: /* sprite 3 horz */
			sprite_horz[n] = data + SPR_ADJUST_X;
			break;
		case 4: /* sprite 0 vert */
		case 5: /* sprite 1 vert */
		case 6: /* sprite 2 vert */
		case 7: /* sprite 3 vert */
			sprite_vert[n] = data + SPR_ADJUST_Y;
			break;
		case  8: /* prom 1 select + reverse shifter */
		case  9: /* prom 2 select + reverse shifter */
		case 10: /* ??? prom 3 select + reverse shifter */
		case 11: /* ??? prom 4 select + reverse shifter */
			sprite_index[n] = data;
			break;
	}
}



