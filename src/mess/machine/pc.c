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

#include "pic8259.h"
#include "mess/vidhrdw/vga.h"
#include "tandy1t.h"

#include "mess/machine/pc.h"

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

static int tandy1000hx=0;

void init_pc(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
}

extern void init_t1000hx(void)
{
	init_pc();
	tandy1000_init();
	tandy1000hx=1;
}

void init_pc_vga(void)
{
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

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
		PIT_timer = timer_pulse(rate, 0, pic8259_0_issue_irq);
}

WRITE_HANDLER ( pc_PIT_w )
{
	switch( offset )
	{
    case 0: case 1: case 2:
        PIT_LOG(1,"PIT_counter_w",("cntr#%d $%02x: ", offset, data));
		switch (PIT_access)
		{
            case 0: /* counter latch command */
				PIT_LOG(1,0,("*latch command* "));
				PIT_msb ^= 1;
				if( !PIT_msb )
                    PIT_access = 3;
                break;
            case 1: /* read/write counter bits 0-7 only */
				PIT_LOG(1,0,("LSB only "));
                PIT_clock[offset] = data & 0xff;
                break;
            case 2: /* read/write counter bits 8-15 only */
				PIT_LOG(1,0,("MSB only "));
                PIT_clock[offset] = (data & 0xff) << 8;
                break;
            case 3: /* read/write bits 0-7 first, then 8-15 */
				if (PIT_msb)
				{
                    PIT_clock[offset] = (PIT_clock[offset] & 0x00ff) | ((data & 0xff) << 8);
					PIT_LOG(1,0,("MSB "));
				}
				else
				{
                    PIT_clock[offset] = (PIT_clock[offset] & 0xff00) | (data & 0xff);
					PIT_LOG(1,0,("LSB "));
				}
                PIT_msb ^= 1;
                break;
        }
        PIT_time_access[offset] = timer_get_time();
		switch( offset )
		{
            case 0:
                PIT_LOG(1,0,("sys ticks $%04x\n", PIT_clock[0]));
				pc_PIT_timer_pulse();
                break;
            case 1:
                PIT_LOG(1,0,("RAM refresh $%04x\n", PIT_clock[1]));
                /* DRAM refresh timer: ignored */
                break;
            case 2:
                PIT_LOG(1,0,("tone freq $%04x = %d Hz\n", PIT_clock[offset], (PIT_clock[2]) ? 1193180 / PIT_clock[2] : 1193810 / 65536));
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
        PIT_LOG(1,"PIT_mode_w",("$%02x: idx %d, access %d, mode %d, BCD %d\n", data, PIT_index, PIT_access, PIT_mode, PIT_bcd));
		if (PIT_access == 0)
		{
            int count = PIT_clock[PIT_index] ? PIT_clock[PIT_index] : 65536;
            PIT_latch[PIT_index] = count -
                ((int)(1193180 * (timer_get_time() - PIT_time_access[PIT_index]))) % count;
            PIT_LOG(1,"PIT latch value",("#%d $%04x\n", PIT_index, PIT_latch[PIT_index]));
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
                    PIT_LOG(1,"PIT_counter_r",("latch#%d MSB $%02x\n", offset&3, data));
				}
				else
				{
                    data = PIT_latch[offset&3] & 0xff;
                    PIT_LOG(1,"PIT_counter_r",("latch#%d LSB $%02x\n", offset&3, data));
                }
                PIT_msb ^= 1;
				if( !PIT_msb )
                    PIT_access = 3; /* switch back to clock access */
                break;
            case 1: /* read/write counter bits 0-7 only */
                data = PIT_clock[offset&3] & 0xff;
                PIT_LOG(1,"PIT_counter_r",("cntr#%d LSB $%02x\n", offset&3, data));
                break;
            case 2: /* read/write counter bits 8-15 only */
                data = (PIT_clock[offset&3] >> 8) & 0xff;
                PIT_LOG(1,"PIT_counter_r",("cntr#%d MSB $%02x\n", offset&3, data));
                break;
            case 3: /* read/write bits 0-7 first, then 8-15 */
				if (PIT_msb)
				{
                    data = (PIT_clock[offset&3] >> 8) & 0xff;
                    PIT_LOG(1,"PIT_counter_r",("cntr#%d MSB $%02x\n", offset&3, data));
				}
				else
				{
                    data = PIT_clock[offset&3] & 0xff;
                    PIT_LOG(1,"PIT_counter_r",("cntr#%d LSB $%02x\n", offset&3, data));
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
			PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
			pc_port[0x60] = data;
			break;

		case 1: /* KB controller port B */
			PIO_LOG(1,"PIO_B_w",("$%02x\n", data));
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
			PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
			pc_port[0x62] = data;
			if (tandy1000hx) {
				if (data&8) timer_set_overclock(0, 1);
				else timer_set_overclock(0, 4.77/8);
			}
			break;

		case 3: /* KB controller control port */
			PIO_LOG(1,"PIO_control_w",("$%02x\n", data));
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
            PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
            break;
		case 1: /* KB port B */
			data = pc_port[0x61];
			PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
			break;
		case 2: /* KB port C: equipment flags */
			if (pc_port[0x61] & 0x08)
			{
				/* read hi nibble of S2 */
				data = (input_port_1_r(0) >> 4) & 0x0f;
				PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
			}
			else
			{
				/* read lo nibble of S2 */
				data = input_port_1_r(0) & 0x0f;
				PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
			}
			if (tandy1000hx) {
				data&=~8;
				data|=pc_port[0x62]&8;
				if (tandy1000_read_eeprom()) data|=0x10;
			} 
			break;
		case 3:    /* KB controller control port */
			data = pc_port[0x63];
			PIO_LOG(1,"PIO_control_r",("$%02x\n", data));
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
			LPT_LOG(1,"LPT_data_w",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			LPT_LOG(1,"LPT_status_w",("LPT%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = data;
			LPT_LOG(1,"LPT_control_w",("%d $%02x\n", n, data));
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
			LPT_LOG(1,"LPT_data_r",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			/* set status 'out of paper', '*no* error', 'IRQ has *not* occured' */
			LPT_status[n] = 0x2c;
			data = LPT_status[n];
			LPT_LOG(1,"LPT_status_r",("%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = 0x0c;
			data = LPT_control[n];
			LPT_LOG(1,"LPT_control_r",("%d $%02x\n", n, data));
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
		COM_LOG(1,"COM_w",("COM%d $%02x: disabled\n", n+1, data));
		return;
    }
	switch (idx)
	{
		case 0:
			if (COM_lcr[n] & 0x80)
			{
				COM_dll[n] = data;
				tmp = COM_dlm[n] * 256 + COM_dll[n];
				COM_LOG(1,"COM_dll_w",("COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?1843200/16/tmp:0));
			}
			else
			{
				COM_thr[n] = data;
				COM_LOG(2,"COM_thr_w",("COM%d $%02x\n", n+1, data));
            }
			break;
		case 1:
			if (COM_lcr[n] & 0x80)
			{
				COM_dlm[n] = data;
				tmp = COM_dlm[n] * 256 + COM_dll[n];
                COM_LOG(1,"COM_dlm_w",("COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?1843200/16/tmp:0));
			}
			else
			{
				COM_ier[n] = data;
				COM_LOG(2,"COM_ier_w",("COM%d $%02x: enable int on RX %d, THRE %d, RLS %d, MS %d\n",
					n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1));
			}
            break;
		case 2:
			COM_LOG(1,"COM_fcr_w",("COM%d $%02x (16550 only)\n", n+1, data));
            break;
		case 3:
			COM_lcr[n] = data;
			COM_LOG(1,"COM_lcr_w",("COM%d $%02x word length %d, stop bits %d, parity %c, break %d, DLAB %d\n",
				n+1, data, 5+(data&3), 1+((data>>2)&1), P[(data>>3)&7], (data>>6)&1, (data>>7)&1));
            break;
		case 4:
			COM_mcr[n] = data;
			COM_LOG(1,"COM_mcr_w",("COM%d $%02x DTR %d, RTS %d, OUT1 %d, OUT2 %d, loopback %d\n",
				n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1));
            break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			COM_scr[n] = data;
			COM_LOG(2,"COM_scr_w",("COM%d $%02x\n", n+1, data));
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
		COM_LOG(1,"COM_r",("COM%d $%02x: disabled\n", n+1, data));
		return data;
    }
	switch (idx)
	{
		case 0:
			if (COM_lcr[n] & 0x80)
			{
				data = COM_dll[n];
				COM_LOG(1,"COM_dll_r",("COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = COM_rbr[n];
				if( COM_lsr[n] & 0x01 )
				{
					COM_lsr[n] &= ~0x01;		/* clear data ready status */
					COM_LOG(2,"COM_rbr_r",("COM%d $%02x\n", n+1, data));
				}
            }
			break;
		case 1:
			if (COM_lcr[n] & 0x80)
			{
				data = COM_dlm[n];
				COM_LOG(1,"COM_dlm_r",("COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = COM_ier[n];
				COM_LOG(2,"COM_ier_r",("COM%d $%02x\n", n+1, data));
            }
            break;
		case 2:
			data = COM_iir[n];
			COM_iir[n] |= 1;
			COM_LOG(2,"COM_iir_r",("COM%d $%02x\n", n+1, data));
            break;
		case 3:
			data = COM_lcr[n];
			COM_LOG(2,"COM_lcr_r",("COM%d $%02x\n", n+1, data));
            break;
		case 4:
			data = COM_mcr[n];
			COM_LOG(2,"COM_mcr_r",("COM%d $%02x\n", n+1, data));
            break;
		case 5:
			COM_lsr[n] |= 0x40; /* set TSRE */
			COM_lsr[n] |= 0x20; /* set THRE */
			data = COM_lsr[n];
			if( COM_lsr[n] & 0x1f )
			{
				COM_lsr[n] &= 0xe1; /* clear FE, PE and OE and BREAK bits */
				COM_LOG(2,"COM_lsr_r",("COM%d $%02x, DR %d, OE %d, PE %d, FE %d, BREAK %d, THRE %d, TSRE %d\n",
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
			COM_LOG(2,"COM_msr_r",("COM%d $%02x\n", n+1, data));
            break;
		case 7:
			data = COM_scr[n];
			COM_LOG(2,"COM_scr_r",("COM%d $%02x\n", n+1, data));
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

#if 0
#define JOY_VALUE_TO_TIME(v) (24.2e-6+11e-9*(100000.0/256)*v)
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
#if 0
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
#endif
	{
		delta=new_time-JOY_time;
		if ( delta>JOY_VALUE_TO_TIME(input_port_16_r(0)) ) data &= ~0x01;
		if ( delta>JOY_VALUE_TO_TIME(input_port_17_r(0)) ) data &= ~0x02;
		if ( delta>JOY_VALUE_TO_TIME(input_port_18_r(0)) ) data &= ~0x04;
		if ( delta>JOY_VALUE_TO_TIME(input_port_19_r(0)) ) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input_port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#else
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();
	
	data=input_port_15_r(0)^0xf0;
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
	{
		delta = 256 * 1000 * (new_time - JOY_time);
		if (input_port_16_r(0) < delta) data &= ~0x01;
		if (input_port_17_r(0) < delta) data &= ~0x02;
		if (input_port_18_r(0) < delta) data &= ~0x04;
		if (input_port_19_r(0) < delta) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input
						   _port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}
	
	return data;
}
#endif



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
	DBG_LOG(1,"EXP_unit_w",("$%02x\n", data));
	pc_port[0x213] = data;
}

READ_HANDLER ( pc_EXP_r )
{
    int data = pc_port[0x213];
    DBG_LOG(1,"EXP_unit_r",("$%02x\n", data));
	return data;
}

/*************************************************************************
 *
 *		T1T
 *		Tandy 1000 / PCjr
 *
 *************************************************************************/

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
	

	if( !pic8259_0_irq_pending(1) )
	{
		if( kb_tail != kb_head )
		{
			pc_port[0x60] = kb_queue[kb_tail];
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_port[0x60]));
			kb_tail = ++kb_tail % 256;
			pic8259_0_issue_irq(1);
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
		pic8259_0_issue_irq(4-(n&1));
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
			DBG_LOG(1,"mouse_packet",("dx:%d, dy:%d, $%02x $%02x $%02x\n", dx, dy, m0, m1, m2));
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
			pic8259_0_issue_irq(4-(n&1));

		DBG_LOG(1,"mouse_data",("$%02x\n", COM_rbr[n]));
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

	if (!tandy1000hx &&(turboswitch !=(input_port_3_r(0)&2)) ) {
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
