/***************************************************************************

  msx.c

  Machine file to handle emulation of the MSX1.

  TODO:
	- Clean up the code
***************************************************************************/

#include "driver.h"
#include "msx.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/tms9928a.h"
#include "mess/sndhrdw/scc.h"

MSX msx1;
static void msx_set_all_mem_banks (void);

int msx_id_rom (const char *name, const char *gamename)
{
    FILE *F;
    unsigned char magic[2];

    if (!name[0]) return 1;

    /* read the first two bytes */
    F = osd_fopen (name, gamename, OSD_FILETYPE_IMAGE_R, 0);
    if (!F) return 0;
    osd_fread (F, magic, 2);
    osd_fclose (F);

    /* first to bytes must be 'AB' */
    return ( (magic[0] == 'A') && (magic[1] == 'B') );
}

int msx_load_rom (int id, const char *name)
{
    FILE *F;
	UINT8 *pmem,*m;
	int size,size_aligned,n,p;

    msx1.empty = (UINT8*)malloc (0x4000);
    msx1.ram = (UINT8*)malloc (0x10000);
    if (!msx1.ram || !msx1.empty) goto fail;

    /* initialize mem regions */
    memset (msx1.empty, 0xff, 0x4000);
    memset (msx1.ram, 0, 0x10000);

	/* load cartridge */
	pmem = (UINT8*)malloc (MSX_MAX_ROMSIZE);
	if (!pmem) goto fail;
	msx1.cart[id].mem = pmem;
	memset (pmem, 0xff, MSX_MAX_ROMSIZE);
	msx1.cart[id].type = 0;
	if( name && name[0] )
	{
		F = osd_fopen (Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
	    if (!F)
	    {
			if (errorlog) fprintf (errorlog, "Can't open ROM image: %s", name);
			goto fail;
	    }
	    size = osd_fread (F, pmem, MSX_MAX_ROMSIZE);
	    size_aligned = 0x2000;
	    while (size_aligned < size) size_aligned *= 2;
	    if (size_aligned <= 0x10000) /* quick hack for ROM type */
	    {
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
		} else if (size <= 0xc000)
		{
		    if (p)
		    {
			/* shift up 16kB; custom memcpy so overlapping memory
			   isn't fucked up. ROM starts in page 1 (0x4000) */
			p = 1;
			n = 0xc000; m = pmem + 0xffff;
			while (n--) { *m = *(m - 0x4000); m--; }
			memset (pmem, 0xff, 0x4000);
		    }
		}
		if (errorlog) {
		    if (p < 0)
				fprintf (errorlog, "Cartridge %d: %s, type 0, all pages\n", id, name);
		    else
				fprintf (errorlog, "Cartridge %d: %s, type 0, page %d\n", id, name, p);
		}
	    } else {
		if (msx1.cart[id].mem[0x10] == 'Y' &&
			msx1.cart[id].mem[0x11] == 'Z')
			msx1.cart[id].type = 2;
	 	else
			msx1.cart[id].type = 1;
		msx1.cart[id].bank_mask = (size_aligned / 0x2000) - 1;
                if (errorlog) fprintf (errorlog,
		    "Cartridge %d: %s type %d, size %x, mask %x\n",
			id, name, msx1.cart[id].type, size_aligned,
			msx1.cart[id].bank_mask);
	    }
 	}
    return 0;

fail:
    if (msx1.empty) free (msx1.empty);
    if (msx1.ram) free (msx1.ram);
	if (msx1.cart[id].mem)
		free (msx1.cart[id].mem);
    return 1;
}

static void msx_vdp_interrupt(int i) {
    cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

void msx_ch_reset(void) {
    int i,n;
    /* reset memory */
    for (i=0;i<4;i++) for (n=0;n<MSX_MAX_CARTS;n++) msx1.cart[n].banks[i] = i;
    msx1.ppi_a = 0;
    msx_set_all_mem_banks ();
    /* reset leds */
    for (i=0;i<3;i++) osd_led_w (i, 0);
}

void init_msx (void) {
    TMS9928A_int_callback(msx_vdp_interrupt);
    msx_ch_reset ();
}


void msx_ch_stop (void) {
     free (msx1.empty);
     free (msx1.ram);
}

/*
** The I/O funtions
*/
void msx_ppi_w (int offset, int data)
{
    int s,i;
	switch (offset)
	{
    case 0: /* 0xa8 */
		msx1.ppi_a = data;
		msx_set_all_mem_banks ();
		break;
    case 3: /* 0xab */
		if (data & 0x80)
		{
			if (errorlog) fprintf (errorlog,
				"Don't know how to handle PPI command %02x\n", data);
			break;
		}
        s = (data / 2) & 7;
        i = msx1.ppi_c & ~(1 << s);
        i |= (data & 1) << s;
        data = i;
    case 2: /* 0xaa */
        if ( (msx1.ppi_c ^ data) & 0x40)
            osd_led_w (0, !(data & 0x40) );
		msx1.ppi_c = data;
		break;
    }
}

int msx_ppi_r (int offset)
{
    int i;
	switch (offset)
	{
    case 0: /* 0xa8 */
		return msx1.ppi_a;
    case 2: /* 0xaa */
		return msx1.ppi_c;
    case 1: /* 0xa9 */
		i = msx1.ppi_c & 0xf;
		if (i <= 8) return readinputport (i);
    default:
		return 0xff;
    }
}

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
	}
	else
	{
		cpu_setbank (1 + page * 2, msx1.empty);
		cpu_setbank (2 + page * 2, msx1.empty);
    }
}

static void msx_set_slot_1 (int page) {
    int i,n;
	if (msx1.cart[0].type == 0)
	{
        cpu_setbank (1 + page * 2, msx1.cart[0].mem + page * 0x4000);
        cpu_setbank (2 + page * 2, msx1.cart[0].mem + page * 0x4000 + 0x2000);
	}
	else
	{
		if (page == 0 || page == 3)
		{
            cpu_setbank (1 + page * 2, msx1.empty);
            cpu_setbank (2 + page * 2, msx1.empty);
			return;
		}
		n = (page - 1) * 2;
		for (i=n;i<(n+2);i++)
		{
			cpu_setbank (3 + i,
				msx1.cart[0].mem + msx1.cart[0].banks[i] * 0x2000);
		}
    }
}

static void msx_set_slot_2 (int page)
{
    int i,n;
	if (msx1.cart[1].type == 0)
	{
        cpu_setbank (1 + page * 2, msx1.cart[1].mem + page * 0x4000);
        cpu_setbank (2 + page * 2, msx1.cart[1].mem + page * 0x4000 + 0x2000);
	}
	else
	{
		if (page == 0 || page == 3)
		{
            cpu_setbank (1 + page * 2, msx1.empty);
            cpu_setbank (2 + page * 2, msx1.empty);
			return;
		}
		n = (page - 1) * 2;
		for (i=n;i<(n+2);i++)
		{
			cpu_setbank (3 + i,
			msx1.cart[1].mem + msx1.cart[1].banks[i] * 0x2000);
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
	{
		msx_set_slot[(msx1.ppi_a>>(i*2))&3](i);
    }
}

void msx_writemem0 (int offset, int data)
{
	if ( (msx1.ppi_a & 0x03) == 0x03 )
		msx1.ram[offset] = data;
}

static void msx_cart_write (int cart, int offset, int data)
{
    int n;

	switch (msx1.cart[cart].type)
	{
    case 0:
		break;
    case 1:
		if (offset && !(offset & 0x0fff) )
		{
			if (offset == 0x5000)
				msx1.cart[cart].scc_active = !(~data & 0x3f);
			n = data & msx1.cart[cart].bank_mask;
			msx1.cart[cart].banks[(offset/0x2000)] = n;
			cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem + n * 0x2000);
		}
		else
		if (msx1.cart[cart].scc_active &&
			(offset >= 0x5800) && (offset < 0x6000) )
		{
			SCCWriteReg (0, offset & 0xff, data, SCC_MEGAROM);
		}
		break;
    case 2: /* Game Master 2 */
		if (!(offset & 0x1000) && (offset >= 0x2000) )
		{
			n = ((data & 0x10) ? ((data & 0x30) ? 0x11:0x10) : (data & 0x0f));
			msx1.cart[cart].banks[(offset/0x2000)] = n;
			cpu_setbank (3+(offset/0x2000),msx1.cart[cart].mem + n * 0x2000);
		}
		else
		if (offset >= 0x7000)
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
    }
}

void msx_writemem1 (int offset, int data)
{
	switch (msx1.ppi_a & 0x0c)
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
	switch (msx1.ppi_a & 0x30)
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
	if ( (msx1.ppi_a & 0xc0) == 0xc0)
		msx1.ram[0xc000+offset] = data;
}

