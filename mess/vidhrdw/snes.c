/***************************************************************************

 Super Nintendo Entertainment System Driver - Written By Lee Hammerton aKa Savoury SnaX

 Acknowledgements

	I gratefully acknowledge the work of Karl Stenerud for his work on the processor
  cores used in this emulation and of course I hope you'll continue to work with me
  to improve this project.

	I would like to acknowledge the support of all those who helped me during SNEeSe and
  in doing so have helped get this project off the ground. There are many, many people
  to thank and so little space that I am keeping this as brief as I can :

		All snes technical document authors!
		All snes emulator authors!
			ZSnes
			Snes9x
			Snemul
			Nlksnes
			and the others....
		The original SNEeSe team members (other than myself ;-)) -
			Charles Bilyue - Your continued work on SNEeSe is fantastic!
			Santeri Saarimaa - Who'd have thought I'd come back to emulation ;-)

	***************************************************************************

	NB: This core is UNoptimised - I`m doing it accurately first!!

	Mode 7 - Written from scratch slowly to ensure no problems.

***************************************************************************/

#include "driver.h"
#include "cpu/g65816/g65816.h"
#include "vidhrdw/generic.h"

#include "../machine/snes.h"

unsigned char *zBuffer;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( snes )
{
	zBuffer = auto_malloc((256+32)*(256+32));
	if (!zBuffer)
		return 1;
	if( video_start_generic_bitmapped() )
		return 1;
	return 0;
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( snes )
{
}

/*   ------ SNES SCREEN DECODING TO BE FOUND HERE -------- */

unsigned char *ScreenAddress[4];	/* Screen Address (in VRAM) for BKGs */
unsigned char *ScreenAddressL;		/* Screen Address left pane during plane update */
unsigned char *ScreenAddressR;		/* Screen Address right pane during plane update */
unsigned char *TileAddress[4];		/* Tile Address (in VRAM) for tiles */
unsigned char *TileAddress_;		/* Tile Address during plane update */
unsigned char *TileAddress_Y;		/* Tile Address during plane update if Y flipped */

struct mame_bitmap *snes_bitmap;

unsigned char ColourBase1=1,ColourBase2=2,ColourBase4=4,ColourBase8=8;
unsigned long Tile_Offset_Table_8[8]={0,2,4,6,8,10,12,14};
unsigned long Tile_Offset_Table_8_Y[8]={14,12,10,8,6,4,2,0};
unsigned long Tile_Offset_Table_16_2[16]={0,2,4,6,8,10,12,14,16*8*2,16*8*2+2,16*8*2+4,16*8*2+6,16*8*2+8,16*8*2+10,16*8*2+12,16*8*2+14};
unsigned long Tile_Offset_Table_16_2_Y[16]={16*8*2+14,16*8*2+12,16*8*2+10,16*8*2+8,16*8*2+6,16*8*2+4,16*8*2+2,16*8*2,14,12,10,8,6,4,2,0};
unsigned long Tile_Offset_Table_16_4[16]={0,2,4,6,8,10,12,14,16*8*4,16*8*4+2,16*8*4+4,16*8*4+6,16*8*4+8,16*8*4+10,16*8*4+12,16*8*4+14};
unsigned long Tile_Offset_Table_16_4_Y[16]={16*8*4+14,16*8*4+12,16*8*4+10,16*8*4+8,16*8*4+6,16*8*4+4,16*8*4+2,16*8*4,14,12,10,8,6,4,2,0};
unsigned long Tile_Offset_Table_16_8[16]={0,2,4,6,8,10,12,14,16*8*8,16*8*8+2,16*8*8+4,16*8*8+6,16*8*8+8,16*8*8+10,16*8*8+12,16*8*8+14};
unsigned long Tile_Offset_Table_16_8_Y[16]={16*8*8+14,16*8*8+12,16*8*8+10,16*8*8+8,16*8*8+6,16*8*8+4,16*8*8+2,16*8*8,14,12,10,8,6,4,2,0};
unsigned long Tile_Offset_Table_32_4[32]={0,2,4,6,8,10,12,14,16*8*4,16*8*4+2,16*8*4+4,16*8*4+6,16*8*4+8,16*8*4+10,16*8*4+12,16*8*4+14,16*8*4*2,16*8*4*2+2,16*8*4*2+4,16*8*4*2+6,16*8*4*2+8,16*8*4*2+10,16*8*4*2+12,16*8*4*2+14};
unsigned long Tile_Offset_Table_32_4_Y[32]={16*8*4*2+14,16*8*4*2+12,16*8*4*2+10,16*8*4*2+8,16*8*4*2+6,16*8*4*2+4,16*8*4*2+2,16*8*4*2,16*8*4+14,16*8*4+12,16*8*4+10,16*8*4+8,16*8*4+6,16*8*4+4,16*8*4+2,16*8*4,14,12,10,8,6,4,2,0};

static void PLOT_8x8_2BplTile(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
				/*  Bp0=*(TileAddress+0) */
				/*  Bp1=*(TileAddress+1) */
	Bitmask=0x80;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase1;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase2;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask>>=1;
	}
}

static void PLOT_8x8_2BplTileX(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
			/*  Bp0=*(TileAddress+0) */
			/*  Bp1=*(TileAddress+1) */
	Bitmask=0x01;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase1;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase2;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask<<=1;
	}
}

static void PLOT_8x8_4BplTile(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bits2,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
				//  Bp0=*(TileAddress+0)
				//  Bp1=*(TileAddress+1)
	Tile+=14;
	Bits2=*Tile++;
	Bits2|=((unsigned short)(*Tile++))<<8;
				//  Bp2=*(TileAddress+2)
				//  Bp3=*(TileAddress+3)
	Bitmask=0x80;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase1;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=ColourBase4;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase2;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=ColourBase8;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask>>=1;
	}
}

static void PLOT_8x8_4BplTileX(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bits2,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
				/*  Bp0=*(TileAddress+0) */
				/*  Bp1=*(TileAddress+1) */
	Tile+=14;
	Bits2=*Tile++;
	Bits2|=((unsigned short)(*Tile++))<<8;
				/*  Bp2=*(TileAddress+2) */
				/*  Bp3=*(TileAddress+3) */
	Bitmask=0x01;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase1;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=ColourBase4;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=ColourBase2;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=ColourBase8;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask<<=1;
	}
}

static void PLOT_8x8_8BplTile(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bits2,Bits3,Bits4,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
				/*  Bp0=*(TileAddress+0) */
				/*  Bp1=*(TileAddress+1) */
	Tile+=14;
	Bits2=*Tile++;
	Bits2|=((unsigned short)(*Tile++))<<8;
				/*  Bp2=*(TileAddress+2) */
				/*  Bp3=*(TileAddress+3) */
	Tile+=14;
	Bits3=*Tile++;
	Bits3|=((unsigned short)(*Tile++))<<8;
				/*  Bp4=*(TileAddress+4) */
				/*  Bp5=*(TileAddress+5) */
	Tile+=14;
	Bits4=*Tile++;
	Bits4|=((unsigned short)(*Tile++))<<8;
				/*  Bp6=*(TileAddress+6) */
				/*  Bp7=*(TileAddress+7) */
	Bitmask=0x80;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=0x01;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=0x04;
		if ((Bitmask&Bits3)==Bitmask)
			DestPixel|=0x10;
		if ((Bitmask&Bits4)==Bitmask)
			DestPixel|=0x40;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=0x02;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=0x08;
		if ((Bitmask&Bits3)==Bitmask)
			DestPixel|=0x20;
		if ((Bitmask&Bits4)==Bitmask)
			DestPixel|=0x80;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask>>=1;
	}
}

static void PLOT_8x8_8BplTileX(unsigned char *Tile,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned char DestPixel;
	int count;
	unsigned short Bits,Bits2,Bits3,Bits4,Bitmask;

	Bits=*Tile++;
	Bits|=((unsigned short)(*Tile++))<<8;
				/*  Bp0=*(TileAddress+0) */
				/*  Bp1=*(TileAddress+1) */
	Tile+=14;
	Bits2=*Tile++;
	Bits2|=((unsigned short)(*Tile++))<<8;
				/*  Bp2=*(TileAddress+2) */
				/*  Bp3=*(TileAddress+3) */
	Tile+=14;
	Bits3=*Tile++;
	Bits3|=((unsigned short)(*Tile++))<<8;
				/*  Bp4=*(TileAddress+4) */
				/*  Bp5=*(TileAddress+5) */
	Tile+=14;
	Bits4=*Tile++;
	Bits4|=((unsigned short)(*Tile++))<<8;
				/*  Bp6=*(TileAddress+6) */
				/*  Bp7=*(TileAddress+7) */
	Bitmask=0x01;

	for (count=0;count<8;count++)
	{
		DestPixel=0;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=0x01;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=0x02;
		if ((Bitmask&Bits3)==Bitmask)
			DestPixel|=0x04;
		if ((Bitmask&Bits4)==Bitmask)
			DestPixel|=0x08;
		Bitmask<<=8;
		if ((Bitmask&Bits)==Bitmask)
			DestPixel|=0x10;
		if ((Bitmask&Bits2)==Bitmask)
			DestPixel|=0x20;
		if ((Bitmask&Bits3)==Bitmask)
			DestPixel|=0x40;
		if ((Bitmask&Bits4)==Bitmask)
			DestPixel|=0x80;
		if (DestPixel)
		{
			if (*zCur<=depth)
			{
				*dst=Machine->pens[palIndx[DestPixel]];
				*zCur=depth;
			}
		}
		Bitmask>>=8;
		dst++;
		zCur++;
		Bitmask<<=1;
	}
}

static int SPLOT_4BplTile(unsigned short xPos,short tileCode,unsigned short *dst,unsigned char *zCur,int width)
{
	unsigned char *TileAddressSpr;
	unsigned char Temp;
	int depth;
	int tileNum;
	int a,cnt=0;
	short xDiff;

	cnt++;
	if (tileCode & 0x8000)
		TileAddressSpr = TileAddress_Y;
	else
		TileAddressSpr = TileAddress_;

	depth = ((tileCode & 0x3000)>>12)+1;

	tileNum = (tileCode & 0x01FF) << 5;						// 4*8

	Temp=(((tileCode & 0x0E00) >>5)|0x80);					// sprites use upper 128 palette colours

	ColourBase1|=Temp;
	ColourBase2|=Temp;
	ColourBase4|=Temp;
	ColourBase8|=Temp;

	// sign extend xPos
	xDiff=xPos<<7;
	xDiff>>=7;

	if (tileCode & 0x4000)
		tileNum+=(width-1)*32;

	for (a=0;a<width;a++)
	{
		if (xDiff>-16)
		{
			if (tileCode & 0x4000)
			{
				PLOT_8x8_4BplTileX(TileAddressSpr + tileNum,dst+xDiff,zCur+xDiff,depth);
				tileNum-=32;
			}
			else
			{
				PLOT_8x8_4BplTile(TileAddressSpr + tileNum,dst+xDiff,zCur+xDiff,depth);
				tileNum+=32;
			}
		}
		xDiff+=8;
	}

	ColourBase1=0x01;
	ColourBase2=0x02;
	ColourBase4=0x04;
	ColourBase8=0x08;

	return cnt;
}

static void RENDER_LINE_8x8_C2(unsigned char PalOffset,unsigned char *Screen,unsigned long Count,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned short TileInfo;
	unsigned long TileNum;
	unsigned char Temp;
	unsigned char adepth;

	while (Count>0)
	{
		TileInfo=*Screen++;
		TileInfo|=((unsigned short)(*Screen++))<<8;
		TileNum=((unsigned long)(TileInfo&0x3FF))<<4;		/* 2*8 */

		adepth=depth;
		if (TileInfo&0x2000)
			adepth++;

		TileInfo&=0xDC00;
		Temp=((TileInfo>>8)&0x1C)+PalOffset;
		ColourBase1|=Temp;
		ColourBase2|=Temp;

		switch (TileInfo>>14)
		{
			case 0:					/* 8x8 Tile | no x flip | no y flip */
				PLOT_8x8_2BplTile(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 1:					/* 8x8 Tile |    x flip | no y flip */
				PLOT_8x8_2BplTileX(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 2:					/* 8x8 Tile | no x flip |    y flip */
				PLOT_8x8_2BplTile(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
			case 3:					/* 8x8 Tile |    x flip |    y flip */
				PLOT_8x8_2BplTileX(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
		}

		ColourBase1=0x01;
		ColourBase2=0x02;

		dst+=8;
		zCur+=8;
		Count--;
	}
}

static void RENDER_LINE_8x8_C4(unsigned char PalOffset,unsigned char *Screen,unsigned long Count,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned short TileInfo;
	unsigned long TileNum;
	unsigned char Temp;
	unsigned char adepth;

	while (Count>0)
	{
		TileInfo=*Screen++;
		TileInfo|=((unsigned short)(*Screen++))<<8;
		TileNum=((unsigned long)(TileInfo&0x3FF))<<5;		/* 4*8 */

		adepth=depth;
		if (TileInfo&0x2000)
			adepth++;

		TileInfo&=0xDC00;
		Temp=((TileInfo>>6)&0x70)/*+(Planenum*0x20)*/;
		ColourBase1|=Temp;
		ColourBase2|=Temp;
		ColourBase4|=Temp;
		ColourBase8|=Temp;

		switch (TileInfo>>14)
		{
			case 0:					/* 8x8 Tile | no x flip | no y flip */
				PLOT_8x8_4BplTile(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 1:					/* 8x8 Tile |    x flip | no y flip */
				PLOT_8x8_4BplTileX(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 2:					/* 8x8 Tile | no x flip |    y flip */
				PLOT_8x8_4BplTile(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
			case 3:					/* 8x8 Tile |    x flip |    y flip */
				PLOT_8x8_4BplTileX(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
		}

		ColourBase1=0x01;
		ColourBase2=0x02;
		ColourBase4=0x04;
		ColourBase8=0x08;

		dst+=8;
		zCur+=8;
		Count--;
	}
}

static void RENDER_LINE_8x8_C8(unsigned char PalOffset,unsigned char *Screen,unsigned long Count,unsigned short *dst,unsigned char *zCur,unsigned char depth)
{
	unsigned short TileInfo;
	unsigned long TileNum;
	unsigned char adepth;

	while (Count>0)
	{
		TileInfo=*Screen++;
		TileInfo|=((unsigned short)(*Screen++))<<8;
		TileNum=((unsigned long)(TileInfo&0x3FF))<<6;		/* 8*8 */

		adepth=depth;
		if (TileInfo&0x2000)
			adepth++;

		TileInfo&=0xC000;

		switch (TileInfo>>14)
		{
			case 0:					/* 8x8 Tile | no x flip | no y flip */
				PLOT_8x8_8BplTile(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 1:					/* 8x8 Tile |    x flip | no y flip */
				PLOT_8x8_8BplTileX(TileAddress_+TileNum,dst,zCur,adepth);
				break;
			case 2:					/* 8x8 Tile | no x flip |    y flip */
				PLOT_8x8_8BplTile(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
			case 3:					/* 8x8 Tile |    x flip |    y flip */
				PLOT_8x8_8BplTileX(TileAddress_Y+TileNum,dst,zCur,adepth);
				break;
		}

		dst+=8;
		zCur+=8;
		Count--;
	}
}

// This function sets up some pointers to allow rendering the snes screens to be a little easier

static void SORT_LAYOUT_8x8(int curLine,unsigned short Planenum)
{
	unsigned long VScroll;
	unsigned long VShift;

	VScroll=(((unsigned long)wport21xx[1][(Planenum*2+0x0E)])<<8)+wport21xx[0][(Planenum*2+0x0E)]+curLine;	// Get VScroll amount
	VShift=(/*curLine+*/VScroll)&0xFF;

	TileAddress_=TileAddress[Planenum]+Tile_Offset_Table_8[VShift&0x07];
	TileAddress_Y=TileAddress[Planenum]+Tile_Offset_Table_8_Y[VShift&0x07];

	switch (port21xx[0x07+Planenum]&0x03)
	{
		case 0:
			ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6);
			ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6);
			break;
		case 1:
			if ((wport21xx[1][(0x0D+Planenum*2)]&0x01)==0)
			{
				ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6);
				ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
			}
			else
			{
				ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6);
				ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
			}
			break;
		case 2:
			if ((VScroll&0x0100)==0)
			{
				ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6);
				ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6);
			}
			else
			{
				ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
				ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
			}
			break;
		case 3:
			if ((VScroll&0x0100)==0)
			{
				if ((wport21xx[1][(0x0D+Planenum*2)]&0x01)==0)
				{
					ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6);
					ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
				}
				else
				{
					ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6);
					ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2);
				}
			}
			else
			{
				if ((wport21xx[1][(0x0D+Planenum*2)]&0x01)==0)
				{
					ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2*2);
					ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2*2)+(32*32*2);
				}
				else
				{
					ScreenAddressR=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2*2);
					ScreenAddressL=ScreenAddress[Planenum]+((VShift>>3)<<6)+(32*32*2*2)+(32*32*2);
				}
			}
			break;
	}
}

static void RENDER_LINE(unsigned short *dst,unsigned char *zCur,int curLine,unsigned char Planenum,void (*RENDER_FUNCTION)(unsigned char,unsigned char *,unsigned long,unsigned short *,unsigned char *,unsigned char),unsigned char Bitmask,unsigned char depth,unsigned char palOffset)
{
	unsigned char HScroll=wport21xx[0][(0x0D+Planenum*2)];
	int Temp,Temp2;

	if ((port21xx[0x05]&Bitmask)==0)		/* 8x8 Tile mode */
	{
		SORT_LAYOUT_8x8(curLine,Planenum);		/* This sets up the screen vert 2! */

		if (HScroll==0)			/* If 0 simply plot 32 tiles! */
			RENDER_FUNCTION(palOffset,ScreenAddressL,32,dst,zCur,depth);
		else
		{
			dst-=HScroll&0x07;
			zCur-=HScroll&0x07;
			Temp=255-(HScroll&0xF8);
			Temp++;
			Temp>>=3;
			Temp2=(32-(Temp&0xFF))*2;
			RENDER_FUNCTION(palOffset,ScreenAddressL+Temp2,Temp,dst,zCur,depth);
			dst+=Temp<<3;
			zCur+=Temp<<3;
			Temp2=(33-Temp);
			RENDER_FUNCTION(palOffset,ScreenAddressR,Temp2,dst,zCur,depth);
		}
	}
	else
	{					/* 16x16 tiles */
		logerror("16x16 tiles not supported yet!\n");
	}
}

/*

 Mode 7 is handled differently to the other modes due to its matrix maths.

	These work something like this :

 A B   X
 C D   Y

 These are the matrix in 8:8 fixed point format for each element

 To find x , y location within tile map the following is done :

 TX = (A * ((SX + HS)-XC) + C * ((SY + VS)-YC)) + XC
 TY = (B * ((SX + HS)-XC) + D * ((SY + VS)-YC)) + YC

 Where

 TX,TY 			- Tile X,Y address (Tile map is 128 x 128)
 				- To find tile map location (TX / 8) * 2 + ((TY / 8) * 128 * 2) - Tiles are always start at beginning of VRAM
 				- Pixel location in tile (pointed to by tilemap) is (TX & 7) * 2 + (TY & 7) * 16
 A,B,C,D		- Matrix parameters
 SX,SY			- Screen location currently drawing
 HS,VS			- Horizontal and Verticle Scroll Registers
 XC,YC			- Center X , Center Y from matrix registers

*/

static void RENDER_LINE_MODE7(unsigned short *dst,unsigned char *zCur,int curLine,unsigned char depth)
{
	unsigned short tileNum;
	unsigned char *tileGFX;
	unsigned char DestPixel;
	short SX,SXDIR,SY,HS,VS,XC,YC,A,B,C,D,TX,TY;
	short cnt;

	A=(wport21xx[1][0x1B] << 8) + wport21xx[0][0x1B];		// These 4 are 8:8 fixed point values
	B=(wport21xx[1][0x1C] << 8) + wport21xx[0][0x1C];
	C=(wport21xx[1][0x1D] << 8) + wport21xx[0][0x1D];
	D=(wport21xx[1][0x1E] << 8) + wport21xx[0][0x1E];

	XC=(wport21xx[1][0x1F] << 8) + wport21xx[0][0x1F];		// These are 13 bit numbers - They must be sign extended
	YC=(wport21xx[1][0x20] << 8) + wport21xx[0][0x20];

	HS=(wport21xx[1][0x0D] << 8) + wport21xx[0][0x0D];		// These are 13 bit numbers - They must be sign extended
	VS=(wport21xx[1][0x0E] << 8) + wport21xx[0][0x0E];

	XC<<=3;													// Sign extend
	XC>>=3;
	YC<<=3;
	YC>>=3;
	HS<<=3;													// Sign extend
	HS>>=3;
	VS<<=3;
	VS>>=3;

	// Get X and Y Flip status

	SY=curLine;
	if (port21xx[0x1A]&0x02)								// If y flipped
		SY=255-SY;

	if (port21xx[0x1A]&0x01)								// If x flipped
	{
		SX=255;
		SXDIR=-1;
	}
	else
	{
		SX=0;
		SXDIR=1;
	}

	switch (port21xx[0x1A]&0xC0)
	{
		case 0x00:																	// Screen Repetition
			for (cnt=0;cnt<256;cnt++)
			{
				TX = (((A * ((SX + HS)-XC) + C * ((SY + VS)-YC))>>8)+XC)&0x3FF;		// Keep results within 0-1023
				TY = (((B * ((SX + HS)-XC) + D * ((SY + VS)-YC))>>8)+YC)&0x3FF;

				tileNum = SNES_VRAM[((TY / 8)*128*2)+((TX / 8)*2)];					// contents of this location is the tile number
				tileGFX = &SNES_VRAM[1 + (tileNum*8*8*2)];							// points to start of tile gfx data
				DestPixel = *(tileGFX + ((TY & 7)*8*2) + ((TX & 7)*2));				// pixel contains actual colour index

				if (DestPixel)
				{
					if (*zCur<=depth)
					{
						*dst=Machine->pens[palIndx[DestPixel]];
						*zCur=depth;
					}
				}
				dst++;
				zCur++;
				SX+=SXDIR;
			}
			break;
		case 0xC0:																	// No Draw If Outside Screen Area
			for (cnt=0;cnt<256;cnt++)
			{
				TX = (((A * ((SX + HS)-XC) + C * ((SY + VS)-YC))>>8)+XC);
				TY = (((B * ((SX + HS)-XC) + D * ((SY + VS)-YC))>>8)+YC);

				if ((TX&0x7FFF)<1024 && (TY&0x7FFF)<1024)
				{
					tileNum = SNES_VRAM[((TY / 8)*128*2)+((TX / 8)*2)];					// contents of this location is the tile number
					tileGFX = &SNES_VRAM[1 + (tileNum*8*8*2)];							// points to start of tile gfx data
					DestPixel = *(tileGFX + ((TY & 7)*8*2) + ((TX & 7)*2));				// pixel contains actual colour index

					if (DestPixel)
					{
						if (*zCur<=depth)
						{
							*dst=Machine->pens[palIndx[DestPixel]];
							*zCur=depth;
						}
					}
				}
				dst++;
				zCur++;
				SX+=SXDIR;
			}
			break;
		case 0x80:																	// Repeat Tile 00 if outside area
			for (cnt=0;cnt<256;cnt++)
			{
				TX = (((A * ((SX + HS)-XC) + C * ((SY + VS)-YC))>>8)+XC);
				TY = (((B * ((SX + HS)-XC) + D * ((SY + VS)-YC))>>8)+YC);

				tileNum = SNES_VRAM[(((TY&0x3FF) / 8)*128*2)+(((TX&0x3FF) / 8)*2)];		// contents of this location is the tile number
				if ((TX&0x7FFF)<1024 && (TY&0x7FFF)<1024)
					tileGFX = &SNES_VRAM[1 + (tileNum*8*8*2)];							// points to start of tile gfx data
				else
					tileGFX = &SNES_VRAM[1];										// points to tile 0
				DestPixel = *(tileGFX + ((TY & 7)*8*2) + ((TX & 7)*2));				// pixel contains actual colour index

				if (DestPixel)
				{
					if (*zCur<=depth)
					{
						*dst=Machine->pens[palIndx[DestPixel]];
						*zCur=depth;
					}
				}
				dst++;
				zCur++;
				SX+=SXDIR;
			}
			break;
		default:
			logerror("Unsupported Mode 7 Draw Type [%d]\n",port21xx[0x1A]);
			break;
	}
}

// This also attempts to fill in the time over and range over bits of STAT77

static void RenderSprites(unsigned short *dst,unsigned char *zCur,int curLine)						// dst is pointer to start of current line
{
	int a,b,size,cnt=0,TOver=0,ROver=0;
	unsigned char *oamPtr=&SNES_ORAM[0x1FF];
	short yPos,tileCode;
	unsigned short xPos;
	int YComp[2],Width[2];
	unsigned long OAM_Extra;
	int yTmp;
	unsigned long *table[2],*tableY[2];
	unsigned char *tmp;

	tmp = &SNES_VRAM[ (((port21xx[0x01]&0x03)*0x2000) + ((port21xx[0x01]&0x18)*(0x1000>>3)))*2 ];

	switch((port21xx[0x01]&0xE0)>>5)
	{
		case 0:												// 8 & 16
			YComp[0]=7;
			YComp[1]=15;
			Width[0]=1;
			Width[1]=2;
			table[0]=Tile_Offset_Table_8;
			tableY[0]=Tile_Offset_Table_8_Y;
			table[1]=Tile_Offset_Table_16_4;
			tableY[1]=Tile_Offset_Table_16_4_Y;
			break;
		case 1:												// 8 & 32
			YComp[0]=7;
			YComp[1]=31;
			Width[0]=1;
			Width[1]=4;
			table[0]=Tile_Offset_Table_8;
			tableY[0]=Tile_Offset_Table_8_Y;
			table[1]=Tile_Offset_Table_32_4;
			tableY[1]=Tile_Offset_Table_32_4_Y;
			break;
		case 3:												// 16 & 32
			YComp[0]=15;
			YComp[1]=31;
			Width[0]=2;
			Width[1]=4;
			table[0]=Tile_Offset_Table_16_4;
			tableY[0]=Tile_Offset_Table_16_4_Y;
			table[1]=Tile_Offset_Table_32_4;
			tableY[1]=Tile_Offset_Table_32_4_Y;
			break;
		default:
			logerror("Unsupported Sprite Size! [%02X]\n",(port21xx[0x01]&0xE0)>>5);
			return;
	}

	// Look at ORAM
	for (a=0;a<32;a++)
	{
		OAM_Extra=SNES_ORAM[0x200+(31-a)];

		for (b=0;b<4;b++)									// For every 2 bits in extra oam data
		{
			tileCode=(*oamPtr--)<<8;									// Tile Code = VHPPCCCT TTTTTTTT = vh=flip , p=priority , c=pal base , t = tile
			tileCode|=*oamPtr--;
			yPos=(*oamPtr--);
			xPos=*oamPtr--;
			size=(OAM_Extra & 0x80)>>7;
			OAM_Extra<<=1;
			xPos+=(OAM_Extra & 0x80)<<1;
			OAM_Extra<<=1;

			if (yPos>239)									// Y is past sprite max y position
				yPos-=256;

			yTmp=(yPos + YComp[size])-curLine+1;

			if (yTmp>=0 && yTmp<=YComp[size])
			{
				TileAddress_=tmp + tableY[size][yTmp];
				TileAddress_Y=tmp + table[size][yTmp];
				cnt=SPLOT_4BplTile(xPos,tileCode,dst,zCur,Width[size]);

				if (cnt)
				{
					TOver++;			// Number of objects rendered on a line
					ROver+=cnt;			// Number of 8x8 tiles rendered on a line
				}
			}
		}
	}

	port21xx[0x3E]&=0x3F;				// Clear Time Over and Range Over bits
	if (TOver>=33)
		port21xx[0x3E]|=0x80;
	if (ROver>=35)
		port21xx[0x3E]|=0x40;
}

#define TABLEA2(a) (port43xx[a+0x04]<<16) + (port43xx[a+0x09]<<8) + port43xx[a+0x08]
#define INDIRECT(a) (port43xx[a+0x07]<<16) + (port43xx[a+0x06]<<8) + port43xx[a+0x05]
#define BBUS(a) 0x00002100 + port43xx[a+1]

static unsigned char setupHDMA(unsigned char bits)
{
	unsigned char bMask=0x01;
	int dmaBase=0x00;
	unsigned long SRC;//,a;

	// This routine sets up the hdma internal variables so that updateHDMA() can work.

	while (bMask)
	{
		if (bMask&bits)					// This port is set to perform hdma
		{
			// First off Set up the table address into the TABLE ADDRESS A2 registers 0x43?8 0x43?9

			port43xx[dmaBase + 0x08]=port43xx[dmaBase + 0x02];
			port43xx[dmaBase + 0x09]=port43xx[dmaBase + 0x03];

			if ((port21xx[0x05] & 0x07)==7)
			{
				SRC=TABLEA2(dmaBase);
/*				for (a=0;a<256*3;a++)
				{
					logerror("%02x ",cpu_readmem24(SRC));
					SRC++;
					if ((a&15)==15)
						logerror("\n");
				}*/
			}

			// Next set up the number of lines to execute (start at 0 to force updateHDMA to read in the first byte from the table
			port43xx[dmaBase + 0x0A]=0;
		}
		dmaBase += 0x10;
		bMask<<=1;
	}

	return bits;
}

static unsigned char updateHDMA(unsigned char bits)		// Returns hdma enable values (if hdma has finished then next line wont do it!)
{
	unsigned char bMask=0x01;
	int dmaBase;
	unsigned long SRC,DST;
	unsigned char contMode;

	for (dmaBase=0x00;dmaBase<0x80;dmaBase+=0x10,bMask<<=1)
	{
		if (bMask&bits)					// This port is performing hdma
		{
			// First off check to see if we need to read in a new line from the table
			if (!(port43xx[dmaBase+0x0A]&0x7F))
			{
				// Out of data need to read in a new line - taken from table a2 address with a1's bank address
				SRC = TABLEA2(dmaBase);
				port43xx[dmaBase+0x0A]=cpu_readmem24(SRC);
/*				if (port43xx[dmaBase + 0x00] & 0x40)
					logerror("Indirect HDMA - Table A2 Address %06X\n",SRC);
				if (port43xx[dmaBase + 0x00] & 0x40)
					logerror("Indirect HDMA - Read In Number Of Lines %02X\n",port43xx[dmaBase + 0x0A]);*/
				if (!port43xx[dmaBase+0x0A])						// If byte just read is end of table marker
				{
//					if (port43xx[dmaBase + 0x00] & 0x40)
//						logerror("Indirect HDMA - End of table read\n",port43xx[dmaBase + 0x0A]);
					bits&=~bMask;				// No more hdma table so clear hdma enable bit for rest of this screen update.
					continue;
				}
				SRC++;
				port43xx[dmaBase+0x08]=SRC&0xFF;
				port43xx[dmaBase+0x09]=(SRC>>8)&0xFF;
//				if (port43xx[dmaBase + 0x00] & 0x40)
//					logerror("Indirect HDMA - Table A2 Address now = %06X\n",SRC);
				if (port43xx[dmaBase + 0x00] & 0x40)				// Indirect hdma requested
				{
					// Update Indirect address pointer
					port43xx[dmaBase+0x05]=cpu_readmem24(SRC++);
					port43xx[dmaBase+0x06]=cpu_readmem24(SRC++);
//					if (port43xx[dmaBase + 0x00] & 0x40)
//						logerror("Indirect HDMA - Indirect Address %02X%02X\n",port43xx[dmaBase+0x06],port43xx[dmaBase+0x05]);

					// Update A2 table pointer
					port43xx[dmaBase+0x08]=SRC&0xFF;
					port43xx[dmaBase+0x09]=(SRC>>8)&0xFF;
//					if (port43xx[dmaBase + 0x00] & 0x40)
//						logerror("Indirect HDMA - Table A2 Address now = %06X\n",SRC);
				}
			}
			// If here we have a valid number of lines to do
			contMode=(--port43xx[dmaBase+0x0A])&0x80;			// If set continue mode is required
//			if (port43xx[dmaBase + 0x00] & 0x40 && contMode)
//				logerror("Indirect HDMA - Continue Mode\n");
			if (port43xx[dmaBase + 0x00] & 0x40)				// Indirect hdma requested
				SRC=INDIRECT(dmaBase);
			else												// Absolute mode
				SRC=TABLEA2(dmaBase);
//			if (port43xx[dmaBase + 0x00] & 0x40)
//				logerror("Indirect HDMA - Indirect Address = %06X\n",SRC);
			DST=BBUS(dmaBase);

			// Now we are here we can perform the hdma copy
			switch (port43xx[dmaBase + 0x00] & 0x07)
			{
				case 0x00:									// Write once L (1 src bytes)
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					break;
				case 0x01:									// Write once L,H (2 src bytes)
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					cpu_writemem24(DST+1,cpu_readmem24(SRC++));
					break;
				case 0x02:									// Write twice L,L (2 src bytes)
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					break;
				case 0x03:									// Write twice L,L H,H (4 src bytes)
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					cpu_writemem24(DST,cpu_readmem24(SRC++));
					cpu_writemem24(DST+1,cpu_readmem24(SRC++));
					cpu_writemem24(DST+1,cpu_readmem24(SRC++));
					break;
				default:
					logerror("HDMA Error : Unsupported HDMA Operation [%d]\n",port43xx[dmaBase + 0x00] & 0x07);
					return 0;
			}

			if (contMode)												// Continue mode so record updated pointer
			{
				if (port43xx[dmaBase + 0x00] & 0x40)					// Indirect mode so update Indirect table pointer
				{
//					if (port43xx[dmaBase + 0x00] & 0x40)
//						logerror("Indirect HDMA - Indirect Address Now = %06X\n",SRC);
					port43xx[dmaBase+0x05]=SRC&0xFF;
					port43xx[dmaBase+0x06]=(SRC>>8)&0xFF;
				}
				else													// Absolute mode so update SRC pointer in A2 table
				{
					port43xx[dmaBase+0x08]=SRC&0xFF;
					port43xx[dmaBase+0x09]=(SRC>>8)&0xFF;
				}
			}
//			if (port43xx[dmaBase + 0x00] & 0x40)
//				logerror("Indirect HDMA - Number Of Lines %02X\n",port43xx[dmaBase + 0x0A]&0x7F);
			if (!(port43xx[dmaBase+0x0A]&0x7F))							// If here last line
			{
				if (!(port43xx[dmaBase + 0x00] & 0x40))					// Absolute mode
				{
					if (!contMode)										// If not in continue mode then we update the A2 table address
					{
						port43xx[dmaBase+0x08]=SRC&0xFF;
						port43xx[dmaBase+0x09]=(SRC>>8)&0xFF;
					}
				}
			}
		}
	}

	return bits;
}

void RenderSNESScreenLine(struct mame_bitmap *bitmap,int curLine)
{
	static unsigned char hdmaEnable;						// Which hdma channels to look at this scanline
	int a,tmp;
	unsigned char SCR_TM=port21xx[0x2C]|port21xx[0x2D];		// Temp hack until sub screens properly supported
	unsigned short *dst=(unsigned short *)bitmap->line[curLine+16] + 16;
	unsigned char *zCur=&zBuffer[(256+32)*(curLine+16) + 16];

	if (curLine==0)											// If start of line then setup HDMA
		hdmaEnable=setupHDMA(port42xx[0x0C]);

	updateHDMA(hdmaEnable);

	for (a=0;a<4;a++)
		ScreenAddress[a]=&SNES_VRAM[(port21xx[0x07+a]&0xFC) << 9];

	TileAddress[0]=&SNES_VRAM[(port21xx[0x0B]&0x0F)<<13];
	TileAddress[1]=&SNES_VRAM[(port21xx[0x0B]&0xF0)<<9];
	TileAddress[2]=&SNES_VRAM[(port21xx[0x0C]&0x0F)<<13];
	TileAddress[3]=&SNES_VRAM[(port21xx[0x0C]&0xF0)<<9];

	if (port21xx[0x00]&0x80)
	{
		// Screen is forceably blanked this line
		for (a=0;a<256;a++)
			*dst++=Machine->pens[0];						// Black is at pos 0 in the pens
	}
	else
	{
		tmp=palIndx[0];
		if (port21xx[0x31] & 0x20)							// If back colour set then use fixed colour for bkg colour
			tmp=palIndx[256];
		for (a=0;a<256;a++)
		{
			dst[a]=Machine->pens[tmp];
			zCur[a]=0;
		}

		if (SCR_TM&0x10)									// Render sprites
			RenderSprites(dst,zCur,curLine);

		switch(port21xx[0x05]&0x07)				  			// Determine background mode
		{
			case 7:											// Mode 7 - 1 BPL 256 - Interleaved byte word map accessed via matrix maths
				RENDER_LINE_MODE7(dst,zCur,curLine,1);
				break;
			case 4:											// Mode 4 - 2 BPLS 256 / 4 colours
			case 5:											// Mode 5 - 2 BPLS 16 / 4 colours
			case 6:											// Mode 6 - 1 BPL 16 colour
				logerror("Unsuported Video Mode %d\n",port21xx[0x05]&0x07);
				break;
			case 0:											// Mode 0 - 4 BPLs 4 / 4 / 4 / 4 colours
				if (SCR_TM&0x01)
					RENDER_LINE(dst,zCur,curLine,0,RENDER_LINE_8x8_C2,0x10,4,0x00);
				if (SCR_TM&0x02)
					RENDER_LINE(dst,zCur,curLine,1,RENDER_LINE_8x8_C2,0x20,3,0x20);
				if (SCR_TM&0x04)
					RENDER_LINE(dst,zCur,curLine,2,RENDER_LINE_8x8_C2,0x40,2,0x40);
				if (SCR_TM&0x08)
					RENDER_LINE(dst,zCur,curLine,3,RENDER_LINE_8x8_C2,0x80,1,0x60);
				break;
			case 1:											// Mode 1 - 3 BPLS 16 / 16 / 4 colours
				if (port21xx[0x05]&0x08)	// if set plane 3 has highest priority
				{
					if (SCR_TM&0x04)
						RENDER_LINE(dst,zCur,curLine,2,RENDER_LINE_8x8_C2,0x40,3,0);
					if (SCR_TM&0x01)
						RENDER_LINE(dst,zCur,curLine,0,RENDER_LINE_8x8_C4,0x10,2,0);
					if (SCR_TM&0x02)
						RENDER_LINE(dst,zCur,curLine,1,RENDER_LINE_8x8_C4,0x20,1,0);
				}
				else
				{
					if (SCR_TM&0x01)
						RENDER_LINE(dst,zCur,curLine,0,RENDER_LINE_8x8_C4,0x10,2,0);
					if (SCR_TM&0x02)
						RENDER_LINE(dst,zCur,curLine,1,RENDER_LINE_8x8_C4,0x20,1,0);
					if (SCR_TM&0x04)
						RENDER_LINE(dst,zCur,curLine,2,RENDER_LINE_8x8_C2,0x40,0,0);
				}
				break;
			case 2:											// Mode 2 - 2 BPLS 16 / 16 colours  (OFFSET CHANGE MODE)
				if (SCR_TM&0x01)
					RENDER_LINE(dst,zCur,curLine,0,RENDER_LINE_8x8_C4,0x10,2,0);
				if (SCR_TM&0x02)
					RENDER_LINE(dst,zCur,curLine,1,RENDER_LINE_8x8_C4,0x20,1,0);
				break;
			case 3:											// Mode 3 - 2 BPLS 256 / 16 colours
				if (SCR_TM&0x01)
					RENDER_LINE(dst,zCur,curLine,0,RENDER_LINE_8x8_C8,0x10,2,0);
				if (SCR_TM&0x02)
					RENDER_LINE(dst,zCur,curLine,1,RENDER_LINE_8x8_C4,0x20,1,0);
				break;
		}
	}
}


