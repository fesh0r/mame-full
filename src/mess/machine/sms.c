#include <stdio.h>
#include "driver.h"

#define SMS_ROM_MAXSIZE     (256 * 16384 + 512)

unsigned char *sms_user_ram;
unsigned int JoyState;
unsigned char sms_country;
static unsigned char *sms_banked_rom;
static unsigned char sms_rom_mask;
static unsigned char sms_battery;

static unsigned char *ROM;

int sms_load_rom(int id, char *rom_name)
{
    FILE *fp;
	int size;

	if(rom_name==NULL){
		printf("Cartridge Name Required!\n");
		return INIT_FAILED;
	}

	if(strlen(rom_name) == 0) return 1;
	fp = osd_fopen(Machine->gamedrv->name, rom_name, OSD_FILETYPE_IMAGE_R, 0);
    if(!fp)
    {
		printf("Cartridge Name Required!\n");
		return INIT_FAILED;
	}
	if( new_memory_region(REGION_CPU1,0x10000) )
		return 1;
	ROM = memory_region(REGION_CPU1);
	if( new_memory_region(REGION_CPU2,SMS_ROM_MAXSIZE) )
		return 1;
	sms_banked_rom = memory_region(REGION_CPU2);
    size = osd_fread(fp, sms_banked_rom, SMS_ROM_MAXSIZE);

    if(!size)
    {
        free(sms_banked_rom);
        return 1;
    }

    /* handle 512-byte header if present */
    if((size/512) & 1)
    {
        size -= 512;
        osd_fseek(fp, 0, SEEK_SET);
        osd_fread(fp, sms_banked_rom, size);
    }

    sms_banked_rom = realloc(sms_banked_rom, size); /* shrink memory */
    osd_fclose(fp);

    sms_rom_mask = (size >> 14)-1;
    sms_country = 0;

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

	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_IMAGE_R, 0))) return 0;

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

	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_IMAGE_R, 0))) return 0;

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


/* todo: stick cart ram handling back in and fix it */
void sms_mapper_w(int offset, int data)
{
    /* bank reg. writes go to RAM as well */
    sms_user_ram[0x1FFC + (offset & 3)] = data;

    switch(offset & 3)
    {
        case 0x00:  /* bank config */
             break;
        case 0x01:  /* page for bank #1 */
        case 0x02:  /* page for bank #2 */
        case 0x03:  /* page for bank #3 */
             data &= sms_rom_mask;
             cpu_setbank ((offset & 0x03), &sms_banked_rom[data<<14]);
             break;
    }
}


int sms_country_r (int offset)
{
	return (((JoyState>>6)&0x80) | (sms_country>1? 0x00:0x40));
}

void sms_country_w(int offset, int data)
{

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


