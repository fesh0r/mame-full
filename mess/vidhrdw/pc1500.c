#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/pc1500.h"

static struct artwork_info *backdrop;

/* palette in red, green, blue tribles */
unsigned char pc1500_palette[242][3] =
{
	{ 107,110,101 },
	{ 56,61,55 }
#if 0
	{ 0, 0, 0 },
	{ 255,255,255 }
#endif
};

/* color table for 1 2 color objects */
unsigned short pc1500_colortable[1][2] = {
	{ 0, 1 },
};

void pc1500_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	char backdrop_name[200];
	int used=2;

	memcpy (sys_palette, pc1500_palette, sizeof (pc1500_palette));
	memcpy(sys_colortable,pc1500_colortable,sizeof(pc1500_colortable));

    /* try to load a backdrop for the machine */
    sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);

    artwork_load (&backdrop, backdrop_name, used, Machine->drv->total_colors - used);

	if (backdrop)
    {
        logerror("backdrop %s successfully loaded\n", backdrop_name);
        memcpy (&sys_palette[used * 3], backdrop->orig_palette, 
				backdrop->num_pens_used * 3 * sizeof (unsigned char));
    }
    else
    {
        logerror("no backdrop loaded\n");
    }
}

int pc1500_vh_start(void)
{
#if 1
	// artwork seams to need this
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)malloc (videoram_size);
	if (!videoram)
        return 1;

    if (backdrop)
        backdrop_refresh (backdrop);

	return video_start_generic();
#else
	return 0;
#endif
}

void pc1500_vh_stop(void)
{
    if (backdrop)
        artwork_free (&backdrop);
}

static const struct {
	int x,y;
} pc1500_pos[15]={
	{0, -9},
	{30, -9},
	{60, -9},
	{80, -9},
	{112,-9}, //de
	{120,-9},
	{125,-9},
	{160,-9}, // run
	{190,-9},
	{210,-9},
	{250,-9},
	{276,-9}, // i
	{282,-9},
	{292,-9},
	{312,-9}
};

static const char
*busy=
"11  1  1  11 1 1\r"
"1 1 1  1 1   1 1\r"
"11  1  1  1  1 1\r"
"1 1 1  1   1  1\r"
"11   11  11   1"
,*shift=
" 11 1 1 1 111 111\r"
"1   1 1 1 1    1\r"
" 1  111 1 111  1\r"
"  1 1 1 1 1    1\r"
"11  1 1 1 1    1"
,*small=
" 11 1   1  1  1   1\r"
"1   11 11 1 1 1   1\r"
" 1  1 1 1 111 1   1\r"
"  1 1   1 1 1 1   1\r"
"11  1   1 1 1 111 111"
,*japanese= // i think 2nd lcd variant with japanese in it 
"  1  1  11\r"
"  1 1 1 1 1\r"
"  1 111 11\r"
"1 1 1 1 1\r"
" 1  1 1 1"
,*de=
"11  111\r"
"1 1 1\r"
"1 1 111\r"
"1 1 1\r"
"11  111"
,*g=
" 111\r"
"1\r"
"1 11\r"
"1  1\r"
" 11"
,*rad=
"11   1  11\r"
"1 1 1 1 1 1\r"
"11  111 1 1\r"
"1 1 1 1 1 1\r"
"1 1 1 1 11"
,* reserve=
"11  111  11 111 11  1 1 111\r"
"1 1 1   1   1   1 1 1 1 1\r"
"11  111  1  111 11  1 1 111\r"
"1 1 1     1 1   1 1 1 1 1\r"
"1 1 111 11  111 1 1  1  111"
,* run= 
"11  1  1 1  1\r"
"1 1 1  1 11 1\r"
"11  1  1 1 11\r"
"1 1 1  1 1  1\r"
"1 1  11  1  1"
,* pro=
"11  11   1\r"
"1 1 1 1 1 1\r"
"11  11  1 1\r"
"1   1 1 1 1\r"
"1   1 1  1"
,*def=
"11  111 111\r"
"1 1 1   1\r"
"1 1 111 111\r"
"1 1 1   1\r"
"11  111 1"
,* romean_i =
"111\r"
" 1\r"
" 1\r"
" 1\r"
"111"
,*romean_ii=
"111 111\r"
" 1   1\r"
" 1   1\r"
" 1   1\r"
"111 111"
,*romean_iii=
"111 111 111\r"
" 1   1   1\r"
" 1   1   1\r"
" 1   1   1\r"
"111 111 111"
,*battery=
" 11\r"
"1111\r"
"1111\r"
" 11"
;

static void pc1500_draw(struct mame_bitmap *bitmap,const char *figure, int color, int x, int y)
{
	INT16 c=Machine->pens[color?1:0];
	int j, xi=0;
	for (j=0; figure[j]; j++) {
		switch (figure[j]) {
		case '1': 
			plot_pixel(bitmap, x+xi, y, c);
			osd_mark_dirty(x+xi,y,x+xi,y);
			xi++;
			break;
		case ' ': 
			xi++;
			break;
		case '\r':
			xi=0;
			y++;
			break;				
		};
	}
}

void pc1500_screenrefresh (struct mame_bitmap *bitmap, int full_refresh, int RIGHT, int DOWN)
{
	int i, x, y;
	UINT8 *mem=memory_region(REGION_CPU1)+0x7600;

    if (backdrop)
        copybitmap (bitmap, backdrop->artwork, 0, 0, 0, 0, NULL, 
					TRANSPARENCY_NONE, 0);
	else
		fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	for (x=RIGHT,y=DOWN,i=0; i<0x4e;x+=2,i+=2) {
		drawgfx(bitmap, Machine->gfx[0], mem[i]&0xf,0,0,0,
				x,y, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[1], mem[i+1]&0xf,0,0,0,
				x,y+8, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[0], mem[i]>>4,0,0,0,
				x+78*2,y, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[1], mem[i+1]>>4,0,0,0,
				x+78*2,y+8, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[0], mem[0x100+i]&0xf,0,0,0,
				x+39*2,y, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[1], mem[0x101+i]&0xf,0,0,0,
				x+39*2,y+8, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[0], mem[0x100+i]>>4,0,0,0,
				x+39*2+78*2,y, 0, TRANSPARENCY_NONE,0);
		drawgfx(bitmap, Machine->gfx[1], mem[0x101+i]>>4,0,0,0,
				x+39*2+78*2,y+8, 0, TRANSPARENCY_NONE,0);
	}


/* busy  shift   small   de g rad   run  pro  reserve  def  i ii iii battery */
/* japanese? */

	pc1500_draw(bitmap, busy,	mem[0x4e]&1, RIGHT+pc1500_pos[0].x, DOWN+pc1500_pos[0].y);
	pc1500_draw(bitmap, shift,	mem[0x4e]&2, RIGHT+pc1500_pos[1].x, DOWN+pc1500_pos[1].y);
	pc1500_draw(bitmap, japanese, mem[0x4e]&4, RIGHT+pc1500_pos[2].x, DOWN+pc1500_pos[2].y);
	pc1500_draw(bitmap, small,	mem[0x4e]&8, RIGHT+pc1500_pos[3].x, DOWN+pc1500_pos[3].y);
	pc1500_draw(bitmap, de,		mem[0x4f]&0x01, RIGHT+pc1500_pos[4].x, DOWN+pc1500_pos[4].y);
	pc1500_draw(bitmap, g,		mem[0x4f]&0x02, RIGHT+pc1500_pos[5].x, DOWN+pc1500_pos[5].y);
	pc1500_draw(bitmap, rad,	mem[0x4f]&0x04, RIGHT+pc1500_pos[6].x, DOWN+pc1500_pos[6].y);
	pc1500_draw(bitmap, run,	mem[0x4f]&0x40, RIGHT+pc1500_pos[7].x, DOWN+pc1500_pos[7].y);
	pc1500_draw(bitmap, pro,	mem[0x4f]&0x20, RIGHT+pc1500_pos[8].x, DOWN+pc1500_pos[8].y);
	pc1500_draw(bitmap, reserve, mem[0x4f]&0x10, RIGHT+pc1500_pos[9].x, DOWN+pc1500_pos[9].y);
	pc1500_draw(bitmap, def,	mem[0x4e]&0x80, RIGHT+pc1500_pos[10].x, DOWN+pc1500_pos[10].y);
	pc1500_draw(bitmap, romean_i, mem[0x4e]&0x40, RIGHT+pc1500_pos[11].x, DOWN+pc1500_pos[11].y);
	pc1500_draw(bitmap, romean_ii, mem[0x4e]&0x20, RIGHT+pc1500_pos[12].x, DOWN+pc1500_pos[12].y);
	pc1500_draw(bitmap, romean_iii,mem[0x4e]&0x10, RIGHT+pc1500_pos[13].x, DOWN+pc1500_pos[13].y);
	pc1500_draw(bitmap, battery, 1, RIGHT+pc1500_pos[14].x, DOWN+pc1500_pos[14].y);
}

void pc1500_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
	pc1500_screenrefresh(bitmap, full_refresh, 86, 51);
}

void pc1500a_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
	pc1500_screenrefresh(bitmap, full_refresh, 85, 52);
}

void trs80pc2_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
	pc1500_screenrefresh(bitmap, full_refresh, 161, 52);
}
