/******************************************************************************
 Contributors:

	Marat Fayzullin (MG source)
	Charles Mac Donald
	Mathis Rosenhauer
	Brad Oliver
	Michael Luong

 To do:

 - PSG control for Game Gear (needs custom SN76489 with stereo output for each channel)
 - SIO interface for Game Gear (needs netplay, I guess)
 - TMS9928A support for 'f16ffight.sms'
 - SMS lightgun support
 - SMS Pause key - certainly there's an effective way to handle this
 - LCD persistence emulation for GG
 - SMS 3D glass support

 The Game Gear SIO and PSG hardware are not emulated but have some
 placeholders in 'machine/sms.c'

 Changes:
	Apr 02 - Added raster palette effects for SMS & GG (ML)
				 - Added sprite collision (ML)
				 - Added zoomed sprites (ML)
	May 02 - Fixed paging bug (ML)
				 - Fixed sprite and tile priority bug (ML)
				 - Fixed bug #66 (ML)
				 - Fixed bug #78 (ML)
				 - try to implement LCD persistence emulation for GG (ML)
	Jun 10, 02 - Added bios emulation (ML)
	Jun 12, 02 - Added PAL & NTSC systems (ML)
	Jun 25, 02 - Added border emulation (ML)
	Jun 27, 02 - Version bits for Game Gear (bits 6 of port 00) (ML)

 ******************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "sound/2413intf.h"
#include "vidhrdw/generic.h"
#include "includes/sms.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( sms_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03FF) AM_ROMBANK(1)					/* First 0x0400 part always points to first page */
	AM_RANGE(0x0400, 0x3FFF) AM_ROMBANK(2)					/* switchable rom bank */
	AM_RANGE(0x4000, 0x7FFF) AM_ROMBANK(3)					/* switchable rom bank */
	AM_RANGE(0x8000, 0xBFFF) AM_READWRITE(MRA8_BANK4, sms_cartram_w)	/* ROM bank / on-cart RAM */
	AM_RANGE(0xC000, 0xDFFB) AM_MIRROR(0x2000) AM_RAM			/* RAM (mirror at 0xE000) */
	AM_RANGE(0xDFFC, 0xDFFF) AM_RAM						/* RAM "underneath" frame registers */
	AM_RANGE(0xFFFC, 0xFFFF) AM_READWRITE(sms_mapper_r, sms_mapper_w)	/* Bankswitch control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sms_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x3e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x3e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_vdp_curline_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0) AM_MIRROR(0x1e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xC1, 0xC1) AM_MIRROR(0x1e) AM_READ(sms_version_r)
	AM_RANGE(0xE0, 0xE0) AM_MIRROR(0x0e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xE1, 0xE1) AM_MIRROR(0x0e) AM_READ(sms_version_r)
	AM_RANGE(0xF0, 0xF0)				 AM_READWRITE(sms_input_port_0_r, sms_YM2413_register_port_0_w)
	AM_RANGE(0xF1, 0xF1)				 AM_READWRITE(sms_version_r, sms_YM2413_data_port_0_w)
	AM_RANGE(0xF2, 0xF2)				 AM_READWRITE(sms_fm_detect_r, sms_fm_detect_w)
	AM_RANGE(0xF3, 0xF3)				 AM_READ(sms_version_r)
	AM_RANGE(0xF4, 0xF4) AM_MIRROR(0x02) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF5, 0xF5) AM_MIRROR(0x02) AM_READ(sms_version_r)
	AM_RANGE(0xF8, 0xF8) AM_MIRROR(0x06) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF9, 0xF9) AM_MIRROR(0x06) AM_READ(sms_version_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gg_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00)				 AM_READ(gg_input_port_2_r)
	AM_RANGE(0x01, 0x05)				 AM_READWRITE(gg_sio_r, gg_sio_w)
	AM_RANGE(0x06, 0x06)				 AM_READWRITE(gg_psg_r, gg_psg_w)
	AM_RANGE(0x07, 0x07) 				 AM_WRITE(sms_version_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x06) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x09, 0x09) AM_MIRROR(0x06) AM_WRITE(sms_version_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e) AM_WRITE(sms_version_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x1e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x21, 0x21) AM_MIRROR(0x1e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_vdp_curline_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0)				 AM_READ(input_port_0_r)
	AM_RANGE(0xC1, 0xC1)				 AM_READ(input_port_1_r)
	AM_RANGE(0xDC, 0xDC)				 AM_READ(input_port_0_r)
	AM_RANGE(0xDD, 0xDD)				 AM_READ(input_port_1_r)
ADDRESS_MAP_END


INPUT_PORTS_START( sms )

	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_8WAY

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* Software Reset bit */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START ) /* Game Gear START */

INPUT_PORTS_END



static MACHINE_DRIVER_START(sms)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(sms_io, 0)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA(29 - 1, NTSC_X_PIXELS - 29 - 1, 10 - 1, NTSC_Y_PIXELS - 9 - 1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)
	/*MDRV_PALETTE_INIT(sms)*/

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 4194304)	/* 4.194304 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smspal)
	MDRV_IMPORT_FROM(sms)
	MDRV_CPU_VBLANK_INT(sms, PAL_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(50)

	MDRV_SCREEN_SIZE(PAL_X_PIXELS, PAL_Y_PIXELS)
	MDRV_VISIBLE_AREA(29 - 1, PAL_X_PIXELS - 29 - 1, 10 - 1, PAL_Y_PIXELS - 9 - 1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsj21)
	MDRV_IMPORT_FROM(smspal)

	MDRV_SOUND_ADD(YM2413, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsm3)
	MDRV_IMPORT_FROM(smspal)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(gamegear)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(gg_io, 0)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA(6*8, 26*8-1, 3*8, 21*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 4194304)	/* 4.194304 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START(sms)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
ROM_END

ROM_START(smspal)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
ROM_END

ROM_START(smsj21)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13) SHA1(a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2))
ROM_END

ROM_START(smsm3)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13) SHA1(a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2))
ROM_END

#define rom_smsss rom_smsj21

ROM_START(smsu13)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7))
ROM_END

ROM_START(smse13)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7))
ROM_END

ROM_START(smsuam)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051))
ROM_END

ROM_START(smseam)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051))
ROM_END

ROM_START(smsesh)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROM_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B) SHA1(6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df))
ROM_END

ROM_START(smsbsh)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROM_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B) SHA1(6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df))
ROM_END

ROM_START(smsuhs24)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517))
ROM_END

ROM_START(smsehs24)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517))
ROM_END

ROM_START(smsuh34)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e))
ROM_END

ROM_START(smseh34)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e))
ROM_END

ROM_START(gamegear)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1,0)
ROM_END

#define rom_gamegj rom_gamegear

ROM_START(gamg)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1,0)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_LOAD("majbios.rom", 0x0000, 0x0400, CRC(0EBEA9D4))
ROM_END

#define rom_gamgj rom_gamg

static void sms_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "sms\0";
	dev->must_be_loaded = 1;
	dev->init = device_init_sms_cart;
	dev->load = device_load_sms_cart;
}

SYSTEM_CONFIG_START(sms)
	CONFIG_DEVICE(sms_cartslot_getinfo)
SYSTEM_CONFIG_END

static void smso_cartslot_getinfo(struct IODevice *dev)
{
	sms_cartslot_getinfo(dev);
	dev->must_be_loaded = 0;
}

SYSTEM_CONFIG_START(smso)
	CONFIG_DEVICE(smso_cartslot_getinfo)
SYSTEM_CONFIG_END

static void gamegear_cartslot_getinfo(struct IODevice *dev)
{
	sms_cartslot_getinfo(dev);
	dev->file_extensions = "gg\0";
}

SYSTEM_CONFIG_START(gamegear)
	CONFIG_DEVICE(gamegear_cartslot_getinfo)
SYSTEM_CONFIG_END

static void gamegearo_cartslot_getinfo(struct IODevice *dev)
{
	gamegear_cartslot_getinfo(dev);
	dev->must_be_loaded = 0;
}

SYSTEM_CONFIG_START(gamegearo)
	CONFIG_DEVICE(gamegearo_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*		YEAR	NAME		PARENT		COMPATIBLE	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
CONS(	1987,	sms,		0,			0,			sms,		sms,	0,		sms,		"Sega",			"Master System - (NTSC)" , 0)
CONS(	1986,	smsu13,		sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System - (NTSC) US/European BIOS v1.3" , 0)
CONS(	1986,	smse13,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System - (PAL) US/European BIOS v1.3" , 0)
//CONS(	1986,	smsumd3d,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Super Master System - (NTSC) US/European Missile Defense 3D BIOS" , 0)
//CONS(	1986,	smsemd3d,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Super Master System - (PAL) US/European Missile Defense 3D BIOS" , 0)
CONS(	1990,	smsuam,		sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System II - (NTSC) US/European BIOS with Alex Kidd in Miracle World" , 0)
CONS(	1990,	smseam,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System II - (PAL) US/European BIOS with Alex Kidd in Miracle World" , 0)
CONS(	1990,	smsesh,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System II - (PAL) European BIOS with Sonic The Hedgehog" , 0)
CONS(	1990,	smsbsh,		sms,		0,			smsm3,		sms,	0,		smso,		"Tec Toy",		"Master System III Compact (Brazil) - (PAL) European BIOS with Sonic The Hedgehog" , 0)
CONS(	1988,	smsuhs24,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System Plus - (NTSC) US/European BIOS v2.4 with Hang On and Safari Hunt" , 0)
CONS(	1988,	smsehs24,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System Plus - (PAL) US/European BIOS v2.4 with Hang On and Safari Hunt" , 0)
CONS(	1988,	smsuh34,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System - (NTSC) US/European BIOS v3.4 with Hang On" , 0)
CONS(	1988,	smseh34,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System - (PAL) US/European BIOS v3.4 with Hang On" , 0)
CONS(	1985,	smspal,		sms,		0,			smspal,		sms,	0,		sms,		"Sega",			"Master System - (PAL)" , 0)
CONS(	1985,	smsj21,		sms,		0,			smsj21,		sms,	0,		smso,		"Sega",			"Master System - (PAL) Japanese SMS BIOS v2.1" , 0)
CONS(	1984,	smsm3,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Mark III - (PAL) Japanese SMS BIOS v2.1" , 0)
CONS(	198?,	smsss,		sms,		0,			smsj21,		sms,	0,		smso,		"Samsung",		"Gamboy - (PAL) Japanese SMS BIOS v2.1" , 0)
CONS(	1990,	gamegear,	0,			sms,		gamegear,	sms,	0,		gamegear,	"Sega",			"Game Gear - European/American" , 0)
CONS(	1990,	gamegj,		gamegear,	0,			gamegear,	sms,	0,		gamegear,	"Sega",			"Game Gear - Japanese" , 0)
CONS(	1990,	gamg,		gamegear,	0,			gamegear,	sms,	0,		gamegearo,	"Majesco",		"Game Gear - European/American Majesco Game Gear BIOS" , 0)
CONS(	1990,	gamgj,		gamegear,	0,			gamegear,	sms,	0,		gamegearo,	"Majesco",		"Game Gear - Japanese Majesco Game Gear BIOS" , 0)

