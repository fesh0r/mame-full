/***************************************************************************

  a7800.c

  Machine file to handle emulation of the Atari 7800.

  TODO: Supercart Bankswitching

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"

unsigned char *a7800_cart_f000;
unsigned char *a7800_bios_f000;
int a7800_ctrl_lock;
int a7800_ctrl_reg;


int maria_flag;

/* local */
unsigned char *a7800_ram;
unsigned char *a7800_cartridge_rom;
unsigned char a7800_cart_type;
static UINT8 *ROM;

void a7800_init_machine(void) {
	a7800_ctrl_lock = 0;
    a7800_ctrl_reg = 0;
    maria_flag=0;
	if (a7800_cart_type & 0x01){
		install_mem_write_handler(0,0x4000,0x7FFF,pokey1_w);
    }
}

void a7800_stop_machine(void) {
    free(a7800_bios_f000);
    free(a7800_cart_f000);
}

/*    Header format

0      Header version     - 1 byte
1..16  "ATARI7800      "  - 16 bytes
17..48 Cart title         - 32 bytes
49..52 data length        - 4 bytes
53..54 cart type          - 2 bytes
    bit 0 - Pokey cart
    bit 1 - Supercart bank switched
    bit 2 - Supercart RAM
55     controller 1 type  - 1 byte
56     controller 2 type  - 1 byte
    0 = None
    1 = Joystick
    2 = Light Gun

100..127 "ACTUAL CART DATA STARTS HERE" - 28 bytes
*/

int a7800_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
    unsigned char header[128];
    char tag[] = "ATARI7800";

    if (errorlog) fprintf(errorlog,"IDROM\n");
	/* If no file was specified, don't bother */
    if (strlen(gamename)==0) return 1;

	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_IMAGE_R, 0))) return 0;
    osd_fread(romfile,header,128);
	osd_fclose (romfile);

    if (memcmp(&tag[0],&header[1],9) == -1) return 0;
    return 1;
}


int a7800_load_rom (int id, const char *rom_name)
{
    FILE *cartfile;
	long len,start;
    unsigned char header[128];

	if (rom_name==NULL)
	{
		printf("%s requires Cartridge!\n", Machine->gamedrv->name);
		return INIT_FAILED;
	}

	ROM = memory_region(REGION_CPU1);

	a7800_bios_f000 = malloc(0x1000);
    a7800_cart_f000 = malloc(0x1000);

    /* save the BIOS so we can switch it in and out */
    memcpy(a7800_bios_f000,&(ROM[0xF000]),0x1000);

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (strlen(rom_name)==0)
    {
        if (errorlog) fprintf(errorlog,"A7800 - warning: no cartridge specified!\n");
	}
	else if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if (errorlog) fprintf(errorlog,"A7800 - Unable to locate cartridge: %s\n",rom_name);
		return 1;
	}

    osd_fread(cartfile,header,128);

    len = (header[49] << 24) | (header[50] << 16) | (header[51] << 8) | header[52];
    a7800_cart_type = (header[53] << 8) | header[54];
	if (errorlog) fprintf(errorlog,"Cart length: %x\n",(unsigned int)len);
    if  (len < 0xC001) {
        start = 0x10000 - len;
        a7800_cartridge_rom = &(ROM[start]);

        if (cartfile!=NULL)
        {
            osd_fread (cartfile, a7800_cartridge_rom, len);
            osd_fclose (cartfile);
        }
    }
    else {
        return 1;
    }

    memcpy(a7800_cart_f000,&(ROM[0xF000]),0x1000);
    memcpy(&(ROM[0xF000]),a7800_bios_f000,0x1000);
	return 0;
}

/******  TIA  *****************************************/

int a7800_TIA_r(int offset) {
    switch (offset) {
        case 0x08:
            if (input_port_1_r(0) & 0x02)
                return 0x80;
            else
                return 0x00;
        case 0x09:
            if (input_port_1_r(0) & 0x08)
                return 0x80;
            else
                return 0x00;
        case 0x0A:
            if (input_port_1_r(0) & 0x01)
                return 0x80;
            else
                return 0x00;
        case 0x0B:
            if (input_port_1_r(0) & 0x04)
                return 0x80;
            else
                return 0x00;
        case 0x0c:
            if ((input_port_1_r(0) & 0x08) || (input_port_1_r(0) & 0x02))
                return 0x00;
            else
                return 0x80;
        case 0x0d:
            if ((input_port_1_r(0) & 0x01) || (input_port_1_r(0) & 0x04))
                return 0x00;
            else
                return 0x80;
        default:
            if (errorlog) fprintf(errorlog,"undefined TIA read %x\n",offset);

    }
    return 0xFF;
}

void a7800_TIA_w(int offset, int data) {
	switch(offset) {
        case 0x01:
            if (data & 0x01) {
                maria_flag=1;
            }
            if (!a7800_ctrl_lock) {
                a7800_ctrl_lock = data & 0x01;
                a7800_ctrl_reg = data;
                if (data & 0x04)
                    memcpy(&(ROM[0xF000]),a7800_cart_f000,0x1000);
                else
                    memcpy(&(ROM[0xF000]),a7800_bios_f000,0x1000);
            }
        break;
    }
    tia_w(offset,data);
    ROM[offset] = data;
}

/****** RIOT ****************************************/

int a7800_RIOT_r(int offset) {
    switch (offset) {
        case 0:
            return input_port_0_r(0);
        case 2:
            return input_port_3_r(0);
        default:
            if (errorlog) fprintf(errorlog,"undefined RIOT read %x\n",offset);
    }
    return 0xFF;
}

void a7800_RIOT_w(int offset, int data) {
}


/****** RAM Mirroring ******************************/

int a7800_MAINRAM_r(int offset) {
	return ROM[0x2000 + offset];
}

void a7800_MAINRAM_w(int offset, int data) {
	ROM[0x2000 + offset] = data;
}

int a7800_RAM0_r(int offset) {
	return ROM[0x2040 + offset];
}


void a7800_RAM0_w(int offset, int data) {
    ROM[0x2040 + offset] = data;
    ROM[0x40 + offset] = data;
}
/*
void a7800_RAM0_w(int offset, int data) {
	ROM[0x2040 + offset] = data;
}	*/

int a7800_RAM1_r(int offset) {
	return ROM[0x2140 + offset];
}

void a7800_RAM1_w(int offset, int data) {
	ROM[0x2140 + offset] = data;
}



