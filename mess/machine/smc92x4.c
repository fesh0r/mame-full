/*
	SMC9224 and SMC9234 Hard and Floppy Disk Controller (HFDC)

	This controller handles MFM and FM encoded floppy disks and hard disks.
	The SMC9224 is used in some DEC systems.  The SMC9234 is used in the Myarc
	HFDC card for the TI99/4a.  The main difference between the two chips is
	the way the ECC bytes are computed; there are differences in the way seek
	times are computed, too.

	I have not found the data book for this chip.  Just a DSR in the VAX BSD
	source code, and another DSR in the HSGPL DSR ROM source code.

	Raphael Nabet, 2003
*/

#include "driver.h"
#include "devices/flopdrv.h"

#include "smc92x4.h"

#define MAX_HFDC 1
#define MAX_SECTOR_LEN 2048	/* ??? */

/*
	hfdc state structure
*/
typedef struct hfdc_t
{
	UINT8 status;	/* controller status */
	UINT8 disk_sel;	/* disk selection state */
	UINT8 regs[10+2];	/* 11th register ("data") is used for communication with disk? */
	int reg_ptr;
	data8_t (*dma_read_callback)(int which, offs_t offset);
	void (*dma_write_callback)(int which, offs_t offset, data8_t data);
	void (*int_callback)(int which, int state);
} hfdc_t;

enum
{
	disk_sel_none = (UINT8) -1
};

enum
{
	hfdc_reg_dma_low = 0,
	hfdc_reg_dma_mid,
	hfdc_reg_dma_high,
	hfdc_reg_sector,
	hfdc_reg_head,
	hfdc_reg_cyl,
	hfdc_reg_sector_count,			/* only used in format(?): 1's complement of the number of sectors per tracks */
	hfdc_reg_retry_count,
	hfdc_reg_mode,
	hfdc_reg_term,
		hfdc_reg_chip_stat /*= hfdc_reg_mode*/,
		hfdc_reg_drive_stat /*= hfdc_reg_term*/
};

/*
	Definition of bits in the status register
*/
#define ST_INTPEND	(1<<7)		/* interrupt pending */
#define ST_DMAREQ	(1<<6)		/* DMA request */
#define ST_DONE		(1<<5)		/* command done */
#define ST_TERMCOD	(3<<3)		/* termination code (see below) */
#define ST_RDYCHNG	(1<<2)		/* ready change */
#define ST_OVRUN	(1<<1)		/* overrun/underrun */
#define ST_BADSECT	(1<<0)		/* bad sector */

/*
	Definition of the termination codes
*/
#define ST_TC_SUCCESS	(0<<3)	/* Successful completion */
#define ST_TC_RDIDERR	(1<<3)	/* Error in READ-ID sequence */
#define ST_TC_VRFYERR	(2<<3)	/* Error in VERIFY sequence */
#define ST_TC_DATAERR	(3<<3)	/* Error in DATA-TRANSFER seq. */


/*
	Definition of bits in the Termination-Conditions register
*/
#define TC_CRCPRE	(1<<7)		/* CRC register preset, must be 1 */
#define TC_UNUSED	(1<<6)		/* bit 6 is not used and must be 0 */
#define TC_INTDONE	(1<<5)		/* interrupt on done */
#define TC_TDELDAT	(1<<4)		/* terminate on deleted data */
#define TC_TDSTAT3	(1<<3)		/* terminate on drive status 3 change */
#define TC_TWPROT	(1<<2)		/* terminate on write-protect (FDD only) */
#define TC_INTRDCH	(1<<1)		/* interrupt on ready change (FDD only) */
#define TC_TWRFLT	(1<<0)		/* interrupt on write-fault (HDD only) */

/*
	Definition of bits in the Disk-Status register
*/
#define DS_SELACK	(1<<7)		/* select acknowledge (harddisk only!) */
#define DS_INDEX	(1<<6)		/* index point */
#define DS_SKCOM	(1<<5)		/* seek complete */
#define DS_TRK00	(1<<4)		/* track 0 */
#define DS_DSTAT3	(1<<3)		/* drive status 3 (MBZ) */
#define DS_WRPROT	(1<<2)		/* write protect (floppy only!) */
#define DS_READY	(1<<1)		/* drive ready bit */
#define DS_WRFAULT	(1<<0)		/* write fault */

typedef struct disk_interface_t
{
	int (*read_sector)(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
	int (*write_sector)(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
	void (*seek)(int which, int disk_unit, int direction);
	UINT8 (*get_disk_status)(int which, int disk_unit);
} disk_interface_t;

static hfdc_t hfdc[MAX_HFDC];

static int floppy_read_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
static int floppy_write_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
static void floppy_seek(int which, int disk_unit, int direction);
static UINT8 floppy_get_disk_status(int which, int disk_unit);

/*static int hard_disk_read_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
static int hard_disk_write_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address);
static UINT8 hard_disk_get_disk_status(int which, int disk_unit);
*/

static disk_interface_t floppy_disk_interface =
{
	floppy_read_sector,
	floppy_write_sector,
	floppy_seek,
	floppy_get_disk_status
};

static disk_interface_t hard_disk_interface =
{
	/*hard_disk_read_sector*/NULL,
	/*hard_disk_write_sector*/NULL,
	NULL,
	/*hard_disk_get_disk_status*/NULL
};

static int floppy_find_sector(int which, int disk_unit, int cylinder, int head, int sector, int *sector_data_id, int *sector_len/*, int *ddam*/)
{
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	while (revolution_count < 4)
	{
		if (floppy_drive_get_next_id(disk_unit, head, &id))
		{
			/* compare id values */
			if ((id.C == cylinder) && (id.H == head) && (id.R == sector))
			{
				* sector_data_id = id.data_id;
				* sector_len = 1 << (id.N+7);
				assert((* sector_len) < MAX_SECTOR_LEN);
				/* get ddam status */
				//* ddam = id.flags & ID_FLAG_DELETED_DATA;
				/* got record type here */
#ifdef VERBOSE
				logerror("sector found! C:$%02x H:$%02x R:$%02x N:$%02x%s\n", id.C, id.H, id.R, id.N, w->ddam ? " DDAM" : "");
#endif
				return TRUE;
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(disk_unit, FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

#ifdef VERBOSE
	logerror("track %d sector %d not found!\n", w->track_reg, w->sector);
#endif

	return FALSE;
}

static int floppy_read_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
	int sector_data_id, sector_len;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;

	if (! floppy_find_sector(which, disk_unit, cylinder, head, sector, & sector_data_id, & sector_len))
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	floppy_drive_read_sector_data(disk_unit, head, sector_data_id, (char *) buf, sector_len);
	for (i=0; i<sector_len; i++)
	{
		(*hfdc[which].dma_write_callback)(which, *dma_address, buf[i]);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}
	return TRUE;
}

static int floppy_write_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
	int sector_data_id, sector_len;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;

	if (! floppy_find_sector(which, disk_unit, cylinder, head, sector, & sector_data_id, & sector_len))
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	for (i=0; i<sector_len; i++)
	{
		buf[i] = (*hfdc[which].dma_read_callback)(which, *dma_address);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}
	floppy_drive_write_sector_data(disk_unit, head, sector_data_id, (char *) buf, sector_len, FALSE);
	return TRUE;
}

static void floppy_seek(int which, int disk_unit, int direction)
{
	floppy_drive_seek(disk_unit, direction);
}

static UINT8 floppy_get_disk_status(int which, int disk_unit)
{
	int status = floppy_status(disk_unit, -1);
	int reply;

	reply = 0;
	if (status & FLOPPY_DRIVE_INDEX)
		reply |= DS_INDEX;
	reply |= DS_SKCOM;
	if (status & FLOPPY_DRIVE_HEAD_AT_TRACK_0)
		reply |= DS_TRK00;
	if (status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED)
		reply |= DS_WRPROT;
	if (status & FLOPPY_DRIVE_READY)
		reply |= DS_READY;

	return reply;
}

/*static int hard_disk_read_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
}*/

/*static int hard_disk_write_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
}*/

/*static UINT8 hard_disk_get_disk_status(int which, int disk_unit)
{
}*/



/*
	Reset the controller
*/
void smc92x4_init(int which, data8_t (*dma_read_callback)(int which, offs_t offset), void (*dma_write_callback)(int which, offs_t offset, data8_t data), void (*int_callback)(int which, int state))
{
	memset(& hfdc[which], 0, sizeof(hfdc[which]));
	hfdc[which].dma_read_callback = dma_read_callback;
	hfdc[which].dma_write_callback = dma_write_callback;
	hfdc[which].int_callback = int_callback;
	if (hfdc[which].int_callback)
		(*hfdc[which].int_callback)(which, 0);
}

/*
	Reset the controller
*/
void smc92x4_reset(int which)
{
	int i;

	hfdc[which].status = 0;			// ???
	for (i=0; i<10; i++)
		hfdc[which].regs[i] = 0;	// ???
	hfdc[which].reg_ptr = 0;		// ???
	if (hfdc[which].int_callback)
		(*hfdc[which].int_callback)(which, 0);
}

/*
	Change the state of IRQ
*/
static void smc92x4_set_interrupt(int which, int state)
{
	if ((state != 0) != ((hfdc[which].status & ST_INTPEND) != 0))
	{
		if (state)
			hfdc[which].status |= ST_INTPEND;
		else
			hfdc[which].status &= ~ST_INTPEND;

		if (hfdc[which].int_callback)
			(*hfdc[which].int_callback)(which, state);
	}
}

/*
	Select currently selected drive 
*/
static int get_selected_drive(int which, disk_interface_t **disk_interface, int *disk_unit)
{
	if (hfdc[which].disk_sel == disk_sel_none)
	{
		* disk_interface = NULL;
		* disk_unit = -1;
		return FALSE;
	}
	else if (hfdc[which].disk_sel & 0x8)
	{
		/* floppy disk */
		/* (hfdc[which].disk_sel & 0x4) is floppy type: (1 for 5.25" drive, 0
		for slower 8" drives???).  Can probably be ignored. */

		/* It seems that these are 4 general purpose output.  Myarc's HFDC card
		use them as floppy select lines, but other hardware will use them for
		other purposes. */
		switch (hfdc[which].regs[hfdc_reg_retry_count] & 0xf)
		{
		case 1:
			*disk_unit = 0;
			break;
		case 2:
			*disk_unit = 1;
			break;
		case 4:
			*disk_unit = 2;
			break;
		case 8:
			*disk_unit = 3;
			break;
		default:
			*disk_unit = -1;
			break;
		}
		if (* disk_unit != -1)
		{
			* disk_interface = &floppy_disk_interface;
			return TRUE;
		}
		else
		{
			* disk_interface = NULL;
			return FALSE;
		}
	}
	else if (hfdc[which].disk_sel & 0x4)
	{
		if (hfdc[which].disk_sel & 0x3)
		{
			* disk_interface = &hard_disk_interface;
			* disk_unit = (hfdc[which].disk_sel & 0x3) - 1;
			return TRUE;
		}
		else
		{
			* disk_interface = NULL;
			* disk_unit = -1;
			return FALSE;
		}
	}
	else
	{
		* disk_interface = NULL;
		* disk_unit = -1;
		return FALSE;
	}
}

/*
	Assert Command Done status bit, triggering interrupts as needed
*/
INLINE void set_command_done(int which)
{
	//assert(! (hfdc[which].status & ST_DONE))
	hfdc[which].status |= ST_DONE;
	if (hfdc[which].regs[hfdc_reg_term] & TC_INTDONE)
		smc92x4_set_interrupt(which, 1);
}

static void do_step(int which, int mode)
{
	disk_interface_t *disk_interface;
	int disk_unit;

	hfdc[which].status = 0;

	if (!get_selected_drive(which, & disk_interface, & disk_unit))
	{
		/* no unit has been selected */
		//hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

	if (disk_interface->seek)
		(*disk_interface->seek)(which, disk_unit, (mode & 2) ? -1 : +1);

cleanup:
	/* update register state */
	set_command_done(which);

	/* set chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = (disk_interface && disk_interface->get_disk_status)
												? (*disk_interface->get_disk_status)(which, disk_unit)
												: 0;
}

static void do_restore(int which, int mode)
{
	disk_interface_t *disk_interface;
	int disk_unit;

	hfdc[which].status = 0;

	if (!get_selected_drive(which, & disk_interface, & disk_unit))
	{
		/* no unit has been selected */
		//hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

	if (disk_interface->get_disk_status && disk_interface->seek)
	{
		while (! ((*disk_interface->get_disk_status)(which, disk_unit) & DS_TRK00))
			(*disk_interface->seek)(which, disk_unit, -1);
	}

cleanup:
	/* update register state */
	set_command_done(which);

	/* set chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = (disk_interface && disk_interface->get_disk_status)
												? (*disk_interface->get_disk_status)(which, disk_unit)
												: 0;
}

static void do_read_logical(int which, int mode)
{
	disk_interface_t *disk_interface;
	int disk_unit;
	int dma_address;
	int cylinder, head, sector;

	hfdc[which].status = 0;

	dma_address = hfdc[which].regs[hfdc_reg_dma_low]
					| (hfdc[which].regs[hfdc_reg_dma_mid] << 8)
					| (hfdc[which].regs[hfdc_reg_dma_high] << 16);

	sector = hfdc[which].regs[hfdc_reg_sector];
	head = hfdc[which].regs[hfdc_reg_head] & 0xf;
	cylinder = (((int) hfdc[which].regs[hfdc_reg_head] << 4) & 0x700)
				| hfdc[which].regs[hfdc_reg_cyl];

	if (!get_selected_drive(which, & disk_interface, & disk_unit))
	{
		/* no unit has been selected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

#if 0
	if (disk_interface->get_disk_status && ! ((*disk_interface->get_disk_status)(which, disk_unit) & DS_READY))
	{
		/* unit is not ready */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}
#endif

	if (disk_interface->get_disk_status && (hfdc[which].regs[hfdc_reg_term] & TC_TWPROT) && ((*disk_interface->get_disk_status)(which, disk_unit) & DS_WRPROT))
	{
		/* unit is write protected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

	/* do read sector */
	if (! disk_interface->read_sector)
	{
		/* read is unimplemented */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}
	else
		(void) (*disk_interface->read_sector)(which, disk_unit, cylinder, head, sector, &dma_address);

cleanup:
	/* update register state */
	set_command_done(which);

	/* update DMA address (right???) */
	hfdc[which].regs[hfdc_reg_dma_low] = dma_address & 0xff;
	hfdc[which].regs[hfdc_reg_dma_mid] = (dma_address >> 8) & 0xff;
	hfdc[which].regs[hfdc_reg_dma_high] = (dma_address >> 16) & 0xff;

	/* set chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = (disk_interface && disk_interface->get_disk_status)
												? (*disk_interface->get_disk_status)(which, disk_unit)
												: 0;
}

static void do_write_logical(int which, int mode)
{
	disk_interface_t *disk_interface;
	int disk_unit;
	int dma_address;
	int cylinder, head, sector;

	hfdc[which].status = 0;

	dma_address = hfdc[which].regs[hfdc_reg_dma_low]
					| (hfdc[which].regs[hfdc_reg_dma_mid] << 8)
					| (hfdc[which].regs[hfdc_reg_dma_high] << 16);

	sector = hfdc[which].regs[hfdc_reg_sector];
	head = hfdc[which].regs[hfdc_reg_head] & 0xf;
	cylinder = ((hfdc[which].regs[hfdc_reg_head] << 4) & 0x700)
				| hfdc[which].regs[hfdc_reg_cyl];

	if (!get_selected_drive(which, & disk_interface, & disk_unit))
	{
		/* no unit has been selected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

#if 0
	if (disk_interface->get_disk_status && ! ((*disk_interface->get_disk_status)(which, disk_unit) & DS_READY))
	{
		/* unit is not ready */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}
#endif

	if (disk_interface->get_disk_status && (hfdc[which].regs[hfdc_reg_term] & TC_TWPROT) && ((*disk_interface->get_disk_status)(which, disk_unit) & DS_WRPROT))
	{
		/* unit is write protected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}

	/* do write sector */
	if (! disk_interface->write_sector) 
	{
		/* write is unimplemented */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}
	else
		(void) (*disk_interface->write_sector)(which, disk_unit, cylinder, head, sector, &dma_address);

cleanup:
	/* update register state */
	set_command_done(which);

	/* update DMA address (right???) */
	hfdc[which].regs[hfdc_reg_dma_low] = dma_address & 0xff;
	hfdc[which].regs[hfdc_reg_dma_mid] = (dma_address >> 8) & 0xff;
	hfdc[which].regs[hfdc_reg_dma_high] = (dma_address >> 16) & 0xff;

	/* set chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = (disk_interface && disk_interface->get_disk_status)
												? (*disk_interface->get_disk_status)(which, disk_unit)
												: 0;
}

/*
	Process a Command
*/
static void smc92x4_process_command(int which, int opcode)
{
	if (opcode < 0x80)
	{
		switch (opcode >> 4)
		{
		case 0x0:	/* misc commands */
			if (opcode < 0x04)
			{
				switch (opcode)
				{
				case 0x00:	/* RESET */
					/* terminate non-data-transfer cmds */
					logerror("smc92x4 reset command\n");
					break;
				case 0x01:	/* DRDESELECT */
					/* done when no drive is in use */
					logerror("smc92x4 drdeselect command\n");
					hfdc[which].disk_sel = disk_sel_none;
					set_command_done(which);
					break;
				case 0x02:	/* RESTORE */
				case 0x03:	/* RESTORE (used by Myarc HFDC DSR, difference with opcode 2 unknown) */
					logerror("smc92x4 restore command %X\n", opcode & 0x01);
					do_restore(which, opcode & 0x01);
					break;
				}
			}
			else if (opcode < 0x08)
			{
				/* STEP (floppy drive only?) */
				/* bit 1: direction (0 -> inward, 1 -> outward i.e. toward cyl #0) */
				/* bit 0: ??? */
				logerror("smc92x4 step command %X\n", opcode & 0x3);
				do_step(which, opcode & 0x03);
			}
			else
			{
				/* ??? */
				logerror("smc92x4 unknown command %X\n", opcode);
			}
			break;
		case 0x1:
			/* POLLDRIVE */
			/* bit 3-0: ??? */
			logerror("smc92x4 polldrive command %X\n", opcode & 0xf);
			break;
		case 0x2:
			/* DRSELECT */
			/* bit 3: 0 for hard disk, 1 for floppy disk */
			/* bit 2: always 1 for hard disk, 0 for (high-speed?) "RX30" floppy,
				1 for (low-speed?) "RX50" floppy. */
			/* bit 1 & 0: unit number (0-3)*/
			logerror("smc92x4 drselect command %X\n", opcode & 0xf);
			hfdc[which].disk_sel = opcode & 0xf;
			set_command_done(which);
			break;
		case 0x3:
			/* ??? */
			logerror("smc92x4 unknown command %X\n", opcode);
			break;
		case 0x4:
			/* SETREGPTR */
			/* bit 3-0: reg-number */
			logerror("smc92x4 setregptr command %X\n", opcode & 0xf);
			hfdc[which].reg_ptr = opcode & 0xf;
			set_command_done(which);
			break;
		case 0x5:
			if (opcode < 0x58)
			{
				/* SEEKREADID */
				/* bit 2-0: ??? */
				logerror("smc92x4 seekreadid command %X\n", opcode & 0x7);
			}
			else
			{
				/* READ */
				/* bits 2-1: 0 -> read physical, 1 -> read track, 2 -> read
					logical and terminate if bad sector, 3 -> read logical and
					bypass bad sectors */
				/* bit 0: 0 -> test only, do not transfer data, 1 -> transfer
					data (?) */
				switch ((opcode >> 1) & 0x3)
				{
				case 0:
					logerror("smc92x4 read physical command %X\n", opcode & 0x1);
					break;
				case 1:
					logerror("smc92x4 read track command %X\n", opcode & 0x1);
					break;
				case 2:
				case 3:
					logerror("smc92x4 read logical command %X\n", opcode & 0x3);
					do_read_logical(which, opcode & 0x3);
					break;
				}
			}
		case 0x6:
			/* FORMATTRACK */
			/* bit 3-0: ??? */
			logerror("smc92x4 formattrack command %X\n", opcode & 0xf);
			break;
		case 0x7:
			/* ??? */
			logerror("smc92x4 unknown command %X\n", opcode);
			break;
		}
	}
	else
	{
		/* WRITE */
		/* bits 6-5: 0 -> write physical, 1 -> ??? (used by all DSRs I know),
			2 -> write logical, 3 -> ??? (bit 5 -> bypass bad sectors???) */
		/* bits 4: ddmark??? */
		/* bits 3-0: precompensation??? */
		logerror("smc92x4 write command %X\n", opcode & 0x7f);
		switch ((opcode >> 5) & 0x3)
		{
		case 0:
			logerror("smc92x4 write physical command %X\n", opcode & 0x7f);
			break;
		case 1:
			logerror("smc92x4 write logical command %X\n", opcode & 0x7f);
			do_write_logical(which, opcode & 0x7f);
			break;
		case 2:
		case 3:
			logerror("smc92x4 write logical command??? %X\n", opcode & 0x7f);
			break;
		}
	}
}

/*
	Memory accessors
*/

int smc92x4_r(int which, int offset)
{
	int reply = 0;

	switch (offset & 1)
	{
	case 0:
		/* data register */
		logerror("smc92x4 data read\n");
		if (hfdc[which].reg_ptr < 10)
		{
			/* config register */
			reply = hfdc[which].regs[(hfdc[which].reg_ptr < 8) ? (hfdc[which].reg_ptr) : (hfdc[which].reg_ptr + 2)];
			hfdc[which].reg_ptr++;
		}
		else if (hfdc[which].reg_ptr == 10)
		{
			/* disk data(?) */
		}
		else
		{
			/* ??? */
		}
		break;

	case 1:
		/* status register */
		logerror("smc92x4 status read\n");
		reply = hfdc[which].status;
		smc92x4_set_interrupt(which, 0);	/* right??? */
		break;
	}

	return reply;
}

void smc92x4_w(int which, int offset, int data)
{
	data &= 0xff;

	switch (offset & 1)
	{
	case 0:
		/* data register */
		logerror("smc92x4 data write %X\n", data);
		if (hfdc[which].reg_ptr < 10)
		{
			/* config register */
			hfdc[which].regs[hfdc[which].reg_ptr] = data;
			hfdc[which].reg_ptr++;
		}
		else if (hfdc[which].reg_ptr == 10)
		{
			/* disk data(?) */
		}
		else
		{
			/* ??? */
		}
		break;

	case 1:
		/* command register */
		smc92x4_process_command(which, data);
		break;
	}
}

READ_HANDLER(smc92x4_0_r)
{
	return smc92x4_r(0, offset);
}

WRITE_HANDLER(smc92x4_0_w)
{
	smc92x4_w(0, offset, data);
}

