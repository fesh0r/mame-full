/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/astrocde.h"

/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

int CurrentScan=0;
static int NextScanInt=0;			/* Normal */

static int screen_interrupts_enabled;
static int screen_interrupt_mode;
static int lightpen_interrupts_enabled;
static int lightpen_interrupt_mode;

int astrocade_load_rom(int id)
{
    void *file;
	int size = 0;

    /* load a cartidge  */
	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
	if (file)
	{
		size = osd_fread(file, memory_region(REGION_CPU1) + 0x2000, 0x8000);
		osd_fclose(file);
	}
	return 0;
}

int astrocade_id_rom(int id)
{
	/* driver doesn't support ID'ing of images */
	return 0;
}

WRITE_HANDLER ( astrocade_interrupt_enable_w )
{

	screen_interrupts_enabled = data & 0x08;
	screen_interrupt_mode = data & 0x04;
	lightpen_interrupts_enabled = data & 0x02;
	lightpen_interrupt_mode = data & 0x01;

#ifdef MAME_DEBUG
    logerror("Interrupt Flag set to %02x\n",data & 0x0f);
#endif
}

WRITE_HANDLER ( astrocade_interrupt_w )
{
	/* A write to 0F triggers an interrupt at that scanline */

#ifdef MAME_DEBUG
	logerror("Scanline interrupt set to %02x\n",data);
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

    if (CurrentScan < 204) astrocade_copy_line(CurrentScan);

    /* Scanline interrupt enabled ? */

    if ((screen_interrupts_enabled) && (screen_interrupt_mode == 0)
	                                && (CurrentScan == NextScanInt))
		res = interrupt();

    return res;
}
