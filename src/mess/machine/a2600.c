/* Machine functions for the a2600 */

#include "driver.h"
#include "mess/machine/riot.h"
#include "sound/tiaintf.h"

/* for detailed logging */
#define TIA_VERBOSE 1
#define RIOT_VERBOSE 0

/* TIA *Write* Addresses (6 bit) */

#define VSYNC	0x00	/* Vertical Sync Set/Clear              */
#define	VBLANK	0x01    /* Vertical Blank Set/Clear	            */
#define	WSYNC	0x02    /* Wait for Horizontal Blank            */
#define	RSYNC	0x03    /* Reset Horizontal Sync Counter        */
#define	NUSIZ0	0x04    /* Number-Size player/missle 0	        */
#define	NUSIZ1	0x05    /* Number-Size player/missle 1		    */
#define	COLUP0	0x06    /* Color-Luminance Player 0			    */
#define	COLUP1	0x07    /* Color-Luminance Player 1				*/
#define	COLUPF	0x08    /* Color-Luminance Playfield			*/
#define	COLUBK	0x09    /* Color-Luminance BackGround			*/
#define	CTRLPF	0x0A    /* Control Playfield, Ball, Collisions	*/
#define	REFP0	0x0B    /* Reflection Player 0					*/
#define	REFP1	0x0C    /* Reflection Player 1					*/
#define PF0	    0x0D    /* Playfield Register Byte 0			*/
#define	PF1	    0x0E    /* Playfield Register Byte 1			*/
#define	PF2	    0x0F    /* Playfield Register Byte 2			*/
#define	RESP0	0x10    /* Reset Player 0						*/
#define	RESP1	0x11    /* Reset Player 1						*/
#define	RESM0	0x12    /* Reset Missle 0						*/
#define	RESM1	0x13    /* Reset Missle 1						*/
#define	RESBL	0x14    /* Reset Ball							*/

#define	AUDC0	0x15    /* Audio Control 0						*/
#define	AUDC1	0x16    /* Audio Control 1						*/
#define	AUDF0	0x17    /* Audio Frequency 0					*/
#define	AUDF1	0x18    /* Audio Frequency 1					*/
#define	AUDV0	0x19    /* Audio Volume 0						*/
#define	AUDV1	0x1A    /* Audio Volume 1						*/
#define	GRP0	0x1B    /* Graphics Register Player 0			*/
#define	GRP1	0x1C    /* Graphics Register Player 0			*/
#define	ENAM0	0x1D    /* Graphics Enable Missle 0				*/
#define	ENAM1	0x1E    /* Graphics Enable Missle 1				*/
#define	ENABL	0x1F    /* Graphics Enable Ball					*/
#define HMP0	0x20    /* Horizontal Motion Player 0			*/
#define	HMP1	0x21    /* Horizontal Motion Player 0			*/
#define	HMM0	0x22    /* Horizontal Motion Missle 0			*/
#define	HMM1	0x23    /* Horizontal Motion Missle 1			*/
#define	HMBL	0x24    /* Horizontal Motion Ball				*/
#define	VDELP0	0x25    /* Vertical Delay Player 0				*/
#define	VDELP1	0x26    /* Vertical Delay Player 1				*/
#define	VDELBL	0x27    /* Vertical Delay Ball					*/
#define	RESMP0	0x28    /* Reset Missle 0 to Player 0			*/
#define	RESMP1	0x29    /* Reset Missle 1 to Player 1			*/
#define	HMOVE	0x2A    /* Apply Horizontal Motion				*/
#define	HMCLR	0x2B    /* Clear Horizontal Move Registers		*/
#define	CXCLR	0x2C    /* Clear Collision Latches				*/

/* TIA *Read* Addresses */
                        /*          bit 6  bit 7				*/
#define	CXM0P	0x00	/* Read Collision M0-P1  M0-P0			*/
#define	CXM1P	0x01    /*                M1-P0  M1-P1			*/
#define	CXP0FB	0x02	/*                P0-PF  P0-BL			*/
#define	CXP1FB	0x03    /*                P1-PF  P1-BL			*/
#define	CXM0FB	0x04    /*                M0-PF  M0-BL			*/
#define	CXM1FB	0x05    /*                M1-PF  M1-BL			*/
#define	CXBLPF	0x06    /*                BL-PF  -----			*/
#define	CXPPMM	0x07    /*                P0-P1  M0-M1			*/
#define	INPT0	0x08    /* Read Pot Port 0						*/
#define	INPT1	0x09    /* Read Pot Port 1						*/
#define	INPT2	0x0A    /* Read Pot Port 2						*/
#define	INPT3	0x0B    /* Read Pot Port 3						*/
#define	INPT4	0x0C    /* Read Input (Trigger) 0				*/
#define	INPT5	0x0D    /* Read Input (Trigger) 1				*/



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



int a2600_TIA_r(int offset)
{

	switch (offset) {
			case CXM0P:
            case CXM1P:
            case CXP0FB:
            case CXP1FB:
            case CXM0FB:
            case CXM1FB:
            case CXBLPF:
            case CXPPMM:
					if (errorlog && TIA_VERBOSE)
						fprintf(errorlog,"TIA_r - COLLISION range\n");
			break;

	        case INPT0:	  /* offset 0x08 */
	            if (input_port_1_r(0) & 0x02)
	                return 0x80;
	            else
	                return 0x00;
	        case INPT1:	  /* offset 0x09 */
	            if (input_port_1_r(0) & 0x08)
	                return 0x80;
	            else
	                return 0x00;
	        case INPT2:	  /* offset 0x0A */
	            if (input_port_1_r(0) & 0x01)
	                return 0x80;
	            else
	                return 0x00;
	        case INPT3:	  /* offset 0x0B */
	            if (input_port_1_r(0) & 0x04)
	                return 0x80;
	            else
	                return 0x00;
	        case INPT4:	  /* offset 0x0C */
	            if ((input_port_1_r(0) & 0x08) || (input_port_1_r(0) & 0x02))
	                return 0x00;
	            else
	                return 0x80;
	        case INPT5:	  /* offset 0x0D */
	            if ((input_port_1_r(0) & 0x01) || (input_port_1_r(0) & 0x04))
	                return 0x00;
	            else
	                return 0x80;
	        default:
	            if (errorlog) fprintf(errorlog,"TIA_r undefined read %x\n",offset);

	    }
    return 0xFF;


}


void a2600_TIA_w(int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	switch (offset)
	{

		case VSYNC:
			if (data == 0x02)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VSYNC Start\n");
			}
			else if (data == 0x00)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VSYNC Stop\n");
			}
			else /* not allowed */
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}



			break;
		case VBLANK:     	/* offset 0x01, bits 7,6 and 1 used */
			if (data == 0x00)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VBLANK Stop\n");
			}
			else if (data == 0x02)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VBLANK Start\n");
			}
			else
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VBLANK Write Error! offset $%02x & data $%02x\n", offset, data);
			}
		    break;
		case WSYNC:     	/* offset 0x02 */
			if (data == 0x00)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - WSYNC \n");
			}
			else
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - WSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            break;






		case 0x15: /* audio control */
		case 0x16: /* audio control */
		case 0x17: /* audio frequency */
		case 0x18: /* audio frequency */
		case 0x19: /* audio volume 0 */
		case 0x1A: /* audio volume 1 */

			tia_w(offset,data);
			ROM[offset] = data;
			break;
		default:
			if (errorlog && TIA_VERBOSE)
				fprintf(errorlog,"TIA_w - UNKNOWN - offset %x & data %x\n", offset, data);
		/* all others */
		ROM[offset] = data;
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




int a2600_load_rom (int id, const char *rom_name)
{
    FILE *cartfile;
	UINT8 *ROM = memory_region(REGION_CPU1);

	if(rom_name==NULL)
	{
		printf("a2600 Requires Cartridge!\n");
		return INIT_FAILED;
	}

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (strlen(rom_name)==0)
    {
        if (errorlog) fprintf(errorlog,"A2600 - warning: no cartridge specified!\n");
	}
	else if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if (errorlog) fprintf(errorlog,"A2600 - Unable to locate cartridge: %s\n",rom_name);
		return 1;
	}
	if (errorlog) fprintf(errorlog,"A2600 - loading Cart - %s \n",rom_name);

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


