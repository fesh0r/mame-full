/******************************************************************************
 *	Sega Saturn
 *
 *	system driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Sep 2000
 *
 *	memory map
 *
 *	**** 00000000-01ffffff
 *	00000000-0007ffff IPL ROM
 *	00100000-0010007f SMPC
 *	00180000-0018ffff BACK RAM
 *	00200000-002fffff WORK RAM L
 *	01000000-01000003 MINIT R
 *	01800000-01800003 SINIT R
 *
 *	**** 02000000-03ffffff
 *	02000000-03ffffff CS0 R
 *
 *	**** 04000000-05ffffff
 *	04000000-04ffffff CS1 R
 *	05000000-057fffff CS2 R
 *	05800000-058fffff CD R
 *  05900000-059fffff Work RAM (System bus)
 *	05a00000-05a7ffff SOUND RAM
 *	05b00000-05b00ee3 DSP R
 *  05c00000-05c7ffff VDP1 VRAM
 *	05c80000-05cbffff Frame buffers
 *	05d00000-05d00017 VDP1 registers
 *	05e00000-05e7ffff VDP2 VRAM
 *	05f00000-05f00fff VDP2 COLOR RAM
 *	05f80000-05f8011f VDP2 registers
 *	05fc0000-05fdffff SMPC
 *	05fe0000-05fe00cf SCU, DMA/DSP
 *
 *	**** 06000000-07ffffff
 *	06000000-060fffff WORK RAM H
 *****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static UINT8 *mem;

#define SATURN_ROM_BASE         0x00000000
#define SATURN_ROM_SIZE 		0x00080000
#define SATURN_WORKL_RAM_BASE	0x00080000
#define SATURN_WORKL_RAM_SIZE	0x00100000
#define SATURN_WORKH_RAM_BASE	0x00180000
#define SATURN_WORKH_RAM_SIZE	0x00100000
#define SATURN_SOUND_RAM_BASE	0x00280000
#define SATURN_SOUND_RAM_SIZE	0x00080000
#define SATURN_VDP1_RAM_BASE	0x00300000
#define SATURN_VDP1_RAM_SIZE	0x00080000
#define SATURN_VDP2_RAM_BASE	0x00380000
#define SATURN_VDP2_RAM_SIZE	0x00080000
#define SATURN_FB1_RAM_BASE 	0x00400000
#define SATURN_FB1_RAM_SIZE 	0x00040000
#define SATURN_FB2_RAM_BASE 	0x00440000
#define SATURN_FB2_RAM_SIZE 	0x00040000
#define SATURN_COLOR_RAM_BASE	0x00480000
#define SATURN_COLOR_RAM_SIZE	0x00001000
#define SATURN_BACK_RAM_BASE	0x00481000
#define SATURN_BACK_RAM_SIZE	0x00010000

READ_HANDLER( saturn_rom_r )    /* ROM */
{
	return mem[offset & (SATURN_ROM_SIZE-1)];
}

WRITE_HANDLER( saturn_rom_w )	/* ROM */
{
	logerror("saturn_rom_w   %07x %02x\n", offset, data);
}

READ_HANDLER( saturn_workl_ram_r )
{
    offs_t ea = SATURN_WORKL_RAM_BASE + (offset & (SATURN_WORKL_RAM_SIZE-1));
	return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_workl_ram_w )
{
	offs_t ea = SATURN_WORKL_RAM_BASE + (offset & (SATURN_WORKL_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_workh_ram_r )
{
	offs_t ea = SATURN_WORKH_RAM_BASE + (offset & (SATURN_WORKH_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_workh_ram_w )
{
	offs_t ea = SATURN_WORKH_RAM_BASE + (offset & (SATURN_WORKH_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_sound_ram_r )
{
	offs_t ea = SATURN_SOUND_RAM_BASE + (offset & (SATURN_SOUND_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_sound_ram_w )
{
	offs_t ea = SATURN_SOUND_RAM_BASE + (offset & (SATURN_SOUND_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_vdp1_ram_r )
{
	offs_t ea = SATURN_VDP1_RAM_BASE + (offset & (SATURN_VDP1_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_vdp1_ram_w )
{
	offs_t ea = SATURN_VDP1_RAM_BASE + (offset & (SATURN_VDP1_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_vdp2_ram_r )
{
	offs_t ea = SATURN_VDP2_RAM_BASE + (offset & (SATURN_VDP2_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_vdp2_ram_w )
{
	offs_t ea = SATURN_VDP2_RAM_BASE + (offset & (SATURN_VDP2_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_fb1_ram_r )
{
	offs_t ea = SATURN_FB1_RAM_BASE + (offset & (SATURN_FB1_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_fb1_ram_w )
{
	offs_t ea = SATURN_FB1_RAM_BASE + (offset & (SATURN_FB1_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_fb2_ram_r )
{
	offs_t ea = SATURN_FB2_RAM_BASE + (offset & (SATURN_FB2_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_fb2_ram_w )
{
	offs_t ea = SATURN_FB2_RAM_BASE + (offset & (SATURN_FB2_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_color_ram_r )
{
	offs_t ea = SATURN_COLOR_RAM_BASE + (offset & (SATURN_COLOR_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_color_ram_w )
{
	offs_t ea = SATURN_COLOR_RAM_BASE + (offset & (SATURN_COLOR_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_back_ram_r )
{
	offs_t ea = SATURN_BACK_RAM_BASE + (offset & (SATURN_BACK_RAM_SIZE-1));
    return READ_WORD(&mem[ea]);
}

WRITE_HANDLER( saturn_back_ram_w )
{
	offs_t ea = SATURN_BACK_RAM_BASE + (offset & (SATURN_BACK_RAM_SIZE-1));
	data_t oldword = READ_WORD(&mem[ea]);
	data_t newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&mem[ea], newword);
}

READ_HANDLER( saturn_smpc_r )   /* SMPC */
{
    logerror("saturn_smpc_r  %07x\n", offset);
	return 0xffff;
}

WRITE_HANDLER( saturn_smpc_w )  /* SMPC */
{
    logerror("saturn_smpc_w  %07x %02x\n", offset, data);
}

READ_HANDLER( saturn_cs0_r )    /* CS0 */
{
	logerror("saturn_cs0_r   %07x\n", offset);
	return 0xffff;
}

WRITE_HANDLER( saturn_cs0_w )	/* CS0 */
{
	logerror("saturn_cs0_w   %07x %02x\n", offset, data);
}

READ_HANDLER( saturn_cs1_r )    /* CS1 */
{
    logerror("saturn_cs1_r   %07x\n", offset);
	return 0xffff;
}

WRITE_HANDLER( saturn_cs1_w )	/* CS1 */
{
    logerror("saturn_cs1_w   %07x %02x\n", offset, data);
}

READ_HANDLER( saturn_cs2_r )    /* CS2 */
{
    logerror("saturn_cs2_r   %07x\n", offset);
	return 0xffff;
}

WRITE_HANDLER( saturn_cs2_w )	/* CS2 */
{
    logerror("saturn_cs2_w   %07x %02x\n", offset, data);
}

/********************************************************
 *	Compact Disc
 ********************************************************/
struct cdblock {
	UINT16 hirq;
	UINT16 hm;
	UINT16 cr1;
	UINT16 cr2;
	UINT16 cr3;
	UINT16 cr4;
};

struct cdblock acdblock;

enum {
	ST_BUSY    = 0x00,
	ST_PAUSE   = 0x01,
	ST_STANDBY = 0x02,
	ST_PLAY    = 0x03,
	ST_SEEK    = 0x04,
	ST_SCAN    = 0x05,
	ST_OPEN    = 0x06,
	ST_NODISC  = 0x27,
	ST_RETRY   = 0x08,
	ST_ERROR   = 0x09,
	ST_FATAL   = 0x0a
};

static void hirq_w(data_t data)
{
	logerror("            CD: write Hirq %04x (PC=%8X)\n", data, cpu_get_reg(SH2_PC));
	acdblock.hirq &= (1 | ~data);
	/* ### irqs */
}

static void do_command(void)
{
	switch(acdblock.cr1>>8)
	{
	case 0x00:
		logerror("CD: Get status\n");
		break;
	case 0x01:
		logerror("CD: Get hardware info\n");
		acdblock.cr1 = ST_NODISC << 8;
		acdblock.cr2 = 0;
		acdblock.cr3 = 0;
		acdblock.cr4 = 0;
		acdblock.hirq |= 0x0fe1;
		/* acdblock.hirq |= 0xffff; */
		break;
	case 0x02:
		logerror("CD: Get TOC\n");
		break;
	case 0x03:
		logerror("CD: Get session info\n");
		break;
	case 0x04:
		logerror("CD: Initialize, stdby=%d, ecc=%d, rf=%d\n\n");
		break;
	case 0x05:
		logerror("CD: Open CD tray\n");
		break;
	case 0x06:
		logerror("CD: End data transfer\n");
		acdblock.cr1 = ST_NODISC<<8;
		acdblock.cr2 = 0;
		acdblock.cr3 = 0;
		acdblock.cr4 = 0;
		acdblock.hirq |= 0x0001;
		break;
	case 0x10:
		logerror("CD: Play disk\n");
		break;
	case 0x11:
		logerror("CD: Disk seek\n");
		break;
	case 0x12:
		logerror("CD: CD scan\n");
		break;
	case 0x20:
		logerror("CD: Get subcode\n");
		break;
	case 0x30:
		logerror("CD: Set device\n");
		break;
	case 0x31:
		logerror("CD: Get device\n");
		break;
	case 0x32:
		logerror("CD: Get last destination\n");
		break;
	case 0x40:
		logerror("CD: Set filter range\n");
		break;
	case 0x41:
		logerror("CD: Get filter range\n");
		break;
	case 0x42:
		logerror("CD: Set filter subheader conditions\n");
		break;
	case 0x43:
		logerror("CD: Get filter subheader conditions\n");
		break;
	case 0x44:
		logerror("CD: Set filter mode\n");
		break;
	case 0x45:
		logerror("CD: Get filter mode\n");
		break;
	case 0x46:
		logerror("CD: Set filter connexion\n");
		break;
	case 0x47:
		logerror("CD: Get filter connexion\n");
		break;
	case 0x48:
		logerror("CD: Reset selector\n");
		break;
	case 0x50:
		logerror("CD: Get CD block size\n");
		break;
	case 0x51:
		logerror("CD: Get buffer size\n");
		break;
	case 0x52:
		logerror("CD: Calculate actual size\n");
		break;
	case 0x53:
		logerror("CD: Get actual size\n");
		break;
	case 0x54:
		logerror("CD: Get sector info\n");
		break;
	case 0x55:
		logerror("CD: Execute FAD search\n");
		break;
	case 0x56:
		logerror("CD: Get FAD search results\n");
		break;
	case 0x60:
		logerror("CD: Get sector length\n");
		break;
	case 0x61:
		logerror("CD: Get sector data\n");
		break;
	case 0x62:
		logerror("CD: Delete sector data\n");
		break;
	case 0x63:
		logerror("CD: Get and delete sector data\n");
		break;
	case 0x65:
		logerror("CD: Copy sector data\n");
		break;
	case 0x66:
		logerror("CD: Move sector data\n");
		break;
	case 0x67:
		logerror("CD: Get copy error\n");
		acdblock.cr1 = ST_STANDBY<<8;
		acdblock.cr2 = 0;
		acdblock.cr3 = 0;
		acdblock.cr4 = 0;
		/* acdblock.hirq |= 0xffff; */
		break;
	case 0x70:
		logerror("CD: Change directory\n");
		break;
	case 0x72:
		logerror("CD: Get file system scope\n");
		break;
	case 0x73:
		logerror("CD: Get file info\n");
		break;
	case 0x74:
		logerror("CD: Read file\n");
		break;
	case 0x75:
		logerror("CD: Abort file\n");
		acdblock.cr1 = ST_STANDBY<<8;
		acdblock.cr2 = 0;
		acdblock.cr3 = 0;
		acdblock.cr4 = 0;
		acdblock.hirq |= 0x0201;
		break;
	default:
		printf("CD: Executing command %02x\n", acdblock.cr1>>8);
		break;
	}
	acdblock.cr1 = ST_STANDBY<<8;
	/* acdblock.cr2 = 0; */
	/* acdblock.cr3 = 0; */
	/* acdblock.cr4 = 0; */
	acdblock.hirq |= 0x0fe1;
}

void cd_init(void)
{
	acdblock.cr1 =			  'C';
	acdblock.cr2 = ('D'<<8) | 'B';
	acdblock.cr3 = ('L'<<8) | 'O';
	acdblock.cr4 = ('C'<<8) | 'K';

	acdblock.hirq = 1;
	acdblock.hm = 0;
}

READ_HANDLER( saturn_cd_r )    /* CD */
{
	data_t data = 0x00;

    /*  static int tick = 0; */
	switch(offset & 0xfffff)
    {
    case 0x90008:
		logerror("            CD: read hirq (PC=%8X)\n", cpu_get_reg(SH2_PC));
		data = acdblock.hirq;
    case 0x90018:
		logerror("            CD: read cr1 (PC=%8X)\n", cpu_get_reg(SH2_PC));
		data = acdblock.cr1;
		break;
    case 0x9001c:
		data = acdblock.cr2;
		break;
    case 0x90020:
		data = acdblock.cr3;
        break;
    case 0x90024:
		data = acdblock.cr4;
		break;
    default:
		printf("            CD: read  %5X (PC=%8X)\n", offset, cpu_get_reg(SH2_PC));
    }
	logerror("saturn_cd_r   %07x %02x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_cd_w )   /* CD */
{
	logerror("saturn_cd_w   %07x %02x\n", offset, data);
	switch(offset & 0xfffff)
	{
    case 0x90008:
		hirq_w(data);
        break;
    case 0x90018:
		acdblock.cr1 = data;
        break;
    case 0x9001c:
		acdblock.cr2 = data;
        break;
    case 0x90020:
		acdblock.cr3 = data;
        break;
    case 0x90024:
		acdblock.cr4 = data;
        do_command();
        break;
    default:
		logerror("            CD: write %5X.%c (%X) (PC=%8X)\n", offset, data, cpu_get_reg(SH2_PC));
	}
}

/********************************************************
 *	FRT master
 ********************************************************/
READ_HANDLER( saturn_minit_r )  /* MINIT */
{
	logerror("saturn_minit_r %07x\n", offset);
	return 0x00;
}

WRITE_HANDLER( saturn_minit_w )  /* MINIT */
{
	logerror("saturn_minit_w %07x %02x\n", offset, data);
	cpu_set_irq_line(0, 1, HOLD_LINE);
}

/********************************************************
 *	FRT slave
 ********************************************************/
READ_HANDLER( saturn_sinit_r )  /* SINIT */
{
	logerror("saturn_sinit_r %07x\n", offset);
	return 0x00;
}

WRITE_HANDLER( saturn_sinit_w )  /* SINIT */
{
	logerror("saturn_sinit_w %07x %02x\n", offset, data);
	cpu_set_irq_line(1, 1, HOLD_LINE);
}

/********************************************************
 *	DSP
 ********************************************************/
READ_HANDLER( saturn_dsp_r )   /* DSP */
{
	logerror("saturn_dsp_r  %07x\n", offset);
    return 0xff;
}

WRITE_HANDLER( saturn_dsp_w )  /* DSP */
{
	logerror("saturn_dsp_w  %07x %02x\n", offset, data);
}

/********************************************************
 *	VDP1
 ********************************************************/
READ_HANDLER( saturn_vdp1_r )   /* VDP1 registers */
{
    logerror("saturn_vdp1_r  %07x\n", offset);
    return 0xff;
}

WRITE_HANDLER( saturn_vdp1_w )	/* VDP1 registers */
{
    logerror("saturn_vdp1_w  %07x %02x\n", offset, data);
}

/********************************************************
 *	VDP2
 ********************************************************/
READ_HANDLER( saturn_vdp2_r )   /* VDP2 registers */
{
    logerror("saturn_vdp2_r  %07x\n", offset);
    return 0xff;
}

WRITE_HANDLER( saturn_vdp2_w )	/* VDP2 registers */
{
    logerror("saturn_vdp2_w  %07x %02x\n", offset, data);
}

/********************************************************
 *	SCU
 ********************************************************/

enum
{
	INT_VBLIN = 0,
	INT_VBLOUT = 1,
	INT_HBLIN = 2,
	INT_TIMER0 = 3,
	INT_TIMER1 = 4,
	INT_DSP = 5,
	INT_SOUND = 6,
	INT_SMPC = 7,
	INT_PAD = 8,
	INT_DMA2 = 9,
	INT_DMA1 = 10,
	INT_DMA0 = 11,
	INT_DMAILL = 12,
	INT_SPRITE = 13,
	INT_ABUS = 16
};

struct dma
{
	PAIR radr;
	PAIR wadr;
	PAIR bytes;
	PAIR add;
	PAIR enable;
	PAIR mode;
};

struct scu
{
	struct dma dma[3];

	PAIR pctrl;

    PAIR imask;
	PAIR istat;
	PAIR aiack;

	PAIR dsp_pc;

	PAIR t0cmp;
	PAIR t1data;
	PAIR t1mode;
};

struct scu ascu;

static const char *irq_names[16] =
{
	"vblin",    "vblout",   "hblin",    "timer0",
	"timer1",   "dsp",      "sound",    "smpc",
	"pad",      "dma2",     "dma1",     "dma0",
	"dmaill",   "sprite",   "bit14",    "abus"
};

static void set_imask(void)
{
	logerror("SCU: Interrupt mask change.  Allowed:");
	if ((ascu.imask.d & 0xbfff) == 0xbfff)
		printf(" <none>\n");
	else
	{
		int i;

		for (i = 0; i < 16; i++)
			if (!((1 << i) & ascu.imask.d))
				logerror(" %s", irq_names[i]);
		logerror("\n");
	}
	/* ### irqs */
}

static void do_irq(int irq)
{
	static int levels[32] =
	{
		15, 14, 13, 12, 11, 10,  9,  8,
		 8,  6,  6,  5,  3,  2,  0,  0,
		 7,  7,  7,  7,  4,  4,  4,  4,
		 1,  1,  1,  1,  1,  1,  1,  1
	};
	cpu_irq_line_vector_w(0, irq, 0x40 + irq);
	cpu_set_irq_line(0, levels[irq], HOLD_LINE);
}

void scu_pulse_interrupt(int irq)
{
	if (irq >= INT_ABUS)
	{
		logerror("SCU: pulsed abus irq\n");
	}
	else
	if (!(ascu.imask.d & (1 << irq)))
		do_irq(irq);
}

static void dma_run(int dma)
{
	if (ascu.dma[dma].mode.d & 0x1000000)
	{
		logerror("SCU: DMA %d indirect mode activated\n", dma);
	}
	else
	{
		static int itab[8] = {0, 2, 4, 8, 16, 32, 64, 128};
		UINT32 radr = ascu.dma[dma].radr.d & 0x1fffffff;
		UINT32 wadr = ascu.dma[dma].wadr.d & 0x1fffffff;
		UINT32 bytes = ascu.dma[dma].bytes.d;
		int addr = ascu.dma[dma].add.d & 0x100 ? 4 : 0;
		int addw = itab[ascu.dma[dma].add.d & 7];
		int bbw = (wadr >= 0x5a00000) && (wadr < 0x5fe0000);

		if (bbw && addr == 4 && addw == 2)
		{
			while (bytes > 0)
			{
				cpu_writemem27bew(wadr, cpu_readmem27bew(radr));
				wadr++;
				radr++;
				bytes--;
			}
		}
		else
		{
			logerror("SCU: DMA %d direct mode activated\n", dma);
			logerror("       read  = %08x\n", radr);
			logerror("       write = %08x\n", wadr);
			logerror("       bytes = %08x\n", bytes);
			logerror("       add   = %d, %d\n", addr, addw);
			logerror("       bbw   = %d\n", bbw);
		}
	}
	scu_pulse_interrupt(INT_DMA0 - dma);
}

static void dma_enable(int dma)
{
	if (((ascu.dma[dma].mode.d & 7) == 7) && ((ascu.dma[dma].enable.d & 0x101) == 0x101))
		dma_run(dma);
}

static void dma_mode(int dma)
{
	if ((ascu.dma[dma].enable.d & 7) != 7)
		logerror("SCU: DMA %d in mode %d\n", dma, ascu.dma[dma].enable.d & 7);
}

static void dsp_run(int step)
{
	logerror("SCU: DSP %s\n", step ? "stepped" : "started");
}

static const char *scu_reg_names[] =
{
	"dma0radr", "dma0wadr",  "dma0count", "dma0inc",  "dma0start", "dma0ctrl",  "-", "-",
	"dma1radr", "dma1wadr",  "dma1count", "dma1inc",  "dma1start", "dma1ctrl",  "-", "-",
	"dma2radr", "dma2wadr",  "dma2count", "dma2inc",  "dma2start", "dma2ctrl",  "-", "-",
	"dmafstop", "-",         "-",         "-",        "dmastat",   "-",         "-", "-",
	"dsppctrl", "dsppdata",  "dspdctrl",  "dspddata", "timer0cmp", "timer1set", "timer1mode",  "-",
	"irqmask",  "irqstat",   "abusiack",  "-",        "abusset0",  "abusset1",  "abusrefresh", "-",
	"-",        "scuramsel", "scuvers",   "-"
};

#define REG_NAME(adr) scu_reg_names[(adr)>>2]

READ_HANDLER( saturn_scu_r )    /* SCU, DMA/DSP */
{
	data_t data = 0;

	logerror("saturn_scu_r   %07x %02x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_scu_w )   /* SCU, DMA/DSP */
{
	data &= 0xff;
    logerror("saturn_scu_w   %07x %02x\n", offset, data);

    switch (offset & 0x1ffff)
	{
	case 0x00: ascu.dma[0].radr.b.h3 = data; break;
	case 0x01: ascu.dma[0].radr.b.h2 = data; break;
	case 0x02: ascu.dma[0].radr.b.h = data; break;
	case 0x03: ascu.dma[0].radr.b.l = data; break;

	case 0x04: ascu.dma[0].wadr.b.h3 = data; break;
	case 0x05: ascu.dma[0].wadr.b.h2 = data; break;
	case 0x06: ascu.dma[0].wadr.b.h = data; break;
	case 0x07: ascu.dma[0].wadr.b.l = data; break;

	case 0x08: ascu.dma[0].bytes.b.h3 = data; break;
	case 0x09: ascu.dma[0].bytes.b.h2 = data; break;
	case 0x0a: ascu.dma[0].bytes.b.h = data; break;
	case 0x0b: ascu.dma[0].bytes.b.l = data; break;

	case 0x0c: ascu.dma[0].add.b.h3 = data; break;
	case 0x0d: ascu.dma[0].add.b.h2 = data; break;
	case 0x0e: ascu.dma[0].add.b.h = data; break;
	case 0x0f: ascu.dma[0].add.b.l = data; break;

	case 0x10: ascu.dma[0].enable.b.h3 = data; break;
	case 0x11: ascu.dma[0].enable.b.h2 = data; break;
	case 0x12: ascu.dma[0].enable.b.h = data; break;
	case 0x13: ascu.dma[0].enable.b.l = data; dma_enable(0); break;

	case 0x14: ascu.dma[0].mode.b.h3 = data; break;
	case 0x15: ascu.dma[0].mode.b.h2 = data; break;
	case 0x16: ascu.dma[0].mode.b.h = data; break;
	case 0x17: ascu.dma[0].mode.b.l = data; dma_mode(0); break;

	case 0x20: ascu.dma[1].radr.b.h3 = data; break;
	case 0x21: ascu.dma[1].radr.b.h2 = data; break;
	case 0x22: ascu.dma[1].radr.b.h = data; break;
	case 0x23: ascu.dma[1].radr.b.l = data; break;

	case 0x24: ascu.dma[1].wadr.b.h3 = data; break;
	case 0x25: ascu.dma[1].wadr.b.h2 = data; break;
	case 0x26: ascu.dma[1].wadr.b.h = data; break;
	case 0x27: ascu.dma[1].wadr.b.l = data; break;

	case 0x28: ascu.dma[1].bytes.b.h3 = data; break;
	case 0x29: ascu.dma[1].bytes.b.h2 = data; break;
	case 0x2a: ascu.dma[1].bytes.b.h = data; break;
	case 0x2b: ascu.dma[1].bytes.b.l = data; break;

	case 0x2c: ascu.dma[1].add.b.h3 = data; break;
	case 0x2d: ascu.dma[1].add.b.h2 = data; break;
	case 0x2e: ascu.dma[1].add.b.h = data; break;
	case 0x2f: ascu.dma[1].add.b.l = data; break;

	case 0x30: ascu.dma[1].enable.b.h3 = data; break;
	case 0x31: ascu.dma[1].enable.b.h2 = data; break;
	case 0x32: ascu.dma[1].enable.b.h = data; break;
	case 0x33: ascu.dma[1].enable.b.l = data; dma_enable(1); break;

	case 0x34: ascu.dma[1].mode.b.h3 = data; break;
	case 0x35: ascu.dma[1].mode.b.h2 = data; break;
	case 0x36: ascu.dma[1].mode.b.h = data; break;
	case 0x37: ascu.dma[1].mode.b.l = data; dma_mode(1); break;

	case 0x40: ascu.dma[2].radr.b.h3 = data; break;
	case 0x41: ascu.dma[2].radr.b.h2 = data; break;
	case 0x42: ascu.dma[2].radr.b.h = data; break;
	case 0x43: ascu.dma[2].radr.b.l = data; break;

	case 0x44: ascu.dma[2].wadr.b.h3 = data; break;
	case 0x45: ascu.dma[2].wadr.b.h2 = data; break;
	case 0x46: ascu.dma[2].wadr.b.h = data; break;
	case 0x47: ascu.dma[2].wadr.b.l = data; break;

	case 0x48: ascu.dma[2].bytes.b.h3 = data; break;
	case 0x49: ascu.dma[2].bytes.b.h2 = data; break;
	case 0x4a: ascu.dma[2].bytes.b.h = data; break;
	case 0x4b: ascu.dma[2].bytes.b.l = data; break;

	case 0x4c: ascu.dma[2].add.b.h3 = data; break;
	case 0x4d: ascu.dma[2].add.b.h2 = data; break;
	case 0x4e: ascu.dma[2].add.b.h = data; break;
	case 0x4f: ascu.dma[2].add.b.l = data; break;

	case 0x50: ascu.dma[2].enable.b.h3 = data; break;
	case 0x51: ascu.dma[2].enable.b.h2 = data; break;
	case 0x52: ascu.dma[2].enable.b.h = data; break;
	case 0x53: ascu.dma[2].enable.b.l = data; dma_enable(2); break;

	case 0x54: ascu.dma[2].mode.b.h3 = data; break;
	case 0x55: ascu.dma[2].mode.b.h2 = data; break;
	case 0x56: ascu.dma[2].mode.b.h = data; break;
	case 0x57: ascu.dma[2].mode.b.l = data; dma_mode(2); break;

	case 0x60: break;
	case 0x61: break;
	case 0x62: break;
	case 0x63:
		if (data & 1)
			logerror("SCU: Dma forced stop\n");
		break;

	case 0x80: ascu.pctrl.b.h3 = data; break;
	case 0x81: ascu.pctrl.b.h2 = data; break;
	case 0x82: ascu.pctrl.b.h = data; break;
	case 0x83: ascu.pctrl.b.l = data;
		logerror("SCU: pctrl: %08x\n", ascu.pctrl.d);
		if (ascu.pctrl.d & 0x100)
		{
			ascu.dsp_pc.d = ascu.pctrl.d & 255;
			logerror("SCU: dsp PC=%x\n", ascu.dsp_pc);
		}
		if (ascu.pctrl.d & 0x10000)
			dsp_run(0);
		else
		if (ascu.pctrl.d & 0x20000)
			dsp_run(1);
		break;

	case 0x90: ascu.t0cmp.b.h3 = data; break;
	case 0x91: ascu.t0cmp.b.h2 = data; break;
	case 0x92: ascu.t0cmp.b.h = data; break;
	case 0x93: ascu.t0cmp.b.l = data;
		if (ascu.t0cmp.d < 720)
			logerror("SCU: timer 0 activated on pixel %d\n", ascu.t0cmp.d);
		break;

	case 0x94: ascu.t1data.b.h3 = data; break;
	case 0x95: ascu.t1data.b.h2 = data; break;
	case 0x96: ascu.t1data.b.h = data; break;
	case 0x97: ascu.t1data.b.l = data; break;

	case 0x98: ascu.t1mode.b.h3 = data; break;
	case 0x99: ascu.t1mode.b.h2 = data; break;
	case 0x9a: ascu.t1mode.b.h = data; break;
	case 0x9b: ascu.t1mode.b.l = data;
		if (ascu.t1mode.d & 1)
			logerror("SCU: timer 1 activated\n");
		break;

	case 0xa0: ascu.imask.b.h3 = data; break;
	case 0xa1: ascu.imask.b.h2 = data; break;
	case 0xa2: ascu.imask.b.h = data; break;
	case 0xa3: ascu.imask.b.l = data; set_imask(); break;

	case 0xa4: ascu.istat.b.h3 &= ~data; break;
	case 0xa5: ascu.istat.b.h2 &= ~data; break;
	case 0xa6: ascu.istat.b.h &= ~data; break;
	case 0xa7: ascu.istat.b.l &= ~data; break;

	case 0xa8: ascu.aiack.b.h3 = data; break;
	case 0xa9: ascu.aiack.b.h2 = data; break;
	case 0xaa: ascu.aiack.b.h = data; break;
	case 0xab: ascu.aiack.b.l = data; break;

	case 0xb0:
		break;
	case 0xb4:
		break;
	case 0xb8:
		break;

	case 0xc4:
		break;

	default:
		logerror("           SCU: write %5X [%s] (%X) (PC=%8X)\n", offset, REG_NAME(offset & 0x1ffff), data, cpu_get_reg(SH2_PC));
		break;
    }
}


void saturn_init_machine(void)
{
    int i;
	UINT8 *mem2;

    mem = memory_region(REGION_CPU1);
	mem2 = memory_region(REGION_CPU2);
    for (i = 0; i < SATURN_ROM_SIZE; i += 2)
	{
		UINT8 tmp = mem[i];
		mem[i] = mem2[i] = mem[i+1];
		mem[i+1] = mem2[i+1] = tmp;
    }

	cpu_setbank(1, &mem[SATURN_WORKL_RAM_BASE]);
	cpu_setbank(2, &mem[SATURN_WORKH_RAM_BASE]);
	cpu_setbank(3, &mem[SATURN_SOUND_RAM_BASE]);
	cpu_setbank(4, &mem[SATURN_VDP1_RAM_BASE]);
	cpu_setbank(5, &mem[SATURN_VDP2_RAM_BASE]);
	cpu_setbank(6, &mem[SATURN_FB1_RAM_BASE]);
	cpu_setbank(7, &mem[SATURN_FB2_RAM_BASE]);
	cpu_setbank(8, &mem[SATURN_COLOR_RAM_BASE]);
	cpu_setbank(9, &mem[SATURN_BACK_RAM_BASE]);

    for (i = 0; i < 2; i++)
	{
		install_mem_read_handler (i, 0x00000000, 0x0007ffff, MRA_ROM );
		install_mem_write_handler(i, 0x00000000, 0x0007ffff, MWA_ROM );

		install_mem_read_handler (i, 0x00100000, 0x0100007f, saturn_smpc_r );
		install_mem_write_handler(i, 0x00100000, 0x0100007f, saturn_smpc_w );

		install_mem_read_handler (i, 0x00180000, 0x0018ffff, saturn_back_ram_r );
		install_mem_write_handler(i, 0x00180000, 0x0018ffff, saturn_back_ram_w );

		install_mem_read_handler (i, 0x00200000, 0x002fffff, saturn_workl_ram_r );
		install_mem_write_handler(i, 0x00200000, 0x002fffff, saturn_workl_ram_w );

		install_mem_read_handler (i, 0x01000000, 0x01000003, saturn_minit_r );
		install_mem_write_handler(i, 0x01000000, 0x01000003, saturn_minit_w );

		install_mem_read_handler (i, 0x01800000, 0x01800003, saturn_sinit_r );
		install_mem_write_handler(i, 0x01800000, 0x01800003, saturn_sinit_w );

		install_mem_read_handler (i, 0x02000000, 0x03ffffff, saturn_cs0_r );
		install_mem_write_handler(i, 0x02000000, 0x03ffffff, saturn_cs0_w );

		install_mem_read_handler (i, 0x04000000, 0x04ffffff, saturn_cs1_r );
		install_mem_write_handler(i, 0x04000000, 0x04ffffff, saturn_cs1_w );

		install_mem_read_handler (i, 0x05000000, 0x057fffff, saturn_cs2_r );
		install_mem_write_handler(i, 0x05000000, 0x057fffff, saturn_cs2_w );

		install_mem_read_handler (i, 0x05800000, 0x058fffff, saturn_cd_r );
		install_mem_write_handler(i, 0x05800000, 0x058fffff, saturn_cd_w );

        install_mem_read_handler (i, 0x05a00000, 0x05a7ffff, saturn_sound_ram_r );
		install_mem_write_handler(i, 0x05a00000, 0x05a7ffff, saturn_sound_ram_w );
		install_mem_read_handler (i, 0x05a80000, 0x05afffff, MRA_NOP );
		install_mem_write_handler(i, 0x05a80000, 0x05afffff, MWA_NOP );

        install_mem_read_handler (i, 0x05b00000, 0x05b00ee3, saturn_dsp_r );
		install_mem_write_handler(i, 0x05b00000, 0x05b00ee3, saturn_dsp_w );

		install_mem_read_handler (i, 0x05c00000, 0x05c7ffff, saturn_vdp1_ram_r );
		install_mem_write_handler(i, 0x05c00000, 0x05c7ffff, saturn_vdp1_ram_w );

		install_mem_read_handler (i, 0x05c80000, 0x05cbffff, saturn_fb1_ram_r );
		install_mem_write_handler(i, 0x05c80000, 0x05cbffff, saturn_fb1_ram_w );

		install_mem_read_handler (i, 0x05d00000, 0x05d00017, saturn_vdp1_r );
        install_mem_write_handler(i, 0x05d00000, 0x05d00017, saturn_vdp1_w );

		install_mem_read_handler (i, 0x05e00000, 0x05e7ffff, saturn_vdp2_ram_r );
		install_mem_write_handler(i, 0x05e00000, 0x05e7ffff, saturn_vdp2_ram_w );

		install_mem_read_handler (i, 0x05f00000, 0x05f00fff, saturn_color_ram_r );
		install_mem_write_handler(i, 0x05f00000, 0x05f00fff, saturn_color_ram_w );

		install_mem_read_handler (i, 0x05f80000, 0x05f8011f, saturn_vdp2_r );
		install_mem_write_handler(i, 0x05f80000, 0x05f8011f, saturn_vdp2_w );

		install_mem_read_handler (i, 0x05fe0000, 0x05fe00cf, saturn_scu_r );
		install_mem_write_handler(i, 0x05fe0000, 0x05fe00cf, saturn_scu_w );

		install_mem_read_handler (i, 0x06000000, 0x060fffff, saturn_workh_ram_r );
		install_mem_write_handler(i, 0x06000000, 0x060fffff, saturn_workh_ram_w );
    }
}

void init_saturn(void)
{
	cd_init();
}


int saturn_vh_start(void)
{
	videoram_size = 1024;
    if (generic_vh_start())
        return 1;
	return 0;
}

void saturn_vh_stop(void)
{
	generic_vh_stop();
}

void saturn_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < videoram_size; offs++ )
	{
		if( dirtybuffer[offs] )
		{
            int sx, sy, code;
			sx = (offs % 80) * 8;
			sy = (offs / 80) * 8;
			code = videoram[offs];
			drawgfx(bitmap,Machine->gfx[0],code,0,0,0,sx,sy,
                &Machine->visible_area,TRANSPARENCY_NONE,0);
        }
	}
}

static struct MemoryReadAddress readmem[] =
{
    {-1}
};

static struct MemoryWriteAddress writemem[] =
{
    {-1}
};

INPUT_PORTS_START( saturn )
	PORT_START /* DIP switches */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)

INPUT_PORTS_END

static struct MachineDriver machine_driver_saturn =
{
	/* basic machine hardware */
	{
		{
			CPU_SH2,
			28000000,	/* 28 MHz */
			readmem,writemem,0,0,
			ignore_interrupt, 1
		},
        {
            CPU_SH2,
            28000000,   /* 28 MHz */
			readmem,writemem,0,0,
            ignore_interrupt, 1
		}
    },
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* dual CPU */
	saturn_init_machine,
	NULL,					/* stop machine */

	/* video hardware - include overscan */
	32*8, 16*16, { 0*8, 32*8 - 1, 0*16, 16*16 - 1},
	NULL,
	2, 2,
	NULL,					/* convert color prom */

	VIDEO_TYPE_RASTER,		/* video flags */
	0,						/* obsolete */
	saturn_vh_start,
	saturn_vh_stop,
	saturn_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START(saturn)
	ROM_REGION(0x00491000, REGION_CPU1)
	ROM_LOAD("sega_101.bin", 0x00000000, 0x00080000, 0x224b752c)
	ROM_REGION(0x00080000, REGION_CPU2)
ROM_END

static const struct IODevice io_saturn[] = {
    { IO_END }
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
CONSX(1992, saturn,   0,		saturn,   saturn,	0,		  "Sega",   "Saturn", GAME_NOT_WORKING )


