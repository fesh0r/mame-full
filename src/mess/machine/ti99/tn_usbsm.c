/****************************************************************************

    Thierry Nouspikel's USB-SmartMedia card emulation

    This card features three USB ports (two host and one device) and a
    SmartMedia interface.  The original prototype was designed by Thierry
    Nouspikel, and its description was published in 2003; a small series of
    approximately 100 printed-circuit boards was ordered shortly afterwards
    by various TI users.

    The specs have been published in <http://www.nouspikel.com/ti99/usb.html>.

    The USB interface uses a Philips ISP1161A USB controller that supports
    USB 2.0 full-speed (12 Mbit/s) and low-speed (1.5 Mbit/s) I/O (high speed
    (480 Mbits/sec) is not supported, but it is hardly a problem since TI99 is
    too slow to take advantage from high speed).  The SmartMedia interface uses
    a few TTL buffers.  The card also includes an 8MByte StrataFlash FEEPROM
    and 1MByte of SRAM for DSR use.

    The card has a 8-bit->16-bit demultiplexer that can be set-up to assume
    either that the LSByte of a word is accessed first or that the MSByte of a word
    is accessed first.  The former is true with ti-99/4(a), the latter with the
    tms9995 CPU used by Geneve and ti-99/8.

    TODO:
    * Test SmartMedia support
    * Implement USB controller and assorted USB devices
    * Save DSR FEEPROM to disk

    Raphael Nabet, 2004.

    Michael Zapf
    September 2010: Rewritten as device
    February 2012: Rewritten as class

    FIXME: Completely untested and likely to be broken

*****************************************************************************/

#include "tn_usbsm.h"
#include "machine/strata.h"

#define BUFFER_TAG "ram"

enum
{
	IO_REGS_ENABLE = 0x02,
	INT_ENABLE = 0x04,
	SM_ENABLE = 0x08,
	FEEPROM_WRITE_ENABLE = 0x10
};

nouspikel_usb_smartmedia_device::nouspikel_usb_smartmedia_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: ti_expansion_card_device(mconfig, TI99_USBSM, "Nouspikel USB/Smartmedia card", tag, owner, clock)
{
	m_shortname = "ti99_usbsm";
}

/*
    demultiplexed read in USB-SmartMedia DSR space
*/
UINT16 nouspikel_usb_smartmedia_device::usbsm_mem_16_r(offs_t offset)
{
	UINT16 reply = 0;

	if (offset < 0x2800)
	{	/* 0x4000-0x4fff range */
		if ((m_cru_register & IO_REGS_ENABLE) && (offset >= 0x27f8))
		{	/* SmartMedia interface */
			if (offset == 0)
				reply = m_smartmedia->data_r() << 8;
		}
		else
		{	/* FEEPROM */
			if (!(m_cru_register & FEEPROM_WRITE_ENABLE))
				reply = strataflash_16_r(m_strata, offset);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((m_cru_register & IO_REGS_ENABLE) && (offset >= 0x2ff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			reply = m_ram[m_sram_page*0x800+(offset-0x2800)];
		}
	}
	return reply;
}

/*
    demultiplexed write in USB-SmartMedia DSR space
*/
void nouspikel_usb_smartmedia_device::usbsm_mem_16_w(offs_t offset, UINT16 data)
{
	if (offset < 0x2800)
	{	/* 0x4000-0x4fff range */
		if ((m_cru_register & IO_REGS_ENABLE) && (offset >= 0x27f8))
		{	/* SmartMedia interface */
			switch (offset & 3)
			{
			case 0:
				m_smartmedia->data_w(data >> 8);
				break;
			case 1:
				m_smartmedia->address_w(data >> 8);
				break;
			case 2:
				m_smartmedia->command_w(data >> 8);
				break;
			case 3:
				/* bogus, don't use(?) */
				break;
			}
		}
		else
		{	/* FEEPROM */
			if (m_cru_register & FEEPROM_WRITE_ENABLE)
				strataflash_16_w(m_strata, offset, data);
		}
	}
	else
	{	/* 0x5000-0x5fff range */
		if ((m_cru_register & IO_REGS_ENABLE) && (offset >= 0x2ff8))
		{	/* USB controller */
		}
		else
		{	/* SRAM */
			m_ram[m_sram_page*0x800+(offset-0x2800)] = data;
		}
	}
}

/*
    CRU read
*/
void nouspikel_usb_smartmedia_device::crureadz(offs_t offset, UINT8 *value)
{
	if ((offset & 0xff00)==m_cru_base)
	{
		UINT8 reply = 0;
		offset &= 3;

		if (offset == 0)
		{
			// bit
			// 0   >1x00   0: USB Host controller requests interrupt.
			// 1   >1x02   0: USB Device controller requests interrupt.
			// 2   >1x04   1: USB Host controller suspended.
			// 3   >1x06   1: USB Device controller suspended.
			// 4   >1x08   0: Strata FEEPROM is busy.
			//             1: Strata FEEPROM is ready.
			// 5   >1x0A   0: SmartMedia card is busy.
			//             1: SmartMedia card absent or ready.
			// 6   >1x0C   0: No SmartMedia card present.
			//             1: A card is in the connector.
			// 7   >1x0E   0: SmartMedia card is protected.
			//             1: Card absent or not protected.

			reply = 0x33;
			if (!m_smartmedia->is_present())
				reply |= 0xc0;
			else if (!m_smartmedia->is_protected())
				reply |= 0x80;
		}
		*value = reply;
	}
}

/*
    CRU write
*/
void nouspikel_usb_smartmedia_device::cruwrite(offs_t offset, UINT8 data)
{
	if ((offset & 0xff00)==m_cru_base)
	{
		int bit = (offset >> 1) & 0x1f;

		switch (bit)
		{
		case 0:
			m_selected = data;
			break;

		case 1:			/* enable I/O registers */
		case 2:			/* enable interrupts */
		case 3:			/* enable SmartMedia card */
		case 4:			/* enable FEEPROM writes (and disable reads) */
			if (data)
				m_cru_register |= 1 << bit;
			else
				m_cru_register &= ~ (1 << bit);
			break;
		case 5:			/* FEEPROM page */
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			if (data)
				m_feeprom_page |= 1 << (bit-5);
			else
				m_feeprom_page &= ~ (1 << (bit-5));
			break;

		case 16:		/* SRAM page */
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			if (data)
				m_sram_page |= 1 << (bit-16);
			else
				m_sram_page &= ~ (1 << (bit-16));
			break;
		}
	}
}

/*
    Memory read
    TODO: Check whether AMA/B/C is actually checked
*/
READ8Z_MEMBER(nouspikel_usb_smartmedia_device::readz)
{
	if (((offset & m_select_mask)==m_select_value) && m_selected)
	{
		if (m_tms9995_mode ? (!(offset & 1)) : (offset & 1))
		{	/* first read triggers 16-bit read cycle */
			m_input_latch = usbsm_mem_16_r((offset >> 1)&0xffff);
		}

		/* return latched input */
		*value = ((offset & 1) ? (m_input_latch) : (m_input_latch >> 8)) & 0xff;
	}
}

/*
    Memory write. The controller is 16 bit, so we need to demultiplex again.
*/
WRITE8_MEMBER(nouspikel_usb_smartmedia_device::write)
{
	if (((offset & m_select_mask)==m_select_value) && m_selected)
	{
		/* latch write */
		if (offset & 1)
			m_output_latch = (m_output_latch & 0xff00) | data;
		else
			m_output_latch = (m_output_latch & 0x00ff) | (data << 8);

		if ((m_tms9995_mode)? (offset & 1) : (!(offset & 1)))
		{	/* second write triggers 16-bit write cycle */
			usbsm_mem_16_w((offset >> 1)&0xffff, m_output_latch);
		}
	}
}

void nouspikel_usb_smartmedia_device::device_start()
{
	m_ram = (UINT16*)(*memregion(BUFFER_TAG));
	/* auto_alloc_array(device->machine(), UINT16, 0x100000/2); */
	m_smartmedia = subdevice<smartmedia_image_device>("smartmedia");
	m_strata = subdevice("strata");
}

void nouspikel_usb_smartmedia_device::device_reset()
{
	m_feeprom_page = 0;
	m_sram_page = 0;
	m_cru_register = 0;
	// m_tms9995_mode = (device->type()==TMS9995);

	if (m_genmod)
	{
		m_select_mask = 0x1fe000;
		m_select_value = 0x174000;
	}
	else
	{
		m_select_mask = 0x7e000;
		m_select_value = 0x74000;
	}
	m_selected = false;

	m_cru_base = ioport("CRUUSBSM")->read();
}

ROM_START( tn_usbsm )
	ROM_REGION16_BE(0x80000, BUFFER_TAG, 0)  /* RAM buffer 512 KiB */
	ROM_FILL(0x0000, 0x80000, 0x0000)
ROM_END

INPUT_PORTS_START( tn_usbsm )
	PORT_START( "CRUUSBSM" )
	PORT_DIPNAME( 0x1f00, 0x1600, "USB/Smartmedia CRU base" )
		PORT_DIPSETTING( 0x1000, "1000" )
		PORT_DIPSETTING( 0x1100, "1100" )
		PORT_DIPSETTING( 0x1200, "1200" )
		PORT_DIPSETTING( 0x1300, "1300" )
		PORT_DIPSETTING( 0x1400, "1400" )
		PORT_DIPSETTING( 0x1500, "1500" )
		PORT_DIPSETTING( 0x1600, "1600" )
		PORT_DIPSETTING( 0x1700, "1700" )
		PORT_DIPSETTING( 0x1800, "1800" )
		PORT_DIPSETTING( 0x1900, "1900" )
		PORT_DIPSETTING( 0x1a00, "1A00" )
		PORT_DIPSETTING( 0x1b00, "1B00" )
		PORT_DIPSETTING( 0x1c00, "1C00" )
		PORT_DIPSETTING( 0x1d00, "1D00" )
		PORT_DIPSETTING( 0x1e00, "1E00" )
		PORT_DIPSETTING( 0x1f00, "1F00" )
INPUT_PORTS_END

MACHINE_CONFIG_FRAGMENT( tn_usbsm )
	MCFG_SMARTMEDIA_ADD( "smartmedia" )
	MCFG_STRATAFLASH_ADD( "strata" )
MACHINE_CONFIG_END

machine_config_constructor nouspikel_usb_smartmedia_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( tn_usbsm );
}

const rom_entry *nouspikel_usb_smartmedia_device::device_rom_region() const
{
	return ROM_NAME( tn_usbsm );
}

ioport_constructor nouspikel_usb_smartmedia_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(tn_usbsm);
}

const device_type TI99_USBSM = &device_creator<nouspikel_usb_smartmedia_device>;

