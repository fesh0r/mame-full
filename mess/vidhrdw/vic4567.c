// don't include this into the makefile
// it is included in vic5667.c yet

#define VIC3_BITPLANES_MASK (vic2.reg[0x32])
/* bit 0, 4 not used !?*/
/* I think hinibbles contains the banknumbers for interlaced modes */
/* if hinibble set then x&1==0 should be in bank1 (0x10000), x&1==1 in bank 0 */
#define VIC3_BITPLANE_ADDR_HELPER(x)  ( (vic2.reg[0x33+x]&0xf) <<12)
#define VIC3_BITPLANE_ADDR(x) ( x&1 ? VIC3_BITPLANE_ADDR_HELPER(x)+0x10000 \
								: VIC3_BITPLANE_ADDR_HELPER(x) )
#define VIC3_BITPLANE_IADDR_HELPER(x)  ( (vic2.reg[0x33+x]&0xf0) <<8)
#define VIC3_BITPLANE_IADDR(x) ( x&1 ? VIC3_BITPLANE_IADDR_HELPER(x)+0x10000 \
								: VIC3_BITPLANE_IADDR_HELPER(x) )

#define OPTIMIZE
#ifndef OPTIMIZE
INLINE UINT8 vic3_bitplane_to_packed(UINT8 *latch, int mask, int number)
{
	UINT8 color=0;
	if ( (mask&1)&&(latch[0]&(1<<number)) ) color|=1;
	if ( (mask&2)&&(latch[1]&(1<<number)) ) color|=2;
	if ( (mask&4)&&(latch[2]&(1<<number)) ) color|=4;
	if ( (mask&8)&&(latch[3]&(1<<number)) ) color|=8;
	if ( (mask&0x10)&&(latch[4]&(1<<number)) ) color|=0x10;
	if ( (mask&0x20)&&(latch[5]&(1<<number)) ) color|=0x20;
	if ( (mask&0x40)&&(latch[6]&(1<<number)) ) color|=0x40;
	if ( (mask&0x80)&&(latch[7]&(1<<number)) ) color|=0x80;
	return color;
}


INLINE void vic3_block_2_color(int offset, UINT8 colors[8])
{
	if (VIC3_BITPLANES_MASK&1) {
		colors[0]=c64_memory[VIC3_BITPLANE_ADDR(0)+offset];
	}
	if (VIC3_BITPLANES_MASK&2) {
		colors[1]=c64_memory[VIC3_BITPLANE_ADDR(1)+offset];
	}
	if (VIC3_BITPLANES_MASK&4) {
		colors[2]=c64_memory[VIC3_BITPLANE_ADDR(2)+offset];
	}
	if (VIC3_BITPLANES_MASK&8) {
		colors[3]=c64_memory[VIC3_BITPLANE_ADDR(3)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x10) {
		colors[4]=c64_memory[VIC3_BITPLANE_ADDR(4)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x20) {
		colors[5]=c64_memory[VIC3_BITPLANE_ADDR(5)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x40) {
		colors[6]=c64_memory[VIC3_BITPLANE_ADDR(6)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x80) {
		colors[7]=c64_memory[VIC3_BITPLANE_ADDR(7)+offset];
	}
}

INLINE void vic3_interlace_block_2_color(int offset, UINT8 colors[8])
{

	if (VIC3_BITPLANES_MASK&1) {
		colors[0]=c64_memory[VIC3_BITPLANE_IADDR(0)+offset];
	}
	if (VIC3_BITPLANES_MASK&2) {
		colors[1]=c64_memory[VIC3_BITPLANE_IADDR(1)+offset];
	}
	if (VIC3_BITPLANES_MASK&4) {
		colors[2]=c64_memory[VIC3_BITPLANE_IADDR(2)+offset];
	}
	if (VIC3_BITPLANES_MASK&8) {
		colors[3]=c64_memory[VIC3_BITPLANE_IADDR(3)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x10) {
		colors[4]=c64_memory[VIC3_BITPLANE_IADDR(4)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x20) {
		colors[5]=c64_memory[VIC3_BITPLANE_IADDR(5)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x40) {
		colors[6]=c64_memory[VIC3_BITPLANE_IADDR(6)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x80) {
		colors[7]=c64_memory[VIC3_BITPLANE_IADDR(7)+offset];
	}
}

INLINE void vic3_draw_block(int x, int y, UINT8 colors[8])
{
	vic2.bitmap->line[YPOS+y][XPOS+x]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 7)];
	vic2.bitmap->line[YPOS+y][XPOS+x+1]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 6)];
	vic2.bitmap->line[YPOS+y][XPOS+x+2]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 5)];
	vic2.bitmap->line[YPOS+y][XPOS+x+3]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 4)];
	vic2.bitmap->line[YPOS+y][XPOS+x+4]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 3)];
	vic2.bitmap->line[YPOS+y][XPOS+x+5]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 2)];
	vic2.bitmap->line[YPOS+y][XPOS+x+6]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 1)];
	vic2.bitmap->line[YPOS+y][XPOS+x+7]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 0)];
}

#else
// very recognizable SPEED improvement
#define VIC3_ADDR(a) VIC3_BITPLANE_IADDR(a)






void vic3_interlace_draw_block(int x, int y, int offset)
{
	int colors[8]={0};
	int i;

	switch (VIC3_BITPLANES_MASK) {
	case 5:
#define VIC3_MASK 5
#include "includes/vic4567.h"
		break;
	case 7:
#undef VIC3_MASK
#define VIC3_MASK 7
#include "includes/vic4567.h"
		break;
	case 0xf:
#undef VIC3_MASK
#define VIC3_MASK 0xf
#include "includes/vic4567.h"
		break;
	case 0x1f:
#undef VIC3_MASK
#define VIC3_MASK 0x1f
#include "includes/vic4567.h"
		break;
	case 0x7f:
#undef VIC3_MASK
#define VIC3_MASK 0x7f
#include "includes/vic4567.h"
		break;
	case 0xff:
#undef VIC3_MASK
#define VIC3_MASK 0xff
#include "includes/vic4567.h"
		break;
	default:
		if (VIC3_BITPLANES_MASK&1) {
			colors[0]=c64_memory[VIC3_BITPLANE_IADDR(0)+offset];
		}
		if (VIC3_BITPLANES_MASK&2) {
			colors[1]=c64_memory[VIC3_BITPLANE_IADDR(1)+offset]<<1;
		}
		if (VIC3_BITPLANES_MASK&4) {
			colors[2]=c64_memory[VIC3_BITPLANE_IADDR(2)+offset]<<2;
		}
		if (VIC3_BITPLANES_MASK&8) {
			colors[3]=c64_memory[VIC3_BITPLANE_IADDR(3)+offset]<<3;
		}
		if (VIC3_BITPLANES_MASK&0x10) {
			colors[4]=c64_memory[VIC3_BITPLANE_IADDR(4)+offset]<<4;
		}
		if (VIC3_BITPLANES_MASK&0x20) {
			colors[5]=c64_memory[VIC3_BITPLANE_IADDR(5)+offset]<<5;
		}
		if (VIC3_BITPLANES_MASK&0x40) {
			colors[6]=c64_memory[VIC3_BITPLANE_IADDR(6)+offset]<<6;
		}
		if (VIC3_BITPLANES_MASK&0x80) {
			colors[7]=c64_memory[VIC3_BITPLANE_IADDR(7)+offset]<<7;
		}
		for (i=7;i>=0;i--) {
			vic2.bitmap->line[YPOS+y][XPOS+x+i]=
				Machine->pens[(colors[0]&1)|(colors[1]&2)
							 |(colors[2]&4)|(colors[3]&8)
							 |(colors[4]&0x10)|(colors[5]&0x20)
							 |(colors[6]&0x40)|(colors[7]&0x80)];
			colors[0]>>=1;
			colors[1]>>=1;
			colors[2]>>=1;
			colors[3]>>=1;
			colors[4]>>=1;
			colors[5]>>=1;
			colors[6]>>=1;
			colors[7]>>=1;
		}
	}
}

#undef VIC3_ADDR
#undef VIC3_MASK
#define VIC3_ADDR(a) VIC3_BITPLANE_ADDR(a)
void vic3_draw_block(int x, int y, int offset)
{
	int colors[8]={0};
	int i;

	switch (VIC3_BITPLANES_MASK) {
	case 5:
#define VIC3_MASK 5
#include "includes/vic4567.h"
		break;
	case 7:
#undef VIC3_MASK
#define VIC3_MASK 7
#include "includes/vic4567.h"
		break;
	case 0xf:
#undef VIC3_MASK
#define VIC3_MASK 0xf
#include "includes/vic4567.h"
		break;
	case 0x1f:
#undef VIC3_MASK
#define VIC3_MASK 0x1f
#include "includes/vic4567.h"
		break;
	case 0x7f:
#undef VIC3_MASK
#define VIC3_MASK 0x7f
#include "includes/vic4567.h"
		break;
	case 0xff:
#undef VIC3_MASK
#define VIC3_MASK 0xff
#include "includes/vic4567.h"
		break;
	default:
		if (VIC3_BITPLANES_MASK&1) {
			colors[0]=c64_memory[VIC3_BITPLANE_ADDR(0)+offset];
		}
		if (VIC3_BITPLANES_MASK&2) {
			colors[1]=c64_memory[VIC3_BITPLANE_ADDR(1)+offset]<<1;
		}
		if (VIC3_BITPLANES_MASK&4) {
			colors[2]=c64_memory[VIC3_BITPLANE_ADDR(2)+offset]<<2;
		}
		if (VIC3_BITPLANES_MASK&8) {
			colors[3]=c64_memory[VIC3_BITPLANE_ADDR(3)+offset]<<3;
		}
		if (VIC3_BITPLANES_MASK&0x10) {
			colors[4]=c64_memory[VIC3_BITPLANE_ADDR(4)+offset]<<4;
		}
		if (VIC3_BITPLANES_MASK&0x20) {
			colors[5]=c64_memory[VIC3_BITPLANE_ADDR(5)+offset]<<5;
		}
		if (VIC3_BITPLANES_MASK&0x40) {
			colors[6]=c64_memory[VIC3_BITPLANE_ADDR(6)+offset]<<6;
		}
		if (VIC3_BITPLANES_MASK&0x80) {
			colors[7]=c64_memory[VIC3_BITPLANE_ADDR(7)+offset]<<7;
		}
		for (i=7;i>=0;i--) {
			vic2.bitmap->line[YPOS+y][XPOS+x+i]=
				Machine->pens[(colors[0]&1)|(colors[1]&2)
							 |(colors[2]&4)|(colors[3]&8)
							 |(colors[4]&0x10)|(colors[5]&0x20)
							 |(colors[6]&0x40)|(colors[7]&0x80)];
			colors[0]>>=1;
			colors[1]>>=1;
			colors[2]>>=1;
			colors[3]>>=1;
			colors[4]>>=1;
			colors[5]>>=1;
			colors[6]>>=1;
			colors[7]>>=1;
		}
	}
}
#endif

void vic3_draw_bitplanes(void)
{
	static int interlace=0;
	int x, y, y1s, offset;
#ifndef OPTIMIZE
	UINT8 colors[8];
#endif
	struct rectangle vis;

	if (VIC3_LINES==400) { /* interlaced! */
		for ( y1s=0, offset=0; y1s<400; y1s+=16) {
			for (x=0; x<VIC3_BITPLANES_WIDTH; x+=8) {
				for ( y=y1s; y<y1s+16; y+=2, offset++) {
#ifndef OPTIMIZE
					if (interlace) {
						vic3_block_2_color(offset,colors);
						vic3_draw_block(x,y,colors);
					} else {
						vic3_interlace_block_2_color(offset,colors);
						vic3_draw_block(x,y+1,colors);
					}
#else
					if (interlace)
						vic3_draw_block(x,y,offset);
					else
						vic3_interlace_draw_block(x,y+1,offset);
#endif
				}
			}
		}
		interlace^=1;
	} else {
		for ( y1s=0, offset=0; y1s<200; y1s+=8) {
			for (x=0; x<VIC3_BITPLANES_WIDTH; x+=8) {
				for ( y=y1s; y<y1s+8; y++, offset++) {
#ifndef OPTIMIZE
					vic3_block_2_color(offset,colors);
					vic3_draw_block(x,y,colors);
#else
					vic3_draw_block(x,y,offset);
#endif
				}
			}
		}
	}
	if (XPOS>0) {
		vis.min_x=0;
		vis.max_x=XPOS-1;
		vis.min_y=0;
		vis.max_y=Machine->visible_area.max_y;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
	if (XPOS+VIC3_BITPLANES_WIDTH<Machine->visible_area.max_x) {
		vis.min_x=XPOS+VIC3_BITPLANES_WIDTH;
		vis.max_x=Machine->visible_area.max_x;
		vis.min_y=0;
		vis.max_y=Machine->visible_area.max_y;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
	if (YPOS>0) {
		vis.min_y=0;
		vis.max_y=YPOS-1;
		vis.min_x=0;
		vis.max_x=Machine->visible_area.max_x;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
#if 0
	if (YPOS+VIC3_LINES<Machine->visible_area.max_y) {
		vis.min_y=YPOS+VIC3_LINES;
		vis.max_y=Machine->visible_area.max_y;
		vis.min_x=0;
		vis.max_x=Machine->visible_area.max_x;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
#endif
}
