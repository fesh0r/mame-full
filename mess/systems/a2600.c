/***************************************************************************

  Atari VCS 2600 driver

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"


extern PALETTE_INIT( tia_NTSC );
extern PALETTE_INIT( tia_PAL );

extern VIDEO_START( tia );
extern VIDEO_UPDATE( tia );

extern READ_HANDLER( tia_r );
extern WRITE_HANDLER( tia_w );

extern void tia_init(void);

DEVICE_LOAD( a2600_cart );

static UINT8* r6532_ram;



DEVICE_LOAD( a2600_cart )
{
	UINT8* ROM = memory_region(REGION_CPU1);

	if (mame_fsize(file) != 0x800 && mame_fsize(file) != 0x1000)
	{
		return INIT_FAIL;
	}

	mame_fread(file, ROM + 0x1000, mame_fsize(file));

	if (mame_fsize(file) == 0x800)
	{
		memcpy(ROM + 0x1800, ROM + 0x1000, 0x800);
	}

	memcpy(ROM + 0x3000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0x5000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0x7000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0x9000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0xB000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0xD000, ROM + 0x1000, 0x1000);
	memcpy(ROM + 0xF000, ROM + 0x1000, 0x1000);

	return 0;
}


static READ_HANDLER( ram_r )
{
	return r6532_ram[offset];
}


static WRITE_HANDLER( ram_w )
{
	r6532_ram[offset] = data;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x007F, tia_r },
	{ 0x0080, 0x00FF, ram_r },
	{ 0x0100, 0x017F, tia_r },
	{ 0x0180, 0x01FF, ram_r },
	{ 0x0280, 0x029F, r6532_0_r },
	{ 0x1000, 0x1FFF, MRA_ROM },
	{ 0x3000, 0x3FFF, MRA_ROM },
	{ 0x5000, 0x5FFF, MRA_ROM },
	{ 0x7000, 0x7FFF, MRA_ROM },
	{ 0x9000, 0x9FFF, MRA_ROM },
	{ 0xB000, 0xBFFF, MRA_ROM },
	{ 0xD000, 0xDFFF, MRA_ROM },
	{ 0xF000, 0xFFFF, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x007F, tia_w },
	{ 0x0080, 0x00FF, ram_w, &r6532_ram },
	{ 0x0100, 0x017F, tia_w },
	{ 0x0180, 0x01FF, ram_w },
	{ 0x0280, 0x029F, r6532_0_w },
	{ 0x1000, 0x1FFF, MWA_ROM },
	{ 0x3000, 0x3FFF, MWA_ROM },
	{ 0x5000, 0x5FFF, MWA_ROM },
	{ 0x7000, 0x7FFF, MWA_ROM },
	{ 0x9000, 0x9FFF, MWA_ROM },
	{ 0xB000, 0xBFFF, MWA_ROM },
	{ 0xD000, 0xDFFF, MWA_ROM },
	{ 0xF000, 0xFFFF, MWA_ROM },
MEMORY_END


static READ_HANDLER( switch_A_r )
{
	UINT8 val = 0;

	int control0 = readinputport(9) % 16;
	int control1 = readinputport(9) / 16;

	val |= readinputport(6 + control0) & 0x0F;
	val |= readinputport(6 + control1) & 0xF0;

	return val;
}


static struct R6532interface r6532_interface =
{
	switch_A_r,
	input_port_8_r,
	NULL,
	NULL
};


static struct TIAinterface tia_interface =
{
	31400, 255, TIA_DEFAULT_GAIN
};


static MACHINE_INIT( a2600 )
{
	r6532_init(0, &r6532_interface);

	tia_init();
}


INPUT_PORTS_START( a2600 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER1 | IPF_REVERSE, 40, 10, 0, 255 )
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER2 | IPF_REVERSE, 40, 10, 0, 255 )
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER3 | IPF_REVERSE, 40, 10, 0, 255 )
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER4 | IPF_REVERSE, 40, 10, 0, 255 )

	PORT_START
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* SWCHA joystick */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START /* SWCHA paddles */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* SWCHB */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_RESETCPU, "Game Reset", KEYCODE_F3, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3, "Game Select", KEYCODE_S, IP_JOY_NONE )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "TV Type" )
	PORT_DIPSETTING(    0x08, "Color" )
	PORT_DIPSETTING(    0x00, "B&W" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Left Diff. Switch" )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Right Diff. Switch" )
	PORT_DIPSETTING(    0x80, "A" )
	PORT_DIPSETTING(    0x00, "B" )

	PORT_START
	PORT_DIPNAME( 0xF0, 0x00, "Left Controller" )
	PORT_DIPSETTING(    0x00, "Joystick" )
	PORT_DIPSETTING(    0x10, "Paddles" )
	PORT_DIPNAME( 0x0F, 0x00, "Right Controller" )
	PORT_DIPSETTING(    0x00, "Joystick" )
	PORT_DIPSETTING(    0x01, "Paddles" )

INPUT_PORTS_END


static MACHINE_DRIVER_START( a2600 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 3584160 / 3)	/* actually M6507 */
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(60)

	MDRV_MACHINE_INIT(a2600)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(160, 262)
	MDRV_VISIBLE_AREA(0, 159, 36, 261)
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(tia_NTSC)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SOUND_ADD(TIA, tia_interface)
MACHINE_DRIVER_END


ROM_START( a2600 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END


SYSTEM_CONFIG_START(a2600)
	CONFIG_DEVICE_CARTSLOT_REQ( 1, "bin\0a26\0", NULL, NULL, device_load_a2600_cart, NULL, NULL, NULL )
SYSTEM_CONFIG_END


/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	NULL,	a2600,	"Atari",	"Atari 2600 (NTSC)" )
