/*
	TODO:
	. Mapper 5 has some issues, RAM banking needs hardware flags to determine size
	. Mappers 9 & 10 have minor probs
	. Mapper 40 seems to run code across bank boundaries (no-no in MESS)
	. Mapper 64 (Klax) needs some work (irq probs?)
	. Mapper 67 vrom problems
	. Mapper 72 totally busted
	. Mapper 82 is busted

	Also, remember that the MMC does not equal the mapper #. In particular, Mapper 4 is
	really MMC3, Mapper 9 is MMC2 and Mapper 10 is MMC4. Makes perfect sense, right?
*/

#include <string.h>

#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"
#include "osdepend.h"
#include "driver.h"
#include "machine/nes.h"
#include "machine/nes_mmc.h"

//#define LOG_MMC
#define LOG_FDS

/* Global variables */
int prg_mask;

int IRQ_enable, IRQ_enable_latch;
UINT16 IRQ_count, IRQ_count_latch;
UINT8 IRQ_status;
UINT8 IRQ_mode_jaleco;

int MMC1_extended;	/* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

void (*mmc_write_low)(int offset, int data);
int (*mmc_read_low)(int offset);
void (*mmc_write_mid)(int offset, int data);
int (*mmc_read_mid)(int offset);
void (*mmc_write)(int offset, int data);
void (*ppu_latch)(int offset);
int (*mmc_irq)(int scanline);

static int vrom_bank[16];
static int mult1, mult2;

/* Local variables */
static int MMC1_Size_16k;
static int MMC1_High;
static int MMC1_reg;
static int MMC1_reg_count;
static int MMC1_Switch_Low, MMC1_SizeVrom_4k;
static int MMC1_bank1, MMC1_bank2, MMC1_bank3, MMC1_bank4;
static int MMC1_extended_bank;
static int MMC1_extended_base;
static int MMC1_extended_swap;

static int MMC2_bank0, MMC2_bank0_hi, MMC2_bank0_latch, MMC2_bank1, MMC2_bank1_hi, MMC2_bank1_latch;

static int MMC3_cmd;
static int MMC3_prg0, MMC3_prg1;
static int MMC3_chr[6];

static int MMC5_rom_bank_mode;
static int MMC5_vrom_bank_mode;
static int MMC5_vram_protect;
static int MMC5_scanline;
UINT8 MMC5_vram[0x400];
int MMC5_vram_control;

static int mapper_warning;

void nes_low_mapper_w (int offset, int data)
{
	if (*mmc_write_low) (*mmc_write_low)(offset, data);
	else
	{
		logerror("Unimplemented LOW mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (! mapper_warning)
		{
			printf ("This game is writing to the low mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}
#endif
	}
}

/* Handle mapper reads from $4100-$5fff */
int nes_low_mapper_r (int offset)
{
	if (*mmc_read_low)
		return (*mmc_read_low)(offset);
	else
		logerror("low mapper area read, addr: %04x\n", offset + 0x4100);

	return 0;
}

WRITE_HANDLER ( nes_mid_mapper_w )
{
	if (*mmc_write_mid) (*mmc_write_mid)(offset, data);
	else if (nes.mid_ram_enable)
		battery_ram[offset] = data;
	else
	{
		logerror("Unimplemented MID mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (! mapper_warning)
		{
			printf ("This game is writing to the MID mapper area but no mapper is set. You may get better results by switching to a valid mapper or changing the battery flag for this ROM.\n");
			mapper_warning = 1;
		}
#endif
	}
}

int nes_mid_mapper_r (int offset)
{
	if ((nes.mid_ram_enable) || (nes.mapper == 5))
		return battery_ram[offset];
	else
		return 0;
}

WRITE_HANDLER ( nes_mapper_w )
{
	if (*mmc_write) (*mmc_write)(offset, data);
	else
	{
		logerror("Unimplemented mapper write, offset: %04x, data: %02x\n", offset, data);
#if 1
		if (! mapper_warning)
		{
			printf ("This game is writing to the mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}

		switch (offset)
		{
			/* Hacked mapper for the 24-in-1 NES_ROM. */
			/* It's really 35-in-1, and it's mostly different versions of Battle City. Unfortunately, the vrom dumps are bad */
			case 0x7fde:
				data &= (nes.prg_chunks - 1);
				cpu_setbank (3, &nes.rom[data * 0x4000 + 0x10000]);
				cpu_setbank (4, &nes.rom[data * 0x4000 + 0x12000]);
				break;
		}
#endif
	}
}

/*
 * Some helpful routines used by the mappers
 */

static void prg8_89 (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	cpu_setbank (1, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ab (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	cpu_setbank (2, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_cd (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	cpu_setbank (3, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ef (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	cpu_setbank (4, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg16_89ab (int bank)
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	cpu_setbank (1, &nes.rom[bank * 0x4000 + 0x10000]);
	cpu_setbank (2, &nes.rom[bank * 0x4000 + 0x12000]);
}

static void prg16_cdef (int bank)
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	cpu_setbank (3, &nes.rom[bank * 0x4000 + 0x10000]);
	cpu_setbank (4, &nes.rom[bank * 0x4000 + 0x12000]);
}

static void prg32 (int bank)
{
	/* assumes that bank references a 32k chunk */
	bank &= ((nes.prg_chunks >> 1) - 1);
	cpu_setbank (1, &nes.rom[bank * 0x8000 + 0x10000]);
	cpu_setbank (2, &nes.rom[bank * 0x8000 + 0x12000]);
	cpu_setbank (3, &nes.rom[bank * 0x8000 + 0x14000]);
	cpu_setbank (4, &nes.rom[bank * 0x8000 + 0x16000]);
}

static void chr8 (int bank)
{
	int i;

	bank &= (nes.chr_chunks - 1);
	for (i = 0; i < 8; i ++)
		nes_vram[i] = bank * 512 + 64*i;
}

static void chr4_0 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 1) - 1);
	for (i = 0; i < 4; i ++)
		nes_vram[i] = bank * 256 + 64*i;
}

static void chr4_4 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 1) - 1);
	for (i = 4; i < 8; i ++)
		nes_vram[i] = bank * 256 + 64*(i-4);
}

static void chr2_0 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 2) - 1);
	for (i = 0; i < 2; i ++)
		nes_vram[i] = bank * 128 + 64*i;
}

static void chr2_2 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 2) - 1);
	for (i = 2; i < 4; i ++)
		nes_vram[i] = bank * 128 + 64*(i-2);
}

static void chr2_4 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 2) - 1);
	for (i = 4; i < 6; i ++)
		nes_vram[i] = bank * 128 + 64*(i-4);
}

static void chr2_6 (int bank)
{
	int i;

	bank &= ((nes.chr_chunks << 2) - 1);
	for (i = 6; i < 8; i ++)
		nes_vram[i] = bank * 128 + 64*(i-6);
}

static void mapper1_w (int offset, int data)
{
	int i;
	int reg;

	/* Find which register we are dealing with */
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */
	reg = (offset >> 13);

	if (data & 0x80)
	{
		MMC1_reg_count = 0;
		MMC1_reg = 0;

		/* Set these to their defaults - needed for Robocop 3, Dynowars */
		MMC1_Size_16k = 1;
		MMC1_Switch_Low = 1;
		/* TODO: should we switch banks at this time also? */
#ifdef LOG_MMC
		logerror("=== MMC1 regs reset to default\n");
#endif
		return;
	}

	if (MMC1_reg_count < 5)
	{
		if (MMC1_reg_count == 0) MMC1_reg = 0;
		MMC1_reg >>= 1;
		MMC1_reg |= (data & 0x01) ? 0x10 : 0x00;
		MMC1_reg_count ++;
	}

	if (MMC1_reg_count == 5)
	{
//		logerror("   MMC1 reg#%02x val:%02x\n", offset, MMC1_reg);
		switch (reg)
		{
			case 0:
				MMC1_Switch_Low = MMC1_reg & 0x04;
				MMC1_Size_16k =   MMC1_reg & 0x08;

				switch (MMC1_reg & 0x03)
				{
					case 0: ppu_mirror_low (); break;
					case 1: ppu_mirror_high (); break;
					case 2: ppu_mirror_v (); break;
					case 3: ppu_mirror_h (); break;
				}

				MMC1_SizeVrom_4k = (MMC1_reg & 0x10);
#ifdef LOG_MMC
				logerror("   MMC1 reg #1 val:%02x\n", MMC1_reg);
					logerror("\t\tBank Size: ");
					if (MMC1_Size_16k)
						logerror("16k\n");
					else logerror("32k\n");

					logerror("\t\tBank Select: ");
					if (MMC1_Switch_Low)
						logerror("$8000\n");
					else logerror("$C000\n");

					logerror("\t\tVROM Bankswitch Size Select: ");
					if (MMC1_SizeVrom_4k)
						logerror("4k\n");
					else logerror("8k\n");

					logerror ("\t\tMirroring: %d\n", MMC1_reg & 0x03);
#endif
				break;

			case 1:
				MMC1_extended_bank = (MMC1_extended_bank & ~0x01) | ((MMC1_reg & 0x10) >> 4);
				if (MMC1_extended == 2)
				{
					/* MMC1_SizeVrom_4k determines if we use the special 256k bank register */
				 	if (!MMC1_SizeVrom_4k)
				 	{
						/* Pick 1st or 4th 256k bank */
						MMC1_extended_base = 0xc0000 * (MMC1_extended_bank & 0x01) + 0x10000;
						cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
						logerror("MMC1_extended 1024k bank (no reg) select: %02x\n", MMC1_extended_bank);
#endif
					}
					else
					{
						/* Set 256k bank based on the 256k bank select register */
						if (MMC1_extended_swap)
						{
							MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
							cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
							logerror("MMC1_extended 1024k bank (reg 1) select: %02x\n", MMC1_extended_bank);
#endif
							MMC1_extended_swap = 0;
						}
						else MMC1_extended_swap = 1;
					}
				}
				else if (MMC1_extended == 1 && nes.chr_chunks == 0)
				{
					/* Pick 1st or 2nd 256k bank */
					MMC1_extended_base = 0x40000 * (MMC1_extended_bank & 0x01) + 0x10000;
					cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
					cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
					logerror("MMC1_extended 512k bank select: %02x\n", MMC1_extended_bank & 0x01);
#endif
				}
				else if (nes.chr_chunks > 0)
				{
//					logerror("MMC1_SizeVrom_4k: %02x bank:%02x\n", MMC1_SizeVrom_4k, MMC1_reg);

					if (!MMC1_SizeVrom_4k)
					{
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);

						for (i = 0; i < 8; i ++)
							nes_vram[i] = bank * 256 + 64*i;
#ifdef LOG_MMC
						logerror("MMC1 8k VROM switch: %02x\n", MMC1_reg);
#endif
					}
					else
					{
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);

						for (i = 0; i < 4; i ++)
							nes_vram[i] = bank * 256 + 64*i;
#ifdef LOG_MMC
						logerror("MMC1 4k VROM switch (low): %02x\n", MMC1_reg);
#endif
					}
				}
				break;
			case 2:
//				logerror("MMC1_Reg_2: %02x\n",MMC1_Reg_2);
				MMC1_extended_bank = (MMC1_extended_bank & ~0x02) | ((MMC1_reg & 0x10) >> 3);
				if (MMC1_extended == 2 && MMC1_SizeVrom_4k)
				{
					if (MMC1_extended_swap)
					{
						/* Set 256k bank based on the 256k bank select register */
						MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
						cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
						logerror("MMC1_extended 1024k bank (reg 2) select: %02x\n", MMC1_extended_bank);
#endif
						MMC1_extended_swap = 0;
					}
					else
						MMC1_extended_swap = 1;
				}
				if (MMC1_SizeVrom_4k)
				{
					int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);

					for (i = 4; i < 8; i ++)
						nes_vram[i] = bank * 256 + 64*(i-4);
#ifdef LOG_MMC
						logerror("MMC1 4k VROM switch (high): %02x\n", MMC1_reg);
#endif
				}
				break;
			case 3:
				/* Switching 1 32k bank of PRG ROM */
				MMC1_reg &= 0x0f;
				if (!MMC1_Size_16k)
				{
					int bank = MMC1_reg & (nes.prg_chunks - 1);

					MMC1_bank1 = bank * 0x4000;
					MMC1_bank2 = bank * 0x4000 + 0x2000;
					cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					if (!MMC1_extended)
					{
						MMC1_bank3 = bank * 0x4000 + 0x4000;
						MMC1_bank4 = bank * 0x4000 + 0x6000;
						cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
					}
#ifdef LOG_MMC
					logerror("MMC1 32k bank select: %02x\n", MMC1_reg);
#endif
				}
				else
				/* Switching one 16k bank */
				{
					if (MMC1_Switch_Low)
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						MMC1_bank1 = bank * 0x4000;
						MMC1_bank2 = bank * 0x4000 + 0x2000;

						cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						if (!MMC1_extended)
						{
							MMC1_bank3 = MMC1_High;
							MMC1_bank4 = MMC1_High + 0x2000;
							cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
#ifdef LOG_MMC
						logerror("MMC1 16k-low bank select: %02x\n", MMC1_reg);
#endif
					}
					else
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						if (!MMC1_extended)
						{

							MMC1_bank1 = 0;
							MMC1_bank2 = 0x2000;
							MMC1_bank3 = bank * 0x4000;
							MMC1_bank4 = bank * 0x4000 + 0x2000;

							cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
#ifdef LOG_MMC
						logerror("MMC1 16k-high bank select: %02x\n", MMC1_reg);
#endif
					}
				}

#ifdef LOG_MMC
				logerror("-- page1: %06x\n", MMC1_bank1);
				logerror("-- page2: %06x\n", MMC1_bank2);
				logerror("-- page3: %06x\n", MMC1_bank3);
				logerror("-- page4: %06x\n", MMC1_bank4);
#endif
				break;
		}
		MMC1_reg_count = 0;
	}
}

static void mapper2_w (int offset, int data)
{
	prg16_89ab (data);
}

static void mapper3_w (int offset, int data)
{
	chr8 (data);
}

static void mapper4_set_prg (void)
{
	MMC3_prg0 &= prg_mask;
	MMC3_prg1 &= prg_mask;

	if (MMC3_cmd & 0x40)
	{
		cpu_setbank (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
		cpu_setbank (3, &nes.rom[0x2000 * (MMC3_prg0) + 0x10000]);
	}
	else
	{
		cpu_setbank (1, &nes.rom[0x2000 * (MMC3_prg0) + 0x10000]);
		cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
	}
	cpu_setbank (2, &nes.rom[0x2000 * (MMC3_prg1) + 0x10000]);
}

static void mapper4_set_chr (void)
{
	UINT8 chr_page = (MMC3_cmd & 0x80) >> 5;

	nes_vram [chr_page] = MMC3_chr[0];
	nes_vram [chr_page+1] = nes_vram [chr_page] + 64;
	nes_vram [chr_page ^ 2] = MMC3_chr[1];
	nes_vram [(chr_page ^ 2)+1] = nes_vram [chr_page ^ 2] + 64;
	nes_vram [chr_page ^ 4] = MMC3_chr[2];
	nes_vram [chr_page ^ 5] = MMC3_chr[3];
	nes_vram [chr_page ^ 6] = MMC3_chr[4];
	nes_vram [chr_page ^ 7] = MMC3_chr[5];
}

int mapper4_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	if (scanline <= BOTTOM_VISIBLE_SCANLINE)
	{
/* Uncommenting this breaks Gauntlet 2 */
//		if (scanline == BOTTOM_VISIBLE_SCANLINE+1)
//			IRQ_count = IRQ_count_latch;
//		else
		{
			/* Decrement & check the IRQ scanline counter */
			if ((IRQ_enable) && (PPU_Control1 & 0x18))
			{
				if (IRQ_count == 0)
				{
					IRQ_count = IRQ_count_latch;
					ret = M6502_INT_IRQ;
				}
				IRQ_count --;
			}
		}
	}

	return ret;
}

static void mapper4_w (int offset, int data)
{
	static UINT8 last_bank = 0xff;

//	logerror("mapper4_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg ();
				mapper4_set_chr ();
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					data &= 0xfe;
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg ();
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg ();
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
			if (data & 0x40)
				ppu_mirror_high();
			else
			{
				if (data & 0x01)
					ppu_mirror_h();
				else
					ppu_mirror_v();
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
#ifdef LOG_MMC
			logerror("     MMC3 mid_ram enable: %02x\n", data);
#endif
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count: %02x\n", data);
#endif
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count latch: %02x\n", data);
#endif
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch; /* TODO: verify this */
#ifdef LOG_MMC
			logerror("     MMC3 disable irqs: %02x\n", data);
#endif
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
#ifdef LOG_MMC
			logerror("     MMC3 enable irqs: %02x\n", data);
#endif
			break;

		default:
			logerror("mapper4_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

int mapper5_irq (int scanline)
{
	int ret = M6502_INT_NONE;

#if 1
	if (scanline == 0)
		IRQ_status |= 0x40;
	else if (scanline > BOTTOM_VISIBLE_SCANLINE)
		IRQ_status &= ~0x40;
#endif

	if (scanline == IRQ_count)
	{
		if (IRQ_enable)
			ret = M6502_INT_IRQ;

		IRQ_status = 0xff;
	}

	return ret;
}

int mapper5_l_r (int offset)
{
	int retVal;

#ifdef MMC5_VRAM
	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		return MMC5_vram[offset - 0x1b00];
	}
#endif

	switch (offset)
	{
		case 0x1104: /* $5204 */
#if 0
			if (current_scanline == MMC5_scanline) return 0x80;
			else return 0x00;
#else
			retVal = IRQ_status;
			IRQ_status &= ~0x80;
			return retVal;
#endif
			break;

		case 0x1105: /* $5205 */
			return (mult1 * mult2) & 0xff;
			break;
		case 0x1106: /* $5206 */
			return ((mult1 * mult2) & 0xff00) >> 8;
			break;

		default:
			logerror("** MMC5 uncaught read, offset: %04x\n", offset + 0x4100);
			return 0x00;
			break;
	}
}

static void mapper5_sync_vrom (int mode)
{
	int i;

	for (i = 0; i < 8; i ++)
		nes_vram[i] = vrom_bank[0 + (mode * 8)] * 64;
}

void mapper5_l_w (int offset, int data)
{
//	static int vrom_next[4];
	static int vrom_page_a;
	static int vrom_page_b;

//	logerror("Mapper 5 write, offset: %04x, data: %02x\n", offset + 0x4100, data);
	/* Send $5000-$5015 to the sound chip */
	if ((offset >= 0xf00) && (offset <= 0xf15))
	{
		NESPSG_0_w (offset & 0x1f, data);
		return;
	}

#ifdef MMC5_VRAM
	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		if (MMC5_vram_protect == 0x03)
			MMC5_vram[offset - 0x1b00] = data;
		return;
	}
#endif

	switch (offset)
	{
		case 0x1000: /* $5100 */
			MMC5_rom_bank_mode = data & 0x03;
			logerror ("MMC5 rom bank mode: %02x\n", data);
			break;

		case 0x1001: /* $5101 */
			MMC5_vrom_bank_mode = data & 0x03;
			logerror ("MMC5 vrom bank mode: %02x\n", data);
			break;

		case 0x1002: /* $5102 */
			if (data == 0x02)
				MMC5_vram_protect |= 1;
			else
				MMC5_vram_protect = 0;
			logerror ("MMC5 vram protect 1: %02x\n", data);
			break;
		case 0x1003: /* 5103 */
			if (data == 0x01)
				MMC5_vram_protect |= 2;
			else
				MMC5_vram_protect = 0;
			logerror ("MMC5 vram protect 2: %02x\n", data);
			break;

		case 0x1004: /* $5104 - Extra VRAM (EXRAM) control */
			MMC5_vram_control = data;
			logerror ("MMC5 exram control: %02x\n", data);
			break;

		case 0x1005: /* $5105 */
			ppu_mirror_custom (0, data & 0x03);
			ppu_mirror_custom (1, (data & 0x0c) >> 2);
			ppu_mirror_custom (2, (data & 0x30) >> 4);
			ppu_mirror_custom (3, (data & 0xc0) >> 6);
			break;

		/* $5106 - ?? cv3 sets to 0 */
		/* $5107 - ?? cv3 sets to 0 */

		case 0x1013: /* $5113 */
			logerror ("MMC5 mid RAM bank select: %02x\n", data & 0x07);
			cpu_setbank (5, &nes.wram[data * 0x2000]);
			/* The & 4 is a hack that'll tide us over for now */
			battery_ram = &nes.wram[(data & 4) * 0x2000];
			break;

		case 0x1014: /* $5114 */
			logerror ("MMC5 $5114 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						logerror ("\tROM bank select (8k, $8000): %02x\n", data);
						data &= ((nes.prg_chunks << 1) - 1);
						cpu_setbank (1, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $8000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						cpu_setbank (1, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1015: /* $5115 */
			logerror ("MMC5 $5115 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x01:
				case 0x02:
					if (data & 0x80)
					{
						/* 16k switch - ROM only */
						prg16_89ab ((data & 0x7f) >> 1);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (16k, $8000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						cpu_setbank (1, &nes.wram[((data & 4) >> 1) * 0x4000]);
						cpu_setbank (2, &nes.wram[((data & 4) >> 1) * 0x4000 + 0x2000]);
					}
					break;
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						cpu_setbank (2, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $a000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						cpu_setbank (2, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1016: /* $5116 */
			logerror ("MMC5 $5116 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x02:
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						cpu_setbank (3, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $c000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						cpu_setbank (3, &nes.wram[(data & 4)* 0x2000]);
					}
					break;
			}
			break;
		case 0x1017: /* $5117 */
			logerror ("MMC5 $5117 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x00:
					/* 32k switch - ROM only */
					prg32 (data >> 2);
					break;
				case 0x01:
					/* 16k switch - ROM only */
					prg16_cdef (data >> 1);
					break;
				case 0x02:
				case 0x03:
					/* 8k switch */
					data &= ((nes.prg_chunks << 1) - 1);
					cpu_setbank (4, &nes.rom[data * 0x2000 + 0x10000]);
					break;
			}
			break;
		case 0x1020: /* $5120 */
			logerror ("MMC5 $5120 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[0] = data;
//					mapper5_sync_vrom(0);
					nes_vram[0] = vrom_bank[0] * 64;
//					nes_vram_sprite[0] = vrom_bank[0] * 64;
//					vrom_next[0] = 4;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1021: /* $5121 */
			logerror ("MMC5 $5121 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[1] = data;
//					mapper5_sync_vrom(0);
					nes_vram[1] = vrom_bank[1] * 64;
//					nes_vram_sprite[1] = vrom_bank[0] * 64;
//					vrom_next[1] = 5;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1022: /* $5122 */
			logerror ("MMC5 $5122 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[2] = data;
//					mapper5_sync_vrom(0);
					nes_vram[2] = vrom_bank[2] * 64;
//					nes_vram_sprite[2] = vrom_bank[0] * 64;
//					vrom_next[2] = 6;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1023: /* $5123 */
			logerror ("MMC5 $5123 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x01:
					chr4_0 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[3] = data;
//					mapper5_sync_vrom(0);
					nes_vram[3] = vrom_bank[3] * 64;
//					nes_vram_sprite[3] = vrom_bank[0] * 64;
//					vrom_next[3] = 7;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1024: /* $5124 */
			logerror ("MMC5 $5124 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[4] = data;
//					mapper5_sync_vrom(0);
					nes_vram[4] = vrom_bank[4] * 64;
//					nes_vram_sprite[4] = vrom_bank[0] * 64;
//					vrom_next[0] = 0;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1025: /* $5125 */
			logerror ("MMC5 $5125 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_4 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[5] = data;
//					mapper5_sync_vrom(0);
					nes_vram[5] = vrom_bank[5] * 64;
//					nes_vram_sprite[5] = vrom_bank[0] * 64;
//					vrom_next[1] = 1;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1026: /* $5126 */
			logerror ("MMC5 $5126 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[6] = data;
//					mapper5_sync_vrom(0);
					nes_vram[6] = vrom_bank[6] * 64;
//					nes_vram_sprite[6] = vrom_bank[0] * 64;
//					vrom_next[2] = 2;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1027: /* $5127 */
			logerror ("MMC5 $5127 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					chr8 (data);
					break;
				case 0x01:
					/* 4k switch */
					chr4_4 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_6 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[7] = data;
//					mapper5_sync_vrom(0);
					nes_vram[7] = vrom_bank[7] * 64;
//					nes_vram_sprite[7] = vrom_bank[0] * 64;
//					vrom_next[3] = 3;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1028: /* $5128 */
			logerror ("MMC5 $5128 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[8] = vrom_bank[12] = data;
//					nes_vram[vrom_next[0]] = data * 64;
//					nes_vram[0 + (vrom_page_a*4)] = data * 64;
//					nes_vram[0] = data * 64;
					nes_vram[4] = data * 64;
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x1029: /* $5129 */
			logerror ("MMC5 $5129 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (data);
					chr2_4 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[9] = vrom_bank[13] = data;
//					nes_vram[vrom_next[1]] = data * 64;
//					nes_vram[1 + (vrom_page_a*4)] = data * 64;
//					nes_vram[1] = data * 64;
					nes_vram[5] = data * 64;
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102a: /* $512a */
			logerror ("MMC5 $512a vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[10] = vrom_bank[14] = data;
//					nes_vram[vrom_next[2]] = data * 64;
//					nes_vram[2 + (vrom_page_a*4)] = data * 64;
//					nes_vram[2] = data * 64;
					nes_vram[6] = data * 64;
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102b: /* $512b */
			logerror ("MMC5 $512b vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					chr8 (data);
					break;
				case 0x01:
					/* 4k switch */
					chr4_0 (data);
					chr4_4 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (data);
					chr2_6 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[11] = vrom_bank[15] = data;
//					nes_vram[vrom_next[3]] = data * 64;
//					nes_vram[3 + (vrom_page_a*4)] = data * 64;
//					nes_vram[3] = data * 64;
					nes_vram[7] = data * 64;
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;

		case 0x1103: /* $5203 */
			IRQ_count = data;
			MMC5_scanline = data;
			logerror ("MMC5 irq scanline: %d\n", IRQ_count);
			break;
		case 0x1104: /* $5204 */
			IRQ_enable = data & 0x80;
			logerror ("MMC5 irq enable: %02x\n", data);
			break;
		case 0x1105: /* $5205 */
			mult1 = data;
			break;
		case 0x1106: /* $5206 */
			mult2 = data;
			break;

		default:
			logerror("** MMC5 uncaught write, offset: %04x, data: %02x\n", offset + 0x4100, data);
			break;
	}
}

void mapper5_w (int offset, int data)
{
	logerror("MMC5 uncaught high mapper w, %04x: %02x\n", offset, data);
}

void mapper7_w (int offset, int data)
{
	if (data & 0x10)
		ppu_mirror_high ();
	else
		ppu_mirror_low ();

	prg32 (data);
}

static void mapper8_w (int offset, int data)
{
#ifdef LOG_MMC
	logerror("* Mapper 8 switch, vrom: %02x, rom: %02x\n", data & 0x07, (data >> 3));
#endif
	/* Switch 8k VROM bank */
	chr8 (data & 0x07);

	/* Switch 16k PRG bank */
	data = (data >> 3) & (nes.prg_chunks - 1);
	cpu_setbank (1, &nes.rom[data * 0x4000 + 0x10000]);
	cpu_setbank (2, &nes.rom[data * 0x4000 + 0x12000]);
}

#if 0
void mapper9_latch (int offset)
{
	if (((offset & 0x3ff0) == 0x0fd0) || ((offset & 0x3ff0) == 0x1fd0))
	{
		logerror ("mapper9 vrom switch (latch low): %02x\n", MMC2_bank1);
		MMC2_bank1_latch = 0xfd;
		chr4_4 (MMC2_bank1);
	}
	else if (((offset & 0x3ff0) == 0x0fe0) || ((offset & 0x3ff0) == 0x1fe0))
	{
		logerror ("mapper9 vrom switch (latch high): %02x\n", MMC2_bank1_hi);
		MMC2_bank1_latch = 0xfe;
		chr4_4 (MMC2_bank1_hi);
	}
}

static void mapper9_w (int offset, int data)
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			cpu_setbank (1, &nes.rom[data * 0x2000 + 0x10000]);
			break;
		case 0x3000:
		case 0x4000:
			MMC2_bank0 = data;
			chr4_0 (MMC2_bank0);
			logerror("MMC2 VROM switch #1 (low): %02x\n", MMC2_bank0);
			break;
		case 0x5000:
			MMC2_bank1 = data;
			if (MMC2_bank1_latch == 0xfd)
				chr4_4 (MMC2_bank1);
			logerror("MMC2 VROM switch #2 (low): %02x\n", data);
			break;
		case 0x6000:
			MMC2_bank1_hi = data;
			if (MMC2_bank1_latch == 0xfe)
				chr4_4 (MMC2_bank1_hi);
			logerror("MMC2 VROM switch #2 (high): %02x\n", data);
			break;
		case 0x7000:
			if (data)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			break;
		default:
			logerror("MMC2 uncaught w: %04x:%02x\n", offset, data);
			break;
	}
}
#endif

void mapper10_latch (int offset)
{
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 low): %02x\n", MMC2_bank0);
		MMC2_bank0_latch = 0xfd;
		chr4_0 (MMC2_bank0);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 high): %02x\n", MMC2_bank0_hi);
		MMC2_bank0_latch = 0xfe;
		chr4_0 (MMC2_bank0_hi);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		logerror ("mapper10 vrom latch switch (bank 1 low): %02x\n", MMC2_bank1);
		MMC2_bank1_latch = 0xfd;
		chr4_4 (MMC2_bank1);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 high): %02x\n", MMC2_bank1_hi);
		MMC2_bank1_latch = 0xfe;
		chr4_4 (MMC2_bank1_hi);
	}
}

void mapper10_w (int offset, int data)
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 8k prg bank */
			cpu_setbank (1, &nes.rom[data * 0x2000 + 0x10000]);
			break;
		case 0x3000:
			MMC2_bank0 = data;
			if (MMC2_bank0_latch == 0xfd)
				chr4_0 (MMC2_bank0);
			logerror("MMC2 VROM switch #1 (low): %02x\n", MMC2_bank0);
			break;
		case 0x4000:
			MMC2_bank0_hi = data;
			if (MMC2_bank0_latch == 0xfe)
				chr4_0 (MMC2_bank0_hi);
			logerror("MMC2 VROM switch #1 (high): %02x\n", MMC2_bank0_hi);
			break;
		case 0x5000:
			MMC2_bank1 = data;
			if (MMC2_bank1_latch == 0xfd)
				chr4_4 (MMC2_bank1);
			logerror("MMC2 VROM switch #2 (low): %02x\n", data);
			break;
		case 0x6000:
			MMC2_bank1_hi = data;
			if (MMC2_bank1_latch == 0xfe)
				chr4_4 (MMC2_bank1_hi);
			logerror("MMC2 VROM switch #2 (high): %02x\n", data);
			break;
		case 0x7000:
			if (data)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			break;
		default:
			logerror("MMC4 uncaught w: %04x:%02x\n", offset, data);
			break;
	}
}

static void mapper11_w (int offset, int data)
{
#ifdef LOG_MMC
	logerror("* Mapper 11 switch, data: %02x\n", data);
#endif
	/* Switch 8k VROM bank */
	chr8 (data >> 4);

	/* Switch 32k prg bank */
	prg32 (data & 0x0f);
}

static void mapper15_w (int offset, int data)
{
	int bank = (data & (nes.prg_chunks - 1)) * 0x4000;
	int base = data & 0x80 ? 0x12000 : 0x10000;

	switch (offset)
	{
		case 0x0000:
			if (data & 0x40)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			cpu_setbank (1, &nes.rom[bank + base]);
			cpu_setbank (2, &nes.rom[bank + (base ^ 0x2000)]);
			cpu_setbank (3, &nes.rom[bank + (base ^ 0x4000)]);
			cpu_setbank (4, &nes.rom[bank + (base ^ 0x6000)]);
			break;

		case 0x0001:
			cpu_setbank (3, &nes.rom[bank + base]);
			cpu_setbank (4, &nes.rom[bank + (base ^ 0x2000)]);
			break;

		case 0x0002:
			cpu_setbank (1, &nes.rom[bank + base]);
			cpu_setbank (2, &nes.rom[bank + base]);
			cpu_setbank (3, &nes.rom[bank + base]);
			cpu_setbank (4, &nes.rom[bank + base]);
        	break;

		case 0x0003:
			if (data & 0x40)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			cpu_setbank (3, &nes.rom[bank + base]);
			cpu_setbank (4, &nes.rom[bank + (base ^ 0x2000)]);
        	break;
	}
}

int bandai_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			ret = M6502_INT_IRQ;
		}
		IRQ_count -= 114;
	}

	return ret;
}

static void mapper16_w (int offset, int data)
{
	logerror ("mapper16 (mid and high) w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x000f)
	{
		case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
			/* Switch 1k VROM at $0000 - $1fff */
			nes_vram [offset & 0x0f] = data * 64;
			break;
		case 8:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;
		case 9:
			switch (data & 0x03)
			{
				case 0: ppu_mirror_h (); break;
				case 1: ppu_mirror_v (); break;
				case 2: ppu_mirror_low (); break;
				case 3: ppu_mirror_high (); break;
			}
			break;
		case 0x0a:
			IRQ_enable = data & 0x01;
			break;
		case 0x0b:
			IRQ_count &= 0xff00;
			IRQ_count |= data;
			break;
		case 0x0c:
			IRQ_count &= 0x00ff;
			IRQ_count |= (data << 8);
			break;
		default:
			logerror("** uncaught mapper 16 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

static void mapper17_l_w (int offset, int data)
{
	switch (offset)
	{
		/* $42fe - mirroring */
		case 0x1fe:
			if (data & 0x10)
				ppu_mirror_low ();
			else
				ppu_mirror_high ();
			break;
		/* $42ff - mirroring */
		case 0x1ff:
			if (data & 0x10)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			break;
		/* $4501 - $4503 */
//		case 0x401:
//		case 0x402:
//		case 0x403:
			/* IRQ control */
//			break;
		/* $4504 - $4507 : 8k PRG-Rom switch */
		case 0x404:
		case 0x405:
		case 0x406:
		case 0x407:
			data &= ((nes.prg_chunks << 1) - 1);
//			logerror("Mapper 17 bank switch, bank: %02x, data: %02x\n", offset & 0x03, data);
			cpu_setbank ((offset & 0x03) + 1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		/* $4510 - $4517 : 1k CHR-Rom switch */
		case 0x410:
		case 0x411:
		case 0x412:
		case 0x413:
		case 0x414:
		case 0x415:
		case 0x416:
		case 0x417:
			data &= ((nes.chr_chunks << 3) - 1);
			nes_vram[offset & 0x07] = data * 64;
			break;
		default:
			logerror("** uncaught mapper 17 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

int jaleco_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	if (scanline <= BOTTOM_VISIBLE_SCANLINE)
	{
		/* Increment & check the IRQ scanline counter */
		if (IRQ_enable)
		{
			IRQ_count -= 0x100;

			logerror ("scanline: %d, irq count: %04x\n", scanline, IRQ_count);
			if (IRQ_mode_jaleco & 0x08)
			{
				if ((IRQ_count & 0x0f) == 0x00)
					/* rollover every 0x10 */
					ret = M6502_INT_IRQ;
			}
			else if (IRQ_mode_jaleco & 0x04)
			{
				if ((IRQ_count & 0x0ff) == 0x00)
					/* rollover every 0x100 */
					ret = M6502_INT_IRQ;
			}
			else if (IRQ_mode_jaleco & 0x02)
			{
				if ((IRQ_count & 0x0fff) == 0x000)
					/* rollover every 0x1000 */
					ret = M6502_INT_IRQ;
			}
			else if (IRQ_count == 0)
				/* rollover at 0x10000 */
				ret = M6502_INT_IRQ;
		}
	}
	else
	{
		IRQ_count = IRQ_count_latch;
	}


	return ret;
}


static void mapper18_w (int offset, int data)
{
//	static int irq;
	static int bank_8000 = 0;
	static int bank_a000 = 0;
	static int bank_c000 = 0;

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 - low 4 bits */
			bank_8000 = (bank_8000 & 0xf0) | (data & 0x0f);
			bank_8000 &= prg_mask;
			cpu_setbank (1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $8000 - high 4 bits */
			bank_8000 = (bank_8000 & 0x0f) | (data << 4);
			bank_8000 &= prg_mask;
			cpu_setbank (1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0002:
			/* Switch 8k bank at $a000 - low 4 bits */
			bank_a000 = (bank_a000 & 0xf0) | (data & 0x0f);
			bank_a000 &= prg_mask;
			cpu_setbank (2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x0003:
			/* Switch 8k bank at $a000 - high 4 bits */
			bank_a000 = (bank_a000 & 0x0f) | (data << 4);
			bank_a000 &= prg_mask;
			cpu_setbank (2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x1000:
			/* Switch 8k bank at $c000 - low 4 bits */
			bank_c000 = (bank_c000 & 0xf0) | (data & 0x0f);
			bank_c000 &= prg_mask;
			cpu_setbank (3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;
		case 0x1001:
			/* Switch 8k bank at $c000 - high 4 bits */
			bank_c000 = (bank_c000 & 0x0f) | (data << 4);
			bank_c000 &= prg_mask;
			cpu_setbank (3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;

		/* $9002, 3 (1002, 3) uncaught = Jaleco Baseball writes 0 */
		/* believe it's related to battery-backed ram enable/disable */

		case 0x2000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x2001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x2002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x2003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x3000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x3001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x3002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x3003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x4000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x4001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x4002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x4003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x5000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x5001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x5002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;
		case 0x5003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;

/* LBO - these are unverified */

		case 0x6000: /* IRQ scanline counter - low byte, low nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xfff0) | (data & 0x0f);
			logerror("     Mapper 18 copy/set irq latch (l-l): %02x\n", data);
			break;
		case 0x6001: /* IRQ scanline counter - low byte, high nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xff0f) | ((data & 0x0f) << 4);
			logerror("     Mapper 18 copy/set irq latch (l-h): %02x\n", data);
			break;
		case 0x6002:
			IRQ_count_latch = (IRQ_count_latch & 0xf0ff) | ((data & 0x0f) << 9);
			logerror("     Mapper 18 copy/set irq latch (h-l): %02x\n", data);
			break;
		case 0x6003:
			IRQ_count_latch = (IRQ_count_latch & 0x0fff) | ((data & 0x0f) << 13);
			logerror("     Mapper 18 copy/set irq latch (h-h): %02x\n", data);
			break;

/* LBO - these 2 are likely wrong */

		case 0x7000: /* IRQ Control 0 */
			if (data & 0x01)
			{
//				IRQ_enable = 1;
				IRQ_count = IRQ_count_latch;
			}
			logerror("     Mapper 18 IRQ Control 0: %02x\n", data);
			break;
		case 0x7001: /* IRQ Control 1 */
			IRQ_enable = data & 0x01;
			IRQ_mode_jaleco = data & 0x0e;
			logerror("     Mapper 18 IRQ Control 1: %02x\n", data);
			break;

		case 0x7002: /* Misc */
			switch (data & 0x03)
			{
				case 0: ppu_mirror_low (); break;
				case 1: ppu_mirror_high (); break;
				case 2: ppu_mirror_v (); break;
				case 3: ppu_mirror_h (); break;
			}
			break;

/* $f003 uncaught, writes 0(start) & various(ingame) */
//		case 0x7003:
//			IRQ_count = data;
//			break;

		default:
			logerror("Mapper 18 uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

int namcot_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	IRQ_count ++;
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (IRQ_count == 0x7fff))
	{
		ret = M6502_INT_IRQ;
	}

	return ret;
}

static void mapper19_l_w (int offset, int data)
{
	switch (offset & 0x1800)
	{
		case 0x1000:
			/* low byte of IRQ */
			IRQ_count = (IRQ_count & 0x7f00) | data;
			break;
		case 0x1800:
			/* high byte of IRQ, IRQ enable in high bit */
			IRQ_count = (IRQ_count & 0xff) | ((data & 0x7f) << 8);
			IRQ_enable = data & 0x80;
			break;
	}
}

static void mapper19_w (int offset, int data)
{
	switch (offset & 0x7800)
	{
		case 0x0000:
			/* Switch 1k VROM at $0000 */
			nes_vram [0] = data * 64;
			break;
		case 0x0800:
			/* Switch 1k VROM at $0400 */
			nes_vram [1] = data * 64;
			break;
		case 0x1000:
			/* Switch 1k VROM at $0800 */
			nes_vram [2] = data * 64;
			break;
		case 0x1800:
			/* Switch 1k VROM at $0c00 */
			nes_vram [3] = data * 64;
			break;
		case 0x2000:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x2800:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x3000:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x3800:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		case 0x6000:
			/* Switch 8k bank at $8000 */
			data &= prg_mask;
			cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x6800:
			/* Switch 8k bank at $a000 */
			data &= prg_mask;
			cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x7000:
			/* Switch 8k bank at $c000 */
			data &= prg_mask;
			cpu_setbank (3, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
	}
}

int fds_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	if (IRQ_enable_latch)
		ret = M6502_INT_IRQ;

	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			ret = M6502_INT_IRQ;
			IRQ_enable = 0;
			nes_fds.status0 |= 0x01;
		}
		else
			IRQ_count -= 114;
	}

	return ret;
}

READ_HANDLER ( fds_r )
{
	data_t ret = 0x00;
	static int last_side = 0;
	static int count = 0;

	switch (offset)
	{
		case 0x00: /* $4030 - disk status 0 */
			ret = nes_fds.status0;
			/* clear the disk IRQ detect flag */
			nes_fds.status0 &= ~0x01;
			break;
		case 0x01: /* $4031 - data latch */
			if (nes_fds.current_side)
				ret = nes_fds.data[(nes_fds.current_side-1) * 65500 + nes_fds.head_position++];
			else
				ret = 0;
			break;
		case 0x02: /* $4032 - disk status 1 */
			/* If we've switched disks, report "no disk" for a few reads */
			if (last_side != nes_fds.current_side)
			{
				ret = 1;
				count ++;
				if (count == 50)
				{
					last_side = nes_fds.current_side;
					count = 0;
				}
			}
			else
				ret = (nes_fds.current_side == 0); /* 0 if a disk is inserted */
			break;
		case 0x03: /* $4033 */
			ret = 0x80;
			break;
		default:
			ret = 0x00;
			break;
	}

#ifdef LOG_FDS
	logerror ("fds_r, address: %04x, data: %02x\n", offset + 0x4030, ret);
#endif

	return ret;
}

WRITE_HANDLER ( fds_w )
{
	switch (offset)
	{
		case 0x00: /* $4020 */
			IRQ_count_latch &= ~0xff;
			IRQ_count_latch |= data;
			break;
		case 0x01: /* $4021 */
			IRQ_count_latch &= ~0xff00;
			IRQ_count_latch |= (data << 8);
			break;
		case 0x02: /* $4022 */
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data;
			break;
		case 0x03: /* $4023 */
			// d0 = sound io (1 = enable)
			// d1 = disk io (1 = enable)
			break;
		case 0x04: /* $4024 */
			/* write data out to disk */
			break;
		case 0x05: /* $4025 */
			nes_fds.motor_on = data & 0x01;
			if (data & 0x02) nes_fds.head_position = 0;
			nes_fds.read_mode = data & 0x04;
			if (data & 0x08)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			if ((!(data & 0x40)) && (nes_fds.write_reg & 0x40))
				nes_fds.head_position -= 2; // ???
			IRQ_enable_latch = data & 0x80;

			nes_fds.write_reg = data;
			break;
	}

#ifdef LOG_FDS
	logerror ("fds_w, address: %04x, data: %02x\n", offset + 0x4020, data);
#endif

}

int konami_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (++IRQ_count == 0x100))
	{
		IRQ_count = IRQ_count_latch;
		IRQ_enable = IRQ_enable_latch;
		ret = M6502_INT_IRQ;
	}

	return ret;
}

static void konami_vrc2a_w (int offset, int data)
{
	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				/* Switch 8k bank at $8000 */
				data &= ((nes.prg_chunks << 1) - 1);
				cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: ppu_mirror_v (); break;
					case 0x01: ppu_mirror_h (); break;
					case 0x02: ppu_mirror_low (); break;
					case 0x03: ppu_mirror_high (); break;
				}
				break;

			case 0x2000:
				/* Switch 8k bank at $a000 */
				data &= ((nes.prg_chunks << 1) - 1);
				cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			default:
				logerror("konami_vrc2a_w uncaught offset: %04x value: %02x\n", offset, data);
				break;
		}
		return;
	}

	switch (offset & 0x7000)
	{
		case 0x3000:
			/* Switch 1k vrom at $0000 */
			vrom_bank[0] = data >> 1;
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x3001:
			/* Switch 1k vrom at $0400 */
			vrom_bank[1] = data >> 1;
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 */
			vrom_bank[2] = data >> 1;
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x4001:
			/* Switch 1k vrom at $0c00 */
			vrom_bank[3] = data >> 1;
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 */
			vrom_bank[4] = data >> 1;
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x5001:
			/* Switch 1k vrom at $1400 */
			vrom_bank[5] = data >> 1;
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 */
			vrom_bank[6] = data >> 1;
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x6001:
			/* Switch 1k vrom at $1c00 */
			vrom_bank[7] = data >> 1;
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;

		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc2a_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static void konami_vrc2b_w (int offset, int data)
{
	UINT16 select;

//	logerror("konami_vrc2b_w offset: %04x value: %02x\n", offset, data);

	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				/* Switch 8k bank at $8000 */
				data &= ((nes.prg_chunks << 1) - 1);
				cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: ppu_mirror_v (); break;
					case 0x01: ppu_mirror_h (); break;
					case 0x02: ppu_mirror_low (); break;
					case 0x03: ppu_mirror_high (); break;
				}
				break;

			case 0x2000:
				/* Switch 8k bank at $a000 */
				data &= ((nes.prg_chunks << 1) - 1);
				cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			default:
				logerror("konami_vrc2b_w offset: %04x value: %02x\n", offset, data);
				break;
		}
		return;
	}

	/* The low 2 select bits vary from cart to cart */
	select = (offset & 0x7000) |
		(offset & 0x03) |
		((offset & 0x0c) >> 2);

	switch (select)
	{
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x3001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x3002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x3003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x4001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x4002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x4003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x5001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x5002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x5003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x6001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x6002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;
		case 0x6003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc2b_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static void konami_vrc4_w (int offset, int data)
{
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((nes.prg_chunks << 1) - 1);
			cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x1000:
			switch (data & 0x03)
			{
				case 0x00: ppu_mirror_v (); break;
				case 0x01: ppu_mirror_h (); break;
				case 0x02: ppu_mirror_low (); break;
				case 0x03: ppu_mirror_high (); break;
			}
			break;

		/* $1001 is uncaught */

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= ((nes.prg_chunks << 1) - 1);
			cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x3002:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
			break;
		case 0x3001:
		case 0x3004:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x3003:
		case 0x3006:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x4002:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
			break;
		case 0x4001:
		case 0x4004:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x4003:
		case 0x4006:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x5002:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
			break;
		case 0x5001:
		case 0x5004:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x5003:
		case 0x5006:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x6002:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
			break;
		case 0x6001:
		case 0x6004:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;
		case 0x6003:
		case 0x6006:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
			break;
		case 0x7000:
			/* Set low 4 bits of latch */
			IRQ_count_latch &= ~0x0f;
			IRQ_count_latch |= data & 0x0f;
//			logerror("konami_vrc4 irq_latch low: %02x\n", IRQ_count_latch);
			break;
		case 0x7002:
		case 0x7040:
			/* Set high 4 bits of latch */
			IRQ_count_latch &= ~0xf0;
			IRQ_count_latch |= (data << 4) & 0xf0;
//			logerror("konami_vrc4 irq_latch high: %02x\n", IRQ_count_latch);
			break;
		case 0x7004:
		case 0x7001:
		case 0x7080:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
//			logerror("konami_vrc4 irq_count set: %02x\n", IRQ_count);
//			logerror("konami_vrc4 enable: %02x\n", IRQ_enable);
//			logerror("konami_vrc4 enable latch: %02x\n", IRQ_enable_latch);
			break;
		case 0x7006:
		case 0x7003:
		case 0x70c0:
			IRQ_enable = IRQ_enable_latch;
//			logerror("konami_vrc4 enable copy: %02x\n", IRQ_enable);
			break;
		default:
			logerror("konami_vrc4_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static void konami_vrc6a_w (int offset, int data)
{
//	logerror("konami_vrc6_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: ppu_mirror_v (); break;
				case 0x04: ppu_mirror_h (); break;
				case 0x08: ppu_mirror_low (); break;
				case 0x0c: ppu_mirror_high (); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;
		case 0x5000:
			/* Switch 1k VROM at $0000 */
			nes_vram [0] = data * 64;
			break;
		case 0x5001:
			/* Switch 1k VROM at $0400 */
			nes_vram [1] = data * 64;
			break;
		case 0x5002:
			/* Switch 1k VROM at $0800 */
			nes_vram [2] = data * 64;
			break;
		case 0x5003:
			/* Switch 1k VROM at $0c00 */
			nes_vram [3] = data * 64;
			break;
		case 0x6000:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x6001:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x6002:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x6003:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7002:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}


static void konami_vrc6b_w (int offset, int data)
{
//	logerror("konami_vrc6_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: ppu_mirror_v (); break;
				case 0x04: ppu_mirror_h (); break;
				case 0x08: ppu_mirror_low (); break;
				case 0x0c: ppu_mirror_high (); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;
		case 0x5000:
			/* Switch 1k VROM at $0000 */
			nes_vram [0] = data * 64;
			break;
		case 0x5002:
			/* Switch 1k VROM at $0400 */
			nes_vram [1] = data * 64;
			break;
		case 0x5001:
			/* Switch 1k VROM at $0800 */
			nes_vram [2] = data * 64;
			break;
		case 0x5003:
			/* Switch 1k VROM at $0c00 */
			nes_vram [3] = data * 64;
			break;
		case 0x6000:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x6002:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x6001:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x6003:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper32_w (int offset, int data)
{
	static int bankSel;

//	logerror("mapper32_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 or $c000 */
			if (bankSel)
				prg8_cd (data);
			else
				prg8_89 (data);
			break;
		case 0x1000:
			bankSel = data & 0x02;
			if (data & 0x01)
				ppu_mirror_h ();
			else
				ppu_mirror_v ();
			break;
		case 0x2000:
			/* Switch 8k bank at $A000 */
			prg8_ab (data);
			break;
		case 0x3000:
			/* Switch 1k VROM at $1000 */
			nes_vram[offset & 0x07] = data * 64;
			break;
		default:
			logerror("Uncaught mapper 32 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper33_w (int offset, int data)
{

//	logerror("mapper33_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((nes.prg_chunks << 1) - 1);
			cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $A000 */
			data &= ((nes.prg_chunks << 1) - 1);
			cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x0002:
			/* Switch 2k VROM at $0000 */
			nes_vram [0] = data * 128;
			nes_vram [1] = nes_vram [0] + 64;
			break;
		case 0x0003:
			/* Switch 2k VROM at $0800 */
			nes_vram [2] = data * 128;
			nes_vram [3] = nes_vram [2] + 64;
			break;
		case 0x2000:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x2001:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x2002:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x2003:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		default:
			logerror("Uncaught mapper 33 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper34_m_w (int offset, int data)
{
	logerror("mapper34_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ffd:
			/* Switch 32k prg banks */
			prg32 (data);
			break;
		case 0x1ffe:
			/* Switch 4k VNES_ROM at 0x0000 */
			chr4_0 (data);
			break;
		case 0x1fff:
			/* Switch 4k VNES_ROM at 0x1000 */
			chr4_4 (data);
			break;
	}
}


static void mapper34_w (int offset, int data)
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a Mapper 34 game - the demo screens look wrong using mapper 7. */
	logerror("Mapper 34 w, offset: %04x, data: %02x\n", offset, data);

	prg32 (data);
}

int mapper40_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	/* Decrement & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
		{
			ret = M6502_INT_IRQ;
		}
	}

	return ret;
}

static void mapper40_w (int offset, int data)
{
	logerror("mapper40_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x6000)
	{
		case 0x0000:
			IRQ_enable = 0;
			IRQ_count = 37; /* Hardcoded scanline scroll */
			break;
		case 0x2000:
			IRQ_enable = 1;
			break;
		case 0x6000:
			prg8_cd (data & 0x07);
			break;
	}
}

static void mapper64_m_w (int offset, int data)
{
	logerror("mapper64_m_w, offset: %04x, data: %02x\n", offset, data);
}

static void mapper64_w (int offset, int data)
{
	static int cmd = 0;
	static int chr = 0;
	static int select_high;
	static int page;

//	logerror("mapper64_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7001)
	{
		case 0x0000:
//			logerror("Mapper 64 0x8000 write value: %02x\n",data);
			cmd = data & 0x0f;
			if (data & 0x80)
				chr = 0x1000;
			else
				chr = 0x0000;

			if (data & 0x10)
			{
				nes_vram[1] = nes_vram[3] = 0;
			}

			page = chr >> 10;
			/* Toggle switching between $8000/$A000/$C000 and $A000/$C000/$8000 */
			if (select_high != (data & 0x40))
			{
				if (data & 0x40)
				{
					cpu_setbank (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
				}
				else
				{
					cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
				}
			}

			select_high = data & 0x40;
//			logerror("   Mapper 64 select_high: %02x\n", select_high);
			break;

		case 0x0001:
			switch (cmd)
			{
				case 0:
					nes_vram [page] = data * 64;
					nes_vram [page+1] = nes_vram [page] + 64;
					break;

				case 1:
					nes_vram [page ^ 2] = data * 64;
					nes_vram [(page ^ 2)+1] = nes_vram [page ^ 2] + 64;
					break;

				case 2:
					nes_vram [page ^ 4] = data * 64;
					break;

				case 3:
					nes_vram [page ^ 5] = data * 64;
					break;

				case 4:
					nes_vram [page ^ 6] = data * 64;
					break;

				case 5:
					nes_vram [page ^ 7] = data * 64;
					break;

				case 6:
					/* These damn games will go to great lengths to switch to banks which are outside the valid range */
					data &= prg_mask;
					if (select_high)
					{
						cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 6 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($8000) cmd 6 value: %02x\n", data);
					}
					break;
				case 7:
					data &= prg_mask;
					if (select_high)
					{
						cpu_setbank (3, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($C000) cmd 7 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 7 value: %02x\n", data);
					}
					break;
				case 8:
					/* Switch 1k VROM at $0400 */
//					nes_vram [1] = data * 64;
//					nes_vram [1] = 0;
					break;
				case 9:
					/* Switch 1k VROM at $0C00 */
//					nes_vram [3] = data * 64;
//					nes_vram [3] = 0;
					break;
				case 15:
					data &= prg_mask;
					if (select_high)
					{
						cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($C000) cmd 15 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (3, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 15 value: %02x\n", data);
					}
					break;
			}
			cmd = 16;
			break;
		case 0x2000:
//			logerror("     Mapper 64 mirroring: %02x\n", data);
			if (data & 0x01)
				ppu_mirror_v();
			else
				ppu_mirror_h();
			break;
		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
			logerror("     MMC3 copy/set irq latch: %02x\n", data);
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
			logerror("     MMC3 set latch: %02x\n", data);
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			logerror("     MMC3 disable irqs: %02x\n", data);
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			logerror("     MMC3 enable irqs: %02x\n", data);
			break;

		default:
			logerror("Mapper 64 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

int irem_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
			ret = M6502_INT_IRQ;
	}

	return ret;
}



static void mapper65_w (int offset, int data)
{
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= prg_mask;
			cpu_setbank (1, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($8000) value: %02x\n", data);
#endif
			break;
		case 0x1001:
			if (data & 0x80)
				ppu_mirror_h();
			else
				ppu_mirror_v();
			break;
		case 0x1005:
			IRQ_count = data << 1;
			break;
		case 0x1006:
			IRQ_enable = IRQ_count;
			break;

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= prg_mask;
			cpu_setbank (2, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($a000) value: %02x\n", data);
#endif
			break;

		case 0x3000:
			/* Switch 1k VROM at $0000 */
			nes_vram [0] = data * 64;
			break;
		case 0x3001:
			/* Switch 1k VROM at $0400 */
			nes_vram [1] = data * 64;
			break;
		case 0x3002:
			/* Switch 1k VROM at $0800 */
			nes_vram [2] = data * 64;
			break;
		case 0x3003:
			/* Switch 1k VROM at $0c00 */
			nes_vram [3] = data * 64;
			break;
		case 0x3004:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x3005:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x3006:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x3007:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		case 0x4000:
			/* Switch 8k bank at $c000 */
			data &= prg_mask;
			cpu_setbank (3, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($c000) value: %02x\n", data);
#endif
			break;

		default:
			logerror("Mapper 65 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper66_w (int offset, int data)
{
#ifdef LOG_MMC
	logerror("* Mapper 66 switch, offset %04x, data: %02x\n", offset, data);
#endif

	prg32 ((data & 0x30) >> 4);
	chr8 (data & 0x03);
}

int sunsoft_irq (int scanline)
{
	int ret = M6502_INT_NONE;

	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			ret = M6502_INT_IRQ;
		}
		IRQ_count -= 114;
	}

	return ret;
}


static void mapper67_w (int offset, int data)
{
//	logerror("mapper67_w, offset %04x, data: %02x\n", offset, data);
	switch (offset & 0x7801)
	{
//		case 0x0000: /* IRQ disable? */
//			IRQ_enable = 0;
//			break;
		case 0x0800:
			chr2_0 (data);
			break;
		case 0x1800:
			chr2_2 (data);
			break;
		case 0x2800:
			chr2_4 (data);
			break;
		case 0x3800:
			chr2_6 (data);
			break;
//		case 0x4800:
//			nes_vram[5] = data * 64;
//			break;
		case 0x4801:
			/* IRQ count? */
			IRQ_count = IRQ_count_latch;
			IRQ_count_latch = data;
			break;
//		case 0x5800:
//			chr4_0 (data);
//			break;
		case 0x5801:
			IRQ_enable = data;
			break;
//		case 0x6800:
//			chr4_4 (data);
//			break;
		case 0x7800:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper67_w uncaught offset: %04x, data: %02x\n", offset, data);
			break;

	}
}

static void mapper68_mirror (int m68_mirror, int m0, int m1)
{
	/* The 0x20000 (128k) offset is a magic number */
	#define M68_OFFSET 0x20000

	switch (m68_mirror)
	{
		case 0x00: ppu_mirror_h (); break;
		case 0x01: ppu_mirror_v (); break;
		case 0x02: ppu_mirror_low (); break;
		case 0x03: ppu_mirror_high (); break;
		case 0x10:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
		case 0x11:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
		case 0x12:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m0 << 10) + M68_OFFSET);
			break;
		case 0x13:
			ppu_mirror_custom_vrom (0, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
	}
}

static void mapper68_w (int offset, int data)
{
	static int m68_mirror;
	static int m0, m1;

	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 2k VROM at $0000 */
			chr2_0 (data);
			break;
		case 0x1000:
			/* Switch 2k VROM at $0800 */
			chr2_2 (data);
			break;
		case 0x2000:
			/* Switch 2k VROM at $1000 */
			chr2_4 (data);
			break;
		case 0x3000:
			/* Switch 2k VROM at $1800 */
			chr2_6 (data);
			break;
		case 0x4000:
			m0 = data;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x5000:
			m1 = data;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x6000:
			m68_mirror = data & 0x13;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x7000:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper68_w uncaught offset: %04x, data: %02x\n", offset, data);
			break;

	}
}

static void mapper69_w (int offset, int data)
{
	static int cmd = 0;

//	logerror("mapper69_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7000)
	{
		case 0x0000:
			cmd = data & 0x0f;
			break;

		case 0x2000:
			switch (cmd)
			{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					nes_vram [cmd] = data * 64;
					break;

				/* TODO: deal with bankswitching/write-protecting the mid-mapper area */
				case 8:
					if (!(data & 0x40))
					{
						cpu_setbank (5, &nes.rom[(data & 0x3f) * 0x2000 + 0x10000]);
					}
					else
						logerror ("mapper69_w, cmd 8, data: %02x\n", data);
					break;

				case 9:
					prg8_89 (data);
					break;
				case 10:
					prg8_ab (data);
					break;
				case 11:
					prg8_cd (data);
					break;
				case 12:
					switch (data & 0x03)
					{
						case 0x00: ppu_mirror_v (); break;
						case 0x01: ppu_mirror_h (); break;
						case 0x02: ppu_mirror_low (); break;
						case 0x03: ppu_mirror_high (); break;
					}
					break;
				case 13:
					IRQ_enable = data;
					break;
				case 14:
					IRQ_count = (IRQ_count & 0xff00) | data;
					break;
				case 15:
					IRQ_count = (IRQ_count & 0x00ff) | (data << 8);
					break;
			}
			break;

		default:
			logerror("mapper69_w uncaught %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper70_w (int offset, int data)
{
	logerror("mapper70_w offset %04x, data: %02x\n", offset, data);

	/* TODO: is the data being written irrelevant? */

	prg16_89ab ((data & 0xf0) >> 4);

	chr8 (data & 0x0f);

#if 1
	if (data & 0x80)
//		ppu_mirror_h();
		ppu_mirror_high();
	else
//		ppu_mirror_v();
		ppu_mirror_low();
#endif
}

static void mapper71_m_w (int offset, int data)
{
	logerror("mapper71_m_w offset: %04x, data: %02x\n", offset, data);

	prg16_89ab (data);
}

static void mapper71_w (int offset, int data)
{
	logerror("mapper71_w offset: %04x, data: %02x\n", offset, data);

	if (offset >= 0x4000)
		prg16_89ab (data);
}

static void mapper72_w (int offset, int data)
{
	logerror("mapper72_w, offset %04x, data: %02x\n", offset, data);
	/* This routine is busted */

//	prg32 ((data & 0xf0) >> 4);
//	prg16_89ab (data & 0x0f);
//	prg16_89ab ((data & 0xf0) >> 4);
//	chr8 (data & 0x0f);
}

static void mapper73_w (int offset, int data)
{
	switch (offset & 0x7000)
	{
		case 0x0000:
		case 0x1000:
			/* dunno which address controls these */
			IRQ_count_latch = data;
			IRQ_enable_latch = data;
			break;
		case 0x2000:
			IRQ_enable = data;
			break;
		case 0x3000:
			IRQ_count &= ~0x0f;
			IRQ_count |= data & 0x0f;
			break;
		case 0x4000:
			IRQ_count &= ~0xf0;
			IRQ_count |= (data & 0x0f) << 4;
			break;
		case 0x7000:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper73_w uncaught, offset %04x, data: %02x\n", offset, data);
			break;
	}
}


static void mapper75_w (int offset, int data)
{
	logerror("mapper75_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset & 0x7000)
	{
		case 0x0000:
			prg8_89 (data);
			break;
		case 0x1000: /* TODO: verify */
			if (data & 0x01)
				ppu_mirror_h();
			else
				ppu_mirror_v();
			/* vrom banking as well? */
			break;
		case 0x2000:
			prg8_ab (data);
			break;
		case 0x4000:
			prg8_cd (data);
			break;
		case 0x6000:
			chr4_0 (data);
			break;
		case 0x7000:
			chr4_4 (data);
			break;
	}
}

static void mapper77_w (int offset, int data)
{

/* Mapper is busted */

	logerror("mapper77_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset)
	{
		case 0x7c84:
//			prg8_89 (data);
			break;
		case 0x7c86:
//			prg8_89 (data);
//			prg8_ab (data);
//			prg8_cd (data);
//			prg16_89ab (data); /* red screen */
//			prg16_cdef (data);
			break;
	}
}

static void mapper78_w (int offset, int data)
{
	logerror("mapper78_w, offset: %04x, data: %02x\n", offset, data);
	/* Switch 8k VROM bank */
	chr8 ((data & 0xf0) >> 4);

	/* Switch 16k ROM bank */
	prg16_89ab (data & 0x0f);
}

static void mapper79_l_w (int offset, int data)
{
	logerror("mapper79_l_w: %04x:%02x\n", offset, data);

	if ((offset-0x100) & 0x0100)
	{
		/* Select 8k VROM bank */
		chr8 (data & 0x07);

		/* Select 32k ROM bank */
		prg32 ((data & 0x08) >> 3);
	}
}

static void mapper79_w (int offset, int data)
{
	logerror("mapper79_w, offset: %04x, data: %02x\n", offset, data);
}

static void mapper80_m_w (int offset, int data)
{
	logerror("mapper80_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 */
			chr2_0 ((data & 0x7f) >> 1);
			if (data & 0x80)
			{
				/* Horizontal, $2000-$27ff */
				ppu_mirror_custom (0, 0);
				ppu_mirror_custom (1, 0);
			}
			else
			{
				/* Vertical, $2000-$27ff */
				ppu_mirror_custom (0, 0);
				ppu_mirror_custom (1, 1);
			}
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0000 */
			chr2_2 ((data & 0x7f) >> 1);
			if (data & 0x80)
			{
				/* Horizontal, $2800-$2fff */
				ppu_mirror_custom (2, 0);
				ppu_mirror_custom (3, 0);
			}
			else
			{
				/* Vertical, $2800-$2fff */
				ppu_mirror_custom (2, 0);
				ppu_mirror_custom (3, 1);
			}
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;
		case 0x1efa: case 0x1efb:
			/* Switch 8k ROM at $8000 */
			prg8_89 (data);
			break;
		case 0x1efc: case 0x1efd:
			/* Switch 8k ROM at $a000 */
			prg8_ab (data);
			break;
		case 0x1efe: case 0x1eff:
			/* Switch 8k ROM at $c000 */
			prg8_cd (data);
			break;
		default:
			logerror("mapper80_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

static void mapper82_m_w (int offset, int data)
{
	static int vrom_switch;

	/* This mapper has problems */

	logerror("mapper82_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 or $1000 */
			if (vrom_switch)
				chr2_4 (data);
			else
				chr2_0 (data);
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0800 or $1800 */
			if (vrom_switch)
				chr2_6 (data);
			else
				chr2_2 (data);
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			nes_vram [4 ^ vrom_switch] = data * 64;
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			nes_vram [5 ^ vrom_switch] = data * 64;
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			nes_vram [6 ^ vrom_switch] = data * 64;
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7 ^ vrom_switch] = data * 64;
			break;
		case 0x1ef6:
			vrom_switch = !((data & 0x02) << 1);
			break;

		case 0x1efa:
			/* Switch 8k ROM at $8000 */
			prg8_89 (data >> 2);
			break;
		case 0x1efb:
			/* Switch 8k ROM at $a000 */
			prg8_ab (data >> 2);
			break;
		case 0x1efc:
			/* Switch 8k ROM at $c000 */
			prg8_cd (data >> 2);
			break;
		default:
			logerror("mapper82_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

static void konami_vrc7_w (int offset, int data)
{
//	logerror("konami_vrc7_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7018)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			prg8_89 (data);
			break;
		case 0x0008: case 0x0010: case 0x0018:
			/* Switch 8k bank at $a000 */
			prg8_ab (data);
			break;

		case 0x1000:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;

/* TODO: there are sound regs in here */

		case 0x2000:
			/* Switch 1k VROM at $0000 */
			nes_vram [0] = data * 64;
			break;
		case 0x2008: case 0x2010: case 0x2018:
			/* Switch 1k VROM at $0400 */
			nes_vram [1] = data * 64;
			break;
		case 0x3000:
			/* Switch 1k VROM at $0800 */
			nes_vram [2] = data * 64;
			break;
		case 0x3008: case 0x3010: case 0x3018:
			/* Switch 1k VROM at $0c00 */
			nes_vram [3] = data * 64;
			break;
		case 0x4000:
			/* Switch 1k VROM at $1000 */
			nes_vram [4] = data * 64;
			break;
		case 0x4008: case 0x4010: case 0x4018:
			/* Switch 1k VROM at $1400 */
			nes_vram [5] = data * 64;
			break;
		case 0x5000:
			/* Switch 1k VROM at $1800 */
			nes_vram [6] = data * 64;
			break;
		case 0x5008: case 0x5010: case 0x5018:
			/* Switch 1k VROM at $1c00 */
			nes_vram [7] = data * 64;
			break;

		case 0x6000:
			switch (data & 0x03)
			{
				case 0x00: ppu_mirror_v (); break;
				case 0x01: ppu_mirror_h (); break;
				case 0x02: ppu_mirror_low (); break;
				case 0x03: ppu_mirror_high (); break;
			}
			break;
		case 0x6008: case 0x6010: case 0x6018:
			IRQ_count_latch = data;
			break;
		case 0x7000:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7008: case 0x7010: case 0x7018:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc7_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper86_w (int offset, int data)
{
	logerror("mapper86_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset)
	{
		case 0x7541:
			prg16_89ab (data >> 4);
//			prg16_cdef (data >> 4);
			break;
		case 0x7d41:
//			prg16_89ab (data >> 4);
//			prg16_cdef (data >> 4);
			break;
	}
}

static void mapper87_m_w (int offset, int data)
{
	logerror("mapper87_m_w %04x:%02x\n", offset, data);

	/* TODO: verify */
	chr8 (data >> 1);
}

static void mapper91_m_w (int offset, int data)
{
	logerror ("mapper91_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x7000)
	{
		case 0x0000:
			switch (offset & 0x03)
			{
				case 0x00: chr2_0 (data); break;
				case 0x01: chr2_2 (data); break;
				case 0x02: chr2_4 (data); break;
				case 0x03: chr2_6 (data); break;
			}
			break;
		case 0x1000:
			switch (offset & 0x01)
			{
				case 0x00: prg8_89 (data); break;
				case 0x01: prg8_ab (data); break;
			}
			break;
		default:
			logerror("mapper91_m_w uncaught addr: %04x value: %02x\n", offset + 0x6000, data);
			break;
	}
}

static void mapper228_w (int offset, int data)
{
	/* The address lines double as data */
	/* --mPPppppPP-cccc */
	int bank;
	int chr;

#ifdef LOG_MMC
	logerror ("mapper228_w, offset: %04x, data: %02x\n", offset, data);
#endif

	/* Determine low 4 bits of program bank */
	bank = (offset & 0x780) >> 7;

#if 0
	/* Determine high 2 bits of program bank */
	switch (offset & 0x1800)
	{
		case 0x0800:
			bank |= 0x10;
			break;
		case 0x1800:
			bank |= 0x20;
			break;
	}
#endif

	bank |= (offset & 0x1800) >> 7;

	/* see if the bank value is 16k or 32k */
	if (offset & 0x20)
	{
		/* 16k bank value, adjust */
		bank *= 2;
		if (offset & 0x40)
			bank ++;

		prg16_89ab (bank << 1);
		prg16_cdef (bank << 1);
	}
	else
	{
		prg32 (bank);
	}

	if (offset & 0x2000)
		ppu_mirror_h();
	else
		ppu_mirror_v();

	/* Now handle vrom banking */
	chr = (data & 0x03) + ((offset & 0x0f) << 2);
	chr8 (chr);
}

/*
// mapper_reset
//
// Resets the mapper bankswitch areas to their defaults. It returns a value "err"
// that indicates if it was successful. Possible values for err are:
//
// 0 = success
// 1 = no mapper found
// 2 = mapper not supported
*/
int mapper_reset (int mapperNum)
{
	int err = 0;
	int i;

	/* Set the vram bank-switch values to the default */
	for (i = 0; i < 8; i ++)
		nes_vram[i] = i * 64;

	mapper_warning = 0;
	/* 8k mask */
	prg_mask = ((nes.prg_chunks << 1) - 1);

	MMC5_vram_control = 0;

	/* Point the WRAM/battery area to the first RAM bank */
	cpu_setbank (5, &nes.wram[0x0000]);

	switch (mapperNum)
	{
		case 0:
			err = 1; /* No mapper found */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 1:
			/* Reset the latch */
			MMC1_reg = 0;
			MMC1_reg_count = 0;

			MMC1_Size_16k = 1;
			MMC1_Switch_Low = 1;
			MMC1_SizeVrom_4k = 0;
			MMC1_extended_bank = 0;
			MMC1_extended_swap = 0;
			MMC1_extended_base = 0x10000;
			MMC1_extended = ((nes.prg_chunks << 4) + nes.chr_chunks * 8) >> 9;

			if (!MMC1_extended)
				/* Set it to the end of the prg rom */
				MMC1_High = (nes.prg_chunks - 1) * 0x4000;
			else
				/* Set it to the end of the first 256k bank */
				MMC1_High = 15 * 0x4000;

			MMC1_bank1 = 0;
			MMC1_bank2 = 0x2000;
			MMC1_bank3 = MMC1_High;
			MMC1_bank4 = MMC1_High + 0x2000;

			cpu_setbank (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
			cpu_setbank (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
			cpu_setbank (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
			cpu_setbank (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
			logerror("-- page1: %06x\n", MMC1_bank1);
			logerror("-- page2: %06x\n", MMC1_bank2);
			logerror("-- page3: %06x\n", MMC1_bank3);
			logerror("-- page4: %06x\n", MMC1_bank4);
			break;
		case 2:
			/* These games don't switch VROM, but some ROMs incorrectly have CHR chunks */
			nes.chr_chunks = 0;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 3:
			/* Doesn't bank-switch */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 4:
			/* Can switch 8k prg banks */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch = 0;
			MMC3_prg0 = 0xfe;
			MMC3_prg1 = 0xff;
			MMC3_cmd = 0;
			cpu_setbank (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 5:
			/* Can switch 8k prg banks, but they are saved as 16k in size */
			MMC5_rom_bank_mode = 3;
			MMC5_vrom_bank_mode = 0;
			MMC5_vram_protect = 0;
			IRQ_enable = 0;
			IRQ_count = 0;
			nes.mid_ram_enable = 0;
			cpu_setbank (1, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x10000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			ppu_mirror_low ();
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 8:
			/* Switches 16k banks at $8000, 1st 2 16k banks loaded on reset */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 9:
			/* Can switch 8k prg banks */
			/* Note that the iNES header defines the number of banks as 8k in size, rather than 16k */
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 10:
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 11:
			/* Switches 32k banks, 1st 32k bank loaded on reset (?) May be more like mapper 7... */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 15:
			/* Can switch 8k prg banks */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 16:
		case 17:
		case 18:
		case 19:
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 20:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			nes_fds.motor_on = 0;
			nes_fds.door_closed = 0;
			nes_fds.current_side = 1;
			nes_fds.head_position = 0;
			nes_fds.status0 = 0;
			nes_fds.read_mode = nes_fds.write_reg = 0;
			break;
		case 21:
		case 25:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			/* fall through */
		case 22:
		case 23:
		case 32:
		case 33:
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 24:
		case 26:
		case 73:
		case 75:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 34:
			/* Can switch 32k prg banks */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 40:
			IRQ_enable = 0;
			IRQ_count = 0;
//			nes.mid_ram_enable = 0;
			/* Who's your daddy? */
			cpu_setbank (5, &nes.rom[6 * 0x2000 + 0x10000]);
//			cpu_setbank (1, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x10000]);
//			cpu_setbank (2, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x12000]);
//			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
//			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (1, &nes.rom[4 * 0x2000 + 0x10000]);
			cpu_setbank (2, &nes.rom[5 * 0x2000 + 0x10000]);
			cpu_setbank (3, &nes.rom[6 * 0x2000 + 0x10000]);
			cpu_setbank (4, &nes.rom[7 * 0x2000 + 0x10000]);
//			memcpy (&nes.rom[0x6000], &nes.rom[6 * 0x2000 + 0x10000], 0x2000);
//			memcpy (&nes.rom[0x8000], &nes.rom[4 * 0x2000 + 0x10000], 0x2000);
//			memcpy (&nes.rom[0xa000], &nes.rom[5 * 0x2000 + 0x10000], 0x2000);
//			memcpy (&nes.rom[0xc000], &nes.rom[6 * 0x2000 + 0x10000], 0x2000);
//			memcpy (&nes.rom[0xe000], &nes.rom[7 * 0x2000 + 0x10000], 0x2000);
			break;
		case 64:
			/* Can switch 3 8k prg banks */
			cpu_setbank (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 65:
			IRQ_enable = 0;
			IRQ_count = 0;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 66:
			/* Can switch 32k prgm banks */
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 70:
//		case 86:
//			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (1, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x10000]);
//			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-2) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 67:
		case 68:
		case 69:
		case 71:
		case 72:
		case 77:
		case 78:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 79:
			/* Mirroring always horizontal...? */
//			Mirroring = 1;
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[0x14000]);
			cpu_setbank (4, &nes.rom[0x16000]);
			break;
		case 80:
		case 82:
		case 85:
		case 86:
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
//		case 70:
		case 87:
		case 228:
			cpu_setbank (1, &nes.rom[0x10000]);
			cpu_setbank (2, &nes.rom[0x12000]);
//			cpu_setbank (3, &nes.rom[0x14000]);
//			cpu_setbank (4, &nes.rom[0x16000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		case 91:
			ppu_mirror_v();
//			cpu_setbank (1, &nes.rom[0x10000]);
//			cpu_setbank (2, &nes.rom[0x12000]);
			cpu_setbank (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (2, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			cpu_setbank (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x12000]);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}

	if (nes.chr_chunks)
		memcpy (videoram, nes.vrom, 0x2000);

	return err;
}

mmc mmc_list[] = {
/*	INES   DESC						LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, IRQ */
	{ 0, "No Mapper",				NULL, NULL, NULL, NULL, NULL, NULL },
	{ 1, "MMC1",					NULL, NULL, NULL, mapper1_w, NULL, NULL },
	{ 2, "74161/32 ROM sw",			NULL, NULL, NULL, mapper2_w, NULL, NULL },
	{ 3, "74161/32 VROM sw",		NULL, NULL, NULL, mapper3_w, NULL, NULL },
	{ 4, "MMC3",					NULL, NULL, NULL, mapper4_w, NULL, mapper4_irq },
	{ 5, "MMC5",					mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, mapper5_irq },
	{ 7, "ROM Switch",				NULL, NULL, NULL, mapper7_w, NULL, NULL },
	{ 8, "FFE F3xxxx",				NULL, NULL, NULL, mapper8_w, NULL, NULL },
//	{ 9, "MMC2",					NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL},
	{ 9, "MMC2",					NULL, NULL, NULL, mapper10_w, mapper10_latch, NULL},
	{ 10, "MMC4",					NULL, NULL, NULL, mapper10_w, mapper10_latch, NULL },
	{ 11, "Color Dreams Mapper",	NULL, NULL, NULL, mapper11_w, NULL, NULL },
	{ 15, "100-in-1",				NULL, NULL, NULL, mapper15_w, NULL, NULL },
	{ 16, "Bandai",					NULL, NULL, mapper16_w, mapper16_w, NULL, bandai_irq },
	{ 17, "FFE F8xxx",				mapper17_l_w, NULL, NULL, NULL, NULL, mapper4_irq },
	{ 18, "Jaleco",					NULL, NULL, NULL, mapper18_w, NULL, jaleco_irq },
	{ 19, "Namco 106",				mapper19_l_w, NULL, NULL, mapper19_w, NULL, namcot_irq },
	{ 20, "Famicom Disk System",	NULL, NULL, NULL, NULL, NULL, fds_irq },
	{ 21, "Konami VRC 4",			NULL, NULL, NULL, konami_vrc4_w, NULL, konami_irq },
	{ 22, "Konami VRC 2a",			NULL, NULL, NULL, konami_vrc2a_w, NULL, konami_irq },
	{ 23, "Konami VRC 2b",			NULL, NULL, NULL, konami_vrc2b_w, NULL, konami_irq },
	{ 24, "Konami VRC 6a",			NULL, NULL, NULL, konami_vrc6a_w, NULL, konami_irq },
	{ 25, "Konami VRC 4",			NULL, NULL, NULL, konami_vrc4_w, NULL, konami_irq },
	{ 26, "Konami VRC 6b",			NULL, NULL, NULL, konami_vrc6b_w, NULL, konami_irq },
	{ 32, "Irem G-101",				NULL, NULL, NULL, mapper32_w, NULL, NULL },
	{ 33, "Taito TC0190",			NULL, NULL, NULL, mapper33_w, NULL, NULL },
	{ 34, "Nina-1",					NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL },
	{ 40, "SMB2j (bootleg)",		NULL, NULL, NULL, mapper40_w, NULL, mapper40_irq },
// 41 - Caltron
	{ 64, "Tengen",					NULL, NULL, mapper64_m_w, mapper64_w, NULL, mapper4_irq },
	{ 65, "Irem H3001",				NULL, NULL, NULL, mapper65_w, NULL, irem_irq },
	{ 66, "74161/32 Jaleco",		NULL, NULL, NULL, mapper66_w, NULL, NULL },
	{ 67, "SunSoft 3",				NULL, NULL, NULL, mapper67_w, NULL, mapper4_irq },
	{ 68, "SunSoft 4",				NULL, NULL, NULL, mapper68_w, NULL, NULL },
	{ 69, "SunSoft 5",				NULL, NULL, NULL, mapper69_w, NULL, sunsoft_irq },
	{ 70, "74161/32 Bandai",		NULL, NULL, NULL, mapper70_w, NULL, NULL },
	{ 71, "Camerica",				NULL, NULL, mapper71_m_w, mapper71_w, NULL, NULL },
	{ 72, "74161/32 Jaleco",		NULL, NULL, NULL, mapper72_w, NULL, NULL },
	{ 73, "Konami VRC 3",			NULL, NULL, NULL, mapper73_w, NULL, konami_irq },
	{ 75, "Konami VRC 1",			NULL, NULL, NULL, mapper75_w, NULL, NULL },
	{ 77, "74161/32 ?",				NULL, NULL, NULL, mapper77_w, NULL, NULL },
	{ 78, "74161/32 Irem",			NULL, NULL, NULL, mapper78_w, NULL, NULL },
	{ 79, "Nina-3 (AVE)",			mapper79_l_w, NULL, mapper79_w, mapper79_w, NULL, NULL },
	{ 80, "Taito X1-005",			NULL, NULL, mapper80_m_w, NULL, NULL, NULL },
	{ 82, "Taito C075",				NULL, NULL, mapper82_m_w, NULL, NULL, NULL },
// 83
	{ 84, "Pasofami",				NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, "Konami VRC 7",			NULL, NULL, NULL, konami_vrc7_w, NULL, konami_irq },
	{ 86, "?",				NULL, NULL, NULL, mapper86_w, NULL, NULL },
	{ 87, "74161/32 VROM sw-a",		NULL, NULL, mapper87_m_w, NULL, NULL, NULL },
	{ 88, "Namco 118",				NULL, NULL, NULL, mapper4_w, NULL, mapper4_irq },
	{ 91, "HK-SF3 (bootleg)",		NULL, NULL, mapper91_m_w, NULL, NULL, NULL },
// 119 - Pinbot
	{ 228, "Action 52",				NULL, NULL, NULL, mapper228_w, NULL, NULL },
	{ -1, "Not Supported",			NULL, NULL, NULL, NULL, NULL, NULL },
};
