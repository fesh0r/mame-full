/***************************************************************************
	zx.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	TODO:
	Find a clean solution for putting the tape image into memory.
	Right now only loading through the IO emulation works (and takes time :)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"

DRIVER_INIT(zx)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i = 0; i < 256; i++)
		gfx[i] = i;
}

static OPBASE_HANDLER(zx_setopbase)
{
	if (address & 0x8000)
		return zx_ula_r(address, REGION_CPU1);
	return address;
}

static OPBASE_HANDLER( pc8300_setopbase )
{
	if (address & 0x8000)
		return zx_ula_r(address, REGION_GFX2);
	return address;
}

static void common_init_machine(void)
{
	memory_set_opbase_handler(0, zx_setopbase);
}

MACHINE_INIT( zx80 )
{
	if (readinputport(0) & 0x80)
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA8_RAM);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA8_RAM);
	}
	else
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA8_NOP);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA8_NOP);
	}
	common_init_machine();
}

MACHINE_INIT( zx81 )
{
	if (readinputport(0) & 0x80)
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA8_RAM);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA8_RAM);
	}
	else
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA8_NOP);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA8_NOP);
	}
	common_init_machine();
}

MACHINE_INIT( pc8300 )
{
	memory_set_opbase_handler(0, pc8300_setopbase);
}

WRITE_HANDLER ( zx_io_w )
{
	logerror("IOW %3d $%04X", cpu_getscanline(), offset);
	if ((offset & 2) == 0)
	{
		logerror(" ULA NMIs off\n");
		timer_reset(ula_nmi, TIME_NEVER);
	}
	else if ((offset & 1) == 0)
	{
		logerror(" ULA NMIs on\n");

		timer_adjust(ula_nmi, 0, 0, TIME_IN_CYCLES(207, 0));

		/* remove the IRQ */
		ula_irq_active = 0;
	}
	else
	{
		logerror(" ULA IRQs on\n");
		zx_ula_bkgnd(1);
		if (ula_frame_vsync == 2)
		{
			cpu_spinuntil_time(cpu_getscanlinetime(Machine->drv->screen_height - 1));
			ula_scanline_count = Machine->drv->screen_height - 1;
		}
	}
}

READ_HANDLER ( zx_io_r )
{
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= readinputport(1);
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= readinputport(2);
		if ((offset & 0x0400) == 0)
			data &= readinputport(3);
		if ((offset & 0x0800) == 0)
			data &= readinputport(4) & extra1;
		if ((offset & 0x1000) == 0)
			data &= readinputport(5) & extra2;
		if ((offset & 0x2000) == 0)
			data &= readinputport(6);
		if ((offset & 0x4000) == 0)
			data &= readinputport(7);
		if ((offset & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->drv->frames_per_second > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			logerror("IOR %3d $%04X data $%02X (ULA IRQs off)\n", cpu_getscanline(), offset, data);
			zx_ula_bkgnd(0);
			ula_irq_active = 0;
		}
		else
		{
			/*data &= ~zx_tape_get_bit();*/
			logerror("IOR %3d $%04X data $%02X (tape)\n", cpu_getscanline(), offset, data);
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			logerror("vsync starts in scanline %3d\n", cpu_getscanline());
		}
	}
	else
	{
		logerror("IOR %3d $%04X data $%02X\n", cpu_getscanline(), offset, data);
	}
	return data;
}

READ_HANDLER ( pow3000_io_r )
{
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= readinputport(1) & extra1;
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= readinputport(2) & extra2;
		if ((offset & 0x0400) == 0)
			data &= readinputport(3);
		if ((offset & 0x0800) == 0)
			data &= readinputport(4);
		if ((offset & 0x1000) == 0)
			data &= readinputport(5);
		if ((offset & 0x2000) == 0)
			data &= readinputport(6);
		if ((offset & 0x4000) == 0)
			data &= readinputport(7);
		if ((offset & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->drv->frames_per_second > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			logerror("IOR %3d $%04X data $%02X (ULA IRQs off)\n", cpu_getscanline(), offset, data);
			zx_ula_bkgnd(0);
			ula_irq_active = 0;
		}
		else
		{
			/*data &= ~zx_tape_get_bit();*/
			logerror("IOR %3d $%04X data $%02X (tape)\n", cpu_getscanline(), offset, data);
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			logerror("vsync starts in scanline %3d\n", cpu_getscanline());
		}
	}
	else
	{
		logerror("IOR %3d $%04X data $%02X\n", cpu_getscanline(), offset, data);
	}
	return data;
}
