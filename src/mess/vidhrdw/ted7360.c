/***************************************************************************

	TExt Display 7360

***************************************************************************/
#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/c16.h"
#include "mess/machine/vc20tape.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"

#include "ted7360.h"

/*#define GFX */

#define VREFRESHINLINES 28

#define TIMER1HELPER (ted7360[0]|(ted7360[1]<<8))
#define TIMER2HELPER (ted7360[2]|(ted7360[3]<<8))
#define TIMER3HELPER (ted7360[4]|(ted7360[5]<<8))
#define TIMER1 (TIMER1HELPER?TIMER1HELPER:0x10000)
#define TIMER2 (TIMER2HELPER?TIMER2HELPER:0x10000)
#define TIMER3 (TIMER3HELPER?TIMER3HELPER:0x10000)
#define TEDTIME_IN_CYCLES(a) ((double)(a)/TED7360_CLOCK)
#define TEDTIME_TO_CYCLES(a) ((int)((a)*TED7360_CLOCK))

#define TED7360_YPOS 40
#define RASTERLINE_2_C16(a) ((a+lines-TED7360_YPOS-5)%lines)
#define C16_2_RASTERLINE(a) ((a+TED7360_YPOS+5)%lines)
#define XPOS 8
#define YPOS 8

#define SCREENON (ted7360[6]&0x10)
#define TEST (ted7360[6]&0x80)
#define VERTICALPOS (ted7360[6]&7)
#define HORICONTALPOS (ted7360[7]&7)
#define ECMON (ted7360[6]&0x40)
#define HIRESON (ted7360[6]&0x20)
#define MULTICOLORON (ted7360[7]&0x10)
#define REVERSEON (!(ted7360[7]&0x80))
/* hardware inverts character when bit 7 set (character taken &0x7f) */
/* instead of fetching character with higher number! */
#define LINES25 (ted7360[6]&8) /* else 24 Lines */
#define LINES (LINES25?25:24)
#define YSIZE (LINES*8)
#define COLUMNS40 (ted7360[7]&8) /* else 38 Columns */
#define COLUMNS (COLUMNS40?40:38)
#define XSIZE (COLUMNS*8)

#define INROM (ted7360[0x12]&4)
#define CHARGENADDR (REVERSEON&&!HIRESON&&!MULTICOLORON?\
		     ((ted7360[0x13]&0xfc)<<8):\
		     ((ted7360[0x13]&0xf8)<<8) )
#define BITMAPADDR ((ted7360[0x12]&0x38)<<10)
#define VIDEOADDR ((ted7360[0x14]&0xf8)<<8)

#define RASTERLINE ( ((ted7360[0xa]&1)<<8)|ted7360[0xb])
#define CURSOR1POS ( ted7360[0xd]|((ted7360[0xc]&3)<<8) )
#define CURSOR2POS ( ted7360[0x1b]|((ted7360[0x1a]&3)<<8) )
#define CURSORRATE ( (ted7360[0x1f]&0x7c)>>2 )

#define BACKGROUNDCOLOR (ted7360[0x15]&0x7f)
#define FOREGROUNDCOLOR (ted7360[0x16]&0x7f)
#define MULTICOLOR1 (ted7360[0x17]&0x7f)
#define MULTICOLOR2 (ted7360[0x18]&0x7f)
#define FRAMECOLOR (ted7360[0x19]&0x7f)

unsigned char ted7360_palette[] = {
/* black, white, red, cyan */
/* purple, green, blue, yellow */
/* orange, light orange, pink, light cyan, */
/* light violett, light green, light blue, light yellow */
/* these 16 colors are 8 times here in different luminance (dark..light) */
/* taken from digitized tv screenshot */
0x06,0x01,0x03, 0x2b,0x2b,0x2b, 0x67,0x0e,0x0f, 0x00,0x3f,0x42,
0x57,0x00,0x6d, 0x00,0x4e,0x00, 0x19,0x1c,0x94, 0x38,0x38,0x00,
0x56,0x20,0x00, 0x4b,0x28,0x00, 0x16,0x48,0x00, 0x69,0x07,0x2f,
0x00,0x46,0x26, 0x06,0x2a,0x80, 0x2a,0x14,0x9b, 0x0b,0x49,0x00,

0x00,0x03,0x02, 0x3d,0x3d,0x3d, 0x75,0x1e,0x20, 0x00,0x50,0x4f,
0x6a,0x10,0x78, 0x04,0x5c,0x00, 0x2a,0x2a,0xa3, 0x4c,0x47,0x00,
0x69,0x2f,0x00, 0x59,0x38,0x00, 0x26,0x56,0x00, 0x75,0x15,0x41,
0x00,0x58,0x3d, 0x15,0x3d,0x8f, 0x39,0x22,0xae, 0x19,0x59,0x00,

0x00,0x03,0x04, 0x42,0x42,0x42, 0x7b,0x28,0x20, 0x02,0x56,0x59,
0x6f,0x1a,0x82, 0x0a,0x65,0x09, 0x30,0x34,0xa7, 0x50,0x51,0x00,
0x6e,0x36,0x00, 0x65,0x40,0x00, 0x2c,0x5c,0x00, 0x7d,0x1e,0x45,
0x01,0x61,0x45, 0x1c,0x45,0x99, 0x42,0x2d,0xad, 0x1d,0x62,0x00,

0x05,0x00,0x02, 0x56,0x55,0x5a, 0x90,0x3c,0x3b, 0x17,0x6d,0x72,
0x87,0x2d,0x99, 0x1f,0x7b,0x15, 0x46,0x49,0xc1, 0x66,0x63,0x00,
0x84,0x4c,0x0d, 0x73,0x55,0x00, 0x40,0x72,0x00, 0x91,0x33,0x5e,
0x19,0x74,0x5c, 0x32,0x59,0xae, 0x59,0x3f,0xc3, 0x32,0x76,0x00,

0x02,0x01,0x06, 0x84,0x7e,0x85, 0xbb,0x67,0x68, 0x45,0x96,0x96,
0xaf,0x58,0xc3, 0x4a,0xa7,0x3e, 0x73,0x73,0xec, 0x92,0x8d,0x11,
0xaf,0x78,0x32, 0xa1,0x80,0x20, 0x6c,0x9e,0x12, 0xba,0x5f,0x89,
0x46,0x9f,0x83, 0x61,0x85,0xdd, 0x84,0x6c,0xef, 0x5d,0xa3,0x29,

0x02,0x00,0x0a, 0xb2,0xac,0xb3, 0xe9,0x92,0x92, 0x6c,0xc3,0xc1,
0xd9,0x86,0xf0, 0x79,0xd1,0x76, 0x9d,0xa1,0xff, 0xbd,0xbe,0x40,
0xdc,0xa2,0x61, 0xd1,0xa9,0x4c, 0x93,0xc8,0x3d, 0xe9,0x8a,0xb1,
0x6f,0xcd,0xab, 0x8a,0xb4,0xff, 0xb2,0x9a,0xff, 0x88,0xcb,0x59,

0x02,0x00,0x0a, 0xc7,0xca,0xc9, 0xff,0xac,0xac, 0x85,0xd8,0xe0,
0xf3,0x9c,0xff, 0x92,0xea,0x8a, 0xb7,0xba,0xff, 0xd6,0xd3,0x5b,
0xf3,0xbe,0x79, 0xe6,0xc5,0x65, 0xb0,0xe0,0x57, 0xff,0xa4,0xcf,
0x89,0xe5,0xc8, 0xa4,0xca,0xff, 0xca,0xb3,0xff, 0xa2,0xe5,0x7a,

0x01,0x01,0x01, 0xff,0xff,0xff, 0xff,0xf6,0xf2, 0xd1,0xff,0xff,
0xff,0xe9,0xff, 0xdb,0xff,0xd3, 0xfd,0xff,0xff, 0xff,0xff,0xa3,
0xff,0xff,0xc1, 0xff,0xff,0xb2, 0xfc,0xff,0xa2, 0xff,0xee,0xff,
0xd1,0xff,0xff, 0xeb,0xff,0xff, 0xff,0xf8,0xff, 0xed,0xff,0xbc
};

struct CustomSound_interface ted7360_sound_interface = {
  ted7360_custom_start,
  ted7360_custom_stop,
  ted7360_custom_update
};

UINT8 ted7360[0x20]={0};

bool ted7360_pal;
bool ted7360_rom;

static int lines;
static void *timer1=0, *timer2=0, *timer3=0;
static bool cursor1=false;
static int(*vic_dma_read)(int);
static int(*vic_dma_read_rom)(int);
static int chargenaddr, bitmapaddr, videoaddr;

static int x_begin, x_end;
static int y_begin, y_end;

static UINT16 c16_bitmap[2], bitmapmulti[4],
  mono[2], monoinversed[2], multi[4], ecmcolor[2], colors[5];

static struct osd_bitmap *ted7360_bitmap; /* Machine->scrbitmap for speedup */
static int rasterline=0,lastline=0;
static double rastertime;
static void ted7360_drawlines(int first, int last);

static unsigned short cursorcolortable[2]={0};
static struct GfxLayout cursorlayout = {
  8,8,
  1,
  1,
  { 0 },
  { 0,1,2,3,4,5,6,7 },
  { 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
  8*8
};

static UINT8 cursormask[]= { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
static struct GfxElement *cursorelement;

void ted7360_init(int pal)
{
  ted7360_pal=pal;
  lines=TED7360_LINES;
  chargenaddr=bitmapaddr=videoaddr=0;
}

void ted7360_set_dma(int(*dma_read)(int), int(*dma_read_rom)(int))
{
  vic_dma_read=dma_read;
  vic_dma_read_rom=dma_read_rom;
}

static void ted7360_set_interrupt(int mask)
{
  /* kernel itself polls for timer 2 shot (interrupt disabled!)
     when cassette loading */
  ted7360[9]|=mask;
  if ( (ted7360[0xa]&ted7360[9]&0x5e) ) {
    if (!(ted7360[9]&0x80) ) {
      DBG_LOG(1,"ted7360",(errorlog,"irq start %.2x\n",mask));
      ted7360[9]|=0x80;
      c16_interrupt(1);
    }
  }
  ted7360[9]|=mask;
}

static void ted7360_clear_interrupt(int mask)
{
  ted7360[9]&=~mask;
  if ((ted7360[9]&0x80)&&!(ted7360[9]&ted7360[0xa]&0x5e)) {
    DBG_LOG(1,"ted7360",(errorlog,"irq end %.2x\n",mask));
    ted7360[9]&=~0x80;
    c16_interrupt(0);
  }
}

static int ted7360_rastercolumn(void)
{
  return (int)((timer_get_time()-rastertime)
	       *TED7360_VRETRACERATE*lines*57*8+0.5);
}

static void ted7360_timer_timeout(int which)
{
  DBG_LOG(3,"ted7360 ",(errorlog,"timer %d timeout\n",which));
  switch (which) {
  case 1:
    timer1=timer_set(TEDTIME_IN_CYCLES(0x10000),1, ted7360_timer_timeout );
    ted7360_set_interrupt(8);
    break;
  case 2:
    timer2=timer_set(TEDTIME_IN_CYCLES(0x10000),2, ted7360_timer_timeout );
    ted7360_set_interrupt(0x10);
    break;
  case 3:
    timer3=timer_set(TEDTIME_IN_CYCLES(0x10000),3, ted7360_timer_timeout );
    ted7360_set_interrupt(0x40);
    break;
  }
}

int ted7360_frame_interrupt(void)
{
  static int count;
  if ((ted7360[0x1f]&0xf)>=0x0f) {
/*	if (count>=CURSORRATE) { */
    cursor1^=true;
    ted7360[0x1f]&=~0xf;
    count=0;
  } else ted7360[0x1f]++;
  return ignore_interrupt();
}

void ted7360_port_w(int offset, int data)
{
  int old;
  if ((offset!=8)&&((offset<0x15)||(offset>0x19))) {
    DBG_LOG(1,"ted7360_port_w",(errorlog,"%.2x:%.2x\n",offset,data));
  }
  switch (offset) {
  case 0xe:case 0xf:case 0x10:case 0x11:case 0x12:
    ted7360_soundport_w(offset,data);
    break;
  }
  switch (offset) {
  case 0: /* stop timer 1 */
    ted7360[offset]=data;
    if (timer1) {
      ted7360[1]=TEDTIME_TO_CYCLES(timer_timeleft(timer1))>>8;
      timer_remove(timer1);timer1=0;
    }
    break;
  case 1: /* start timer 1 */
    ted7360[offset]=data;
    if (!timer1)
      timer1=timer_set(TEDTIME_IN_CYCLES(TIMER1),1, ted7360_timer_timeout );
    else timer_reset(timer1,TEDTIME_IN_CYCLES(TIMER1) );
    break;
  case 2: /* stop timer 2 */
    ted7360[offset]=data;
    if (timer2) {
      ted7360[3]=TEDTIME_TO_CYCLES(timer_timeleft(timer2))>>8;
      timer_remove(timer2);timer2=0;
    }
    break;
  case 3: /* start timer 2 */
    ted7360[offset]=data;
    if (!timer2)
      timer2=timer_set(TEDTIME_IN_CYCLES(TIMER2),2, ted7360_timer_timeout );
    else timer_reset(timer2,TEDTIME_IN_CYCLES(TIMER2) );
    break;
  case 4: /* stop timer 3 */
    ted7360[offset]=data;
    if (timer3) {
      ted7360[5]=TEDTIME_TO_CYCLES(timer_timeleft(timer3))>>8;
      timer_remove(timer3);timer3=0;
    }
    break;
  case 5: /* start timer 3 */
    ted7360[offset]=data;
    if (!timer3)
      timer3=timer_set(TEDTIME_IN_CYCLES(TIMER3),3, ted7360_timer_timeout );
    else timer_reset(timer3,TEDTIME_IN_CYCLES(TIMER3) );
    break;
  case 6:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      if (LINES25) { y_begin=0; y_end=y_begin+200; }
      else { y_begin=4; y_end=y_begin+192; }
      chargenaddr=CHARGENADDR;
    }
    break;
  case 7:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      if (COLUMNS40) { x_begin=0; x_end=x_begin+320; }
      else { x_begin=HORICONTALPOS; x_end=x_begin+320; }
      DBG_LOG(3,"ted7360_port_w",(errorlog,"%s %s\n",data&0x40?"ntsc":"pal",
				  data&0x20?"hori freeze":""));
      chargenaddr=CHARGENADDR;
    }
    break;
  case 8:
    ted7360[offset]=c16_read_keyboard(data);
    break;
  case 9:
    if (data&8) ted7360_clear_interrupt(8);
    if (data&0x10) ted7360_clear_interrupt(0x10);
    if (data&0x40) ted7360_clear_interrupt(0x40);
    if (data&2) ted7360_clear_interrupt(2);
    break;
  case 0xa:
    old=data;
    ted7360[offset]=data|0xa0;
#if 0
    ted7360[9]=(ted7360[9]&0xa1)|(ted7360[9]&data&0x5e);
    if (ted7360[9]&0x80) ted7360_clear_interrupt(0);
#endif
    if ((data^old)&1) {
/*    DBG_LOG(1,"set rasterline hi",(errorlog,"soll:%d\n",RASTERLINE)); */
    }
    break;
  case 0xb:
    if (data!=ted7360[offset]) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      /*  DBG_LOG(1,"set rasterline lo",(errorlog,"soll:%d\n",RASTERLINE)); */
    }
    break;
  case 0xc:case 0xd:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
    }
    break;
  case 0x12:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      bitmapaddr=BITMAPADDR;
      chargenaddr=CHARGENADDR;
      DBG_LOG(3,"ted7360_port_w",(errorlog,"bitmap %.4x %s\n",
				  BITMAPADDR,INROM?"rom":"ram"));
    }
    break;
  case 0x13:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      chargenaddr=CHARGENADDR;
      DBG_LOG(3,"ted7360_port_w",(errorlog,"chargen %.4x %s %d\n",
				  CHARGENADDR,data&2?"":"doubleclock",
				  data &1));
    }
    break;
  case 0x14:
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      videoaddr=VIDEOADDR;
      DBG_LOG(3,"ted7360_port_w",(errorlog,"videoram %.4x\n",
				  VIDEOADDR));
    }
    break;
  case 0x15: /* backgroundcolor */
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      monoinversed[1]=mono[0]=bitmapmulti[0]=multi[0]=colors[0]=
	Machine->pens[BACKGROUNDCOLOR];
    }
    break;
  case 0x16: /* foregroundcolor */
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      bitmapmulti[3]=multi[1]=colors[1]=Machine->pens[FOREGROUNDCOLOR];
    }
    break;
  case 0x17: /* multicolor 1 */
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      multi[2]=colors[2]=Machine->pens[MULTICOLOR1];
    }
    break;
  case 0x18: /* multicolor 2 */
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      colors[3]=Machine->pens[MULTICOLOR2];
    }
    break;
  case 0x19: /* framecolor */
    if (ted7360[offset]!=data) {
      ted7360_drawlines(lastline, rasterline);
      ted7360[offset]=data;
      colors[4]=Machine->pens[FRAMECOLOR];
    }
    break;
  case 0x1c:
      ted7360[offset]=data; /*? */
      DBG_LOG(1,"ted7360_port_w",(errorlog,"write to rasterline high %.2x\n",
				  data));
      break;
  case 0x1f:
    ted7360[offset]=data;
    DBG_LOG(1,"ted7360_port_w",(errorlog,"write to cursorblink %.2x\n",data));
    break;
  default:
    ted7360[offset]=data;
    break;
  }
}

int ted7360_port_r(int offset)
{
  int val=0;

  switch(offset) {
  case 0:
    if (timer1) val=TEDTIME_TO_CYCLES(timer_timeleft(timer1))&0xff;
    else val=ted7360[offset];
    break;
  case 1:
    if (timer1) val=TEDTIME_TO_CYCLES(timer_timeleft(timer1))>>8;
    else val=ted7360[offset];
    break;
  case 2:
    if (timer2) val=TEDTIME_TO_CYCLES(timer_timeleft(timer2))&0xff;
    else val=ted7360[offset];
    break;
  case 3:
    if (timer2) val=TEDTIME_TO_CYCLES(timer_timeleft(timer2))>>8;
    else val=ted7360[offset];
    break;
  case 4:
    if (timer3) val=TEDTIME_TO_CYCLES(timer_timeleft(timer3))&0xff;
    else val=ted7360[offset];
    break;
  case 5:
    if (timer3) val=TEDTIME_TO_CYCLES(timer_timeleft(timer3))>>8;
    else val=ted7360[offset];
    break;
  case 7:
    val=(ted7360[offset]&~0x40);
    if (!ted7360_pal) val|=0x40;
    break;
  case 9:
    val=ted7360[offset]|1;
    break;
  case 0xa:
    val=ted7360[offset];
    break;
  case 0xb:
    val=ted7360[offset];
    break;
  case 0x0c:
    val=ted7360[offset]|=0xfc;
    break;
  case 0x13:
    val=ted7360[offset]&~1;
    if (ted7360_rom) val|=1;
    break;
  case 0x1c: /*rasterline */
    ted7360_drawlines(lastline, rasterline);
    val=((RASTERLINE_2_C16(rasterline)&0x100)>>8)|0xfe; /* expected by matrix */
    break;
  case 0x1d: /*rasterline */
    ted7360_drawlines(lastline, rasterline);
    val=RASTERLINE_2_C16(rasterline)&0xff;
    break;
  case 0x1e: /*rastercolumn */
    val=ted7360_rastercolumn()/2; /* pengo >=0x99 */
    break;
  case 0x1f:
    val=((rasterline&7)<<4)|(ted7360[offset]&0x0f);
    DBG_LOG(1,"ted7360_port_w",(errorlog,"read from cursorblink %.2x\n",val));
    break;
  default:
    val=ted7360[offset];
    break;
  }
  if ((offset!=8)&&(offset>=6)&&(offset!=0x1c)&&(offset!=0x1d)&&(offset!=9)
      &&((offset<0x15)||(offset>0x19)) ){
    DBG_LOG(1,"ted7360_port_r",(errorlog,"%.2x:%.2x\n",offset,val));
  }
  return val;
}

int ted7360_vh_start(void)
{
  cursorelement=decodegfx(cursormask,&cursorlayout);
  cursorelement->colortable= cursorcolortable;
  cursorcolortable[1]=Machine->pens[1];
  cursorelement->total_colors= 2;
  ted7360_bitmap=Machine->scrbitmap;
  return 0;
}

void ted7360_vh_stop(void)
{
  freegfx(cursorelement);
}

static void ted7360_draw_character(int ybegin, int yend,int ch, int yoff, int xoff,
			   UINT16* color)
{
  int y,code;

  if (Machine->color_depth==8) {
    for (y=ybegin; y<=yend; y++) {
      if (INROM) code=vic_dma_read_rom(chargenaddr+ch*8+y);
      else code=vic_dma_read(chargenaddr+ch*8+y);
      ted7360_bitmap->line[y+yoff][xoff]=color[code>>7];
      ted7360_bitmap->line[y+yoff][1+xoff]=color[(code>>6)&1];
      ted7360_bitmap->line[y+yoff][2+xoff]=color[(code>>5)&1];
      ted7360_bitmap->line[y+yoff][3+xoff]=color[(code>>4)&1];
      ted7360_bitmap->line[y+yoff][4+xoff]=color[(code>>3)&1];
      ted7360_bitmap->line[y+yoff][5+xoff]=color[(code>>2)&1];
      ted7360_bitmap->line[y+yoff][6+xoff]=color[(code>>1)&1];
      ted7360_bitmap->line[y+yoff][7+xoff]=color[code&1];
    }
  } else {
    for (y=ybegin; y<=yend; y++) {
      if (INROM) code=vic_dma_read_rom(chargenaddr+ch*8+y);
      else code=vic_dma_read(chargenaddr+ch*8+y);
      *((short*)ted7360_bitmap->line[y+yoff]+xoff)=color[code>>7];
      *((short*)ted7360_bitmap->line[y+yoff]+1+xoff)=color[(code>>6)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+2+xoff)=color[(code>>5)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+3+xoff)=color[(code>>4)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+4+xoff)=color[(code>>3)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+5+xoff)=color[(code>>2)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+6+xoff)=color[(code>>1)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+7+xoff)=color[code&1];
    }
  }
}

static void ted7360_draw_character_multi(int ybegin, int yend, int ch, int yoff,
				 int xoff)
{
  int y,code;

  if (Machine->color_depth==8) {
    for (y=ybegin; y<=yend; y++) {
      if (INROM) code=vic_dma_read_rom(chargenaddr+ch*8+y);
      else code=vic_dma_read(chargenaddr+ch*8+y);
      ted7360_bitmap->line[y+yoff][xoff]=
	ted7360_bitmap->line[y+yoff][xoff+1]=multi[code>>6];
      ted7360_bitmap->line[y+yoff][xoff+2]=
	ted7360_bitmap->line[y+yoff][xoff+3]=multi[(code>>4)&3];
      ted7360_bitmap->line[y+yoff][xoff+4]=
	ted7360_bitmap->line[y+yoff][xoff+5]=multi[(code>>2)&3];
      ted7360_bitmap->line[y+yoff][xoff+6]=
	ted7360_bitmap->line[y+yoff][xoff+7]=multi[code&3];
    }
  } else {
    for (y=ybegin; y<=yend; y++) {
      if (INROM) code=vic_dma_read_rom(chargenaddr+ch*8+y);
      else code=vic_dma_read(chargenaddr+ch*8+y);
      *((short*)ted7360_bitmap->line[y+yoff]+xoff)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+1)=multi[code>>6];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+2)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+3)=multi[(code>>4)&3];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+4)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+5)=multi[(code>>2)&3];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+6)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+7)=multi[code&3];
    }
  }
}

static void ted7360_draw_bitmap(int ybegin, int yend,
			int ch, int yoff, int xoff)
{
  int y,code;

  if (Machine->color_depth==8) {
    for (y=ybegin; y<=yend; y++) {
      code=vic_dma_read(bitmapaddr+ch*8+y);
      ted7360_bitmap->line[y+yoff][xoff]=c16_bitmap[code>>7];
      ted7360_bitmap->line[y+yoff][1+xoff]=c16_bitmap[(code>>6)&1];
      ted7360_bitmap->line[y+yoff][2+xoff]=c16_bitmap[(code>>5)&1];
      ted7360_bitmap->line[y+yoff][3+xoff]=c16_bitmap[(code>>4)&1];
      ted7360_bitmap->line[y+yoff][4+xoff]=c16_bitmap[(code>>3)&1];
      ted7360_bitmap->line[y+yoff][5+xoff]=c16_bitmap[(code>>2)&1];
      ted7360_bitmap->line[y+yoff][6+xoff]=c16_bitmap[(code>>1)&1];
      ted7360_bitmap->line[y+yoff][7+xoff]=c16_bitmap[code&1];
    }
  } else {
    for (y=ybegin; y<=yend; y++) {
      code=vic_dma_read(bitmapaddr+ch*8+y);
      *((short*)ted7360_bitmap->line[y+yoff]+xoff)=c16_bitmap[code>>7];
      *((short*)ted7360_bitmap->line[y+yoff]+1+xoff)=c16_bitmap[(code>>6)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+2+xoff)=c16_bitmap[(code>>5)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+3+xoff)=c16_bitmap[(code>>4)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+4+xoff)=c16_bitmap[(code>>3)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+5+xoff)=c16_bitmap[(code>>2)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+6+xoff)=c16_bitmap[(code>>1)&1];
      *((short*)ted7360_bitmap->line[y+yoff]+7+xoff)=c16_bitmap[code&1];
    }
  }
}

static void ted7360_draw_bitmap_multi(int ybegin, int yend,
			      int ch, int yoff, int xoff)
{
  int y,code;

  if (Machine->color_depth==8) {
    for (y=ybegin; y<=yend; y++) {
      code=vic_dma_read(bitmapaddr+ch*8+y);
      ted7360_bitmap->line[y+yoff][xoff]=
	ted7360_bitmap->line[y+yoff][xoff+1]=bitmapmulti[code>>6];
      ted7360_bitmap->line[y+yoff][xoff+2]=
	ted7360_bitmap->line[y+yoff][xoff+3]=bitmapmulti[(code>>4)&3];
      ted7360_bitmap->line[y+yoff][xoff+4]=
	ted7360_bitmap->line[y+yoff][xoff+5]=bitmapmulti[(code>>2)&3];
      ted7360_bitmap->line[y+yoff][xoff+6]=
	ted7360_bitmap->line[y+yoff][xoff+7]=bitmapmulti[code&3];
    }
  } else {
    for (y=ybegin; y<=yend; y++) {
      code=vic_dma_read(bitmapaddr+ch*8+y);
      *((short*)ted7360_bitmap->line[y+yoff]+xoff)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+1)=bitmapmulti[code>>6];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+2)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+3)=bitmapmulti[(code>>4)&3];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+4)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+5)=bitmapmulti[(code>>2)&3];
      *((short*)ted7360_bitmap->line[y+yoff]+xoff+6)=
	*((short*)ted7360_bitmap->line[y+yoff]+xoff+7)=bitmapmulti[code&3];
    }
  }
}

static void ted7360_draw_cursor(int ybegin, int yend, int yoff, int xoff,
				int color)
{
  int y;
  if (Machine->color_depth==8) {
    for (y=ybegin; y<=yend; y++) {
      memset(ted7360_bitmap->line[yoff+y]+xoff,color,8);
    }
  } else {
    for (y=ybegin; y<=yend; y++) {
      memset16((short*)ted7360_bitmap->line[yoff+y]+xoff,color,8);
    }
  }
}

static void ted7360_drawlines(int first, int last)
{
  int line, vline, end;
  int attr, ch, c1, c2, ecm;
  int offs, yoff, xoff, ybegin, yend, xbegin, xend;
  int i;

  lastline=last;

  if (osd_skip_this_frame()) return;

  /* top part of display not rastered */
  first-=TED7360_YPOS;
  last-=TED7360_YPOS;
  if ((first>=last)||(last<=0)) return;
  if (first<0) first=0;

  if (!SCREENON) {
    if (Machine->color_depth==8) {
      for (line=first;(line<last)&&(line<ted7360_bitmap->height);line++)
	memset(ted7360_bitmap->line[line],Machine->pens[0],ted7360_bitmap->width);
    } else {
      for (line=first;(line<last)&&(line<ted7360_bitmap->height);line++)
	memset16(ted7360_bitmap->line[line],Machine->pens[0],ted7360_bitmap->width);
    }
    return;
  }

  if (COLUMNS40) xbegin=XPOS,xend=xbegin+320;
  else xbegin=XPOS+7,xend=xbegin+304;

  if (last<y_begin) end=last;
  else end=y_begin+YPOS;
  if (Machine->color_depth==8) {
    for (line=first;line<end;line++)
      memset(ted7360_bitmap->line[line],Machine->pens[FRAMECOLOR],
	     ted7360_bitmap->width);
  } else {
    for (line=first;line<end;line++)
      memset16(ted7360_bitmap->line[line],Machine->pens[FRAMECOLOR],
	       ted7360_bitmap->width);
  }
  if (LINES25) {
    vline=line-y_begin-YPOS;
  } else {
    vline=line-y_begin-YPOS+8-VERTICALPOS;
  }
  if (last<y_end+YPOS) end=last;
  else end=y_end+YPOS;
  for (;line<end;vline=(vline+8)&~7,line=line+1+yend-ybegin) {
    offs=(vline>>3)*40;
    ybegin=vline&7;
    yoff=line-ybegin;
    yend=(yoff+7<end)?7:(end-yoff-1);
    /* rendering 39 characters */
    /* left and right borders are overwritten later */

    for (xoff=x_begin+XPOS;xoff<x_end+XPOS; xoff+=8,offs++) {
      if (HIRESON) {
	ch=vic_dma_read((videoaddr|0x400)+offs);
	attr=vic_dma_read(((videoaddr)+offs));
	c1=((ch>>4)&0xf)|(attr<<4);
	c2=(ch&0xf)|(attr&0x70);
	bitmapmulti[1]=c16_bitmap[1]=Machine->pens[c1&0x7f];
	bitmapmulti[2]=c16_bitmap[0]=Machine->pens[c2&0x7f];
	if (MULTICOLORON) {
	  ted7360_draw_bitmap_multi(ybegin,yend,offs, yoff,xoff);
	} else {
	  ted7360_draw_bitmap(ybegin, yend,offs, yoff, xoff);
	}
      } else {
	ch=vic_dma_read((videoaddr|0x400)+offs);
	attr=vic_dma_read(((videoaddr)+offs));
	/* draw the character */
	if (cursor1 && (offs==CURSOR1POS)) {
#ifndef GFX
	  ted7360_draw_cursor(ybegin, yend, yoff,xoff,
			      Machine->pens[attr&0x7f]);
#else
	  drawgfx(ted7360_bitmap,cursorelement,0,Machine->pens[attr&0x7f],0,0,
		  xoff, yoff, 0, TRANSPARENCY_NONE,0);
#endif
	} else if (ECMON) {
	  ecm=ch>>6;
	  ecmcolor[0]=colors[ecm];
	  ecmcolor[1]=Machine->pens[attr&0x7f];
	  ted7360_draw_character(ybegin, yend,ch&~0xC0, yoff,xoff,ecmcolor);
	} else if (REVERSEON&&(ch&0x80)) {
	  if (MULTICOLORON&&(attr&8)) {
	    multi[3]=Machine->pens[attr&0x77];
	    ted7360_draw_character_multi(ybegin, yend,ch&~0x80, yoff,xoff);
	  } else { /* multicolor and hardware reverse? */
	    monoinversed[0]=Machine->pens[attr&0x7f];
	    ted7360_draw_character(ybegin, yend,ch&~0x80, yoff,xoff,
				   monoinversed);
	  }
	} else {
	  if (MULTICOLORON&&(attr&8)) {
	    multi[3]=Machine->pens[attr&0x77];
	    ted7360_draw_character_multi(ybegin, yend,ch, yoff,xoff);
	  } else {
	    mono[1]=Machine->pens[attr&0x7f];
#if 0
	    if (cursor1 && (attr&0x80) )
	      ted7360_draw_cursor(ybegin, yend, yoff,xoff,mono[1]);
	    else
#endif
	      ted7360_draw_character(ybegin, yend,ch, yoff,xoff,mono);
	  }
	}
      }
    }
    if (Machine->color_depth==8) {
      for (i=ybegin;i<=yend;i++) {
	memset(ted7360_bitmap->line[yoff+i],Machine->pens[FRAMECOLOR],xbegin);
	memset(ted7360_bitmap->line[yoff+i]+xend,Machine->pens[FRAMECOLOR],
	       ted7360_bitmap->width-xend);
      }
    } else {
      for (i=ybegin;i<=yend;i++) {
	memset16(ted7360_bitmap->line[yoff+i],Machine->pens[FRAMECOLOR],
			 xbegin);
	memset16((short*)ted7360_bitmap->line[yoff+i]+xend,
			 Machine->pens[FRAMECOLOR], ted7360_bitmap->width-xend);
      }
    }
  }
  if (last<ted7360_bitmap->height) end=last;
  else end=ted7360_bitmap->height;
  if (Machine->color_depth==8) {
    for (;line<end;line++)
      memset(ted7360_bitmap->line[line],Machine->pens[FRAMECOLOR],
	     ted7360_bitmap->width);
  } else {
    for (;line<end;line++)
      memset16(ted7360_bitmap->line[line],Machine->pens[FRAMECOLOR],
	       ted7360_bitmap->width);
  }
}

static void ted7360_draw_text(struct osd_bitmap *bitmap, char *text, int *y)
{
  int x, x0, y1t,
    width=(Machine->gamedrv->drv->visible_area.max_x-
	   Machine->gamedrv->drv->visible_area.min_x)/Machine->uifont->width;
  if (text[0]!=0) {
    x=strlen(text);
    *y-=Machine->uifont->height*((x+width-1)/width);
    y1t= *y+Machine->uifont->height;
    x=0;
    while (text[x]) {
      for( x0=Machine->gamedrv->drv->visible_area.min_x;
	   text[x]&&(x0<Machine->gamedrv->drv->visible_area.max_x-
		     Machine->uifont->width);
	   x++, x0+=Machine->uifont->width ) {
	drawgfx(bitmap,Machine->uifont,text[x],0,0,0, x0,y1t,0,
		TRANSPARENCY_NONE,0);
      }
      y1t += Machine->uifont->height;
    }
  }
}

int ted7360_raster_interrupt(void)
{
  int y;
  char text[70];

  rasterline++;
  rastertime=timer_get_time();
  if (rasterline>=lines) {
    rasterline=0;
    ted7360_drawlines(lastline, TED7360_LINES);
    lastline=0;

    y = Machine->gamedrv->drv->visible_area.max_y+1-Machine->uifont->height;

    vc20_tape_status(text, sizeof(text));
    ted7360_draw_text(ted7360_bitmap, text, &y);
#ifdef VC1541
    vc1541_drive_status(text,sizeof(text));
#else
    cbm_drive_0_status(text,sizeof(text));
#endif
    ted7360_draw_text(ted7360_bitmap, text, &y);

    cbm_drive_1_status(text,sizeof(text));
    ted7360_draw_text(ted7360_bitmap, text, &y);
  }
  if (rasterline==C16_2_RASTERLINE(RASTERLINE)) {
    ted7360_drawlines(lastline,rasterline);
    ted7360_set_interrupt(2);
  }
  return ignore_interrupt();
}

void ted7360_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
}
