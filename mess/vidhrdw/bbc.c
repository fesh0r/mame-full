/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "includes/bbc.h"
#include "vidhrdw/m6845.h"
#include "vidhrdw/bbctext.h"


/* just needed here temp. while the SAA5050 is being worked on */
void BBC_ula_showteletext(int col);



/************************************************************************
 * video_refresh flag is used in optimising the screen redrawing
 * it is set whenever a 6845 or VideoULA registers are change.
 * This will then cause a full screen refresh.
 * The vidmem array is used to optimise the screen redrawing
 * whenever a memory location is written to the same location is set in the vidmem array
 * if none of the video registers have been changed and a full redraw is not needed
 * the video display emulation will only redraw the video memory locations that have been changed.
 ************************************************************************/

static int video_refresh;
unsigned char vidmem[0x8000];

/************************************************************************
 * video memory lookup arrays.
 * this is a set of quick lookup arrays that stores the logic for the following:
 * the array to be used is selected by the output from bits 4 and 5 (C0 and C1) on IC32 74LS259
 * which is controlled by the system VIA.
 * C0 and C1 along with MA12 output from the 6845 drive 4 NAND gates in ICs 27,36 and 40
 * the outputs from these NAND gates (B1 to B4) along with MA8 to MA11 from the 6845 (A1 to B4) are added together
 * in IC39 74LS283 4 bit adder to form (S1 to S4) the logic is used to loop the screen memory for hardware scrolling.
 * when MA13 from the 6845 is low the latches IC8 and IC9 are enabled
 * they control the memory addressing for the Hi-Res modes.
 * when MA13 from the 6845 is high the latches IC10 and IC11 are enabled
 * they control the memory addressing for the Teletext mode.
 * IC 8 or IC10 drives the row select in the memory (the lower 7 bits in the memory address) and
 * IC 9 or IC11 drives the column select in the memory (the next 7 bits in the memory address) this
 * gives control of the bottom 14 bits of the memory, in a 32K model B 15 bits are needed to access
 * all the RAM, so S4 for the adder drives the CAS0 and CAS1 to access the top bit, in a 16K model A
 * the output of S4 is linked out to a 0v supply by link S25 to just access the 16K memory area.
 ************************************************************************/

static unsigned int video_ram_lookup0[0x4000];
static unsigned int video_ram_lookup1[0x4000];
static unsigned int video_ram_lookup2[0x4000];
static unsigned int video_ram_lookup3[0x4000];

static unsigned int *video_ram_lookup;

void set_video_memory_lookups(int ramsize)
{

	int ma; // output from IC2 6845 MA address

	int c0,c1; // output from IC32 74LS259 bits 4 and 5
	int ma12; // bit 12 of 6845 MA address

	int b1,b2,b3,b4; // 4 bit input B on IC39 74LS283 (4 bit adder)
	int a,b,s;

	unsigned int m;

	for(c1=0;c1<2;c1++)
	{
		for(c0=0;c0<2;c0++)
		{
			if ((c0==0) && (c1==0)) video_ram_lookup=video_ram_lookup0;
			if ((c0==1) && (c1==0)) video_ram_lookup=video_ram_lookup1;
			if ((c0==0) && (c1==1)) video_ram_lookup=video_ram_lookup2;
			if ((c0==1) && (c1==1)) video_ram_lookup=video_ram_lookup3;

			for(ma=0;ma<0x4000;ma++)
			{


				/* the 4 bit input port b on IC39 are produced by 4 NAND gates.
				these NAND gates take their
				inputs from c0 and c1 (from IC32) and ma12 (from the 6845) */

				/* get bit m12 from the 6845 */
				ma12=(ma>>12)&1;

				/* 3 input NAND part of IC 36 */
				b1=(~(c1&c0&ma12))&1;
				/* 2 input NAND part of IC40 (b3 is calculated before b2 and b4 because b3 feed back into b2 and b4) */
				b3=(~(c0&ma12))&1;
				/* 3 input NAND part of IC 36 */
				b2=(~(c1&b3&ma12))&1;
				/* 2 input NAND part of IC 27 */
				b4=(~(b3&ma12))&1;

				/* inputs port a to IC39 are MA8 to MA11 from the 6845 */
				a=(ma>>8)&0xf;
				/* inputs port b to IC39 are taken from the NAND gates b1 to b4 */
				b=(b1<<0)|(b2<<1)|(b3<<2)|(b4<<3);

				/* IC39 performs the 4 bit add with the carry input set high */
				s=(a+b+1)&0xf;

				/* if MA13 (TTXVDU) is low then IC8 and IC9 are used to calculate
				   the memory location required for the hi res video.
				   if MA13 is hight then IC10 and IC11 are used to calculate the memory location for the teletext chip
				   Note: the RA0,RA1,RA2 inputs to IC8 in high res modes will need to be added else where */
				if (((ma>>13)&1)==0)
				{
					m=((ma&0xff)<<3)|(s<<11);
				} else {
					m=((ma&0x3ff)|0x3c00)|((s&0x8)<<11);
				}
				if (ramsize==16)
				{
					video_ram_lookup[ma]=m & 0x3fff;
				} else {
					video_ram_lookup[ma]=m;
				}

			}
		}
	}
}


/* called from 6522 system via */
void setscreenstart(int c0,int c1)
{
	if ((c0==0) && (c1==0)) video_ram_lookup=video_ram_lookup0;
	if ((c0==1) && (c1==0)) video_ram_lookup=video_ram_lookup1;
	if ((c0==0) && (c1==1)) video_ram_lookup=video_ram_lookup2;
	if ((c0==1) && (c1==1)) video_ram_lookup=video_ram_lookup3;

	// emulation refresh optimisation
	video_refresh=1;
}


/************************************************************************
 * SAA5050 Teletext
 ************************************************************************/

static char *tt_lookup=teletext_characters;
static char *tt_graphics=teletext_graphics;
static int tt_colour=7;
static int tt_bgcolour=0;
static int tt_start_line=0;
static int tt_double_height=0;
static int tt_double_height_set=0;
static int tt_double_height_offset=0;
static int tt_linecount=0;

void teletext_DEW(void)
{
	tt_linecount=9;
	tt_double_height_set=0;
	tt_double_height_offset=0;
}

void teletext_LOSE(void)
{
	tt_lookup=teletext_characters;
	tt_colour=7;
	tt_bgcolour=0;
	tt_graphics=teletext_graphics;
	tt_linecount=(tt_linecount+1)%10;
	tt_start_line=0;
	tt_double_height=0;


	/* this double hight stuff works but has got a bit messy I will have another go and clean it up soon */
	if (tt_linecount==0)
	{
		if (tt_double_height_set==1)
		{
			if (tt_double_height_offset)
			{
				tt_double_height_offset=0;
			} else {
				tt_double_height_offset=10;
			}
		} else {
			tt_double_height_offset=0;
		}
		tt_double_height_set=0;
	}
}




void SAA5050_clock(int code)
{
	int sc1;

	code=code&0x7f;

	switch (code)
	{
		// 0x00 Not used

		case 0x01: case 0x02: case 0x03: case 0x04:
		case 0x05: case 0x06: case 0x07:
			tt_lookup=teletext_characters;
			tt_colour=code;
			break;


		// 0x08		Flash      TO BE DONE
		// 0x09		Steady     TO BE DOME

		// 0x0a		End Box    NOT USED
		// 0x0b     Start Box  NOT USED

		case 0x0c:	// Normal Height
			tt_double_height=0;
			tt_start_line=0;
			break;
		case 0x0d:	// Double Height
			tt_double_height=1;
			tt_double_height_set=1;
			tt_start_line=tt_double_height_offset;
			break;

		// 0x0e		S0         NOT USED
		// 0x0f		S1         NOT USED
		// 0x10		DLE        NOT USED

		case 0x11: case 0x12: case 0x13: case 0x14:
		case 0x15: case 0x16: case 0x17:
			tt_lookup=teletext_graphics;
			tt_colour=code&0x07;
			break;

		// 0x18		Conceal Display

		case 0x19:	//  Contiguois Graphics
			tt_graphics=teletext_graphics;
			if (tt_lookup!=teletext_characters)
				tt_lookup=tt_graphics;
			break;
		case 0x1a:	//  Separated Graphics
			tt_graphics=teletext_separated_graphics;
			if (tt_lookup!=teletext_characters)
				tt_lookup=tt_graphics;
			break;

		// 0x1b		ESC        NOT USED

		case 0x1c:  //  Black Background
			tt_bgcolour=0;
			break;
		case 0x1d:  //  New Background
			tt_bgcolour=tt_colour;
			break;

		// 0x1e		Hold Graphics    TO BE DONE
		// 0x1f		Release Graphics TO BE DONE

	}

	if (code<0x20) {code=0x20;}
	code=(code-0x20)*60+(6*((tt_linecount+tt_start_line)>>tt_double_height));
	for(sc1=0;sc1<6;sc1++)
	{
		BBC_ula_showteletext(tt_lookup[code++]?tt_colour:tt_bgcolour);
	}
}


/************************************************************************
 * VideoULA
 ************************************************************************/

static int videoULA_Reg;
static int videoULA_pallet0[16];// flashing colours A no cursor
static int videoULA_pallet1[16];// flashing colours B no cursor
static int videoULA_pallet2[16];// flashing colours A cursor on
static int videoULA_pallet3[16];// flashing colours B cursor on
static int VideoULA_border_colour;// normally black but can go white when the cursor is out of the normal area

static int *videoULA_pallet_lookup;// holds the pallet now being used.

static int VideoULA_DE=0;          // internal videoULA Display Enabled set by 6845 DE and the scanlines<8
static int VideoULA_CR=0;		   // internal videoULA Cursor Enabled set by 6845 CR and then cleared after a number clock cycles
static int VideoULA_CR_counter=0;  // number of clock cycles left before the CR is disabled


static int videoULA_master_cursor_size;
static int videoULA_width_of_cursor;
static int videoULA_6845_clock_rate;
static int videoULA_characters_per_line;
static int videoULA_teletext_normal_select;
static int videoULA_flash_colour_select;

static unsigned int width_of_cursor_set[4]={ 1,0,2,4 };
static unsigned int pixels_per_byte_set[8]={ 2,4,8,16,1,2,4,8 };
static unsigned int pixels_per_clock_set[4]={ 8,4,2,1 };

static int emulation_pixels_per_character;
static int pixels_per_byte;
static int pixels_per_clock;

void videoULA_select_pallet(void)
{
	if ((!videoULA_flash_colour_select==0) && (!VideoULA_CR)) videoULA_pallet_lookup=videoULA_pallet0;
	if (( videoULA_flash_colour_select==0) && (!VideoULA_CR)) videoULA_pallet_lookup=videoULA_pallet1;
	if ((!videoULA_flash_colour_select==0) && ( VideoULA_CR)) videoULA_pallet_lookup=videoULA_pallet2;
	if (( videoULA_flash_colour_select==0) && ( VideoULA_CR)) videoULA_pallet_lookup=videoULA_pallet3;
	VideoULA_border_colour=VideoULA_CR?Machine->pens[0]:Machine->pens[7];
}


// this is the pixel position of the start of a scanline
// -96 sets the screen display to the middle of emulated screen.
static int x_screen_offset=-96;

static int y_screen_offset=0;

WRITE_HANDLER ( videoULA_w )
{

	int tpal,tcol;

	switch (offset&0x01)
	{
	// Set the control register in the Video ULA
	case 0:
		videoULA_Reg=data;
		videoULA_master_cursor_size=    (videoULA_Reg>>7)&0x01;
		videoULA_width_of_cursor=       (videoULA_Reg>>5)&0x03;
		videoULA_6845_clock_rate=       (videoULA_Reg>>4)&0x01;
		videoULA_characters_per_line=   (videoULA_Reg>>2)&0x03;
		videoULA_teletext_normal_select=(videoULA_Reg>>1)&0x01;
		videoULA_flash_colour_select=    videoULA_Reg    &0x01;
		videoULA_select_pallet();

		if (videoULA_teletext_normal_select)
		{
			emulation_pixels_per_character=18;
			x_screen_offset=-154;
			y_screen_offset=0;
		} else {
			// this is the number of pixels per 6845 character on the emulated screen display
			emulation_pixels_per_character=videoULA_6845_clock_rate?8:16;

			// this is the number of pixels per videoULA clock tick
			pixels_per_byte=pixels_per_byte_set[videoULA_characters_per_line|(videoULA_6845_clock_rate<<2)];
			pixels_per_clock=pixels_per_clock_set[videoULA_characters_per_line];
			x_screen_offset=-96;
			y_screen_offset=0;
		}

		break;
	// Set a pallet register in the Video ULA
	case 1:
		tpal=(data>>4)&0x0f;
		tcol=data&0x0f;
		videoULA_pallet0[tpal]=Machine->pens[tcol];
		videoULA_pallet1[tpal]=tcol>7?Machine->pens[tcol^7]:Machine->pens[tcol];

		videoULA_pallet2[tpal]=Machine->pens[tcol^7];
		videoULA_pallet3[tpal]=tcol>7?Machine->pens[tcol^7^7]:Machine->pens[tcol^7];
		break;
	}


	// emulation refresh optimisation
	video_refresh=1;
}

READ_HANDLER ( videoULA_r )
{
	//logerror("video ULA read register %02x\n",offset);
	return 0;
}



/* this is a quick lookup array that puts bits 0,2,4,6 into bits 0,1,2,3
   this is used by the pallette lookup in the video ULA */
static unsigned char pixel_bits[256];

void set_pixel_lookup(void)
{
	int i;
	for (i=0; i<256; i++)
	{
		pixel_bits[i] = (((i>>7)&1)<<3) | (((i>>5)&1)<<2) | (((i>>3)&1)<<1) | (((i>>1)&1)<<0);
	}
}



static int BBC_HSync=0;
static int BBC_VSync=0;
static int BBC_Character_Row=0;
static int BBC_DE=0;


static unsigned char *BBC_Video_RAM;
static unsigned char *BBC_display;
static struct osd_bitmap *BBC_bitmap;

static int x_screen_pos;
static int y_screen_pos;

static void (*draw_function)(void);

void BBC_draw_hi_res_enabled(void)
{
	int meml;
	unsigned char i=0;
	int sc1,sc2;
	int c=0;
	int pixel_temp=0;

	// read the memory location for the next screen location.
	// the logic for the memory location address is very complicated so it
	// is stored in a number of look up arrays (and is calculated once at the start of the emulator).
	meml=video_ram_lookup[crtc6845_memory_address_r(0)]|(BBC_Character_Row&0x7);
	if (vidmem[meml] || video_refresh )
	{
		vidmem[meml]=0;
		i=BBC_Video_RAM[meml];

		for(sc1=0;sc1<pixels_per_byte;sc1++)
		{
			pixel_temp=videoULA_pallet_lookup[pixel_bits[i]];
			i=(i<<1)|1;
			for(sc2=0;sc2<pixels_per_clock;sc2++)
				BBC_display[c++]=pixel_temp;
		}
	}

}


/*********************
part of the video ula
that recieves the output from the teletext IC
**********************/

static unsigned int teletext_cursor_state=7;

void teletext_set_cursor(void)
{
	teletext_cursor_state=VideoULA_CR?0:7;
}

static int ttx_c;

void BBC_ula_showteletext(int col)
{
	int tcol;
	tcol=Machine->pens[col^teletext_cursor_state];
	BBC_display[ttx_c++]=tcol;
	BBC_display[ttx_c++]=tcol;
	BBC_display[ttx_c++]=tcol;
}



void BBC_draw_teletext_enabled(void)
{
	ttx_c=0;
	SAA5050_clock( BBC_Video_RAM[ video_ram_lookup[crtc6845_memory_address_r(0)-1] ]&0x7f );
}


void BBC_draw_screen_disabled(void)
{
	int sc1;
	if (video_refresh)
	{
		// if the display is not enable, just draw a blank area.
		for(sc1=0;sc1<emulation_pixels_per_character;sc1++)
		{
			BBC_display[sc1]=VideoULA_border_colour;
		}
	}
}

// Select the Function to draw the screen area
void BBC_Set_VideoULA_DE(void)
{
	if (videoULA_teletext_normal_select)
	{
		if (BBC_DE)
		{
			teletext_LOSE();
			draw_function=*BBC_draw_teletext_enabled;
		} else {
			draw_function=*BBC_draw_screen_disabled;
		}
	} else {
		// This line is taking DEN and RA3 from the 6845 and making DISEN for the VideoULA
		// as done by IC41 74LS02
		VideoULA_DE=(BBC_DE) && (!(BBC_Character_Row&8));
		if (VideoULA_DE)
		{
			draw_function=*BBC_draw_hi_res_enabled;
		} else {
			draw_function=*BBC_draw_screen_disabled;
		}
	}
}


/************************************************************************
 * BBC 6845 Outputs to Video ULA
 ************************************************************************/

// called when the 6845 changes the character row
void BBC_Set_Character_Row(int offset, int data)
{
	BBC_Character_Row=data;
	BBC_Set_VideoULA_DE();
}

// called when the 6845 changes the HSync
void BBC_Set_HSync(int offset, int data)
{
	BBC_HSync=data;
	if(!BBC_HSync)
	{
		y_screen_pos+=1;
		x_screen_pos=x_screen_offset;
		BBC_display=(BBC_bitmap->line[y_screen_pos])+x_screen_pos;


	}
}

// called when the 6845 changes the VSync
void BBC_Set_VSync(int offset, int data)
{
	BBC_VSync=data;
	if (!BBC_VSync)
	{
		y_screen_pos=y_screen_offset;
		BBC_display=(BBC_bitmap->line[y_screen_pos])+x_screen_pos;

		teletext_DEW();
	}
}

// called when the 6845 changes the Display Enabled
void BBC_Set_DE(int offset, int data)
{
	BBC_DE=data;
	BBC_Set_VideoULA_DE();

}

// called when the 6845 changes the Cursor Enabled
void BBC_Set_CR(int offset, int data)
{
	if (data) {
		VideoULA_CR_counter=width_of_cursor_set[videoULA_width_of_cursor];
		VideoULA_CR=1;
		// turn on the video refresh for the cursor area
		video_refresh=video_refresh|4;
		// set the pallet to the cursor pallet
		videoULA_select_pallet();
		teletext_set_cursor();
	}
}

// If the cursor is on there is a counter in the VideoULA to control the length of the Cursor
void BBC_Clock_CR(void)
{
	VideoULA_CR_counter-=1;
	if (VideoULA_CR_counter<=0) {
		VideoULA_CR=0;
		video_refresh=video_refresh&0xfb;
		videoULA_select_pallet();
		teletext_set_cursor();
	}
}


static struct crtc6845_interface
BBC6845= {
	0,// Memory Address register
	BBC_Set_Character_Row,// Row Address register
	BBC_Set_HSync,// Horizontal status
	BBC_Set_VSync,// Vertical status
	BBC_Set_DE,// Display Enabled status
	BBC_Set_CR,// Cursor status
};


WRITE_HANDLER ( BBC_6845_w )
{
	switch (offset&1)
	{
		case 0:
			crtc6845_address_w(0,data);
			break;
		case 1:
			crtc6845_register_w(0,data);
			break;
	}

	// emulation refresh optimisation
	video_refresh=1;
}

READ_HANDLER (BBC_6845_r)
{
	int retval=0;

	switch (offset&1)
	{
		case 0:
			break;
		case 1:
			retval=crtc6845_register_r(0);
			break;
	}
	return retval;
}





/************************************************************************
 * bbc_vh_screenrefresh
 * resfresh the BBC video screen
 ************************************************************************/

void bbc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	long c=0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.



	BBC_bitmap=bitmap;
	c=0;

	// video_refresh is set if any of the 6845 or Video ULA registers are changed
	// this then forces a full screen redraw

	if (full_refresh)
	{
		video_refresh=video_refresh|2;
	}

	// loop until the end of the Vertical Sync pulse
	while((BBC_VSync)&&(c<50000))
	{
		// Clock the 6845
		crtc6845_clock();
		c++;
	}


	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!BBC_VSync)&&(c<50000))
	{
		while ((BBC_HSync)&&(c<50000))
		{
			crtc6845_clock();
			c++;
		}
		// Do all the clever split mode changes in here before the next while loop


		while ((!BBC_HSync)&&(c<50000))
		{
			// check that we are on the emulated screen area.
			if ((x_screen_pos>=0) && (x_screen_pos<800) && (y_screen_pos>=0) && (y_screen_pos<300))
			{
				// if the video ULA DE 'Display Enabled' input is high then draw the pixels else blank the screen
				(draw_function)();

			}

			// Move the CRT Beam on one 6845 character distance
			x_screen_pos=x_screen_pos+emulation_pixels_per_character;
			BBC_display=BBC_display+emulation_pixels_per_character;

			// and check the cursor
			if (VideoULA_CR) BBC_Clock_CR();

			// Clock the 6845
			crtc6845_clock();
			c++;
		}
	}

	// redraw the screen so reset video_refresh
	video_refresh=0;

}


/************************************************************************
 * bbc_vh_start
 * Initialize the BBC video emulation
 ************************************************************************/

int bbc_vh_starta(void)
{
	set_pixel_lookup();
	set_video_memory_lookups(16);
	crtc6845_config(&BBC6845);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	draw_function=*BBC_draw_screen_disabled;
	return 0;

}


int bbc_vh_startb(void)
{
	set_pixel_lookup();
	set_video_memory_lookups(32);
	crtc6845_config(&BBC6845);

	BBC_Video_RAM= memory_region(REGION_CPU1);
	draw_function=*BBC_draw_screen_disabled;
	return 0;

}

/************************************************************************
 * bbc_vh_stop
 * Shutdown the BBC video emulation
 ************************************************************************/

void bbc_vh_stop(void)
{

}

