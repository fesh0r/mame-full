/***************************************************************************
  Note - pc_scanblit is screwed - highly commented to compile

	vidhrdw/pc.c

	Functions to emulate the video hardware of the PC/XT.
	MDA (monochrome display adapter)
	CGA (color graphics adapter)
	HGC (hercules graphics card(?))

	TODO:
	Maybe support the 8x9 scanline mode of the Tandy 1000 / PCjr
	Right now there's a hack to keep it 8x8 to avoid visible are
	changes. BTW I have no 8x9 font :(

***************************************************************************/
#include "mess/machine/pc.h"


int pc_blink = 0;
int pc_fill_odd_scanlines = 0;

int DOCLIP(struct rectangle *r1, const struct rectangle *r2)
{
    if (r1->min_x > r2->max_x) return 0;
    if (r1->max_x < r2->min_x) return 0;
    if (r1->min_y > r2->max_y) return 0;
    if (r1->max_y < r2->min_y) return 0;
    if (r1->min_x < r2->min_x) r1->min_x = r2->min_x; 
    if (r1->max_x > r2->max_x) r1->max_x = r2->max_x;
    if (r1->min_y < r2->min_y) r1->min_y = r2->min_y;
    if (r1->max_y > r2->max_y) r1->max_y = r2->max_y;
	return 1;
}

void pc_fullblit(struct osd_bitmap *dest,const struct GfxElement *gfx,
        unsigned int code,unsigned int color,int sx,int sy,const struct rectangle *clip)
{
    int ox,oy,ex,ey,y,start;
    const unsigned char *sd;
	unsigned char *bm,*bme;
	const unsigned short *paldata;	/* ASG 980209 */

    if (!gfx) return;

    code %= gfx->total_elements;
    color %= gfx->total_colors;

    /* check bounds */
    ox = sx;
    oy = sy;
    ex = sx + gfx->width-1;
    if (sx < 0) sx = 0;
    if (clip && sx < clip->min_x) sx = clip->min_x;
    if (ex >= dest->width) ex = dest->width-1;
    if (clip && ex > clip->max_x) ex = clip->max_x;
    if (sx > ex) return;
    ey = sy + gfx->height-1;
    if (sy < 0) sy = 0;
    if (clip && sy < clip->min_y) sy = clip->min_y;
    if (ey >= dest->height) ey = dest->height-1;
    if (clip && ey > clip->max_y) ey = clip->max_y;
    if (sy > ey) return;

    osd_mark_dirty (sx,sy,ex,ey,0); /* ASG 971011 */

    start = code * gfx->height + (sy-oy);


	paldata = &gfx->colortable[gfx->color_granularity * color];

	for (y = sy; y <= ey; y++)
	{
		bm	= dest->line[y];
		bme = bm + ex;
		sd = gfx->gfxdata + start * gfx->width + (sx-ox);
		for( bm += sx; bm <= bme-7 ; bm += 8 )
		{
			bm[0] = paldata[sd[0]];
			bm[1] = paldata[sd[1]];
			bm[2] = paldata[sd[2]];
			bm[3] = paldata[sd[3]];
			bm[4] = paldata[sd[4]];
			bm[5] = paldata[sd[5]];
			bm[6] = paldata[sd[6]];
			bm[7] = paldata[sd[7]];
			sd+=8;
		}
		for( ; bm <= bme ; bm++ )
			*bm = paldata[*(sd++)];
		start++;
	}
}


void pc_scanblit(struct osd_bitmap *dest,const struct GfxElement *gfx,
        unsigned int code,unsigned int color,int sx,int sy,const struct rectangle *clip)
{
    int ox,oy,ex,ey,y,start;
    const unsigned char *sd;
	unsigned char *bm0, *bm1, *bme;
	const unsigned short *paldata;	/* ASG 980209 */

    if (!gfx) return;

    code %= gfx->total_elements;
    color %= gfx->total_colors;

    /* check bounds */
    ox = sx;
    oy = sy;
    ex = sx + gfx->width-1;
    if (sx < 0) sx = 0;
    if (clip && sx < clip->min_x) sx = clip->min_x;
    if (ex >= dest->width) ex = dest->width-1;
    if (clip && ex > clip->max_x) ex = clip->max_x;
    if (sx > ex) return;
    ey = sy + gfx->height-1;
    if (sy < 0) sy = 0;
    if (clip && sy < clip->min_y) sy = clip->min_y;
    if (ey >= dest->height) ey = dest->height-1;
    if (clip && ey > clip->max_y) ey = clip->max_y;
    if (sy > ey) return;

    osd_mark_dirty (sx,sy,ex,ey,0); /* ASG 971011 */

    start = code * gfx->height + (sy-oy);


	paldata = &gfx->colortable[gfx->color_granularity * color];

	if (pc_fill_odd_scanlines)
	{
		for (y = sy; y <= ey; y += 2)
		{
			bm0  = dest->line[y];
			bm1  = dest->line[y+1];
			bme = bm0 + ex;
			sd = gfx->gfxdata + start * gfx->width + (sx-ox);
			for( bm0 += sx, bm1 += sx ; bm0 <= bme-7 ; bm0 += 8, bm1 += 8 )
			{
				bm0[0] = bm1[0] = paldata[sd[0]];
				bm0[1] = bm1[1] = paldata[sd[1]];
				bm0[2] = bm1[2] = paldata[sd[2]];
				bm0[3] = bm1[3] = paldata[sd[3]];
				bm0[4] = bm1[4] = paldata[sd[4]];
				bm0[5] = bm1[5] = paldata[sd[5]];
				bm0[6] = bm1[6] = paldata[sd[6]];
				bm0[7] = bm1[7] = paldata[sd[7]];
				sd+=8;
			}
			for( ; bm0 <= bme ; bm0++, bm1++ )
				*bm0 = *bm1 = paldata[*(sd++)];
            start++;
        }
    } else {
		for (y = sy; y <= ey; y += 2)
		{
			bm0  = dest->line[y];
			bme = bm0 + ex;
			sd = gfx->gfxdata + start * gfx->width + (sx-ox);
			for( bm0 += sx ; bm0 <= bme-7 ; bm0+=8 )
			{
				bm0[0] = paldata[sd[0]];
				bm0[1] = paldata[sd[1]];
				bm0[2] = paldata[sd[2]];
				bm0[3] = paldata[sd[3]];
				bm0[4] = paldata[sd[4]];
				bm0[5] = paldata[sd[5]];
				bm0[6] = paldata[sd[6]];
				bm0[7] = paldata[sd[7]];
				sd+=8;
			}
			for( ; bm0 <= bme ; bm0++ )
				*bm0 = paldata[*(sd++)];
			start++;
		}
	}
}
