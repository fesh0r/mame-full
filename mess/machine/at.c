#include "driver.h"

// support for old i86 core, until new one is in mame
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

void init_atcga(void)
{
	AT8042_CONFIG at8042={
		AT8042_STANDARD, i286_set_address_mask
	};

	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_DMA8237_AT);

	pit8253_config(0, &at_pit8253_config);

	pc_cga_init();
	mc146818_init(MC146818_STANDARD);

	soundblaster_config(&soundblaster);
	at_8042_init(&at8042);
}

#ifdef HAS_I386
void init_at386(void)
{
	AT8042_CONFIG at8042={
		AT8042_AT386, NULL /*i386_set_address_mask*/
	};
	init_atcga();
	at_8042_init(&at8042);
}
#endif

void init_at_vga(void)
{
	AT8042_CONFIG at8042={
		AT8042_STANDARD, i286_set_address_mask
	};

	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_DMA8237_AT);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);

	pit8253_config(0, &at_pit8253_config);

	mc146818_init(MC146818_STANDARD);

	vga_init(input_port_0_r);
	soundblaster_config(&soundblaster);
	at_8042_init(&at8042);
}

void init_ps2m30286(void)
{
	AT8042_CONFIG at8042={
		AT8042_PS2, i286_set_address_mask
	};
	init_at_vga();
	at_8042_init(&at8042);
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
	pc_cga_timer();

	at_keyboard_polling();
	at_8042_time();
}

void at_vga_frame_interrupt (void)
{
//	vga_timer();

	at_keyboard_polling();
	at_8042_time();
}
