/***************************************************************************

  amstrad.c.c

  Functions to emulate the video hardware of the amstrad CPC.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amstrad.h"
#include "cpuintrf.h"

/* CRTC emulation code */
#include "vidhrdw/m6845.h"

static crtc6845_state amstrad_vidhrdw_6845_state;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

extern int amstrad_cycles_last_write;
extern int amstrad_vh_execute_crtc_cycles_count;

/* CRTC status */
static int amstrad_CRTC_MA = 0; /* MA = Memory Address output */
static int amstrad_CRTC_RA = 0; /* RA = Row Address output */
static int amstrad_CRTC_HS = 0; /* HS = Horizontal Sync */
int amstrad_CRTC_VS = 0;        /* VS = Vertical Sync */
static int amstrad_CRTC_DE = 0; /* DE = Display Enabled */
int amstrad_CRTC_CR = 0;        /* CR = Cursor Enabled */

/* this is the real pixel position */
static int x_screen_pos;
//static int y_screen_pos;
int y_screen_pos;

/* this is the pixel position of the start of a scanline for the amstrad screen */
static int y_screen_offset = -32;

/* this contains the colours in Machine->pens form.*/
/* this is updated from the eventlist and reflects the current state
of the render colours - these may be different to the current colour palette values */
/* colours can be changed at any time and will take effect immediatly */
static unsigned long amstrad_GateArray_render_colours[17];

static struct mame_bitmap	*amstrad_bitmap;

/* the mode is re-loaded at each HSYNC */
/* current mode to render */
static int amstrad_render_mode;
/* current programmed mode */
static int amstrad_current_mode;
 
/* */
extern int amstrad_CRTC_HS_After_VS_Counter;

static unsigned long Mode0Lookup[256];
static unsigned long Mode1Lookup[256];
static unsigned long Mode3Lookup[256];

/* The Amstrad CPC has a fixed palette of 27 colours generated from 3 levels of Red, Green and Blue.
The hardware allows selection of 32 colours, but these extra colours are copies of existing colours.*/ 

unsigned char amstrad_palette[32 * 3] =
{
	0x080, 0x080, 0x080,			   /* white */
	0x080, 0x080, 0x080,			   /* white */
	0x000, 0x0ff, 0x080,			   /* sea green */
	0x0ff, 0x0ff, 0x080,			   /* pastel yellow */
	0x000, 0x000, 0x080,			   /* blue */
	0x0ff, 0x000, 0x080,			   /* purple */
	0x000, 0x080, 0x080,			   /* cyan */
	0x0ff, 0x080, 0x080,			   /* pink */
	0x0ff, 0x000, 0x080,			   /* purple */
	0x0ff, 0x0ff, 0x080,			   /* pastel yellow */
	0x0ff, 0x0ff, 0x000,			   /* bright yellow */
	0x0ff, 0x0ff, 0x0ff,			   /* bright white */
	0x0ff, 0x000, 0x000,			   /* bright red */
	0x0ff, 0x000, 0x0ff,			   /* bright magenta */
	0x0ff, 0x080, 0x000,			   /* orange */
	0x0ff, 0x080, 0x0ff,			   /* pastel magenta */
	0x000, 0x000, 0x080,			   /* blue */
	0x000, 0x0ff, 0x080,			   /* sea green */
	0x000, 0x0ff, 0x000,			   /* bright green */
	0x000, 0x0ff, 0x0ff,			   /* bright cyan */
	0x000, 0x000, 0x000,			   /* black */
	0x000, 0x000, 0x0ff,			   /* bright blue */
	0x000, 0x080, 0x000,			   /* green */
	0x000, 0x080, 0x0ff,			   /* sky blue */
	0x080, 0x000, 0x080,			   /* magenta */
	0x080, 0x0ff, 0x080,			   /* pastel green */
	0x080, 0x0ff, 0x080,			   /* lime */
	0x080, 0x0ff, 0x0ff,			   /* pastel cyan */
	0x080, 0x000, 0x000,			   /* Red */
	0x080, 0x000, 0x0ff,			   /* mauve */
	0x080, 0x080, 0x000,			   /* yellow */
	0x080, 0x080, 0x0ff	  		   /* pastel blue */
};


/* Initialise the palette */
PALETTE_INIT( amstrad_cpc )
{
	palette_set_colors(0, amstrad_palette, sizeof(amstrad_palette) / 3);
}

/*************************************************************************/
/* KC Compact

The palette is defined by a colour rom. The only rom dump that exists (from the KC-Club webpage)
is 2K, which seems correct. In this rom the same 32 bytes of data is repeated throughout the rom.

When a I/O write is made to "Gate Array" to select the colour, Bit 7 and 6 are used by the 
"Gate Array" to define the function, bit 7 = 0, bit 6 = 1. In the  Amstrad CPC, bits 4..0 
define the hardware colour number, but in the KC Compact, it seems bits 5..0 
define the hardware colour number allowing 64 colours to be chosen.

It is possible therefore that the colour rom could be reprogrammed, so that other colour
selections could be chosen allowing 64 different colours to be used. But this has not been tested
and co

colour rom byte:

Bit Function 
7 not used 
6 not used 
5,4 Green value
3,2 Red value
1,0 Blue value

Green value, Red value, Blue value: 0 = 0%, 01/10 = 50%, 11 = 100%.
The 01 case is not used, it is unknown if this produces a different amount of colour.
*/ 

static unsigned char kccomp_get_colour_element(int colour_value)
{
	switch (colour_value)
	{
		case 0:
			return 0x00;
		case 1:
			return 0x60;
		case 2:
			return 0x60;
		case 3:
			return 0x0ff;
	}

	return 0xff;
}


/* the colour rom has the same 32 bytes repeated, but it might be possible to put a new rom in
with different data and be able to select the other entries - not tested on a real kc compact yet
and not supported by this driver */
PALETTE_INIT( kccomp )
{
	int i;

	color_prom = color_prom+0x018000;

	for (i=0; i<32; i++)
	{
		colortable[i] = i;
		palette_set_color(i,
			kccomp_get_colour_element((color_prom[i]>>2) & 0x03),
			kccomp_get_colour_element((color_prom[i]>>4) & 0x03),
			kccomp_get_colour_element((color_prom[i]>>0) & 0x03));
	}
}


/********************************************
Amstrad Plus

The Amstrad Plus has a 4096 colour palette
*********************************************/

PALETTE_INIT( amstrad_plus )
{
	int i;

	for ( i = 0; i < 0x1000; i++ ) 
	{
		int r, g, b;

		r = ( i >> 8 ) & 0x0f;
		g = ( i >> 4 ) & 0x0f;
		b = i & 0x0f;

		r = ( r << 4 ) | ( r );
		g = ( g << 4 ) | ( g );
		b = ( b << 4 ) | ( b );

		palette_set_color(i, r, g, b);
		colortable[i] = i;
	}
}


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

		Mode3Lookup[i] = pen & 0x03;

		pen = (
			( ( (i & (1<<7)) >>7) <<0) |
		        ( ( (i & (1<<3)) >>3) <<1)
			);

		Mode1Lookup[i] = pen;

	}
}

static void amstrad_draw_screen_enabled(void)
{
	struct mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
	/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	/* amstrad fetches two bytes per CRTC clock. */
	int byte1 = mess_ram[addr];
	int byte2 = mess_ram[addr+1];

	int x = x_screen_pos;
	int y = y_screen_pos;
  
    /* render screen depending on the mode! */
	switch (amstrad_render_mode)		
	{
    
		/* mode 0 - low resolution - 16 colours */
		case 0:
		{
			int cpcpen,messpen;
			unsigned char data;

			data = byte1;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];

				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
			
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;                

			data = byte2;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
		}
		break;

        /* mode 1 - medium resolution - 4 colours */
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
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
                
				data = data<<1;
			}

                        data = byte2;

                        for (i=0; i<4; i++)
			{
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
                
				data = data<<1;
			}

		}
		break;

		/* mode 2: high resolution - 2 colours */
		case 2:
		{
			int i;
			unsigned long data = (byte1<<8) | byte2;
			int cpcpen,messpen;

			for (i=0; i<16; i++)
			{
				cpcpen = (data>>15) & 0x01;
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
                
				data = data<<1;

			}

		}
		break;

		/* undocumented mode. low resolution - 4 colours */
		case 3:
		{
			int cpcpen,messpen;
			unsigned char data;

			data = byte1;

			{
				cpcpen = Mode3Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];

				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				data = data<<1;

				cpcpen = Mode3Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
			
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
                
			}

			data = byte2;

			{
				cpcpen = Mode3Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
                

				data = data<<1;

				cpcpen = Mode3Lookup[data];
				messpen = amstrad_GateArray_render_colours[cpcpen];
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
				plot_pixel(bitmap,x,y,messpen);
				x++;
                
                	}
		}
		break;

		default:
			break;
	}


}

static void amstrad_draw_screen_disabled(void)
{
	struct mame_bitmap *bitmap = amstrad_bitmap;
	int i;
	for(i=0;i<(AMSTRAD_CHARACTERS*2);i++)
	{
		plot_pixel(bitmap,x_screen_pos+i,y_screen_pos,amstrad_GateArray_render_colours[16]);
	}
}


/************************************************************************
 * amstrad CRTC 6845 Status
 ************************************************************************/
/* CRTC - Set the new Memory Address output */
static void amstrad_Set_MA(int offset, int data)
{
	amstrad_CRTC_MA = data;
}
/* CRTC - Set the new Row Address output */
static void amstrad_Set_RA(int offset, int data)
{
	amstrad_CRTC_RA = data;
}
/* CRTC - Set new Horizontal Sync Status */
static void amstrad_Set_HS(int offset, int data)
		{
//  if (((amstrad_CRTC_HS^data) != 0)&&(data != 0)) {// New CRTC_HSync ? 
  if (data != 0) {
    amstrad_render_mode = amstrad_current_mode;
  } else { // End of CRTC_HSync
		if (y_screen_pos<AMSTRAD_SCREEN_HEIGHT) {
      y_screen_pos++;
    } //else {y_screen_pos = y_screen_offset;}
    x_screen_pos = 0;

//	The GA has a counter that increments on every falling edge of the CRTC generated HSYNC signal.
                amstrad_interrupt_timer_update();
  }
  amstrad_CRTC_HS = data;
}

/* CRTC - Set new Vertical Sync Status*/
static void amstrad_Set_VS(int offset, int data)
{
/* New CRTC_VSync */
//  if (((amstrad_CRTC_VS^data) != 0)&&(data != 0)) {
  if (data != 0) {
    y_screen_pos = y_screen_offset;
    x_screen_pos = 0;
/* Reset the amstrad_CRTC_HS_After_VS_Counter */
    amstrad_CRTC_HS_After_VS_Counter = 2;
  }
  amstrad_CRTC_VS = data;
}

/* CRTC - Set new Display Enabled Status*/
static void amstrad_Set_DE(int offset, int data)
{
	amstrad_CRTC_DE = data;
}

/* CRTC - Set new Cursor Status */
static void amstrad_Set_CR(int offset, int data)
{
	amstrad_CRTC_CR = data;
}
/* The cursor is not used on Amstrad. The CURSOR signal is available on the Expansion port for other hardware to use. */

static struct crtc6845_interface amstrad6845= {
	amstrad_Set_MA, // Memory Address register
	amstrad_Set_RA, // Row Address register
	amstrad_Set_HS, // Horizontal status
	amstrad_Set_VS, // Vertical status
	amstrad_Set_DE, // Display Enabled status
	amstrad_Set_CR, // Cursor status 
};

/* Set the new colour from the GateArray */
void amstrad_vh_update_colour(int PenIndex, int hw_colour_index)
{
	amstrad_GateArray_render_colours[PenIndex] = Machine->pens[hw_colour_index];
}

/* Set the new screen mode (0,1,2,4) from the GateArray */
void amstrad_vh_update_mode(int new_mode)
{
	amstrad_current_mode = new_mode;
}

/* execute crtc_execute_cycles of crtc */
void amstrad_vh_execute_crtc_cycles(int crtc_execute_cycles)
{
  while (crtc_execute_cycles > 0) {
		/* check that we are on the emulated screen area. */
			/* render the screen */
/* Move the CRT Beam on two 6845 character distance */
        
/*      if ((((x_screen_pos)==0)||(x_screen_pos == AMSTRAD_SCREEN_WIDTH-16))&&(y_screen_pos == 0))
		{
      	logerror("VSync CRTC STATUS(%04d,%04d) : MA(%04x) RA(%01x) HS(%01x) VS(%01x) DE(%01x) CR(%01x) Counter(%02d)\n"
        ,x_screen_pos
        ,y_screen_pos
        ,amstrad_CRTC_MA // Memory Address register
        ,amstrad_CRTC_RA // Row Address register
        ,amstrad_CRTC_HS // Horizontal status
        ,amstrad_CRTC_VS // Vertical status
        ,amstrad_CRTC_DE // Display Enabled status
        ,amstrad_CRTC_CR // Cursor status
        ,amstrad_CRTC_HS_After_VS_Counter);
		}
*/
		if ((x_screen_pos>=0) && (x_screen_pos<AMSTRAD_SCREEN_WIDTH) && (y_screen_pos>=0) && (y_screen_pos<AMSTRAD_SCREEN_HEIGHT)) {
      if (amstrad_CRTC_DE == 0) {
        amstrad_draw_screen_disabled();
      } else {
        amstrad_draw_screen_enabled();
      }

      x_screen_pos += (AMSTRAD_CHARACTERS*2);
    }
// Clock the 6845
		crtc6845_clock();
		crtc_execute_cycles--;
	}
}

/************************************************************************
 * amstrad_vh_screenrefresh
 * resfresh the amstrad video screen
 ************************************************************************/

VIDEO_UPDATE( amstrad )
{
	struct rectangle rect;

	rect.min_x = 0;
	rect.max_x = AMSTRAD_SCREEN_WIDTH-1;
	rect.min_y = -8;
	rect.max_y = AMSTRAD_SCREEN_HEIGHT-1;

    copybitmap(bitmap, amstrad_bitmap, 0,0,0,0,&rect, TRANSPARENCY_NONE,0); 
}


/************************************************************************
 * amstrad_vh_start
 * Initialize the amstrad video emulation
 ************************************************************************/

VIDEO_START( amstrad )
{
	amstrad_init_lookups();

	crtc6845_start();
	crtc6845_config(&amstrad6845);
	crtc6845_reset(0);
	crtc6845_get_state(0, &amstrad_vidhrdw_6845_state);
	
	amstrad_cycles_last_write = 0;
	amstrad_CRTC_HS_After_VS_Counter = 2;

	amstrad_bitmap = auto_bitmap_alloc_depth(AMSTRAD_SCREEN_WIDTH, AMSTRAD_SCREEN_HEIGHT,16);

	return 0;

}
