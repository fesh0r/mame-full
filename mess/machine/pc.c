/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

***************************************************************************/
#include <assert.h>
#include "driver.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"

#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/dma8237.h"
#include "includes/uart8250.h"
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_aga.h"
#include "includes/pc_t1t.h"

#include "includes/pc_mouse.h"
#include "includes/pckeybrd.h"

#include "includes/pclpt.h"
#include "includes/centroni.h"

#include "devices/pc_hdc.h"
#include "includes/nec765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "includes/ibmpc.h"
#include "includes/pcshare.h"

#include "includes/pc.h"
#include "mscommon.h"

DRIVER_INIT( pccga )
{
	pc_cga_init();
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
	ppi8255_init(&pc_ppi8255_interface);
	pc_rtc_init();
}

DRIVER_INIT( bondwell )
{
	pc_cga_init();
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
	ppi8255_init(&pc_ppi8255_interface);
}

DRIVER_INIT( pcmda )
{
	pc_mda_init();
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
	ppi8255_init(&pc_ppi8255_interface);
}

DRIVER_INIT( europc ) 
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	UINT8 *rom = &memory_region(REGION_CPU1)[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
	  fix century rom bios bug !
	  if year <79 month (and not CENTURY) is loaded with 0x20
	*/
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);

	europc_rtc_init();
//	europc_rtc_set_time();
}

DRIVER_INIT( t1000hx )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
}

DRIVER_INIT( pc200 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb0000, 0xbffff, pc200_videoram_r );
	install_mem_write_handler(0, 0xb0000, 0xbffff, pc200_videoram_w );
	videoram_size=0x10000;
	videoram=memory_region(REGION_CPU1)+0xb0000;

	// 0x3dd, 0x3d8, 0x3d4, 0x3de are also in mda mode present!?
	install_port_read_handler(0, 0x3d0, 0x3df, pc200_cga_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc200_cga_w );

	install_port_read_handler(0, 0x3b0, 0x3bf, pc_MDA_r );
	install_port_write_handler(0, 0x3b0, 0x3bf, pc_MDA_w );

	install_port_read_handler(0, 0x278, 0x27b, pc200_port378_r );

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
}

DRIVER_INIT( pc1512 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb8000, 0xbbfff, MRA8_BANK1 );
	install_mem_write_handler(0, 0xb8000, 0xbbfff, pc1512_videoram_w );

	install_port_read_handler(0, 0x3d0, 0x3df, pc1512_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc1512_w );

	install_port_read_handler(0, 0x278, 0x27b, pc_parallelport2_r );


	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
	mc146818_init(MC146818_IGNORE_CENTURY);
}

DRIVER_INIT( pc1640 )
{
	vga_init(input_port_0_r);
	install_mem_read_handler(0, 0xa0000, 0xaffff, MRA8_BANK1 );
	install_mem_read_handler(0, 0xb0000, 0xb7fff, MRA8_BANK2 );
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA8_BANK3 );

	install_mem_write_handler(0, 0xa0000, 0xaffff, MWA8_BANK1 );
	install_mem_write_handler(0, 0xb0000, 0xb7fff, MWA8_BANK2 );
	install_mem_write_handler(0, 0xb8000, 0xbffff, MWA8_BANK3 );

	install_port_read_handler(0, 0x3b0, 0x3bf, vga_port_03b0_r );
	install_port_read_handler(0, 0x3c0, 0x3cf, paradise_ega_03c0_r );
	install_port_read_handler(0, 0x3d0, 0x3df, pc1640_port3d0_r );

	install_port_write_handler(0, 0x3b0, 0x3bf, vga_port_03b0_w );
	install_port_write_handler(0, 0x3c0, 0x3cf, vga_port_03c0_w );
	install_port_write_handler(0, 0x3d0, 0x3df, vga_port_03d0_w );

	install_port_read_handler(0, 0x278, 0x27b, pc1640_port278_r );
	install_port_read_handler(0, 0x4278, 0x427b, pc1640_port4278_r );

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);

	mc146818_init(MC146818_IGNORE_CENTURY);
}

DRIVER_INIT( pc_vga )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC);
	ppi8255_init(&pc_ppi8255_interface);

	vga_init(input_port_0_r);
}

MACHINE_INIT( pc_mda )
{
	dma8237_reset(dma8237);
}

MACHINE_INIT( pc_cga )
{
	dma8237_reset(dma8237);
}

MACHINE_INIT( pc_t1t )
{
	pc_t1t_reset();
	dma8237_reset(dma8237);
}

MACHINE_INIT( pc_aga )
{
	dma8237_reset(dma8237);
}

MACHINE_INIT( pc_vga )
{
	vga_reset();
	dma8237_reset(dma8237);
}

/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/
static void pc_generic_frame_interrupt(int has_turbo, void (*pc_timer)(void))
{
	if (has_turbo)
	{
		static int turboswitch = -1;
		if (turboswitch !=(input_port_3_r(0)&2) )
		{
			if (input_port_3_r(0)&2)
				cpunum_set_clockscale(0, 1);
			else
				cpunum_set_clockscale(0, 4.77/12);
			turboswitch=input_port_3_r(0)&2;
		}
	}

	if (pc_timer)
		pc_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();
}

void pc_mda_frame_interrupt (void)
{
	pc_generic_frame_interrupt(TRUE, pc_mda_timer);
}

void pc_cga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(TRUE, pc_cga_timer);
}

void tandy1000_frame_interrupt (void)
{
	pc_generic_frame_interrupt(TRUE, pc_t1t_timer);
}

void pc_aga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(FALSE, pc_aga_timer);
}

void pc_vga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(TRUE, NULL /* vga_timer */);
}

