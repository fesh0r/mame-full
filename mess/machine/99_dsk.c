/*
	TI-99/4 and /4a disk controller emulation.

	Known disk controllers:
	* TI original SD disk controller.  First sold as a side port module
	  (PHP1800), later as a card to be inserted in the PE-box (PHP1240).
	  Uses a WD1771 FDC, and supports a format of 40 tracks of 9 sectors.
	* TI DD disk controller prototype.  Uses some NEC FDC and supports 16
	  sectors per track.  (Not emulated)
	* CorComp FDC.  Specs unknown, but it supports DD.  (Not emulated)
	* SNUG BwG.  A nice DD disk controller.  Supports 18 sectors per track.
	* Myarc HFDC.  Extremely elaborate, supports DD (an HD upgrade may have
	  existed) and MFM harddisks.  (Preliminary emulation)

	An alternative was installing the hexbus controller (which was just a
	prototype, and is not emulated) and connecting a hexbus floppy disk
	controller.  The integrated hexbus port was the only supported way to
	attach a floppy drive to a TI-99/2 or a TI-99/5.  The integrated hexbus
	port was the recommended way to attach a floppy drive to the TI-99/8,
	but I think this computer supported the TI-99/4(a) disk controllers, too.

	Raphael Nabet, 1999-2003.
*/

#include "devices/basicdsk.h"
#include "includes/wd179x.h"
#include "ti99_4x.h"
#include "99_dsk.h"
#include "mm58274c.h"


int ti99_floppy_load(int id, mame_file *fp, int open_mode)
{
	typedef struct ti99_vib
	{
		char	name[10];			/* volume name (10 characters, pad with spaces) */
		UINT8	totsecsMSB;			/* disk length in sectors (big-endian) (usually 360, 720 or 1440) */
		UINT8	totsecsLSB;
		UINT8	secspertrack;		/* sectors per track (usually 9 (FM) or 18 (MFM)) */
		UINT8 id[3];				/* 'DSK' */
		UINT8 protection;			/* 'P' if disk is protected, ' ' otherwise. */
		UINT8	tracksperside;		/* tracks per side (usually 40) */
		UINT8	sides;				/* sides (1 or 2) */
		UINT8	density;			/* 1 (FM) or 2 (MFM) */
		UINT8	res[36];			/* reserved */
		UINT8	abm[200];			/* allocation bitmap: a 1 for each sector in use (sector 0 is LSBit of byte 0, sector 7 is MSBit of byte 0, sector 8 is LSBit of byte 1, etc.) */
	} ti99_vib;

	ti99_vib vib;
	int totsecs;
	int done;


	if (basicdsk_floppy_load(id, fp, open_mode)==INIT_PASS)
	{
		done = FALSE;

		/* Read sector 0 to identify format */
		if (fp && (! mame_fseek(fp, 0, SEEK_SET)) && (mame_fread(fp, & vib, sizeof(vib)) == sizeof(vib)))
		{
			/* If we have read the sector successfully, let us parse it */
			totsecs = (vib.totsecsMSB << 8) | vib.totsecsLSB;
			if (vib.tracksperside == 0)
				/* Some images are like this, because the TI controller always assumes 40. */
				vib.tracksperside = 40;
			if (vib.sides == 0)
				/* Some images are like this, because the TI controller always assumes
				tracks beyond 40 are on side 2. */
				vib.sides = totsecs / (vib.secspertrack * vib.tracksperside);
			/* check that the format makes sense */
			if (((vib.secspertrack * vib.tracksperside * vib.sides) == totsecs)
				&& (totsecs >= 2) && (totsecs <= 1600) && (! memcmp(vib.id, "DSK", 3))
				&& (image_length(IO_FLOPPY, id) == totsecs*256))
			{
				/* set geometry */
				basicdsk_set_geometry(id, vib.tracksperside, vib.sides, vib.secspertrack, 256, 0, 0);
				done = TRUE;
			}
		}

		/* If we have been unable to parse the format, let us guess according
		to file lenght */
		if (! done)
		{
			switch (image_length(IO_FLOPPY, id))
			{
			case 1*40*9*256:	/* 90kbytes: SSSD */
			default:
				basicdsk_set_geometry(id, 40, 1, 9, 256, 0, 0);
				break;

			case 2*40*9*256:	/* 180kbytes: either DSSD or 18-sector-per-track
								SSDD.  We assume DSSD since DSSD is more common
								and is supported by the original TI SD disk
								controller. */
				basicdsk_set_geometry(id, 40, 2, 9, 256, 0, 0);
				break;

			case 1*40*16*256:	/* 160kbytes: 16-sector-per-track SSDD (standard
								format for TI DD disk controller prototype, TI
								hexbus disk controller, and possibly some Myarc
								disk controller) */
				basicdsk_set_geometry(id, 40, 1, 16, 256, 0, 0);
				break;

			case 2*40*16*256:	/* 320kbytes: 16-sector-per-track DSDD (standard
								format for TI DD disk controller prototype, TI
								hexbus disk controller, and possibly some Myarc
								disk controller) */
				basicdsk_set_geometry(id, 40, 2, 16, 256, 0, 0);
				break;

			case 2*40*18*256:	/* 360kbytes: 18-sector-per-track DSDD (standard
								format for most third-party DD disk controllers,
								but reportedly not supported by the original TI
								DD disk controller) */
				basicdsk_set_geometry(id, 40, 2, 18, 256, 0, 0);
				break;
			}
		}

		return INIT_PASS;
	}

	return INIT_FAIL;
}


/*===========================================================================*/
/*
	TI99/4(a) Floppy disk controller card emulation
*/

/* prototypes */
static void fdc_callback(int);
static void motor_on_timer_callback(int dummy);
static int fdc_cru_r(int offset);
static void fdc_cru_w(int offset, int data);
static READ_HANDLER(fdc_mem_r);
static WRITE_HANDLER(fdc_mem_w);

/* pointer to the fdc ROM data */
static UINT8 *ti99_disk_DSR;

/* when TRUE the CPU is halted while DRQ/IRQ are true */
static int DSKhold;

/* defines for DRQ_IRQ_status */
enum
{
	fdc_IRQ = 1,
	fdc_DRQ = 2
};

/* current state of the DRQ and IRQ lines */
static int DRQ_IRQ_status;

/* disk selection bits */
static int DSEL;
/* currently selected disk unit */
static int DSKnum;
/* current side */
static int DSKside;

/* 1 if motor is on */
static int DVENA;
/* on rising edge, sets DVENA for 4.23 seconds on rising edge */
static int motor_on;
/* count 4.23s from rising edge of motor_on */
void *motor_on_timer;

static const ti99_exp_card_handlers_t fdc_handlers =
{
	fdc_cru_r,
	fdc_cru_w,
	fdc_mem_r,
	fdc_mem_w
};


/*
	Reset fdc card, set up handlers
*/
void ti99_fdc_init(void)
{
	ti99_disk_DSR = memory_region(region_dsr) + offset_fdc_dsr;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;
	motor_on_timer = timer_alloc(motor_on_timer_callback);

	ti99_exp_set_card_handlers(0x1100, & fdc_handlers);

	wd179x_init(WD_TYPE_179X, fdc_callback);		/* initialize the floppy disk controller */
	wd179x_set_density(DEN_FM_LO);
}


/*
	call this when the state of DSKhold or DRQ/IRQ or DVENA change

	Emulation is faulty because the CPU is actually stopped in the midst of instruction, at
	the end of the memory access
*/
static void fdc_handle_hold(void)
{
	if (DSKhold && (!DRQ_IRQ_status) && DVENA)
		cpu_set_halt_line(0, ASSERT_LINE);
	else
		cpu_set_halt_line(0, CLEAR_LINE);
}

/*
	callback called whenever DRQ/IRQ state change
*/
static void fdc_callback(int event)
{
	switch (event)
	{
	case WD179X_IRQ_CLR:
		DRQ_IRQ_status &= ~fdc_IRQ;
		ti99_exp_set_ilb_bit(intb_fdc_bit, 0);
		break;
	case WD179X_IRQ_SET:
		DRQ_IRQ_status |= fdc_IRQ;
		ti99_exp_set_ilb_bit(intb_fdc_bit, 1);
		break;
	case WD179X_DRQ_CLR:
		DRQ_IRQ_status &= ~fdc_DRQ;
		break;
	case WD179X_DRQ_SET:
		DRQ_IRQ_status |= fdc_DRQ;
		break;
	}

	fdc_handle_hold();
}

/*
	callback called at the end of DVENA pulse
*/
static void motor_on_timer_callback(int dummy)
{
	DVENA = 0;
	fdc_handle_hold();
}

/*
	Read disk CRU interface

	bit 0: HLD pin
	bit 1-3: drive n active
	bit 4: 0: motor strobe on
	bit 5: always 0
	bit 6: always 1
	bit 7: selected side
*/
static int fdc_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		reply = 0x40;
		if (DSKside)
			reply |= 0x80;
		if (DVENA)
			reply |= (DSEL << 1);
		if (! DVENA)
			reply |= 0x10;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}

/*
	Write disk CRU interface
*/
static void fdc_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in ti99_expansion_CRU_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold();
			timer_adjust(motor_on_timer, 4.23, 0, 0.);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
			0: ignore IRQ and DRQ
			1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
			  for 4.23s after write to revelant CRU bit, this is not emulated and could cause
			  the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold();
		break;

	case 3:
		/* Load disk heads (HLT pin) (bit 3) */
		/* ... */
		break;

	case 4:
	case 5:
	case 6:
		/* Select drive X (bits 4-6) */
		{
			int drive = offset-4;					/* drive # (0-2) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd179x_set_drive(DSKnum);
					wd179x_set_side(DSKside);
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd179x_set_side(DSKside);
		break;
	}
}


/*
	read a byte in disk DSR space
*/
static READ_HANDLER(fdc_mem_r)
{
	switch (offset)
	{
	case 0x1FF0:					/* Status register */
		return (wd179x_status_r(offset) ^ 0xFF);
		break;
	case 0x1FF2:					/* Track register */
		return wd179x_track_r(offset) ^ 0xFF;
		break;
	case 0x1FF4:					/* Sector register */
		return wd179x_sector_r(offset) ^ 0xFF;
		break;
	case 0x1FF6:					/* Data register */
		return wd179x_data_r(offset) ^ 0xFF;
		break;
	default:						/* DSR ROM */
		return ti99_disk_DSR[offset];
		break;
	}
}

/*
	write a byte in disk DSR space
*/
static WRITE_HANDLER(fdc_mem_w)
{
	data ^= 0xFF;	/* inverted data bus */

	switch (offset)
	{
	case 0x1FF8:					/* Command register */
		wd179x_command_w(offset, data);
		break;
	case 0x1FFA:					/* Track register */
		wd179x_track_w(offset, data);
		break;
	case 0x1FFC:					/* Sector register */
		wd179x_sector_w(offset, data);
		break;
	case 0x1FFE:					/* Data register */
		wd179x_data_w(offset, data);
		break;
	}
}


/*===========================================================================*/
/*
	Alternate fdc: BwG card from SNUG

	Advantages:
	* this card supports Double Density.
	* as this card includes its own RAM, it does not need to allocate a portion
	  of VDP RAM to store I/O buffers.
	* this card includes a MM58274C RTC.

	Reference:
	* BwG Disketten-Controller: Beschreibung der DSR
		<http://home.t-online.de/home/harald.glaab/snug/bwg.pdf>
*/

/* prototypes */
static int bwg_cru_r(int offset);
static void bwg_cru_w(int offset, int data);
static READ_HANDLER(bwg_mem_r);
static WRITE_HANDLER(bwg_mem_w);

static const ti99_exp_card_handlers_t bwg_handlers =
{
	bwg_cru_r,
	bwg_cru_w,
	bwg_mem_r,
	bwg_mem_w
};

static int bwg_rtc_enable;
static int bwg_ram_offset;
static int bwg_rom_offset;
static UINT8 *bwg_ram;


/*
	Reset fdc card, set up handlers
*/
void ti99_bwg_init(void)
{
	ti99_disk_DSR = memory_region(region_dsr) + offset_bwg_dsr;
	bwg_ram = memory_region(region_dsr) + offset_bwg_ram;
	bwg_ram_offset = 0;
	bwg_rom_offset = 0;
	bwg_rtc_enable = 0;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;
	motor_on_timer = timer_alloc(motor_on_timer_callback);

	ti99_exp_set_card_handlers(0x1100, & bwg_handlers);

	wd179x_init(WD_TYPE_179X, fdc_callback);		/* initialize the floppy disk controller */
	wd179x_set_density(DEN_MFM_LO);


	mm58274c_init();	/* initialize the RTC */
}


/*
	Read disk CRU interface

	bit 0: drive 4 active (not emulated)
	bit 1-3: drive n active
	bit 4-7: dip switches 1-4
*/
static int bwg_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		/* CRU bits: beware, logic levels of DIP-switches are inverted  */
		reply = 0x50;
		if (DVENA)
			reply |= ((DSEL << 1) | (DSEL >> 3)) & 0x0f;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


/*
	Write disk CRU interface
*/
static void bwg_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in ti99_expansion_CRU_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold();
			timer_adjust(motor_on_timer, 4.23, 0, 0.);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
			0: ignore IRQ and DRQ
			1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
			  for 4.23s after write to revelant CRU bit, this is not emulated and could cause
			  the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold();
		break;

	case 4:
	case 5:
	case 6:
		/* Select drive X (bits 4-6) */
		{
			int drive = offset-4;					/* drive # (0-2) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd179x_set_drive(DSKnum);
					wd179x_set_side(DSKside);
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd179x_set_side(DSKside);
		break;

	case 8:
		/* Select drive 3 (DSK4) */
		break;

	case 10:
		/* double density enable (active low) */
		wd179x_set_density(data ? DEN_FM_LO : DEN_MFM_LO);
		break;

	case 11:
		/* EPROM A13 */
		if (data)
			bwg_rom_offset |= 0x2000;
		else
			bwg_rom_offset &= ~0x2000;
		break;

	case 13:
		/* RAM A10 */
		if (data)
			bwg_ram_offset = 0x0400;
		else
			bwg_ram_offset = 0x0000;
		break;

	case 14:
		/* override FDC with RTC (active high) */
		bwg_rtc_enable = data;
		break;

	case 15:
		/* EPROM A14 */
		if (data)
			bwg_rom_offset |= 0x4000;
		else
			bwg_rom_offset &= ~0x4000;
		break;

	case 3:
	case 9:
	case 12:
		/* Unused (bit 3, 9 & 12) */
		break;
	}
}


/*
	read a byte in disk DSR space
*/
static READ_HANDLER(bwg_mem_r)
{
	int reply = 0;

	if (offset < 0x1c00)
		reply = ti99_disk_DSR[bwg_rom_offset+offset];
	else if (offset < 0x1fe0)
		reply = bwg_ram[bwg_ram_offset+(offset-0x1c00)];
	else if (bwg_rtc_enable)
	{
		if (! (offset & 1))
			reply = mm58274c_r((offset - 0x1FE0) >> 1);
	}
	else
	{
		if (offset < 0x1ff0)
			reply = bwg_ram[bwg_ram_offset+(offset-0x1c00)];
		else
			switch (offset)
			{
			case 0x1FF0:					/* Status register */
				reply = wd179x_status_r(offset);
				break;
			case 0x1FF2:					/* Track register */
				reply = wd179x_track_r(offset);
				break;
			case 0x1FF4:					/* Sector register */
				reply = wd179x_sector_r(offset);
				break;
			case 0x1FF6:					/* Data register */
				reply = wd179x_data_r(offset);
				break;
			default:
				reply = 0;
				break;
			}
	}

	return reply;
}

/*
	write a byte in disk DSR space
*/
static WRITE_HANDLER(bwg_mem_w)
{
	if (offset < 0x1c00)
		;
	else if (offset < 0x1fe0)
		bwg_ram[bwg_ram_offset+(offset-0x1c00)] = data;
	else if (bwg_rtc_enable)
	{
		if (! (offset & 1))
			mm58274c_w((offset - 0x1FE0) >> 1, data);
	}
	else
	{
		if (offset < 0x1ff0)
		{
			bwg_ram[bwg_ram_offset+(offset-0x1c00)] = data;
		}
		else
		{
			switch (offset)
			{
			case 0x1FF8:					/* Command register */
				wd179x_command_w(offset, data);
				break;
			case 0x1FFA:					/* Track register */
				wd179x_track_w(offset, data);
				break;
			case 0x1FFC:					/* Sector register */
				wd179x_sector_w(offset, data);
				break;
			case 0x1FFE:					/* Data register */
				wd179x_data_w(offset, data);
				break;
			}
		}
	}
}


/*===========================================================================*/
/*
	Alternate fdc: HFDC card built by Myarc

	Advantages: same as BwG, plus:
	* high density support (only on upgraded cards, I think)
	* hard disk support (only prehistoric mfm hard disks are supported, though)
	* DMA support (I think the DMA controller can only access the on-board RAM)

	This card includes a MM58274C RTC and a 9234 HFDC with its various support
	chips.  I have failed to find information about the 9234 or its cousin the
	9224.

	Reference:
	* hfdc manual
		<ftp://ftp.whtech.com//datasheets & manuals/Hardware manuals/hfdc manual.max>
*/

/* prototypes */
static int hfdc_cru_r(int offset);
static void hfdc_cru_w(int offset, int data);
static READ_HANDLER(hfdc_mem_r);
static WRITE_HANDLER(hfdc_mem_w);

static const ti99_exp_card_handlers_t hfdc_handlers =
{
	hfdc_cru_r,
	hfdc_cru_w,
	hfdc_mem_r,
	hfdc_mem_w
};

static int hfdc_rtc_enable;
static int hfdc_ram_offset[3];
static int hfdc_rom_offset;
static int cru_sel;
static UINT8 *hfdc_ram;


/*
	Reset fdc card, set up handlers
*/
void ti99_hfdc_init(void)
{
	ti99_disk_DSR = memory_region(region_dsr) + offset_hfdc_dsr;
	hfdc_ram = memory_region(region_dsr) + offset_hfdc_ram;
	hfdc_ram_offset[0] = hfdc_ram_offset[1] = hfdc_ram_offset[2] = 0;
	hfdc_rom_offset = 0;
	hfdc_rtc_enable = 0;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;
	motor_on_timer = timer_alloc(motor_on_timer_callback);

	ti99_exp_set_card_handlers(0x1100, & hfdc_handlers);

	//wd179x_init(WD_TYPE_179X, fdc_callback);		/* initialize the floppy disk controller */
	//wd179x_set_density(DEN_MFM_LO);


	mm58274c_init();	/* initialize the RTC */
}


/*
	Read disk CRU interface
*/
static int hfdc_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		/* CRU bits */
		if (cru_sel)
			/* DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
			bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
			dip 8 -> bit 0).  Return value examples:
				ff -> 4 slow 40-track DD drives
				55 -> 4 fast 40-track DD drives
				aa -> 4 80-track DD drives
				00 -> 4 80-track HD drives */
			reply = 0x55;	
		else
		{
			reply = 0;
			/*if (hfdc_interrupt)
				reply |= 1;*/
			if (motor_on)
				reply |= 2;
			/*if (hfdc_dma_in_progress)
				reply |= 4;*/
		}
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


/*
	Write disk CRU interface
*/
static void hfdc_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in ti99_expansion_CRU_w() */
		break;

	case 1:
		/* reset fdc (active low) */
		break;

	case 2:
		/* Strobe motor + density */
		if (data && !motor_on)
		{
			DVENA = 1;
			fdc_handle_hold();
			timer_adjust(motor_on_timer, 4.23, 0, 0.);
		}
		motor_on = data;
		break;

	case 3:
		/* rom page select 0 + cru sel */
		if (data)
			hfdc_rom_offset |= 0x2000;
		else
			hfdc_rom_offset &= ~0x2000;
		cru_sel = data;
		break;

	case 4:
		/* rom page select 1 + density */
		if (data)
			hfdc_rom_offset |= 0x1000;
		else
			hfdc_rom_offset &= ~0x1000;
		break;

	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		/* ram page select */
		if (data)
			hfdc_ram_offset[(offset-9)/5] |= 0x400 << ((offset-9)%5);
		else
			hfdc_ram_offset[(offset-9)/5] &= ~(0x400 << ((offset-9)%5));
		break;
	}
}


/*
	read a byte in disk DSR space
*/
static READ_HANDLER(hfdc_mem_r)
{
	int reply = 0;

	if (offset < 0x0fc0)
	{
		/* dsr ROM */
		reply = ti99_disk_DSR[hfdc_rom_offset+offset];
	}
	else if (offset < 0x0fd0)
	{
		/* tape controller */
	}
	else if (offset < 0x0fe0)
	{
		/* disk controller */
		/* >4fd0: data read?? */
		/* >4fd4: controller status? */
		logerror("hfdc9234 read %d\n", offset);
	}
	else if (offset < 0x1000)
	{
		/* rtc */
		if (! (offset & 1))
			reply = mm58274c_r((offset - 0x1FE0) >> 1);
	}
	else if (offset < 0x1400)
	{
		/* ram page >10 */
		reply = hfdc_ram[/*0x4000+*/(offset-0x1000)];
	}
	else
	{
		/* ram with mapper */
		reply = hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10]+((offset-0x1000) & 0x3ff)];
	}
	return reply;
}

/*
	write a byte in disk DSR space
*/
static WRITE_HANDLER(hfdc_mem_w)
{
	if (offset < 0x0fc0)
	{
		/* dsr ROM */
	}
	else if (offset < 0x0fd0)
	{
		/* tape controller */
	}
	else if (offset < 0x0fe0)
	{
		/* disk controller */
		/* >4fd4: data write? */
		/* >4fd6: command write? */
		logerror("hfdc9234 write %d %d\n", offset, data);
	}
	else if (offset < 0x1000)
	{
		/* rtc */
		if (! (offset & 1))
			mm58274c_w((offset - 0x1FE0) >> 1, data);
	}
	else if (offset < 0x1400)
	{
		/* ram page >10 */
		hfdc_ram[/*0x4000+*/(offset-0x1000)] = data;
	}
	else
	{
		/* ram with mapper */
		hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10]+((offset-0x1000) & 0x3ff)] = data;
	}
}

