/*
	Corvus Concept driver

	Raphael Nabet, 2003
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/concept.h"
#include "machine/6522via.h"
#include "machine/mm58274c.h"	/* hopefully, the mm58274 is compatible with mm58174 (I haven't checked) */
//#include "includes/6551.h"
#include "includes/wd179x.h"
#include "cpu/m68000/m68k.h"

/* interrupt priority encoder */
static UINT8 pending_interrupts;
enum
{
	IOCINT_level = 1,	/* serial lines (CTS, DSR & DCD) and I/O ports */
	SR1INT_level,		/* serial port 1 acia */
	OMINT_level,		/* omninet */
	SR0INT_level,		/* serial port 0 acia */
	TIMINT_level,		/* via */
	KEYINT_level,		/* keyboard acia */
	MIINT_level			/* reserved */
};

/* Clock interface */
static char clock_enable;
static char clock_address;

/* Omninet */
/*static int ready;*/			/* ready line from monochip, role unknown */

/* Via */
static READ_HANDLER(via_in_a);
static WRITE_HANDLER(via_out_a);
static READ_HANDLER(via_in_b);
static WRITE_HANDLER(via_out_b);
static WRITE_HANDLER(via_out_cb2);
static void via_irq_func(int state);


static struct via6522_interface concept_via6522_intf =
{	/* main via */
	via_in_a, via_in_b,
	NULL, NULL,
	NULL, NULL,
	via_out_a, via_out_b,
	NULL, via_out_cb2,
	via_irq_func,
	NULL,
	NULL,
	NULL,
	NULL,
};

/* Expansion slots */

struct
{
	mem_read_handler reg_read;
	mem_write_handler reg_write;
	mem_read_handler rom_read;
	mem_write_handler rom_write;
} expansion_slots[4];

static void concept_fdc_init(int slot);


MACHINE_INIT(concept)
{
	cpu_setbank(1, memory_region(REGION_CPU1) + rom0_base);

	/* initialize int state */
	pending_interrupts = 0;

	/* configure via */
	via_config(0, & concept_via6522_intf);
	via_reset();

	/* initialize clock interface */
	clock_enable = 0/*1*/;
	mm58274c_init(0);

	/* initialize expansion slots */
	memset(expansion_slots, 0, sizeof(expansion_slots));

	concept_fdc_init(0);
}

static void install_expansion_slot(int slot,
									mem_read_handler reg_read, mem_write_handler reg_write,
									mem_read_handler rom_read, mem_write_handler rom_write)
{
	expansion_slots[slot].reg_read = reg_read;
	expansion_slots[slot].reg_write = reg_write;
	expansion_slots[slot].rom_read = rom_read;
	expansion_slots[slot].rom_write = rom_write;
}

VIDEO_START(concept)
{
	return 0;
}

VIDEO_UPDATE(concept)
{
	UINT16 *v;
	int x, y;
	UINT8 line_buffer[720];
	/* resolution is 720*560 */

	v = /*videoram_ptr*/ (UINT16 *) (memory_region(REGION_CPU1) + 0x80000);

	for (y = 0; y < 560; y++)
	{
		for (x = 0; x < 720; x++)
			line_buffer[720-1-x] = (v[(x+48+y*768)>>4] & (0x8000 >> ((x+48+y*768) & 0xf))) ? 1 : 0;
		draw_scanline8(bitmap, 0, (560-1-y), 720, line_buffer, Machine->pens, -1);
	}
}

static void concept_set_interrupt(int level, int state)
{
	int interrupt_mask;
	int final_level;

	if (state)
		pending_interrupts |= 1 << level;
	else
		pending_interrupts &= ~ (1 << level);

	for (final_level = 7, interrupt_mask = pending_interrupts; (final_level > 0) && ! (interrupt_mask & 0x80); final_level--, interrupt_mask <<= 1)
		;

	if (final_level)
		/* assert interrupt */
		cpu_set_irq_line_and_vector(0, M68K_IRQ_1+final_level-1, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else
		/* clear all interrupts */
		cpu_set_irq_line_and_vector(0, M68K_IRQ_1, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
}

/*
	VIA port A

	0: omninet ready (I)
	1: CTS0 (I)
	2: CTS1 (I)
	3: DSR0 (I)
	4: DSR1 (I)
	5: DCD0 (I)
	6: DCD1 (I)
	7: IOX (O)
*/
static READ_HANDLER(via_in_a)
{
	return 1;		/* omninet ready always 1 */
}

static WRITE_HANDLER(via_out_a)
{
	/*iox = (data & 0x80) != 0;*/
}

/*
	VIA port B

	0: video off (O)
	1: video address 17 (O)
	2: video address 18 (O)
	3: monitor orientation (I)
	4: CH rate select DC0 (serial port line) (O)
	5: CH rate select DC1 (serial port line) (O)
	6: boot switch 0 (I)
	7: boot switch 1 (I)
*/
static READ_HANDLER(via_in_b)
{
	return 0;
}

static WRITE_HANDLER(via_out_b)
{
}

/*
	VIA CB2: used as sound output
*/
static WRITE_HANDLER(via_out_cb2)
{
}

/*
	VIA irq -> 68k level 5
*/
static void via_irq_func(int state)
{
	concept_set_interrupt(TIMINT_level, state);
}

READ16_HANDLER(concept_io_r)
{
	if (! ACCESSING_LSB)
		return 0;

	switch ((offset >> 8) & 7)
	{
	case 0:
		/* I/O slot regs */
		switch ((offset >> 4) & 7)
		{
		case 1:
			/* IO1 registers */
			break;

		case 2:
			/* IO2 registers */
			break;

		case 3:
			/* IO3 registers */
			break;

		case 4:
			/* IO4 registers */
			break;

		default:
			/* ??? */
			break;
		}
		break;

	case 1:
		/* IO1 ROM */
		break;

	case 2:
		/* IO2 ROM */
		break;

	case 3:
		/* IO3 ROM */
		break;

	case 4:
		/* IO4 ROM */
		break;

	case 5:
		/* slot status */
		break;

	case 6:
		/* calendar R/W */
		if (!clock_enable)
			return mm58274c_r(0, clock_address);
		break;

	case 7:
		/* I/O ports */
		switch ((offset >> 4) & 7)
		{
		case 0:
			/* NKBP keyboard */
		case 1:
			/* NSR0 data comm port 0 */
		case 2:
			/* NSR1 data comm port 1 */
			/*return acia_6551_r((offset >> 4) & 7, offset & 0x3);*/
			break;

		case 3:
			/* NVIA versatile system interface */
			return via_read(0, offset & 0xf);
			break;

		case 4:
			/* NCALM clock calendar address and strobe register */
			/* write-only? */
			break;

		case 5:
			/* NOMNI omninet strobe */
			break;

		case 6:
			/* NOMOFF reset omninet interrupt flip-flop */
			break;

		case 7:
			/* NIOSTRB external I/O ROM strobe */
			break;
		}
		break;
	}

	return 0;
}

WRITE16_HANDLER(concept_io_w)
{
	if (! ACCESSING_LSB)
		return;

	data &= 0xff;

	switch ((offset >> 8) & 7)
	{
	case 0:
		/* I/O slot regs */
		switch ((offset >> 4) & 7)
		{
		case 1:
			/* IO1 registers */
			break;

		case 2:
			/* IO2 registers */
			break;

		case 3:
			/* IO3 registers */
			break;

		case 4:
			/* IO4 registers */
			break;

		default:
			/* ??? */
			break;
		}
		break;

	case 1:
		/* IO1 ROM */
		break;

	case 2:
		/* IO2 ROM */
		break;

	case 3:
		/* IO3 ROM */
		break;

	case 4:
		/* IO4 ROM */
		break;

	case 5:
		/* slot status */
		break;

	case 6:
		/* calendar R/W */
		if (!clock_enable)
			mm58274c_w(0, clock_address, data & 0xf);
		break;

	case 7:
		/* I/O ports */
		switch ((offset >> 4) & 7)
		{
		case 0:
			/* NKBP keyboard */
		case 1:
			/* NSR0 data comm port 0 */
		case 2:
			/* NSR1 data comm port 1 */
			/*acia_6551_w((offset >> 4) & 7, offset & 0x3, data);*/
			break;

		case 3:
			/* NVIA versatile system interface */
			via_write(0, offset & 0xf, data);
			break;

		case 4:
			/* NCALM clock calendar address and strobe register */
			if (clock_enable != ((data & 0x10) != 0))
			{
				clock_enable = (data & 0x10) != 0;
				if (! clock_enable)
					/* latch address when enable goes low */
					clock_address = data & 0x0f;
			}
			/*volume_control = (data & 0x20) != 0;*/
			/*alt_map = (data & 0x40) != 0;*/
			break;

		case 5:
			/* NOMNI omninet strobe */
			break;

		case 6:
			/* NOMOFF reset omninet interrupt flip-flop */
			break;

		case 7:
			/* NIOSTRB external I/O ROM strobe */
			break;
		}
		break;
	}
}

/*
	Concept fdc controller
*/

static UINT8 fdc_local_status;
static UINT8 fdc_local_command;
enum
{
	LS_DRQ_bit		= 0,	// DRQ
	LS_INT_bit		= 1,	// INT
	LS_SS_bit		= 4,	// 1 if single-sided (floppy or drive?)
	LS_8IN_bit		= 5,	// 1 if 8" floppy drive?
	LS_DSKCHG_bit	= 6,	// 0 if disk changed, 1 if not
	LS_SD_bit		= 7,	// 1 if single density

	LS_DRQ_mask		= (1 << LS_DRQ_bit),
	LS_INT_mask		= (1 << LS_INT_bit),
	LS_SS_mask		= (1 << LS_SS_bit),
	LS_8IN_mask		= (1 << LS_8IN_bit),
	LS_DSKCHG_mask	= (1 << LS_DSKCHG_bit),
	LS_SD_mask		= (1 << LS_SD_bit)
};
enum
{
	LC_FLPSD1_bit	= 0,	// 0 if side 0 , 1 if side 1
	LC_DE0_bit		= 1,	// drive select bit 0
	LC_DE1_bit		= 4,	// drive select bit 1
	LC_MOTOROF_bit	= 5,	// 1 if motor to be turned off
	LC_FLP8IN_bit	= 6,	// 1 to select 8", 0 for 5"1/4 (which I knew what it means)
	LC_FMMFM_bit	= 7,	// 1 to select single density, 0 for double

	LC_FLPSD1_mask	= (1 << LC_FLPSD1_bit),
	LC_DE0_mask		= (1 << LC_DE0_bit),
	LC_DE1_mask		= (1 << LC_DE1_bit),
	LC_MOTOROF_mask	= (1 << LC_MOTOROF_bit),
	LC_FLP8IN_mask	= (1 << LC_FLP8IN_bit),
	LC_FMMFM_mask	= (1 << LC_FMMFM_bit)
};

static void fdc_callback(int event);

static READ_HANDLER(concept_fdc_reg_r);
static WRITE_HANDLER(concept_fdc_reg_w);
static READ_HANDLER(concept_fdc_rom_r);

static void concept_fdc_init(int slot)
{
	fdc_local_status = 0;
	fdc_local_command = 0;

	wd179x_init(WD_TYPE_179X, fdc_callback);

	install_expansion_slot(slot, concept_fdc_reg_r, concept_fdc_reg_w, concept_fdc_rom_r, NULL);
}

static void fdc_callback(int event)
{
	switch (event)
	{
	case WD179X_IRQ_CLR:
		fdc_local_status &= ~LS_INT_mask;
		break;
	case WD179X_IRQ_SET:
		fdc_local_status |= LS_INT_mask;
		break;
	case WD179X_DRQ_CLR:
		fdc_local_status &= ~LS_DRQ_mask;
		break;
	case WD179X_DRQ_SET:
		fdc_local_status |= LS_DRQ_mask;
		break;
	}
}

static READ_HANDLER(concept_fdc_reg_r)
{
	switch (offset)
	{
	case 0:
		/* local Status reg */
		return fdc_local_status;
		break;

	case 8:
		/* FDC STATUS REG */
		return wd179x_status_r(offset);
		break;

	case 9:
		/* FDC TRACK REG */
		return wd179x_track_r(offset);
		break;

	case 10:
		/* FDC SECTOR REG */
		return wd179x_sector_r(offset);
		break;

	case 11:
		/* FDC DATA REG */
		return wd179x_data_r(offset);
		break;
	}

	return 0;
}

static WRITE_HANDLER(concept_fdc_reg_w)
{
	switch (offset)
	{
	case 0:
		/* local command reg */
		fdc_local_command = data;

		wd179x_set_side((data & LC_FLPSD1_mask) != 0);
		wd179x_set_drive(((data >> LC_DE0_bit) & 1) | ((data >> (LC_DE1_bit-1)) & 2));
		/*motor_on = (data & LC_MOTOROF_mask) == 0;*/
		/*flp_8in = (data & LC_FLP8IN_mask) != 0;*/
		wd179x_set_density((data & LC_FMMFM_mask) ? DEN_FM_LO : DEN_MFM_LO);
		break;

	case 8:
		/* FDC COMMAMD REG */
		wd179x_command_w(offset, data);
		break;

	case 9:
		/* FDC TRACK REG */
		wd179x_track_w(offset, data);
		break;

	case 10:
		/* FDC SECTOR REG */
		wd179x_sector_w(offset, data);
		break;

	case 11:
		/* FDC DATA REG */
		wd179x_data_w(offset, data);
		break;
	}
}

static READ_HANDLER(concept_fdc_rom_r)
{
	UINT8 data[8] = "CORVUS01";
	return (offset < 8) ? data[offset] : 0;
}
