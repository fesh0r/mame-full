/* Machine functions for the a2600 */

#include "driver.h"
#include "mess/machine/riot.h"
#include "sound/tiaintf.h"
#include "cpuintrf.h"
#include "mess/machine/tia.h"

/* for detailed logging */
#define TIA_VERBOSE 0
#define RIOT_VERBOSE 0

TIA tia;

int a2600_scanline_interrupt(void);
static int cpu_current_state;
static void *RIOT_timer;

#ifdef USE_RIOT
static int a2600_riot_a_r(int chip);
static int a2600_riot_b_r(int chip);
static void a2600_riot_a_w(int chip, int offset, int data);
static void a2600_riot_b_w(int chip, int offset, int data);


static struct RIOTinterface a2600_riot = {
	1,						/* number of chips */
	{ 1190000 },			/* baseclock of chip */
	{ a2600_riot_a_r }, 	/* port a input */
	{ a2600_riot_b_r }, 	/* port b input */
	{ a2600_riot_a_w }, 	/* port a output */
	{ a2600_riot_b_w }, 	/* port b output */
	{ NULL }				/* interrupt callback */
};
#endif


static int msize0;
static int msize1;

static int cpu_current_state = 1; /*running*/

/* bitmap */
struct osd_bitmap *stella_bitmap;

/* local */
static unsigned char *a2600_cartridge_rom;


#ifdef USE_RIOT
static int a2600_riot_a_r(int chip)
{
	/* joystick !? */
    return readinputport(0);
}

static int a2600_riot_b_r(int chip)
{
	/* console switches !? */
    return readinputport(1);
}

static void a2600_riot_a_w(int chip, int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	ROM[offset] = data;
}

static void a2600_riot_b_w(int chip, int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	ROM[offset] = data;
}
#endif


/***************************************************************************

  RIOT Reads.

***************************************************************************/
int a2600_riot_r(int offset)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	switch(offset)
	{
		case 0x00:
        return readinputport(0);


		case 0x01:
		return readinputport(1);


		case 0x02:
		return readinputport(2);


		case 0x04: /*TIMER READ */

		{
			int time_left = timer_timeleft(RIOT_timer);
			if(time_left) return (int)1190000*1/time_left;
		}

	}

	return 	ROM[offset];
}


static void riot_timer_cb(int param)
{
	logerror("riot timer expired\n");

}


/***************************************************************************

  RIOT Writes.

***************************************************************************/

void a2600_riot_w(int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	switch (offset)
	{
		case 0x16: /* Timer 64 Start */
			if( RIOT_timer )
				timer_remove(RIOT_timer);
			logerror("riot TMR64 write: data %02x - time is $%f\n", data,64.0 * data/1190000 );
			RIOT_timer=timer_pulse(64.0 * data/1190000, 0, riot_timer_cb);
			ROM[0x284]=	(int)64.0 * data/1190000;
			break;





	}
	ROM[offset] = data;

}







/***************************************************************************

  TIA Reads.

***************************************************************************/

int a2600_TIA_r(int offset)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	switch (offset) {
			case CXM0P:
            case CXM1P:
            case CXP0FB:
            case CXP1FB:
            case CXM0FB:
            case CXM1FB:
            case CXBLPF:
            case CXPPMM:
			//break;

	        case INPT0:	  /* offset 0x08 */
	            if (input_port_1_r(0) & 0x02)
	                return 0x80;
	            else
	                return 0x00;
				//break;
	        case INPT1:	  /* offset 0x09 */
	            if (input_port_1_r(0) & 0x08)
	                return 0x80;
	            else
	                return 0x00;
				//break;
	        case INPT2:	  /* offset 0x0A */
	            if (input_port_1_r(0) & 0x01)
	                return 0x80;
	            else
	                return 0x00;
				//break;
	        case INPT3:	  /* offset 0x0B */
	            if (input_port_1_r(0) & 0x04)
	                return 0x80;
	            else
	                return 0x00;
				//break;
	        case INPT4:	  /* offset 0x0C */
	            if ((input_port_1_r(0) & 0x08) || (input_port_1_r(0) & 0x02))
	                return 0x00;
	            else
	                return 0x80;
				//break;
	        case INPT5:	  /* offset 0x0D */
	            if ((input_port_1_r(0) & 0x01) || (input_port_1_r(0) & 0x04))
	                return 0x00;
	            else
	                return 0x80;
				//break;

	        default:
	            logerror("TIA_r undefined read %x\n",offset);

	    }
    //return 0xFF;
	return 	ROM[offset];

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

				if (TIA_VERBOSE)
				{
                    logerror("TIA_w - VSYNC Stop\n");
				}

			}
			else if (data & 0x02)
			{

				if (TIA_VERBOSE)
					logerror("TIA_w - VSYNC Start\n");

			}
			else /* not allowed */
			{
				logerror("TIA_w - VSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
           // break;


		case VBLANK:     	/* offset 0x01, bits 7,6 and 1 used */
			if ( !(data & 0x00) )
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - VBLANK Stop\n");
			}
			else if (data & 0x02)
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - VBLANK Start\n");
			}
			else
			{
				logerror("TIA_w - VBLANK Write Error! offset $%02x & data $%02x\n", offset, data);
			}
		    //break;


		case WSYNC:     	/* offset 0x02 */
			if ( !(data & 0x00) )
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - WSYNC \n");

				//logerror("WSYNC - waiting for %lf\n", cpu_getscanlinetime(cpu_getscanline()));
				//cpu_spinuntil_time(cpu_getscanlinetime(cpu_getscanline()));

			}

			else
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - WSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            //break;



		case RSYNC:     	/* offset 0x03 */
			if ( !(data & 0x00) )
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - RSYNC \n");
			}
			else
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - RSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
			}
            //break;



		case NUSIZ0:     	/* offset 0x04 */
			msize0 = 2^(data>>4);
			logerror("TIA_w - NUSIZ0, Missile Size = %d clocks at horzpos %d\n",msize0, cpu_gethorzbeampos());
			/* must implement player size checking! */

            //break;


		case NUSIZ1:     	/* offset 0x05 */
			msize1 = 2^(data>>4);
			logerror("TIA_w - NUSIZ1, Missile Size = %d clocks at horzpos %d\n",msize1, cpu_gethorzbeampos());
			/* must implement player size checking! */

            //break;


		case COLUP0:     	/* offset 0x06 */

			tia.colreg.P0 = data>>4;
			tia.colreg.M0 = tia.colreg.P0; /* missile same color */
   			if (TIA_VERBOSE)
				logerror("TIA_w - COLUP0 Write color is $%02x\n", tia.colreg.P0);
            //break;

		case COLUP1:     	/* offset 0x07 */

			tia.colreg.P1 = data>>4;
			tia.colreg.M1 = tia.colreg.P1; /* missile same color */
   			if (TIA_VERBOSE)
				logerror("TIA_w - COLUP1 Write color is $%02x\n", tia.colreg.P1);
            //break;

		case COLUPF:     	/* offset 0x08 */

			tia.colreg.PF = data>>4;
			tia.colreg.BL = data>>4;  /* ball is same as playfield */
   			if (TIA_VERBOSE)
				logerror("TIA_w - COLUPF Write color is $%02x\n", tia.colreg.PF);
            //break;

		case COLUBK:     	/* offset 0x09 */

			tia.colreg.BK = data>>4;
   			if (TIA_VERBOSE)
				logerror("TIA_w - COLUBK Write color is $%02x\n", tia.colreg.BK);
            //break;


		case CTRLPF:     	/* offset 0x0A */


				if (TIA_VERBOSE)
					logerror("TIA_w - CTRLPF Write offset $%02x & data $%02x\n", offset, data);

            //break;



		case REFP0:
			if ( !(data & 0x00) )
			{
                if (TIA_VERBOSE)
					logerror("TIA_w - REFP0 No reflect \n");
			}
			else if ( data & 0x08)
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - REFP0 Reflect \n");
			}
			else
			{
            	if (TIA_VERBOSE)
					logerror("TIA_w - Write Error, REFP0 offset $%02x & data $%02x\n", offset, data);
			}
            //break;


		case REFP1:
			if ( !(data & 0x00) )
			{
                if (TIA_VERBOSE)
					logerror("TIA_w - REFP1 No reflect \n");
			}
			else if ( data & 0x08)
			{
				if (TIA_VERBOSE)
					logerror("TIA_w - REFP1 Reflect \n");
			}
			else
			{
            	if (TIA_VERBOSE)
					logerror("TIA_w - Write Error, REFP1 offset $%02x & data $%02x\n", offset, data);
			}
            //break;






		case PF0:	    /* 0x0D Playfield Register Byte 0 */
			tia.pfreg.B0 = data;
			if (TIA_VERBOSE)
				logerror("TIA_w - PF0 register is $%02x \n", tia.pfreg.B0);
		//	break;

        case PF1:		/* 0x0E Playfield Register Byte 1 */
			tia.pfreg.B1 = data;
			if (TIA_VERBOSE)
				logerror("TIA_w - PF1 register is $%02x \n", tia.pfreg.B1);
			//break;

		case PF2: 		/* 0x0F Playfield Register Byte 2 */
			tia.pfreg.B2 = data;
			if (TIA_VERBOSE)
				logerror("TIA_w - PF2 register is $%02x \n", tia.pfreg.B2);
			//break;


/* These next 5 Registers are Strobe registers            */
/* They will need to update the screen as soon as written */

		case RESP0: 	/* 0x10 Reset Player 0 */
			//break;

		case RESP1: 	/* 0x11 Reset Player 1 */
			//break;

		case RESM0:		/* 0x12 Reset Missle 0 */
			//break;

		case RESM1: 	/* 0x13 Reset Missle 1 */
			//break;

		case RESBL: 	/* 0x14 Reset Ball */
			//break;







		case AUDC0: /* audio control */
		case AUDC1: /* audio control */
		case AUDF0: /* audio frequency */
		case AUDF1: /* audio frequency */
		case AUDV0: /* audio volume 0 */
		case AUDV1: /* audio volume 1 */

			tia_w(offset,data);
			//break;


		case GRP0:		/* 0x1B Graphics Register Player 0 */
			//break;

		case GRP1:		/* 0x1C Graphics Register Player 0 */
			//break;

		case ENAM0: 	/* 0x1D Graphics Enable Missle 0 */
			//break;

		case ENAM1:		/* 0x1E Graphics Enable Missle 1 */
			//break;

		case ENABL: 	/* 0x1F Graphics Enable Ball */
			//break;

		case HMP0:		/* 0x20	Horizontal Motion Player 0 */
			//break;

		case HMP1: 		/* 0x21 Horizontal Motion Player 0 */
			//break;

		case HMM0:		/* 0x22 Horizontal Motion Missle 0 */
			//break;

		case HMM1: 		/* 0x23 Horizontal Motion Missle 1 */
			//break;

		case HMBL: 		/* 0x24 Horizontal Motion Ball */
			//break;

		case VDELP0:	/* 0x25 Vertical Delay Player 0 */
			//break;

		case VDELP1: 	/* 0x26 Vertical Delay Player 1 */
			//break;

		case VDELBL: 	/* 0x27 Vertical Delay Ball	*/
			//break;

		case RESMP0:	/* 0x28 Reset Missle 0 to Player 0 */
			//break;

		case RESMP1:	/* 0x29 Reset Missle 1 to Player 1 */
			//break;

		case HMOVE: 	/* 0x2A Apply Horizontal Motion	*/
			//break;

		case HMCLR: 	/* 0x2B Clear Horizontal Move Registers */
			//break;

		case CXCLR:		/* 0x2C Clear Collision Latches	*/
			//break;


		ROM[offset] = data;
		break;
		default:
			logerror("TIA_w - UNKNOWN - offset %02x & data %02x\n", offset, data);
		/* all others */
		ROM[offset] = data;
	}
}

void a2600_init_machine(void)
{


	/* start RIOT interface */
	///riot_init(&a2600_riot);
	cpu_current_state=1;
	return;

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
		logerror("A2600 - Unable to locate cartridge: %s\n",rom_name);
		return 1;
	}
	logerror("A2600 - loading Cart - %s \n",rom_name);

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



	cpu_current_state=1; /* running again */


	/* plot the playfield and background for now               */
	/* each register value is 4 color clocks                   */
	/* to pick the color, need to bit check the playfield regs */

	/* set color to background */
	backcolor=tia.colreg.BK;

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
    copybitmap(bitmap,stella_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

}


