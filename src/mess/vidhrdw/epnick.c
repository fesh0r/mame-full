/* NICK GRAPHICS CHIP */
/* (found in Enterprise) */
/* this is a display list graphics chip, with bitmap,
character and attribute graphics modes. Each entry in the
display list defines a char line, with variable number of
scanlines. Colour modes are 2,4, 16 and 256 colour. 
Nick has 256 colours, 3 bits for R and G, with 2 bits for Blue.
It's a nice and flexible graphics processor..........
 */

#include "nick.h"
#include "epnick.h"

/*************************************************************/
/* MESS stuff */
static NICK_STATE Nick;

extern unsigned char *Enterprise_RAM;

// MESS specific
/* fetch a byte from "video ram" at Addr specified */
static char Nick_FetchByte(unsigned long Addr)
{
   return Enterprise_RAM[Addr & 0x0ffff];
}

// MESS specific
/* 8-bit pixel write! */
#define NICK_WRITE_PIXEL(ci, dest)	\
	*dest = Machine->pens[ci];	\
	dest++

/*****************************************************/


/* first clock visible on left hand side */
static unsigned char Nick_FirstVisibleClock;
/* first clock visible on right hand side */
static unsigned char Nick_LastVisibleClock;

/* No of highest resolution pixels per "clock" */
#define NICK_PIXELS_PER_CLOCK	16

/* "clocks" per line */
#define NICK_TOTAL_CLOCKS_PER_LINE	64

/* we align based on the clocks */
static void Nick_CalcVisibleClocks(int Width)
{
	/* number of clocks we can see */
	int NoOfVisibleClocks = Width/NICK_PIXELS_PER_CLOCK;
	
	Nick_FirstVisibleClock = 
		(NICK_TOTAL_CLOCKS_PER_LINE - NoOfVisibleClocks)>>1;

	Nick_LastVisibleClock = Nick_FirstVisibleClock + NoOfVisibleClocks;
}


/* given a bit pattern, this will get the pen index */
static unsigned int	Nick_PenIndexLookup_4Colour[256];
/* given a bit pattern, this will get the pen index */
static unsigned int	Nick_PenIndexLookup_16Colour[256];

void	Nick_Init(void)
{
	int i;

	memset(&Nick, 0, sizeof(NICK_STATE));

	for (i=0; i<256; i++)
	{
		int PenIndex;

		PenIndex = (
				((i & 0x080)>>(7-1)) | 
				((i & 0x08)>>(3-0))
				);
		
		Nick_PenIndexLookup_4Colour[i] = 

		PenIndex = (
				((i & 0x080)>>(7-3)) | 
				((i & 0x08)>>(3-2)) |
				((i & 0x020)>>(5-1)) |
				((i & 0x02)>>(1-0))
				);
				
		Nick_PenIndexLookup_16Colour[i] = PenIndex;
	}

	Nick_CalcVisibleClocks(50*16);	

//	Nick.BORDER = 0;
//	Nick.FIXBIAS = 0;	
}

/* write border colour */
static void	Nick_WriteBorder(int Clocks)
{
	int i;
	int ColIndex = Nick.BORDER;

	for (i=0; i<(Clocks<<4); i++)
	{
		NICK_WRITE_PIXEL(ColIndex, Nick.dest);
	}
}


static void Nick_DoLeftMargin(LPT_ENTRY *pLPT)
{
		unsigned char LeftMargin;

		LeftMargin = NICK_GET_LEFT_MARGIN(pLPT->LM);
	
		if (LeftMargin>Nick_FirstVisibleClock)
		{
			unsigned char LeftMarginVisible;

			/* some of the left margin is visible */
			LeftMarginVisible = LeftMargin-Nick_FirstVisibleClock;

			/* render the border */
			Nick_WriteBorder(LeftMarginVisible);
		}
}

static void Nick_DoRightMargin(LPT_ENTRY *pLPT)
{
		unsigned char RightMargin;

		RightMargin = NICK_GET_RIGHT_MARGIN(pLPT->RM);

		if (RightMargin<Nick_LastVisibleClock)
		{
			unsigned char RightMarginVisible;

			/* some of the right margin is visible */
			RightMarginVisible = Nick_LastVisibleClock - RightMargin;

			/* render the border */
			Nick_WriteBorder(RightMarginVisible);
		}
}

static void Nick_WritePixels2Colour(unsigned char Pen0, unsigned char Pen1, unsigned char DataByte)
{
	int i;
	int ColIndex;
	int PenIndex;
	unsigned char Data;

	Data = DataByte;

	for (i=0; i<8; i++)
	{
		if (Data & 0x080)
		{
			PenIndex = Pen1;
		}
		else
		{
			PenIndex = Pen0;
		}

		ColIndex = Nick.LPT.COL[PenIndex];

		NICK_WRITE_PIXEL(ColIndex, Nick.dest);
		
		Data = Data<<1;
	}
}


static void Nick_WritePixels(unsigned char DataByte, unsigned char CharIndex)
{
	int i;

	/* pen index colour 2-C (0,1), 4-C (0..3) 16-C (0..16) */
	int PenIndex;
	/* Col index = EP colour value */
	int PalIndex;
	unsigned char	ColourMode = NICK_GET_COLOUR_MODE(Nick.LPT.MB);
	unsigned char Data = DataByte;

	switch (ColourMode)
	{
		case NICK_2_COLOUR_MODE:
		{
			int PenOffset = 0;

			/* do before displaying byte */

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				if (Data & 0x080)
				{
					PenOffset |= 2;
				}

				Data &=~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				if (Data & 0x001)
				{
					PenOffset |= 4;
				}

				Data &=~0x01;
			}

			if (Nick.LPT.RM & NICK_RM_ALTIND1)
			{
				if (CharIndex & 0x080)
				{
					PenOffset|=0x02;
				}
			}

//			if (Nick.LPT.RM & NICK_RM_ALTIND0)
//			{
//				if (Data & 0x040)
//				{
//					PenOffset|=0x04;		
//				}
//			}


			
			Nick_WritePixels2Colour(PenOffset,
				(PenOffset|0x01), Data);
		}
		break;

		case NICK_4_COLOUR_MODE:
		{
			//printf("4 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<4; i++)
			{
				PenIndex = Nick_PenIndexLookup_4Colour[Data];
				PalIndex = Nick.LPT.COL[PenIndex & 0x03];

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

				Data = Data<<1;
 			}
		}
		break;

		case NICK_16_COLOUR_MODE:
		{
			//printf("16 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<2; i++)
			{
				PenIndex = Nick_PenIndexLookup_16Colour[Data];

				if (PenIndex & 0x08)
				{
					PalIndex = ((Nick.FIXBIAS & 0x01f)<<3) | (PenIndex & 0x07);
				}
				else
				{
					PalIndex = Nick.LPT.COL[PenIndex & 0x07];
				}

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
//				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

				Data = Data<<1;
			}
		}
		break;

		case NICK_256_COLOUR_MODE:
		{
			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			PalIndex = Data;

			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
//			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
//			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
//			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
//			NICK_WRITE_PIXEL(PalIndex, Nick.dest);


		}
		break; 
	}
}

static void Nick_DoPixel(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		
		Buf2 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Nick_WritePixels(Buf1,Buf1);

		Nick_WritePixels(Buf2,Buf1);
	}
}


static void Nick_DoLPixel(int ClocksVisible)
{
	int i;
	unsigned char Buf1;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Nick_WritePixels(Buf1,Buf1);
		Nick_WritePixels(Buf1,Buf1);
	}
}

static void Nick_DoAttr(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Buf2 = Nick_FetchByte(Nick.LD2);
		Nick.LD2++;		

		{
			unsigned char BackgroundColour = ((Buf1>>4) & 0x0f);
			unsigned char ForegroundColour = (Buf1 & 0x0f);

			Nick_WritePixels2Colour(BackgroundColour, ForegroundColour, Buf2);
		}
	}
}

static void Nick_DoCh256(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH256(Nick.LD2, Buf1));

		Nick_WritePixels(Buf2,Buf1);
	}
}

static void Nick_DoCh128(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH128(Nick.LD2, Buf1));
	
		Nick_WritePixels(Buf2,Buf1);
	}
}

static void Nick_DoCh64(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH64(Nick.LD2, Buf1));

		Nick_WritePixels(Buf2,Buf1);
	}
}


static void Nick_DoDisplay(LPT_ENTRY *pLPT)
{
	unsigned char ClocksVisible;
	unsigned char RightMargin, LeftMargin;

	LeftMargin = NICK_GET_LEFT_MARGIN(pLPT->LM);
	RightMargin = NICK_GET_RIGHT_MARGIN(pLPT->RM);

	ClocksVisible = RightMargin - LeftMargin;

	if (ClocksVisible!=0)
	{
		unsigned char DisplayMode;
	
		/* get display mode */
		DisplayMode = NICK_GET_DISPLAY_MODE(pLPT->MB);
	
		if ((Nick.ScanLineCount == 0))	// || 
			//((pLPT->MB & NICK_MB_VRES)==0))
		{
			/* doing first line */
			/* reload LD1, and LD2 (if necessary) regardless of display mode */
			Nick.LD1 = 	(pLPT->LD1L & 0x0ff) |
					((pLPT->LD1H & 0x0ff)<<8);
			
			if ((DisplayMode != NICK_LPIXEL_MODE) && (DisplayMode != NICK_PIXEL_MODE))
			{
				/* lpixel and pixel modes don't use LD2 */
				Nick.LD2 = (pLPT->LD2L & 0x0ff) |
					((pLPT->LD2H & 0x0ff)<<8);
			}
		}
		else
		{
			/* not first line */

			switch (DisplayMode)
			{
				case NICK_ATTR_MODE:
				{
					/* reload LD1 */
					Nick.LD1 = (pLPT->LD1L & 0x0ff) |
					((pLPT->LD1H & 0x0ff)<<8);
				}
				break;

				case NICK_CH256_MODE:
				case NICK_CH128_MODE:
				case NICK_CH64_MODE:
				{
					/* reload LD1 */
					Nick.LD1 = (pLPT->LD1L & 0x0ff) | 
						((pLPT->LD1H & 0x0ff)<<8);
					Nick.LD2++;
				}
				break;

				default:
					break;
			}
		}

		switch (DisplayMode)
		{
			case NICK_PIXEL_MODE:
			{
				Nick_DoPixel(ClocksVisible);
			}
			break;

			case NICK_ATTR_MODE:
			{
				//printf("attr mode\r\n");
				Nick_DoAttr(ClocksVisible);
			}
			break;

			case NICK_CH256_MODE:
			{
				//printf("ch256 mode\r\n");
				Nick_DoCh256(ClocksVisible);
			}
			break;

			case NICK_CH128_MODE:
			{	
				Nick_DoCh128(ClocksVisible);
			}
			break;

			case NICK_CH64_MODE:
			{
				//printf("ch64 mode\r\n");
				Nick_DoCh64(ClocksVisible);
			}
			break;

			case NICK_LPIXEL_MODE:
			{
				Nick_DoLPixel(ClocksVisible);
			}
			break;

			default:
				break;
		}
	}
}

void	Nick_UpdateLPT(void)
{
	unsigned long CurLPT;

	CurLPT = (Nick.LPL & 0x0ff) | ((Nick.LPH & 0x0f)<<8);
	CurLPT++;
	Nick.LPL = CurLPT & 0x0ff;
	Nick.LPH = (Nick.LPH & 0x0f0) | ((CurLPT>>8) & 0x0f);		
}


void	Nick_ReloadLPT(void)
{
	unsigned long LPT_Addr;
		
		/* get addr of LPT */
		LPT_Addr = ((Nick.LPL & 0x0ff)<<4) | ((Nick.LPH & 0x0f)<<(8+4));

		/* update internal LPT state */
		Nick.LPT.SC = Nick_FetchByte(LPT_Addr);
		Nick.LPT.MB = Nick_FetchByte(LPT_Addr+1);
		Nick.LPT.LM = Nick_FetchByte(LPT_Addr+2);
		Nick.LPT.RM = Nick_FetchByte(LPT_Addr+3);
		Nick.LPT.LD1L = Nick_FetchByte(LPT_Addr+4);
		Nick.LPT.LD1H = Nick_FetchByte(LPT_Addr+5);
		Nick.LPT.LD2L = Nick_FetchByte(LPT_Addr+6);
		Nick.LPT.LD2H = Nick_FetchByte(LPT_Addr+7);
		Nick.LPT.COL[0] = Nick_FetchByte(LPT_Addr+8);
		Nick.LPT.COL[1] = Nick_FetchByte(LPT_Addr+9);
		Nick.LPT.COL[2] = Nick_FetchByte(LPT_Addr+10);
		Nick.LPT.COL[3] = Nick_FetchByte(LPT_Addr+11);
		Nick.LPT.COL[4] = Nick_FetchByte(LPT_Addr+12);
		Nick.LPT.COL[5] = Nick_FetchByte(LPT_Addr+13);
		Nick.LPT.COL[6] = Nick_FetchByte(LPT_Addr+14);
		Nick.LPT.COL[7] = Nick_FetchByte(LPT_Addr+15);
}

/* call here to render a line of graphics */
void	Nick_DoLine(void)
{
	unsigned char ScanLineCount;

	if ((Nick.LPT.MB & NICK_MB_LPT_RELOAD)!=0)
	{
		/* reload LPT */

		Nick.LPL = Nick.Reg[2];
		Nick.LPH = Nick.Reg[3];

		Nick_ReloadLPT();
	}
		
		/* left border */
		Nick_DoLeftMargin(&Nick.LPT);

		/* do visible part */
		Nick_DoDisplay(&Nick.LPT);

		/* right border */
		Nick_DoRightMargin(&Nick.LPT);

	// 0x0f7 is first!	
	/* scan line count for this LPT */
	ScanLineCount = ((~Nick.LPT.SC)+1) & 0x0ff;

//	printf("ScanLineCount %02x\r\n",ScanLineCount);

	/* update count of scanlines done so far */
	Nick.ScanLineCount++;
	
	if (((unsigned char)Nick.ScanLineCount) == 
		((unsigned char)ScanLineCount))
	{
		/* done all scanlines of this Line Parameter Table, get next */


		Nick.ScanLineCount = 0;

		Nick_UpdateLPT();
		Nick_ReloadLPT();


	}
}

/* MESS specific */
int	Nick_vh_start(void)
{
  Nick_Init();
  return 0;
}

void	Nick_vh_stop(void)
{
}

int	Nick_reg_r(int RegIndex)
{
  /* read from a nick register - return floating bus,
   because the registers are not readable! */
  return 0x0ff;
}

void	Nick_reg_w(int RegIndex, int Data)
{
	//printf("Nick write %02x %02x\r\n",RegIndex, Data);

  /* write to a nick register */
  Nick.Reg[RegIndex & 0x0f] = Data;

  if ((RegIndex == 0x03) || (RegIndex == 0x02))
  {
    /* write LPH */

    /* reload LPT base? */
//    if (NICK_RELOAD_LPT(Data))
    {
      /* reload LPT base pointer */
      Nick.LPL = Nick.Reg[2];
      Nick.LPH = Nick.Reg[3];

	Nick_ReloadLPT();
    }
  }

  if (RegIndex == 0x01)
  {
	Nick.BORDER = Data; 
  }

  if (RegIndex == 0x00)
  {
	Nick.FIXBIAS = Data;
  }
}

void    Nick_DoScreen(struct osd_bitmap *bm)
{
  int line = 0;

  do
  {
  
    /* set write address for line */
    Nick.dest = bm->line[line];
    
    /* write line */
    Nick_DoLine();

    /* next line */
    line++;
  }
  while (((Nick.LPT.MB & 0x080)==0) && (line<(35*8)));

}






















