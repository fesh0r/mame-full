/* Machine functions for the a2600 */

#include "driver.h"
#include "mess/machine/riot.h"
#include "sound/tiaintf.h"
#include "cpuintrf.h"

/* for detailed logging */
#define TIA_VERBOSE 0
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
/*
COLUP0, COLUP1, COLUPF, COLUBK:
These addresses write data into the player, playfield
and background color-luminance registers:

COLOR           D7  D6  D5  D4 | D3  D2  D1  LUM
grey/gold       0   0   0   0  | 0   0   0   black
                0   0   0   1  | 0   0   1   dark grey
orange/brt org  0   0   1   0  | 0   1   0
                0   0   1   1  | 0   1   1   grey
pink/purple     0   1   0   0  | 1   0   0
                0   1   0   1  | 1   0   1
purp/blue/blue  0   1   1   0  | 1   1   0   light grey
                0   1   1   1  | 1   1   1   white
blue/lt blue    1   0   0   0
                1   0   0   1
torq/grn blue   1   0   1   0
                1   0   1   1
grn/yel grn     1   1   0   0
                1   1   0   1
org.grn/lt org  1   1   1   0
                1   1   1   1
*/

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

/* The next 5 registers are flash registers */
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


int a2600_scanline_interrupt(void);

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

/* keep a record of the colors here */
struct color_registers {
	int P0; /* player 0   */
	int M0;	/* missile 0  */
	int P1;	/* player 1   */
	int M1;	/* missile 1  */
	int PF;	/* playfield  */
	int BL;	/* ball       */
	int BK;	/* background */
} colreg;


/* keep a record of the playfield registers here */
struct playfield_registers {
	int B0; /* 8 bits, only left 4 bits used */
	int B1; /* 8 bits  */
	int B2;	/* 8 bits  */
} pfreg;

int scanline_registers[80]; /* array to hold info on who gets displayed */

static int msize0;
static int msize1;

/* bitmap */
struct osd_bitmap *stella_bitmap;

/* local */
static unsigned char *a2600_cartridge_rom;

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



/***************************************************************************

  TIA Reads.

***************************************************************************/

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



/***************************************************************************

  TIA Writes.

***************************************************************************/

void a2600_TIA_w(int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);


	switch (offset)
	{

		case VSYNC:
			if ( !(data & 0x00) )
			{

				if (errorlog && TIA_VERBOSE)
				{
                    fprintf(errorlog,"TIA_w - VSYNC Stop\n");
				}

			}
			else if (data & 0x02)
			{

				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VSYNC Start\n");

			}
			else /* not allowed */
			{
				if (errorlog)
					fprintf(errorlog,"TIA_w - VSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            break;


		case VBLANK:     	/* offset 0x01, bits 7,6 and 1 used */
			if ( !(data & 0x00) )
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VBLANK Stop\n");
			}
			else if (data & 0x02)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - VBLANK Start\n");
			}
			else
			{
				if (errorlog)
					fprintf(errorlog,"TIA_w - VBLANK Write Error! offset $%02x & data $%02x\n", offset, data);
			}
		    break;


		case WSYNC:     	/* offset 0x02 */
			if ( !(data & 0x00) )
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - WSYNC \n");
				//cpu_spinuntil_int (); /* wait til end of scanline */
			}
			else
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - WSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            break;



		case RSYNC:     	/* offset 0x03 */
			if ( !(data & 0x00) )
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - RSYNC \n");
			}
			else
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - RSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            break;



		case NUSIZ0:     	/* offset 0x04 */
			msize0 = 2^(data>>4);
			if (errorlog)
				fprintf(errorlog,"TIA_w - NUSIZ0, Missile Size = %d clocks at horzpos %d\n",msize0, cpu_gethorzbeampos());
			/* must implement player size checking! */

            break;


		case NUSIZ1:     	/* offset 0x05 */
			msize1 = 2^(data>>4);
			if (errorlog)
				fprintf(errorlog,"TIA_w - NUSIZ1, Missile Size = %d clocks at horzpos %d\n",msize1, cpu_gethorzbeampos());
			/* must implement player size checking! */

            break;


		case COLUP0:     	/* offset 0x06 */

			colreg.P0 = data>>4;
			colreg.M0 = colreg.P0; /* missile same color */
   			if (errorlog && TIA_VERBOSE)
				fprintf(errorlog,"TIA_w - COLUP0 Write color is $%02x\n", colreg.P0);
            break;

		case COLUP1:     	/* offset 0x07 */

			colreg.P1 = data>>4;
			colreg.M1 = colreg.P1; /* missile same color */
   			if (errorlog && TIA_VERBOSE)
				fprintf(errorlog,"TIA_w - COLUP1 Write color is $%02x\n", colreg.P1);
            break;

		case COLUPF:     	/* offset 0x08 */

			colreg.PF = data>>4;
			colreg.BL = data>>4;  /* ball is same as playfield */
   			if (errorlog && TIA_VERBOSE)
				fprintf(errorlog,"TIA_w - COLUPF Write color is $%02x\n", colreg.PF);
            break;

		case COLUBK:     	/* offset 0x09 */

			colreg.BK = data>>4;
   			if (errorlog && TIA_VERBOSE)
				fprintf(errorlog,"TIA_w - COLUBK Write color is $%02x\n", colreg.BK);
            break;


		case CTRLPF:     	/* offset 0x0A */


				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - CTRLPF Write offset $%02x & data $%02x\n", offset, data);

            break;



		case REFP0:
			if ( !(data & 0x00) )
			{
                if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - REFP0 No reflect \n");
			}
			else if ( data & 0x08)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - REFP0 Reflect \n");
			}
			else
			{
            	if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - Write Error, REFP0 offset $%02x & data $%02x\n", offset, data);
			}
            break;


		case REFP1:
			if ( !(data & 0x00) )
			{
                if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - REFP1 No reflect \n");
			}
			else if ( data & 0x08)
			{
				if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - REFP1 Reflect \n");
			}
			else
			{
            	if (errorlog && TIA_VERBOSE)
					fprintf(errorlog,"TIA_w - Write Error, REFP1 offset $%02x & data $%02x\n", offset, data);
			}
            break;






		case PF0:	    /* 0x0D Playfield Register Byte 0 */
			pfreg.B0 = data;
			if (errorlog)
				fprintf(errorlog,"TIA_w - PF0 register is $%02x \n", pfreg.B0);
			break;

        case PF1:		/* 0x0E Playfield Register Byte 1 */
			pfreg.B1 = data;
			if (errorlog)
				fprintf(errorlog,"TIA_w - PF1 register is $%02x \n", pfreg.B1);
			break;

		case PF2: 		/* 0x0F Playfield Register Byte 2 */
			pfreg.B2 = data;
			if (errorlog)
				fprintf(errorlog,"TIA_w - PF2 register is $%02x \n", pfreg.B2);
			break;


/* These next 5 Registers are Strobe registers            */
/* They will need to update the screen as soon as written */

		case RESP0: 	/* 0x10 Reset Player 0 */
			break;

		case RESP1: 	/* 0x11 Reset Player 1 */
			break;

		case RESM0:		/* 0x12 Reset Missle 0 */
			break;

		case RESM1: 	/* 0x13 Reset Missle 1 */
			break;

		case RESBL: 	/* 0x14 Reset Ball */
			break;







		case AUDC0: /* audio control */
		case AUDC1: /* audio control */
		case AUDF0: /* audio frequency */
		case AUDF1: /* audio frequency */
		case AUDV0: /* audio volume 0 */
		case AUDV1: /* audio volume 1 */

			tia_w(offset,data);
			//ROM[offset] = data;
			break;


		case GRP0:		/* 0x1B Graphics Register Player 0 */
			break;

		case GRP1:		/* 0x1C Graphics Register Player 0 */
			break;

		case ENAM0: 	/* 0x1D Graphics Enable Missle 0 */
			break;

		case ENAM1:		/* 0x1E Graphics Enable Missle 1 */
			break;

		case ENABL: 	/* 0x1F Graphics Enable Ball */
			break;

		case HMP0:		/* 0x20	Horizontal Motion Player 0 */
			break;

		case HMP1: 		/* 0x21 Horizontal Motion Player 0 */
			break;

		case HMM0:		/* 0x22 Horizontal Motion Missle 0 */
			break;

		case HMM1: 		/* 0x23 Horizontal Motion Missle 1 */
			break;

		case HMBL: 		/* 0x24 Horizontal Motion Ball */
			break;

		case VDELP0:	/* 0x25 Vertical Delay Player 0 */
			break;

		case VDELP1: 	/* 0x26 Vertical Delay Player 1 */
			break;

		case VDELBL: 	/* 0x27 Vertical Delay Ball	*/
			break;

		case RESMP0:	/* 0x28 Reset Missle 0 to Player 0 */
			break;

		case RESMP1:	/* 0x29 Reset Missle 1 to Player 1 */
			break;

		case HMOVE: 	/* 0x2A Apply Horizontal Motion	*/
			break;

		case HMCLR: 	/* 0x2B Clear Horizontal Move Registers */
			break;

		case CXCLR:		/* 0x2C Clear Collision Latches	*/
			break;




		default:
			if (errorlog)
				fprintf(errorlog,"TIA_w - UNKNOWN - offset %02x & data %02x\n", offset, data);
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


int a2600_id_rom (int id)
{
	return 0;		/* no id possible */

}




int a2600_load_rom (int id)
{
	const char *rom_name = device_filename(IO_CARTSLOT,id);
    FILE *cartfile;
	UINT8 *ROM = memory_region(REGION_CPU1);

	if(rom_name==NULL)
	{
		printf("a2600 Requires Cartridge!\n");
		return INIT_FAILED;
	}

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (!(cartfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if (errorlog) fprintf(errorlog,"A2600 - Unable to locate cartridge: %s\n",rom_name);
		return 1;
	}
	if (errorlog) fprintf(errorlog,"A2600 - loading Cart - %s \n",rom_name);

    a2600_cartridge_rom = &(ROM[0x1000]);		/* 'plug' cart into 0x1000 */

    if (cartfile!=NULL)
    {
        osd_fread (cartfile, a2600_cartridge_rom, 2048); /* testing Combat for now */
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






/* Video functions for the a2600         */
/* Since all software drivern, have here */


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int a2600_vh_start(void)
{	if ((stella_bitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
    return 0;
}

void a2600_vh_stop(void)
{
	free(stella_bitmap);

}


/* when called, update the bitmap. */
int a2600_scanline_interrupt(void)
{
	int regpos, pixpos;
	int xs = Machine->drv->visible_area.min_x;//68;
	int ys = Machine->drv->visible_area.max_y;//228;
	int currentline =  cpu_getscanline();
	int backcolor;

	/* plot the playfield and background for now               */
	/* each register value is 4 color clocks                   */
	/* to pick the color, need to bit check the playfield regs */

	/* set color to background */
	backcolor=colreg.BK;

	/* check PF register 0 (left 4 bits only) */

	//pfreg.P0




	/* now we have color, plot for 4 color cycles */
	for (regpos=xs;regpos<ys;regpos=regpos+=4)
	{
		for (pixpos = regpos;pixpos<regpos+4;pixpos++)
			plot_pixel (stella_bitmap, pixpos, currentline, Machine->pens[backcolor]);
	}





	return 0;
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/
/* This routine is called at the start of vblank to refresh the screen */
void a2600_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	if (errorlog)
		fprintf(errorlog,"SCREEN UPDATE CALLED\n");

    copybitmap(bitmap,stella_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

}


