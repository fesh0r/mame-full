/*
 *	Beatmania DJ Main Board (GX753)
 *	emulate video hardware
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


data32_t *djmain_obj_ram;

#define NUM_SPRITES	(0x800 / 16)


struct sprite_data
{
	int priority;
	int code;
	int x;
	int y;
	int width;
	int height;
	int color;
	int flipx;
	int flipy;
	int xscale;
	int yscale;
};


static int compare_sprite_priority(const void *p1, const void *p2)
{
	const struct sprite_data *spr1 = p1;
	const struct sprite_data *spr2 = p2;

	return spr2->priority - spr1->priority;
}

static void djmain_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	struct sprite_data spr_buf[NUM_SPRITES];
	struct sprite_data *spr;
	int num_sprites = 0;
	int i;

	for (i = 0, spr = spr_buf; i < NUM_SPRITES; i++)
	{
		static int width[4] =  { 1, 2, 4, 8 };
		static int height[4] = { 1, 2, 4, 8 };
		data32_t *data = &djmain_obj_ram[i*4];
		int size;

		if (!(data[0] & 0x00008000))
			continue;

		spr->code = data[0] >> 16;
		if (spr->code & 0x8000)
			continue;

		spr->flipx = (data[0] >> 10) & 1;
		spr->flipy = (data[0] >> 11) & 1;
		size = (data[0] >> 8) & 3;
		spr->priority = ((data[0] & 0xff) << 16) | i;
		spr->width = width[size];
		spr->height = height[size];

		spr->x = (INT16)(data[1] & 0xffff);
		spr->y = (INT16)(data[1] >> 16);

		spr->xscale = (data[2] >> 16) & 0xff;
		spr->yscale = data[2] & 0xff;

		if (!spr->xscale || !spr->yscale)
			continue;

		spr->xscale = (0x40 << 16) / spr->xscale;
		spr->yscale = (0x40 << 16) / spr->yscale;
		spr->x -= (spr->width * spr->xscale) >> 13;
		spr->y -= (spr->height * spr->yscale) >> 13;

		spr->color = (data[3] >> 16) & 15;

		spr++;
		num_sprites++;
	}

	qsort(spr_buf, num_sprites, sizeof spr_buf[0], compare_sprite_priority);

	for (i = 0, spr = spr_buf; i < num_sprites; i++, spr++)
	{
		static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
		static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
		int x, y;

		for (x = 0; x < spr->width; x++)
			for (y = 0; y < spr->height; y++)
			{
				int c = spr->code;

				if (spr->flipx)
					c += xoffset[spr->width - x - 1];
				else
					c += xoffset[x];

				if (spr->flipy)
					c += yoffset[spr->height - y - 1];
				else
					c += yoffset[y];

				if (spr->xscale != 0x10000 || spr->yscale != 0x10000)
				{
					int sx = spr->x + ((x * spr->xscale + (1 << 11)) >> 12);
					int sy = spr->y + ((y * spr->yscale + (1 << 11)) >> 12);
					int zw =spr->x + (((x + 1) * spr->xscale + (1 << 11)) >> 12) - sx;
					int zh =spr->y + (((y + 1) * spr->yscale + (1 << 11)) >> 12) - sy;

					drawgfxzoom(bitmap,
					            Machine->gfx[0],
					            c,
					            spr->color,
					            spr->flipx,
					            spr->flipy,
					            sx,
					            sy,
					            cliprect,
					            TRANSPARENCY_PEN,
					            0,
					            (zw << 16) / 16,
					            (zh << 16) / 16);
				}
				else
				{
					int sx = spr->x + x * 16;
					int sy = spr->y + y * 16;

					drawgfx(bitmap,
					        Machine->gfx[0],
					        c,
					        spr->color,
					        spr->flipx,
					        spr->flipy,
					        sx,
					        sy,
					        cliprect,
					        TRANSPARENCY_PEN,
					        0);
				}
			}
	}
}

static void game_tile_callback(int layer, int *code, int *color)
{
}

VIDEO_START( djmain )
{
	static int scrolld[2][4][2] = {
	 	{{ 0, 0}, {0, 0}, {0, 0}, {0, 0}},
	 	{{ 0, 0}, {0, 0}, {0, 0}, {0, 0}}
	};

	if (K056832_vh_start(REGION_GFX2, K056832_BPP_4dj, 1, scrolld, game_tile_callback))
		return 1;

	K056832_set_LayerOffset(0, -92, -27);
	K056832_set_LayerOffset(1, -87, -27);

	K055555_vh_start();

	return 0;
}

VIDEO_UPDATE( djmain )
{
	int enables = K055555_read_register(K55_INPUT_ENABLES);
	int pri[3];
	int order[3];
	int i, j;

	pri[0] = K055555_read_register(K55_PRIINP_0);
	pri[1] = K055555_read_register(K55_PRIINP_3);
	pri[2] = K055555_read_register(K55_PRIINP_10);

	order[0] = 0;
	order[1] = 1;
	order[2] = 2;

	for (i = 0; i < 2; i++)
		for (j = i + 1; j < 3; j++)
			if (pri[order[i]] > pri[order[j]])
			{
				int temp = order[i];

				order[i] = order[j];
				order[j] = temp;
			}

	fillbitmap(bitmap, get_black_pen(), cliprect);

	for (i = 0; i < 3; i++)
		switch (order[i])
		{
		case 0:
			if (enables & K55_INP_VRAM_A)
				K056832_tilemap_draw_dj(bitmap, cliprect, 0, 0, 1);
			break;
		case 1:
			if (enables & K55_INP_VRAM_B)
				K056832_tilemap_draw_dj(bitmap, cliprect, 1, 0, 4);
			break;
		default:
			if (enables & K55_INP_SUB2)
				djmain_draw_sprites(bitmap, cliprect);
		}

}
