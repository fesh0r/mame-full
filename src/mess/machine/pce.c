
#include "driver.h"
#include "mess/vidhrdw/vdc.h"
#include "cpu/h6280/h6280.h"

/* the largest possible cartridge image, excluding special games */
#define PCE_ROM_MAXSIZE  (0x2000 * 256 + 512)

/* system RAM */
unsigned char *pce_user_ram;    /* scratch RAM at F8 */
unsigned char *pce_save_ram;    /* battery backed RAM at F7 */

/* joystick related data*/

#define JOY_CLOCK   0x01
#define JOY_RESET   0x02

static int joystick_port_select;        /* internal index of joystick ports */
static int joystick_data_select;        /* which nibble of joystick data we want */

int pce_load_rom(int id)
{
	int size;
    FILE *fp = NULL;
	unsigned char *ROM;
	if(errorlog) fprintf(errorlog, "*** pce_load_rom : %s\n", device_filename(IO_CARTSLOT,id));

    /* open file to get size */
	fp = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
    if(!fp) return 1;
	if( new_memory_region(REGION_CPU1,PCE_ROM_MAXSIZE) )
		return 1;
	ROM = memory_region(REGION_CPU1);
    size = osd_fread(fp, ROM, PCE_ROM_MAXSIZE);

    /* position back at start of file */
    osd_fseek(fp, 0, SEEK_SET);

    /* handle header accordingly */
    if((size/512)&1)
    {
        if(errorlog) fprintf(errorlog, "*** pce_load_rom : Header present\n");
        size -= 512;
        osd_fseek(fp, 512, SEEK_SET);
    }
    size = osd_fread(fp, ROM, size);
    osd_fclose(fp);

    return 0;
}

int pce_id_rom (int id)
{
    if(errorlog) fprintf(errorlog, "*** pce_id_rom\n");
    return 0;
}

void pce_init_machine(void)
{
    void *f;

    if(errorlog) fprintf(errorlog, "*** pce_init_machine\n");
    pce_user_ram = calloc(0x2000, 1);
    pce_save_ram = calloc(0x2000, 1);


    /* load battery backed memory from disk */
    f = osd_fopen(Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
    if(f)
    {
        if(errorlog) fprintf(errorlog, "*** pce_init_machine - BRAM loaded\n");
        osd_fread(f, pce_save_ram, 0x2000);
        osd_fclose(f);
    }
}

void pce_stop_machine(void)
{
    void *f;

    if(errorlog) fprintf(errorlog, "*** pce_stop_machine\n");

    /* write battery backed memory to disk */
    f = osd_fopen(Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
    if(f)
    {
        if(errorlog) fprintf(errorlog, "*** pce_stop_machine - BRAM saved\n");
        osd_fwrite(f, pce_save_ram, 0x2000);
        osd_fclose(f);
    }

    if(pce_user_ram) free(pce_user_ram);
    if(pce_save_ram) free(pce_save_ram);
}


/* todo: how many input ports does the PCE have? */
void pce_joystick_w(int offset, int data)
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

int pce_joystick_r(int offset)
{
    int data = readinputport(0);
    if(joystick_data_select) data >>= 4;
    return (data);
}
