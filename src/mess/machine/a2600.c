/* Machine functions for the a2600 */

#include "driver.h"
#include "mess/machine/riot.h"
#include "sound/tiaintf.h"

unsigned char *ROM;

static int a2600_riot_a_r(int chip);
static int a2600_riot_b_r(int chip);
static void a2600_riot_a_w(int chip, int data);
static void a2600_riot_b_w(int chip, int data);


static struct RIOTinterface a2600_riot = {
	1,						/* number of chips */
	{ 1190000 },			/* baseclock of chip */
	{ a2600_riot_a_r }, 	/* port a input */
	{ a2600_riot_b_r }, 	/* port b input */
	{ a2600_riot_a_w }, 	/* port a output */
	{ a2600_riot_b_w }, 	/* port b output */
	{ NULL }				/* interrupt callback */
};


/* local */
unsigned char *a2600_cartridge_rom;


int  a2600_ROM_r (int offset)
{
 //if(errorlog) fprintf(errorlog, "offset is %04X\n", offset);
 //if(errorlog) fprintf(errorlog, "opcode at ROM[0x1000 + offset] is %04X\n", ROM[0x1000 + offset]);
 //exit(1);
 return ROM[0x1000 + offset];
}


static int a2600_riot_a_r(int chip)
{
	/* joystick !? */
    return readinputport(0);
}

static int a2600_riot_b_r(int chip)
{
	/* console switches !? */
    return readinputport(0);
}

static void a2600_riot_a_w(int chip, int data)
{
	/* anything? */
}

static void a2600_riot_b_w(int chip, int data)
{
	/* anything? */
}



//  TIA *Read* Addresses
//                                   bit 6  bit 7
// 	CXM0P	0x0    Read Collision M0-P1  M0-P0
// 	CXM1P	0x1                   M1-P0  M1-P1
// 	CXP0FB	0x2                   P0-PF  P0-BL
// 	CXP1FB	0x3                   P1-PF  P1-BL
//	CXM0FB	0x4                   M0-PF  M0-BL
// 	CXM1FB	0x5                   M1-PF  M1-BL
// 	CXBLPF	0x6                   BL-PF  -----
// 	CXPPMM	0x7                   P0-P1  M0-M1
// 	INPT0	0x8     Read Pot Port 0
// 	INPT1	0x9     Read Pot Port 1
// 	INPT2	0xA     Read Pot Port 2
// 	INPT3	0xB     Read Pot Port 3
// 	INPT4	0xC     Read Input (Trigger) 0
// 	INPT5	0xD     Read Input (Trigger) 1

int a2600_TIA_r(int offset)
{
	if (errorlog) fprintf(errorlog,"***A2600 - TIA_r with offset %x\n", offset );
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


void a2600_TIA_w(int offset, int data)
{

	switch (offset)
	{
		case 0x15: /* audio control */
		case 0x16: /* audio control */
		case 0x17: /* audio frequency */
		case 0x18: /* audio frequency */
		case 0x19: /* audio volume 0 */
		case 0x1A: /* audio volume 1 */
			if (errorlog) fprintf(errorlog,"***A2600 - TIA_w with offset %x & data %x\n", offset, data );
			tia_w(offset,data);
			//ROM[offset] = data;
			break;
		//default:
		/* all others */
		//ROM[offset] = data;
	}
}

void a2600_init_machine(void)
{
	/* start RIOT interface */
	riot_init(&a2600_riot);
}


void a2600_stop_machine(void)
{

}


int a2600_id_rom (const char *name, const char *gamename)
{
	return 0;		/* no id possible */

}




int a2600_load_rom (void)
{
    FILE *cartfile;
	ROM = memory_region(REGION_CPU1);

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (strlen(rom_name[0])==0)
    {
        if (errorlog) fprintf(errorlog,"A2600 - warning: no cartridge specified!\n");
	}
	else if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_IMAGE_R, 0)))
	{
        if (errorlog) fprintf(errorlog,"A2600 - Unable to locate cartridge: %s\n",rom_name[0]);
		return 1;
	}
    if (errorlog) fprintf(errorlog,"A2600 - loading Cart - %s \n",rom_name[0]);

    a2600_cartridge_rom = &(ROM[0x1000]);		/* 'plug' cart into 0x1000 */

    if (cartfile!=NULL)
    {
        osd_fread (cartfile, a2600_cartridge_rom, 2048); /* testing COmbat for now */
        osd_fclose (cartfile);
		/* copy to mirrorred memory regions */
		memcpy(&ROM[0x1800], &ROM[0x1000], 0x0800);
		memcpy(&ROM[0xf000], &ROM[0x1000], 0x0800);
		memcpy(&ROM[0xf800], &ROM[0x1000], 0x0800);
    }
    else
    {
        return 1;
    }

	return 0;
}


