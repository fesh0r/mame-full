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

extern int amstrad_plus_asic_enabled;
extern int amstrad_plus_pri;
extern int amstrad_system_type;
extern unsigned char *amstrad_plus_asic_ram;

int amstrad_plus_split_scanline;  // ASIC split screen 
int amstrad_plus_split_address;

#ifdef MAME_DEBUG
extern int amstrad_plus_lower_enabled;
#endif

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

/* CRTC status */
static int amstrad_CRTC_MA = 0; /* MA = Memory Address output */
static int amstrad_CRTC_RA = 0; /* RA = Row Address output */
static int amstrad_CRTC_HS = 0; /* HS = Horizontal Sync */
int amstrad_CRTC_VS = 0;        /* VS = Vertical Sync */
static int amstrad_CRTC_DE = 0; /* DE = Display Enabled */
int amstrad_CRTC_CR = 0;        /* CR = Cursor Enabled */

/* this is the real pixel position */
static int x_screen_pos;
static int y_screen_pos;

/* this is the pixel position of the start of a scanline for the amstrad screen */
static int x_screen_offset = -224;
static int y_screen_offset = -32;

/* this contains the colours in Machine->pens form.*/
/* this is updated from the eventlist and reflects the current state
of the render colours - these may be different to the current colour palette values */
/* colours can be changed at any time and will take effect immediatly */
static unsigned long amstrad_GateArray_render_colours[17];
/* The gate array counts CRTC HSYNC pulses. (It has a internal 6-bit counter). */
int amstrad_CRTC_HS_Counter;
/* 2 HSYNCS after the VSYNC Counter */
static int amstrad_CRTC_HS_After_VS_Counter;

static mame_bitmap	*amstrad_bitmap;

static int amstrad_scanline;


/* the mode is re-loaded at each HSYNC */
/* current mode to render */
static int amstrad_render_mode;
static void (*draw_function)(void);
/* current programmed mode */
static int amstrad_current_mode;
 
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

/* the green brightness is equal to the firmware colour index */
unsigned char amstrad_green_palette[32 * 3] =
{
	0x000, 0x07F, 0x000,        /*13*/
	0x000, 0x07F, 0x000,        /*13*/
	0x000, 0x0BA, 0x000,        /*19*/
	0x000, 0x0F5, 0x000,        /*25*/
	0x000, 0x009, 0x000,        /*1*/
	0x000, 0x044, 0x000,        /*7*/
	0x000, 0x062, 0x000,        /*10*/
	0x000, 0x09C, 0x000,        /*16*/
	0x000, 0x044, 0x000,        /*7*/
	0x000, 0x0F5, 0x000,        /*25*/
	0x000, 0x0EB, 0x000,        /*24*/
	0x000, 0x0FF, 0x000,        /*26*/
	0x000, 0x03A, 0x000,        /*6*/
	0x000, 0x04E, 0x000,        /*8*/
	0x000, 0x093, 0x000,        /*15*/
	0x000, 0x0A6, 0x000,        /*17*/
	0x000, 0x009, 0x000,        /*1*/
	0x000, 0x0BA, 0x000,        /*19*/
	0x000, 0x0B0, 0x000,        /*18*/
	0x000, 0x0C4, 0x000,        /*20*/
	0x000, 0x000, 0x000,        /*0*/
	0x000, 0x013, 0x000,        /*2*/
	0x000, 0x058, 0x000,        /*9*/
	0x000, 0x06B, 0x000,        /*11*/
	0x000, 0x027, 0x000,        /*4*/
	0x000, 0x0D7, 0x000,        /*22*/
	0x000, 0x0CD, 0x000,        /*21*/
	0x000, 0x0E1, 0x000,        /*23*/
	0x000, 0x01D, 0x000,        /*3*/
	0x000, 0x031, 0x000,        /*5*/
	0x000, 0x075, 0x000,        /*12*/
	0x000, 0x089, 0x000         /*14*/
};
/* Initialise the palette */
PALETTE_INIT( amstrad_cpc )
{
   	if ( ((readinputportbytag("green_display")) & 0x01)==0 )
	   palette_set_colors(0, amstrad_palette, sizeof(amstrad_palette) / 3);
   	else
	   palette_set_colors(0, amstrad_green_palette, sizeof(amstrad_green_palette) / 3);   	
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
			return 0xff;
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

	palette_set_colors(0, amstrad_palette, sizeof(amstrad_palette) / 3);
	for ( i = 0; i < 0x1000; i++ ) 
	{
		int r, g, b;

		r = ( i >> 8 ) & 0x0f;
		g = ( i >> 4 ) & 0x0f;
		b = i & 0x0f;

		r = ( r << 4 ) | ( r );
		g = ( g << 4 ) | ( g );
		b = ( b << 4 ) | ( b );

		palette_set_color(i+48, g, r, b);
		colortable[i+48] = i+48;  // take into account the original palette, and sprite palette
	}
}

void amstrad_plus_setspritecolour(unsigned int off, int r, int g, int b)
{
	palette_set_color((off/2) + 33, r, g, b);
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
/* Set the new colour from the GateArray */
void amstrad_vh_update_colour(int PenIndex, int hw_colour_index)
{
	int val;
/*  int cpu_cycles = ((cycles_currently_ran()>>2)-1) & 63;

	logerror("color is changed(%d,%d) = %d\n",PenIndex, cpu_cycles, Machine->pens[hw_colour_index]);
  amstrad_GateArray_colours_ischanged++;
	amstrad_GateArray_changed_colours[cpu_cycles][PenIndex] = Machine->pens[hw_colour_index];
*/
	amstrad_GateArray_render_colours[PenIndex] = Machine->pens[hw_colour_index];
	if(amstrad_system_type != 0)
	{  // CPC+/GX4000 - normal palette changes through the Gate Array also makes the corresponding change in the ASIC palette
		val = amstrad_palette[hw_colour_index*3+2] >> 4;
		val += amstrad_palette[hw_colour_index*3] & 0xf0;
		amstrad_plus_asic_ram[0x2400+PenIndex*2] = val;
		val = amstrad_palette[hw_colour_index*3+1] >> 4;
		amstrad_plus_asic_ram[0x2401+PenIndex*2] = val;
	}
}

/* Set the new screen mode (0,1,2,4) from the GateArray */
void amstrad_vh_update_mode(int new_mode)
{
	amstrad_current_mode = new_mode;
	
}

static void amstrad_draw_screen_disabled(void)
{
	int colour;

	if(amstrad_plus_asic_enabled == 0)
		plot_box(amstrad_bitmap,x_screen_pos,y_screen_pos,AMSTRAD_CHARACTERS*2,1,amstrad_GateArray_render_colours[16]);
	else
	{
		colour = 48 + amstrad_plus_asic_ram[0x2420];
		colour += amstrad_plus_asic_ram[0x2421] << 8;
		plot_box(amstrad_bitmap,x_screen_pos,y_screen_pos,AMSTRAD_CHARACTERS*2,1,colour);
	}
}

/* mode 0 - low resolution - 16 colours */
static void amstrad_plus_draw_screen_enabled_mode_0(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
	/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int cpcpen, messpen;

	unsigned char data = mess_ram[addr];

	cpcpen = Mode0Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode0Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+4,y,4,1,messpen);

	data = mess_ram[addr+1];

	cpcpen = Mode0Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+8,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode0Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+12,y,4,1,messpen);
}

/* mode 1 - medium resolution - 4 colours */
static void amstrad_plus_draw_screen_enabled_mode_1(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;

  int i, cpcpen, messpen; 
  unsigned char data1 = mess_ram[addr];
  unsigned char data2 = mess_ram[addr+1];

  for (i=0;i<4;i++) {
		cpcpen = Mode1Lookup[data1& 0xFF];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
  	plot_box(bitmap,x,y,2,1,messpen);

		cpcpen = Mode1Lookup[data2];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
  	plot_box(bitmap,x+8,y,2,1,messpen);

  	x += 2;
		data1 = data1<<1;
		data2 = data2<<1;
	}
}

/* mode 2: high resolution - 2 colours */
static void amstrad_plus_draw_screen_enabled_mode_2(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int i, cpcpen, messpen;
	unsigned long data = (mess_ram[addr]<<8) | mess_ram[addr+1];


	for (i=0; i<16; i++)
	{
		cpcpen = (data>>15) & 0x01;
		messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
		messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
		plot_pixel(bitmap,x,y,messpen);
    x++;        
		data = data<<1;
	}
}

/* undocumented mode. low resolution - 4 colours */
static void amstrad_plus_draw_screen_enabled_mode_3(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int cpcpen, messpen;
	unsigned char data = mess_ram[addr];

	cpcpen = Mode3Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode3Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+4,y,4,1,messpen);

	data = mess_ram[addr+1];

	cpcpen = Mode3Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+8,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode3Lookup[data];
	messpen = 48 + (amstrad_plus_asic_ram[0x2400+cpcpen*2]);//amstrad_GateArray_render_colours[cpcpen];
	messpen += (amstrad_plus_asic_ram[0x2401+cpcpen*2]) << 8;
	plot_box(bitmap,x+12,y,4,1,messpen);
}

/* mode 0 - low resolution - 16 colours */
static void amstrad_draw_screen_enabled_mode_0(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
	/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int cpcpen, messpen;

	unsigned char data = mess_ram[addr];

	if(amstrad_plus_asic_enabled != 0)
	{
		amstrad_plus_draw_screen_enabled_mode_0();
		return;
	}

	cpcpen = Mode0Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode0Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+4,y,4,1,messpen);

	data = mess_ram[addr+1];

	cpcpen = Mode0Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+8,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode0Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+12,y,4,1,messpen);
}

/* mode 1 - medium resolution - 4 colours */
static void amstrad_draw_screen_enabled_mode_1(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;

  int i, cpcpen, messpen; 
  unsigned char data1 = mess_ram[addr];
  unsigned char data2 = mess_ram[addr+1];

	if(amstrad_plus_asic_enabled != 0)
	{
		amstrad_plus_draw_screen_enabled_mode_1();
		return;
	}

  for (i=0;i<4;i++) {
		cpcpen = Mode1Lookup[data1& 0xFF];
    messpen = amstrad_GateArray_render_colours[cpcpen];
  	plot_box(bitmap,x,y,2,1,messpen);

		cpcpen = Mode1Lookup[data2];
    messpen = amstrad_GateArray_render_colours[cpcpen];
  	plot_box(bitmap,x+8,y,2,1,messpen);

  	x += 2;
		data1 = data1<<1;
		data2 = data2<<1;
	}
}

/* mode 2: high resolution - 2 colours */
static void amstrad_draw_screen_enabled_mode_2(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int i, cpcpen, messpen;
	unsigned long data = (mess_ram[addr]<<8) | mess_ram[addr+1];


	for (i=0; i<16; i++)
	{
		cpcpen = (data>>15) & 0x01;
		messpen = amstrad_GateArray_render_colours[cpcpen];
		plot_pixel(bitmap,x,y,messpen);
    x++;        
		data = data<<1;
	}
}

/* undocumented mode. low resolution - 4 colours */
static void amstrad_draw_screen_enabled_mode_3(void)
{
	mame_bitmap *bitmap = amstrad_bitmap;

	int ma = amstrad_CRTC_MA; // crtc6845_memory_address_r(0);
	int ra = amstrad_CRTC_RA; // crtc6845_row_address_r(0);
/* calc mem addr to fetch data from	based on ma, and ra */
	unsigned int addr = (((ma>>(4+8)) & 0x03)<<14) |
			((ra & 0x07)<<11) |
			((ma & 0x03ff)<<1);

	int x = x_screen_pos;
	int y = y_screen_pos;
	int cpcpen, messpen;
	unsigned char data = mess_ram[addr];

	cpcpen = Mode3Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode3Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+4,y,4,1,messpen);

	data = mess_ram[addr+1];

	cpcpen = Mode3Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+8,y,4,1,messpen);

	data = data<<1;

	cpcpen = Mode3Lookup[data];
	messpen = amstrad_GateArray_render_colours[cpcpen];
	plot_box(bitmap,x+12,y,4,1,messpen);
}

/* execute crtc_execute_cycles of crtc */
void amstrad_vh_execute_crtc_cycles(int dummy)
{
	crtc6845_clock(); // Clock the 6845
	if ((x_screen_pos >= 0) && (x_screen_pos < AMSTRAD_SCREEN_WIDTH) && (y_screen_pos >= 0))
	{
		/* render the screen */
		(draw_function)();
	}
	x_screen_pos += (AMSTRAD_CHARACTERS*2); // Move to next raster
/*    }*/
}

/*
  ASIC hardware sprites
  16 sprites, 15 colours, 4096 colour palette

  ASIC sprite memory map, must be mapped in using the secondary lower ROM select register

  &4000 - &4fff   Sprite bitmap data, lower 4 bits.

  &6000 - &607f   Sprite properties, 8 bytes each
                  byte 0-1: Sprite X location
				  byte 2-3: Sprite Y location
				  byte 5:   Sprite magnication (LSB first)
				            bit 0-1: Y Magnification
							bit 2-3: X magnification
                                     00 = not displayed
									 01 = x1
									 10 = x2
									 11 = x4

  &6422 - &643f   Sprite palette, 12-bit, xxxxGGGGRRRRBBBB, sprite pens 1-15 (0 is always transparent)
*/
void amstrad_plus_sprite_draw(mame_bitmap* scr_bitmap)
{
	int spr;  // sprite number
	int xloc,yloc;
	int xmag,ymag;  // sprite properties
	int sprptr;  // sprite location in ASIC RAM
	rectangle rect;
	crtc6845_state vid;

	crtc6845_get_state(0,&vid);

	// get display bounds from CRTC registers (sprites are bound and clipped to inside the border)
	rect.min_x = (((vid.registers[0] - 1) - (vid.registers[2] - 1))*4)+8;
	rect.max_x = rect.min_x + (vid.registers[1] * 16);
	rect.min_y = (((vid.registers[4] - 1) - (vid.registers[7] - 1))*4)+4;
	rect.max_y = rect.min_y + (vid.registers[6] * (vid.registers[9]+1));

	for(spr=0;spr<16;spr++)
	{
		sprptr = 0x2000 + (8*spr);
		xmag = (amstrad_plus_asic_ram[sprptr+4] & 0x0c) >> 2;
		ymag = amstrad_plus_asic_ram[sprptr+4] & 0x03;
		if(xmag != 0 && ymag != 0)
		{
				xmag = 1<<(15+xmag);
				ymag = 1<<(15+ymag);
			xloc = amstrad_plus_asic_ram[sprptr] + (amstrad_plus_asic_ram[sprptr+1] << 8);
			xloc += rect.min_x;
			yloc = amstrad_plus_asic_ram[sprptr+2] + (amstrad_plus_asic_ram[sprptr+3] << 8);
			yloc += rect.min_y;
			decodechar(Machine->gfx[0],spr,amstrad_plus_asic_ram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			drawgfxzoom(scr_bitmap,Machine->gfx[0],spr,0,0,0,xloc,yloc,&rect,
				TRANSPARENCY_COLOR,32,xmag,ymag);
		}
	}
}

/*
	CPC+ / GX4000 ASIC split screen registers
*/
void amstrad_plus_setsplitline(unsigned int line, unsigned int address)
{
	amstrad_plus_split_scanline = line;
	amstrad_plus_split_address = address;
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
	if (data != 0)
	{
		amstrad_render_mode = amstrad_current_mode;
		x_screen_pos = x_screen_offset;
	}
	else
	{
		/* End of CRTC_HSync */
		if (y_screen_pos<AMSTRAD_SCREEN_HEIGHT)
		{
			y_screen_pos++;
		}
//	The GA has a counter that increments on every falling edge of the CRTC generated HSYNC signal.
 
		amstrad_CRTC_HS_Counter++;
		amstrad_scanline++;

		if(amstrad_plus_asic_enabled == 0 || amstrad_plus_pri == 0 || amstrad_system_type == 0)
		{
			if (amstrad_CRTC_HS_After_VS_Counter != 0)
			{
				amstrad_CRTC_HS_After_VS_Counter--;
				
				if (amstrad_CRTC_HS_After_VS_Counter == 0)
				{
					if (amstrad_CRTC_HS_Counter >= 32)
					{
//						logerror("IRQ: standard GA IRQ triggered, scanline %i\n",amstrad_scanline);
						cpunum_set_input_line(0,0, ASSERT_LINE);
					}
					amstrad_CRTC_HS_Counter = 0;
				}
			}
		
			if (amstrad_CRTC_HS_Counter == 52)
			{
				amstrad_CRTC_HS_Counter = 0;
//				logerror("IRQ: standard GA IRQ triggered, scanline %i\n",amstrad_scanline);
				cpunum_set_input_line(0,0, ASSERT_LINE);
			}
		}
		else
		{  // CPC+/GX4000 Programmable Raster Interrupt (disabled if &6800 in ASIC RAM is 0)
			if(amstrad_scanline == amstrad_plus_pri && amstrad_plus_pri != 0)  
			{
//				logerror("IRQ: PRI triggered, scanline %i\n",amstrad_scanline);
				cpunum_set_input_line(0,0,ASSERT_LINE);
				amstrad_plus_asic_ram[0x2804] |= 0x80;
				amstrad_CRTC_HS_Counter &= ~0x20;  // ASIC PRI resets the MSB of the raster counter
			}
			// CPC+/GX4000 Split screen registers  (disabled if &6801 in ASIC RAM is 0)
			if(amstrad_plus_split_scanline != 0)
			{
				if(amstrad_scanline == amstrad_plus_split_scanline) // split occurs here (hopefully)
				{
					crtc6845_state vid;

					// This if off by a bit (see Robocop 2), but is the easiest way that works consistently that I know of.
					crtc6845_get_state(0,&vid);
					vid.Memory_Address_of_next_Character_Row = amstrad_plus_split_address;
					crtc6845_set_state(0,&vid);
				}
			}
		}
	}
	amstrad_CRTC_HS = data;
	if(amstrad_scanline > 311) // 312 scanlines by default
		amstrad_scanline = 0;
} 

/* CRTC - Set new Vertical Sync Status*/
static void amstrad_Set_VS(int offset, int data)
{
/* New CRTC_VSync */
//  if (((amstrad_CRTC_VS^data) != 0)&&(data != 0)) {
  if (data != 0) {
    y_screen_pos = y_screen_offset;
      x_screen_pos = x_screen_offset;
/* Reset the amstrad_CRTC_HS_After_VS_Counter */
    amstrad_CRTC_HS_After_VS_Counter = 2;
  }
  amstrad_CRTC_VS = data;
}

/* CRTC - Set new Display Enabled Status*/
static void amstrad_Set_DE(int offset, int data)
{
	amstrad_CRTC_DE = data;
	if (amstrad_CRTC_DE == 0)
	{
		draw_function = amstrad_draw_screen_disabled;
	}
	else
	{
		switch (amstrad_current_mode) {
		case 0x00:
			draw_function = amstrad_draw_screen_enabled_mode_0;
			break;
		case 0x01:
			draw_function = amstrad_draw_screen_enabled_mode_1;
			break;
		case 0x02:
			draw_function = amstrad_draw_screen_enabled_mode_2;
			break;
		case 0x03:
			draw_function = amstrad_draw_screen_enabled_mode_3;
			break;
		}
  	 }
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

/************************************************************************
 * amstrad_vh_screenrefresh
 * resfresh the amstrad video screen
 ************************************************************************/

VIDEO_UPDATE( amstrad )
{
	rectangle rect;

	rect.min_x = 0;
	rect.max_x = AMSTRAD_SCREEN_WIDTH-1;
	rect.min_y = 0;
	rect.max_y = AMSTRAD_SCREEN_HEIGHT-1;

#ifdef MAME_DEBUG
	if(code_pressed(KEYCODE_Z) && amstrad_system_type == 1)
	{
		int x;
		for(x=0;x<32;x+=2)
		{
			amstrad_plus_asic_ram[0x2400+x] = ((x/2)<< 4) + x/2;
			amstrad_plus_asic_ram[0x2401+x] = x/2;
		}
	}
#endif
	copybitmap(bitmap, amstrad_bitmap, 0,0,0,0,&rect, TRANSPARENCY_NONE,0); 
	if(amstrad_plus_asic_enabled != 0)
		amstrad_plus_sprite_draw(bitmap);

	return 0;
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
	
	draw_function = amstrad_draw_screen_disabled;
	
	amstrad_CRTC_HS_After_VS_Counter = 2;
	x_screen_pos = x_screen_offset;
	y_screen_pos = y_screen_offset;

	amstrad_bitmap = auto_bitmap_alloc_depth(AMSTRAD_SCREEN_WIDTH, AMSTRAD_SCREEN_HEIGHT,16);

	return 0;

}
