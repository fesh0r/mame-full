/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/coupe.h"
#include "includes/basicdsk.h"
#include "includes/wd179x.h"

UINT8 LMPR,HMPR,VMPR;	/* Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252) */
UINT8 CLUT[16]; 		/* 16 entries in a palette (no line affects supported yet!) */
UINT8 SOUND_ADDR;		/* Current Address in sound registers */
UINT8 SOUND_REG[32];	/* 32 sound registers */
UINT8 LINE_INT; 		/* Line interrupt */
UINT8 LPEN,HPEN;		/* ??? */
UINT8 CURLINE;			/* Current scanline */
UINT8 STAT; 			/* returned when port 249 read */
UINT32 RAM_SIZE;		/* RAM size (256K or 512K) */
UINT8 PAGE_MASK;		/* 256K = 0x0f, 512K = 0x1f */

extern UINT8 *sam_screen;

int coupe_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_OK)
	{

		basicdsk_set_geometry(id, 80, 2, 10, 512, 1);

		return INIT_OK;
	}

	return INIT_FAILED;
}

void coupe_update_memory(void)
{
	UINT8 *mem = memory_region(REGION_CPU1);

    if (LMPR & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((LMPR & 0x1F) <= PAGE_MASK)
		{
			cpu_setbank(1,&mem[(LMPR & PAGE_MASK) * 0x4000]);
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(1, 0, MWA_BANK1);
		}
		else
		{
			memory_set_bankhandler_r(1, 0, MRA_NOP);	/* Attempt to page in non existant ram region */
			memory_set_bankhandler_w(1, 0, MWA_NOP);
		}
	}
	else
	{
		cpu_setbank(1,memory_region(REGION_CPU1) + RAM_SIZE);	/* Rom0 paged in */
		cpu_setbank(1,memory_region(REGION_CPU1) + RAM_SIZE);
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(1, 0, MWA_ROM);
	}

	if (( (LMPR+1) & 0x1F) <= PAGE_MASK)
	{
		cpu_setbank(2,&mem[((LMPR+1) & PAGE_MASK) * 0x4000]);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_w(2, 0, MWA_BANK2);
	}
	else
	{
		memory_set_bankhandler_r(2, 0, MRA_NOP);	/* Attempt to page in non existant ram region */
		memory_set_bankhandler_w(2, 0, MWA_NOP);
	}

	if ( (HMPR & 0x1F) <= PAGE_MASK )
	{
		cpu_setbank(3,&mem[(HMPR & PAGE_MASK)*0x4000]);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_w(3, 0, MWA_BANK3);
	}
	else
	{
		memory_set_bankhandler_r(3, 0, MRA_NOP);	/* Attempt to page in non existant ram region */
		memory_set_bankhandler_w(3, 0, MWA_NOP);
	}

	if (LMPR & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
	{
		cpu_setbank(4,mem + RAM_SIZE + 0x4000);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);
		memory_set_bankhandler_w(4, 0, MWA_ROM);
	}
	else
	{
		if (( (HMPR+1) & 0x1F) <= PAGE_MASK)
		{
			cpu_setbank(4,&mem[((HMPR+1) & PAGE_MASK) * 0x4000]);
			memory_set_bankhandler_r(4, 0, MRA_BANK4);
			memory_set_bankhandler_w(4, 0, MWA_BANK4);
		}
		else
		{
			memory_set_bankhandler_r(4, 0, MRA_NOP);	/* Attempt to page in non existant ram region */
			memory_set_bankhandler_w(4, 0, MWA_NOP);
		}
	}

	if (VMPR & 0x40)	/* if bit set in 2 bank screen mode */
		sam_screen = &mem[((VMPR&0x1E) & PAGE_MASK) * 0x4000];
	else
		sam_screen = &mem[((VMPR&0x1F) & PAGE_MASK) * 0x4000];
}

void coupe_init_machine_common(void)
{
	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_w(1, 0, MWA_BANK1);
    memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_w(2, 0, MWA_BANK2);
    memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_w(3, 0, MWA_BANK3);
    memory_set_bankhandler_r(4, 0, MRA_BANK4);
	memory_set_bankhandler_w(4, 0, MWA_BANK4);


    LMPR = 0x0F;            /* ROM0 paged in, ROM1 paged out RAM Banks */
    HMPR = 0x01;
    VMPR = 0x81;

    LINE_INT = 0xFF;
    LPEN = 0x00;
    HPEN = 0x00;

    STAT = 0x1F;

    CURLINE = 0x00;

    coupe_update_memory();

    wd179x_init(NULL);
}

void coupe_init_machine_256(void)
{
	PAGE_MASK = 0x0f;
	RAM_SIZE = 0x40000;
	coupe_init_machine_common();
}

void coupe_init_machine_512(void)
{
	PAGE_MASK = 0x1f;
	RAM_SIZE = 0x80000;
	coupe_init_machine_common();
}

void coupe_shutdown_machine(void)
{
	wd179x_exit();
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

void coupe_nmi_generate(int param)
{
	cpu_cause_interrupt(0, Z80_NMI_INT);
}


