/***************************************************************************

  odyssey2.c

  Machine file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"
#include "image.h"
#include "devices/cartslot.h"

static UINT8 ram[0x100];

//static UINT8 *ROM;
static UINT8 p1,p2;

MACHINE_INIT( odyssey2 )
{
    cpu_setbank(1, memory_region(REGION_USER1)+3*0x800);
    p1 = 0;
    p2 = 0;
    return;
}


DEVICE_LOAD( odyssey2_cart )
{
	/* non banked carts */
	return cartslot_load_generic(file, REGION_USER1, 0, 0x0001, 0x2000, CARTLOAD_MUSTBEPOWEROFTWO | CARTLOAD_MIRROR);
}


/****** External RAM ******************************/

 READ8_HANDLER ( odyssey2_bus_r )
{
    if ((p1&0x48)==0) return odyssey2_video_r(offset); // seams to have higher priority than ram???
    if (!(p1&0x10)) return ram[offset];
    return 0;
}

WRITE8_HANDLER ( odyssey2_bus_w )
{
    if ((p1&0x50)==0x00) ram[offset]=data;
    if (!(p1&8)) odyssey2_video_w(offset, data);
}



/***** 8048 Ports ************************/

 READ8_HANDLER ( odyssey2_getp1 )
{
    UINT8 data=p1;
    logerror("%.6f p1 read %.2x\n", timer_get_time(), data);
    return data;
}


WRITE8_HANDLER ( odyssey2_putp1 )
{
    p1=data;
    cpu_setbank(1, memory_region(REGION_USER1)+(((data&3)^3)<<11));
/* 2kbyte eprom are connected a0..a9 to a0..a9
   but a10 of the eprom is connected to a11 of the cpu

   the first 0x400 bytes are internal rom, than comes 0x400 bytes of the eprom
   and the 2nd 0x400 bytes are mapped 2 times
*/
    cpu_setbank(2, memory_region(REGION_USER1)+(((data&3)^3)<<11)+0x400);
    logerror("%.6f p1 written %.2x\n", timer_get_time(), data);
}

 READ8_HANDLER ( odyssey2_getp2 )
{
    UINT8 data=p2;
    UINT8 h=0xff;
    int i, j;
    if (!(p1&4)) {
	if ((p2&7)<=5) h&=readinputport(p2&7);

	for (i=0x80, j=0; i>0; i>>=1, j++) {
	    if (!(h&i)) {
		data&=~0x10;
		data=(data&~0xe0)|(j<<5);
		break;
	    }
	}
    }
    logerror("%.6f p2 read %.2x\n", timer_get_time(), data);
    return data;
}

WRITE8_HANDLER ( odyssey2_putp2 )
{
    p2=data;
    logerror("%.6f p2 written %.2x\n", timer_get_time(), data);
}

 READ8_HANDLER ( odyssey2_getbus )
{
    UINT8 data=0xff;
    if ((p2&7)!=0) data&=readinputport(6);
//    if ((p2&7)==1) data&=readinputport(6);
    if ((p2&7)==0) data&=readinputport(7);
    logerror("%.6f bus read %.2x\n", timer_get_time(), data);
    return data;
}

WRITE8_HANDLER ( odyssey2_putbus )
{
    logerror("%.6f bus written %.2x\n", timer_get_time(), data);
}
