/******************************************************************************
	SYM-1

	Peter.Trauner@jk.uni-linz.ac.at May 2000

******************************************************************************/
#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/sym1.h"

static struct artwork_info *sym1_backdrop;
UINT8 sym1_led[6]= {0};

unsigned char sym1_palette[242][3] =
{
  	{ 0x20,0x02,0x05 },
	{ 0xc0, 0, 0 },
};

void sym1_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	char backdrop_name[200];
    int nextfree;

    /* try to load a backdrop for the machine */
    sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);

	memcpy (palette, sym1_palette, sizeof (sym1_palette));

    nextfree = 2;

    artwork_load (&sym1_backdrop, backdrop_name, nextfree, Machine->drv->total_colors - nextfree);
	if (sym1_backdrop)
    {
        logerror("backdrop %s successfully loaded\n", backdrop_name);
        memcpy (&palette[nextfree * 3], sym1_backdrop->orig_palette, sym1_backdrop->num_pens_used * 3 * sizeof (unsigned char));
    }
    else
    {
        logerror("no backdrop loaded\n");
    }

}

int sym1_vh_start (void)
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)malloc (videoram_size);
	if (!videoram)
        return 1;
    if (sym1_backdrop)
        backdrop_refresh (sym1_backdrop);
    if (generic_vh_start () != 0)
        return 1;

    return 0;
}

void sym1_vh_stop (void)
{
    if (sym1_backdrop)
        artwork_free (&sym1_backdrop);
    sym1_backdrop = NULL;
    if (videoram)
        free (videoram);
    videoram = NULL;
    generic_vh_stop ();
}

static const char led[] =
	"          aaaaaaaaa\r" 
	"       ff aaaaaaaaa bb\r"
	"       ff           bb\r"
	"       ff           bb\r"
	"       ff           bb\r" 
	"       ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"         gggggggg\r" 
	"     ee  gggggggg cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee ddddddddd cc\r" 
	"ii     ddddddddd      hh\r"
    "ii                    hh";

static void sym1_draw_7segment(struct osd_bitmap *bitmap,int value, int x, int y)
{
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++) {
		mask=0;
		switch (led[i]) {
		case 'a': mask=1; break;
		case 'b': mask=2; break;
		case 'c': mask=4; break;
		case 'd': mask=8; break;
		case 'e': mask=0x10; break;
		case 'f': mask=0x20; break;
		case 'g': mask=0x40; break;
		case 'h': mask=0x80; break;
		}
		
		if (mask!=0) {
			color=Machine->pens[(value&mask)?1:0];
			plot_pixel(bitmap, x+xi, y+yi, color);
			osd_mark_dirty(x+xi,y+yi,x+xi,y+yi);
		}
		if (led[i]!='\r') xi++;
		else { yi++, xi=0; }
	}
}

static const struct {
	int x,y;
} sym1_led_pos[8]={
	{594,262},
	{624,262},
	{653,262},
	{682,262},
	{711,262},
	{741,262},
	{80,228},
	{360,32}
};

static const char* single_led=
" 111\r"
"11111\r"
"11111\r"
"11111\r"
" 111"
;

static void sym1_draw_led(struct osd_bitmap *bitmap,INT16 color, int x, int y)
{
	int j, xi=0;
	for (j=0; single_led[j]; j++) {
		switch (single_led[j]) {
		case '1': 
			plot_pixel(bitmap, x+xi, y, color);
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

void sym1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;

    if (full_refresh)
    {
        osd_mark_dirty (0, 0, bitmap->width, bitmap->height);
    }
    if (sym1_backdrop)
        copybitmap (bitmap, sym1_backdrop->artwork, 0, 0, 0, 0, NULL, 
					TRANSPARENCY_NONE, 0);
	else
		fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	for (i=0; i<6; i++) {
		sym1_draw_7segment(bitmap, sym1_led[i], sym1_led_pos[i].x, sym1_led_pos[i].y);
//		sym1_draw_7segment(bitmap, sym1_led[i], sym1_led_pos[i].x-160, sym1_led_pos[i].y-120);
		sym1_led[i]=0;
	}

	sym1_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[6].x, sym1_led_pos[6].y);
	sym1_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[7].x, sym1_led_pos[7].y);
}

