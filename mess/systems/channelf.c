/******************************************************************
 *  Fairchild Channel F driver
 *
 *  Juergen Buchmueller & Frank Palazzolo
 *
 *  TBD:
 *      Verify timing on real unit
 *
 ******************************************************************/

#include "driver.h"
#include "image.h"
#include "vidhrdw/generic.h"
#include "includes/channelf.h"
#include "devices/cartslot.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/* The F8 has latches on its port pins
 * These mimic's their behavior
 * [0]=port0, [1]=port1, [2]=port4, [3]=port5
 *
 * Note: this should really be moved into f8.c,
 * but it's complicated by possible peripheral usage.
 *
 * If the read/write operation is going external to the F3850
 * (or F3853, etc.), then the latching applies.  However, relaying the
 * port read/writes from the F3850 to a peripheral like the F3853
 * should not be latched in this way. (See mk1 driver)
 *
 * The f8 cannot determine how its ports are mapped at runtime,
 * so it can't easily decide to latch or not.
 *
 * ...so it stays here for now.
 */

static UINT8 latch[4];

static UINT8 port_read_with_latch(UINT8 ext, UINT8 latch_state)
{
	return (~ext | latch_state);
}

static DEVICE_LOAD( channelf_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0x800, 0x800, 0x800, 0);
}

static READ_HANDLER( channelf_port_0_r )
{
	return port_read_with_latch(readinputport(0),latch[0]);
}

static READ_HANDLER( channelf_port_1_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = readinputport(1);
	}
	else
	{
		ext_value = 0xc0 | readinputport(1);
	}
	return port_read_with_latch(ext_value,latch[1]);
}

static READ_HANDLER( channelf_port_4_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = readinputport(2);
	}
	else
	{
		ext_value = 0xff;
	}
	return port_read_with_latch(ext_value,latch[2]);
}

static READ_HANDLER( channelf_port_5_r )
{
	return port_read_with_latch(0xff,latch[3]);
}

static WRITE_HANDLER( channelf_port_0_w )
{
	int offs;

	latch[0] = data;

    if (data & 0x20)
	{
		offs = channelf_row_reg*128+channelf_col_reg;
		if (videoram[offs] != channelf_val_reg)
			videoram[offs] = channelf_val_reg;
	}
}

static WRITE_HANDLER( channelf_port_1_w )
{
	latch[1] = data;
    channelf_val_reg = ((data ^ 0xff) >> 6) & 0x03;
}

static WRITE_HANDLER( channelf_port_4_w )
{
    latch[2] = data;
    channelf_col_reg = (data | 0x80) ^ 0xff;
}

static WRITE_HANDLER( channelf_port_5_w )
{
    latch[3] = data;
	channelf_sound_w((data>>6)&3);
    channelf_row_reg = (data | 0xc0) ^ 0xff;
}

static ADDRESS_MAP_START( channelf_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READWRITE(MRA8_ROM, MWA8_ROM)
	AM_RANGE(0x0800, 0x0fff) AM_READ(MRA8_ROM) /* Cartridge Data */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(channelf_port_0_r) /* Front panel switches */
	AM_RANGE(0x01, 0x01) AM_READ(channelf_port_1_r) /* Right controller     */
	AM_RANGE(0x04, 0x04) AM_READ(channelf_port_4_r) /* Left controller      */
	AM_RANGE(0x05, 0x05) AM_READ(channelf_port_5_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(channelf_port_0_w) /* Enable Controllers & ARM WRT */
	AM_RANGE(0x01, 0x01) AM_WRITE(channelf_port_1_w) /* Video Write Data */
	AM_RANGE(0x04, 0x04) AM_WRITE(channelf_port_4_w) /* Video Horiz */
	AM_RANGE(0x05, 0x05) AM_WRITE(channelf_port_5_w) /* Video Vert & Sound */
ADDRESS_MAP_END

INPUT_PORTS_START( channelf )
	PORT_START /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START )	/* START (1) */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 )	/* HOLD  (2) */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 )	/* MODE  (3) */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 )	/* TIME  (4) */
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER1 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER1 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER1 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER1 ) /* PUSH DOWN   */

	PORT_START /* Left controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER2 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER2 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER2 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER2 ) /* PUSH DOWN   */

INPUT_PORTS_END

static struct CustomSound_interface channelf_sound_interface = {
	channelf_sh_custom_start,
	channelf_sh_stop,
	channelf_sh_custom_update
};


static MACHINE_DRIVER_START( channelf )
	/* basic machine hardware */
	MDRV_CPU_ADD(F8, 3579545/2)        /* Colorburst/2 */
	MDRV_CPU_PROGRAM_MAP(channelf_map, 0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_VISIBLE_AREA(1, 112 - 1, 0, 64 - 1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, channelf_sound_interface)
MACHINE_DRIVER_END

ROM_START(channelf)
	ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("sl31253.rom",  0x0000, 0x0400, CRC(04694ed9))
		ROM_LOAD("sl31254.rom",  0x0400, 0x0400, CRC(9c047ba3))
ROM_END

SYSTEM_CONFIG_START(channelf)
	CONFIG_DEVICE_CARTSLOT_OPT( 1, "bin\0", NULL, NULL, device_load_channelf_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT		CONFIG		COMPANY		 FULLNAME */
CONS( 1976, channelf, 0,		0,		channelf, channelf, 0,			channelf,	"Fairchild", "Channel F" )


