/*
	Thierry Nouspikel's IDE card emulation

	This card is just a prototype.  It has been designed by Thierry Nouspikel,
	and its description was published in 2001.

	The specs have been published in <http://www.nouspikel.com/ti99/ide.html>.

	The IDE interface is quite simple, since it only implements PIO transfer.
	The card includes a clock chip to timestamp files, and an SRAM for the DSR.
	It should be possible to use a battery backed DSR SRAM, but since the clock
	chip includes 4kb of battery-backed RAM, a bootstrap loader can be saved in
	the clock SRAM in order to load the DSR from the HD when the computer
	starts.

	Raphael Nabet, 2002-2003.
*/

/*#include "harddisk.h"*/
#include "machine/idectrl.h"
#include "devices/idedrive.h"
#include "machine/rtc65271.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "99_ide.h"


/* prototypes */
static void ide_interrupt_callback(int state);

static int ide_cru_r(int offset);
static void ide_cru_w(int offset, int data);
static READ_HANDLER(ide_mem_r);
static WRITE_HANDLER(ide_mem_w);

/* pointer to the IDE RAM area */
static UINT8 *ti99_ide_RAM;
static int cur_page;
enum
{	/* 0xff for 2 mbytes, 0x3f for 512kbytes, 0x03 for 32 kbytes */
	page_mask = /*0xff*/0x3f
};

static const ti99_peb_card_handlers_t ide_handlers =
{
	ide_cru_r,
	ide_cru_w,
	ide_mem_r,
	ide_mem_w
};

static int sram_enable;
static int sram_enable_dip = /*0*/1;
static int cru_register;
enum
{
	cru_reg_page_switching = 0x04,
	cru_reg_page_0 = 0x08,
	/*cru_reg_rambo = 0x10,*/	/* not emulated */
	cru_reg_wp = 0x20,
	/*cru_reg_unused = 0x40,*/
	cru_reg_reset = 0x80
};
static int input_latch, output_latch;

static int ide_irq;

struct ide_interface ti99_ide_interface =
{
	ide_interrupt_callback
};

/*
	ide_interrupt_callback()

	set a flag
*/
static void ide_interrupt_callback(int state)
{
	ide_irq = state;
}

/*
	Init an IDE image
*/
DEVICE_INIT( ti99_ide )
{
	return ide_hd_init(image, 0, 0, &ti99_ide_interface);
}

/*
	Load an IDE image
*/
DEVICE_LOAD( ti99_ide )
{
	return ide_hd_load(image, file, open_mode, 0, 0, &ti99_ide_interface);
}

/*
	Unload an IDE image
*/
DEVICE_UNLOAD( ti99_ide )
{
	ide_hd_unload(image, 0, 0, &ti99_ide_interface);
}

/*
	Reset ide card, set up handlers
*/
void ti99_ide_init(void)
{
	rtc65271_init(memory_region(region_dsr) + offset_ide_ram2);

	ti99_ide_RAM = memory_region(region_dsr) + offset_ide_ram;

	ti99_peb_set_card_handlers(0x1000, & ide_handlers);

	ide_hd_machine_init(0, 0, & ti99_ide_interface);

	cur_page = 0;
	sram_enable = 0;
	cru_register = 0;
}

/*
	Read ide CRU interface
*/
static int ide_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	default:
		reply = cru_register;
		if (sram_enable_dip)
			reply |= 2;
		if (! ide_irq)
			reply |= 1;
		break;
	}

	return reply;
}

/*
	Write ide CRU interface
*/
static void ide_cru_w(int offset, int data)
{
	offset &= 7;


	switch (offset)
	{
	case 0:			/* turn card on: handled by core */
		break;

	case 1:			/* enable SRAM or registers in 0x4000-0x40ff */
		sram_enable = data;
		break;

	case 2:			/* enable SRAM page switching */
	case 3:			/* force SRAM page 0 */
	case 4:			/* enable SRAM in 0x6000-0x7000 ("RAMBO" mode) */
	case 5:			/* write-protect RAM */
	case 6:			/* not used */
	case 7:			/* reset drive */
		if (data)
			cru_register |= 1 << offset;
		else
			cru_register &= ~ (1 << offset);

		if ((offset == 7) && data)
			ide_controller_reset(0);
		break;
	}
}

/*
	read a byte in ide DSR space
*/
static READ_HANDLER(ide_mem_r)
{
	int reply = 0;


	if ((offset <= 0xff) && (sram_enable == sram_enable_dip))
	{	/* registers */
		switch ((offset >> 5) & 0x3)
		{
		case 0:		/* RTC RAM */
			if (offset & 0x80)
				/* RTC RAM page register */
				reply = rtc65271_r(1, (offset & 0x1f) | 0x20);
			else
				/* RTC RAM read */
				reply = rtc65271_r(1, offset);
			break;
		case 1:		/* RTC registers */
			if (offset & 0x10)
				/* register data */
				reply = rtc65271_r(0, 1);
			else
				/* register select */
				reply = rtc65271_r(0, 0);
			break;
		case 2:		/* IDE registers set 1 (CS1Fx) */
			if (offset & 1)
			{
				if (! (offset & 0x10))
					reply = ide_bus_0_r(0, (offset >> 1) & 0x7);

				input_latch = (reply >> 8) & 0xff;
				reply &= 0xff;
			}
			else
				reply = input_latch;
			break;
		case 3:		/* IDE registers set 2 (CS3Fx) */
			if (offset & 1)
			{
				if (! (offset & 0x10))
					reply = ide_bus_0_r(1, (offset >> 1) & 0x7);

				input_latch = (reply >> 8) & 0xff;
				reply &= 0xff;
			}
			else
				reply = input_latch;
			break;
		}
	}
	else
	{	/* sram */
		if ((cru_register & cru_reg_page_0) || (offset >= 0x1000))
			reply = ti99_ide_RAM[offset+0x2000*cur_page];
		else
			reply = ti99_ide_RAM[offset];
	}

	return reply;
}

/*
	write a byte in ide DSR space
*/
static WRITE_HANDLER(ide_mem_w)
{
	if (cru_register & cru_reg_page_switching)
	{
		cur_page = (offset >> 1) & page_mask;
	}

	if ((offset <= 0xff) && (sram_enable == sram_enable_dip))
	{	/* registers */
		switch ((offset >> 5) & 0x3)
		{
		case 0:		/* RTC RAM */
			if (offset & 0x80)
				/* RTC RAM page register */
				rtc65271_w(1, (offset & 0x1f) | 0x20, data);
			else
				/* RTC RAM write */
				rtc65271_w(1, offset, data);
			break;
		case 1:		/* RTC registers */
			if (offset & 0x10)
				/* register data */
				rtc65271_w(0, 1, data);
			else
				/* register select */
				rtc65271_w(0, 0, data);
			break;
		case 2:		/* IDE registers set 1 (CS1Fx) */
			if (offset & 1)
				output_latch = data;
			else
				ide_bus_0_w(0, (offset >> 1) & 0x7, ((int) data << 8) | output_latch);
			break;
		case 3:		/* IDE registers set 2 (CS3Fx) */
			if (offset & 1)
				output_latch = data;
			else
				ide_bus_0_w(1, (offset >> 1) & 0x7, ((int) data << 8) | output_latch);
			break;
		}
	}
	else
	{	/* sram */
		if (! (cru_register & cru_reg_wp))
		{
			if ((cru_register & cru_reg_page_0) || (offset >= 0x1000))
				ti99_ide_RAM[offset+0x2000*cur_page] = data;
			else
				ti99_ide_RAM[offset] = data;
		}
	}
}
