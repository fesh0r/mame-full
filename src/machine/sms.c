#include <stdio.h>
#include "driver.h"
#include "vidhrdw/sms.h"

#define SMS_ROM_MAXSIZE 256*0x4000      /* We support up to 256 16kb ROM pages (4096k ROMs) */

unsigned char *sms_user_ram;            /* RAM at $c000-$dfff, mirrored at $e000-$ffff */

unsigned int JoyState;                            /* State of joystick buttons        */
unsigned char sms_country;              /* 0 = Auto, 1 = US/World, 2 = Japanese */

static unsigned char *sms_banked_rom;
static unsigned char sms_rom_mask;

static unsigned char sms_battery;       /* set to 1 if cart has a battery */

int sms_load_rom (void)
{
	FILE *cartfile;
	int region;
	int size;
	int rom_settings;
		

	cartfile = NULL;

	/* If no cartridge is given, bail */
	if (strlen(rom_name[0])==0)
	{
		if (errorlog) fprintf(errorlog,"SMS: no cartridge specified!\n");
		return 1;
	}
	else if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_ROM_CART, 0)))
	{
		if (errorlog) fprintf(errorlog,"SMS - Unable to locate cartridge: %s\n",rom_name[0]);
		return 1;
	}

	/* Allocate memory and set up memory regions */
	for (region = 0;region < MAX_MEMORY_REGIONS;region++)
		Machine->memory_region[region] = 0;

	ROM = malloc (0x10000);
	if (!ROM)
	{
		if (errorlog) fprintf(errorlog,"SMS - RAM allocation failed on cartridge load!\n");
		return 1;
	}
	
	Machine->memory_region[0] = ROM;

	/* Allocate memory for largest possible ROM size (4096k) */
	sms_banked_rom = malloc (SMS_ROM_MAXSIZE);
	if (!sms_banked_rom)
	{
		if (errorlog) fprintf(errorlog,"SMS - ROM allocation failed on cartridge load!\n");
		return 1;
	}
	
	/* Read in the ROM data and get the total size */
	size = osd_fread (cartfile, sms_banked_rom, SMS_ROM_MAXSIZE);
	osd_fclose (cartfile);

	if (size <= 0)
	{
		free (sms_banked_rom);
		if (errorlog) fprintf(errorlog,"SMS - ROM size was bogus!\n");
		return 1;
	}
	
	/* Shrink the ROM pointer down to the proper size */
	sms_banked_rom = realloc (sms_banked_rom, size);

	rom_settings = sms_banked_rom[0x7fff];
		
	/* Auto-set the country code if needed */
	if (!sms_country)
	{
		int country = (rom_settings & 0xf0) >> 4;
		
		if (errorlog)
			fprintf (errorlog, "ROM Settings:\n\tCountry: %02x\n\tBank Size: %02x\n", country, rom_settings & 0x0f);
		
		if (country == 5)
			/* Japanese */
			sms_country = 2; 
		else if (country == 7)
			/* US */
			sms_country = 1;
	}

	/* Set the mask for ROM bank selections */
	sms_rom_mask = (size/0x4000) - 1;

	return 0;
}

void sms_init_machine (void)
{
	/* Set the default country setting to Auto */
	sms_country = 0; /* TODO: move this to a fake dipswitch */
	sms_battery = 0;
	
	sms_user_ram[0x0000] = sms_user_ram[0x1ffc] = 0x00;
	
	sms_user_ram[0x1ffd] = 0;
	sms_user_ram[0x1ffe] = 1;
	sms_user_ram[0x1fff] = 2 & sms_rom_mask;
	cpu_setbank (1, &sms_banked_rom[0x4000 * sms_user_ram[0x1ffd]]);
	cpu_setbank (2, &sms_banked_rom[0x4000 * sms_user_ram[0x1ffe]]);
	cpu_setbank (3, &sms_banked_rom[0x4000 * sms_user_ram[0x1fff]]);

	JoyState=0xFFFF;
}

int sms_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
	char magic[9];
	unsigned char extra;
	int retval;

	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_ROM_CART, 0))) return 0;

	retval = 0;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	osd_fseek (romfile, 0x7ff0, SEEK_SET);
	osd_fread (romfile, magic, 8);
	magic[8] = 0;
	if (!strcmp(magic,"TMR SEGA"))
	{
		/* TODO: Is this right? If it's $50 or greater, it is SMS */
		osd_fseek (romfile, 0x7ffd, SEEK_SET);
		osd_fread (romfile, &extra, 1);
		if (extra >= 0x50)
			retval = 1;
	}
	/* Check at $81f0 also */
	if (!retval)
	{
		osd_fseek (romfile, 0x81f0, SEEK_SET);
		osd_fread (romfile, magic, 8);
		magic[8] = 0;
		if (!strcmp(magic,"TMR SEGA"))
		{
			/* TODO: Is this right? If it's $50 or greater, it is SMS */
			osd_fseek (romfile, 0x81fd, SEEK_SET);
			osd_fread (romfile, &extra, 1);
			if (extra >= 0x50)
				retval = 1;
		}
	}

	osd_fclose (romfile);
	
	return retval;
}

int gamegear_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
	char magic[9];
	unsigned char extra;
	int retval;

	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_ROM_CART, 0))) return 0;

	retval = 0;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	osd_fseek (romfile, 0x7ff0, SEEK_SET);
	osd_fread (romfile, magic, 8);
	magic[8] = 0;
	if (!strcmp(magic,"TMR SEGA"))
	{
		/* TODO: Is this right? If it's less than 0x50, it is a GameGear image */
		osd_fseek (romfile, 0x7ffd, SEEK_SET);
		osd_fread (romfile, &extra, 1);
		if (extra < 0x50)
			retval = 1;
	}
	/* Check at $81f0 also */
	if (!retval)
	{
		osd_fseek (romfile, 0x81f0, SEEK_SET);
		osd_fread (romfile, magic, 8);
		magic[8] = 0;
		if (!strcmp(magic,"TMR SEGA"))
		{
			/* TODO: Is this right? If it's less than 0x50, it is a GameGear image */
			osd_fseek (romfile, 0x81fd, SEEK_SET);
			osd_fread (romfile, &extra, 1);
			if (extra < 0x50)
				retval = 1;
		}
	}

	osd_fclose (romfile);
	
	return retval;
}

int sms_ram_r (int offset)
{
	return (sms_user_ram[offset]);
}

void sms_ram_w (int offset, int data)
{
    sms_user_ram[offset] = data;
}

void sms_mapper_w (int offset, int data)
{
	static unsigned char sms_ram_enable;  /* is RAM or ROM mapped into $8000-$bfff? */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

//	if (errorlog) fprintf (errorlog, "sms_mapper_w, offset: %02x, data: %02x\n", offset & 0x03, data);
	
	switch (offset & 0x03)
	{
		case 0:
			/* Battery & ROM bank 3 enable */
			sms_ram_enable = data & 0x08;
			
			/* TODO: bit 0x80 may be a 'reset' bit */
			if (sms_ram_enable)
			{
				/* RAM mapped to $8000-$bfff */
				sms_battery = 1;
				/* pick the proper 16k RAM bank */
				if (data & 0x04)
				{
					/* high bank */
					cpu_setbank (3, &RAM[0x8000]);
				}
				else
				{
					/* low bank */
					cpu_setbank (3, &RAM[0x4000]);
				}
			}
			else
			{
				/* ROM is now mapped to $8000-$bfff, switch to the last chosen ROM bank (if it exists) */
				if (sms_user_ram[0x1fff] > sms_rom_mask)
				{
					cpu_setbank (3, &RAM[0x8000]);
				}
				else
				{
					cpu_setbank (3, &sms_banked_rom[0x4000 * sms_user_ram[0x1fff]]);
				}
			}
			break;
		case 1: 
		case 2:
			/* ROM banks 1 & 2 */
			data &= sms_rom_mask;
			cpu_setbank ((offset & 0x03), &sms_banked_rom[0x4000 * data]);
			break;
		case 3:
			/* ROM bank 3, only if it's enabled */
			data &= sms_rom_mask;
			if (!sms_ram_enable) 
				cpu_setbank(3, &sms_banked_rom[0x4000 * data]);
			break;
	}
}

int sms_country_r (int offset)
{
	if (errorlog) fprintf (errorlog, "* sms_country_r\n");
	return (((JoyState>>6)&0x80) | (sms_country>1? 0x00:0x40));
}

void sms_country_w(int offset, int data)
{
	if (errorlog) fprintf (errorlog, "* sms_country_w, data: %02x\n", data);

	switch (data & 0x0f)
	{
		case 5: /* Set localization */
			JoyState &= 0x3fff;
		
			if (sms_country > 1)
				data = ~data;

			JoyState |= (data & 0x80) << 8;
			JoyState |= (data & 0x20) << 9;
			break;
		default:
			JoyState |= 0xc000;
			break;
	}
}


