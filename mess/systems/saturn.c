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
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static UINT8 *mem;
static int video_w = 0;
static struct osd_bitmap *saturn_bitmap[2];

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
	logerror("saturn_rom_w    %07x %02x\n", offset, data);
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
	data_t data = 0x0000;
	logerror("saturn_smpc_r   %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_smpc_w )  /* SMPC */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_smpc_w   %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_smpc_w   %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_smpc_w   %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
}

READ_HANDLER( saturn_cs0_r )    /* CS0 */
{
	data_t data = 0x0000;
	logerror("saturn_cs0_r    %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_cs0_w )	/* CS0 */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_cs0_w    %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_cs0_w    %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_cs0_w    %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
}

READ_HANDLER( saturn_cs1_r )    /* CS1 */
{
    data_t data = 0x0000;
	logerror("saturn_cs1_r    %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_cs1_w )	/* CS1 */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_cs1_w    %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_cs1_w    %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_cs1_w    %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
}

READ_HANDLER( saturn_cs2_r )    /* CS2 */
{
    data_t data = 0x0000;
	logerror("saturn_cs2_r    %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_cs2_w )	/* CS2 */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_cs2_w    %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_cs2_w    %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_cs2_w    %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
}

/********************************************************
 *  SCU
 ********************************************************/

#define INT_VBLIN	0
#define INT_VBLOUT	1
#define INT_HBLIN	2
#define INT_TIMER0	3
#define INT_TIMER1	4
#define INT_DSP 	5
#define INT_SOUND	6
#define INT_SMPC	7
#define INT_PAD 	8
#define INT_DMA2	9
#define INT_DMA1	10
#define INT_DMA0	11
#define INT_DMAILL	12
#define INT_SPRITE	13
#define INT_ABUS	16

struct dma_s
{
    PAIR radr;
    PAIR wadr;
    PAIR bytes;
    PAIR add;
    PAIR enable;
    PAIR mode;
};

struct scu_s
{
    struct dma_s dma[3];

    PAIR pctrl;

    PAIR imask;
    PAIR istat;
    PAIR aiack;

    PAIR dsp_pc;

    PAIR t0cmp;
    PAIR t1data;
    PAIR t1mode;
};

struct scu_s scu;

static const char *irq_names[16] =
{
    "vblin",    "vblout",   "hblin",    "timer0",
    "timer1",   "dsp",      "sound",    "smpc",
    "pad",      "dma2",     "dma1",     "dma0",
    "dmaill",   "sprite",   "bit14",    "abus"
};

static void set_imask(void)
{
    logerror("saturn_scu_w    interrupt mask change.  Allowed:");
    if ((scu.imask.d & 0xbfff) == 0xbfff)
        logerror(" <none>");
    else
    {
        int i;

        for (i = 0; i < 16; i++)
            if (!((1 << i) & scu.imask.d))
                logerror(" %s", irq_names[i]);
    }
    logerror("\n");
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
	cpu_irq_line_vector_w(0, levels[irq], 0x40 + irq);
    cpu_set_irq_line(0, levels[irq], HOLD_LINE);
}

void scu_pulse_interrupt(int irq)
{
    if (irq >= INT_ABUS)
    {
		LOG(("saturn_scu_w    pulsed abus irq\n"));
    }
    else
    {
		LOG(("saturn_scu_w    pulsed irq %d", irq));
		if ((scu.imask.d & (1 << irq)) == 0)
        {
			LOG((" - do_irq"));
            do_irq(irq);
        }
        else
        {
			LOG((" - masked"));
        }
		LOG(("\n"));
    }
}

static void dma_run(int dma)
{
    if (scu.dma[dma].mode.d & 0x1000000)
    {
        logerror("saturn_scu_w    DMA %d indirect mode activated\n", dma);
    }
    else
    {
        static int itab[8] = {0, 2, 4, 8, 16, 32, 64, 128};
        UINT32 radr = scu.dma[dma].radr.d & 0x1fffffff;
        UINT32 wadr = scu.dma[dma].wadr.d & 0x1fffffff;
        UINT32 bytes = scu.dma[dma].bytes.d;
        int addr = scu.dma[dma].add.d & 0x100 ? 4 : 0;
        int addw = itab[scu.dma[dma].add.d & 7];
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
    if (((scu.dma[dma].mode.d & 7) == 7) && ((scu.dma[dma].enable.d & 0x101) == 0x101))
        dma_run(dma);
}

static void dma_mode(int dma)
{
    if ((scu.dma[dma].enable.d & 7) != 7)
        logerror("saturn_scu_w    DMA %d in mode %d\n", dma, scu.dma[dma].enable.d & 7);
}

static void dsp_run(int step)
{
    logerror("saturn_scu_w    DSP %s\n", step ? "stepped" : "started");
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

#define SCU_REG_NAME(adr) scu_reg_names[(adr)>>2]

READ_HANDLER( saturn_scu_r )    /* SCU, DMA/DSP */
{
    data_t data = 0x0000;
    logerror("saturn_scu_r    %07x -> %04x\n", offset, data);
    return data;
}

WRITE_HANDLER( saturn_scu_w )   /* SCU, DMA/DSP */
{
    data_t oldword = READ_WORD((UINT8 *)&scu + offset);
    data_t newword = COMBINE_WORD(oldword, data);

    switch (offset & 0x1fffe)
    {
    case 0x00: scu.dma[0].radr.w.h = newword; break;
    case 0x02: scu.dma[0].radr.w.l = newword; break;

    case 0x04: scu.dma[0].wadr.w.h = newword; break;
    case 0x06: scu.dma[0].wadr.w.l = newword; break;

    case 0x08: scu.dma[0].bytes.w.h = newword; break;
    case 0x0a: scu.dma[0].bytes.w.l = newword; break;

    case 0x0c: scu.dma[0].add.w.h = newword; break;
    case 0x0e: scu.dma[0].add.w.l = newword; break;

    case 0x10: scu.dma[0].enable.w.h = newword; break;
    case 0x12: scu.dma[0].enable.w.l = newword; dma_enable(0); break;

    case 0x14: scu.dma[0].mode.w.h = newword; break;
    case 0x16: scu.dma[0].mode.w.l = newword; dma_mode(0); break;

    case 0x20: scu.dma[1].radr.w.h = newword; break;
    case 0x22: scu.dma[1].radr.w.l = newword; break;

    case 0x24: scu.dma[1].wadr.w.h = newword; break;
    case 0x26: scu.dma[1].wadr.w.l = newword; break;

    case 0x28: scu.dma[1].bytes.w.h = newword; break;
    case 0x2a: scu.dma[1].bytes.w.l = newword; break;

    case 0x2c: scu.dma[1].add.w.h = newword; break;
    case 0x2e: scu.dma[1].add.w.l = newword; break;

    case 0x30: scu.dma[1].enable.w.h = newword; break;
    case 0x32: scu.dma[1].enable.w.l = newword; dma_enable(1); break;

    case 0x34: scu.dma[1].mode.w.h = newword; break;
    case 0x36: scu.dma[1].mode.w.l = newword; dma_mode(1); break;

    case 0x40: scu.dma[2].radr.w.h = newword; break;
    case 0x42: scu.dma[2].radr.w.l = newword; break;

    case 0x44: scu.dma[2].wadr.w.h = newword; break;
    case 0x46: scu.dma[2].wadr.w.h = newword; break;

    case 0x48: scu.dma[2].bytes.w.h = newword; break;
    case 0x4a: scu.dma[2].bytes.w.l = newword; break;

    case 0x4c: scu.dma[2].add.w.h = newword; break;
    case 0x4e: scu.dma[2].add.w.l = newword; break;

    case 0x50: scu.dma[2].enable.w.h = newword; break;
    case 0x52: scu.dma[2].enable.w.l = newword; dma_enable(2); break;

    case 0x54: scu.dma[2].mode.w.h = newword; break;
    case 0x56: scu.dma[2].mode.w.l = newword; dma_mode(2); break;

    case 0x60: break;
    case 0x62:
        if (newword & 1)
            logerror("saturn_scu_w    DMA forced stop\n");
        break;

    case 0x80: scu.pctrl.w.h = newword; break;
    case 0x82: scu.pctrl.w.l = newword;
        logerror("saturn_scu_w    pctrl %08x\n", scu.pctrl.d);
        if (scu.pctrl.d & 0x100)
        {
            scu.dsp_pc.d = scu.pctrl.d & 255;
            logerror("saturn_scu_w    DSP PC=%x\n", scu.dsp_pc);
        }
        if (scu.pctrl.d & 0x10000)
            dsp_run(0);
        else
        if (scu.pctrl.d & 0x20000)
            dsp_run(1);
        break;

    case 0x90: scu.t0cmp.w.h = newword; break;
    case 0x92: scu.t0cmp.w.l = newword;
        if (scu.t0cmp.d < 720)
            logerror("saturn_scu_w    timer 0 activated on pixel %d\n", scu.t0cmp.d);
        break;

    case 0x94: scu.t1data.w.h = newword; break;
    case 0x96: scu.t1data.w.l = newword; break;

    case 0x98: scu.t1mode.w.h = newword; break;
    case 0x99: scu.t1mode.b.l = newword;
        if (scu.t1mode.d & 1)
            logerror("saturn_scu_w    timer 1 activated\n");
        break;

    case 0xa0: scu.imask.w.h = newword; break;
    case 0xa2: scu.imask.w.l = newword; set_imask(); break;

    case 0xa4: scu.istat.w.h &= ~newword; break;
    case 0xa6: scu.istat.w.l &= ~newword; break;

    case 0xa8: scu.aiack.w.h = newword; break;
    case 0xaa: scu.aiack.w.l = newword; break;

    case 0xb0:
        break;
    case 0xb4:
        break;
    case 0xb8:
        break;

    case 0xc4:
        break;

    default:
        if ((data & 0xffff0000) == 0xffff0000)
            logerror("saturn_scu_w    %07x <- %04x (%s, PC=%08x)\n", offset, data & 0x0000ffff, SCU_REG_NAME(offset & 0x1ffff), cpu_get_reg(SH2_PC));
        else
        if ((data & 0xffff0000) == 0xff000000)
            logerror("saturn_scu_w    %07x <- xx%02x (%s, PC=%08x)\n", offset, data & 0x000000ff, SCU_REG_NAME(offset & 0x1ffff), cpu_get_reg(SH2_PC));
        else
            logerror("saturn_scu_w    %07x <- %02xxx (%s, PC=%08x)\n", offset, (data & 0x0000ff00) >> 8, SCU_REG_NAME(offset & 0x1ffff), cpu_get_reg(SH2_PC));
        break;
    }
}

/********************************************************
 *	Compact Disc
 ********************************************************/
struct cdblock_s {
	UINT16 hirq;
	UINT16 hm;
	UINT16 cr1;
	UINT16 cr2;
	UINT16 cr3;
	UINT16 cr4;
};

struct cdblock_s cdblock;

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
	logerror("saturn_cd_w     hirq %04x (PC=%08x)\n", data & 0x0000ffff, cpu_get_reg(SH2_PC));
	cdblock.hirq &= (1 | ~data);
	/* ### irqs */
}

static void do_command(void)
{
	switch(cdblock.cr1>>8)
	{
	case 0x00:
		LOG(("  do_command:   Get status\n"));
		break;
	case 0x01:
		LOG(("  do_command:   Get hardware info\n"));
		cdblock.cr1 = ST_NODISC << 8;
		cdblock.cr2 = 0;
		cdblock.cr3 = 0;
		cdblock.cr4 = 0;
		cdblock.hirq |= 0x0fe1;
		/* cdblock.hirq |= 0xffff; */
		break;
	case 0x02:
		LOG(("  do_command:   get TOC\n"));
		break;
	case 0x03:
		LOG(("  do_command:   get session info\n"));
		break;
	case 0x04:
		LOG(("  do_command:   initialize\n"));
		break;
	case 0x05:
		LOG(("  do_command:   open CD tray\n"));
		break;
	case 0x06:
		LOG(("  do_command:   end data transfer\n"));
		cdblock.cr1 = ST_NODISC<<8;
		cdblock.cr2 = 0;
		cdblock.cr3 = 0;
		cdblock.cr4 = 0;
		cdblock.hirq |= 0x0001;
		break;
	case 0x10:
		LOG(("  do_command:   play disk\n"));
		break;
	case 0x11:
		LOG(("  do_command:   disk seek\n"));
		break;
	case 0x12:
		LOG(("  do_command:   CD scan\n"));
		break;
	case 0x20:
		LOG(("  do_command:   get subcode\n"));
		break;
	case 0x30:
		LOG(("  do_command:   set device\n"));
		break;
	case 0x31:
		LOG(("  do_command:   get device\n"));
		break;
	case 0x32:
		LOG(("  do_command:   get last destination\n"));
		break;
	case 0x40:
		LOG(("  do_command:   set filter range\n"));
		break;
	case 0x41:
		LOG(("  do_command:   get filter range\n"));
		break;
	case 0x42:
		LOG(("  do_command:   set filter subheader conditions\n"));
		break;
	case 0x43:
		LOG(("  do_command:   get filter subheader conditions\n"));
		break;
	case 0x44:
		LOG(("  do_command:   set filter mode\n"));
		break;
	case 0x45:
		LOG(("  do_command:   get filter mode\n"));
		break;
	case 0x46:
		LOG(("  do_command:   set filter connexion\n"));
		break;
	case 0x47:
		LOG(("  do_command:   get filter connexion\n"));
		break;
	case 0x48:
		LOG(("  do_command:   reset selector\n"));
		break;
	case 0x50:
		LOG(("  do_command:   get CD block size\n"));
		break;
	case 0x51:
		LOG(("  do_command:   get buffer size\n"));
		break;
	case 0x52:
		LOG(("  do_command:   calculate actual size\n"));
		break;
	case 0x53:
		LOG(("  do_command:   get actual size\n"));
		break;
	case 0x54:
		LOG(("  do_command:   get sector info\n"));
		break;
	case 0x55:
		LOG(("  do_command:   execute FAD search\n"));
		break;
	case 0x56:
		LOG(("  do_command:   get FAD search results\n"));
		break;
	case 0x60:
		LOG(("  do_command:   get sector length\n"));
		break;
	case 0x61:
		LOG(("  do_command:   get sector data\n"));
		break;
	case 0x62:
		LOG(("  do_command:   delete sector data\n"));
		break;
	case 0x63:
		LOG(("  do_command:   get and delete sector data\n"));
		break;
	case 0x65:
		LOG(("  do_command:   copy sector data\n"));
		break;
	case 0x66:
		LOG(("  do_command:   move sector data\n"));
		break;
	case 0x67:
		LOG(("  do_command:   get copy error\n"));
		cdblock.cr1 = ST_STANDBY<<8;
		cdblock.cr2 = 0;
		cdblock.cr3 = 0;
		cdblock.cr4 = 0;
		/* cdblock.hirq |= 0xffff; */
		break;
	case 0x70:
		LOG(("  do_command:   change directory\n"));
		break;
	case 0x72:
		LOG(("  do_command:   get file system scope\n"));
		break;
	case 0x73:
		LOG(("  do_command:   get file info\n"));
		break;
	case 0x74:
		LOG(("  do_command:   read file\n"));
		break;
	case 0x75:
		LOG(("  do_command:   abort file\n"));
		cdblock.cr1 = ST_STANDBY<<8;
		cdblock.cr2 = 0;
		cdblock.cr3 = 0;
		cdblock.cr4 = 0;
		cdblock.hirq |= 0x0201;
		break;
	default:
		LOG(("  do_command:   executing command %02x\n", cdblock.cr1>>8));
		cdblock.cr1 = ST_STANDBY<<8;
		/* cdblock.cr2 = 0; */
		/* cdblock.cr3 = 0; */
		/* cdblock.cr4 = 0; */
		cdblock.hirq |= 0x0fe1;
		break;
	}
}

void cd_init(void)
{
	cdblock.cr1 =			 'C';
	cdblock.cr2 = ('D'<<8) | 'B';
	cdblock.cr3 = ('L'<<8) | 'O';
	cdblock.cr4 = ('C'<<8) | 'K';

	cdblock.hirq = 1;
	cdblock.hm = 0;
}

READ_HANDLER( saturn_cd_r )    /* CD */
{
	data_t data = 0x0000;

    /*  static int tick = 0; */
	switch(offset & 0xfffff)
    {
    case 0x90008:
		data = cdblock.hirq;
		logerror("saturn_cd_r     hirq %04x (PC=%08x)\n", data, cpu_get_reg(SH2_PC));
		break;
    case 0x90018:
		data = cdblock.cr1;
		logerror("saturn_cd_r     cr1  %04x (PC=%08x)\n", data, cpu_get_reg(SH2_PC));
		break;
    case 0x9001c:
		data = cdblock.cr2;
		logerror("saturn_cd_r     cr2  %04x (PC=%08x)\n", data, cpu_get_reg(SH2_PC));
		break;
    case 0x90020:
		data = cdblock.cr3;
		logerror("saturn_cd_r     cr3  %04x (PC=%08x)\n", data, cpu_get_reg(SH2_PC));
        break;
    case 0x90024:
		data = cdblock.cr4;
		logerror("saturn_cd_r     cr4  %04x (PC=%08x)\n", data, cpu_get_reg(SH2_PC));
		break;
    default:
		logerror("saturn_cd_r     %04x %04x (PC=%08x)\n", offset, data, cpu_get_reg(SH2_PC));
    }
	return data;
}

WRITE_HANDLER( saturn_cd_w )   /* CD */
{
	switch(offset & 0xfffff)
	{
    case 0x90008:
		hirq_w(data);
        break;
    case 0x90018:
		cdblock.cr1 = data;
        break;
    case 0x9001c:
		cdblock.cr2 = data;
        break;
    case 0x90020:
		cdblock.cr3 = data;
        break;
    case 0x90024:
		cdblock.cr4 = data;
        do_command();
        break;
    default:
		if ((data & 0xffff0000) == 0xffff0000)
			logerror("saturn_cd_w     %07x <- %04x (PC=%08x)\n", offset, data & 0x0000ffff, cpu_get_reg(SH2_PC));
		else
		if ((data & 0xffff0000) == 0xff000000)
			logerror("saturn_cd_w     %07x <- xx%02x (PC=%08x)\n", offset, (data & 0x000000ff), cpu_get_reg(SH2_PC));
		else
			logerror("saturn_cd_w     %07x <- %02xxx (PC=%08x)\n", offset, (data & 0x0000ff00) >> 8, cpu_get_reg(SH2_PC));
	}
}

/********************************************************
 *	FRT master
 ********************************************************/
READ_HANDLER( saturn_minit_r )  /* MINIT */
{
	data_t data = 0x0000;
	logerror("saturn_minit_r  %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_minit_w )  /* MINIT */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_minit_w  %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_minit_w  %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_minit_w  %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
    cpu_set_irq_line(0, 1, HOLD_LINE);
}

/********************************************************
 *	FRT slave
 ********************************************************/
READ_HANDLER( saturn_sinit_r )  /* SINIT */
{
	data_t data = 0x0000;
    logerror("saturn_sinit_r  %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_sinit_w )  /* SINIT */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_sinit_w  %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_sinit_w  %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_sinit_w  %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
    cpu_set_irq_line(1, 1, HOLD_LINE);
}

/********************************************************
 *	DSP
 ********************************************************/
READ_HANDLER( saturn_dsp_r )   /* DSP */
{
	data_t data = 0x0000;
	logerror("saturn_dsp_r    %07x -> %04x\n", offset, data);
	return data;
}

WRITE_HANDLER( saturn_dsp_w )  /* DSP */
{
	if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_dsp_w    %07x <- %04x\n", offset, data & 0x0000ffff);
	else
	if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_dsp_w    %07x <- xx%02x\n", offset, data & 0x000000ff);
	else
		logerror("saturn_dsp_w    %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
}

/********************************************************
 *	VDP1
 ********************************************************/
struct vdp1_s {
	UINT16 tv_mode;
	UINT16 fb_mode;
	UINT16 trigger;
	UINT16 ew_data;
	UINT16 ew_ul;
	UINT16 ew_lr;

	int line_bytes, line_count, line_pixels, pix_size;
	int ew_offset, ew_lines, ew_words, ew_jump;

	UINT8 *work_fb, *show_fb;
	int erase_next, trig;
};

struct vdp1_s vdp1;

const char* vdp1_reg_names[] =
{
	"tvmr", "fbcr", "ptmr", "ewdr", "ewlr", "ewrr", "endr", "-", "edsr", "lopr", "copr", "modr"
};

static void flip_fbs(void)
{
	if (vdp1.work_fb == &mem[SATURN_FB1_RAM_BASE])
	{
		vdp1.work_fb = &mem[SATURN_FB2_RAM_BASE];
		vdp1.show_fb = &mem[SATURN_FB1_RAM_BASE];
	}
	else
	{
		vdp1.work_fb = &mem[SATURN_FB1_RAM_BASE];
		vdp1.show_fb = &mem[SATURN_FB2_RAM_BASE];
	}
}

static void vdp1_calc_erase(void)
{
	int x1 = (vdp1.ew_ul >> 9) & 0x3f;
	int y1 = vdp1.ew_ul & 0x1ff;
	int x2 = (vdp1.ew_lr >> 9) & 0x7f;
	int y2 = vdp1.ew_lr & 0x1ff;

	switch (vdp1.tv_mode & 7)
	{
	case 0:
		vdp1.line_bytes = 1024;
		vdp1.line_count = 256;
		vdp1.line_pixels = 512;
		vdp1.pix_size = 2;
		break;
	case 1:
		vdp1.line_bytes = 1024;
		vdp1.line_count = 256;
		vdp1.line_pixels = 1024;
		vdp1.pix_size = 1;
		break;
	case 2:
		vdp1.line_bytes = 1024;
		vdp1.line_count = 256;
		vdp1.line_pixels = 512;
		vdp1.pix_size = 2;
		break;
	case 3:
		vdp1.line_bytes = 512;
		vdp1.line_count = 512;
		vdp1.line_pixels = 512;
		vdp1.pix_size = 1;
		break;
	default:
		vdp1.line_bytes = 1024;
		vdp1.line_count = 256;
		vdp1.line_pixels = 512;
		vdp1.pix_size = 2;
		break;
	}

	vdp1.ew_offset = x1 * 16 + y1 * vdp1.line_bytes;
	vdp1.ew_lines = y2 - y1 + 1;
	vdp1.ew_words = (x2 - x1) * 8;
	vdp1.ew_jump = vdp1.line_bytes - vdp1.ew_words * 2;
}

static void vdp1_erase_write(void)
{
	int i, j;
	UINT16 data = vdp1.ew_data;
	UINT16 *buf = (UINT16 *)(vdp1.show_fb + vdp1.ew_offset);
	int words = vdp1.ew_words;
	int lines = vdp1.ew_lines;
	int jump = vdp1.ew_jump;

	for (i = 0; i < lines; i++)
	{
		for (j = 0; j < words; j++)
			*buf++ = data;
		buf = (UINT16 *) (((UINT8 *) buf) + jump);
	}
}

#define EDC_RGB 0x7fff

static void sprite_rgb_replace_es_xy(UINT16 * src, INT16 sx, INT16 sy, INT16 x, INT16 y)
{
	UINT16 *dst = (UINT16 *) (vdp1.work_fb + (x << 1) + y * vdp1.line_bytes);
	INT16 xx, yy;

	for (yy = 0; yy < sy; yy++)
	{
		UINT16 *src1 = src;
		UINT16 *dst1 = dst;
		int ec = 0;

		for (xx = 0; xx < sx; xx++)
		{
			UINT16 color = *src1++;

			if (color == EDC_RGB)
			{
				if (ec)
					break;
				*dst1++ = 0x7c00;
			}
			else if (!color)
				dst1++;
			else
				*dst1++ = color;
		}
		src += sx;
		dst = (UINT16 *) (((UINT8 *) dst) + vdp1.line_bytes);
	}
	// logerror("sprite_rgb_replace_es_xy activated\n");
}

static void draw_commands(void)
{
    UINT8 *cmd = &mem[SATURN_VDP1_RAM_BASE];
	UINT32 offs = 0;
/*	UINT32 link = 0; */
	UINT32 back = 0;
	UINT16 scx = 0, scy = 0;
	UINT16 lx = 0, ly = 0;

	/* logerror("VDP1: draw commands\n"); */

	for (;;)
	{
		if (cmd[offs+0] & 0x80)
			break;

		if (!(cmd[offs+0] & 0x40))
		{
			switch (cmd[offs+1] & 15)
			{
			case 0:
				{
					UINT16 coff = 256 * cmd[offs+8] + cmd[offs+9];
					void *cadr = &mem[SATURN_VDP1_RAM_BASE + coff * 8];
					INT16 sx = cmd[offs+10] << 3;
					INT16 sy = cmd[offs+11];
					INT16 x = (256 * cmd[offs+12] + cmd[offs+13]) + lx;
					INT16 y = (256 * cmd[offs+14] + cmd[offs+15]) + ly;

					switch ((cmd[offs+5] >> 3) & 7)
					{					/* Color mode */
					case 5:
						switch (cmd[offs+5] & 7)
						{				/* Color calculation */
						case 0:
							switch (cmd[offs+5] & 0xc0)
							{
							case 0:
								switch (cmd[offs+1] & 0x30)
								{
								case 0:
									sprite_rgb_replace_es_xy(cadr, sx, sy, x, y);
									break;
								default:
									goto bail0;
								}
								break;
							default:
								goto bail0;
							}
							break;
						default:
							goto bail0;
						}
						break;
					default:
						goto bail0;
					}
					break;
				  bail0:
					LOG(("VDP1: sprite draw, ih=%d, iv=%d, x=%d, y=%d, sx=%d, sy=%d, adr=%08x\n",
						(cmd[offs+1] & 0x10) != 0, (cmd[offs+1] & 0x20) != 0, x, y, sx, sy, coff));
					LOG(("       clip=%d, uce=%d, ecd=%d, spd=%d, cmode=%d, ccalc=%d, mon=%d\n",
						   (cmd[offs+4] & 2) != 0,
						   (cmd[offs+4] & 1) != 0,
						   (cmd[offs+5] & 0x80) != 0,
						   (cmd[offs+5] & 0x40) != 0,
						   (cmd[offs+5] >> 3) & 7,
						   cmd[offs+5] & 7,
						   (cmd[offs+4] & 0x80) != 0));
					break;
				}
			case 1:
				LOG(("VDP1: scaled sprite draw\n"));
				break;
			case 2:
				{
					UINT32 coff = (256 * cmd[offs+8] + cmd[offs+9]) * 8;
					void *cadr = &mem[SATURN_VDP1_RAM_BASE + coff];
					INT16 x1 = (256 * cmd[offs+12] + cmd[offs+13]) + lx;
					INT16 y1 = (256 * cmd[offs+14] + cmd[offs+15]) + ly;
					INT16 x2 = (256 * cmd[offs+16] + cmd[offs+17]) + lx;
					INT16 y2 = (256 * cmd[offs+18] + cmd[offs+19]) + ly;
					INT16 x3 = (256 * cmd[offs+20] + cmd[offs+21]) + lx;
					INT16 y3 = (256 * cmd[offs+22] + cmd[offs+23]) + ly;
					INT16 x4 = (256 * cmd[offs+24] + cmd[offs+25]) + lx;
					INT16 y4 = (256 * cmd[offs+26] + cmd[offs+27]) + ly;
                    (void)cadr;
					LOG(("VDP1: distorted sprite draw\n"));
					LOG(("      (%d, %d)-(%d, %d)-(%d, %d)-(%d, %d)\n", x1, y1, x2, y2, x3, y3, x4, y4));
#if VERBOSE
					{
						int i;

						for (i = 0; i < 0x20; i++)
							logerror(" %02x", cmd[i]);
						logerror("\n");
					}
#endif
					LOG(("VDP1: distorded sprite draw, ih=%d, iv=%d, adr=%08x\n", (cmd[offs+1] & 0x10) != 0, (cmd[offs+1] & 0x20) != 0,
						   0x5c00000 | (cmd[0x08] << 11 | cmd[0x09] << 3)));
					LOG(("       clip=%d, uce=%d, ecd=%d, spd=%d, cmode=%d, ccalc=%d, mon=%d\n",
						   (cmd[offs+4] & 2) != 0,
						   (cmd[offs+4] & 1) != 0,
						   (cmd[offs+5] & 0x80) != 0,
						   (cmd[offs+5] & 0x40) != 0,
						   (cmd[offs+5] >> 3) & 7,
						   cmd[offs+5] & 7,
						   (cmd[offs+4] & 0x80) != 0));
					break;
				}
			case 4:
				LOG(("VDP1: polygon draw\n"));
				break;
			case 5:
				LOG(("VDP1: polyline draw\n"));
				break;
			case 6:
				LOG(("VDP1: line draw\n"));
				break;
			case 8:
				LOG(("VDP1: user clipping\n"));
				break;
			case 9:
				scx = 256 * cmd[offs+20] + cmd[offs+21];
				scy = 256 * cmd[offs+22] + cmd[offs+23];
				LOG(("VDP1: scx=%d, scy=%d\n", scx, scy));
                break;
			case 10:
				lx = 256 * cmd[offs+12] + cmd[offs+13];
				ly = 256 * cmd[offs+14] + cmd[offs+15];
				LOG(("VDP1: lx=%d, ly=%d\n", lx, ly));
                break;
			default:
				LOG(("VDP1: Unknown command %d\n", cmd[offs+1] & 15));
				break;
			}
		}

		switch (cmd[offs+0] & 0x30)
		{
		case 0x00:
			offs += 0x20;
			LOG(("VDP1: offs = next (%x)\n", offs));
			if (offs >= SATURN_VDP1_RAM_SIZE)
			{
				return;
			}
            break;
		case 0x10:
			offs = (256 * cmd[offs+2] + cmd[offs+3]) * 8;
			LOG(("VDP1: offs = link %x\n", offs));
            break;
        case 0x20:
            back = offs + 0x20;
			offs = (256 * cmd[offs+2] + cmd[offs+3]) * 8;
			LOG(("VDP1: offs = link %x, back = %x\n", offs, back));
            break;
		default:
			offs = back;
			LOG(("VDP1: offs = back (%x)\n", offs));
            break;
		}
	}
}

void vdp1_vblout_draw(void)
{
	int mode = vdp1.fb_mode & 3;

	if ((vdp1.tv_mode & 4) || vdp1.erase_next || (mode == 0))
		vdp1_erase_write();

    if ((mode == 0) || ((mode == 3) && vdp1.trig))
	{
		flip_fbs();
		if ((vdp1.trigger & 3) == 2)
			draw_commands();
	}

	vdp1.erase_next = (mode == 2) && vdp1.trig ? 1 : 0;
	vdp1.trig = 0;
	vdp1.tv_mode &= ~4;
}

#define VDP1_REG_NAME(adr) vdp1_reg_names[(adr)>>1]

READ_HANDLER( saturn_vdp1_r )   /* VDP1 registers */
{
	data_t data = 0x0000;
	logerror("saturn_vdp1_r   %07x -> %04x\n", offset, data);
    return data;
}

WRITE_HANDLER( saturn_vdp1_w )	/* VDP1 registers */
{
	switch(offset & 0x1ffff)
	{
	case 0x00:
		vdp1.tv_mode = data;
		logerror("saturn_vdp1_w   screen mode hdtv=%d, rotation=%d, depth=%d\n", (data&4)!=0, (data&2)!=0, (data&1) ? 8 : 16);
		vdp1_calc_erase();
		break;
	case 0x02:
		vdp1.fb_mode = data;
		vdp1.trig = 1;
		if(data & 12)
			logerror("saturn_vdp1_w   screen mode 2 die=%d, dil=%d\n", (data&8)!=0, (data&4)!=0);
		break;
	case 0x04:
		vdp1.trigger = data;
		if((data & 3)==1)
			draw_commands();
		break;
	case 0x06:
		vdp1.ew_data = data;
		break;
	case 0x08:
		vdp1.ew_ul = data;
		vdp1_calc_erase();
		break;
	case 0x0a:
		vdp1.ew_lr = data;
		vdp1_calc_erase();
		break;
	case 0x0c:
		logerror("saturn_vdp1_w   forced drawing termination\n");
		break;
	default:
		if ((data & 0xffff0000) == 0xffff0000)
			logerror("saturn_vdp1_w   %07x <- %04x\n", offset, data & 0x0000ffff);
		else
		if ((data & 0xffff0000) == 0xff000000)
			logerror("saturn_vdp1_w   %07x <- xx%02x\n", offset, data & 0x000000ff);
		else
			logerror("saturn_vdp1_w   %07x <- %02xxx\n", offset, (data & 0x0000ff00) >> 8);
    }
}

static void vdp1_init(void)
{
	vdp1.tv_mode = 0;
	vdp1.fb_mode = 0;
	vdp1.trigger = 0;
	vdp1.ew_data = 0;
	vdp1.ew_ul = 0;
	vdp1.ew_lr = 0;

	vdp1.trig = 0;
	vdp1.erase_next = 0;

	vdp1.work_fb = &mem[SATURN_FB1_RAM_BASE];
	vdp1.show_fb = &mem[SATURN_FB2_RAM_BASE];

	vdp1_calc_erase();
}

/********************************************************
 *	VDP2
 ********************************************************/

#define PLANE_SX (704+16)
#define PLANE_SY (512+16)

/* NTSC
 *	 Frame time  = 16.6833 ms (59.94Hz)
 *	 Line time	 = 63.5 us	  (15734.264Hz)
 *	 Line pixels = 858
 */

#define NTSC_FRAME 433783
#define NTSC_LPERIOD 1651
#define NTSC_HRES 858

#define SATURN_SCR_WIDTH	704
#define SATURN_SCR_HEIGHT	512

struct vdp2_s {
	UINT16 reg[144];
	int pal;
	UINT64 scr_clock;

	int resx, resy;
	int interlace;
	int hbl_in_clock, vbl_in_clock;
};

struct vdp2_s vdp2;

UINT32 colortab[2048];


static UINT32 lookup_0_15[256], lookup_1_15[256];

static UINT32 vdp2_plane0[PLANE_SX * PLANE_SY];
static UINT32 vdp2_plane1[PLANE_SX * PLANE_SY];
static UINT32 vdp2_plane2[PLANE_SX * PLANE_SY];
static UINT32 vdp2_plane3[PLANE_SX * PLANE_SY];
static UINT8 vdp2_pri0[PLANE_SX * PLANE_SY];
static UINT8 vdp2_pri1[PLANE_SX * PLANE_SY];
static UINT8 vdp2_pri2[PLANE_SX * PLANE_SY];
static UINT8 vdp2_pri3[PLANE_SX * PLANE_SY];

static int vdp2_nplanes;
static UINT32 *vdp2_dadr[4];
static UINT8 *vdp2_padr[4];

static void draw_normal_cell_scroll_plane(
	UINT32 * buf,
	UINT8 * pri,
	int cbpp,
	int cpsize,
	UINT16 patctrl,
	int psize,
	int mapoff,
	UINT16 planeab,
	UINT16 planecd,
	UINT16 scrollx,
	UINT16 scrolly,
	int coloff,
	int transp,
	UINT8 basepri)
{
#if VERBOSE
	printf("VDP2: draw cell scroll:\n");
	printf("        cbpp    = %d\n", cbpp);
	printf("        cpsize  = %d\n", cpsize != 0);
	printf("        patctrl = %x\n", patctrl);
	printf("        psize   = %d\n", psize);
	printf("        mapoff  = %d\n", mapoff);
	printf("        planeab = %x\n", planeab);
	printf("        planecd = %x\n", planecd);
	printf("        scrollx = %d\n", scrollx);
	printf("        scrolly = %d\n", scrolly);
	printf("        coloff  = %d\n", coloff);
	printf("        transp  = %d\n", transp == 0);
	printf("        pri     = %x\n", basepri);
#endif

	{
		int map_adr_shift;
		UINT8 *padra, *padrb, *padrc, *padrd;
//		rpat draw_f = vdp2_rend_get_function(cbpp, cpsize, psize, transp);

		vdp2_dadr[vdp2_nplanes] = buf;
		vdp2_padr[vdp2_nplanes] = pri;
		vdp2_nplanes++;

		if (cpsize)
			map_adr_shift = (patctrl & 0x8000) ? 11 : 12;
		else
			map_adr_shift = (patctrl & 0x8000) ? 13 : 14;

		mapoff <<= 6;
		coloff <<= 8;

		padra = &mem[SATURN_VDP2_RAM_BASE + ((((planeab >> 8) & 0x3f) | mapoff) << map_adr_shift)];
		padrb = &mem[SATURN_VDP2_RAM_BASE + ((((planeab) & 0x3f) | mapoff) << map_adr_shift)];
		padrc = &mem[SATURN_VDP2_RAM_BASE + ((((planecd >> 8) & 0x3f) | mapoff) << map_adr_shift)];
		padrd = &mem[SATURN_VDP2_RAM_BASE + ((((planecd) & 0x3f) | mapoff) << map_adr_shift)];

//		draw_f(buf, pri, padra, coloff, 44, 32, basepri, 0xffff);
	}
}

static void vdp2_vblout_draw(struct osd_bitmap * bitmap)
{
	if (!vdp2.interlace)
	{

#if VERBOSE
		static int nc[8] = {4, 8, 11, 15, 24, 0, 0, 0};
		static int rx[2] = {512, 1024};
		static int ry[2] = {256, 512};

		switch (vdp2.reg[7] & 0x3000)
		{
		case 0x0000:
			break;
		case 0x1000:
			printf("VDP2: color mode 2048x16\n");
			break;
		case 0x2000:
			printf("VDP2: color mode 1024x32\n");
			break;
		}
		printf("VDP2: enabled %cN0%c %cN1%c %cN2%c %cN3%c %cR0%c %cR1o\n",
			   vdp2.reg[0x10] & 1 ? '+' : '-',
			   vdp2.reg[0x10] & 0x100 ? 'o' : 't',
			   vdp2.reg[0x10] & 2 ? '+' : '-',
			   vdp2.reg[0x10] & 0x200 ? 'o' : 't',
			   vdp2.reg[0x10] & 4 ? '+' : '-',
			   vdp2.reg[0x10] & 0x400 ? 'o' : 't',
			   vdp2.reg[0x10] & 8 ? '+' : '-',
			   vdp2.reg[0x10] & 0x800 ? 'o' : 't',
			   vdp2.reg[0x10] & 16 ? '+' : '-',
			   vdp2.reg[0x10] & 0x1000 ? 'o' : 't',
			   vdp2.reg[0x10] & 32 ? '+' : '-');
		printf("VDP2: colors N0/R1=%dx%dx%d%c, N1=%dx%dx%d%c, N2=%d%c, N3=%d%c, R0=512x%dx%d%c\n",

			   rx[(vdp2.reg[0x14] >> 3) & 1],
			   ry[(vdp2.reg[0x14] >> 2) & 1],
			   nc[(vdp2.reg[0x14] >> 4) & 7],
			   vdp2.reg[0x14] & 2 ? 'b' : vdp2.reg[0x14] & 1 ? 'C' : 'c',

			   rx[(vdp2.reg[0x14] >> 11) & 1],
			   ry[(vdp2.reg[0x14] >> 10) & 1],
			   nc[(vdp2.reg[0x14] >> 12) & 3],
			   vdp2.reg[0x14] & 0x200 ? 'b' : vdp2.reg[0x14] & 0x100 ? 'C' : 'c',

			   nc[(vdp2.reg[0x15] >> 1) & 1],
			   vdp2.reg[0x15] & 1 ? 'C' : 'c',

			   nc[(vdp2.reg[0x15] >> 5) & 1],
			   vdp2.reg[0x15] & 0x10 ? 'C' : 'c',

			   ry[(vdp2.reg[0x15] >> 9) & 1],
			   nc[(vdp2.reg[0x14] >> 12) & 7],
			   vdp2.reg[0x15] & 0x200 ? 'b' : vdp2.reg[0x15] & 0x100 ? 'C' : 'c');
		printf("VDP2: sprite plane mode=%c\n",
			   vdp2.reg[0x70] & 0x20 ? 'r' : 'p');

#endif

//		vdp2_rend_colors(vdp2.reg[7] & 0x3000);
		vdp2_nplanes = 0;

		/* Plane 0 */
		if (vdp2.reg[0x10] & 1)
			draw_normal_cell_scroll_plane(
				vdp2_plane0, vdp2_pri0,
				(vdp2.reg[0x14] >> 4) & 7,
				vdp2.reg[0x14] & 1,
				vdp2.reg[0x18],
				vdp2.reg[0x1d] & 3,
				vdp2.reg[0x1e] & 7,
				vdp2.reg[0x20],
				vdp2.reg[0x21],
				vdp2.reg[0x38],
				vdp2.reg[0x3a],
				vdp2.reg[0x72] & 7,
				vdp2.reg[0x10] & 0x100,
				((vdp2.reg[0x7c] & 0x7) << 4) | 3);
		/* Plane 1 */
		if (vdp2.reg[0x10] & 2)
			draw_normal_cell_scroll_plane(
				vdp2_plane1, vdp2_pri1,
				(vdp2.reg[0x14] >> 12) & 3,
				vdp2.reg[0x14] & 0x100,
				vdp2.reg[0x19],
				(vdp2.reg[0x1d] >> 2) & 3,
				(vdp2.reg[0x1e] >> 4) & 7,
				vdp2.reg[0x22],
				vdp2.reg[0x23],
				vdp2.reg[0x40],
				vdp2.reg[0x42],
				(vdp2.reg[0x72] >> 4) & 7,
				vdp2.reg[0x10] & 0x200,
				((vdp2.reg[0x7c] & 0x700) >> 4) | 2);
		/* Plane 2 */
		if (vdp2.reg[0x10] & 4)
			draw_normal_cell_scroll_plane(
				vdp2_plane2, vdp2_pri2,
				(vdp2.reg[0x15] >> 1) & 1,
				vdp2.reg[0x15] & 1,
				vdp2.reg[0x1a],
				(vdp2.reg[0x1d] >> 4) & 3,
				(vdp2.reg[0x1e] >> 8) & 7,
				vdp2.reg[0x24],
				vdp2.reg[0x25],
				vdp2.reg[0x48],
				vdp2.reg[0x49],
				(vdp2.reg[0x72] >> 8) & 7,
				vdp2.reg[0x10] & 0x400,
				((vdp2.reg[0x7d] & 0x7) << 4) | 1);
		/* Plane 3 */
		if (vdp2.reg[0x10] & 8)
			draw_normal_cell_scroll_plane(
				vdp2_plane3, vdp2_pri3,
				(vdp2.reg[0x15] >> 5) & 1,
				vdp2.reg[0x15] & 0x10,
				vdp2.reg[0x1b],
				(vdp2.reg[0x1d] >> 6) & 3,
				(vdp2.reg[0x1e] >> 12) & 7,
				vdp2.reg[0x26],
				vdp2.reg[0x27],
				vdp2.reg[0x4a],
				vdp2.reg[0x4b],
				(vdp2.reg[0x72] >> 12) & 7,
				vdp2.reg[0x10] & 0x800,
				(vdp2.reg[0x7d] & 0x700) >> 4);

	}
	else
		printf("VDP2: interlaced screen\n");

	/* Sprite plane */
#if 0
	{
		UINT8 *src = vdp1.show_fb;
		int x, y;

		for (y = 0; y < 224; y++)
		{
			for (x = 0; x < 320; x++)
			{
				if (*src & 0x80)
				{
					UINT16 color = lookup_0_15[src[0]];

					color |= lookup_1_15[src[1]];
					*buf++ = color;
					*buf++ = color;
				}
				else
					buf += 2;
				src += 2;
			}
			buf += SATURN_SCR_WIDTH * 2 - 320 * 2;
			src += vdp1.line_bytes - 320 * 2;
		}
	}
#endif
	if (vdp2_nplanes)
	{
		int x, y, i;
		UINT8 *spr_src = vdp1.show_fb;
		int offset = 0;
		int spr_offset = 0;

		for (y = 0; y < SATURN_SCR_HEIGHT / 2; y++)
		{
			UINT32 *rbuf = (UINT32 *)&bitmap->line[y];
            int offset1 = offset;
			int spr_offset1 = spr_offset;

			for (x = 0; x < SATURN_SCR_WIDTH / 2; x++)
			{
				UINT32 color = lookup_0_15[spr_src[spr_offset1]] | lookup_1_15[spr_src[spr_offset1 + 1]];
				UINT8 cpri = spr_src[spr_offset1] & 0x80 ? 255 : 0;

				for (i = 0; i < vdp2_nplanes; i++)
				{
					UINT8 cpri1 = vdp2_padr[i][offset1];

					if (cpri1 > cpri)
					{
						cpri = cpri1;
						color = vdp2_dadr[i][offset1];
					}
				}
				*rbuf++ = color;
				*rbuf++ = color;
				offset1++;
				spr_offset1 += 2;
			}
			offset += PLANE_SX;
			spr_offset += vdp1.line_bytes;
		}
	}
}

static void vdp2_vblout_clean(struct osd_bitmap *bitmap)
{
	fillbitmap(bitmap, Machine->pens[0], NULL);
}

void *vbl_out;
void *vbl_in;

void f_vbl_in(int param)
{
	scu_pulse_interrupt(INT_VBLIN);
}

void f_vbl_out(int param)
{
	static int count = 0;
	struct osd_bitmap *bitmap = saturn_bitmap[video_w];
	static int blank = 0;

	if (count++ == 10)
	{
		vdp1_vblout_draw();
		if (vdp2.reg[0] & 0x8000)
		{
			vdp2_vblout_draw(bitmap);
			blank = 0;
			video_w ^= 1;
		}
		else
		{
			if (!blank)
			{
				vdp2_vblout_clean(bitmap);
				video_w ^= 1;
				blank = 1;
			}
		}
		count = 0;
	}

	vdp2.scr_clock += NTSC_FRAME;
	vbl_out = timer_set(TIME_IN_CYCLES(NTSC_FRAME,0), 0, f_vbl_out);
	vbl_in = timer_set(TIME_IN_CYCLES(vdp2.vbl_in_clock,0), 0, f_vbl_in);;
	scu_pulse_interrupt(INT_VBLOUT);
}

static void vdp2_calc_res(void)
{
	static int vres[4] = {224, 240, 256, 256};
	static int hres[4] = {320, 352, 640, 704};
	UINT16 tvmd = vdp2.reg[0];

	vdp2.interlace = (tvmd & 0x80) != 0;
	vdp2.resx = hres[tvmd & 3];
	vdp2.resy = vres[(tvmd >> 4) & 3] * (vdp2.interlace ? 2 : 1);

	vdp2.hbl_in_clock = ((vdp2.resx < 600 ? 2 * vdp2.resx : vdp2.resx) * NTSC_LPERIOD) / NTSC_HRES;
	vdp2.vbl_in_clock = (vdp2.resy - 1) * NTSC_LPERIOD + vdp2.hbl_in_clock;

	printf("VDP2: Graphic mode %dx%d%s, hin=%d, vin=%d\n",
		   vdp2.resx,
		   vdp2.resy,
		   vdp2.interlace ? " (interlaced)" : "",
		   vdp2.hbl_in_clock,
		   vdp2.vbl_in_clock);
}

static const char *vdp2_reg_names[] =
{
	"tvmd",   "exten",  "tvstat", "vrsize", "hcnt",   "vcnt",   "-",      "ramctl",
	"cyca0l", "cyca0u", "cyca1l", "cyca1u", "cycb0l", "cycb1u", "cycb1l", "cycb1u",
	"bgon",   "mzctl",  "sfsel",  "sfcode", "chctla", "chctlb", "bmpna",  "bmpnb",
	"pncn0",  "pncn1",  "pncn2",  "pncn3",  "pncr",   "plsz",   "mpofn",  "mpofr",
	"mpabn0", "mpcdn0", "mpabn1", "mpcdn1", "mpabn2", "mpcdn2", "mpabn3", "mpcdn3",
	"mpabra", "mpcdra", "mpefra", "mpghra", "mpijra", "mpklra", "mpmnra", "mpopra",
	"mpabrb", "mpcdrb", "mpefrb", "mpghrb", "mpijrb", "mpklrb", "mpmnrb", "mpoprb",
	"scxin0", "scxdn0", "scyin0", "scydn0", "zmxin0", "zmxdn0", "zmyin0", "zmydn0",
	"scxin1", "scxdn1", "scyin1", "scydn1", "zmxin1", "zmxdn1", "zmyin1", "zmydn1",
	"scxn2",  "scyn2",  "scxn3",  "scyn3",  "zmctl",  "scrctl", "vcstau", "vcstal",
	"lsta0u", "lsta0l", "lsta1u", "lsta1l", "lctau",  "lctal",  "bktau",  "bktal",
	"rpmd",   "rprctl", "ktctl",  "ktaof",  "ovpnra", "ovpnrb", "rptau",  "rptal",
	"wpsx0",  "wpsy0",  "wpex0",  "wpey0",  "wpsx1",  "wpsy1",  "wpex1",  "wpey1",
	"wctla",  "wctlb",  "wctlc",  "wctld",  "lwta0u", "lwta0l", "lwta1u", "lwta1l",
	"spctl",  "sdctl",  "craofa", "craofb", "lnclen", "sfprmd", "ccctl",  "sfccmd",
	"prisa",  "prisb",  "prisc",  "prisd",  "prina",  "prinb",  "prir",   "-",
	"ccrsa",  "ccrsb",  "ccrsc",  "ccrsd",  "ccrna",  "ccrnb",  "ccrr",   "ccrlb",
	"clofen", "clofsl", "coar",   "coag",   "coab",   "cobr",   "cobg",   "cobb"
};

#define VDP2_REG_NAME(adr) ((adr < 144) ? vdp2_reg_names[(adr)>>1] : "n/a")


void vdp2_init(void)
{
	int i;

	for (i = 0; i < 144; i++)
		vdp2.reg[i] = 0;

	vdp2.pal = 0;
	vdp2.scr_clock = 0;

	vdp2_calc_res();

	vbl_out = timer_set(TIME_IN_CYCLES(NTSC_FRAME,0), 0, f_vbl_out);

	for (i = 0; i < 256; i++)
	{
		UINT16 color;

		color = i << 8;
		lookup_0_15[i] = ((color & 0x7c00) >> 7) | ((color & 0x03e0) << 6) | ((color & 0x001f) << 19);
		color = i;
		lookup_1_15[i] = ((color & 0x7c00) >> 7) | ((color & 0x03e0) << 6) | ((color & 0x001f) << 19);
	}
}

READ_HANDLER( saturn_vdp2_r )   /* VDP2 registers */
{
	data_t data = 0x0000;
	switch (offset & 0x3ffff)
    {
    case 0x04:
        // ### tvstat, some fields to add.
		data = vdp2.pal | 2;
        break;
    }
	logerror("saturn_vdp2_r   %07x -> %04x (%s, PC=%08x)\n", offset, data, VDP2_REG_NAME(offset), cpu_get_reg(SH2_PC));
    return data;
}

WRITE_HANDLER( saturn_vdp2_w )	/* VDP2 registers */
{
	data_t oldword = READ_WORD(&vdp2.reg[offset >> 1]);
    data_t newword = COMBINE_WORD(oldword, data);
	vdp2.reg[offset >> 1] = newword;

    if ((data & 0xffff0000) == 0xffff0000)
		logerror("saturn_vdp2_w   %07x <- %04x (%s, PC=%08x)\n", offset, data & 0x0000ffff, VDP2_REG_NAME(offset), cpu_get_reg(SH2_PC));
    else
    if ((data & 0xffff0000) == 0xff000000)
		logerror("saturn_vdp2_w   %07x <- xx%02x (%s, PC=%08x)\n", offset, data & 0x000000ff, VDP2_REG_NAME(offset), cpu_get_reg(SH2_PC));
    else
		logerror("saturn_vdp2_w   %07x <- %02xxx (%s, PC=%08x)\n", offset, (data & 0x0000ff00) >> 8, VDP2_REG_NAME(offset), cpu_get_reg(SH2_PC));
    switch (offset & 0x3ffff)
    {
    case 0x000:
		if (newword & 0x8000)
            printf("VDP2: TV screen enabled\n");
        else
            printf("VDP2: TV screen disabled\n");
		vdp2_calc_res();
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
	vdp1_init();
	vdp2_init();
}


int saturn_vh_start(void)
{
	saturn_bitmap[0] = osd_alloc_bitmap(SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT, 16);
	saturn_bitmap[1] = osd_alloc_bitmap(SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT, 16);

    if (!saturn_bitmap[0] || !saturn_bitmap[1])
		return 1;

    return 0;
}

void saturn_vh_stop(void)
{
	if (saturn_bitmap[0])
		osd_free_bitmap(saturn_bitmap[0]);
	saturn_bitmap[0] = NULL;
	if (saturn_bitmap[1])
		osd_free_bitmap(saturn_bitmap[1]);
	saturn_bitmap[1] = NULL;
}

void saturn_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	copybitmap(bitmap, saturn_bitmap[video_w], 0, 0, 0, 0, NULL, TRANSPARENCY_NONE, 0);
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

void saturn_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for ( i = 0; i < 0x8000; i++ )
	{
		int r, g, b;

		r = (( i >> 10 ) & 0x1f) << 3;
		g = (( i >> 5 ) & 0x1f) << 3;
		b = (i & 0x1f) << 3;

		*palette++ = r;
		*palette++ = g;
		*palette++ = b;

		colortable[i] = i;
	}
}


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

	/* video hardware */
	SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT,
	{ 0, SATURN_SCR_WIDTH - 1, 0, SATURN_SCR_HEIGHT - 1},
	NULL,
	32768, 32768,
	saturn_init_palette,	/* convert color prom */

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
CONSX(1992, saturn,   0,		saturn,   saturn,	saturn,   "Sega",   "Saturn", GAME_NOT_WORKING | GAME_REQUIRES_16BIT )


