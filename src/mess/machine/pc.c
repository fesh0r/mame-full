/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

	TODO:
	clean up (maybe split) the different pieces of hardware
	PIC, PIT, DMA... add support for LPT, COM (almost done)
	emulation of a serial mouse on a COM port (almost done)
	support for Game Controller port at 0x0201
	support for XT harddisk (under the way, see machine/pc_hdc.c)
	whatever 'hardware' comes to mind,
	maybe SoundBlaster? EGA? VGA?

***************************************************************************/
#include "mess/machine/pc.h"

#include "mess/vidhrdw/vga.h"

static void (*pc_blink_textcolors)(int on);

int pc_framecnt = 0;
int pc_blink = 0;

UINT8 pc_port[0x400];

/* keyboard */
static UINT8 kb_queue[256];
static UINT8 kb_head = 0;
static UINT8 kb_tail = 0;
static UINT8 make[128] = {0, };
static UINT8 kb_delay = 60;   /* 240/60 -> 0,25s */
static UINT8 kb_repeat = 8;   /* 240/ 8 -> 30/s */
static int keyboard_numlock=0;
static const struct {
        char* pressed;
        char* released;
} keyboard_mf2_code[2/*numlock off, on*/][0x10]={ 
        {
                { "\xe0\x12", "\xe0\x92" },
                { "\xe0\x13", "\xe0\x93" },
                { "\xe0\x35", "\xe0\xb5" },
                { "\xe0\x37", "\xe0\xb7" },
                { "\xe0\x38", "\xe0\xb8" },
                { "\xe0\x47", "\xe0\xc7" },
                { "\xe0\x48", "\xe0\xc8" },
                { "\xe0\x49", "\xe0\xc9" },
                { "\xe0\x4b", "\xe0\xcb" },
                { "\xe0\x4d", "\xe0\xcd" },
                { "\xe0\x4f", "\xe0\xcf" },
                { "\xe0\x50", "\xe0\xd0" },
                { "\xe0\x51", "\xe0\xd1" },
                { "\xe0\x52", "\xe0\xd2" },
                { "\xe0\x53", "\xe0\xd3" },
                { "\xe1\x1d\x45\xe1\x9d\xc5", "" }
        },{
                { "\xe0\x12", "\xe0\x92" },
                { "\xe0\x13", "\xe0\x93" },
                { "\xe0\x35", "\xe0\xb5" },
                { "\xe0\x37", "\xe0\xb7" },
                { "\xe0\x38", "\xe0\xb8" },
                { "\xe0\x2a\xe0\x47", "\xe0\xc7\xe0\xaa" },
                { "\xe0\x2a\xe0\x48", "\xe0\xc8\xe0\xaa" },
                { "\xe0\x2a\xe0\x49", "\xe0\xc9\xe0\xaa" },
                { "\xe0\x2a\xe0\x4b", "\xe0\xcb\xe0\xaa" },
                { "\xe0\x2a\xe0\x4d", "\xe0\xcd\xe0\xaa" },
                { "\xe0\x2a\xe0\x4f", "\xe0\xcf\xe0\xaa" },
                { "\xe0\x2a\xe0\x50", "\xe0\xd0\xe0\xaa" },
                { "\xe0\x2a\xe0\x51", "\xe0\xd1\xe0\xaa" },
                { "\xe0\x2a\xe0\x52", "\xe0\xd2\xe0\xaa" },
                { "\xe0\x2a\xe0\x53", "\xe0\xd3\xe0\xaa" },
                { "\xe0\x2a\xe1\x1d\x45\xe1\x9d\xc5", "" }
        }
};


/* mouse */
static UINT8 m_queue[256];
static UINT8 m_head = 0, m_tail = 0, mb = 0;
static void *mouse_timer = NULL;

static void pc_mouse_scan(int n);
static void pc_mouse_poll(int n);

void init_pc(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
}

void init_pc_vga(void)
{
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;

#if 0
        for (chksum=0, i=0;i<0x8000;i++) {
                chksum+=memory[i];
        }
        printf("checksum %.2x\n",chksum);
#endif

        vga_init(input_port_15_r);
        pc_blink_textcolors = NULL;
}

int pc_floppy_init(int id)
{
	static int common_length_spt_heads[][3] = {
    { 8*1*40*512,  8, 1},   /* 5 1/4 inch double density single sided */
    { 8*2*40*512,  8, 2},   /* 5 1/4 inch double density */
    { 9*1*40*512,  9, 1},   /* 5 1/4 inch double density single sided */
    { 9*2*40*512,  9, 2},   /* 5 1/4 inch double density */
    { 9*2*80*512,  9, 2},   /* 80 tracks 5 1/4 inch drives rare in PCs */
    { 9*2*80*512,  9, 2},   /* 3 1/2 inch double density */
    {15*2*80*512, 15, 2},   /* 5 1/4 inch high density (or japanese 3 1/2 inch high density) */
    {18*2*80*512, 18, 2},   /* 3 1/2 inch high density */
    {36*2*80*512, 36, 2}};  /* 3 1/2 inch enhanced density */
	int i;

    pc_fdc_file[id] = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	/* find the sectors/track and bytes/sector values in the boot sector */
	if( pc_fdc_file[id] )
	{
		int length;

		/* tracks pre sector recognition with image size
		   works only 512 byte sectors! and 40 or 80 tracks*/
		pc_fdc_scl[id]=2;
		pc_fdc_heads[id]=2;
		length=osd_fsize(pc_fdc_file[id]);
		for( i = sizeof(common_length_spt_heads)/sizeof(common_length_spt_heads[0])-1; i >= 0; --i )
		{
			if( length == common_length_spt_heads[i][0] )
			{
				pc_fdc_spt[id] = common_length_spt_heads[i][1];
				pc_fdc_heads[id] = common_length_spt_heads[i][2];
				break;
			}
		}
		if( i < 0 )
		{
			/*
			 * get info from boot sector.
			 * not correct on all disks
			 */
			osd_fseek(pc_fdc_file[id], 0x0c, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_scl[id], 1);
			osd_fseek(pc_fdc_file[id], 0x018, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_spt[id], 1);
			osd_fseek(pc_fdc_file[id], 0x01a, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_heads[id], 1);
		}
	}
	return INIT_OK;
}

void pc_floppy_exit(int id)
{
	if( pc_fdc_file[id] )
		osd_fclose(pc_fdc_file[id]);
	pc_fdc_file[id] = NULL;
}

int pc_harddisk_init(int id)
{
	pc_hdc_file[id] = image_fopen(IO_HARDDISK, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	return INIT_OK;
}

void pc_harddisk_exit(int id)
{
	if( pc_hdc_file[id] )
		osd_fclose(pc_hdc_file[id]);
    pc_hdc_file[id] = NULL;
}

void pc_mda_init_machine(void)
{
	int i;

	keyboard_numlock=0;
	pc_blink_textcolors = pc_cga_blink_textcolors;

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < Machine->gfx[0]->height; y++ )
				Machine->gfx[0]->gfxdata[(i * Machine->gfx[0]->height + y) * Machine->gfx[0]->width + 8] = 0;
		}
	}
}

void pc_cga_init_machine(void)
{
	keyboard_numlock=0;
	pc_blink_textcolors = pc_cga_blink_textcolors;
}

void pc_vga_init_machine(void)
{
        keyboard_numlock=0;
}

void pc_t1t_init_machine(void)
{
	keyboard_numlock=0;
	pc_blink_textcolors = pc_t1t_blink_textcolors;
}

/*************************************
 *
 *		Port handlers.
 *
 *************************************/

/*************************************************************************
 *
 *		DMA
 *		direct memory access
 *
 *************************************************************************/
UINT8 pc_DMA_msb = 0;
UINT8 pc_DMA_temp = 0;
int pc_DMA_address[4] = {0,0,0,0};
int pc_DMA_count[4] = {0,0,0,0};
int pc_DMA_page[4] = {0,0,0,0};
UINT8 pc_DMA_transfer_mode[4] = {0,0,0,0};
INT8 pc_DMA_direction[4] = {0,0,0,0};
UINT8 pc_DMA_operation[4] = {0,0,0,0};
UINT8 pc_DMA_status = 0x00;
UINT8 pc_DMA_mask = 0x00;
UINT8 pc_DMA_command = 0x00;
UINT8 pc_DMA_DACK_hi = 0;
UINT8 pc_DMA_DREQ_hi = 0;
UINT8 pc_DMA_write_extended = 0;
UINT8 pc_DMA_rotate_priority = 0;
UINT8 pc_DMA_compressed_timing = 0;
UINT8 pc_DMA_enable_controller = 0;
UINT8 pc_DMA_channel = 0;

WRITE_HANDLER ( pc_DMA_w )
{
	switch( offset )
	{
    case 0: case 2: case 4: case 6:
        DMA_LOG(1,"DMA_address_w",(errorlog,"chan #%d $%02x: ", offset>>1, data));
        if (pc_DMA_msb)
            pc_DMA_address[offset>>1] |= (data & 0xff) << 8;
        else
            pc_DMA_address[offset>>1] = data & 0xff;
        DMA_LOG(1,0,(errorlog,"[$%04x]\n", pc_DMA_address[offset>>1]));
        pc_DMA_msb ^= 1;
        break;
    case 1: case 3: case 5: case 7:
        DMA_LOG(1,"DMA_count_w",(errorlog,"chan #%d $%02x: ", offset>>1, data));
        if (pc_DMA_msb)
            pc_DMA_count[offset>>1] |= (data & 0xff) << 8;
        else
            pc_DMA_count[offset>>1] = data & 0xff;
        DMA_LOG(1,0,(errorlog,"[$%04x]\n", pc_DMA_count[offset>>1]));
        pc_DMA_msb ^= 1;
        break;
    case 8:    /* DMA command register */
        pc_DMA_command = data;
        pc_DMA_DACK_hi = (pc_DMA_command >> 7) & 1;
        pc_DMA_DREQ_hi = (pc_DMA_command >> 6) & 1;
        pc_DMA_write_extended = (pc_DMA_command >> 5) & 1;
        pc_DMA_rotate_priority = (pc_DMA_command >> 4) & 1;
        pc_DMA_compressed_timing = (pc_DMA_command >> 3) & 1;
        pc_DMA_enable_controller = (pc_DMA_command >> 2) & 1;
        pc_DMA_channel = pc_DMA_command & 3;
        DMA_LOG(1,"DMA_command_w",(errorlog,"$%02x: chan #%d, enable %d, CT %d, RP %d, WE %d, DREQ %d, DACK %d\n", data,
            pc_DMA_channel, pc_DMA_enable_controller, pc_DMA_compressed_timing, pc_DMA_rotate_priority, pc_DMA_write_extended, pc_DMA_DREQ_hi, pc_DMA_DACK_hi));
        break;
    case 9:    /* DMA write request register */
        pc_DMA_channel = data & 3;
        if (data & 0x04) {
            DMA_LOG(1,"DMA_request_w",(errorlog,"$%02x: set chan #%d\n", data, pc_DMA_channel));
            /* set the DMA request bit for the given channel */
            pc_DMA_status |= 0x10 << pc_DMA_channel;
        } else {
            DMA_LOG(1,"DMA_request_w",(errorlog,"$%02x: clear chan #%d\n", data, pc_DMA_channel));
            /* clear the DMA request bit for the given channel */
            pc_DMA_status &= ~(0x11 << pc_DMA_channel);
        }
        break;
    case 10:    /* DMA mask register */
        pc_DMA_channel = data & 3;
        if (data & 0x04) {
            DMA_LOG(1,"DMA_mask_w",(errorlog,"$%02x: set chan #%d\n", data, pc_DMA_channel));
            /* set the DMA request bit for the given channel */
            pc_DMA_mask |= 0x11 << pc_DMA_channel;
        } else {
            DMA_LOG(1,"DMA_mask_w",(errorlog,"$%02x: clear chan #%d\n", data, pc_DMA_channel));
            /* clear the DMA request bit for the given channel */
            pc_DMA_mask &= ~(0x11 << pc_DMA_channel);
        }
        break;
    case 11:    /* DMA mode register */
        pc_DMA_channel = data & 3;
        pc_DMA_operation[pc_DMA_channel] = (data >> 2) & 3;
        pc_DMA_direction[pc_DMA_channel] = (data & 0x20) ? -1 : +1;
        pc_DMA_transfer_mode[pc_DMA_channel] = (data >> 6) & 3;
        DMA_LOG(1,"DMA_mode_w",(errorlog,"$%02x: chan #%d, oper %d, dir %d, mode %d\n", data, data&3, (data>>2)&3, (data>>5)&1, (data>>6)&3 ));
        break;
    case 12:    /* DMA clear byte pointer flip-flop */
        DMA_LOG(1,"DMA_clear_ff_w",(errorlog,"$%02x\n", data));
        pc_DMA_temp = data;
        pc_DMA_msb = 0;
        break;
    case 13:    /* DMA master clear */
        DMA_LOG(1,"DMA_master_w",(errorlog,"$%02x\n", data));
        pc_DMA_msb = 0;
        break;
    case 14:    /* DMA clear mask register */
        pc_DMA_mask &= ~data;
        DMA_LOG(1,"DMA_mask_clr_w",(errorlog,"$%02x -> mask $%02x\n", data, pc_DMA_mask));
        break;
    case 15:    /* DMA write mask register */
        pc_DMA_mask |= data;
        DMA_LOG(1,"DMA_mask_clr_w",(errorlog,"$%02x -> mask $%02x\n", data, pc_DMA_mask));
        break;
    }
}

WRITE_HANDLER ( pc_DMA_page_w )
{
	switch( offset )
	{
		case 1: /* DMA page register 2 */
			DMA_LOG(1,"DMA_page_2_w",(errorlog, "$%02x\n", data));
			pc_DMA_page[2] = (data << 16) & 0xff0000;
			break;
		case 2:    /* DMA page register 3 */
			DMA_LOG(1,"DMA_page_3_w",(errorlog, "$%02x\n", data));
			pc_DMA_page[3] = (data << 16) & 0xff0000;
			break;
		case 3:    /* DMA page register 1 */
			DMA_LOG(1,"DMA_page_1_w",(errorlog, "$%02x\n", data));
			pc_DMA_page[1] = (data << 16) & 0xff0000;
			break;
		case 7:    /* DMA page register 0 */
			DMA_LOG(1,"DMA_page_0_w",(errorlog, "$%02x\n", data));
			pc_DMA_page[0] = (data << 16) & 0xff0000;
			break;
    }
}

READ_HANDLER ( pc_DMA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			if (pc_DMA_msb)
				data = (pc_DMA_address[offset>>1] >> 8) & 0xff;
			else
				data = pc_DMA_address[offset>>1] & 0xff;
			DMA_LOG(1,"DMA_address_r",(errorlog,"chan #%d $%02x ($%04x)\n", offset>>1, data, pc_DMA_address[offset>>1]));
			pc_DMA_msb ^= 1;
			break;

		case 1: case 3: case 5: case 7:
			if (pc_DMA_msb)
				data = (pc_DMA_count[offset>>1] >> 8) & 0xff;
			else
				data = pc_DMA_count[offset>>1] & 0xff;
			DMA_LOG(1,"DMA_count_r",(errorlog,"chan #%d $%02x ($%04x)\n", offset>>1, data, pc_DMA_count[offset>>1]));
			pc_DMA_msb ^= 1;
			break;

		case 8: /* DMA status register */
			data = pc_DMA_status;
			DMA_LOG(1,"DMA_status_r",(errorlog,"$%02x\n", data));
			break;

		case 9: /* DMA write request register */
			break;

		case 10: /* DMA mask register */
			data = pc_DMA_mask;
			DMA_LOG(1,"DMA_mask_r",(errorlog,"$%02x\n", data));
			break;

		case 11: /* DMA mode register */
			break;

		case 12: /* DMA clear byte pointer flip-flop */
			break;

		case 13: /* DMA master clear */
			data = pc_DMA_temp;
			DMA_LOG(1,"DMA_temp_r",(errorlog,"$%02x\n", data));
			break;

		case 14: /* DMA clear mask register */
			break;

		case 15: /* DMA write mask register */
			break;
	}
	return data;
}

READ_HANDLER ( pc_DMA_page_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 1: /* DMA page register 2 */
			data = pc_DMA_page[2] >> 16;
			DMA_LOG(1,"DMA_page_2_r",(errorlog, "$%02x\n", data));
			break;
		case 2:    /* DMA page register 3 */
			data = pc_DMA_page[3] >> 16;
			DMA_LOG(1,"DMA_page_3_r",(errorlog, "$%02x\n", data));
			break;
		case 3:    /* DMA page register 1 */
			data = pc_DMA_page[1] >> 16;
			DMA_LOG(1,"DMA_page_1_r",(errorlog, "$%02x\n", data));
			break;
		case 7:    /* DMA page register 0 */
			data = pc_DMA_page[0] >> 16;
			DMA_LOG(1,"DMA_page_0_w",(errorlog, "$%02x\n", data));
			break;
    }
	return data;
}

/**************************************************************************
 *
 *		PIC
 *		programmable interrupt controller
 *
 **************************************************************************/
static UINT8 PIC_icw2 = 0;
static UINT8 PIC_icw3 = 0;
static UINT8 PIC_icw4 = 0;

static UINT8 PIC_special = 0;
static UINT8 PIC_input = 0x00;

static UINT8 PIC_level_trig_mode = 0;
static UINT8 PIC_vector_size = 0;
static UINT8 PIC_cascade = 0;
static UINT8 PIC_base = 0x00;
static UINT8 PIC_slave = 0x00;

static UINT8 PIC_nested = 0;
static UINT8 PIC_mode = 0;
static UINT8 PIC_auto_eoi = 0;
static UINT8 PIC_x86 = 0;

static UINT8 PIC_enable = 0xff;
static UINT8 PIC_in_service = 0x00;
static UINT8 PIC_pending = 0x00;
static UINT8 PIC_prio = 0;


void pc_PIC_issue_irq(int irq)
{
	UINT8 mask = 1 << irq;
	PIC_LOG(1,"PIC_issue_irq",(errorlog,"IRQ%d: ", irq));

	/* PIC not initialized? */
	if( PIC_icw2 || PIC_icw3 || PIC_icw4 )
	{
		PIC_LOG(1,0,(errorlog, "PIC not initialized!\n"));
        return;
    }

    /* can't we handle it? */
	if( irq < 0 || irq > 7 )
	{
		PIC_LOG(1,0,(errorlog, "out of range!\n"));
		return;
	}

	/* interrupt not enabled? */
	if( PIC_enable & mask )
	{
		PIC_LOG(1,0,(errorlog,"is not enabled\n"));
/*		PIC_pending &= ~mask; */
/*		PIC_in_service &= ~mask; */
        return;
	}

	/* same interrupt not yet acknowledged ? */
	if( PIC_in_service & mask )
	{
		PIC_LOG(1,0,(errorlog,"is already in service\n"));
		/* save request mask for later */
/* HACK! */
		PIC_in_service &= ~mask;
        PIC_pending |= mask;
        return;
	}

    /* higher priority interrupt in service? */
	if( PIC_in_service & (mask-1) )
	{
		PIC_LOG(1,0,(errorlog,"is lower priority\n"));
		/* save request mask for later */
		PIC_pending |= mask;
        return;
	}

    /* remove from the requested INTs */
	PIC_pending &= ~mask;

    /* mask interrupt until acknowledged */
	PIC_in_service |= mask;

    irq += PIC_base;
	PIC_LOG(1,0,(errorlog,"INT %02X\n", irq));
	cpu_irq_line_vector_w(0,0,irq);
	cpu_set_irq_line(0,0,HOLD_LINE);
}

int pc_PIC_irq_pending(int irq)
{
	UINT8 mask = 1 << irq;
	return (PIC_pending & mask) ? 1 : 0;
}

WRITE_HANDLER ( pc_PIC_w )
{
	switch( offset )
	{
    case 0:    /* PIC acknowledge IRQ */
		if( data & 0x10 )	/* write ICW1 ? */
		{
            PIC_icw2 = 1;
            PIC_icw3 = 1;
            PIC_level_trig_mode = (data >> 3) & 1;
            PIC_vector_size = (data >> 2) & 1;
			PIC_cascade = ((data >> 1) & 1) ^ 1;
			if( PIC_cascade == 0 )
				PIC_icw3 = 0;
            PIC_icw4 = data & 1;
            PIC_LOG(1,"PIC_ack_w",(errorlog, "$%02x: ICW1, icw4 %d, cascade %d, vec size %d, ltim %d\n",
                data, PIC_icw4, PIC_cascade, PIC_vector_size, PIC_level_trig_mode));
		}
		else
		if (data & 0x08)
		{
            PIC_LOG(1,"PIC_ack_w",(errorlog, "$%02x: OCW3", data));
			switch (data & 0x60)
			{
                case 0x00:
                case 0x20:
                    break;
                case 0x40:
                    PIC_LOG(1,0,(errorlog, ", reset special mask"));
                    break;
                case 0x60:
                    PIC_LOG(1,0,(errorlog, ", set special mask"));
                    break;
            }
			switch (data & 0x03)
			{
                case 0x00:
				case 0x01:
					PIC_LOG(1,0,(errorlog, ", no operation"));
                    break;
                case 0x02:
                    PIC_LOG(1,0,(errorlog, ", read request register"));
                    PIC_special = 1;
					PIC_input = PIC_pending;
                    break;
                case 0x03:
                    PIC_LOG(1,0,(errorlog, ", read in-service register"));
                    PIC_special = 1;
					PIC_input = PIC_in_service & ~PIC_enable;
                    break;
            }
            PIC_LOG(1,0,(errorlog, "\n"));
		}
		else
		{
			int n = data & 7;
            UINT8 mask = 1 << n;
            PIC_LOG(1,"PIC_ack_w",(errorlog, "$%02x: OCW2", data));
			switch (data & 0xe0)
			{
                case 0x00:
                    PIC_LOG(1,0,(errorlog, " rotate auto EOI clear\n"));
					PIC_prio = 0;
                    break;
                case 0x20:
                    PIC_LOG(1,0,(errorlog, " nonspecific EOI\n"));
					for( n = 0, mask = 1<<PIC_prio; n < 8; n++, mask = (mask<<1) | (mask>>7) )
					{
						if( PIC_in_service & mask )
						{
                            PIC_in_service &= ~mask;
							PIC_pending &= ~mask;
                            break;
                        }
                    }
                    break;
                case 0x40:
                    PIC_LOG(1,0,(errorlog, " OCW2 NOP\n"));
                    break;
                case 0x60:
                    PIC_LOG(1,0,(errorlog, " OCW2 specific EOI%d\n", n));
					if( PIC_in_service & mask )
                    {
						PIC_in_service &= ~mask;
						PIC_pending &= ~mask;
					}
                    break;
                case 0x80:
					PIC_LOG(1,0,(errorlog, " OCW2 rotate auto EOI set\n"));
					PIC_prio = ++PIC_prio & 7;
                    break;
                case 0xa0:
                    PIC_LOG(1,0,(errorlog, " OCW2 rotate on nonspecific EOI\n"));
					for( n = 0, mask = 1<<PIC_prio; n < 8; n++, mask = (mask<<1) | (mask>>7) )
					{
						if( PIC_in_service & mask )
						{
                            PIC_in_service &= ~mask;
							PIC_pending &= ~mask;
							PIC_prio = ++PIC_prio & 7;
                            break;
                        }
                    }
					break;
                case 0xc0:
                    PIC_LOG(1,0,(errorlog, " OCW2 set priority\n"));
					PIC_prio = n & 7;
                    break;
                case 0xe0:
                    PIC_LOG(1,0,(errorlog, " OCW2 rotate on specific EOI%d\n", n));
					if( PIC_in_service & mask )
					{
						PIC_in_service &= ~mask;
						PIC_pending &= ~mask;
						PIC_prio = ++PIC_prio & 7;
					}
                    break;
            }
        }
        break;
    case 1:    /* PIC ICW2,3,4 or OCW1 */
		if( PIC_icw2 )
		{
            PIC_base = data & 0xf8;
            PIC_LOG(1,"PIC_enable_w",(errorlog, "$%02x: ICW2 (base)\n", PIC_base));
            PIC_icw2 = 0;
		}
		else
		if( PIC_icw3 )
		{
            PIC_slave = data;
            PIC_LOG(1,"PIC_enable_w",(errorlog, "$%02x: ICW3 (slave)\n", PIC_slave));
            PIC_icw3 = 0;
		}
		else
		if( PIC_icw4 )
		{
            PIC_nested = (data >> 4) & 1;
            PIC_mode = (data >> 2) & 3;
            PIC_auto_eoi = (data >> 1) & 1;
            PIC_x86 = data & 1;
            PIC_LOG(1,"PIC_enable_w",(errorlog, "$%02x: ICW4 x86 mode %d, auto EOI %d, mode %d, nested %d\n",
                data, PIC_x86, PIC_auto_eoi, PIC_mode, PIC_nested));
            PIC_icw4 = 0;
		}
		else
		{
            PIC_LOG(1,"PIC_enable_w",(errorlog, "$%02x: OCW1 enable\n", data));
            PIC_enable = data;
			PIC_in_service &= data;
			PIC_pending &= data;
        }
        break;
    }
	if (PIC_pending & 0x01) pc_PIC_issue_irq(0);
    if (PIC_pending & 0x02) pc_PIC_issue_irq(1);
    if (PIC_pending & 0x04) pc_PIC_issue_irq(2);
    if (PIC_pending & 0x08) pc_PIC_issue_irq(3);
    if (PIC_pending & 0x10) pc_PIC_issue_irq(4);
    if (PIC_pending & 0x20) pc_PIC_issue_irq(5);
    if (PIC_pending & 0x40) pc_PIC_issue_irq(6);
    if (PIC_pending & 0x80) pc_PIC_issue_irq(7);
}

READ_HANDLER ( pc_PIC_r )
{
	int data = 0xff;

	switch( offset )
	{
	case 0: /* PIC acknowledge IRQ */
        if (PIC_special) {
            PIC_special = 0;
            data = PIC_input;
            PIC_LOG(1,"PIC_ack_r",(errorlog, "$%02x read special\n", data));
        } else {
            PIC_LOG(1,"PIC_ack_r",(errorlog, "$%02x\n", data));
        }
        break;

	case 1: /* PIC mask register */
        data = PIC_enable;
        PIC_LOG(1,"PIC_enable_r",(errorlog, "$%02x\n", data));
        break;
	}
	return data;
}

/*************************************************************************
 *
 *		PIT
 *		programmable interval timer
 *
 *************************************************************************/
static UINT8 PIT_index = 0;
static UINT8 PIT_access = 0;
static UINT8 PIT_mode = 0;
static UINT8 PIT_bcd = 0;
static UINT8 PIT_msb = 0;
static void * PIT_timer = 0;
static double PIT_time_access[3] = {0.0, };
int PIT_clock[3] = {0, };
int PIT_latch[3] = {0, };

static void pc_PIT_timer_pulse(void)
{
	double rate = 0.0;

	if( PIT_timer )
		timer_remove( PIT_timer );
	PIT_timer = NULL;

	if (PIT_clock[0])
	{
		/* sanity check: do not really run a very high speed timer */
		if( PIT_clock[0] > 128 )
			rate = PIT_clock[0] / 1193180.0;
	} else rate = 65536 / 1193180.0;

	if (rate > 0.0)
		PIT_timer = timer_pulse(rate, 0, pc_PIC_issue_irq);
}

WRITE_HANDLER ( pc_PIT_w )
{
	switch( offset )
	{
    case 0: case 1: case 2:
        PIT_LOG(1,"PIT_counter_w",(errorlog, "cntr#%d $%02x: ", offset, data));
		switch (PIT_access)
		{
            case 0: /* counter latch command */
				PIT_LOG(1,0,(errorlog, "*latch command* "));
				PIT_msb ^= 1;
				if( !PIT_msb )
                    PIT_access = 3;
                break;
            case 1: /* read/write counter bits 0-7 only */
				PIT_LOG(1,0,(errorlog, "LSB only "));
                PIT_clock[offset] = data & 0xff;
                break;
            case 2: /* read/write counter bits 8-15 only */
				PIT_LOG(1,0,(errorlog, "MSB only "));
                PIT_clock[offset] = (data & 0xff) << 8;
                break;
            case 3: /* read/write bits 0-7 first, then 8-15 */
				if (PIT_msb)
				{
                    PIT_clock[offset] = (PIT_clock[offset] & 0x00ff) | ((data & 0xff) << 8);
					PIT_LOG(1,0,(errorlog, "MSB "));
				}
				else
				{
                    PIT_clock[offset] = (PIT_clock[offset] & 0xff00) | (data & 0xff);
					PIT_LOG(1,0,(errorlog, "LSB "));
				}
                PIT_msb ^= 1;
                break;
        }
        PIT_time_access[offset] = timer_get_time();
		switch( offset )
		{
            case 0:
                PIT_LOG(1,0,(errorlog, "sys ticks $%04x\n", PIT_clock[0]));
				pc_PIT_timer_pulse();
                break;
            case 1:
                PIT_LOG(1,0,(errorlog, "RAM refresh $%04x\n", PIT_clock[1]));
                /* DRAM refresh timer: ignored */
                break;
            case 2:
                PIT_LOG(1,0,(errorlog, "tone freq $%04x = %d Hz\n", PIT_clock[offset], (PIT_clock[2]) ? 1193180 / PIT_clock[2] : 1193810 / 65536));
				/* frequency updated in pc_sh_speaker */
				switch( pc_port[0x61] & 3 )
				{
					case 0: pc_sh_speaker(0); break;
					case 1: pc_sh_speaker(1); break;
					case 2: pc_sh_speaker(1); break;
					case 3: pc_sh_speaker(2); break;
				}
                break;
        }
        break;

    case 3: /* PIT mode port */
        PIT_index = (data >> 6) & 3;
        PIT_access = (data >> 4) & 3;
        PIT_mode = (data >> 1) & 7;
        PIT_bcd = data & 1;
		PIT_msb = (PIT_access == 2) ? 1 : 0;
        PIT_LOG(1,"PIT_mode_w",(errorlog, "$%02x: idx %d, access %d, mode %d, BCD %d\n", data, PIT_index, PIT_access, PIT_mode, PIT_bcd));
		if (PIT_access == 0)
		{
            int count = PIT_clock[PIT_index] ? PIT_clock[PIT_index] : 65536;
            PIT_latch[PIT_index] = count -
                ((int)(1193180 * (timer_get_time() - PIT_time_access[PIT_index]))) % count;
            PIT_LOG(1,"PIT latch value",(errorlog, "#%d $%04x\n", PIT_index, PIT_latch[PIT_index]));
        }
        break;
	}
}

READ_HANDLER ( pc_PIT_r )
{
	int data = 0xff;
	switch( offset )
	{
	case 0: case 1: case 2:
		switch (PIT_access)
		{
            case 0: /* counter latch command */
				if( PIT_msb )
				{
                    data = (PIT_latch[offset&3] >> 8) & 0xff;
                    PIT_LOG(1,"PIT_counter_r",(errorlog, "latch#%d MSB $%02x\n", offset&3, data));
				}
				else
				{
                    data = PIT_latch[offset&3] & 0xff;
                    PIT_LOG(1,"PIT_counter_r",(errorlog, "latch#%d LSB $%02x\n", offset&3, data));
                }
                PIT_msb ^= 1;
				if( !PIT_msb )
                    PIT_access = 3; /* switch back to clock access */
                break;
            case 1: /* read/write counter bits 0-7 only */
                data = PIT_clock[offset&3] & 0xff;
                PIT_LOG(1,"PIT_counter_r",(errorlog, "cntr#%d LSB $%02x\n", offset&3, data));
                break;
            case 2: /* read/write counter bits 8-15 only */
                data = (PIT_clock[offset&3] >> 8) & 0xff;
                PIT_LOG(1,"PIT_counter_r",(errorlog, "cntr#%d MSB $%02x\n", offset&3, data));
                break;
            case 3: /* read/write bits 0-7 first, then 8-15 */
				if (PIT_msb)
				{
                    data = (PIT_clock[offset&3] >> 8) & 0xff;
                    PIT_LOG(1,"PIT_counter_r",(errorlog, "cntr#%d MSB $%02x\n", offset&3, data));
				}
				else
				{
                    data = PIT_clock[offset&3] & 0xff;
                    PIT_LOG(1,"PIT_counter_r",(errorlog, "cntr#%d LSB $%02x\n", offset&3, data));
                }
                PIT_msb ^= 1;
                break;
        }
        break;
	}
	return data;
}

/*************************************************************************
 *
 *		PIO
 *		parallel input output
 *
 *************************************************************************/
WRITE_HANDLER ( pc_PIO_w )
{
	switch( offset )
	{
		case 0: /* KB controller port A */
			PIO_LOG(1,"PIO_A_w",(errorlog, "$%02x\n", data));
			pc_port[0x60] = data;
			break;

		case 1: /* KB controller port B */
			PIO_LOG(1,"PIO_B_w",(errorlog, "$%02x\n", data));
			pc_port[0x61] = data;
			switch( data & 3 )
			{
				case 0: pc_sh_speaker(0); break;
				case 1: pc_sh_speaker(1); break;
				case 2: pc_sh_speaker(1); break;
				case 3: pc_sh_speaker(2); break;
			}
			break;

		case 2: /* KB controller port C */
			PIO_LOG(1,"PIO_C_w",(errorlog, "$%02x\n", data));
			pc_port[0x62] = data;
			break;

		case 3: /* KB controller control port */
			PIO_LOG(1,"PIO_control_w",(errorlog, "$%02x\n", data));
			pc_port[0x63] = data;
			break;
    }
}

READ_HANDLER ( pc_PIO_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: /* KB port A */
            data = pc_port[0x60];
            PIO_LOG(1,"PIO_A_r",(errorlog, "$%02x\n", data));
            break;
		case 1: /* KB port B */
			data = pc_port[0x61];
			PIO_LOG(1,"PIO_B_r",(errorlog, "$%02x\n", data));
			break;
		case 2: /* KB port C: equipment flags */
			if (pc_port[0x61] & 0x08)
			{
				/* read hi nibble of S2 */
				data = (input_port_1_r(0) >> 4) & 0x0f;
				PIO_LOG(1,"PIO_C_r (hi)",(errorlog, "$%02x\n", data));
			}
			else
			{
				/* read lo nibble of S2 */
				data = input_port_1_r(0) & 0x0f;
				PIO_LOG(1,"PIO_C_r (lo)",(errorlog, "$%02x\n", data));
			}
			break;
		case 3:    /* KB controller control port */
			data = pc_port[0x63];
			PIO_LOG(1,"PIO_control_r",(errorlog, "$%02x\n", data));
			break;
	}
	return data;
}

/*************************************************************************
 *
 *		LPT
 *		line printer
 *
 *************************************************************************/
static UINT8 LPT_data[3] = {0, };
static UINT8 LPT_status[3] = {0, };
static UINT8 LPT_control[3] = {0, };

static void pc_LPT_w(int n, int offset, int data)
{
	if ( !(input_port_2_r(0) & (0x08>>n)) ) return;
	switch( offset )
	{
		case 0:
			LPT_data[n] = data;
			LPT_LOG(1,"LPT_data_w",(errorlog,"LPT%d $%02x\n", n, data));
			break;
		case 1:
			LPT_LOG(1,"LPT_status_w",(errorlog,"LPT%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = data;
			LPT_LOG(1,"LPT_control_w",(errorlog,"%d $%02x\n", n, data));
			break;
    }
}

WRITE_HANDLER ( pc_LPT1_w ) { pc_LPT_w(0,offset,data); }
WRITE_HANDLER ( pc_LPT2_w ) { pc_LPT_w(1,offset,data); }
WRITE_HANDLER ( pc_LPT3_w ) { pc_LPT_w(2,offset,data); }

static int pc_LPT_r(int n, int offset)
{
    int data = 0xff;
	if ( !(input_port_2_r(0) & (0x08>>n)) ) return data;
	switch( offset )
	{
		case 0:
			data = LPT_data[n];
			LPT_LOG(1,"LPT_data_r",(errorlog, "LPT%d $%02x\n", n, data));
			break;
		case 1:
			/* set status 'out of paper', '*no* error', 'IRQ has *not* occured' */
			LPT_status[n] = 0x2c;
			data = LPT_status[n];
			LPT_LOG(1,"LPT_status_r",(errorlog, "%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = 0x0c;
			data = LPT_control[n];
			LPT_LOG(1,"LPT_control_r",(errorlog, "%d $%02x\n", n, data));
			break;
    }
	return data;
}
READ_HANDLER ( pc_LPT1_r) { return pc_LPT_r(0, offset); }
READ_HANDLER ( pc_LPT2_r) { return pc_LPT_r(1, offset); }
READ_HANDLER ( pc_LPT3_r) { return pc_LPT_r(2, offset); }

/*************************************************************************
 *
 *		COM
 *		serial communications
 *
 *************************************************************************/
static UINT8 COM_thr[4] = {0, };  /* 0 -W transmitter holding register */
static UINT8 COM_rbr[4] = {0, };  /* 0 R- receiver buffer register */
static UINT8 COM_ier[4] = {0, };  /* 1 RW interrupt enable register */
static UINT8 COM_dll[4] = {0, };  /* 0 RW divisor latch lsb (if DLAB = 1) */
static UINT8 COM_dlm[4] = {0, };  /* 1 RW divisor latch msb (if DLAB = 1) */
static UINT8 COM_iir[4] = {0, };  /* 2 R- interrupt identification register */
static UINT8 COM_lcr[4] = {0, };  /* 3 RW line control register (bit 7: DLAB) */
static UINT8 COM_mcr[4] = {0, };  /* 4 RW modem control register */
static UINT8 COM_lsr[4] = {0, };  /* 5 R- line status register */
static UINT8 COM_msr[4] = {0, };  /* 6 R- modem status register */
static UINT8 COM_scr[4] = {0, };  /* 7 RW scratch register */

static void pc_COM_w(int n, int idx, int data)
{
#if VERBOSE_COM
    static char P[8] = "NONENHNL";  /* names for parity select */
#endif
    int tmp;

	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		COM_LOG(1,"COM_w",(errorlog,"COM%d $%02x: disabled\n", n+1, data));
		return;
    }
	switch (idx)
	{
		case 0:
			if (COM_lcr[n] & 0x80)
			{
				COM_dll[n] = data;
				tmp = COM_dlm[n] * 256 + COM_dll[n];
				COM_LOG(1,"COM_dll_w",(errorlog,"COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?1843200/16/tmp:0));
			}
			else
			{
				COM_thr[n] = data;
				COM_LOG(2,"COM_thr_w",(errorlog,"COM%d $%02x\n", n+1, data));
            }
			break;
		case 1:
			if (COM_lcr[n] & 0x80)
			{
				COM_dlm[n] = data;
				tmp = COM_dlm[n] * 256 + COM_dll[n];
                COM_LOG(1,"COM_dlm_w",(errorlog,"COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?1843200/16/tmp:0));
			}
			else
			{
				COM_ier[n] = data;
				COM_LOG(2,"COM_ier_w",(errorlog,"COM%d $%02x: enable int on RX %d, THRE %d, RLS %d, MS %d\n",
					n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1));
			}
            break;
		case 2:
			COM_LOG(1,"COM_fcr_w",(errorlog,"COM%d $%02x (16550 only)\n", n+1, data));
            break;
		case 3:
			COM_lcr[n] = data;
			COM_LOG(1,"COM_lcr_w",(errorlog,"COM%d $%02x word length %d, stop bits %d, parity %c, break %d, DLAB %d\n",
				n+1, data, 5+(data&3), 1+((data>>2)&1), P[(data>>3)&7], (data>>6)&1, (data>>7)&1));
            break;
		case 4:
			COM_mcr[n] = data;
			COM_LOG(1,"COM_mcr_w",(errorlog,"COM%d $%02x DTR %d, RTS %d, OUT1 %d, OUT2 %d, loopback %d\n",
				n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1));
            break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			COM_scr[n] = data;
			COM_LOG(2,"COM_scr_w",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
	}
}

WRITE_HANDLER ( pc_COM1_w ) { pc_COM_w(0, offset, data); }
WRITE_HANDLER ( pc_COM2_w ) { pc_COM_w(1, offset, data); }
WRITE_HANDLER ( pc_COM3_w ) { pc_COM_w(2, offset, data); }
WRITE_HANDLER ( pc_COM4_w ) { pc_COM_w(3, offset, data); }

static int pc_COM_r(int n, int idx)
{
    int data = 0xff;

	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		COM_LOG(1,"COM_r",(errorlog,"COM%d $%02x: disabled\n", n+1, data));
		return data;
    }
	switch (idx)
	{
		case 0:
			if (COM_lcr[n] & 0x80)
			{
				data = COM_dll[n];
				COM_LOG(1,"COM_dll_r",(errorlog,"COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = COM_rbr[n];
				if( COM_lsr[n] & 0x01 )
				{
					COM_lsr[n] &= ~0x01;		/* clear data ready status */
					COM_LOG(2,"COM_rbr_r",(errorlog,"COM%d $%02x\n", n+1, data));
				}
            }
			break;
		case 1:
			if (COM_lcr[n] & 0x80)
			{
				data = COM_dlm[n];
				COM_LOG(1,"COM_dlm_r",(errorlog,"COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = COM_ier[n];
				COM_LOG(2,"COM_ier_r",(errorlog,"COM%d $%02x\n", n+1, data));
            }
            break;
		case 2:
			data = COM_iir[n];
			COM_iir[n] |= 1;
			COM_LOG(2,"COM_iir_r",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
		case 3:
			data = COM_lcr[n];
			COM_LOG(2,"COM_lcr_r",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
		case 4:
			data = COM_mcr[n];
			COM_LOG(2,"COM_mcr_r",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
		case 5:
			COM_lsr[n] |= 0x40; /* set TSRE */
			COM_lsr[n] |= 0x20; /* set THRE */
			data = COM_lsr[n];
			if( COM_lsr[n] & 0x1f )
			{
				COM_lsr[n] &= 0xe1; /* clear FE, PE and OE and BREAK bits */
				COM_LOG(2,"COM_lsr_r",(errorlog,"COM%d $%02x, DR %d, OE %d, PE %d, FE %d, BREAK %d, THRE %d, TSRE %d\n",
					n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1, (data>>5)&1, (data>>6)&1));
			}
            break;
		case 6:
			if (COM_mcr[n] & 0x10)	/* loopback test? */
			{
				data = COM_mcr[n] << 4;
				/* build delta values */
				COM_msr[n] = (COM_msr[n] ^ data) >> 4;
				COM_msr[n] |= data;
			}
			data = COM_msr[n];
			COM_msr[n] &= 0xf0; /* reset delta values */
			COM_LOG(2,"COM_msr_r",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
		case 7:
			data = COM_scr[n];
			COM_LOG(2,"COM_scr_r",(errorlog,"COM%d $%02x\n", n+1, data));
            break;
	}
	if( input_port_3_r(0) & (0x80>>n) )  /* mouse on this COM? */
        pc_mouse_poll(n);

    return data;
}
READ_HANDLER ( pc_COM1_r ) { return pc_COM_r(0, offset); }
READ_HANDLER ( pc_COM2_r ) { return pc_COM_r(1, offset); }
READ_HANDLER ( pc_COM3_r ) { return pc_COM_r(2, offset); }
READ_HANDLER ( pc_COM4_r ) { return pc_COM_r(3, offset); }


/*************************************************************************
 *
 *		JOY
 *		joystick port
 *
 *************************************************************************/

static double JOY_time = 0.0;

WRITE_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}

READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",(errorlog,"$%02x, time > 0.01s\n", data));
	}
	else
	{
		delta = 256 * 1000 * (new_time - JOY_time);
		if (input_port_16_r(0) < delta) data &= ~0x01;
		if (input_port_17_r(0) < delta) data &= ~0x02;
		if (input_port_18_r(0) < delta) data &= ~0x04;
		if (input_port_19_r(0) < delta) data &= ~0x08;
		JOY_LOG(1,"JOY_r",(errorlog,"$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input_port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}

/*************************************************************************
 *
 *		FDC
 *		floppy disk controller
 *
 *************************************************************************/

WRITE_HANDLER ( pc_FDC_w )
{
	switch( offset )
	{
		case 0: /* n/a */				   break;
		case 1: /* n/a */				   break;
		case 2: pc_fdc_DOR_w(data); 	   break;
		case 3: /* tape drive select? */   break;
		case 4: pc_fdc_data_rate_w(data);  break;
		case 5: pc_fdc_command_w(data);    break;
		case 6: /* fdc reserved */		   break;
		case 7: /* n/a */ break;
	}
}

READ_HANDLER ( pc_FDC_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: /* n/a */				   break;
		case 1: /* n/a */				   break;
		case 2: /* n/a */				   break;
		case 3: /* tape drive select? */   break;
		case 4: data = pc_fdc_status_r();  break;
		case 5: data = pc_fdc_data_r();    break;
		case 6: /* FDC reserved */		   break;
		case 7: data = pc_fdc_DIR_r();	   break;
    }
	return data;
}

/*************************************************************************
 *
 *		ATHD
 *		AT hard disk
 *
 *************************************************************************/
#if 0
#define ATHD1_W \
	case 0x1f0: pc_ide_data_w(data);				break; \
	case 0x1f1: pc_ide_write_precomp_w(data);		break; \
	case 0x1f2: pc_ide_sector_count_w(data);		break; \
	case 0x1f3: pc_ide_sector_number_w(data);		break; \
	case 0x1f4: pc_ide_cylinder_number_l_w(data);	break; \
	case 0x1f5: pc_ide_cylinder_number_h_w(data);	break; \
	case 0x1f6: pc_ide_drive_head_w(data);			break; \
	case 0x1f7: pc_ide_command_w(data); 			break

#define ATHD1_R \
	case 0x1f0: data = pc_ide_data_r(); 			break; \
	case 0x1f1: data = pc_ide_error_r();			break; \
	case 0x1f2: data = pc_ide_sector_count_r(); 	break; \
	case 0x1f3: data = pc_ide_sector_number_r();	break; \
	case 0x1f4: data = pc_ide_cylinder_number_l_r();break; \
	case 0x1f5: data = pc_ide_cylinder_number_h_r();break; \
	case 0x1f6: data = pc_ide_drive_head_r();		break; \
	case 0x1f7: data = pc_ide_status_r();			break
#endif

/*************************************************************************
 *
 *		HDC
 *		hard disk controller
 *
 *************************************************************************/
void pc_HDC_w(int chip, int offset, int data)
{
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file[chip<<1] )
		return;
	switch( offset )
	{
		case 0: pc_hdc_data_w(chip,data);	 break;
		case 1: pc_hdc_reset_w(chip,data);	 break;
		case 2: pc_hdc_select_w(chip,data);  break;
		case 3: pc_hdc_control_w(chip,data); break;
	}
}
WRITE_HANDLER ( pc_HDC1_w ) { pc_HDC_w(0, offset, data); }
WRITE_HANDLER ( pc_HDC2_w ) { pc_HDC_w(1, offset, data); }

int pc_HDC_r(int chip, int offset)
{
	int data = 0xff;
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file[chip<<1] )
		return data;
	switch( offset )
	{
		case 0: data = pc_hdc_data_r(chip); 	 break;
		case 1: data = pc_hdc_status_r(chip);	 break;
		case 2: data = pc_hdc_dipswitch_r(chip); break;
		case 3: break;
	}
	return data;
}
READ_HANDLER ( pc_HDC1_r ) { return pc_HDC_r(0, offset); }
READ_HANDLER ( pc_HDC2_r ) { return pc_HDC_r(1, offset); }

/*************************************************************************
 *
 *		MDA
 *		monochrome display adapter
 *
 *************************************************************************/
WRITE_HANDLER ( pc_MDA_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			pc_mda_index_w(data);
			break;
		case 1: case 3: case 5: case 7:
			pc_mda_port_w(data);
			break;
		case 8:
			pc_mda_mode_control_w(data);
			break;
		case 9:
			pc_mda_color_select_w(data);
			break;
		case 10:
			pc_mda_feature_control_w(data);
			break;
		case 11:
			pc_mda_lightpen_strobe_w(data);
			break;
		case 15:
			pc_hgc_config_w(data);
			break;
	}
}

READ_HANDLER ( pc_MDA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = pc_mda_index_r();
			break;
		case 1: case 3: case 5: case 7:
			data = pc_mda_port_r();
			break;
		case 8:
			data = pc_mda_mode_control_r();
			break;
		case 9:
			/* -W set lightpen flipflop */
			break;
		case 10:
			data = pc_mda_status_r();
			break;
		case 11:
			/* -W lightpen strobe reset */
			break;
		/* 12, 13, 14  are the LPT1 ports */
		case 15:
			data = pc_hgc_config_r();
			break;
    }
	return data;
}

/*************************************************************************
 *
 *		CGA
 *		color graphics adapter
 *
 *************************************************************************/
WRITE_HANDLER ( pc_CGA_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			pc_cga_index_w(data);
			break;
		case 1: case 3: case 5: case 7:
			pc_cga_port_w(data);
			break;
		case 8:
			pc_cga_mode_control_w(data);
			break;
		case 9:
			pc_cga_color_select_w(data);
			break;
		case 10:
			pc_cga_feature_control_w(data);
			break;
		case 11:
			pc_cga_lightpen_strobe_w(data);
			break;
	}
}

READ_HANDLER ( pc_CGA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = pc_cga_index_r();
			break;
		case 1: case 3: case 5: case 7:
			data = pc_cga_port_r();
			break;
		case 8:
			data = pc_cga_mode_control_r();
			break;
		case 9:
			/* -W set lightpen flipflop */
			break;
		case 10:
			data = pc_cga_status_r();
			break;
		case 11:
			/* -W lightpen strobe reset */
			break;
    }
	return data;
}

/*************************************************************************
 *
 *		EXP
 *		expansion port
 *
 *************************************************************************/
WRITE_HANDLER ( pc_EXP_w )
{
	DBG_LOG(1,"EXP_unit_w",(errorlog, "$%02x\n", data));
	pc_port[0x213] = data;
}

READ_HANDLER ( pc_EXP_r )
{
    int data = pc_port[0x213];
    DBG_LOG(1,"EXP_unit_r",(errorlog, "$%02x\n", data));
	return data;
}

/*************************************************************************
 *
 *		T1T
 *		Tandy 1000 / PCjr
 *
 *************************************************************************/
WRITE_HANDLER ( pc_t1t_p37x_w )
{
	DBG_LOG(1,"T1T_p37x_w",(errorlog, "#%d $%02x\n", offset, data));
	switch( offset )
	{
		case 0: pc_port[0x378] = data; break;
		case 1: pc_port[0x379] = data; break;
		case 2: pc_port[0x37a] = data; break;
		case 3: pc_port[0x37b] = data; break;
		case 4: pc_port[0x37c] = data; break;
		case 5: pc_port[0x37d] = data; break;
		case 6: pc_port[0x37e] = data; break;
		case 7: pc_port[0x37f] = data; break;
	}
}

READ_HANDLER ( pc_t1t_p37x_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: data = pc_port[0x378]; break;
		case 1: data = pc_port[0x379]; break;
		case 2: data = pc_port[0x37a]; break;
		case 3: data = pc_port[0x37b]; break;
		case 4: data = pc_port[0x37c]; break;
		case 5: data = pc_port[0x37d]; break;
		case 6: data = pc_port[0x37e]; break;
		case 7: data = pc_port[0x37f]; break;
	}
	DBG_LOG(1,"T1T_p37x_r",(errorlog, "#%d $%02x\n", offset, data));
    return data;
}

WRITE_HANDLER ( pc_T1T_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			pc_t1t_index_w(data);
			break;
		case 1: case 3: case 5: case 7:
			pc_t1t_port_w(data);
			break;
		case 8:
			pc_t1t_mode_control_w(data);
			break;
		case 9:
			pc_t1t_color_select_w(data);
			break;
		case 10:
			pc_t1t_vga_index_w(data);
            break;
        case 11:
			pc_t1t_lightpen_strobe_w(data);
			break;
		case 12:
            break;
		case 13:
            break;
        case 14:
			pc_t1t_vga_data_w(data);
            break;
        case 15:
			pc_t1t_bank_w(data);
			break;
    }
}

READ_HANDLER ( pc_T1T_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = pc_t1t_index_r();
			break;
		case 1: case 3: case 5: case 7:
			data = pc_t1t_port_r();
			break;
		case 8:
			data = pc_t1t_mode_control_r();
			break;
		case 9:
			data = pc_t1t_color_select_r();
			break;
		case 10:
			data = pc_t1t_status_r();
			break;
		case 11:
			/* -W lightpen strobe reset */
			break;
		case 12:
			break;
		case 13:
            break;
		case 14:
			data = pc_t1t_vga_data_r();
            break;
        case 15:
			data = pc_t1t_bank_r();
    }
	return data;
}

/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/

static void pc_keyboard_helper(const char *codes)
{
	int i;
	for (i=0; codes[i]; i++) {
		kb_queue[kb_head] = codes[i];
		kb_head = ++kb_head % 256;
	}
}

/**************************************************************************
 *	scan keys and stuff make/break codes
 **************************************************************************/
static void pc_keyboard(void)
{
	int i;

	update_input_ports();
    for( i = 0x01; i < 0x60; i++  )
	{
		if( readinputport(i/16 + 4) & (1 << (i & 15)) )
		{
			if( make[i] == 0 )
			{
				make[i] = 1;
				if (i==0x45) keyboard_numlock^=1;
				kb_queue[kb_head] = i;
				kb_head = ++kb_head % 256;
			}
			else
			{
				make[i] += 1;
				if( make[i] == kb_delay )
				{
					kb_queue[kb_head] = i;
					kb_head = ++kb_head % 256;
				}
				else
				if( make[i] == kb_delay + kb_repeat )
				{
					make[i] = kb_delay;
					kb_queue[kb_head] = i;
					kb_head = ++kb_head % 256;
				}
			}
		}
		else
		if( make[i] )
		{
			make[i] = 0;
			kb_queue[kb_head] = i | 0x80;
			kb_head = ++kb_head % 256;
		}
    }

    for( i = 0x60; i < 0x70; i++  )
	{
		if( readinputport(i/16 + 4) & (1 << (i & 15)) )
		{
			if( make[i] == 0 )
			{
				make[i] = 1;
				pc_keyboard_helper(keyboard_mf2_code[keyboard_numlock][i-0x60].pressed);
			}
			else
			{
				make[i] += 1;
				if( make[i] == kb_delay )
				{
					pc_keyboard_helper(keyboard_mf2_code[keyboard_numlock][i-0x60].pressed);
				}
				else
					if( make[i] == kb_delay + kb_repeat )
					{
						make[i]=kb_delay;
						pc_keyboard_helper(keyboard_mf2_code[keyboard_numlock][i-0x60].pressed);
					}
			}
		}
		else
			if( make[i] )
			{
				make[i] = 0;
				pc_keyboard_helper(keyboard_mf2_code[keyboard_numlock][i-0x60].released);
			}
    }
	

	if( !pc_PIC_irq_pending(1) )
	{
		if( kb_tail != kb_head )
		{
			pc_port[0x60] = kb_queue[kb_tail];
			DBG_LOG(1,"KB_scancode",(errorlog, "$%02x\n", pc_port[0x60]));
			kb_tail = ++kb_tail % 256;
			pc_PIC_issue_irq(1);
		}
	}
}

/**************************************************************************
 *
 *  'MS' serial mouse support
 *
 **************************************************************************/
/**************************************************************************
 *	change the modem status register
 **************************************************************************/
static void change_msr(int n, int new_msr)
{
	/* no change in modem status bits? */
	if( ((COM_msr[n] ^ new_msr) & 0xf0) == 0 )
		return;

	/* set delta status bits 0..3 and new modem status bits 4..7 */
    COM_msr[n] = (((COM_msr[n] ^ new_msr) >> 4) & 0x0f) | (new_msr & 0xf0);

	/* set up interrupt information register */
    COM_iir[n] &= ~(0x06 | 0x01);

    /* OUT2 + modem status interrupt enabled? */
	if( (COM_mcr[n] & 0x08) && (COM_ier[n] & 0x08) )
		/* issue COM1/3 IRQ4, COM2/4 IRQ3 */
		pc_PIC_issue_irq(4-(n&1));
}

/**************************************************************************
 *	Check for mouse moves and buttons. Build delta x/y packets
 **************************************************************************/
static void pc_mouse_scan(int n)
{
	static int ox = 0, oy = 0;
    int dx, dy, nb;

	dx = readinputport(13) - ox;
	if (dx>=0x800) dx-=0x1000;
	else if (dx<=-0x800) dx+=0x1000;
	dy = readinputport(14) - oy;
	if (dy>=0x800) dy-=0x1000;
	else if (dy<=-0x800) dy+=0x1000;
	nb = readinputport(12);

	/* check if there is any delta or mouse buttons changed */
	if ( (m_head==m_tail) &&( dx || dy || nb != mb ) )
	{
		ox = readinputport(13);
		oy = readinputport(14);
		mb = nb;
		/* split deltas into packtes of -128..+127 max */
		do
		{
			UINT8 m0, m1, m2;
			int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
			int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;
			m0 = 0x40 | ((nb << 4) & 0x30) | ((ddx >> 6) & 0x03) | ((ddy >> 4) & 0x0c);
			m1 = ddx & 0x3f;
			m2 = ddy & 0x3f;
			m_queue[m_head] = m0 | 0x40;
			m_head = ++m_head % 256;
			m_queue[m_head] = m1 & 0x3f;
			m_head = ++m_head % 256;
			m_queue[m_head] = m2 & 0x3f;
			m_head = ++m_head % 256;
			DBG_LOG(1,"mouse_packet",(errorlog, "dx:%d, dy:%d, $%02x $%02x $%02x\n", dx, dy, m0, m1, m2));
			dx -= ddx;
			dy -= ddy;
		} while( dx || dy );
    }

	if( m_tail != m_head )
	{
        /* check if data rate 1200 baud is set */
		if( COM_dlm[n] != 0x00 || COM_dll[n] != 0x60 )
            COM_lsr[n] |= 0x08; /* set framing error */

        /* if data not yet serviced */
		if( COM_lsr[n] & 0x01 )
			COM_lsr[n] |= 0x02; /* set overrun error */

        /* put data into receiver buffer register */
        COM_rbr[n] = m_queue[m_tail];
        m_tail = ++m_tail & 255;

        /* set data ready status */
        COM_lsr[n] |= 0x01;

		/* set up the interrupt information register */
		COM_iir[n] &= ~(0x06 | 0x01) | 0x04;
        /* OUT2 + received line data avail interrupt enabled? */
		if( (COM_mcr[n] & 0x08) && (COM_ier[n] & 0x01) )
            /* issue COM1/3 IRQ4, COM2/4 IRQ3 */
			pc_PIC_issue_irq(4-(n&1));

		DBG_LOG(1,"mouse_data",(errorlog, "$%02x\n", COM_rbr[n]));
    }
}

/**************************************************************************
 *	Check for mouse control line changes and (de-)install timer
 **************************************************************************/
static void pc_mouse_poll(int n)
{
    int new_msr = 0x00;

    /* check if mouse port has DTR set */
	if( COM_mcr[n] & 0x01 )
		new_msr |= 0x20;	/* set DSR */

    /* check if mouse port has RTS set */
	if( COM_mcr[n] & 0x02 )
		new_msr |= 0x10;	/* set CTS */

	/* CTS just went to 1? */
	if( !(COM_msr[n] & 0x10) && (new_msr & 0x10) )
	{
		/* reset mouse */
		m_head = m_tail = mb = 0;
		m_queue[m_head] = 'M';  /* put 'M' into the buffer.. hmm */
		m_head = ++m_head % 256;
		/* start a timer to scan the mouse input */
		mouse_timer = timer_pulse(TIME_IN_HZ(240), n, pc_mouse_scan);
    }

    /* CTS just went to 0? */
	if( (COM_msr[n] & 0x10) && !(new_msr & 0x10) )
	{
		if( mouse_timer )
			timer_remove(mouse_timer);
		mouse_timer = NULL;
		m_head = m_tail = 0;
	}

    change_msr(n, new_msr);
}

int pc_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2)) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else 
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	if( ((++pc_framecnt & 63) == 63)&&pc_blink_textcolors)
		(*pc_blink_textcolors)(pc_framecnt & 64 );

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}
