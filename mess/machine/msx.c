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
#include "includes/msx_slot.h"
#include "includes/msx.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "includes/tc8521.h"
#include "includes/wd179x.h"
#include "devices/basicdsk.h"
#include "vidhrdw/tms9928a.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/v9938.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "utils.h"
#include "image.h"
#include "osdepend.h"

#ifndef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x) )
#endif

MSX msx1;
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

DEVICE_LOAD (msx_cart)
{
	int size;
	int size_aligned;
	UINT8 *mem;
	int type;
	const char *extra;
	slot_state *state;
	int id;

	size = mame_fsize (file);
	if (size < 0x2000) {
		logerror ("msx_cart: error: file is smaller than 2kb, too small "
				  "to be true!\n");
		return INIT_FAIL;
	}

	/* allocate memory and load */
	size_aligned = 0x2000;
	while (size_aligned < size) {
		size_aligned *= 2;
	}
	mem = image_malloc (image, size_aligned);
	if (!mem) {
		logerror ("msx_cart: error: failed to allocate memory for cartridge\n");
		return INIT_FAIL;
	}
	if (size < size_aligned) {
		memset (mem, 0xff, size_aligned);
	}
	if (mame_fread (file, mem, size) != size) {
		logerror ("%s: can't read full %d bytes\n", 
						image_filename (image), size);
		return INIT_FAIL;
	}

	/* see if msx.crc will tell us more */
	extra = image_extrainfo (image);
	if (!extra) {
		logerror("msx_cart: warning: no information in crc file\n");
		type = -1;
	}
	else if ((1 != sscanf (extra, "%d", &type) ) || type < 0 || type > 17) {
		logerror("msx_cart: warning: information in crc file not valid\n");
		type = -1;
	}
	else {
		logerror ("msx_cart: info: cart extra info: '%s' = %s", extra,
						msx_slot_list[type].name);
	}

	/* if not, attempt autodetection */
	if (type < 0) {
		type = msx_probe_type (mem, size);

		if (mem[0] != 'A' || mem[1] != 'B') {
			logerror("%s: May not be a valid ROM file\n",
							image_filename (image));
		}

		logerror("Probed cartridge mapper %d/%s\n", 
						type, msx_slot_list[type].name);
	}

	/* mapper type 0 always needs 64kB */
	if (!type && size_aligned != 0x10000)
	{
		size_aligned = 0x10000;
		mem = image_realloc(image, mem, 0x10000);
		if (!mem) {
			logerror ("msx_cart: error: cannot allocate memory\n");
			return INIT_FAIL;
		}
		
		if (size < 0x10000) {
			memset (mem + size, 0xff, 0x10000 - size);
		}
		if (size > 0x10000) {
			logerror ("msx_cart: warning: rom truncated to 64kb due to "
					  "mapperless type (possibly detected)\n");

			size = 0x10000;
		}
	}

	/* mapper type 0 (ROM) might need moving around a bit */
	if (!type) {
		int i, page = 1;
		
		/* find the correct page */
		if (mem[0] == 'A' && mem[1] == 'B') {
			for (i=2; i<=8; i += 2) {
				if (mem[i] || mem[i+1]) {
					page = mem[i+1] / 0x40;
					break;
				}
			}
		}

		if (size <= 0x4000) {
			if (page == 1 || page == 2) {
				/* copy to the respective page */
				memcpy (mem + (page * 0x4000), mem, 0x4000);
				memset (mem, 0xff, 0x4000);
			} 
			else {
				/* memory is repeated 4 times */
				page = -1;
				memcpy (mem + 0x4000, mem, 0x4000);
				memcpy (mem + 0x8000, mem, 0x4000);
				memcpy (mem + 0xc000, mem, 0x4000);
			}
		}
		else /*if (size <= 0xc000) */ {
			if (page) {
				/* shift up 16kB; custom memcpy so overlapping memory
				   isn't corrupted. ROM starts in page 1 (0x4000) */
				UINT8 *m;

				page = 1;
				i = 0xc000; m = mem + 0xffff;
				while (i--) { 
					*m = *(m - 0x4000); m--; 
				}
				memset (mem, 0xff, 0x4000);
			}
		}

		if (page) {
			logerror ("msx_cart: info: rom in page %d\n", page);
		}
		else {
			logerror ("msx_cart: info: rom duplicted in all pages\n");
		}
	}

	/* allocate and set slot_state for this cartridge */
	state = (slot_state*)auto_malloc (sizeof (slot_state));
	if (!state) {
		logerror ("msx_cart: error: cannot allocate memory for "
				  "cartridge state\n");
	}
	memset (state, 0, sizeof (slot_state));

	state->type = type;
	state->sramfile = image_malloc (image, strlen (image_filename (image) + 1));
	if (state->sramfile) {
		char *ext;

		strcpy (state->sramfile, image_basename (image));
		ext = strrchr (state->sramfile, '.');
		if (ext) {
			*ext = 0;
		}
	}

	msx_slot_list[type].init (state, 0, mem, size_aligned);
	if (msx_slot_list[type].loadsram) {
		msx_slot_list[type].loadsram (state);
	}

	id = image_index_in_device (image);
	msx1.cart_state[id] = state;

	return INIT_PASS;
}

DEVICE_UNLOAD (msx_cart)
{
	int id;
	
	id = image_index_in_device (image);
	if (msx_slot_list[msx1.cart_state[id]->type].savesram) {
		msx_slot_list[msx1.cart_state[id]->type].savesram (msx1.cart_state[id]);
	}
}

void msx_vdp_interrupt(int i)
{
	cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

static void msx_ch_reset_core (void)
{
	/* set interrupt stuff */
	cpu_irq_line_vector_w(0,0,0xff);
	/* setup PPI */
	ppi8255_init (&msx_ppi8255_interface);

	msx_memory_reset ();
	msx_memory_map_all ();

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
static struct {
	int table;
	const void *old_table;
} z80_cycle_table[] = {
	{ Z80_TABLE_op, NULL },
	{ Z80_TABLE_cb, NULL },
	{ Z80_TABLE_xy, NULL },
	{ Z80_TABLE_ed, NULL },
	{ Z80_TABLE_xycb, NULL },
	{ Z80_TABLE_ex, NULL },
	{ -1, NULL }
};

static void msx_wd179x_int (int state);

DRIVER_INIT( msx )
{
	int i,n;

	wd179x_init (WD_TYPE_179X,msx_wd179x_int);
	wd179x_set_density (DEN_FM_HI);
	msx1.dsk_stat = 0x7f;

	msx_memory_init ();

	/* adjust z80 cycles for the M1 wait state */
	for (i=0; z80_cycle_table[i].table != -1; i++) {
		UINT8 *table = auto_malloc (0x100);

		z80_cycle_table[i].old_table = 
				z80_get_cycle_table (z80_cycle_table[i].table);
		memcpy (table, z80_cycle_table[i].old_table, 0x100);

		if (z80_cycle_table[i].table == Z80_TABLE_ex) {
			table[0x66]++; /* NMI overhead (not used) */
			table[0xff]++; /* INT overhead */
		}
		else {
			for (n=0; n<256; n++) {
				if (z80_cycle_table[i].table == Z80_TABLE_op) {
					table[n]++;
				}
				else {
					table[n] += 2;
				}
			}
		}
		z80_set_cycle_table (z80_cycle_table[i].table, (void*)table);
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

	for (i=0; z80_cycle_table[i].table != -1; i++) {
		z80_set_cycle_table (z80_cycle_table[i].table, 
						(void*)z80_cycle_table[i].old_table);
	}
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

7FF8H read	status register
	  write command register
7FF9H  r/w	track register (r/o on NMS 8245 and Sony)
7FFAH  r/w	sector register (r/o on NMS 8245 and Sony)
7FFBH  r/w	data register


hardware registers

adress

7FFCH r/w  bit 0 side select
7FFDH r/w  b7>M-on , b6>in-use , b1>ds1 , b0>ds0  (all neg. logic)
7FFEH		  not used
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
	msx1.primary_slot = ppi8255_peek (0,0);
	logerror ("write to primary slot select: %02x\n", msx1.primary_slot);
	msx_memory_map_all ();
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
		cassette_change_state(cassette_device_image(), 
						(data & 0x10) ? CASSETTE_MOTOR_DISABLED : 
										CASSETTE_MOTOR_ENABLED, 
						CASSETTE_MASK_MOTOR);

	/* cassette signal write */
	if ( (old_val ^ data) & 0x20)
		cassette_output(cassette_device_image(), (data & 0x20) ? -1.0 : 1.0);

	old_val = data;
}

static READ_HANDLER( msx_ppi_port_b_r )
{
	int row, data;

	row = ppi8255_0_r (2) & 0x0f;
	if (row <= 10) {
		data = readinputport (row/2);
		if (row & 1) data >>= 8;
		return data & 0xff;
	}
	else {
		return 0xff;
	}
}

/************************************************************************
 *
 * New memory emulation !!
 *
 ***********************************************************************/

void msx_memory_init (void)
{
	int	prim, sec, page, extent;
	int size = 0;
	const msx_slot_layout *layout= (msx_slot_layout*)NULL;
	const msx_slot *slot;
	const msx_driver_struct *driver;
	slot_state *st;
	UINT8 *mem = NULL;

	msx1.empty = (UINT8*)auto_malloc (0x4000);
	if (!msx1.empty) {
		logerror ("msx_memory_init: error: cannot allocate empty page\n");
		return;
	}
	memset (msx1.empty, 0xff, 0x4000);

	for (prim=0; prim<4; prim++) {
		for (sec=0; sec<4; sec++) {
			for (page=0; page<4; page++) {
				msx1.all_state[prim][sec][page]= (slot_state*)NULL;
			}
		}
	}

	for (driver = msx_driver_list; driver->name[0]; driver++) {
		if (!strcmp (driver->name, Machine->gamedrv->name)) {
			layout = driver->layout;
		}
	}

	if (!layout) {
		logerror ("msx_memory_init: error: missing layout definition in "
				  "msx_driver_list\n");
		return;
	}

	for (; layout->type != SLOT_END; layout++) {
		
		prim = layout->slot_primary;
		sec = layout->slot_secondary;
		page = layout->slot_page;
		extent = layout->page_extent;

		if (layout->slot_secondary) {
			msx1.slot_expanded[layout->slot_primary]= TRUE;
		}

		slot = &msx_slot_list[layout->type];
		if (slot->slot_type != layout->type) {
			logerror ("internal error: msx_slot_list[%d].type != %d\n",
							slot->slot_type, slot->slot_type);
		}

		size = layout->size;

		logerror ("slot %d/%d/%d-%d: type %s, size 0x%x\n",
				prim, sec, page, page + extent - 1, slot->name, size);

		st = (slot_state*)NULL;
		if (layout->type == SLOT_CARTRIDGE1) {
			st = msx1.cart_state[0];
			if (!st) {
				slot = &msx_slot_list[SLOT_SOUNDCARTRIDGE];
				size = 0x10000;
			}
		}
		if (layout->type == SLOT_CARTRIDGE2) {
			st = msx1.cart_state[1];
			if (!st) {
				slot = &msx_slot_list[SLOT_FMPAC];
				mem = memory_region(REGION_CPU1) + 0x10000;
				size = 0x10000;
			}
		}

		if (!st) {
			switch (slot->mem_type) {

			case MSX_MEM_HANDLER:
			case MSX_MEM_ROM:
				mem = memory_region(REGION_CPU1) + layout->option;
				break;
			case MSX_MEM_RAM:
				mem = NULL;
				break;
			}
			st = (slot_state*)auto_malloc (sizeof (slot_state));
			if (!st) {
				logerror ("fatal error: cannot malloc %d\n", 
								sizeof (slot_state));
				continue;
			}
			memset (st, 0, sizeof (slot_state));

			if (slot->init (st, layout->slot_page, mem, size)) {
				continue;
			}
		}

		while (extent--) {
			if (page > 3) {
				logerror ("internal error: msx_slot_layout wrong, "
						 "page + extent > 3\n");
				break;
			}
			msx1.all_state[prim][sec][page] = st;
			page++;
		}
	}
}

void msx_memory_reset (void)
{
	slot_state *state, *last_state= (slot_state*)NULL;
	int prim, sec, page;

	msx1.primary_slot = 0;

	for (prim=0; prim<4; prim++) {
		msx1.secondary_slot[prim] = 0;
		for (sec=0; sec<4; sec++) {
			for (page=0; page<4; page++) {
				state = msx1.all_state[prim][sec][page];
				if (state) {
					if (state != last_state) {
						msx_slot_list[state->type].reset (state);
					}
				}
				last_state = state;
			}
		}
	}
}

void msx_memory_map_page (int page)
{
	int slot_primary;
	int slot_secondary;
	slot_state *state;
	const msx_slot *slot;

	switch (page) {
	case 1:
		memory_set_bankhandler_r (3, 0, MRA_BANK3);
		memory_set_bankhandler_r (4, 0, MRA_BANK4);
		break;
	case 2:
		memory_set_bankhandler_r (5, 0, MRA_BANK5);
		memory_set_bankhandler_r (6, 0, MRA_BANK6);
		break;
	}

	slot_primary = (msx1.primary_slot >> (page * 2)) & 3;
	slot_secondary = (msx1.secondary_slot[slot_primary] >> (page * 2)) & 3;

	state = msx1.all_state[slot_primary][slot_secondary][page];
	slot = state ? &msx_slot_list[state->type] : &msx_slot_list[SLOT_EMPTY];
	msx1.state[page] = state;
	msx1.slot[page] = slot;
	logerror ("mapping %s in %d/%d/%d\n", slot->name, slot_primary, 
			slot_secondary, page);
	slot->map (state, page);
}

void msx_memory_map_all (void)
{
	int i;

	for (i=0; i<4; i++) {
		msx_memory_map_page (i);
	}
}

WRITE_HANDLER (msx_superloadrunner_w)
{
	msx1.superloadrunner_bank = data;
	if (msx1.slot[2]->slot_type == SLOT_SUPERLOADRUNNER) {
		msx1.slot[2]->map (msx1.state[2], 2);
	}
	msx_page0_w (0, data);
}

WRITE_HANDLER (msx_page0_w)
{
	switch (msx1.slot[0]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[0][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[0]->write (msx1.state[0], offset, data);
	}
}

WRITE_HANDLER (msx_page1_w)
{
	switch (msx1.slot[1]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[1][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[1]->write (msx1.state[1], 0x4000 + offset, data);
	}
}

WRITE_HANDLER (msx_page2_w)
{
	switch (msx1.slot[2]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[2][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[2]->write (msx1.state[2], 0x8000 + offset, data);
	}
}

WRITE_HANDLER (msx_page3_w)
{
	switch (msx1.slot[3]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[3][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[3]->write (msx1.state[3], 0xc000 + offset, data);
	}
}

WRITE_HANDLER (msx_sec_slot_w)
{
	int slot = msx1.primary_slot >> 6;
	if (msx1.slot_expanded[slot]) {
		logerror ("write to secondary slot %d select: %02x\n", slot, data);
		msx1.secondary_slot[slot] = data;
		msx_memory_map_all ();
	}
	else {
		msx_page3_w (0x3fff, data);
	}
}

READ_HANDLER (msx_sec_slot_r)
{
	int slot = msx1.primary_slot >> 6;
	if (msx1.slot_expanded[slot]) {
		return ~msx1.secondary_slot[slot];
	}
	else {
		return 0xff; /* FIXME!! */
	}
}

WRITE_HANDLER (msx_ram_mapper_w)
{
	msx1.ram_mapper[offset] = data;
	if (msx1.slot[offset]->slot_type == SLOT_RAM_MM) {
		msx1.slot[offset]->map (msx1.state[offset], offset);
	}
}

READ_HANDLER (msx_ram_mapper_r)
{
	return msx1.ram_mapper[offset];
}
