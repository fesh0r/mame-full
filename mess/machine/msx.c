/*
** msx.c : MSX1 emulation
**
** Todo:
**
** - memory emulation needs be rewritten
** - add support for serial ports
** - fix mouse support
** - add support for SCC+ and megaRAM
** - diskdrives support doesn't work yet!
**
** Sean Young
*/

#include "driver.h"
#include "includes/msx.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "includes/tc8521.h"
#include "includes/wd179x.h"
#include "devices/basicdsk.h"
#include "vidhrdw/tms9928a.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80_msx.h"
#include "vidhrdw/v9938.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "utils.h"
#include "image.h"

#ifndef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x) )
#endif

static MSX msx1;
static void msx_set_all_mem_banks (void);
static WRITE_HANDLER ( msx_ppi_port_a_w );
static WRITE_HANDLER ( msx_ppi_port_c_w );
static READ_HANDLER (msx_ppi_port_b_r );

static ppi8255_interface msx_ppi8255_interface =
{
	1,
	{NULL}, 
	{msx_ppi_port_b_r},
	{NULL},
	{msx_ppi_port_a_w},
	{NULL}, 
	{msx_ppi_port_c_w}
};

static char PAC_HEADER[] = "PAC2 BACKUP DATA";
#define PAC_HEADER_LEN (16)

static int msx_probe_type (UINT8* pmem, int size)
{
    int kon4, kon5, asc8, asc16, i;

    if (size <= 0x10000) return 0;

    if ( (pmem[0x10] == 'Y') && (pmem[0x11] == 'Z') && (size > 0x18000) )
        return 6;

    kon4 = kon5 = asc8 = asc16 = 0;

    for (i=0;i<size-3;i++)
    {
        if (pmem[i] == 0x32 && pmem[i+1] == 0)
        {
            switch (pmem[i+2]) {
            case 0x60:
            case 0x70:
                asc16++;
                asc8++;
                break;
            case 0x68:
            case 0x78:
                asc8++;
                asc16--;
            }

            switch (pmem[i+2]) {
            case 0x60:
            case 0x80:
            case 0xa0:
                kon4++;
                break;
            case 0x50:
            case 0x70:
            case 0x90:
            case 0xb0:
                kon5++;
            }
        }
    }

    if (MAX (kon4, kon5) > MAX (asc8, asc16) )
        return (kon5 > kon4) ? 2 : 3;
    else
        return (asc8 > asc16) ? 4 : 5;
}

DEVICE_LOAD( msx_cart )
{
    UINT8 *pmem,*m;
    int size,size_aligned,n,p,type,i;
    const char *pext;
	char *pext2;
	char buf[PAC_HEADER_LEN + 2];
    static const char *mapper_types[] = { "none", "MSX-DOS 2", "konami5 with SCC",
        "konami4 without SCC", "ASCII/8kB", "ASCII//16kB",
        "Konami Game Master 2", "ASCII/8kB with 8kB SRAM",
        "ASCII/16kB with 2kB SRAM", "R-Type", "Konami Majutsushi",
        "Panasonic FM-PAC", "Super Load Runner",
        "Konami Synthesizer", "Cross Blaim", "Disk ROM",
		"Korean 80-in-1", "Korean 126-in-1" };

	int id = image_index_in_device(image);

    /* try to load it */
    size = mame_fsize (file);
    if (size < 0x2000)
    {
        logerror("%s: file to small\n", image_filename(image));
        return 1;
    }
    /* get mapper type */
    pext = image_extrainfo (image);
	if (!pext || (1 != sscanf (pext, "%d", &type) ) )
   	{
		logerror("Cart: No extra info found in crc file\n");
       	type = -1;
   	}
   	else
   	{
       	if (type < 0 || type > 17)
		{
			logerror("Cart: Invalid extra info\n");
           	type = -1;
		}
      	else logerror("Cart extra info: %s\n", pext);
   	}

    /* calculate aligned size (8, 16, 32, 64, 128, 256, etc. (kB) ) */
    size_aligned = 0x2000;
    while (size_aligned < size) size_aligned *= 2;

    pmem = (UINT8*) image_malloc(image, size_aligned);
    if (!pmem)
		goto error;

	memset (pmem, 0xff, size_aligned);
    if (mame_fread (file, pmem, size) != size)
    {
        logerror("%s: can't read file\n", image_filename (image));
		goto error;
    }

    /* check type */
    if (type < 0)
    {
        type = msx_probe_type (pmem, size);

        if ( !( (pmem[0] == 'A') && (pmem[1] == 'B') ) )
        {
            logerror("%s: May not be a valid ROM file\n",image_filename (image) );
        }

        logerror("Probed cartridge mapper %s\n", mapper_types[type]);
    }

    /* mapper type 0 always needs 64kB */
    if (!type)
    {
        size_aligned = 0x10000;
        pmem = image_realloc(image, pmem, 0x10000);
        if (!pmem)
			goto error;

		if (size < 0x10000)
			memset (pmem + size, 0xff, 0x10000 - size);
        if (size > 0x10000)
			size = 0x10000;
    }

    /* set mapper specific stuff */
    msx1.cart[id].mem = pmem;
    msx1.cart[id].type = type;
    msx1.cart[id].bank_mask = (size_aligned / 0x2000) - 1;
    for (i=0;i<4;i++) msx1.cart[id].banks[i] = (i & msx1.cart[id].bank_mask);
    logerror("Cart #%d size %d, mask %d, type: %s\n",id, size, msx1.cart[id].bank_mask, mapper_types[type]);
    /* set filename for sram (memcard) */
    msx1.cart[id].sramfile = image_malloc(image, strlen(image_filename(image)) + 1);
    if (!msx1.cart[id].sramfile)
		goto error;

	strcpy(msx1.cart[id].sramfile, image_basename(image));
    pext2 = strrchr (msx1.cart[id].sramfile, '.');
    if (pext2)
		*pext2 = 0;

	/* do some stuff for some types :)) */
    switch (type) {
    case 0:
        /*
         * mapper-less type; determine what page it should be in .
         * After the 'AB' there are 4 pointers to somewhere in the
         * rom itself. NULL doesn't count, so the first non-zero
         * pointer determines the page. page 1 is the most common,
         * so we default to that.
         */

        p = 1;
		if ( (pmem[0] == 'A') && (pmem[1] == 'B') )
		{
	    	for (n=2;n<=8;n+=2)
        	{
            	if (pmem[n] || pmem[n+1])
            	{
                	/* this hack works on all byte order systems */
                	p = pmem[n+1] / 0x40;
                	break;
            	}
        	}
		}

        if (size <= 0x4000)
        {
            if (p == 1 || p == 2)
            {
                /* copy to the respective page */
                memcpy (pmem+(p*0x4000), pmem, 0x4000);
                memset (pmem, 0xff, 0x4000);
            } else {
                /* memory is repeated 4 times */
                p = -1;
                memcpy (pmem + 0x4000, pmem, 0x4000);
                memcpy (pmem + 0x8000, pmem, 0x4000);
                memcpy (pmem + 0xc000, pmem, 0x4000);
            }
        }
        else /*if (size <= 0xc000) */
        {
            if (p)
            {
                /* shift up 16kB; custom memcpy so overlapping memory
                   isn't corrupted. ROM starts in page 1 (0x4000) */
                p = 1;
                n = 0xc000; m = pmem + 0xffff;
                while (n--) { *m = *(m - 0x4000); m--; }
                memset (pmem, 0xff, 0x4000);
            }
        }

        {
            if (p >= 0)
                logerror("Cart #%d in page %d\n", id, p);
            else
                logerror("Cart #%d memory duplicated in all pages\n", id);
        }
        break;
   case 1: /* msx-dos 2: extra blank page for page 2 */
        pmem = image_realloc(image, msx1.cart[id].mem, 0x12000);
        if (!pmem)
			goto error;

		msx1.cart[id].mem = pmem;
        msx1.cart[id].banks[2] = 8;
        msx1.cart[id].banks[3] = 8;
        break;
   case 6: /* game master 2; try to load sram */
        pmem = image_realloc(image, msx1.cart[id].mem, 0x24000);
        if (!pmem)
			goto error;

		i = 0;
        file = mame_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                FILETYPE_MEMCARD, 0);
		if (file)
			{
			n = mame_fsize (file);
			if (n == 0x2000)
				{
	        	if (mame_fread (file, pmem + 0x21000, 0x2000) == 0x2000)
	   		    	{
   	         		memcpy (pmem + 0x20000, pmem + 0x21000, 0x1000);
            		memcpy (pmem + 0x23000, pmem + 0x22000, 0x1000);
            		i = 1;
					}
				}

			/* if it's an Virtual MSX Game Master 2 file, convert */
			if (n == 0x4000)
				{
	        	if (mame_fread (file, pmem + 0x20000, 0x4000) == 0x4000)
	   		    	{
   	         		memcpy (pmem + 0x20000, pmem + 0x21000, 0x1000);
            		memcpy (pmem + 0x22000, pmem + 0x23000, 0x1000);
            		i = 1;
					}
				}
			}

        if (file) mame_fclose (file);

		if (i)
			logerror ("Cart #%d SRAM loaded\n", id);
		else
		{
			memset (pmem + 0x20000, 0, 0x4000);
			logerror("Cart #%d Failed to load SRAM\n", id);
		}

        msx1.cart[id].mem = pmem;
        break;

    case 2: /* Konami SCC */
        /* we want an extra page that looks like the SCC page */
        pmem = image_realloc(image, pmem, size_aligned + 0x2000);
        if (!pmem)
			goto error;

		memcpy (pmem + size_aligned, pmem + size_aligned - 0x2000, 0x1800);
        for (i=0;i<8;i++)
        {
            memset (pmem + size_aligned + i * 0x100 + 0x1800, 0, 0x80);
            memset (pmem + size_aligned + i * 0x100 + 0x1880, 0xff, 0x80);
        }
        msx1.cart[id].mem = pmem;
        /*msx1.cart[id].banks[0] = 0x1; */
        break;
   case 7: /* ASCII/8kB with SRAM */
		pmem = image_realloc(image, msx1.cart[id].mem, size_aligned + 0x2000);
		if (!pmem)
			goto error;

		file = mame_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
				FILETYPE_MEMCARD, 0);
		if (file && (mame_fread (file, pmem + size_aligned, 0x2000) == 0x2000) )
		{
			logerror("Cart #%d SRAM loaded\n", id);
		} else {
			memset (pmem + size_aligned, 0, 0x2000);
			logerror("Cart #%d Failed to load SRAM\n", id);
		}
		if (file) mame_fclose (file);

		msx1.cart[id].mem = pmem;
		break;
   case 8: /* ASCII/16kB with SRAM */
		pmem = image_realloc(image, msx1.cart[id].mem, size_aligned + 0x4000);
		if (!pmem)
			goto error;

		file = mame_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
				FILETYPE_MEMCARD, 0);
		if (file && (mame_fread (file, pmem + size_aligned, 0x2000) == 0x2000) )
		{
			for (i=1;i<8;i++)
			{
				memcpy (pmem + size_aligned + i * 0x800,
					pmem + size_aligned, 0x800);
			}
			logerror("Cart #%d SRAM loaded\n", id);
			if (file)
				mame_fclose (file);
		}
		else
		{
			memset (pmem + size_aligned, 0, 0x4000);
			logerror("Cart #%d Failed to load SRAM\n", id);
		}

		msx1.cart[id].mem = pmem;
		break;
    case 9: /* R-Type */
        msx1.cart[id].banks[0] = 0x1e;
        msx1.cart[id].banks[1] = 0x1f;
        msx1.cart[id].banks[2] = 0x1e;
        msx1.cart[id].banks[3] = 0x1f;
        break;
    case 11: /* fm-pac */
		msx1.cart[id].pacsram = !strncmp ((char*)msx1.cart[id].mem + 0x18, "PAC2", 4);
		pmem = image_realloc(image, msx1.cart[id].mem, 0x18000);
		if (!pmem)
		{
			msx1.cart[id].mem = NULL;
			return 1;
		}
		memset (pmem + size_aligned, 0xff, 0x18000 - size_aligned);
		pmem[0x13ff6] = 0;
		pmem[0x13ff7] = 0;
		if (msx1.cart[id].pacsram)
		{
			file = mame_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
				FILETYPE_MEMCARD, 0);
			if (file &&
				(mame_fread (file, buf, PAC_HEADER_LEN) == PAC_HEADER_LEN) &&
				!strncmp (buf, PAC_HEADER, PAC_HEADER_LEN) &&
				(mame_fread (file, pmem + 0x10000, 0x1ffe) == 0x1ffe) )
			{
				logerror("Cart #%d SRAM loaded\n", id);
			} else {
				memset (pmem + 0x10000, 0, 0x2000);
				logerror("Cart #%d Failed to load SRAM\n", id);
			}
			if (file)
				mame_fclose (file);
		}
		msx1.cart[id].banks[2] = (0x14000/0x2000);
		msx1.cart[id].banks[3] = (0x16000/0x2000);
		msx1.cart[id].mem = pmem;
		break;
    case 5: /* ASCII 16kb */
        msx1.cart[id].banks[0] = 0;
        msx1.cart[id].banks[1] = 1;
        msx1.cart[id].banks[2] = 0;
        msx1.cart[id].banks[3] = 1;
        break;
    case 12: /* Super Load Runner */
        msx1.cart[id].banks[0] = 1;
        msx1.cart[id].banks[1] = 1;
        msx1.cart[id].banks[2] = 0;
        msx1.cart[id].banks[3] = 1;
        break;
	case 15: /* disk rom */
        pmem = image_realloc(image, msx1.cart[id].mem, 0x8000);
        if (!pmem)
			goto error;

		msx1.cart[id].mem = pmem;
		memset (pmem + 0x4000, 0xff, 0x4000);
        msx1.cart[id].banks[2] = 2;
        msx1.cart[id].banks[3] = 3;
		break;
	}

    if (msx1.run)
		msx_set_all_mem_banks ();
    return INIT_PASS;

error:
	msx1.cart[id].mem = NULL;
	return INIT_FAIL;
}

static int save_sram (int id, char *filename, UINT8* pmem, int size)
{
    mame_file *F;
    int res;

    F = mame_fopen (Machine->gamedrv->name, filename, FILETYPE_MEMCARD, 1);
    res = F && (mame_fwrite (F, pmem, size) == size);
    if (F) mame_fclose (F);
    return res;
}

DEVICE_UNLOAD( msx_cart )
{
	int id = image_index_in_device(image);
    mame_file *F;
    int size,res;

    if (msx1.cart[id].mem)
    {
        /* save sram thingies */
        switch (msx1.cart[id].type) {
        case 6:
            res = save_sram (id, msx1.cart[id].sramfile,
                msx1.cart[id].mem + 0x21000, 0x2000);
            break;
        case 7:
            res = save_sram (id, msx1.cart[id].sramfile,
                msx1.cart[id].mem + (msx1.cart[id].bank_mask + 1) * 0x2000,
                0x2000);
            break;
        case 8:
            res = save_sram (id, msx1.cart[id].sramfile,
                msx1.cart[id].mem + (msx1.cart[id].bank_mask + 1) * 0x2000,
                0x800);
            break;
        case 11: /* fm-pac */
            res = 1;
            F = mame_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                FILETYPE_MEMCARD, 1);
            if (!F) break;
            size = strlen (PAC_HEADER);
            if (mame_fwrite (F, PAC_HEADER, size) != size)
                { mame_fclose (F); break; }
            if (mame_fwrite (F, msx1.cart[id].mem + 0x10000, 0x1ffe) != 0x1ffe)
                { mame_fclose (F); break; }
            mame_fclose (F);
            res = 0;
            break;
        default:
            res = -1;
            break;
        }
        if (res == 0) {
            logerror("Cart %d# SRAM saved\n", id);
        } else if (res > 0) {
            logerror("Cart %d# failed to save SRAM\n", id);
        }
    }
}

void msx_vdp_interrupt(int i)
{
    cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

static void msx_ch_reset_core (void)
{
	int i;

    /* set interrupt stuff */
    cpu_irq_line_vector_w(0,0,0xff);
    /* setup PPI */
    ppi8255_init (&msx_ppi8255_interface);

    /* initialize mem regions */
	msx1.empty = (UINT8*)auto_malloc (0x4000);
	msx1.ram = (UINT8*)auto_malloc (0x20000);
	for (i=0;i<4;i++) msx1.ramp[i] = 3 - i;

	if (!msx1.ram || !msx1.empty)
	{
		logerror("auto_malloc() in msx_ch_reset() failed!\n");
		return;
	}

	memset (msx1.empty, 0xff, 0x4000);
	memset (msx1.ram, 0, 0x20000);

	msx1.run = 1;
}

MACHINE_INIT( msx )
{
	TMS9928A_reset ();
	msx_ch_reset_core ();
}

MACHINE_INIT( msx2 )
{
	v9938_reset ();
	msx_ch_reset_core ();
}

/* z80 stuff */
static int z80_table_num[5] = { Z80_TABLE_op, Z80_TABLE_xy,
	Z80_TABLE_ed, Z80_TABLE_cb, Z80_TABLE_xycb };
static const UINT8 *old_z80_tables[5];
static UINT8 *z80_table;

static void msx_wd179x_int (int state);

DRIVER_INIT( msx )
{
    int i,n;

	wd179x_init (WD_TYPE_179X,msx_wd179x_int);
	wd179x_set_density (DEN_FM_HI);
	msx1.dsk_stat = 0x7f;

    /* adjust z80 cycles for the M1 wait state */
    z80_table = auto_malloc (0x500);

    if (!z80_table)
		logerror ("Cannot malloc z80 cycle table, using default values\n");
	else
	{
        for (i=0;i<5;i++)
		{
			old_z80_tables[i] = z80_msx_get_cycle_table (z80_table_num[i]);
			for (n=0;n<256;n++)
				z80_table[i*0x100+n] = old_z80_tables[i][n] + (i > 1 ? 2 : 1);
			z80_msx_set_cycle_table (i, z80_table + i*0x100);
		}
	}
}

static struct tc8521_interface tc = { NULL };

DRIVER_INIT( msx2 )
{
	init_msx ();
	tc8521_init (&tc);
}

MACHINE_STOP( msx )
{
	int i;
	for (i=0;i<5;i++)
		z80_msx_set_cycle_table (i, (void *) old_z80_tables[i]);
    msx1.run = 0;
}

INTERRUPT_GEN( msx2_interrupt )
{
	v9938_set_sprite_limit(readinputport (8) & 0x20);
	v9938_set_resolution(readinputport (8) & 0x03);
	v9938_interrupt();
}

INTERRUPT_GEN( msx_interrupt )
{
    int i;

    for (i=0;i<2;i++)
	{
        msx1.mouse[i] = readinputport (9+i);
        msx1.mouse_stat[i] = -1;
	}

    TMS9928A_set_spriteslimit (readinputport (8) & 0x20);
    TMS9928A_interrupt();
}

/*
** The I/O funtions
*/

READ_HANDLER ( msx_psg_r )
{
    return AY8910_read_port_0_r (offset);
}

WRITE_HANDLER ( msx_psg_w )
{
    if (offset & 0x01)
        AY8910_write_port_0_w (offset, data);
    else
        AY8910_control_port_0_w (offset, data);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

static mess_image *printer_image(void)
{
	return image_from_devtype_and_index(IO_PRINTER, 0);
}

READ_HANDLER ( msx_psg_port_a_r )
{
    int data, inp;

    data = (device_input(cassette_device_image()) > 255 ? 0x80 : 0);

    if ( (msx1.psg_b ^ readinputport (8) ) & 0x40)
		{
		/* game port 2 */
        inp = input_port_7_r (0) & 0x7f;
#if 0
		if ( !(inp & 0x80) )
			{
#endif
			/* joystick */
			return (inp & 0x7f) | data;
#if 0
			}
		else
			{
			/* mouse */
			data |= inp & 0x70;
			if (msx1.mouse_stat[1] < 0)
				inp = 0xf;
			else
				inp = ~(msx1.mouse[1] >> (4*msx1.mouse_stat[1]) ) & 15;

			return data | inp;
			}
#endif
		}
    else
		{
		/* game port 1 */
        inp = input_port_6_r (0) & 0x7f;
#if 0
		if ( !(inp & 0x80) )
			{
#endif
			/* joystick */
			return (inp & 0x7f) | data;
#if 0
			}
		else
			{
			/* mouse */
			data |= inp & 0x70;
			if (msx1.mouse_stat[0] < 0)
				inp = 0xf;
			else
				inp = ~(msx1.mouse[0] >> (4*msx1.mouse_stat[0]) ) & 15;

			return data | inp;
			}
#endif
		}

    return 0;
}

READ_HANDLER ( msx_psg_port_b_r )
{
    return msx1.psg_b;
}

WRITE_HANDLER ( msx_psg_port_a_w )
{

}

WRITE_HANDLER ( msx_psg_port_b_w )
{
    /* Arabic or kana mode led */
	if ( (data ^ msx1.psg_b) & 0x80)
		set_led_status (2, !(data & 0x80) );

    if ( (msx1.psg_b ^ data) & 0x10)
		{
		if (++msx1.mouse_stat[0] > 3) msx1.mouse_stat[0] = -1;
		}
    if ( (msx1.psg_b ^ data) & 0x20)
		{
		if (++msx1.mouse_stat[1] > 3) msx1.mouse_stat[1] = -1;
		}

    msx1.psg_b = data;
}

WRITE_HANDLER ( msx_printer_w )
{
	if (readinputport (8) & 0x80)
	{
	/* SIMPL emulation */
		if (offset == 1)
        	DAC_signed_data_w (0, data);
	}
	else
	{
   		if (offset == 1)
			msx1.prn_data = data;
		else
		{
			if ( (msx1.prn_strobe & 2) && !(data & 2) )
				device_output(printer_image(), msx1.prn_data);

			msx1.prn_strobe = data;
		}
	}
}

READ_HANDLER ( msx_printer_r )
{
	if (offset == 0 && ! (readinputport (8) & 0x80) &&
			device_status(printer_image(), 0) )
		return 253;

    return 0xff;
}

WRITE_HANDLER ( msx_fmpac_w )
{
    if (msx1.opll_active & 1)
    {
        if (offset == 1)
			YM2413_data_port_0_w (0, data);
        else
			YM2413_register_port_0_w (0, data);
    }
}

/*
** RTC functions
*/

WRITE_HANDLER (msx_rtc_latch_w)
{
	msx1.rtc_latch = data & 15;
}

WRITE_HANDLER (msx_rtc_reg_w)
{
	tc8521_w (msx1.rtc_latch, data);
}

READ_HANDLER (msx_rtc_reg_r)
{
	return tc8521_r (msx1.rtc_latch);
}

NVRAM_HANDLER( msx2 )
{
	if (file)
	{
		if (read_or_write)
			tc8521_save_stream (file);
		else
			tc8521_load_stream (file);
	}
}

/*
** The evil disk functions ...
*/

/*
From: erbo@xs4all.nl (erik de boer)

sony and philips have used (almost) the same design
and this is the memory layout
but it is not a msx standard !

WD1793 or wd2793 registers

adress

7FF8H read  status register
      write command register
7FF9H  r/w  track register (r/o on NMS 8245 and Sony)
7FFAH  r/w  sector register (r/o on NMS 8245 and Sony)
7FFBH  r/w  data register


hardware registers

adress

7FFCH r/w  bit 0 side select
7FFDH r/w  b7>M-on , b6>in-use , b1>ds1 , b0>ds0  (all neg. logic)
7FFEH         not used
7FFFH read b7>drq , b6>intrq

set on 7FFDH bit 2 always to 0 (some use it as disk change reset)

*/

static void msx_wd179x_int (int state)
	{
	switch (state)
		{
		case WD179X_IRQ_CLR: msx1.dsk_stat |= 0x40; break;
		case WD179X_IRQ_SET: msx1.dsk_stat &= ~0x40; break;
		case WD179X_DRQ_CLR: msx1.dsk_stat |= 0x80; break;
		case WD179X_DRQ_SET: msx1.dsk_stat &= ~0x80; break;
		}
	}

static READ_HANDLER (msx_disk_p1_r)
	{
	switch (offset)
		{
		case 0x1ff8: return wd179x_status_r (0);
		case 0x1ff9: return wd179x_track_r (0);
		case 0x1ffa: return wd179x_sector_r (0);
		case 0x1ffb: return wd179x_data_r (0);
		case 0x1fff: return msx1.dsk_stat;
		default: return msx1.disk[offset];
		}
	}

static READ_HANDLER (msx_disk_p2_r)
	{
	if (offset >= 0x1ff8)
		{
		switch (offset)
			{
			case 0x1ff8: return wd179x_status_r (0);
			case 0x1ff9: return wd179x_track_r (0);
			case 0x1ffa: return wd179x_sector_r (0);
			case 0x1ffb: return wd179x_data_r (0);
			case 0x1fff: return msx1.dsk_stat;
			default: return msx1.disk[offset];
			}
		}
	else
		return 0xff;
	}

static WRITE_HANDLER (msx_disk_w)
	{
	switch (offset)
		{
		case 0x1ff8:
			wd179x_command_w (0, data);
			break;
		case 0x1ff9:
			wd179x_track_w (0, data);
			break;
		case 0x1ffa:
			wd179x_sector_w (0, data);
			break;
		case 0x1ffb:
			wd179x_data_w (0, data);
			break;
		case 0x1ffc:
			wd179x_set_side (data & 1);
			msx1.disk[0x1ffc] = data | 0xfe;
			break;
		case 0x1ffd:
			wd179x_set_drive (data & 1);
			if ( (msx1.disk[0x1ffd] ^ data) & 2)
				set_led_status (0, !(data & 2) );
			msx1.disk[0x1ffd] = data | 0x7c;
			break;
		}
	}

DEVICE_LOAD( msx_floppy )
{
	int size, heads = 2;

	if (! image_has_been_created(image))
		{
		size = mame_fsize (file);

		switch (size)
			{
			case 360*1024:
				heads = 1;
			case 720*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image, file) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 80, heads, 9, 512, 1, 0, FALSE);

	return INIT_PASS;
}

/*
** The PPI functions
*/

static WRITE_HANDLER ( msx_ppi_port_a_w )
{
	msx_set_all_mem_banks ();
}

static WRITE_HANDLER ( msx_ppi_port_c_w )
{
	static int old_val = 0xff;

	/* caps lock */
	if ( (old_val ^ data) & 0x40)
		set_led_status (1, !(data & 0x40) );

	/* key click */
	if ( (old_val ^ data) & 0x80)
		DAC_signed_data_w (0, (data & 0x80 ? 0x7f : 0));

	/* cassette motor on/off */
	if ( (old_val ^ data) & 0x10)
		cassette_change_state(cassette_device_image(), (data & 0x10) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* cassette signal write */
	if ( (old_val ^ data) & 0x20)
		cassette_output(cassette_device_image(), (data & 0x20) ? -1.0 : 1.0);

	old_val = data;
}

static READ_HANDLER( msx_ppi_port_b_r )
	{
    int row, data;

    row = ppi8255_0_r (2) & 0x0f;
    if (row <= 10)
		{
		data = readinputport (row/2);
		if (row & 1) data >>= 8;
		return data & 0xff;
		}
    else return 0xff;
	}

/*
** The memory functions
*/
static void msx_set_slot_0 (int page)
{
    unsigned char *ROM;
    ROM = memory_region(REGION_CPU1);
    if (page < (strncmp (Machine->gamedrv->name, "msxkr", 5) ? 2 : 3) )
    {
        cpu_setbank (1 + page * 2, ROM + page * 0x4000);
        cpu_setbank (2 + page * 2, ROM + page * 0x4000 + 0x2000);
    } else {
        cpu_setbank (1 + page * 2, msx1.empty);
        cpu_setbank (2 + page * 2, msx1.empty);
    }
}

static void msx_set_slot_1 (int page) {
    int n;
    unsigned char *ROM;
    ROM = memory_region(REGION_CPU1);

    if (msx1.cart[0].type == 0 && msx1.cart[0].mem)
    {
        cpu_setbank (1 + page * 2, msx1.cart[0].mem + page * 0x4000);
        cpu_setbank (2 + page * 2, msx1.cart[0].mem + page * 0x4000 + 0x2000);
    } else {
        if (page == 0 || page == 3 || !msx1.cart[0].mem)
        {
    		if (!page && !strncmp (Machine->gamedrv->name, "msx2", 4) )
				{
		        cpu_setbank (1, ROM + 0x8000);
       			cpu_setbank (2, ROM + 0xa000);
				}
			else
				{
            	cpu_setbank (1 + page * 2, msx1.empty);
            	cpu_setbank (2 + page * 2, msx1.empty);
				}
            return;
        }
        n = (page - 1) * 2;
        cpu_setbank (3 + n, msx1.cart[0].mem + msx1.cart[0].banks[n] * 0x2000);
        cpu_setbank (4 + n, msx1.cart[0].mem + msx1.cart[0].banks[1 + n] * 0x2000);
		if (page == 1) {
			if (msx1.cart[0].type == 15) {
				memory_set_bankhandler_r (4, 0, msx_disk_p1_r);
				msx1.disk = msx1.cart[0].mem + 0x2000;
			}
		}
		if (page == 2) {
			if (msx1.cart[0].type == 15) {
				memory_set_bankhandler_r (6, 0, msx_disk_p2_r);
				msx1.disk = msx1.cart[0].mem + 0x2000;
			}
		}
    }
}

static void msx_set_slot_2 (int page)
{
    int n;

    if (msx1.cart[1].type == 0 && msx1.cart[1].mem)
    {
        cpu_setbank (1 + page * 2, msx1.cart[1].mem + page * 0x4000);
        cpu_setbank (2 + page * 2, msx1.cart[1].mem + page * 0x4000 + 0x2000);
    } else {
        if (page == 0 || page == 3 || !msx1.cart[1].mem)
        {
            cpu_setbank (1 + page * 2, msx1.empty);
            cpu_setbank (2 + page * 2, msx1.empty);
            return;
        }
        n = (page - 1) * 2;
        cpu_setbank (3 + n, msx1.cart[1].mem + msx1.cart[1].banks[n] * 0x2000);
        cpu_setbank (4 + n, msx1.cart[1].mem + msx1.cart[1].banks[1 + n] * 0x2000);
		if (page == 1) {
			if (msx1.cart[1].type == 15) {
				memory_set_bankhandler_r (4, 0, msx_disk_p1_r);
				msx1.disk = msx1.cart[1].mem + 0x2000;
			}
		}
		if (page == 2) {
			if (msx1.cart[1].type == 15) {
				memory_set_bankhandler_r (6, 0, msx_disk_p2_r);
				msx1.disk = msx1.cart[1].mem + 0x2000;
			}
		}
    }
}

static void msx_set_slot_3 (int page)
{
    cpu_setbank (1 + page * 2, msx1.ram + (7 - msx1.ramp[page]) * 0x4000);
    cpu_setbank (2 + page * 2, msx1.ram + (7 - msx1.ramp[page]) * 0x4000 + 0x2000);
}

static void (*msx_set_slot[])(int) = {
    msx_set_slot_0, msx_set_slot_1, msx_set_slot_2, msx_set_slot_3
};

static void msx_set_all_mem_banks (void)
{
    int i;

	memory_set_bankhandler_r (4, 0, MRA_BANK4);
	memory_set_bankhandler_r (6, 0, MRA_BANK6);

    for (i=0;i<4;i++)
        msx_set_slot[(ppi8255_peek (0,0)>>(i*2))&3](i);
}

static void msx_cart_write (int cart, int offset, int data);

WRITE_HANDLER ( msx_writemem0 )
	{
	if (!offset)
		{
		/*
         * Super Load Runner ignores the CS, it responds to any
         * write to address 0x0000 (!).
		 */

		if (msx1.cart[0].type == 12)
			msx_cart_write (0, -1, data);

		if (msx1.cart[1].type == 12)
			msx_cart_write (1, -1, data);
		}

    if ( (ppi8255_0_r(0) & 0x03) == 0x03 )
        msx1.ram[(7 - msx1.ramp[0]) * 0x4000 + offset] = data;
	}

static int msx_cart_page_2 (int cart)
{
    /* returns non-zero if `cart' is in page 2 */
    switch (ppi8255_0_r (0) & 0x30)
    {
    case 0x10: return (cart == 0);
    case 0x20: return (cart == 1);
    }
    return 0;
}

static void msx_cart_write (int cart, int offset, int data)
{
    int n,i;
    UINT8 *p;

    switch (msx1.cart[cart].type)
    {
    case 0:
		logerror ("Write %02x to %04x in cartridge slot #%d\n", data, offset,
			cart + 1);
        break;
    case 1: /* MSX-DOS 2 cartridge */
        if (offset == 0x2000)
        {
            n  = (data * 2) & 7;
            msx1.cart[cart].banks[0] = n;
            msx1.cart[cart].banks[1] = n + 1;
            cpu_setbank (3,msx1.cart[cart].mem + n * 0x2000);
            cpu_setbank (4,msx1.cart[cart].mem + (n + 1) * 0x2000);
        }
        break;
    case 2: /* Konami5 with SCC */
        if ( (offset & 0x1800) == 0x1000)
        {
            /* check if SCC should be activated */
            if ( ( (offset & 0x7800) == 0x5000) && !(~data & 0x3f) )
                n = msx1.cart[cart].bank_mask + 1;
            else
                n = data & msx1.cart[cart].bank_mask;
            msx1.cart[cart].banks[(offset/0x2000)] = n;
            cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem + n * 0x2000);
        }
        else if ( (msx1.cart[cart].banks[2] > msx1.cart[cart].bank_mask) &&
                (offset >= 0x5800) && (offset < 0x6000) )
        {
            offset &= 0xff;
            if (offset < 0x80)
				{
				K051649_waveform_w (offset, data);
                p = msx1.cart[cart].mem +
                    (msx1.cart[cart].bank_mask + 1) * 0x2000;
                for (n=0;n<8;n++) p[n*0x100+0x1800+(offset&0x7f)] = data;
				}
            else if (offset < 0x8a) K051649_frequency_w (offset - 0x80 , data);
            else if (offset < 0x8f) K051649_volume_w (offset - 0x8a, data);
            else if (offset == 0x8f) K051649_keyonoff_w (0, data);
        }
        /* quick sound cartridge hack */
        else if ( (offset >= 0x7800) && (offset < 0x8000) )
        {
            offset &= 0xff;
            if (offset < 0xa0) K052539_waveform_w (offset, data);
            else if (offset < 0xaa) K051649_frequency_w (offset - 0xa0 , data);
            else if (offset < 0xaf) K051649_volume_w (offset - 0xaa, data);
            else if (offset == 0xaf) K051649_keyonoff_w (0, data);
        }
        break;
    case 3: /* Konami4 without SCC */
        if (offset && !(offset & 0x1fff) )
        {
            n = data & msx1.cart[cart].bank_mask;
            msx1.cart[cart].banks[(offset/0x2000)] = n;
            cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem + n * 0x2000);
        }
        break;
    case 4: /* ASCII 8kB */
        if ( (offset >= 0x2000) && (offset < 0x4000) )
        {
            offset -= 0x2000;
            n = data & msx1.cart[cart].bank_mask;
            msx1.cart[cart].banks[(offset/0x800)] = n;
            if ((offset/0x800) < 2 || msx_cart_page_2 (cart) )
                cpu_setbank (3+(offset/0x800),msx1.cart[cart].mem + n * 0x2000);
        }
   case 16: /* Korean 80-in-1 */
        if ( (offset < 4) )
        {
            n = data & msx1.cart[cart].bank_mask;
            msx1.cart[cart].banks[offset] = n;
            if (offset < 2 || msx_cart_page_2 (cart) )
                cpu_setbank (3+offset,msx1.cart[cart].mem + n * 0x2000);
        }
        break;
        break;
    case 12: /* Super Load Runner */
		if (offset == -1)
			{
            n = (data * 2) & msx1.cart[cart].bank_mask;

            /* page 2 */
            msx1.cart[cart].banks[2] = n;
            msx1.cart[cart].banks[3] = n + 1;
            if (msx_cart_page_2 (cart))
                {
                cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
                }
			}

		break;
    case 5: /* ASCII 16kB */
        if ( (offset & 0x6800) == 0x2000)
        {
            n = (data * 2) & msx1.cart[cart].bank_mask;

            if (offset & 0x1000)
            {
                /* page 2 */
                msx1.cart[cart].banks[2] = n;
                msx1.cart[cart].banks[3] = n + 1;
                if (msx_cart_page_2 (cart))
                {
                    cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                    cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
                }
            } else {
                /* page 1 */
                msx1.cart[cart].banks[0] = n;
                msx1.cart[cart].banks[1] = n + 1;
                cpu_setbank (3,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (4,msx1.cart[cart].mem + (n + 1) * 0x2000);
            }
        }
        break;
   case 17: /* 126-in-1 */
        if (offset < 2)
        {
            n = (data * 2) & msx1.cart[cart].bank_mask;

            if (offset)
            {
                /* page 2 */
                msx1.cart[cart].banks[2] = n;
                msx1.cart[cart].banks[3] = n + 1;
                if (msx_cart_page_2 (cart))
                {
                    cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                    cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
                }
            } else {
                /* page 1 */
                msx1.cart[cart].banks[0] = n;
                msx1.cart[cart].banks[1] = n + 1;
                cpu_setbank (3,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (4,msx1.cart[cart].mem + (n + 1) * 0x2000);
            }
        }
        break;
    case 6: /* Game Master 2 */
        if (!(offset & 0x1000) && (offset >= 0x2000) )
        {
            n = ((data & 0x10) ? ((data & 0x20) ? 0x11:0x10) : (data & 0x0f));
            msx1.cart[cart].banks[(offset/0x2000)] = n;
            cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem+n*0x2000);
        }
        else if (offset >= 0x7000)
        {
            switch (msx1.cart[cart].banks[3])
            {
            case 0x10:
                msx1.cart[cart].mem[0x20000+(offset&0x0fff)] = data;
                msx1.cart[cart].mem[0x21000+(offset&0x0fff)] = data;
                break;
            case 0x11:
                msx1.cart[cart].mem[0x22000+(offset&0x0fff)] = data;
                msx1.cart[cart].mem[0x23000+(offset&0x0fff)] = data;
                break;
            }
        }
        break;
    case 7: /* ASCII 8kB/SRAM */
        if ( (offset >= 0x2000) && (offset < 0x4000) )
        {
            offset -= 0x2000;
            if (data > msx1.cart[cart].bank_mask)
                n = msx1.cart[cart].bank_mask + 1;
            else
                n = data;
            msx1.cart[cart].banks[(offset/0x800)] = n;
            if ((offset/0x800) < 2 || msx_cart_page_2 (cart) )
                cpu_setbank (3+(offset/0x800),msx1.cart[cart].mem + n * 0x2000);
        }
        else if (offset >= 0x4000)
        {
            n = (offset >= 0x6000 ? 1 : 0);
            if (msx1.cart[cart].banks[2+n] > msx1.cart[cart].bank_mask)
                msx1.cart[cart].mem[(offset&0x1fff)+
                    (msx1.cart[cart].bank_mask+1)*0x2000] = data;
        }
        break;
	case 14: /* Cross Blaim */
		if (offset == 0x0045)
			{
            n = (data * 2) & msx1.cart[cart].bank_mask;

            /* page 2 */
            msx1.cart[cart].banks[2] = n;
            msx1.cart[cart].banks[3] = n + 1;
            if (msx_cart_page_2 (cart) )
	            {
                cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
                }
			}
		break;
    case 8: /* ASCII 16kB */
        if ( (offset & 0x6800) == 0x2000)
        {
            if (data > (msx1.cart[cart].bank_mask/2))
                n = msx1.cart[cart].bank_mask + 1;
            else
                n = (data * 2) & msx1.cart[cart].bank_mask;

            if (offset & 0x1000)
            {
                /* page 2 */
                msx1.cart[cart].banks[2] = n;
                msx1.cart[cart].banks[3] = n + 1;
                if (msx_cart_page_2 (cart) )
                {
                    cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                    cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
                }
            } else {
                /* page 1 */
                msx1.cart[cart].banks[0] = n;
                msx1.cart[cart].banks[1] = n + 1;
                cpu_setbank (3,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (4,msx1.cart[cart].mem + (n + 1) * 0x2000);
            }
        }
        else if (offset >= 0x4000 &&
            msx1.cart[cart].banks[2] > msx1.cart[cart].bank_mask)
        {
            for (i=0;i<8;i++)
                msx1.cart[cart].mem[i*0x800+(offset&0x7ff)+
                    (msx1.cart[cart].bank_mask+1)*0x2000] = data;
        }
        break;
    case 9: /* R-Type */
        if (offset >= 0x3000 && offset < 0x4000)
        {
            if (data & 0x10)
            {
                n = (( (data & 0x07) | 0x10) * 2) & msx1.cart[cart].bank_mask;
            } else {
                n = ((data & 0x0f) * 2) & msx1.cart[cart].bank_mask;
            }

            msx1.cart[cart].banks[2] = n;
            msx1.cart[cart].banks[3] = n + 1;
            if (msx_cart_page_2 (cart))
            {
                cpu_setbank (5,msx1.cart[cart].mem + n * 0x2000);
                cpu_setbank (6,msx1.cart[cart].mem + (n + 1) * 0x2000);
            }
        }
        break;
    case 10: /* Konami majutushi */
        if (offset >= 0x1000 && offset < 0x2000)
			DAC_data_w (0, data);
        else if (offset >= 0x2000)
        {
            n = data & msx1.cart[cart].bank_mask;
            msx1.cart[cart].banks[(offset/0x2000)] = n;
            cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem + n * 0x2000);
        }
        break;
    case 11: /* FM-PAC */
        if (offset < 0x1ffe && msx1.cart[cart].pacsram)
        {
            if (msx1.cart[cart].banks[1] > 7)
                msx1.cart[cart].mem[0x10000 + offset] = data;
            break;
        }
        if (offset == 0x3ff4 && msx1.opll_active)
        {
            YM2413_register_port_0_w (0, data);
            break;
        }
        if (offset == 0x3ff5 && msx1.opll_active)
        {
            YM2413_data_port_0_w (0, data);
            break;
        }
        if (offset == 0x3ff6)
        {
            n = data & 0x11;
            msx1.cart[cart].mem[0x3ff6] = n;
            msx1.cart[cart].mem[0x7ff6] = n;
            msx1.cart[cart].mem[0xbff6] = n;
            msx1.cart[cart].mem[0xfff6] = n;
            msx1.cart[cart].mem[0x13ff6] = n;
            msx1.opll_active = data & 1;
            logerror("FM-PAC: OPLL %s\n",(data & 1 ? "activated" : "deactivated"));
            break;
        }
        if ( (offset == 0x1ffe || offset == 0x1fff) && msx1.cart[cart].pacsram)
        {
            msx1.cart[cart].mem[0x10000 + offset] = data;
            if (msx1.cart[cart].mem[0x11ffe] == 0x4d &&
                msx1.cart[cart].mem[0x11fff] == 0x69)
                n = 8;
            else
                n = msx1.cart[cart].mem[0x13ff7] * 2;
        }
        else
        {
            if (offset == 0x3ff7)
            {
                msx1.cart[cart].mem[0x13ff7] = data & 3;
                if (msx1.cart[cart].banks[1] > 7) break;
                n = ((data & 3) * 2) & msx1.cart[cart].bank_mask;
            } else break;
        }
        msx1.cart[cart].banks[0] = n;
        msx1.cart[cart].banks[1] = n + 1;
        cpu_setbank (3,msx1.cart[cart].mem + n * 0x2000);
        cpu_setbank (4,msx1.cart[cart].mem + (n + 1) * 0x2000);
        break;
    case 13: /* Konami Synthesizer */
		if ( (offset < 0x4000) && !(offset & 0x0010) )
			DAC_data_w (0, data);
        break;
	case 15: /* disk rom */
		if ( (offset >= 0x2000) && (offset < 0x4000) )
			msx_disk_w (offset - 0x2000, data);
		else if ( (offset >= 0x6000) && (offset < 0x8000) )
			msx_disk_w (offset - 0x6000, data);
		break;
    }
}

WRITE_HANDLER ( msx_writemem1 )
{
    switch (ppi8255_0_r(0) & 0x0c)
    {
    case 0x04:
        msx_cart_write (0, offset, data);
        break;
    case 0x08:
        msx_cart_write (1, offset, data);
        break;
    case 0x0c:
        msx1.ram[(7 - msx1.ramp[1]) * 0x4000 + offset] = data;
    }
}

WRITE_HANDLER ( msx_writemem2 )
{
    switch (ppi8255_0_r(0) & 0x30)
    {
    case 0x10:
        msx_cart_write (0, 0x4000 + offset, data);
        break;
    case 0x20:
        msx_cart_write (1, 0x4000 + offset, data);
        break;
    case 0x30:
        msx1.ram[(7 - msx1.ramp[2]) * 0x4000 + offset] = data;
    }
}

WRITE_HANDLER ( msx_writemem3 )
{
    if ( (ppi8255_0_r(0) & 0xc0) == 0xc0)
        msx1.ram[(7 - msx1.ramp[7]) * 0x4000 + offset] = data;
}

WRITE_HANDLER (msx_mapper_w)
{
	msx1.ramp[offset] = data & 7;
	msx_set_all_mem_banks ();
}

READ_HANDLER (msx_mapper_r)
{
	return msx1.ramp[offset];
}

