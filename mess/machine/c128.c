/***************************************************************************
    commodore c128 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation:
 	 iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/cia6526.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/vic6567.h"
#include "includes/vdc8563.h"
#include "includes/sid6581.h"
#include "mscommon.h"

#include "includes/c128.h"


/*
 two cpus share 1 memory system, only 1 cpu can run
 controller is the mmu

 mame has different memory subsystems for each cpu
*/


/* m8502 port
 in c128 mode
 bit 0 color
 bit 1 lores
 bit 2 hires
 3 textmode
 5 graphics (turned on ram at 0x1000 for video chip
*/

UINT8 *c128_basic;
UINT8 *c128_kernal;
UINT8 *c128_chargen;
UINT8 *c128_z80;
UINT8 *c128_editor;
UINT8 *c128_internal_function;
UINT8 *c128_external_function;
UINT8 *c128_vdcram;

static int c64mode = 0, c128_write_io;

static int c128_ram_bottom, c128_ram_top;
static UINT8 *c128_ram;

int c128_capslock_r (void)
{
	return !KEY_DIN;
}

static WRITE_HANDLER(c128_dma8726_port_w)
{
	DBG_LOG(1,"dma write",("%.3x %.2x\n",offset,data));
}

static READ_HANDLER(c128_dma8726_port_r)
{
	DBG_LOG(1,"dma read",("%.3x\n",offset));
	return 0xff;
}

WRITE_HANDLER( c128_mmu8722_port_w );
READ_HANDLER( c128_mmu8722_port_r );
WRITE_HANDLER( c128_write_d000 )
{
	if (!c128_write_io) {
		if (offset + 0xd000 >= c128_ram_top)
			c64_memory[0xd000 + offset] = data;
		else
			c128_ram[0xd000 + offset] = data;
	} else {
		switch ((offset&0xf00)>>8) {
		case 0:case 1: case 2: case 3:
			vic2_port_w (offset & 0x3ff, data);
			break;
		case 4:
			sid6581_0_port_w (offset & 0x3f, data);
			break;
		case 5:
			c128_mmu8722_port_w (offset & 0xff, data);
			break;
		case 6:case 7:
			vdc8563_port_w (offset & 0xff, data);
			break;
		case 8: case 9: case 0xa: case 0xb:
		    if (c64mode)
			c64_colorram[(offset & 0x3ff)] = data | 0xf0;
		    else
			c64_colorram[(offset & 0x3ff)|((c64_port6510&3)<<10)] = data | 0xf0; // maybe all 8 bit connected!
		    break;
		case 0xc:
			cia6526_0_port_w (offset & 0xff, data);
			break;
		case 0xd:
			cia6526_1_port_w (offset & 0xff, data);
			break;
		case 0xf:
			c128_dma8726_port_w(offset&0xff,data);
			break;
		case 0xe:
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
			break;
		}
	}
}

static READ_HANDLER( c128_read_io )
{
	switch ((offset&0xf00)>>8) {
	case 0:case 1: case 2: case 3:
		return vic2_port_r (offset & 0x3ff);
	case 4:
		return sid6581_0_port_r (offset & 0xff);
	case 5:
		return c128_mmu8722_port_r (offset & 0xff);
	case 6:case 7:
		return vdc8563_port_r (offset & 0xff);
	case 8: case 9: case 0xa: case 0xb:
		return c64_colorram[offset & 0x3ff];
	case 0xc:
		return cia6526_0_port_r (offset & 0xff);
	case 0xd:
		return cia6526_1_port_r (offset & 0xff);
	case 0xf:
		return c128_dma8726_port_r(offset&0xff);
	case 0xe:default:
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		return 0xff;
	}
}

void c128_bankswitch_64 (int reset)
{
	static int old, exrom, game;
	int data, loram, hiram, charen;

	if (!c64mode)
		return;

	data = ((c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff)) & 7;
	if ((data == old)&&(exrom==c64_exrom)&&(game==c64_game)&&!reset)
		return;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		cpu_setbank (8, c64_roml);
	}
	else
	{
		cpu_setbank (8, c64_memory + 0x8000);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		cpu_setbank (9, c64_romh);
	}
	else if (loram && hiram)
	{
		cpu_setbank (9, c64_basic);
	}
	else
	{
		cpu_setbank (9, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		memory_set_bankhandler_r (13, 0, c128_read_io);
		c128_write_io = 1;
	}
	else
	{
		memory_set_bankhandler_r (13, 0, MRA8_BANK5);
		c128_write_io = 0;
		if ((!charen && (loram || hiram)))
		{
			cpu_setbank (13, c64_chargen);
		}
		else
		{
			cpu_setbank (13, c64_memory + 0xd000);
		}
	}

	if (!c64_game && c64_exrom)
	{
		cpu_setbank (14, c64_romh);
		cpu_setbank (15, c64_romh+0x1f00);
		cpu_setbank (16, c64_romh+0x1f05);
	}
	else
	{
		if (hiram)
		{
			cpu_setbank (14, c64_kernal);
			cpu_setbank (15, c64_kernal+0x1f00);
			cpu_setbank (16, c64_kernal+0x1f05);
		}
		else
		{
			cpu_setbank (14, c64_memory + 0xe000);
			cpu_setbank (15, c64_memory + 0xff00);
			cpu_setbank (16, c64_memory + 0xff05);
		}
	}
	old = data;
	exrom= c64_exrom;
	game=c64_game;
}

UINT8 c128_mmu[0x0b];
static int c128_mmu_helper[4] =
{0x400, 0x1000, 0x2000, 0x4000};
static int mmu_cpu=0;
static int mmu_page0, mmu_page1;

#define MMU_PAGE1 ((((c128_mmu[10]&0xf)<<8)|c128_mmu[9])<<8)
#define MMU_PAGE0 ((((c128_mmu[8]&0xf)<<8)|c128_mmu[7])<<8)
#define MMU_VIC_ADDR ((c128_mmu[6]&0xc0)<<10)
#define MMU_RAM_RCR_ADDR ((c128_mmu[6]&0x30)<<14)
#define MMU_SIZE (c128_mmu_helper[c128_mmu[6]&3])
#define MMU_BOTTOM (c128_mmu[6]&4)
#define MMU_TOP (c128_mmu[6]&8)
#define MMU_CPU8502 (c128_mmu[5]&1)	   /* else z80 */
/* fastio output (c128_mmu[5]&8) else input */
#define MMU_GAME_IN (c128_mmu[5]&0x10)
#define MMU_EXROM_IN (c128_mmu[5]&0x20)
#define MMU_64MODE (c128_mmu[5]&0x40)
#define MMU_40_IN (c128_mmu[5]&0x80)

#define MMU_RAM_CR_ADDR ((c128_mmu[0]&0xc0)<<10)
#define MMU_RAM_LO (c128_mmu[0]&2)	   /* else rom at 0x4000 */
#define MMU_RAM_MID ((c128_mmu[0]&0xc)==0xc)	/* 0x8000 - 0xbfff */
#define MMU_ROM_MID ((c128_mmu[0]&0xc)==0)
#define MMU_EXTERNAL_ROM_MID ((c128_mmu[0]&0xc)==8)
#define MMU_INTERNAL_ROM_MID ((c128_mmu[0]&0xc)==4)

#define MMU_IO_ON (!(c128_mmu[0]&1))   /* io window at 0xd000 */
#define MMU_ROM_HI ((c128_mmu[0]&0x30)==0)	/* rom at 0xc000 */
#define MMU_EXTERNAL_ROM_HI ((c128_mmu[0]&0x30)==0x20)
#define MMU_INTERNAL_ROM_HI ((c128_mmu[0]&0x30)==0x10)
#define MMU_RAM_HI ((c128_mmu[0]&0x30)==0x30)

#define MMU_RAM_ADDR (MMU_RAM_RCR_ADDR|MMU_RAM_CR_ADDR)

/* typical z80 configuration
   0x3f 0x3f 0x7f 0x3e 0x7e 0xb0 0x0b 0x00 0x00 0x01 0x00 */
 static void c128_bankswitch_z80 (void)
 {
	 c128_ram = c64_memory + MMU_RAM_ADDR;
	 c128_va1617 = MMU_VIC_ADDR;
#if 1
	 cpu_setbank (10, c128_z80);
	 cpu_setbank (11, c128_ram+0x1000);
	 if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
		  ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) )
		 c128_ram=NULL;
#else
	 if (MMU_BOTTOM)
		 c128_ram_bottom = MMU_SIZE;
	 else
		 c128_ram_bottom = 0;

	 if (MMU_RAM_ADDR==0) { /* this is used in z80 mode for rom on/off switching !*/
		 cpu_setbank (10, c128_z80);
		 cpu_setbank (11, c128_z80+0x400);
	 } else {
		 cpu_setbank (10, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
		 cpu_setbank (11, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
	 }
	 cpu_setbank (1, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
	 cpu_setbank (2, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);

	 cpu_setbank (3, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
	 cpu_setbank (4, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);
	 cpu_setbank (5, c128_ram + 0x4000);

	 if (MMU_TOP)
		 c128_ram_top = 0x10000 - MMU_SIZE;
	 else
		 c128_ram_top = 0x10000;

	 if (c128_ram_top > 0xc000) { cpu_setbank (6, c128_ram + 0xc000); }
	 else { cpu_setbank (6, c64_memory + 0xc000); }
	 if (c128_ram_top > 0xe000) { cpu_setbank (7, c128_ram + 0xe000); }
	 else { cpu_setbank (7, c64_memory + 0xd000); }
	 if (c128_ram_top > 0xf000) { cpu_setbank (8, c128_ram + 0xf000); }
	 else { cpu_setbank (8, c64_memory + 0xe000); }
	 if (c128_ram_top > 0xff05) { cpu_setbank (9, c128_ram + 0xff05); }
	 else { cpu_setbank (9, c64_memory + 0xff05); }

	 if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
		  ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) )
		 c128_ram=NULL;
#endif
 }

 static void c128_bankswitch_128 (int reset)
 {
	c64mode = MMU_64MODE;
	if (c64mode)
	{
		/* mmu works also in c64 mode, but this can wait */
		c128_ram = c64_memory;
		c128_va1617 = 0;
		c128_ram_bottom = 0;
		c128_ram_top = 0x10000;

		cpu_setbank (1, c64_memory);
		cpu_setbank (2, c64_memory + 0x100);

		cpu_setbank (3, c64_memory + 0x200);
		cpu_setbank (4, c64_memory + 0x400);
		cpu_setbank (5, c64_memory + 0x1000);
		cpu_setbank (6, c64_memory + 0x2000);

		cpu_setbank (7, c64_memory + 0x4000);

		cpu_setbank (12, c64_memory + 0xc000);

		c128_bankswitch_64 (reset);
	}
	else
	{
		c128_ram = c64_memory + MMU_RAM_ADDR;
		c128_va1617 = MMU_VIC_ADDR;
		cpu_setbank (1, c64_memory + mmu_page0);
		cpu_setbank (2, c64_memory + mmu_page1);
		if (MMU_BOTTOM)
			{
				c128_ram_bottom = MMU_SIZE;
			}
		else
			c128_ram_bottom = 0;
		cpu_setbank (3, (c128_ram_bottom > 0x200 ? c64_memory : c128_ram) + 0x200);
		cpu_setbank (4, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
		cpu_setbank (5, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
		cpu_setbank (6, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);

		if (MMU_RAM_LO)
			{
				cpu_setbank (7, c128_ram + 0x4000);
			}
		else
			{
				cpu_setbank (7, c128_basic);
			}

		if (MMU_RAM_MID)
			{
				cpu_setbank (8, c128_ram + 0x8000);
				cpu_setbank (9, c128_ram + 0xa000);
			}
		else if (MMU_ROM_MID)
			{
				cpu_setbank (8, c128_basic + 0x4000);
				cpu_setbank (9, c128_basic + 0x6000);
			}
		else if (MMU_INTERNAL_ROM_MID)
			{
				cpu_setbank (8, c128_internal_function);
				cpu_setbank (9, c128_internal_function + 0x2000);
			}
		else
			{
				cpu_setbank (8, c128_external_function);
				cpu_setbank (9, c128_external_function + 0x2000);
			}

		if (MMU_TOP)
			{
				c128_ram_top = 0x10000 - MMU_SIZE;
			}
		else
			c128_ram_top = 0x10000;

		memory_set_bankhandler_r (15, 0, c128_mmu8722_ff00_r);

		if (MMU_IO_ON)
			{
				memory_set_bankhandler_r (13, 0, c128_read_io);
				c128_write_io = 1;
			}
		else
			{
				c128_write_io = 0;
				memory_set_bankhandler_r (13, 0, MRA_BANK13);
			}

		if (MMU_RAM_HI)
			{
				if (c128_ram_top > 0xc000)
					{
						cpu_setbank (12, c128_ram + 0xc000);
					}
				else
					{
						cpu_setbank (12, c64_memory + 0xc000);
					}
				if (!MMU_IO_ON) {
					if (c128_ram_top > 0xd000)
						{
							cpu_setbank (13, c128_ram + 0xd000);
						}
					else
						{
							cpu_setbank (13, c64_memory + 0xd000);
						}
				}
				if (c128_ram_top > 0xe000)
					{
						cpu_setbank (14, c128_ram + 0xe000);
					}
				else
					{
						cpu_setbank (14, c64_memory + 0xe000);
					}
				if (c128_ram_top > 0xff05)
					{
						cpu_setbank (16, c128_ram + 0xff05);
					}
				else
					{
						cpu_setbank (16, c64_memory + 0xff05);
					}
			}
		else if (MMU_ROM_HI)
			{
				cpu_setbank (12, c128_editor);
				if (!MMU_IO_ON) {
					cpu_setbank (13, c128_chargen);
				}
				cpu_setbank (14, c128_kernal);
				cpu_setbank (16, c128_kernal + 0x1f05);
			}
		else if (MMU_INTERNAL_ROM_HI)
			{
				cpu_setbank (12, c128_internal_function);
				if (!MMU_IO_ON) {
					cpu_setbank (13, c128_internal_function + 0x1000);
				}
				cpu_setbank (14, c128_internal_function + 0x2000);
				cpu_setbank (16, c128_internal_function + 0x3f05);
			}
		else					   /*if (MMU_EXTERNAL_ROM_HI) */
			{
				cpu_setbank (12, c128_external_function);
				if (!MMU_IO_ON) {
					cpu_setbank (13, c128_external_function + 0x1000);
				}
				cpu_setbank (14, c128_external_function + 0x2000);
				cpu_setbank (16, c128_external_function + 0x3f05);
			}
		if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
			 ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) ) c128_ram=NULL;
	}

 }


static void c128_bankswitch (int reset)
{
	if (mmu_cpu != MMU_CPU8502) {
		if (!MMU_CPU8502)
			{
				{
					DBG_LOG (1, "switching to z80",
							 ("active %d\n",cpu_getactivecpu()) );
					memory_set_context(0);
					c128_bankswitch_z80();
					memory_set_context(1);
					cpu_set_halt_line (0, 0);
					cpu_set_halt_line (1, 1);
				}
			}
		else
			{
				DBG_LOG (1, "switching to m6502",
						 ("active %d\n",cpu_getactivecpu()) );
				memory_set_context(1);
				c128_bankswitch_128(reset);
				memory_set_context(0);
				cpu_set_halt_line (1, 0);
				cpu_set_halt_line (0, 1);
			}
		mmu_cpu = MMU_CPU8502;
		return;
	}
	if (!MMU_CPU8502) {
		c128_bankswitch_z80();
	} else {
		c128_bankswitch_128(reset);
	}
}

static void c128_mmu8722_reset (void)
{
#if 0
	cpu_setbank (1, c64_memory);
	cpu_setbank (2, c64_memory + 0x100);

	cpu_setbank (3, c64_memory + 0x200);
	cpu_setbank (4, c64_memory + 0x400);
	cpu_setbank (5, c64_memory + 0x1000);
	cpu_setbank (6, c64_memory + 0x2000);
	cpu_setbank (7, c64_memory + 0x4000);

	c128_mmu[0] &= ~1;
	c128_mmu[1] = c128_mmu[2] = c128_mmu[3] = c128_mmu[4] = 0;
	c128_mmu[5] &= ~9;
	c128_mmu[6] &= ~3;
	c128_mmu[7] = c128_mmu[8] = c128_mmu[9] = 0;
	c128_mmu[10] = 1;
	c128_va1617 = 0;
#else
	memset (c128_mmu, 0, sizeof (c128_mmu));
#endif
	c128_mmu[5] |= 0x38;
	c128_mmu[10] = 1;
	mmu_cpu=0;
    mmu_page0=0;
    mmu_page1=0x0100;
	c128_bankswitch (1);
}

WRITE_HANDLER( c128_mmu8722_port_w )
{
	offset &= 0xf;
	switch (offset)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 8:
	case 10:
		c128_mmu[offset] = data;
		break;
	case 0:
	case 5:
	case 6:
		c128_mmu[offset] = data;
		c128_bankswitch (0);
		break;
	case 7:
		c128_mmu[offset] = data;
		mmu_page0=MMU_PAGE0;
		break;
	case 9:
		c128_mmu[offset] = data;
		mmu_page1=MMU_PAGE1;
		c128_bankswitch (0);
		break;
	case 0xb:
		break;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		break;
	}
}

READ_HANDLER( c128_mmu8722_port_r )
{
	int data;

	offset &= 0x0f;
	switch (offset)
	{
	case 5:
		data = c128_mmu[offset] | 6;
		if ( /*disk enable signal */ 0)
			data &= ~8;
		if (!c64_game)
			data &= ~0x10;
		if (!c64_exrom)
			data &= ~0x20;
		if (KEY_4080)
			data &= ~0x80;
		break;
	case 0xb:
		/* hinybble number of 64 kb memory blocks */
		if (C128_MAIN_MEMORY==RAM256KB) data=0x4f;
		else if (C128_MAIN_MEMORY==RAM1MB) data=0xf;
		else data=0x2f;
		break;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		data=0xff;
		break;
	default:
		data=c128_mmu[offset];
	}
	return data;
}

WRITE_HANDLER( c128_mmu8722_ff00_w )
{
	switch (offset)
	{
	case 0:
		c128_mmu[offset] = data;
		c128_bankswitch (0);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
#if 1
		c128_mmu[0]= c128_mmu[offset];
#else
		c128_mmu[0]|= c128_mmu[offset];
#endif
		c128_bankswitch (0);
		break;
	}
}

READ_HANDLER( c128_mmu8722_ff00_r )
{
	return c128_mmu[offset];
}

WRITE_HANDLER( c128_write_0000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x0000 + offset] = data;
}

WRITE_HANDLER( c128_write_1000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x1000 + offset] = data;
}

WRITE_HANDLER( c128_write_4000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x4000 + offset] = data;
}

WRITE_HANDLER( c128_write_8000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x8000 + offset] = data;
}

WRITE_HANDLER( c128_write_a000 )
{
	if (c128_ram!=NULL)
		c128_ram[0xa000 + offset] = data;
}

WRITE_HANDLER( c128_write_e000 )
{
	if (offset + 0xe000 >= c128_ram_top)
		c64_memory[0xe000 + offset] = data;
	else if (c128_ram!=NULL)
		c128_ram[0xe000 + offset] = data;
}

WRITE_HANDLER( c128_write_ff00 )
{
	if (!c64mode)
		c128_mmu8722_ff00_w (offset, data);
	else if (c128_ram!=NULL)
		c64_memory[0xff00 + offset] = data;
}

WRITE_HANDLER( c128_write_ff05 )
{
	if (offset + 0xff05 >= c128_ram_top)
		c64_memory[0xff05 + offset] = data;
	else if (c128_ram!=NULL)
		c128_ram[0xff05 + offset] = data;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c128_dma_read(int offset)
{
	/* main memory configuration to include */
	if (c64mode)
	{
		if (!c64_game && c64_exrom)
		{
			if (offset < 0x3000)
				return c64_memory[offset];
			return c64_romh[offset & 0x1fff];
		}
		if (((c64_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
			return c64_chargen[offset & 0xfff];
		return c64_vicaddr[offset];
	}
	if ( !(c64_port6510&4)
		 && (((c128_vicaddr - c64_memory + offset) & 0x7000) == 0x1000) )
		return c128_chargen[offset & 0xfff];
	return c128_vicaddr[offset];
}

static int c128_dma_read_color (int offset)
{
    if (c64mode) return c64_colorram[offset & 0x3ff] & 0xf;
    return c64_colorram[(offset & 0x3ff)|((c64_port6510&0x3)<<10)] & 0xf;
}

static void c128_common_driver_init (void)
{
	UINT8 *gfx=memory_region(REGION_GFX1);
	int i;

#if 0
	{0x100000, 0x107fff, MWA8_ROM, &c128_basic},	/* maps to 0x4000 */
	{0x108000, 0x109fff, MWA8_ROM, &c64_basic},	/* maps to 0xa000 */
	{0x10a000, 0x10bfff, MWA8_ROM, &c64_kernal},	/* maps to 0xe000 */
	{0x10c000, 0x10cfff, MWA8_ROM, &c128_editor},
	{0x10d000, 0x10dfff, MWA8_ROM, &c128_z80},		/* maps to z80 0 */
	{0x10e000, 0x10ffff, MWA8_ROM, &c128_kernal},
	{0x110000, 0x117fff, MWA8_ROM, &c128_internal_function},
	{0x118000, 0x11ffff, MWA8_ROM, &c128_external_function},
	{0x120000, 0x120fff, MWA8_ROM, &c64_chargen},
	{0x121000, 0x121fff, MWA8_ROM, &c128_chargen},
	{0x122000, 0x1227ff, MWA8_RAM, &c64_colorram},
	{0x122800, 0x1327ff, MWA8_RAM, &c128_vdcram},
#endif
		c128_basic=memory_region(REGION_CPU1)+0x100000;
		c64_basic=memory_region(REGION_CPU1)+0x108000;
		c64_kernal=memory_region(REGION_CPU1)+0x10a000;
		c128_editor=memory_region(REGION_CPU1)+0x10c000;
		c128_z80=memory_region(REGION_CPU1)+0x10d000;
		c128_kernal=memory_region(REGION_CPU1)+0x10e000;
		c128_internal_function=memory_region(REGION_CPU1)+0x110000;
		c128_external_function=memory_region(REGION_CPU1)+0x118000;
		c64_chargen=memory_region(REGION_CPU1)+0x120000;
		c128_chargen=memory_region(REGION_CPU1)+0x121000;
		c64_colorram=memory_region(REGION_CPU1)+0x122000;
		c128_vdcram=memory_region(REGION_CPU1)+0x122800;

	for (i=0; i<0x100; i++) gfx[i]=i;

	memset(c64_memory, 0xff, 0x100000);
	c128 = 1;
	vc20_tape_open (c64_tape_read);

	cbm_drive_open ();

	cia6526_init();

	c64_cia0.todin50hz = c64_pal;
	cia6526_config (0, &c64_cia0);
	c64_cia1.todin50hz = c64_pal;
	cia6526_config (1, &c64_cia1);
}

void c128_driver_init (void)
{
	c128_common_driver_init ();
	vic6567_init (1, c64_pal,
				  c128_dma_read, c128_dma_read_color, c64_vic_interrupt);
	vic2_set_rastering(0);
	vdc8563_init(0);
	vdc8563_set_rastering(1);
	statetext_add_function(c128_state);
}

void c128pal_driver_init (void)
{
	c64_pal = 1;
	c128_common_driver_init ();
	vic6567_init (1, c64_pal,
				  c128_dma_read, c128_dma_read_color, c64_vic_interrupt);
	vic2_set_rastering(1);
	vdc8563_init(0);
	vdc8563_set_rastering(0);
	statetext_add_function(c128_state);
}

void c128_driver_shutdown (void)
{
	cbm_drive_close ();
	vc20_tape_close ();
}

MACHINE_INIT( c128 )
{
	logerror("reset\n");
	c64_common_init_machine ();
	c128_vicaddr = c64_vicaddr = c64_memory;

	sid6581_reset(0);

	c64_rom_recognition ();
	c64_rom_load();

#if 1
	c64mode = 0;
#else
	c64mode = 1;
#endif
	c128_mmu8722_reset ();
	cpu_set_halt_line (0, 0);
	cpu_set_halt_line (1, 1);
}

VIDEO_START( c128 )
{
	return video_start_vdc8563() || video_start_vic2();
}

VIDEO_UPDATE( c128 )
{
	video_update_vdc8563(bitmap, cliprect, do_skip);
	video_update_vic2(bitmap, cliprect, do_skip);
}

void c128_state(void)
{
	char text[70];

	vc20_tape_status (text, sizeof (text));
	statetext_display_text (text);
#ifdef VC1541
	vc1541_drive_status (text, sizeof (text));
#else
	cbm_drive_0_status (text, sizeof (text));
#endif
	statetext_display_text (text);

	cbm_drive_1_status (text, sizeof (text));
	statetext_display_text (text);
}
