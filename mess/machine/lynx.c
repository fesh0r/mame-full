#include <assert.h>
#include "includes/lynx.h"
#include "cpu/m6502/m6502.h"

UINT16 lynx_granularity=1;

typedef struct {
	UINT8 data[0x100];
	UINT8 high;
	int low;
} SUZY;

static SUZY suzy={ { 0 } };

static struct {
	UINT8 *mem;
	// global
	UINT16 screen;
	UINT16 collision;
	UINT16 xoff, yoff;
	// in command
	UINT16 cmd;
	UINT8 color[16]; // or stored
	UINT16 width, height;
	int stretch, tilt;
	int x,y;
	void (*line_function)(UINT8 *dest, const int xdir);
	UINT16 bitmap;
} blitter;

#define GET_WORD(mem, index) ((mem)[(index)]|((mem)[(index)+1]<<8))

#define PLOT_PIXEL(yptr, x, color) \
 if (!((x)&1)) { \
	 (yptr)[(x)/2]=(((yptr)[(x)/2])&0xf)|((color)<<4); \
 } else { \
	 (yptr)[(x)/2]=(((yptr)[(x)/2])&0xf0)|(color); \
 }

/*
2 color rle: ??
 0, 4 bit repeat count-1, 1 bit color
 1, 4 bit count of values-1, 1 bit color, ....
*/

/*
4 color rle:
 0, 4 bit repeat count-1, 2 bit color
 1, 4 bit count of values-1, 2 bit color, ....
*/

/*
8 color rle:
 0, 4 bit repeat count-1, 3 bit color
 1, 4 bit count of values-1, 3 bit color, ....
*/
	
/*
16 color rle:
 0, 4 bit repeat count-1, 4 bit color
 1, 4 bit count of values-1, 4 bit color, ....
*/

#define INCLUDE_LYNX_LINE_FUNCTION
static void lynx_blit_2color_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=1; 
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=2; 
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=3; 
	const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=4; 
	const int mask=0xf;
#include "includes/lynx.h"
}


static void lynx_blit_2color_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=1; 
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=2; 
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=3; const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=4; 
	const int mask=0xf;
#include "includes/lynx.h"
}
#undef INCLUDE_LYNX_LINE_FUNCTION


#define INCLUDE_LYNX_LINE_RLE_FUNCTION
static void lynx_blit_2color_rle_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=1; 
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_rle_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=2; 
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_rle_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=3; 
	const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_rle_line(UINT8 *dest, const int xdir)
{
	const int trans=0;
	const int bits=4; 
	const int mask=0xf;
#include "includes/lynx.h"
}

static void lynx_blit_2color_rle_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=1; 
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_rle_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=2; 
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_rle_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=3;
	const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_rle_line_trans(UINT8 *dest, const int xdir)
{
	const int trans=1;
	const int bits=4; 
	const int mask=0xf;
#include "includes/lynx.h"
}
#undef INCLUDE_LYNX_LINE_RLE_FUNCTION


static void lynx_blit_lines(void)
{
	int i, hi, y;
	int ydir, xdir;
	int flip=blitter.mem[blitter.cmd+1]&3;

	switch (blitter.mem[blitter.cmd]&0x30) {
	case 0x0000:
		xdir=ydir=1;break;
	case 0x0010:
		xdir=1;ydir=-1;break;
	case 0x0020:
		xdir=-1;ydir=1;break;
	case 0x0030:
		xdir=ydir=-1;break;
	}
	switch (blitter.mem[blitter.cmd+1]&3) {
	case 0: 
		flip =0;
		break;
	case 1:
		xdir*=-1;
		flip=2;
		break;
	case 2:
		ydir*=-1;
		flip=1;
		break;
	case 3:
		xdir*=-1;
		ydir*=-1;
		flip=3;
		break;
	}
	// examples sprdemo3, fat bobby for rotation

	for ( y=blitter.y, hi=0; (i=blitter.mem[blitter.bitmap]); blitter.bitmap+=i ) {
		if (i==1) {
			hi=0;
			switch (flip&3) {
			case 0:
				ydir*=-1;
				y=blitter.y+ydir;
				break;
			case 1:
				xdir*=-1;
				y=blitter.y+ydir;
				break;
			case 2:
				ydir*=-1;
				y=blitter.y;
				break;
			case 3:
				xdir*=-1;
				y=blitter.y;
				break;
			}
			flip++;
			continue;
		}
		for (;(hi<blitter.height); hi+=0x100, y+=ydir) {
			if ((y>=0)&&(y<102))
				blitter.line_function(blitter.mem+blitter.screen+y*80,xdir);
			blitter.width+=blitter.stretch;
			blitter.x+=blitter.tilt;
		}
		hi-=blitter.height;
	}
}

/*
  control 0
   bit 7,6: 00 2 color
            01 4 color
            11 8 colors?
            11 16 color
   bit 5,4: 00 right down
            01 right up
            10 left down
            11 left up

#define SHADOW         (0x07)
#define XORSHADOW      (0x06)
#define NONCOLLIDABLE  (0x05)
#define NORMAL         (0x04)
#define BOUNDARY       (0x03)
#define BOUNDARYSHADOW (0x02)
#define BKGRNDNOCOL    (0x01)
#define BKGRND         (0x00)

  control 1
   bit 7: 0 bitmap rle encoded
          1 not encoded
   bit 3: 0 color info with command
          1 no color info with command

#define LITERAL        (0x80)
#define RELHVST        (0x30)
#define RELHVS         (0x20)
#define RELHV          (0x10)
#define RELPALETTE     (0x00)
#define EXISTPALETTE   (0x08)
#define SKIPSPRITE     (0x04)
#define DUP            (0x02)
#define DDOWN          (0x00)
#define DLEFT          (0x01)
#define DRIGHT         (0x00)


  coll
#define DONTCOLLIDE    (0x20)

  word next
  word data
  word x
  word y
  word width
  word height

  pixel c0 90 20 0000 datapointer x y 0100 0100 color (8 colorbytes)
  4 bit direct?
  datapointer 2 10 0
  98 (0 colorbytes)

  box c0 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  c1 98 00 4 bit direct without color bytes (raycast)

  40 10 20 4 bit rle (sprdemo2)

  line c1 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color (8 color bytes)
  or
  line c0 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color
  datapointer 2 11 0

  text ?(04) 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  stretch: hsize adder
  tilt: hpos adder

*/
static void lynx_blitter(void)
{
	int i; int o;

	blitter.mem=memory_region(REGION_CPU1);
	blitter.collision=GET_WORD(suzy.data, 0xa);
	blitter.screen=GET_WORD(suzy.data, 8);
	blitter.xoff=GET_WORD(suzy.data,4);
	blitter.yoff=GET_WORD(suzy.data,6);

//	logerror("blitter start\n");

	for (blitter.cmd=GET_WORD(suzy.data, 0x10); blitter.cmd; 
		 blitter.cmd=GET_WORD(blitter.mem, blitter.cmd+3) ) {

		if (blitter.mem[blitter.cmd+1]&4) continue;

		blitter.bitmap=GET_WORD(blitter.mem,blitter.cmd+5);
		blitter.x=(INT16)GET_WORD(blitter.mem, blitter.cmd+7)-blitter.xoff;
		blitter.y=(INT16)GET_WORD(blitter.mem, blitter.cmd+9)-blitter.yoff;
		blitter.width=GET_WORD(blitter.mem, blitter.cmd+11);
		blitter.height=GET_WORD(blitter.mem, blitter.cmd+13);

		switch ( GET_WORD(blitter.mem,blitter.cmd)&0x80c7) {
		case 0x8000: 
			blitter.line_function=lynx_blit_2color_line;break;
		case 0x8001: case 0x8002: case 0x8003: 
		case 0x8004: case 0x8005: case 0x8006: case 0x8007: 
			blitter.line_function=lynx_blit_2color_line_trans;break;
		case 0x0000:
			blitter.line_function=lynx_blit_2color_rle_line;break;
		case 0x0001: case 0x0002: case 0x0003: 
		case 0x0004: case 0x0005: case 0x0006: case 0x0007: 
			blitter.line_function=lynx_blit_2color_rle_line_trans;break;
		case 0x8040: 
			blitter.line_function=lynx_blit_4color_line;break;
		case 0x8041: case 0x8042: case 0x8043: 
		case 0x8044: case 0x8045: case 0x8046: case 0x8047: 
			blitter.line_function=lynx_blit_4color_line_trans;break;
		case 0x0040:
			blitter.line_function=lynx_blit_4color_rle_line;break;
		case 0x0041: case 0x0042: case 0x0043: 
		case 0x0044: case 0x0045: case 0x0046: case 0x0047: 
			blitter.line_function=lynx_blit_4color_rle_line_trans;break;
		case 0x8080: 
			blitter.line_function=lynx_blit_8color_line;break;
		case 0x8081: case 0x8082: case 0x8083: 
		case 0x8084: case 0x8085: case 0x8086: case 0x8087: 
			blitter.line_function=lynx_blit_8color_line_trans;break;
		case 0x0080:
			blitter.line_function=lynx_blit_8color_rle_line;break;
		case 0x0081: case 0x0082: case 0x0083: 
		case 0x0084: case 0x0085: case 0x0086: case 0x0087: 
			blitter.line_function=lynx_blit_8color_rle_line_trans;break;
		case 0x80c0: 
			blitter.line_function=lynx_blit_16color_line;break;
		case 0x80c1: case 0x80c2: case 0x80c3: 
		case 0x80c4: case 0x80c5: case 0x80c6: case 0x80c7: 
			blitter.line_function=lynx_blit_16color_line_trans;break;
		case 0x00c0:
			blitter.line_function=lynx_blit_16color_rle_line;break;
		case 0x00c1: case 0x00c2: case 0x00c3: 
		case 0x00c4: case 0x00c5: case 0x00c6: case 0x00c7: 
			blitter.line_function=lynx_blit_16color_rle_line_trans;break;
		}

		o=0xf;
		blitter.stretch=0;
		blitter.tilt=0;
		if (blitter.mem[blitter.cmd+1]&0x20) {
			blitter.stretch=GET_WORD(blitter.mem, blitter.cmd+o);
			o+=2;
			if (blitter.mem[blitter.cmd+1]&0x10) {
				blitter.tilt=GET_WORD(blitter.mem, blitter.cmd+o);
				o+=2;
			}
		}
		if (!(blitter.mem[blitter.cmd+1]&8)) {
			switch (blitter.mem[blitter.cmd]&0xc0) {
			case 0:
				blitter.color[0]=blitter.mem[blitter.cmd+o]>>4;
				blitter.color[1]=blitter.mem[blitter.cmd+o]&0xf;
				break;
			case 0x40:
				for (i=0; i<2; i++) {
					blitter.color[i*2]=blitter.mem[blitter.cmd+o+i]>>4;
					blitter.color[i*2+1]=blitter.mem[blitter.cmd+o+i]&0xf;
				}
				break;
			case 0x80:
				for (i=0; i<4; i++) {
					blitter.color[i*2]=blitter.mem[blitter.cmd+o+i]>>4;
					blitter.color[i*2+1]=blitter.mem[blitter.cmd+o+i]&0xf;
				}
				break;
			case 0xc0:
				for (i=0; i<8; i++) {
					blitter.color[i*2]=blitter.mem[blitter.cmd+o+i]>>4;
					blitter.color[i*2+1]=blitter.mem[blitter.cmd+o+i]&0xf;
				}
				break;
			}
		}
#if 1
		if (blitter.mem[blitter.cmd+1]&0x20) {
			logerror("%04x %.2x %.2x %.2x x:%.4x y:%.4x w:%.4x h:%.4x s:%.4x t:%.4x %.4x\n",
					 blitter.cmd,
					 blitter.mem[blitter.cmd],blitter.mem[blitter.cmd+1],blitter.mem[blitter.cmd+2],
					 blitter.x,blitter.y,
					 blitter.width,blitter.height,
					 blitter.stretch,
					 blitter.tilt,
					 blitter.bitmap);
		} else {
			logerror("%04x %.2x %.2x %.2x x:%.4x y:%.4x w:%.4x h:%.4x %.4x\n",
					 blitter.cmd,
					 blitter.mem[blitter.cmd],blitter.mem[blitter.cmd+1],blitter.mem[blitter.cmd+2],
					 blitter.x,blitter.y,
					 blitter.width,blitter.height,
					 blitter.bitmap);
		}
#endif
		lynx_blit_lines();
	}
}

/*
  writes (0x52 0x53) (0x54 0x55) expects multiplication result at 0x60 0x61
  writes (0x56 0x57) (0x60 0x61 0x62 0x63) division expects result at (0x52 0x53)
 */

void lynx_divide(void)
{
	UINT32 left=suzy.data[0x60]|(suzy.data[0x61]<<8)|(suzy.data[0x62]<<16)|(suzy.data[0x63]<<24);
	UINT16 right=suzy.data[0x56]|(suzy.data[0x57]<<8);
	UINT16 res;
	if (left==0) {
		res=0;
	} else if (right==0) {
		res=0xffff; //estimated
	} else {
		res=left/right;
	}
//	logerror("coprocessor %8x / %4x = %4x\n", left, right, res);
	suzy.data[0x52]=res&0xff;
	suzy.data[0x53]=res>>8;
}

void lynx_multiply(void)
{
	UINT16 left, right, res;
	left=suzy.data[0x52]|(suzy.data[0x53]<<8);
	right=suzy.data[0x54]|(suzy.data[0x55]<<8);
	res=left*right;
//	logerror("coprocessor %4x * %4x = %4x\n", left, right, res);
	suzy.data[0x60]=res&0xff;
	suzy.data[0x61]=res>>8;
}

READ_HANDLER(suzy_read)
{
	UINT8 data=0;
	switch (offset) {
	case 0x88:
		data=1; // must not be 0 for correct power up
		break;
	case 0x92:
		data=suzy.data[offset];
		data&=~0x80; // math finished
		data&=~1; //blitter finished
		break;
	case 0xb0:
		if (suzy.data[0x92]&8) {
			data=0;
			if (readinputport(0)&0x80) data|=0x40;
			if (readinputport(0)&0x40) data|=0x80;
			if (readinputport(0)&0x20) data|=0x10;
			if (readinputport(0)&0x10) data|=0x20;
			if (readinputport(0)&8) data|=8;
			if (readinputport(0)&4) data|=4;
			if (readinputport(0)&2) data|=2;
			if (readinputport(0)&1) data|=1;
		} else {
			data=readinputport(0);
		}
		break;
	case 0xb1: data=readinputport(1);break;
	case 0xb2:
		data=*(memory_region(REGION_USER1)+(suzy.high*lynx_granularity)+suzy.low);
		suzy.low=(suzy.low+1)&(lynx_granularity-1);
		break;
	default:
		data=suzy.data[offset];
	}
//	logerror("suzy read %.2x %.2x\n",offset,data);
	return data;
}

WRITE_HANDLER(suzy_write)
{
	suzy.data[offset]=data;
//	logerror("suzy write %.2x %.2x\n",offset,data);
	switch(offset) {
	case 0x55: lynx_multiply();break;
	case 0x63: lynx_divide();break;
	case 0x91:
		if (data&1) {
			lynx_blitter();
		}
		break;
	}
}

/*
 0xfd0a r sync signal?
 0xfd81 r interrupt source bit 2 vertical refresh
 0xfd80 w interrupt quit
 0xfd87 w bit 1 !clr bit 0 blocknumber clk
 0xfd8b w bit 1 blocknumber hi B
 0xfd94 w 0
 0xfd95 w 4
 0xfda0-f rw farben 0..15
 0xfdb0-f rw bit0..3 farben 0..15
*/
MIKEY mikey={ { 0 } };

typedef struct {
	const int nr;
	int counter;
	void *timer;
	UINT8 data[4];
	double settime;
} LYNX_TIMER;
static LYNX_TIMER lynx_timer[8]= {
	{ 0 },
	{ 1 },
	{ 2 },
	{ 3 },
	{ 4 },
	{ 5 },
	{ 6 },
	{ 7 }
};

static void lynx_timer_count_down(LYNX_TIMER *This);
static void lynx_timer_signal_irq(LYNX_TIMER *This)
{
//	if ((This->data[1]&0x80)&&!(mikey.data[0x81]&(1<<This->nr))) {
	if ((This->data[1]&0x80)) { // irq flag handling later
		cpu_set_irq_line(0, M65SC02_INT_IRQ, PULSE_LINE);
		mikey.data[0x81]|=1<<This->nr; // vertical
	}
	switch (This->nr) {
	case 0:
		if ((lynx_timer[2].data[1]&7)==7) lynx_timer_count_down(lynx_timer+2);
		break;
	case 2:
		if ((lynx_timer[4].data[1]&7)==7) lynx_timer_count_down(lynx_timer+4);
		break;
	case 1:
		if ((lynx_timer[3].data[1]&7)==7) lynx_timer_count_down(lynx_timer+3);
		break;
	case 3:
		if ((lynx_timer[5].data[1]&7)==7) lynx_timer_count_down(lynx_timer+5);
		break;
	case 5:
		if ((lynx_timer[7].data[1]&7)==7) lynx_timer_count_down(lynx_timer+7);
		break;
	case 7:
		// audio 1
		break;
	}
}

static void lynx_timer_count_down(LYNX_TIMER *This)
{
	if (This->counter>0) {
		This->counter--;
		return;
	} else if (This->counter==0) {
		lynx_timer_signal_irq(This);
		if (This->data[1]&0x10) {
			This->counter=This->data[0];
		} else {
			This->counter--;
		}
		return;
	}
}

static void lynx_timer_shot(int nr)
{
	LYNX_TIMER *This=lynx_timer+nr;
	lynx_timer_signal_irq(This);
	if (!(This->data[1]&0x10)) This->timer=NULL;
}

static double times[]= { 1e-6, 2e-6, 4e-6, 8e-6, 16e-6, 32e-6, 64e-6 };

static UINT8 lynx_timer_read(LYNX_TIMER *This, int offset)
{
	UINT8 data;
	switch (offset) {
	case 2:
		data=This->counter;
		break;
	default:
		data=This->data[offset];
	}
//	logerror("timer %d read %x %.2x\n",This-lynx_timer,offset,data);
	return data;
}

static void lynx_timer_write(LYNX_TIMER *This, int offset, UINT8 data)
{
	int t;
	logerror("timer %d write %x %.2x\n",This-lynx_timer,offset,data);
	This->data[offset]=data;

	if (offset==0) This->counter=This->data[0]+1;
	if (This->timer) { timer_remove(This->timer); This->timer=NULL; }
//	if ((This->data[1]&0x80)&&(This->nr!=4)) { //timers are combined!
	if ((This->data[1]&0x8)&&(This->nr!=4)) {
		if ((This->data[1]&7)!=7) {
			t=This->data[0]?This->data[0]:0x100;
			if (This->data[1]&0x10) {
				This->timer=timer_pulse(t*times[This->data[1]&7],
										This->nr, lynx_timer_shot);
			} else {
				This->timer=timer_set(t*times[This->data[1]&7],
									  This->nr, lynx_timer_shot);
			}
		}
	}
}

READ_HANDLER(mikey_read)
{
	UINT8 data=0;
	switch (offset) {
	case 0: case 1: case 2: case 3:
	case 4: case 5: case 6: case 7:
	case 8: case 9: case 0xa: case 0xb:
	case 0xc: case 0xd: case 0xe: case 0xf:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		data=lynx_timer_read(lynx_timer+(offset/4), offset&3);
		break;
	case 0x8c:
		data=mikey.data[offset]&~0x40; // no serial data received
		break;
	default:
		data=mikey.data[offset];
	}
//	logerror("mikey read %.2x %.2x\n",offset,data);
	return data;
}

WRITE_HANDLER(mikey_write)
{
	mikey.data[offset]=data;
//	logerror("mikey write %.2x %.2x\n",offset,data);
	switch (offset) {
	case 0: case 1: case 2: case 3:
	case 4: case 5: case 6: case 7:
	case 8: case 9: case 0xa: case 0xb:
	case 0xc: case 0xd: case 0xe: case 0xf:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		lynx_timer_write(lynx_timer+(offset/4), offset&3, data);
		break;
	case 0x87:
		if (data&2) {
			if (data&1) {
				suzy.high<<=1;
				if (mikey.data[0x8b]&2) suzy.high|=1;
				suzy.low=0;
			}
		} else {
			suzy.high=0;
			suzy.low=0;
		}
		break;
	case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
	case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		mikey.data[offset]=data;
		palette_change_color(offset&0xf,
							 (mikey.data[0xa0+(offset&0xf)]&0xf)<<4,
							 (mikey.data[0xb0+(offset&0xf)]&0xf)<<4,
							 mikey.data[0xb0+(offset&0xf)]&0xf0 );
		break;
	}
}

WRITE_HANDLER( lynx_memory_config )
{
	memory_region(REGION_CPU1)[0xfff9]=data;
	if (data&1) {
		memory_set_bankhandler_r(1, 0, MRA_RAM);
		memory_set_bankhandler_w(1, 0, MWA_RAM);
	} else {
		memory_set_bankhandler_r(1, 0, suzy_read);
		memory_set_bankhandler_w(1, 0, suzy_write);
	}
	if (data&2) {
		memory_set_bankhandler_r(2, 0, MRA_RAM);
		memory_set_bankhandler_w(2, 0, MWA_RAM);
	} else {
		memory_set_bankhandler_r(2, 0, mikey_read);
		memory_set_bankhandler_w(2, 0, mikey_write);
	}
	if (data&4) {
		memory_set_bankhandler_r(3, 0, MRA_RAM);
	} else {
		cpu_setbank(3,memory_region(REGION_CPU1)+0x10000);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
	}
	if (data&8) {
		memory_set_bankhandler_r(4, 0, MRA_RAM);
	} else {
		memory_set_bankhandler_r(4, 0, MRA_BANK4);
		cpu_setbank(4,memory_region(REGION_CPU1)+0x101f8);
	}
}
