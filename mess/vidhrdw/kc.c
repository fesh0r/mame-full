/***************************************************************************

  kc.c

  Functions to emulate the video hardware of the kc85/4,kc85/3

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/kc.h"

/* KC85/4 and KC85/3 common graphics hardware */

static unsigned short kc85_colour_table[KC85_PALETTE_SIZE] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23
};

static unsigned char kc85_palette[KC85_PALETTE_SIZE * 3] =
{
		 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xd0,
		 0xd0, 0x00, 0x00,
		 0xd0, 0x00, 0xd0,
		 0x00, 0xd0, 0x00,
		 0x00, 0xd0, 0xd0,
		 0xd0, 0x00, 0x00,
		 0xd0, 0xd0, 0xd0,

		 0x00, 0x00, 0x00,
		 0x60, 0x00, 0xa0,
		 0xa0, 0x60, 0x00,
		 0xa0, 0x00, 0x60,
		 0x00, 0xa0, 0x60,
		 0x00, 0x60, 0xa0,
		 0x30, 0xa0, 0x30,
		 0xd0, 0xd0, 0xd0,

		 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xa0,
		 0xa0, 0x00, 0x00,
		 0xa0, 0x00, 0xa0,
		 0x00, 0xa0, 0x00,
		 0x00, 0xa0, 0xa0,
		 0xa0, 0xa0, 0x00,
		 0xa0, 0xa0, 0xa0

};


/* Initialise the palette */
void kc85_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy(sys_palette, kc85_palette, sizeof (kc85_palette));
	memcpy(sys_colortable, kc85_colour_table, sizeof (kc85_colour_table));
}


static void kc85_draw_8_pixels(struct osd_bitmap *bitmap,int x,int y, unsigned char colour_byte, unsigned char gfx_byte)
{
	int a;
	int background_pen;
	int foreground_pen;
	
    background_pen = (colour_byte&7) | 0x010;
    foreground_pen = (colour_byte>>3) & 0x015;

    background_pen = Machine->pens[background_pen];
    foreground_pen = Machine->pens[foreground_pen];

    for (a=0; a<8; a++)
    {
        int pen;

        if (gfx_byte & 0x080)
        {
            pen = foreground_pen;
        }
        else
        {
            pen = background_pen;
        }

        plot_pixel(bitmap, (x<<3)+a, y, pen);

        gfx_byte = gfx_byte<<1;
	}
}

/***************************************************************************
 KC85/4 video hardware
***************************************************************************/

static unsigned char *kc85_4_display_video_ram;

static unsigned char *kc85_4_video_ram;

int kc85_4_vh_start(void)
{
    kc85_4_video_ram = malloc(
        (KC85_4_SCREEN_COLOUR_RAM_SIZE*2) +
        (KC85_4_SCREEN_PIXEL_RAM_SIZE*2));

	kc85_4_display_video_ram = kc85_4_video_ram;

	return 0;
}

void kc85_4_video_ram_select_bank(int bank)
{
    /* calculate address of video ram to display */
    unsigned char *video_ram;

    video_ram = kc85_4_video_ram;

    if (bank!=0)
    {
		video_ram +=
				   (KC85_4_SCREEN_PIXEL_RAM_SIZE +
				   KC85_4_SCREEN_COLOUR_RAM_SIZE);
	}

    kc85_4_display_video_ram = video_ram;
}

unsigned char *kc85_4_get_video_ram_base(int bank, int colour)
{
    /* base address: screen 0 pixel data */
	unsigned char *addr = kc85_4_video_ram;

	if (bank!=0)
	{
		/* access screen 1 */
		addr += KC85_4_SCREEN_PIXEL_RAM_SIZE +
				KC85_4_SCREEN_COLOUR_RAM_SIZE;
	}

	if (colour!=0)
	{
		/* access colour information of selected screen */
		addr += KC85_4_SCREEN_PIXEL_RAM_SIZE;
	}

	return addr;
}

void    kc85_4_vh_stop(void) 
{
	if (kc85_4_video_ram)
	{
		free(kc85_4_video_ram);
		kc85_4_video_ram = NULL;
	}
}


/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void kc85_4_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    unsigned char *pixel_ram = kc85_4_display_video_ram;
    unsigned char *colour_ram = pixel_ram + 0x04000;

    int x,y;

	for (y=0; y<KC85_SCREEN_HEIGHT; y++)
	{
		for (x=0; x<(KC85_SCREEN_WIDTH>>3); x++)
		{
			unsigned char colour_byte, gfx_byte;
			int offset;
			
			offset = y | (x<<8);

			colour_byte = colour_ram[offset];
		    gfx_byte = pixel_ram[offset];

			kc85_draw_8_pixels(bitmap,x,y, colour_byte, gfx_byte);
		
		}
	}
}

/***************************************************************************
 KC85/3 video
***************************************************************************/

int kc85_3_vh_start(void)
{
	return 0;
}

void    kc85_3_vh_stop(void) 
{
}

extern unsigned char *kc85_ram;

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void kc85_3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* colour ram takes up 0x02800 bytes */
	   unsigned char *pixel_ram = kc85_ram+0x08000;	
    unsigned char *colour_ram = pixel_ram + 0x02800;

    int x,y;

	for (y=0; y<KC85_SCREEN_HEIGHT; y++)
	{
		for (x=0; x<(KC85_SCREEN_WIDTH>>3); x++)
		{
			unsigned char colour_byte, gfx_byte;
			int pixel_offset,colour_offset;
			
			if ((x & 0x020)==0)
			{
				pixel_offset = (x & 0x01f) | (((y>>2) & 0x03)<<5) |
				((y & 0x03)<<7) | (((y>>4) & 0x0f)<<9);
	
				colour_offset = (x & 0x01f) | (((y>>2) & 0x03f)<<5);
			}
			else
			{
				/* 1  0  1  0  0  V7 V6 V1  V0 V3 V2 V5 V4 H2 H1 H0 */
				/* 1  0  1  1  0  0  0  V7  V6 V3 V2 V5 V4 H2 H1 H0 */

				pixel_offset = 0x02000+((x & 0x07) | (((y>>4) & 0x03)<<3) |
					(((y>>2) & 0x03)<<5) | ((y & 0x03)<<7) | ((y>>6) & 0x03)<<9);
	
				colour_offset = 0x0800+((x & 0x07) | (((y>>4) & 0x03)<<3) |
					(((y>>2) & 0x03)<<5) | ((y>>6) & 0x03)<<7);
			}
		
            colour_byte = colour_ram[colour_offset];
            gfx_byte = pixel_ram[pixel_offset];

			kc85_draw_8_pixels(bitmap,x,y, colour_byte, gfx_byte);
		}
	}
}
