/***************************************************************************
	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

****************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/generic.h"
#include "mess/machine/wd179x.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

static const char *cassette_name = NULL;
static const char *floppy_name[4] = {NULL,NULL,NULL,NULL};
static void *fdc_file[4] = {NULL,NULL};
static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;
static void pio_interrupt(int state);

static z80pio_interface pio_intf =
{
	1,					/* number of PIOs to emulate */
	{ pio_interrupt },	/* callback when change interrupt status */
	{ 0 },				/* portA ready active callback (do not support yet)*/
	{ 0 }				/* portB ready active callback (do not support yet)*/
};

static void pio_interrupt(int state)
{
	cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

void mbee_init_machine(void)
{
	z80pio_init(&pio_intf);
	wd179x_init(1);
}

void mbee_shutdown_machine(void)
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		if( fdc_file[i] )
			osd_fclose(fdc_file[i]);
		fdc_file[i] = NULL;
	}
}

int mbee_pio_r(int offset)
{
	return z80pio_0_r(offset);
}

void mbee_pio_w(int offset, int data)
{
	z80pio_0_w(offset,data);
	/* PIO B data bits
	 * 0	cassette data (input)
	 * 1	cassette data (output)
	 * 2	rs232 clock or DTR line
	 * 3	rs232 CTS line (0: clear to send)
	 * 4	rs232 input (0: mark)
	 * 5	rs232 output (1: mark)
	 * 6	speaker
	 * 7	network interrupt
	 */
	if( offset == 2 )
	{
		int dac = 0;
		if( data & 0x02 )
			dac |= 0x7f;
		if( data & 0x40 )
			dac |= 0x80;
		DAC_data_w(0, dac);
	}
}

static void mbee_fdc_callback(int param)
{
	switch( param )
	{
	case WD179X_IRQ_CLR:
//		cpu_set_irq_line(0,0,CLEAR_LINE);
		fdc_status &= ~0x40;
        break;
	case WD179X_IRQ_SET:
//		cpu_set_irq_line(0,0,HOLD_LINE);
		fdc_status |= 0x40;
        break;
	case WD179X_DRQ_CLR:
		fdc_status &= ~0x80;
		break;
	case WD179X_DRQ_SET:
		fdc_status |= 0x80;
        break;
    }
}

int mbee_fdc_status_r(int offset)
{
	if( errorlog ) fprintf(errorlog,"mbee fdc_motor_r $%02X\n", fdc_status);
	return fdc_status;
}

void mbee_fdc_motor_w(int offset, int data)
{
	if( errorlog ) fprintf(errorlog,"mbee fdc_motor_w $%02X\n", data);
	/* Controller latch bits
	 * 0-1	driver select
	 * 2	head select
	 * 3	density (0: FM, 1:MFM)
	 * 4-7	unused
	 */
	fdc_drv = data & 3;
	fdc_head = (data >> 2) & 1;
	fdc_den = (data >> 3) & 1;
	fdc_file[fdc_drv] = wd179x_select_drive(fdc_drv, fdc_head, mbee_fdc_callback, floppy_name[fdc_drv]);
}

int mbee_cassette_init(int id, const char *name)
{
	cassette_name = name;
	return 0;
}

int mbee_floppy_init(int id, const char *name)
{
	floppy_name[id] = name;
	return 0;
}

int mbee_rom_load(int id, const char *name)
{
    void *file;

	if( name && name[0] )
	{
		file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
		if( file )
		{
			int size = osd_fsize(file);
			UINT8 *mem = malloc(size);
			if( mem )
			{
				if( osd_fread(file, mem, size) == size )
				{
					memcpy(memory_region(REGION_CPU1)+0x8000, mem, size);
				}
				free(mem);
			}
			osd_fclose(file);
		}
	}
	return 0;
}

int mbee_rom_id(const char *name, const char *gamename)
{
    void *file;

    file = osd_fopen(gamename, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
		osd_fclose(file);
    }
    return 1;
}


