#include <stdio.h>
#include "driver.h"

int vectrex_load_rom (void);
int vectrex_id_rom (const char *name, const char *gamename);

int vectrex_load_rom (void)
{
	FILE *cartfile;
	unsigned char *vectrex_cartridge_rom;
	
	cartfile = NULL;

	/* If no cartridge is given, the built in game is started (minestorm) */
	if (strlen(rom_name[0])==0)
	{
		if (errorlog) fprintf(errorlog,"Vectrex: no cartridge specified!\n");
	}
	else if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_ROM_CART, 0)))
	{
		if (errorlog) fprintf(errorlog,"Vectrex - Unable to locate cartridge: %s\n",rom_name[0]);
		return 1;
	}

	vectrex_cartridge_rom = &(ROM[0x0000]);

	if (cartfile!=NULL)
	{
		osd_fread (cartfile, vectrex_cartridge_rom, 0x8000);
		osd_fclose (cartfile);
	}

	return 0;
}

int vectrex_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
	char magic[7];
	int retval;
	
	/* If no file was specified, don't bother */
	if (strlen(gamename)==0) return 1;
	
	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_ROM_CART, 0))) return 0;

	retval = 0;

	/* Verify the file is accepted by the Vectrex bios */
	osd_fread (romfile, magic, 5);
	magic[5] = 0;
	if (!strcmp(magic,"g GCE"))
		retval = 1;

	osd_fclose (romfile);

	return retval;
}
