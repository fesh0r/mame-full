
/************************************************************************
	crct6845

	MESS Driver By:

 	Gordon Jefferyes
 	mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/

#include "mess/vidhrdw/m6845.h"

#define True 1
#define False 0

// local copy of the 6845 external procedure calls
static struct crtc6845_interface
crct6845_calls= {
	0,// Memory Address register
	0,// Row Address register
	0,// Horizontal status
	0,// Vertical status
	0,// Display Enabled status
	0,// Cursor status
};

/* set up the local copy of the 6845 external procedure calls */
void crtc6845_config(const struct crtc6845_interface *intf)
{
	crct6845_calls.out_MA_func=*intf->out_MA_func;
	crct6845_calls.out_RA_func=*intf->out_RA_func;
	crct6845_calls.out_HS_func=*intf->out_HS_func;
	crct6845_calls.out_VS_func=*intf->out_VS_func;
	crct6845_calls.out_DE_func=*intf->out_DE_func;
	crct6845_calls.out_CR_func=*intf->out_CR_func;
}


/* 6845 registers */

static int address_register; /* Register Select */

static int R0_horizontal_total;          /* total number of chr -1 */
static int R1_horizontal_displayed;      /* total number of displayed chr */
static int R2_horizontal_sync_position;  /* position of horizontal Sync pulse */
static int R3_sync_width;                /* HSYNC & VSYNC width */
/* only the horizontal sync pulse width is controlled at the moment
some variants of the 6845 used the top 4 bits to set the vertical sync pulse width */

static int R4_vertical_total;            /* total number of character rows -1 */
static int R5_vertical_total_adjust;
/* *** Not implemented yet ***
R5 Vertical total adjust
This 5 bit write only register is programmed with the fraction
for use in conjunction with register R4. It is programmed with
a number of scan lines. If can be varied
slightly in conjunction with R4 to move the whole display area
up or down a little on the screen.
BBC Emulator: It is usually set to 0 except
when using mode 3,6 and 7 in which it is set to 2
*/

static int R6_vertical_displayed;        /* total number of displayed chr rows */
static int R7_vertical_sync_position;    /* position of vertical sync pulse */
static int R8_interlace_display_enabled;
/* *** Part not implemented ***
R8 interlace settings
Interlace mode (bits 0,1)
Bit 1	Bit 0	Description
0		0		Normal (non-interlaced) sync mode
1		0		Normal (non-interlaced) sync mode
0		1		Interlace sync mode
1		1		Interlace sync and video
*/

static int R9_scan_lines_per_character;  /* scan lines per character -1 */
static int R10_cursor_start;
/* *** Part not implemented yet ***
R10 The cursor start register
Bit 6 	Bit 5
0		0		Solid cursor
0		1		No cursor (This no cursor setting is working)
1		0		slow flashing cursor
1		1		fast flashing cursor
*/

static int R11_cursor_end;               /* cursor end row */
static int R12_screen_start_address_H;   /* screen start high */
static int R13_screen_start_address_L;   /* screen start low */
static int R14_cursor_address_H;         /* Cursor address high */
static int R15_cursor_address_L;         /* Cursor address low */
static int R16_light_pen_address_H;      /* *** Not implemented yet *** */
static int R17_light_pen_address_L;      /* *** Not implemented yet *** */

static int screen_start_address;         /* = R12<<8 + R13 */
static int cursor_address;				  /* = R14<<8 + R15 */
//static int light_pen_address;			  /* = R16<<8 + R17 */

static int scan_lines_increment=1;

/* functions to set the 6845 registers */
void crtc6845_address_w(int offset, int data)
{
	address_register=data & 0x1f;
}

void crtc6845_register_w(int offset, int data)
{
	switch (address_register)
	{
		case 0:
			R0_horizontal_total=data;
			break;
		case 1:
			R1_horizontal_displayed=data;
			break;
		case 2:
			R2_horizontal_sync_position=data;
			break;
		case 3:
			R3_sync_width=data;
			break;
		case 4:
			R4_vertical_total=data&0x7f;
			break;
		case 5:
			R5_vertical_total_adjust=data&0x1f;
			break;
		case 6:
			R6_vertical_displayed=data&0x7f;
			break;
		case 7:
			R7_vertical_sync_position=data&0x7f;
			break;
		case 8:
			R8_interlace_display_enabled=data&0xf3;
			scan_lines_increment=((R8_interlace_display_enabled&0x03)==3)?2:1;
			break;
		case 9:
			R9_scan_lines_per_character=data&0x1f;
			break;
		case 10:
			R10_cursor_start=data&0x7f;
			break;
		case 11:
			R11_cursor_end=data&0x1f;
			break;
		case 12:
			R12_screen_start_address_H=data&0x3f;
			screen_start_address=(R12_screen_start_address_H<<8)+R13_screen_start_address_L;
			break;
		case 13:
			R13_screen_start_address_L=data;
			screen_start_address=(R12_screen_start_address_H<<8)+R13_screen_start_address_L;
			break;
		case 14:
			R14_cursor_address_H=data&0x3f;
			cursor_address=(R14_cursor_address_H<<8)+R15_cursor_address_L;
			break;
		case 15:
			R15_cursor_address_L=data;
			cursor_address=(R14_cursor_address_H<<8)+R15_cursor_address_L;
			break;
		case 16:
			/* light pen H  (read only) */
			break;
		case 17:
			/* light pen L  (read only) */
			break;
		default:
			break;
	}
}


int crtc6845_register_r(int offset)
{
	int retval=0;

	switch (address_register)
	{
		case 14:
			retval=R14_cursor_address_H;
			break;
		case 15:
			retval=R15_cursor_address_L;
			break;
		case 16:
			retval=R16_light_pen_address_H;
			break;
		case 17:
			retval=R17_light_pen_address_L;
			break;
		default:
			break;
	}
	return retval;
}


/* other internal registers and counters */

static int Horizontal_Counter=0;
static int Horizontal_Counter_Reset=True;

static int Scan_Line_Counter=0;
static int Scan_Line_Counter_Reset=True;

static int Character_Row_Counter=0;
static int Character_Row_Counter_Reset=True;

static int Horizontal_Sync_Width_Counter=0;
static int Vertical_Sync_Width_Counter=0;

static int HSYNC=False;
static int VSYNC=False;

static int Memory_Address=0;
static int Memory_Address_of_next_Character_Row=0;
static int Memory_Address_of_this_Character_Row=0;

static int Horizontal_Display_Enabled=False;
static int Vertical_Display_Enabled=False;
static int Display_Enabled=False;
static int Display_Delayed_Enabled=False;

static int Cursor_Delayed_Status=False;

static int Delay_Flags=0;
#define Cursor_Start_Delay_Flag 1
#define Cursor_On_Flag 2
#define Display_Enabled_Delay_Flag 4
#define Display_Disable_Delay_Flag 8

static int Cursor_Start_Delay=0;
static int Display_Enabled_Delay=0;
static int Display_Disable_Delay=0;


/* called when the internal horizontal display enabled or the
vertical display enabled changed to set up the real
display enabled output (which may be delayed 0,1 or 2 characters */
void check_display_enabled(void)
{
	int Next_Display_Enabled;


	Next_Display_Enabled=Horizontal_Display_Enabled&Vertical_Display_Enabled;
	if ((Next_Display_Enabled) && (!Display_Enabled))
	{
		Display_Enabled_Delay=(R8_interlace_display_enabled>>4)&0x03;
		if (Display_Enabled_Delay<3)
		{
			Delay_Flags=Delay_Flags | Display_Enabled_Delay_Flag;
		}
	}
	if ((!Next_Display_Enabled) && (Display_Enabled))
	{
		Display_Disable_Delay=(R8_interlace_display_enabled>>4)&0x03;
		Delay_Flags=Delay_Flags | Display_Disable_Delay_Flag;
	}
	Display_Enabled=Next_Display_Enabled;
}

/* clock the 6845 */
void crtc6845_clock(void)
{
	Memory_Address=(Memory_Address+1)%0x4000;

	Horizontal_Counter=(Horizontal_Counter+1)%256;
	if (Horizontal_Counter_Reset)
	{
		/* End of a Horizontal scan line */
		Horizontal_Counter=0;
		Horizontal_Counter_Reset=False;
		Horizontal_Display_Enabled=True;
		check_display_enabled();

		Memory_Address=Memory_Address_of_this_Character_Row;

		/* Vertical clock pulse (R0 CO out) */
		Scan_Line_Counter=(Scan_Line_Counter+scan_lines_increment)%32;
		if (Scan_Line_Counter_Reset)
		{
			/* End of a Vertical Character row */
			Scan_Line_Counter=0;
			Scan_Line_Counter_Reset=False;
			Memory_Address=(Memory_Address_of_this_Character_Row=Memory_Address_of_next_Character_Row);

			/* Character row clock pulse (R9 CO out) */
			Character_Row_Counter=(Character_Row_Counter+1)%128;
			if (Character_Row_Counter_Reset)
			{
				/* End of All Vertical Character rows */
				Character_Row_Counter=0;
				Character_Row_Counter_Reset=False;
				Vertical_Display_Enabled=True;
				check_display_enabled();

				Memory_Address=(Memory_Address_of_this_Character_Row=screen_start_address);
			}
			/* Check for end of All Vertical Character rows */
			if (Character_Row_Counter>=R4_vertical_total)
			{
				Character_Row_Counter_Reset=True;
			}

			/* Check for end of Displayed Vertical Character rows */
			if (Character_Row_Counter==R6_vertical_displayed)
			{
				Vertical_Display_Enabled=False;
				check_display_enabled();
			}

			/* Vertical Sync Clock Pulse (In Vertical control) */
			if (VSYNC)
			{
				Vertical_Sync_Width_Counter+=1;
			}

			/* Check for start of Vertical Sync Pulse */
			if (Character_Row_Counter==R7_vertical_sync_position)
			{
				VSYNC=True;
				if (crct6845_calls.out_VS_func) (crct6845_calls.out_VS_func)(0,VSYNC); /* call VS update */
			}

			/* Check for end of Vertical Sync Pulse */
			if (Vertical_Sync_Width_Counter>=((R3_sync_width>>4)&0xf))
			{
				Vertical_Sync_Width_Counter=0;
				VSYNC=False;
				if (crct6845_calls.out_VS_func) (crct6845_calls.out_VS_func)(0,VSYNC); /* call VS update */
			}

		}

		/* Check for end of Vertical Character Row */
		if (Scan_Line_Counter>=R9_scan_lines_per_character)
		{
			Scan_Line_Counter_Reset=True;
		}
		if (crct6845_calls.out_RA_func) (crct6845_calls.out_RA_func)(0,Scan_Line_Counter); /* call RA update */
	}
	/* end of vertical clock pulse */

	/* Check for end of Horizontal Scan line */
	if (Horizontal_Counter>=R0_horizontal_total)
	{
		Horizontal_Counter_Reset=True;
	}

	/* Check for end of Display Horizontal Scan line */
	if (Horizontal_Counter==R1_horizontal_displayed)
	{
		Memory_Address_of_next_Character_Row=Memory_Address;
		Horizontal_Display_Enabled=False;
		check_display_enabled();
	}

	/* Horizontal Sync Clock Pulse (Clk) */
	if (HSYNC)
	{
		Horizontal_Sync_Width_Counter+=1;
	}

	/* Check for start of Horizontal Sync Pulse */
	if (Horizontal_Counter==R2_horizontal_sync_position)
	{
		HSYNC=True;
		if (crct6845_calls.out_HS_func) (crct6845_calls.out_HS_func)(0,HSYNC); /* call HS update */
	}

	/* Check for end of Horizontal Sync Pulse */
	if (Horizontal_Sync_Width_Counter>=(R3_sync_width&0xf))
	{
		Horizontal_Sync_Width_Counter=0;
		HSYNC=False;
		if (crct6845_calls.out_HS_func) (crct6845_calls.out_HS_func)(0,HSYNC); /* call HS update */
	}

	if (crct6845_calls.out_MA_func) (crct6845_calls.out_MA_func)(0,Memory_Address);	/* call MA update */



	/* *** cursor checks still to be done *** */
	if (Memory_Address==cursor_address)
	{
		if ((Scan_Line_Counter>=(R10_cursor_start&0x1f)) && (Scan_Line_Counter<=R11_cursor_end) && (Display_Enabled))
		{
			Cursor_Start_Delay=(R8_interlace_display_enabled>>6)&0x03;
			if (Cursor_Start_Delay<3) Delay_Flags=Delay_Flags | Cursor_Start_Delay_Flag;
		}
	}


    /* all the cursor and delay flags are stored in one byte so that we can very quickly (for speed) check if anything
       needs doing with them, if any are on then we need to do more longer test to find which ones */
	if (Delay_Flags)
	{
        /* if the cursor is on, then turn it off on the next clock */
		if (Delay_Flags & Cursor_On_Flag)
		{
			Delay_Flags=Delay_Flags^Cursor_On_Flag;
			Cursor_Delayed_Status=False;
			if (crct6845_calls.out_CR_func) (crct6845_calls.out_CR_func)(0,Cursor_Delayed_Status); /* call CR update */
		}

		/* cursor enabled delay */
		if (Delay_Flags & Cursor_Start_Delay_Flag)
		{
			Cursor_Start_Delay-=1;
			if (Cursor_Start_Delay<0)
			{
				if ((R10_cursor_start&0x60)!=0x20)
				{
					Delay_Flags=(Delay_Flags^Cursor_Start_Delay_Flag)|Cursor_On_Flag;
					Cursor_Delayed_Status=True;
					if (crct6845_calls.out_CR_func) (crct6845_calls.out_CR_func)(0,Cursor_Delayed_Status); /* call CR update */
				}
			}
		}

    	/* display enabled delay */
		if (Delay_Flags & Display_Enabled_Delay_Flag)
		{
			Display_Enabled_Delay-=1;
			if (Display_Enabled_Delay<0)
			{
				Delay_Flags=Delay_Flags^Display_Enabled_Delay_Flag;
				Display_Delayed_Enabled=True;
				if (crct6845_calls.out_DE_func) (crct6845_calls.out_DE_func)(0,Display_Delayed_Enabled); /* call DE update */
			}
		}

		/* display disable delay */
		if (Delay_Flags & Display_Disable_Delay_Flag)
		{
			Display_Disable_Delay-=1;
			if (Display_Disable_Delay<0)
			{
				Delay_Flags=Delay_Flags^Display_Disable_Delay_Flag;
				Display_Delayed_Enabled=False;
				if (crct6845_calls.out_DE_func) (crct6845_calls.out_DE_func)(0,Display_Delayed_Enabled); /* call DE update */
			}
		}
	}

}

/* functions to read the 6845 outputs */

int crtc6845_memory_address_r(int offset)  { return Memory_Address; }    /* MA = Memory Address output */
int crtc6845_row_address_r(int offset)     { return Scan_Line_Counter; } /* RA = Row Address output */
int crtc6845_horizontal_sync_r(int offset) { return HSYNC; }             /* HS = Horizontal Sync */
int crtc6845_vertical_sync_r(int offset)   { return VSYNC; }             /* VS = Vertical Sync */
int crtc6845_display_enabled_r(int offset) { return Display_Delayed_Enabled; }   /* DE = Display Enabled */
int crtc6845_cursor_enabled_r(int offset)  { return Cursor_Delayed_Status; }             /* CR = Cursor Enabled */


