/***************************************************************************
	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

****************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/generic.h"
#include "includes/wd179x.h"
#include "machine/mbee.h"

static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;
static void pio_interrupt(int state);
static void mbee_fdc_callback(int);

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
    wd179x_init(WD_TYPE_179X,mbee_fdc_callback);
}

void mbee_shutdown_machine(void)
{
	wd179x_exit();
}

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
READ_HANDLER ( mbee_pio_r )
{
    int data = z80pio_0_r(offset);
	if( offset != 2 )
		return data;

    data |= 0x01;
	if( device_input(IO_CASSETTE,0) > 255 )
		data &= ~0x01;

    return data;
}

WRITE_HANDLER ( mbee_pio_w )
{
    z80pio_0_w(offset,data);
	if( offset == 2 )
	{
		device_output(IO_CASSETTE,0,(data & 0x02) ? -32768 : 32767);
		speaker_level_w(0, (data >> 6) & 1);
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

READ_HANDLER ( mbee_fdc_status_r )
{
	logerror("mbee fdc_motor_r $%02X\n", fdc_status);
	return fdc_status;
}

WRITE_HANDLER ( mbee_fdc_motor_w )
{
	logerror("mbee fdc_motor_w $%02X\n", data);
	/* Controller latch bits
	 * 0-1	driver select
	 * 2	head select
	 * 3	density (0: FM, 1:MFM)
	 * 4-7	unused
	 */
	fdc_drv = data & 3;
	fdc_head = (data >> 2) & 1;
	fdc_den = (data >> 3) & 1;
	wd179x_set_drive(fdc_drv);
	wd179x_set_side(fdc_head);
	if (data & (1<<3))
	{
	   wd179x_set_density(DEN_FM_HI);
	}
	else
	{
	   wd179x_set_density(DEN_MFM_LO);
	}

}

int mbee_interrupt(void)
{
	int tape = readinputport(9);

	if( tape & 1 )
		device_status(IO_CASSETTE,0,1);
	if( tape & 2 )
		device_status(IO_CASSETTE,0,0);
	if( tape & 4 )
		device_seek(IO_CASSETTE,0,0,SEEK_SET);

    /* once per frame, pulse the PIO B bit 7 */
    logerror("mbee interrupt\n");
	z80pio_p_w(0, 1, 0x80);
    z80pio_p_w(0, 1, 0x00);
    return ignore_interrupt();
}

int mbee_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if( file )
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		if( device_open(IO_CASSETTE,id,0,&wa) )
			return INIT_FAIL;
        return INIT_PASS;
	}
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW_CREATE);
	if( file )
    {
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 11025;
		if( device_open(IO_CASSETTE,id,1,&wa) )
            return INIT_FAIL;
		return INIT_PASS;
    }
	return INIT_PASS;
}

void mbee_cassette_exit(int id)
{
	device_close(IO_CASSETTE,id);
}
#if 0
int mbee_floppy_init(int id)
{
	flop_specified[id] = device_filename(IO_FLOPPY,id) != NULL;
	return 0;
}
#endif

int mbee_rom_load(int id)
{
    void *file;

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
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
	return INIT_PASS;
}

