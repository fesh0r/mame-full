/***************************************************************************

	systems/bebox.c

	BeBox

***************************************************************************/

#include "includes/bebox.h"
#include "vidhrdw/pc_vga.h"
#include "cpu/powerpc/ppc.h"
#include "machine/uart8250.h"
#include "machine/pic8259.h"
#include "machine/mc146818.h"
#include "machine/pc_fdc.h"
#include "machine/pci.h"

static ADDRESS_MAP_START( bebox_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x7FFFF0F0, 0x7FFFF0F7) AM_READWRITE( bebox_cpu0_imask_r, bebox_cpu0_imask_w )
	AM_RANGE(0x7FFFF1F0, 0x7FFFF1F7) AM_READWRITE( bebox_cpu1_imask_r, bebox_cpu1_imask_w )
	AM_RANGE(0x7FFFF2F0, 0x7FFFF2F7) AM_READ( bebox_interrupt_sources_r )
	AM_RANGE(0x7FFFF3F0, 0x7FFFF3F7) AM_READWRITE( bebox_crossproc_interrupts_r, bebox_crossproc_interrupts_w )
	AM_RANGE(0x7FFFF4F0, 0x7FFFF4F7) AM_WRITE( bebox_processor_resets_w )

	AM_RANGE(0x80000020, 0x8000003F) AM_READWRITE( pic8259_64be_0_r, pic8259_64be_0_w )
	AM_RANGE(0x80000070, 0x8000007F) AM_READWRITE( mc146818_port64be_r, mc146818_port64be_w )
	AM_RANGE(0x800000A0, 0x800000BF) AM_READWRITE( pic8259_64be_1_r, pic8259_64be_1_w )
	AM_RANGE(0x800001F0, 0x800001F7) AM_READWRITE( bebox_800001F0_r, bebox_800001F0_w )
	AM_RANGE(0x800002F8, 0x800002FF) AM_READWRITE( uart8250_64be_1_r, uart8250_64be_1_w )
	AM_RANGE(0x80000380, 0x80000387) AM_READWRITE( uart8250_64be_2_r, uart8250_64be_2_w )
	AM_RANGE(0x80000388, 0x8000038F) AM_READWRITE( uart8250_64be_3_r, uart8250_64be_3_w )
	AM_RANGE(0x800003F0, 0x800003F7) AM_READWRITE( bebox_800003F0_r, bebox_800003F0_w )
	AM_RANGE(0x800003F8, 0x800003FF) AM_READWRITE( uart8250_64be_0_r, uart8250_64be_0_w )
	AM_RANGE(0x80000CF8, 0x80000CFF) AM_READWRITE( pci_64be_r, pci_64be_w )

	AM_RANGE(0xBFFFFFF0, 0xBFFFFFFF) AM_READ( bebox_interrupt_ack_r )

	AM_RANGE(0xFFF00000, 0xFFF03FFF) AM_ROMBANK(2)
	AM_RANGE(0xFFF04000, 0xFFFFFFFF) AM_ROMBANK(1)
ADDRESS_MAP_END



static ppc_config bebox_ppc_config =
{
	PPC_MODEL_603
};



static MACHINE_DRIVER_START( bebox )
	/* basic machine hardware */
	MDRV_CPU_ADD(PPC603, 66000000)        /* 66 Mhz */
	MDRV_CPU_CONFIG(bebox_ppc_config)
	MDRV_CPU_PROGRAM_MAP(bebox_mem, 0)

	MDRV_CPU_ADD(PPC603, 66000000)        /* 66 Mhz */
	MDRV_CPU_CONFIG(bebox_ppc_config)
	MDRV_CPU_PROGRAM_MAP(bebox_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_IMPORT_FROM( pcvideo_vga )

	MDRV_MACHINE_INIT( bebox )

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM3812, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static INPUT_PORTS_START( bebox )
INPUT_PORTS_END

ROM_START(bebox)
    ROM_REGION(0x20000, REGION_USER1, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootmain.rom", 0x000000, 0x20000, CRC(df2d19e0) SHA1(da86a7d23998dc953dd96a2ac5684faaa315c701) )
    ROM_REGION(0x4000, REGION_USER2, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootnub.rom", 0x000000, 0x4000, CRC(5348d09a) SHA1(1b637a3d7a2b072aa128dd5c037bbb440d525c1a) )
ROM_END

SYSTEM_CONFIG_START(bebox)
	CONFIG_RAM(8 * 1024 * 1024)
	CONFIG_RAM(16 * 1024 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024 * 1024)
SYSTEM_CONFIG_END


/*     YEAR     NAME        PARENT  COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY             FULLNAME */
COMPX( 1996,	bebox,		0,		0,		bebox,    bebox,	bebox,  bebox,	"Be Incorporated",	"BeBox", GAME_NOT_WORKING )
