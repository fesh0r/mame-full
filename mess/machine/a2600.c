/* The book of Revelations according to LollypopMan */
/*                      aka                         */
/*         Machine functions for the a2600          */
/*      The Blaggers Guide to Emu Programming       */

/* TODO: Better Comments ;) */


#include "driver.h"
#include "mess/machine/riot.h"
#include "sound/tiaintf.h"
#include "sound/tiasound.h"
#include "cpuintrf.h"
#include "mess/machine/tia.h"
#include "drawgfx.h"

struct rectangle stella_size = {0, 227, 0, 281};

/* for detailed logging */
#define TIA_VERBOSE 0
#define RIOT_VERBOSE 0

#define TRIGGER_HSYNC 65532

UINT8 PF_Ref = 0;
UINT8 TIA_player_0_reflect = 0;
UINT8 TIA_player_1_reflect = 0;
UINT8 PF_Score = 0;
UINT8 PF_Pfp = 0;
UINT8 TIA_player_0_tick = 8;
UINT8 TIA_player_1_tick = 8;

UINT8 PF_Data[160];


TIA tia;

UINT8 TMR_Intim = 0;
UINT16 TMR_Period = 0;
UINT16 TMR_tmp = 0;
UINT8 PF0_Rendered = 0;
UINT8 PF1_Rendered = 0;
UINT8 PF2_Rendered = 0;
UINT8 TIA_hmp0 = 0;
UINT8 TIA_hmp1 = 0;
char TIA_movement_table[] =
{0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};
char TIA_motion_player_0 = 0;
char TIA_motion_player_1 = 0;
UINT32 a2600_Cycle_Count = 0;
UINT8 TIA_vblank;
UINT8 halted = 0;
UINT32 global_tia_cycle = 0;
UINT32 previous_tia_cycle = 0;

UINT16 HSYNC_Period = 0;
UINT16 TIA_reset_player_0 = 0;
UINT16 TIA_reset_player_1 = 0;
UINT8 TIA_player_pattern_0 = 0;
UINT8 TIA_player_pattern_0_tmp = 0;
UINT8 TIA_player_pattern_1 = 0;
UINT8 TIA_player_pattern_1_tmp = 0;

UINT8 HSYNC_colour_clock = 3;
int flashycolour = 0;

UINT8 RIOT_passedzero = 0;

static void a2600_main_cb(int param);

static void a2600_scanline_cb(void);
static void a2600_Cycle_cb(int param);

static int cpu_current_state;

static void *HSYNC_timer;

#ifdef USE_RIOT
static int a2600_riot_a_r(int chip);
static int a2600_riot_b_r(int chip);
static void a2600_riot_a_w(int chip, int offset, int data);
static void a2600_riot_b_w(int chip, int offset, int data);



static struct RIOTinterface a2600_riot =
{
	1,									/* number of chips */
	{1190000},							/* baseclock of chip */
	{a2600_riot_a_r},					/* port a input */
	{a2600_riot_b_r},					/* port b input */
	{a2600_riot_a_w},					/* port a output */
	{a2600_riot_b_w},					/* port b output */
	{NULL}								/* interrupt callback */
};

#endif


static int msize0 = 0;
static int msize1 = 0;

static int cpu_current_state = 1;		/*running */

/* bitmap */
static struct osd_bitmap *stella_bitmap = NULL;
static struct osd_bitmap *stella_backbuffer = NULL;


/* local */
static unsigned char *a2600_cartridge_rom;

/* scanline */
static int currentline = 0;

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
	UINT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

	//logerror("TIMER Read previous riot cycle %08x, current riot cycle %08x, difference %08x\n", previous_tia_cycle, global_tia_cycle + TIME_TO_CYCLES( 0, timer_timeelapsed(HSYNC_timer)), riotdiff);
	/* resync the riot timer */
	previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
	riotdiff *= 3;
	for (; riotdiff > 0; riotdiff--)
	{
		//  logerror("diff 0x%04x\n", riotdiff);
		a2600_Cycle_cb(0);
		//  logerror("tmr temp = 0x%04x\n", TMR_tmp);
	}

	switch (offset)
	{
	case 0x00:
		return readinputport(0);


	case 0x01:
		return readinputport(1);


	case 0x02:
		return readinputport(2);


	case 0x04:							/*TIMER READ */

		{
			//  logerror("Timer read 0x%02x\n", TMR_Intim);
			return TMR_Intim;
		}

	}

	return ROM[offset];
}

/***************************************************************************

  RIOT Writes.

***************************************************************************/

void a2600_riot_w(int offset, int data)
{

	UINT8 *ROM = memory_region(REGION_CPU1);

	{
		UINT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		//  logerror("PF2 previous riot cycle %08x, current riot cycle %08x, difference %08x\n", previous_tia_cycle, global_tia_cycle + TIME_TO_CYCLES( 0, timer_timeelapsed(HSYNC_timer)), riotdiff);
		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		for (; riotdiff > 0; riotdiff--)
		{
			//  logerror("diff 0x%04x\n", riotdiff);
			a2600_Cycle_cb(0);
			//  logerror("tmr temp = 0x%04x\n", TMR_tmp);
		}
	}

	switch (offset)
	{
	case 0x14:							/* Timer 1 Start */
		TMR_Intim = data;
		TMR_Period = 1;
		TMR_tmp = 1;
		ROM[0x284] = (int) data;
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));

		break;


	case 0x15:							/* timer 8 start */
		TMR_Intim = data;
		TMR_Period = 8;
		TMR_tmp = 8;
		ROM[0x284] = (int) data;
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		break;

	case 0x16:							/* Timer 64 Start */
		TMR_Intim = data;
		TMR_Period = 64;
		TMR_tmp = 64;
		ROM[0x284] = (int) data;
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));

		break;

	case 0x17:							/* Timer 1024 Start */
		TMR_Intim = data;
		TMR_Period = 1024;
		TMR_tmp = 1024;
		ROM[0x284] = (int) data;
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));

		break;





	}
	//logerror("Riot Write offset %x, data %x\n", offset, data);
	ROM[offset] = data;

}







/***************************************************************************

  TIA Reads.

***************************************************************************/

int a2600_TIA_r(int offset)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	unsigned int pc;

	pc = cpu_get_pc();
	{
		UINT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		//  logerror("PF2 previous riot cycle %08x, current riot cycle %08x, difference %08x\n", previous_tia_cycle, global_tia_cycle + TIME_TO_CYCLES( 0, timer_timeelapsed(HSYNC_timer)), riotdiff);
		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		for (; riotdiff > 0; riotdiff--)
		{
			//  logerror("diff 0x%04x\n", riotdiff);
			a2600_Cycle_cb(0);
			//  logerror("tmr temp = 0x%04x\n", TMR_tmp);
		}
	}

	switch (offset)
	{
	case CXM0P:
	case CXM1P:
	case CXP0FB:
	case CXP1FB:
	case CXM0FB:
	case CXM1FB:
	case CXBLPF:
	case CXPPMM:
		break;

	case INPT0:						/* offset 0x08 */
		if (input_port_1_r(0) & 0x02)
			return 0x80;
		else
			return 0x00;
		break;
	case INPT1:						/* offset 0x09 */
		if (input_port_1_r(0) & 0x08)
			return 0x80;
		else
			return 0x00;
		break;
	case INPT2:						/* offset 0x0A */
		if (input_port_1_r(0) & 0x01)
			return 0x80;
		else
			return 0x00;
		break;
	case INPT3:						/* offset 0x0B */
		if (input_port_1_r(0) & 0x04)
			return 0x80;
		else
			return 0x00;
		break;
	case INPT4:						/* offset 0x0C */
		if ((input_port_1_r(0) & 0x08) || (input_port_1_r(0) & 0x02))
			return 0x00;
		else
			return 0x80;
		break;
	case INPT5:						/* offset 0x0D */
		if ((input_port_1_r(0) & 0x01) || (input_port_1_r(0) & 0x04))
			return 0x00;
		else
			return 0x80;
		break;

	default:
		logerror("TIA_r undefined read %x PC %x\n", offset, pc);

	}
	return ROM[offset];

}


/***************************************************************************

  TIA Writes.

***************************************************************************/

void a2600_TIA_w(int offset, int data)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	unsigned int pc;

	//int forecolour;
	pc = cpu_get_pc();
	{
		UINT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		//  logerror("PF2 previous riot cycle %08x, current riot cycle %08x, difference %08x\n", previous_tia_cycle, global_tia_cycle + TIME_TO_CYCLES( 0, timer_timeelapsed(HSYNC_timer)), riotdiff);
		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		for (; riotdiff > 0; riotdiff--)
		{
			//  logerror("diff 0x%04x\n", riotdiff);
			a2600_Cycle_cb(0);
			//  logerror("tmr temp = 0x%04x\n", TMR_tmp);
		}
	}

	switch (offset)
	{

	case VSYNC:
		if (!(data & 0x02))
		{
			plot_pixel(stella_backbuffer, 69, currentline + Machine->visible_area.min_y, Machine->pens[flashycolour]);

			logerror("TIA_w - VSYNC Stop\n");

		}
		else if (data & 0x02)
		{
			logerror("TIA_w - VSYNC Start\n");
			plot_pixel(stella_backbuffer, 69, currentline + Machine->visible_area.min_y, Machine->pens[flashycolour]);
		}
		break;


	case VBLANK:						/* offset 0x01, bits 7,6 and 1 used */
		if (!(data & 0x02))
		{

			TIA_vblank = 0;
			plot_pixel(stella_backbuffer, 69, currentline + Machine->visible_area.min_y, Machine->pens[flashycolour]);
			flashycolour++;
			flashycolour = flashycolour & 0x0f;

			currentline = 0;
			copybitmap(stella_bitmap, stella_backbuffer, 0, 0, 0, 0, &stella_size, TRANSPARENCY_NONE, 0);


			logerror("TIA_w - VBLANK Stop\n");

		}
		else if (data & 0x02)
		{
			TIA_vblank = 1;
			plot_pixel(stella_backbuffer, 69, currentline + Machine->visible_area.min_y, Machine->pens[flashycolour]);
			logerror("TIA_w - VBLANK Start\n");

		}
		break;


	case WSYNC:						/* offset 0x02 */
		if (!(data & 0x01))
		{
			//  if (TIA_VERBOSE)
			logerror("TIA_w - WSYNC \n");
			timer_reset(HSYNC_timer, TIME_IN_CYCLES(76, 0));
			halted = 1;

			a2600_main_cb(0);

		}

		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - WSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
		}
		break;



	case RSYNC:						/* offset 0x03 */
		if (!(data & 0x01))
		{
			timer_reset(HSYNC_timer, TIME_IN_CYCLES(76, 0));
			if (TIA_VERBOSE)
				logerror("TIA_w - RSYNC \n");
		}
		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - RSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
		}
		break;



	case NUSIZ0:						/* offset 0x04 */
		msize0 = 2 ^ ((data & 0x30) >> 4);
		/* must implement player size checking! */

		break;


	case NUSIZ1:						/* offset 0x05 */
		msize1 = 2 ^ ((data * 0x30) >> 4);
		/* must implement player size checking! */

		break;


	case COLUP0:						/* offset 0x06 */

		tia.colreg.P0 = data;
		tia.colreg.M0 = tia.colreg.P0;	/* missile same color */
		if (TIA_VERBOSE)
			logerror("TIA_w - COLUP0 Write color is $%02x\n", tia.colreg.P0);
		break;

	case COLUP1:						/* offset 0x07 */

		tia.colreg.P1 = data;
		tia.colreg.M1 = tia.colreg.P1;	/* missile same color */
		if (TIA_VERBOSE)
			logerror("TIA_w - COLUP1 Write color is $%02x\n", tia.colreg.P1);
		break;

	case COLUPF:						/* offset 0x08 */

		tia.colreg.PF = data;
		tia.colreg.BL = data;			/* ball is same as playfield */
		if (TIA_VERBOSE)
			logerror("TIA_w - COLUPF Write color is $%02x\n", tia.colreg.PF);
		break;

	case COLUBK:						/* offset 0x09 */

		tia.colreg.BK = data;
		break;


	case CTRLPF:						/* offset 0x0A */


		PF_Ref = data & 0x01;
		PF_Score = (data & 0x02) >> 1;
		PF_Pfp = (data & 0x04) >> 2;

		break;



	case REFP0:
		if (!(data & 0x08))
		{
			TIA_player_0_reflect = 0;
			// if (TIA_VERBOSE)
			logerror("TIA_w - REFP0 No reflect \n");
		}
		else if (data & 0x08)
		{
			TIA_player_0_reflect = 1;
			//  if (TIA_VERBOSE)
			logerror("TIA_w - REFP0 Reflect \n");
		}
		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - Write Error, REFP0 offset $%02x & data $%02x\n", offset, data);
		}
		break;


	case REFP1:
		if (!(data & 0x08))
		{
			TIA_player_1_reflect = 0;
			// if (TIA_VERBOSE)
			logerror("TIA_w - REFP1 No reflect \n");
		}
		else if (data & 0x08)
		{
			TIA_player_1_reflect = 1;
			//  if (TIA_VERBOSE)
			logerror("TIA_w - REFP1 Reflect \n");
		}
		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - Write Error, REFP1 offset $%02x & data $%02x\n", offset, data);
		}
		break;

	case PF0:							/* 0x0D Playfield Register Byte 0 */

		tia.pfreg.B0 = data;
		PF0_Rendered = 1;

		break;

	case PF1:							/* 0x0E Playfield Register Byte 1 */
		tia.pfreg.B1 = data;
		PF1_Rendered = 1;

		break;

	case PF2:							/* 0x0F Playfield Register Byte 2 */
		tia.pfreg.B2 = data;
		PF2_Rendered = 1;

		break;


/* These next 5 Registers are Strobe registers            */
/* They will need to update the screen as soon as written */

	case RESP0:						/* 0x10 Reset Player 0 */
		logerror("Reset Player 0 0x%02x\n", HSYNC_Period);
		TIA_reset_player_0 = HSYNC_Period;
		break;

	case RESP1:						/* 0x11 Reset Player 1 */
		TIA_reset_player_1 = HSYNC_Period;
		break;

	case RESM0:						/* 0x12 Reset Missle 0 */
		break;

	case RESM1:						/* 0x13 Reset Missle 1 */
		break;

	case RESBL:						/* 0x14 Reset Ball */
		break;







	case AUDC0:						/* audio control */
	case AUDC1:						/* audio control */
	case AUDF0:						/* audio frequency */
	case AUDF1:						/* audio frequency */
	case AUDV0:						/* audio volume 0 */
	case AUDV1:						/* audio volume 1 */

		tia_w(offset, data);
		break;


	case GRP0:							/* 0x1B Graphics Register Player 0 */
		TIA_player_pattern_0 = data;
		TIA_player_pattern_0_tmp = data;
		break;

	case GRP1:							/* 0x1C Graphics Register Player 0 */
		TIA_player_pattern_1 = data;
		TIA_player_pattern_1_tmp = data;

		break;

	case ENAM0:						/* 0x1D Graphics Enable Missle 0 */
		break;

	case ENAM1:						/* 0x1E Graphics Enable Missle 1 */
		break;

	case ENABL:						/* 0x1F Graphics Enable Ball */
		break;

	case HMP0:							/* 0x20 Horizontal Motion Player 0 */
		logerror("Horizontal move write %02x\n", data);
		TIA_hmp0 = (data & 0xe0) >> 4;
		TIA_motion_player_0 = TIA_movement_table[TIA_hmp0];
		break;

	case HMP1:							/* 0x21 Horizontal Motion Player 1 */
		logerror("Horizontal move write %02x\n", data);
		TIA_hmp1 = (data & 0xe0) >> 4;
		TIA_motion_player_1 = TIA_movement_table[TIA_hmp1];

		break;

	case HMM0:							/* 0x22 Horizontal Motion Missle 0 */
		break;

	case HMM1:							/* 0x23 Horizontal Motion Missle 1 */
		break;

	case HMBL:							/* 0x24 Horizontal Motion Ball */
		break;

	case VDELP0:						/* 0x25 Vertical Delay Player 0 */
		break;

	case VDELP1:						/* 0x26 Vertical Delay Player 1 */
		break;

	case VDELBL:						/* 0x27 Vertical Delay Ball */
		break;

	case RESMP0:						/* 0x28 Reset Missle 0 to Player 0 */
		break;

	case RESMP1:						/* 0x29 Reset Missle 1 to Player 1 */
		break;

	case HMOVE:						/* 0x2A Apply Horizontal Motion */
		TIA_motion_player_0 = TIA_movement_table[TIA_hmp0];
		TIA_motion_player_1 = TIA_movement_table[TIA_hmp1];
		break;

	case HMCLR:						/* 0x2B Clear Horizontal Move Registers */
		TIA_motion_player_0 = 0;
		TIA_motion_player_1 = 0;
		break;

	case CXCLR:						/* 0x2C Clear Collision Latches */
		break;


		ROM[offset] = data;
		break;
	default:

		/* all others */
		ROM[offset] = data;
	}
}

void a2600_stop_machine(void)
{

}

int a2600_id_rom(int id)
{
	return 0;							/* no id possible */

}

int a2600_load_rom(int id)
{
	const char *rom_name = device_filename(IO_CARTSLOT, id);
	FILE *cartfile;
	UINT8 *ROM = memory_region(REGION_CPU1);

	if (rom_name == NULL)
	{
		printf("a2600 Requires Cartridge!\n");
		return INIT_FAILED;
	}

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (!(cartfile = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		return 1;
	}

	a2600_cartridge_rom = &(ROM[0x1000]);	/* 'plug' cart into 0x1000 */

	if (cartfile != NULL)
	{
		osd_fread(cartfile, a2600_cartridge_rom, 2048);		/* testing Combat for now */
		osd_fclose(cartfile);
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
{
	if ((stella_bitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0)
		return 1;
	if ((stella_backbuffer = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0)
		return 1;
	return 0;
}

void a2600_vh_stop(void)
{
	if (stella_bitmap)
		bitmap_free(stella_bitmap);
	stella_bitmap = NULL;
	if (stella_backbuffer)
		bitmap_free(stella_backbuffer);
	stella_bitmap = NULL;
}


/* when called, update the bitmap. */
static void a2600_scanline_cb(void)
{
	int regpos;

//pixpos;
	int xs = Machine->visible_area.min_x;	//68;

	//int ys = Machine->visible_area.max_x;//228;
	int yys = Machine->visible_area.min_y;	//68;

	int backcolor;

	//int forecolour;

	//logerror("Current Line = %x\n", currentline);


	/* plot the playfield and background for now               */
	/* each register value is 4 color clocks                   */
	/* to pick the color, need to bit check the playfield regs */

	/* set color to background */
	backcolor = tia.colreg.BK % Machine->drv->color_table_len;

	if (((currentline + yys) <= 299))	/*  && (TIA_vblank == 0) */
	{
		//  UINT8 pfpos = 0;
		/* now we have color, plot for 4 color cycles */
		for (regpos = 0; regpos < 160; regpos = regpos++)
		{
			int i = PF_Data[regpos] % Machine->drv->color_table_len;

			//logerror("Playfield pos 0x%02x, value 0x%02x\n", pfpos, PF_Data[pfpos]);


			plot_pixel(stella_backbuffer, regpos + xs, currentline + yys, Machine->pens[0]);

			if (i == 0)
			{
				plot_pixel(stella_backbuffer, regpos + xs, currentline + yys, Machine->pens[backcolor]);
			}
			else
			{
				plot_pixel(stella_backbuffer, regpos + xs, currentline + yys, Machine->pens[i]);
			}
		}
		currentline++;
	}

	PF0_Rendered = 0;
	PF1_Rendered = 0;
	PF2_Rendered = 0;
}

static void a2600_main_cb(int param)
{
	UINT32 riotdiff = (global_tia_cycle + 76) - previous_tia_cycle;

	//logerror("TIMER Read previous riot cycle %08x, current riot cycle %08x, difference %08x\n", previous_tia_cycle, global_tia_cycle + TIME_TO_CYCLES( 0, timer_timeelapsed(HSYNC_timer)), riotdiff);
	/* resync the riot timer */
	previous_tia_cycle = global_tia_cycle + 76;
	riotdiff *= 3;
	for (; riotdiff > 0; riotdiff--)
	{
		//  logerror("diff 0x%04x\n", riotdiff);
		a2600_Cycle_cb(0);
		//  logerror("tmr temp = 0x%04x\n", TMR_tmp);
	}

	a2600_scanline_cb();
	global_tia_cycle += 76;

}

static void a2600_Cycle_cb(int param)
{
	int forecolour;

	//logerror("counting Cycles 0x%08x....\n", a2600_Cycle_Count);
	a2600_Cycle_Count++;

	/* Hsync Timing */

	HSYNC_Period++;
	HSYNC_colour_clock--;
	/* raster stuff. it should hopefully be in sync :) */
	if (PF_Score)
	{
		forecolour = tia.colreg.P0;
	}
	else
	{
		forecolour = tia.colreg.PF;
	}

	if (HSYNC_Period == 68)
	{
		PF_Data[0] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
	}

	if (HSYNC_Period == 69)
	{
		PF_Data[1] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
	}

	if (HSYNC_Period == 70)
	{
		PF_Data[2] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
	}

	if (HSYNC_Period == 71)
	{
		PF_Data[3] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
	}

	if (HSYNC_Period == 72)
	{
		PF_Data[4] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
	}

	if (HSYNC_Period == 73)
	{
		PF_Data[5] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
	}

	if (HSYNC_Period == 74)
	{
		PF_Data[6] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
	}

	if (HSYNC_Period == 75)
	{
		PF_Data[7] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
	}

	if (HSYNC_Period == 76)
	{
		PF_Data[8] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
	}

	if (HSYNC_Period == 77)
	{
		PF_Data[9] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
	}

	if (HSYNC_Period == 78)
	{
		PF_Data[10] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
	}

	if (HSYNC_Period == 79)
	{
		PF_Data[11] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
	}

	if (HSYNC_Period == 80)
	{
		PF_Data[12] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
	}

	if (HSYNC_Period == 81)
	{
		PF_Data[13] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
	}

	if (HSYNC_Period == 82)
	{
		PF_Data[14] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
	}

	if (HSYNC_Period == 83)
	{
		PF_Data[15] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
	}

	if (HSYNC_Period == 84)
	{
		PF_Data[16] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
	}

	if (HSYNC_Period == 85)
	{
		PF_Data[17] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
	}

	if (HSYNC_Period == 86)
	{
		PF_Data[18] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
	}

	if (HSYNC_Period == 87)
	{
		PF_Data[19] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
	}

	if (HSYNC_Period == 88)
	{
		PF_Data[20] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
	}

	if (HSYNC_Period == 89)
	{
		PF_Data[21] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
	}

	if (HSYNC_Period == 90)
	{
		PF_Data[22] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
	}

	if (HSYNC_Period == 91)
	{
		PF_Data[23] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
	}

	if (HSYNC_Period == 92)
	{
		PF_Data[24] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
	}

	if (HSYNC_Period == 93)
	{
		PF_Data[25] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
	}

	if (HSYNC_Period == 94)
	{
		PF_Data[26] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
	}

	if (HSYNC_Period == 95)
	{
		PF_Data[27] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
	}

	if (HSYNC_Period == 96)
	{
		PF_Data[28] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
	}

	if (HSYNC_Period == 97)
	{
		PF_Data[29] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
	}

	if (HSYNC_Period == 98)
	{
		PF_Data[30] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
	}

	if (HSYNC_Period == 99)
	{
		PF_Data[31] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
	}

	if (HSYNC_Period == 100)
	{
		PF_Data[32] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
	}

	if (HSYNC_Period == 101)
	{
		PF_Data[33] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
	}

	if (HSYNC_Period == 102)
	{
		PF_Data[34] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
	}

	if (HSYNC_Period == 103)
	{
		PF_Data[35] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
	}

	if (HSYNC_Period == 104)
	{
		PF_Data[36] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
	}

	if (HSYNC_Period == 105)
	{
		PF_Data[37] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
	}

	if (HSYNC_Period == 106)
	{
		PF_Data[38] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
	}

	if (HSYNC_Period == 107)
	{
		PF_Data[39] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
	}

	if (HSYNC_Period == 108)
	{
		PF_Data[40] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
	}

	if (HSYNC_Period == 109)
	{
		PF_Data[41] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
	}

	if (HSYNC_Period == 110)
	{
		PF_Data[42] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
	}

	if (HSYNC_Period == 111)
	{
		PF_Data[43] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
	}

	if (HSYNC_Period == 112)
	{
		PF_Data[44] = forecolour * (tia.pfreg.B1 & 0x01);
	}

	if (HSYNC_Period == 113)
	{
		PF_Data[45] = forecolour * (tia.pfreg.B1 & 0x01);
	}

	if (HSYNC_Period == 114)
	{
		PF_Data[46] = forecolour * (tia.pfreg.B1 & 0x01);
	}

	if (HSYNC_Period == 115)
	{
		PF_Data[47] = forecolour * (tia.pfreg.B1 & 0x01);
	}

	if (HSYNC_Period == 116)
	{
		PF_Data[48] = forecolour * (tia.pfreg.B2 & 0x01);
	}

	if (HSYNC_Period == 117)
	{
		PF_Data[49] = forecolour * (tia.pfreg.B2 & 0x01);
	}

	if (HSYNC_Period == 118)
	{
		PF_Data[50] = forecolour * (tia.pfreg.B2 & 0x01);
	}

	if (HSYNC_Period == 119)
	{
		PF_Data[51] = forecolour * (tia.pfreg.B2 & 0x01);
	}

	if (HSYNC_Period == 120)
	{
		PF_Data[52] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
	}

	if (HSYNC_Period == 121)
	{
		PF_Data[53] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
	}

	if (HSYNC_Period == 122)
	{
		PF_Data[54] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
	}

	if (HSYNC_Period == 123)
	{
		PF_Data[55] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
	}

	if (HSYNC_Period == 124)
	{
		PF_Data[56] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
	}

	if (HSYNC_Period == 125)
	{
		PF_Data[57] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
	}

	if (HSYNC_Period == 126)
	{
		PF_Data[58] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
	}

	if (HSYNC_Period == 127)
	{
		PF_Data[59] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
	}

	if (HSYNC_Period == 128)
	{
		PF_Data[60] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
	}

	if (HSYNC_Period == 129)
	{
		PF_Data[61] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
	}

	if (HSYNC_Period == 130)
	{
		PF_Data[62] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
	}

	if (HSYNC_Period == 131)
	{
		PF_Data[63] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
	}

	if (HSYNC_Period == 132)
	{
		PF_Data[64] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
	}

	if (HSYNC_Period == 133)
	{
		PF_Data[65] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
	}

	if (HSYNC_Period == 134)
	{
		PF_Data[66] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
	}

	if (HSYNC_Period == 135)
	{
		PF_Data[67] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
	}

	if (HSYNC_Period == 136)
	{
		PF_Data[68] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
	}

	if (HSYNC_Period == 137)
	{
		PF_Data[69] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);

	}

	if (HSYNC_Period == 138)
	{
		PF_Data[70] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);

	}

	if (HSYNC_Period == 139)
	{
		PF_Data[71] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);

	}

	if (HSYNC_Period == 140)
	{
		PF_Data[72] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);

	}

	if (HSYNC_Period == 141)
	{
		PF_Data[73] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
	}

	if (HSYNC_Period == 142)
	{
		PF_Data[74] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
	}

	if (HSYNC_Period == 143)
	{
		PF_Data[75] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
	}

	if (HSYNC_Period == 144)
	{
		PF_Data[76] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
	}

	if (HSYNC_Period == 145)
	{
		PF_Data[77] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
	}

	if (HSYNC_Period == 146)
	{
		PF_Data[78] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
	}

	if (HSYNC_Period == 147)
	{
		PF_Data[79] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
	}

	if (PF_Score)
	{
		forecolour = tia.colreg.P1;
	}
	else
	{
		forecolour = tia.colreg.PF;
	}



	if (HSYNC_Period == 148)
	{
		if (PF_Ref)
		{
			PF_Data[80] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
		else
		{
			PF_Data[80] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 149)
	{
		if (PF_Ref)
		{
			PF_Data[81] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
		else
		{
			PF_Data[81] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}

	}

	if (HSYNC_Period == 150)
	{
		if (PF_Ref)
		{
			PF_Data[82] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
		else
		{
			PF_Data[82] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}

	}

	if (HSYNC_Period == 151)
	{
		if (PF_Ref)
		{
			PF_Data[83] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
		else
		{
			PF_Data[83] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}

	}

	if (HSYNC_Period == 152)
	{
		if (PF_Ref)
		{
			PF_Data[84] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
		else
		{
			PF_Data[84] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}

	}

	if (HSYNC_Period == 153)
	{
		if (PF_Ref)
		{
			PF_Data[85] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
		else
		{
			PF_Data[85] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 154)
	{
		if (PF_Ref)
		{
			PF_Data[86] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
		else
		{
			PF_Data[86] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 155)
	{
		if (PF_Ref)
		{
			PF_Data[87] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
		else
		{
			PF_Data[87] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 156)
	{
		if (PF_Ref)
		{
			PF_Data[88] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
		else
		{
			PF_Data[88] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 157)
	{
		if (PF_Ref)
		{
			PF_Data[89] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
		else
		{
			PF_Data[89] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 158)
	{
		if (PF_Ref)
		{
			PF_Data[90] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
		else
		{
			PF_Data[90] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 159)
	{
		if (PF_Ref)
		{
			PF_Data[91] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
		else
		{
			PF_Data[91] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 160)
	{
		if (PF_Ref)
		{
			PF_Data[92] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
		else
		{
			PF_Data[92] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 161)
	{
		if (PF_Ref)
		{
			PF_Data[93] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
		else
		{
			PF_Data[93] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 162)
	{
		if (PF_Ref)
		{
			PF_Data[94] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
		else
		{
			PF_Data[94] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 163)
	{
		if (PF_Ref)
		{
			PF_Data[95] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
		else
		{
			PF_Data[95] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 164)
	{
		if (PF_Ref)
		{
			PF_Data[96] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
		else
		{
			PF_Data[96] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 165)
	{
		if (PF_Ref)
		{
			PF_Data[97] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
		else
		{
			PF_Data[97] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 166)
	{
		if (PF_Ref)
		{
			PF_Data[98] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
		else
		{
			PF_Data[98] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 167)
	{
		if (PF_Ref)
		{
			PF_Data[99] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
		else
		{
			PF_Data[99] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 168)
	{
		if (PF_Ref)
		{
			PF_Data[100] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
		else
		{
			PF_Data[100] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 169)
	{
		if (PF_Ref)
		{
			PF_Data[101] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
		else
		{
			PF_Data[101] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 170)
	{
		if (PF_Ref)
		{
			PF_Data[102] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
		else
		{
			PF_Data[102] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 171)
	{
		if (PF_Ref)
		{
			PF_Data[103] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
		else
		{
			PF_Data[103] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 172)
	{
		if (PF_Ref)
		{
			PF_Data[104] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
		else
		{
			PF_Data[104] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 173)
	{
		if (PF_Ref)
		{
			PF_Data[105] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
		else
		{
			PF_Data[105] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 174)
	{
		if (PF_Ref)
		{
			PF_Data[106] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
		else
		{
			PF_Data[106] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 175)
	{
		if (PF_Ref)
		{
			PF_Data[107] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
		else
		{
			PF_Data[107] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 176)
	{
		if (PF_Ref)
		{
			PF_Data[108] = forecolour * (tia.pfreg.B2 & 0x01);
		}
		else
		{
			PF_Data[108] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 177)
	{
		if (PF_Ref)
		{
			PF_Data[109] = forecolour * (tia.pfreg.B2 & 0x01);
		}
		else
		{
			PF_Data[109] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 178)
	{
		if (PF_Ref)
		{
			PF_Data[110] = forecolour * (tia.pfreg.B2 & 0x01);
		}
		else
		{
			PF_Data[110] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 179)
	{
		if (PF_Ref)
		{
			PF_Data[111] = forecolour * (tia.pfreg.B2 & 0x01);
		}
		else
		{
			PF_Data[111] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 180)
	{
		if (PF_Ref)
		{
			PF_Data[112] = forecolour * (tia.pfreg.B1 & 0x01);
		}
		else
		{
			PF_Data[112] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 181)
	{
		if (PF_Ref)
		{
			PF_Data[113] = forecolour * (tia.pfreg.B1 & 0x01);
		}
		else
		{
			PF_Data[113] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 182)
	{
		if (PF_Ref)
		{
			PF_Data[114] = forecolour * (tia.pfreg.B1 & 0x01);
		}
		else
		{
			PF_Data[114] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 183)
	{
		if (PF_Ref)
		{
			PF_Data[115] = forecolour * (tia.pfreg.B1 & 0x01);
		}
		else
		{
			PF_Data[115] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 184)
	{
		if (PF_Ref)
		{
			PF_Data[116] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
		else
		{
			PF_Data[116] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 185)
	{
		if (PF_Ref)
		{
			PF_Data[117] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
		else
		{
			PF_Data[117] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 186)
	{
		if (PF_Ref)
		{
			PF_Data[118] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
		else
		{
			PF_Data[118] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 187)
	{
		if (PF_Ref)
		{
			PF_Data[119] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
		else
		{
			PF_Data[119] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 188)
	{
		if (PF_Ref)
		{
			PF_Data[120] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
		else
		{
			PF_Data[120] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 189)
	{
		if (PF_Ref)
		{
			PF_Data[121] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
		else
		{
			PF_Data[121] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 190)
	{
		if (PF_Ref)
		{
			PF_Data[122] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
		else
		{
			PF_Data[122] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 191)
	{
		if (PF_Ref)
		{
			PF_Data[123] = forecolour * ((tia.pfreg.B1 & 0x04) >> 2);
		}
		else
		{
			PF_Data[123] = forecolour * ((tia.pfreg.B1 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 192)
	{
		if (PF_Ref)
		{
			PF_Data[124] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
		else
		{
			PF_Data[124] = forecolour * (tia.pfreg.B1 & 0x01);
		}
	}

	if (HSYNC_Period == 193)
	{
		if (PF_Ref)
		{
			PF_Data[125] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
		else
		{
			PF_Data[125] = forecolour * (tia.pfreg.B1 & 0x01);
		}
	}

	if (HSYNC_Period == 194)
	{
		if (PF_Ref)
		{
			PF_Data[126] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
		else
		{
			PF_Data[126] = forecolour * (tia.pfreg.B1 & 0x01);
		}
	}

	if (HSYNC_Period == 195)
	{
		if (PF_Ref)
		{
			PF_Data[127] = forecolour * ((tia.pfreg.B1 & 0x08) >> 3);
		}
		else
		{
			PF_Data[127] = forecolour * (tia.pfreg.B1 & 0x01);
		}
	}

	if (HSYNC_Period == 196)
	{
		if (PF_Ref)
		{
			PF_Data[128] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
		else
		{
			PF_Data[128] = forecolour * (tia.pfreg.B2 & 0x01);
		}
	}

	if (HSYNC_Period == 197)
	{
		if (PF_Ref)
		{
			PF_Data[129] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
		else
		{
			PF_Data[129] = forecolour * (tia.pfreg.B2 & 0x01);
		}
	}

	if (HSYNC_Period == 198)
	{
		if (PF_Ref)
		{
			PF_Data[130] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
		else
		{
			PF_Data[130] = forecolour * (tia.pfreg.B2 & 0x01);
		}
	}

	if (HSYNC_Period == 199)
	{
		if (PF_Ref)
		{
			PF_Data[131] = forecolour * ((tia.pfreg.B1 & 0x10) >> 4);
		}
		else
		{
			PF_Data[131] = forecolour * (tia.pfreg.B2 & 0x01);
		}
	}

	if (HSYNC_Period == 200)
	{
		if (PF_Ref)
		{
			PF_Data[132] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
		else
		{
			PF_Data[132] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 201)
	{
		if (PF_Ref)
		{
			PF_Data[133] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
		else
		{
			PF_Data[133] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 202)
	{
		if (PF_Ref)
		{
			PF_Data[134] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
		else
		{
			PF_Data[134] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 203)
	{
		if (PF_Ref)
		{
			PF_Data[135] = forecolour * ((tia.pfreg.B1 & 0x20) >> 5);
		}
		else
		{
			PF_Data[135] = forecolour * ((tia.pfreg.B2 & 0x02) >> 1);
		}
	}

	if (HSYNC_Period == 204)
	{
		if (PF_Ref)
		{
			PF_Data[136] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
		else
		{
			PF_Data[136] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 205)
	{
		if (PF_Ref)
		{
			PF_Data[137] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
		else
		{
			PF_Data[137] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 206)
	{
		if (PF_Ref)
		{
			PF_Data[138] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
		else
		{
			PF_Data[138] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 207)
	{
		if (PF_Ref)
		{
			PF_Data[139] = forecolour * ((tia.pfreg.B1 & 0x40) >> 6);
		}
		else
		{
			PF_Data[139] = forecolour * ((tia.pfreg.B2 & 0x04) >> 2);
		}
	}

	if (HSYNC_Period == 208)
	{
		if (PF_Ref)
		{
			PF_Data[140] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
		else
		{
			PF_Data[140] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 209)
	{
		if (PF_Ref)
		{
			PF_Data[141] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
		else
		{
			PF_Data[141] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 210)
	{
		if (PF_Ref)
		{
			PF_Data[142] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
		else
		{
			PF_Data[142] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 211)
	{
		if (PF_Ref)
		{
			PF_Data[143] = forecolour * ((tia.pfreg.B1 & 0x80) >> 7);
		}
		else
		{
			PF_Data[143] = forecolour * ((tia.pfreg.B2 & 0x08) >> 3);
		}
	}

	if (HSYNC_Period == 212)
	{
		if (PF_Ref)
		{
			PF_Data[144] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
		else
		{
			PF_Data[144] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 213)
	{
		if (PF_Ref)
		{
			PF_Data[145] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
		else
		{
			PF_Data[145] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 214)
	{
		if (PF_Ref)
		{
			PF_Data[146] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
		else
		{
			PF_Data[146] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 215)
	{
		if (PF_Ref)
		{
			PF_Data[147] = forecolour * ((tia.pfreg.B0 & 0x80) >> 7);
		}
		else
		{
			PF_Data[147] = forecolour * ((tia.pfreg.B2 & 0x10) >> 4);
		}
	}

	if (HSYNC_Period == 216)
	{
		if (PF_Ref)
		{
			PF_Data[148] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
		else
		{
			PF_Data[148] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 217)
	{
		if (PF_Ref)
		{
			PF_Data[149] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
		else
		{
			PF_Data[149] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 218)
	{
		if (PF_Ref)
		{
			PF_Data[150] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
		else
		{
			PF_Data[150] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 219)
	{
		if (PF_Ref)
		{
			PF_Data[151] = forecolour * ((tia.pfreg.B0 & 0x40) >> 6);
		}
		else
		{
			PF_Data[151] = forecolour * ((tia.pfreg.B2 & 0x20) >> 5);
		}
	}

	if (HSYNC_Period == 220)
	{
		if (PF_Ref)
		{
			PF_Data[152] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
		else
		{
			PF_Data[152] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
	}


	if (HSYNC_Period == 221)
	{
		if (PF_Ref)
		{
			PF_Data[153] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
		else
		{
			PF_Data[153] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
	}

	if (HSYNC_Period == 222)
	{
		if (PF_Ref)
		{
			PF_Data[154] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
		else
		{
			PF_Data[154] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
	}


	if (HSYNC_Period == 223)
	{
		if (PF_Ref)
		{
			PF_Data[155] = forecolour * ((tia.pfreg.B0 & 0x20) >> 5);
		}
		else
		{
			PF_Data[155] = forecolour * ((tia.pfreg.B2 & 0x40) >> 6);
		}
	}


	if (HSYNC_Period == 224)
	{
		if (PF_Ref)
		{
			PF_Data[156] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}
		else
		{
			PF_Data[156] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 225)
	{
		if (PF_Ref)
		{
			PF_Data[157] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}
		else
		{
			PF_Data[157] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 226)
	{
		if (PF_Ref)
		{
			PF_Data[158] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}
		else
		{
			PF_Data[158] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
	}

	if (HSYNC_Period == 227)
	{
		if (PF_Ref)
		{
			PF_Data[159] = forecolour * ((tia.pfreg.B0 & 0x10) >> 4);
		}
		else
		{
			PF_Data[159] = forecolour * ((tia.pfreg.B2 & 0x80) >> 7);
		}
	}

	if ((HSYNC_Period - 1) == 0)
	{
		TIA_player_pattern_0_tmp = TIA_player_pattern_0;
		TIA_player_pattern_1_tmp = TIA_player_pattern_1;


	}
/* Player 0 stuff */
	if ((TIA_reset_player_0) && (((HSYNC_Period - 1) >= (TIA_reset_player_0 + TIA_motion_player_0)) && ((HSYNC_Period - 1) <=
																															   ((TIA_reset_player_0
																																+
																															   7)
																															   + TIA_motion_player_0))))
	{
		logerror("motion player 0 %d \n", TIA_motion_player_0);
		if (!TIA_player_0_reflect)
		{
			if ((TIA_player_pattern_0_tmp & 0x80))
				PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
			//  TIA_reset_player_0 = 0;
			logerror("Hsync Period = %02x\n", HSYNC_Period);
			TIA_player_pattern_0_tmp = TIA_player_pattern_0_tmp << 1;
			if (!TIA_player_0_tick)
			{
				TIA_player_pattern_0_tmp = TIA_player_pattern_0;
				TIA_player_0_tick = 8;
			}
		}
		else
		{
			if ((TIA_player_pattern_0_tmp & 0x01))
				PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
			//  TIA_reset_player_0 = 0;
			logerror("Hsync Period = %02x\n", HSYNC_Period);
			TIA_player_pattern_0_tmp = TIA_player_pattern_0_tmp >> 1;
			if (!TIA_player_0_tick)
			{
				TIA_player_pattern_0_tmp = TIA_player_pattern_0;
				TIA_player_0_tick = 8;
			}
		}
	}

/* Player 1 stuff */
	if ((TIA_reset_player_1) && (((HSYNC_Period - 1) >= (TIA_reset_player_1 + TIA_motion_player_1)) && ((HSYNC_Period - 1) <=
																															   ((TIA_reset_player_1
																																+
																															   7)
																															   + TIA_motion_player_1))))
	{
		logerror("motion player 1 %d \n", TIA_motion_player_1);
		if (!TIA_player_1_reflect)
		{
			if ((TIA_player_pattern_1_tmp & 0x80))
				PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
			//  TIA_reset_player_1 = 0;
			logerror("Hsync Period = %02x\n", HSYNC_Period);
			TIA_player_pattern_1_tmp = TIA_player_pattern_1_tmp << 1;
			if (!TIA_player_1_tick)
			{
				TIA_player_pattern_1_tmp = TIA_player_pattern_1;
				TIA_player_1_tick = 8;
			}
		}
		else
		{
			if ((TIA_player_pattern_1_tmp & 0x01))
				PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
			//  TIA_reset_player_1 = 0;
			logerror("Hsync Period = %02x\n", HSYNC_Period);
			TIA_player_pattern_1_tmp = TIA_player_pattern_1_tmp >> 1;
			if (!TIA_player_1_tick)
			{
				TIA_player_pattern_1_tmp = TIA_player_pattern_1;
				TIA_player_1_tick = 8;
			}
		}
	}

	if (HSYNC_Period >= 228)
	{
		HSYNC_Period = 0;
	}

	/* Deal with the RIOT Timer */
	if (!HSYNC_colour_clock)
	{
		if (TMR_Period)
		{
			TMR_tmp--;
			if (!TMR_tmp)
			{
				TMR_tmp = TMR_Period;
				TMR_Intim--;
				if (TMR_Intim == 0)
				{
					TMR_Period = 1;		/* Deals with the zero passing stuff */
				}
			}
		}
		HSYNC_colour_clock = 3;
	}
}


void a2600_init_machine(void)
{

	/* start RIOT interface */
	///riot_init(&a2600_riot);
	cpu_current_state = 1;
	currentline = 0;
	HSYNC_timer = timer_pulse(TIME_IN_CYCLES(76, 0), 0, a2600_main_cb);
	return;

}

/***************************************************************************

  Refresh the video screen

***************************************************************************/
/* This routine is called at the start of vblank to refresh the screen */
void a2600_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	copybitmap(bitmap, stella_bitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}
