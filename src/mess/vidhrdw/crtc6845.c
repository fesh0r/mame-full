/***************************************************************************

  motorola cathode ray tube controller 6845

  praster version

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 1
#include "mess/machine/cbm.h"

#include "praster.h"
#include "crtc6845.h"

static struct {
	UINT8 reg[18];
	UINT8 index;
	int cursor_on; /* cursor output used */
} crtc;

void crtc6845_init (UINT8 *memory)
{
	memset(&crtc, 0, sizeof(crtc));
	crtc.cursor_on=1;

	praster_2_init();
	raster2.memory.ram=memory;
	raster2.raytube.screenpos.x=raster2.raytube.screenpos.y=0;
	raster2.mode=PRASTER_GFXTEXT;

	raster2.text.charsize.x=raster2.text.charsize.y=8;
	raster2.text.visiblesize.x=raster2.text.visiblesize.y=8;
	raster2.linediff=0;raster2.text.size.x=80;
	raster2.text.size.y=25;
	raster2.cursor.on=0;
	raster2.cursor.pos=0;
	raster2.cursor.blinking=1;
	raster2.cursor.delay=16;
	raster2.cursor.ybegin=0;raster2.cursor.yend=7;

	raster2.memory.mask=raster2.memory.videoram.mask=0xffff;
}

void crtc6845_pet_init (UINT8 *memory)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i=0; i<0x400; i++) {
		gfx[0x800+i]=gfx[0x400+i];
		gfx[0xc00+i]=gfx[0x800+i]^0xff;
		gfx[0x400+i]=gfx[i]^0xff;
	}

	crtc6845_init(memory);
	crtc.cursor_on=0;
	raster2.text.fontsize.y=8;
	raster2.memory.mask=raster2.memory.videoram.mask=0x7ff;
	raster2.text.charsize.x=8;
	raster2.text.visiblesize.x=8;
}

void crtc6845_superpet_init (UINT8 *memory)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i=0; i<0x400; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0xc00+i];
		gfx[0x1c00+i]=gfx[0x1800+i]^0xff;
		gfx[0x1400+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[0x400+i];
		gfx[0xc00+i]=gfx[0x800+i]^0xff;
		gfx[0x400+i]=gfx[i]^0xff;
	}

	crtc6845_init(memory);
	crtc.cursor_on=0;
	raster2.text.fontsize.y=8;
	raster2.memory.mask=raster2.memory.videoram.mask=0x7ff;
	raster2.text.charsize.x=8;
	raster2.text.visiblesize.x=8;
}

void crtc6845_cbm600_init(UINT8 *memory)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}

	crtc6845_init(memory);
	raster2.text.fontsize.y=16;
	raster2.text.charsize.x=8;
	raster2.text.visiblesize.x=8;
}

void crtc6845_cbm600pal_init(UINT8 *memory)
{
	/* no hardware reverse logic, instead double size charrom,
	   and switching between hungarian and ascii charset */
	crtc6845_init(memory);
	raster2.text.fontsize.y=16;
	raster2.text.charsize.x=8;
	raster2.text.visiblesize.x=8;
}

void crtc6845_cbm700_init(UINT8 *memory)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i=0; i<0x800; i++) {
		gfx[0x1000+i]=gfx[0x800+i];
		gfx[0x1800+i]=gfx[0x1000+i]^0xff;
		gfx[0x800+i]=gfx[i]^0xff;
	}

	crtc6845_init(memory);
	raster2.text.fontsize.y=16;
	raster2.text.charsize.x=9;
	raster2.text.charsize.y=14;
	raster2.text.visiblesize.x=8;
	raster2.text.visiblesize.y=14;
}

void crtc6845_set_rastering(int on)
{
	raster2.display.no_rastering=!on;
/*	raster2.text.visiblesize.x=raster2.text.visiblesize.y=8; */
}

#define COLUMNS (crtc.reg[0]+1)
#define COLUMNS_VISIBLE (crtc.reg[1])
#define COLUMNS_SYNC_POS (crtc.reg[2])
#define COLUMNS_SYNC_SIZE ((crtc.reg[3]&0xf)-1)
#define LINES_SYNC_SIZE (crtc.reg[3]>>4)
#define LINES (crtc.reg[4]*CHARHEIGHT+crtc.reg[5]&0x1f)
#define CHARLINES (crtc.reg[6]&0x1f)
#define LINES_SYNC_POS (crtc.reg[7])
#define INTERLACE (crtc.reg[8]&1)
#define LINES400 ((crtc.reg[8]&3)==3)
#define CHARHEIGHT (crtc.reg[9]%0x1f)


WRITE_HANDLER ( crtc6845_port_w )
{
	if (offset & 1)
	{
		if ((crtc.index & 0x1f) < 18)
		{
			switch (crtc.index & 0x1f)
			{
			case 1:
				crtc.reg[crtc.index]=data;
				raster2.text.size.x=data;
				raster2.memory.videoram.size=
					raster2.text.size.y*data;
				praster_2_update();
				break;
			case 6:
				crtc.reg[crtc.index]=data;
				raster2.text.size.y=data;
				raster2.memory.videoram.size=
					raster2.text.size.x*data;
				praster_2_update();
				break;
			case 9:
				crtc.reg[crtc.index]=data;
				raster2.text.charsize.y=(data&0x1f)+1;
				praster_2_update();
				break;
			case 0xa:
				crtc.reg[crtc.index]=data;
				raster2.cursor.on=((data&0x60)!=0x20)&&crtc.cursor_on;
				raster2.cursor.blinking=(data&0x60)!=0;
				if ((data&0x60)==0x60) raster2.cursor.delay=32;
				else raster2.cursor.delay=16;
				raster2.cursor.ybegin=data&0x1f;
				praster_2_cursor_update();
				break;
			case 0xb:
				crtc.reg[crtc.index]=data;
				raster2.cursor.yend=data&0x1f;
				praster_2_cursor_update();
				break;
			case 0xc: case 0xd:
				crtc.reg[crtc.index]=data;
				raster2.memory.videoram.offset=
					( (crtc.reg[0xc]<<8)|crtc.reg[0xd])
					&raster2.memory.videoram.mask;
				praster_2_update();
				break;
			case 0xe: case 0xf:
				crtc.reg[crtc.index]=data;
				raster2.cursor.pos=((crtc.reg[0xe]&0xf)<<8)|crtc.reg[0xf];
				praster_2_cursor_update();
				break;
			default:
				crtc.reg[crtc.index]=data;
				DBG_LOG (2, "crtc_port_w",
						 ("%.2x:%.2x\n", crtc.index, data));
				break;
			}
		}
		DBG_LOG (1, "crtc6845_port_w",
				 ("%.2x:%.2x\n", crtc.index, data));
	}
	else
	{
		crtc.index = data;
	}
}

/* internal flipflop for doubling horizontal
   value */
WRITE_HANDLER ( crtc6845_pet_port_w )
{
	if (offset & 1)
	{
		if ((crtc.index & 0x1f) < 18)
		{
			switch (crtc.index & 0x1f)
			{
			case 1:
				crtc.reg[crtc.index]=data;
				raster2.text.size.x=data*2;
				raster2.memory.videoram.size=
					raster2.text.size.y*data*2;
				praster_2_update();
				break;
			default:
				crtc6845_port_w(offset,data);
				break;
			}
		} else
			crtc6845_port_w(offset,data);
	}
	else
	{
		crtc6845_port_w(offset,data);
	}
}

READ_HANDLER ( crtc6845_port_r )
{
	int val;

	val = 0xff;
	if (offset & 1)
	{
		if ((crtc.index & 0x1f) < 18)
		{
			switch (crtc.index & 0x1f)
			{
			case 0x14: val=crtc.reg[offset]&0x3f;break;
			default:
				val=crtc.reg[offset];
			}
		}
		DBG_LOG (1, "crtc6845_port_r", ("%.2x:%.2x\n", crtc.index, val));
	}
	else
	{
		val = crtc.index;
	}
	return val;
}

void crtc6845_address_line_11(int level)
{
	raster2.memory.fontram.offset=level?0x800:0;
}

void crtc6845_address_line_12(int level)
{
	raster2.memory.fontram.offset=level?0x1000:0;
}

extern void crtc6845_status (char *text, int size)
{
	text[0]=0;
#if VERBOSE_DBG
		snprintf (text, sizeof (text), "crtc6845 %.2x %.2x %.2x %.2x",
				  crtc.reg[0xc], crtc.reg[0xd], crtc.reg[0xe],crtc.reg[0xf]);
#endif
}
