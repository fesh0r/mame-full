
#include "driver.h"
#include "vidhrdw/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "image.h"

/* the largest possible cartridge image, excluding special games */
#define PCE_ROM_MAXSIZE  (0x2000 * 256 + 512)

/* system RAM */
unsigned char *pce_user_ram;    /* scratch RAM at F8 */

/* joystick related data*/

#define JOY_CLOCK   0x01
#define JOY_RESET   0x02

static int joystick_port_select;        /* internal index of joystick ports */
static int joystick_data_select;        /* which nibble of joystick data we want */

DEVICE_LOAD(pce_cart)
{
	int size;
	unsigned char *ROM;
	logerror("*** pce_load_rom : %s\n", image_filename(image));

    /* open file to get size */
	if( new_memory_region(REGION_CPU1,PCE_ROM_MAXSIZE,0) )
		return 1;
	ROM = memory_region(REGION_CPU1);
    size = mame_fread(file, ROM, PCE_ROM_MAXSIZE);

    /* position back at start of file */
    mame_fseek(file, 0, SEEK_SET);

    /* handle header accordingly */
    if((size/512)&1)
    {
        logerror("*** pce_load_rom : Header present\n");
        size -= 512;
        mame_fseek(file, 512, SEEK_SET);
    }
    size = mame_fread(file, ROM, size);
	return 0;
}

NVRAM_HANDLER( pce )
{
	void *pce_save_ram = &memory_region(REGION_CPU1)[0x1EE000];

	if (read_or_write)
	{
		mame_fwrite(file, pce_save_ram, 0x2000);
	}
	else
	{
	    /* load battery backed memory from disk */
		memset(pce_save_ram, 0, 0x2000);
		if (file)
			mame_fread(file, pce_save_ram, 0x2000);
	}
}


/* todo: how many input ports does the PCE have? */
WRITE8_HANDLER ( pce_joystick_w )
{

    /* bump counter on a low-to-high transition of bit 1 */
    if((!joystick_data_select) && (data & JOY_CLOCK))
    {
        joystick_port_select = (joystick_port_select + 1) & 0x07;
    }

    /* do we want buttons or direction? */
    joystick_data_select = data & JOY_CLOCK;

    /* clear counter if bit 2 is set */
    if(data & JOY_RESET)
    {
        joystick_port_select = 0;
    }
}

 READ8_HANDLER ( pce_joystick_r )
{
    int data = readinputport(0);
    if(joystick_data_select) data >>= 4;
    return (data);
}
