/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

int dragon_cart_inserted;
UINT8 *dragon_tape;

int dragon_rom_load(void)
{
	void *fp;
	int tapesize;
	
	fp = NULL;
	dragon_cart_inserted = 0;
	dragon_tape = NULL;

	if (strlen(rom_name[0])==0)
	{
		if (errorlog) fprintf(errorlog,"No image specified!\n");
	}
	else if (!(fp = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_ROM_CART, 0)))
	{
		if (errorlog) fprintf(errorlog,"Unable to locate image: %s\n",rom_name[0]);
		return 1;
	}

	if (fp!=NULL)
	{
		tapesize = osd_fsize(fp);
		if ((dragon_tape = (UINT8 *)malloc(tapesize)) == NULL)
		{
			if (errorlog) fprintf(errorlog,"Not enough memory.\n");
			return 1;
		}
		osd_fread (fp, dragon_tape, tapesize);
	}
	
	return 0;
}

void dragon_init_machine(void)
{
	if (dragon_cart_inserted)
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
}

int dragon_mapped_irq_r(int offset)
{
	return ROM[0xbff0 + offset];
}
