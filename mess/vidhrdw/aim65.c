/******************************************************************************

	Peter.Trauner@jk.uni-linz.ac.at Nov 2000

******************************************************************************/
#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/aim65.h"

static struct artwork_info *aim65_backdrop;

typedef struct {
	int digit[4]; // 16 segment digit, decoded from 7 bit data!
	int cursor_on[4];
} DL1416A;

static DL1416A dl1416a[5]= { { {0} } };

void dl1416a_write(int chip, int digit, int value, int cursor)
{
	digit^=3;
	if (cursor) {
		dl1416a[chip].cursor_on[0]=value&1;
		dl1416a[chip].cursor_on[1]=value&2;
		dl1416a[chip].cursor_on[2]=value&4;
		dl1416a[chip].cursor_on[3]=value&8;
	} else {
		dl1416a[chip].digit[digit]=value;
	}
	logerror("dl1416a:%d digit:%d value:%.2x cursor:%d\n",chip,digit,value,cursor);
}

// aim 65 users guide table 7.6
static const int dl1416a_segments[0x80]={ // witch segments must be turned on for this value!?
	0x0000, 0x0000, 0x0000, 0x0000, // 0x00 // not known
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, // 0x10
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1121, 0x0180, 0x0f3c, // 0x20   ! " # 
	0x0fbb, 0xaf99, 0x5979, 0x2000, //      $ % & '
	0x6000, 0x9000, 0xff00, 0x0f00, //      ( ) * +
	0x8000, 0x0a00, 0x0020, 0xa000, //      , - . /
	0x05e1, 0x0500, 0x0961, 0x0d21, // 0x30 0 1 2 3
	0x0d80, 0x0ca1, 0x0ce1, 0x0501, //      4 5 6 7
	0x0de1, 0x0da1, 0x0021, 0x8001, //      8 9 : ;
	0xa030, 0x0a30, 0x5030, 0x0607, //      < = > ?
	0x0c7f, 0x0acf, 0x073f, 0x00f3, // 0x40 @ A B C
	0x053f, 0x08f3, 0x08c3, 0x02fb, //      D E F G
	0x0acc, 0x0533, 0x0563, 0x68c0, //      H I J K
	0x00f0, 0x30cc, 0x50cc, 0x00ff, //      L M N O
	0x0ac7, 0x40ff, 0x4ac7, 0x0abb, // 0x50 P Q R S
	0x0503, 0x00fc, 0xa0c0, 0xc0cc, //      T U V W
	0xf000, 0x3400, 0xa033, 0x00e1, //      X Y Z [
	0x5000, 0x001e, 0xc000, 0x0030, //      \ ] ^ _
	0x0000, 0x0000, 0x0000, 0x0000, // 0x60
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, // 0x70
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000
};

unsigned char aim65_palette[242][3] =
{
  	{ 0x20,0x02,0x05 },
	{ 0xc0, 0, 0 },
};

void aim65_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	char backdrop_name[200];
    int nextfree;

    /* try to load a backdrop for the machine */
    sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);

	memcpy (palette, aim65_palette, sizeof (aim65_palette));

    nextfree = 2;

    artwork_load (&aim65_backdrop, backdrop_name, nextfree, Machine->drv->total_colors - nextfree);
	if (aim65_backdrop)
    {
        logerror("backdrop %s successfully loaded\n", backdrop_name);
        memcpy (&palette[nextfree * 3], aim65_backdrop->orig_palette, aim65_backdrop->num_pens_used * 3 * sizeof (unsigned char));
    }
    else
    {
        logerror("no backdrop loaded\n");
    }

}

int aim65_vh_start (void)
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)malloc (videoram_size);
	if (!videoram)
        return 1;
    if (aim65_backdrop)
        backdrop_refresh (aim65_backdrop);
    if (generic_vh_start () != 0)
        return 1;

    return 0;
}

void aim65_vh_stop (void)
{
    if (aim65_backdrop)
        artwork_free (&aim65_backdrop);
    aim65_backdrop = NULL;
    if (videoram)
        free (videoram);
    videoram = NULL;
    generic_vh_stop ();
}

static const char led[] = {
	"      aaaaaaa    bbbbbb\r" 
	"      aaaaaaa    bbbbbb\r"
	"   hh         ii        cc\r"
	"   hh mm      ii     nn cc\r"
	"   hh  mm     ii    nn  cc\r" 
	"   hh   mm    ii   nn   cc\r" 
	"  hh    mm   ii   nn   cc\r" 
	"  hh     mm  ii  nn    cc\r" 
	"  hh      mm ii nn     cc\r" 
	"  hh         ii        cc\r" 
	"     lllllll    jjjjjj\r" 
	"    lllllll    jjjjjj\r" 
	" gg         kk        dd\r" 
	" gg      pp kk oo     dd\r" 
	" gg     pp  kk  oo    dd\r" 
	" gg    pp   kk  oo    dd\r" 
	"gg   pp    kk    oo  dd\r" 
	"gg  pp     kk    oo  dd\r" 
	"gg pp      kk     oo dd\r" 
	"gg         kk        dd\r" 
	"   fffffff    eeeeee\r" 
	"   fffffff    eeeeee" 
};

static void aim65_draw_7segment(struct osd_bitmap *bitmap,int value, int x, int y)
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
		case 'i': mask=0x100; break;
		case 'j': mask=0x200; break;

		case 'k': mask=0x400; break;
		case 'l': mask=0x800; break;
		case 'm': mask=0x1000; break;
		case 'n': mask=0x2000; break;
		case 'o': mask=0x4000; break;
		case 'p': mask=0x8000; break;
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
} aim65_led_pos[20]={
	{0,0},
	{30,0},
	{60,0},
	{90,0},
	{120,0},
	{150,0},
	{180,0},
	{210,0},
	{240,0},
	{270,0},

	{300,0},
	{330,0},
	{360,0},
	{390,0},
	{420,0},
	{450,0},
	{480,0},
	{510,0},
	{540,0},
	{570,0}
};

#if 0
static const char* single_led=
" 111\r"
"11111\r"
"11111\r"
"11111\r"
" 111"
;

static void aim65_draw_led(struct osd_bitmap *bitmap,INT16 color, int x, int y)
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
#endif

void aim65_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i, j;

    if (full_refresh)
    {
        osd_mark_dirty (0, 0, bitmap->width, bitmap->height);
    }
    if (aim65_backdrop)
        copybitmap (bitmap, aim65_backdrop->artwork, 0, 0, 0, 0, NULL, 
					TRANSPARENCY_NONE, 0);
	else
		fillbitmap (bitmap, Machine->pens[2], &Machine->visible_area);

	for (j=0; j<5; j++) {
		for (i=0; i<4; i++) {
			if (dl1416a[j].cursor_on[i]) {
				aim65_draw_7segment(bitmap, 0xffff,
									aim65_led_pos[j*4+i].x, aim65_led_pos[j*4+i].y);
			} else {
				aim65_draw_7segment(bitmap, dl1416a_segments[dl1416a[j].digit[i]],
									aim65_led_pos[j*4+i].x, aim65_led_pos[j*4+i].y);
			}
		}
	}

#if 0
	aim65_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[6].x, sym1_led_pos[6].y);
	aim65_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[7].x, sym1_led_pos[7].y);
#endif
}

