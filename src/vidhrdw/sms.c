/***************************************************************************
  vidhrdw/sms.c
  This file is heavyly based on the files SMS.c and Common.h in
  Marat Fayzullin's MG (see copyright notice below).

  070398 Mathis Rosenhauer

  070598 This is _NOT_ intended for public release. The whole
  VDP emulation needs to be rewritten from scratch because we
  are not allowed to use Marat's source in verbatim.
  
  Marat's code is enabled by #define OLD_VIDEO, otherwise the new code will
  be used.
  
***************************************************************************/

/** MasterGear: portable MasterSystem/GameGear emulator ******/
/**                                                         **/
/**                           SMS.c                         **/
/**                                                         **/
/** This file contains implementation for the SMS-specific  **/
/** hardware: VDP, interrupts, etc. Initialization code and **/
/** definitions needed for the machine-dependent drivers    **/
/** are also here.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994,1995,1996            **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/sms.h"
#include "vidhrdw/smsvdp.h"

static unsigned char Conv[256] =
{
	0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,
	0x30,0xB0,0x70,0xF0,0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,
	0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,0x04,0x84,0x44,0xC4,
	0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
	0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,
	0x3C,0xBC,0x7C,0xFC,0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,
	0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,0x0A,0x8A,0x4A,0xCA,
	0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
	0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,
	0x36,0xB6,0x76,0xF6,0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,
	0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,0x01,0x81,0x41,0xC1,
	0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
	0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,
	0x39,0xB9,0x79,0xF9,0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,
	0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,0x0D,0x8D,0x4D,0xCD,
	0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
	0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,
	0x33,0xB3,0x73,0xF3,0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,
	0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,0x07,0x87,0x47,0xC7,
	0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
	0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,
	0x3F,0xBF,0x7F,0xFF
};

extern unsigned int JoyState;                     /* State of joystick buttons        */

static unsigned char DelayedRD   = 0;             /* <>0: VRAM reads delayed by one   */
static unsigned char *VRAM,*PRAM;                 /* Video, and Palette RAMs          */
static unsigned char VDP[16];                     /* VDP registers                    */
static unsigned char VDPStatus;                   /* VDP Status register              */
static unsigned int VAddr;                        /* VRAM and Palette Address latches */
static unsigned char PAddr;
static unsigned char VKey,DKey,RKey;              /* VRAM Address and Access keys     */

static unsigned char MinLine,MaxLine;             /* Top/bottom scanlines to refresh  */
unsigned char GameGear;             /* <>0: GameGear emulation          */
static int  CURLINE;                              /* Current scanline being displayed */
static unsigned char AutoA=0,AutoB=0;             /* Joystick autofire ON/OFF switch  */

static unsigned char *ChrTab;                     /* Character Name Table             */
static unsigned char *SprTab,*SprGen;             /* Sprite Attribute/Pattern Tables  */
static unsigned char *ChrPal,*SprPal;             /* Palette Tables                   */
static unsigned char BGColor;                     /* Background color                 */
static unsigned char ScrollX,ScrollY;             /* Current screen scroll values     */

static void sms_vdp_set_scroll(void);
static void sms_vdp_change_register(int reg, int data);
static void sms_vdp_refresh_line(int Y, struct osd_bitmap *bitmap);
static void sms_vdp_refresh_sprites(unsigned char Line,unsigned char *ZBuf, struct osd_bitmap *bitmap);
static unsigned char sms_vdp_check_sprites(void);

/***************************************************************************
  sms_vdp_start
***************************************************************************/
#ifdef OLD_VIDEO
int sms_vdp_start(void)
{
	GameGear = 0;
	return sms_shared_vh_start ();
}

int gamegear_vdp_start(void)
{
	GameGear = 1;
	return sms_shared_vh_start ();
}
#else
int sms_vdp_start(void)
{
	GameGear = 0;
	return SMSVDP_start (0x4000);
}

int gamegear_vdp_start(void)
{
	GameGear = 1;
	return SMSVDP_start (0x4000);
}
#endif

int sms_shared_vh_start(void)
{
	static unsigned char VDPInit[16] =
	{
		0x00,0x60,0x0E,0x00,0x00,0x7F,0x00,0x00,
		0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00
	};

	if(!(VRAM=malloc(0x4040))) 
		return 1;

	memset(VRAM,NORAM,0x4040);
	PRAM=VRAM+0x4000;

	memcpy(VDP,VDPInit,16);                /* VDP registers              */
	VKey=DKey=RKey=1;                      /* Accessing VRAM...          */
	VAddr=0x0000;PAddr=0;                  /* ...from address 0000h      */
	BGColor=0;                             /* Background color           */
	CURLINE=0;                             /* Current scanline           */
	VDPStatus=0x00;                        /* VDP status register        */
	ChrTab=VRAM+0x3800;                    /* Screen buffer at 3800h     */
	SprTab=VRAM+0x3F00;                    /* Sprite attributes at 3F00h */
	SprGen=VRAM;                           /* Sprite patterns at 0000h   */
	ChrPal=PRAM;                           /* Screen palette             */
	SprPal=PRAM+(GameGear? 0x20:0x10);     /* Sprite palette             */
	sms_vdp_set_scroll();                  /* ScrollX,ScrollY            */
	MinLine=GameGear? (192-144)/2:0;       /* First scanline to refresh  */
	MaxLine=MinLine+(GameGear? 143:191);   /* Last scanline to refresh   */

	if (!(tmpbitmap = osd_create_bitmap(256,193)))
	{
		free(VRAM);
		return 1;
	}

	return 0;
}

/***************************************************************************
  sms_vdp_stop
***************************************************************************/
#ifdef OLD_VIDEO
void sms_vdp_stop(void)
{
	if (tmpbitmap)
		free(tmpbitmap);
	if(VRAM)
		free(VRAM);
	return;
}
#else
void sms_vdp_stop (void)
{
	SMSVDP_stop();
}
#endif

/*************************************************************
   sms_vdp_set_scroll()
**************************************************************/
void sms_vdp_set_scroll(void)
{
	ScrollX=NoTopScroll&&(CURLINE<MinLine+16)? 0:256-VDP[8];
	ScrollY=VDP[9];
}

#ifdef OLD_VIDEO
/***************************************************************************
  sms_vdp_vram_r (0xbe)
***************************************************************************/
int sms_vdp_vram_r(int offset)
{
	int Port;

	if(DKey)
	{
		Port=VRAM[VAddr];
		if(RKey&&DelayedRD) RKey=0;
		else VAddr=(VAddr+1)&0x3FFF;
	}
	else
	{
		Port=PRAM[PAddr];
		PAddr=(PAddr+1)&(GameGear? 0x3F:0x1F);
	}
	return Port;
}

/***************************************************************************
  sms_vdp_vram_w (0xbe)
***************************************************************************/
void sms_vdp_vram_w(int offset, int data)
{
//	if (errorlog) fprintf (errorlog, "SMS_VDP (old) vram_w, offset: %04x, data: %02x\n", VAddr, data);
	if (errorlog) fprintf (errorlog, "SMS_VDP vram_w, offset: %04x, data: %02x DKey: %02x\n", VAddr, data, DKey);
	
	VKey=1;
	if(DKey) { VRAM[VAddr]=data;VAddr=(VAddr+1)&0x3FFF;RKey=0; }
	else
	{
		unsigned char N,R,G,B;
		
		PRAM[PAddr]=data;
		if(GameGear)
		{
			N=PRAM[PAddr&0xFE];
			R=(N<<4)&0xE0;
			G=N&0xE0;
			B=(PRAM[PAddr|0x01]<<4)&0xE0;
			N=PAddr>>1;
			PAddr=(PAddr+1)&0x3F;
		}
		else
		{
			R=(data<<6)&0xff;
			G=(data<<4)&0xC0;
			B=(data<<2)&0xC0;
			N=PAddr;
			PAddr=(PAddr+1)&0x1F;
		}
		palette_change_color(N,R,G,B);
//		if (errorlog) fprintf (errorlog, "SMS color: %02x, r: %02x g: %02x b: %02x\n", N, R, G, B);
	}
}


/***************************************************************************
  sms_vdp_register_r (0xbd, 0xbf)
***************************************************************************/
int sms_vdp_register_r(int offset)
{
	int Port;

	if (errorlog) fprintf (errorlog, "SMS_VDP (old) status_r, data: %02x\n", VDPStatus);
	
	Port=VDPStatus;
	VDPStatus&=0x3F;
	return(Port);
}
#endif

/***************************************************************************
  sms_vdp_curline_r
***************************************************************************/
#ifdef OLD_VIDEO
int sms_vdp_curline_r (int offset)
{
	return (CURLINE<256? CURLINE:255);
}
#else
int sms_vdp_curline_r (int offset)
{
	return cpu_getscanline ();
}
#endif

/***************************************************************************
  sms_vdp_change_register
***************************************************************************/
void sms_vdp_change_register(int reg, int data)
{
  switch(reg)
  {
    case 2:  ChrTab=VRAM+((data<<10)&0x3800);break;
    case 5:  SprTab=VRAM+((data<< 7)&0x3F00);break;
    case 6:  SprGen=VRAM+((data<<11)&0x2000);break;
    case 7:  BGColor=data&0x0F;break;
    case 8:
    case 9:  VDP[reg]=data;sms_vdp_set_scroll();return;
  }

  if (errorlog) fprintf (errorlog, "SprGen: %04x, SprTab: %04x\n", (int) SprGen, (int) SprTab);
  VDP[reg]=data;
}

#ifdef OLD_VIDEO
/***************************************************************************
  sms_vdp_register_w (0xbd, 0xbf)
***************************************************************************/
void sms_vdp_register_w(int offset, int data)
{
	static unsigned int Addr;

	if (errorlog) fprintf (errorlog, "SMS_VDP (old) reg_w, data: %02x\n", data);
	
	if(VKey) { Addr=data;VKey=0; }
	else
	{
		VKey=1;
		switch(data&0xF0)
		{
		case 0xC0:
		case 0xD0:
		case 0xE0:
		case 0xF0:
			PAddr=Addr&(GameGear? 0x3F:0x1F);
			DKey=0;
			if (errorlog) fprintf (errorlog, "SMS_VDP color_w, data: %02x\n", data);
			break;
		case 0x80: sms_vdp_change_register(data&0x0F,Addr);break;
		default: VAddr=((data<<8)+Addr)&0x3FFF;DKey=1;
			RKey=data&0x40;break;
		}
	}
}

/***************************************************************************
  sms_refresh_line
***************************************************************************/
void sms_vdp_refresh_line(int Y, struct osd_bitmap *bitmap)
{
	unsigned char I,M,J,K,*T,R,*C,*Z;
	unsigned int L;
	int P, SX, SY;
	unsigned char X1,X2,XR,Offset,Shift;
	unsigned char ZBuf[35];

	/* Calculate the starting address of a picture in XBuf, ZBuf */
	P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2+WIDTH*Y;
	Z=ZBuf+1;

	L = Y + ScrollY;
	if(L>223) L-=223;
	T = ChrTab+((L&0xF8)<<3);
	Offset = (L&0x07)<<2;
	X2 = (ScrollX & 0xF8)>>2;
	Shift= ScrollX & 0x07;
	P-=Shift;Z--;
	Z[0]=0x00;
	XR=NoRghScroll? 24:255;
	
	for(X1=0;X1<33;X1++)
	{
		/* If right 9 columns not scrolled, modify T and Offset */
		if(X1==XR) { T= ChrTab+((int)(Y&0xF8)<<3);Offset=(Y&0x07)<<2; }
		
		C=T+X2;                             /* C <== Address of tile #, attrs */
		J=C[1];                             /* J <== Tile attributes          */
		
		C = VRAM
			+ (J&0x01? 0x2000:0x0000)           /* 0x01: Pattern table selection  */
			+ ((int)C[0]<<5)                    /* Pattern offset in the table    */
			+ (J&0x04? 28-Offset:Offset);       /* 0x04: Vertical flip            */
		
		R=J&0x08? 0x10:0;                   /* 0x08: Use sprite palette       */
		K=J&0x10;                           /* 0x10: Show in front of sprites */
		J&=0x02;                            /* 0x02: Horizontal flip          */
		
		I=M=J? Conv[C[0]]:C[0];
		L=((M&0x88)<<21)|((M&0x44)<<14)|((M&0x22)<<7)|(M&0x11);
		I|=M=J? Conv[C[1]]:C[1];
		L|=((M&0x88)<<22)|((M&0x44)<<15)|((M&0x22)<<8)|((M&0x11)<<1);
		I|=M=J? Conv[C[2]]:C[2];
		L|=((M&0x88)<<23)|((M&0x44)<<16)|((M&0x22)<<9)|((M&0x11)<<2);
		I|=M=J? Conv[C[3]]:C[3];
		L|=((M&0x88)<<24)|((M&0x44)<<17)|((M&0x22)<<10)|((M&0x11)<<3);
		

		SY = P/WIDTH;
		SX = P%WIDTH;
		bitmap->line[SY][SX]=Machine->pens[R+((L>>28)&0x0F)];
		bitmap->line[SY][SX+1]=Machine->pens[R+((L>>20)&0x0F)];
		bitmap->line[SY][SX+2]=Machine->pens[R+((L>>12)&0x0F)];
		bitmap->line[SY][SX+3]=Machine->pens[R+((L>>4)&0x0F)];
		bitmap->line[SY][SX+4]=Machine->pens[R+((L>>24)&0x0F)];
		bitmap->line[SY][SX+5]=Machine->pens[R+((L>>16)&0x0F)];
		bitmap->line[SY][SX+6]=Machine->pens[R+((L>>8)&0x0F)];
		bitmap->line[SY][SX+7]=Machine->pens[R+(L & 0x0F)];
		P+=8;

		if(K)
		{
			L=(unsigned int)I<<Shift;
			Z[0]|=L>>8;Z[1]=L&0xFF;
		}
		else Z[1]=0x00;
		
		X2=(X2+2)&0x3F;
		Z++;
	}
	/* Refresh sprites in this scanline */
	sms_vdp_refresh_sprites(Y,ZBuf+1,bitmap);
}


/** sms_vdp_refresh_sprites() *****************************************/
/** Refresh sprites in a given scanline.                             **/
/**********************************************************************/
void sms_vdp_refresh_sprites(unsigned char Line,unsigned char *ZBuf, struct osd_bitmap *bitmap)
{
	unsigned char H,J,*C,*Z,Shift;
	unsigned int L,M;
	int P,N,Y,X, SX, SY;

	/* Find the number of the last displayed sprite */
	for(N=0;(N<64)&&(SprY(N)!=208);N++);
	/* Find the sprite height */
	H = Sprites16? 16:8;

	for(N--;N>=0;N--)
	{
		Y=SprY(N)+1;
		if(Y>240) Y-=256;
		if((Line>=Y)&&(Line<Y+H))
		{
			X=SprX(N);
			if(LeftSprites) X-=8;
			J=SprP(N);
			if(H>8) J&=0xFE;
			C=SprGen+((int)J<<5)+((int)(Line-Y)<<2);
			P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2+WIDTH*Line+X;
			Z=ZBuf+((X&0xFF)>>3);
			Shift=8-(X&0x07);
			
			J=M=C[0];
			L=((M&0x88)<<21)|((M&0x44)<<14)|((M&0x22)<<7)|(M&0x11);
			J|=M=C[1];
			L|=((M&0x88)<<22)|((M&0x44)<<15)|((M&0x22)<<8)|((M&0x11)<<1);
			J|=M=C[2];
			L|=((M&0x88)<<23)|((M&0x44)<<16)|((M&0x22)<<9)|((M&0x11)<<2);
			J|=M=C[3];
			L|=((M&0x88)<<24)|((M&0x44)<<17)|((M&0x22)<<10)|((M&0x11)<<3);
			J&=~((((int)Z[0]<<8)|Z[1])>>Shift);
			
			if(J)
			{
				SY = P/WIDTH;
				SX = P%WIDTH;
				if(J&0x80) bitmap->line[SY][SX]=Machine->pens[0x10+((L>>28)&0x0F)];
				if(J&0x40) bitmap->line[SY][SX+1]=Machine->pens[0x10+((L>>20)&0x0F)];
				if(J&0x20) bitmap->line[SY][SX+2]=Machine->pens[0x10+((L>>12)&0x0F)];
				if(J&0x10) bitmap->line[SY][SX+3]=Machine->pens[0x10+((L>>4)&0x0F)];
				if(J&0x08) bitmap->line[SY][SX+4]=Machine->pens[0x10+((L>>24)&0x0F)];
				if(J&0x04) bitmap->line[SY][SX+5]=Machine->pens[0x10+((L>>16)&0x0F)];
				if(J&0x02) bitmap->line[SY][SX+6]=Machine->pens[0x10+((L>>8)&0x0F)];
				if(J&0x01) bitmap->line[SY][SX+7]=Machine->pens[0x10+(L&0x0F)];
			}
		}
	}
}

/** sms_vdp_check_sprites() **********************************/
/** This function is periodically called to check for the   **/
/** sprite collisions and set appropriate bit in the VDP    **/
/** status register.                                        **/
/*************************************************************/
unsigned char sms_vdp_check_sprites(void)
{ 
	int Max,N1,N2,X1,X2,Y1,Y2,H;
       
	for(Max=0;(Max<64)&&(SprY(Max)!=208);Max++);
	H=Sprites16? 16:8;   
	
	for(N1=0;N1<Max-1;N1++)
	{
		Y1=SprY(N1)+1;X1=SprX(N1);
		if((Y1<192)||(Y1>240))
			for(N2=N1+1;N2<Max;N2++)
			{
				Y2=SprY(N2)+1;X2=SprX(N2);
				if((Y2<192)||(Y2>240))
					if((X2>X1-8)&&(X2<X1+8)&&(Y2>Y1-H)&&(Y2<Y1+H)) return(1);
			}
	}
	
	return(0);
}

/***************************************************************************
  sms_vdp_refresh
***************************************************************************/
void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int I,J;

	if (!(ScreenON))
	{
		fillbitmap(bitmap,Machine->pens[BGColor],&Machine->drv->visible_area);
		return;
	}

	if(LeftMask)
		for(J=0;J<192;J++)
			for (I=0;I<8;I++)
				tmpbitmap->line[J][I]=Machine->pens[BGColor];

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	return;
}
#else
void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	SMSVDP_refresh (bitmap);
}
#endif

#ifdef OLD_VIDEO
/***************************************************************************
  sms_interrupt
***************************************************************************/
int sms_interrupt(void)
{
	static unsigned char NeedVBlank = 0, LinesLeft=255;
	static int ACount = 0;
	int J;

	CURLINE=(CURLINE+1)%SCANLINES_PER_FRAME;

	if(CURLINE==193)
	{
		/* Polling joypads */
		J=(input_port_0_r(0) + ((input_port_1_r(0))<<8));
		
		/* We preserve lightgun bits */
		JoyState=(JoyState&0xC000)|(~J&0x3FFF);
	
		if(GameGear)
		{
			JoyState&=(JoyState>>6)|0xFFC0; /* A single joypad */
			JoyState|=0x1FC0;               /* No RESET button */
			J&=0xFFFF;                      /* No PAUSE button */
		}
		else
		{
			JoyState&=(JoyState>>9)|0xFFEF; /* [START] = [A]   */
		}
	
		/* Autofire emulation */
		ACount=(ACount+1)&0x07;
		if(ACount>3)
		{
			if(AutoA) JoyState|=0x0410;
			if(AutoB) JoyState|=0x0820;
		}
		/* If PAUSE pressed, generate NMI */
		if(J&0x10000)
			return nmi_interrupt();
	}

	if(CURLINE>=193)
	{
		if((CURLINE<240)&&(VDPStatus&0x80)&& VBlankON && VKey)
			return 0x38;
	}
	else
	{
		if((CURLINE>=MinLine)&&(CURLINE<=MaxLine))
		{
			if(!CURLINE||(CURLINE==16))
				sms_vdp_set_scroll();
			sms_vdp_refresh_line(CURLINE, tmpbitmap);
		}
		if(CURLINE==MaxLine)
		{
			/* Checking for sprite collisions */
			if(sms_vdp_check_sprites()) 
				VDPStatus|=0x20;
			else
				VDPStatus&=0xDF;
		}
		
		/* We need VBlank at line 192 */
		if(CURLINE==192)
			NeedVBlank=1;

		/* Generate line interrupts */
		if(!CURLINE)
			LinesLeft=VDP[10];
		if(LinesLeft)
			LinesLeft--;
		else
		{ 
			LinesLeft=VDP[10];
			VDPStatus|=0x40;
		}

		/* If line interrupt is generated in line 192, */
		/* the VBlank flag is not set yet.             */
		if((VDPStatus&0x40) && HBlankON)
		{
			if (errorlog) fprintf (errorlog, "HBlank IRQ generated at %i\n",CURLINE);
			return 0x38;
		}
	}
	
	/* Setting VBlank flag if needed */
	if(NeedVBlank)
	{ 
		NeedVBlank=0;
		VDPStatus |= 0x80;
	}
	
	return ignore_interrupt();
}
#endif