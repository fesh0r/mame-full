#include "includes/lynx.h"

typedef struct {
	UINT8 data[0x100];
	UINT8 color[16];
	UINT8 high;
	int low;
} SUZY;
static SUZY suzy={ { 0 } };

#define PLOT_PIXEL(yptr, x, color) \
 if (!((x)&1)) { \
	 (yptr)[(x)/2]=((yptr)[(x)/2]&0xf)|((color)<<4); \
 } else { \
	 (yptr)[(x)/2]=((yptr)[(x)/2]&0xf0)|(color); \
 }

INLINE void lynx_blit_line(UINT8 *mem, UINT16 screen, UINT16 bitmap, int x, int w, int xdir)
{
	int j, xi, wi, b, i;

	i=mem[bitmap];
	for (xi=x, j=1, wi=0; (j<i)&&(xi<160); j++) {
		b=mem[bitmap+j]>>4;
		for (;(wi<w)&&(xi<160);wi+=0x100, xi++) {
			if (!(xi&1)) {
				mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b]<<4);
			} else {
				mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b];
			}
		}
		wi-=w;
		
		b=mem[bitmap+j]&0xf;
		for (;(wi<w)&&(xi<160);wi+=0x100, xi+=xdir) {
			if (!(xi&1)) {
				mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b]<<4);
			} else {
				mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b];
			}
		}
		wi-=w;
	}
}

/*
4 color rle:
 0, 4 bit repeat count-1, 2 bit color
 1, 4 bit count of values-1, 2 bit color, ....

16 color rle:
 0, 4 bit repeat count-1, 4 bit color
 1, 4 bit count of values-1, 4 bit color, ....

c0 10 00:
--------
0011100
0122210
1233321
1233321
1233321
0122210
0011100
04 09 c4 84
04 09 c4 84
04 09 44 40
04 08 84 00
01 
04 09 c4 84 
04 09 44 40
04 08 84 00
01 04 91

*/
static void lynx_blit_rle42_line(UINT8 *mem, UINT16 screen, UINT16 bitmap, int x, int w)
{
	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p=0, j=0, b=0, xi=x, wi=0; ; ) { // through the rle entries
		if (p<7) { // under 7 bits no complete entry
			j++;
			if (j>=mem[bitmap]) return;
			p+=8;
			b=(b<<8)|mem[bitmap+j];
		}
		t=(b>>(p-1))&1;p--;
		count=((b>>(p-4))&0xf)+1;p-=4;
		if (t) { // count of different pixels
			for (;count; count--) {
				if (p<2) {
					j++;
					if (j>=mem[bitmap]) return;
					p+=8;
					b=(b<<8)|mem[bitmap+j];
				}
				color=suzy.color[(b>>(p-2))&3];p-=2;
				for (;(wi<w);wi+=0x100, xi++) {
					if (xi>=160) return;
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(color<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|color;
					}
				}
				wi-=w;
			}
		} else { // count of same pixels
			if (p<2) {
				j++;
				if (j>=mem[bitmap]) return;
				p+=8;
				b=(b<<8)|mem[bitmap+j];
			}
			color=suzy.color[(b>>(p-2))&3];p-=2;
			for (;count; count--) {
				for (;(wi<w);wi+=0x100, xi++) {
					if (xi>=160) return;
					PLOT_PIXEL(mem+screen, xi, color);
				}
				wi-=w;
			}
		}
	}
}

static void lynx_blit_rle44_line(UINT8 *mem, UINT16 screen, UINT16 bitmap, int x, int w, int xdir)
{
	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p=0, j=0, b=0, xi=x, wi=0; ; ) { // through the rle entries
		if (p<9) { // under 7 bits no complete entry
			j++;
			if (j>=mem[bitmap]) return;
			p+=8;
			b=(b<<8)|mem[bitmap+j];
		}
		t=(b>>(p-1))&1;p--;
		count=((b>>(p-4))&0xf)+1;p-=4;
		if (t) { // count of different pixels
			for (;count; count--) {
				if (p<4) {
					j++;
					if (j>=mem[bitmap]) return;
					p+=8;
					b=(b<<8)|mem[bitmap+j];
				}
				color=suzy.color[(b>>(p-4))&0xf];p-=4;
				for (;(wi<w);wi+=0x100, xi+=xdir) {
					if (xi>=160) return;
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(color<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|color;
					}
				}
				wi-=w;
			}
		} else { // count of same pixels
			if (p<4) {
				j++;
				if (j>=mem[bitmap]) return;
				p+=8;
				b=(b<<8)|mem[bitmap+j];
			}
			color=suzy.color[(b>>(p-4))&0xf];p-=4;
			for (;count; count--) {
				for (;(wi<w);wi+=0x100, xi+=xdir) {
					if (xi>=160) return;
					PLOT_PIXEL(mem+screen, xi, color);
				}
				wi-=w;
			}
		}
	}
}
	
static void lynx_blit_2color(UINT16 cmd)
{
	UINT16 screen, data, bitmap, x, y, w, h;
	UINT8 *mem=memory_region(REGION_CPU1);
	UINT8 b;
	int xi, i, j;

	screen=suzy.data[0x8]|(suzy.data[0x9]<<8);
	data=mem[cmd+5]|(mem[cmd+6]<<8);
	x=mem[cmd+7]|(mem[cmd+8]<<8);
	y=mem[cmd+9]|(mem[cmd+10]<<8);
	w=(mem[cmd+11]|(mem[cmd+12]<<8));
	h=(mem[cmd+13]|(mem[cmd+14]<<8));
	w>>=8;
	h>>=8;
	suzy.color[0]=mem[cmd+0xf]&0xf;
	suzy.color[1]=mem[cmd+0xf]>>4;
	for ( screen+=y*80, bitmap=data,i=mem[bitmap]; i==mem[bitmap]; screen+=80, bitmap+=i ) {
		for (xi=x, j=1; j<i; j++, xi+=8) {
			b=mem[bitmap+j];
			mem[screen+xi/2]=(suzy.color[b&0x80?1:0]<<4)|suzy.color[b&0x40?1:0];
			mem[screen+xi/2+1]=(suzy.color[b&0x20?1:0]<<4)|suzy.color[b&0x10?1:0];
			mem[screen+xi/2+2]=(suzy.color[b&8?1:0]<<4)|suzy.color[b&4?1:0];
			mem[screen+xi/2+3]=(suzy.color[b&2?1:0]<<4)|suzy.color[b&1?1:0];
		}
	}	
}

static void lynx_blit_4color(UINT16 cmd)
{
	UINT16 screen, data, bitmap, x, y, w, h;
	UINT8 *mem=memory_region(REGION_CPU1);
	int xi, i, j, hi, wi, b;

	screen=suzy.data[0x8]|(suzy.data[0x9]<<8);
	data=mem[cmd+5]|(mem[cmd+6]<<8);
	x=mem[cmd+7]|(mem[cmd+8]<<8);
	y=mem[cmd+9]|(mem[cmd+10]<<8);
	w=(mem[cmd+11]|(mem[cmd+12]<<8));
	h=(mem[cmd+13]|(mem[cmd+14]<<8));

	suzy.color[0]=mem[cmd+0xf]>>4;
	suzy.color[1]=mem[cmd+0xf]&0xf;
	suzy.color[2]=mem[cmd+0xf+1]>>4;
	suzy.color[3]=mem[cmd+0xf+1]&0xf;

	for ( screen+=y*80, bitmap=data, hi=0; (i=mem[bitmap])&&(y<102); bitmap+=i ) {
		for (;(hi<h)&&(y<102); hi+=0x100, screen+=80, y++) {
			for (xi=x, j=1, wi=0; (j<i)&&(xi<160); j++) {
				b=mem[bitmap+j];
				for (;(wi<w)&&(xi<160);wi+=0x100, xi++) {
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b>>6]<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b>>6];
					}
				}
				wi-=w;

				b&=~0xc0;
				for (;(wi<w)&&(xi<160);wi+=0x100, xi++) {
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b>>4]<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b>>4];
					}
				}
				wi-=w;

				b&=~0x30;
				for (;(wi<w)&&(xi<160);wi+=0x100, xi++) {
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b>>2]<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b>>2];
					}
				}
				wi-=w;

				b&=~0xc;
				for (;(wi<w)&&(xi<160);wi+=0x100, xi++) {
					if (!(xi&1)) {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf)|(suzy.color[b]<<4);
					} else {
						mem[screen+xi/2]=(mem[screen+xi/2]&0xf0)|suzy.color[b];
					}
				}
				wi-=w;

			}
		}
		hi-=h;
	}	
}

static void lynx_blit_4color_rle(UINT16 cmd)
{
	UINT16 screen, data, bitmap, w, h;
	UINT8 *mem=memory_region(REGION_CPU1);
	int y, x, i, hi;

	screen=suzy.data[0x8]|(suzy.data[0x9]<<8);
	data=mem[cmd+5]|(mem[cmd+6]<<8);
	x=mem[cmd+7]|(mem[cmd+8]<<8);
	y=mem[cmd+9]|(mem[cmd+10]<<8);
	w=(mem[cmd+11]|(mem[cmd+12]<<8));
	h=(mem[cmd+13]|(mem[cmd+14]<<8));

	suzy.color[0]=mem[cmd+0xf]>>4;
	suzy.color[1]=mem[cmd+0xf]&0xf;
	suzy.color[2]=mem[cmd+0xf+1]>>4;
	suzy.color[3]=mem[cmd+0xf+1]&0xf;

	for ( screen+=y*80, bitmap=data, hi=0; (i=mem[bitmap])&&(y<102); bitmap+=i ) {
		for (;(hi<h)&&(y<102); hi+=0x100, screen+=80, y++) {
			lynx_blit_rle42_line(mem, screen, bitmap, x, w);
		}
		hi-=h;
	}	
}

static void lynx_blit_16color_rle(UINT16 cmd, int xdir, int ydir)
{
	UINT16 screen, data, bitmap, x, w, h;
	UINT8 *mem=memory_region(REGION_CPU1);
	int y, i, hi;
	int flip=0;

	screen=suzy.data[0x8]|(suzy.data[0x9]<<8);
	data=mem[cmd+5]|(mem[cmd+6]<<8);
	x=(mem[cmd+7]|(mem[cmd+8]<<8))-(suzy.data[4]|(suzy.data[5]<<8));
	y=(mem[cmd+9]|(mem[cmd+10]<<8))-(suzy.data[6]|(suzy.data[7]<<8));
	w=(mem[cmd+11]|(mem[cmd+12]<<8));
	h=(mem[cmd+13]|(mem[cmd+14]<<8));

	if (mem[cmd+1]!=0x98) {
		for (i=0; i<8; i++) {
			suzy.color[i*2]=mem[cmd+0xf+i]>>4;
			suzy.color[i*2+1]=mem[cmd+0xf+i]&0xf;
		}
	}

	for ( screen+=y*80, bitmap=data, hi=0; (i=mem[bitmap])&&(y<102); bitmap+=i ) {
		if (i==1) {
			hi=0;
			switch (flip) {
			case 0:
				ydir*=-1;
				y=(mem[cmd+9]|(mem[cmd+10]<<8))+ydir;
				screen=suzy.data[0x8]|(suzy.data[0x9]<<8)+y*80;
				break;
			case 1:
				xdir*=-1;
				y=(mem[cmd+9]|(mem[cmd+10]<<8))+ydir;
				screen=suzy.data[0x8]|(suzy.data[0x9]<<8)+y*80;
				break;
			case 2:
				ydir*=-1;
				y=(mem[cmd+9]|(mem[cmd+10]<<8));
				screen=suzy.data[0x8]|(suzy.data[0x9]<<8)+y*80;
				break;
			case 3:
				xdir*=-1;
				y=(mem[cmd+9]|(mem[cmd+10]<<8));
				screen=suzy.data[0x8]|(suzy.data[0x9]<<8)+y*80;
				break;
			}
			flip++;
			continue;
		}
		for (;(hi<h)&&(y<102); hi+=0x100, screen+=80*ydir, y+=ydir) {
			lynx_blit_rle44_line(mem, screen, bitmap, x, w, xdir);
		}
		hi-=h;
	}	
}

static void lynx_blit_16color(UINT16 cmd, int xdir, int ydir)
{
	UINT16 screen, data, bitmap, x, w, h;
	UINT8 *mem=memory_region(REGION_CPU1);
	int y, i, hi;

	screen=suzy.data[0x8]|(suzy.data[0x9]<<8);
	data=mem[cmd+5]|(mem[cmd+6]<<8);
	x=(mem[cmd+7]|(mem[cmd+8]<<8))-(suzy.data[4]|(suzy.data[5]<<8));
	y=(mem[cmd+9]|(mem[cmd+10]<<8))-(suzy.data[6]|(suzy.data[7]<<8));
	w=(mem[cmd+11]|(mem[cmd+12]<<8));
	h=(mem[cmd+13]|(mem[cmd+14]<<8));

	if (mem[cmd+1]!=0x98) {
		for (i=0; i<8; i++) {
			suzy.color[i*2]=mem[cmd+0xf+i]>>4;
			suzy.color[i*2+1]=mem[cmd+0xf+i]&0xf;
		}
	}

	for ( screen+=y*80, bitmap=data, hi=0; (i=mem[bitmap])&&(y<102); bitmap+=i ) {
		if (i==1) {
			hi=0;
			ydir*=-1;
			y=(mem[cmd+9]|(mem[cmd+10]<<8))+ydir;
			screen=suzy.data[0x8]|(suzy.data[0x9]<<8)+y*80;
			continue;
		}
		for (;(hi<h)&&(y<102); hi+=0x100, screen+=80*ydir, y+=ydir) {
			lynx_blit_line(mem, screen, bitmap, x, w,xdir);
		}
		hi-=h;
	}	
}

/*
  control 0
   bit 7,6: 00 2 color
            01 4 color
            11 8 colors?
            11 16 color
   bit 5,4: 00 right down
            01 left down??
            10 right up??
            11 left up??

  control 1
   bit 7: 0 bitmap rle encoded
          1 not encoded
  coll
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
  
  line c1 b0 20 0000 datapointer x y 0100 0100 0000 0000 color (8 color bytes)
  or
  line c0 b0 20 0000 datapointer x y 0100 0100 0000 0000 color
  datapointer 2 11 0
  
  text ?(04) 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

*/
static void lynx_blitter(void)
{
	UINT8 *mem=memory_region(REGION_CPU1);
	UINT16 cmd, data;

	logerror("blitter start\n");
	
	for (cmd=suzy.data[0x10]|(suzy.data[0x11]<<8); cmd; cmd=mem[cmd+3]|(mem[cmd+4]<<8) ) {

		data=mem[cmd+5]|(mem[cmd+6]<<8);

		switch (mem[cmd]) {
		case 0x4: // text (2 color bitmap?)
			lynx_blit_2color(cmd);
			break;
		case 0x40: // compressed 16 color in sprdemo2 ?
			lynx_blit_4color_rle(cmd);
			break;
		case 0xc0: case 0xc1:
			if (mem[cmd+1]&0x80)
				lynx_blit_16color(cmd, 1, 1);
			else
				lynx_blit_16color_rle(cmd, 1, 1);
			break;
		case 0xd0: 
			if (mem[cmd+1]&0x80)
				lynx_blit_16color(cmd, 1, -1);
			else
				lynx_blit_16color_rle(cmd, 1, -1);
			break;
		case 0xe0: 
			if (mem[cmd+1]&0x80)
				lynx_blit_16color(cmd, -1, 1);
			else
				lynx_blit_16color_rle(cmd, -1, 1);
			break;
		case 0xf0:
			if (mem[cmd+1]&0x80)
				lynx_blit_16color(cmd, -1, -1);
			else
				lynx_blit_16color_rle(cmd, -1, -1);
			break;
		}
#if 1
		logerror("%04x %.2x %.2x %.2x x:%.4x y:%.4x w:%.4x h:%.4x c:%.2x %.4x\n",
				 cmd,
				 mem[cmd],mem[cmd+1],mem[cmd+2],
				 mem[cmd+7]|(mem[cmd+8]<<8),mem[cmd+9]|(mem[cmd+10]<<8),
				 mem[cmd+11]|(mem[cmd+12]<<8),mem[cmd+13]|(mem[cmd+14]<<8),
				 mem[cmd+15],
				 data);
#endif
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
	UINT16 res=left/right;
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
		data=suzy.data[offset]&0x7f; // math finished
		break;
	case 0xb0: data=readinputport(0);break;
	case 0xb2:
		data=*(memory_region(REGION_USER1)+(suzy.high<<11)+suzy.low);
		logerror("mikey high %.2x low %.4x\n",suzy.high,suzy.low);
		suzy.low=(suzy.low+1)&0x7ff;
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

READ_HANDLER(mikey_read)
{
	UINT8 data=0;
	switch (offset) {
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
//		logerror("mikey high %.2x low %.4x\n",suzy.high,suzy.low);
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
