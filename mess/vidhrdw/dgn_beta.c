/*
	vidhrdw/dgn_beta.c
new
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6845.h"
#include "mscommon.h"

#include "includes/dgn_beta.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#endif

//#define LOG_VIDEO

/* Names for 6845 regs, to save having to remember their offsets */
/* perhaps these should move into 6845 header ? */
typedef enum {
	H_TOTAL = 0,		// Horizontal total
	H_DISPLAYED,		// Horizontal displayed
	H_SYNC_POS,		// Horizontal sync pos
	H_SYNC_WIDTH,		// Horizontal sync width
	V_TOTAL,		// Vertical total
	V_TOTAL_ADJ,		// Vertical total adjust
	V_DISPLAYED,		// Vertical displayed
	V_SYNC_POS,		// Vertical sync pos
	INTERLACE,		// Interlace 
	MAX_SCAN,		// Maximum scan line
	CURS_START,		// Cursor start pos
	CURS_END,		// Cursor end pos
	START_ADDR_H,		// Start address High
	START_ADDR_L,		// Start address low
	CURS_H,			// Cursor addr High
	CURS_L			// CURSOR addr Low
} crtc6845_regs;

static int beta_6845_RA = 0;
static int beta_scr_x   = 0;
static int beta_scr_y   = 0;
static int beta_HSync   = 0;
static int beta_VSync   = 0;
static int beta_DE      = 0;

//static BETA_VID_MODES VIDMODE = TEXT_40x25;

#ifdef MAME_DEBUG
static int LogRegWrites	= 0;	// Log register writes to debug console.
static int VidAddr		= 0;	// Last address reg written
static int BoxColour	= 1;
static int BoxMinX	= 100;
static int BoxMinY	= 100;
static int BoxMaxX	= 500;
static int BoxMaxY	= 500;
#endif /* MAME_DEBUG */

static int NoScreen		= 0;

static void beta_Set_RA(int offset, int data);
static void beta_Set_HSync(int offset, int data);
static void beta_Set_VSync(int offset, int data);
static void beta_Set_DE(int offset, int data);

static void ToggleRegLog(int ref, int params, const char *param[]);
static void RegLog(int offset, int data);
static void FillScreen(int ref, int params, const char *param[]);
static void ScreenBox(int ref, int params, const char *param[]);
static void VidToggle(int ref, int params, const char *param[]);
static void ShowVidLimits(int ref, int params, const char *param[]);

static mame_bitmap	*bit;
static int MinAddr	= 0xFFFF;
static int MaxAddr	= 0x0000;
static int MinX	= 0xFFFF;
static int MaxX	= 0x0000;
static int MinY	= 0xFFFF;
static int MaxY	= 0x0000;

static struct crtc6845_interface
beta_crtc6845_interface= {
	0,		// Memory Address register
	beta_Set_RA,	// Row Address register
	beta_Set_HSync,	// Horizontal status
	beta_Set_VSync,	// Vertical status
	beta_Set_DE,	// Display Enabled status
	0,		// Cursor status
};

//static int beta_state;

// called when the 6845 changes the character row
static void beta_Set_RA(int offset, int data)
{
	beta_6845_RA=data;
}

// called when the 6845 changes the HSync
static void beta_Set_HSync(int offset, int data)
{
	beta_HSync=data;
	if(!beta_HSync)
	{
		int HT=crtc6845_get_register(H_TOTAL);			// Get H total
		int HS=crtc6845_get_register(H_SYNC_POS);		// Get Hsync pos
		int HW=crtc6845_get_register(H_SYNC_WIDTH)&0xF;	// Hsync width (in chars)
		
		beta_scr_y++;
		beta_scr_x=0-((HT-HS-HW)*8);	// Number of dots after HS to wait before start of next line
	}
}

// called when the 6845 changes the VSync
static void beta_Set_VSync(int offset, int data)
{
	beta_VSync=data;
	if (!beta_VSync)
	{
		beta_scr_y = 0;
	}
	dgn_beta_frame_interrupt(data);
}

static void beta_Set_DE(int offset, int data)
{
	beta_DE = data;
	if(NoScreen)
		beta_DE=0;
}

/* Video init */
void init_video(void)
{
	/* initialise 6845 */
	crtc6845_config(&beta_crtc6845_interface);
	crtc6845_set_personality(M6845_PERSONALITY_HD6845S);
	
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	/* setup debug commands */
	debug_console_register_command("beta_vid_log", CMDFLAG_NONE, 0, 0, 0,ToggleRegLog);
	debug_console_register_command("beta_vid_fill", CMDFLAG_NONE, 0, 0, 0,FillScreen);
	debug_console_register_command("beta_vid_box", CMDFLAG_NONE, 0, 0, 5,ScreenBox);
	debug_console_register_command("beta_vid", CMDFLAG_NONE, 0, 0, 0,VidToggle);
	debug_console_register_command("beta_vid_limits", CMDFLAG_NONE, 0, 0, 0,ShowVidLimits);
	LogRegWrites=0;
#endif /* defined(MAME_DEBUG) && defined(NEW_DEBUGGER) */
}

static void beta_plot_char_line(int x,int y, mame_bitmap *bitmap)
{
	int CharsPerLine	= crtc6845_get_register(H_DISPLAYED);	// Get chars per line.
	unsigned char *data 	= memory_region(REGION_GFX1);
	int Dot;
	int ScreenX;
	unsigned char data_byte;
	int char_code;
	int crtcAddr;
	int Offset;
	int FgColour;
	int BgColour;
	int Colour;

	bit=bitmap;

	if(x>MaxX) MaxX=x;
	if(x<MinX) MinX=x;
	if(y>MaxY) MaxY=y;
	if(y<MinY) MinY=y;
	
	/* We do this so that we can plot 40 column characters twice as wide */
	if(CharsPerLine==40)
		ScreenX=x*2;
	else
		ScreenX=x;
		
	if (beta_DE)
	{
		
		/* The beta text RAM contains alternate character and attribute bytes */
		crtcAddr=crtc6845_memory_address_r(0)-1;
		Offset=crtcAddr*2;
		
		if(crtcAddr<MinAddr) 
			MinAddr=crtcAddr;

		if(crtcAddr>MaxAddr) 
			MaxAddr=crtcAddr;
		
		char_code 	= videoram[Offset];
		FgColour	= (videoram[Offset+1] & 0x38) >> 3;
		BgColour	= (videoram[Offset+1] & 0x07);
		
		/* The beta Character ROM has characters of 8x10, each aligned to a 16 byte boundry */
		data_byte = data[(char_code*16) + beta_6845_RA];

		for (Dot=0; Dot<8; Dot++)
		{
			if (data_byte & 0x080)
				Colour=FgColour;
			else
				Colour=BgColour;
			
			/* Plot characters twice as wide in 40 col mode */
			if(CharsPerLine==40)
			{
				plot_pixel(bitmap, (ScreenX+(Dot*2)), y,Colour);
				plot_pixel(bitmap, (ScreenX+(Dot*2)+1), y,Colour);
			}
			else
				plot_pixel(bitmap, (ScreenX+Dot), y,Colour);
			
			data_byte = data_byte<<1;
		}
	}
	else
	{
		plot_pixel(bitmap, ScreenX+0, y, 1);
		plot_pixel(bitmap, ScreenX+1, y, 1);
		plot_pixel(bitmap, ScreenX+2, y, 1);
		plot_pixel(bitmap, ScreenX+3, y, 1);
		plot_pixel(bitmap, ScreenX+4, y, 1);
		plot_pixel(bitmap, ScreenX+5, y, 1);
		plot_pixel(bitmap, ScreenX+6, y, 1);
		plot_pixel(bitmap, ScreenX+7, y, 1);
	}

}

VIDEO_UPDATE( dgnbeta )
{	
	long c=0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.
	
	bit=bitmap;
	
//	ScreenBox(0,0,(char **)0);
	
//	return 0;

	c=0;

	// loop until the end of the Vertical Sync pulse
	while((beta_VSync)&&(c<33274))
	{
		// Clock the 6845
		crtc6845_clock();
		c++;
	}

	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!beta_VSync)&&(c<33274))
	{
		while ((beta_HSync)&&(c<33274))
		{
			crtc6845_clock();
			c++;
		}
		// Do all the clever split mode changes in here before the next while loop

		while ((!beta_HSync)&&(c<33274))
		{
			// check that we are on the emulated screen area.
			if ((beta_scr_x>=0) && (beta_scr_x<900) && (beta_scr_y>=0) && (beta_scr_y<800))
			{
				beta_plot_char_line(beta_scr_x, beta_scr_y, bitmap);
			}

			beta_scr_x+=8;

			// Clock the 6845
			crtc6845_clock();
			c++;
		}
	}
	return 0;
}

READ8_HANDLER(dgnbeta_6845_r)
{
	return crtc6845_register_r(offset);
}

WRITE8_HANDLER(dgnbeta_6845_w)
{
	if(offset&0x1)
	{
		crtc6845_register_w(offset,data);
	}
	else
	{
		crtc6845_address_w(offset,data);
	}
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	if (LogRegWrites) 
		RegLog(offset,data);
#endif
}

/*************************************
 *
 *  Debugging
 *
 *************************************/

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
static void ToggleRegLog(int ref, int params, const char *param[])
{
	LogRegWrites=!LogRegWrites;

	debug_console_printf("6845 register write info set : %d\n",LogRegWrites);
}


static void RegLog(int offset, int data)
{
	char	RegName[16];

	switch (VidAddr) 
	{
		case H_TOTAL		: sprintf(RegName,"H Total      "); break;
		case H_DISPLAYED	: sprintf(RegName,"H Displayed  "); break;
		case H_SYNC_POS		: sprintf(RegName,"H Sync Pos   "); break;
		case H_SYNC_WIDTH	: sprintf(RegName,"H Sync Width "); break;
		case V_TOTAL		: sprintf(RegName,"V Total      "); break;
		case V_TOTAL_ADJ	: sprintf(RegName,"V Total Adj  "); break;
		case V_DISPLAYED	: sprintf(RegName,"V Displayed  "); break;
		case V_SYNC_POS		: sprintf(RegName,"V Sync Pos   "); break;
		case INTERLACE		: sprintf(RegName,"Interlace    "); break;
		case MAX_SCAN		: sprintf(RegName,"Max Scan Line"); break;
		case CURS_START		: sprintf(RegName,"Cusror Start "); break;
		case CURS_END		: sprintf(RegName,"Cusrsor End  "); break;
		case START_ADDR_H	: sprintf(RegName,"Start Addr H "); break;
		case START_ADDR_L	: sprintf(RegName,"Start Addr L "); break;
		case CURS_H		: sprintf(RegName,"Cursor H     "); break;
		case CURS_L		: sprintf(RegName,"Cursor L     "); break;
	}	
	
	if(offset&0x1)
		debug_console_printf("6845 write Reg %s Addr=%3d Data=%3d\n",RegName,VidAddr,data);
	else
		VidAddr=data;
}

static void FillScreen(int ref, int params, const char *param[])
{
	int	x,y;
	
	for(x=1;x<899;x++)
	{
		for(y=1;y<699;y++)
		{
			plot_pixel(bit,x,y,x&0x7);
		}
	}
	NoScreen=1;
}

static void ScreenBox(int ref, int params, const char *param[])
{
	int	x,y;
	
	if(params>0)	sscanf(param[0],"%d",&BoxMinX);
	if(params>1)	sscanf(param[1],"%d",&BoxMinY);
	if(params>2)	sscanf(param[2],"%d",&BoxMaxX);
	if(params>3)	sscanf(param[3],"%d",&BoxMaxY);
	if(params>4)	sscanf(param[4],"%d",&BoxColour);
	
	for(x=BoxMinX;x<BoxMaxX;x++)
	{
		plot_pixel(bit,x,BoxMinY,BoxColour);
		plot_pixel(bit,x,BoxMaxY,BoxColour);
	}

	for(y=BoxMinY;y<BoxMaxY;y++)
	{
		plot_pixel(bit,BoxMinX,y,BoxColour);
		plot_pixel(bit,BoxMaxX,y,BoxColour);
	}
	debug_console_printf("ScreenBox()\n");
}


static void VidToggle(int ref, int params, const char *param[])
{
	NoScreen=!NoScreen;
}

static void ShowVidLimits(int ref, int params, const char *param[])
{
	debug_console_printf("Min X     =%4X, Max X     =%4X\n",MinX,MaxX);
	debug_console_printf("Min Y     =%4X, Max Y     =%4X\n",MinY,MaxY);
	debug_console_printf("MinVidAddr=%4X, MaxVidAddr=%4X\n",MinAddr,MaxAddr);
}
#endif
