// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/**************************************************************************************************

PowerVu D9234 STB (c) 1997 Scientific Atlanta

20-mar-2010 skeleton driver

References:
- http://www.vetrun.net/forums/showthread.php?t=395
- http://colibri.net63.net/powervu.htm
- http://www.growl.de/d9234/

TODO:
- everything, including PCB pictures and user manual;
- Probably shared with other PowerVu DVB-S STB models;

Front Panel:
- On/standby / Signal on left
- arrow keys with select in the middle, next to DVB logo

Back Panel:
- CH3 / CH4 dip;
- Ant In and Tv Out UHF connectors, near composite audio/video jacks
- Satellite LNB PWR +13/+19V 250mA
- LNB PWR ON / OFF dip;
- AC IN, 100V-240V, 50/60Hz

===================================================================================================

Meant for payTV providers to decrypt signals from the satellite and pump
them out on a local cable network. The powervu encryption is very secure.

**************************************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"
#include "emupal.h"
#include "screen.h"


namespace {

class pv9234_state : public driver_device
{
public:
	pv9234_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_p_ram(*this, "p_ram")
		, m_maincpu(*this, "maincpu")
	{
	}

	void pv9234(machine_config &config);

private:
	void debug_w(uint32_t data);
	void debug1_w(uint32_t data);
	void debug2_w(uint32_t data);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_map(address_map &map) ATTR_COLD;

	virtual void machine_reset() override ATTR_COLD;
	virtual void video_start() override ATTR_COLD;

	required_shared_ptr<uint32_t> m_p_ram;
	required_device<cpu_device> m_maincpu;
};

void pv9234_state::video_start()
{
}

uint32_t pv9234_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	return 0;
}


/*
This is the serial output to a terminal, used for debugging.
The boot process goes something like this:

Start

Config: 0x00001080 (Max Config: 00003C80)
MV 00000004.00000003
DL Avail
IOP Com. O.K. 00000004
Check CRC ...
CRC O.K.
Launch App
**************
* Ver 2.05 *
**************
Compiled by: FURLANO
Date & time: Nov 3 1997, 15:34:29
All printing enabled. Press space bar to toggle on/off.
Time stamping enabled. Press 't' to turn on/off.
Press 'o' to toggle printing of MPEG Xport error messages.
*/
void pv9234_state::debug_w(uint32_t data)
{
	if (data)
	{
		//printf("%02x %c\n",data,data); // this prints 'Start' to the console.
		logerror("debug=%02x %c\n",data,data);
	}
}

void pv9234_state::debug1_w(uint32_t data)
{
	uint8_t i,j;
	if (data)
	{
		for (i = 0; i < 4; i++)
		{
			j = (data & 0xff000000) >> 24;
			data <<= 8;
//          printf("%c",j); // this prints 'OFF' to the console.
			logerror("debug1=%02x %c\n",j,j);
		}
//      printf("\n");
	}
}

void pv9234_state::debug2_w(uint32_t data)
{
	if (data)
		logerror("debug2=%02x\n",data); // ignore the huge amount of zeroes here
}

void pv9234_state::main_map(address_map &map)
{
	map(0x00000000, 0x0007ffff).rom().region("maincpu", 0); //FLASH ROM!
	// map(0x00000000, 0x00000033).w(FUNC(pv9234_state::)); something
	// map(0x00000044, 0x00000047).w(FUNC(pv9234_state::));
	// map(0x00000060, 0x0000006b).w(FUNC(pv9234_state::));
	// map(0x00007000, 0x00007003).w(FUNC(pv9234_state::));
	// map(0x00008000, 0x00008003).w(FUNC(pv9234_state::));
	map(0x00008014, 0x00008017).w(FUNC(pv9234_state::debug1_w));
	// map(0x00008020, 0x00008027).w(FUNC(pv9234_state::));
	map(0x000080c0, 0x000080c3).w(FUNC(pv9234_state::debug2_w));
	map(0x000080cc, 0x000080cf).w(FUNC(pv9234_state::debug_w));
	// map(0x000080d0, 0x000080d3).w(FUNC(pv9234_state::));
	map(0x0003e000, 0x0003efff).ram().share("p_ram");
	map(0x00080000, 0x00087fff).mirror(0x78000).ram().share("share1");//mirror is a guess, writes a prg at 0xc0200 then it jumps at b0200 (!)
	map(0xe0000000, 0xe0007fff).mirror(0x0fff8000).ram().share("share1");
	map(0xffffff00, 0xffffffff).ram(); //i/o? stack ram?
}

/* Input ports */
static INPUT_PORTS_START( pv9234 )
INPUT_PORTS_END


void pv9234_state::machine_reset()
{
	int i;

	for(i=0;i<0x1000/4;i++)
		m_p_ram[i] = 0;
}

void pv9234_state::pv9234(machine_config &config)
{
	/* basic machine hardware */
	ARM7(config, m_maincpu, 4915000); // TODO: unknown type
	m_maincpu->set_addrmap(AS_PROGRAM, &pv9234_state::main_map);

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500)); /* not accurate */
	screen.set_size(640, 480);
	screen.set_visarea(0, 640-1, 0, 480-1);
	screen.set_screen_update(FUNC(pv9234_state::screen_update));
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME);
}

/* ROM definition */
ROM_START( pv9234 )
	ROM_REGION32_LE( 0x80000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "u19.bin", 0x00000, 0x20000, CRC(1e06b0c8) SHA1(f8047f7127919e73675375578bb9fcc0eed2178e))
	ROM_LOAD16_BYTE( "u18.bin", 0x00001, 0x20000, CRC(924487dd) SHA1(fb1d7c9a813ded8c820589fa85ae72265a0427c7))
	ROM_LOAD16_BYTE( "u17.bin", 0x40000, 0x20000, CRC(cac03650) SHA1(edd8aec6fed886d47de39ed4e127de0a93250a45))
	ROM_LOAD16_BYTE( "u16.bin", 0x40001, 0x20000, CRC(bd07d545) SHA1(90a63af4ee82b0f7d0ed5f0e09569377f22dd98c))
ROM_END

} // anonymous namespace


SYST( 1997, pv9234, 0,      0,      pv9234,  pv9234, pv9234_state, empty_init, "Scientific Atlanta", "PowerVu D9234 Business Satellite Receiver", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
