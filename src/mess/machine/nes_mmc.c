/*
	TODO:
	. Fix Mappers 9 & 10 (Punch-out)
	. Mapper 18 VROM switching is broken
	. Mapper 25 VROM switching is broken
	. Mapper 79 VROM switching is broken
	
	Also, remember that the MMC does not equal the mapper #. In particular, Mapper 4 is
	really MMC3, Mapper 9 is MMC2 and Mapper 10 is MMC4. Makes perfect sense, right?
*/
#include <string.h>

#include "vidhrdw/generic.h"
#include "osdepend.h"
#include "driver.h"
#include "mess/machine/nes.h"
#include "mess/machine/nes_mmc.h"

#define VERBOSE_ERRORS

/* Global variables */
int Mapper;
int uses_irqs;
int MMC_Table;
int MMC2_bank0, MMC2_bank1, MMC2_hibank1_val, MMC2_hibank2_val;
int MMC3_DOIRQ, MMC3_IRQ;
int MMC1_extended;	/* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */
void (*mmc_write)(int offset, int data);


/* Local variables */
//static int MMC_chr;
static int MMC1_Size_16k;
static int MMC1_High;
static int MMC1_reg;
static int MMC1_reg_count;
static int MMC1_Switch_Low, MMC1_SizeVrom_4k;
static int MMC1_bank1, MMC1_bank2, MMC1_bank3, MMC1_bank4;
static int MMC1_extended_bank;
static int MMC1_extended_base;
static int MMC1_extended_swap;

static int MMC5_rom_bank_mode;
static int MMC5_vrom_bank_mode;
static int MMC5_exram_tile_enable;
static int MMC5_exram_color_enable;
static int MMC5_scanline;

static int mapper_warning;

void nes_mapper_w (int offset, int data)
{
	(*mmc_write)(offset, data);
}

/* Handle unusual mapper reads from $4100-$5fff */
int nes_strange_mapper_r (int offset)
{
	if (errorlog) fprintf (errorlog, "low mapper area read, addr: %04x\n", offset + 0x4100);
	switch (Mapper)
	{
		case 5:
			switch (offset)
			{
				/* $5204 - irq scanline hit? */
				case 0x1104:
					if (current_scanline == MMC5_scanline)
						return 0x80;
					else return (rand() & 0xff);
			}
			break;
	}
	return 0;
}

/* Handle unusual mapper writes to $4100-$5fff */
void nes_strange_mapper_w (int offset, int data)
{
	switch (Mapper)
	{
		case 5:
			/* The bulk of this information comes via D and Jim Geffre */
			if (errorlog) fprintf (errorlog, "Mapper 5 write, offset: %04x, data: %02x\n", offset + 0x4100, data);
			switch (offset)
			{
				/* $5010 - sound IRQ control? uncharted, cv3 set to 0 */
				/* $5011 - sound sample register? */
				/* $5015 - ?? uncharted sets to 0 */
				
				/* $5101 - vrom banking? - uncharted sets these 3 to 3, 2, 1 respectively */
				/* $5102 - vrom banking? */
				/* $5104 - exram control? uncharted sets to 1, cv3 sets to 0 */
				/* $5105 - Mirroring? uncharted sets to 0, cv3 to 44 */
				/* $5106 - ?? cv3 sets to 0 */
				/* $5107 - ?? cv3 sets to 0 */
				/* $5100 - ROM bank mode, 0:32k, 1:16k, 2,3:8k */
				case 0x1000:
					MMC5_rom_bank_mode = data & 0x03;
					break;
				case 0x1003:
					/*
					There are 4 CHRROM modes:

					0. CHRROM is a single 8k bank. The 8k page number is written to $5127.
					Maxium CHRROM addressable this way is 1 megabyte (the most, according to
					Nintendo Power, that this chip can control).

					1. CHRROM is divided into two 4k banks. The 4k page number for the lower
					bank is written to $5123; the upper bank is controlled by $5127. Maximum
					CHRROM addressable this way is 1 megabyte.

					2. CHRROM is divided into four 2k banks. The 2k page number is written to
					$5121, $5123, $5125, or $5127 depending on which bank is to be switched.
					Maximum CHRROM addressable this way is 512k.

					3. CHRROM is divided into eight 1k banks. The 1k page number is written to
					$5120-$5127 depending on which bank is to be switched. Maximum CHRROM
					addressable this way is 256k.
					*/
				
					MMC5_vrom_bank_mode = data & 0x03;
					break;
				/* $5104 - EXRAM control */
				case 0x1004:
					MMC5_exram_tile_enable = data & 0x01;
					MMC5_exram_color_enable = data & 0x02;
					break;
					
				case 0x1005:
					/*
					bits 0-1	VRAM page select for $2000-23FF
					bits 2-3	VRAM page select for $2400-27FF
					bits 4-5	VRAM page select for $2800-2BFF
					bits 6-7	VRAM page select for $2C00-2FFF
					*/
					Mirroring = (data & 0x01) + 1;
					break;
				
				/* cv3 sets bit 0x80 in the banks, uncharted sets 0xc0 */
				/* $5113 - ?? bank switch control? uncharted sets to 5, cv3 unused */
				/* $5114 - ?? bank switch 1 8k bank at $8000? cv3 unused, uncharted uses */
				/* $5115 - Select 8k ROM bank at $8000 */
				case 0x1014:
					data &= ((PRG_Rom << 1) - 1);
					cpu_setbank (1, &ROM[data * 0x2000 + 0x10000]);
					break;
				/* $5115 - Select 16k (8k also?) ROM bank at $a000 */
				case 0x1015:
					if (MMC5_rom_bank_mode == 0x02)
					{
						data &= ((PRG_Rom << 1) - 1);
						cpu_setbank (1, &ROM[data * 0x2000 + 0x10000]);
						cpu_setbank (2, &ROM[data * 0x2000 + 0x12000]);
					}
					else
					{
						data &= ((PRG_Rom << 1) - 1);
						cpu_setbank (2, &ROM[data * 0x2000 + 0x10000]);
					}
					break;
				/* $5116 - Select 8k ROM bank at $c000 */
				case 0x1016:
					data &= ((PRG_Rom << 1) - 1);
					cpu_setbank (3, &ROM[data * 0x2000 + 0x10000]);
					break;
				/* $5117 - Select 8k ROM bank at $e000 */
				case 0x1017:
					if (MMC5_rom_bank_mode == 0x00)
					{
						/* 32k switch */
						data &= ((PRG_Rom << 1) - 1);
						cpu_setbank (1, &ROM[data * 0x2000 + 0x10000]);
						cpu_setbank (2, &ROM[data * 0x2000 + 0x12000]);
						cpu_setbank (3, &ROM[data * 0x2000 + 0x14000]);
						cpu_setbank (4, &ROM[data * 0x2000 + 0x16000]);
					}
					else if (MMC5_rom_bank_mode == 0x02)
					{
						/* 16k switch */
						data &= ((PRG_Rom << 1) - 1);
						cpu_setbank (3, &ROM[data * 0x2000 + 0x10000]);
						cpu_setbank (4, &ROM[data * 0x2000 + 0x12000]);
					}
					else
					{
						/* 8k switch */
						data &= ((PRG_Rom << 1) - 1);
						cpu_setbank (4, &ROM[data * 0x2000 + 0x10000]);
					}
					break;
				
				case 0x1103:
					MMC3_IRQ = data;
					MMC5_scanline = data;
					break;
				case 0x1104:
					MMC3_DOIRQ = data & 0x80;
					break;
				/* $5200 - ?? uncharted, cv3 sets to 0 */
				default:
					break;
			}
			break;
		case 17:
			switch (offset)
			{
				/* $42ff - mirroring */
				case 0x1ff:
					if (data & 0x10)
						Mirroring = 2; // Vertical mirroring
					else
						Mirroring = 1;
					break;
				/* $4501 - $4503 */
				case 0x401:
				case 0x402:
				case 0x403:
					/* IRQ control */
					break;
				/* $4504 - $4507 : 8k PRG-Rom switch */
				case 0x404:
				case 0x405:
				case 0x406:
				case 0x407:
					data &= ((PRG_Rom << 1) - 1);
					if (errorlog) fprintf (errorlog, "Mapper 17 bank switch, bank: %02x, data: %02x\n", offset & 0x03, data);
					cpu_setbank ((offset & 0x03) + 1, &ROM[0x2000 * (data) + 0x10000]);
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
					data &= ((CHR_Rom << 3) - 1);
					nes_vram[offset & 0x07] = data * 64;
					break;
				default:
					if (errorlog) fprintf (errorlog, "Mapper 17 write, offset: %04x, data: %02x\n", offset, data);
					break;
			}
			break;
		default:
			if (errorlog) fprintf (errorlog, "Unimplemented write, offset: %04x, data: %02x\n", offset + 0x4100, data);
			break;
	}
}
/*
 * For games w/o a mapper & unimplemented mappers 
 */
static void NoneWR (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "Unimplemented mapper write, offset: %04x, data: %02x\n", offset, data);
#if 1
	if (! mapper_warning)
	{
		printf ("This game is writing to the mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
		mapper_warning = 1;
	}

	switch (offset)
	{
		/* Hacked mapper for the 24-in-1 ROM. */
		/* It's really 35-in-1, and it's mostly different versions of Battle City. Unfortunately, the CHR_Rom dumps are bad */
		case 0x7fde:
			data &= (PRG_Rom - 1);
			cpu_setbank (3, &ROM[data * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[data * 0x4000 + 0x12000]);
			break;
	}
#endif
}

/* 
 * Rom switch (74HC161/74HC32) - mapper 2
 */
static void RomSw_Write (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "* Mapper 2 switch %02x\n", data);
	data &= (PRG_Rom - 1);
	cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
}

/* 
 * FFE F3xxxx - mapper 8
 */
static void Mapper8_Write (int offset, int data)
{
	int bank;
	int i;
	
	if (errorlog) fprintf (errorlog, "* Mapper 8 switch, vrom: %02x, rom: %02x\n", data & 0x07, (data >> 3));
	/* Switch 8k VROM bank */
	bank = data & (CHR_Rom - 1);
	for (i = 0; i < 8; i ++)
		nes_vram[i] = bank * 512 + 64*i;
	/* Switch 16k ROM bank */
	data = (data >> 3) & (PRG_Rom - 1);
	cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
}

/* 
 * Color dreams - mapper 11
 */
static void CDrms_Write (int offset, int data)
{
	int bank;
	int i;
	
	if (errorlog) fprintf (errorlog, "* Mapper 11 switch, data: %02x\n", data);

	/* Switch 8k VROM bank */
	bank = (data >> 4) & (CHR_Rom - 1);
	for (i = 0; i < 8; i ++)
		nes_vram[i] = bank * 512 + 64*i;
	/* Switch 32k ROM bank */
	data &= ((PRG_Rom >> 1) - 1);
	cpu_setbank (1, &ROM[data * 0x8000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x8000 + 0x12000]);
	cpu_setbank (3, &ROM[data * 0x8000 + 0x14000]);
	cpu_setbank (4, &ROM[data * 0x8000 + 0x16000]);

}

/*
 * Mapper 1 - MMC1
 */

static void MMC1_Write (int offset, int data)
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
		if (errorlog) fprintf (errorlog, "=== MMC1 regs reset to default\n");
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
//		if (errorlog) fprintf (errorlog, "   MMC1 reg#%02x val:%02x\n", offset, MMC1_reg);
		switch (reg)
		{
			case 0:
/*
 ³ $8000 Ä $9FFF ÃÄ´ RxxCFHPM                                               ³
 ³ (Register 0)  ³ ³ ³  ³³³³³                                               ³
 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ³ ³  ³³³³ÀÄÄÄ Mirroring Flag                             ³
                   ³ ³  ³³³³      0 = Horizontal                            ³
                   ³ ³  ³³³³      1 = Vertical                              ³
                   ³ ³  ³³³³                                                ³
                   ³ ³  ³³³ÀÄÄÄÄ OneÄScreen Mirroring                       ³
                   ³ ³  ³³³       0 = All pages mirrored from PPU $2000     ³
                   ³ ³  ³³³       1 = Regular mirroring                     ³
                   ³ ³  ³³³                                                 ³
                   ³ ³  ³³ÀÄÄÄÄÄ PRG Switching Area                         ³
                   ³ ³  ³³        0 = Swap ROM bank at $C000                ³
                   ³ ³  ³³        1 = Swap ROM bank at $8000                ³
                   ³ ³  ³³                                                  ³
                   ³ ³  ³ÀÄÄÄÄÄÄ PRG Switching Size                         ³
                   ³ ³  ³         0 = Swap 32K of ROM at $8000              ³
                   ³ ³  ³         1 = Swap 16K of ROM based on bit 2        ³
                   ³ ³  ³                                                   ³
                   ³ ³  ÀÄÄÄÄÄÄÄ <Carts with VROM>                          ³
                   ³ ³           VROM Switching Size                        ³
                   ³ ³            0 = Swap 8K of VROM at PPU $0000          ³
                   ³ ³            1 = Swap 4K of VROM at PPU $0000 and $1000³
                   ³ ³           <1024K carts>                              ³
                   ³ ³            0 = Ignore 256K selection register 0      ³
                   ³ ³            1 = Acknowledge 256K selection register 1 ³
*/
				MMC1_Switch_Low = MMC1_reg & 0x04;
				MMC1_Size_16k =   MMC1_reg & 0x08;

				Mirroring =      !(MMC1_reg & 0x01) + 1; /* I believe the docs above are wrong on this */
				PPU_one_screen = (!(MMC1_reg & 0x02)) << 13;
				MMC1_SizeVrom_4k = (MMC1_reg & 0x10);
#ifdef VERBOSE_ERRORS
				if (errorlog) fprintf (errorlog, "   MMC1 reg #1 val:%02x\n", MMC1_reg);
				if (errorlog)
				{
					fprintf (errorlog, "\t\tBank Size: ");
					if (MMC1_Size_16k)
						fprintf (errorlog, "16k\n");
					else fprintf (errorlog, "32k\n");

					fprintf (errorlog, "\t\tBank Select: ");
					if (MMC1_Switch_Low)
						fprintf (errorlog, "$8000\n");
					else fprintf (errorlog, "$C000\n");

					fprintf (errorlog, "\t\tVROM Bankswitch Size Select: ");
					if (MMC1_SizeVrom_4k)
						fprintf (errorlog, "4k\n");
					else fprintf (errorlog, "8k\n");
					
					fprintf (errorlog, "\t\tMirroring: ");
					if (Mirroring == 2)
						fprintf (errorlog, "Vertical\n");
					else fprintf (errorlog, "Horizontal\n");
					
					fprintf (errorlog, "\t\tOne Screen: ");
					if (PPU_one_screen)
						fprintf (errorlog, "On\n");
					else fprintf (errorlog, "Off\n");
				}
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
						cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
						cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
						if (errorlog) fprintf (errorlog, "MMC1_extended 1024k bank (no reg) select: %02x\n", MMC1_extended_bank);
					}
					else
					{
						/* Set 256k bank based on the 256k bank select register */
						if (MMC1_extended_swap)
						{
							MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
							cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
							cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
							cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
							if (errorlog) fprintf (errorlog, "MMC1_extended 1024k bank (reg 1) select: %02x\n", MMC1_extended_bank);
							MMC1_extended_swap = 0;
						}
						else MMC1_extended_swap = 1;
					}
				}
				else if (MMC1_extended == 1 && CHR_Rom == 0)
				{
					/* Pick 1st or 2nd 256k bank */
					MMC1_extended_base = 0x40000 * (MMC1_extended_bank & 0x01) + 0x10000;
					cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
					cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
					cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
					cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
					if (errorlog) fprintf (errorlog, "MMC1_extended 512k bank select: %02x\n", MMC1_extended_bank & 0x01);
				}
				else if (CHR_Rom > 0)
				{
//					if (errorlog) fprintf (errorlog, "MMC1_SizeVrom_4k: %02x bank:%02x\n", MMC1_SizeVrom_4k, MMC1_reg);

					if (!MMC1_SizeVrom_4k)
					{
						int bank = MMC1_reg & ((CHR_Rom << 1) - 1);
						
						for (i = 0; i < 8; i ++)
							nes_vram[i] = bank * 256 + 64*i;
#ifdef VERBOSE_ERRORS
						if (errorlog) fprintf (errorlog, "MMC1 8k VROM switch: %02x\n", MMC1_reg);
						#ifdef macintosh
//						SysBeep (0);
						#endif
#endif
					}
					else
					{
						int bank = MMC1_reg & ((CHR_Rom << 1) - 1);
						
						for (i = 0; i < 4; i ++)
							nes_vram[i] = bank * 256 + 64*i;
#ifdef VERBOSE_ERRORS
						if (errorlog) fprintf (errorlog, "MMC1 4k VROM switch (low): %02x\n", MMC1_reg);
#endif
					}
				}
				break;
			case 2:
//Ò   2   Ò $C000 Ò ssssssss Ò CHR-RAM 4K Page Selection Register         Ò
//Ò       Ò  ...  Ò          Ò                                            Ò
//Ò       Ò $DFFF Ò          Ò  Sets the 4K CHR-RAM page at $1000, but    Ò
//Ò       Ò       Ò          Ò  only if 4K CHR-RAM pages are selected via Ò
//Ò       Ò       Ò          Ò  Register #0 (otherwise ignored).          Ò
//				if (errorlog) fprintf (errorlog, "MMC1_Reg_2: %02x\n",MMC1_Reg_2);
				MMC1_extended_bank = (MMC1_extended_bank & ~0x02) | ((MMC1_reg & 0x10) >> 3);
				if (MMC1_extended == 2 && MMC1_SizeVrom_4k)
				{
					if (MMC1_extended_swap)
					{
						/* Set 256k bank based on the 256k bank select register */
						MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
						cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
						cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
						if (errorlog) fprintf (errorlog, "MMC1_extended 1024k bank (reg 2) select: %02x\n", MMC1_extended_bank);
						MMC1_extended_swap = 0;
					}
					else
						MMC1_extended_swap = 1;
				}
				if (MMC1_SizeVrom_4k)
				{
					int bank = MMC1_reg & ((CHR_Rom << 1) - 1);
						
					for (i = 4; i < 8; i ++)
						nes_vram[i] = bank * 256 + 64*(i-4);
#ifdef VERBOSE_ERRORS
						if (errorlog) fprintf (errorlog, "MMC1 4k VROM switch (high): %02x\n", MMC1_reg);
#endif
				}
				break;
			case 3:
/*
Ò   3   Ò $E000 Ò ssssssss Ò PRG-ROM 16K Page Selection Register        Ò
Ò       Ò  ...  Ò          Ò                                            Ò
Ò       Ò $FFFF Ò          Ò  Sets the 16K ROM page at $8000. The page  Ò
Ò       Ò       Ò          Ò  at $C000 is hardwired to the last ROM     Ò
Ò       Ò       Ò          Ò  page in the cart. Page 0 starts at $8000. Ò
*/

				/* Switching 1 32k bank of PRG-ROM */
				MMC1_reg &= 0x0f;
				if (!MMC1_Size_16k)
				{
					int bank = MMC1_reg & (PRG_Rom - 1);

					MMC1_bank1 = bank * 0x4000;
					MMC1_bank2 = bank * 0x4000 + 0x2000;
					cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
					cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
					if (!MMC1_extended)
					{
						MMC1_bank3 = bank * 0x4000 + 0x4000;
						MMC1_bank4 = bank * 0x4000 + 0x6000;
						cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
						cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
					}
					if (errorlog) fprintf (errorlog, "MMC1 32k bank select: %02x\n", MMC1_reg);
				}
				else
				/* Switching one 16k bank */
				{
					if (MMC1_Switch_Low)
					{
						int bank = MMC1_reg & (PRG_Rom - 1);

						MMC1_bank1 = bank * 0x4000;
						MMC1_bank2 = bank * 0x4000 + 0x2000;
						
						cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
						cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
						if (!MMC1_extended)
						{
							MMC1_bank3 = MMC1_High;
							MMC1_bank4 = MMC1_High + 0x2000;
							cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
						}
						if (errorlog) fprintf (errorlog, "MMC1 16k-low bank select: %02x\n", MMC1_reg);
					}
					else
					{
						int bank = MMC1_reg & (PRG_Rom - 1);

						if (!MMC1_extended)
						{

							MMC1_bank1 = 0;
							MMC1_bank2 = 0x2000;
							MMC1_bank3 = bank * 0x4000;
							MMC1_bank4 = bank * 0x4000 + 0x2000;

							cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
							cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
							cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
							cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
						}
						if (errorlog) fprintf (errorlog, "MMC1 16k-high bank select: %02x\n", MMC1_reg);
					}
				}
			if (errorlog)
			{
				fprintf (errorlog, "-- page1: %06x\n", MMC1_bank1);
				fprintf (errorlog, "-- page2: %06x\n", MMC1_bank2);
				fprintf (errorlog, "-- page3: %06x\n", MMC1_bank3);
				fprintf (errorlog, "-- page4: %06x\n", MMC1_bank4);
			}
				break;
		}
	MMC1_reg_count = 0;
	}
}

/*
	Mapper 3 - VROM Switch
*/

static void VRomSw_Write (int offset, int data)
{
	int i;
	
	if (errorlog) fprintf (errorlog, "Mapper 3 vrom switch, %04x:%02x\n", offset, data);
	for (i = 0; i < 8; i ++)
		nes_vram[i] = (data & (CHR_Rom -1)) * 512 + 64*i;
}

static void Mapper4_Write (int offset, int data)
{
	static int cmd = 0;
	static int chr = 0;
	static int irq;
	static int select_high;
	static int page;
						
//	if (errorlog) fprintf (errorlog, "mapper4_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);
	
	switch (offset & 0x7001)
	{
		case 0x0000:
//			if (errorlog) fprintf (errorlog, "MMC3 0x8000 write value: %02x\n",data);
			cmd = data & 0x07;
			if (data & 0x80)
				chr = 0x1000;
			else
				chr = 0x0000;
			
			page = chr >> 10;
			/* Toggle between switching $8000 and $c000 */
			if (select_high != (data & 0x40))
			{
				/* Reset the banks */
				#ifdef macintosh
				SysBeep (0);
				#endif
				if (errorlog) fprintf (errorlog, "HACK ALERT -- MMC3 select_high has changed!\n");
				/* TODO: this makes PowerPunch 2 work, but is it correct? */
				if (data & 0x40)
				{
					cpu_setbank (1, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
				}
				else
				{
					cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
				}
			}
			select_high = data & 0x40;
			if (errorlog) fprintf (errorlog, "   MMC3 select_high: %02x\n", select_high);
			break;

		case 0x0001:
			switch (cmd)
			{
				case 0:
					data &= 0xFE;
					nes_vram [page] = data * 64;
					nes_vram [page+1] = nes_vram [page] + 64;
					break;
								
				case 1:
					data &= 0xFE;
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
					/* i.e. the Simpsons series, the Batman games, Gauntlet 2, etc. */
					data &= ((PRG_Rom << 1) - 1);
					if (select_high)
					{
						cpu_setbank (3, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     MMC3 switch ($c000) cmd 6 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     MMC3 switch ($8000) cmd 6 value: %02x\n", data);
					}
					break;
				case 7:
					/* These damn games will go to great lengths to switch to banks which are outside the valid range */
					/* i.e. the Simpsons series, the Batman games, Gauntlet 2, etc. */
					data &= ((PRG_Rom << 1) - 1);
					cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
					if (errorlog) fprintf (errorlog, "     MMC3 switch ($a000) cmd 7 value: %02x select_high: %02x\n", data, select_high);
					break;
			}
			cmd = 8;
			break;
		case 0x2000:
			if (errorlog) fprintf (errorlog, "     MMC3 mirroring: %02x\n", data);
			Mirroring = 0x02 >> (data & 0x01);
			break;

		case 0x4000: /* IRQ scanline counter */
			MMC3_IRQ = data;
			if (errorlog) fprintf (errorlog, "     MMC3 copy/set irq latch: %02x\n", data);
			break;

		case 0x4001: /* IRQ latch value */
			irq = data;
			if (errorlog) fprintf (errorlog, "     MMC3 set latch: %02x\n", data);
			break;
                
		case 0x6000: /* Copy latch & disable IRQs */
			if (errorlog) fprintf (errorlog, "     MMC3 copy latch (disable irqs): %02x\n", data);
			MMC3_IRQ = irq;
			MMC3_DOIRQ = 0;
			break;
						
		case 0x6001: /* Enable IRQs */
			MMC3_DOIRQ = 1;
			if (errorlog) fprintf (errorlog, "     MMC3 enable irqs: %02x\n", data);
			break;

		default:
			if (errorlog) fprintf (errorlog, "MMC3 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

void Mapper5_Write (int offset, int data)
{
//	if (errorlog) fprintf (errorlog, "MMC5 W %04x: %02x\n", addr, data);
}

void Mapper7_Write (int offset, int data)
{
	/* Note - mirroring is different - one table is mirrored 3 times */
	if (data & 0x10)
		PPU_one_screen = 0x2400;
	else
		PPU_one_screen = 0x2000;
	
	/* Castle of Deceit tries to deceive us by switching to invalid banks */
	data &= ((PRG_Rom >> 1) - 1);
	cpu_setbank (1, &ROM[data * 0x8000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x8000 + 0x12000]);
	cpu_setbank (3, &ROM[data * 0x8000 + 0x14000]);
	cpu_setbank (4, &ROM[data * 0x8000 + 0x16000]);
}

/*
	Mapper 9 - MMC2
*/
static void MMC2_Write (int offset, int data)
{
/*
  PRG-ROM banks are 16K in size, and CHR-RAM banks are 4K.

ÚÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³ Address ³ Stats ³ Bits     ³ Description                                 ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $A000   ³  W    ³ pppppppp ³ PRG-ROM Page Select Register                ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    p = 32K PRG-ROM Select                   ³
³         ³       ³          ³         Selects 16K PRG-ROM bank at $8000.  ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $B000   ³  W    ³ CCCCCCCC ³ CHR-RAM Page Select Register #1             ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    C = 4K CHR-RAM Select                    ³
³         ³       ³          ³         Selects 4K CHR-RAM bank at videoram     ³
³         ³       ³          ³         address $0000.                      ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $C000   ³  W    ³ CCCCCCCC ³ CHR-RAM Page Select Register #2             ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    C = 4K CHR-RAM Select                    ³
³         ³       ³          ³         Selects 4K CHR-RAM bank at videoram     ³
³         ³       ³          ³         address $1000.                      ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $D000   ³  W    ³ CCCCCCCC ³ CHR-RAM Page Select Register #3             ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    C = 4K CHR-RAM Select                    ³
³         ³       ³          ³         Selects 4K CHR-RAM bank at videoram     ³
³         ³       ³          ³         address $0000.                      ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $E000   ³  W    ³ CCCCCCCC ³ CHR-RAM Page Select Register #4             ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    C = 4K CHR-RAM Select                    ³
³         ³       ³          ³         Selects 4K CHR-RAM bank at videoram     ³
³         ³       ³          ³         address $1000.                      ³
ÃÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $F000   ³  W    ³ dddddddd ³ Mirroring Select Register                   ³
³         ³       ³          ³                                             ³
³         ³       ³          ³    d = Mirroring Select                     ³
³         ³       ³          ³         0 = Horizontal                      ³
³         ³       ³          ³         1 = Vertical                        ³
ÀÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

  This mapper has some very peculiar aspects, especially when it comes to
accessing videoram. The CHR-RAM Page Select Registers listed above are auto-
matically "enabled" and "disabled" depending upon which address(es) in
videoram you access.

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³ videoram Address Range ³ Description                                 ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³ $0FD8-0FDF         ³ Switches videoram $0000-0FFF swapping to $C000  ³
³ $0FE8-0FEF         ³ Switches videoram $0000-0FFF swapping to $B000  ³
³ $1FD8-1FDF         ³ Switches videoram $1000-1FFF swapping to $E000  ³
³ $1FE8-1FEF         ³ Switches videoram $1000-1FFF swapping to $D000  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/
	int i;
	
	switch (offset & 0x7000)
	{
		case 0x2000:
			cpu_setbank (1, &ROM[data * 0x2000 + 0x10000]);
			break;
		case 0x3000:
		case 0x4000:
			{
				data = data & ((CHR_Rom << 1) - 1);
				for (i = 0; i < 4; i ++)
					nes_vram[i] = data * 256 + 64*i;
			}
			if (errorlog) fprintf (errorlog, "MMC2 VROM switch #1 (low): %02x\n", data);
			break;
		case 0x5000:
			if (MMC2_bank1 == 0xfd)
			{
				data = data & ((CHR_Rom << 1) - 1);
				for (i = 4; i < 8; i ++)
					nes_vram[i] = data * 256 + 64*(i-4);
				MMC2_hibank1_val = data;
			}
			else if (errorlog) fprintf (errorlog, "IGNORED -- ");
			if (errorlog) fprintf (errorlog, "MMC2 VROM switch #1 (high): %02x\n", data);
			break;
		case 0x6000:
			if (MMC2_bank1 == 0xfe)
			{
				data = data & ((CHR_Rom << 1) - 1);
				for (i = 4; i < 8; i ++)
					nes_vram[i] = data * 256 + 64*(i-4);
				MMC2_hibank2_val = data;
			}
			else if (errorlog) fprintf (errorlog, "IGNORED -- ");
			if (errorlog) fprintf (errorlog, "MMC2 VROM switch #2 (high): %02x\n", data);
			break;
		/* TODO: Punch-Out seems to need CHR-ROM bank 0 switched in high at the top of each frame */
		case 0x7000:
			if (data)
				Mirroring = 2; // Vertical Mirroring
			else
				Mirroring = 1;
			break;
	}
	if (errorlog) fprintf (errorlog, "MMC2 w: %04x:%02x\n", offset, data);
}

/*
	Mapper 10 - MMC4
*/
void MMC4_Write (int offset, int data)
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 8k ROM bank */
			cpu_setbank (1, &ROM[data * 0x2000 + 0x10000]);
			break;
			
		case 0x3000:
		case 0x4000:
//			memcpy (&videoram [0] , &VROM [data * 0x1000], 0x1000);
//			dirty_char = 1;
			break;
			
		case 0x5000:
		case 0x6000:
//			memcpy (&videoram [0x1000], &VROM [data * 0x1000], 0x1000 );
//			dirty_char = 1;
			break;
			
		case 0x7000:
			if (data)
				Mirroring = 2;
			else
				Mirroring = 1;
			break;
	}
}

/*
	Mapper 15 - 100-in-1 ROM
*/
static void Mapper15_Write (int offset, int data)
{
	int bank = (data & (PRG_Rom - 1)) * 0x4000;
	int base = data & 0x80 ? 0x12000:0x10000;
	
	switch (offset)
	{
		case 0x0000:
			Mirroring = (!(data & 0x40)) + 1;
			cpu_setbank (1, &ROM[bank + base]);
			cpu_setbank (2, &ROM[bank + (base ^ 0x2000)]);
			cpu_setbank (3, &ROM[bank + base + 0x4000]);
			cpu_setbank (4, &ROM[bank + (base ^ 0x2000) + 0x4000]);
			break;

		case 0x0001:
			cpu_setbank (3, &ROM[bank + base]);
			cpu_setbank (4, &ROM[bank + (base ^ 0x2000)]);
			break;

		case 0x0002:
			cpu_setbank (1, &ROM[bank + base]);
			cpu_setbank (2, &ROM[bank + base]);
			cpu_setbank (3, &ROM[bank + base]);
			cpu_setbank (4, &ROM[bank + base]);
        	break;

		case 0x0003:
			Mirroring = (!(data & 0x40)) + 1;
			cpu_setbank (3, &ROM[bank + base]);
			cpu_setbank (4, &ROM[bank + (base ^ 0x2000)]);
        	break;
	}
}

static void Bandai_Write (int offset, int data)
{
	int i;
	
	cpu_setbank (1, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x10000]);
	cpu_setbank (2, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x12000]);

	for (i = 0; i < 8; i ++)
		nes_vram[i] = (data & 0x03) * 512 + 64*i;
}

static void Mapper18_Write (int offset, int data)
{
//	static int irq;
	static int bank_8000 = 0;
	static int bank_a000 = 0;
	static int bank_c000 = 0;
	static int vrom_bank[8];

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 - low 4 bits */
			bank_8000 = (bank_8000 & 0xf0) | (data & 0x0f);
			bank_8000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (1, &ROM[0x2000 * (bank_8000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($8000 low 4) value: %02x\n", data);
			break;
		case 0x0001:
			/* Switch 8k bank at $8000 - high 4 bits */
			bank_8000 = (bank_8000 & 0x0f) | (data << 4);
			bank_8000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (1, &ROM[0x2000 * (bank_8000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($8000 high 4) value: %02x\n", data);
			break;
		case 0x0002:
			/* Switch 8k bank at $a000 - low 4 bits */
			bank_a000 = (bank_a000 & 0xf0) | (data & 0x0f);
			bank_a000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (2, &ROM[0x2000 * (bank_a000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($a000 low 4) value: %02x\n", data);
			break;
		case 0x0003:
			/* Switch 8k bank at $a000 - high 4 bits */
			bank_a000 = (bank_a000 & 0x0f) | (data << 4);
			bank_a000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (2, &ROM[0x2000 * (bank_a000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($a000 high 4) value: %02x\n", data);
			break;
		case 0x1000:
			/* Switch 8k bank at $c000 - low 4 bits */
			bank_c000 = (bank_c000 & 0xf0) | (data & 0x0f);
			bank_c000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (3, &ROM[0x2000 * (bank_c000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($c000 low 4) value: %02x, new bank: %02x\n", data, bank_c000);
			break;
		case 0x1001:
			/* Switch 8k bank at $c000 - high 4 bits */
			bank_c000 = (bank_c000 & 0x0f) | (data << 4);
			bank_c000 &= ((PRG_Rom << 1) - 1);
			cpu_setbank (3, &ROM[0x2000 * (bank_c000) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 18 switch ($c000 high 4) value: %02x, new bank: %02x\n", data, bank_c000);
			break;
		case 0x2000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((CHR_Rom << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0000 low 4) value: %02x\n", data);
			break;
		case 0x2001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((CHR_Rom << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0000 high 4) value: %02x\n", data);
			break;
		case 0x2002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((CHR_Rom << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0400 low 4) value: %02x\n", data);
			break;
		case 0x2003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((CHR_Rom << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0400 high 4) value: %02x\n", data);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((CHR_Rom << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0800 low 4) value: %02x\n", data);
			break;
		case 0x3001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((CHR_Rom << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0800 high 4) value: %02x\n", data);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((CHR_Rom << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0c00 low 4) value: %02x\n", data);
			break;
		case 0x3003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((CHR_Rom << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($0c00 high 4) value: %02x\n", data);
			break;
		case 0x4000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((CHR_Rom << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1000 low 4) value: %02x\n", data);
			break;
		case 0x4001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((CHR_Rom << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1000 high 4) value: %02x\n", data);
			break;
		case 0x4002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((CHR_Rom << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1400 low 4) value: %02x\n", data);
			break;
		case 0x4003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((CHR_Rom << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1400 high 4) value: %02x\n", data);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((CHR_Rom << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1800 low 4) value: %02x\n", data);
			break;
		case 0x5001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((CHR_Rom << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1800 high 4) value: %02x new bank: %02x\n", data, vrom_bank[6]);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((CHR_Rom << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1c00 low 4) value: %02x\n", data);
			break;
		case 0x5003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((CHR_Rom << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 18 vrom ($1c00 high 4) value: %02x new bank: %02x\n", data, vrom_bank[7]);
			break;

		case 0x6000: /* IRQ scanline counter - low byte */
			MMC3_IRQ = (MMC3_IRQ & 0xff00) | data;
			if (errorlog) fprintf (errorlog, "     Mapper 18 copy/set irq latch (low): %02x\n", data);
			break;
		case 0x6001: /* IRQ scanline counter - high byte */
			MMC3_IRQ = (MMC3_IRQ & 0x00ff) | (data << 8);
			if (errorlog) fprintf (errorlog, "     Mapper 18 copy/set irq latch (high): %02x\n", data);
			break;

		case 0x7000: /* IRQ Control 0 */
			if (data & 0x01) MMC3_DOIRQ = 1;
			if (errorlog) fprintf (errorlog, "     Mapper 18 IRQ Control 0: %02x\n", data);
			break;
		case 0x7001: /* IRQ Control 1 */
			MMC3_DOIRQ = data & 0x01;
			if (errorlog) fprintf (errorlog, "     Mapper 18 IRQ Control 1: %02x\n", data);
			break;
						
		case 0x7002: /* Misc */
			Mirroring = (!(data & 0x01)) + 1;
			PPU_one_screen = (data & 0x02) << 12;
			if (errorlog) fprintf (errorlog, "     Mapper 18 IRQ Control 1: %02x\n", data);
			break;
						

		default:
			if (errorlog) fprintf (errorlog, "Mapper 18 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void Mapper25_Write (int offset, int data)
{
	static int vrom_bank[8];

	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 25 switch ($8000) value: %02x\n", data);
			break;

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
//			if (errorlog) fprintf (errorlog, "     Mapper 25 switch ($a000) value: %02x\n", data);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((CHR_Rom << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0000 low 4) value: %02x\n", data);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((CHR_Rom << 3) - 1);
			nes_vram[0] = vrom_bank[0] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0000 high 4) value: %02x\n", data);
			break;
		case 0x3001:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((CHR_Rom << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0400 low 4) value: %02x\n", data);
			break;
		case 0x3003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((CHR_Rom << 3) - 1);
			nes_vram[1] = vrom_bank[1] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0400 high 4) value: %02x\n", data);
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((CHR_Rom << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0800 low 4) value: %02x\n", data);
			break;
		case 0x4002:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((CHR_Rom << 3) - 1);
			nes_vram[2] = vrom_bank[2] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0800 high 4) value: %02x\n", data);
			break;
		case 0x4001:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((CHR_Rom << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0c00 low 4) value: %02x\n", data);
			break;
		case 0x4003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((CHR_Rom << 3) - 1);
			nes_vram[3] = vrom_bank[3] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($0c00 high 4) value: %02x\n", data);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((CHR_Rom << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1000 low 4) value: %02x\n", data);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((CHR_Rom << 3) - 1);
			nes_vram[4] = vrom_bank[4] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1000 high 4) value: %02x\n", data);
			break;
		case 0x5001:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((CHR_Rom << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1400 low 4) value: %02x\n", data);
			break;
		case 0x5003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((CHR_Rom << 3) - 1);
			nes_vram[5] = vrom_bank[5] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1400 high 4) value: %02x\n", data);
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((CHR_Rom << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1800 low 4) value: %02x\n", data);
			break;
		case 0x6002:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((CHR_Rom << 3) - 1);
			nes_vram[6] = vrom_bank[6] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1800 high 4) value: %02x\n", data);
			break;
		case 0x6001:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((CHR_Rom << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1c00 low 4) value: %02x\n", data);
			break;
		case 0x6003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((CHR_Rom << 3) - 1);
			nes_vram[7] = vrom_bank[7] * 64;
//			if (errorlog) fprintf (errorlog, "     Mapper 25 vrom ($1c00 high 4) value: %02x\n", data);
			break;

		default:
			if (errorlog) fprintf (errorlog, "Mapper 25 offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static void Mapper26_Write (int offset, int data)
{

//	if (errorlog) fprintf (errorlog, "mapper26_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);
	
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 16k bank at $8000 */
			data &= (PRG_Rom - 1);
			cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
			cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
			break;

		case 0x3003:
			if (data & 0x0c)
			{
				data &= 0x0c;
				Mirroring = 1;
				PPU_one_screen = 0;
				if (data == 0x08) PPU_one_screen = 0x2000;
				else if (data == 0x0c) PPU_one_screen = 0x2400;
				else if (data) Mirroring = 2;
			}
			break;
		case 0x1000:
			/* Switch 2k VROM at $0000 */
			data &= 0xFE;
			nes_vram [0] = data * 64;
			nes_vram [1] = nes_vram [0] + 64;
			break;
		case 0x2000:
			/* Switch 2k VROM at $0800 */
			data &= 0xFE;
			nes_vram [2] = data * 64;
			nes_vram [3] = nes_vram [2] + 64;
			break;
		case 0x3000:
			/* Switch 2k VROM at $1000 */
			data &= 0xFE;
			nes_vram [4] = data * 64;
			nes_vram [5] = nes_vram [4] + 64;
			break;
		case 0x4000:
			/* Switch 2k VROM at $1800 */
			data &= 0xFE;
			nes_vram [6] = data * 64;
			nes_vram [7] = nes_vram [6] + 64;
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

		/* Note - this is an incrementing IRQ counter */
		case 0x7000: /* IRQ latch value */
			MMC3_IRQ = data;
			if (errorlog) fprintf (errorlog, "     Mapper 26 set latch: %02x\n", data);
			break;
		case 0x7001: /* Enable IRQs */
			MMC3_DOIRQ = data & 0x01;
			if (errorlog) fprintf (errorlog, "     Mapper 26 enable irqs: %02x\n", data);
			break;
		case 0x7002: /* Reset counter */
			if (errorlog) fprintf (errorlog, "     Mapper 26 copy latch (disable irqs): %02x\n", data);
			MMC3_IRQ = 0;
			break;
						
		default:
			if (errorlog) fprintf (errorlog, "Mapper 26 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void Mapper33_Write (int offset, int data)
{

//	if (errorlog) fprintf (errorlog, "mapper33_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);
	
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $A000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
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
			if (errorlog) fprintf (errorlog, "Uncaught mapper 33 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void Mapper34_Write (int offset, int data)
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a Mapper 34 game - the demo screens look wrong using mapper 7. */
	if (errorlog) fprintf (errorlog, "Mapper 34 w, offset: %04x, data: %02x\n", offset, data);
	data &= ((PRG_Rom >> 1) - 1);
	cpu_setbank (1, &ROM[data * 0x8000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x8000 + 0x12000]);
	cpu_setbank (3, &ROM[data * 0x8000 + 0x14000]);
	cpu_setbank (4, &ROM[data * 0x8000 + 0x16000]);
}	

static void Mapper64_Write (int offset, int data)
{
	static int cmd = 0;
	static int chr = 0;
	static int select_high;
	static int page;
						
//	if (errorlog) fprintf (errorlog, "mapper64_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);
	
	switch (offset & 0x7001)
	{
		case 0x0000:
//			if (errorlog) fprintf (errorlog, "Mapper 64 0x8000 write value: %02x\n",data);
			cmd = data & 0x0f;
			if (data & 0x80)
				chr = 0x1000;
			else
				chr = 0x0000;
			
			page = chr >> 10;
			/* Toggle switching between $8000/$A000/$C000 and $A000/$C000/$8000 */
			if (select_high != (data & 0x40))
			{
				if (errorlog) fprintf (errorlog, "HACK ALERT -- select_high has changed!\n");
				if (data & 0x40)
				{
					cpu_setbank (1, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
				}
				else
				{
					cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
				}
			}
				
			select_high = data & 0x40;
			if (errorlog) fprintf (errorlog, "   Mapper 64 select_high: %02x\n", select_high);
			break;

		case 0x0001:
			switch (cmd)
			{
				case 0:
					data &= 0xFE;
					nes_vram [page] = data * 64;
					nes_vram [page+1] = nes_vram [page] + 64;
					break;
								
				case 1:
					data &= 0xFE;
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
					data &= ((PRG_Rom << 1) - 1);
					if (select_high)
					{
						cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($A000) cmd 6 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($8000) cmd 6 value: %02x\n", data);
					}
					break;
				case 7:
					data &= ((PRG_Rom << 1) - 1);
					if (select_high)
					{
						cpu_setbank (3, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($C000) cmd 7 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($A000) cmd 7 value: %02x\n", data);
					}
					break;
				case 8:
					/* Switch 1k VROM at $0400 */
					nes_vram [1] = data * 64;
					break;
				case 9:
					/* Switch 1k VROM at $0C00 */
					nes_vram [3] = data * 64;
					break;
				case 15:
					data &= ((PRG_Rom << 1) - 1);
					if (select_high)
					{
						cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($C000) cmd 15 value: %02x\n", data);
					}
					else
					{
						cpu_setbank (3, &ROM[0x2000 * (data) + 0x10000]);
						if (errorlog) fprintf (errorlog, "     Mapper 64 switch ($A000) cmd 15 value: %02x\n", data);
					}
					break;
			}
			cmd = 16;
			break;
		case 0x2000:
			if (errorlog) fprintf (errorlog, "     Mapper 64 mirroring: %02x\n", data);
			Mirroring = 0x02 >> (data & 0x01);
			break;
		default:
			if (errorlog) fprintf (errorlog, "Mapper 64 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void Mapper65_Write (int offset, int data)
{
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (1, &ROM[0x2000 * (data) + 0x10000]);
			if (errorlog) fprintf (errorlog, "     Mapper 65 switch ($8000) value: %02x\n", data);
			break;

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= ((PRG_Rom << 1) - 1);
			cpu_setbank (2, &ROM[0x2000 * (data) + 0x10000]);
			if (errorlog) fprintf (errorlog, "     Mapper 65 switch ($a000) value: %02x\n", data);
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
		default:
			if (errorlog) fprintf (errorlog, "Mapper 65 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/* 
 * Rom switch (74HC161/74HC32) Jaleco - mapper 66
 * 
 * The PRG-ROM is switched in 32k chunks.
 */
static void Mapper66_Write (int offset, int data)
{
	int i;
	if (errorlog) fprintf (errorlog, "* Mapper 66 switch, offset %04x, data: %02x\n", offset, data);
	
	cpu_setbank (1, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x10000]);
	cpu_setbank (2, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x12000]);
	cpu_setbank (3, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x14000]);
	cpu_setbank (4, &ROM[((data & 0x30) >> 4) * 0x8000 + 0x16000]);

	for (i = 0; i < 8; i ++)
		nes_vram[i] = (data & 0x03) * 512 + 64*i;
}

/*
 * Mapper 68 - Sunsoft 4
 */
static void Mapper68_Write (int offset, int data)
{
	/* TODO: I can't figure out the mirroring/ppu screen base! */
	if (errorlog) fprintf (errorlog, "mapper68_w offset: %04x, data: %02x\n", offset, data);
	
	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 2k VROM at $0000 */
			nes_vram [0] = data * 128;
			nes_vram [1] = nes_vram [0] + 64;
			break;
		case 0x1000:
			/* Switch 2k VROM at $0800 */
			nes_vram [2] = data * 128;
			nes_vram [3] = nes_vram [2] + 64;
			break;
		case 0x2000:
			/* Switch 2k VROM at $1000 */
			nes_vram [4] = data * 128;
			nes_vram [5] = nes_vram [4] + 64;
			break;
		case 0x3000:
			/* Switch 2k VROM at $1800 */
			nes_vram [6] = data * 128;
			nes_vram [7] = nes_vram [6] + 64;
			break;
#if 0
		case 0x4000:
			/* Switch 4k VROM at $0000 */
			data = data & ((CHR_Rom << 1) - 1);
			for (i = 0; i < 4; i ++)
				nes_vram[i] = data * 256 + 64*i;
			break;
		case 0x5000:
			/* Switch 4k VROM at $1000 */
			data = data & ((CHR_Rom << 1) - 1);
			for (i = 4; i < 8; i ++)
				nes_vram[i] = data * 256 + 64*(i-4);
			break;
#endif
		case 0x6000:
			PPU_one_screen = 0;
			if (data & 0x10)
			{
				if (data & 0x01) PPU_one_screen = 0x2000;
				else PPU_one_screen = 0x2400;
			}
			Mirroring = (data & 0x01) + 1;
			break;
		case 0x7000:
			data &= (PRG_Rom - 1);
			cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
			cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
			break;
	}
}

/*
 * Mapper 71 - Camerica
 */
static void Mapper71_Write (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "* Mapper 71 switch %02x\n", data);
	data &= (PRG_Rom - 1);
	cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
}

/* 
 * Rom switch (74HC161/74HC32) Irem - mapper 78
 */
static void Mapper78_Write (int offset, int data)
{
	int bank;
	int i;
	
	if (errorlog) fprintf (errorlog, "* Mapper 78 switch %02x\n", data);
	/* Switch 8k VROM bank */
	bank = ((data & 0xf0)) >> 4 & (CHR_Rom - 1);
	for (i = 0; i < 8; i ++)
		nes_vram[i] = bank * 512 + 64*i;

	data &= (PRG_Rom - 1);
	cpu_setbank (1, &ROM[data * 0x4000 + 0x10000]);
	cpu_setbank (2, &ROM[data * 0x4000 + 0x12000]);
}

/*
	Mapper 79 - American Video Entertainment
	
	Writes to $4100 may be relevant also
*/

static void Mapper79_Write (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "Mapper 79 w: %04x:%02x\n", offset, data);
}

/*
// Reset_Mapper
//
// Resets the mapper bankswitch areas to their defaults. It returns a value "err"
// that indicates if it was successful. Possible values for err are:
//
// 0 = success
// 1 = no mapper found
// 2 = mapper not supported
*/
int Reset_Mapper (int mapperNum)
{
	int err = 0;
	int i;
	
	/* Set the vram bank-switch values to the default */
	for (i = 0; i < 8; i ++)
		nes_vram[i] = i * 64;
	
	mapper_warning = 0;
	uses_irqs = 0;
	
	switch (mapperNum)
	{
		case 0:
			err = 1; /* No mapper found */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
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
			
			if (!MMC1_extended)
				/* Set it to the end of the ROM */
				MMC1_High = (PRG_Rom - 1) * 0x4000;
			else
				/* Set it to the end of the first 256k bank */
				MMC1_High = 15 * 0x4000;

			MMC1_bank1 = 0;
			MMC1_bank2 = 0x2000;
			MMC1_bank3 = MMC1_High;
			MMC1_bank4 = MMC1_High + 0x2000;

			cpu_setbank (1, &ROM[MMC1_extended_base + MMC1_bank1]);
			cpu_setbank (2, &ROM[MMC1_extended_base + MMC1_bank2]);
			cpu_setbank (3, &ROM[MMC1_extended_base + MMC1_bank3]);
			cpu_setbank (4, &ROM[MMC1_extended_base + MMC1_bank4]);
			if (errorlog)
			{
				fprintf (errorlog, "-- page1: %06x\n", MMC1_bank1);
				fprintf (errorlog, "-- page2: %06x\n", MMC1_bank2);
				fprintf (errorlog, "-- page3: %06x\n", MMC1_bank3);
				fprintf (errorlog, "-- page4: %06x\n", MMC1_bank4);
			}
			break;
		case 2:
			/* These games don't switch VROM, but some ROMs incorrectly have CHR-ROM chunks */
			CHR_Rom = 0;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 3:
			/* Doesn't bank-switch */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 4:
			/* Can switch 8k ROM banks */
			MMC3_DOIRQ = 0;
			MMC3_IRQ = 0;
			uses_irqs = 1;
			cpu_setbank (1, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (2, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 5:
			/* Can switch 8k ROM banks, but they are saved as 16k in size */
			MMC5_rom_bank_mode = 0;
			MMC5_vrom_bank_mode = 0;
			MMC5_exram_tile_enable = 0;
			MMC5_exram_color_enable = 0;
			MMC3_DOIRQ = 0;
			MMC3_IRQ = 0;
			uses_irqs = 1;
			cpu_setbank (1, &ROM[(PRG_Rom-2) * 0x4000 + 0x10000]);
			cpu_setbank (2, &ROM[(PRG_Rom-2) * 0x4000 + 0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			PPU_one_screen = 0x2000;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 8:
			/* Switches 16k banks at $8000, 1st 2 16k banks loaded on reset */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 9:
			/* Can switch 8k ROM banks */
			/* Note that the iNES header defines the number of banks as 8k in size, rather than 16k */
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0xfe;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[(PRG_Rom-3) * 0x2000 + 0x10000]);
			cpu_setbank (3, &ROM[(PRG_Rom-2) * 0x2000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x2000 + 0x10000]);
			break;
		case 10:
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0xfe;
			MMC2_hibank1_val = MMC2_hibank2_val = 0;
			/* On reset, the second-to-last 16k of the cart is swapped in $8000-bfff */
			cpu_setbank (1, &ROM[(PRG_Rom-2) * 0x4000 + 0x10000]);
			cpu_setbank (2, &ROM[(PRG_Rom-2) * 0x4000 + 0x12000]);
			/* On reset, the last 16k of the cart is swapped in $c000-ffff */
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 11:
			/* Switches 32k banks, 1st 32k bank loaded on reset (?) May be more like mapper 7... */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 15:
			/* Can switch 8k ROM banks */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 16:
		case 17:
		case 18:
			uses_irqs = 1;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 25:
		case 33:
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 26:
			MMC3_DOIRQ = 0;
			MMC3_IRQ = 0;
			uses_irqs = 1;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 34:
			/* Can switch 32k ROM banks */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 64:
			/* Can switch 3 8k ROM banks */
			cpu_setbank (1, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			cpu_setbank (2, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 65:
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;		
		case 66:
			/* Can switch 32k ROM banks */
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 68:
		case 69:
		case 71:
		case 78:
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		case 79:
			/* Mirroring always horizontal...? */
//			Mirroring = 1;
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[0x14000]);
			cpu_setbank (4, &ROM[0x16000]);
			break;
		case 85:
			cpu_setbank (1, &ROM[0x10000]);
			cpu_setbank (2, &ROM[0x12000]);
			cpu_setbank (3, &ROM[(PRG_Rom-1) * 0x4000 + 0x10000]);
			cpu_setbank (4, &ROM[(PRG_Rom-1) * 0x4000 + 0x12000]);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}

	if (CHR_Rom)
		memcpy (videoram, VROM, 0x2000);
	
	return err;
}

mmc mmc_list[] = {
/*	INES   DESC                    WRITE    */
/*	  |     |                        |      */
	{ 0, "No Mapper",             NoneWR },
	{ 1, "MMC1",                  MMC1_Write },
	{ 2, "74HC161/74HC32",        RomSw_Write },
	{ 3, "VROM Switch",           VRomSw_Write },
	{ 4, "MMC3",                  Mapper4_Write }, 
	{ 5, "MMC5",                  Mapper5_Write }, 
	{ 7, "Rom Switch",            Mapper7_Write },
	{ 8, "FFE F3xxxx",            Mapper8_Write },
	{ 9, "MMC2",                  MMC2_Write },
	{ 10, "MMC4",                 MMC4_Write },
	{ 11, "Color Dreams Mapper",  CDrms_Write },
	{ 15, "100-in-1",             Mapper15_Write },
	{ 16, "Bandai",               Bandai_Write },
	{ 17, "FFE F8xxx",            NoneWR },
	{ 18, "Jaleco SS8806",        Mapper18_Write },
	{ 25, "Konami VRC 2/4 Even",  Mapper25_Write }, 
	{ 26, "Konami VRC 6 V",       Mapper26_Write }, 
	{ 33, "Taito TC0190",         Mapper33_Write },
	{ 34, "Nina-1",               Mapper34_Write },
	{ 64, "Rambo",                Mapper64_Write },
	{ 65, "Irem H3001",           Mapper65_Write },
	{ 66, "74161/32 Jaleco",      Mapper66_Write },
	{ 68, "SunSoft 4",            Mapper68_Write },
	{ 69, "SunSoft 5",            NoneWR },
	{ 71, "Camerica",             Mapper71_Write },
	{ 78, "74161/32 Irem",        Mapper78_Write },
	{ 79, "Nina-3 (AVE)",         Mapper79_Write },
	{ 85, "Konami VRC 7",         NoneWR },
	{ -1, "Not Supported",        NoneWR },
};
