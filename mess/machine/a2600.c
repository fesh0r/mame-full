/***************************************************************************

	          The book of Revelations according to LollypopMan

                                  aka
                      Machine functions for the a2600
                  The Blaggers Guide to Emu Programming

              Thanks to Cowering for the research efforts ;)

                         TODO: Better Comments ;)

***************************************************************************/


#include "driver.h"
#include "sound/tiaintf.h"
#include "sound/tiasound.h"
#include "cpuintrf.h"
#include "machine/tia.h"
#include "drawgfx.h"
#include "zlib.h"
#include "vidhrdw/generic.h"
#include "image.h"

#include "includes/a2600.h"

/* This code is not to be used yet */
//#define USE_SCANLINE_WSYNC


UINT8 Bankswitch_Method = 0;
static union {
		UINT8 pf[4];
		UINT32 shiftreg;
	} TIA_playfield;

static union {
		UINT8 pf[4];
		UINT32 shiftreg;
	} TIA_pf_mask;

UINT8 TIA_pixel_clock = 0;
UINT8 TIA_player_0_finished = 0;
UINT8 TIA_player_1_finished = 0;

/* to make the shift regs easier to deal with */
/*         have fun Mac guys.....;)           */

#define SHIFT_LEFT20(x) { unsigned char b = (x.pf[2] & 0x08) >> 3; x.shiftreg = (x.shiftreg << 1) | b; x.pf[2] = x.pf[2] & 0x0f; }
#define SHIFT_RIGHT20(x) { unsigned char b = (x.pf[0] & 0x01) << 3; x.shiftreg = x.shiftreg >> 1; x.pf[2] = (x.pf[2] | b) & 0x0f; }
#define SHIFT_LEFT8(x) { unsigned char b = (x & 0x80) >> 7; x = (x << 1) | b; }
#define SHIFT_RIGHT8(x) { unsigned char b = (x & 0x01) << 7; x = (x >> 1) | b; }
#define SHIFT_RIGHT10(x) { UINT16 b = (x & 0x01) << 9; x = (x >> 1) | b; }
UINT8 TIA_player_0_block = 0;
UINT8 TIA_player_1_block = 0;
UINT8 bit_flip_table[] = {
		0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,
		0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
		0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,
		0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
		0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,
		0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
		0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,
		0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
		0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,
		0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
		0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,
		0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
		0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,
		0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
		0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,
		0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
		0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,
		0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
		0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,
		0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
		0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,
		0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
		0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,
		0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
		0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,
		0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
		0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,
		0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
		0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,
		0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
		0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,
		0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff
};

/* Back buffer */
struct rectangle stella_size = {0, 227, 0, 261};

/* for detailed logging */
#define TIA_VERBOSE 0
#define RIOT_VERBOSE 0

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
static char TIA_movement_table[] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};
static char TIA_motion_player_0 = 0;
static char TIA_motion_player_1 = 0;
UINT8 TIA_vblank;
UINT32 global_tia_cycle = 0;
UINT32 previous_tia_cycle = 0;

/* player graphics shift registers */
UINT8 TIA_player_0_mask_bit = 10;
UINT8 TIA_player_1_mask_bit = 10;
UINT16 TIA_player_mask[] = { 0x0001, 0x0005, 0x0011, 0x0015, 0x1001, 0x0003, 0x1011, 0x000f };
UINT8 TIA_player_pixel_delay[] = { 1, 1, 1, 1, 1, 2, 1, 4 };

UINT16 TIA_player_0_mask = 0;
UINT8 TIA_player_0_pixel_delay = 0;

UINT16 TIA_player_1_mask = 0;
UINT8 TIA_player_1_pixel_delay = 0;

UINT16 TIA_player_0_mask_tmp = 0;
UINT8 TIA_player_0_pixel_delay_tmp = 0;

UINT16 TIA_player_1_mask_tmp = 0;
UINT8 TIA_player_1_pixel_delay_tmp = 0;

UINT8 TIA_player_0_offset = 0;
UINT8 TIA_player_1_offset = 0;
UINT16 HSYNC_Period = 0;
UINT16 TIA_reset_player_0 = 0;
UINT16 TIA_reset_player_1 = 0;
UINT8 TIA_player_pattern_0 = 0;
UINT8 TIA_player_pattern_0_delayed = 0;
UINT8 TIA_player_pattern_0_tmp = 0;
UINT8 TIA_player_delay_0 = 0;
UINT8 TIA_player_pattern_1 = 0;
UINT8 TIA_player_pattern_1_delayed = 0;
UINT8 TIA_player_pattern_1_tmp = 0;
UINT8 TIA_player_delay_1 = 0;

UINT8 HSYNC_colour_clock = 3;

static void a2600_main_cb(int param);

static void a2600_scanline_cb(void);
static void a2600_Cycle_cb(int riotdiff);

static void *HSYNC_timer;

static int msize0 = 0;
static int msize1 = 0;

/* local */
static unsigned char *a2600_cartridge_rom;

/* scanline */
static int currentline = 0;

#ifdef USE_SCANLINE_WSYNC
/* wsync */
#define WSYNC_TRIGGER 2600
static int TIA_wsync = 0;
#endif

/***************************************************************************

  Bankswitching stuff

***************************************************************************/
READ_HANDLER  ( a2600_bs_r )
{
	UINT8 value;
	UINT16 address = offset + 0x1f00;
	UINT8 *ROM = memory_region(REGION_CPU1);


	value = ROM[address];

	switch(Bankswitch_Method)
	{
		case 0xF8:
					switch( address )
					{
						case 0x1ff8:
									memcpy(&ROM[0x1000], &ROM[0x10000], 0x1000);
									memcpy(&ROM[0xf000], &ROM[0x10000], 0x1000);
									break;
						case 0x1ff9:
									memcpy(&ROM[0x1000], &ROM[0x11000], 0x1000);
									memcpy(&ROM[0xf000], &ROM[0x11000], 0x1000);
									break;
					}
					break;
		case 0xe0:
					switch( address )
					{
						case 0x1fe0:
									memcpy(&ROM[0x1000], &ROM[0x10000], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x10000], 0x0400);
									break;
						case 0x1fe1:
									memcpy(&ROM[0x1000], &ROM[0x10400], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x10400], 0x0400);
									break;
						case 0x1fe2:
									memcpy(&ROM[0x1000], &ROM[0x10800], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x10800], 0x0400);
									break;
						case 0x1fe3:
									memcpy(&ROM[0x1000], &ROM[0x10c00], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x10c00], 0x0400);
									break;
						case 0x1fe4:
									memcpy(&ROM[0x1000], &ROM[0x11000], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x11000], 0x0400);
									break;
						case 0x1fe5:
									memcpy(&ROM[0x1000], &ROM[0x11400], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x11400], 0x0400);
									break;
						case 0x1fe6:
									memcpy(&ROM[0x1000], &ROM[0x11800], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x11800], 0x0400);
									break;
						case 0x1fe7:
									memcpy(&ROM[0x1000], &ROM[0x11c00], 0x0400);
									memcpy(&ROM[0xf000], &ROM[0x11c00], 0x0400);
									break;


						case 0x1fe8:
									memcpy(&ROM[0x1400], &ROM[0x10000], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x10000], 0x0400);
									break;
						case 0x1fe9:
									memcpy(&ROM[0x1400], &ROM[0x10400], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x10400], 0x0400);
									break;
						case 0x1fea:
									memcpy(&ROM[0x1400], &ROM[0x10800], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x10800], 0x0400);
									break;
						case 0x1feb:
									memcpy(&ROM[0x1400], &ROM[0x10c00], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x10c00], 0x0400);
									break;
						case 0x1fec:
									memcpy(&ROM[0x1400], &ROM[0x11000], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x11000], 0x0400);
									break;
						case 0x1fed:
									memcpy(&ROM[0x1400], &ROM[0x11400], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x11400], 0x0400);
									break;
						case 0x1fee:
									memcpy(&ROM[0x1400], &ROM[0x11800], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x11800], 0x0400);
									break;
						case 0x1fef:
									memcpy(&ROM[0x1400], &ROM[0x11c00], 0x0400);
									memcpy(&ROM[0xf400], &ROM[0x11c00], 0x0400);
									break;


						case 0x1ff0:
									memcpy(&ROM[0x1800], &ROM[0x10000], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x10000], 0x0400);
									break;
						case 0x1ff1:
									memcpy(&ROM[0x1800], &ROM[0x10400], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x10400], 0x0400);
									break;
						case 0x1ff2:
									memcpy(&ROM[0x1800], &ROM[0x10800], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x10800], 0x0400);
									break;
						case 0x1ff3:
									memcpy(&ROM[0x1800], &ROM[0x10c00], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x10c00], 0x0400);
									break;
						case 0x1ff4:
									memcpy(&ROM[0x1800], &ROM[0x11000], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x11000], 0x0400);
									break;
						case 0x1ff5:
									memcpy(&ROM[0x1800], &ROM[0x11400], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x11400], 0x0400);
									break;
						case 0x1ff6:
									memcpy(&ROM[0x1800], &ROM[0x11800], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x11800], 0x0400);
									break;
						case 0x1ff7:
									memcpy(&ROM[0x1800], &ROM[0x11c00], 0x0400);
									memcpy(&ROM[0xf800], &ROM[0x11c00], 0x0400);
									break;
				}
	}
	return value;
}

/***************************************************************************

  RIOT Reads.

***************************************************************************/
READ_HANDLER( a2600_riot_r )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	INT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;
	/* resync */
	previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
	riotdiff *= 3;
	a2600_Cycle_cb(riotdiff);

	switch (offset)
	{
	case 0x00:
		return readinputport(0);

	case 0x01:
		return readinputport(1);

	case 0x02:
		return readinputport(2);

	case 0x04:							/*TIMER READ */
		return TMR_Intim;

	}

	return ROM[offset];
}

/***************************************************************************

  RIOT Writes.

***************************************************************************/
WRITE_HANDLER( a2600_riot_w )
{

	UINT8 *ROM = memory_region(REGION_CPU1);
	{
		INT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		a2600_Cycle_cb(riotdiff);
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

	ROM[offset] = data;
}


/***************************************************************************

  TIA Reads.

***************************************************************************/
READ_HANDLER( a2600_TIA_r )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	unsigned int pc;

	pc = activecpu_get_pc();
	{
		INT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		a2600_Cycle_cb(riotdiff);
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
WRITE_HANDLER( a2600_TIA_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	unsigned int pc;

	pc = activecpu_get_pc();
	{
		INT32 riotdiff = (global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer))) - previous_tia_cycle;

		/* resync the riot timer */
		previous_tia_cycle = global_tia_cycle + TIME_TO_CYCLES(0, timer_timeelapsed(HSYNC_timer));
		riotdiff *= 3;
		a2600_Cycle_cb(riotdiff);
	}

	switch (offset)
	{

	case VSYNC:
		if (!(data & 0x02))
		{
			logerror("TIA_w - VSYNC Stop\n");

		}
		else if (data & 0x02)
		{
			logerror("TIA_w - VSYNC Start\n");
			currentline = 0;
		}
		break;


	case VBLANK:						/* offset 0x01, bits 7,6 and 1 used */
		if (!(data & 0x02))
		{

			TIA_vblank = 0;
			logerror("TIA_w - VBLANK Stop\n");

		}
		else if (data & 0x02)
		{
			TIA_vblank = 1;
			logerror("TIA_w - VBLANK Start\n");

		}
		break;


	case WSYNC:						/* offset 0x02 */
		if (!(data & 0x01))
		{
			logerror("TIA_w - WSYNC \n");
		#ifndef USE_SCANLINE_WSYNC
			timer_reset(HSYNC_timer, TIME_IN_CYCLES(76, 0));
			a2600_main_cb(0);
		#else
			TIA_wsync = 1;
			cpu_spinuntil_trigger(WSYNC_TRIGGER);
		#endif
		}

		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - WSYNC Write Error! offset $%02x & data $%02x\n", offset, data);
		}
		return;



	case RSYNC:						/* offset 0x03 */
		if (!(data & 0x01))
		{
			timer_reset(HSYNC_timer, TIME_IN_CYCLES(76, 0));
			previous_tia_cycle = 0;
			HSYNC_Period = 0;
			TIA_pixel_clock = 0;
			TIA_pf_mask.shiftreg = 0x080000;
			TIA_player_0_offset=0;
			TIA_player_1_offset=0;
			TIA_player_0_mask_tmp = TIA_player_0_mask;
			TIA_player_1_mask_tmp = TIA_player_1_mask;
			TIA_player_0_mask_bit = 10;
			TIA_player_1_mask_bit = 10;
			TIA_player_0_block = 0;
			TIA_player_1_block = 0;
			TIA_player_0_finished = 0;
			TIA_player_1_finished = 0;
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
		TIA_player_0_mask = TIA_player_mask[data & 0x07];
		TIA_player_0_pixel_delay = TIA_player_pixel_delay[data & 0x07];

		TIA_player_0_mask_tmp = TIA_player_0_mask;
		TIA_player_0_pixel_delay_tmp = TIA_player_0_pixel_delay;

		/* must implement player size checking! */

		break;


	case NUSIZ1:						/* offset 0x05 */
		msize1 = 2 ^ ((data * 0x30) >> 4);
		TIA_player_1_mask = TIA_player_mask[data & 0x07];
		TIA_player_1_pixel_delay = TIA_player_pixel_delay[data & 0x07];

		TIA_player_1_mask_tmp = TIA_player_1_mask;
		TIA_player_1_pixel_delay_tmp = TIA_player_1_pixel_delay;
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
			logerror("TIA_w - REFP0 No reflect \n");
		}
		else if (data & 0x08)
		{
			TIA_player_0_reflect = 1;
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
			logerror("TIA_w - REFP1 No reflect \n");
		}
		else if (data & 0x08)
		{
			TIA_player_1_reflect = 1;
			logerror("TIA_w - REFP1 Reflect \n");
		}
		else
		{
			if (TIA_VERBOSE)
				logerror("TIA_w - Write Error, REFP1 offset $%02x & data $%02x\n", offset, data);
		}
		break;

	case PF0:							/* 0x0D Playfield Register Byte 0 */
		TIA_playfield.pf[2] = bit_flip_table[data & 0xf0];
		tia.pfreg.B0 = data;
		PF0_Rendered = 1;

		break;

	case PF1:							/* 0x0E Playfield Register Byte 1 */
		TIA_playfield.pf[1] = data;
		tia.pfreg.B1 = data;
		PF1_Rendered = 1;

		break;

	case PF2:							/* 0x0F Playfield Register Byte 2 */
		TIA_playfield.pf[0] = bit_flip_table[data];
		tia.pfreg.B2 = data;
		PF2_Rendered = 1;

		break;


/* These next 5 Registers are Strobe registers            */
/* They will need to update the screen as soon as written */

	case RESP0:						/* 0x10 Reset Player 0 */
		logerror("Reset Player 0 0x%02x\n", HSYNC_Period);
		TIA_reset_player_0 = HSYNC_Period + 1;
		break;

	case RESP1:						/* 0x11 Reset Player 1 */
		TIA_reset_player_1 = HSYNC_Period + 1;
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
		TIA_hmp0 = (data & 0xf0) >> 4;
		TIA_motion_player_0 = TIA_movement_table[0x0f - TIA_hmp0];
		break;

	case HMP1:							/* 0x21 Horizontal Motion Player 1 */
		logerror("Horizontal move write %02x\n", data);
		TIA_hmp1 = (data & 0xf0) >> 4;
		TIA_motion_player_1 = TIA_movement_table[0x0f - TIA_hmp1];
		break;

	case HMM0:							/* 0x22 Horizontal Motion Missle 0 */
		break;

	case HMM1:							/* 0x23 Horizontal Motion Missle 1 */
		break;

	case HMBL:							/* 0x24 Horizontal Motion Ball */
		break;

	case VDELP0:						/* 0x25 Vertical Delay Player 0 */
		TIA_player_delay_0 = data & 0x01;
		logerror("Delay 0 = 0x%02x", TIA_player_delay_0);
		break;

	case VDELP1:						/* 0x26 Vertical Delay Player 1 */
		TIA_player_delay_1 = data & 0x01;
	 	logerror("Delay 1 = 0x%02x", TIA_player_delay_1);
		break;

	case VDELBL:						/* 0x27 Vertical Delay Ball */
		break;

	case RESMP0:						/* 0x28 Reset Missle 0 to Player 0 */
		break;

	case RESMP1:						/* 0x29 Reset Missle 1 to Player 1 */
		break;

	case HMOVE:						/* 0x2A Apply Horizontal Motion */
		TIA_motion_player_0 = TIA_movement_table[0x0f - TIA_hmp0];
		TIA_motion_player_1 = TIA_movement_table[0x0f - TIA_hmp1];
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


/***************************************************************************

  Stop MAchine

***************************************************************************/
MACHINE_STOP( a2600 )
{
	/* Make sane the hardware */
	timer_reset(HSYNC_timer, TIME_IN_CYCLES(76, 0));
	TIA_pixel_clock = 0;
	TIA_player_0_finished = 0;
	TIA_player_1_finished = 0;
	TIA_player_0_block = 0;
	TIA_player_1_block = 0;
	PF_Ref = 0;
	TIA_player_0_reflect = 0;
	TIA_player_1_reflect = 0;
	PF_Score = 0;
	PF_Pfp = 0;
	TIA_player_0_tick = 8;
	TIA_player_1_tick = 8;
	TMR_Intim = 0;
	TMR_Period = 0;
	TMR_tmp = 0;
	PF0_Rendered = 0;
	PF1_Rendered = 0;
	PF2_Rendered = 0;
	TIA_hmp0 = 0;
	TIA_hmp1 = 0;
	TIA_motion_player_0 = 0;
	TIA_motion_player_1 = 0;

	global_tia_cycle = 0;
	previous_tia_cycle = 0;
	TIA_player_0_mask_bit = 10;
	TIA_player_1_mask_bit = 10;
	TIA_player_0_mask = 0;
	TIA_player_0_pixel_delay = 0;

	TIA_player_1_mask = 0;
	TIA_player_1_pixel_delay = 0;

	TIA_player_0_mask_tmp = 0;
	TIA_player_0_pixel_delay_tmp = 0;

	TIA_player_1_mask_tmp = 0;
	TIA_player_1_pixel_delay_tmp = 0;

	TIA_player_0_offset = 0;
	TIA_player_1_offset = 0;
	HSYNC_Period = 0;
	TIA_reset_player_0 = 0;
	TIA_reset_player_1 = 0;
	TIA_player_pattern_0 = 0;
	TIA_player_pattern_0_delayed = 0;
	TIA_player_pattern_0_tmp = 0;
	TIA_player_delay_0 = 0;
	TIA_player_pattern_1 = 0;
	TIA_player_pattern_1_delayed = 0;
	TIA_player_pattern_1_tmp = 0;
	TIA_player_delay_1 = 0;

	HSYNC_colour_clock = 3;

	currentline = 0;
	msize0 = 0;
	msize1 = 0;
}




/* Video functions for the a2600         */
/* Since all software drivern, have here */

/***************************************************************************

  Update Bitmap When called

***************************************************************************/
static void a2600_scanline_cb(void)
{
	int regpos;
	int xs = Machine->visible_area.min_x;
	int backcolor;
	UINT16 scanline[160];

	profiler_mark(PROFILER_VIDEO);

	/* set color to background */
	backcolor = tia.colreg.BK % Machine->drv->color_table_len;

	if (!osd_skip_this_frame())
	{
		if ((currentline <= 261) && (TIA_vblank == 0))
		{
			/* now we have color, plot for 4 color cycles */
			for (regpos = 0; regpos < 160; regpos++)
			{
				int i = PF_Data[regpos] % Machine->drv->color_table_len;
				if (i == 0)
					scanline[regpos] = Machine->pens[backcolor];
				else
					scanline[regpos] = Machine->pens[i];
			}
			draw_scanline16(tmpbitmap, xs, currentline, 160, scanline, NULL, -1);
		}
	}
	#ifndef USE_SCANLINE_WSYNC
	currentline++;
	#endif

/*bail:*/
	PF0_Rendered = 0;
	PF1_Rendered = 0;
	PF2_Rendered = 0;

	profiler_mark(PROFILER_END);
}


/***************************************************************************

  Main callback - 76 Cycle Timer callback!

***************************************************************************/
static void a2600_main_cb(int param)
{
	INT32 riotdiff = (global_tia_cycle + 76) - previous_tia_cycle;

	profiler_mark(PROFILER_USER1);

	/* resync the riot timer */
	previous_tia_cycle = global_tia_cycle + 76;
	riotdiff *= 3;
	a2600_Cycle_cb(riotdiff);
	TIA_pixel_clock = 0;
	TIA_pf_mask.shiftreg = 0x080000;
	TIA_player_pattern_0_delayed = TIA_player_pattern_0;
	TIA_player_pattern_1_delayed = TIA_player_pattern_1;
	TIA_player_0_offset=0;
	TIA_player_1_offset=0;
	TIA_player_0_mask_tmp = TIA_player_0_mask;
	TIA_player_1_mask_tmp = TIA_player_1_mask;
	TIA_player_0_mask_bit = 10;
	TIA_player_1_mask_bit = 10;
	TIA_player_0_block = 0;
	TIA_player_1_block = 0;
	TIA_player_0_finished = 0;
	TIA_player_1_finished = 0;
	a2600_scanline_cb();
	global_tia_cycle += 76;

	profiler_mark(PROFILER_END);
}


/***************************************************************************

  Cycle Callback to Provide accurate timing

***************************************************************************/
static void a2600_Cycle_cb(int riotdiff)
{
	int i, forecolour;
	int plyr0 = TIA_reset_player_0 + TIA_motion_player_0 + TIA_player_0_offset;

	logerror("Cycle Callback - RIOTDIFF = %d\n", riotdiff);

	for (i=0; i<riotdiff; i++)
	{

		/* Hsync Timing */

		HSYNC_Period++;
		HSYNC_colour_clock--;


		profiler_mark(PROFILER_USER2);

		if(HSYNC_Period >= 68)
		{
			if(HSYNC_Period <= 147)
			{
				UINT8 pix = (TIA_playfield.shiftreg & TIA_pf_mask.shiftreg)? 1:0;

				if (PF_Score)
				{
					forecolour = tia.colreg.P0;
				}
				else
				{
					forecolour = tia.colreg.PF;
				}

				PF_Data[HSYNC_Period - 68] = forecolour * pix;

				TIA_pixel_clock++;

				if(TIA_pixel_clock == 4)
				{
					SHIFT_RIGHT20(TIA_pf_mask)

					TIA_pixel_clock = 0;
				}
			}
			else
			{
				if(HSYNC_Period == 148 && PF_Ref)
				{
					TIA_pf_mask.shiftreg = 0x000001;
				}

				{
					UINT8 pix = (TIA_playfield.shiftreg & TIA_pf_mask.shiftreg)? 1:0;

					if (PF_Score)
					{
						forecolour = tia.colreg.P1;
					}
					else
					{
						forecolour = tia.colreg.PF;
					}


					PF_Data[HSYNC_Period - 68] = forecolour * pix;

					TIA_pixel_clock++;

					if(TIA_pixel_clock == 4)
					{
						if(!PF_Ref)
						{
							SHIFT_RIGHT20(TIA_pf_mask)
						}
						else
						{
							SHIFT_LEFT20(TIA_pf_mask)
						}
						TIA_pixel_clock = 0;
					}
				}
			}
		}

		if ((HSYNC_Period - 1) == 0)
		{
			TIA_player_pattern_0_tmp = TIA_player_pattern_0;
			TIA_player_0_tick = 8;
			TIA_player_pattern_1_tmp = TIA_player_pattern_1;
			TIA_player_1_tick = 8;
		}
		profiler_mark(PROFILER_END);


		profiler_mark(PROFILER_USER3);
	/* Player 0 stuff */
		if ((TIA_player_0_mask_tmp & 0x01) && (TIA_reset_player_0) && (((HSYNC_Period - 1) >= (plyr0)) && ((HSYNC_Period - 1) <= ((TIA_reset_player_0 + 7) + TIA_motion_player_0 + TIA_player_0_offset))) && (!TIA_player_0_finished))
		{
			//logerror("motion player 0 %d \n", TIA_motion_player_0);
			if (!TIA_player_0_reflect)
			{
				UINT8 tmpbit = 0;
				if (!TIA_player_delay_0)
				{
					if ((TIA_player_pattern_0_tmp & 0x80))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
						tmpbit = 0x01;
					}
				}
				else
				{
					if ((TIA_player_pattern_0_delayed & 0x80))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
						tmpbit = 0x01;
					}
				}
				//logerror("Hsync Period = %02x\n", HSYNC_Period);
				TIA_player_0_pixel_delay_tmp--;
				if(!TIA_player_0_pixel_delay_tmp)
				{
					TIA_player_0_pixel_delay_tmp = TIA_player_0_pixel_delay;
					SHIFT_LEFT8(TIA_player_pattern_0_tmp)
					SHIFT_LEFT8(TIA_player_pattern_0_delayed)
					TIA_player_0_tick--;
					if (!TIA_player_0_tick)
					{
					TIA_player_0_tick = 8;
					}
				}
			}
			else
			{
				UINT8 tmpbit = 0;
				if (!TIA_player_delay_0)
				{
					if ((TIA_player_pattern_0_tmp & 0x01))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
						tmpbit = 0x80;
					}
				}
				else
				{
					if ((TIA_player_pattern_0_delayed & 0x01))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P0;
						tmpbit = 0x80;
					}
				}
				//logerror("Hsync Period = %02x\n", HSYNC_Period);
				TIA_player_0_pixel_delay_tmp--;
				if(!TIA_player_0_pixel_delay_tmp)
				{
					TIA_player_0_pixel_delay_tmp = TIA_player_0_pixel_delay;
					SHIFT_RIGHT8(TIA_player_pattern_0_tmp)
					SHIFT_RIGHT8(TIA_player_pattern_0_delayed)
					TIA_player_0_tick--;
					if (!TIA_player_0_tick)
					{
						TIA_player_0_tick = 8;
					}
				}
			}
		}
		profiler_mark(PROFILER_END);

		profiler_mark(PROFILER_USER4);
	/* Player 1 stuff */
		if ((TIA_player_1_mask_tmp & 0x01) && (TIA_reset_player_1) && (((HSYNC_Period - 1) >= (TIA_reset_player_1 + TIA_motion_player_1 + TIA_player_1_offset)) && ((HSYNC_Period - 1) <= ((TIA_reset_player_1 + 7) + TIA_motion_player_1 + TIA_player_1_offset))) && (!TIA_player_1_finished))
		{
			//logerror("motion player 1 %d \n", TIA_motion_player_1);
			if (!TIA_player_1_reflect)
			{
				UINT8 tmpbit = 0;
				if (!TIA_player_delay_1)
				{
					if ((TIA_player_pattern_1_tmp & 0x80))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
						tmpbit = 0x01;
					}
				}
				else
				{
					if ((TIA_player_pattern_1_delayed & 0x80))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
						tmpbit = 0x01;
					}
				}
				//logerror("Hsync Period = %02x\n", HSYNC_Period);
				TIA_player_1_pixel_delay_tmp--;
				if(!TIA_player_1_pixel_delay_tmp)
				{
					TIA_player_1_pixel_delay_tmp = TIA_player_1_pixel_delay;
					SHIFT_LEFT8(TIA_player_pattern_1_tmp)
					SHIFT_LEFT8(TIA_player_pattern_1_delayed)
					TIA_player_1_tick--;
					if (!TIA_player_1_tick)
					{
					TIA_player_1_tick = 8;
					}
				}
			}
			else
			{
				UINT8 tmpbit = 0;
				if (!TIA_player_delay_1)
				{
					if ((TIA_player_pattern_1_tmp & 0x01))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
						tmpbit = 0x80;
					}
				}
				else
				{
					if ((TIA_player_pattern_1_delayed & 0x01))
					{
						PF_Data[((HSYNC_Period - 1) - 68)] = tia.colreg.P1;
						tmpbit = 0x80;
					}
				}
				//logerror("Hsync Period = %02x\n", HSYNC_Period);
				TIA_player_1_pixel_delay_tmp--;
				if(!TIA_player_1_pixel_delay_tmp)
				{
					TIA_player_1_pixel_delay_tmp = TIA_player_1_pixel_delay;
					SHIFT_RIGHT8(TIA_player_pattern_1_tmp)
					SHIFT_RIGHT8(TIA_player_pattern_1_delayed)
					TIA_player_1_tick--;
					if (!TIA_player_1_tick)
					{
						TIA_player_1_tick = 8;
					}
				}
			}
		}

		if((HSYNC_Period >= 68) && ((HSYNC_Period - 1)>=TIA_reset_player_0 + TIA_motion_player_0))
		{
			TIA_player_0_block++;

			if(TIA_player_0_block == 8)
			{
				TIA_player_0_offset+=8;
				SHIFT_RIGHT10(TIA_player_0_mask_tmp)
				TIA_player_0_mask_bit--;
				TIA_player_0_block = 0;
				if(!TIA_player_0_mask_bit)
				{
					TIA_player_0_mask_bit = 10;
					TIA_player_0_finished = 1;
				}

			}
		}
		if((HSYNC_Period >= 68) && ((HSYNC_Period - 1)>=TIA_reset_player_1 + TIA_motion_player_1))
		{
			TIA_player_1_block++;

			if(TIA_player_1_block == 8)
			{
				TIA_player_1_offset+=8;

				SHIFT_RIGHT10(TIA_player_1_mask_tmp)
				TIA_player_1_mask_bit--;
				TIA_player_1_block = 0;
				if(!TIA_player_1_mask_bit)
				{
					TIA_player_1_mask_bit = 10;
					TIA_player_1_finished = 1;
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

		profiler_mark(PROFILER_END);
	}
}


/***************************************************************************

  Machine Initialisation

***************************************************************************/
MACHINE_INIT( a2600 )
{
	/* start RIOT interface */
	currentline = 0;
	HSYNC_timer = timer_alloc(a2600_main_cb);
	timer_adjust(HSYNC_timer, 0, 0, TIME_IN_CYCLES(76, 0));
	TIA_pf_mask.shiftreg = 0x080000;
	return;

}


/***************************************************************************

  Cartridge Loading

***************************************************************************/
DEVICE_LOAD( a2600_cart )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	UINT32 crc;
	int cart_size;

	a2600_cartridge_rom = &(ROM[0x10000]);	/* Load the cart outside the cpuspace for b/s purposes */

	cart_size = mame_fsize(file);
	mame_fread(file, a2600_cartridge_rom, cart_size);		/* testing everything now :) */
	/* copy to mirrorred memory regions */
	crc = (UINT32) crc32(0L,&ROM[0x10000], cart_size);
	Bankswitch_Method = 0;

	switch(cart_size)
	{
		case 0x10000:
					break;
		case 0x08000:
					break;
		case 0x04000:
					break;
		case 0x03000:
					break;
		case 0x02000:
					switch(crc)
					{

						case 0x91b8f1b2:
									Bankswitch_Method = 0xFE;
									logerror("Decathlon detected and loaded\n");
									break;
						case 0xfd8c81e5:
									Bankswitch_Method = 0xE0;
									logerror("Tooth Protectors detected and loaded\n");
									break;
						case 0x0886a55d:
									Bankswitch_Method = 0xE0;
									logerror("SW: Death Star Battle detected and loaded\n");
									break;
						case 0x0d78e8a9:
									Bankswitch_Method = 0xE0;
									logerror("Gyruss detected and loaded\n");
									break;
						case 0x34d3ffc8:
									Bankswitch_Method = 0xE0;
									logerror("James Bond 007 detected and loaded\n");
									break;
						case 0xde97103d:
									Bankswitch_Method = 0xE0;
									logerror("Super Cobra detected and loaded\n");
									break;
						case 0xec959bf2:
									Bankswitch_Method = 0xE0;
									logerror("Tutankham detected and loaded\n");
									break;
						case 0x7d287f20:
									Bankswitch_Method = 0xE0;
									logerror("Popeye detected and loaded\n");
									break;
						case 0x65c31ca4:
									Bankswitch_Method = 0xE0;
									logerror("SW: Arcade Game detected and loaded\n");
									break;
						case 0xa87be8fd:
									Bankswitch_Method = 0xE0;
									logerror("Q*Bert's Qubes detected and loaded\n");
									break;
						case 0x3ba0d9bf:
									Bankswitch_Method = 0xE0;
									logerror("Frogger ][: Threeedeep detected and loaded\n");
									break;
						case 0xe680a1c9:
									Bankswitch_Method = 0xE0;
									logerror("Montezuma's Revenge detected and loaded\n");
									break;
						case 0x044735b9:
									Bankswitch_Method = 0xE0;
									logerror("Mr. Do's Castle detected and loaded\n");
									break;
						case 0xc820bd75:
									Bankswitch_Method = 0x3F;
									logerror("River Patrol detected and loaded\n");
									break;
						case 0xdd183a4f:
									Bankswitch_Method = 0x3F;
									logerror("Springer detected and loaded\n");
									break;
						case 0xdb376663:
									Bankswitch_Method = 0x3F;
									logerror("Polaris detected and loaded\n");
									break;
						case 0xbd08d915:
									Bankswitch_Method = 0x3F;
									logerror("Miner 2049'er detected and loaded\n");
									break;
						case 0xbfa477cd:
									Bankswitch_Method = 0x3F;
									logerror("Miner 2049'er Volume ][ detected and loaded\n");
									break;
						case 0x34b80a97:
									Bankswitch_Method = 0x3F;
									logerror("Espial detected and loaded\n");
									break;
						default:
									Bankswitch_Method = 0xf8;
									break;
					}

					switch(Bankswitch_Method)
					{
						case 0xf8:
									memcpy(&ROM[0x1000], &ROM[0x11000], 0x1000);
									memcpy(&ROM[0xf000], &ROM[0x11000], 0x1000);
									break;
						case 0xe0:
									memcpy(&ROM[0x1000], &ROM[0x10000], 0x1000);
									memcpy(&ROM[0x1400], &ROM[0x10400], 0x1000);
									memcpy(&ROM[0x1800], &ROM[0x10800], 0x1000);
									memcpy(&ROM[0x1c00], &ROM[0x11c00], 0x1000);

									memcpy(&ROM[0xf000], &ROM[0x10000], 0x1000);
									memcpy(&ROM[0xf400], &ROM[0x10400], 0x1000);
									memcpy(&ROM[0xf800], &ROM[0x10800], 0x1000);
									memcpy(&ROM[0xfc00], &ROM[0x11c00], 0x1000);
									break;
					}
					break;
		case 0x01000:
					memcpy(&ROM[0x1000], &ROM[0x10000], 0x1000);
					memcpy(&ROM[0xf000], &ROM[0x10000], 0x1000);
					break;
		case 0x00800:
					memcpy(&ROM[0x1000], &ROM[0x10000], 0x0800);
					memcpy(&ROM[0x1800], &ROM[0x10000], 0x0800);
					memcpy(&ROM[0xf000], &ROM[0x10000], 0x0800);
					memcpy(&ROM[0xf800], &ROM[0x10000], 0x0800);
					break;
	}
	logerror("cartridge crc = %08x\n", crc);
	return 0;
}


#ifdef USE_SCANLINE_WSYNC
/***************************************************************************

  Fake Scanline Interrupt - for timing only

***************************************************************************/
int a2600_scanline_int(void)
{

	/* Increment the Scanline Counter */
	currentline++;

	/* Restart the CPU if WSYNC has been written to and reset wsync */
	if (TIA_wsync == 1)
	{
	logerror("WSYNC SCANLINE INT - Expected Cycles=%d, Frame #%d, Scanline #%d HorizPos=%d \n",
				TIME_TO_CYCLES(0,cpu_getscanlineperiod()),
				cpu_getcurrentframe(),
				cpu_getscanline(),
				cpu_gethorzbeampos() );

		cpu_trigger(WSYNC_TRIGGER);
		TIA_wsync = 0;
	}

	/* Fake Interrupt - return ignore */
	return ignore_interrupt();
}
#endif

