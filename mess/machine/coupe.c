/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/coupe.h"
#include "devices/basicdsk.h"
#include "includes/wd179x.h"

UINT8 LMPR,HMPR,VMPR;	/* Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252) */
UINT8 CLUT[16]; 		/* 16 entries in a palette (no line affects supported yet!) */
UINT8 SOUND_ADDR;		/* Current Address in sound registers */
UINT8 SOUND_REG[32];	/* 32 sound registers */
UINT8 LINE_INT; 		/* Line interrupt */
UINT8 LPEN,HPEN;		/* ??? */
UINT8 CURLINE;			/* Current scanline */
UINT8 STAT; 			/* returned when port 249 read */

extern UINT8 *sam_screen;

DEVICE_LOAD( coupe_floppy )
{
	if (device_load_basicdsk_floppy(image, file, open_mode)==INIT_PASS)
	{
		basicdsk_set_geometry(image, 80, 2, 10, 512, 1, 0, FALSE);
		return INIT_PASS;
	}
	return INIT_FAIL;
}

void coupe_update_memory(void)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int PAGE_MASK = (mess_ram_size / 0x4000) - 1;

    if (LMPR & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((LMPR & 0x1F) <= PAGE_MASK)
		{
			cpu_setbank(1,&mess_ram[(LMPR & PAGE_MASK) * 0x4000]);
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
		cpu_setbank(1, rom);	/* Rom0 paged in */
		cpu_setbank(1, rom);
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(1, 0, MWA_ROM);
	}

	if (( (LMPR+1) & 0x1F) <= PAGE_MASK)
	{
		cpu_setbank(2,&mess_ram[((LMPR+1) & PAGE_MASK) * 0x4000]);
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
		cpu_setbank(3,&mess_ram[(HMPR & PAGE_MASK)*0x4000]);
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
		cpu_setbank(4, rom + 0x4000);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);
		memory_set_bankhandler_w(4, 0, MWA_ROM);
	}
	else
	{
		if (( (HMPR+1) & 0x1F) <= PAGE_MASK)
		{
			cpu_setbank(4,&mess_ram[((HMPR+1) & PAGE_MASK) * 0x4000]);
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
		sam_screen = &mess_ram[((VMPR&0x1E) & PAGE_MASK) * 0x4000];
	else
		sam_screen = &mess_ram[((VMPR&0x1F) & PAGE_MASK) * 0x4000];
}

MACHINE_INIT( coupe )
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

    wd179x_init(WD_TYPE_177X,NULL);
}


/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

static void coupe_nmi_generate(int param)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}


