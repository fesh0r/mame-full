/***************************************************************************

  advision.c

  Machine file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"

static UINT8 ram[0x100];

//static UINT8 *ROM;
static UINT8 p1,p2;

void odyssey2_init_machine(void)
{
    cpu_setbank(1, memory_region(REGION_USER1)+3*0x800);
    p1=0;
    p2=0;
    return;
}


int odyssey2_load_rom (int id)
{
    int size;
    FILE *cartfile;
    
    logerror("ODYSSEY2 - Load_rom()\n");
    if(device_filename(IO_CARTSLOT,id) == NULL)
    {
	logerror("%s requires Cartridge!\n", Machine->gamedrv->name);
	return INIT_FAILED;
    }
    
//    ROM = memory_region(REGION_CPU1);
    cartfile = NULL;
    logerror("ODYSSEY2 - Loading Image\n");
    if (!(cartfile = (FILE*)image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
    {
	logerror("ODYSSEY2 - Unable to locate cartridge: %s\n",device_filename(IO_CARTSLOT,id) );
	return 1;
    }
    else
    {
	logerror("ODYSSEY2 - Found cartridge\n");
	
    }
    logerror("ODYSSEY2 - Done\n");
    
    size=osd_fsize(cartfile);
    osd_fread (cartfile, memory_region(REGION_USER1), size);	 /* non banked carts */
    osd_fclose (cartfile);

    if (size<=0x800) 
	memcpy(memory_region(REGION_USER1)+0x800, memory_region(REGION_USER1), 0x800);
    if (size<=0x1000) 
	memcpy(memory_region(REGION_USER1)+0x1000, memory_region(REGION_USER1), 0x1000);

    return 0;
}


/****** External RAM ******************************/

READ_HANDLER ( odyssey2_bus_r )
{
    if ((p1&0x48)==0) return odyssey2_video_r(offset); // seams to have higher priority than ram???
    if (!(p1&0x10)) return ram[offset];
    return 0;
}

WRITE_HANDLER ( odyssey2_bus_w )
{
    if ((p1&0x50)==0x00) ram[offset]=data;
    if (!(p1&8)) odyssey2_video_w(offset, data);
}



/***** 8048 Ports ************************/

READ_HANDLER ( odyssey2_getp1 )
{
    UINT8 data=p1;

    return data;
}


WRITE_HANDLER ( odyssey2_putp1 )
{
    p1=data;
    cpu_setbank(1, memory_region(REGION_USER1)+(((data&3)^3)<<11));
//    logerror("%.6f p1 written %.2x\n", timer_get_time(), data);
}

READ_HANDLER ( odyssey2_getp2 )
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
    return data;
}

WRITE_HANDLER ( odyssey2_putp2 )
{
    p2=data;
    logerror("%.6f p2 written %.2x\n", timer_get_time(), data);
}

READ_HANDLER ( odyssey2_getbus )
{
    UINT8 data=0xff;
    if ((p2&7)==1) data&=readinputport(6);
    if ((p2&7)==0) data&=readinputport(7);
    logerror("%.6f bus read %.2x\n", timer_get_time(), data);
    return data;
}

WRITE_HANDLER ( odyssey2_putbus )
{
}
