/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "mess/machine/bbc.h"
#include "mess/vidhrdw/bbc.h"

/************************************************************************
 * reg_refresh flag is used in optimising the screen redrawing
 * it is set whenever a 6845 or VideoULA register is change.
 * This will then cause a full screen refresh.
 ************************************************************************/

unsigned char vidmem[0x8000];

static int reg_refresh=0xff;

/************************************************************************
 * bbc_vh_start
 * Initialize the BBC video emulation
 ************************************************************************/

static unsigned char pixel0[256];
static unsigned char pixel1[256];
static unsigned char pixel2[256];
static unsigned char pixel3[256];
static unsigned char pixel4[256];
static unsigned char pixel5[256];
static unsigned char pixel6[256];
static unsigned char pixel7[256];


int bbc_vh_start(void)
{
	int i;

	for (i=0; i<256; i++)
	{
		pixel0[i] = (((i>>7)&1)<<3) | (((i>>5)&1)<<2) | (((i>>3)&1)<<1) | (((i>>1)&1)<<0);
		pixel1[i] = (((i>>6)&1)<<3) | (((i>>4)&1)<<2) | (((i>>2)&1)<<1) | (((i>>0)&1)<<0);
		pixel2[i] = (((i>>5)&1)<<3) | (((i>>3)&1)<<2) | (((i>>1)&1)<<1) | ((       1)<<0);
		pixel3[i] = (((i>>4)&1)<<3) | (((i>>2)&1)<<2) | (((i>>0)&1)<<1) | ((       1)<<0);
		pixel4[i] = (((i>>3)&1)<<3) | (((i>>1)&1)<<2) | ((       1)<<1) | ((       1)<<0);
		pixel5[i] = (((i>>2)&1)<<3) | (((i>>0)&1)<<2) | ((       1)<<1) | ((       1)<<0);
		pixel6[i] = (((i>>1)&1)<<3) | ((       1)<<2) | ((       1)<<1) | ((       1)<<0);
		pixel7[i] = (((i>>0)&1)<<3) | ((       1)<<2) | ((       1)<<1) | ((       1)<<0);

	}

	return 0;

}



/************************************************************************
 * bbc_vh_init
 * Initialize the BBC video emulation
 ************************************************************************/

int bbc_vh_init(void)
{

    return 0;
}


/************************************************************************
 * bbc_vh_stop
 * Shutdown the BBC video emulation
 ************************************************************************/

void bbc_vh_stop(void)
{

}


/************************************************************************
 * crct6845
 ************************************************************************/

static unsigned int crct6845reg[17];
static unsigned int crct6845reg_mask[16]={ 0xff,0xff,0xff,0xff,0x7f,0x1f,0x7f,0x7f,0xff,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff };
static unsigned int crct6845reg_no;

WRITE_HANDLER ( crtc6845_w )
{
	if (errorlog) { fprintf(errorlog, "crct6845 write register %d = %d\n",offset,data); };
	switch (offset&0x01)
	{
	case 0:
		crct6845reg_no=(data & 0x1f);
		break;
	case 1:
	    if ((crct6845reg_no<16) && (crct6845reg[crct6845reg_no]!=(data & crct6845reg_mask[crct6845reg_no])))
	    {
			crct6845reg[crct6845reg_no]=(data & crct6845reg_mask[crct6845reg_no]);
			reg_refresh|=1;
		};
		break;
	};
}

READ_HANDLER ( crtc6845_r )
{
	int result=0;
	switch (offset&0x01)
	{
	case 0:
		break;
	case 1:
		switch (crct6845reg_no)
		{
			case 14:
			case 15:
				result=crct6845reg[crct6845reg_no];
				break;
		};
		break;
	};
	return result;
}


/************************************************************************
 * VideoULA
 * Palette register
 ************************************************************************/

static int VideoULAReg;
static int VideoULAPallet[16];

WRITE_HANDLER ( videoULA_w )
{
	//if (errorlog) { fprintf(errorlog, "video ULA write register %d = %d\n",offset,data); };
	switch (offset&0x01)
	{
		case 0:
			if (VideoULAReg!=data)
			{
				VideoULAReg=data;
				reg_refresh|=2;
			};
			break;
		case 1:
			if (VideoULAPallet[data>>4]!=(data&0x0f))
			{
				VideoULAPallet[data>>4]=data&0x0f;
				reg_refresh|=2;
			};
			break;
	};
}

READ_HANDLER ( videoULA_r )
{
	//if (errorlog) { fprintf(errorlog, "video ULA read register %02x\n",offset); }
	return 0;
}


/************************************************************************
 * Hardware scroll settings
 * called from 6522 system via
 ************************************************************************/

static unsigned int hardware_screenstart;
static unsigned int hardware_screenstart_lookup[4]=
{
	0x4000,
	0x6000,
	0x3000,
	0x5800
};

void setscreenstart(int b4,int b5)
{
		hardware_screenstart=hardware_screenstart_lookup[b4+(b5<<1)];
		reg_refresh=1;
	//	if (errorlog) { fprintf(errorlog, "screen hardware scroll %04x\n",hardware_screenstart); };
}


/************************************************************************
 * bbc_vh_screenrefresh
 * resfresh the BBC video screen
 ************************************************************************/

/* total number of 6845 characters in a full scanline minus one */
#define horizontal_total 0
/* total number of 6845 characters that are displayed on the screen in a scanline */
#define horizontal_displayed 1
/* position in the scan line of the sync pulse measured in 6845 characters */
#define horizontal_sync_position 2
/* with of the sync pulse measured in 6845 characters */
#define sync_width 3
/* total number of character lines in a full screen minus one */
#define vertical_total 4
/* number of extra scan lines to be added to get the full number of required scan lines */
#define vertical_total_adjust 5
/* total number of character lines that are displayed on the screen */
#define vertical_displayed 6
/* position of the vertical synce pulse measured in character lines */
#define vertical_sync_position 7

#define interlace_and_delay_register 8
/* total number of scan lines per character minus one */
#define scan_lines_per_character 9

/* The cursor start register
	7 bit register
bit 7 is not used
bit 6 enables or disables the blink feature
bit 5 0=16 times the refresh rate, 1=32 times the refresh rate
bits 0 to 4 set the cursor start line */
#define cursor_start_register 10
/* The cursor end register
    5 bit register
bits 0 to 4 set the cursor end line */
#define cursor_end_register 11
#define screen_start_address_high 12
#define screen_start_address_low 13
#define cursor_position_high 14
#define cursor_position_low 15

#define y_screen_offset 14

static int vertical_scan_lines;
static int vertical_sync_postion_in_scan_lines;
static int vertical_scan_lines_displayed;
static int video_memory_start;

static int mode_type;
static int clock_speed;
static int emulation_screen_width;
static int emulation_pixels_per_character;
static int x_screen_offset;

static int pallet1[16];
static int lookupx[100];
static int lookupxf[100];
static int lookupy[300];
static int lookupyf[300];

void bbc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{

unsigned char *display;
unsigned char *RAM = memory_region(REGION_CPU1);

int x_screen_pos;
int y_screen_pos;
int y_line_pos;


int video_memory_line;
int video_memory;

int pixel_temp;

int ploop;
unsigned char i;


if (full_refresh)
{
	reg_refresh|=8;
};

/* only do this bit if the video ULA registers have been updated */
if (reg_refresh&2)
{
	mode_type=(VideoULAReg>>2)&0x07;
	clock_speed=(VideoULAReg>>4)&1;
	emulation_screen_width=clock_speed?100:50;
	emulation_pixels_per_character=clock_speed?8:16;
	x_screen_offset=clock_speed?20:10;

	for(ploop=0;ploop<=15;ploop++)
	{

		if ((VideoULAReg&1) && (VideoULAPallet[ploop]>7))
			pallet1[ploop]=Machine->pens[(VideoULAPallet[ploop]&7)^7];
		else
			pallet1[ploop]=Machine->pens[VideoULAPallet[ploop]&7];
	}
};


/* only do this bit if the 6845 registers have been updated */
if (reg_refresh&1)
{
	vertical_scan_lines=(crct6845reg[vertical_total]+1)*(crct6845reg[scan_lines_per_character]+1)+crct6845reg[vertical_total_adjust];
	vertical_sync_postion_in_scan_lines=crct6845reg[vertical_sync_position]*(crct6845reg[scan_lines_per_character]+1);
	vertical_scan_lines_displayed=crct6845reg[vertical_displayed]*(crct6845reg[scan_lines_per_character]+1);

	video_memory_start=((crct6845reg[screen_start_address_high]<<11)+(crct6845reg[screen_start_address_low]<<3));

	for(y_screen_pos=0;y_screen_pos<300;y_screen_pos++)
	{
		y_line_pos=(vertical_sync_postion_in_scan_lines+y_screen_pos+y_screen_offset)%vertical_scan_lines;
		lookupyf[y_screen_pos]=((y_line_pos>=0) && (y_line_pos<vertical_scan_lines_displayed) && ((y_line_pos%(crct6845reg[scan_lines_per_character]+1))<8));
		if (lookupyf[y_screen_pos])
			lookupy[y_screen_pos]=video_memory_start+(y_line_pos%(crct6845reg[scan_lines_per_character]+1))+((y_line_pos/(crct6845reg[scan_lines_per_character]+1))*(crct6845reg[horizontal_displayed]*8));
	};
	for(x_screen_pos=0;x_screen_pos<100;x_screen_pos++)
	{
		lookupx[x_screen_pos]=(crct6845reg[horizontal_sync_position]+x_screen_pos+x_screen_offset)%(crct6845reg[horizontal_total]+1);
		lookupxf[x_screen_pos]=((lookupx[x_screen_pos]>=0) && (lookupx[x_screen_pos]<crct6845reg[horizontal_displayed]));
		lookupx[x_screen_pos]<<=3;
	};
};

for(y_screen_pos=0;y_screen_pos<300;y_screen_pos++)
{

	display=bitmap->line[y_screen_pos];
	if (lookupyf[y_screen_pos])
	{

		video_memory_line=lookupy[y_screen_pos];
		for(x_screen_pos=0;x_screen_pos<emulation_screen_width;x_screen_pos++)
		{
			if (lookupxf[x_screen_pos])
			{
				if ((video_memory=(video_memory_line+lookupx[x_screen_pos]))>=0x8000)
					video_memory=((video_memory &0x7fff)+hardware_screenstart)&0x7fff;

				if ((reg_refresh) || (vidmem[video_memory]))
				{
					vidmem[video_memory]=0;
					i=RAM[video_memory];
					switch (mode_type) {
					case 0:  //Mode 8
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=pixel_temp;
						display[3]=pixel_temp;
						display[4]=pixel_temp;
						display[5]=pixel_temp;
						display[6]=pixel_temp;
						display[7]=pixel_temp;
						display[8]=(pixel_temp=pallet1[pixel1[i]]);
						display[9]=pixel_temp;
						display[10]=pixel_temp;
						display[11]=pixel_temp;
						display[12]=pixel_temp;
						display[13]=pixel_temp;
						display[14]=pixel_temp;
						display[15]=pixel_temp;
						break;
					case 1:  //Mode 5
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=pixel_temp;
						display[3]=pixel_temp;
						display[4]=(pixel_temp=pallet1[pixel1[i]]);
						display[5]=pixel_temp;
						display[6]=pixel_temp;
						display[7]=pixel_temp;
						display[8]=(pixel_temp=pallet1[pixel2[i]]);
						display[9]=pixel_temp;
						display[10]=pixel_temp;
						display[11]=pixel_temp;
						display[12]=(pixel_temp=pallet1[pixel3[i]]);
						display[13]=pixel_temp;
						display[14]=pixel_temp;
						display[15]=pixel_temp;
						break;
					case 2:  //Mode 4,6
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=(pixel_temp=pallet1[pixel1[i]]);
						display[3]=pixel_temp;
						display[4]=(pixel_temp=pallet1[pixel2[i]]);
						display[5]=pixel_temp;
						display[6]=(pixel_temp=pallet1[pixel3[i]]);
						display[7]=pixel_temp;
						display[8]=(pixel_temp=pallet1[pixel4[i]]);
						display[9]=pixel_temp;
						display[10]=(pixel_temp=pallet1[pixel5[i]]);
						display[11]=pixel_temp;
						display[12]=(pixel_temp=pallet1[pixel6[i]]);
						display[13]=pixel_temp;
						display[14]=(pixel_temp=pallet1[pixel7[i]]);
						display[15]=pixel_temp;
						break;
					case 3:	 //Not a valid mode
						display[0]=pallet1[pixel0[i]];
						display[1]=pallet1[pixel1[i]];
						display[2]=pallet1[pixel2[i]];
						display[3]=pallet1[pixel3[i]];
						display[4]=pallet1[pixel4[i]];
						display[5]=pallet1[pixel5[i]];
						display[6]=pallet1[pixel6[i]];
						display[7]=pallet1[pixel7[i]];
						display[8]=pallet1[15];
						display[9]=pallet1[15];
						display[10]=pallet1[15];
						display[11]=pallet1[15];
						display[12]=pallet1[15];
						display[13]=pallet1[15];
						display[14]=pallet1[15];
						display[15]=pallet1[15];
						break;
					case 4:  //Not a valid mode
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=pixel_temp;
						display[3]=pixel_temp;
						display[4]=pixel_temp;
						display[5]=pixel_temp;
						display[6]=pixel_temp;
						display[7]=pixel_temp;
						break;
					case 5:  //Mode 2
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=pixel_temp;
						display[3]=pixel_temp;
						display[4]=(pixel_temp=pallet1[pixel1[i]]);
						display[5]=pixel_temp;
						display[6]=pixel_temp;
						display[7]=pixel_temp;
						break;
					case 6:  //Mode 1
						display[0]=(pixel_temp=pallet1[pixel0[i]]);
						display[1]=pixel_temp;
						display[2]=(pixel_temp=pallet1[pixel1[i]]);
						display[3]=pixel_temp;
						display[4]=(pixel_temp=pallet1[pixel2[i]]);
						display[5]=pixel_temp;
						display[6]=(pixel_temp=pallet1[pixel3[i]]);
						display[7]=pixel_temp;
						break;
					case 7:  //Mode 0,3
						display[0]=pallet1[pixel0[i]];
						display[1]=pallet1[pixel1[i]];
						display[2]=pallet1[pixel2[i]];
						display[3]=pallet1[pixel3[i]];
						display[4]=pallet1[pixel4[i]];
						display[5]=pallet1[pixel5[i]];
						display[6]=pallet1[pixel6[i]];
						display[7]=pallet1[pixel7[i]];
						break;
					};
				};
			} else {
				if (reg_refresh&9)
				{
					pixel_temp=Machine->pens[8];
					display[0]=pixel_temp;
					display[1]=pixel_temp;
					display[2]=pixel_temp;
					display[3]=pixel_temp;
					display[4]=pixel_temp;
					display[5]=pixel_temp;
					display[6]=pixel_temp;
					display[7]=pixel_temp;
					if (emulation_pixels_per_character==16) {
						display[8]=pixel_temp;
						display[9]=pixel_temp;
						display[10]=pixel_temp;
						display[11]=pixel_temp;
						display[12]=pixel_temp;
						display[13]=pixel_temp;
						display[14]=pixel_temp;
						display[15]=pixel_temp;
					};
				};
			};
			display+=emulation_pixels_per_character;
		};
	} else {
		if (reg_refresh&9)
		{
			pixel_temp=Machine->pens[8];
			for(x_screen_pos=0;x_screen_pos<50;x_screen_pos++)
			{
				display[0]=pixel_temp;
				display[1]=pixel_temp;
				display[2]=pixel_temp;
				display[3]=pixel_temp;
				display[4]=pixel_temp;
				display[5]=pixel_temp;
				display[6]=pixel_temp;
				display[7]=pixel_temp;
				display[8]=pixel_temp;
				display[9]=pixel_temp;
				display[10]=pixel_temp;
				display[11]=pixel_temp;
				display[12]=pixel_temp;
				display[13]=pixel_temp;
				display[14]=pixel_temp;
				display[15]=pixel_temp;
				display+=16;
			};
		};
	};
};

reg_refresh=0;


}
