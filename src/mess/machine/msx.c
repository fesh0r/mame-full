/***************************************************************************

  msx.c

  Machine file to handle emulation of the MSX1.

  TODO:
	- Clean up the code
***************************************************************************/

#include "driver.h"
#include "msx.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "mess/vidhrdw/tms9928a.h"
#include "mess/sndhrdw/scc.h"

MSX msx1;
static void msx_set_all_mem_banks (void);
static void msx_ppi_port_a_w (int, int);
static int msx_ppi_port_b_r (int);
static void msx_ppi_port_c_w (int, int);
static ppi8255_interface msx_ppi8255_interface = {
    1,
    NULL,
    msx_ppi_port_b_r,
    NULL,
    msx_ppi_port_a_w,
    NULL,
    msx_ppi_port_c_w
};

int msx_id_rom (const char *name, const char *gamename)
{
    FILE *F;
    unsigned char magic[2];

    /* read the first two bytes */
    F = osd_fopen (name, gamename, OSD_FILETYPE_IMAGE_R, 0);
    if (!F) return 0;
    osd_fread (F, magic, 2);
    osd_fclose (F);

    /* first to bytes must be 'AB' */
    return ( (magic[0] == 'A') && (magic[1] == 'B') );
}

static int get_mapper_type (int crc)
{
/* temp hack since crc is disabled */
static struct {
    int crc, mapper;
    }
MSX_CRCs[] =
{
#include "msx-crcs.h"
  { 0, 0 }
};
    int i=0;

    while (MSX_CRCs[i].crc) {
	if (MSX_CRCs[i].crc == crc) return MSX_CRCs[i].mapper;
	i++;
    }
    return 0;
}

int msx_load_rom (int id, const char *name)
{
    void *F;
    UINT8 *pmem,*m;
    int size,size_aligned,n,p,type,i;
    char *pext;
    static char *mapper_types[] = { "none", "MSX-DOS 2", "konami5 with SCC",
	"konami4 without SCC", "ASCII/8kB", "ASCII//16kB",
	"Konami Game Master 2", "ASCII/8kB with 8kB SRAM",
        "ASCII/16kB with 2kB SRAM", "R-Type", "Konami Majutsushi",
	"Panasonic FM-PAC" };

    if (!name || !name[0]) return 0;

    /* try to load it */
    F = osd_fopen (Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
    if (!F) return 1;
    size = osd_fsize (F);
    if (size < 0x2000)
    {
        if (errorlog) fprintf (errorlog, "%s: file to small\n", name);
	osd_fclose (F);
	return 1;
    }
    /* get mapper type */
    type = get_mapper_type (osd_fcrc(F));
    /* mapper type 0 always needs 64kB */
    if (!type)
    {
	size_aligned = 0x10000;
	if (size > 0x10000) size = 0x10000;
    }
    else
    {
        /* calculate aligned size (8, 16, 32, 64, 128, 256, etc. (kB) ) */
        size_aligned = 0x2000;
    	while (size_aligned < size) size_aligned *= 2;
    }
    pmem = (UINT8*)malloc (size_aligned);
    if (!pmem)
    {
	if (errorlog) fprintf (errorlog, "malloc () failed\n");
	osd_fclose (F);
	return 1;
    }
    memset (pmem, 0xff, size_aligned);
    if (osd_fread (F, pmem, size) != size)
    {
        if (errorlog) fprintf (errorlog, "%s: can't read file\n", name);
	osd_fclose (F);
        free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
	return 1;
    }
    osd_fclose (F);
    /* set mapper specific stuff */
    msx1.cart[id].mem = pmem;
    msx1.cart[id].type = type;
    msx1.cart[id].bank_mask = (size_aligned / 0x2000) - 1;
    for (i=0;i<4;i++) msx1.cart[id].banks[i] = (i & msx1.cart[id].bank_mask);
    if (errorlog) fprintf (errorlog, "Cart #%d size %d, mask %d, type: %s\n",
        id, size, msx1.cart[id].bank_mask, mapper_types[type]);
    /* set filename for sram (memcard) */
    msx1.cart[id].sramfile = malloc (strlen (name) + 1);
/*    msx1.cart[id].mem[0] = 0; */
    if (!msx1.cart[id].sramfile)
    {
	if (errorlog) fprintf (errorlog, "malloc () failed\n");
        free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
	return 1;
    }
    strcpy (msx1.cart[id].sramfile, name);
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
	if (errorlog)
	{
	    if (p >= 0)
		fprintf (errorlog, "Cart #%d in page %d\n", id, p);
	    else
		fprintf (errorlog, "Cart #%d memory duplicated in all pages\n", id);
	}
	break;
   case 6: /* game master 2; try to load sram */
        pmem = realloc (msx1.cart[id].mem, 0x24000);
        if (!pmem)
	{
	    free (msx1.cart[id].mem); msx1.cart[id].mem = NULL;
	    return 1;
	}
	F = osd_fopen (Machine->gamedrv->name, msx1.cart[id].sramfile,
		OSD_FILETYPE_MEMCARD, 0);
	if (F && (osd_fread (F, pmem + 0x21000, 0x2000) == 0x2000) )
	{
	    memcpy (pmem + 0x20000, pmem + 0x21000, 0x1000);
	    memcpy (pmem + 0x23000, pmem + 0x22000, 0x1000);
	    if (errorlog) fprintf (errorlog, "Cart #%d SRAM loaded\n", id);
	} else {
	    memset (pmem + 0x20000, 0, 0x4000);
	    if (errorlog) fprintf (errorlog, "Cart #%d Failed to load SRAM\n", id);
	}
	if (F) osd_fclose (F);

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
	    if (errorlog) fprintf (errorlog, "Cart #%d SRAM loaded\n", id);
	} else {
	    memset (pmem + size_aligned, 0, 0x2000);
	    if (errorlog) fprintf (errorlog, "Cart #%d Failed to load SRAM\n", id);
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
	    if (errorlog) fprintf (errorlog, "Cart #%d SRAM loaded\n", id);
	} else {
	    memset (pmem + size_aligned, 0, 0x4000);
	    if (errorlog) fprintf (errorlog, "Cart #%d Failed to load SRAM\n", id);
	}
	if (F) osd_fclose (F);

	msx1.cart[id].mem = pmem;
	break;
    }
    return 0;
}

static void save_sram (int id, char *filename, UINT8* pmem, int size)
{
    void *F;

    F = osd_fopen (Machine->gamedrv->name, filename, OSD_FILETYPE_MEMCARD, 1);
    if (F && (osd_fwrite (F, pmem, size) == size) )
    {
	if (errorlog) fprintf (errorlog, "Cart %d# SRAM saved\n", id);
    } else {
	if (errorlog) fprintf (errorlog, "Cart %d# failed to save SRAM\n", id);
    }
    if (F) osd_fclose (F);
}

void msx_exit_rom (int id)
{
    if (msx1.cart[id].mem)
    {
	/* save sram thingies */
	switch (msx1.cart[id].type) {
	case 6:
	    save_sram (id, msx1.cart[id].sramfile,
		msx1.cart[id].mem + 0x21000, 0x2000);
	    break;
        case 7:
	    save_sram (id, msx1.cart[id].sramfile,
		msx1.cart[id].mem + (msx1.cart[id].bank_mask + 1) * 0x2000,
		0x2000);
	    break;
        case 8:
	    save_sram (id, msx1.cart[id].sramfile,
		msx1.cart[id].mem + (msx1.cart[id].bank_mask + 1) * 0x2000,
		0x800);
	    break;
	}
        free (msx1.cart[id].mem);
        free (msx1.cart[id].sramfile);
    }
}

static void msx_vdp_interrupt(int i) {
    cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

void msx_ch_reset(void) {
    int i/*,n*/;

    /* reset memory */
    ppi8255_0_w (0, 0);
    ppi8255_0_w (2, 0xff);
    /* reset leds */
    for (i=0;i<3;i++) osd_led_w (i, 0);
}

void init_msx (void) {
    FILE *F;
    /* set interrupt stuff */
    TMS9928A_int_callback(msx_vdp_interrupt);
    cpu_irq_line_vector_w(0,0,0xff);

    /* setup PPI */
    ppi8255_init (&msx_ppi8255_interface);

    /* initialize mem regions */
    msx1.empty = (UINT8*)malloc (0x4000);
    msx1.ram = (UINT8*)malloc (0x10000);
    if (!msx1.ram || !msx1.empty)
    {
	if (errorlog) fprintf (errorlog, "malloc () in init_msx () failed!\n");
	return;
    }

    memset (msx1.empty, 0xff, 0x4000);
    memset (msx1.ram, 0, 0x10000);

    F = fopen ("/home/sean/msx/fmsx-2.1/monmsx.bin", "r");
    fread (msx1.ram + 0xc001 - 7, 1, 0x1800, F);
    fclose (F);
    return;
}

void msx_ch_stop (void) {
     free (msx1.empty);
     free (msx1.ram);
}

/*
** The I/O funtions
*/

int msx_vdp_r(int offset)
{
    if (offset & 0x01)
	return TMS9928A_register_r();
    else
	return TMS9928A_vram_r();
}

void msx_vdp_w(int offset, int data)
{
    if (offset & 0x01)
	TMS9928A_register_w(data);
    else
	TMS9928A_vram_w(data);
}

int msx_psg_r (int offset)
{
    return AY8910_read_port_0_r (offset);
}

void msx_psg_w (int offset, int data)
{
    if (offset & 0x01)
	AY8910_write_port_0_w (offset, data);
    else
	AY8910_control_port_0_w (offset, data);
}

int msx_psg_port_a_r (int offset)
{
    if (msx1.psg_b & 0x40)
	return input_port_10_r (0);
    else
	return input_port_9_r (0);
}

int msx_psg_port_b_r (int offset) {
    return msx1.psg_b;
}

void msx_psg_port_a_w (int offset, int data) { }

void msx_psg_port_b_w (int offset, int data) {
    /* Arabic or kana mode led */
    if ( (data ^ msx1.psg_b) & 0x80) osd_led_w (1, !(data & 0x80) );
	msx1.psg_b = data;
}

void msx_printer_w (int offset, int data)
{
     if (offset == 1) {
        /* SIMPL emulation */
        DAC_signed_data_w (0, data);
     }
}

int msx_printer_r (int offset) {
    return 0xff;
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
        osd_led_w (0, !(data & 0x40) );
    /* key click */
    if ( (old_val ^ data) & 0x80)
        DAC_signed_data_w (0, (data & 0x80 ? 0x7f : 0));
    /* tape emulation */
    if ( !(data & 0x10) )
    {
	DAC_signed_data_w (0, (data & 0x20 ? 0x7f : 0));
    }

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
    if (page < 2)
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

void msx_writemem0 (int offset, int data)
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
	/* still till to be done */
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
	if (offset >= 0x1000 && offset < 0x2000)
	{
	    if (data & 0x10)
	    {
		n = (( (~data & 0x07) | 0x10) * 2) & msx1.cart[cart].bank_mask;
	    } else {
		n = ((~data & 0x0f) * 2) & msx1.cart[cart].bank_mask;
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
    case 0x80: /* Konami Sound Cartridge (SCC+) */
	/* mode register */
        if (offset >= 0x7ffe)
	{
	    msx1.cart[cart].bank_mask = data;
	    /* check for changes in SCC/SCC+ pages */
	} else {
	    int bank,rammode;
	    bank = (offset / 0x2000);
	    switch (bank) {
	    case 0:
	 	rammode = msx1.cart[cart].bank_mask & 0x11;
		break;
	    case 1:
		rammode = msx1.cart[cart].bank_mask & 0x12;
	        break;
	    case 2:
		rammode = !(msx1.cart[cart].bank_mask & 0x20) &&
			msx1.cart[cart].bank_mask & 0x14;
		break;
	    case 3:
		rammode = msx1.cart[cart].bank_mask & 0x10;
	    }
	    if (rammode)  ;
	}
    }
}

void msx_writemem1 (int offset, int data)
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

void msx_writemem2 (int offset, int data)
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

void msx_writemem3 (int offset, int data)
{
    if ( (ppi8255_0_r(0) & 0xc0) == 0xc0)
	msx1.ram[0xc000+offset] = data;
}

