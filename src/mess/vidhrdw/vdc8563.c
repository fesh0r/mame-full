/***************************************************************************

  CBM Video Device Chip 8563

***************************************************************************/
#include <math.h>
#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/c128.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"

#include "vdc8563.h"

/* seems to be a motorola m6845 variant */

unsigned char vdc8563_palette[] =
{
/* ripped from vice, a very excellent emulator */
/* black, white, red, cyan */
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0,
/* purple, green, blue, yellow */
	0x60, 0x00, 0x60, 0x00, 0xa0, 0x00, 0x00, 0x00, 0xf0, 0xd0, 0xd0, 0x00,
/* orange, light orange, pink, light cyan, */
	0xc0, 0xa0, 0x00, 0xff, 0xa0, 0x00, 0xf0, 0x80, 0x80, 0x00, 0xff, 0xff,
/* light violett, light green, light blue, light yellow */
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xa0, 0xff, 0xff, 0xff, 0x00
};

UINT8 vdc8563[37], vdcindex;

#define COLUMNS (vdc8563[0]+1)
#define COLUMNS_VISIBLE (vdc8563[1])
#define COLUMNS_SYNC_POS (vdc8563[2])
#define COLUMNS_SYNC_SIZE ((vdc8563[3]&0xf)-1)
#define LINES_SYNC_SIZE (vdc8563[3]>>4)
#define LINES (vdc8563[4]*CHARHEIGHT+vdc8563[5]&0x1f)
#define CHARLINES (vdc8563[6]&0x1f)
#define LINES_SYNC_POS (vdc8563[7])
#define INTERLACE (vdc8563[8]&1)
#define LINES400 ((vdc8563[8]&3)==3)
#define CHARHEIGHT (vdc8563[9]%0x1f)
#define CURSORON ((vdc8563[10]&0x60)!=0x20)
#define CURSOR16 ((vdc8563[10]&0x60)==0x40)
#define CURSOR32 ((vdc8563[10]&0x60)==0x60)
#define CURSOR_STARTLINE (vdc8563[11]&0x1f)
#define CURSOR_ENDLINE (vdc8563[12]&0x1f)

void vdc8563_port_w (int offset, int data)
{
	if (offset & 1)
	{
		if ((vdcindex & 0x3f) < 37)
		{
			switch (vdcindex & 0x3f)
			{
			case 0:
				break;
			case 1:
				break;
			}
		}
		DBG_LOG (1, "vdc8563_port_w", (errorlog, "%.2x:%.2x\n", vdcindex, data));
	}
	else
	{
		vdcindex = data;
	}
}

int vdc8563_port_r (int offset)
{
	int val;

	val = 0xff;
	if (offset & 1)
	{
		if ((vdcindex & 0x3f) < 37)
		{
			switch (vdcindex & 0x3f)
			{
			case 0:
				break;
			}
		}
		DBG_LOG (1, "vdc8563_port_r", (errorlog, "%.2x:%.2x\n", vdcindex, val));
	}
	else
	{
		val = vdcindex | 0x80;
	}
	return val;
}
