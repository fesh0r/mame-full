/***************************************************************************

  vidhrdw/oric.c

  do all the real wacky stuff to render an Oric Text / Hires screen

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/oric.h"

static void oric_vh_update_flash(void);

/* current state of the display */
/* some attributes persist until they are turned off.
This structure holds this persistant information */
struct oric_vh_state
{
	/* foreground and background colour used for rendering */
	/* if flash attribute is set, these two will both be equal
	to background colour */
	int active_foreground_colour;
	int active_background_colour;
	/* current foreground and background colour */
	int foreground_colour;
	int background_colour;
	int mode;
	/* text attributes */
	int text_attributes;
	
	/* if (1<<3), display graphics, if 0, hide graphics */
	int flash_state;
	/* current count */
	int flash_count;

	void *timer;
};


static struct oric_vh_state vh_state;

static void oric_vh_timer_callback(int reg)
{
	/* update flash count */
	vh_state.flash_count++;
	if (vh_state.flash_count == 30)
	{	
		vh_state.flash_count = 0;
		vh_state.flash_state ^=(1<<3);
		oric_vh_update_flash();
	}
}

int oric_vh_start(void)
{
	/* initialise flash timer */
	vh_state.flash_count = 0;
	vh_state.flash_state = 0;
	vh_state.timer = timer_pulse(TIME_IN_HZ(50), 0, oric_vh_timer_callback);
    return 0;
}

void oric_vh_stop(void)
{
	if (vh_state.timer!=NULL)
	{
		timer_remove(vh_state.timer);
		vh_state.timer = NULL;
	}
}


static void oric_vh_update_flash(void)
{
	/* flash active? */
	if (vh_state.text_attributes & (1<<2))
	{
		/* yes */

		/* show or hide text? */
		if (vh_state.flash_state)
		{
			/* hide */
			/* set foreground and background to be the same */
			vh_state.active_foreground_colour = vh_state.background_colour;
			vh_state.active_background_colour = vh_state.background_colour;
			return;
		}
	}
	

	/* show */
	vh_state.active_foreground_colour = vh_state.foreground_colour;
	vh_state.active_background_colour = vh_state.background_colour;
}


static void oric_vh_update_attribute(int c)
{
	/* attribute */
	int attribute = c & 0x03f;

	switch ((attribute>>3) & 0x03)
	{
		case 0:
		{
			/* set foreground colour */
			vh_state.foreground_colour = attribute & 0x07;
			oric_vh_update_flash();
		}
		break;

		case 1:
		{
			vh_state.text_attributes = attribute & 0x07;
			/* text attributes */
			oric_vh_update_flash();
		}
		break;

		case 2:
		{
			/* set background colour */
			vh_state.background_colour = attribute & 0x07;
			oric_vh_update_flash();
		}
		break;

		case 3:
		{
			/* set video mode */
			vh_state.mode = attribute & 0x07;
		}
		break;

		default:
			break;
	}
}


/* render 6-pixels using foreground and background colours specified */
static void oric_vh_render_6pixels(struct osd_bitmap *bitmap,int x,int y, int fg, int bg,int data)
{
	int i;
	int fg_pen = Machine->pens[fg];
	int bg_pen = Machine->pens[bg];

	for (i=0; i<6; i++)
	{
		int col;

		if ((data & (1<<5))!=0)
		{
			col = fg_pen;
		}
		else
		{
			col = bg_pen;
		}

		plot_pixel(bitmap,x+i, y, col);

		data = data<<1;
	}
}

			


/***************************************************************************
  oric_vh_screenrefresh
***************************************************************************/
void oric_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{

/**** TODO ... HIRES ... how do the hires attr's set HIRES mode?? ****/
/**** TODO ... HIRES MODE LINES,  render cell from HIRES BITMAP MEM AREA ****/
//		unsigned char *text_ram;
  //      unsigned char *hires_ram;
        unsigned char *char_ram;
        unsigned char *achar_ram;
        unsigned char *hchar_ram;
        unsigned char *hachar_ram;
        unsigned char *tchar_ram;
        unsigned char *tachar_ram;
        unsigned char *RAM = memory_region(REGION_CPU1);
    //    text_ram    = &RAM[0xBB80] ;
      //  hires_ram   = &RAM[0xA000] ;
        char_ram    = &RAM[0xB400] ;
        achar_ram   = &RAM[0xB800] ;
        hchar_ram   = &RAM[0x9800] ;
        hachar_ram  = &RAM[0x9c00] ;
        tchar_ram   = &RAM[0xB400] ;
        tachar_ram  = &RAM[0xB800] ;

	

		{
			int char_line;
			int byte_offset;
			int y;
			int old_read_addr;
			int read_addr;

			if (vh_state.mode & (1<<3))
			{
				read_addr = 0x0a000;
			}
			else
			{
				read_addr = 0x0bb80;
			}

			y = 0;
			char_line = 0;
			old_read_addr = read_addr;
			for (y = 0; y < 224; y++)
			{
				int x = 0;

				/* foreground colour 7 */
				oric_vh_update_attribute(7);
				/* background colour 0 */
				oric_vh_update_attribute((1<<3));
				/* no flash,text */
				oric_vh_update_attribute((1<<4));
				/* mode */
				oric_vh_update_attribute((1<<3)|(1<<4));

				for (byte_offset=0; byte_offset<40; byte_offset++)
				{
					int c;

					c = RAM[read_addr];
					read_addr++;

					/* if bits 6 and 5 are zero, the byte contains a serial attribute */
					if ((c & ((1<<6) | (1<<5)))==0)
					{
						oric_vh_update_attribute(c);
												

						/* display background colour when attribute has been found */
						oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, 0);
					}
					else
					{
						/* hires? */
						if (vh_state.mode & (1<<3))
						{
							int pixel_data = c & 0x03f;

							oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, pixel_data);
						}
						else
						{
							int char_index;
							int char_data;

							char_index = (c & 0x07f);
							char_data = char_ram[(char_index<<3) | char_line] & 0x03f;

							oric_vh_render_6pixels(bitmap,x,y,
								vh_state.active_foreground_colour, 
								vh_state.active_background_colour, char_data);
						}

					}


					x=x+6;	
				}
				
				/* if in text mode and char line is not equal to 7, then reset read addr */
				if (((vh_state.mode & (1<<3))==0) && (char_line!=7))
				{
					/* no! */

					read_addr = old_read_addr;
				}

				if (y==200)
				{
					read_addr = 0x0bf68;
				}

				char_line++;
				char_line = char_line & 7;
			
				if (char_line==0)
				{
					old_read_addr = read_addr;
				}

			}
        }
}


