/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

#include "emu.h"
#include "isa_mda.h"
#include "video/mc6845.h"
#include "machine/pc_lpt.h"

#define MDA_SCREEN_NAME	"mda_screen"
#define MDA_MC6845_NAME	"mc6845_mda"

/*
  Hercules video card
 */
#define HERCULES_SCREEN_NAME "hercules_screen"
#define HERCULES_MC6845_NAME "mc6845_hercules"

#define VERBOSE_MDA	0		/* MDA (Monochrome Display Adapter) */

#define MDA_CLOCK	16257000

#define MDA_LOG(N,M,A) \
	do { \
		if(VERBOSE_MDA>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",device->machine().time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

static const unsigned char mda_palette[4][3] =
{
	{ 0x00,0x00,0x00 },
	{ 0x00,0x55,0x00 },
	{ 0x00,0xaa,0x00 },
	{ 0x00,0xff,0x00 }
};

static MC6845_UPDATE_ROW( mda_update_row );

/* F4 Character Displayer */
static const gfx_layout pc_16_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 2048*8, 2049*8, 2050*8, 2051*8, 2052*8, 2053*8, 2054*8, 2055*8 },
	8*8					/* every char takes 2 x 8 bytes */
};

static const gfx_layout pc_8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	512,					/* 512 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( pcmda )
	GFXDECODE_ENTRY( "mda:gfx1", 0x0000, pc_16_charlayout, 1, 1 )
	GFXDECODE_ENTRY( "mda:gfx1", 0x1000, pc_8_charlayout, 1, 1 )
GFXDECODE_END

static const mc6845_interface mc6845_mda_intf =
{
	MDA_SCREEN_NAME, /* screen number */
	9,					/* number of pixels per video memory address */
	NULL,				/* begin_update */
	mda_update_row,		/* update_row */
	NULL,				/* end_update */
	DEVCB_NULL,				/* on_de_changed */
	DEVCB_NULL,				/* on_cur_changed */
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_mda_device, hsync_changed),	/* on_hsync_changed */
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_mda_device, vsync_changed),	/* on_vsync_changed */
	NULL
};

static WRITE_LINE_DEVICE_HANDLER(pc_cpu_line)
{
	isa8_mda_device	*mda  = downcast<isa8_mda_device *>(device->owner());
	mda->m_isa->irq7_w(state);
}
static const pc_lpt_interface pc_lpt_config =
{
	DEVCB_LINE(pc_cpu_line)
};


MACHINE_CONFIG_FRAGMENT( pcvideo_mda )
	MCFG_SCREEN_ADD( MDA_SCREEN_NAME, RASTER)
	MCFG_SCREEN_RAW_PARAMS(MDA_CLOCK, 882, 0, 720, 370, 0, 350 )
	MCFG_SCREEN_UPDATE_DEVICE( MDA_MC6845_NAME, mc6845_device, screen_update )

	MCFG_PALETTE_LENGTH( 4 )

	MCFG_MC6845_ADD( MDA_MC6845_NAME, MC6845, MDA_CLOCK/9, mc6845_mda_intf)

	//MCFG_GFXDECODE(pcmda)

	MCFG_PC_LPT_ADD("lpt", pc_lpt_config)
MACHINE_CONFIG_END

ROM_START( mda )
	/* IBM 1501981(CGA) and 1501985(MDA) Character rom */
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x02000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) /* "AMI 8412PI // 5788005 // (C) IBM CORP. 1981 // KOREA" */
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_MDA = &device_creator<isa8_mda_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_mda_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( pcvideo_mda );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *isa8_mda_device::device_rom_region() const
{
	return ROM_NAME( mda );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_mda_device - constructor
//-------------------------------------------------

isa8_mda_device::isa8_mda_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8_MDA, "IBM Monochrome Display and Printer Adapter", tag, owner, clock),
		device_isa8_card_interface(mconfig, *this)
{
	m_shortname = "isa_ibm_mda";
}

isa8_mda_device::isa8_mda_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_isa8_card_interface(mconfig, *this)
{
	m_shortname = "isa_ibm_mda";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_mda_device::device_start()
{
	set_isa_device();
	m_videoram = auto_alloc_array(machine(), UINT8, 0x1000);
	m_isa->install_device(0x3b0, 0x3bf, 0, 0, read8_delegate( FUNC(isa8_mda_device::io_read), this ), write8_delegate( FUNC(isa8_mda_device::io_write), this ) );
	m_isa->install_bank(0xb0000, 0xb0fff, 0, 0x07000, "bank_mda", m_videoram);

	/* Initialise the mda palette */
	for(int i = 0; i < (sizeof(mda_palette) / 3); i++)
		palette_set_color_rgb(machine(), i, mda_palette[i][0], mda_palette[i][1], mda_palette[i][2]);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_mda_device::device_reset()
{
	m_update_row = NULL;
	m_framecnt = 0;
	m_mode_control = 0;
	m_vsync = 0;
	m_hsync = 0;
	m_pixel = 0;

	astring tempstring;
	m_chr_gen = memregion(subtag(tempstring, "gfx1"))->base();
}

/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

/***************************************************************************
  Draw text mode with 80x25 characters (default) and intense background.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/

static MC6845_UPDATE_ROW( mda_text_inten_update_row )
{
	isa8_mda_device	*mda  = downcast<isa8_mda_device *>(device->owner());
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	UINT32	*p = &bitmap.pix32(y);
	UINT16	chr_base = ( ra & 0x08 ) ? 0x800 | ( ra & 0x07 ) : ra;
	int i;

	if ( y == 0 ) MDA_LOG(1,"mda_text_inten_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x0FFF;
		UINT8 chr = mda->m_videoram[ offset ];
		UINT8 attr = mda->m_videoram[ offset + 1 ];
		UINT8 data = mda->m_chr_gen[ chr_base + chr * 8 ];
		UINT8 fg = ( attr & 0x08 ) ? 3 : 2;
		UINT8 bg = 0;

		if ( ( attr & ~0x88 ) == 0 )
		{
			data = 0x00;
		}

		switch( attr )
		{
		case 0x70:
			bg = 2;
			fg = 0;
			break;
		case 0x78:
			bg = 2;
			fg = 1;
			break;
		case 0xF0:
			bg = 3;
			fg = 0;
			break;
		case 0xF8:
			bg = 3;
			fg = 1;
			break;
		}

		if ( ( i == cursor_x && ( mda->m_framecnt & 0x08 ) ) || ( attr & 0x07 ) == 0x01 )
		{
			data = 0xFF;
		}

		*p = palette[( data & 0x80 ) ? fg : bg]; p++;
		*p = palette[( data & 0x40 ) ? fg : bg]; p++;
		*p = palette[( data & 0x20 ) ? fg : bg]; p++;
		*p = palette[( data & 0x10 ) ? fg : bg]; p++;
		*p = palette[( data & 0x08 ) ? fg : bg]; p++;
		*p = palette[( data & 0x04 ) ? fg : bg]; p++;
		*p = palette[( data & 0x02 ) ? fg : bg]; p++;
		*p = palette[( data & 0x01 ) ? fg : bg]; p++;
		if ( ( chr & 0xE0 ) == 0xC0 )
		{
			*p = palette[( data & 0x01 ) ? fg : bg]; p++;
		}
		else
		{
			*p = palette[bg]; p++;
		}
	}
}


/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking characters.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/

static MC6845_UPDATE_ROW( mda_text_blink_update_row )
{
	isa8_mda_device	*mda  = downcast<isa8_mda_device *>(device->owner());
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	UINT32	*p = &bitmap.pix32(y);
	UINT16	chr_base = ( ra & 0x08 ) ? 0x800 | ( ra & 0x07 ) : ra;
	int i;

	if ( y == 0 ) MDA_LOG(1,"mda_text_blink_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x0FFF;
		UINT8 chr = mda->m_videoram[ offset ];
		UINT8 attr = mda->m_videoram[ offset + 1 ];
		UINT8 data = mda->m_chr_gen[ chr_base + chr * 8 ];
		UINT8 fg = ( attr & 0x08 ) ? 3 : 2;
		UINT8 bg = 0;

		if ( ( attr & ~0x88 ) == 0 )
		{
			data = 0x00;
		}

		switch( attr )
		{
		case 0x70:
		case 0xF0:
			bg = 2;
			fg = 0;
			break;
		case 0x78:
		case 0xF8:
			bg = 2;
			fg = 1;
			break;
		}

		if ( ( attr & 0x07 ) == 0x01 )
		{
			data = 0xFF;
		}

		if ( i == cursor_x )
		{
			if ( mda->m_framecnt & 0x08 )
			{
				data = 0xFF;
			}
		}
		else
		{
			if ( ( attr & 0x80 ) && ( mda->m_framecnt & 0x10 ) )
			{
				data = 0x00;
			}
		}

		*p = palette[( data & 0x80 ) ? fg : bg]; p++;
		*p = palette[( data & 0x40 ) ? fg : bg]; p++;
		*p = palette[( data & 0x20 ) ? fg : bg]; p++;
		*p = palette[( data & 0x10 ) ? fg : bg]; p++;
		*p = palette[( data & 0x08 ) ? fg : bg]; p++;
		*p = palette[( data & 0x04 ) ? fg : bg]; p++;
		*p = palette[( data & 0x02 ) ? fg : bg]; p++;
		*p = palette[( data & 0x01 ) ? fg : bg]; p++;
		if ( ( chr & 0xE0 ) == 0xC0 )
		{
			*p = palette[( data & 0x01 ) ? fg : bg]; p++;
		}
		else
		{
			*p = palette[bg]; p++;
		}
	}
}


static MC6845_UPDATE_ROW( mda_update_row )
{
	isa8_mda_device	*mda  = downcast<isa8_mda_device *>(device->owner());
	if ( mda->m_update_row )
	{
		mda->m_update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


WRITE_LINE_MEMBER( isa8_mda_device::hsync_changed )
{
	m_hsync = state ? 1 : 0;
}


WRITE_LINE_MEMBER( isa8_mda_device::vsync_changed )
{
	m_vsync = state ? 0x80 : 0;
	if ( state )
	{
		m_framecnt++;
	}
}


/*
 *  rW  MDA mode control register (see #P138)
 */
WRITE8_MEMBER( isa8_mda_device::mode_control_w )
{
	m_mode_control = data;

	switch( m_mode_control & 0x2a )
	{
	case 0x08:
		m_update_row = mda_text_inten_update_row;
		break;
	case 0x28:
		m_update_row = mda_text_blink_update_row;
		break;
	default:
		m_update_row = NULL;
	}
}


/*  R-  CRT status register (see #P139)
 *      (EGA/VGA) input status 1 register
 *      7    HGC vertical sync in progress
 *      6-4  adapter 000  hercules
 *                   001  hercules+
 *                   101  hercules InColor
 *                   else unknown
 *      3    pixel stream (0 black, 1 white)
 *      2-1  reserved
 *      0    horizontal drive enable
 */
READ8_MEMBER( isa8_mda_device::status_r)
{
	// Faking pixel stream here
	m_pixel++;

	return 0xF0 | (m_pixel & 0x08) | m_hsync;
}


/*************************************************************************
 *
 *      MDA
 *      monochrome display adapter
 *
 *************************************************************************/
WRITE8_MEMBER( isa8_mda_device::io_write)
{
	mc6845_device *mc6845 = subdevice<mc6845_device>(MDA_MC6845_NAME);
	device_t *lpt = subdevice("lpt");
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			mc6845->address_w( space, offset, data );
			break;
		case 1: case 3: case 5: case 7:
			mc6845->register_w( space, offset, data );
			break;
		case 8:
			mode_control_w(space, offset, data);
			break;
		case 12: case 13:  case 14:
			pc_lpt_w(lpt, offset - 12, data);
			break;
	}
}

READ8_MEMBER( isa8_mda_device::io_read)
{
	int data = 0xff;
	mc6845_device *mc6845 = subdevice<mc6845_device>(MDA_MC6845_NAME);
	device_t *lpt = subdevice("lpt");
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			/* return last written mc6845 address value here? */
			break;
		case 1: case 3: case 5: case 7:
			data = mc6845->register_r( space, offset );
			break;
		case 10:
			data = status_r(space, offset);
			break;
		/* 12, 13, 14  are the LPT ports */
		case 12: case 13:  case 14:
			data = pc_lpt_r(lpt, offset - 12);
			break;
    }
	return data;
}


/***************************************************************************

  Hercules Display Adapter section (re-uses parts from the MDA section)

***************************************************************************/

/*
When the Hercules changes to graphics mode, the number of pixels per access and
clock divider should be changed. The currect mc6845 implementation does not
allow this.

The divder/pixels per 6845 clock is 9 for text mode and 16 for graphics mode.
*/

static const mc6845_interface mc6845_hercules_intf =
{
	HERCULES_SCREEN_NAME,	/* screen number */
	9,						/* number of pixels per video memory address */
	NULL,					/* begin_update */
	mda_update_row,			/* update_row */
	NULL,					/* end_update */
	DEVCB_NULL,				/* on_de_changed */
	DEVCB_NULL,				/* on_cur_changed */
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_mda_device, hsync_changed),	/* on_hsync_changed */
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_mda_device, vsync_changed),	/* on_vsync_changed */
	NULL
};

static GFXDECODE_START( pcherc )
	GFXDECODE_ENTRY( "hercules:gfx1", 0x0000, pc_16_charlayout, 1, 1 )
GFXDECODE_END

MACHINE_CONFIG_FRAGMENT( pcvideo_hercules )
	MCFG_SCREEN_ADD( HERCULES_SCREEN_NAME, RASTER)
	MCFG_SCREEN_RAW_PARAMS(MDA_CLOCK, 882, 0, 720, 370, 0, 350 )
	MCFG_SCREEN_UPDATE_DEVICE( HERCULES_MC6845_NAME, mc6845_device, screen_update )

	MCFG_PALETTE_LENGTH( 4 )

	MCFG_MC6845_ADD( HERCULES_MC6845_NAME, MC6845, MDA_CLOCK/9, mc6845_hercules_intf)

	//MCFG_GFXDECODE(pcherc)

	MCFG_PC_LPT_ADD("lpt", pc_lpt_config)
MACHINE_CONFIG_END

ROM_START( hercules )
	ROM_REGION(0x1000,"gfx1", 0)
	ROM_LOAD("um2301.bin",  0x00000, 0x1000, CRC(0827bdac) SHA1(15f1aceeee8b31f0d860ff420643e3c7f29b5ffc))
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_HERCULES = &device_creator<isa8_hercules_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_hercules_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( pcvideo_hercules );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *isa8_hercules_device::device_rom_region() const
{
	return ROM_NAME( hercules );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_hercules_device - constructor
//-------------------------------------------------

isa8_hercules_device::isa8_hercules_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        isa8_mda_device(mconfig, ISA8_HERCULES, "Hercules Graphics Card", tag, owner, clock)
{
	m_shortname = "isa_hercules";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_hercules_device::device_start()
{
	m_videoram = auto_alloc_array(machine(), UINT8, 0x10000);
	set_isa_device();
	m_isa->install_device(0x3b0, 0x3bf, 0, 0, read8_delegate( FUNC(isa8_hercules_device::io_read), this ), write8_delegate( FUNC(isa8_hercules_device::io_write), this ) );
	m_isa->install_bank(0xb0000, 0xbffff, 0, 0, "bank_hercules", m_videoram);

	/* Initialise the mda palette */
	for(int i = 0; i < (sizeof(mda_palette) / 3); i++)
		palette_set_color_rgb(machine(), i, mda_palette[i][0], mda_palette[i][1], mda_palette[i][2]);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_hercules_device::device_reset()
{
	isa8_mda_device::device_reset();
	m_configuration_switch = 0;

	astring tempstring;
	m_chr_gen = memregion(subtag(tempstring, "gfx1"))->base();
}

/***************************************************************************
  Draw graphics with 720x348 pixels (default); so called Hercules gfx.
  The memory layout is divided into 4 banks where of size 0x2000.
  Every bank holds data for every n'th scanline, 8 pixels per byte,
  bit 7 being the leftmost.
***************************************************************************/

static MC6845_UPDATE_ROW( hercules_gfx_update_row )
{
	isa8_hercules_device *herc  = downcast<isa8_hercules_device *>(device->owner());
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	UINT32	*p = &bitmap.pix32(y);
	UINT16	gfx_base = ( ( herc->m_mode_control & 0x80 ) ? 0x8000 : 0x0000 ) | ( ( ra & 0x03 ) << 13 );
	int i;
	if ( y == 0 ) MDA_LOG(1,"hercules_gfx_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT8	data = herc->m_videoram[ gfx_base + ( ( ma + i ) << 1 ) ];

		*p = palette[( data & 0x80 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x40 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x20 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x10 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x08 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x04 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x02 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x01 ) ? 2 : 0]; p++;

		data = herc->m_videoram[ gfx_base + ( ( ma + i ) << 1 ) + 1 ];

		*p = palette[( data & 0x80 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x40 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x20 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x10 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x08 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x04 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x02 ) ? 2 : 0]; p++;
		*p = palette[( data & 0x01 ) ? 2 : 0]; p++;
	}
}


WRITE8_MEMBER( isa8_hercules_device::mode_control_w )
{
	mc6845_device *mc6845 = subdevice<mc6845_device>(HERCULES_MC6845_NAME);

	m_mode_control = data;

	switch( m_mode_control & 0x2a )
	{
	case 0x08:
		m_update_row = mda_text_inten_update_row;
		break;
	case 0x28:
		m_update_row = mda_text_blink_update_row;
		break;
	case 0x0A:          /* Hercules modes */
	case 0x2A:
		m_update_row = hercules_gfx_update_row;
		break;
	default:
		m_update_row = NULL;
	}

	mc6845->set_clock( m_mode_control & 0x02 ? MDA_CLOCK / 16 : MDA_CLOCK / 9 );
	mc6845->set_hpixels_per_column( m_mode_control & 0x02 ? 16 : 9 );
}


WRITE8_MEMBER( isa8_hercules_device::io_write )
{
	mc6845_device *mc6845 = subdevice<mc6845_device>(HERCULES_MC6845_NAME);
	device_t *lpt = subdevice("lpt");
	switch( offset )
	{
	case 0: case 2: case 4: case 6:
		mc6845->address_w( space, offset, data );
		break;
	case 1: case 3: case 5: case 7:
		mc6845->register_w( space, offset, data );
		break;
	case 8:
		mode_control_w(space, offset, data);
		break;
	case 12: case 13:  case 14:
		pc_lpt_w(lpt, offset - 12, data);
		break;
	case 15:
		m_configuration_switch = data;
		break;
	}
}


/*  R-  CRT status register (see #P139)
 *      (EGA/VGA) input status 1 register
 *      7    HGC vertical sync in progress
 *      6-4  adapter 000  hercules
 *                   001  hercules+
 *                   101  hercules InColor
 *                   else unknown
 *      3    pixel stream (0 black, 1 white)
 *      2-1  reserved
 *      0    horizontal drive enable
 */
READ8_MEMBER( isa8_hercules_device::status_r )
{
	// Faking pixel stream here
	m_pixel++;

	return m_vsync | ( m_pixel & 0x08 ) | m_hsync;
}


READ8_MEMBER( isa8_hercules_device::io_read )
{
	int data = 0xff;
	mc6845_device *mc6845 = subdevice<mc6845_device>(HERCULES_MC6845_NAME);
	device_t *lpt = subdevice("lpt");
	switch( offset )
	{
	case 0: case 2: case 4: case 6:
		/* return last written mc6845 address value here? */
		break;
	case 1: case 3: case 5: case 7:
		data = mc6845->register_r( space, offset );
		break;
	case 10:
		data = status_r(space, offset);
		break;
	/* 12, 13, 14  are the LPT ports */
	case 12: case 13:  case 14:
		data = pc_lpt_r(lpt, offset - 12);
		break;
	}
	return data;
}
