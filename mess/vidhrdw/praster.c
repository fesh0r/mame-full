/***************************************************************************

  Pet's Rasterengine

***************************************************************************/
#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/praster.h"

/* todo update praster_raster_monotext, praster_raster_graphic */

PRASTER raster1= { 0 } , raster2= { 0 };

/* struct to be initialised with the functions for the colordepth */
static struct {
	void (*draw_character)(struct osd_bitmap *bitmap,int ybegin, int yend,
							UINT8 *font, int y, int x, UINT16 *color);
	void (*draw_bytecode)(struct osd_bitmap *bitmap, UINT8 code,
						  int y, int x, UINT16 *color);
	void (*draw_pixel)(struct osd_bitmap *bitmap, int y, int x, UINT16 color);
} praster = { NULL };

static void praster_videoram_w(PRASTER *this, int offset, int data)
{
	offset&=this->memory.mask;
	if (this->memory.ram[offset]!=data) {
	   this->memory.ram[offset] = data;
	   if ((offset>=this->memory.videoram.offset)
		   &&(offset<this->memory.videoram.offset+this->memory.videoram.size) )
		   this->text.dirtybuffer[offset] = 1;
	   if ( (offset>=this->memory.colorram.offset)
			&&(offset<this->memory.colorram.offset+this->memory.colorram.size))
		   this->text.dirtybuffer[offset-this->memory.colorram.offset] = 1;
	}
}

static int praster_videoram_r(PRASTER * this, int offset)
{
	return this->memory.ram[offset&this->memory.mask];
}

static	void praster_draw_pixel8(struct osd_bitmap *bitmap, int y, int x, UINT16 color)
{
	bitmap->line[y][x] = color;
}

static	void praster_draw_pixel16(struct osd_bitmap *bitmap, int y, int x, UINT16 color)
{
	*((short *) bitmap->line[y] + x) = color;
}

static void praster_draw_bytecode8(struct osd_bitmap *bitmap, UINT8 code, int y, int x, UINT16 *color)
{
	bitmap->line[y][x] = color[code >> 7];
	bitmap->line[y][1 + x] = color[(code >> 6) & 1];
	bitmap->line[y][2 + x] = color[(code >> 5) & 1];
	bitmap->line[y][3 + x] = color[(code >> 4) & 1];
	bitmap->line[y][4 + x] = color[(code >> 3) & 1];
	bitmap->line[y][5 + x] = color[(code >> 2) & 1];
	bitmap->line[y][6 + x] = color[(code >> 1) & 1];
	bitmap->line[y][7 + x] = color[code & 1];
}

static void praster_draw_bytecode16(struct osd_bitmap *bitmap, UINT8 code, int y, int x, UINT16 *color)
{
	*((short *) bitmap->line[y] + x) = color[code >> 7];
	*((short *) bitmap->line[y] + 1 + x) = color[(code >> 6) & 1];
	*((short *) bitmap->line[y] + 2 + x) = color[(code >> 5) & 1];
	*((short *) bitmap->line[y] + 3 + x) = color[(code >> 4) & 1];
	*((short *) bitmap->line[y] + 4 + x) = color[(code >> 3) & 1];
	*((short *) bitmap->line[y] + 5 + x) = color[(code >> 2) & 1];
	*((short *) bitmap->line[y] + 6 + x) = color[(code >> 1) & 1];
	*((short *) bitmap->line[y] + 7 + x) = color[code & 1];
}

static void praster_draw_character8(struct osd_bitmap *bitmap, int ybegin, int yend,
									UINT8 *font, int y, int x, UINT16 *color)
{
	int i, code;
	for (i = ybegin; i <= yend; i++)
		{
			code=font[i];
			bitmap->line[y + i][x] = color[code >> 7];
			bitmap->line[y + i][1 + x] = color[(code >> 6) & 1];
			bitmap->line[y + i][2 + x] = color[(code >> 5) & 1];
			bitmap->line[y + i][3 + x] = color[(code >> 4) & 1];
			bitmap->line[y + i][4 + x] = color[(code >> 3) & 1];
			bitmap->line[y + i][5 + x] = color[(code >> 2) & 1];
			bitmap->line[y + i][6 + x] = color[(code >> 1) & 1];
			bitmap->line[y + i][7 + x] = color[code & 1];
		}
}

static void praster_draw_character16(struct osd_bitmap *bitmap, int ybegin, int yend,
									 UINT8 *font, int y, int x, UINT16 *color)
{
	int i, code;
	for (i = ybegin; i <= yend; i++)
		{
			code=font[i];
			*((short *) bitmap->line[y + i] + x) = color[code >> 7];
			*((short *) bitmap->line[y + i] + 1 + x) = color[(code >> 6) & 1];
			*((short *) bitmap->line[y + i] + 2 + x) = color[(code >> 5) & 1];
			*((short *) bitmap->line[y + i] + 3 + x) = color[(code >> 4) & 1];
			*((short *) bitmap->line[y + i] + 4 + x) = color[(code >> 3) & 1];
			*((short *) bitmap->line[y + i] + 5 + x) = color[(code >> 2) & 1];
			*((short *) bitmap->line[y + i] + 6 + x) = color[(code >> 1) & 1];
			*((short *) bitmap->line[y + i] + 7 + x) = color[code & 1];
		}
}

static void praster_markdirty(PRASTER *this, int px, int py)
{
	int t1, t2;
	if (px<this->raytube.screenpos.x) return ;
	if (py<this->raytube.screenpos.y) return ;
	t1 = (px-this->raytube.screenpos.x)/this->text.charsize.x;
	if (t1>=this->text.size.x) return ;
	t2 = (py-this->raytube.screenpos.y)/this->text.charsize.y;
	if (t2>=this->text.size.y) return ;
	this->text.dirtybuffer[t2*this->text.size.x+t1]=1;
}

void praster_draw_text (PRASTER *this, char *text, int *y)
{
	int x, x0, y2, width = (Machine->visible_area.max_x -
							Machine->visible_area.min_x) / Machine->uifont->width;

	if (text[0] != 0)
	{
		x = strlen (text);
		*y -= Machine->uifont->height * ((x + width - 1) / width);
		y2 = *y + Machine->uifont->height;
		x = 0;
		while (text[x])
		{
			for (x0 = Machine->visible_area.min_x;
				 text[x] && (x0 < Machine->visible_area.max_x -
							 Machine->uifont->width);
				 x++, x0 += Machine->uifont->width)
			{
				drawgfx (this->display.bitmap, Machine->uifont,
						 text[x], 0, 0, 0, x0, y2, 0,
						 TRANSPARENCY_NONE, 0);
				/* i hope its enough to mark the chars under the four edge as
				   dirty */
				praster_markdirty(this, x0, y2);
				praster_markdirty(this, x0+width-1, y2);
				praster_markdirty(this, x0, y2+Machine->uifont->height-1);
				praster_markdirty(this, x0+width-1, y2+Machine->uifont->height-1);
			}
			y2 += Machine->uifont->height;
		}
	}
}

static void praster_init (PRASTER *this)
{
	if (praster.draw_character==NULL) {
		if (Machine->color_depth == 8)
			{
				praster.draw_character=praster_draw_character8;
				praster.draw_bytecode=praster_draw_bytecode8;
				praster.draw_pixel=praster_draw_pixel8;
			}
		else
			{
				praster.draw_character=praster_draw_character16;
				praster.draw_bytecode=praster_draw_bytecode16;
				praster.draw_pixel=praster_draw_pixel16;
			}
	}
	this->on=1;
}

static void praster_update(PRASTER *this)
{
	memset(raster2.text.dirtybuffer, 1,
		   raster2.text.size.x*raster2.text.size.y);
}

static void praster_cursor_update(PRASTER *this)
{
	if (this->cursor.pos<this->text.size.x*this->text.size.y)
		this->text.dirtybuffer[this->cursor.pos]=1;
}

static int praster_raster_interrupt (PRASTER *this)
{
	this->raytube.current.y++;
	if (this->raytube.current.y>=this->raytube.size.y) {
		this->raytube.current.y=0;
		if (this->cursor.on && this->cursor.blinking) {
			this->cursor.counter++;
			if (this->cursor.counter>=this->cursor.delay) {
				this->cursor.counter=0;
				this->cursor.displayed=!this->cursor.displayed;
				this->text.dirtybuffer[this->cursor.pos]=1;
			}
		}
	}
	return 0;
}

static void praster_raster_monotext (PRASTER *this)
{
	int i, j, k, sy, sx, ty, tx, code;

	for (i = 0, ty=0, sy = this->raytube.screenpos.y;
		 (ty<this->text.size.y)&&(sy<Machine->visible_area.max_y);
		 ty++, sy+=this->text.charsize.y, i+=this->linediff+this->text.size.x) {
		for (sx = this->raytube.screenpos.x, tx=0, j=i;
			 (tx<this->text.size.x)&&(sx<Machine->visible_area.max_x);
			 tx++, sx+=this->text.charsize.x, j++ ) {
			if (this->text.dirtybuffer[j]) {
				code=this->memory.ram[(this->memory.videoram.offset+j)
									 &this->memory.videoram.mask];
				praster.draw_character
					(this->display.bitmap,0, this->text.visiblesize.y-1,
					 this->memory.ram+((this->memory.fontram.offset+
										code*this->text.fontsize.y)
									   & this->memory.fontram.mask),
					 sy, sx, this->monocolor);
				if (this->cursor.on&&(this->cursor.pos==j)
					&&(!this->cursor.blinking||this->cursor.displayed) ) {
					for (k=this->cursor.ybegin;
						 (k<this->text.charsize.y)&&(k<=this->cursor.yend); k++) {
						praster.draw_bytecode(this->display.bitmap, 0xff, sy+k, sx,
											  this->monocolor);
					}
				}
				/*osd_mark_dirty (sx, sy, sx+7, sy+7, 0); */
				this->text.dirtybuffer[j]=0;
			}
		}
	}
}

static void praster_raster_text (PRASTER *this)
{
	int i, j, k, sy, sx, tx, ty, code, attr;
	UINT16 color[2];

	for (i = 0, ty=0, sy = this->raytube.screenpos.y;
		 (ty<this->text.size.y)&&(sy<Machine->visible_area.max_y);
		 ty++, sy+=this->text.charsize.y, i+=this->linediff+this->text.size.x) {
		for (sx = this->raytube.screenpos.x, tx=0, j=i;
			 (tx<this->text.size.x)&&(sx<Machine->visible_area.max_x);
			 tx++, sx+=this->text.charsize.x, j++ ) {
			if (1||this->text.dirtybuffer[j]) {
				code=this->memory.ram[(this->memory.videoram.offset+j)
									 &this->memory.videoram.mask];
				attr=this->memory.ram[(this->memory.colorram.offset+j)
									 &this->memory.colorram.mask];
				color[0]=this->display.pens[(attr>>4)&7];
				color[1]=this->display.pens[attr&0x0f];
				if (attr&0x80) code|=0x100;
				praster.draw_character
					(this->display.bitmap,0, this->text.visiblesize.y-1,
					 this->memory.ram+((this->memory.fontram.offset+
										code*this->text.fontsize.y)
									   & this->memory.fontram.mask),
					 sy, sx, color);
				if (this->cursor.on&&(this->cursor.pos==j)
					&&(!this->cursor.blinking||this->cursor.displayed) ) {
					for (k=this->cursor.ybegin;
						 (k<this->text.charsize.y)&&(k<=this->cursor.yend); k++) {
						praster.draw_bytecode(this->display.bitmap, 0xff, sy+k, sx,
											  color);
					}
				}
				/*osd_mark_dirty (sx, sy, sx+7, sy+7, 0); */
				this->text.dirtybuffer[j]=0;
			}
		}
	}
}

static void praster_raster_gfxtext (PRASTER *this)
{
	int i, j, k, l, sy, sx, tx, ty, code;

	for (i = 0, ty=0, sy = this->raytube.screenpos.y;
		 (ty<this->text.size.y)&&(sy<Machine->visible_area.max_y);
		 ty++, sy+=this->text.charsize.y, i+=this->linediff+this->text.size.x) {
		for (sx = this->raytube.screenpos.x, tx=0, j=i;
			 (tx<this->text.size.x)&&(sx<Machine->visible_area.max_x);
			 tx++, sx+=this->text.charsize.x, j++ ) {
			if (this->text.dirtybuffer[j]) {
				code=this->memory.ram[(this->memory.videoram.offset+j)
									 &this->memory.videoram.mask]
					+this->memory.fontram.offset/this->text.fontsize.y;

				drawgfx (this->display.bitmap, Machine->gfx[0], code,
						 0, 0, 0, sx, sy, 0, TRANSPARENCY_NONE, 0);

#if 0
				praster.draw_character
					(this->display.bitmap,0, this->text.visiblesize.y-1,
					 this->memory.ram+((this->memory.fontram.offset+
										code*this->text.fontsize.y)
									   & this->memory.fontram.mask),
					 sy, sx, this->display.pens);
#endif
				if (this->cursor.on&&(this->cursor.pos==j)
					&&(!this->cursor.blinking||this->cursor.displayed) ) {
					for (k=this->cursor.ybegin;
						 (k<this->text.charsize.y)&&(k<=this->cursor.yend); k++) {
						praster.draw_bytecode(this->display.bitmap, 0xff, sy+k, sx,
											  this->display.pens);
					}
				}
				if (this->text.charsize.x>this->text.visiblesize.x) {
					for (k=0;k<this->text.charsize.y;k++) {
						for (l=this->text.visiblesize.x;
							 l<this->text.visiblesize.x;l++) {
							praster.draw_pixel(this->display.bitmap, sy+k,
											  sx+l, this->display.pens[0]);
						}
					}
				}
				if (this->text.charsize.y>this->text.visiblesize.y) {
					for (k=this->text.visiblesize.y;
						 k<this->text.charsize.y;k++) {
						for (l=0;
							 l<this->text.visiblesize.x;l++) {
							praster.draw_pixel(this->display.bitmap, sy+k,
											  sx+l, this->display.pens[0]);
						}
					}
				}
				osd_mark_dirty (sx, sy,
								sx+this->text.charsize.x-1,
								sy+this->text.charsize.y-1, 0);
				this->text.dirtybuffer[j]=0;
			}
		}
	}
}

static void praster_raster_graphic (PRASTER *this)
{
	int i, sy, sx, tx, ty, code;

	for (i = 0, ty = 0, sy=this->raytube.screenpos.y;
		 (ty<this->text.size.y)&&(sy<Machine->visible_area.max_y);
		 sy++, ty++, i+=this->linediff ) {
		for (sx = this->raytube.screenpos.x, tx = 0;
			 (tx<this->text.size.x)&&(sx<Machine->visible_area.max_x);
			 tx+=8, sx+=8) {
			if (this->text.dirtybuffer[i]) {
				code=this->memory.ram[(this->memory.videoram.offset+i)
									 &this->memory.videoram.mask];
				praster.draw_bytecode (this->display.bitmap,
									   code, sy, sx, this->monocolor);
				this->text.dirtybuffer[i]=0;
			}
		}
	}
}

int praster_vh_start (void)
{
	if ((raster2.text.dirtybuffer=malloc(0x10000))==0) return 1;
	raster1.display.pens=raster2.display.pens=Machine->pens;
	raster2.display.bitmap=Machine->scrbitmap;
    return 0;
}

void praster_vh_stop (void)
{
}

int praster_vretrace_irq (void)
{
	return 0;
}


int praster_raster_irq (void)
{
	if (raster1.on) {
		praster_raster_interrupt(&raster1);
	}
	if (raster2.on) {
		praster_raster_interrupt(&raster2);
	}
	return 0;
}

void praster_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	struct rectangle vis;
	if (raster2.display.no_rastering) return;

	if( palette_recalc() )
		full_refresh = 1;

	if( full_refresh ) {
		memset(raster2.text.dirtybuffer, 1,
			   raster2.text.size.x*raster2.text.size.y);
    }

	switch (raster2.mode) {
	case PRASTER_MONOTEXT: praster_raster_monotext(&raster2);break;
	case PRASTER_TEXT: praster_raster_text(&raster2);break;
	case PRASTER_GFXTEXT: praster_raster_gfxtext(&raster2);break;
	case PRASTER_GRAPHIC: praster_raster_graphic(&raster2);break;
	}
	if (raster2.raytube.screenpos.x>0) {
		vis.min_x=0;
		vis.max_x=raster2.raytube.screenpos.x-1;
		vis.min_y=0;
		vis.max_y=Machine->visible_area.max_y;
		fillbitmap(raster2.display.bitmap, raster2.raytube.framecolor,&vis);
	}
	if (raster2.raytube.screenpos.x+raster2.text.size.x*raster2.text.charsize.x<
		Machine->visible_area.max_x) {
		vis.min_x=raster2.raytube.screenpos.x
			+raster2.text.size.x*raster2.text.charsize.x;
		vis.max_x=Machine->visible_area.max_x;
		vis.min_y=0;
		vis.max_y=Machine->visible_area.max_y;
		fillbitmap(raster2.display.bitmap, raster2.raytube.framecolor,&vis);
	}
	if (raster2.raytube.screenpos.y>0) {
		vis.min_y=0;
		vis.max_y=raster2.raytube.screenpos.y-1;
		vis.min_x=0;
		vis.max_x=Machine->visible_area.max_x;
		fillbitmap(raster2.display.bitmap, raster2.raytube.framecolor,&vis);
	}
	if (raster2.raytube.screenpos.y+raster2.text.size.y*raster2.text.charsize.y<
		Machine->visible_area.max_y) {
		vis.min_y=raster2.raytube.screenpos.y
			+raster2.text.size.y*raster2.text.charsize.y;
		vis.max_y=Machine->visible_area.max_y;
		vis.min_x=0;
		vis.max_x=Machine->visible_area.max_x;
		fillbitmap(raster2.display.bitmap, raster2.raytube.framecolor,&vis);
	}
	if (raster2.display_state) raster2.display_state(&raster2);
}

extern void praster_1_init (void) { praster_init(&raster1); }
extern void praster_2_init (void) { praster_init(&raster2); }

extern WRITE_HANDLER ( praster_1_videoram_w )
{ praster_videoram_w(&raster1, offset, data); }
extern WRITE_HANDLER ( praster_2_videoram_w )
{ praster_videoram_w(&raster2, offset, data); }

extern READ_HANDLER ( praster_1_videoram_r )
{ return praster_videoram_r(&raster1, offset); }
extern READ_HANDLER ( praster_2_videoram_r )
{ return praster_videoram_r(&raster2, offset); }

extern void praster_1_update(void)
{ praster_update(&raster1); }
extern void praster_2_update(void)
{ praster_update(&raster2); }
extern void praster_1_cursor_update(void)
{ praster_cursor_update(&raster1); }
extern void praster_2_cursor_update(void)
{ praster_cursor_update(&raster2); }
