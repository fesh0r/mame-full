#include <assert.h>
#include "driver.h"

#include "includes/vc4000.h"
#include "cpu/s2650/s2650.h"

/*
  emulation of signetics 2636 video/audio device

  seams to me like a special microcontroller
  mask programmed

  note about mame s2636 emulation:
  less encapsuled
  missing grid, sound generation, retriggered sprites, paddle reading, score display
  not "rasterline" based
 */

static UINT8 sprite_collision[0x20];
static UINT8 background_collision[0x20];

typedef struct {
    UINT8 bitmap[10],x1,x2,y1,y2, res1, res2;
} SPRITE_HELPER;

typedef struct {
    SPRITE_HELPER *const data;
    const int mask;
    int state;
    int delay;
    int size;
    int y;
    int finished;
} SPRITE;

#ifdef DEBUG
int _y;
#endif

static struct {
    SPRITE sprites[4];
    int line;
    UINT8 sprite_collision;
    UINT8 background_collision;
    union {
	UINT8 data[0x100];
	struct {
	    SPRITE_HELPER sprites[3];
	    UINT8 res[0x10];
	    SPRITE_HELPER sprite4;
	    UINT8 res2[0x30];
	    UINT8 grid[20][2];
	    UINT8 grid_control[5];
	    UINT8 res3[0x13];
	    UINT8 sprite_sizes;
	    UINT8 sprite_colors[2];
	    UINT8 score_control;
	    UINT8 res4[2];
	    UINT8 background;
	    UINT8 sound;
	    UINT8 bcd[2];
	    UINT8 background_collision;
	    UINT8 sprite_collision;
	} d;
    } reg;
} vc4000_video={
    { 
	{ &vc4000_video.reg.d.sprites[0],1 },
	{ &vc4000_video.reg.d.sprites[1],2 }, 
	{ &vc4000_video.reg.d.sprites[2],4 },
	{ &vc4000_video.reg.d.sprite4,8 }
    }    
};

VIDEO_START(vc4000)
{
    int i;
    for (i=0;i<0x20; i++) {
	sprite_collision[i]=0;
	if ((i&3)==3) sprite_collision[i]|=0x20;
	if ((i&5)==5) sprite_collision[i]|=0x10;
	if ((i&9)==9) sprite_collision[i]|=8;
	if ((i&6)==6) sprite_collision[i]|=4;
	if ((i&0xa)==0xa) sprite_collision[i]|=2;
	if ((i&0xc)==0xc) sprite_collision[i]|=1;
	background_collision[i]=0;
	if ((i&0x11)==0x11) background_collision[i]|=0x80;
	if ((i&0x12)==0x12) background_collision[i]|=0x40;
	if ((i&0x14)==0x14) background_collision[i]|=0x20;
	if ((i&0x18)==0x18) background_collision[i]|=0x10;
    }

    return 0;
}

READ_HANDLER(vc4000_video_r)
{
    UINT8 data=0;
    switch (offset) {
    case 0xca:
	data|=vc4000_video.background_collision;
	if (vc4000_video.sprites[3].finished) data |=1;
	if (vc4000_video.sprites[2].finished) data |=2;
	if (vc4000_video.sprites[1].finished) data |=4;
	if (vc4000_video.sprites[0].finished) data |=8;
//	logerror("%d read %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xcb:
	data=vc4000_video.sprite_collision
	    |(vc4000_video.reg.d.sprite_collision&0xc0);
//	logerror("%d read %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
#ifndef ANALOG_HACK
    case 0xcc:
	if (!activecpu_get_reg(S2650_FO)) data=input_port_7_r(0)<<3;
	else data=input_port_8_r(0)<<3;
	break;
    case 0xcd:
	if (!activecpu_get_reg(S2650_FO)) data=input_port_9_r(0)<<3;
	else data=input_port_10_r(0)<<3;
	break;
#else
    case 0xcc:
	data = 0x66;
	// between 20 and 225
	if (!activecpu_get_reg(S2650_FO)) {
	    if (input_port_7_r(0)&0x1) data=20; // 0x20 too big
	    if (input_port_7_r(0)&0x2) data=225;
	} else {
	    if (input_port_7_r(0)&0x4) data=225;
	    if (input_port_7_r(0)&0x8) data=20;
	}
	break;
    case 0xcd:
	data = 0x66; //shootout 0x67 shoots right, 0x66 shoots left
	if (!activecpu_get_reg(S2650_FO)) {
	    if (input_port_7_r(0)&0x10) data=20;
	    if (input_port_7_r(0)&0x20) data=225;
	} else {
	    if (input_port_7_r(0)&0x40) data=225;
	    if (input_port_7_r(0)&0x80) data=20;
	}
	break;
#endif
    default:
	data=vc4000_video.reg.data[offset];
    }
    return data;
}

WRITE_HANDLER(vc4000_video_w)
{
    int nr;
    switch (offset) {
    case 0xa: case 0x1a: case 0x2a: case 0x4a:
	vc4000_video.reg.data[offset]=data;
	nr=offset>>4; if (nr==4) nr=3;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xc: case 0x1c: case 0x2c: case 0x4c:
	vc4000_video.reg.data[offset]=data;
	nr=offset>>4; if (nr==4) nr=3;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xb: case 0x1b: case 0x2b: case 0x4b:
    case 0xd: case 0x1d: case 0x2d: case 0x4d:
	vc4000_video.reg.data[offset]=data;
	nr=offset>>4; if (nr==4) nr=3;
	vc4000_video.sprites[nr].finished=FALSE;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xc0:
	vc4000_video.sprites[0].size=1<<(data&3); // invaders bases, shootout kaktus else too large
	vc4000_video.sprites[1].size=1<<((data>>2)&3);
	vc4000_video.sprites[2].size=1<<((data>>4)&3);
	vc4000_video.sprites[3].size=1<<((data>>6)&3);
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xc7:
	vc4000_video.reg.data[offset]=data;
	vc4000_soundport_w(0, data);
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xca:
	vc4000_video.reg.data[offset]=data;
	vc4000_video.background_collision=data;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    case 0xcb:
	vc4000_video.reg.data[offset]=data;
	vc4000_video.sprite_collision=data;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
	break;
    default:
	vc4000_video.reg.data[offset]=data;
//	logerror("%d wrote %.2x %.2x\n", vc4000_video.line, offset, data);
    }
}

READ_HANDLER(vc4000_vsync_r)
{
    return vc4000_video.line>=250?0x80:0;
}

static char led[20][12+1]={
    "  aaaaaaaa  ",
    "  aaaaaaaa  ",
    "ff        bb",
    "ff        bb",
    "ff        bb",
    "ff        bb",
    "ff        bb",
    "ff        bb",
    "ff        bb",
    "  gggggggg  ",
    "  gggggggg  ",
    "ee        cc",
    "ee        cc",
    "ee        cc",
    "ee        cc",
    "ee        cc",
    "ee        cc",
    "ee        cc",
    "  dddddddd  ",
    "  dddddddd  "
};

static void vc4000_draw_digit(struct mame_bitmap *bitmap, int x, int y, int d, int line)
{
    static const int digit_to_segment[0x10]={ 
	0x3f, 6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 7, 0x7f, 0x6f
    };
    int i,j;
    i=line;
    for (j=0; j<sizeof(led[0]); j++) {
	if (digit_to_segment[d]&(1<<(led[i][j]-'a')) ) {
	    plot_pixel(bitmap, x+j, y+i, Machine->pens[((vc4000_video.reg.d.background>>4)&7)^7]);
	}
    }
}

INLINE void vc4000_collision_plot(UINT8 *collision, UINT8 data, UINT8 color, int scale)
{
    int j,m;
    for (j=0,m=0x80; j<8; j++, m>>=1) {
	if (data&m) {
	    int i;
	    for (i=0; i<scale; i++, collision++) *collision|=color;
	} else
	    collision+=scale;
    }
}

static void vc4000_sprite_update(struct mame_bitmap *bitmap, UINT8 *collision, SPRITE *This)
{
#ifdef DEBUG
    char message[40];
    int oldstate=This->state;
#endif

    if (vc4000_video.line==0) {
	if (This->data->y1!=0) {
	    This->state=0;
	    This->y=This->data->y1;
	} else {
	    This->state=11;
	}
	This->delay=0;
	This->finished=FALSE;
#ifdef DEBUG
	_y=0;
#endif
    }

    if (vc4000_video.line>280) return;

    switch (This->state) {
    case 0:
//	if (vc4000_video.line!=This->data->y1) break;
	if (This->data->y1==0) {
	    This->state=11;
	    break;
	}
	if (This->y!=0) {
	    This->y--;
	    break;
	}
#ifdef DEBUG
	{
	int i;
	for (i=0;i<10;i++) 
	    drawgfx(bitmap, Machine->gfx[0], This->data->bitmap[i],0,
		    0,0,240,_y+i,
		    0, TRANSPARENCY_PEN,0);
	sprintf(message, "%d %d %.2x %.2x", vc4000_video.line, This-vc4000_video.sprites, This->data->x1, This->data->y1);
	ui_text(Machine->scrbitmap, message, 170, _y);_y+=10;
	}
#endif

	This->state++;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:case 9:case 10:
	if (vc4000_video.line<bitmap->height
	    &&This->data->x1<bitmap->width) {
	    vc4000_collision_plot(collision+This->data->x1, This->data->bitmap[This->state-1],
				  This->mask,This->size);
	    drawgfxzoom(bitmap, Machine->gfx[0], This->data->bitmap[This->state-1],0,
			0,0,This->data->x1,vc4000_video.line,
			0, TRANSPARENCY_PEN,0, This->size<<16, This->size<<16);
	}
	This->delay++;
	if (This->delay>=This->size) {
	    This->delay=0;
	    This->state++;
	}
	break;
    case 11:
	if (This->data->y2==0) {
	    This->finished=TRUE;
	    This->state=23;
	    break;
	}
	This->y=This->data->y2;
	This->delay=0;
	This->state++;
    case 12:
	if (This->y!=0) {
	    This->y--;
	    break;
	}
#ifdef DEBUG
	{
	int i;
	for (i=0;i<10;i++) 
	    drawgfx(bitmap, Machine->gfx[0], This->data->bitmap[i],0,
		    0,0,240,_y+i,
		    0, TRANSPARENCY_PEN,0);
	sprintf(message, ":%d %d %.2x %.2x", vc4000_video.line, This-vc4000_video.sprites, This->data->x2, This->data->y2);
	ui_text(Machine->scrbitmap, message, 170, _y);
	_y+=10;
	}
#endif
	This->state++;
    case 13: case 14: case 15: case 16: case 17: case 18: case 19:case 20:case 21:case 22:
	if (vc4000_video.line<bitmap->height
	    &&This->data->x2<bitmap->width) {
	    vc4000_collision_plot(collision+This->data->x2,This->data->bitmap[This->state-13],
				  This->mask,This->size);
	    drawgfxzoom(bitmap, Machine->gfx[0], This->data->bitmap[This->state-13],0,
			0,0,This->data->x2,vc4000_video.line,
			0, TRANSPARENCY_PEN,0, This->size<<16, This->size<<16);
	}
	This->delay++;
	if (This->delay<This->size) break;
	This->delay=0;
	This->state++;
	if (This->state<23) break;
	This->finished=TRUE;
	break;
    case 23:
	if (This->finished) break;
	This->state=11;
	break;
    }
#ifdef DEBUG
    if (oldstate!=This->state)
	logerror("%d %d state changed from %d to %d\n",vc4000_video.line,
		 This-vc4000_video.sprites,oldstate,This->state);
#endif
}


INLINE void vc4000_draw_grid(UINT8 *collision)
{
    int i, j, m, x, line=vc4000_video.line-20;
    int w, k;

    if (vc4000_video.line>=Machine->scrbitmap->height) return;
#ifndef DEBUG
    plot_box(Machine->scrbitmap, 0, vc4000_video.line, Machine->scrbitmap->width, 1, Machine->pens[(vc4000_video.reg.d.background)&7]);
#else
    plot_box(Machine->scrbitmap, 0, vc4000_video.line, 170, 1, Machine->pens[5]);
#endif

	if (line<0 || line>=200) return;
	if (!vc4000_video.reg.d.background&8) return;

	i=(line/20)*2;
	if (line%20>=2) i++;

	k=vc4000_video.reg.d.grid_control[i>>2];
	switch (k>>6) {
		default:
		case 0:case 2: w=1;break;
		case 1: w=2;break;
		case 3: w=4;break;
	}
	switch (i&3) {
	case 0: 
		if (k&1) w=8;break;
	case 1:
		if ((line%40)<=10) {
			if (k&2) w=8;
		} else {
			if (k&4) w=8;
		}
		break;
	case 2: if (k&8) w=8;break;
	case 3:
		if ((line%40)<=30) {
			if (k&0x10) w=8;
		} else {
			if (k&0x20) w=8;
		}
		break;
	}
	for (x=30, j=0, m=0x80; j<16; j++, x+=8, m>>=1) {
		if (vc4000_video.reg.d.grid[i][j>>3]&m) {
			int l;
			for (l=0; l<w; l++) {
			collision[x+l]|=0x10;
			}
			plot_box(Machine->scrbitmap, x, vc4000_video.line, w, 1, Machine->pens[(vc4000_video.reg.d.background>>4)&7]);
		}
		if (j==7) m=0x100;
	}
}

INTERRUPT_GEN( vc4000_video_line )
{
    int x,y,i;
    UINT8 collision[400]={0}; // better alloca or gcc feature of non constant long automatic arrays
    assert(sizeof(collision)>=Machine->scrbitmap->width);

    vc4000_video.line++;
    vc4000_video.line%=312;

    if (vc4000_video.line==0) {
	vc4000_video.background_collision=0;
	vc4000_video.sprite_collision=0;
#ifdef DEBUG
	logerror("begin of frame\n");
	plot_box(Machine->scrbitmap, 0, 0, 
		 Machine->scrbitmap->width, Machine->scrbitmap->height, 
		 Machine->pens[1]);
	for (i=0; i<4; i++) {
	    char message[40];
	    sprintf(message, "%.2x %.2x %.2x %.2x", 
		    vc4000_video.sprites[i].data->x1, vc4000_video.sprites[i].data->y1,
		    vc4000_video.sprites[i].data->x2, vc4000_video.sprites[i].data->y2);
	    ui_text(Machine->scrbitmap, message, 250, i*8);
	}
#endif
    }

    vc4000_draw_grid(collision);

    Machine->gfx[0]->colortable[1]=Machine->pens[8|((vc4000_video.reg.d.sprite_colors[0]>>3)&7)];
    vc4000_sprite_update(Machine->scrbitmap, collision, &vc4000_video.sprites[0]);
    Machine->gfx[0]->colortable[1]=Machine->pens[8|(vc4000_video.reg.d.sprite_colors[0]&7)];
    vc4000_sprite_update(Machine->scrbitmap, collision, &vc4000_video.sprites[1]);
    Machine->gfx[0]->colortable[1]=Machine->pens[8|((vc4000_video.reg.d.sprite_colors[1]>>3)&7)];
    vc4000_sprite_update(Machine->scrbitmap, collision, &vc4000_video.sprites[2]);
    Machine->gfx[0]->colortable[1]=Machine->pens[8|(vc4000_video.reg.d.sprite_colors[1]&7)];
    vc4000_sprite_update(Machine->scrbitmap, collision, &vc4000_video.sprites[3]);

//    if (!(vc4000_video.reg.d.sprite_collision&0x40)) {
//	for (int i=0; i<Machine->scrbitmap->width; i++) {
	for (i=0; i<160; i++) {
	    vc4000_video.sprite_collision|=sprite_collision[collision[i]];
	    vc4000_video.background_collision|=background_collision[collision[i]];
	}
//    }

    y=vc4000_video.reg.d.score_control&1?180:0;
    y+=20;
    if ((vc4000_video.line>=y)&&(vc4000_video.line<y+20)) {
	x=128/2+30;
	if (vc4000_video.reg.d.score_control&2) x-=10;
	vc4000_draw_digit(Machine->scrbitmap, x-16*2+2, y, vc4000_video.reg.d.bcd[0]>>4, vc4000_video.line-y);
	vc4000_draw_digit(Machine->scrbitmap, x-16+2, y, vc4000_video.reg.d.bcd[0]&0xf, vc4000_video.line-y);
	if (vc4000_video.reg.d.score_control&2) x+=20;
	vc4000_draw_digit(Machine->scrbitmap, x+2, y, vc4000_video.reg.d.bcd[1]>>4, vc4000_video.line-y);
	vc4000_draw_digit(Machine->scrbitmap, x+16+2, y, vc4000_video.reg.d.bcd[1]&0xf, vc4000_video.line-y);
    }
	
    cpu_irq_line_vector_w(0, 0, 3);
//    cpu_set_irq_line(0, S2650_INT_IRQ, vc4000_video.line==280?1:0);
    cpu_set_irq_line(0, 0, vc4000_video.line==280?1:0);
}

VIDEO_UPDATE( vc4000 )
{
#if 0
    char str[0x40];
//    snprintf(str, sizeof(str), "%.2x %.2x %.2x %.2x",
//	     input_port_7_r(0), input_port_8_r(0),
//	     input_port_9_r(0), input_port_10_r(0));

//    snprintf(str, sizeof(str), "%.2x %.2x %.2x", 
//	     arcadia_video.reg.d.control, arcadia_video.reg.d.sound1, arcadia_video.reg.d.sound2);
	snprintf(str, sizeof(str), "%.2x:%.2x %.2x:%.2x %.2x:%.2x %.2x:%.2x",
			vc4000_video.reg.d.pos[0].x,
			vc4000_video.reg.d.pos[0].y,
			vc4000_video.reg.d.pos[1].x,
			vc4000_video.reg.d.pos[1].y,
			vc4000_video.reg.d.pos[2].x,
			vc4000_video.reg.d.pos[2].y,
			vc4000_video.reg.d.pos[3].x,
			vc4000_video.reg.d.pos[3].y );
	ui_text(bitmap, str, 0, 0);
	snprintf(str, sizeof(str), "%.2x:%.2x %.2x:%.2x %.2x:%.2x %.2x:%.2x",
			vc4000_video.pos[0].x,
			vc4000_video.pos[0].y,
			vc4000_video.pos[1].x,
			vc4000_video.pos[1].y,
			vc4000_video.pos[2].x,
			vc4000_video.pos[2].y,
			vc4000_video.pos[3].x,
			vc4000_video.pos[3].y );
	ui_text(bitmap, str, 0, 8);
#endif
}
