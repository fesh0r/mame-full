/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/coupe.h"

#define COUPE_256

#ifdef COUPE_256
	#define PAGE_MASK	0x0f
#else
	#define PAGE_MASK	0x1f
#endif

unsigned char *coupe_ram=NULL;

unsigned char LMPR,HMPR,VMPR;								// Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252)
unsigned char CLUT[16];										// 16 entries in a palette (no line affects supported yet!)
unsigned char SOUND_ADDR;									// Current Address in sound registers
unsigned char SOUND_REG[32];								// 32 sound registers
unsigned char LINE_INT;										// Line interrupt
unsigned char LPEN,HPEN;									// ???
unsigned char CURLINE;										// Current scanline
unsigned char STAT;											// returned when port 249 read

struct sCoupe_fdc1772 coupe_fdc1772[2];						// Holds the floppy controller vars for each drive

extern unsigned char *sam_screen;

int coupe_fdc_init(int id)
{
	coupe_fdc1772[id].Dsk_Command=0x00;		// not needed to be set really (just done for consistency)
	coupe_fdc1772[id].Dsk_Status=0x80;		// not ready - incase someone tries to read when a dsk image is not present
	coupe_fdc1772[id].Dsk_Data=0x00;
	coupe_fdc1772[id].Dsk_Track=0x00;
	coupe_fdc1772[id].Dsk_Sector=0x00;

	coupe_fdc1772[id].f = image_fopen(IO_FLOPPY,id,OSD_FILETYPE_IMAGE_RW,0);	// attempt to open requested file

	if ( coupe_fdc1772[id].f == NULL )
	{
		logerror("Could not open image %s on floppy %d\n", device_filename(IO_FLOPPY,id),id);
		return INIT_FAILED;
	}

	coupe_fdc1772[id].Dsk_Status=0x00;		// disk inserted so drive is now ready!

	return INIT_OK;
}

void coupe_update_memory(void)
{
	if (LMPR & LMPR_RAM0)										// Is ram paged in at bank 1
	{
		if ((LMPR & 0x1F)<=PAGE_MASK)
		{
			cpu_setbank(1,coupe_ram + ((LMPR & PAGE_MASK)*0x4000));
			cpu_setbank(5,coupe_ram + ((LMPR & PAGE_MASK)*0x4000));
			cpu_setbankhandler_r(1, MRA_BANK1);
			cpu_setbankhandler_w(5, MWA_BANK5);
		}
		else
		{
			cpu_setbankhandler_r(1, MRA_NOP);					// Attempt to page in non existant ram region
			cpu_setbankhandler_w(5, MWA_NOP);
		}
	}
	else
	{
		cpu_setbank(1,memory_region(REGION_CPU1) + 0x010000);	// Rom0 paged in
		cpu_setbank(5,memory_region(REGION_CPU1) + 0x010000);
		cpu_setbankhandler_r(1, MRA_BANK1);
		cpu_setbankhandler_w(5, MWA_NOP);
	}

	if (( (LMPR+1) & 0x1F)<=PAGE_MASK)
	{
		cpu_setbank(2,coupe_ram + (((LMPR+1) & PAGE_MASK)*0x4000));
		cpu_setbank(6,coupe_ram + (((LMPR+1) & PAGE_MASK)*0x4000));
		cpu_setbankhandler_r(2, MRA_BANK2);
		cpu_setbankhandler_w(6, MWA_BANK6);
	}
	else
	{
		cpu_setbankhandler_r(2, MRA_NOP);					// Attempt to page in non existant ram region
		cpu_setbankhandler_w(6, MWA_NOP);
	}

	if (( HMPR & 0x1F)<=PAGE_MASK)
	{
		cpu_setbank(3,coupe_ram + ((HMPR & PAGE_MASK)*0x4000));
		cpu_setbank(7,coupe_ram + ((HMPR & PAGE_MASK)*0x4000));
		cpu_setbankhandler_r(3, MRA_BANK3);
		cpu_setbankhandler_w(7, MWA_BANK7);
	}
	else
	{
		cpu_setbankhandler_r(3, MRA_NOP);					// Attempt to page in non existant ram region
		cpu_setbankhandler_w(7, MWA_NOP);
	}

	if (LMPR & LMPR_ROM1)										// Is Rom1 paged in at bank 4
	{
		cpu_setbank(4,memory_region(REGION_CPU1) + 0x014000);
		cpu_setbank(8,memory_region(REGION_CPU1) + 0x014000);
		cpu_setbankhandler_r(4, MRA_BANK4);
		cpu_setbankhandler_w(8, MWA_NOP);
	}
	else
	{
		if (( (HMPR+1) & 0x1F)<=PAGE_MASK)
		{
			cpu_setbank(4,coupe_ram + (((HMPR+1) & PAGE_MASK)*0x4000));
			cpu_setbank(8,coupe_ram + (((HMPR+1) & PAGE_MASK)*0x4000));
			cpu_setbankhandler_r(4, MRA_BANK4);
			cpu_setbankhandler_w(8, MWA_BANK8);
		}
		else
		{
			cpu_setbankhandler_r(4, MRA_NOP);					// Attempt to page in non existant ram region
			cpu_setbankhandler_w(8, MWA_NOP);
		}
	}

	if (VMPR & 0x40)											// if bit set in 2 bank screen mode
		sam_screen=coupe_ram + (((VMPR&0x1E) & PAGE_MASK)*0x4000);
	else
		sam_screen=coupe_ram + (((VMPR&0x1F) & PAGE_MASK)*0x4000);
}

void coupe_init_machine(void)
{
	coupe_ram = (unsigned char *)malloc((PAGE_MASK+1)*16*1024);
	memset(coupe_ram, 0, (PAGE_MASK+1)*16*1024);

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
	cpu_setbankhandler_r(3, MRA_BANK3);
	cpu_setbankhandler_r(4, MRA_BANK4);

	cpu_setbankhandler_w(5, MWA_BANK5);
	cpu_setbankhandler_w(6, MWA_BANK6);
	cpu_setbankhandler_w(7, MWA_BANK7);
	cpu_setbankhandler_w(8, MWA_BANK8);

	LMPR = 0x0F;			// ROM0 paged in, ROM1 paged out RAM Banks
	HMPR = 0x01;
	VMPR = 0x81;

	LINE_INT = 0xFF;
	LPEN = 0x00;
	HPEN = 0x00;

	STAT = 0x1F;

	CURLINE = 0x00;

	coupe_update_memory();
}

void coupe_shutdown_machine(void)
{
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


