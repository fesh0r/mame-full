/***************************************************************************

  vidhrdw/sms.c

  The Sega Master System/Game Gear use a variant of the TMS 9928A family of
  vdps.

  There are 11 Control Registers in the SMS VDP, these are write-only.
  Reg	D7	D6	D5	D4	D3	D2	D1	D0
  0 	0	DG	0	0	M5	M4	M3	0
  1 	0	BLK IE0 M1	M2	0	SZ	MAG
  2 	0	A16 A15 A14 A13 A12 A11 A10
  3 	B13 B12 B11 B10 B9	B8	B7	B6
  4 	0	0	C16 C15 C14 C13 C12 C11
  5 	D14 D13 D12 D11 D10 D9	D8	D7
  6 	0	0	E16 E15 E14 E13 E12 E11
  7 	TC3 TC2 TC1 TC0 BC3 BC2 BC1 BC0
  8     x7  x6  x5  x4  x3  x2  x1  x0
  9     y7  y6  y5  y4  y3  y2  y1  y0
  A     yi7 yi6 yi5 yi4 yi3 yi2 yi1 yi0

  
  DG = Digitize (use external input)
  IE0 = Enable Vertical Retrace Interrupt
  M1-M5 = Screen mode select
		M1	M2	M5	M4	M3
		1	0	0	0	0	Screen 0 (WIDTH 40)
		0	0	0	0	0	Screen 1
		0	0	0	0	1	Screen 2
		0	1	0	0	0	Screen 3
  BLK = 0:Disable Display, 1:Enable Display
  SZ = 0:Sprite size 8x8, 1:Sprite size 8x16
  MAG = 0:Sprite magnification 1x, 1:Sprite magnification 2x
  A10-A16 - Pattern name table base address
  B6-B13 - Color table base address
  C11-C16 - Pattern generator table base address
  D7-D14 - Sprite attribute table base address
  E11-E16 - Sprite pattern generator table base address
  TC0-TC3 - Text color
  BC0-BC3 - Background color

  x0-x7 - Horizontal scroll
  y0-y7 - Vertical scroll

  yi0-yi7 - Vertical line on which to generate an interrupt

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
#include "vidhrdw/generic.h"
#include "smsvdp.h"

static SMSVDP sms_vdp;

#define SMS_GET_SCREEN_MODE (((sms_vdp.creg[0] & 0x0E)>>1) | (sms_vdp.creg[1] & 0x18))
#define SMS_TEXT1_MODE			0x10	/* Screen 0 */
#define SMS_GRAPHIC1_MODE		0x00	/* Screen 1 */
#define SMS_GRAPHIC2_MODE		0x01	/* Screen 2 */
#define SMS_MULTICOLOR_MODE 	0x08	/* Screen 3 */

#define SMS_DISPLAY_ENABLED (sms_vdp.creg[1] & 0x40)
#define SMS_SPRITES_8_NORMAL ((sms_vdp.creg[1] & 0x03) == 0x00)
#define SMS_SPRITES_16_NORMAL ((sms_vdp.creg[1] & 0x03) == 0x02)
#define SMS_SPRITES_8_DOUBLED ((sms_vdp.creg[1] & 0x03) == 0x01)
#define SMS_SPRITES_16_DOUBLED ((sms_vdp.creg[1] & 0x03) == 0x03)

extern int frameskip;
extern int framecount;

extern int GameGear;
void SMSVDP_sprite_collision(void);

/***************************************************************************
  SMSVDP_start
***************************************************************************/
int SMSVDP_start(unsigned int vram_size)
{
	int i;

	/* We'd like our VRAM in $400 byte chunks, thank you very much. */
	if ((vram_size==0) || ((vram_size % 0x400)!=0))
		return 1;

	if ((sms_vdp.i_tmpbitmap = osd_create_bitmap(256,192)) == 0)
	{
		return 1;
	}

	sms_vdp.VRAM = calloc(vram_size, 1);
	if (sms_vdp.VRAM==NULL)
	{
		osd_free_bitmap(sms_vdp.i_tmpbitmap);
		return 1;
	}

	Machine->memory_region[1] = sms_vdp.VRAM;
	
	sms_vdp.i_dirtyPatGen = malloc(MAX_DIRTY_PAT_GEN);
	if (sms_vdp.i_dirtyPatGen==NULL)
	{
		osd_free_bitmap(sms_vdp.i_tmpbitmap);
		free(sms_vdp.VRAM);
		return 1;
	}

	sms_vdp.i_dirtyColor = malloc(MAX_DIRTY_COLOR);
	if (sms_vdp.i_dirtyColor==NULL)
	{
		osd_free_bitmap(sms_vdp.i_tmpbitmap);
		free(sms_vdp.VRAM);
		free(sms_vdp.i_dirtyPatGen);
		return 1;
	}

	sms_vdp.i_dirtyPatName = malloc(MAX_DIRTY_PAT_NAME);
	if (sms_vdp.i_dirtyPatName==NULL)
	{
		osd_free_bitmap(sms_vdp.i_tmpbitmap);
		free(sms_vdp.VRAM);
		free(sms_vdp.i_dirtyPatGen);
		free(sms_vdp.i_dirtyColor);
		return 1;
	}

	/* We'll use this as a mask to make sure we don't overflow memory */
	/* TODO: this mask breaks on some $400 byte boundaries */
	sms_vdp.VRAM_size = vram_size-1;
	sms_vdp.write_ptr = 0;
	sms_vdp.read_ptr = 0;
	sms_vdp.palette_ptr = -1;
	sms_vdp.temp_write = -1;
	for (i=0;i<MAX_CONTROL_REGISTERS;i++)
		sms_vdp.creg[i]=0;
	for (i=0;i<MAX_STATUS_REGISTERS;i++)
		sms_vdp.sreg[i]=0;

	/* Initialize TMS registers */
	sms_vdp.creg[1]=SMS_GRAPHIC1_MODE;
	sms_vdp.sreg[0]=0x80;

	/* Initialize internal variables */
	sms_vdp.i_screenMode=SMS_GET_SCREEN_MODE;
	sms_vdp.i_backColor=0;
	sms_vdp.i_textColor=0;

	sms_vdp.i_patNameTabStart=0;
	sms_vdp.i_colorTabStart=0;
	sms_vdp.i_patGenTabStart=0;
	sms_vdp.i_spriteAttrTabStart=0;
	sms_vdp.i_spritePatGenTabStart=0;

	sms_vdp.i_patNameTabEnd=0;
	sms_vdp.i_colorTabEnd=0;
	sms_vdp.i_patGenTabEnd=0;
	sms_vdp.i_spriteAttrTabEnd=0;
	sms_vdp.i_spritePatGenTabEnd=0;

	sms_vdp.i_anyDirtyPatGen=1;
	sms_vdp.i_anyDirtyPatName=1;
	sms_vdp.i_anyDirtyColor=1;
	memset(sms_vdp.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
	memset(sms_vdp.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
	memset(sms_vdp.i_dirtyColor,1,MAX_DIRTY_COLOR);

	return 0;
}

/***************************************************************************
  SMSVDP_stop
***************************************************************************/
void SMSVDP_stop(void)
{
//	if (sms_vdp.VRAM!=NULL)
//		free(sms_vdp.VRAM);
	if (sms_vdp.i_dirtyPatGen!=NULL)
		free(sms_vdp.i_dirtyPatGen);
	if (sms_vdp.i_dirtyColor!=NULL)
		free(sms_vdp.i_dirtyColor);
	if (sms_vdp.i_dirtyPatName!=NULL)
		free(sms_vdp.i_dirtyPatName);
	osd_free_bitmap(sms_vdp.i_tmpbitmap);
	sms_vdp.VRAM=NULL;
	sms_vdp.i_dirtyPatGen=NULL;
	sms_vdp.i_dirtyColor=NULL;
	sms_vdp.i_dirtyPatName=NULL;
	return;
}

/***************************************************************************
  SMSVDP_interrupt
***************************************************************************/
int SMSVDP_interrupt(void)
{
	/* Needed for collision detection when frameskipping */
	if (framecount != frameskip)
		SMSVDP_sprite_collision();

	sms_vdp.sreg[0] |= 0x80;

	if ((sms_vdp.creg[1] & 0x20) && (sms_vdp.temp_write == -1))
		return 1;

	return 0x38;
}

/***************************************************************************
  SMSVDP_vram_r
***************************************************************************/
int SMSVDP_vram_r(int offset)
{
	int data;

	if (sms_vdp.palette_ptr)
	{
		data = sms_vdp.palette[sms_vdp.palette_ptr];
		sms_vdp.palette_ptr ++;
		if (GameGear)
			sms_vdp.palette_ptr &= 0x3f;
		else
			sms_vdp.palette_ptr &= 0x1f;
	}
	else
	{
		data = sms_vdp.VRAM[sms_vdp.read_ptr & sms_vdp.VRAM_size];
		sms_vdp.read_ptr = (sms_vdp.read_ptr + 1);
		sms_vdp.temp_write=-1;
	}
	return data;
}

/***************************************************************************
  SMSVDP_vram_w
***************************************************************************/
void SMSVDP_vram_w(int offset, int data)
{
//	if (errorlog) fprintf (errorlog, "SMS_VDP vram_w, offset: %04x, data: %02x palette_ptr: %02x\n", sms_vdp.write_ptr, data, sms_vdp.palette_ptr);

	if (sms_vdp.palette_ptr != -1)
	{
		int color, r, g, b;
		
		sms_vdp.palette[sms_vdp.palette_ptr] = data;
		if (GameGear)
		{
			/* 0000BBB0GGG0RRR0 */
			color = sms_vdp.palette_ptr >> 1;
			r = (sms_vdp.palette[sms_vdp.palette_ptr & ~0x01] << 4) & 0xe0;
			g =  sms_vdp.palette[sms_vdp.palette_ptr & ~0x01]       & 0xe0;
			b = (sms_vdp.palette[sms_vdp.palette_ptr |  0x01] << 4) & 0xe0;
			sms_vdp.palette_ptr = (sms_vdp.palette_ptr + 1) & 0x3f;
		}
		else
		{
			/* 00BBGGRR */
			color = sms_vdp.palette_ptr;
			r = (data << 6) & 0xc0;
			g = (data << 4) & 0xc0;
			b = (data << 2) & 0xc0;
			sms_vdp.palette_ptr = (sms_vdp.palette_ptr + 1) & 0x1f;
		}
		palette_change_color (color, r, g, b);
		sms_vdp.temp_write = -1;

		if (errorlog) fprintf (errorlog, "SMS color: %02x, r: %02x g: %02x b: %02x\n", color, r, g, b);
		return;
	}
	
	sms_vdp.write_ptr = sms_vdp.write_ptr & sms_vdp.VRAM_size;
	sms_vdp.temp_write = -1;

	if (sms_vdp.VRAM[sms_vdp.write_ptr] != data)
	{
		sms_vdp.VRAM[sms_vdp.write_ptr] = data;

		/* Internal dirty buffer optimizing starts here */
		if ((sms_vdp.write_ptr >= sms_vdp.i_patNameTabStart) && (sms_vdp.write_ptr <= sms_vdp.i_patNameTabEnd))
		{
			sms_vdp.i_anyDirtyPatName=1;
			sms_vdp.i_dirtyPatName[(sms_vdp.write_ptr - sms_vdp.i_patNameTabStart)/2] = 1;
		}
		if ((sms_vdp.write_ptr >= sms_vdp.i_patGenTabStart) && (sms_vdp.write_ptr <= sms_vdp.i_patGenTabEnd))
		{
			sms_vdp.i_anyDirtyPatGen=1;
			sms_vdp.i_dirtyPatGen[(sms_vdp.write_ptr - sms_vdp.i_patGenTabStart) / 32] = 1;
			/* TODO: move this somewhere more efficient */
			decodechar(Machine->gfx[0], (sms_vdp.write_ptr - sms_vdp.i_patGenTabStart) / 32, sms_vdp.VRAM, Machine->drv->gfxdecodeinfo[0].gfxlayout);
		}
		if ((sms_vdp.write_ptr >= sms_vdp.i_colorTabStart) && (sms_vdp.write_ptr <= sms_vdp.i_colorTabEnd))
		{
			sms_vdp.i_anyDirtyColor=1;
			if (sms_vdp.i_screenMode==SMS_GRAPHIC2_MODE)
				sms_vdp.i_dirtyColor[(sms_vdp.write_ptr - sms_vdp.i_colorTabStart) >> 3] = 1;
			else
				sms_vdp.i_dirtyColor[(sms_vdp.write_ptr - sms_vdp.i_colorTabStart)] = 1;
		}
		/* Internal dirty buffer optimizing stops here */
	}

	sms_vdp.write_ptr = (sms_vdp.write_ptr + 1);
}

/***************************************************************************
  SMSVDP_register_r
***************************************************************************/
int SMSVDP_register_r(int offset)
{
	int data;

	/* TODO: make status register work */
	data = sms_vdp.sreg[0];
	sms_vdp.sreg[0] &= 0x5F;
	sms_vdp.temp_write=-1;

	return data;

}

/***************************************************************************
  SMSVDP_change_register
***************************************************************************/
void SMSVDP_change_register(int reg, int data)
{
	int old_value;

	old_value=sms_vdp.creg[reg];
	sms_vdp.creg[reg]=data;

	switch (reg)
	{
		case 0:
//			if (sms_vdp.i_screenMode != SMS_GET_SCREEN_MODE)
			{
//				sms_vdp.i_screenMode=SMS_GET_SCREEN_MODE;
				sms_vdp.i_anyDirtyPatGen=1;
				sms_vdp.i_anyDirtyPatName=1;
				sms_vdp.i_anyDirtyColor=1;
				memset(sms_vdp.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
				memset(sms_vdp.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
				memset(sms_vdp.i_dirtyColor,1,MAX_DIRTY_COLOR);
			}
			break;
		case 1:
//			if (sms_vdp.i_screenMode != SMS_GET_SCREEN_MODE)
			{
//				sms_vdp.i_screenMode=SMS_GET_SCREEN_MODE;
				sms_vdp.i_anyDirtyPatGen=1;
				sms_vdp.i_anyDirtyPatName=1;
				sms_vdp.i_anyDirtyColor=1;
				memset(sms_vdp.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
				memset(sms_vdp.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
				memset(sms_vdp.i_dirtyColor,1,MAX_DIRTY_COLOR);
			}
			break;
		case 2:
			sms_vdp.i_anyDirtyPatName=1;
			memset(sms_vdp.i_dirtyPatName,1,MAX_DIRTY_PAT_NAME);
			break;
		case 3:
			sms_vdp.i_anyDirtyColor=1;
			memset(sms_vdp.i_dirtyColor,1,MAX_DIRTY_COLOR);
			break;
		case 4:
			sms_vdp.i_anyDirtyPatGen=1;
			memset(sms_vdp.i_dirtyPatGen,1,MAX_DIRTY_PAT_GEN);
			break;
		case 5:
			/* Sprite table can move from $0000 to $3fff */
			sms_vdp.i_spriteAttrTabStart = (sms_vdp.creg[5] & 0x7e) << 7;
			break;
		case 6:
			/* Sprite pattern table is at $0000 (256 char set) or $2000 (192 char set) */
			sms_vdp.i_spritePatGenTabStart = (sms_vdp.creg[6] & 0x04) << 11;
			break;
		case 7:
			sms_vdp.i_backColor=sms_vdp.creg[7] & 0x0F;
			sms_vdp.i_textColor=(sms_vdp.creg[7] & 0xF0) >> 4;
			sms_vdp.i_anyDirtyColor=1;
			memset(sms_vdp.i_dirtyColor,1,MAX_DIRTY_COLOR);
			break;
	}

	/* The name table can move from $0000 to $3f00 */
	sms_vdp.i_patNameTabStart = (sms_vdp.creg[2] & 0x0e) << 10;
	sms_vdp.i_colorTabStart = sms_vdp.creg[3] << 6;
//	sms_vdp.i_patGenTabStart = (sms_vdp.creg[4] & 0x3f) << 8;
	sms_vdp.i_patGenTabStart = 0x0000;
	/* Each entry in the name table takes up 2 bytes, thus it's 64x24 instead of 32x24 */
	sms_vdp.i_patNameTabEnd = sms_vdp.i_patNameTabStart + 64*24;
	sms_vdp.i_colorTabEnd = sms_vdp.i_colorTabEnd;
	/* The pattern tables are 4bpp, split into 256 & 192 character chunks */
	sms_vdp.i_patGenTabEnd = sms_vdp.i_patGenTabStart + 4*8*(256+192);

#if 0
	if (errorlog) fprintf(errorlog,"Reg %d = %02X\n",reg,data);

	if (SMS_SPRITES_8_NORMAL)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 8 Normal\n");
	}
	else if (SMS_SPRITES_8_DOUBLED)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 8 Doubled\n");
	}
	else if (SMS_SPRITES_16_NORMAL)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 16 Normal\n");
	}
	else if (SMS_SPRITES_16_DOUBLED)
	{
		if (errorlog) fprintf(errorlog,"Sprite Mode = 16 Doubled\n");
	}
#endif
}


/***************************************************************************
  SMSVDP_register_w
***************************************************************************/
void SMSVDP_register_w(int offset, int data)
{
	if (errorlog) fprintf (errorlog, "SMS_VDP (new) reg_w, data: %02x\n", data);
	
	/* Commands are 16 bits, so just hold the first 8 bits till we get the rest */
	if (sms_vdp.temp_write==-1)
	{
		sms_vdp.temp_write = data;
		return;
	}
	else
	{
		/* Process command */

		/* Are we writing to a palette register? */
		if ((data & 0xc0) == 0xc0)
		{
			if (GameGear)
				sms_vdp.palette_ptr = sms_vdp.temp_write & 0x3f;
			else
				sms_vdp.palette_ptr = sms_vdp.temp_write & 0x1f;
			sms_vdp.temp_write = -1;
			return;
		}
		/* Did we write to a register? */
		else if ((data & 0xc0) == 0x80)
		{
			SMSVDP_change_register(data & 0x3F,sms_vdp.temp_write);
			sms_vdp.temp_write = -1;
			return;
		}

		/* Did we write to VRAM write pointer? */
		else if ((data & 0xc0) == 0x40)
		{
			sms_vdp.write_ptr = ((data & 0x3F)<<8) | sms_vdp.temp_write;
			sms_vdp.palette_ptr = -1;
			sms_vdp.temp_write = -1;
			return;
		}

		/* Did we write to VRAM read pointer? */
		else if ((data & 0xc0) == 0x00)
		{
			sms_vdp.read_ptr = ((data & 0x3F)<<8) | sms_vdp.temp_write;
			sms_vdp.palette_ptr = -1;
			sms_vdp.temp_write = -1;
			return;
		}
	}
}

/***************************************************************************
  SMSVDP_refresh_graphic
***************************************************************************/
void SMSVDP_refresh_graphic(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int charcode;
	unsigned char *nameTab;
	unsigned char *patTab;
	int dirtyName;

	/* internal dirty buffer optimization check */
	if ((sms_vdp.i_anyDirtyPatGen==0) && (sms_vdp.i_anyDirtyPatName==0) && (sms_vdp.i_anyDirtyColor==0))
		return;

	nameTab = &(sms_vdp.VRAM[sms_vdp.i_patNameTabStart]);
	dirtyName=0;

	for (y=0;y<24;y++)
	{
		for (x=0;x<32;x++)
		{
			charcode = (*(nameTab++));

			/* Only draw character again if memory has changed - internal optimization */
			if ((sms_vdp.i_dirtyPatName[dirtyName]) || (sms_vdp.i_dirtyPatGen[charcode]) || (sms_vdp.i_dirtyColor[charcode>>3]))
			{
				patTab = &(sms_vdp.VRAM[sms_vdp.i_patGenTabStart + (32*charcode) + 256 * (*nameTab & 0x01)]);

				for (cy=0;cy<8;cy++)
				{
					for (cx=0;cx<8;cx++)
					{
						int color;
						
						/* TODO: This is embarrassingly inefficient */
						color = (((*(patTab)) >> (7-cx)) & 0x01) +
								(((*(patTab+1)) >> (7-cx)) & 0x01)*2 +
								(((*(patTab+2)) >> (7-cx)) & 0x01)*4 +
								(((*(patTab+3)) >> (7-cx)) & 0x01)*8;
						if (*nameTab & 0x08) color += 0x0f;
								
						bitmap->line[8*y+cy][8*x+cx] = Machine->pens[color];
					}
					patTab += 4;
				}
			}

			nameTab ++;
			dirtyName ++;
		}
	}

	/* Set memory to unchanged - internal optimization */
	sms_vdp.i_anyDirtyPatGen=0;
	sms_vdp.i_anyDirtyPatName=0;
	sms_vdp.i_anyDirtyColor=0;
	memset(sms_vdp.i_dirtyPatGen,0,MAX_DIRTY_PAT_GEN);
	memset(sms_vdp.i_dirtyPatName,0,MAX_DIRTY_PAT_NAME);
	memset(sms_vdp.i_dirtyColor,0,MAX_DIRTY_COLOR);
}

/***************************************************************************
  SMSVDP_sprite_collision_8

  We only do approximate collision detection for pixel-doubled sprites.
  It should be close enough not to be noticable.  If not, this is the
  function to change. :)
***************************************************************************/
void SMSVDP_sprite_collision_8(void)
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
	if (SMS_SPRITES_8_NORMAL)	doubled=0;
	else						doubled=1;

	nameTab1 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#63 won't be displayed. */
	for (last_sprite=0;(last_sprite < MAX_SPRITES) && (nameTab1[0]!=208);last_sprite++,nameTab1+=4);

	for (sprite1=0;sprite1<last_sprite;sprite1++)
	{
		nameTab1 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart + 4*sprite1]);
		y1 = nameTab1[0];
		x1 = nameTab1[1];
		charcode = nameTab1[2];
		if (nameTab1[3] & 0x80)
			x1 -= 32;
		fore_color = Machine->pens[(nameTab1[3] & 0x0F)];
		patTab1 = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y1!=0) && (fore_color != Machine->pens[0]))
		{
			if (y1==255) y1=0;

			for (sprite2=sprite1+1;sprite2<last_sprite;sprite2++)
			{
				nameTab2 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart + 4*sprite2]);
				y2 = nameTab2[0];
				x2 = nameTab2[1];
				charcode = nameTab2[2];
				if (nameTab2[3] & 0x80)
					x2 -= 32;
				fore_color = Machine->pens[(nameTab2[3] & 0x0F)];
				patTab2 = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

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
								sms_vdp.sreg[0] |= 0x20;
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
  SMSVDP_sprite_collision_16

  We only do approximate collision detection for pixel-doubled sprites.
  It should be close enough not to be noticable.  If not, this is the
  function to change. :)
***************************************************************************/
void SMSVDP_sprite_collision_16(void)
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
	if (SMS_SPRITES_16_NORMAL)	doubled=0;
	else						doubled=1;

	nameTab1 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#63 won't be displayed. */
	for (last_sprite=0;(last_sprite<MAX_SPRITES) && (nameTab1[0]!=208);last_sprite++,nameTab1+=4);

	for (sprite1=0;sprite1<last_sprite;sprite1++)
	{
		nameTab1 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart + 4*sprite1]);
		y1 = nameTab1[0];
		x1 = nameTab1[1];
		charcode = nameTab1[2] & 0xFC;
		if (nameTab1[3] & 0x80)
			x1 -= 32;
		fore_color = Machine->pens[(nameTab1[3] & 0x0F)];
		patTab1 = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if ((y1!=0) && (fore_color != Machine->pens[0]))
		{
			if (y1==255) y1=0;

			for (sprite2=sprite1+1;sprite2<last_sprite;sprite2++)
			{
				nameTab2 = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart + 4*sprite2]);
				y2 = nameTab2[0];
				x2 = nameTab2[1];
				charcode = nameTab2[2] & 0xFC;
				if (nameTab2[3] & 0x80)
					x2 -= 32;
				fore_color = Machine->pens[(nameTab2[3] & 0x0F)];
				patTab2 = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

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
								sms_vdp.sreg[0] |= 0x20;
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
  SMSVDP_sprite_collision
***************************************************************************/
void SMSVDP_sprite_collision(void)
{
	if (SMS_SPRITES_8_NORMAL)
		SMSVDP_sprite_collision_8();
	else if (SMS_SPRITES_8_DOUBLED)
		SMSVDP_sprite_collision_8();
	else if (SMS_SPRITES_16_NORMAL)
		SMSVDP_sprite_collision_16();
	else if (SMS_SPRITES_16_DOUBLED)
		SMSVDP_sprite_collision_16();
}


/***************************************************************************
  SMSVDP_refresh_sprites_8_normal
***************************************************************************/
void SMSVDP_refresh_sprites_8_normal(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
    int sx,sy;
	int charcode;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#31 won't be displayed. */
	for (last_sprite = 0;(last_sprite < MAX_SPRITES) && (nameTab[last_sprite]!=208);last_sprite++);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		y = nameTab[sprite];
		x = nameTab[(sprite << 1) + 0x80];
		charcode = nameTab[(sprite << 1) + 0x81];

		patTab = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if (y!=0)
		{
			if (y==255) y=0;

			for (cy=0;cy<8;cy++)
			{
                sy=y+cy;
				for (cx=0;cx<8;cx++)
				{
					int color;
					
					/* TODO: This is embarrassingly inefficient */
					color = (((*(patTab)) >> (7-cx)) & 0x01) +
							(((*(patTab+1)) >> (7-cx)) & 0x01)*2 +
							(((*(patTab+2)) >> (7-cx)) & 0x01)*4 +
							(((*(patTab+3)) >> (7-cx)) & 0x01)*8;

                    sx=x+cx;
					if (color)
					{
						if ((sy<192) && (sx>=0) && (sx<256))
						{
							bitmap->line[sy][sx] = Machine->pens[color+0x10];
						}
					}
				}
	
				patTab += 4;
			}

		}
	}
}

/***************************************************************************
  SMSVDP_refresh_sprites_8_doubled
***************************************************************************/
void SMSVDP_refresh_sprites_8_doubled(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int sx,sy;
	int charcode;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#32 won't be displayed. */
	for (last_sprite=0;(last_sprite < MAX_SPRITES) && (nameTab[last_sprite]!=208);last_sprite++);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		y = nameTab[sprite];
		x = nameTab[(sprite << 1) + 0x80];
		charcode = nameTab[(sprite << 1) + 0x81];

		patTab = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if (y!=0)
		{
			if (y==255) y=0;

			for (cy=0;cy<16;cy+=2)
			{
				sy=y+cy;
				for (cx=0;cx<8;cx+=2)
				{
					int color;
					sx=x+cx*2;
					
					/* TODO: This is embarrassingly inefficient */
					color = (((*(patTab)) >> (7-cx)) & 0x01) +
							(((*(patTab+1)) >> (7-cx)) & 0x01)*2 +
							(((*(patTab+2)) >> (7-cx)) & 0x01)*4 +
							(((*(patTab+3)) >> (7-cx)) & 0x01)*8;
					
					if (color)
					{
						if (sy<191)
						{
							if ((sx>=0) && (sx<255))
							{
								bitmap->line[sy][sx] = Machine->pens[color+0x10];
								bitmap->line[sy][sx+1] = Machine->pens[color+0x10];
							}
						}
					}
				}

				patTab += 4;
			}
		}
	}
}

/***************************************************************************
  SMSVDP_refresh_sprites_16_normal
***************************************************************************/
void SMSVDP_refresh_sprites_16_normal(struct osd_bitmap *bitmap)
{
	int x,y;
	int cx,cy;
	int sx,sy;
	int charcode;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#53 won't be displayed. */
	for (last_sprite=0;(last_sprite < MAX_SPRITES) && (nameTab[last_sprite]!=208);last_sprite++);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		y = nameTab[sprite];
		x = nameTab[(sprite << 1) + 0x80];
		charcode = nameTab[(sprite << 1) + 0x81];

		patTab = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if (y!=0)
		{
			if (y==255) y=0;

			for (cy=0;cy<16;cy++)
			{
                sy=y+cy;
				for (cx=0;cx<8;cx++)
				{
					int color;
					
					/* TODO: This is embarrassingly inefficient */
					color = (((*(patTab)) >> (7-cx)) & 0x01) +
							(((*(patTab+1)) >> (7-cx)) & 0x01)*2 +
							(((*(patTab+2)) >> (7-cx)) & 0x01)*4 +
							(((*(patTab+3)) >> (7-cx)) & 0x01)*8;

                    sx=x+cx;
					if (color)
					{
						if ((sy<192) && (sx>=0) && (sx<256))
						{
							bitmap->line[sy][sx] = Machine->pens[color+0x10];
						}
					}
				}
	
				patTab += 4;
			}
		}
	}
}

/***************************************************************************
  SMSVDP_refresh_sprites_16_doubled
***************************************************************************/
void SMSVDP_refresh_sprites_16_doubled(struct osd_bitmap *bitmap)
{
	/* TODO: fix pixel doubling? */
	int x,y;
	int cx,cy;
	int sx,sy;
	int charcode;
	unsigned char *nameTab;
	unsigned char *patTab;
	int sprite;
	int last_sprite;

	nameTab = &(sms_vdp.VRAM[sms_vdp.i_spriteAttrTabStart]);

	/* If a sprite has Y=208, all lower-priority sprites will not be displayed. */
	/* Ex: if sprite #10 has Y=208, sprites #10-#63 won't be displayed. */
	for (last_sprite=0;(last_sprite < MAX_SPRITES) && (nameTab[last_sprite]!=208);last_sprite++);

	for (sprite=last_sprite;sprite>0;sprite--)
	{
		y = nameTab[sprite];
		x = nameTab[(sprite << 1) + 0x80];
		charcode = nameTab[(sprite << 1) + 0x81];

		patTab = &(sms_vdp.VRAM[sms_vdp.i_spritePatGenTabStart + (32*charcode)]);

		/* If y=0, don't draw.  If y=255, draw at y=0. */
		if (y!=0)
		{
			if (y==255) y=0;

			for (cy=0;cy<32;cy+=2)
			{
				for (cx=0;cx<8;cx++)
				{
					int color;
						
					sy=y+cy;
					sx=x+cx*2;

					/* TODO: This is embarrassingly inefficient */
					color = (((*(patTab)) >> (7-cx)) & 0x01) +
							(((*(patTab+1)) >> (7-cx)) & 0x01)*2 +
							(((*(patTab+2)) >> (7-cx)) & 0x01)*4 +
							(((*(patTab+3)) >> (7-cx)) & 0x01)*8;

					if (color)
					{
						if (sy<191)
						{
							if ((sx>=0) && (sx<255))
							{
								bitmap->line[sy][sx] = Machine->pens[color+0x10];
								bitmap->line[sy][sx+1] = Machine->pens[color+0x10];
							}
						}
					}
				}
	
				patTab += 4;
			}
		}
	}
}

/***************************************************************************
  SMSVDP_refresh_sprites
***************************************************************************/
void SMSVDP_refresh_sprites(struct osd_bitmap *bitmap)
{
	/* Clear collisions and 5-sprite bits */
	sms_vdp.sreg[0] &= 0x80;

	SMSVDP_sprite_collision();

	if (SMS_SPRITES_8_NORMAL)
		SMSVDP_refresh_sprites_8_normal(bitmap);
	else if (SMS_SPRITES_8_DOUBLED)
		SMSVDP_refresh_sprites_8_doubled(bitmap);
	else if (SMS_SPRITES_16_NORMAL)
		SMSVDP_refresh_sprites_16_normal(bitmap);
	else if (SMS_SPRITES_16_DOUBLED)
		SMSVDP_refresh_sprites_16_doubled(bitmap);
}

/***************************************************************************
  SMSVDP_refresh
***************************************************************************/
void SMSVDP_refresh(struct osd_bitmap *bitmap)
{
	int yscroll = -sms_vdp.creg[9];

	if (!(SMS_DISPLAY_ENABLED))
	{
		/* TODO: is this right? */
		fillbitmap(bitmap,Machine->pens[sms_vdp.i_backColor],&Machine->drv->visible_area);
		return;
	}

	SMSVDP_refresh_graphic (sms_vdp.i_tmpbitmap);
//	copybitmap (bitmap,sms_vdp.i_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copyscrollbitmap (bitmap,sms_vdp.i_tmpbitmap,1,&sms_vdp.creg[8],1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	SMSVDP_refresh_sprites (bitmap);

	return;
}

