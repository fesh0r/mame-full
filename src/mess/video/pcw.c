/***************************************************************************

  pcw.c

  Functions to emulate the video hardware of the Amstrad PCW.

***************************************************************************/

#include "emu.h"
#include "includes/pcw.h"
#include "machine/ram.h"

INLINE void pcw_plot_pixel(bitmap_ind16 &bitmap, int x, int y, UINT32 color)
{
	bitmap.pix16(y, x) = (UINT16)color;
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

VIDEO_START( pcw )
{
	pcw_state *state = machine.driver_data<pcw_state>();
	rectangle rect(0, PCW_PRINTER_WIDTH - 1, 0, PCW_PRINTER_HEIGHT - 1);

	state->m_prn_output = auto_bitmap_ind16_alloc(machine,PCW_PRINTER_WIDTH,PCW_PRINTER_HEIGHT);
	state->m_prn_output->fill(1, rect);
}

/* two colours */
static const unsigned short pcw_colour_table[PCW_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static const rgb_t pcw_palette[PCW_NUM_COLOURS] =
{
	MAKE_RGB(0x000, 0x000, 0x000),
	MAKE_RGB(0x0ff, 0x0ff, 0x0ff)
};


/* Initialise the palette */
PALETTE_INIT( pcw )
{
	palette_set_colors(machine, 0, pcw_palette, ARRAY_LENGTH(pcw_palette));
}

/***************************************************************************
  Draw the game screen in the given bitmap_ind16.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
SCREEN_UPDATE_IND16( pcw )
{
	pcw_state *state = screen.machine().driver_data<pcw_state>();
	int x,y,b;
	unsigned short roller_ram_offs;
	unsigned char *roller_ram_ptr;
	int pen0,pen1;

	pen0 = 0;
	pen1 = 1;

	/* invert? */
	if (state->m_vdu_video_control_register & (1<<7))
	{
		/* yes */
		pen1^=1;
		pen0^=1;
	}

	/* video enable? */
	if ((state->m_vdu_video_control_register & (1<<6))!=0)
	{
		/* render top border */
		rectangle rect(0, PCW_SCREEN_WIDTH, 0, PCW_BORDER_HEIGHT);
		bitmap.fill(pen0, rect);

		/* render bottom border */
		rect.set(0, PCW_SCREEN_WIDTH, PCW_BORDER_HEIGHT + PCW_DISPLAY_HEIGHT, PCW_BORDER_HEIGHT + PCW_DISPLAY_HEIGHT + PCW_BORDER_HEIGHT);
		bitmap.fill(pen0, rect);

		/* offset to start in table */
		roller_ram_offs = (state->m_roller_ram_offset<<1);

		for (y=0; y<256; y++)
		{
			int by;
			unsigned short line_data;
			unsigned char *line_ptr;

			x = PCW_BORDER_WIDTH;

			roller_ram_ptr = screen.machine().device<ram_device>(RAM_TAG)->pointer() + state->m_roller_ram_addr + roller_ram_offs;

			/* get line address */
			/* b16-14 control which bank the line is to be found in, b13-3 the address in the bank (in 16-byte units), and b2-0 the offset. Thus a roller RAM address bbbxxxxxxxxxxxyyy indicates bank bbb, address 00xxxxxxxxxxx0yyy. */
			line_data = ((unsigned char *)roller_ram_ptr)[0] | (((unsigned char *)roller_ram_ptr)[1]<<8);

			/* calculate address of pixel data */
			line_ptr = screen.machine().device<ram_device>(RAM_TAG)->pointer() + ((line_data & 0x0e000)<<1) + ((line_data & 0x01ff8)<<1) + (line_data & 0x07);

			for (by=0; by<90; by++)
			{
				unsigned char byte;

				byte = line_ptr[0];

				for (b=0; b<8; b++)
				{
					if (byte & 0x080)
					{
						pcw_plot_pixel(bitmap,x+b, y+PCW_BORDER_HEIGHT, pen1);
					}
					else
					{
						pcw_plot_pixel(bitmap,x+b, y+PCW_BORDER_HEIGHT, pen0);

					}
					byte = byte<<1;
				}

				x = x + 8;


				line_ptr = line_ptr+8;
			}

			/* update offset, wrap within 512 byte range */
			roller_ram_offs+=2;
			roller_ram_offs&=511;

		}

		/* render border */
		/* 8 pixels either side of display */
		for (y=0; y<256; y++)
		{
			pcw_plot_pixel(bitmap, 0, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 1, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 2, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 3, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 4, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 5, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 6, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 7, y+PCW_BORDER_HEIGHT, pen0);

			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+0, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+1, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+2, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+3, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+4, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+5, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+6, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+7, y+PCW_BORDER_HEIGHT, pen0);
		}
	}
	else
	{
		/* not video - render whole lot in pen 0 */
		rectangle rect(0, PCW_SCREEN_WIDTH, 0, PCW_SCREEN_HEIGHT);
		bitmap.fill(pen0, rect);
	}
	return 0;
}

SCREEN_UPDATE_IND16( pcw_printer )
{
	pcw_state *state = screen.machine().driver_data<pcw_state>();

	// printer output
	INT32 feed;
	rectangle rect(0, PCW_PRINTER_WIDTH - 1, 0, PCW_PRINTER_HEIGHT - 1);
	feed = -(state->m_paper_feed / 2);
	copyscrollbitmap(bitmap,*state->m_prn_output,0,NULL,1,&feed,rect);
	bitmap.pix16(PCW_PRINTER_HEIGHT-1, state->m_printer_headpos) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-2, state->m_printer_headpos) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-3, state->m_printer_headpos) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-1, state->m_printer_headpos-1) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-2, state->m_printer_headpos-1) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-1, state->m_printer_headpos+1) = 0;
	bitmap.pix16(PCW_PRINTER_HEIGHT-2, state->m_printer_headpos+1) = 0;
	return 0;
}

