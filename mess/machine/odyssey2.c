/***************************************************************************

  advision.c

  Machine file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"


static UINT8 *ROM;


void odyssey2_init_machine(void) {

	return;
}


int odyssey2_load_rom (int id)
{
    FILE *cartfile;

	if(device_filename(IO_CARTSLOT,id) == NULL)
	{
		printf("%s requires Cartridge!\n", Machine->gamedrv->name);
		return INIT_FAILED;
    }

    ROM = memory_region(REGION_CPU1);
    cartfile = NULL;
	if (!(cartfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("ODYSSEY2 - Unable to locate cartridge: %s\n",device_filename(IO_CARTSLOT,id) );
		return 1;
	}
	osd_fread (cartfile, &ROM[0x0400], 2048);	 /* non banked carts */
    osd_fclose (cartfile);

	return 0;
}


/****** External RAM ******************************/

READ_HANDLER ( odyssey2_MAINRAM_r ) {
    return 0;
}

WRITE_HANDLER ( odyssey2_MAINRAM_w ) {
    return;
}



/***** 8048 Ports ************************/

READ_HANDLER ( odyssey2_getp1 ) {
    int d,in;

    logerror("P1 READ PC=%x\n",cpu_get_pc());
    in = input_port_0_r(0);
    d = in | 0x0F;
    if (in & 0x02) d = d & 0xF7;    /* Button 3 */
    if (in & 0x08) d = d & 0xCF;    /* Button 1 */
    if (in & 0x04) d = d & 0xAF;    /* Button 2 */
    if (in & 0x01) d = d & 0x6F;    /* Button 4 */

    d = d & 0xF8;
    return d;
}


WRITE_HANDLER ( odyssey2_putp1 ) {

	  ROM = memory_region(REGION_CPU1);
}
