#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/mk2.h"

UINT8 mk2_led[5]= {0};

static struct artwork_info *backdrop;

unsigned char mk2_palette[242][3] =
{
	{ 0x20,0x02,0x05 },
	{ 0xc0, 0, 0 },
};

unsigned short mk2_colortable[1][2] = {
	{ 0, 1 },
};

void mk2_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	char backdrop_name[200];
	int used=2;

	memcpy (sys_palette, mk2_palette, sizeof (mk2_palette));
	memcpy(sys_colortable,mk2_colortable,sizeof(mk2_colortable));

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

int mk2_vh_start(void)
{
#if 1
	// artwork seams to need this
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)malloc (videoram_size);
	if (!videoram)
        return 1;

    if (backdrop)
        backdrop_refresh (backdrop);

	return generic_vh_start();
#else
	return 0;
#endif
}

void mk2_vh_stop(void)
{
    if (backdrop)
        artwork_free (&backdrop);
#if 1
	generic_vh_stop();
#endif
}

static const char led[]={
	"      aaaaaaaaaaaaaaa\r"
	"     f aaaaaaaaaaaaa b\r"
	"     ff aaaaaaaaaaa bb\r"
	"     fff           bbb\r"
	"     fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"   fff           bbb\r"
	"   fff           bbb\r"
	"   ff             bb\r"
    "   f ggggggggggggg b\r"
    "    gggggggggggggg\r"
	"  e ggggggggggggg c\r"
	"  ee             cc\r"
	"  eee           ccc\r"
	"  eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	"eee           ccc\r"
	"eee           ccc\r"
	"ee ddddddddddd cc   hh\r"
    "e ddddddddddddd c  hhh\r"
    " ddddddddddddddd   hh"
};

static void mk2_draw_7segment(struct osd_bitmap *bitmap,int value, int x, int y)
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
		case 'h': 
			// this is more likely wired to the separate leds
			mask=0x80; 
			break;
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
} mk2_led_pos[8]={
	{70,96},
	{99,96},
	{128,96},
	{157,96},
	{47,223},
	{85,223},
	{123,223},
	{162,223}
};

static const char* single_led=
" 111\r"
"11111\r"
"11111\r"
"11111\r"
" 111"
;

static void mk2_draw_led(struct osd_bitmap *bitmap,INT16 color, int x, int y)
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

void mk2_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;

    if (backdrop)
        copybitmap (bitmap, backdrop->artwork, 0, 0, 0, 0, NULL, 
					TRANSPARENCY_NONE, 0);
	else
		fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	for (i=0; i<4; i++) {
		mk2_draw_7segment(bitmap, mk2_led[i]&0x7f, mk2_led_pos[i].x, mk2_led_pos[i].y);
	}

	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&8?1:0], 
				 mk2_led_pos[4].x, mk2_led_pos[4].y);
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x20?1:0], 
				 mk2_led_pos[5].x, mk2_led_pos[5].y); //?
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x10?1:0], 
				 mk2_led_pos[6].x, mk2_led_pos[6].y);
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x10?0:1], 
				 mk2_led_pos[7].x, mk2_led_pos[7].y);

	mk2_led[0]= mk2_led[1]= mk2_led[2]= mk2_led[3]= mk2_led[4]= 0;
}
