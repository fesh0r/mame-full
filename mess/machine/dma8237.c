/* direct memory access controller
   intel 8237

   page register for bigger than 16 bit address buses are done in separate discrete hardware
   used in complete pc series */

#include "driver.h"

#include "includes/dma8237.h"

#if 0
#define DMA_LOG(level, text, print) \
		if (level>0) { \
				logerror("%s %g %06x\t", text, timer_get_time(), activecpu_get_pc()); \
				logerror print; \
		}
#else
#define DMA_LOG(level, text, print)
#endif

DMA8237 dma8237[2]= {
	{ { DMA8237_PC } }
};

void dma8237_config(DMA8237 *This, DMA8237_CONFIG *config)
{
	This->config=*config;
}

void dma8237_reset(DMA8237 *This)
{
	This->status &= ~0xf0;	/* reset DMA running flag */
	This->status |= 0x0f;		/* set DMA terminal count flag */
	This->chan[0].operation=0;
	This->chan[1].operation=0;
	This->chan[2].operation=0;
	This->chan[3].operation=0;
}

static void dma8237_w(DMA8237 *this, offs_t offset, data8_t data)
{
	offset&=0xf;
	switch( offset )
	{
    case 0: case 2: case 4: case 6:
        DMA_LOG(1,"DMA_address_w",("chan #%d $%02x: ", offset>>1, data));
        if (this->msb) {
            this->chan[offset>>1].address |= (data & 0xff) << 8;
            this->chan[offset>>1].base_address |= (data & 0xff) << 8;
        } else {
            this->chan[offset>>1].address = data & 0xff;
            this->chan[offset>>1].base_address = data & 0xff;
		}
        DMA_LOG(1,"",("[$%04x]\n", this->chan[offset>>1].address));
        this->msb ^= 1;
        break;
    case 1: case 3: case 5: case 7:
        DMA_LOG(1,"DMA_count_w",("chan #%d $%02x: ", offset>>1, data));
        if (this->msb) {
            this->chan[offset>>1].count |= (data & 0xff) << 8;
            this->chan[offset>>1].base_count |= (data & 0xff) << 8;
        } else {
            this->chan[offset>>1].count = data & 0xff;
            this->chan[offset>>1].base_count = data & 0xff;
		}
        DMA_LOG(1,"",("[$%04x]\n", this->chan[offset>>1].count));
        this->msb ^= 1;
        break;
    case 8:    /* DMA command register */
        this->command = data;
        this->DACK_hi = (this->command >> 7) & 1;
        this->DREQ_hi = (this->command >> 6) & 1;
        this->write_extended = (this->command >> 5) & 1;
        this->rotate_priority = (this->command >> 4) & 1;
        this->compressed_timing = (this->command >> 3) & 1;
        this->enable_controller = (this->command >> 2) & 1;
        this->channel = this->command & 3;
        DMA_LOG(1,"DMA_command_w",("$%02x: chan #%d, enable %d, CT %d, RP %d, WE %d, DREQ %d, DACK %d\n", data,
            this->channel, this->enable_controller, this->compressed_timing, this->rotate_priority, this->write_extended, this->DREQ_hi, this->DACK_hi));
        break;
    case 9:    /* DMA write request register */
        this->channel = data & 3;
        if (data & 0x04) {
            DMA_LOG(1,"DMA_request_w",("$%02x: set chan #%d\n", data, this->channel));
            /* set the DMA request bit for the given channel */
            this->status |= 0x10 << this->channel;
        } else {
            DMA_LOG(1,"DMA_request_w",("$%02x: clear chan #%d\n", data, this->channel));
            /* clear the DMA request bit for the given channel */
            this->status &= ~(0x11 << this->channel);
        }
        break;
    case 10:    /* DMA mask register */
        this->channel = data & 3;
        if (data & 0x04) {
            DMA_LOG(1,"DMA_mask_w",("$%02x: set chan #%d\n", data, this->channel));
            /* set the DMA request bit for the given channel */
            this->mask |= 0x11 << this->channel;
        } else {
            DMA_LOG(1,"DMA_mask_w",("$%02x: clear chan #%d\n", data, this->channel));
            /* clear the DMA request bit for the given channel */
            this->mask &= ~(0x11 << this->channel);
        }
        break;
    case 11:    /* DMA mode register */
        this->channel = data & 3;
        this->chan[this->channel].operation = (data >> 2) & 3;
        this->chan[this->channel].direction = (data & 0x20) ? -1 : +1;
        this->chan[this->channel].transfer_mode = (data >> 6) & 3;
        DMA_LOG(1,"DMA_mode_w",("$%02x: chan #%d, oper %d, dir %d, mode %d\n", data, data&3, (data>>2)&3, (data>>5)&1, (data>>6)&3 ));
        break;
    case 12:    /* DMA clear byte pointer flip-flop */
        DMA_LOG(1,"DMA_clear_ff_w",("$%02x\n", data));
        this->temp = data;
        this->msb = 0;
        break;
    case 13:    /* DMA master clear */
        DMA_LOG(1,"DMA_master_w",("$%02x\n", data));
        this->msb = 0;
        break;
    case 14:    /* DMA clear mask register */
        this->mask &= ~data;
        DMA_LOG(1,"DMA_mask_clr_w",("$%02x -> mask $%02x\n", data, this->mask));
        break;
    case 15:    /* DMA write mask register */
        this->mask |= data;
        DMA_LOG(1,"DMA_mask_clr_w",("$%02x -> mask $%02x\n", data, this->mask));
        break;
    }
}

static int dma8237_r(DMA8237 *this, offs_t offset)
{
	int data = 0xff;
	offset&=0xf;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			if (this->msb)
				data = (this->chan[offset>>1].address >> 8) & 0xff;
			else
				data = this->chan[offset>>1].address & 0xff;

			DMA_LOG(1,"DMA_address_r",("chan #%d $%02x ($%04x)\n", offset>>1, data, 
									   this->chan[offset>>1].address));
			this->msb ^= 1;

			if ( (this->chan[0].operation==2)&&(offset==0) ) {
				// hack simulating refresh activity for ibmxt bios
				this->chan[0].address++;
				this->chan[0].count--;
				if (this->chan[0].count==0xffff) {
					this->chan[0].address=this->chan[0].base_address;
					this->chan[0].count=this->chan[0].base_count;
				}
			}
			break;

		case 1: case 3: case 5: case 7:
			if (this->msb)
				data = (this->chan[offset>>1].count >> 8) & 0xff;
			else
				data = this->chan[offset>>1].count & 0xff;
			DMA_LOG(1,"DMA_count_r",("chan #%d $%02x ($%04x)\n", offset>>1, data, this->chan[offset>>1].count));
			this->msb ^= 1;
			break;

		case 8: /* DMA status register */
			data = this->status|1; // for noname turbo xt
			DMA_LOG(1,"DMA_status_r",("$%02x\n", data));
			break;

		case 9: /* DMA write request register */
			break;

		case 10: /* DMA mask register */
			data = this->mask;
			DMA_LOG(1,"DMA_mask_r",("$%02x\n", data));
			break;

		case 11: /* DMA mode register */
			break;

		case 12: /* DMA clear byte pointer flip-flop */
			break;

		case 13: /* DMA master clear */
			data = this->temp;
			DMA_LOG(1,"DMA_temp_r",("$%02x\n", data));
			break;

		case 14: /* DMA clear mask register */
			break;

		case 15: /* DMA write mask register */
			break;
	}
	return data;
}

UINT8 dma8237_write(DMA8237 *this, int channel)
{
	data8_t data;

	/* read byte from pc mem and update address */
	if (this->chan[channel].operation == 2)
		data = program_read_byte(this->chan[channel].page + this->chan[channel].address);
	else
		data = 0x0ff;

	this->chan[channel].address += this->chan[channel].direction;
	this->chan[channel].count--;

	/* if all data transfered, issue a terminal count to fdc */
	if (this->chan[channel].count==0xffff)
	{
		this->status &= ~(0x10 << channel);	/* reset DMA running flag */
		this->status |= 0x01 << channel;		/* set DMA terminal count flag */

	}
	return data;
}

/* reading from FDC with dma */
void dma8237_read(DMA8237 *this, int channel, UINT8 data)
{

	/* write byte to pc mem and update mem address */
	if (this->chan[channel].operation == 1)
		program_write_byte(this->chan[channel].page + this->chan[channel].address, data);

	this->chan[channel].address += this->chan[channel].direction;
	this->chan[channel].count--;

	/* if all data transfered, issue a terminal count to fdc */
	if (this->chan[channel].count==0xffff)
	{
		this->status &= ~(0x10 << channel);	/* reset DMA running flag */
		this->status |= 0x01 << channel;		/* set DMA terminal count flag */
	}

}


void pc_page_w(offs_t offset, data8_t data)
{
	offset&=3;
	switch( offset )
	{
	case 1: /* DMA page register 2 */
		DMA_LOG(1,"DMA_page_2_w",("$%02x\n", data));
		dma8237->chan[2].page = (data << 16) & 0x0f0000;
		break;
	case 2:    /* DMA page register 3 */
		DMA_LOG(1,"DMA_page_3_w",("$%02x\n", data));
		dma8237->chan[3].page = (data << 16) & 0x0f0000;
		break;
	case 3:    /* DMA page register 0 */
		DMA_LOG(1,"DMA_page_0+1_w",("$%02x\n", data));
		/* this is verified with pc hardwarebuch and pc circuit diagram !*/
		dma8237->chan[0].page = dma8237->chan[1].page = (data << 16) & 0x0f0000;
		break;
    }
}


static UINT8 pages[0x10]={ 0 };

/* in an at SN74LS612N, full 16 register memory mapper,
   so 0x80-0x8f read and writeable */
void at_page_w(offs_t offset, data8_t data)
{
	offset&=0xf;
	pages[offset]=data;
	logerror("at page write %g %06x\t%02x %02x\n", timer_get_time(), activecpu_get_pc(), offset, data);
	switch( offset )
	{
	case 0:
		logerror("postcode %.2x\n",data);
		break;
	case 1: /* DMA page register 2 */
		DMA_LOG(1,"DMA_page_2_w",("$%02x\n", data));
		dma8237->chan[2].page = (data << 16) & 0xff0000;
		break;
	case 2:    /* DMA page register 3 */
		DMA_LOG(1,"DMA_page_3_w",("$%02x\n", data));
		dma8237->chan[3].page = (data << 16) & 0xff0000;
		break;
	case 3:    /* DMA page register 1 */
		DMA_LOG(1,"DMA_page_1_w",( "$%02x\n", data));
		dma8237->chan[1].page = (data << 16) & 0xff0000;
		break;
	case 7:    /* DMA page register 0 */
		DMA_LOG(1,"DMA_page_0_w",("$%02x\n", data));
		dma8237->chan[0].page = (data << 16) & 0xff0000;
		break;
	case 0xf:
		DMA_LOG(1,"DMA_page_4_w",("$%02x\n", data));
		dma8237[1].chan[0].page = (data << 16) & 0xff0000;
		break;
	case 0xb:
		DMA_LOG(1,"DMA_page_5_w",("$%02x\n", data));
		dma8237[1].chan[1].page = (data << 16) & 0xff0000;
		break;
	case 0x9:
		DMA_LOG(1,"DMA_page_6_w",("$%02x\n", data));
		dma8237[1].chan[2].page = (data << 16) & 0xff0000;
		break;
	case 0xa:
		DMA_LOG(1,"DMA_page_7_w",("$%02x\n", data));
		dma8237[1].chan[3].page = (data << 16) & 0xff0000;
		break;
    }
}

READ_HANDLER( pc_page_r )
{
	return 0xff; // in pc not readable !
}

READ_HANDLER( at_page_r )
{
	int data = 0xff;
	offset&=0xf;
	data=pages[offset];
	switch( offset )
	{
	case 1: /* DMA page register 2 */
		data = dma8237->chan[2].page >> 16;
		DMA_LOG(1,"DMA_page_2_r",("$%02x\n", data));
		break;
	case 2:    /* DMA page register 3 */
		data = dma8237->chan[3].page >> 16;
		DMA_LOG(1,"DMA_page_3_r",("$%02x\n", data));
		break;
	case 3:    /* DMA page register 1 */
		data = dma8237->chan[1].page >> 16;
		DMA_LOG(1,"DMA_page_1_r",("$%02x\n", data));
		break;
	case 7:    /* DMA page register 0 */
		data = dma8237->chan[0].page >> 16;
		DMA_LOG(1,"DMA_page_0_w",("$%02x\n", data));
		break;
	case 0xf:    /* DMA page register 0 */
		data = dma8237[1].chan[0].page >> 16;
		DMA_LOG(1,"DMA_page_4_w",("$%02x\n", data));
		break;
	case 0xb:    /* DMA page register 0 */
		data = dma8237[1].chan[1].page >> 16;
		DMA_LOG(1,"DMA_page_5_w",("$%02x\n", data));
		break;
	case 0x9:    /* DMA page register 0 */
		data = dma8237[1].chan[2].page >> 16;
		DMA_LOG(1,"DMA_page_6_w",("$%02x\n", data));
		break;
	case 0xa:    /* DMA page register 0 */
		data = dma8237[1].chan[3].page >> 16;
		DMA_LOG(1,"DMA_page_7_w",("$%02x\n", data));
		break;
    }
	logerror("at page read %g %06x\t%02x %02x\n", timer_get_time(), activecpu_get_pc(), offset, data);
	return data;
}

WRITE_HANDLER ( dma8237_0_w ) { dma8237_w(dma8237, offset, data); }
WRITE_HANDLER ( dma8237_1_w ) { dma8237_w(dma8237+1, offset, data); }
READ_HANDLER ( dma8237_0_r ) { return dma8237_r(dma8237, offset); }
READ_HANDLER ( dma8237_1_r ) { return dma8237_r(dma8237+1, offset); }

WRITE_HANDLER ( dma8237_at_0_w ) { dma8237_w(dma8237, offset>>1, data); }
WRITE_HANDLER ( dma8237_at_1_w ) { dma8237_w(dma8237+1, offset>>1, data); }
READ_HANDLER ( dma8237_at_0_r ) { return dma8237_r(dma8237, offset>>1); }
READ_HANDLER ( dma8237_at_1_r ) { return dma8237_r(dma8237+1, offset>>1); }

