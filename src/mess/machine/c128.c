/***************************************************************************


***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "cia6526.h"
#include "c1551.h"
#include "vc1541.h"
#include "vc20tape.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/vidhrdw/vdc8563.h"
#include "mess/sndhrdw/sid6581.h"

#include "c128.h"


/*
 two cpus share 1 memory system, only 1 cpu can run
 controller is the mmu
 
 mame has different memory subsystems for each cpu
*/


/* computer is a c128 */
int c128 = 0;

UINT8 c128_keyline[3] =
{0xff, 0xff, 0xff};

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

static void c128_dma8726_port_w(int offset, int data)
{
	DBG_LOG(1,"dma write",(errorlog,"%.3x %.2x\n",offset,data));
}

static int c128_dma8726_port_r(int offset)
{
	DBG_LOG(1,"dma read",(errorlog,"%.3x\n",offset));
	return 0xff;
}

void c128_mmu8722_port_w (int offset, int data);
int c128_mmu8722_port_r (int offset);
void c128_write_d000 (int offset, int value)
{
	if (!c128_write_io)
	{
		if (offset + 0xd000 >= c128_ram_top)
			c64_memory[0xd000 + offset] = value;
		else
			c128_ram[0xd000 + offset] = value;
	}
	else if (offset < 0x400)
		vic2_port_w (offset & 0x3ff, value);
	else if ((offset < 0x500)&&!c64mode)
		sid6581_0_port_w (offset & 0x3f, value);
	else if ((offset < 0x600)&&!c64mode)
		c128_mmu8722_port_w (offset & 0xff, value);
	else if ((offset < 0x700) && !c64mode)
		vdc8563_port_w (offset & 0xff, value);
	else if (offset < 0x800)
	{
		DBG_LOG (1, "io write", (errorlog, "%.3x %.2x\n", offset, value));
	}
	else if (offset < 0xc00)
		c64_colorram[offset & 0x3ff] = value | 0xf0;
	else if (offset < 0xd00)
		cia6526_0_port_w (offset & 0xff, value);
	else if (offset < 0xe00)
		cia6526_1_port_w (offset & 0xff, value);
	else if (offset<0xf00) c128_dma8726_port_w(offset&0xff,value);
	else
		DBG_LOG (1, "io write", (errorlog, "%.3x %.2x\n", offset, value));
}

static int c128_read_io (int offset)
{
	if (offset < 0x400)
		return vic2_port_r (offset & 0x3ff);
	else if (offset < 0x500)
		return sid6581_0_port_r (offset & 0xff);
	else if ( (offset < 0x600)&&!c64mode)
		return c128_mmu8722_port_r (offset & 0xff);
	else if ((offset < 0x700) && !c64mode)
		return vdc8563_port_r (offset & 0xff);
	else if (offset < 0x800)
	{
		DBG_LOG (1, "io read", (errorlog, "%.3x\n", offset));
	}
	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];
	else if (offset < 0xd00)
		return cia6526_0_port_r (offset & 0xff);
	else if (offset < 0xe00)
		return cia6526_1_port_r (offset & 0xff);
	else if (offset<0xf00) return c128_dma8726_port_r(offset&0xff);
	DBG_LOG (1, "io read", (errorlog, "%.3x\n", offset));
	return 0xff;
}

void c128_bankswitch_64 (void)
{
	static int old = -1, data, loram, hiram, charen;

	if (!c64mode)
		return;

	data = ((c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff)) & 7;
	if (data == old)
		return;

	DBG_LOG (1, "bankswitch", (errorlog, "%d\n", data & 7));
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
		cpu_setbankhandler_r (13, c128_read_io);
		c128_write_io = 1;
	}
	else
	{
		cpu_setbankhandler_r (13, MRA_BANK5);
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
		cpu_setbank (15, 0x1f00 + c64_romh);
		cpu_setbank (16, 0x1f05 + c64_romh);
	}
	else
	{
		if (hiram)
		{
			cpu_setbank (14, c64_kernal);
			cpu_setbank (15, 0x1f00 + c64_kernal);
			cpu_setbank (16, 0x1f05 + c64_kernal);
		}
		else
		{
			cpu_setbank (14, c64_memory + 0xe000);
			cpu_setbank (15, c64_memory + 0xff00);
			cpu_setbank (16, c64_memory + 0xff05);
		}
	}
	old = data;
}

void c128_z80_bankswitch (void)
{
	cpu_setbank (10, c128_z80);
}


UINT8 c128_mmu[0x0b];
static int c128_mmu_helper[4] =
{0x400, 0x1000, 0x2000, 0x4000};

#define MMU_PAGE0 ((((c128_mmu[10]&0xf)<<8)|c128_mmu[9])<<8)
#define MMU_PAGE1 ((((c128_mmu[8]&0xf)<<8)|c128_mmu[7])<<8)
#define MMU_VIC_ADDR ((c128_mmu[6]&0xc0)<<10)
#define MMU_RAM_RCR_ADDR ((c128_mmu[6]&0x30)<<14)
#define MMU_SIZE (c128_mmu_helper[c128_mmu[6]&3])
#define MMU_BOTTOM (c128_mmu[6]&4)
#define MMU_TOP (c128_mmu[6]&8)
#define MMU_CPU8502 (c128_mmu[5]&1)	   /* else z80 */
#define MMU_GAME_IN (c128_mmu[5]&0x10)
#define MMU_EXROM_IN (c128_mmu[5]&0x20)
#define MMU_64MODE (c128_mmu[5]&0x40)
#define MMU_40_IN (c128_mmu[5]&0x80)
#define MMU_RAM_ADDR (MMU_RAM_RCR_ADDR|MMU_RAM_CR_ADDR)

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

static void c128_bankswitch (void);
#ifdef C128_Z80
static void c128_timer_callback(int arg)
{
	DBG_LOG (1, "timer aktiv", 
			 (errorlog, "active %d\n",cpu_getactivecpu()) );
	c128_bankswitch();
}
#endif
static void c128_bankswitch (void)
{
#ifdef C128_Z80
	static int oldcpu = -1;

	if (!MMU_CPU8502)
	{
		if (oldcpu != MMU_CPU8502)
		{
			DBG_LOG (1, "Z80 aktiv", 
					 (errorlog, "active %d\n",cpu_getactivecpu()) );
			cpu_set_reset_line (1, 1);
			
			cpu_set_halt_line (0, 0);
			DBG_LOG (1, "Z80 aktiv", 
					 (errorlog, "active %d\n",cpu_getactivecpu()) );
			oldcpu = MMU_CPU8502;
#if 0
			timer_set(TIME_NOW, 0, c128_timer_callback);
			return;
#else
			memorycontextswap(0);
			c128_bankswitch();
			if (cpu_getactivecpu()!=0) {
				memorycontextswap(cpu_getactivecpu());
			}
			return;
#endif
		}
		c128_z80_bankswitch ();
	}
	if ((oldcpu != MMU_CPU8502))
	{
		DBG_LOG (1, "M8502 aktiv", 
				 (errorlog, "active %d\n",cpu_getactivecpu()) );
		cpu_set_halt_line (0, 1);
		cpu_set_reset_line (1, 0);
		DBG_LOG (1, "M8502 aktiv", 
				 (errorlog, "active %d\n",cpu_getactivecpu()) );
		oldcpu = MMU_CPU8502;

#if 0
		timer_set(TIME_NOW, 0, c128_timer_callback);
		return;
#else
			memorycontextswap(1);
			c128_bankswitch();
			if (cpu_getactivecpu()!=1) {
				memorycontextswap(cpu_getactivecpu());
			}
			return;
#endif
	}
#endif
	c64mode = MMU_64MODE;
	if (c64mode)
	{
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

		c128_bankswitch_64 ();
	}
	else
	{
		c128_ram = c64_memory + MMU_RAM_ADDR;
		c128_va1617 = MMU_VIC_ADDR;
		if (MMU_BOTTOM)
		{
			c128_ram_bottom = MMU_SIZE;
		}
		else
			c128_ram_bottom = 0;
		cpu_setbank (3, (c128_ram_bottom >= 0x200 ? c64_memory : c128_ram) + 0x200);
		cpu_setbank (4, (c128_ram_bottom >= 0x400 ? c64_memory : c128_ram) + 0x400);
		cpu_setbank (5, (c128_ram_bottom >= 0x1000 ? c64_memory : c128_ram) + 0x1000);
		cpu_setbank (6, (c128_ram_bottom >= 0x2000 ? c64_memory : c128_ram) + 0x2000);

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
		cpu_setbankhandler_r (15, c128_mmu8722_port_r);
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
			cpu_setbankhandler_r (13, MRA_BANK13);
			if (c128_ram_top > 0xd000)
			{
				cpu_setbank (13, c128_ram + 0xd000);
			}
			else
			{
				cpu_setbank (13, c64_memory + 0xd000);
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
		else
		{
			if (MMU_IO_ON)
			{
				cpu_setbankhandler_r (13, c128_read_io);
				c128_write_io = 1;
			}
			else
			{
				c128_write_io = 0;
				cpu_setbankhandler_r (13, MRA_BANK13);
				cpu_setbank (13, c64_chargen);
			}
			if (MMU_ROM_HI)
			{
				cpu_setbank (12, c128_editor);
				cpu_setbank (14, c128_kernal);
				cpu_setbank (16, c128_kernal + 0x1f05);
			}
			else if (MMU_INTERNAL_ROM_HI)
			{
				cpu_setbank (12, c128_internal_function);
				cpu_setbank (14, c128_internal_function + 0x2000);
				cpu_setbank (16, c128_internal_function + 0x3f05);
			}
			else					   /*if (MMU_EXTERNAL_ROM_HI) */
			{
				cpu_setbank (12, c128_external_function);
				cpu_setbank (14, c128_external_function + 0x2000);
				cpu_setbank (16, c128_external_function + 0x3f05);
			}
		}
	}
}

void c128_mmu8722_reset (void)
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
	c128_mmu[10] = 1;
	c128_mmu[5] |= 0x38;
	c128_bankswitch ();
}

void c128_mmu8722_port_w (int offset, int data)
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
		c128_bankswitch ();
		break;
	case 7:
		c128_mmu[offset] = data;
		cpu_setbank (1, c64_memory + MMU_PAGE0);
		break;
	case 9:
		c128_mmu[offset] = data;
		cpu_setbank (2, c64_memory + MMU_PAGE1);
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

int c128_mmu8722_port_r (int offset)
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
		return data;
	case 0xb:
		/* hinybble number of 64 kb memory blocks */
		if (C128_MAIN_MEMORY==RAM256KB) return 0x4f;
		if (C128_MAIN_MEMORY==RAM1MB) return 0xf;
		return 0xf;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		return 0xff;
		break;
	default:
		return c128_mmu[offset];
	}
}

void c128_mmu8722_ff00_w (int offset, int data)
{
	switch (offset)
	{
	case 0:
		c128_mmu[offset] = data;
		c128_bankswitch ();
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		c128_mmu[0] = c128_mmu[offset];
		c128_bankswitch ();
		break;
	}
}

int c128_mmu8722_ff00_r (int offset)
{
	return c128_mmu[offset];
}

void c128_write_4000 (int offset, int value)
{
	c128_ram[0x4000 + offset] = value;
}

void c128_write_8000 (int offset, int value)
{
	c128_ram[0x8000 + offset] = value;
}

void c128_write_a000 (int offset, int value)
{
	c128_ram[0xa000 + offset] = value;
}

void c128_write_e000 (int offset, int value)
{
	if (offset + 0xe000 >= c128_ram_top)
		c64_memory[0xe000 + offset] = value;
	else
		c128_ram[0xe000 + offset] = value;
}

void c128_write_ff00 (int offset, int value)
{
	if (!c64mode)
		c128_mmu8722_ff00_w (offset, value);
	else
		c64_memory[0xff00 + offset] = value;
}

void c128_write_ff05 (int offset, int value)
{
	if (offset + 0xe000 >= c128_ram_top)
		c64_memory[0xff05 + offset] = value;
	else
		c128_ram[0xff05 + offset] = value;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c128_dma_read (int offset)
{
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
	if (((c128_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
		return c128_chargen[offset & 0xfff];
	return c128_vicaddr[offset];
}

static int c128_dma_read_color (int offset)
{
	return c64_colorram[offset & 0x3ff] & 0xf;
}

static void c128_common_driver_init (void)
{
	/*    memset(c128_memory, 0, 0xfd00); */
	c128 = 1;
	vc20_tape_open (c64_tape_read);


	//c128_kernal[0x5c1] = c128_kernal[0x5c2] = 0x90;
	cbm_drive_open ();

	cbm_drive_attach_fs (0);
	cbm_drive_attach_fs (1);

#ifdef VC1541
	vc1541_driver_init ();
#endif

	sid6581_0_init (c64_paddle_read);
	c64_cia0.todin50hz = c64_pal;
	cia6526_config (0, &c64_cia0);
	c64_cia1.todin50hz = c64_pal;
	cia6526_config (1, &c64_cia1);
	vic6567_init (1, c64_pal, c128_dma_read, c128_dma_read_color, c64_vic_interrupt);

			cpu_set_halt_line (1, 1);
			cpu_set_halt_line (0, 0);
}

void c128_driver_init (void)
{
	c128_common_driver_init ();
}

void c128pal_driver_init (void)
{
	c64_pal = 1;
	c128_common_driver_init ();
}

void c128_driver_shutdown (void)
{
	cbm_drive_close ();
	vc20_tape_close ();
}

void c128_init_machine (void)
{
	c64_common_init_machine ();
	c128_vicaddr = c64_vicaddr = c64_memory;

	c64_rom_recognition ();
	c64_rom_load();

#if 1
	c64mode = 0;
#else
	c64mode = 1;
#endif
	c128_mmu8722_reset ();
	c128_bankswitch ();
}

void c128_shutdown_machine (void)
{
}

/*only for debugging */
void c128_status (char *text, int size)
{
	text[0] = 0;
#if VERBOSE_DBG
	snprintf (text, size, "c128 vic:%.5x m6510:%d exrom:%d game:%d",
			  c128_vicaddr - c64_memory, c64_port6510 & 7,
			  c64_exrom, c64_game);
#endif
}
