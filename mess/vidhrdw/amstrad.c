/***************************************************************************

  amstrad.c.c

  Functions to emulate the video hardware of the amstrad CPC.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/includes/amstrad.h"
#include "mess/vidhrdw/hd6845s.h"

/* CRTC emulation code */
#include "mess/vidhrdw/m6845.h"
/* event list for storing colour changes, mode changes and CRTC writes */
#include "mess/eventlst.h"
/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* this contains the colours in Machine->pens form.*/
/* this is updated from the eventlist and reflects the current state
of the render colours - these may be different to the current colour palette values */
/* colours can be changed at any time and will take effect immediatly */
static unsigned long amstrad_render_colours[17];

/* the mode is re-loaded at each HSYNC */
/* current mode to render */
static int amstrad_render_mode;

/* current programmed mode */
static int amstrad_current_mode;

static unsigned long Mode0Lookup[256];
static unsigned long Mode1Lookup[256];

static void amstrad_init_lookups(void)
{
	int i;

	for (i=0; i<256; i++)
	{
		int pen;

		pen = (
			((i & (1<<7))>>(7-0)) |
			((i & (1<<3))>>(3-1)) |
			((i & (1<<5))>>(5-2)) |
                        ((i & (1<<1))<<2)
			);

		Mode0Lookup[i] = pen;

		pen = (
			( ( (i & (1<<7)) >>7) <<0) |
		        ( ( (i & (1<<3)) >>3) <<1)
			);

		Mode1Lookup[i] = pen;

	}
}

extern unsigned char *Amstrad_Memory;
extern short AmstradCPC_PenColours[18];
extern unsigned char AmstradCPC_GA_RomConfiguration;

// this is the pixel position of the start of a scanline
// -96 sets the screen display to the middle of emulated screen.
static int x_screen_offset=0;	//-96;

static int y_screen_offset=0;

static int amstrad_HSync=0;
static int amstrad_VSync=0;
static int amstrad_Character_Row=0;
static int amstrad_DE=0;


//static unsigned char *amstrad_Video_RAM;
static unsigned char *amstrad_display;
static struct osd_bitmap *amstrad_bitmap;

static int x_screen_pos;
static int y_screen_pos;

static void (*draw_function)(void);

void amstrad_draw_screen_enabled(void)
{
	int sc1;
	int ma, ra;
	int addr;
	int byte1, byte2;

	sc1 = 0;
	ma = crtc6845_memory_address_r(0);
	ra = crtc6845_row_address_r(0);

	// calc mem addr to fetch data from
	//based on ma, and ra
	addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	// amstrad fetches two bytes per CRTC clock.
	byte1 = Amstrad_Memory[addr];
	byte2 = Amstrad_Memory[addr+1];

    // depending on the mode!
	switch (amstrad_render_mode)		//AmstradCPC_GA_RomConfiguration & 0x03)
	{
    
		// mode 0 - low resolution - 16 colours
		case 0:
		{
			int cpcpen,messpen;
			unsigned char data;

			data = byte1;

			{
				cpcpen = Mode0Lookup[data];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];

				amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
			
				amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                
			}

			data = byte2;

			{
				cpcpen = Mode0Lookup[data];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                
                	}
		}
		break;

        // mode 1 - medium resolution - 4 colours
		case 1:
		{
                        int i;
                        int cpcpen;
                        int messpen;
                        unsigned char data;

                        data = byte1;

                        for (i=0; i<4; i++)
                        {
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
    			amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                
				data = data<<1;
			}

                        data = byte2;

                        for (i=0; i<4; i++)
			{
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
				amstrad_display[sc1] = messpen;
				sc1++;
                amstrad_display[sc1] = messpen;
				sc1++;
                
				data = data<<1;
			}

		}
		break;

		// mode 2: high resolution - 2 colours
		case 2:
		{
			int i;
			unsigned long Data = (byte1<<8) | byte2;
			int cpcpen,messpen;

			for (i=0; i<16; i++)
			{
				cpcpen = (Data>>15) & 0x01;
				messpen = amstrad_render_colours[cpcpen];
				//Machine->pens[AmstradCPC_PenColours[cpcpen]];
//				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
				amstrad_display[sc1] = messpen;
				sc1++;
                
				Data = Data<<1;

			}

		}
		break;
	}


}

void amstrad_draw_screen_disabled(void)
{
	int sc1;
	int border_colour;

	border_colour = amstrad_render_colours[16];	//Machine->pens[AmstradCPC_PenColours[16]];

	// if the display is not enable, just draw a blank area.
	for(sc1=0;sc1<16;sc1++)
	{
		amstrad_display[sc1]=border_colour;	//VideoULA_border_colour;
	}
}

// Select the Function to draw the screen area
void amstrad_Set_VideoULA_DE(void)
{
	if (amstrad_DE)
	{
		draw_function=*amstrad_draw_screen_enabled;
	} 
	else 
	{
		draw_function=*amstrad_draw_screen_disabled;
	}
}


/************************************************************************
 * amstrad 6845 Outputs to Video ULA
 ************************************************************************/

// called when the 6845 changes the character row
void amstrad_Set_Character_Row(int offset, int data)
{
	amstrad_Character_Row=data;
	amstrad_Set_VideoULA_DE();
}

/* the horizontal screen position on the display is determined
by the length of the HSYNC and the position of the hsync */
void amstrad_Set_HSync(int offset, int data)
{
        /* hsync changed state? */
	if ((amstrad_HSync^data)!=0)
	{
		if (data!=0)
		{
                        /* start of hsync */
	
			/* set new render mode */
			amstrad_render_mode = amstrad_current_mode;
		}
                else
                {
                        /* end of hsync */
                        y_screen_pos+=1;
                        x_screen_pos=x_screen_offset;
                        amstrad_display=(amstrad_bitmap->line[y_screen_pos])+x_screen_pos;
                }
	}


	amstrad_HSync=data;
}

void amstrad_Set_VSync(int offset, int data)
{
        /* vsync changed state? */
        if ((amstrad_VSync^data)!=0)
	{
                if (data!=0)
                {
                   y_screen_pos=y_screen_offset;
                   amstrad_display=(amstrad_bitmap->line[y_screen_pos])+x_screen_pos;
                 }
       }
       amstrad_VSync=data;

}

// called when the 6845 changes the Display Enabled
void amstrad_Set_DE(int offset, int data)
{
	amstrad_DE=data;
	amstrad_Set_VideoULA_DE();
}

// called when the 6845 changes the Cursor Enabled
void amstrad_Set_CR(int offset, int data)
{
}

// If the cursor is on there is a counter in the VideoULA to control the length of the Cursor
void amstrad_Clock_CR(void)
{
}


/* The cursor is not used on Amstrad. The CURSOR signal is available on the Expansion port
for other hardware to use, but as far as I know it is not used by anything. */


static struct crtc6845_interface
amstrad6845= {
	0,// Memory Address register
	amstrad_Set_Character_Row,// Row Address register
	amstrad_Set_HSync,// Horizontal status
	amstrad_Set_VSync,// Vertical status
	amstrad_Set_DE,// Display Enabled status
	NULL,// Cursor status 
};

/************************************************************************
 * amstrad_vh_screenrefresh
 * resfresh the amstrad video screen
 ************************************************************************/

void amstrad_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int c;


	int crtc_execute_cycles;
	int num_cycles_remaining;
	EVENT_LIST_ITEM *pItem;
	int NumItemsRemaining;
	int previous_time;

	previous_time = 0;
	num_cycles_remaining = 19968;

	amstrad_bitmap=bitmap;
	amstrad_display = amstrad_bitmap->line[0];
	c=0;

	// video_refresh is set if any of the 6845 or Video ULA registers are changed
	// this then forces a full screen redraw


        pItem = EventList_GetFirstItem();
        NumItemsRemaining = EventList_NumEvents();

	do
	{
                if (NumItemsRemaining==0)
		{
			crtc_execute_cycles = num_cycles_remaining;
			num_cycles_remaining = 0;
		}
		else
		{
			int time_delta;

			/* calculate time between last event and this event */
			time_delta = pItem->Event_Time - previous_time;
		
			crtc_execute_cycles = time_delta/4;

			num_cycles_remaining -= time_delta/4;

                  //      logerror("Time Delta: %04x\r\n", time_delta);

                }

                while (crtc_execute_cycles>0)
		{
			// check that we are on the emulated screen area.
			if ((x_screen_pos>=0) && (x_screen_pos<AMSTRAD_SCREEN_WIDTH) && (y_screen_pos>=0) && (y_screen_pos<AMSTRAD_SCREEN_HEIGHT))
			{
				// Move the CRT Beam on one 6845 character distance
				x_screen_pos=x_screen_pos+16;	
				amstrad_display=amstrad_display+16;	

				// if the video ULA DE 'Display Enabled' input is high then draw the pixels else blank the screen
				(draw_function)();

			}

			// Clock the 6845
			crtc6845_clock();
			crtc_execute_cycles--;
		}

		if (NumItemsRemaining!=0)
		{
			switch ((pItem->Event_ID>>6) & 0x03)
			{
				case EVENT_LIST_CODE_GA_COLOUR:
				{
					int PenIndex = pItem->Event_ID & 0x03f;
					int Colour = pItem->Event_Data;

					amstrad_render_colours[PenIndex] = Machine->pens[/*AmstradCPC_PenColours[*/Colour/*]*/];
				}
				break;

				case EVENT_LIST_CODE_GA_MODE:
				{
					amstrad_current_mode = pItem->Event_Data;
				}
				break;

				case EVENT_LIST_CODE_CRTC_INDEX_WRITE:
				{
					/* register select */
					crtc6845_address_w(0,pItem->Event_Data);
				}
				break;

				case EVENT_LIST_CODE_CRTC_WRITE:
				{
					crtc6845_register_w(0, pItem->Event_Data);
				}
				break;

				default:
					break;
			}
		
			/* store time for next calculation */
			previous_time = pItem->Event_Time;
			pItem++;
			NumItemsRemaining--;		
		}
	}
	while (num_cycles_remaining>0);

    /* Assume all other routines have processed their data from the list */
    EventList_Reset();
    EventList_SetOffsetStartTime ( cpu_getcurrentcycles() );


}


/************************************************************************
 * amstrad_vh_start
 * Initialize the amstrad video emulation
 ************************************************************************/

int amstrad_vh_start(void)
{
        int i;

	amstrad_init_lookups();

	crtc6845_config(&amstrad6845);

//	amstrad_Video_RAM= memory_region(REGION_CPU1);
	draw_function=*amstrad_draw_screen_disabled;


	/* 64 us Per Line, 312 lines (PAL) = 19968 */
        amstrad_render_mode = 0;
        amstrad_current_mode = 0;
        for (i=0; i<17; i++)
        {
                amstrad_render_colours[i] = 0;
        }

	EventList_Initialise(19968);

	return 0;

}

/************************************************************************
 * amstrad_vh_stop
 * Shutdown the amstrad video emulation
 ************************************************************************/

void amstrad_vh_stop(void)
{
	EventList_Finish();
}
