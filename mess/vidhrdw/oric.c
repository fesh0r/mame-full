/***************************************************************************

  vidhrdw/oric.c

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

	/* current memory address to fetch char data */
	unsigned char *text_char_data;
	unsigned char *hires_char_data;

	unsigned char last_text_attribute;

	/* It is set to 0 on the first line that has
	double height attribute set, and alternates between 0 and 1 each line
	which has double attribute set after */

	int dbl_line;
	
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
			/* This is a bit of a hack. In Fabrice's documents it says that
			double height is reset for each scan-line, but we need to know if the
			previous char line was double height so we can display the bottom half
			of the character.

			The attribute is stored on the last char and last line of the line,
			if this has changed, then dbl_line will be reset to 0, otherwise dbl_line
			will keep toggling as long as double height remains on and will work properly */

			/* if double height toggle the dbl line counter */
			if (attribute & (1<<1))
			{
				vh_state.dbl_line^=1;
			}

			/* changed from last time it was latched? */
			if (((vh_state.last_text_attribute^attribute) & (1<<1))!=0)
			{
				/* reset dbl line count */
				vh_state.dbl_line = 0;
			}

			/* alternate char set? */
			if ((vh_state.text_attributes & (1<<0))==0)
			{
				/* no */
				vh_state.text_char_data = memory_region(REGION_CPU1) + 0x0b400;
				vh_state.hires_char_data = memory_region(REGION_CPU1) + 0x09800;
			}
			else
			{
				vh_state.text_char_data = memory_region(REGION_CPU1) + 0x0b400 + (128*8);
				vh_state.hires_char_data = memory_region(REGION_CPU1) + 0x09800 + (128*8);
			}

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
static void oric_vh_render_6pixels(struct osd_bitmap *bitmap,int x,int y, int fg, int bg,int data, int invert_flag)
{
	int i;
	int fg_pen;
	int bg_pen;
				
	/* invert? */
	if (invert_flag)
	{
		fg ^=0x07;
		bg ^=0x07;
	}

	fg_pen = Machine->pens[fg];
	bg_pen = Machine->pens[bg];
	
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
	unsigned char *RAM;
	int char_line;
	int byte_offset;
	int y;
	int old_read_addr;
	int read_addr;

	RAM = memory_region(REGION_CPU1);

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
				oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, 0,(c & 0x080));

			}
			else
			{
				/* hires? */
				if (vh_state.mode & (1<<3))
				{
					int pixel_data = c & 0x03f;

					oric_vh_render_6pixels(bitmap,x,y,vh_state.active_foreground_colour, vh_state.active_background_colour, pixel_data,(c & 0x080));
				}
				else
				{
					int char_index;
					int char_data;
					int ch_line;

					char_index = (c & 0x07f);

					ch_line = char_line;

					/* is double height set? */
					if (vh_state.text_attributes & (1<<1))
					{ 
						/* fetch different line */
						ch_line = (ch_line>>1) + (vh_state.dbl_line<<2);
					}
					
					char_data = vh_state.text_char_data[(char_index<<3) | ch_line] & 0x03f;

					oric_vh_render_6pixels(bitmap,x,y,
						vh_state.active_foreground_colour, 
						vh_state.active_background_colour, char_data, (c & 0x080));
				}

			}
			
			x=x+6;	
		}
		
		/* char mode? */
		if ((vh_state.mode & (1<<3))==0)
		{
			/* if in text mode and char line is not equal to 7, then reset read addr */
			if (char_line!=7)
			{
				read_addr = old_read_addr;
			}
			else
			{
				/* push current text attribute at end of this line */
				vh_state.last_text_attribute = vh_state.text_attributes;
			}
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


