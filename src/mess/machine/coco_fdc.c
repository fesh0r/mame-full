/*********************************************************************

    coco_fdc.c

    CoCo/Dragon FDC

    The CoCo and Dragon both use the Western Digital floppy disk controllers.
    The CoCo uses either the WD1793 or the WD1773, the Dragon uses the WD2797,
    which mostly uses the same command set with some subtle differences, most
    notably the 2797 handles disk side select internally. The Dragon Alpha also
    uses the WD2797, however as this is a built in interface and not an external
    cartrige, it is dealt with in the main coco.c file.

    The wd's variables are mapped to $FF48-$FF4B on the CoCo and on $FF40-$FF43
    on the Dragon.  In addition, there is another register
    called DSKREG that controls the interface with the wd1793.  DSKREG is
    detailed below:  But they appear to be

    References:
        CoCo:   Disk Basic Unravelled
        Dragon: Inferences from the PC-Dragon source code
                DragonDos Controller, Disk and File Formats by Graham E Kinns

    ---------------------------------------------------------------------------

    DSKREG - the control register
    CoCo ($FF40)                                    Dragon ($FF48)

    Bit                                              Bit
    7 halt enable flag                               7 not used
    6 drive select #3                                6 not used
    5 density (0=single, 1=double)                   5 NMI enable flag
        and NMI enable flag
    4 write precompensation                          4 write precompensation
    3 drive motor activation                         3 single density enable
    2 drive select #2                                2 drive motor activation
    1 drive select #1                                1 drive select high bit
    0 drive select #0                                0 drive select low bit

    Reading from $FF48-$FF4F clears bit 7 of DSKREG ($FF40)

    ---------------------------------------------------------------------------

    2007-02-22, P.Harvey-Smith

    Began implementing the Dragon Delta Dos controler, this was actually the first
    Dragon disk controler to market, beating Dragon Data's by a couple of months,
    it is based around the WD2791 FDC, which is compatible with the WD1793/WD2797 used
    by the standard CoCo and Dragon disk controlers except that it used an inverted
    data bus, which is the reason the read/write handlers invert the data. This
    controler like, the DragonDos WD2797 is mapped at $FF40-$FF43, in the normal
    register order.

    The Delta cart also has a register (74LS174 hex flipflop) at $FF44 encoded as
    follows :-

    Bit
    7 not used
    6 not used
    5 not used
    4 Single (0) / Double (1) density select
    3 5.25"(0) / 8"(1) Clock select
    2 Side select
    1 Drive select ms bit
    0 Drive select ls bit

*********************************************************************/

#include "emu.h"
#include "coco_fdc.h"
#include "imagedev/flopdrv.h"
#include "includes/coco.h"
#include "machine/wd17xx.h"
#include "machine/ds1315.h"
#include "imagedev/flopdrv.h"
#include "formats/coco_dsk.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_FDC					0
#define WD_TAG					"wd17xx"
#define DISTO_TAG				"disto"
#define CLOUD9_TAG				"cloud9"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static const floppy_interface coco_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(coco),
	NULL,
	NULL
};


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/
/*-------------------------------------------------
    real_time_clock
-------------------------------------------------*/

coco_rtc_type_t coco_fdc_device::real_time_clock()
{
	coco_rtc_type_t result = (coco_rtc_type_t) machine().root_device().ioport("real_time_clock")->read_safe(RTC_NONE);

	/* check to make sure we don't have any invalid values */
	if (((result == RTC_DISTO) && (m_disto_msm6242 == NULL))
		|| ((result == RTC_CLOUD9) && (m_ds1315 == NULL)))
	{
		result = RTC_NONE;
	}

	return result;
}
/*-------------------------------------------------
    fdc_intrq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( fdc_intrq_w )
{
	coco_fdc_device *fdc = dynamic_cast<coco_fdc_device *>(device->owner());
	fdc->set_intrq(state);
	fdc->update_lines();
}


/*-------------------------------------------------
    fdc_drq_w - callback from the FDC
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( fdc_drq_w )
{
	coco_fdc_device *fdc = dynamic_cast<coco_fdc_device *>(device->owner());
	fdc->set_drq(state);
	fdc->update_lines();
}


//**************************************************************************
//              COCO FDC
//**************************************************************************

static const wd17xx_interface coco_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(fdc_intrq_w),
	DEVCB_LINE(fdc_drq_w),
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};

static MSM6242_INTERFACE( coco_fdc_rtc_intf )
{
	DEVCB_NULL
};

static MACHINE_CONFIG_FRAGMENT(coco_fdc)
	MCFG_WD1773_ADD(WD_TAG, coco_wd17xx_interface)
	MCFG_MSM6242_ADD(DISTO_TAG, coco_fdc_rtc_intf)
	MCFG_DS1315_ADD(CLOUD9_TAG)

	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(coco_floppy_interface)
MACHINE_CONFIG_END

ROM_START( coco_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(	"disk10.rom",	0x0000, 0x2000, CRC(b4f9968e) SHA1(04115be3f97952b9d9310b52f806d04f80b40d03))
ROM_END

const device_type COCO_FDC = &device_creator<coco_fdc_device>;

//-------------------------------------------------
//  coco_fdc_device - constructor
//-------------------------------------------------
coco_fdc_device::coco_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, type, name, tag, owner, clock),
	  device_cococart_interface( mconfig, *this )
{
}

coco_fdc_device::coco_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, COCO_FDC, "CoCo FDC", tag, owner, clock),
		device_cococart_interface( mconfig, *this )
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void coco_fdc_device::device_start()
{
	m_owner = dynamic_cast<cococart_slot_device *>(owner());
	m_drq           	= 1;
	m_disto_msm6242		= subdevice<msm6242_device>(DISTO_TAG);
	m_ds1315			= subdevice(CLOUD9_TAG);
	m_wd17xx			= subdevice(WD_TAG);
	m_dskreg			= 0x00;
	m_intrq				= 0;
	m_msm6242_rtc_address = 0;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void coco_fdc_device::device_config_complete()
{
	m_shortname = "coco_fdc";
}
//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor coco_fdc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( coco_fdc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *coco_fdc_device::device_rom_region() const
{
	return ROM_NAME( coco_fdc );
}

/*-------------------------------------------------
    get_cart_base
-------------------------------------------------*/

UINT8* coco_fdc_device::get_cart_base()
{
	return memregion("eprom")->base();
}

/*-------------------------------------------------
    update_lines - CoCo specific disk
    controller lines
-------------------------------------------------*/

void coco_fdc_device::update_lines()
{
	/* clear HALT enable under certain circumstances */
	if ((m_intrq != 0) && (m_dskreg & 0x20))
		m_dskreg &= ~0x80;	/* clear halt enable */

	/* set the NMI line */
	m_owner->cart_set_line(COCOCART_LINE_NMI,
		((m_intrq != 0) && (m_dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the HALT line */
	m_owner->cart_set_line(COCOCART_LINE_HALT,
		((m_drq == 0) && (m_dskreg & 0x80)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}

/*-------------------------------------------------
    dskreg_w - function to write to CoCo
    dskreg
-------------------------------------------------*/

void coco_fdc_device::dskreg_w(UINT8 data)
{
	UINT8 drive = 0;
	UINT8 head = 0;

	if (LOG_FDC)
	{
		logerror("fdc_coco_dskreg_w(): %c%c%c%c%c%c%c%c ($%02x)\n",
			data & 0x80 ? 'H' : 'h',
			data & 0x40 ? '3' : '.',
			data & 0x20 ? 'D' : 'S',
			data & 0x10 ? 'P' : 'p',
			data & 0x08 ? 'M' : 'm',
			data & 0x04 ? '2' : '.',
			data & 0x02 ? '1' : '.',
			data & 0x01 ? '0' : '.',
			data);
	}

	/* An email from John Kowalski informed me that if the DS3 is
     * high, and one of the other drive bits is selected (DS0-DS2), then the
     * second side of DS0, DS1, or DS2 is selected.  If multiple bits are
     * selected in other situations, then both drives are selected, and any
     * read signals get yucky.
     */

	if (data & 0x04)
		drive = 2;
	else if (data & 0x02)
		drive = 1;
	else if (data & 0x01)
		drive = 0;
	else if (data & 0x40)
		drive = 3;

	device_t *floppy[4];

	floppy[0] = subdevice(FLOPPY_0);
	floppy[1] = subdevice(FLOPPY_1);
	floppy[2] = subdevice(FLOPPY_2);
	floppy[3] = subdevice(FLOPPY_3);

	for (int i = 0; i < 4; i++)
	{
		floppy_mon_w(floppy[i], i == drive ? CLEAR_LINE : ASSERT_LINE);
	}

	head = ((data & 0x40) && (drive != 3)) ? 1 : 0;

	m_dskreg = data;

	update_lines();

	wd17xx_set_drive(m_wd17xx, drive);
	wd17xx_set_side(m_wd17xx, head);
	wd17xx_dden_w(m_wd17xx, !BIT(m_dskreg, 5));
}

/*-------------------------------------------------
    read
-------------------------------------------------*/

READ8_MEMBER(coco_fdc_device::read)
{
	UINT8 result = 0;

	switch(offset & 0xEF)
	{
		case 8:
			result = wd17xx_status_r(m_wd17xx, 0);
			break;
		case 9:
			result = wd17xx_track_r(m_wd17xx, 0);
			break;
		case 10:
			result = wd17xx_sector_r(m_wd17xx, 0);
			break;
		case 11:
			result = wd17xx_data_r(m_wd17xx, 0);
			break;
	}

	/* other stuff for RTCs */
	switch(offset)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock() == RTC_DISTO)
				result = m_disto_msm6242->read(space,m_msm6242_rtc_address);
			break;

		case 0x38:	/* FF78 */
			if (real_time_clock() == RTC_CLOUD9)
				ds1315_r_0(m_ds1315, offset);
			break;

		case 0x39:	/* FF79 */
			if (real_time_clock() == RTC_CLOUD9)
				ds1315_r_1(m_ds1315, offset);
			break;

		case 0x3C:	/* FF7C */
			if (real_time_clock() == RTC_CLOUD9)
				result = ds1315_r_data(m_ds1315, offset);
			break;
	}
	return result;
}



/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(coco_fdc_device::write)
{
	switch(offset & 0x1F)
	{
		case 0: case 1: case 2: case 3:
		case 4: case 5: case 6: case 7:
			dskreg_w(data);
			break;
		case 8:
			wd17xx_command_w(m_wd17xx, 0, data);
			break;
		case 9:
			wd17xx_track_w(m_wd17xx, 0, data);
			break;
		case 10:
			wd17xx_sector_w(m_wd17xx, 0, data);
			break;
		case 11:
			wd17xx_data_w(m_wd17xx, 0, data);
			break;
	};

	/* other stuff for RTCs */
	switch(offset)
	{
		case 0x10:	/* FF50 */
			if (real_time_clock() == RTC_DISTO)
				m_disto_msm6242->write(space,m_msm6242_rtc_address, data);
			break;

		case 0x11:	/* FF51 */
			if (real_time_clock() == RTC_DISTO)
				m_msm6242_rtc_address = data & 0x0f;
			break;
	}
}


//**************************************************************************
//              DRAGON FDC
//**************************************************************************

static MACHINE_CONFIG_FRAGMENT(dragon_fdc)
	MCFG_WD2797_ADD(WD_TAG, coco_wd17xx_interface)
	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(coco_floppy_interface)
MACHINE_CONFIG_END


ROM_START( dragon_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(  "ddos10.rom",   0x0000,  0x2000, CRC(b44536f6) SHA1(a8918c71d319237c1e3155bb38620acb114a80bc))
ROM_END

const device_type DRAGON_FDC = &device_creator<dragon_fdc_device>;

//-------------------------------------------------
//  dragon_fdc_device - constructor
//-------------------------------------------------
dragon_fdc_device::dragon_fdc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
	: coco_fdc_device(mconfig, type, name, tag, owner, clock)
{
}
dragon_fdc_device::dragon_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, DRAGON_FDC, "Dragon FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dragon_fdc_device::device_start()
{
	m_owner = dynamic_cast<cococart_slot_device *>(owner());
	m_drq           	= 0;
	m_wd17xx			= subdevice(WD_TAG);
	m_dskreg			= 0x00;
	m_intrq				= 0;
	m_msm6242_rtc_address = 0;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void dragon_fdc_device::device_config_complete()
{
	m_shortname = "dragon_fdc";
}
//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor dragon_fdc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( dragon_fdc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *dragon_fdc_device::device_rom_region() const
{
	return ROM_NAME( dragon_fdc );
}



/*-------------------------------------------------
    update_lines - Dragon specific disk
    controller lines
-------------------------------------------------*/

void dragon_fdc_device::update_lines()
{
	/* set the NMI line */
	m_owner->cart_set_line(COCOCART_LINE_NMI,
		((m_intrq != 0) && (m_dskreg & 0x20)) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);

	/* set the CART line */
	m_owner->cart_set_line(COCOCART_LINE_CART,
		(m_drq != 0) ? COCOCART_LINE_VALUE_ASSERT : COCOCART_LINE_VALUE_CLEAR);
}


/*-------------------------------------------------
    dskreg_w - function to write to
    Dragon dskreg
-------------------------------------------------*/

void dragon_fdc_device::dskreg_w(UINT8 data)
{
	if (LOG_FDC)
	{
		logerror("fdc_dragon_dskreg_w(): %c%c%c%c%c%c%c%c ($%02x)\n",
			data & 0x80 ? 'X' : 'x',
			data & 0x40 ? 'X' : 'x',
			data & 0x20 ? 'N' : 'n',
			data & 0x10 ? 'P' : 'p',
			data & 0x08 ? 'S' : 'D',
			data & 0x04 ? 'M' : 'm',
			data & 0x02 ? '1' : '0',
			data & 0x01 ? '1' : '0',
			data);
	}

	if (data & 0x04)
		wd17xx_set_drive(m_wd17xx, data & 0x03);

	wd17xx_dden_w(m_wd17xx, BIT(data, 3));
	m_dskreg = data;
}



/*-------------------------------------------------
    read
-------------------------------------------------*/

READ8_MEMBER(dragon_fdc_device::read)
{
	UINT8 result = 0;
	switch(offset & 0xEF)
	{
		case 0:
			result = wd17xx_status_r(m_wd17xx, 0);
			break;
		case 1:
			result = wd17xx_track_r(m_wd17xx, 0);
			break;
		case 2:
			result = wd17xx_sector_r(m_wd17xx, 0);
			break;
		case 3:
			result = wd17xx_data_r(m_wd17xx, 0);
			break;
	}
	return result;
}



/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(dragon_fdc_device::write)
{
	switch(offset & 0xEF)
	{
		case 0:
			wd17xx_command_w(m_wd17xx, 0, data);

			/* disk head is encoded in the command byte */
			/* Only for type 3 & 4 commands */
			if (data & 0x80)
				wd17xx_set_side(m_wd17xx, (data & 0x02) ? 1 : 0);
			break;
		case 1:
			wd17xx_track_w(m_wd17xx, 0, data);
			break;
		case 2:
			wd17xx_sector_w(m_wd17xx, 0, data);
			break;
		case 3:
			wd17xx_data_w(m_wd17xx, 0, data);
			break;
		case 8: case 9: case 10: case 11:
		case 12: case 13: case 14: case 15:
			dskreg_w(data);
			break;
	};
}

//**************************************************************************
//              SDTANDY FDC
//**************************************************************************

ROM_START( sdtandy_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(  "sdtandy.rom",   0x0000,  0x2000, CRC(5d7779b7) SHA1(ca03942118f2deab2f6c8a89b8a4f41f2d0b94f1))
ROM_END

const device_type SDTANDY_FDC = &device_creator<sdtandy_fdc_device>;

//-------------------------------------------------
//  sdtandy_fdc_device - constructor
//-------------------------------------------------

sdtandy_fdc_device::sdtandy_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : dragon_fdc_device(mconfig, SDTANDY_FDC, "SDTANDY FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void sdtandy_fdc_device::device_config_complete()
{
	m_shortname = "sdtandy_fdc";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *sdtandy_fdc_device::device_rom_region() const
{
	return ROM_NAME( sdtandy_fdc );
}

//**************************************************************************
//              COCO FDC v1.1
//**************************************************************************

ROM_START( coco_fdc_v11 )
	ROM_REGION(0x8000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD_OPTIONAL(	"disk11.rom",	0x0000, 0x2000, CRC(0b9c5415) SHA1(10bdc5aa2d7d7f205f67b47b19003a4bd89defd1))
	ROM_RELOAD(0x2000, 0x2000)
	ROM_RELOAD(0x4000, 0x2000)
	ROM_RELOAD(0x6000, 0x2000)
ROM_END

const device_type COCO_FDC_V11 = &device_creator<coco_fdc_v11_device>;

//-------------------------------------------------
//  coco_fdc_v11_device - constructor
//-------------------------------------------------

coco_fdc_v11_device::coco_fdc_v11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, COCO_FDC_V11, "CoCo FDC v1.1", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void coco_fdc_v11_device::device_config_complete()
{
	m_shortname = "coco_fdc_v11";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *coco_fdc_v11_device::device_rom_region() const
{
	return ROM_NAME( coco_fdc_v11 );
}

//**************************************************************************
//              CP400 FDC
//**************************************************************************

ROM_START( cp400_fdc )
	ROM_REGION(0x4000,"eprom",ROMREGION_ERASE00)
	ROM_LOAD("cp400dsk.rom",  0x0000, 0x2000, CRC(e9ad60a0) SHA1(827697fa5b755f5dc1efb054cdbbeb04e405405b))
ROM_END

const device_type CP400_FDC = &device_creator<cp400_fdc_device>;

//-------------------------------------------------
//  cp400_fdc_device - constructor
//-------------------------------------------------

cp400_fdc_device::cp400_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : coco_fdc_device(mconfig, CP400_FDC, "CP400 FDC", tag, owner, clock)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cp400_fdc_device::device_config_complete()
{
	m_shortname = "cp400_fdc";
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *cp400_fdc_device::device_rom_region() const
{
	return ROM_NAME( cp400_fdc );
}
