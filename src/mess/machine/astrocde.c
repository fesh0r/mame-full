/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

extern void AstrocadeCopyLine(int Line); 

/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

int CurrentScan=0;
static int NextScanInt=0;			/* Normal */

static int screen_interrupts_enabled;
static int screen_interrupt_mode;
static int lightpen_interrupts_enabled;
static int lightpen_interrupt_mode;
	
int astrocade_load_rom(void)
{
	static char filename[200];
	void *file;
	int size;
	
	/* load a cartidge  */
	if (strlen(rom_name[0]))
	{
		strcpy(filename, rom_name[0]);
		file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_ROM_CART, 0);
		if (file)
		{
			size = osd_fread(file, Machine->memory_region[0] + 0x2000, 0x8000);
			osd_fclose(file);
        }
	}	return 0;
}

int astrocade_id_rom(const char *name, const char *gamename)
{
	/* driver doesn't support ID'ing of images */
	return 0;
}

void astrocade_interrupt_enable_w(int offset, int data)
{

	screen_interrupts_enabled = data & 0x08;
	screen_interrupt_mode = data & 0x04;
	lightpen_interrupts_enabled = data & 0x02;
	lightpen_interrupt_mode = data & 0x01;

#ifdef MAME_DEBUG
    if (errorlog) fprintf(errorlog,"Interrupt Flag set to %02x\n",data & 0x0f);
#endif
}

void astrocade_interrupt_w(int offset, int data)
{
	/* A write to 0F triggers an interrupt at that scanline */

#ifdef MAME_DEBUG
	if (errorlog) fprintf(errorlog,"Scanline interrupt set to %02x\n",data);
#endif

    NextScanInt = data;
}

int astrocade_interrupt(void)
{
	int res=ignore_interrupt();
//    int Direction;

    CurrentScan++;

    if (CurrentScan == Machine->drv->cpu[0].vblank_interrupts_per_frame)
	{
		CurrentScan = 0;
    }

    if (CurrentScan < 204) AstrocadeCopyLine(CurrentScan);

    /* Scanline interrupt enabled ? */

    if ((screen_interrupts_enabled) && (screen_interrupt_mode == 0) 
	                                && (CurrentScan == NextScanInt))
		res = interrupt();
		
    return res;
}
