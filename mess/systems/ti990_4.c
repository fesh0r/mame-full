/*
	ti990/4 driver

	We emulate a basic ti990/4 board, with a FD800 disk controller and an 733
	ASR terminal.  A little floppy-based software for this computer is
	available thanks to efforts by Dave Pitts: mostly, Forth and TX990.


	Board setup options:
	8kb of DRAM (onboard option, with optional parity): base 0x0000 or 0x2000
	4 banks of 512 bytes of ROM or SRAM: base 0x0000, 0x0800, 0xF000 or 0xF800
	power-up vector: 0x0000 (level 0) or 0xFFFC (load)
	optional memerr interrupt (level 2)
	optional power fail interrupt (level 1)
	optional real-time clock interrupt (level 5 or 7)


	Setup for the emulated system:
	0x0000: 8kb on-board DRAM + 24kb extension RAM (total 32kb)
	0xF800: 512 bytes SRAM
	0xFA00: 512 bytes SRAM (or empty?)
	0xFC00: 512 bytes self-test ROM
	0xFE00: 512 bytes loader ROM
	power-up vector: 0xFFFC (load)

	Note that only interrupt levels 3-7 are supported by the board (8-15 are not wired).

TODO:
* finish ASR emulation
* programmer panel
* emulate other devices: card reader, printer

*/

/*if 1, use 911 VDT; if 0, use 733 ASR  */
#define VIDEO_911 0

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/ti990.h"
#include "machine/990_dk.h"
#if VIDEO_911
#include "vidhrdw/911_vdt.h"
#else
#include "vidhrdw/733_asr.h"
#endif
#include "devices/basicdsk.h"

static void machine_init_ti990_4(void)
{
	ti990_hold_load();

	ti990_reset_int();

#if !VIDEO_911
	asr733_reset(0);
#endif

	fd800_machine_init(ti990_set_int7);
}


static void ti990_4_line_interrupt(void)
{
#if VIDEO_911
	vdt911_keyboard(0);
#else
	asr733_keyboard(0);
#endif

	ti990_line_interrupt();
}

/*static void idle_callback(int state)
{
}*/

static WRITE16_HANDLER ( rset_callback )
{
	ti990_cpuboard_reset();

#if VIDEO_911
	vdt911_reset();
#endif
	/* ... */

	/* clear controller panel and smi fault LEDs */
}

static WRITE16_HANDLER ( ckon_ckof_callback )
{
	ti990_ckon_ckof_callback((offset & 0x1000) ? 1 : 0);
}

static WRITE16_HANDLER ( lrex_callback )
{
	/* right??? */
	ti990_hold_load();
}

#if VIDEO_911

/*
	TI990/4 video emulation.

	We emulate a single VDT911 CRT terminal.
*/

static int video_start_ti990_4(void)
{
	const vdt911_init_params_t params =
	{
		char_1920,
		vdt911_model_US,
		ti990_set_int3
	};

	return vdt911_init_term(0, & params);
}

static VIDEO_UPDATE( ti990_4 )
{
	vdt911_refresh(bitmap, 0, 0, 0);
}

#else

static VIDEO_START( ti990_4 )
{
	return asr733_init_term(0, ti990_set_int6);
}

static VIDEO_UPDATE( ti990_4 )
{
	asr733_refresh(bitmap, 0, 0, 0);
}

#endif

/*
	Memory map - see description above
*/

static MEMORY_READ16_START (ti990_4_readmem)

	{ 0x0000, 0x7fff, MRA16_RAM },		/* dynamic RAM */
	{ 0x8000, 0xf7ff, MRA16_NOP },		/* reserved for expansion */
	{ 0xf800, 0xfbff, MRA16_RAM },		/* static RAM ? */
	{ 0xfc00, 0xffff, MRA16_ROM },		/* LOAD ROM */

MEMORY_END

static MEMORY_WRITE16_START (ti990_4_writemem)

	{ 0x0000, 0x7fff, MWA16_RAM },		/* dynamic RAM */
	{ 0x8000, 0xf7ff, MWA16_NOP },		/* reserved for expansion */
	{ 0xf800, 0xfbff, MWA16_RAM },		/* static RAM ? */
	{ 0xfc00, 0xffff, MWA16_ROM },		/* LOAD ROM */

MEMORY_END


/*
	CRU map

	0x000-0xF7F: user devices
	0xF80-0xF9F: CRU interrupt + expansion control
	0xFA0-0xFAF: TILINE coupler interrupt control
	0xFB0-0xFCF: reserved
	0xFD0-0xFDF: memory mapping and memory protect
	0xFE0-0xFEF: internal interrupt control
	0xFF0-0xFFF: front panel

	Default user map:
	0x000-0x00f: 733 ASR (int 6)
	0x010-0x01f: PROM programmer (wired to int 15, unused)
	0x020-0x02f: 804 card reader (int 4)
	0x030-0x03f: line printer (wired to int 14, unused)
	0x040-0x05f: FD800 floppy controller (int 7)
	0x060-0x07f: VDT1 (int 3 - wired to int 11, unused)
	0x080-0x09f: VDT2, or CRU expansion (int ??? - wired to int 10, unused)
	0x0a0-0x0bf: VDT3 (int ??? - wired to int 9, unused)
*/

static PORT_WRITE16_START ( ti990_4_writeport )

#if VIDEO_911
	{ 0x80 << 1, (0x8f << 1)+1, vdt911_0_cru_w },
#else
	{ 0x00 << 1, (0x0f << 1)+1, asr733_0_cru_w },
#endif

	{ 0x40 << 1, (0x5f << 1)+1, fd800_cru_w },

	{ 0xff0 << 1, (0xfff << 1)+1, ti990_panel_write },

	/* external instruction decoding */
	/*{ 0x2000 << 1, (0x2fff << 1)+1, idle_callback },*/
	{ 0x3000 << 1, (0x3fff << 1)+1, rset_callback },
	{ 0x5000 << 1, (0x6fff << 1)+1, ckon_ckof_callback },
	{ 0x7000 << 1, (0x7fff << 1)+1, lrex_callback },

PORT_END

static PORT_READ16_START ( ti990_4_readport )

#if VIDEO_911
	{ 0x10 << 1, (0x11 << 1)+1, vdt911_0_cru_r },
#else
	{ 0x00 << 1, (0x01 << 1)+1, asr733_0_cru_r },
#endif

	{ 0x08 << 1, (0x0b << 1)+1, fd800_cru_r },

	{ 0x1fe << 1, (0x1ff << 1)+1, ti990_panel_read },

PORT_END

#if VIDEO_911
static struct beep_interface vdt_911_beep_interface =
{
	1,
	{ 50 }
};
#endif

/*static tms9900reset_param reset_params =
{
	idle_callback
};*/

static MACHINE_DRIVER_START(ti990_4)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0(???) MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(reset_params)*/
	MDRV_CPU_MEMORY(ti990_4_readmem, ti990_4_writemem)
	MDRV_CPU_PORTS(ti990_4_readport, ti990_4_writeport)
	/*MDRV_CPU_VBLANK_INT(NULL, 0)*/
	MDRV_CPU_PERIODIC_INT(ti990_4_line_interrupt, 120/*or 100 in Europe*/)

	/* video hardware - we emulate a single 911 vdt display */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti990_4 )
	/*MDRV_MACHINE_STOP( ti990_4 )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
#if VIDEO_911
	MDRV_SCREEN_SIZE(560, 280)
	MDRV_VISIBLE_AREA(0, 560-1, 0, /*250*/280-1)
#else
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 480-1)
#endif

#if VIDEO_911
	MDRV_GFXDECODE(vdt911_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(vdt911_palette_size)
	MDRV_COLORTABLE_LENGTH(vdt911_colortable_size)
#else
	MDRV_GFXDECODE(asr733_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(asr733_palette_size)
	MDRV_COLORTABLE_LENGTH(asr733_colortable_size)
#endif

#if VIDEO_911
	MDRV_PALETTE_INIT(vdt911)
#else
	MDRV_PALETTE_INIT(asr733)
#endif
	MDRV_VIDEO_START(ti990_4)
	/*MDRV_VIDEO_STOP(ti990_4)*/
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(ti990_4)

	MDRV_SOUND_ATTRIBUTES(0)
#if VIDEO_911
	/* 911 VDT has a beep tone generator */
	MDRV_SOUND_ADD(BEEP, vdt_911_beep_interface)
#endif

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(ti990_4)
	/*CPU memory space*/
	ROM_REGION16_BE(0x10000, REGION_CPU1,0)

#if 0
	/* ROM set 945121-5: "733 ASR ROM loader with self test (prototyping)"
	(cf 945401-9701 pp. 1-19) */

	/* test ROM */
	ROMX_LOAD("94519209.u39", 0xFC00, 0x100, CRC(0a0b0c42), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519210.u55", 0xFC00, 0x100, CRC(d078af61), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD("94519211.u61", 0xFC01, 0x100, CRC(6cf7d4a0), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519212.u78", 0xFC01, 0x100, CRC(d9522458), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))

	/* LOAD ROM */
	ROMX_LOAD("94519113.u3", 0xFE00, 0x100, CRC(8719b04e), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519114.u4", 0xFE00, 0x100, CRC(72a040e0), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD("94519115.u6", 0xFE01, 0x100, CRC(9ccf8cca), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519116.u7", 0xFE01, 0x100, CRC(fa387bf3), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))

#else
	/* ROM set 945121-4(?): "Floppy disc loader with self test" (cf 945401-9701
	pp. 1-19) */

	ROM_LOAD16_WORD("ti9904.rom", 0xFC00, 0x400, CRC(691e7d19))

#endif

#if VIDEO_911
	/* VDT911 character definitions */
	ROM_REGION(vdt911_chr_region_len, vdt911_chr_region, 0)
#else
	ROM_REGION(asr733_chr_region_len, asr733_chr_region, 0)
#endif

ROM_END

static void init_ti990_4(void)
{
#if VIDEO_911
	vdt911_init();
#else
	asr733_init();
#endif
}

INPUT_PORTS_START(ti990_4)
#if VIDEO_911
	VDT911_KEY_PORTS
#else
	ASR733_KEY_PORTS
#endif
INPUT_PORTS_END

SYSTEM_CONFIG_START(ti990_4)
	/*CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",			device_load_fd800)*/
	CONFIG_DEVICE_BASE(IO_FLOPPY, 4, "dsk\0", DEVICE_LOAD_RESETS_NONE,
		OSD_FOPEN_RW_CREATE_OR_READ, /*device_init_basicdsk_floppy*/device_init_fd800, NULL, device_load_fd800, device_unload_basicdsk_floppy, NULL, NULL,
		NULL, NULL, floppy_status, NULL, NULL, NULL, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1976,	ti990_4,	0,		0,		ti990_4,	ti990_4,	ti990_4,	ti990_4,	"Texas Instruments",	"TI Model 990/4 Microcomputer System" )
