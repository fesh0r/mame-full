/***************************************************************************

  CBM Video Device Chip 8563

  copyright 2000 peter.trauner@jk.uni-linz.ac.at

***************************************************************************/
/*
 several graphic problems
 some are in the rastering engine and should be solved during its evalution
 rare and short documentation,
 registers and some words of description in the c128 user guide */

#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 0
#include "mess/includes/cbm.h"

#include "mess/includes/praster.h"
#include "mess/includes/vdc8563.h"

/* uses raster2, to allow vic6567 use raster1 */

/* seems to be a motorola m6845 variant */

/* permutation for c64/vic6567 conversion to vdc8563
 0 --> 0 black
 1 --> 0xf white
 2 --> 8 red
 3 --> 7 cyan
 4 --> 0xb violett
 5 --> 4 green
 6 --> 2 blue
 7 --> 0xd yellow
 8 --> 0xa orange
 9 --> 0xc brown
 0xa --> 9 light red
 0xb --> 6 dark gray
 0xc --> 1 gray
 0xd --> 5 light green
 0xe --> 3 light blue
 0xf --> 0xf light gray
 */

/* x128
 commodore assignment!?
 black gray orange yellow dardgrey vio red lgreen
 lred lgray brown blue white green cyan lblue
*/
unsigned char vdc8563_palette[] =
{
#if 0
	0x00, 0x00, 0x00, /* black */
	0x70, 0x74, 0x6f, /* gray */
	0x21, 0x1b, 0xae, /* blue */
	0x5f, 0x53, 0xfe, /* light blue */
	0x1f, 0xd2, 0x1e, /* green */
	0x59, 0xfe, 0x59, /* light green */
	0x42, 0x45, 0x40, /* dark gray */
	0x30, 0xe6, 0xc6, /* cyan */
	0xbe, 0x1a, 0x24, /* red */
	0xfe, 0x4a, 0x57, /* light red */
	0xb8, 0x41, 0x04, /* orange */
	0xb4, 0x1a, 0xe2, /* purple */
	0x6a, 0x33, 0x04, /* brown */
	0xdf, 0xf6, 0x0a, /* yellow */
	0xa4, 0xa7, 0xa2, /* light gray */
	0xfd, 0xfe, 0xfc /* white */
#else
	/* vice */
	0,0,0, /* black */
	0x20,0x20,0x20, /* gray */
	0,0,0x80, /* blue */
	0,0,0xff, /* light blue */
	0,0x80,0, /* green */
	0,0xff,0, /* light green */
	0,0x80,0x80, /* cyan */
	0,0xff,0xff, /* light cyan */
	0x80,0,0, /* red */
	0xff,0,0, /* light red */
	0x80,0,0x80, /* purble */
	0xff,0,0xff, /* light purble */
	0x80,0x80,0, /* brown */
	0xff, 0xff, 0, /* yellow */
	0xc0,0xc0,0xc0, /* light gray */
	0xff, 0xff,0xff /* white */
#endif
};


static struct {
	int state;
	UINT8 reg[37];
	UINT8 index;
	int ram16konly;
	UINT16 addr, src;
} vdc;

void vdc8563_init (UINT8 *memory, int ram16konly)
{
	memset(&vdc, 0, sizeof(vdc));
	vdc.state=1;
	vdc.ram16konly=ram16konly;

	praster_2_init();
	raster2.memory.ram=memory;
	raster2.text.fontsize.y=16;
	raster2.raytube.screenpos.x=raster2.raytube.screenpos.y=8;

	raster2.text.charsize.x=raster2.text.charsize.y=8;
	raster2.text.visiblesize.x=raster2.text.visiblesize.y=8;
	raster2.linediff=0;raster2.text.size.x=80;
	raster2.text.size.y=25;
	raster2.cursor.on=1;
	raster2.cursor.pos=0;
	raster2.cursor.blinking=1;
	raster2.cursor.delay=16;
	raster2.cursor.ybegin=0;raster2.cursor.yend=7;

	if (ram16konly) {
		raster2.memory.mask=raster2.memory.videoram.mask=
			raster2.memory.colorram.mask=raster2.memory.fontram.mask=0x3fff;
	} else {
		raster2.memory.mask=raster2.memory.videoram.mask=
			raster2.memory.colorram.mask=raster2.memory.fontram.mask=0xffff;
	}
}

void vdc8563_set_rastering(int on)
{
	raster2.display.no_rastering=!on;
}

int vdc8563_vh_start (void)
{
    int r=praster_vh_start();
	raster2.display.pens=Machine->pens+0x10;
	raster2.display.bitmap=Machine->scrbitmap;
	return r;
}

#define COLUMNS (vdc.reg[0]+1)
#define COLUMNS_VISIBLE (vdc.reg[1])
#define COLUMNS_SYNC_POS (vdc.reg[2])
#define COLUMNS_SYNC_SIZE ((vdc.reg[3]&0xf)-1)
#define LINES_SYNC_SIZE (vdc.reg[3]>>4)
#define LINES (vdc.reg[4]*CHARHEIGHT+vdc.reg[5]&0x1f)
#define CHARLINES (vdc.reg[6]&0x1f)
#define LINES_SYNC_POS (vdc.reg[7])
#define INTERLACE (vdc.reg[8]&1)
#define LINES400 ((vdc.reg[8]&3)==3)
#define CHARHEIGHT (vdc.reg[9]%0x1f)

#define BLOCK_COPY (vdc.reg[0x18]&0x80)

WRITE_HANDLER ( vdc8563_port_w )
{
	UINT8 i;

	if (offset & 1)
	{
		if ((vdc.index & 0x3f) < 37)
		{
			switch (vdc.index & 0x3f)
			{
			case 1:
				vdc.reg[vdc.index]=data;
				raster2.text.size.x=data;
				raster2.memory.videoram.size=
					raster2.memory.colorram.size=
					raster2.text.size.y*data;
				praster_2_update();
				break;
			case 4:
				vdc.reg[vdc.index]=data;
				raster2.text.size.y=data+1;
				raster2.memory.videoram.size=
					raster2.memory.colorram.size=
					raster2.text.size.x*data;
				praster_2_update();
				break;
			case 9:
				vdc.reg[vdc.index]=data;
				raster2.text.charsize.y=(data&0x1f)+1;
				praster_2_update();
				break;
			case 0xa:
				vdc.reg[vdc.index]=data;
				raster2.cursor.on=(data&0x60)!=0x20;
				raster2.cursor.blinking=(data&0x60)!=0;
				if ((data&0x60)==0x60) raster2.cursor.delay=32;
				else raster2.cursor.delay=16;
				raster2.cursor.ybegin=data&0x1f;
				praster_2_cursor_update();
				break;
			case 0xb:
				vdc.reg[vdc.index]=data;
				raster2.cursor.yend=data&0x1f;
				praster_2_cursor_update();
				break;
			case 0xc: case 0xd:
				vdc.reg[vdc.index]=data;
				raster2.memory.videoram.offset=(vdc.reg[0xc]<<8)|vdc.reg[0xd];
				praster_2_update();
				break;
			case 0xe: case 0xf:
				vdc.reg[vdc.index]=data;
				raster2.cursor.pos=(vdc.reg[0xe]<<8)|vdc.reg[0xf];
				praster_2_cursor_update();
				break;
			case 0x12:
				vdc.addr=(vdc.addr&0xff)|(data<<8);
				break;
			case 0x13:
				vdc.addr=(vdc.addr&0xff00)|data;
				break;
			case 0x14: case 0x15:
				vdc.reg[vdc.index]=data;
				raster2.memory.colorram.offset=(vdc.reg[0x14]<<8)|vdc.reg[0x15];
				praster_2_update();
				break;
			case 0x16:
				vdc.reg[vdc.index]=data;
				raster2.text.visiblesize.x=(data&0xf)+1;
				raster2.text.charsize.x=((data&0xf0)>>4)+1;
				praster_2_update();
				break;
			case 0x19:
				vdc.reg[vdc.index]=data;
				if ((data&0xc0)==0) raster2.mode=PRASTER_MONOTEXT;
				else if ((data&0xc0)==0x40) raster2.mode=PRASTER_TEXT;
				else raster2.mode=PRASTER_GRAPHIC;
				praster_2_update();
				break;
			case 0x1a:
				vdc.reg[vdc.index]=data;
				raster2.monocolor[0]=raster2.raytube.framecolor
					=raster2.display.pens[data&0xf];
				raster2.monocolor[1]=raster2.display.pens[data>>4];
				praster_2_update();
				break;
			case 0x1b:
				vdc.reg[vdc.index]=data;
				raster2.linediff=data;
				praster_2_update();
				break;
			case 0x1c:
				vdc.reg[vdc.index]=data;
				raster2.memory.fontram.offset=(data&0xf0)<<8;
				/* 0x10 dram 0:4416, 1: 4164 */
				praster_2_update();
				break;
			case 0x1d:
				vdc.reg[vdc.index]=data;
				/* 0x1f counter for underlining */
				break;
			case 0x1e:
				vdc.reg[vdc.index]=data;
				if (BLOCK_COPY) {
					DBG_LOG (2, "vdc block copy",
							 (errorlog, "src:%.4x dst:%.4x size:%.2x\n",
							  vdc.src, vdc.addr, data));
					i=data;do {
						praster_2_videoram_w(vdc.addr++,
											 praster_2_videoram_r(vdc.src++));
					} while (--i!=0);
				} else {
					DBG_LOG (2, "vdc block set",
							 (errorlog, "dest:%.4x value:%.2x size:%.2x\n",
							  vdc.addr, vdc.reg[0x1f], data));
					i=data;do {
						praster_2_videoram_w(vdc.addr++, vdc.reg[0x1f]);
					} while (--i!=0);
				}
				break;
			case 0x1f:
				DBG_LOG (2, "vdc written",
						 (errorlog, "dest:%.4x size:%.2x\n",
						  vdc.addr, data));
				vdc.reg[vdc.index]=data;
				praster_2_videoram_w(vdc.addr++, data);
				break;
			case 0x20:
				vdc.src=(vdc.src&0xff)|(data<<8);
				break;
			case 0x21:
				vdc.src=(vdc.src&0xff00)|data;
				break;
			case 0x22:
				vdc.reg[vdc.index]=data;
				/* number of chars from start of line to positiv edge of display enable */
				break;
			case 0x23:
				vdc.reg[vdc.index]=data;
				/* number of chars from start of line to negativ edge of display enable */
				break;
			case 0x24:
				vdc.reg[vdc.index]=data;
				/* 0xf number of refresh cycles per line */
				break;
			default:
				vdc.reg[vdc.index]=data;
				DBG_LOG (2, "vdc8563_port_w",
						 (errorlog, "%.2x:%.2x\n", vdc.index, data));
				break;
			}
		}
		DBG_LOG (3, "vdc8563_port_w",
				 (errorlog, "%.2x:%.2x\n", vdc.index, data));
	}
	else
	{
		vdc.index = data;
	}
}

READ_HANDLER ( vdc8563_port_r )
{
	int val;

	val = 0xff;
	if (offset & 1)
	{
		if ((vdc.index & 0x3f) < 37)
		{
			switch (vdc.index & 0x3f)
			{
			case 0x12:
				val=vdc.addr>>8;
				break;
			case 0x13:
				val=vdc.addr&0xff;
				break;
			case 0x1e:
				val=0;
				break;
			case 0x1f:
				val=praster_2_videoram_r(vdc.addr);
				DBG_LOG (2, "vdc read", (errorlog, "%.4x %.2x\n", vdc.addr, val));
				break;
			case 0x20:
				val=vdc.src>>8;
				break;
			case 0x21:
				val=vdc.src&0xff;
				break;
			default:
				val=vdc.reg[offset];
			}
		}
		DBG_LOG (2, "vdc8563_port_r", (errorlog, "%.2x:%.2x\n", vdc.index, val));
	}
	else
	{
		val = vdc.index;
		if (vdc.state) val|=0x80;
	}
	return val;
}

extern void vdc8563_status (char *text, int size)
{
	text[0]=0;
#if VERBOSE_DBG
#if 1
		snprintf (text, sizeof (text), "vdc %.2x", vdc.reg[0x1b]);
#else
		snprintf (text, sizeof (text), "enable:%.2x occured:%.2x",
				  vdc.reg[0x1a], vdc.reg[0x19]);
#endif
#endif
}
