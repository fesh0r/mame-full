/* direct memory access controller 8237
   used in complete pc series */

#include "driver.h"

#include "includes/dma8237.h"

#if 0
#define DMA_LOG(level, text, print) \
		if (level>0) { \
				logerror("%s\t", text); \
				logerror print; \
		}
#else
#define DMA_LOG(level, text, print)
#endif

DMA8237 dma8237[2]= { {0} };

static void dma8237_w(DMA8237 *this, offs_t offset, data8_t data)
{
	offset&=0xf;
	switch( offset )
	{
    case 0: case 2: case 4: case 6:
        DMA_LOG(1,"DMA_address_w",("chan #%d $%02x: ", offset>>1, data));
        if (this->msb)
            this->address[offset>>1] |= (data & 0xff) << 8;
        else
            this->address[offset>>1] = data & 0xff;
        DMA_LOG(1,"",("[$%04x]\n", this->address[offset>>1]));
        this->msb ^= 1;
        break;
    case 1: case 3: case 5: case 7:
        DMA_LOG(1,"DMA_count_w",("chan #%d $%02x: ", offset>>1, data));
        if (this->msb)
            this->count[offset>>1] |= (data & 0xff) << 8;
        else
            this->count[offset>>1] = data & 0xff;
        DMA_LOG(1,"",("[$%04x]\n", this->count[offset>>1]));
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
        this->operation[this->channel] = (data >> 2) & 3;
        this->direction[this->channel] = (data & 0x20) ? -1 : +1;
        this->transfer_mode[this->channel] = (data >> 6) & 3;
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
				data = (this->address[offset>>1] >> 8) & 0xff;
			else
				data = this->address[offset>>1] & 0xff;
			DMA_LOG(1,"DMA_address_r",("chan #%d $%02x ($%04x)\n", offset>>1, data, this->address[offset>>1]));
			this->msb ^= 1;
			break;

		case 1: case 3: case 5: case 7:
			if (this->msb)
				data = (this->count[offset>>1] >> 8) & 0xff;
			else
				data = this->count[offset>>1] & 0xff;
			DMA_LOG(1,"DMA_count_r",("chan #%d $%02x ($%04x)\n", offset>>1, data, this->count[offset>>1]));
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

/* in an at SN74LS612N, full 16 register memory mapper,
   so 0x80-0x8f read and writeable */
static void dma8237_page_w(DMA8237 *this, offs_t offset, data8_t data)
{
	offset&=7;
	this->pagereg.data[offset]=data;
	switch( offset )
	{
	case 0:
		logerror("postcode %.2x\n",data);
		break;
	case 1: /* DMA page register 2 */
		DMA_LOG(1,"DMA_page_2_w",("$%02x\n", data));
		this->page[2] = (data << 16) & 0xff0000;
		break;
	case 2:    /* DMA page register 3 */
		DMA_LOG(1,"DMA_page_3_w",("$%02x\n", data));
		this->page[3] = (data << 16) & 0xff0000;
		break;
	case 3:    /* DMA page register 1 */
		DMA_LOG(1,"DMA_page_1_w",( "$%02x\n", data));
		this->page[1] = (data << 16) & 0xff0000;
		break;
	case 7:    /* DMA page register 0 */
		DMA_LOG(1,"DMA_page_0_w",("$%02x\n", data));
		this->page[0] = (data << 16) & 0xff0000;
		break;
    }
}

static int dma8237_page_r(DMA8237 *this, offs_t offset)
{
	int data = 0xff;
	offset&=7;
	data=this->pagereg.data[offset];
	switch( offset )
	{
	case 1: /* DMA page register 2 */
		data = this->page[2] >> 16;
		DMA_LOG(1,"DMA_page_2_r",("$%02x\n", data));
		break;
	case 2:    /* DMA page register 3 */
		data = this->page[3] >> 16;
		DMA_LOG(1,"DMA_page_3_r",("$%02x\n", data));
		break;
	case 3:    /* DMA page register 1 */
		data = this->page[1] >> 16;
		DMA_LOG(1,"DMA_page_1_r",("$%02x\n", data));
		break;
	case 7:    /* DMA page register 0 */
		data = this->page[0] >> 16;
		DMA_LOG(1,"DMA_page_0_w",("$%02x\n", data));
		break;
    }
	return data;
}

WRITE_HANDLER ( dma8237_0_w )
{
	dma8237_w(dma8237, offset, data);
}

WRITE_HANDLER ( dma8237_1_w )
{
	dma8237_w(dma8237+1, offset, data);
}

READ_HANDLER ( dma8237_0_r )
{
	return dma8237_r(dma8237, offset);
}

READ_HANDLER ( dma8237_1_r )
{
	return dma8237_r(dma8237+1, offset);
}

WRITE_HANDLER ( dma8237_at_0_w )
{
	dma8237_w(dma8237, offset>>1, data);
}

WRITE_HANDLER ( dma8237_at_1_w )
{
	dma8237_w(dma8237+1, offset>>1, data);
}

READ_HANDLER ( dma8237_at_0_r )
{
	return dma8237_r(dma8237, offset>>1);
}

READ_HANDLER ( dma8237_at_1_r )
{
	return dma8237_r(dma8237+1, offset>>1);
}

WRITE_HANDLER ( dma8237_0_page_w )
{
	dma8237_page_w(dma8237, offset, data);
}

WRITE_HANDLER ( dma8237_1_page_w )
{
	dma8237_page_w(dma8237+1, offset, data);
}

READ_HANDLER ( dma8237_0_page_r )
{
	return dma8237_page_r(dma8237, offset);
}

READ_HANDLER ( dma8237_1_page_r )
{
	return dma8237_page_r(dma8237+1, offset);
}
