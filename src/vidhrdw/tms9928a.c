/***************************************************************************

  vidhrdw/TMS9928A.c

  TODO:
	- Finish TMS9918A/TMS9928A emulation
	- Start V9938, V9958 emulation
	- Clean up code
	- Speed up code

  Hopefully, this will eventually fully emulate the 99xx series of
  video chips.	This series contains the TMS9918A and TMS9928A from
  Texas Instruments, and the V9938, V9958, and V9990 from Yamaha.  All
  of these chips are backward-compatible, so emulating the V9990 provides
  an emulation for every chip in this series.

  These chips are used in the ColecoVision, the Coleco Adam, the TI99/4A,
  and the MSX and MSX2 computers.  A variation is used in the Sega Master System.

  Recommended reading for these chips:
  1)  V9938 MSX-Video Technical Data Book (Yamaha).
		* The best reference for this chip, obviously.
  2)  MSX2 Technical Manual (WWW)
		* Contains a few inaccuracies, but is mostly helpful.
  3)  "High-Resolution Sprite-Oriented Color Graphics (Byte, Aug 82).
		* Contains many inaccuracies and incomplete descriptions, but
		  it's a good introduction to the chip.

  There are 8 Control Registers in the 9928, these are write-only.
  Reg	D7	D6	D5	D4	D3	D2	D1	D0
  0 	0	DG	0	0	M5	M4	M3	0
  1 	0	BLK IE0 M1	M2	0	SZ	MAG
  2 	0	A16 A15 A14 A13 A12 A11 A10
  3 	B13 B12 B11 B10 B9	B8	B7	B6
  4 	0	0	C16 C15 C14 C13 C12 C11
  5 	D14 D13 D12 D11 D10 D9	D8	D7
  6 	0	0	E16 E15 E14 E13 E12 E11
  7 	TC3 TC2 TC1 TC0 BC3 BC2 BC1 BC0

  DG = Digitize (use external input)
  IE0 = Enable Vertical Retrace Interrupt
  M1-M5 = Screen mode select
		M1	M2	M5	M4	M3
		1	0	0	0	0	Screen 0 (WIDTH 40)
		0	0	0	0	0	Screen 1
		0	0	0	0	1	Screen 2
		0	1	0	0	0	Screen 3
  BLK = 0:Disable Display, 1:Enable Display
  SZ = 0:Sprite size 8x8, 1:Sprite size 16x16
  MAG = 0:Sprite magnification 1x, 1:Sprite magnification 2x
  A10-A16 - Pattern name table base address
  B6-B13 - Color table base address
  C11-C16 - Pattern generator table base address
  D7-D14 - Sprite attribute table base address
  E11-E16 - Sprite pattern generator table base address
  TC0-TC3 - Text color
  BC0-BC3 - Background color


  There is 1 status register in the 9928.  It is read-only.
  SReg	D7	D6	D5	D4	D3	D2	D1	D0
  0 	F	SD	C	S4	S3	S2	S1	S0

  F = Vertical scanning interrupt flag (1:interrupt)
  SD = 5th sprite detected
  C = sprite collision detected
  S0-S4 = sprite number of 5th sprite


  - Display resolution = 256x192
  - Has 16 colors (Color 0 = Transparent)
  - Supports 16K of VRAM
  - 32 sprites
  - Four display modes:
	Mode 1: Graphics I (256x192, limited color)
	Mode 2: Graphics II (256x192, extended color)
	Mode 3: Text (40x24 of user-defined characters)
	Mode 4: Multicolor (64x48 low-resolution positions)
  - Allows an external video input

  It draws the screen in the following order:
  - Fills with black
  - "Draws" external video input
  - Backdrop plane (solid color)
  - Pattern plane
  - Sprites

  Here's what we know about sprites:
  - A global size switch sets sprite sizes to either 8x8 or 16x16
  - A global 2x magnification switch doubles sprite sizes to 16x16 or 32x32
  - Sprites are a single color
  - Only 4 sprites can occupy a single horizontal scanline.  If a fifth
	sprite is on the line, the conflicting parts of the lowest-priority
	sprite will be made transparent.  The number of the fifth sprite 
	will appear in the status register.

  Colors:
  0 = transparent
  1 = black
  2 = medium green
  3 = light green
  4 = dark blue
  5 = light blue
  6 = dark red
  7 = cyan
  8 = medium red
  9 = light red
  A = dark yellow
  B = light yellow
  C = dark green
  D = magenta
  E = gray
  F = white


  SGT = Sprite Generator Table (can be 32 bytes * 32 sprites = $400 bytes)
		This table contains the bits for the 8x8 or 16x16 sprites
  SAT = Sprite Attribute Table (4 bytes * 32 sprites = $80 bytes)
		Byte 0 = Vertical Position (Y)
		Byte 1 = Horizontal Position (X)
		Byte 2 = "Name" (low-order bits of address in SGT)
		Byte 3 = misc
				D0 = Early Clock Bit
				D4-D7 = Color Code
  PGT = Pattern Generator Table
  PCT = Pattern Color Table

  1 VRAM write pointer
  1 VRAM read pointer
  1 Register to temporarily hold a register write

  Graphics I mode:
  - 32x24 patterns, each pattern is 8x8
  - up to 256 patterns may be stored
  - each pattern can contain two colors

  Graphics II mode:
  - 32x24 patterns, each pattern is 8x8
  - up to 768 patterns may be stored
  - each byte of each pattern can contain two colors

  Multicolor mode:
  - 64x48 blocks, each block is 4x4
  - each block is a single color

  Text mode:
  - 40x24 patterns, each pattern is 6x8
  - up to 256 patterns may be stored
  - sprites aren't drawn

  MODE = 0 : Address VDP RAM
  MODE = 1 : Address VDP Registers
***************************************************************************/

#include "driver.h"
#include "generic.h"
#include "TMS9928A.h"

unsigned char TMS9928A_palette[] =
{
	0x00, 0x00, 0x00, /* Transparent */
	0x00, 0x00, 0x00, /* Black */
	0x20, 0xC0, 0x20, /* Medium Green */
	0x60, 0xE0, 0x60, /* Light Green */
	0x20, 0x20, 0xE0, /* Dark Blue */
	0x40, 0x60, 0xE0, /* Light Blue */
	0xA0, 0x20, 0x20, /* Dark Red */
	0x40, 0xC0, 0xE0, /* Cyan */
	0xE0, 0x20, 0x20, /* Medium Red */
	0xE0, 0x60, 0x60, /* Light Red */
	0xC0, 0xC0, 0x20, /* Dark Yellow */
	0xC0, 0xC0, 0x80, /* Light Yellow */
	0x20, 0x80, 0x20, /* Dark Green */
	0xC0, 0x40, 0xA0, /* Magenta */
	0xA0, 0xA0, 0xA0, /* Gray */
	0xE0, 0xE0, 0xE0, /* White */
};

unsigned short TMS9928A_colortable[] =
{
	0,1,
	0,2,
	0,3,
	0,4,
	0,5,
	0,6,
	0,7,
	0,8,
	0,9,
	0,10,
	0,11,
	0,12,
	0,13,
	0,14,
	0,15,
};

static TMS9928A tms;

#define TMS_GET_SCREEN_MODE (((tms.creg[0] & 0x0E)>>1) | (tms.creg[1] & 0x18))
#define TMS_TEXT1_MODE			0x10	/* Screen 0 */
#define TMS_GRAPHIC1_MODE		0x00	/* Screen 1 */
#define TMS_GRAPHIC2_MODE		0x01	/* Screen 2 */
#define TMS_MULTICOLOR_MODE 	0x08	/* Screen 3 */

#define TMS_DISPLAY_ENABLED (tms.creg[1] & 0x40)
#define TMS_SPRITES_8_NORMAL ((tms.creg[1] & 0x03) == 0x00)
#define TMS_SPRITES_16_NORMAL ((tms.creg[1] & 0x03) == 0x02)
#define TMS_SPRITES_8_DOUBLED ((tms.creg[1] & 0x03) == 0x01)
#define TMS_SPRITES_16_DOUBLED ((tms.creg[1] & 0x03) == 0x03)

extern int frameskip;
extern int framecount;
void TMS9928A_sprite_collision(void);

/***************************************************************************
  TMS9928A_start
***************************************************************************/
int TMS9928A_start(unsigned int vram_size)
{
	int i;

	/* We'd like our VRAM in $400 byte chunks, thank you very much. */
	if ((vram_size==0) || ((vram_size % 0x400)!=0))
		return 1;

	if ((tms.i_tmpbitmap = osd_create_bitmap(256,192)) == 0)
	{
		return 1;
	}

	tms.VRAM = malloc(vram_size);
	if (tms.VRAM==NULL)
	{
		osd_free_bitmap(tms.i_tmpbitmap);
		return 1;
	}

	tms.i_dirtyPatGen = malloc(MAX_DIRTY_PAT_GEN);
	if (tms.i_dirtyPatGen==NULL)
	{
		osd_free_bitmap(tms.i_tmpbitmap);
		free(tms.VRAM);
		return 1;
	}

	tms.i_dirtyColor = malloc(MAX_DIRTY_COLOR);
	if (tms.i_dirtyColor==NULL)
	{
		osd_free_bitmap(tms.i_tmpbitmap);
		free(tms.VRAM);
		free(tms.i_dirtyPatGen);
		return 1;
	}

	tms.i_dirtyPatName = malloc(MAX_DIRTY_PAT_NAME);
	if (tms.i_dirtyPatName==NULL)
	{
		osd_free_bitmap(tms.i_tmpbitmap);
		free(tms.VRAM);
		free(tms.i_dirtyPatGen);
		free(tms.i_dirtyColor);
		return 1;
	}

	/* We'll use this as a mask to make sure we don't overflow memory */
	/* TODO: this mask breaks on some $400 byte boundaries */
	tms.VRAM_size=vram_size-1;
	tms.write_ptr=0;
	tms.read_ptr=0;
	tms.temp_write=-1;
	for (i=0;i<MAX_CONTROL_REGISTERS;i++)
		tms.creg[i]=0;
	for (i=0;i<MAX_STATUS_REGISTERS;i++)
		tms.sreg[i]=0;

	/* Initialize TMS registers */
	tms.creg[1]=TMS_TEXT1_MODE;
	tms.sreg[0]=0x80;

	/* Initialize internal variables */
	tms.i_screenMode=TMS_GET_SCREEN_MODE;
	tms.i_backColor=0;
	tms.i_textColor=0;

	tms.i_patNameTabStart=0;
	tms.i_colorTabStart=0;
	tms.i_patGenTabStart=0;
	tms.i_spriteAttrTabStart=0;
	tms.i_spritePatGenTabStart=0;

	tms.i_patNameTabEnd=0;
	tms.i_colorTabEnd=0;
	tms.i_patGenTabEnd=0;
	tms.i_spriteAttrTabEnd=0;
	tms.i_spritePatGenTabEnd=0;

	tms.i_anyDirtyPatGen=1;
	tms.i_anyDirtyPatName=1;
	tms.i_anyDirtyColor=1;
	memset(tms.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
	memset(tms.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
	memset(tms.i_dirtyColor,1,MAX_DIRTY_COLOR);

	return 0;
}

/***************************************************************************
  TMS9928A_stop
***************************************************************************/
void TMS9928A_stop(void)
{
	if (tms.VRAM!=NULL)
		free(tms.VRAM);
	if (tms.i_dirtyPatGen!=NULL)
		free(tms.i_dirtyPatGen);
	if (tms.i_dirtyColor!=NULL)
		free(tms.i_dirtyColor);
	if (tms.i_dirtyPatName!=NULL)
		free(tms.i_dirtyPatName);
	osd_free_bitmap(tms.i_tmpbitmap);
	tms.VRAM=NULL;
	tms.i_dirtyPatGen=NULL;
	tms.i_dirtyColor=NULL;
	tms.i_dirtyPatName=NULL;
	return;
}

/***************************************************************************
  TMS9928A_interrupt
***************************************************************************/
int TMS9928A_interrupt(void)
{
	/* Needed for collision detection when frameskipping */
	if (framecount != frameskip)
		TMS9928A_sprite_collision();

	tms.sreg[0] |= 0x80;

	if ((tms.creg[1] & 0x20) && (tms.temp_write == -1))
		return 1;

	return 0;
}

/***************************************************************************
  TMS9928A_vram_r
***************************************************************************/
int TMS9928A_vram_r(void)
{
	int data;

	data = tms.VRAM[tms.read_ptr & tms.VRAM_size];
	tms.read_ptr = (tms.read_ptr + 1);
	tms.temp_write=-1;
	return data;
}

/***************************************************************************
  TMS9928A_vram_w
***************************************************************************/
void TMS9928A_vram_w(int data)
{
	tms.write_ptr = tms.write_ptr & tms.VRAM_size;
	tms.temp_write = -1;

	if (tms.VRAM[tms.write_ptr] != data)
	{
		tms.VRAM[tms.write_ptr] = data;

		/* Internal dirty buffer optimizing starts here */
		if ((tms.write_ptr >= tms.i_patNameTabStart) && (tms.write_ptr <= tms.i_patNameTabEnd))
		{
			tms.i_anyDirtyPatName=1;
			tms.i_dirtyPatName[(tms.write_ptr - tms.i_patNameTabStart)] = 1;
		}
		if ((tms.write_ptr >= tms.i_patGenTabStart) && (tms.write_ptr <= tms.i_patGenTabEnd))
		{
			tms.i_anyDirtyPatGen=1;
			tms.i_dirtyPatGen[(tms.write_ptr - tms.i_patGenTabStart) >> 3] = 1;
		}
		if ((tms.write_ptr >= tms.i_colorTabStart) && (tms.write_ptr <= tms.i_colorTabEnd))
		{
			tms.i_anyDirtyColor=1;
			if (tms.i_screenMode==TMS_GRAPHIC2_MODE)
				tms.i_dirtyColor[(tms.write_ptr - tms.i_colorTabStart) >> 3] = 1;
			else
				tms.i_dirtyColor[(tms.write_ptr - tms.i_colorTabStart)] = 1;
		}
		/* Internal dirty buffer optimizing stops here */
	}


	tms.write_ptr = (tms.write_ptr + 1);
}

/***************************************************************************
  TMS9928A_register_r
***************************************************************************/
int TMS9928A_register_r(void)
{
	int data;

	/* TODO: make status register work */
	data = tms.sreg[0];
	tms.sreg[0] &= 0x5F;
	tms.temp_write=-1;

	return data;

}

/***************************************************************************
  TMS9928A_change_register
***************************************************************************/
void TMS9928A_change_register(int reg, int data)
{
	int old_value;

	old_value=tms.creg[reg];
	tms.creg[reg]=data;

	switch (reg)
	{
		case 0:
			if (tms.i_screenMode != TMS_GET_SCREEN_MODE)
			{
				tms.i_screenMode=TMS_GET_SCREEN_MODE;
				tms.i_anyDirtyPatGen=1;
				tms.i_anyDirtyPatName=1;
				tms.i_anyDirtyColor=1;
				memset(tms.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
				memset(tms.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
				memset(tms.i_dirtyColor,1,MAX_DIRTY_COLOR);
			}
			break;
		case 1:
			if (tms.i_screenMode != TMS_GET_SCREEN_MODE)
			{
				tms.i_screenMode=TMS_GET_SCREEN_MODE;
				tms.i_anyDirtyPatGen=1;
				tms.i_anyDirtyPatName=1;
				tms.i_anyDirtyColor=1;
				memset(tms.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
				memset(tms.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
				memset(tms.i_dirtyColor,1,MAX_DIRTY_COLOR);
			}
			break;
		case 2:
			tms.i_anyDirtyPatName=1;
			memset(tms.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
			break;
		case 3:
			tms.i_anyDirtyColor=1;
			memset(tms.i_dirtyColor,1,MAX_DIRTY_COLOR);
			break;
		case 4:
			tms.i_anyDirtyPatGen=1;
			memset(tms.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
			break;
		case 5:
			tms.i_spriteAttrTabStart = tms.creg[5] << 7;
			break;
		case 6:
			tms.i_spritePatGenTabStart = (tms.creg[6] & 0x3F) << 11;
			break;
		case 7:
			tms.i_backColor=tms.creg[7] & 0x0F;
			tms.i_textColor=(tms.creg[7] & 0xF0) >> 4;
			tms.i_anyDirtyColor=1;
			memset(tms.i_dirtyColor,1,MAX_DIRTY_COLOR);
			break;
	}

	switch (tms.i_screenMode)
	{
		case TMS_TEXT1_MODE:
			tms.i_patNameTabStart = (tms.creg[2] & 0x7F) << 10;
			tms.i_colorTabStart = tms.creg[3] << 6;
			tms.i_patGenTabStart = (tms.creg[4] & 0x3F) << 11;
			tms.i_patNameTabEnd = tms.i_patNameTabStart + 40*24;
			tms.i_colorTabEnd = tms.i_colorTabEnd;
			tms.i_patGenTabEnd = tms.i_patGenTabStart + 8*256;
			break;
		case TMS_GRAPHIC1_MODE:
			tms.i_patNameTabStart = (tms.creg[2] & 0x7F) << 10;
			tms.i_colorTabStart = tms.creg[3] << 6;
			tms.i_patGenTabStart = (tms.creg[4] & 0x3F) << 11;
			tms.i_patNameTabEnd = tms.i_patNameTabStart + 32*24;
			tms.i_colorTabEnd = tms.i_colorTabEnd + 256/8;
			tms.i_patGenTabEnd = tms.i_patGenTabStart + 8*256;
			break;
		case TMS_GRAPHIC2_MODE:
			tms.i_patNameTabStart = (tms.creg[2] & 0x7F) << 10;
			tms.i_colorTabStart = (tms.creg[3] << 6) & 0xE03F;
			tms.i_patGenTabStart = ((tms.creg[4] & 0x3F) << 11) & 0xE7FF;
			tms.i_patNameTabEnd = tms.i_patNameTabStart + 32*24;
			tms.i_colorTabEnd = tms.i_colorTabEnd + 256*8*3;
			tms.i_patGenTabEnd = tms.i_patGenTabStart + 256*8*3;
			break;
		case TMS_MULTICOLOR_MODE:
			/* TODO: add something here!!! */
			break;
	}

	if (errorlog) fprintf(errorlog,"Reg %d = %02X\n",reg,data);

	if (TMS_SPRITES_8_NORMAL)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 8 Normal\n");
	}
	else if (TMS_SPRITES_8_DOUBLED)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 8 Doubled\n");
	}
	else if (TMS_SPRITES_16_NORMAL)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 16 Normal\n");
	}
	else if (TMS_SPRITES_16_DOUBLED)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 16 Doubled\n");
	}
}


/***************************************************************************
  TMS9928A_register_w
***************************************************************************/
void TMS9928A_register_w(int data)
{
	/* Commands are 16 bits, so just hold the first 8 bits till we get the rest */
	if (tms.temp_write==-1)
	{
		tms.temp_write = data;
		return;
	}
	else
	{
		/* Process command */

		/* Did we write to a register? */
		if ((data & 0xC0) == 0x80)
		{
			TMS9928A_change_register(data & 0x3F,tms.temp_write);
			tms.temp_write=-1;
			return;
		}

		/* Did we write to VRAM write pointer? */
		else if ((data & 0xC0) == 0x40)
		{
			tms.write_ptr = ((data & 0x3F)<<8) | tms.temp_write;
			tms.temp_write=-1;
			return;
		}

		/* Did we write to VRAM read pointer? */
		else if ((data & 0xC0) == 0x00)
		{
			tms.read_ptr = ((data & 0x3F)<<8) | tms.temp_write;
			tms.temp_write=-1;
			return;
		}
	}
}

/***************************************************************************
  TMS9928A_refresh_text1
***************************************************************************/
void TMS9928A_refresh_text1(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int charcode;
	unsigned char fore_color;
	unsigned char back_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	int dirtyName;


	/* internal dirty buffer optimization check */
	if ((tms.i_anyDirtyPatGen==0) && (tms.i_anyDirtyPatName==0) && (tms.i_anyDirtyColor==0))
		return;

	back_color = Machine->pens[(tms.creg[7] & 0x0F)];
	fore_color = Machine->pens[((tms.creg[7] & 0xF0) >> 4)];

	nameTab = &(tms.VRAM[tms.i_patNameTabStart]);
	dirtyName=0;

	for (y=0;y<24;y++)
	{
		for (x=0;x<40;x++)
		{
			charcode = (*nameTab);

			/* Only draw character again if memory has changed - internal optimization */
			if ((tms.i_dirtyPatName[dirtyName]) || (tms.i_dirtyPatGen[charcode]))
			{
				patTab = &(tms.VRAM[tms.i_patGenTabStart + 8*charcode]);
	
				for (cy=0;cy<8;cy++)
				{
					for (cx=0;cx<6;cx++)
					{
						if (((*patTab) >> (7-cx)) & 0x01)
							bitmap->line[8*y+cy][6*x+cx]=fore_color;
						else
							bitmap->line[8*y+cy][6*x+cx]=back_color;
					}
	
					patTab++;
				}
			}

			nameTab++;
			dirtyName++;
		}
	}

	/* Set memory to unchanged - internal optimization */
	tms.i_anyDirtyPatGen=0;
	tms.i_anyDirtyPatName=0;
	tms.i_anyDirtyColor=0;
	memset(tms.i_dirtyPatGen,0,MAX_DIRTY_PAT_GEN);
	memset(tms.i_dirtyPatName,0,MAX_DIRTY_PAT_NAME);
	memset(tms.i_dirtyColor,0,MAX_DIRTY_COLOR);
}

/***************************************************************************
  TMS9928A_refresh_graphic1
***************************************************************************/
void TMS9928A_refresh_graphic1(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int charcode;
	unsigned char fore_color;
	unsigned char back_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	int dirtyName;

	/* internal dirty buffer optimization check */
	if ((tms.i_anyDirtyPatGen==0) && (tms.i_anyDirtyPatName==0) && (tms.i_anyDirtyColor==0))
		return;

	nameTab = &(tms.VRAM[tms.i_patNameTabStart]);
	dirtyName=0;

	for (y=0;y<24;y++)
	{
		for (x=0;x<32;x++)
		{
			charcode = (*nameTab);

			/* Only draw character again if memory has changed - internal optimization */
			if ((tms.i_dirtyPatName[dirtyName]) || (tms.i_dirtyPatGen[charcode]) || (tms.i_dirtyColor[charcode>>3]))
			{
				patTab = &(tms.VRAM[tms.i_patGenTabStart + (8*charcode)]);

				back_color = Machine->pens[(tms.VRAM[tms.i_colorTabStart + (charcode>>3)] & 0x0F)];
				fore_color = Machine->pens[((tms.VRAM[tms.i_colorTabStart + (charcode>>3)] & 0xF0) >> 4)];

				for (cy=0;cy<8;cy++)
				{
					for (cx=0;cx<8;cx++)
					{
						if (((*patTab) >> (7-cx)) & 0x01)
							bitmap->line[8*y+cy][8*x+cx]=fore_color;
						else	
							bitmap->line[8*y+cy][8*x+cx]=back_color;
					}
	
					patTab++;
				}
			}

			nameTab++;
			dirtyName++;
		}
	}

	/* Set memory to unchanged - internal optimization */
	tms.i_anyDirtyPatGen=0;
	tms.i_anyDirtyPatName=0;
	tms.i_anyDirtyColor=0;
	memset(tms.i_dirtyPatGen,0,MAX_DIRTY_PAT_GEN);
	memset(tms.i_dirtyPatName,0,MAX_DIRTY_PAT_NAME);
	memset(tms.i_dirtyColor,0,MAX_DIRTY_COLOR);
}

/***************************************************************************
  TMS9928A_refresh_graphic2
***************************************************************************/
void TMS9928A_refresh_graphic2(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int charcode;
	int block;
	unsigned char fore_color;
	unsigned char back_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	int dirtyName;

	/* internal dirty buffer optimization check */
	if ((tms.i_anyDirtyPatGen==0) && (tms.i_anyDirtyPatName==0) && (tms.i_anyDirtyColor==0))
		return;

	nameTab = &(tms.VRAM[tms.i_patNameTabStart]);
	dirtyName=0;

	for (y=0;y<24;y++)
	{
		block = (y >> 3) * 0x100;

		for (x=0;x<32;x++)
		{
			charcode = block + (*nameTab);

			/* Only draw character again if memory has changed - internal optimization */
			if ((tms.i_dirtyPatName[dirtyName]) || (tms.i_dirtyPatGen[charcode]) || (tms.i_dirtyColor[charcode]))
			{
				patTab = &(tms.VRAM[tms.i_patGenTabStart + (8*charcode)]);

				for (cy=0;cy<8;cy++)
				{
					back_color = Machine->pens[(tms.VRAM[tms.i_colorTabStart + (charcode * 8) + cy] & 0x0F)];
					fore_color = Machine->pens[((tms.VRAM[tms.i_colorTabStart + (charcode * 8) + cy] & 0xF0) >> 4)];

					for (cx=0;cx<8;cx++)
					{
						if (((*patTab) >> (7-cx)) & 0x01)
							bitmap->line[8*y+cy][8*x+cx]=fore_color;
						else
							bitmap->line[8*y+cy][8*x+cx]=back_color;
					}
	
					patTab++;
				}
			}

			nameTab++;
			dirtyName++;
		}
	}

	/* Set memory to unchanged - internal optimization */
	tms.i_anyDirtyPatGen=0;
	tms.i_anyDirtyPatName=0;
	tms.i_anyDirtyColor=0;
	memset(tms.i_dirtyPatGen,0,MAX_DIRTY_PAT_GEN);
	memset(tms.i_dirtyPatName,0,MAX_DIRTY_PAT_NAME);
	memset(tms.i_dirtyColor,0,MAX_DIRTY_COLOR);
}

/***************************************************************************
  TMS9928A_refresh_multicolor
***************************************************************************/
void TMS9928A_refresh_multicolor(struct osd_bitmap *bitmap)
{
	/* TODO: CHANGE THIS!!! */

	/* internal dirty buffer optimization check */
	if ((tms.i_anyDirtyPatGen==0) && (tms.i_anyDirtyPatName==0) && (tms.i_anyDirtyColor==0))
		return;

	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
}

/***************************************************************************
  TMS9928A_sprite_collision_8

  We only do approximate collision detection for pixel-doubled sprites.
  It should be close enough not to be noticable.  If not, this is the
  function to change. :)
***************************************************************************/
void TMS9928A_sprite_collision_8(void)
{
	int x1,y1,x2,y2;
	int cy;
    int sx,sy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab1;
	unsigned char *nameTab2;
	unsigned char *patTab1;
	unsigned char *patTab2;
	int sprite1,sprite2;
	long g1,g2;
	int last_sprite;

	int doubled;

	/* This is used for our pixel-doubling approximation */
	if (TMS_SPRITES_8_NORMAL)	doubled=0;
	else						doubled=1;

	nameTab1 = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab1[0]!=208);last_sprite++,nameTab1+=4);

	for (sprite1=0;sprite1<last_sprite;sprite1++)
	{
		nameTab1 = &(tms.VRAM[tms.i_spriteAttrTabStart + 4*sprite1]);
		y1 = nameTab1[0];
		x1 = nameTab1[1];
		charcode = nameTab1[2];
		if (nameTab1[3] & 0x80)
			x1 -= 32;
		fore_color = Machine->pens[(nameTab1[3] & 0x0F)];
		patTab1 = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y1!=0) && (fore_color != Machine->pens[0]))
		{
			if (y1==255) y1=0;

			for (sprite2=sprite1+1;sprite2<last_sprite;sprite2++)
			{
				nameTab2 = &(tms.VRAM[tms.i_spriteAttrTabStart + 4*sprite2]);
				y2 = nameTab2[0];
				x2 = nameTab2[1];
				charcode = nameTab2[2];
				if (nameTab2[3] & 0x80)
					x2 -= 32;
				fore_color = Machine->pens[(nameTab2[3] & 0x0F)];
				patTab2 = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

				/* If y=0, don't draw.  If y=255, draw at y=0. */
				if ((y2!=0) && (fore_color != Machine->pens[0]))
				{
					if (y2==255) y2=0;

					/* Here's the heart of our approximation.  This affects our pixel comparisons.
					   To properly deal with pixel doubling, this routine would need to change starting here. */
					sy = (y2-y1) >> doubled;
					sx = (x2-x1) >> doubled;

					/* Only check pixels if sprites overlap */
					if ((sy<8) && (sy>-8) && (sx<8) && (sx>-8))
					{
						int abssy;
						/* Get absolute value of sy */
						if (sy<0)	abssy = -sy;
						else		abssy = sy;

						for (cy=abssy;cy<8;cy++)
						{
							long mask;

							mask = 0xFF;

							if (sy<0)
							{
								g1=patTab1[cy-abssy];
								g2=patTab2[cy];
							}
							else
							{
								g1=patTab1[cy];
								g2=patTab2[cy-abssy];
							}

							if (sx>0)
							{
								g2=g2 >> sx;
								if ((255-x2) <= 7)
									mask = mask >> (7 - (255-x2));
							}
							else
							{
								g1=g1 >> (-sx);
								if ((255-x1) <= 7)
									mask = mask >> (7 - (255-x1));
							}

							if ((g1 & g2 & mask) != 0)
							{
								tms.sreg[0] |= 0x20;
								return;
							}
						}

					}
				}
			}
		}
	}
}

/***************************************************************************
  TMS9928A_sprite_collision_16

  We only do approximate collision detection for pixel-doubled sprites.
  It should be close enough not to be noticable.  If not, this is the
  function to change. :)
***************************************************************************/
void TMS9928A_sprite_collision_16(void)
{
	int x1,y1,x2,y2;
	int cy;
    int sx,sy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab1;
	unsigned char *nameTab2;
	unsigned char *patTab1;
	unsigned char *patTab2;
	int sprite1,sprite2;
	long g1,g2;
	int last_sprite;

	int doubled;

	/* This is used for our pixel-doubling approximation */
	if (TMS_SPRITES_16_NORMAL)	doubled=0;
	else						doubled=1;

	nameTab1 = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab1[0]!=208);last_sprite++,nameTab1+=4);

	for (sprite1=0;sprite1<last_sprite;sprite1++)
	{
		nameTab1 = &(tms.VRAM[tms.i_spriteAttrTabStart + 4*sprite1]);
		y1 = nameTab1[0];
		x1 = nameTab1[1];
		charcode = nameTab1[2] & 0xFC;
		if (nameTab1[3] & 0x80)
			x1 -= 32;
		fore_color = Machine->pens[(nameTab1[3] & 0x0F)];
		patTab1 = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y1!=0) && (fore_color != Machine->pens[0]))
		{
			if (y1==255) y1=0;

			for (sprite2=sprite1+1;sprite2<last_sprite;sprite2++)
			{
				nameTab2 = &(tms.VRAM[tms.i_spriteAttrTabStart + 4*sprite2]);
				y2 = nameTab2[0];
				x2 = nameTab2[1];
				charcode = nameTab2[2] & 0xFC;
				if (nameTab2[3] & 0x80)
					x2 -= 32;
				fore_color = Machine->pens[(nameTab2[3] & 0x0F)];
				patTab2 = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

				/* If y=0, don't draw.  If y=255, draw at y=0. */
				if ((y2!=0) && (fore_color != Machine->pens[0]))
				{
					if (y2==255) y2=0;

					/* Here's the heart of our approximation.  This affects our pixel comparisons.
					   To properly deal with pixel doubling, this routine would need to change starting here. */
					sy = (y2-y1) >> doubled;
					sx = (x2-x1) >> doubled;

					/* Only check pixels if sprites overlap */
					if ((sy<16) && (sy>-16) && (sx<16) && (sx>-16))
					{
						int abssy;
						/* Get absolute value of sy */
						if (sy<0)	abssy = -sy;
						else		abssy = sy;

						for (cy=abssy;cy<16;cy++)
						{
							long mask;

							mask = 0xFFFF;

							if (sy<0)
							{
								g1=(patTab1[cy-abssy] << 8) | (patTab1[cy-abssy + 16]);
								g2=(patTab2[cy] << 8) | (patTab2[cy + 16]);
							}
							else
							{
								g1=(patTab1[cy] << 8) | (patTab1[cy + 16]);
								g2=(patTab2[cy-abssy] << 8) | (patTab2[cy-abssy + 16]);
							}

							if (sx>0)
							{
								g2=g2 >> sx;
								if ((255-x2) < 16)
									mask = mask >> (15 - (255-x2));
							}
							else
							{
								g1=g1 >> (-sx);
								if ((255-x1) < 16)
									mask = mask >> (15 - (255-x1));
							}

							if ((g1 & g2 & mask) != 0)
							{
								tms.sreg[0] |= 0x20;
								return;
							}
						}

					}
				}
			}
		}
	}
}

/***************************************************************************
  TMS9928A_sprite_collision
***************************************************************************/
void TMS9928A_sprite_collision(void)
{
	if (TMS_SPRITES_8_NORMAL)
		TMS9928A_sprite_collision_8();
	else if (TMS_SPRITES_8_DOUBLED)
		TMS9928A_sprite_collision_8();
	else if (TMS_SPRITES_16_NORMAL)
		TMS9928A_sprite_collision_16();
	else if (TMS_SPRITES_16_DOUBLED)
		TMS9928A_sprite_collision_16();
}


/***************************************************************************
  TMS9928A_refresh_sprites_8_normal
***************************************************************************/
void TMS9928A_refresh_sprites_8_normal(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
    int sx,sy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab[0]!=208);last_sprite++,nameTab+=4);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		nameTab-=4;

		y = nameTab[0];
		x = nameTab[1];
		charcode = nameTab[2];
		fore_color = Machine->pens[(nameTab[3] & 0x0F)];
		if (nameTab[3] & 0x80)
			x -= 32;

		patTab = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y!=0) && (fore_color != Machine->pens[0]))
		{
			if (y==255) y=0;

			for (cy=0;cy<8;cy++)
			{
                sy=y+cy;
				for (cx=0;cx<8;cx++)
				{
                    sx=x+cx;
					if (((*patTab) >> (7-cx)) & 0x01)
					{
						if ((sy<192) && (sx>=0) && (sx<256))
						{
							bitmap->line[sy][sx]=fore_color;
						}
					}
				}
	
				patTab++;
			}

		}
	}
}

/***************************************************************************
  TMS9928A_refresh_sprites_8_doubled
***************************************************************************/
void TMS9928A_refresh_sprites_8_doubled(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int sx,sy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab[0]!=208);last_sprite++,nameTab+=4);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		nameTab-=4;

		y = nameTab[0];
		x = nameTab[1];
		charcode = nameTab[2];
		fore_color = Machine->pens[(nameTab[3] & 0x0F)];
		if (nameTab[3] & 0x80)
			x -= 32;

		patTab = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y!=0) && (fore_color != Machine->pens[0]))
		{
			if (y==255) y=0;

			for (cy=0;cy<16;cy+=2)
			{
				sy=y+cy;
				for (cx=0;cx<16;cx+=2)
				{
					sx=x+cx;
					if (((*patTab) >> (7-cx)) & 0x01)
					{
						if (sy<191)
						{
							if ((sx>=0) && (sx<255))
							{
								bitmap->line[sy][sx]=fore_color;
								bitmap->line[sy][sx+1]=fore_color;
								bitmap->line[sy+1][sx]=fore_color;
								bitmap->line[sy+1][sx+1]=fore_color;
							}
						}
					}
				}

				patTab++;
			}
		}
	}
}

/***************************************************************************
  TMS9928A_refresh_sprites_16_normal
***************************************************************************/
void TMS9928A_refresh_sprites_16_normal(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	unsigned char *patTab2;
	int sprite;
	int last_sprite;

	nameTab = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab[0]!=208);last_sprite++,nameTab+=4);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		nameTab-=4;

		y = nameTab[0];
		x = nameTab[1];
		charcode = nameTab[2] & 0xFC;
		fore_color = Machine->pens[(nameTab[3] & 0x0F)];
		if (nameTab[3] & 0x80)
			x -= 32;

		patTab = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);
		patTab2 = &(patTab[16]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y!=0) && (fore_color != Machine->pens[0]))
		{
			if (y==255) y=0;

			for (cy=0;cy<16;cy++)
			{
				long grfx;

				grfx = ((*patTab) << 8) | (*patTab2);

				for (cx=0;cx<16;cx++)
				{
					if ((grfx >> (15-cx)) & 0x01)
					{
						if (((y+cy)<192) && ((x+cx)>=0) && ((x+cx)<256))
						{
							bitmap->line[y+cy][x+cx]=fore_color;
						}
					}
				}
	
				patTab++;
				patTab2++;
			}
		}
	}
}

/***************************************************************************
  TMS9928A_refresh_sprites_16_doubled
***************************************************************************/
void TMS9928A_refresh_sprites_16_doubled(struct osd_bitmap *bitmap)
{
	/* TODO: fix pixel doubling? */
	int x,y;
	int cx,cy;
	int sx,sy;
	int charcode;
	unsigned char fore_color;
	unsigned char *nameTab;
	unsigned char *patTab;
	unsigned char *patTab2;
	int sprite;
	int last_sprite;

	nameTab = &(tms.VRAM[tms.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite=0;(last_sprite<32) && (nameTab[0]!=208);last_sprite++,nameTab+=4);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		nameTab-=4;

		y = nameTab[0];
		x = nameTab[1];
		charcode = nameTab[2] & 0xFC;
		fore_color = Machine->pens[(nameTab[3] & 0x0F)];
		if (nameTab[3] & 0x80)
			x -= 32;

		patTab = &(tms.VRAM[tms.i_spritePatGenTabStart + (8*charcode)]);
		patTab2 = &(patTab[16]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y!=0) && (fore_color != Machine->pens[0]))
		{
			if (y==255) y=0;

			for (cy=0;cy<32;cy+=2)
			{
				long grfx;

				grfx = ((*patTab) << 8) | (*patTab2);

				for (cx=0;cx<16;cx++)
				{
					sy=y+cy;
					sx=x+cx;
					if ((grfx >> (15-cx)) & 0x01)
					{
						if (sy<191)
						{
							if ((sx>=0) && (sx<255))
							{
								bitmap->line[sy][sx]=fore_color;
								bitmap->line[sy][sx+1]=fore_color;
								bitmap->line[sy+1][sx]=fore_color;
								bitmap->line[sy+1][sx+1]=fore_color;
							}
						}
					}
				}
	
				patTab++;
				patTab2++;
			}
		}
	}
}

/***************************************************************************
  TMS9928A_refresh_sprites
***************************************************************************/
void TMS9928A_refresh_sprites(struct osd_bitmap *bitmap)
{
	/* Clear collisions and 5-sprite bits */
	tms.sreg[0] &= 0x80;

	TMS9928A_sprite_collision();

	if (TMS_SPRITES_8_NORMAL)
		TMS9928A_refresh_sprites_8_normal(bitmap);
	else if (TMS_SPRITES_8_DOUBLED)
		TMS9928A_refresh_sprites_8_doubled(bitmap);
	else if (TMS_SPRITES_16_NORMAL)
		TMS9928A_refresh_sprites_16_normal(bitmap);
	else if (TMS_SPRITES_16_DOUBLED)
		TMS9928A_refresh_sprites_16_doubled(bitmap);

}

/***************************************************************************
  TMS9928A_refresh
***************************************************************************/
void TMS9928A_refresh(struct osd_bitmap *bitmap)
{
	if (!(TMS_DISPLAY_ENABLED))
	{
		/* TODO: is this right? */
		fillbitmap(bitmap,Machine->pens[tms.i_backColor],&Machine->drv->visible_area);
		return;
	}

	switch(tms.i_screenMode)
	{
		case TMS_TEXT1_MODE:
			TMS9928A_refresh_text1(tms.i_tmpbitmap);
			copybitmap(bitmap,tms.i_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			break;
		case TMS_GRAPHIC1_MODE:
			TMS9928A_refresh_graphic1(tms.i_tmpbitmap);
			copybitmap(bitmap,tms.i_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			TMS9928A_refresh_sprites(bitmap);
			break;
		case TMS_GRAPHIC2_MODE:
			TMS9928A_refresh_graphic2(tms.i_tmpbitmap);
			copybitmap(bitmap,tms.i_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			TMS9928A_refresh_sprites(bitmap);
			break;
		case TMS_MULTICOLOR_MODE:
			TMS9928A_refresh_multicolor(tms.i_tmpbitmap);
			copybitmap(bitmap,tms.i_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			TMS9928A_refresh_sprites(bitmap);
			break;
		default:
			fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
			break;
	}

	return;
}

