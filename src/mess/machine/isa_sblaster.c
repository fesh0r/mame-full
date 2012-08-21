/***************************************************************************

  ISA 8 bit Creative Labs Sound Blaster Sound Card

  TODO:
  - implement DAC
  - DSP type is a MCS-51 family, it has an internal ROM that needs decapping;
  - implement jumpers DIP-SWs;

***************************************************************************/

#include "emu.h"
#include "isa_sblaster.h"
#include "sound/speaker.h"
#include "sound/3812intf.h"
#include "sound/saa1099.h"
#include "sound/dac.h"
#include "machine/pic8259.h"

/*
  adlib (YM3812/OPL2 chip), part of many many soundcards (soundblaster)
  soundblaster: YM3812 also accessible at 0x228/9 (address jumperable)
  soundblaster pro version 1: 2 YM3812 chips
   at 0x388 both accessed,
   at 0x220/1 left?, 0x222/3 right? (jumperable)
  soundblaster pro version 2: 1 OPL3 chip

  pro audio spectrum +: 2 OPL2
  pro audio spectrum 16: 1 OPL3

  2 x saa1099 chips
    also on sound blaster 1.0
    option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define ym3812_StdClock XTAL_3_579545MHz

static INPUT_PORTS_START( sblaster )
	PORT_START("pc_joy")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW,	 IPT_UNUSED ) // x/y ad stick to digital converters
	PORT_BIT( 0x10, IP_ACTIVE_LOW,   IPT_BUTTON1) PORT_NAME("SB: Joystick Button 1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW,   IPT_BUTTON2) PORT_NAME("SB: Joystick Button 2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW,   IPT_BUTTON3) PORT_NAME("SB: Joystick Button 3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW,   IPT_BUTTON4) PORT_NAME("SB: Joystick Button 4")

	PORT_START("pc_joy_1")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START("pc_joy_2")
	PORT_BIT(0xff,0x80,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1) PORT_MINMAX(1,0xff) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END

ioport_constructor sb8_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( sblaster );
}

static const int m_cmd_fifo_length[256] =
{
/*   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F        */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 0x */
	 2, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1,	-1,  1, -1, -1, -1, /* 1x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 2x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 3x */
	 2, -1, -1, -1, -1, -1, -1, -1,  3, -1, -1,	-1, -1, -1, -1, -1, /* 4x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 5x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 6x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 7x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 8x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* 9x */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* Ax */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* Bx */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	-1, -1, -1, -1, -1, /* Cx */
	 1,  1, -1,  1, -1, -1, -1, -1,  1, -1, -1,	-1, -1, -1, -1, -1, /* Dx */
	 2,  1,  2, -1,  2, -1, -1, -1,  1, -1, -1,	-1, -1, -1, -1, -1, /* Ex */
	-1, -1,  1, -1, -1, -1, -1, -1,  1, -1, -1,	-1, -1, -1, -1, -1  /* Fx */
};

static const int protection_magic[4][9] =
{
    {  1, -2, -4,  8, -16,  32,  64, -128, -106 },
    { -1,  2, -4,  8,  16, -32,  64, -128,  165 },
    { -1,  2,  4, -8,  16, -32, -64,  128, -151 },
    {  1, -2,  4, -8, -16,  32, -64,  128,  90 }
};

static const ym3812_interface pc_ym3812_interface =
{
	NULL
};

static MACHINE_CONFIG_FRAGMENT( sblaster1_0_config )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 3.00)
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

    MCFG_SOUND_ADD("sbdac", DAC, 0)
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( sblaster1_5_config )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	/* no CM/S support (empty sockets) */

    MCFG_SOUND_ADD("sbdac", DAC, 0)
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


static READ8_DEVICE_HANDLER( ym3812_16_r )
{
	UINT8 retVal = 0xff;
	switch(offset)
	{
		case 0 : retVal = ym3812_status_port_r( device, offset ); break;
	}
	return retVal;
}

static WRITE8_DEVICE_HANDLER( ym3812_16_w )
{
	switch(offset)
	{
		case 0 : ym3812_control_port_w( device, offset, data ); break;
		case 1 : ym3812_write_port_w( device, offset, data ); break;
	}
}

static READ8_DEVICE_HANDLER( saa1099_16_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( saa1099_16_w )
{
	switch(offset)
	{
		case 0 : saa1099_control_w( device, offset, data ); break;
		case 1 : saa1099_data_w( device, offset, data ); break;
	}
}

void sb8_device::queue(UINT8 data)
{
	if (m_dsp.fifo_ptr < 15)
	{
		m_dsp.fifo[m_dsp.fifo_ptr] = data;

		m_dsp.fifo_ptr++;
	}
	else
	{
		// FIFO gets to la-la-land
		//logerror("FIFO?\n");
	}
}

void sb8_device::queue_r(UINT8 data)
{
	m_dsp.rbuf_status |= 0x80;

	if (m_dsp.fifo_r_ptr < 15)
	{
		m_dsp.fifo_r[m_dsp.fifo_r_ptr] = data;

		m_dsp.fifo_r_ptr++;
	}
	else
	{
		// FIFO gets to la-la-land
		//logerror("FIFO?\n");
	}
}

UINT8 sb8_device::dequeue_r()
{
	UINT8 data = m_dsp.fifo_r[0];

	if (m_dsp.fifo_r_ptr > 0)
	{
		for (int i = 0; i < 15; i++)
			m_dsp.fifo_r[i] = m_dsp.fifo_r[i + 1];

		m_dsp.fifo_r[15] = 0;

		m_dsp.fifo_r_ptr--;
	}

	if(m_dsp.fifo_r_ptr == 0)
		m_dsp.rbuf_status &= ~0x80;

	return data;
}


READ8_MEMBER( sb8_device::dsp_reset_r )
{
//    printf("read DSP reset @ %x\n", offset);
	if(offset)
		return 0xff;
	logerror("Soundblaster DSP Reset port undocumented read\n");
	return 0xff;
}

WRITE8_MEMBER( sb8_device::dsp_reset_w )
{
//    printf("%02x to DSP reset @ %x\n", data, offset);
	if(offset)
		return;

	if(data == 0 && m_dsp.reset_latch == 1)
	{
		// reset routine
		m_dsp.fifo_ptr = 0;
		m_dsp.fifo_r_ptr = 0;
		for(int i=0;i < 15; i++)
		{
			m_dsp.fifo[i] = 0;
			m_dsp.fifo_r[i] = 0;
		}
		queue_r(0xaa); // reset OK ID
	}

	m_dsp.reset_latch = data;
	m_isa->drq1_w(0);
	m_dsp.dma_autoinit = 0;
	m_isa->irq5_w(0);
	m_timer->adjust(attotime::never, 0);
    m_dsp.d_rptr = 0;
    m_dsp.d_wptr = 0;
    m_dsp.dma_throttled = false;
    m_dsp.dma_timer_started = false;

	//printf("%02x\n",data);
}

READ8_MEMBER( sb8_device::dsp_data_r )
{
//    printf("read DSP data @ %x\n", offset);
	if(offset)
		return 0xff;

	return dequeue_r();
}

WRITE8_MEMBER( sb8_device::dsp_data_w )
{
//    printf("%02x to DSP data @ %x\n", data, offset);
	if(offset)
		return;
	logerror("Soundblaster DSP data port undocumented write\n");
}

READ8_MEMBER(sb8_device::dsp_rbuf_status_r)
{
//    printf("read Rbufstat @ %x\n", offset);

    if(offset)
		return 0xff;

//    printf("Clear IRQ5\n");
    m_isa->irq5_w(0);   // reading this port ACKs the card's IRQ

	return m_dsp.rbuf_status;
}

READ8_MEMBER(sb8_device::dsp_wbuf_status_r)
{
//    printf("read Wbufstat @ %x\n", offset);
	if(offset)
		return 0xff;

	return m_dsp.wbuf_status;
}

WRITE8_MEMBER(sb8_device::dsp_rbuf_status_w)
{
//    printf("%02x to Rbufstat @ %x\n", data, offset);
	if(offset)
		return;

	logerror("Soundblaster DSP Read Buffer status undocumented write\n");
}

void sb8_device::process_fifo(UINT8 cmd)
{
	if (m_cmd_fifo_length[cmd] == -1)
	{
		printf("unemulated or undefined fifo command %02x\n",cmd);
	}
	else if(m_dsp.fifo_ptr == m_cmd_fifo_length[cmd])
	{
		/* get FIFO params */
//        printf("SB FIFO command: %02x\n", cmd);
		switch(cmd)
		{
            case 0x10:  // Direct DAC
                break;

            case 0x14:  // 8-bit DMA, no autoinit
                m_dsp.dma_length = (m_dsp.fifo[1] + (m_dsp.fifo[2]<<8)) + 1;
//                printf("Start DMA (not autoinit, size = %x)\n", m_dsp.dma_length);
                m_dsp.dma_transferred = 0;
                m_dsp.dma_autoinit = 0;
                m_dsp.dma_timer_started = false;
                m_dsp.dma_throttled = false;
                m_isa->drq1_w(1);
                break;

            case 0x1c:  // 8-bit DMA with autoinit
//              printf("Start DMA (autoinit, size = %x)\n", m_dsp.dma_length);
                m_dsp.dma_transferred = 0;
                m_dsp.dma_autoinit = 1;
                m_dsp.dma_timer_started = false;
                m_dsp.dma_throttled = false;
                m_isa->drq1_w(1);
            	break;

            case 0x40:  // set time constant
                m_dsp.frequency = (1000000 / (256 - m_dsp.fifo[1]));
                printf("Set time constant: %02x -> %d\n", m_dsp.fifo[1], m_dsp.frequency);
                break;

			case 0x48:	// set DMA block size (for auto-init)
                m_dsp.dma_length = (m_dsp.fifo[1] + (m_dsp.fifo[2]<<8)) + 1;
				break;

            case 0xd0:  // halt 8-bit DMA
                m_timer->adjust(attotime::never, 0);
                m_isa->drq1_w(0);   // drop DRQ
                m_dsp.dma_throttled = false;
                m_dsp.dma_timer_started = false;
                break;

            case 0xd1: // speaker on
				// ...
				m_dsp.speaker_on = 1;
				break;

			case 0xd3: // speaker off
				// ...
				m_dsp.speaker_on = 0;
				break;

			case 0xd8: // speaker status
				queue_r(m_dsp.speaker_on ? 0xff : 0x00);
				break;

			case 0xe0: // get DSP identification
				queue_r(m_dsp.fifo[1] ^ 0xff);
				break;

			case 0xe1: // get DSP version
				queue_r(m_dsp.version >> 8);
				queue_r(m_dsp.version & 0xff);
				break;

            case 0xe2: // DSP protection
                for (int i = 0; i < 8; i++)
                {
                    if ((m_dsp.fifo[1] >> i) & 0x01)
                    {
                        m_dsp.prot_value += protection_magic[m_dsp.prot_count % 4][i];
                    }
                }

                m_dsp.prot_value += protection_magic[m_dsp.prot_count % 4][8];
                m_dsp.prot_count++;

                m_dack_out = (UINT8)(m_dsp.prot_value & 0xff);
                m_isa->drq1_w(1);
                break;

			case 0xe4: // write test register
				m_dsp.test_reg = m_dsp.fifo[1];
				break;

			case 0xe8: // read test register
				queue_r(m_dsp.test_reg);
				break;

			case 0xf2: // send PIC irq
				m_isa->irq5_w(1);
				break;

			case 0xf8: // ???
				logerror("SB: Unknown command write 0xf8");
				queue_r(0);
				break;
		}

		m_dsp.fifo_ptr = 0;
	}
}

WRITE8_MEMBER(sb8_device::dsp_cmd_w)
{
//  printf("%02x to DSP command @ %x\n", data, offset);

    if(offset)
		return;

	queue(data);

	process_fifo(m_dsp.fifo[0]);
}


READ8_MEMBER ( sb8_device::joy_port_r )
{
	UINT8 data = 0;
	int delta;
	attotime new_time = machine().time();

	{
		data = ioport("pc_joy")->read() | 0x0f;

		{
			delta = ((new_time - m_joy_time) * 256 * 1000).seconds;

			if (ioport("pc_joy_1")->read() < delta) data &= ~0x01;
			if (ioport("pc_joy_2")->read() < delta) data &= ~0x02;
		}
	}

	return data;
}



WRITE8_MEMBER ( sb8_device::joy_port_w )
{
	m_joy_time = machine().time();
}


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_SOUND_BLASTER_1_0 = &device_creator<isa8_sblaster1_0_device>;
const device_type ISA8_SOUND_BLASTER_1_5 = &device_creator<isa8_sblaster1_5_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_sblaster1_0_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( sblaster1_0_config );
}

machine_config_constructor isa8_sblaster1_5_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( sblaster1_5_config );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

sb8_device::sb8_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, UINT32 clock, const char *name) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_isa8_card_interface(mconfig, *this),
    m_dac(*this, "sbdac")
{
}


//-------------------------------------------------
//  isa8_sblaster_device - constructor
//-------------------------------------------------

isa8_sblaster1_0_device::isa8_sblaster1_0_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    sb8_device(mconfig, ISA8_SOUND_BLASTER_1_0, tag, owner, clock, "Sound Blaster 1.0")
{
}

isa8_sblaster1_5_device::isa8_sblaster1_5_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    sb8_device(mconfig, ISA8_SOUND_BLASTER_1_5, tag, owner, clock, "Sound Blaster 1.5")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void sb8_device::device_start()
{
	m_isa->install_device(                   0x0200, 0x0207, 0, 0, read8_delegate(FUNC(sb8_device::joy_port_r), this), write8_delegate(FUNC(sb8_device::joy_port_w), this));
	m_isa->install_device(                   0x0226, 0x0227, 0, 0, read8_delegate(FUNC(sb8_device::dsp_reset_r), this), write8_delegate(FUNC(sb8_device::dsp_reset_w), this));
	m_isa->install_device(subdevice("ym3812"),    0x0228, 0x0229, 0, 0, FUNC(ym3812_16_r), FUNC(ym3812_16_w) );
	m_isa->install_device(                   0x022a, 0x022b, 0, 0, read8_delegate(FUNC(sb8_device::dsp_data_r), this), write8_delegate(FUNC(sb8_device::dsp_data_w), this) );
	m_isa->install_device(                   0x022c, 0x022d, 0, 0, read8_delegate(FUNC(sb8_device::dsp_wbuf_status_r), this), write8_delegate(FUNC(sb8_device::dsp_cmd_w), this) );
	m_isa->install_device(                   0x022e, 0x022f, 0, 0, read8_delegate(FUNC(sb8_device::dsp_rbuf_status_r), this), write8_delegate(FUNC(sb8_device::dsp_rbuf_status_w), this) );
	m_isa->install_device(subdevice("ym3812"),    0x0388, 0x0389, 0, 0, FUNC(ym3812_16_r), FUNC(ym3812_16_w) );

    m_timer = timer_alloc(0, NULL);
}

void isa8_sblaster1_0_device::device_start()
{
    set_isa_device();
    // 1.0 always has the SAA1099s for CMS back-compatibility
	m_isa->install_device(subdevice("saa1099.1"), 0x0220, 0x0221, 0, 0, FUNC(saa1099_16_r), FUNC(saa1099_16_w) );
	m_isa->install_device(subdevice("saa1099.2"), 0x0222, 0x0223, 0, 0, FUNC(saa1099_16_r), FUNC(saa1099_16_w) );
	m_isa->set_dma_channel(1, this, FALSE);
	m_dsp.version = 0x0105;
    sb8_device::device_start();
}

void isa8_sblaster1_5_device::device_start()
{
    set_isa_device();
	/* 1.5 makes CM/S support optional (empty sockets, but they work if the user populates them!) */
	m_isa->set_dma_channel(1, this, FALSE);
	m_dsp.version = 0x0200;
    sb8_device::device_start();
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void sb8_device::device_reset()
{
    m_dsp.prot_value = 0xaa;
    m_dsp.prot_count = 0;
    m_dack_out = 0;
    m_dsp.fifo_ptr = 0;
    m_dsp.fifo_r_ptr = 0;
    m_dsp.wbuf_status = 0;
    m_dsp.rbuf_status = 0;
}

UINT8 sb8_device::dack_r(int line)
{
//    printf("dack_r: line %x out = %02x\n", line, m_dack_out);
	m_isa->drq1_w(0);	// drop DRQ?
    return m_dack_out;
}

/* TODO: this mustn't be instant! */
void sb8_device::dack_w(int line, UINT8 data)
{
//    printf("dack_w: line %x data %02x\n", line, data);
//  if(data != 0x80)
//      printf("%02x\n",data);

    // set the transfer timer on the 1st byte
    if (!m_dsp.dma_timer_started)
    {
        m_timer->adjust(attotime::from_hz((double)m_dsp.frequency), 0, attotime::from_hz((double)m_dsp.frequency));
        m_dsp.d_rptr = m_dsp.d_wptr = 0;
        m_dsp.dma_timer_started = true;
    }

    m_dsp.data[m_dsp.d_wptr++] = data;
    m_dsp.d_wptr %= 128;

    if (m_dsp.d_wptr == m_dsp.d_rptr)
    {
//        printf("throttling DRQ\n");
        m_isa->drq1_w(0);	// drop DRQ here
        m_dsp.dma_throttled = true;
    }

    m_dsp.dma_transferred++;
    if (m_dsp.dma_transferred >= m_dsp.dma_length)
    {
//        printf("DMA fill completed (%d out of %d)\n", m_dsp.dma_transferred, m_dsp.dma_length);

        m_isa->drq1_w(0);	// drop DRQ here

        if (m_dsp.dma_autoinit)
        {
//            printf("autoinit reset\n");
            m_dsp.dma_transferred = 0;
            if (!m_dsp.dma_throttled)   // if we're not throttled, re-raise DRQ right now
            {
                m_isa->drq1_w(1);   // raise DRQ again (page 3-15 of the Creative manual indicates auto-init will keep going until you stop it)
            }
        }

        m_isa->irq5_w(1);	// raise IRQ as per the Creative manual
    }
}

void sb8_device::device_timer(emu_timer &timer, device_timer_id tid, int param, void *ptr)
{
//    printf("DMA timer expire\n");

    m_dac->write_unsigned8(m_dsp.data[m_dsp.d_rptr]);
    m_dsp.data[m_dsp.d_rptr++] = 0x80;
    m_dsp.d_rptr %= 128;

    if (m_dsp.dma_throttled)
    {
        if (m_dsp.d_rptr == m_dsp.d_wptr)
        {
//            printf("unthrottling DRQ\n");
            m_isa->drq1_w(1);   // raise DRQ
            m_dsp.dma_throttled = false;
        }
    }
}
