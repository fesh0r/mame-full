/***************************************************************************
    
	IBM AT Compatibles

***************************************************************************/

#include "driver.h"

#include "cpu/i86/i286.h"

#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pcshare.h"
#include "includes/ibmat.h"
#include "includes/at.h"
#include "includes/pckeybrd.h"
#include "includes/sblaster.h"

#include "8237dma.h"

static SOUNDBLASTER_CONFIG soundblaster = { 1,5, {1,0} };

static struct pit8253_config at_pit8253_config =
{
	TYPE8254,
	{
		{
			4770000/4,				/* heartbeat IRQ */
			pic8259_0_issue_irq,
			NULL
		}, {
			4770000/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4770000/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};



static void at286_set_gate_a20(int a20)
{
	i286_set_address_mask(a20 ? 0x00ffffff : 0x000fffff);
}



static void at386_set_gate_a20(int a20)
{
	offs_t mirror = a20 ? 0xff000000 : 0xfff00000;
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x000000, 0x09ffff, 0, mirror, MRA32_BANK10);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x09ffff, 0, mirror, MWA32_BANK10);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0a0000, 0x0affff, 0, mirror, MRA32_NOP);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0a0000, 0x0affff, 0, mirror, MWA32_NOP);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0b0000, 0x0b7fff, 0, mirror, MRA32_NOP);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0b0000, 0x0b7fff, 0, mirror, MWA32_NOP);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0b8000, 0x0bffff, 0, mirror, MRA32_RAM);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0b8000, 0x0bffff, 0, mirror, pc_video_videoram32_w);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0c0000, 0x0c7fff, 0, mirror, MRA32_ROM);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0c0000, 0x0c7fff, 0, mirror, MWA32_ROM);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0d0000, 0x0effff, 0, mirror, MRA32_ROM);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0d0000, 0x0effff, 0, mirror, MWA32_ROM);
	memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x0f0000, 0x0fffff, 0, mirror, MRA32_ROM);
	memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x0f0000, 0x0fffff, 0, mirror, MWA32_ROM);
	cpu_setbank(10, mess_ram);

	if (a20)
	{
		offs_t ram_limit = 0x100000 + mess_ram_size - 0x0a0000;
		memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1,	0, mirror, MRA32_BANK1);
		memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1,	0, mirror, MWA32_BANK1);
		memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, ram_limit, 0xffffff,		0, mirror, MRA32_NOP);
		memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, ram_limit, 0xffffff,		0, mirror, MWA32_NOP);
		memory_install_read32_handler_mirror(0,  ADDRESS_SPACE_PROGRAM, 0xff0000,  0xffffff,		0, mirror, MRA32_BANK2);
		memory_install_write32_handler_mirror(0, ADDRESS_SPACE_PROGRAM, 0xff0000,  0xffffff,		0, mirror, MWA32_ROM);
		cpu_setbank(1, mess_ram + 0xa0000);
		cpu_setbank(2, memory_region(REGION_CPU1) + 0x0f0000);
	}
}



static void init_at_common(AT8042_CONFIG *at8042)
{
	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_DMA8237_AT);
	pit8253_config(0, &at_pit8253_config);
	mc146818_init(MC146818_STANDARD);
	soundblaster_config(&soundblaster);
	at_8042_init(at8042);
}



DRIVER_INIT( atcga )
{
	AT8042_CONFIG at8042={
		AT8042_STANDARD, at386_set_gate_a20
	};
	init_at_common(&at8042);
}



DRIVER_INIT( at386 )
{
	AT8042_CONFIG at8042={
		AT8042_AT386, at386_set_gate_a20
	};
	init_at_common(&at8042);
}



DRIVER_INIT( at_vga )
{
	AT8042_CONFIG at8042={
		AT8042_STANDARD, at286_set_gate_a20
	};

	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	vga_init(input_port_0_r);
}



DRIVER_INIT( ps2m30286 )
{
	AT8042_CONFIG at8042={
		AT8042_PS2, at386_set_gate_a20
	};
	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	vga_init(input_port_0_r);
}



MACHINE_INIT( at )
{
	dma8237_reset();
}



MACHINE_INIT( at_vga )
{
	vga_reset();
	dma8237_reset();
}

void at_cga_frame_interrupt (void)
{
	at_keyboard_polling();
	at_8042_time();
}

void at_vga_frame_interrupt (void)
{
//	vga_timer();

	at_keyboard_polling();
	at_8042_time();
}
