#include "driver.h"

#include "includes/arcadia.h"

/*
  emulation of signetics 2637 video/audio device

  seams to me like a special microcontroller
  mask programmed
  1k x 8 ram available; only first 0x300 bytes mapped to cpu
 */

static UINT8 rectangle[0x40][8]={
    { 0 }
};
static UINT8 chars[0x40][8]={
    // read from the screen generated from a palladium
    { 0,0,0,0,0,0,0,0 },			// 00 (space)
    { 1,2,4,8,16,32,64,128 }, //           ; 01 (\)
    { 128,64,32,16,8,4,2,1 }, //           ; 02 (/)
    { 255,255,255,255,255,255,255,255 }, //; 03 (solid block)
    { 0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00 },//  04 (?)
    { 3,3,3,3,3,3,3,3 }, //        ; 05 (half square right on)
    { 0,0,0,0,0,0,255,255 }, //              ; 06 (horz lower line)
    { 0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },// 07 (half square left on)
    { 0xff,0xff,3,3,3,3,3,3 },			// 08 (?)
    { 0xff,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },// 09 (?)
    { 192,192,192,192,192,192,255,255 }, //; 0A (!_)
    { 3,3,3,3,3,3,255,255 }, //            ; 0B (_!)
    { 1,3,7,15,31,63,127,255 }, //         ; 0C (diagonal block)
    { 128,192,224,240,248,252,254,255 }, //; 0D (diagonal block)
    { 255,254,252,248,240,224,192,128 }, //; 0E (diagonal block)
    { 255,127,63,31,15,7,3,1 }, //         ; 0F (diagonal block)
    { 0x00,0x1c,0x22,0x26,0x2a,0x32,0x22,0x1c },// 10 0
    { 0x00,0x08,0x18,0x08,0x08,0x08,0x08,0x1c },// 10 1
    { 0x00,0x1c,0x22,0x02,0x0c,0x10,0x20,0x3e },// 10 2
    { 0x00,0x3e,0x02,0x04,0x0c,0x02,0x22,0x1c },// 10 3
    { 0x00,0x04,0x0c,0x14,0x24,0x3e,0x04,0x04 },// 10 4
    { 0x00,0x3e,0x20,0x3c,0x02,0x02,0x22,0x1c },// 10 5
    { 0x00,0x0c,0x10,0x20,0x3c,0x22,0x22,0x1c },// 10 6
    { 0x00,0x7c,0x02,0x04,0x08,0x10,0x10,0x10 },// 10 7
    { 0x00,0x1c,0x22,0x22,0x1c,0x22,0x22,0x1c },// 10 8
    { 0x00,0x1c,0x22,0x22,0x3e,0x02,0x04,0x18 },// 10 9
    { 0x00,0x08,0x14,0x22,0x22,0x3e,0x22,0x22 },// 10 A
    { 0x00,0x3c,0x22,0x22,0x3c,0x22,0x22,0x3c },// 10 B
    { 0x00,0x1c,0x22,0x20,0x20,0x20,0x22,0x1c },// 10 C
    { 0x00,0x3c,0x22,0x22,0x22,0x22,0x22,0x3c },// 10 D
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x3e },// 10 E
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x20 },// 10 F
    { 0x00,0x1e,0x20,0x20,0x20,0x26,0x22,0x1e },// 10 G
    { 0x00,0x22,0x22,0x22,0x3e,0x22,0x22,0x22 },// 10 H
    { 0x00,0x1c,0x08,0x08,0x08,0x08,0x08,0x1c },// 10 I
    { 0x00,0x02,0x02,0x02,0x02,0x02,0x22,0x1c },// 10 J
    { 0x00,0x22,0x24,0x28,0x30,0x28,0x24,0x22 },// 10 K
    { 0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x3e },// 10 L
    { 0x00,0x22,0x36,0x2a,0x2a,0x22,0x22,0x22 },// 10 M
    { 0x00,0x22,0x22,0x32,0x2a,0x26,0x22,0x22 },// 10 N
    { 0x00,0x1c,0x22,0x22,0x22,0x22,0x22,0x1c },// 10 O
    { 0x00,0x3c,0x22,0x22,0x3c,0x20,0x20,0x20 },// 10 P
    { 0x00,0x1c,0x22,0x22,0x22,0x2a,0x24,0x1a },// 10 Q
    { 0x00,0x3c,0x22,0x22,0x3c,0x28,0x24,0x22 },// 10 R
    { 0x00,0x1c,0x22,0x20,0x1c,0x02,0x22,0x1c },// 10 S
    { 0x00,0x3e,0x08,0x08,0x08,0x08,0x08,0x08 },// 10 T
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x22,0x1c },// 10 U
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x14,0x08 },// 10 V
    { 0x00,0x22,0x22,0x22,0x2a,0x2a,0x36,0x22 },// 10 W
    { 0x00,0x22,0x22,0x14,0x08,0x14,0x22,0x22 },// 10 X
    { 0x00,0x22,0x22,0x14,0x08,0x08,0x08,0x08 },// 10 Y
    { 0x00,0x3e,0x02,0x04,0x08,0x10,0x20,0x3e },// 10 Z
    { 0,0,0,0,0,0,0,8 },			// 34  .
    { 0,0,0,0,0,8,8,0x10 },			// 35 ,
    { 0,0,8,8,0x3e,8,8,0 },			// 36 +
    { 0,8,0x1e,0x28,0x1c,0xa,0x3c,8 },		// 37 $
    // 8x user defined
};

static struct {
    int line;
    int charline;
    int shift;
    int ad_delay;
    int ad_select;
    struct { int x, y; } pos[4];
    UINT8 bg[208+YPOS+YBOTTOM_SIZE][16+2*XPOS/8];
    union {
	UINT8 data[0x400];
	struct {
	    // 0x1800
	    UINT8 chars1[13][16];
	    UINT8 ram1[2][16];
	    struct {
		UINT8 y,x;
	    } pos[4];
	    UINT8 ram2[4];
	    UINT8 control;
	    UINT8 sound1, sound2;
	    UINT8 char_line;
	    // 0x1900
	    UINT8 pad1a, pad1b, pad1c, pad1d;
	    UINT8 pad2a, pad2b, pad2c, pad2d;
	    UINT8 keys, unmapped3[0x80-9];
	    UINT8 chars[8][8];
	    UINT8 unknown[0x38];
	    UINT8 resolution, // bit 6 set means 208 lines, cleared 104 lines doublescanned
		pal1, pal2, pal3;
	    UINT8 collision_bg, 
		collision_sprite;
	    UINT8 ad[2];
	    // 0x1a00
	    UINT8 chars2[13][16];
	    UINT8 ram3[3][16];
	} d;
    } reg;
} arcadia_video={ 0 };

int arcadia_vh_start(void)
{
    int i;
    for (i=0; i<0x40; i++) {
	rectangle[i][0]=0;
	rectangle[i][4]=0;
	if (i&1) rectangle[i][0]|=7;
	if (i&2) rectangle[i][0]|=0x38;
	if (i&4) rectangle[i][0]|=0xc0;
	if (i&8) rectangle[i][4]|=7;
	if (i&0x10) rectangle[i][4]|=0x38;
	if (i&0x20) rectangle[i][4]|=0xc0;
	rectangle[i][1]=rectangle[i][2]=rectangle[i][3]=rectangle[i][0];
	rectangle[i][5]=rectangle[i][6]=rectangle[i][7]=rectangle[i][4];
    }

    return 0;
}

void arcadia_vh_stop(void)
{
}

READ_HANDLER(arcadia_video_r)
{
    UINT8 data=0;
    switch (offset) {
    case 0xff: data=arcadia_video.charline|0xf0;break;
    case 0x100: data=input_port_1_r(0);break;
    case 0x101: data=input_port_2_r(0);break;
    case 0x102: data=input_port_3_r(0);break;
    case 0x103: data=input_port_4_r(0);break;
    case 0x104: data=input_port_5_r(0);break;
    case 0x105: data=input_port_6_r(0);break;
    case 0x106: data=input_port_7_r(0);break;
    case 0x107: data=input_port_8_r(0);break;
    case 0x108: data=input_port_0_r(0);break;
#if 0
    case 0x1fe:
	if (arcadia_video.ad_select) data=input_port_10_r(0)<<3;
	else data=input_port_9_r(0)<<3;
	break;
    case 0x1ff:
	if (arcadia_video.ad_select) data=input_port_7_r(0)<<3;
	else data=input_port_8_r(0)<<3;
	break;
#else
    case 0x1fe:
	data = 0x80;
	if (arcadia_video.ad_select) {
	    if (input_port_9_r(0)&0x10) data=0;
	    if (input_port_9_r(0)&0x20) data=0xff;
	} else {
	    if (input_port_9_r(0)&0x40) data=0xff;
	    if (input_port_9_r(0)&0x80) data=0;
	}
	break;
    case 0x1ff:
	data = 0x6f; // 0x7f too big for alien invaders (movs right)
	if (arcadia_video.ad_select) {
	    if (input_port_9_r(0)&0x1) data=0;
	    if (input_port_9_r(0)&0x2) data=0xff;
	} else {
	    if (input_port_9_r(0)&0x4) data=0xff;
	    if (input_port_9_r(0)&0x8) data=0;
	}
	break;
#endif
    default:
	data=arcadia_video.reg.data[offset];
    }
    return data;
}

#if DEBUG
static int _y;
#endif

WRITE_HANDLER(arcadia_video_w)
{
#if DEBUG
    char str[40];
#endif
    switch (offset) {
    case 0xfc: case 0xfd: case 0xfe:
	arcadia_video.reg.data[offset]=data;
	arcadia_soundport_w(offset&3, data);
	if (offset==0xfe)
	    arcadia_video.shift=(data>>5);
	break;
    case 0xf0: case 0xf2: case 0xf4: case 0xf6:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.pos[(offset>>1)&3].y=(data^0xff)-16;
#if DEBUG
	snprintf(str, sizeof(str), "y %d %d",(offset>>1)&3,
		 arcadia_video.reg.d.pos[(offset>>1)&3].y );
	ui_text(Machine->scrbitmap, str, 120, _y);
	_y+=8;
#endif
	break;
    case 0xf1: case 0xf3: case 0xf5: case 0xf7:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.pos[(offset>>1)&3].x=data-44;
#if DEBUG
	snprintf(str, sizeof(str), "x %d %d",(offset>>1)&3,
		 arcadia_video.reg.d.pos[(offset>>1)&3].x );
	ui_text(Machine->scrbitmap, str, 120, _y);
	_y+=8;
#endif
	break;
    case 0x180: case 0x181: case 0x182: case 0x183: case 0x184: case 0x185: case 0x186: case 0x187:
    case 0x188: case 0x189: case 0x18a: case 0x18b: case 0x18c: case 0x18d: case 0x18e: case 0x18f:
    case 0x190: case 0x191: case 0x192: case 0x193: case 0x194: case 0x195: case 0x196: case 0x197:
    case 0x198: case 0x199: case 0x19a: case 0x19b: case 0x19c: case 0x19d: case 0x19e: case 0x19f:
    case 0x1a0: case 0x1a1: case 0x1a2: case 0x1a3: case 0x1a4: case 0x1a5: case 0x1a6: case 0x1a7:
    case 0x1a8: case 0x1a9: case 0x1aa: case 0x1ab: case 0x1ac: case 0x1ad: case 0x1ae: case 0x1af:
    case 0x1b0: case 0x1b1: case 0x1b2: case 0x1b3: case 0x1b4: case 0x1b5: case 0x1b6: case 0x1b7:
    case 0x1b8: case 0x1b9: case 0x1ba: case 0x1bb: case 0x1bc: case 0x1bd: case 0x1be: case 0x1bf:
	arcadia_video.reg.data[offset]=data;
	chars[0x38|((offset>>3)&7)][offset&7]=data;
	break;
    case 0x1f9:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.ad_delay=10;
	break;
    default:
	arcadia_video.reg.data[offset]=data;
    }
}

INLINE void arcadia_draw_char(struct osd_bitmap *bitmap, UINT8 *ch, int color, 
			      int y, int x)
{
    int k,b;
#if DEBUG
    Machine->gfx[0]->colortable[1]=Machine->pens[4];
#else
    Machine->gfx[0]->colortable[1]=
	Machine->pens[((arcadia_video.reg.d.pal1>>3)&1)|((color>>5)&6)];
#endif
    if (!(arcadia_video.reg.d.resolution&0x40)) {
	for (k=0; k<8; k++) {
	    b=ch[k];
	    arcadia_video.bg[y+k*2][x>>3]|=b>>(x&7);
	    arcadia_video.bg[y+k*2][(x>>3)+1]|=b<<(8-(x&7));
	    arcadia_video.bg[y+k*2+1][x>>3]|=b>>(x&7);
	    arcadia_video.bg[y+k*2+1][(x>>3)+1]|=b<<(8-(x&7));
	    drawgfx(bitmap, Machine->gfx[0], b,0,
		    0,0,x,y+k*2,
		    0, TRANSPARENCY_NONE,0);
	    drawgfx(bitmap, Machine->gfx[0], b,0,
		    0,0,x,y+k*2+1,
		    0, TRANSPARENCY_NONE,0);
	}
    } else {
	for (k=0; k<8; k++) {
	    b=ch[k];
	    arcadia_video.bg[y+k][x>>3]|=b>>(x&7);
	    arcadia_video.bg[y+k][(x>>3)+1]|=b<<(8-(x&7));
	    drawgfx(bitmap, Machine->gfx[0], b,0,
		    0,0,x,y+k,
		    0, TRANSPARENCY_NONE,0);
	}
    }
}

INLINE void arcadia_vh_draw_line(struct osd_bitmap *bitmap,
				 int y, UINT8 chars1[16])
{
    int x, ch, j, h;
    h=arcadia_video.reg.d.resolution&0x40?8:16;
    plot_box(bitmap, 0, y, XPOS+arcadia_video.shift, h, Machine->gfx[0]->colortable[0]);
    if (chars1[0]==0xc0) {
	for (x=XPOS+arcadia_video.shift, j=0; j<16;j++,x+=8) {
	    ch=chars1[j];
	    arcadia_draw_char(bitmap, rectangle[ch&0x3f], ch, y, x);
	}
    } else {
	for (x=XPOS+arcadia_video.shift, j=0; j<16;j++,x+=8) {
	    ch=chars1[j];	    
	    arcadia_draw_char(bitmap, chars[ch&0x3f], ch, y, x);
	}
    }
#if !DEBUG
    plot_box(bitmap, x, y, bitmap->width-x, h, Machine->gfx[0]->colortable[0]);
#endif
}

int arcadia_video_line(void)
{
    if (arcadia_video.ad_delay<=0)
	arcadia_video.ad_select=arcadia_video.reg.d.pal1&0x40;
    else arcadia_video.ad_delay--;

    arcadia_video.line++;
    arcadia_video.line%=262;
#if DEBUG
    if (arcadia_video.line==0) _y=0;
#endif
    // unbelievable, reflects only charline, but alien invaders uses it for 
    // alien scrolling
    if (!(arcadia_video.reg.d.resolution&0x40)) {
	if ( (arcadia_video.line>=8)&&
	     (((arcadia_video.line-8)&0xf)==0)&&(arcadia_video.line<=8+208)) {
	    arcadia_video.charline=(arcadia_video.line-8)>>4;
	    if (arcadia_video.charline<13) {
		Machine->gfx[0]->colortable[0]=Machine->pens[arcadia_video.reg.d.pal1&7];
		memset(arcadia_video.bg[YPOS+arcadia_video.charline*16], 0, 
		       sizeof(arcadia_video.bg[0])*16);
		arcadia_vh_draw_line(Machine->scrbitmap, arcadia_video.charline*16+YPOS,
				     arcadia_video.reg.d.chars1[arcadia_video.charline]);
	    }
	}
    } else {
	if ( (arcadia_video.line>=8)&&
	     (((arcadia_video.line-8)&0x7)==0)&&(arcadia_video.line<=8+208)) {
	    arcadia_video.charline=(arcadia_video.line-8)>>3;
	    if (arcadia_video.charline<13) {
		Machine->gfx[0]->colortable[0]=Machine->pens[arcadia_video.reg.d.pal1&7];
		memset(arcadia_video.bg[YPOS+arcadia_video.charline*8], 0, 
		       sizeof(arcadia_video.bg[0])*8);
		arcadia_vh_draw_line(Machine->scrbitmap, arcadia_video.charline*8+YPOS,
				     arcadia_video.reg.d.chars1[arcadia_video.charline]);
	    } else {
		arcadia_video.charline-=13;
		if (arcadia_video.charline<13) {
		    Machine->gfx[0]->colortable[0]=Machine->pens[arcadia_video.reg.d.pal1&7];
		    memset(arcadia_video.bg[YPOS+arcadia_video.charline*8+13*8], 0, 
			   sizeof(arcadia_video.bg[0])*8);
		    arcadia_vh_draw_line(Machine->scrbitmap, arcadia_video.charline*8+13*8+YPOS,
					 arcadia_video.reg.d.chars2[arcadia_video.charline]);
		}
	    }
	}
    }
    return 0;
}

READ_HANDLER(arcadia_vsync_r)
{
    return arcadia_video.line>=216?0x80:0;
}

//bool arcadia_sprite_collision(int n1, int n2)
int arcadia_sprite_collision(int n1, int n2)
{
    int k, b1, b2, x;
    if (arcadia_video.pos[n1].x+8<=arcadia_video.pos[n2].x) return FALSE;
    if (arcadia_video.pos[n1].x>=arcadia_video.pos[n2].x+8) return FALSE;
    for (k=0; k<8; k++) {
	if (arcadia_video.pos[n1].y+k<arcadia_video.pos[n2].y) continue;
	if (arcadia_video.pos[n1].y+k>=arcadia_video.pos[n2].y+8) break;
	x=arcadia_video.pos[n1].x-arcadia_video.pos[n2].x;
	b1=arcadia_video.reg.d.chars[n1][k];
	b2=arcadia_video.reg.d.chars[n2][arcadia_video.pos[n1].y+k-arcadia_video.pos[n2].y];
	if (x<0) b2>>=-x;
	if (x>0) b1>>=x;
	if (b1&b2) return TRUE;
    }
    return FALSE;
}

void arcadia_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
    int i, k, x, y;
    UINT8 b;
    
    arcadia_video.reg.d.collision_bg|=0xf;
    arcadia_video.reg.d.collision_sprite|=0x3f;
    plot_box(bitmap, 0, 0, bitmap->width, YPOS, Machine->gfx[0]->colortable[0]);
    plot_box(bitmap, 0, YPOS+208, bitmap->width, bitmap->height-YPOS-208, Machine->gfx[0]->colortable[0]);
    for (i=0; i<4; i++) {
	if (arcadia_video.pos[i].y<=-YPOS) continue;
	if (arcadia_video.pos[i].y>=bitmap->height-YPOS-8) continue;
	if (arcadia_video.pos[i].x<=-XPOS) continue;
	if (arcadia_video.pos[i].x>=128+XPOS-8) continue;

	switch (i) {
	case 0:
	    Machine->gfx[0]->colortable[1]=Machine->pens[(arcadia_video.reg.d.pal3>>3)&7];
	    break;
	case 1:
	    Machine->gfx[0]->colortable[1]=Machine->pens[arcadia_video.reg.d.pal3&7];
	    break;
	case 2:
	    Machine->gfx[0]->colortable[1]=Machine->pens[(arcadia_video.reg.d.pal2>>3)&7];
	    break;
	case 3:
	    Machine->gfx[0]->colortable[1]=Machine->pens[arcadia_video.reg.d.pal2&7];
	    break;
	}
	for (k=0; k<8; k++) {
	    b=arcadia_video.reg.d.chars[i][k];
	    x=arcadia_video.pos[i].x+XPOS;
	    y=arcadia_video.pos[i].y+YPOS+k;
	    drawgfx(bitmap, Machine->gfx[0], b,0,
		    0,0,x,y,
		    0, TRANSPARENCY_PEN,0);
	    if (arcadia_video.reg.d.collision_bg&(1<<i)) {
		if ( (b<<(8-(x&7))) & ((arcadia_video.bg[y][x>>3]<<8)
				       |arcadia_video.bg[y][(x>>3)+1]) )
		    arcadia_video.reg.d.collision_bg&=~(1<<i);
	    }
	}
    }
    if (arcadia_sprite_collision(0,1)) arcadia_video.reg.d.collision_sprite&=~1;
    if (arcadia_sprite_collision(0,2)) arcadia_video.reg.d.collision_sprite&=~2;
    if (arcadia_sprite_collision(0,3)) arcadia_video.reg.d.collision_sprite&=~4;
    if (arcadia_sprite_collision(1,2)) arcadia_video.reg.d.collision_sprite&=~8;
    if (arcadia_sprite_collision(1,3)) arcadia_video.reg.d.collision_sprite&=~0x10; //guess
    if (arcadia_sprite_collision(2,3)) arcadia_video.reg.d.collision_sprite&=~0x20; //guess

#if DEBUG
{
    char str[0x40];
//    snprintf(str, sizeof(str), "%.2x %.2x %.2x %.2x",
//	     input_port_7_r(0), input_port_8_r(0),
//	     input_port_9_r(0), input_port_10_r(0));

//    snprintf(str, sizeof(str), "%.2x %.2x %.2x", 
//	     arcadia_video.reg.d.control, arcadia_video.reg.d.sound1, arcadia_video.reg.d.sound2);
    snprintf(str, sizeof(str), "%.2x:%.2x %.2x:%.2x %.2x:%.2x %.2x:%.2x",
	     arcadia_video.reg.d.pos[0].x,
	     arcadia_video.reg.d.pos[0].y,
	     arcadia_video.reg.d.pos[1].x,
	     arcadia_video.reg.d.pos[1].y,
	     arcadia_video.reg.d.pos[2].x,
	     arcadia_video.reg.d.pos[2].y,
	     arcadia_video.reg.d.pos[3].x,
	     arcadia_video.reg.d.pos[3].y );
    ui_text(bitmap, str, 0, 0);
    snprintf(str, sizeof(str), "%.2x:%.2x %.2x:%.2x %.2x:%.2x %.2x:%.2x",
	     arcadia_video.pos[0].x,
	     arcadia_video.pos[0].y,
	     arcadia_video.pos[1].x,
	     arcadia_video.pos[1].y,
	     arcadia_video.pos[2].x,
	     arcadia_video.pos[2].y,
	     arcadia_video.pos[3].x,
	     arcadia_video.pos[3].y );
    ui_text(bitmap, str, 0, 8);
    snprintf(str, sizeof(str), "%.2x %.2x %.2x %.2x",
	     arcadia_video.reg.d.resolution,
	     arcadia_video.reg.d.pal1,
	     arcadia_video.reg.d.pal2,
	     arcadia_video.reg.d.pal3 );
    ui_text(bitmap, str, 0, 16);
}
#endif
}
