/**********************************************************************

    Xebec S1410 5.25" Winchester Disk Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

Xebec S1410

PCB Layout
----------

ASSY 104527 REV E04 SN 127623

|-------------------------------------------|
|                                           |
|CN1                                        |
|                                           |
|                                           |
|CN2                                        |
|                   XEBEC2               CN5|
|   PROM                        2114        |
|CN3                XEBEC1      2114        |
|                                           |
|CN4                Z80         ROM         |
|   20MHz                             16MHz |
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    Z80     - Zilog Z8400APS Z80A CPU
    ROM     - 2732 pinout ROM "XEBEC 104521G 2155008 M460949"
    PROM    - National Semiconductor DM74S288N "103911" 32x8 TTL PROM
    2114    - National Semiconductor NMC2148HN-3 1Kx4 RAM
    XEBEC1  - Xebec 3198-0009
    XEBEC2  - Xebec 3198-0045 "T20"
    CN1     - 4-pin Molex, drive power
    CN2     - 34-pin PCB edge, ST-506 drive 0/1 control
    CN3     - 2x10 PCB header, ST-506 drive 0 data
    CN4     - 2x10 PCB header, ST-506 drive 1 data
    CN5     - 2x25 PCB header, SASI host interface


ASSY 104766 REV C02 SN 231985P

|-------------------------------------------|
|                                           |
| CN1                           SY2158      |
|                               CN7     CN6 |
|                               ROM         |
| CN2                                       |
|           XEBEC1          Z80             |
|                                       CN5 |
| CN3       XEBEC2   20MHz      XEBEC3      |
|                                           |
| CN4       XEBEC4              XEBEC5      |
|                                           |
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    Z80     - Zilog Z8400APS Z80A CPU
    ROM     - 2732 pinout ROM "104788D"
    SY2158  - Synertek SY2158A-3 1Kx8 RAM
    XEBEC1  - Xebec 3198-0046N8445
    XEBEC2  - Xebec 3198-0009
    XEBEC3  - Xebec 3198-0057
    XEBEC4  - Xebec 3198-0058
    XEBEC5  - Xebec 3198-0056
    CN1     - 4-pin Molex, drive power
    CN2     - 34-pin PCB edge, ST-506 drive 0/1 control
    CN3     - 2x10 PCB header, ST-506 drive 0 data
    CN4     - 2x10 PCB header, ST-506 drive 1 data
    CN5     - 2x25 PCB header, SASI host interface
    CN6     - 2x8 PCB header
    CN7     - 2x10 PCB header (test only)

*/


#include "emu.h"
#include "s1410.h"
#include "cpu/z80/z80.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define Z8400A_TAG			"z80"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type S1410 = &device_creator<s1410_device>;

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void s1410_device::device_config_complete()
{
	m_shortname = "s1410";
}


//-------------------------------------------------
//  ROM( s1410 )
//-------------------------------------------------

ROM_START( s1410 )
	ROM_REGION( 0x1000, Z8400A_TAG, 0 )
	ROM_LOAD( "104521f", 0x0000, 0x1000, CRC(305b8e76) SHA1(9efaa53ae86bc111bd263ad433e083f78a000cab) )
	ROM_LOAD( "104521g", 0x0000, 0x1000, CRC(24385115) SHA1(c389f6108cd5ed798a090acacce940ee43d77042) )
	ROM_LOAD( "104788d", 0x0000, 0x1000, CRC(2e385e2d) SHA1(7e2c349b2b6e95f2134f82cffc38d86b8a68390d) )

	ROM_REGION( 0x20, "103911", 0 )
	ROM_LOAD( "103911", 0x00, 0x20, NO_DUMP ) // DM74S288N
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *s1410_device::device_rom_region() const
{
	return ROM_NAME( s1410 );
}


//-------------------------------------------------
//  ADDRESS_MAP( s1410_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( s1410_mem, AS_PROGRAM, 8, s1410_device )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_REGION(Z8400A_TAG, 0)
	AM_RANGE(0x1000, 0x13ff) AM_RAM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( s1410_io )
//-------------------------------------------------

static ADDRESS_MAP_START( s1410_io, AS_IO, 8, s1410_device )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x66, 0x66) AM_WRITENOP
	AM_RANGE(0x67, 0x67) AM_WRITENOP
	AM_RANGE(0x68, 0x68) AM_READNOP
	AM_RANGE(0x69, 0x69) AM_WRITENOP
	AM_RANGE(0x6a, 0x6a) AM_WRITENOP
	AM_RANGE(0x6b, 0x6b) AM_WRITENOP
	AM_RANGE(0x6c, 0x6c) AM_WRITENOP
	AM_RANGE(0xa0, 0xa0) AM_NOP
	AM_RANGE(0xc1, 0xc1) AM_WRITENOP
	AM_RANGE(0xc2, 0xc2) AM_WRITENOP
	AM_RANGE(0xc3, 0xc3) AM_WRITENOP
ADDRESS_MAP_END

//-------------------------------------------------
//  MACHINE_DRIVER( s1410 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( s1410 )
	MCFG_CPU_ADD(Z8400A_TAG, Z80, XTAL_16MHz/4)
	MCFG_CPU_PROGRAM_MAP(s1410_mem)
	MCFG_CPU_IO_MAP(s1410_io)
	MCFG_DEVICE_DISABLE()
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor s1410_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( s1410 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s1410_device - constructor
//-------------------------------------------------

s1410_device::s1410_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, S1410, "Xebec S1410", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s1410_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void s1410_device::device_reset()
{
}
