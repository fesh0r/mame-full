/*
** msx.c : MSX1 emulation
**
** Todo:
**
** - memory emulation needs be rewritten
** - add support for serial ports
** - fix mouse support
** - add support for SCC+ and megaRAM
** - add support for diskdrives
**
** Sean Young
*/

#include "driver.h"
#include "includes/msx.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "sndhrdw/scc.h"
#include "formats/fmsx_cas.h"
#include "printer.h"

MSX msx1;
static void msx_set_all_mem_banks (void);
static void msx_ppi_port_a_w (int chip, int data);
static void msx_ppi_port_c_w (int chip, int data);
static int msx_ppi_port_b_r (int chip);
static ppi8255_interface msx_ppi8255_interface = {
    1,
    NULL,
    msx_ppi_port_b_r,
    NULL,
    msx_ppi_port_a_w, 
    NULL,
    msx_ppi_port_c_w
};

static char PAC_HEADER[] = "PAC2 BACKUP DATA";
#define PAC_HEADER_LEN (16)

int msx_id_rom (int id)
{
    FILE *F;
    unsigned char magic[2];

    /* read the first two bytes */
    F = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
    if (!F) return 0;
    osd_fread (F, magic, 2);
    osd_fclose (F);

    /* first to bytes must be 'AB' */
    return ( (magic[0] == 'A') && (magic[1] == 'B') );
}

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

#define MAX(x, y) ((x) < (y) ? (y) : (x) )

    if (MAX (kon4, kon5) > MAX (asc8, asc16) )
        return (kon5 > kon4) ? 2 : 3;
    else
        return (asc8 > asc16) ? 4 : 5;
}

int msx_load_rom (int id)
{
    void *F;
    UINT8 *pmem,*m;
    int size,size_aligned,n,p,type,i;
    char *pext, buf[PAC_HEADER_LEN + 2];
    static char *mapper_types[] = { "none", "MSX-DOS 2", "konami5 with SCC",
        "konami4 without SCC", "ASCII/8kB", "ASCII//16kB",
        "Konami Game Master 2", "ASCII/8kB with 8kB SRAM",
        "ASCII/16kB with 2kB SRAM", "R-Type", "Konami Majutsushi",
        "Panasonic FM-PAC", "ASCII/16kB (bogus; use type 5)",
        "Konami Synthesizer" };

    /* try to load it */
    F = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
    if (!F) return 1;
    size = osd_fsize (F);
    if (size < 0x2000)
    {
        logerror("%s: file to small\n",
            device_filename (IO_CARTSLOT, id));
        osd_fclose (F);
        return 1;
    }
    /* get mapper type */
    pext = (char *)device_extrainfo (IO_CARTSLOT, id);
	if (!pext || (1 != sscanf (pext, "%d", &type) ) )
   		{
       	logerror("Cart #%d No extra info found in crc file\n", id);
       	type = -1;
   		}
   	else
   		{
       	if (type < 0 || type > 13)
       		{
           	logerror("Cart #%d Invalid extra info\n", id);
           	type = -1;
       		}
       	else logerror("Cart %d extra info: %s\n", id, pext);
   		}

    /* calculate aligned size (8, 16, 32, 64, 128, 256, etc. (kB) ) */
    size_aligned = 0x2000;
    while (size_aligned < size) size_aligned *= 2;

    pmem = (UINT8*)malloc (size_aligned);
    if (!pmem)
    {
        logerror("malloc () failed\n");
        osd_fclose (F);
        return 1;
    }
    memset (pmem, 0xff, size_aligned);
    if (osd_fread (F, pmem, size) != size)
    {
        logerror("%s: can't read file\n",
            device_filename (IO_CARTSLOT, id));
        osd_fclose (F);
        free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
        return 1;
    }
    osd_fclose (F);
    /* check type */
    if (type < 0)
    {
        type = msx_probe_type (pmem, size);

        if ( !( (pmem[0] == 'A') && (pmem[1] == 'B') ) )
        {
            logerror("%s: May not be a valid ROM file\n",device_filename (IO_CARTSLOT, id) );
        }

        logerror("Probed cartridge mapper %s\n", mapper_types[type]);
    }

    /* mapper type 0 always needs 64kB */
    if (!type)
    {
        size_aligned = 0x10000;
        pmem = realloc (pmem, 0x10000);
        if (!pmem)
        {
            logerror("Realloc failed!\n");
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        if (size < 0x10000) memset (pmem + size, 0xff, 0x10000 - size);
        if (size > 0x10000) size = 0x10000;
    }

    /* set mapper specific stuff */
    msx1.cart[id].mem = pmem;
    msx1.cart[id].type = type;
    msx1.cart[id].bank_mask = (size_aligned / 0x2000) - 1;
    for (i=0;i<4;i++) msx1.cart[id].banks[i] = (i & msx1.cart[id].bank_mask);
    logerror("Cart #%d size %d, mask %d, type: %s\n",id, size, msx1.cart[id].bank_mask, mapper_types[type]);
    /* set filename for sram (memcard) */
    msx1.cart[id].sramfile = malloc (strlen (device_filename
        (IO_CARTSLOT, id)) + 1);
    if (!msx1.cart[id].sramfile)
    {
        logerror("malloc () failed\n");
        free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
        return 1;
    }
    strcpy (msx1.cart[id].sramfile, device_filename (IO_CARTSLOT, id) );
    pext = strrchr (msx1.cart[id].sramfile, '.');
    if (pext) *pext = 0;
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
        for (n=2;n<=8;n+=2)
        {
            if (pmem[n] || pmem[n+1])
            {
                /* this hack works on all byte order systems */
                p = pmem[n+1] / 0x40;
                break;
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
        else if (size <= 0xc000)
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
        pmem = realloc (msx1.cart[id].mem, 0x12000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        msx1.cart[id].mem = pmem;
        msx1.cart[id].banks[2] = 8;
        msx1.cart[id].banks[3] = 8;
        break;
   case 6: /* game master 2; try to load sram */
        pmem = realloc (msx1.cart[id].mem, 0x24000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
		
		i = 0;
        F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                OSD_FILETYPE_MEMCARD, 0);
		if (F)
			{
			n = osd_fsize (F);
			if (n == 0x2000)
				{
	        	if (osd_fread (F, pmem + 0x21000, 0x2000) == 0x2000) 
	   		    	{
   	         		memcpy (pmem + 0x20000, pmem + 0x21000, 0x1000);
            		memcpy (pmem + 0x23000, pmem + 0x22000, 0x1000);
            		i = 1;
					}
				}

			/* if it's an Virtual MSX Game Master 2 file, convert */
			if (n == 0x4000)
				{
	        	if (osd_fread (F, pmem + 0x20000, 0x4000) == 0x4000) 
	   		    	{
   	         		memcpy (pmem + 0x20000, pmem + 0x21000, 0x1000);
            		memcpy (pmem + 0x22000, pmem + 0x23000, 0x1000);
            		i = 1;
					}
				}
			}

        if (F) osd_fclose (F);

		if (i) logerror ("Cart #%d SRAM loaded\n", id);
		else
			{
            memset (pmem + 0x20000, 0, 0x4000);
            logerror("Cart #%d Failed to load SRAM\n", id);
        	}

        msx1.cart[id].mem = pmem;
        break;
    case 2: /* Konami SCC */
        /* we want an extra page that looks like the SCC page */
        pmem = realloc (pmem, size_aligned + 0x2000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        memcpy (pmem + size_aligned, pmem + size_aligned - 0x2000, 0x1800);
        for (i=0;i<8;i++)
        {
            memset (pmem + size_aligned + i * 0x100 + 0x1800, 0, 0x80);
            memset (pmem + size_aligned + i * 0x100 + 0x1880, 0xff, 0x80);
        }
        msx1.cart[id].mem = pmem;
        break;
   case 7: /* ASCII/8kB with SRAM */
        pmem = realloc (msx1.cart[id].mem, size_aligned + 0x2000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                OSD_FILETYPE_MEMCARD, 0);
        if (F && (osd_fread (F, pmem + size_aligned, 0x2000) == 0x2000) )
        {
            logerror("Cart #%d SRAM loaded\n", id);
        } else {
            memset (pmem + size_aligned, 0, 0x2000);
            logerror("Cart #%d Failed to load SRAM\n", id);
        }
        if (F) osd_fclose (F);

        msx1.cart[id].mem = pmem;
        break;
   case 8: /* ASCII/16kB with SRAM */
        pmem = realloc (msx1.cart[id].mem, size_aligned + 0x4000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                OSD_FILETYPE_MEMCARD, 0);
        if (F && (osd_fread (F, pmem + size_aligned, 0x2000) == 0x2000) )
        {
            for (i=1;i<8;i++)
            {
                memcpy (pmem + size_aligned + i * 0x800,
                    pmem + size_aligned, 0x800);
            }
            logerror("Cart #%d SRAM loaded\n", id);
        } else {
            memset (pmem + size_aligned, 0, 0x4000);
            logerror("Cart #%d Failed to load SRAM\n", id);
        }
        if (F) osd_fclose (F);

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
        pmem = realloc (msx1.cart[id].mem, 0x18000);
        if (!pmem)
        {
            free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
            return 1;
        }
        memset (pmem + size_aligned, 0xff, 0x18000 - size_aligned);
        pmem[0x13ff6] = 0;
        pmem[0x13ff7] = 0;
        if (msx1.cart[id].pacsram)
        {
            F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                OSD_FILETYPE_MEMCARD, 0);
            if (F &&
                (osd_fread (F, buf, PAC_HEADER_LEN) == PAC_HEADER_LEN) &&
                !strncmp (buf, PAC_HEADER, PAC_HEADER_LEN) &&
                (osd_fread (F, pmem + 0x10000, 0x1ffe) == 0x1ffe) )
            {
               logerror("Cart #%d SRAM loaded\n", id);
            } else {
               memset (pmem + 0x10000, 0, 0x2000);
               logerror("Cart #%d Failed to load SRAM\n", id);
            }
            if (F) osd_fclose (F);
        }
        msx1.cart[id].banks[2] = (0x14000/0x2000);
        msx1.cart[id].banks[3] = (0x16000/0x2000);
        msx1.cart[id].mem = pmem;
        break;
    case 5: /* ASCII 16kb */
    case 12: /* Gall Force */
        msx1.cart[id].banks[0] = 0;
        msx1.cart[id].banks[1] = 1;
        msx1.cart[id].banks[2] = 0;
        msx1.cart[id].banks[3] = 1;
        break;
    }
    if (msx1.run) msx_set_all_mem_banks ();
    return 0;
}

static int save_sram (int id, char *filename, UINT8* pmem, int size)
{
    void *F;
    int res;

    F = osd_fopen (Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, 1);
    res = F && (osd_fwrite (F, pmem, size) == size);
    if (F) osd_fclose (F);
    return res;
}

void msx_exit_rom (int id)
{
    void *F;
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
            F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
                OSD_FILETYPE_MEMCARD, 1);
            if (!F) break;
            size = strlen (PAC_HEADER);
            if (osd_fwrite (F, PAC_HEADER, size) != size)
                { osd_fclose (F); break; }
            if (osd_fwrite (F, msx1.cart[id].mem + 0x10000, 0x1ffe) != 0x1ffe)
                { osd_fclose (F); break; }
            osd_fclose (F);
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
        free (msx1.cart[id].mem);
        free (msx1.cart[id].sramfile);
    }
}

static void msx_vdp_interrupt(int i) {
    cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

void msx_ch_reset(void) {
    TMS9928A_reset ();
    SCCResetChip (0);
    /* set interrupt stuff */
    cpu_irq_line_vector_w(0,0,0xff);
    /* setup PPI */
    ppi8255_init (&msx_ppi8255_interface);

    /* initialize mem regions */
    if (!msx1.empty || !msx1.ram)
    {
        msx1.empty = (UINT8*)malloc (0x4000);
        msx1.ram = (UINT8*)malloc (0x10000);
        if (!msx1.ram || !msx1.empty)
        {
            logerror("malloc () in msx_ch_reset () failed!\n");
            return;
        }

        memset (msx1.empty, 0xff, 0x4000);
        memset (msx1.ram, 0, 0x10000);
    }
    msx1.run = 1;

    return;
}

/* z80 stuff */
static int z80_table_num[5] = { Z80_TABLE_op, Z80_TABLE_xy, 
	Z80_TABLE_ed, Z80_TABLE_cb, Z80_TABLE_xycb };
static UINT8 *old_z80_tables[5], *z80_table;

void init_msx (void)
    {
    int i,n;

    /* this function is called at a very early stage, and not after a reset. */
    TMS9928A_int_callback(msx_vdp_interrupt);
	
    /* adjust z80 cycles for the M1 wait state */
    z80_table = malloc (0x500);
    if (!z80_table)
		logerror ("Cannot malloc z80 cycle table, using default values\n");
	else
		{
        for (i=0;i<5;i++)
			{
			old_z80_tables[i] = z80_get_cycle_table (z80_table_num[i]);
			for (n=0;n<256;n++)
				{
				z80_table[i*0x100+n] = old_z80_tables[i][n] + (i > 1 ? 2 : 1);
				}
			z80_set_cycle_table (i, z80_table + i*0x100);
			}	
		}
    }

void msx_ch_stop (void) 
	{
	int i;

    free (msx1.empty); msx1.empty = NULL;
    free (msx1.ram); msx1.ram = NULL;
	if (z80_table)
		{
		for (i=0;i<5;i++)
			z80_set_cycle_table (i, old_z80_tables[i]);
			
		free (z80_table);
		}
    msx1.run = 0;
	}

int msx_interrupt()
	{
    int i;

    for (i=0;i<2;i++)
        {
        msx1.mouse[i] = readinputport (12+i);
        msx1.mouse_stat[i] = -1;
        }

    TMS9928A_set_spriteslimit (readinputport (11) & 0x20);
    TMS9928A_interrupt();

    return ignore_interrupt();
	}

/*
** The I/O funtions
*/

READ_HANDLER ( msx_vdp_r )
{
    if (offset & 0x01)
        return TMS9928A_register_r();
    else
        return TMS9928A_vram_r();
}

WRITE_HANDLER ( msx_vdp_w )
{
    if (offset & 0x01)
        TMS9928A_register_w(data);
    else
        TMS9928A_vram_w(data);
}

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

READ_HANDLER ( msx_psg_port_a_r )
{
    int data, inp;

    data = (device_input (IO_CASSETTE, 0) > 255 ? 0x80 : 0);

    if ( (msx1.psg_b ^ readinputport (11) ) & 0x40)
		{
		/* game port 2 */
        inp = input_port_10_r (0) & 0x7f;
		if ( !(inp & 0x80) )
			{
			/* joystick */
			return (inp & 0x7f) | data;
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
		}
    else
		{
		/* game port 1 */
        inp = input_port_9_r (0) & 0x7f;
		if ( !(inp & 0x80) )
			{
			/* joystick */
			return (inp & 0x7f) | data;
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
	if ( (data ^ msx1.psg_b) & 0x80) set_led_status (1, !(data & 0x80) );

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
	if (readinputport (11) & 0x80)
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
				device_output (IO_PRINTER, 0, msx1.prn_data);
	
			msx1.prn_strobe = data;
			}
		}
	}

READ_HANDLER ( msx_printer_r )
	{
	if (offset == 0 && ! (readinputport (11) & 0x80) && 
		device_status (IO_PRINTER, 0, 0) )
		return 253;

    return 0xff;
	}

WRITE_HANDLER ( msx_fmpac_w )
{
    if (msx1.opll_active & 1)
    {
        if (offset == 1) YM2413_data_port_0_w (0, data);
        else YM2413_register_port_0_w (0, data);
    }
}

/*
** The PPI functions
*/

static void msx_ppi_port_a_w (int chip, int data)
{
    msx_set_all_mem_banks ();
}

static void msx_ppi_port_c_w (int chip, int data)
{
    static int old_val = 0xff;

    /* caps lock */
    if ( (old_val ^ data) & 0x40)
		set_led_status (0, !(data & 0x40) );
    /* key click */
    if ( (old_val ^ data) & 0x80)
        DAC_signed_data_w (0, (data & 0x80 ? 0x7f : 0));
    /* cassette motor on/off */
    if ( (old_val ^ data) & 0x10)
        device_status (IO_CASSETTE, 0, (data & 0x10) ? 0 : 1);
    /* cassette signal write */
    if ( (old_val ^ data) & 0x20)
        device_output (IO_CASSETTE, 0, (data & 0x20) ? -32768 : 32767);

    old_val = data;
}

static int msx_ppi_port_b_r (int chip)
{
    int row;

    row = ppi8255_0_r (2) & 0x0f;
    if (row <= 8) return readinputport (row);
    else return 0xff;
}


/*
** The memory functions
*/
static void msx_set_slot_0 (int page)
{
    unsigned char *ROM;
    ROM = memory_region(REGION_CPU1);
    if (page < (strcmp (Machine->gamedrv->name, "msxkr") ? 2 : 3) )
    {
        cpu_setbank (1 + page * 2, ROM + page * 0x4000);
        cpu_setbank (2 + page * 2, ROM + page * 0x4000 + 0x2000);
    } else {
        cpu_setbank (1 + page * 2, msx1.empty);
        cpu_setbank (2 + page * 2, msx1.empty);
    }
}

static void msx_set_slot_1 (int page) {
    int i,n;

    if (msx1.cart[0].type == 0 && msx1.cart[0].mem)
    {
        cpu_setbank (1 + page * 2, msx1.cart[0].mem + page * 0x4000);
        cpu_setbank (2 + page * 2, msx1.cart[0].mem + page * 0x4000 + 0x2000);
    } else {
        if (page == 0 || page == 3 || !msx1.cart[0].mem)
        {
            cpu_setbank (1 + page * 2, msx1.empty);
            cpu_setbank (2 + page * 2, msx1.empty);
            return;
        }
        n = (page - 1) * 2;
        for (i=0;i<2;i++)
        {
            cpu_setbank (3 + i + n,
            msx1.cart[0].mem + msx1.cart[0].banks[i + n] * 0x2000);
        }
    }
}

static void msx_set_slot_2 (int page)
{
    int i,n;

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
        for (i=0;i<2;i++)
        {
            cpu_setbank (3 + i + n,
            msx1.cart[1].mem + msx1.cart[1].banks[i + n] * 0x2000);
        }
    }
}

static void msx_set_slot_3 (int page)
{
    cpu_setbank (1 + page * 2, msx1.ram + page * 0x4000);
    cpu_setbank (2 + page * 2, msx1.ram + page * 0x4000 + 0x2000);
}

static void (*msx_set_slot[])(int) = {
    msx_set_slot_0, msx_set_slot_1, msx_set_slot_2, msx_set_slot_3
};

static void msx_set_all_mem_banks (void)
{
    int i;

    for (i=0;i<4;i++)
        msx_set_slot[(ppi8255_0_r(0)>>(i*2))&3](i);
}

WRITE_HANDLER ( msx_writemem0 )
{
     if ( (ppi8255_0_r(0) & 0x03) == 0x03 )
        msx1.ram[offset] = data;
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
            SCCWriteReg (0, offset & 0xff, data, SCC_MEGAROM);
            if (! (offset & 0x80) )
            {
                p = msx1.cart[cart].mem +
                    (msx1.cart[cart].bank_mask + 1) * 0x2000;
                for (n=0;n<8;n++) p[n*0x100+0x1800+(offset&0x7f)] = data;
            }
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
        break;
    case 12: /* Gall Force */
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
        if (!offset) DAC_data_w (0, data);
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
        msx1.ram[0x4000+offset] = data;
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
        msx1.ram[0x8000+offset] = data;
    }
}

WRITE_HANDLER ( msx_writemem3 )
{
    if ( (ppi8255_0_r(0) & 0xc0) == 0xc0)
        msx1.ram[0xc000+offset] = data;
}

/*
** Cassette functions
*/

static INT16* cas_samples;
static int cas_len;

int msx_cassette_fill_wave (INT16* samples, int wavlen, UINT8* casdata)
{
	if (casdata == CODE_HEADER || casdata == CODE_TRAILER)
		return 0;

	if (wavlen < cas_len)
	{
		logerror ("Not enough space to store converted cas file!\n");
		return 0;
	}

	memcpy (samples, cas_samples, cas_len * 2);

	return cas_len;
}

static int check_fmsx_cas (void *f)
{
	UINT8* casdata;
	int caslen, ret;

    caslen = osd_fsize (f);
	if (caslen < 9) return -1;
	
    casdata = (UINT8*)malloc (caslen);
    if (!casdata)
	{
       	logerror ("cas2wav: out of memory!\n");
       	return -1;
   	}

    osd_fseek (f, 0, SEEK_SET);
 	if (caslen != osd_fread (f, casdata, caslen) ) return -1;
   	osd_fseek (f, 0, SEEK_SET);

    ret = fmsx_cas_to_wav (casdata, caslen, &cas_samples, &cas_len);
    if (ret == 2)
	logerror ("cas2wav: out of memory\n");
    else if (ret)
	logerror ("cas2wav: conversion error\n");

    free (casdata);

    return ret;
}

int msx_cassette_init(int id)
{
    void *file;
	int ret;

    file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
        struct wave_args wa = {0,};
        wa.file = file;
        wa.display = 1;
		/* for cas files */
		cas_samples = NULL;
		cas_len = -1;
		if (!check_fmsx_cas (file) )
		{
			wa.smpfreq = 22050;
			wa.fill_wave = msx_cassette_fill_wave;
			wa.header_samples = cas_len;
			wa.trailer_samples = 0;
			wa.chunk_size = cas_len;
			wa.chunk_samples = 0;
		}
        ret = device_open(IO_CASSETTE,id,0,&wa);
		free (cas_samples);
		cas_samples = NULL;
		cas_len = -1;

		return (ret ? INIT_FAILED : INIT_OK);
    }
    file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW,
        OSD_FOPEN_RW_CREATE);
    if( file )
    {
        struct wave_args wa = {0,};
        wa.file = file;
        wa.display = 1;
        wa.smpfreq = 44100;
        if( device_open(IO_CASSETTE,id,1,&wa) )
            return INIT_FAILED;
        return INIT_OK;
    }
    return INIT_FAILED;
}

void msx_cassette_exit(int id)
{
    device_close(IO_CASSETTE,id);
}

